function [id, elements, dataArray] = readDataSTM32(s)
    persistent accBuffer
    if isempty(accBuffer)
        accBuffer = uint8([]);
    end
    
    id = 0;
    elements = 0;
    dataArray = [];
    
    try
        newData = read(s, s.NumBytesAvailable, 'uint8');
        accBuffer = [accBuffer; newData(:)];
        
        while true
            % Hledej startovní sekvenci
            startIdx = find(accBuffer(1:end-1) == 0xCD & accBuffer(2:end) == 0xAB, 1);
            if isempty(startIdx)
                break;
            end
            
            currentBuffer = accBuffer(startIdx:end);
            
            % Minimální délka hlavičky
            if length(currentBuffer) < 5
                break;
            end
            
            % Čtení ID a elements s ochranou
            id = currentBuffer(3);
            elements = typecast(currentBuffer(4:5), 'uint16');
            elements = min(elements, 1000); % Omezení na 1000 elementů
            
            % Zpracování paketu
            [totalLength, dataArray] = processPacketFast(currentBuffer, elements);
            
            % Kontrola platnosti totalLength
            if totalLength > length(currentBuffer)
                error("Neplatný totalLength: %d > %d", totalLength, length(currentBuffer));
            end
            
            accBuffer = currentBuffer((totalLength + 1):end);
            return;
        end
    catch ME
        accBuffer = uint8([]);
        rethrow(ME);
    end
end

function [totalLength, dataArray] = processPacketFast(buffer, elements)
    totalLength = 5;
    dataArray = struct('data_id', {}, 'data', {});
    pointer = 6;
    
    for i = 1:elements
        % Kontrola dostupnosti hlavičky
        if pointer + 2 > length(buffer)
            error("Neplatná hlavička (element %d)", i);
        end
        
        data_id = buffer(pointer);
        data_size = double(buffer(pointer + 1)); % ← Konvertuj na double
        data_count = double(buffer(pointer + 2)); % ← Konvertuj na double
        
        % Kritická validace data_count
        if data_count == 0
            error("Neplatný data_count: 0 (element %d)", i);
        end
        
        % Omezení data_count na maximálně 65535
        data_count = min(data_count, 65535);
        
        % Výpočet pozic
        dataStart = pointer + 3;
        dataEnd = dataStart + data_size * data_count - 1;
        
        if dataEnd > length(buffer)
            error("Poškozená data (element %d)", i);
        end
        
        % Konverze dat
        data = typecast(buffer(dataStart:dataEnd), getDataType(data_size));
        dataArray(end+1) = struct('data_id', data_id, 'data', data);
        
        pointer = dataEnd + 1;
        totalLength = totalLength + 3 + data_size * data_count;
    end
end

function dtype = getDataType(size)
    switch size
        case 1, dtype = 'uint8';
        case 2, dtype = 'uint16';
        case 4, dtype = 'uint32';
        otherwise, error("Nepodporovaný typ");
    end
end
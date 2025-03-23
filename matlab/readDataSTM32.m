function [id, elements, dataBuffer] = readDataSTM32(s)
    id = 0;
    elements = 0;
    dataBuffer = struct();

    data_type = "uint32"; % default
    
    try
        % reads whole line up to terminator
        rawData = readline(s);
        buffer = uint8(char(rawData)); % typecast to uint8
        
        % start bytes
        if length(buffer) < 5 || buffer(1) ~= 0xCD || buffer(2) ~= 0xAB
            error("Invalid start sequence");
        end
        
        % read the buffer header
        id = buffer(3);
        elements = typecast(buffer(4:5), 'uint16');
        
        if elements == 0
            error("No data packets");
        end

        
        pointer = 6; % position of first data head
        for i = 1:elements
            
            if pointer + 2 > length(buffer)
                error("Poškozený paket");
            end

            data = [];
            
            % data head
            data_id = buffer(pointer);
            data_size = buffer(pointer + 1);
            data_count = buffer(pointer + 2);
            
            switch data_size
                case 1
                    data_type = "uint8";
                case 2
                    data_type = "uint16";
                case 4
                    data_type = "uint32";
                otherwise
                    error("Unsupported data type: %d bytes", data_size);
            end

            for y = 1:data_count
                data_start = pointer + 3 + ((y-1) * data_size);
                data_end = data_start + data_size - 1;

                value = typecast(buffer(data_start:data_end), data_type);
                data(end+1) = value;
            end

            % APPEND TO dataBuffer
            dataBuffer(i).data_id = data_id;
            dataBuffer(i).data = data;
            pointer = data_end + 1;
        end
        
    catch ME
        flush(s); % Vyčisti buffer při chybě
        rethrow(ME);
    end
end

function statusIsReady = isReady(s, nX)

    for n = 0:nX
        pause(0.01);
        if(s.NumBytesAvailable>=(nX*4))
            statusIsReady = 1;
            return;
        end
    end

    statusIsReady = 0;

end

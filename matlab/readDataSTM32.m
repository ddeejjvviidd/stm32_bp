function [id, elements, dataBuffer] = readDataSTM32(s)
    try
        
        % buffer head
        id=0; 
        elements=0; 
        
        % elements
        data_id = 0;
        data_size = 0; % in bytes
        data_type = "uint32"; % handing this over to read func
        data_count = 0;

        dataBuffer = struct();
        
        %flushinput(s); % for some reason I need to flush the buffer before reading
        flush(s, "input")
        s.ByteOrder = 'little-endian';

        if(s.NumBytesAvailable > 3)
            %fprintf("Available bytes: %d\n", s.NumBytesAvailable);
            
            % read the buffer head
            id = read(s, 1, "uint8");
            elements = read(s, 1, "uint16");
            
            % read data
            if elements==0
                return;
            end

            for i = 1:elements

                data = [];

                % LOAD DATA HEAD
                % load the data id
                data_id = read(s, 1, "uint8");
                % load the byte size
                data_size = read(s, 1, "uint8");
                % load the number of data
                data_count = read(s, 1, "uint8");
                
                switch data_size
                    case 1
                        data_type = "uint8";
                    case 2
                        data_type = "uint16";
                    case 4
                        data_type = "uint32";
                    otherwise
                        flushinput(s);
                        error("Unsupported data type: %d bytes", data_size);
                end

                % LOAD DATA
                for y = 1:data_count
                    data(end+1) = read(s, 1, data_type);
                end

                % APPEND TO dataBuffer
                dataBuffer(i).data_id = data_id;
                dataBuffer(i).data = data;
            end
            
            %disp("Data loaded");

            return
        end
    catch ME
        disp(ME.message)   %             rethrow(ME)
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

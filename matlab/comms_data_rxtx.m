classdef comms_data_rxtx < handle
    
    properties
        tx_buffer % buffer for the tx data
        tx_buffer_elements = 0 % number of datas in the buffer
        serial_port
        connected = false % serial port connection status
    end
    
    methods
        function obj = comms_data_rxtx()
            % constructor
            obj = obj.reset_buffer();
        end
        
        function delete(obj)
            % destructor
            obj.close_port();
        end
        
        function success = open_port(obj, port, baudrate)
            if nargin < 3
                baudrate = 115200;
            end
            
            try
                obj.close_port();
                
                obj.serial_port = serialport(port, baudrate, ...
                    'DataBits', 8, ...
                    'StopBits', 1, ...
                    'Parity', 'none', ...
                    'FlowControl', 'hardware');
                
                flush(obj.serial_port);
                configureTerminator(obj.serial_port, "CR/LF")
                obj.connected = true;
                success = true;
            catch E
                obj.connected = false;
                success = false;
                error(E.identifier, 'Failed to open serial port: %s', E.message)
            end
        end
        
        function close_port(obj)
            if ~isempty(obj.serial_port) && isvalid(obj.serial_port)
                delete(obj.serial_port);
                obj.serial_port = [];
            end
            obj.connected = false;
        end
        
        function connected_status = is_connected(obj)
            connected_status = obj.connected;
        end
        
        function obj = reset_buffer(obj)
            obj.tx_buffer = uint8([]);
            obj.tx_buffer_elements = 0;
        end
        
        function obj = comms_append_int32(obj, data_id, data_count, data)
            obj.tx_buffer_elements = obj.tx_buffer_elements + 1;
            
            % append id, data size and data count
            obj.tx_buffer = [obj.tx_buffer, uint8(data_id)];
            obj.tx_buffer = [obj.tx_buffer, uint8(4)]; % 4 bytes for int32
            obj.tx_buffer = [obj.tx_buffer, uint8(data_count)];
            
            % append data
            for i = 1:data_count
                obj.tx_buffer = [obj.tx_buffer, typecast(uint32(data(i)), 'uint8')];
            end
        end
        
        function comms_send(obj)
            if isempty(obj.serial_port) || ~isvalid(obj.serial_port)
                error('Serial port is not open (comms_send)');
            end
            
            % add buffer id and data count
            start_bytes = typecast(uint16(hex2dec('ABCD')), 'uint8');
            num_elements = typecast(uint16(obj.tx_buffer_elements), 'uint8');
            
            buffer_to_send = [start_bytes, uint8(0), num_elements, obj.tx_buffer];
            
            % send data
            write(obj.serial_port, buffer_to_send, 'uint8');
            
            % reset buffer
            obj.reset_buffer();
        end
        
        function [id, elements, dataArray] = read_data(obj)
            persistent accBuffer
            
            if isempty(accBuffer)
                accBuffer = uint8([]);
            end
            
            id = 0;
            elements = 0;
            dataArray = [];
            
            try
                if isempty(obj.serial_port) || ~isvalid(obj.serial_port)
                    error('Serial port is not open (read_data)');
                end
                
                newData = read(obj.serial_port, obj.serial_port.NumBytesAvailable, 'uint8');
                accBuffer = [accBuffer; newData(:)];
                
                while true
                    % serach for start bytes
                    startIdx = find(accBuffer(1:end-1) == 0xCD & accBuffer(2:end) == 0xAB, 1);
                    if isempty(startIdx)
                        break;
                    end
                    
                    currentBuffer = accBuffer(startIdx:end);
                    
                    % min header length
                    if length(currentBuffer) < 5
                        break;
                    end
                    
                    id = currentBuffer(3);
                    elements = typecast(currentBuffer(4:5), 'uint16');
                    elements = min(elements, 1000);
                    
                    % process the data packet
                    [totalLength, dataArray] = obj.process_packet(currentBuffer, elements);
                    
                    if totalLength > length(currentBuffer)
                        error("Invalid totalLength: %d > %d", totalLength, length(currentBuffer));
                    end
                    
                    accBuffer = currentBuffer((totalLength + 1):end);
                    return;
                end
            catch ME
                accBuffer = uint8([]);
                rethrow(ME);
            end
        end
        
        function configure_callback(obj, app)
            if isempty(obj.serial_port) || ~isvalid(obj.serial_port)
                error('Serial port object is invalid or empty (configure_callback)');
            end
            if ~obj.connected
                error('Serial port is not open (configure_callback)');
            end
            configureCallback(obj.serial_port, "terminator", @(src, event) obj.handle_rx_callback(app, src, event));
        end

        function handle_rx_callback(obj, app, src, ~)
            tic;

            try
                [id, elements, dataBuffer] = obj.read_data();

                if ~isempty(dataBuffer)
                    app.processReceivedData(id, elements, dataBuffer);
                end

                elapsedTime = toc;
                %fprintf('Callback took: %.6f seconds\n', elapsedTime);

            catch ME
                disp(ME.message); % or rethrow(ME)
            end
        end

    end
    
    methods (Access = private)
        function [totalLength, dataArray] = process_packet(~, buffer, elements)
            totalLength = 5;
            dataArray = struct('data_id', {}, 'data', {});
            pointer = 6;
            
            for i = 1:elements
                if pointer + 2 > length(buffer)
                    error("Invalid header (element %d)", i);
                end
                
                data_id = buffer(pointer);
                data_size = double(buffer(pointer + 1));
                data_count = double(buffer(pointer + 2));
                
                if data_count == 0
                    error("Invalid data_count: 0 (element %d)", i);
                end
                
                data_count = min(data_count, 65535);
                
                dataStart = pointer + 3;
                dataEnd = dataStart + data_size * data_count - 1;
                if dataEnd > length(buffer)
                    error("Corrupted data (element %d)", i);
                end
                
                data = typecast(buffer(dataStart:dataEnd), get_data_type(data_size));
                dataArray(end+1) = struct('data_id', data_id, 'data', data);
                
                pointer = dataEnd + 1;
                totalLength = totalLength + 3 + data_size * data_count;
            end
        end
    end
end

function dtype = get_data_type(size)
    switch size
        case 1, dtype = 'uint8';
        case 2, dtype = 'uint16';
        case 4, dtype = 'uint32';
        otherwise, error("Unsupported type");
    end
end
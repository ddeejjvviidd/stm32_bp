function SerialPortRxCallback(app, src, ~)

    tic;

    try
        [id, elements, dataBuffer] = readDataSTM32(src);
        
        if ~isempty(dataBuffer)
            app.processReceivedData(id, elements, dataBuffer);
        end

        clear id;
        clear elements;
        clear dataBuffer;

        elapsedTime = toc;
        %fprintf('Callback took: %.6f seconds\n', elapsedTime);
        % t = clock;
        % fprintf('Time: %02d:%02d:%02.2f\n', t(4), t(5), t(6));

        return

    catch ME
        disp(ME.message) % or rethrow(ME)
    end
end

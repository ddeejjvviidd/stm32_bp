function writeDataSTM32(s, tx_buffer)
    try
        % write(s,iD,"uint16");
        % write(s,nData,"uint16");
        % if nData==0 
        %     return;
        % end
        % write(s,xData(1:nData),"uint32");

        write(s, tx_buffer(1:length(tx_buffer)),"uint8")
    catch ME
        disp(ME.message)
    end
end



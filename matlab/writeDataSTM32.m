function writeDataSTM32(s, xData)
    try
        % write(s,iD,"uint16");
        % write(s,nData,"uint16");
        % if nData==0 
        %     return;
        % end
        % write(s,xData(1:nData),"uint32");

        write(s, xData(1:length(xData)),"uint8")
    catch ME
        disp(ME.message)
    end
end



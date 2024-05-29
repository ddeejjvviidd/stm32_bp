function writeDataSTM32(s, iD, nData, xData)
    try
        write(s,iD,"uint16");
        write(s,nData,"uint16");
        if nData==0 
            return;
        end
        write(s,xData(1:nData),"uint32");
    catch ME
        disp(ME.message)   %             rethrow(ME)
    end
end



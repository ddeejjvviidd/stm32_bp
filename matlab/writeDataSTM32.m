function writeDataSTM32(s, iD, nData, xData)
try
    write(s,iD,"uint16");
    write(s,nData,"uint16");
    if nData==0 
        return;
    end
    if iD > 2^15
        write(s,xData(1:nData),"single");
    else
        write(s,xData(1:nData),"uint32");
    end
catch ME
    disp(ME.message)   %             rethrow(ME)
end
end



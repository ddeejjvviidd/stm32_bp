function [iD, nData, xData] = readDataSTM32(s)
try
    iD=0; nData=0; xData=0;
    if(s.NumBytesAvailable>3)
        iD = read(s, 1, "uint16");
        nData = read(s, 1, "uint16");
        if nData==0
           return;
        end
        if iD > 2^15
            if isReady(s, nData)
                xData= read(s, nData, "single");
            end
        else
            if isReady(s, nData)
                xData= read(s, nData, "uint32");
            end            
        end
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

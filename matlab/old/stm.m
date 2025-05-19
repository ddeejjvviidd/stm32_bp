clear
s = serialport('/dev/ttyACM1', 2*115200);
pause(0.5);
%flush(s)
x = 1:10;
writeDataSTM32(s, 11, 10, x)
pause(0.1);
[iD, nData, xData] = readDataSTM32(s)
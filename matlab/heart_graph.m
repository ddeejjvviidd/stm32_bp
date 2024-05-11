function y = heart_graph(x, a)
    %y = (x.^2/3) + ( 0.9 * ((pi - x.^2).^1/2) .* sin(a*pi*x) );
    %x = -1.75:0.01:1.75;
    y = abs(x).^(2/3) + 0.9 .* (3.3 - abs(x).^2).^(1/2) .* sin(a .* pi .* abs(x));
end
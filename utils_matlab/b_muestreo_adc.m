% CONSTANTES DE CALIBRACIÓN DE PLACA II -----------------------------------
gopt = 2.45 / .512;             % ganancia optoacoplador
gadc = 1024 / 2.5;              % ganancia ADC
r_div = 496.1;
gdiv = (r_div + 857e3) / r_div; % ganancia divisor resistivo entrada Vac
gshunt = 10;                    % 1/resistencia shunt

adc = load('fs2000/placa2_40w_2.txt');
fs = 2000;
n = adc(:,1);
v = adc(:,2);
i = adc(:,3);

v = v * gdiv / (gadc * gopt);   % conversión de muestras a Volt
i = i * gshunt / (gadc * gopt);
[ax, h1, h2] = plotyy(n, v, n, -i);

title(sprintf('N=%i, Fs=%i Hz, Carga=%i W', length(v), 2000, 35))
legend('Tensión','Corriente')
xlabel('n')
ylabel(ax(1), 'Volt')
ylabel(ax(2), 'Ampere')
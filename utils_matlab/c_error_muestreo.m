N = 120;
fs = 2000;
t = 1/fs * (0:N-1);
t_delay = 24.31e-6;     % retraso entre muestras
disp('Delay con respecto al período (%)')
disp(t_delay * 100 * fs)

v = 512 * sin(2*pi*t*50);                       % tensión
i = 512 * sin(2*pi*t*50 + pi/4);                % corriente
i_d = 512 * sin(2*pi*(t+t_delay)*50 + pi/4);    % corriente retrasada

stem(v, 'b-');      hold on
stem(i_d, 'r-');    hold off

p = v .* i;         % potencia
p_d = v .* i_d;     % potencia con error

figure
stem(p, 'b-');      hold on
stem(p_d, 'r-');    hold off

fprintf('Error máximo\t%i\n', max(abs(p-p_d)));
n = length(p);
fprintf('EMC\t\t\t\t%i %%\n', sqrt( sum((p-p_d).^2) ) / (n*(n-1)) );







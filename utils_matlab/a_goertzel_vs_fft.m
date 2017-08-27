file = 'fs2000/40w_2';
adc = load(file);
v = adc(:, 2);
N = length(v);
Fs = 2000;

% ganancias de conversion a volts para Placa I
gopt = 2.461 / .512;
gadc = 1024 / 2.5;
r_div = 478;    gdiv = (r_div + 840e3) / r_div;
kv = gdiv / (gadc * gopt);
% escala para FFT y Goertzel
scale = N/2;

% -- GOERTZEL -------------------------------------------------------------

% coeficientes
k_50 = N*50/Fs;     coeff_50 = 2*cos(2*pi*k_50/N);
k_150 = N*150/Fs;   coeff_150 = 2*cos(2*pi*k_150/N);
k_250 = N*250/Fs;   coeff_250 = 2*cos(2*pi*k_250/N);

% valores previos
delay_50 = zeros(1,3);
delay_150 = zeros(1,3);
delay_250 = zeros(1,3);

% etapa recursiva
for n=1:N
    delay_50 = goertzel_filter(delay_50, v(n), coeff_50);
    delay_150 = goertzel_filter(delay_150, v(n), coeff_150);
    delay_250 = goertzel_filter(delay_250, v(n), coeff_250);
end

% potencia de salida |y(N)|^2
pow_50 = calculate_goertzel_output_power(delay_50, coeff_50);
pow_150 = calculate_goertzel_output_power(delay_150, coeff_150);
pow_250 = calculate_goertzel_output_power(delay_250, coeff_250);

% -- Grafica Goertzel vs FFT ----------------------------------------------

f = Fs/N * (1:N/2);                     % eje de frecuencias
abs_fft = abs(fft(v)) * kv/scale;       % FFT
spectrum = ones(1, N/2) * min(abs_fft); % Goertzel
spectrum(3) = sqrt(pow_50) * kv/scale;  % componentes armónicas (escaladas)
spectrum(9) = sqrt(pow_150) * kv/scale;
spectrum(15) = sqrt(pow_250) * kv/scale;

plot(f, mag2db(spectrum)); hold on;
plot(f, mag2db(abs_fft(2:N/2+1)), 'r'); hold off;
title(sprintf('Goertzel vs. FFT, N=%i, Fs=%i Hz', N, Fs));
legend('Goertzel','FFT')
xlabel('f [Hz]')
ylabel('|y(N)| [dB]')
axis([0 1000 min(mag2db(abs_fft))-5 max(mag2db(abs_fft))+5])

% -- Salida en consola ----------------------------------------------------

fprintf('Datos: %s  ----------------------\r\n', file);
disp('Magnitud Goertzel |y[N]|');
fprintf('\t50 Hz\t%i\n\t150 Hz\t%i\n\t250 Hz\t%i\r\n', spectrum(3), spectrum(9), spectrum(15))
fprintf('\tTHD 3\t%i%%\n\tTHD 5\t%i%%\r\n', sqrt(pow_150)/sqrt(pow_50)*100, sqrt(pow_250)/sqrt(pow_50)*100)
disp('Magnitud FFT |y[N]|');
fprintf('\t50 Hz\t%i\n\t150 Hz\t%i\n\t250 Hz\t%i\r\n', abs_fft(4), abs_fft(10), abs_fft(16))
fprintf('\tTHD 3\t%i%%\n\tTHD 5\t%i%%\r\n', abs_fft(10)/abs_fft(4)*100, abs_fft(16)/abs_fft(4)*100)
/*/*********************************************************************
 * eMeter Sensor
 *
 * Copyright (C) 2014 Manuel Argüelles - manu.argue@gmail.com
 *
 * This file is part of "eMeter WSN".
 *
 * eMeter WSN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eMeter WSN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eMeter WSN. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STRUCTS_H
#define STRUCTS_H

/** Valores previos para la actualización del algoritmo de Goertzel. */
typedef struct
{
    short prev;     // s[n-1]
    short prev2;    // s[n-2]
} goertzel_state_t;

/** Variables de trabajo de un determinado canal. */
struct channel_parms_s
{
    short sample;               // Muestra actual (sin valor de VDC)
    goertzel_state_t harm_1;    // Valores previos para frec. fundamental
    goertzel_state_t harm_3;    // Valores previos para 3º armónico
    goertzel_state_t harm_5;    // Valores previos para 5º armónico
    //long pow_1;     // Potencia frec. fundamental
    //long pow_2;     // Potencia 3º armónico
    //long pow_3;     // Potencia 5º armónico
};

/** Variables de trabajo para el calculo de los parámetros de la fase. Se
    actualizan en cada muestra tomada de los canales de tensión y corriente. */
struct phase_working_parms_s
{
    long act_power_count;       // Suma de términos de potencia activa
    unsigned long v_rms_count;  // Suma de términos de tensión RMS
    unsigned long i_rms_count;  // Suma de términos de corriente RMS
    struct channel_parms_s voltage;    // Canal de tensión
    struct channel_parms_s current;    // Canal de corriente
};

/** Parámetros calculados (definitivos) de la fase. */
// TODO: ver tipo de dato. Puede que lo envíe en Q15. ver precisión de cada
// medición.
struct phase_readings_s
{
    double v_rms;           // Tensión RMS
    double i_rms;           // Corriente RMS
    double act_power;       // Potencia activa
    double app_power;       // Potencia aparente
    double energy;          // Energía activa
    double power_factor;    // Factor de potencia
    double v_thd_3;         // Pot. relativa 3º armónico con respecto a fundamental
    double v_thd_5;         // Pot. relativa 5º armónico con respecto a fundamental
    double i_thd_3;
    double i_thd_5;
};

#endif //STRUCTS_H

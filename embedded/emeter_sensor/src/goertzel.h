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

#ifndef GOERTZEL_H
#define GOERTZEL_H

#include "structs.h"
//#define GTZL_PHASE    // descomentar para compilar la función de calculo de fase

/*
 * Coeficientes del filtro de Goertzel para N=120, Fs=2000 Hz. Los coeficientes
 * están divididos por dos y convertidos a formato Q15.
 *      k = (int) (N * f_tono / Fs)
 *      c = 2 * cos(2*pi*k/N)
 *      c_q15 = (int) ( c/2 * 2^15 )
 */
#define GTZL_50_HZ          32364
#define GTZL_150_HZ         29196
#define GTZL_250_HZ         23170

/* Escalado señal de entrada */
#define INPUT_SCALE         3
#define INPUT_SCALE_2       6   // x2
     
#ifdef GTZL_PHASE
/*
 * Coeficientes para el calculo de la fase, para N=120. En tipo double.
 *      COEF_REAL = cos(2*pi*k/N)
 *      COEF_IMAG = sin(2*pi*k/N)
 */
#define GTZL_COEF_RE_50     0.987688340595138
#define GTZL_COEF_IM_50     0.156434465040231
#define GTZL_COEF_RE_150    0.891006524188368
#define GTZL_COEF_IM_150    0.453990499739547
#define GTZL_COEF_RE_250    0.707106781186548
#define GTZL_COEF_IM_250    0.707106781186547

double gtzlOutPhase(goertzel_state_t *s, double coeffReal, double coeffImag);
#endif

extern inline void gtzlInit(goertzel_state_t *s);
void gtzlFilter(goertzel_state_t *s, short input, short coefficient);
unsigned long gtzlOutPower(goertzel_state_t *s, short coefficient);

#endif

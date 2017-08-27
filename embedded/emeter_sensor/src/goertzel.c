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

#include "goertzel.h"
#ifdef GTZL_PHASE
#include <math.h>
#endif

/**
 * @brief   Inicializa valores previos de Goertzel.
 *
 * @param   s   Valores previos de Goertzel a inicializar.
 */
inline void gtzlInit(goertzel_state_t *s)
{
    s->prev = s->prev2 = 0;
}

/**
 * @brief   Calcula el valor de Goertzel para una entrada en un determinado
 *          instante, y para una sola frecuencia. Implementa la ecuación:
 *          s[n] = x[n] + coeff * s[n-1] - s[n-2]
 *
 * @param   s           Valores previos de Goertzel.
 * @param   input       Valor de la señal de entrada.
 * @param   coefficient Coeficiente para la frecuencia en particular.
 */
void gtzlFilter(goertzel_state_t *s, short input, short coefficient)
{
    long product;
      
    product = ((long)s->prev * coefficient) >> 14;
    // escala la entrada para prevenir overflow
	short newOut = (short)((input >> INPUT_SCALE) + (short)product - s->prev2);
    // actualiza los valores calculados previamente
    s->prev2 = s->prev;
    s->prev = newOut;
} 

/**
 * @brief   Calcula la potencia de salida del algoritmo de Goertzel para una
 *          determinada frecuencia, en base a los valores previamente calculados.
 *          Implementa la ecuación:
 *          |y[N]|^2 = s[n-1]^2 + s[n-2]^2 - coeff * s[n-1] * s[n-2]
 *
 * @param   s           Valores previos de Goertzel.
 * @param   coefficient Coeficiente para la frecuencia en particular.
 */
unsigned long gtzlOutPower(goertzel_state_t *s, short coefficient)
{
    long product1, product2, product3;
    unsigned long power;

    // calcula último paso con entrada s[N] = 0
    gtzlFilter(s, 0, coefficient);
    
    // calcula salida
    product1 = (long)s->prev * s->prev;     // s[n-1]^2
    product2 = (long)s->prev2 * s->prev2;   // s[n-2]^2
    product3 = ((long)s->prev * coefficient) >> 14;
    product3 = product3 * s->prev2;
         
    power = (unsigned long)((product1 + product2 - product3));
    power = power << INPUT_SCALE_2;
    
    return power;
}

#ifdef  GTZL_PHASE
double gtzlOutPhase(goertzel_state_t *s, double coeffReal, double coeffImag)
{
    // fase. usa math lib para calcular el arco tangente
    // s[n] = Re{s[n]} + i * Im{s[n]} = s[n-1] * cr - s[n-2] + i * s[n-1] * ci;
    double outReal = (double)s->prev * coeffReal - (double)s->prev2;
    double outImag = (double)s->prev * coeffImag;
    return atan2(outImag, outReal);
}
#endif

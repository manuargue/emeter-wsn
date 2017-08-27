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

#ifndef SENSOR_END_DEVICE_H
#define SENSOR_END_DEVICE_H

// MACROS ----------------------------------------------------------------------
/** Determina si un mensaje en el buffer ZNP pertenece al cluster de la aplicación. */
#define IS_MESSAGE_CLUSTER()        (AF_INCOMING_MESSAGE_CLUSTER() == MESSAGE_CLUSTER)
/** Conecta carga a la red a través del relé. */
#define CONNECT_LOAD()              (P2OUT &= ~RELAY)
/** Desconecta carga a la red a través del relé. */
#define DISCONNECT_LOAD()           (P2OUT |= RELAY)
/** Determina si la carga se encuentra conectada a la red. */
#define IS_LOAD_CONNECTED()         (!(P2IN & RELAY))
/** Conmuta el estado de conexión de la carga. */
#define TOGGLE_LOAD_CONNECTION()    (P2OUT ^= RELAY)

// DEFINICIONES ----------------------------------------------------------------

// MSP430F2274
#define MCLK_FREQ           8000000
#define SMCLK_FREQ          MCLK_FREQ/2

#define SAMPLE_FREQ         2000        // frecuencia de muestreo (Hz)
#define N_SAMPLES           2000        // numero de muestras tomadas
#define BUF_LEN             120         // tamaño buffer de muestras (40 muestras/ciclo)

#define ACHAN_V             INCH_2      // canal analógico de tensión
#define ACHAN_I             INCH_3      // canal analógico de corriente
#define RELAY               BIT4        // P2.4 (output)

// Flag de estados de la aplicación
#define STATE_IDLE                          0x01
#define STATE_ZNP_STARTUP                   0x02
#define STATE_INIT_MEASURE                  0x04
#define STATE_CALC_PARAMETERS               0x08
#define STATE_SEND_REPORT_MSG               0x10
#define STATE_INCOMING_MSG                  0x20
#define STATE_CHECK_ALARM_FLAGS             0x40
#define STATE_ERROR                         0x80

#define ZNP_RESTART_DELAY                   5   // seg
#define ZNP_RESTART_DELAY_IF_MESSAGE_FAIL   5
#define REPORT_MSG_RETRY_DELAY              1
#define MEASURE_PERIOD                      5
#define LOAD_RECONNECTION_DELAY             8

#define MAX_REPORT_MSG_RETRY    3       // máx. intentos de envío de reporte

// VARIABLES EXTERNAS ----------------------------------------------------------
extern unsigned char znpBuf[100];       // buffer para ZNP
extern signed int znpResult;            // resultado de operaciones ZNP

// FUNCIONES EXTERNAS ----------------------------------------------------------
extern void (*timerAIsr)(void);         // puntero a función ISR del Timer A
extern void (*timerBIsr)(void);         // puntero a función ISR del Timer B
extern void (*srdyIsr)(void);           // puntero a función ISR de señal SRDY
extern void (*buttonIsr)(void);         // puntero a función ISR del botón

// VARIABLES LOCALES -----------------------------------------------------------
static signed short v_buf[BUF_LEN];            // buffer muestras de tensión
static signed short i_buf[BUF_LEN];            // buffer muestras de corriente
static unsigned int state = 0;                 // flag de estados de la aplicación
static unsigned int stateFlags = 0;            // flags seteados durante los estados
static unsigned int seqNumber = 0;             // número de secuencia de paquetes
static unsigned char reportRetry = 0;          // reintentos de envio de reporte
static unsigned int vloFreq;                   // frecuencia del VLO
static struct message_t outMsg;                // buffers de mensajes
static struct message_t inMsg;
static struct phase_working_parms_s phase;
static struct phase_readings_s readings;
static unsigned int samp_count;                // cantidad de muestras tomadas
static char signal_sign;
static char first_samp;
static char sampling;

// FUNCIONES LOCALES -----------------------------------------------------------
static void stateMachine();                    // ciclo principal de la aplicación
void isrInitMeasure();                  // ISR iniciar medición
void isrSrdy();                         // ISR de SRDY
void isrZnpStartup();                   // ISR inicio de ZNP
void isrSendReport();                   // ISR enviar mensaje
void isrAdcTimerA();                    // ISR Timer A para muestreo
void isrToggleLoad();                   // ISR del botón
void isrReconnectLoad();                // ISR relé
static int initRetryTimerA(unsigned int sleepTime, void (*handler)(void));
static int initTimerB(unsigned int seconds);
static void initTimerAdc();                    // configura e inicia el Timer A para muestreo
static void initAdc();                         // configura registros del ADC
static char detectZeroCrossing();
static void harmonics();

#endif //SENSOR_END_DEVICE

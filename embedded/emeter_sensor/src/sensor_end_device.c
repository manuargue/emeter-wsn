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

#define BOARD_ONE             // definir la placa antes de incluir calibrate.h
#define DETECT_ZERO_CROSSING
//#define VERBOSE_APP         // para ver salida con printf
//#define PRINT_BUFFER        // imprime muestras de los buffers v_buf[] e i_buf[]

// INCLUDES --------------------------------------------------------------------
#include <string.h>
#include <math.h>
#include "../lib/HAL/hal.h"
#include "../lib/ZNP/znp_interface.h"
#include "../lib/ZNP/znp_interface_spi.h"
#include "../lib/ZNP/af_zdo.h"
#include "../lib/ZNP/application_configuration.h"
#include "znp_simple_app_utils.h"
#include "sensor_end_device.h"
#include "message.h"
#include "structs.h"
#include "calibrate.h"
#include "goertzel.h"

unsigned char msgBuf[LEN_HEADER+MAX_DATA_MESSAGE];  // buffer de mensajes

int main(void)
{
    /* INICIALIZACIÓN */
    halInit();
    HAL_DISABLE_INTERRUPTS();

    // calibra el VLO
    vloFreq = calibrateVlo();
    if (vloFreq == -1)
    {
#ifdef VERBOSE_APP
        printf("\r\nError calibrando VLO. Frecuencia fuera de rango\r\n");
#endif
        state |= STATE_ERROR;
    }
#ifdef VERBOSE_APP
    printf("\r\nIniciando... VLO=%uHz\r\n", vloFreq);
#endif

    // config. interrupción por SRDY en flanco descendente
    halConfigureSrdyInterrrupt(SRDY_INTERRUPT_FALLING_EDGE | WAKEUP_AFTER_SRDY);
    srdyIsr = &isrSrdy;

    // config. ISR del botón
    buttonIsr = &isrToggleLoad;

    // pin relé
    P2DIR |= RELAY;     // output
    P2REN &= ~RELAY;    // pull down/up resistor deshabilitada
    CONNECT_LOAD();

    // configura ADC
    initAdc();
    HAL_ENABLE_INTERRUPTS();

    outMsg.sequence = 0;
    state = STATE_ZNP_STARTUP;            
    
    // ciclo principal de la aplicación
    stateMachine();
}

/**
 * @brief   Ciclo principal de la aplicación. Consiste en un bucle infinito que
 *          de acuerdo al estado de la aplicación (variable state) ejecuta una
 *          acción.
 */
static void stateMachine()
{
    while (1)
    {
        if (state & STATE_IDLE)
        {
            // no hace nada!
        }
        else if (state & STATE_ZNP_STARTUP)
        {
            // inicia el stack de ZigBee, como dispositivo lógico end-device
            signed int startResult = startZnp(END_DEVICE);
            if (startResult != ZNP_SUCCESS)
            {
                // reintenta
#ifdef VERBOSE_APP
                printf("Error iniciando red. Error %i, ZNP Result %i. Reintentando...\r\n",
                       startResult, znpResult);
#endif
                initRetryTimerA(ZNP_RESTART_DELAY, &isrZnpStartup);
            }
            else
            {
#ifdef VERBOSE_APP
                printf("Red iniciada\r\n");
#endif
                reportRetry = 0;
                // almacena dirección MAC en el mensaje
                memcpy(outMsg.mac, getMacAddress(), 8);
#ifdef VERBOSE_APP
                // imprime información de la red
                printf("RED:");
                getNetworkConfigurationParameters();
                getDeviceInformation();
#endif
                blinkLedsTwice(100);
                // configura Timer B para disparar las mediciones
                initTimerB(MEASURE_PERIOD);
            }
            state &= ~STATE_ZNP_STARTUP;
        }
        else if (state & STATE_INIT_MEASURE)
        {
            // inicializa variables para medición
            samp_count = 0;
            first_samp = 1;
            sampling = 0;
            phase.act_power_count = 0;          // acumulaciones de productos
            phase.i_rms_count = 0;
            phase.v_rms_count = 0;
            gtzlInit(&(phase.voltage.harm_1));  //valores previos de Goertzel
            gtzlInit(&(phase.voltage.harm_3));
            gtzlInit(&(phase.voltage.harm_5));
            gtzlInit(&(phase.current.harm_1));
            gtzlInit(&(phase.current.harm_3));
            gtzlInit(&(phase.current.harm_5));
            
            state &= ~STATE_INIT_MEASURE;
            state |= STATE_IDLE;
            // configura Timer A con tiempo = 1/Fs disparar el ADC
            initTimerAdc();
        }
        else if (state & STATE_CALC_PARAMETERS)
        {
#ifdef PRINT_BUFFER
            // imprime muestras en buffer como: n, v[n], i[n]
            for (int i = 0; i < BUF_LEN; i++)
                printf("%u,%i,%i\r\n", i, v_buf[i], i_buf[i]);
#endif
            // calcula parámetros de la red eléctrica
            readings.v_rms = KV * sqrt(phase.v_rms_count / N_SAMPLES);          // V RMS
            readings.i_rms = KI * sqrt(phase.i_rms_count / N_SAMPLES);          // I RMS
            readings.act_power = KP * phase.act_power_count / N_SAMPLES;        // P. Activa
            readings.app_power = readings.v_rms * readings.i_rms;               // P. Aparente
            readings.power_factor = readings.act_power / readings.app_power;    // FP
            readings.energy = readings.act_power * N_SAMPLES / SAMPLE_FREQ;     // Energía
            harmonics();    // Armónicos con Goertzel
            
            // crea header de mensaje reporte
            outMsg.sequence = seqNumber++;
            outMsg.msgType = MSG_TYPE_REPORT;
            outMsg.lenData = MSG_LEN_DATA_REPORT;
            
            // copia los parámetros calculados al campo de datos
            // se almacenan en little-endian (LSB primero)
            memcpy(&(outMsg.data[MSG_VRMS_FIELD]), &(readings.v_rms), sizeof(double));
            memcpy(&(outMsg.data[MSG_IRMS_FIELD]), &(readings.i_rms), sizeof(double));
            memcpy(&(outMsg.data[MSG_PACT_FIELD]), &(readings.act_power), sizeof(double));
            memcpy(&(outMsg.data[MSG_PAPP_FIELD]), &(readings.app_power), sizeof(double));
            memcpy(&(outMsg.data[MSG_ENERGY_FIELD]), &(readings.energy), sizeof(double));
            memcpy(&(outMsg.data[MSG_FP_FIELD]), &(readings.power_factor), sizeof(double));
            memcpy(&(outMsg.data[MSG_V_THD_3_FIELD]), &(readings.v_thd_3), sizeof(double));
            memcpy(&(outMsg.data[MSG_V_THD_5_FIELD]), &(readings.v_thd_5), sizeof(double));
            memcpy(&(outMsg.data[MSG_I_THD_3_FIELD]), &(readings.i_thd_3), sizeof(double));
            memcpy(&(outMsg.data[MSG_I_THD_5_FIELD]), &(readings.i_thd_5), sizeof(double));
            
            // guarda mensaje serializado en el buffer 
            serializeMessage(&outMsg, msgBuf);
            
            state &= ~STATE_CALC_PARAMETERS;
            state |= STATE_SEND_REPORT_MSG;
        }
        else if (state & STATE_SEND_REPORT_MSG)
        {
            // deshabilita int. por SRDY para que pueda comunicarse con el
            // CC2530 sin inconvenientes
            DISABLE_SRDY_INTERRUPT();
            setLed(GREEN_LED);
            state &= ~STATE_SEND_REPORT_MSG;
#ifdef VERBOSE_APP
            // imprime mensaje a enviar
            printf("\r\nENVIANDO REPORTE - intento=%i\r\n", reportRetry);
            printMessage(&outMsg);
#endif
            // como uso un tiempo pequeño entre cada medición, detengo el
            // timer para q no interfiera con el envio. Si el tiempo fuera
            // mayor, no seria necesario detenerlo.
            STOP_TIMER_B();
            // envia mensaje
            afSendData(DEFAULT_ENDPOINT, DEFAULT_ENDPOINT, 0,
                       MESSAGE_CLUSTER, msgBuf, getSizeOfMessage(&outMsg));
            clearLeds();
            // hubo un error enviando el mensaje
            if (znpResult != ZNP_SUCCESS)
            {               
                // si quedan reintentos de envio, vuelve a intentar
                if (++reportRetry < MAX_REPORT_MSG_RETRY)
                {
#ifdef VERBOSE_APP
                    printf("Error znpResult=%i. Reintentando...\r\n", znpResult);
#endif
                    initRetryTimerA(REPORT_MSG_RETRY_DELAY, &isrSendReport);
                }
                else
                {
                    // se reconecta a la red.
                    reportRetry = 0;
                    initRetryTimerA(ZNP_RESTART_DELAY_IF_MESSAGE_FAIL, &isrZnpStartup);
                }
            }
            else
            {
                initTimerB(MEASURE_PERIOD);
                reportRetry = 0;
                // espera respuesta del coordinador
                ENABLE_SRDY_INTERRUPT();
            }            
        }
        else if (state & STATE_INCOMING_MSG)
        {
            // deshabilita int. por SRDY para poder hacer el polling al CC2530
            DISABLE_SRDY_INTERRUPT();
            delayMs(200);
            
            setLed(RED_LED);
            // polling al CC2530 para obtener los datos recibidos            
            if (spiPoll() == 0 && znpBuf[SRSP_LENGTH_FIELD] > 0)
            {
                clearLeds();
                // si el mensaje pertenece al cluster
                if (IS_AF_INCOMING_MESSAGE() && IS_MESSAGE_CLUSTER())
                {
                    // deserializa mensaje
                    inMsg = deserializeMessage(znpBuf+20);
                    //znpBuf[SRSP_LENGTH_FIELD] = 0;
                    // si el mensaje es una alarma chequea los flags recibidos
                    if (inMsg.msgType == MSG_TYPE_ALARM)
                    {
#ifdef VERBOSE_APP
                        printf("Rx:\n\r");
                        printMessage(&inMsg);
#endif
                        state |= STATE_CHECK_ALARM_FLAGS;
                    }
                }
            }
            state &= ~STATE_INCOMING_MSG;
        }
        else if (state & STATE_CHECK_ALARM_FLAGS)
        {
            // obtiene los flags de alarma desde el mensaje recibido
            unsigned int alarmFlags;
            memcpy(&alarmFlags, &(inMsg.data[MSG_ALARM_FLAGS_FIELD]),
                   sizeof(alarmFlags));
            
            if (alarmFlags != 0)
            {
#ifdef VERBOSE_APP
                // imprime flags activados
                printf("ALARMAS: ");
                if (alarmFlags & ALARM_FLAG_OVERVOLTAGE)
                    printf("SOBRETENSION | ");
                else if (alarmFlags & ALARM_FLAG_UNDERVOLTAGE)
                    printf("BAJATENSION | ");
                if (alarmFlags & ALARM_FLAG_V_THD_3)
                    printf("THD 3 | ");
                if (alarmFlags & ALARM_FLAG_V_THD_5)
                    printf("THD 5 | ");
                if (alarmFlags & ALARM_FLAG_NEGATIVE_POWER)
                    printf("P.ACT NEGATIVA | ");
                if (alarmFlags & ALARM_FLAG_MAX_ENERGY)
                    printf("CONSUMO ELECTRICO SUPERADO | ");
                if (alarmFlags & ALARM_FLAG_MAX_LOAD)
                    printf("CORRIENTE MAX. SUPERADA");
                printf("\n\r");
#endif
                if ((alarmFlags & ALARM_FLAG_MAX_LOAD) ||
                    (alarmFlags & ALARM_FLAG_MAX_ENERGY))
                {
#ifdef VERBOSE_APP
                    printf("\n\rCARGA DESCONECTADA\n\r");
#endif
                    // desconecta la carga y detiene las mediciones
                    DISCONNECT_LOAD();
                    STOP_TIMER_B();
                    setLed(GREEN_LED);
                    setLed(RED_LED);
                    // vuelve a reconectar pasado un tiempo
                    initRetryTimerA(LOAD_RECONNECTION_DELAY, &isrReconnectLoad);
                }
            }
            state &= ~STATE_CHECK_ALARM_FLAGS;
        }
        else if (state & STATE_ERROR)
        {
            // detiene interrupciones y se pone a dormir indefinidamente
#ifdef VERBOSE_APP
            printf("\r\nERROR: DETENIDO\r\n");
#endif
            HAL_DISABLE_INTERRUPTS();
            HAL_SLEEP();
        }
        else
        {
            // nada que hacer ... zzz ...
            HAL_SLEEP();
        }
    }
}

/**
 * @brief   Inicializa el ADC para muestrear los canales de tensión y corriente,
 *          utilizando la referencia de voltaje interna de 2.5V y un tiempo de
 *          sample-and-hold de 64 ciclos de ADC10CLK.
 */
static void initAdc(void)
{
    ADC10CTL0 = SREF_1 +    // referencia VR+ = VREF+, VR- = VSS
                REFON +     // usa referencia interna
                REF2_5V +   // referencia de 2.5V
                ADC10ON +   // habilita ADC core
                ADC10SHT_3; // sample and hold time 64 x ADC10CLKs

    delayMs(1);             // espera que se estabilice la referencia
                            // (tarda aprox. 30us)
    
    ADC10CTL1 = SHS_0;      // trigger por software
    ADC10AE0 = BIT2 + BIT3; // habilita A2 y A3 como entradas
}

/**
 * @brief   Configura el Timer A para que interrumpa cada 1/Fs, de manera de
 *          disparar las mediciones con el ADC. Utiliza como clock el SMCLK.
 */
static void initTimerAdc(void)
{
    STOP_TIMER_A();                 // detiene Timer A en ejecución
    timerAIsr = &isrAdcTimerA;
    CCTL0 = CCIE;                   // CCR0 interrupt enabled
#define ADC_COUNT SMCLK_FREQ/SAMPLE_FREQ
    CCR0 = ADC_COUNT;               // CCR0=2000 @ Fs=2kHz
    TACTL = TASSEL_2 + MC_1;        // SMCLK, up mode
}

/**
 * @brief   Configura el Timer A con un tiempo de espera para realizar el reintento
 *          de alguna operación (reenvío de mensajes, reinicio de ZNP, etc) y
 *          establece la función pasada como argumento como ISR.
 *
 * @pre     VLO configurado como ACLK.
 * @pre     Se calibró el VLO; frecuencia del VLO almacenada en vloFreq.
 *
 * @param   sleepTime   Tiempo de espera (segundos).
 * @param   handler     Puntero a la función ISR.
 * @return  -1 si el tiempo de espera esta fuera de rango; -2 si no se configuró
 *          el VLO previamente; 0 en caso de éxito.
 */
static int initRetryTimerA(unsigned int sleepTime, void (*handler)(void))
{
    // verifica que no se pase del tiempo máximo
    //if (sleepTime > (long)2*0xFFFF/vloFreq)
    if (sleepTime > 10)
        return -1;
    // y que se haya calibrado el VLO
    if (vloFreq == 0)
        return -2;

    STOP_TIMER_A();                     // detiene timer A en ejecución
    CCTL0 = CCIE;                       // CCR0 interrupt enabled
    unsigned int fclk = vloFreq / 2;
    CCR0 = fclk * sleepTime;            // configura contador
    TACTL = TASSEL_1 + MC_1 + ID_1;     // ACLK, upmode, divider ACLK/2

    timerAIsr = handler;                // cambia el handler del Timer A ISR
    return 0;
}

/**
 * @brief   Configura e inicia el Timer B con el tiempo pasado como parámetro.
 *
 * @pre     VLO configurado como ACLK.
 * @pre     Se calibró el VLO; frecuencia del VLO almacenada en vloFreq.
 * @post    La variable 'state' cambia el valor al próximo estado. No debe
 *          modificarse mientras el timer esté corriendo.
 *
 * @param   seconds     Tiempo, en segudos.
 * @return  -1 si el tiempo esta fuera de rango; -2 si no se configuró
 *          el VLO previamente; 0 en caso de éxito.
 */
static int initTimerB(unsigned int seconds)
{
    // verifica que no se pase del tiempo máximo
    if (seconds > 10)
        return -1;
    // y que se haya calibrado el VLO
    if (vloFreq == 0)
        return -2;

    STOP_TIMER_B();                     // detiene timer B
    TBCCTL0 = CCIE;                     // TBCCR0 interrupt enabled
    unsigned int fclk = vloFreq / 2;
    TBCCR0 = fclk * seconds;            // configura contador
    TBCTL = TASSEL_1 + MC_1 + ID_1;     // ACLK, upmode, divider ACLK/2
    timerBIsr = &isrInitMeasure;        // handle del Timer B ISR
    return 0;
}

/**
 * @brief   Reconecta la carga a la red eléctrica e inicia timer de mediciones.
 */
void isrReconnectLoad(void)
{
    STOP_TIMER_A();
    CONNECT_LOAD();
    clearLeds();
#ifdef VERBOSE_APP
    printf("\n\rCARGA CONECTADA\n\r");
#endif
    initTimerB(MEASURE_PERIOD);
}

/**
 * @brief   Setea el flag de estado STATE_ZNP_STARTUP.
 */
void isrZnpStartup(void)
{
    STOP_TIMER_A();
    state |= STATE_ZNP_STARTUP;
}

/**
 * @brief   Setea el flag de estado STATE_SEND_REPORT_MSG.
 */
void isrSendReport(void)
{
    STOP_TIMER_A();
    state |= STATE_SEND_REPORT_MSG;
}

/**
 * @brief   ISR de la señal SRDY. Establece el flag de estado
 *          STATE_INCOMING_MSG indicando un mensaje recibido.
 */
void isrSrdy(void)
{
    state |= STATE_INCOMING_MSG;
}

/**
 * @brief   ISR del Timer B. Establece el flag de estado STATE_INIT_MEASURE
 *          indicando que se deben iniciar las mediciones.
 */
void isrInitMeasure()
{   
    state |= STATE_INIT_MEASURE;
}

/**
 * @brief   ISR del botón. Conecta la carga si se encuentra desconectada, o la
 *          desconecta en caso contrario.
 */
void isrToggleLoad(void)
{
    TOGGLE_LOAD_CONNECTION();
/*
    if (!IS_LOAD_CONNECTED())
    {
#ifdef VERBOSE_APP
        printf("\n\rCarga conectada a la red\n\r");
#endif
        CONNECT_LOAD();
    }
*/
}

/**
 * @brief   ISR del Timer A para realizar el muestreo. El muestreo se hace en la
 *          ISR para disminuir el overhead y mejorar la precisión de la
 *          frecuencia de muestreo.
 */
void isrAdcTimerA(void)
{
#ifdef DETECT_ZERO_CROSSING
    // TODO: arreglar esto.. algún día! :)
    if (sampling == 0)
    {
        sampling = detectZeroCrossing();
        if (sampling == 0)
            return;
    }
#endif

    unsigned short buf16;
    ADC10CTL0 &= ~(ADC10IFG + ENC);     // borra flag interrupción de ADC
    ADC10CTL1 = ACHAN_V;                // lee canal de tension
    ADC10CTL0 |= ENC + ADC10SC;         // comienza conversion
    while (!(ADC10CTL0 & ADC10IFG));    // espera que finalice
    buf16 = ADC10MEM;                   // guarda muestra en buffer
    
    ADC10CTL0 &= ~(ADC10IFG + ENC);     // repite para el canal de corriente
    ADC10CTL1 = ACHAN_I;
    ADC10CTL0 |= ENC + ADC10SC;         
    while (!(ADC10CTL0 & ADC10IFG));

    // resta el offset Vdc a la muestra
    phase.current.sample = (short)ADC10MEM - OFFSET_IN_I;
    phase.voltage.sample = (short)buf16 - OFFSET_IN_V;

    // acumula términos de RMS
    phase.v_rms_count += (unsigned long)phase.voltage.sample * phase.voltage.sample;
    phase.i_rms_count += (unsigned long)phase.current.sample * phase.current.sample;

    // acumula término de potencia activa
    phase.act_power_count += (long)phase.voltage.sample * phase.current.sample;
    
    // si los buffers no estan llenos se guardan las muestras nuevas
    if (samp_count < BUF_LEN)
    {
        v_buf[samp_count] = phase.voltage.sample;
        i_buf[samp_count] = phase.current.sample;
    }
    
    // si se tomaron todas las muestras calcula parámetros
    if (samp_count++ > N_SAMPLES)
    {
        STOP_TIMER_A();
        state &= ~STATE_IDLE;
        state |= STATE_CALC_PARAMETERS;
    }
}

#define S_NEGATIVE  0       /** Señal con signo negativo. */
#define S_POSITIVE  1       /** Señal con signo positivo. */
/**
 * @brief   Detecta si hubo un cruce por cero (cambio de signo) en el canal de
 *          tensión. Utiliza la variable signal_sign.
 * @return  0, si no hubo cruce; 1, si hubo cruce.
 */
static char detectZeroCrossing() {
    ADC10CTL0 &= ~(ADC10IFG + ENC);     // borra flag interrupción de ADC
    ADC10CTL1 = ACHAN_V;                // lee canal de tension
    ADC10CTL0 |= ENC + ADC10SC;         // comienza conversion
    while (!(ADC10CTL0 & ADC10IFG));    // espera que finalice
    
    if (first_samp == 1)
    {
        first_samp = 0;
        if (ADC10MEM >= OFFSET_IN_V)
            signal_sign = S_POSITIVE;
        else
            signal_sign = S_NEGATIVE;
    }
    else
    {   
        if ( ((ADC10MEM >= OFFSET_IN_V) && (signal_sign == S_NEGATIVE)) || 
             ((ADC10MEM < OFFSET_IN_V) && (signal_sign == S_POSITIVE)) )
        {
            // hubo un cambio de signo
            return 1;
        }
    }
    return 0; 
}

/**
 * @brief   Aplica el algoritmo de Goertzel a las muestras de tensión y corriente
 *          almacenadas en los buffers (v_buf, i_buf) para calcular el HD%f de
 *          la 3º y 5º componente espectral.
 *
 * @pre     Se deben haber almacenado las muestras de tensión/corriente en sus
 *          respectivos buffers.
 */
static void harmonics(void)
{
    // etapa recursiva de Goertzel
    for (int i = 0; i < BUF_LEN; i++)
    {
        // tensión
        gtzlFilter(&(phase.voltage.harm_1), v_buf[i], GTZL_50_HZ);
        gtzlFilter(&(phase.voltage.harm_3), v_buf[i], GTZL_150_HZ);
        gtzlFilter(&(phase.voltage.harm_5), v_buf[i], GTZL_250_HZ);
        // corriente
        gtzlFilter(&(phase.current.harm_1), i_buf[i], GTZL_50_HZ);
        gtzlFilter(&(phase.current.harm_3), i_buf[i], GTZL_150_HZ);
        gtzlFilter(&(phase.current.harm_5), i_buf[i], GTZL_250_HZ);
    }

    // etapa no recursiva. calculo de potencia y hd de cada armónico con
    // respecto a la fundamental.
    // tensión
    double fund_pow = sqrt(gtzlOutPower(&(phase.voltage.harm_1), GTZL_50_HZ));    
    readings.v_thd_3 = sqrt(gtzlOutPower(&(phase.voltage.harm_3), GTZL_150_HZ)) / fund_pow * 100;
    readings.v_thd_5 = sqrt(gtzlOutPower(&(phase.voltage.harm_5), GTZL_250_HZ)) / fund_pow * 100;
    // corriente
    fund_pow = sqrt(gtzlOutPower(&(phase.current.harm_1), GTZL_50_HZ));    
    readings.i_thd_3 = sqrt(gtzlOutPower(&(phase.current.harm_3), GTZL_150_HZ)) / fund_pow * 100;
    readings.i_thd_5 = sqrt(gtzlOutPower(&(phase.current.harm_5), GTZL_250_HZ)) / fund_pow * 100;    
}

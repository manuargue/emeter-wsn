/*/*********************************************************************
 * eMeter Collector
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

/*/****************************************************************************
 * INCLUDES
 */

#include <string.h>	// TODO: se puede usar osal mem!
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "sapi.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_lcd.h"
#include "hal_uart.h"
#include "collector.h"
#include "message.h"

/*/****************************************************************************
 * CONSTANTES
 */

// Offsets de mensaje UART
#define UART_FRAME_SOF_OFFSET       0   /**< Offset UART de Start of Frame */
#define UART_FRAME_LENGTH_OFFSET    1   /**< Offset UART de longitud de DATA */
#define UART_FRAME_CMD_OFFSET       2   /**< Offset UART de comando */
#define UART_FRAME_MAC_OFFSET       3   /**< Offset UART de dirección MAC */
#define UART_FRAME_DATA_OFFSET      11  /**< Offset UART de campo DATA */

#define CPT_SOP                     0xFE
#define UART_FRAME_MAX_LENGTH       60
#define UART_FRAME_HEADER_LENGTH    UART_FRAME_DATA_OFFSET
#define UART_FRAME_REPORT_LENGTH    MSG_LEN_DATA_REPORT + 4

// Stack Profile
#define ZIGBEE_2007                 0x0040
#define ZIGBEE_PRO_2007             0x0041

#ifdef ZIGBEEPRO
#define STACK_PROFILE               ZIGBEE_PRO_2007
#else 
#define STACK_PROFILE               ZIGBEE_2007
#endif

// Estados de la app
#define APP_INIT                    0   /**< Estado inicializando aplicación */
#define APP_START                   2   /**< Estado aplicación iniciada */

// Eventos OSAL
#define MY_START_EVT                0x0001  /**< Evento de inicialización de la aplicación */
#define SEND_UART_MSG               0x0002  /**< Evento de envío de mensaje por UART */
#define MY_TEST_EVT                 0x0004  /**< Evento para realizar pruebas */

#define REPORT_FAILURE_LIMIT        5   /**< Cantidad máx. de fallos al envíar
                                          *  un mensaje */

/*/****************************************************************************
 * VARIABLES LOCALES 
 */

static uint8 appState = APP_INIT;
static uint8 myStartRetryDelay = 10;          // milisegundos
static uint8 reportFailureNr = 0;
static uint8 msgBuf[MSG_LEN_HEADER+MSG_MAX_DATA_LEN];
static uint16 alarmFlags = 0;
static struct message_t msgReport;
// condiciones de alarmas (configurables por joystick)
static double alarm_max_irms = I_RMS_MAX_LOAD;
static uint16 alarm_max_energy = MAX_ENERGY_CONSUMPTION;

/*/****************************************************************************
 * FUNCIONES LOCALES
 */

void initUart(halUARTCBack_t pf);
void uartRxCB(uint8 port, uint8 event);
static void sendUartMessage(struct message_t *msg, void *other);
static uint16 checkAlarms(struct message_t *msg);

/*/****************************************************************************
 * VARIABLES GLOBALES
 */

/**
 * Simple Descriptor para la aplicación. No se utiliza bindings, por lo tanto
 * el número de comandos de entrada y salida es establecido en 0.
 */
const SimpleDescriptionFormat_t zb_SimpleDesc = 
{
    MY_ENDPOINT_ID,             //  Endpoint
    MY_PROFILE_ID,              //  Profile ID
    DEV_ID_COLLECTOR,           //  Device ID
    DEVICE_VERSION_COLLECTOR,   //  Device version
    0,                          //  Reservado
    0,                          //  Nº de comandos de entrada
    (cId_t *) NULL,             //  Lista de comandos de entrada
    0,                          //  Nº de comandos de salida
    (cId_t *) NULL              //  Lista de comandos de salida
};

/*/****************************************************************************
 * FUNCIONES
 */

/**
 * @brief   Inicializa UART.
 * @param   pf  Puntero a la función callback del UART.
 */
void initUart(halUARTCBack_t pf)
{
    halUARTCfg_t uartConfig;
    
    uartConfig.configured           = TRUE;              
    uartConfig.baudRate             = HAL_UART_BR_19200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 48;
    uartConfig.rx.maxBufSize        = RX_BUF_LEN;
    uartConfig.tx.maxBufSize        = 128;
    
    uartConfig.idleTimeout          = 6;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = pf;
    
    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
}

/**
 * @brief   Esta función es llamada por el SO cuando se debe atender un
 *          evento de la tarea.
 * @param   event   Mascara de bits conteniendo los eventos a atender
 */
void zb_HandleOsalEvent(uint16 event)
{
    uint8 logicalType;
    
    if (event & SYS_EVENT_MSG)
    {      
    }
    if (event & ZB_ENTRY_EVENT)
    {  
        // inicializa UART
        initUart(uartRxCB);
        // blick LED 1 para indicar que se está uniendo a la red
        HalLedBlink(HAL_LED_1, 0, 50, 500 );
        HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);   
        // lee tipo de logical device desde la memoria NV
        zb_ReadConfiguration(ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &logicalType);
        // inicia la red
        zb_StartRequest();
    }
    if (event & MY_START_EVT)
    {
        // inicia la red
        zb_StartRequest();
    }
    if (event & SEND_UART_MSG)
    {
        // envia mensaje por UART a la PC
        //HalLcdWriteString("UART_EVT", HAL_LCD_LINE_1);
        sendUartMessage(&msgReport, &alarmFlags);
    }
    if (event & MY_TEST_EVT)
    {
        // crea mensaje de prueba
        msgReport.msgType = MSG_TYPE_REPORT;
        msgReport.sequence++;
        msgReport.lenData = MSG_LEN_DATA_REPORT;
        static uint8 m[8] = {0xBE, 0xBA, 0xCA, 0xFE, 0xEA, 0xEA, 0x0A, 0xFF};
        for (uint8 i = 0; i < 8; i++)
            msgReport.mac[i] = m[i];
        // datos de prueba
        static double d[10] = {220.5, 0.354, 85.12, 88.33, 85.06, 0.98, 1.23, 3.25, 0.97, 4.55};
        for (int i = 0; i < 10; i++)
            memcpy(&(msgReport.data[i*4]), &(d[i]), sizeof(double));
        // flags de prueba
        uint16 flags = 0;
        // imprime info en LCD
        HalLcdWriteString("TEST_EVT", HAL_LCD_LINE_1);
        HalLcdWriteStringValue("#", msgReport.sequence, 10, HAL_LCD_LINE_2);
        // envia mensaje por UART
        sendUartMessage(&msgReport, &flags);
        // crea evento nuevamente
        osal_start_timerEx(sapi_TaskID, MY_TEST_EVT, 5000);
    }
}

/**
 * @brief   Maneja los eventos de teclas, generados cuando ocurre un pulsación
 *          en el joystick de la placa.
 * @param   shift   true si shift/alt está presionado.
 * @param   keys    Mascara de bits para los eventos de teclado. Válidos:
 *                  EVAL_SW4, EVAL_SW3, EVAL_SW2, EVAL_SW1.
 */
void zb_HandleKeys(uint8 shift, uint8 keys)
{
    static uint8 allowJoin = TRUE;
    uint8 logicalType;
    
    if (keys & HAL_KEY_CENTER)
    {
        if (appState == APP_INIT)
        {
            // configura el dispositivo como coordinador
            logicalType = ZG_DEVICETYPE_COORDINATOR;
            zb_WriteConfiguration(ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &logicalType);
            zb_SystemReset();
        }
    }
    if (keys & HAL_KEY_UP)
    {
        // configura condición de máxima carga (I RMS) en pasos de 0.1 A, hasta 2 A
        if (alarm_max_irms > 2)
            alarm_max_irms = 0.05;
        else
            alarm_max_irms += 0.05;
        HalLcdWriteString("MAX I RMS (mA)", HAL_LCD_LINE_1);
        HalLcdWriteStringValue("", (int)(alarm_max_irms*1000), 10, 2);
    }
    if (keys & HAL_KEY_DOWN)    // joystick abajo
    {
        // configura condicion de máximo consumo (E) en pasos de 50 Ws, hasta 400 Ws
        if (alarm_max_energy > 400)
            alarm_max_energy = 20;
        else
            alarm_max_energy += 20;
        HalLcdWriteString("MAX ENERGIA (Ws)", HAL_LCD_LINE_1);
        HalLcdWriteStringValue("", alarm_max_energy, 10, 2);
    }
    if (keys & HAL_KEY_RIGHT)    // joystick derecha
    {
        // inicia evento para pruebas
        msgReport.sequence = 0;
        osal_start_timerEx(sapi_TaskID, MY_TEST_EVT, 5000);
    }
    if (keys & HAL_KEY_LEFT)    // joystick izquierda
    {
        // controla si el dispositivo acepta join requests
        // refleja este estado en el LED 3
        allowJoin ^= 1;
        if (allowJoin)
        {
            NLME_PermitJoiningRequest(0xFF);
            HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
        }
        else
        {
            NLME_PermitJoiningRequest(0);
            HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
        }
    }
}

/**
 * @brief   Este callback es llamado por el stack de ZigBee luego de que la 
 *          operación de Start Request es completada.
 * @param   status  El estado de la operación. ZB_SUCCESS indica que la
 *                  operación fue completada exitosamente. Sino, el estado es
 *                  un código de error.
 */
void zb_StartConfirm(uint8 status)
{ 
    // si el dispositivo se inicio correctamente, cambia el estado
    if (status == ZB_SUCCESS)
    {
        // enciende LED 1 para indicar que el coordinador inició la red
        HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
        // actualiza display LCD
        HalLcdWriteString("COORD. INICIADO", HAL_LCD_LINE_1);
        HalLcdWriteString("", HAL_LCD_LINE_2);
        // cambia estado de la app a iniciada
        appState = APP_START;
    }
    else
    {
        // reintenta iniciarse con un tiempo de espera
        osal_start_timerEx(sapi_TaskID, MY_START_EVT, myStartRetryDelay);
    }
}

/**
 * @brief   Este callback es llamado por el stack de ZigBee luego de que la
 *          operación de envío de datos es completada.
 * @param   handle  El handle que identifica la transmisión de datos.
 * @param   status  El estado de la operación.
 */
void zb_SendDataConfirm(uint8 handle, uint8 status)
{
    if (status != ZB_SUCCESS) 
    {
        if (++reportFailureNr >= REPORT_FAILURE_LIMIT)
        {   
            // máx. de envíos fallidos alcanzado, reinicia la red
            HalLcdWriteString("REINICIANDO...", HAL_LCD_LINE_1);
            osal_set_event(sapi_TaskID, MY_START_EVT);
            reportFailureNr = 0;
        }
    }
}

/**
 * @brief   Este callback es llamado asincrónicamente por el stack de ZigBee
 *          para notificar a la aplicación que se recibieron datos, enviados 
 *          por otro nodo de la red.
 * @param   source  Short Address (NWK) del nodo que envío los datos.
 * @param   command ID del comando asociado con los datos.
 * @param   len     Cantidad de bytes del parámetro pData.
 * @param   pData   Puntero al inicio de los datos envíados por el nodo.
 */
void zb_ReceiveDataIndication(uint16 source, uint16 command, uint16 len, uint8 *pData)
{ 
    // imprime la dirección fuente en el display
    HalLcdWriteStringValue("RCB", source, 16, 1);
    // flashea el LED 2 para indicar que se recibió un mensaje
    HalLedSet(HAL_LED_2, HAL_LED_MODE_FLASH);
    
    // deserealiza mensaje recibido
    msgReport = deserializeMessage(pData);
    
    if (msgReport.msgType == MSG_TYPE_REPORT)
    {
        HalLcdWriteStringValue("#", msgReport.sequence, 10, 2);
        // verifica condiciones de alarmas
        alarmFlags = checkAlarms(&msgReport);
        // imprime alarma en LCD
        //HalLcdWriteStringValue("ALARM", alarmFlags, 16, 2);
        
        if (alarmFlags != 0)
        {
            // crea paquete de respuesta con igual MAC y nº de secuencia
            struct message_t msgAlarm;
            msgAlarm.sequence = msgReport.sequence;
            memcpy(&(msgAlarm.mac), &(msgReport.mac), sizeof(msgReport.mac));
            msgAlarm.msgType = MSG_TYPE_ALARM;
            msgAlarm.lenData = MSG_LEN_DATA_ALARM;
            // flags en little-endian
            msgAlarm.data[0] = LO_UINT16(alarmFlags);
            msgAlarm.data[1] = HI_UINT16(alarmFlags);
            
            // serializa mensaje y lo envía
            serializeMessage(&(msgAlarm), msgBuf);
            zb_SendDataRequest(source, MESSAGE_CLUSTER,
                               getSizeOfMessage(&(msgAlarm)),
                               msgBuf, 0, AF_TX_OPTIONS_NONE, 0);
        }
        // crea evento para enviar el reporte por UART a la PC, añandiendo
        // los flags
        osal_start_timerEx(sapi_TaskID, SEND_UART_MSG, 0);
        HalLcdWriteStringValue("ALARM", alarmFlags, 16, 3);
    }
}

/**
 * @brief   Verifica las condiciones de alarmas para un mensaje de tipo REPORT
 *          y establece en 1 el flag correspondiente.
 * @param   msg     Puntero a mensaje de tipo REPORT.
 * @return  Entero de 16 bits (sin signo) con los flags correspondientes a cada
 *          alarma.
 */
static uint16 checkAlarms(struct message_t *msg)
{
    uint16 flags = 0;
    double param;

    // VRMS - sobretensión/bajatensión
    memcpy(&param, &(msg->data[MSG_VRMS_FIELD]), sizeof(double));
    if (param > V_RMS_OVERVOLTAGE)
        flags |= ALARM_FLAG_OVERVOLTAGE;
    else if (param < V_RMS_UNDERVOLTAGE)
        flags |= ALARM_FLAG_UNDERVOLTAGE;
    
    // distorsión 3º armónico de tensión
    memcpy(&param, &(msg->data[MSG_V_THD_3_FIELD]), sizeof(double));
    if (param > V_THD_3_THRESHOLD)
        flags |= ALARM_FLAG_MAX_V_THD_3;

    // distorsión 5º armónico de tensión
    memcpy(&param, &(msg->data[MSG_V_THD_5_FIELD]), sizeof(double));
    if (param > V_THD_5_THRESHOLD)
        flags |= ALARM_FLAG_MAX_V_THD_5;

    // potencia activa negativa
    memcpy(&param, &(msg->data[MSG_PACT_FIELD]), sizeof(double));
    if (param < 0)
        flags |= ALARM_FLAG_NEGATIVE_POWER;
    
    // corriente máxima
    memcpy(&param, &(msg->data[MSG_IRMS_FIELD]), sizeof(double));
    //if (param > I_RMS_MAX_LOAD)
    if (param > alarm_max_irms)
        flags |= ALARM_FLAG_MAX_LOAD;
    
    // consumo eléctrico máximo
    memcpy(&param, &(msg->data[MSG_ENERGY_FIELD]), sizeof(double));
    //if (param > MAX_ENERGY_CONSUMPTION)
    if (param > (double)alarm_max_energy)
        flags |= ALARM_FLAG_MAX_ENERGY;
    
    return flags;
}

/** No se usa binding */
void zb_BindConfirm(uint16 commandId, uint8 status) {}

/** No se usa binding */
void zb_AllowBindConfirm(uint16 source) {}

/** No se usa */
void zb_FindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result ) {}

/**
 * @brief   Función callback de UART, llamada cuando ocurre un evento.
 * @param   port    Puerto UART.
 * @param   event   Evento UART que causó el callback.
 */
void uartRxCB(uint8 port, uint8 event)
{
    // No se implementa.
}

/**
 * @brief   Envía un mensaje por UART según el protocolo implementado. Este se
 *          crea a partir de un mensaje y otros datos (genéricos) pasados como
 *          argumento. Para el mensaje de tipo REPORT, los otros datos
 *          corresponden a los flags de alarmas, y son añadidos al mensaje antes
 *          de los parámetros eléctricos.
 *          Los mensajes tienen el siguiente formato:
 *              HEADER                                     DATA
 *              SOF | DATA LENGTH | COMMAND | SENSOR MAC | DATA[DATA LENGTH]
 *          Donde SOF=0xFE es el delimitador del inicio del frame; DATA LENGTH 
 *          es la cantidad de bytes de datos envíados luego del HEADER; COMMAND
 *          es el comando enviado a la PC (REPORT, etc.); SENSOR MAC es la 
 *          dirección MAC de 64 bytes del nodo al cual se refiere la información;
 *          y DATA corresponde a los datos enviados en formato Little Endian.
 *
 * @param   msg     Mensaje que se desea retransmitir por UART.
 * @param   other   Otros datos para añadir al mensaje.
 */
static void sendUartMessage(struct message_t *msg, void *other)
{
    uint8 pFrame[UART_FRAME_MAX_LENGTH];

    // start of frame
    pFrame[UART_FRAME_SOF_OFFSET] = CPT_SOP;
    // tipo de mensaje
    pFrame[UART_FRAME_CMD_OFFSET] = msg->msgType;
    // MAC
    memcpy(&(pFrame[UART_FRAME_MAC_OFFSET]), &(msg->mac), sizeof(msg->mac));
    
    if (msg->msgType == MSG_TYPE_REPORT)
    {
        // length
        pFrame[UART_FRAME_LENGTH_OFFSET] = UART_FRAME_REPORT_LENGTH;
        
        // secuencia
        pFrame[UART_FRAME_DATA_OFFSET] = LO_UINT16(msg->sequence);
        pFrame[UART_FRAME_DATA_OFFSET+1] = HI_UINT16(msg->sequence);
        // flags alarmas
        uint16 flags = *((uint16*)other);
        pFrame[UART_FRAME_DATA_OFFSET+2] = LO_UINT16(flags);
        pFrame[UART_FRAME_DATA_OFFSET+3] = HI_UINT16(flags);
        // parámetros eléctricos
        memcpy(&(pFrame[UART_FRAME_DATA_OFFSET+4]), &(msg->data), msg->lenData);
    }
    else
    {
        // otros tipos de mensaje
        pFrame[UART_FRAME_LENGTH_OFFSET] = 0;
    }
    
    // escribe mensaje en UART
    HalUARTWrite(HAL_UART_PORT_0, pFrame,
                 UART_FRAME_HEADER_LENGTH+pFrame[UART_FRAME_LENGTH_OFFSET]);
}

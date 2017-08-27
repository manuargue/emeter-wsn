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

#ifndef MESSAGE_H
#define MESSAGE_H

#define MESSAGE_CLUSTER         0x0007
#define MAX_DATA_MESSAGE        40
#define LEN_HEADER              12        // longitud (en bytes) del HEADER

// Tipos de mensaje
#define MSG_TYPE_REPORT         0x01      // reporte enviado por nodo sensor
#define MSG_TYPE_ALARM          0x02      // alarma enviada por el coordinador
#define MSG_TYPE_INFO           0x03

// Cantidad de bytes en campo data[]
#define MSG_LEN_DATA_REPORT     40      
#define MSG_LEN_DATA_ALARM      2
#define MSG_LEN_DATA_NOTIFY     2

// Offset del dato en data[]
#define MSG_VRMS_FIELD          0       // REPORT, todos los datos son de tipo
#define MSG_IRMS_FIELD          1*4     // double de 4 bytes
#define MSG_PACT_FIELD          2*4
#define MSG_PAPP_FIELD          3*4
#define MSG_ENERGY_FIELD        4*4
#define MSG_FP_FIELD            5*4
#define MSG_V_THD_3_FIELD       6*4
#define MSG_V_THD_5_FIELD       7*4
#define MSG_I_THD_3_FIELD       8*4
#define MSG_I_THD_5_FIELD       9*4
#define MSG_ALARM_FLAGS_FIELD   0       // ALARM, único dato de tipo uint16

// Flags de alarmas
#define ALARM_FLAG_OVERVOLTAGE      0x0001
#define ALARM_FLAG_UNDERVOLTAGE     0x0002
#define ALARM_FLAG_V_THD_3          0x0004
#define ALARM_FLAG_V_THD_5          0x0008
#define ALARM_FLAG_NEGATIVE_POWER   0x0010
#define ALARM_FLAG_MAX_ENERGY       0x0020
#define ALARM_FLAG_MAX_LOAD         0x0040

struct message_t {
    // HEADER
    unsigned int sequence;      // nº de secuencia del paquete
    unsigned char mac[8];       // dirección MAC del nodo sensor (end-device)
    unsigned char msgType;      // tipo de mensaje (reporte, alarma, etc.)
    unsigned char lenData;      // cantidad de bytes en data[]
    // DATA
    unsigned char data[MAX_DATA_MESSAGE];   // datos en little-endian
};

void printMessage(struct message_t* im);
void serializeMessage(struct message_t* im, unsigned char* destinationPtr);
struct message_t deserializeMessage(unsigned char* source);
unsigned int getSizeOfMessage(struct message_t* im);

#endif

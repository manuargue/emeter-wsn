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

#include <string.h>
#include "message.h"

/**
 * @brief   Imprime el mensaje en la consola. 
 * @param   im  El mensaje a imprimir.
 * @todo    printf() no está implementado!
 */
void printMessage(struct message_t* im)
{   
    // header
    //printf("#%u MAC=", im->sequence);
    for (int j = 0; j < 8; j++) {
        //printf("%02X", im->mac[j]);
        //if (j != 7) printf(":");
    }
    //printf(" type=%02X, lenData=%u: ", im->msgType, im->lenData);
    
    // data
    if (im->msgType == MSG_TYPE_REPORT)
    {
        double d;
        for (int i = 0; i < MSG_LEN_DATA_REPORT; i+=4)
        {
            memcpy(&d, &(im->data[i]), sizeof(d));
            // printf es small format!
            //printf("[%i.%i] ", (int)d, (int)(d*100.0-(int)d*100));
        }
    }
    else if (im->msgType == MSG_TYPE_ALARM)
    {
        uint16 flags;
        memcpy(&flags, &(im->data[0]), sizeof(flags));
        //printf("[%04X] ", flags);
    }
    else
    {
        for (int i = 0; i < im->lenData; i++); // descomentar ;
            //printf("%04X ", im->data[i]);
    }
    //printf("\r\n");
}

/**
 * @brief   Copia el mensaje como un stream de bytes a la ubicación de memoria
 *          apuntada por destinationPtr. El número de bytes copiados es igual a
 *          getSizeOfInfoMessage().
 * @param   im              Mensaje a serializar.
 * @param   destinationPtr  Puntero a un región de memoria de al menos 
 *                          getSizeOfInfoMessage() bytes.
 */
void serializeMessage(struct message_t* im, uint8* destinationPtr)
{  
    // header
    *destinationPtr++ = im->sequence & 0xFF;  //LSB primero
    *destinationPtr++ = im->sequence >> 8;
    for (int i = 0; i < 8; i++)
        *destinationPtr++ = im->mac[i];
    *destinationPtr++ = im->msgType;
    *destinationPtr++ = im->lenData;
    // data
    for (int i = 0; i < im->lenData; i++)
        *destinationPtr++ = im->data[i];
}

/**
 * @brief   Crea un mensaje a partir de un stream de bytes apuntado por source.
 * @param   source  Puntero al comienzo del mensaje serializado.
 * @return  struct de tipo message_t creado con los datos parseados.
 * @todo    Verificar que lenData < MSG_MAX_DATA_LEN
 */
struct message_t deserializeMessage(uint8* source) {
    uint8* sourcePtr = source;
    struct message_t im;
    // header
    im.sequence = BUILD_UINT16( (*sourcePtr), (*(sourcePtr+1)) );
    sourcePtr += 2;
    for (int i = 0; i < 8; i++)
        im.mac[i] = *sourcePtr++;
    im.msgType = *sourcePtr++;
    im.lenData = *sourcePtr++;
    // data
    for (int i = 0; i < im.lenData; i++)
        im.data[i] = *sourcePtr++;

    return im;
}

/**
 * @return La longitud del mensaje en bytes, incluyendo el Header.
 */
uint16 getSizeOfMessage(struct message_t* im) { 
  return (MSG_LEN_HEADER + im->lenData);
}

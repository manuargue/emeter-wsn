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

#ifndef COLLECTOR_APP_H
#define COLLECTOR_APP_H

/******************************************************************************
 * CONSTANTES
 */

// Definiciones para el stack de ZigBee
#define MY_PROFILE_ID               0xD7D7  /**< ZigBee Profile ID de la aplicación */
#define MY_ENDPOINT_ID              0xD7    /**< ZigBee End Point ID de la aplicación */
#define DEV_ID_SENSOR               1   /**< ID del dispositivo Sensor */
#define DEV_ID_COLLECTOR            2   /**< ID del dispositivo Colector/Coordinador */
#define DEVICE_VERSION_SENSOR       1   /**< Versión del dispositivo Sensor */
#define DEVICE_VERSION_COLLECTOR    1   /**< Versión del dispositivo Colector/Coordinador */

#define RX_BUF_LEN                  128 /**< Tamaño del buffer de RX de UART */

// Condiciones de alarmas
#define V_RMS_OVERVOLTAGE           220*(1+0.1) // 220V + 10%
#define V_RMS_UNDERVOLTAGE          220*(1-0.1) // 220V - 10%
#define V_THD_3_THRESHOLD           5           // % con respecto fundamental
#define V_THD_5_THRESHOLD           6           // % con respecto fundamental
#define I_RMS_MAX_LOAD              1.0         // Ampere
#define MAX_ENERGY_CONSUMPTION      100         // Joules = Watt * seg

#endif // COLLECTOR_APP_H

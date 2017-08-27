/*/*********************************************************************
 * eMeter Logger
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

package ar.emeterlogger;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.sql.Timestamp;
import java.util.Date;

public class ReportMessage
{
	private Date time;
    private int sequence;
    // parámetros eléctricos
    private float[] params;
    // flags de alarmas
    private int alarmFlags;
    
    public ReportMessage(byte[] uartRx)
    {
    	// uartRx contiene los bytes recibidos por el puerto serie,
    	// en little-endian (LSB primero).
    	// 2b=uint16	SEQ
    	// 2b=uint16	FLAGALARMS
    	// 4b=float		VRMS, IRMS, ACTPOW, APPPOW, POWFACTOR, ENERGY,
    	// 				VTHD3, VTHD5, ITHD3, ITHD5
    	
    	this.time = new Date();
    	this.sequence = this.byteToUInt16(uartRx[1], uartRx[0]); 
    	this.alarmFlags = this.byteToUInt16(uartRx[3], uartRx[2]);
    	this.params = new float[10];
    	for (int i = 0; i < params.length; i++)
    	{
    		params[i] = ByteBuffer.wrap(uartRx, 4+i*4, 4).
    				order(ByteOrder.LITTLE_ENDIAN).getFloat(4+i*4);
    	}
    }
    
    /**
     * Convierte dos bytes a un int. Los dos bytes juntos son considerados
     * como un unsigned int de 16 bits, por lo que 0xFFFF = 65535.
     * @param hi	MSB
     * @param lo	LSB
     * @return
     */
    private int byteToUInt16(byte hi, byte lo)
    {
    	return ( ((hi & 0x000000FF) << 8) + (lo & 0x00FF) );
    }

	public Timestamp getTimestamp()
	{
		return new Timestamp(this.time.getTime());
	}

	public Date getTime()
	{
		return this.time;
	}
	
	public int getSequence()
	{	
		return this.sequence;
	}

	public int getAlarmFlags() {
		return this.alarmFlags;
	}

	public float[] getParameters() {
		return this.params;
	}
}

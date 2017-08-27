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

import java.util.Date;
import java.util.Formatter;


/**
 * Representa un nodo sensor de la red (ZigBee End Device), identificado con
 * su dirección MAC. 
 * 
 * @author Manuel Argüelles
 */
public class SensorNode
{
	/** Nombre del nodo. Puede ser el lugar dónde se encuentra ubicado
	 * (ej: cocina, living). */
    private String name = "";
    /** Dirección MAC del dispositivo. Se almacena como un String en hexadecimal
     * (ej: AB001122FF44FFCC). */
    private String mac = "";
    /** Estado del nodo mientras está aplicación está corriendo. */
    private NodeState state = NodeState.INACTIVE;
    private Date lastActivity = null;
    
    /**
     * Inicializa un objeto nodo.
     * 
     * @param mac		Dirección MAC.
     * @param name		Nombre del nodo.
     * @param state		Estado actual.
     */
    public SensorNode(String mac, String name, NodeState state)
    {
        this.mac = mac;
        this.setName(name);
        this.state = state;
    }
    
    /**
     * Inicializa el objeto nodo a partir de la dirección MAC pasada como un
     * arreglo de bytes (en formato Big-Endian) y con estado NodeState.ACTIVE.
     * 
     * @param macBytes		Arreglo con bytes de la dirección MAC (Big-Endian).
     * @param name			Nombre del nodo.
     */
    public SensorNode(byte[] macBytes, String name)
    {
    	this("", name, NodeState.ACTIVE);

    	// convierte MAC a string
    	Formatter formatter = new Formatter();
    	for (byte b : macBytes)
    		formatter.format("%02x", b);
    	this.mac = formatter.toString();
    	formatter.close();
    }
    
    /**
     * Establece un nuevo nombre para el nodo. Este se guarda en la base de
     * datos en un campo del tipo char(20), por lo que si el nombre pasado como
     * argumento supera este largo, es truncado.
     * 
     * @param newName	Nuevo nombre a asignar.
     */
    public void setName(String newName)
    {
        if (newName.length() > 20)
            // el nombre se almacena en SQL char(20)
            this.name = newName.substring(1, 20);
        else
            this.name = newName;
    }
    
    /**
     * @return	El nombre del nodo.
     */
    public String getName()
    {
        return this.name;
    }
    
    /**
     * @return String conteniendo la dirección MAC del nodo en formato
     * 			hexadecimal.
     */
    public String getMAC()
    {
        return this.mac;
    }
    
    /**
     * @return String conteniendo la dirección MAC del nodo en formato
     * 			hexadecimal, intercalando cada byte con el símbolo ':'.
     */
    public String getFormatMAC()
    {
        String formatMAC = "";
        for (int i = 0; i < this.mac.length(); i+=2)
        {
            formatMAC += this.mac.substring(i, i+2);
            if (i < 14)
                formatMAC += ":";
        }
        return formatMAC;
    }
 
    /**
     * @return true, si el nodo está activo; false, en caso contrario.
     */
    public boolean getState()
    {
        return this.state.toBoolean();
    }
    
    /**
     * Actualiza el estado del nodo.
     * 
     * @param state		Nuevo estado del nodo.
     */
    public void setState(NodeState state)
    {
    	this.state = state;
    }
       
    /**
     * @return String con el nombre de la tabla formado como 'node_' seguido
     * 			por los primeros 8 dígitos en hexadecimal de la MAC.
     */
    public String getTableName()
    {
        return ("node_" + mac.substring(0, 8));
    }

    /**
     * @return	Representación en String del nodo, indicando dirección MAC,
     * 			nombre y estado actual.
     */
    @Override
    public String toString()
    {
        String strOut = "Nodo '" + this.name + "', MAC=" + this.getFormatMAC() +
                        ", Estado:" + this.state.toString();
        return strOut;
    }

	public void setLastActivity(Date newTime)
	{
		this.lastActivity = newTime;
	}
	
	public Date getLastActivity()
	{
		return this.lastActivity;
	}
}

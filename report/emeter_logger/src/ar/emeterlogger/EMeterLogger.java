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

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Date;
import java.util.Enumeration;
import java.util.TooManyListenersException;

import gnu.io.*;


public class EMeterLogger implements Runnable, SerialPortEventListener
{

	private Thread readThread;
	private InputStream inputStream;
    private SerialPort serialPort;
	private CommPortIdentifier portId;
	private SQLHandler dao;
	
	final static String portName = "/dev/ttyUSB0";
	
	final static int HEADER_LENGTH = 11;
	final static int SOF_OFFSET = 0;
	final static int DATA_LENGTH_OFFSET = 1;
	final static int COMMAND_OFFSET = 2;
	final static int MAC_OFFSET = 3;
	final static int DATA_OFFSET = 11;
	
	public static void main(String[] args)
	{
		new EMeterLogger();
	}
    
    public EMeterLogger()
    {
    	// conexión con base de datos
    	this.dao = new SQLHandler();
        this.dao.connect();
        this.dao.createNodesTable();
    	
    	try
    	{
    		Enumeration portIdentifiers = CommPortIdentifier.getPortIdentifiers();
    		
    		CommPortIdentifier portId = null;  // will be set if port found
    		while (portIdentifiers.hasMoreElements())
    		{
    		    CommPortIdentifier pid = (CommPortIdentifier) portIdentifiers.nextElement();
    		    if(pid.getPortType() == CommPortIdentifier.PORT_SERIAL &&
    		       pid.getName().equals(portName)) 
    		    {
    		        portId = pid;
    		        break;
    		    }
    		}
    		if (portId == null)
    		{
    		    Debugger.log("Could not find serial port " + portName);
    		    System.exit(1);
    		}
    		this.portId = CommPortIdentifier.getPortIdentifier(portName);
			this.serialPort = (SerialPort) this.portId.open("eMeterLogger", 10000);
			Debugger.log("puerto " + portName + " abierto");
			this.serialPort.addEventListener(this);
			this.serialPort.notifyOnDataAvailable(true);
			this.serialPort.setSerialPortParams(
					19200,
					SerialPort.DATABITS_8,
			        SerialPort.STOPBITS_1,
			        SerialPort.PARITY_NONE);
			this.inputStream = this.serialPort.getInputStream();
			// hago un flush input para descartar la basura que haya en el buffer 
			while (this.inputStream.available() > 0)
				this.inputStream.read();
		}
    	catch (NoSuchPortException e)
    	{
    		Debugger.log("El puerto " + portName + " no existe");
			e.printStackTrace();
		}
    	catch (PortInUseException e)
    	{
    		Debugger.log("El puerto " + portName + " está en uso");
			e.printStackTrace();
		}
    	catch (IOException e)
    	{
			e.printStackTrace();
		}
    	catch (TooManyListenersException e)
    	{
			e.printStackTrace();
		}
    	catch (UnsupportedCommOperationException e)
    	{
			e.printStackTrace();
		}
		
    	// inicia el thread
		this.readThread = new Thread(this);
		this.readThread.start();
    }

	@Override
	public void serialEvent(SerialPortEvent evt) {
		switch (evt.getEventType())
		{
		case SerialPortEvent.BI:
        case SerialPortEvent.OE:
        case SerialPortEvent.FE:
        case SerialPortEvent.PE:
        case SerialPortEvent.CD:
        case SerialPortEvent.CTS:
        case SerialPortEvent.DSR:
        case SerialPortEvent.RI:
        case SerialPortEvent.OUTPUT_BUFFER_EMPTY:
            break;
        case SerialPortEvent.DATA_AVAILABLE:
        {       	
        	byte[] rxBuf = new byte[60];
        	SensorNode node;
            try
            {
            	if (this.inputStream.available() > 0)
            	{
	            	// lee HEADER. Lo implemento de esta forma porque por alguna
            		// razón no funciona el timeout
            		int n = 0;
            		while (n < HEADER_LENGTH)
            			n += this.inputStream.read(rxBuf, n, HEADER_LENGTH-n);
            		
	                if (n != HEADER_LENGTH)
	                {
	                	Debugger.log("Error leyendo HEADER. " +
	                				Integer.toString(n) + " bytes leidos");
	                }
	                else if (rxBuf[SOF_OFFSET] != (byte)0xFE)
	                {
	                	Debugger.log("Error SOF != 0xFE");
	                }
	                else
	                {
	                	// imprime HEADER
	                	Debugger.log("UART HEADER: " +
	                				bytesToHex(rxBuf, 0, HEADER_LENGTH));
	                	
	                	byte[] mac = Arrays.copyOfRange(rxBuf, MAC_OFFSET, DATA_OFFSET);
	                	// crea nodo sensor
	                	node = new SensorNode(mac, "sensor");
	                	// lo añade en la base de datos si es necesario
	                	this.dao.addNode(node);
	                	
	                	int cmd = (int)rxBuf[COMMAND_OFFSET];
	                	if (cmd == 1)
	                	{
			                // lee DATA
		                	int dataLen = (int)rxBuf[DATA_LENGTH_OFFSET];
		                	n = 0;
		            		while (n < dataLen)
		            			n += this.inputStream.read(rxBuf, n, dataLen-n);
		            		
			                if (n != dataLen)
			                {
			                	Debugger.log("Error leyendo DATA. " +
			                				Integer.toString(n) + " bytes leidos");
			                }
			                else
			                {
			                	// imprime DATA
			                	Debugger.log("UART DATA: " +
			                				bytesToHex(rxBuf, 0, dataLen));
			                	
			                	// crea objeto reporte
			                	byte[] data = Arrays.copyOfRange(rxBuf, 0, dataLen);
			                	ReportMessage report = new ReportMessage(data);
			                			                				                	
			                	// lo añade a la base de datos
			                	this.dao.insertLogReport(node, report);
			                }
	                	}
	                }
            	}
            }
            catch (IOException e)
            {
            	Debugger.log(e);
            }
            break;
        }
        }
	}

	@Override
	public void run()
	{
		try
		{
			Thread.sleep(2000);
		}
		catch (InterruptedException e)
		{
			Debugger.log("Interrupted Exception");
		}
	}
	
	final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();
	public static String bytesToHex(byte[] bytes, int offset, int len)
	{
	    char[] hexChars = new char[bytes.length * 2];
	    for (int j = offset; j < len; j++)
	    {
	        int v = bytes[j] & 0xFF;
	        hexChars[j * 2] = hexArray[v >>> 4];
	        hexChars[j * 2 + 1] = hexArray[v & 0x0F];
	    }
	    return new String(hexChars);
	}
}

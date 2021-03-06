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

import java.sql.DriverManager;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;


public class SQLHandler
{ 
    private final String dbAddress = "jdbc:mysql://localhost/";
    private final String dbName = "emeter";
    private final String dbUser = "";
    private final String dbPass = "";
    private Connection conn = null;
    private Statement st = null;
    
    /**
     * Crea la tabla de dispositivos conectados, si no existe.
     */
    public void createNodesTable()
    {
        try
        {
            String updateString = 
            		"CREATE TABLE IF NOT EXISTS " + this.dbName + ".nodes" +
                    "(mac bigint unsigned unique, name char(20), " +
                    "state bool);";
            
            this.st = this.conn.createStatement();
            this.st.executeUpdate(updateString);
            Debugger.log("Tabla " + this.dbName + ".nodes creada");
        }
        catch (SQLException e)
        {
            e.printStackTrace();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
        
    /**
     * Inserta un nuevo nodo en la tabla de dispositivos conectados. Si la MAC
     * del nodo ya existe, lo ignora. En caso contrario, lo añade a la tabla
     * y crea la tabla de log correspondiente al nodo.
     * 
     * @param node	Nodo a insertar en la tabla.
     */
    public void addNode(SensorNode node)
    {
        try
        {
            /* inserta el nuevo nodo en la tabla emeter.nodeslist. Si la MAC
             * ya existe en la tabla, la ignora.
             */
            String updateString = 
            		"INSERT IGNORE INTO " + this.dbName + ".nodes " +
                    "VALUES (x'" + node.getMAC() + "', '" + node.getName() +
                    "', " + Boolean.toString(node.getState()) + ");";
            
            this.st.executeUpdate(updateString);
            Debugger.log(node + " añadido");
            
            /* crea tabla de log para el nodo utilizando los últimos 4 bytes
             * de la MAC para el nombre. Si la tabla ya existe no se crea.
             */
            updateString = 
            		"CREATE TABLE IF NOT EXISTS " + 
            		this.dbName + "." + node.getTableName() +
            		"(time timestamp, sequence int unsigned, flag_alarm int unsigned, " +
            		"v_rms float, i_rms float, act_power float, app_power float, " +
            		"energy float, power_factor float, v_thd_3 float, v_thd_5 float, " +
            		"i_thd_3 float, i_thd_5 float);";

            this.st = this.conn.createStatement();
            this.st.executeUpdate(updateString);
            
            Debugger.log(
                "Tabla " + this.dbName + "." + node.getTableName() + " creada");
        }
        catch (SQLException e)
        {
            e.printStackTrace();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void insertLogReport(SensorNode node, ReportMessage report)
    {
    	try
    	{
    		String sqlSt = 
    				"INSERT INTO " + this.dbName + "." + node.getTableName() +
					" values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
			PreparedStatement st = this.conn.prepareStatement(sqlSt);

			st.setTimestamp(1, report.getTimestamp());	// timestamp
			st.setInt(2, report.getSequence());			// sequence			
			st.setInt(3, report.getAlarmFlags());		// flags de alarmas
			
			float[] params = report.getParameters();	// parámetros eléctricos
			for (int i = 0; i < params.length; i++)
				st.setFloat(i+4, params[i]);
			
			st.executeUpdate();
		}
    	catch (SQLException e)
    	{
			e.printStackTrace();
		}
    }
    
    /**
     * Actualiza en la base de datos el estado de un determinado nodo. 
     * 
     * @param node		Nodo a actualizar.
     */
    public void updateNodeState(SensorNode node)
    {
    	try
    	{
    		String updateString = 
        			"UPDATE emeter.nodes SET state = " + 
        			Boolean.toString(node.getState()) + " WHERE hex(mac)='" +
        			node.getMAC() + "';";
			
    		this.st = this.conn.createStatement();
			this.st.executeUpdate(updateString);
			
			Debugger.log(node);
		}
    	catch (SQLException e)
    	{
			e.printStackTrace();
		}    	
    }

    /**
     * Carga el driver de MySQL, se conecta con la URL de la base de datos,
     * y en caso de no existir, la crea.
     * 
     * @return -1 si el driver de MySQL no se pudo cargar; -2 si hubo un error
     * 			conectandose a la base de datos; 0 en caso de éxito. 
     */
    public int connect()
    {
        try
        {
            // carga el driver de MySQL
	        Class.forName("com.mysql.jdbc.Driver");
        }
        catch (ClassNotFoundException e)
        {
	        Debugger.log("No se pudo cargar el driver de MySQL");
	        return -1;
        }
        Debugger.log("Driver de MySQL cargado");

        try
        {
        	// se conecta con la URL
	        this.conn = DriverManager.
	        		getConnection(this.dbAddress, this.dbUser, this.dbPass);
	        
	        // crea la base de datos si no existe
	        String updateString = 
	        		"CREATE DATABASE IF NOT EXISTS " + this.dbName + ";";
	        this.st = this.conn.createStatement();
	        st.executeUpdate(updateString);
        }
        catch (SQLException e)
        {
            e.printStackTrace();
	        return -2;
        }

        if (this.conn != null)
        {
	        Debugger.log("Conexión con la base de datos exitosa");
        }
        else
        {
	        Debugger.log("Falló al conectarse con la URL " + this.dbAddress);
	        return -2;
        }

        return 0;
    }
    
    /**
     * Cierra la conexión con la base de datos.
     */
    public void close()
    {
        try
        {
            this.st.close();
            this.conn.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        Debugger.log("Conexión con " + dbAddress + " cerrada");
    }
}

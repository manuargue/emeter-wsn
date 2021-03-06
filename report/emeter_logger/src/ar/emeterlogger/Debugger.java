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

/**
 * Clase simple para imprimir mensajes de debug en la consola.
 */
public class Debugger
{
	private static boolean enabled = true;
	
    public static void enabled(boolean state)
    {
        enabled = state;
    }

    public static void log(Object o)
    {
    	if (enabled)
    	{
            System.out.println("[i] " + o.toString());
        }
    }
}

package com.xperia64.rompatcher.javapatchers.nsmbe;

import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.DSFile;
/*
*   This file is part of NSMB Editor 5.
*
*   NSMB Editor 5 is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   NSMB Editor 5 is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with NSMB Editor 5.  If not, see <http://www.gnu.org/licenses/>.
*/

    public class AlreadyEditingException extends Exception
    {
        /**
		 * 
		 */
		private static final long serialVersionUID = 1L;
		public DSFile f;
        public AlreadyEditingException(DSFile f)
        {
        	super("Already editing file: " + f.name());
            this.f = f;
        }

    }

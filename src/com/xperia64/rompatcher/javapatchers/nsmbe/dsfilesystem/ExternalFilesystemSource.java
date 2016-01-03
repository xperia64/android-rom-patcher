package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.FileStream;
import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.Stream;

import java.io.FileNotFoundException;
import java.io.IOException;

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


    public class ExternalFilesystemSource extends FilesystemSource
    {
        public String fileName;

        public ExternalFilesystemSource(String n)
        {
            this.fileName = n;
        }
        @Override
        public Stream load() throws FileNotFoundException
        {
            s = new FileStream(fileName);
            return s;
        }
        
        @Override
        public void save()
        {
            //just do nothing, any modifications are directly written to disk
        }
        @Override
        public void close()
        {
            try {
				s.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }
        @Override
        public String getDescription()
        {
            return fileName;
        }
    }

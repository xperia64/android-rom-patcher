package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.ROM;
import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.MemoryStream;
import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.Stream;

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


    class FileFilesystemSource extends FilesystemSource
    {

        DSFile f;
        MemoryStream str;
        boolean lz;

        public FileFilesystemSource(DSFile f, boolean compressed)
        {
            this.f = f;
            lz = compressed;
        }
        
        @Override
        public Stream load() throws Exception
        {
            f.beginEdit(this);
            str = new MemoryStream();
            byte[] data = f.getContents();
            if (lz)
                data = ROM.LZ77_Decompress(data);

            str.write(data, 0, data.length);

            return str;
        }
        
        @Override
        public void save() throws Exception
        {
            byte[] data = str.array();

            if (lz)
                data = ROM.LZ77_Compress(data, false);
            
            f.replace(data, this);
        }

        @Override
        public void close() throws Exception
        {
            save();
            str.close();
            f.endEdit(this);
        }

        @Override
        public String getDescription()
        {
            return f.name();
        }

    }


package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.MemoryStream;

import java.io.ByteArrayInputStream;

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



    public class NitroFilesystem extends PhysicalFilesystem
    {
        public PhysicalFile fatFile, fntFile;

        public NitroFilesystem(FilesystemSource s) throws Exception
        {
        	super(s);
            mainDir = new DSDirectory(this, null, true, "FILESYSTEM ["+s.getDescription()+"]", -100);
            load();
        }

        public void load() throws Exception
        {
            addDir(mainDir);

            addDSFile(fntFile);
            mainDir.childrenDSFiles.add(fntFile);
            addDSFile(fatFile);
            mainDir.childrenDSFiles.add(fatFile);

            freeSpaceDelimiter = fntFile;

            //read the fnt
            MemoryStream.SeekableByteArrayInputStream fnt = new MemoryStream.SeekableByteArrayInputStream(fntFile.getContents());
            
            loadDir(fnt, "root", 0xF000, mainDir);
            
        }

        public static long readUint(ByteArrayInputStream b)
        {
        	long res = 0;
        	for (int i = 0; i < 4; i++)
            {
                res |= b.read() << 8 * i;
            }
        	return res;
        }
        public static int readUshort(ByteArrayInputStream b)
        {
        	int res = 0;
        	for (int i = 0; i < 2; i++)
            {
                res |= b.read() << 8 * i;
            }
        	return res;
        }
        private void loadDir(MemoryStream.SeekableByteArrayInputStream fnt, String dirName, int dirID, DSDirectory parent) throws Exception
        {
            long pointer = fnt.getFilePointer();
            fnt.seek(8 * (dirID & 0xFFF));
            long subTableOffs = readUint(fnt);

            int fileID = readUshort(fnt);

            //Crappy hack for MKDS course .carc's. 
            //Their main dir starting ID is 2, which is weird...
          //  if (parent == mainDir) fileID = 0; 

            DSDirectory thisDir = new DSDirectory(this, parent, false, dirName, dirID);
            addDir(thisDir);
            parent.childrenDirs.add(thisDir);

            fnt.seek((int)subTableOffs);
            while (true)
            {
                byte data = (byte) fnt.read();
                int len = data & 0x7F;
                boolean isDir = (data & 0x80) != 0;
                if (len == 0)
                    break;
                byte[] tmp = new byte[len];
                fnt.read(tmp);
                String name = new String(tmp, "US-ASCII");

                if (isDir)
                {
                    int subDirID = readUshort(fnt);
                    loadDir(fnt, name, subDirID, thisDir);
                }
                else
                {
                    loadFile(name, fileID, thisDir);
                    fileID++;
                }
            }
            fnt.seek(pointer);
        }

        protected void loadNamelessFiles(DSDirectory parent) throws Exception
        {
            boolean ok = true;
            for (int i = 0; i < fatFile.fileSize() / 8; i++)
            {
                if (getDSFileById(i) == null)
                    ok = false;
            }

            if (ok) return;

            DSDirectory d = new DSDirectory(this, parent, true, "Unnamed files", -94);
            parent.childrenDirs.add(d);
            allDirs.add(d);

            for (int i = 0; i < fatFile.fileSize() / 8; i++)
            {
                if (getDSFileById(i) == null)
                    loadFile("File " + i, i, d);
            }
        }

        protected DSFile loadFile(String fileName, int fileID, DSDirectory parent) throws Exception
        {
            int beginOffs = fileID * 8;
            int endOffs = fileID * 8 + 4;
            DSFile f = new PhysicalFile(this, parent, fileID, fileName, fatFile, beginOffs, endOffs);
            parent.childrenDSFiles.add(f);
            addDSFile(f);
            return f;

        }
    }

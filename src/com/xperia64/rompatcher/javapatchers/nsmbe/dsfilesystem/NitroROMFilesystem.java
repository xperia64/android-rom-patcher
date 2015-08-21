package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.ROM;

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

//using System;
//using System.Collections.Generic;
//using System.Text;
//using System.IO;


    public class NitroROMFilesystem extends NitroFilesystem
    {
        public PhysicalFile arm7binFile, arm7ovFile, arm9ovFile, bannerFile;
        public PhysicalFile arm9binFile;
        public PhysicalFile rsaSigFile;
        public HeaderFile headerFile;
        private String filename;

        public NitroROMFilesystem(String n) throws Exception
        {
        	super((new ExternalFilesystemSource(n)));
            filename = n;
        }
        @Override
        public String getRomPath()
        {
            return filename;
        }
        
        @Override
        public void load() throws Exception
        {
            headerFile = new HeaderFile(this, mainDir);

            fntFile = new PhysicalFile(this, mainDir, -1, "fnt.bin", headerFile, 0x40, 0x44, true);
            fatFile = new PhysicalFile(this, mainDir, -2, "fat.bin", headerFile, 0x48, 0x4C, true);

            super.load();

            arm9ovFile = new PhysicalFile(this, mainDir, -3, "arm9ovt.bin", headerFile, 0x50, 0x54, true);
            arm7ovFile = new PhysicalFile(this, mainDir, -4, "arm7ovt.bin", headerFile, 0x58, 0x5C, true);
            //            arm9binFile = new Arm9BinFile(this, mainDir, headerFile);
            //            DSFile arm9binFile2 = new PhysicalFile(this, mainDir, true, -2, "arm9.bin", headerFile, 0x20, 0xC, true);
            arm9binFile = new PhysicalFile(this, mainDir, -5, "arm9.bin", headerFile, 0x20, 0x2C, true);
            arm9binFile.alignment = 0x1000;
            arm9binFile.canChangeOffset = false;
            arm7binFile = new PhysicalFile(this, mainDir, -6, "arm7.bin", headerFile, 0x30, 0x3C, true);
            arm7binFile.alignment = 0x200; //Not sure what should be used here...
            bannerFile = new BannerFile(this, mainDir, headerFile);
            bannerFile.alignment = 0x200; //Not sure what should be used here...

            long rsaOffs = headerFile.getUintAt(0x1000);

            if (rsaOffs == 0)
            {
                rsaOffs = headerFile.getUintAt(0x80);
                headerFile.setUintAt(0x1000, rsaOffs);
            }

            rsaSigFile = new PhysicalFile(this, mainDir, -7, "rsasig.bin", (int)rsaOffs, 136);
            rsaSigFile.canChangeOffset = false;

            addDSFile(headerFile);
            mainDir.childrenDSFiles.add(headerFile);
            addDSFile(arm9ovFile);
            mainDir.childrenDSFiles.add(arm9ovFile);
            addDSFile(arm7ovFile);
            mainDir.childrenDSFiles.add(arm7ovFile);
            addDSFile(arm9binFile);
            mainDir.childrenDSFiles.add(arm9binFile);
            addDSFile(arm7binFile);
            mainDir.childrenDSFiles.add(arm7binFile);
            addDSFile(bannerFile);
            mainDir.childrenDSFiles.add(bannerFile);
            addDSFile(rsaSigFile);
            mainDir.childrenDSFiles.add(rsaSigFile);

            loadOvTable("overlay7", -99, mainDir, arm7ovFile);
            loadOvTable("overlay9", -98, mainDir, arm9ovFile);
            loadNamelessFiles(mainDir);
        }

        private void loadOvTable(String dirName, int id, DSDirectory parent, DSFile table) throws Exception
        {
        	DSDirectory dir = new DSDirectory(this, parent, true, dirName, id);
            addDir(dir);
            parent.childrenDirs.add(dir);

            ByteArrayInputStream tbl = new ByteArrayInputStream(table.getContents());

            //int i = 0;
            while (tbl.read()>=0)
            {
            	tbl.reset();
                long ovId = readUint(tbl);
                /*long ramAddr = */readUint(tbl);
                /*long ramSize = */readUint(tbl);
                /*long bssSize = */readUint(tbl);
                /*long staticInitStart = */readUint(tbl);
                /*long staticInitEnd = */readUint(tbl);
                int fileID = readUshort(tbl);
                tbl.skip(6); //unused 0's
				
                /*DSFile f = */loadFile(dirName+"_"+ovId+".bin", fileID, dir);
                //i++;
                tbl.mark(0);
            }
        }
        /*private void loadOvTable(String dirName, int id, DSDirectory parent, DSFile table)
        {
            DSDirectory dir = new DSDirectory(this, parent, true, dirName, id);
            addDir(dir);
            parent.childrenDirs.add(dir);

            ByteArrayInputStream tbl = new ByteArrayInputStream(table.getContents());

            int i = 0;
            while (tbl.lengthAvailable(32))
            {
                long ovId = tbl.readUInt();
                long ramAddr = tbl.readUInt();
                long ramSize = tbl.readUInt();
                long bssSize = tbl.readUInt();
                long staticInitStart = tbl.readUInt();
                long staticInitEnd = tbl.readUInt();
                int fileID = tbl.readUShort();
                tbl.skip(6); //unused 0's

                DSFile f = loadFile(dirName+"_"+ovId+".bin", fileID, dir);
//                f.isSystemFile = true;

                i++;
            }
        }*/
        @Override
        public void DSFileMoved(DSFile f) throws Exception
        {
            if (!ROM.dlpMode)
            {
                long end = (long)getFilesystemEnd();
                headerFile.setUintAt(0x80, end);
                headerFile.UpdateCRC16();
            }
        }
    }


package com.xperia64.rompatcher.javapatchers.nsmbe;
import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.DSFile;

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




    public class Overlay
    {
		public DSFile f;
        private DSFile ovTableDSFile;
        private long ovTableOffs;
        
        public long ovId() throws Exception { return ovTableDSFile.getUintAt((int)ovTableOffs + 0x00); }
        public long ramAddr() throws Exception { return ovTableDSFile.getUintAt((int)ovTableOffs + 0x04); }
        public long ramSize() throws Exception { return ovTableDSFile.getUintAt((int)ovTableOffs + 0x08); }
        public long bssSize() throws Exception { return ovTableDSFile.getUintAt((int)ovTableOffs + 0x0C); }
        public long staticInitStart() throws Exception { return ovTableDSFile.getUintAt((int)ovTableOffs + 0x10); }
        public long staticInitEnd() throws Exception { return ovTableDSFile.getUintAt((int)ovTableOffs + 0x14); }

        public boolean isCompressed()
        {
            //get
            //{
                byte b = 0;
				try {
					b = ovTableDSFile.getByteAt((int)ovTableOffs + 0x1F);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
                return (b & 0x1) != 0;
            //}
            //set
            //{
                
            //}
        }
        public void setCompressed(boolean value) throws Exception
        {
        	byte b = ovTableDSFile.getByteAt((int)ovTableOffs + 0x1F);
            b &= 0xFE; // clear bit
            if(value)
                b |= 0x1;
            ovTableDSFile.setByteAt((int)ovTableOffs + 0x1F, b);
        }

        public Overlay(DSFile file, DSFile ovTableDSFile, long ovTableOffs)
        {
        	this.f = file;
            this.ovTableDSFile = ovTableDSFile;
            this.ovTableOffs = ovTableOffs;
        }

        public byte[] getDecompressedContents()
        {
            byte[] data = null;
			try {
				data = f.getContents();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
            if (isCompressed())
                data = ROM.DecompressOverlay(data);
            return data;
        }

        public void decompress() throws Exception
        {
            if (isCompressed())
            {
                byte[] data = f.getContents();
                data = ROM.DecompressOverlay(data);
                f.beginEdit(this);
                f.replace(data, this);
                f.endEdit(this);
                setCompressed(false);
            }
        }

        public boolean containsRamAddr(int addr)
        {
            try {
				return addr >= ramAddr() && addr < ramAddr() + ramSize();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			return false;
        }

        public long readFromRamAddr(int addr) throws Exception
        {
            decompress();

            addr -= (int)ramAddr();
            return f.getUintAt(addr);
        }

        public void writeToRamAddr(int addr, long val) throws Exception
        {
            decompress();
            addr -= (int)ramAddr();
            f.setUintAt(addr, val);
        }
    }

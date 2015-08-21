package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

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

	public abstract class DSFile
	{
        private DSFilesystem parentP;
        public DSFilesystem parent() { return parentP; }

        private DSDirectory parentDirP;
        public DSDirectory parentDir() { return parentDirP; }

        protected String nameP;
        public String name() { return nameP; }

        private int idP;
        public int id() { return idP; }
        public boolean isSystemFile() { return idP<0; }

        protected int fileSizeP;
        public int fileSize() { return fileSizeP; }
		
		public DSFile() {}
		
		public DSFile(DSFilesystem parent, DSDirectory parentDir, String name, int id)
		{
			this.parentP = parent;
			this.parentDirP = parentDir;
			this.nameP = name;
			this.idP = id;
		}

		//File functions
        public abstract byte[] getContents() throws IOException;
        public abstract void replace(byte[] newFile, Object editor) throws Exception;
		public abstract byte[] getInterval(int start, int end) throws Exception;
        public abstract void replaceInterval(byte[] newFile, int start) throws Exception;
		
		//Handy read/write functions.
        public long getUintAt(int offset) throws Exception
        {
        	byte[] data = getInterval(offset, offset+4);
        	long ret = 0;
        	ret |= (((data[0]&0xFF) | ((data[1]&0xFF)<<8) | ((data[2]&0xFF)<<16) | ((data[3]&0xFF)<<24)));
        	return ret;
        }

        public int getUshortAt(int offset) throws Exception
        {
        	byte[] data = getInterval(offset, offset+2);
        	return (int)((data[0]&0xFF) | ((data[1]&0xFF)<<8));
        }
        public byte getByteAt(int offset) throws Exception
        {
        	byte[] data = getInterval(offset, offset+1);
        	return (byte)((data[0]&0xFF));
        }

        public void setUintAt(int offset, long val) throws Exception
        {
        	byte[] data = {(byte)(val&0xFF), (byte)((val>>8)&0xFF), (byte)((val>>16)&0xFF), (byte)((val>>24)&0xFF)};
        	beginEditInterval(offset, offset+data.length);
        	//System.out.println("Replacing uint at: "+offset );
        	replaceInterval(data, offset);
        	endEditInterval(offset, offset+data.length);
        }
        
        public void setUshortAt(int offset, int val) throws Exception
        {
        	byte[] data = {(byte)(val), (byte)(val>>8)};
        	beginEditInterval(offset, offset+data.length);
        	//System.out.println("Replacing ushort at: "+offset );
        	replaceInterval(data, offset);
        	endEditInterval(offset, offset+data.length);
        }

        public void setByteAt(int offset, byte val) throws Exception
        {
        	byte[] data = {(byte)(val)};
        	beginEditInterval(offset, offset+data.length);
        	//System.out.println("Replacing byte at: "+offset );
        	replaceInterval(data, offset);
        	endEditInterval(offset, offset+data.length);
        }
        
        
        //Lock/unlock functions
        public abstract void beginEdit(Object editor) throws Exception;
        public abstract void endEdit(Object editor) throws Exception;
        public abstract void beginEditInterval(int start, int end) throws Exception;
        public abstract void endEditInterval(int start, int end) throws Exception;
        public abstract boolean beingEditedBy(Object editor) throws Exception;
        
        //Misc functions
        public String getPath()
        {
            return parentDir().getPath() + "/" + name();
        }

		protected void validateInterval(int start, int end) throws Exception
		{
			if(end < start)
				throw new Exception("Wrong interval: end < start");
//			Console.Out.WriteLine("Checking interval "+start+" - " +end +" on "+name);
			if(start < 0 || start > fileSize())
				throw new Exception("Wrong interval: start out of bounds");
			if(end < 0 || end > fileSize())
				throw new Exception("Wrong interval: end out of bounds");
		}
	}


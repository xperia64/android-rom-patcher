package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.Stream;

import java.io.IOException;
/*
*   This file instanceof part of NSMB Editor 5.
*
*   NSMB Editor 5 instanceof free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   NSMB Editor 5 instanceof distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with NSMB Editor 5.  If not, see <http://www.gnu.org/licenses/>.
*/
import java.util.Collections;
import java.util.Comparator;

    public class PhysicalFile extends FileWithLock implements Comparable<Object>
    {
        public boolean isSystemFile;

		//File that specifies where the file begins.
        protected DSFile beginFile;
        protected int beginOffset;
        
        //File that specifies where the file ends OR the file size.
        protected DSFile endFile;
        protected int endOffset;
        protected boolean endIsSize;
        
        //If true, file begin/size can't change at all.
        //TODO: Make sure these are set properly. I think they aren't.
        public boolean canChangeOffset = true;
        public boolean canChangeSize = true;

		//File begin offset
        public int fileBeginP;
        public int fileBegin() { return fileBeginP; }

        public int alignment = 4; // word align by default
        private Comparator<DSFile> dcomp = new Comparator<DSFile>(){
      	     public int compare(DSFile o1, DSFile o2){
      	    	 PhysicalFile f1 = (PhysicalFile) o1;
      	    	 PhysicalFile f2 = (PhysicalFile) o2;
      	            if (f1.fileBegin() == f2.fileBegin())
      	                return f1.fileSize()>f2.fileSize()?1:-1;
      	            return f1.fileBegin()>f2.fileBegin()?1:-1;
      	    	
      	     	}
           };
        //For convenience
        public Stream filesystemStream() { return ((PhysicalFilesystem)parent()).s; }
        public int filesystemDataOffset() { return ((PhysicalFilesystem)parent()).fileDataOffset(); }
        
        public PhysicalFile(DSFilesystem parent, DSDirectory parentDir, String name)
        {
        	super(parent, parentDir, name, -1);
        }
    
        public PhysicalFile(DSFilesystem parent, DSDirectory parentDir, int id, String name, DSFile alFile, int alBeg, int alEnd)
        {
        	super(parent, parentDir, name, id);
            this.beginFile = alFile;
            this.endFile = alFile;
            this.beginOffset = alBeg;
            this.endOffset = alEnd;
            try {
				refreshOffsets();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }

        public PhysicalFile(DSFilesystem parent, DSDirectory parentDir, int id, String name, DSFile alFile, int alBeg, int alEnd, boolean endsize)
        {
        	super(parent, parentDir, name, id);
            this.beginFile = alFile;
            this.endFile = alFile;
            this.beginOffset = alBeg;
            this.endOffset = alEnd;
            this.endIsSize = endsize;
            try {
				refreshOffsets();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }

        public PhysicalFile(DSFilesystem parent, DSDirectory parentDir, int id, String name, int alBeg, int alSize)
        {
        	super(parent, parentDir, name, id);
            this.fileBeginP = alBeg;
            this.fileSizeP = alSize;
            this.canChangeOffset = false;
            this.canChangeSize = false;
            try {
				refreshOffsets();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }

        public void refreshOffsets() throws Exception
        {
        	
            if (beginFile != null)
            {
                fileBeginP = (int)(beginFile.getUintAt(beginOffset)) + filesystemDataOffset();
            }
            if (endFile != null)
            {
                long end = endFile.getUintAt(endOffset);
                if (endIsSize)
                    fileSizeP = (int)end;
                else
                    fileSizeP = (int)end + filesystemDataOffset() - fileBegin();
            }
        }

        public void saveOffsets() throws Exception
        {
            if (beginFile != null)
                beginFile.setUintAt(beginOffset, (long)(fileBegin() - filesystemDataOffset()));

            if (endFile != null)
                if (endIsSize)
                    endFile.setUintAt(endOffset, (long)fileSize());
                else
                    endFile.setUintAt(endOffset, (long)(fileBegin() + fileSize() - filesystemDataOffset()));
        }
	
		//Reading and writing!
        @Override
		public byte[] getInterval(int start, int end) throws Exception
		{
			validateInterval(start, end);
			
			int len = end - start;
            byte[] file = new byte[len];
            filesystemStream().seek(fileBegin()+start);
            filesystemStream().read(file, 0, len);
            return file;
		}

        @Override
        public byte[] getContents() throws IOException
        {
            byte[] file = new byte[fileSize()];
            filesystemStream().seek(fileBegin());
            filesystemStream().read(file, 0, file.length);
            return file;
        }
        
        @Override
        public void replaceInterval(byte[] newFile, int start) throws Exception
		{
			validateInterval(start, start+newFile.length);
			boolean contains = false;
			for(Interval i : editedIntervals)
			{
				if(i.end==start+newFile.length && i.start ==start)
				{
					contains = true;
					break;
				}
			}
			if(!contains && editedBy == null)
				throw new Exception("NOT CORRECT EDITOR " + name());
            filesystemStream().seek(fileBegin()+start);
            filesystemStream().write(newFile, 0, newFile.length);
		}
		
		//TODO: Clean up this mess.
        @Override
        public void replace(byte[] newFile, Object editor) throws Exception
        {
            if(!isAGoodEditor(editor))
                throw new Exception("NOT CORRECT EDITOR " + name());

            if(newFile.length != fileSize() && !canChangeSize)
                throw new Exception("TRYING TO RESIZE CONSTANT-SIZE FILE: " + name());

            int newStart = fileBegin();
            //if we insert a bigger file it might not fit in the current place
            if (newFile.length > fileSize()) 
            {
                if (canChangeOffset && !(parent() instanceof NarcFilesystem))
                {
                    newStart = ((PhysicalFilesystem)parent()).findFreeSpace(newFile.length, alignment);
                    if (newStart % alignment != 0)
                    {
                    	newStart += alignment - newStart % alignment;
                    }
                }
                else
                {
                	//TODO: Keep the list always sorted in order to avoid stupid useless sorts.
                    Collections.sort(parent().allDSFiles, dcomp);
                    if (!(parent().allDSFiles.indexOf(this) == parent().allDSFiles.size() - 1))
                    {
                        PhysicalFile nextFile = (PhysicalFile) parent().allDSFiles.get(parent().allDSFiles.indexOf(this) + 1);
                        ((PhysicalFilesystem)parent()).moveAllFiles(nextFile, fileBegin() + newFile.length);
                    }
                }
            }
            //This instanceof for keeping NARC filesystems compact. Sucks.
            else if(parent() instanceof NarcFilesystem)
            {
                Collections.sort(parent().allDSFiles, dcomp);
                if (!(parent().allDSFiles.indexOf(this) == parent().allDSFiles.size() - 1))
                {
                    PhysicalFile nextFile = (PhysicalFile) parent().allDSFiles.get(parent().allDSFiles.indexOf(this) + 1);
                    ((PhysicalFilesystem)parent()).moveAllFiles(nextFile, fileBegin() + newFile.length);
                }
            }
            
            //Stupid check.
            if (newStart % alignment != 0)
                System.out.print("Warning: DSFile is not being aligned: " + name() + ", at " + String.format("%x",newStart));
            
            //write the file
            filesystemStream().seek(newStart);
            filesystemStream().write(newFile, 0, newFile.length);
            
            //This should be handled in NarcFilesystem instead, in fileMoved (?)
            if(parent() instanceof NarcFilesystem)
            {
            	PhysicalFile lastFile = (PhysicalFile) parent().allDSFiles.get(parent().allDSFiles.size() - 1);
                filesystemStream().setLength(lastFile.fileBegin() + lastFile.fileSize() + 16);
			}
			
            //update ending pos
            fileBeginP = newStart;
            fileSizeP = newFile.length;
            saveOffsets();

			//Updates total used rom size in header, and/or other stuff.
            parent().DSFileMoved(this); 
        }

        public void moveTo(int newOffs) throws Exception
        {
            if (newOffs % alignment != 0)
                System.out.print("Warning: DSFile is not being aligned: " + name() + ", at " + String.format("%x",newOffs));

            byte[] data = getContents();
            
            filesystemStream().seek(newOffs);
            filesystemStream().write(data, 0, data.length);

            fileBeginP = newOffs;
            saveOffsets();
        }
		

		//Misc crap
		
        @Override
        public int compareTo(Object obj)
        {
            PhysicalFile f = (PhysicalFile) obj;
            if (fileBegin() == f.fileBegin())
            {
            	if(fileSize() == f.fileSize())
            	{
            		return 0;
            	}
                return fileSize()>f.fileSize()?(1):(-1);
            }
            return fileBegin()>f.fileBegin()?(1):(-1);
        }

        public boolean isAddrInFdile(int addr)
        {
            return addr >= fileBegin() && addr < fileBegin() + fileSize();
        }
    }

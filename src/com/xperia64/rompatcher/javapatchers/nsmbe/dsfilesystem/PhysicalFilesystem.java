package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.Stream;

import java.io.FileWriter;
import java.io.IOException;
import java.util.Collections;
import java.util.Comparator;

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



    public abstract class PhysicalFilesystem extends DSFilesystem
    {
        protected FilesystemSource source;
        Stream s;
        protected DSFile freeSpaceDelimiter;

        // I think this is correct
        private Comparator<DSFile> dcomp = new Comparator<DSFile>(){
   	     public int compare(DSFile o1, DSFile o2){
   	    	 PhysicalFile f1 = (PhysicalFile) o1;
   	    	 PhysicalFile f2 = (PhysicalFile) o2;
   	            if (f1.fileBegin() == f2.fileBegin())
   	                return f1.fileSize()>f2.fileSize()?1:-1;
   	            return f1.fileBegin()>f2.fileBegin()?1:-1;
   	    	
   	     	}
        };
        protected int fileDataOffsetP;
        public int fileDataOffset() { return fileDataOffsetP; }

        protected PhysicalFilesystem(FilesystemSource fs) throws Exception
        {
            this.source = fs;
            this.s = source.load();
        }

        public boolean done = false;
        //Tries to find LEN bytes of continuous unused space AFTER the freeSpaceDelimiter (usually fat or fnt)
        public int findFreeSpace(int len, int align)
        {
        	Collections.sort(allDSFiles, dcomp);
            PhysicalFile bestSpace = null;
            int bestSpaceLeft = Integer.MAX_VALUE;
            int bestSpaceBegin = -1;

            for (int i = allDSFiles.indexOf(freeSpaceDelimiter); i < allDSFiles.size() - 1; i++)
            {
            	PhysicalFile a = (PhysicalFile) allDSFiles.get(i);
            	PhysicalFile b = (PhysicalFile) allDSFiles.get(i+1);
            	
                int spBegin = a.fileBegin() + a.fileSize(); //- 1 + 1;
                spBegin = alignUp(spBegin, align);

                int spEnd = b.fileBegin();
                spEnd = alignDown(spEnd, align);

                int spSize = spEnd - spBegin;

                if (spSize >= len)
                {
                    int spLeft = spSize - len;
                    if (spLeft < bestSpaceLeft)
                    {
                        bestSpaceLeft = spLeft;
                        bestSpace = a;
                        bestSpaceBegin = spBegin;
                    }
                }
            }

            if (bestSpace != null)
                return bestSpaceBegin;
            else
            {
            	PhysicalFile last = (PhysicalFile) allDSFiles.get(allDSFiles.size() - 1);
                return alignUp(last.fileBegin() + last.fileSize(), align);
            }
        }

        public void moveAllFiles(PhysicalFile first, int firstOffs) throws Exception
        {
        	Collections.sort(allDSFiles, dcomp);
            //System.out.println("Moving file " + first.name());
            //System.out.println("Into " + String.format("%x", firstOffs));

            int firstStart = first.fileBegin();
            int diff = (int)firstOffs - (int)firstStart;
            //System.out.println("DIFF " + String.format("%x", diff));
            //if (diff < 0)
                //throw new Exception("DOSADJODJOSAJD");
            //    return;

            //WARNING: I assume all the aligns are powers of 2
            int maxAlign = 4;
            for(int i = allDSFiles.indexOf(first); i < allDSFiles.size(); i++)
            {
            	int align = ((PhysicalFile)allDSFiles.get(i)).alignment;
                if(align > maxAlign)
                    maxAlign = align;
            }

            //To preserve the alignment of all the moved files
            if(diff % maxAlign != 0)
                diff += (int)(maxAlign - diff % maxAlign);


            int fsEnd = getFilesystemEnd();
            int toCopy = (int)fsEnd - (int)firstStart;
            byte[] data = new byte[toCopy];

            s.seek(firstStart);
            s.read(data, 0, toCopy);
            s.seek(firstStart + diff);
            s.write(data, 0, toCopy);

            for (int i = allDSFiles.indexOf(first); i < allDSFiles.size(); i++)
                ((PhysicalFile)allDSFiles.get(i)).fileBeginP += diff;
            for (int i = allDSFiles.indexOf(first); i < allDSFiles.size(); i++)
                ((PhysicalFile)allDSFiles.get(i)).saveOffsets();
        }
        
        @Override
        public void close() throws Exception
        {
            source.close();
        }
        @Override
        public void save() throws Exception
        {
            source.save();
        }

        public void dumpFilesOrdered(FileWriter outs)
        {
        	Collections.sort(allDSFiles, dcomp);
            for(DSFile qq : allDSFiles)
            {
            	PhysicalFile f = (PhysicalFile) qq;
            	try {
					outs.write(String.format("%08x",f.fileBegin()) + " .. " + String.format("%08x",(f.fileBegin() + f.fileSize() - 1)) + ":  " + f.getPath());
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
            }
                
        }

        public int getFilesystemEnd()
        {
        	Collections.sort(allDSFiles, dcomp);
            PhysicalFile lastFile = (PhysicalFile) allDSFiles.get(allDSFiles.size() - 1);
            int end = lastFile.fileBegin() + lastFile.fileSize();
            return end;
        }
	}
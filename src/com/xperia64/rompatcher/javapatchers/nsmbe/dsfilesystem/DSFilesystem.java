package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import android.util.SparseArray;

import com.xperia64.rompatcher.javapatchers.nsmbe.streamutils.Stream;

import java.io.IOException;
import java.util.ArrayList;

/*
*   This DSFile is part of NSMB Editor 5.
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
//using System.Drawing;

    public abstract class DSFilesystem
    {
        public ArrayList<DSFile> allDSFiles = new ArrayList<DSFile>();
        public ArrayList<DSDirectory> allDirs = new ArrayList<DSDirectory>();
        protected SparseArray<DSFile> DSFilesById = new SparseArray<DSFile>();
        protected SparseArray<DSDirectory> DSDirsById = new SparseArray<DSDirectory>();
        public DSDirectory mainDir;

//        public DSFilesystemBrowser viewer;

        public DSFile getDSFileById(int id)
        {
            //if (!DSFilesById.containsKey(id))
            //    return null;
            return DSFilesById.get(id, null);
        }

        public DSFile getDSFileByName(String name)
        {
            for(DSFile f : allDSFiles)
                if (f.name().equals(name))
                    return f;

            return null;
        }

        public DSDirectory getDirByPath(String path)
        {
            String[] shit = path.split("/");
            DSDirectory dir = mainDir;
            for (int i = 0; i < shit.length; i++)
            {
                DSDirectory newDir = null;
                for(DSDirectory d : dir.childrenDirs)
                    if(d.name() == shit[i])
                    {
                        newDir = d;
                        break;
                    }
                if(newDir == null) return null;

                dir = newDir;
            }
            return dir;
        }

        protected void addDSFile(DSFile f) throws Exception
        {
            allDSFiles.add(f);
            //System.out.println(f.getPath()+" "+f.id());
            if (DSFilesById.get(f.id(),null)!=null)
                throw new Exception("Duplicate DSFile ID");

            DSFilesById.put(f.id(), f);
//            DSFilesByName.Add(f.name, f);
        }


        protected void addDir(DSDirectory d) throws Exception
        {
            allDirs.add(d);
            if(DSDirsById.get(d.id(),null)!=null)
                throw new Exception("Duplicate dir ID");
            DSDirsById.put(d.id(), d);
//            dirsByName.Add(d.name, d);
        }

        public int alignUp(int what, int align)
        {
            if (what % align != 0)
                what += align - what % align;
            return what;
        }
        public int alignDown(int what, int align)
        {
            what -= what % align;
            return what;
        }

        public void DSFileMoved(DSFile f) throws Exception
        {
        }


        public long readUInt(Stream s) throws IOException
        {
            long res = 0;
            for (int i = 0; i < 4; i++)
            {
                res |= (long)(s.read() << 8 * i);
            }
            return res;
        }

		//Saving and closing
		public void save() throws IOException, Exception {}
		public void close() throws Exception {}

		// Note: This is in the original C# version. Seems important.
        public String getRomPath()
        {
            return "Wadafuq.";
        }
    }

package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import java.util.ArrayList;

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


    public class DSDirectory
    {
        private boolean isSystemFolderP;
        public boolean isSystemFolder() { return isSystemFolderP; } 

        private String nameP;
        public String name() { return nameP; } 

        private int idP;
        public int id() { return idP; } 

        private DSDirectory parentDirP;
        public DSDirectory parentDir() { return parentDirP; } 

        public ArrayList<DSFile> childrenDSFiles = new ArrayList<DSFile>();
        public ArrayList<DSDirectory> childrenDirs = new ArrayList<DSDirectory>();

        //private DSFilesystem parent;

        public DSDirectory(DSFilesystem parent, DSDirectory parentDir, boolean system, String name, int id)
        {
            //this.parent = parent;
            this.parentDirP = parentDir;
            this.isSystemFolderP = system;
            this.nameP = name;
            this.idP = id;
        }

        public String getPath()
        {
            if (parentDir() == null)
                return "FS";
            else
                return parentDir().getPath() + "/" + name();
        }
    }

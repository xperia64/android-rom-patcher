package com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem;

import com.xperia64.rompatcher.javapatchers.nsmbe.ROM;

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

	//Seriouisy, wtf.
    public class BannerFile extends PhysicalFile
    {
        public BannerFile(DSFilesystem parent, DSDirectory parentDir, DSFile headerFile)
        {
        	super(parent, parentDir, -9, "banner.bin", headerFile, 0x68, 0, true);
            endFile = null;
            fileSizeP = 0x840;
            try {
				refreshOffsets();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }

        //Hack to prevent stack overflow...
        private boolean updatingCrc = false;

        public void updateCRC16()
        {
            updatingCrc = true;
            byte[] contents = null;
			try {
				contents = getContents();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
            byte[] checksumArea = new byte[0x820];
            System.arraycopy(contents, 0x20, checksumArea, 0, 0x820);
            int checksum = ROM.CalcCRC16(checksumArea);
            try {
				setUshortAt(2, checksum);
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
            //Console.Out.WriteLine("UPDATING BANNER CHECKSUM!!!!");
            updatingCrc = false;
        }

        @Override
        public void endEdition()
        {
            super.endEdition();
            if(!updatingCrc)
                updateCRC16();
        }
    }


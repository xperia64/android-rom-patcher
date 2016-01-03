package com.xperia64.rompatcher.javapatchers.nsmbe;

import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.DSFile;
import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.DSFilesystem;
import com.xperia64.rompatcher.javapatchers.nsmbe.dsfilesystem.NitroROMFilesystem;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

/*
*   This DSDSFile is part of NSMB Editor 5.
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
//using NSMBe4.DSFileSystem;
//using System.Runtime.InteropServices;


/**
 * This class handles internal NSMB-specific data in the ROM.
 * Right now it can decompress the Overlay data and read
 * data from several tables contained in the ROM.
 * 
 * Data description about overlay 0: (From an old text file)
 * 76 max tilesets. Each table is 0x130 big.
 * 
 * 2F8E4: Object definition indexes (unt+hd) table
 * 2FA14: Object definitions (unt) table
 * 2FB44: Tile behaviours (chk) table
 * 2FC74: Animated tileset graphics (ncg) table
 * 2FDA4: Jyotyu tile behaviour file
 * 30D74: Background graphics (ncg) table
 * 30EA4: Tileset graphics (ncg) table
 * 30FD4: Foreground graphics (ncg) table
 * 31104: Foreground design (nsc) table
 * 31234: Background design (nsc) table
 * 31364: Background palette (ncl) table
 * 31494: Tileset palette (ncl) table
 * 315C4: Foreground palette (ncl) table
 * 316F4: Map16 (pnl) table
 * 
 **/

    public class ROM {
    	
        public static byte[] Overlay0;
        public static DSFilesystem FS;
        public static String filename;
        //public static System.IO.FileInfo romfile;
        public static File romfile;
        //public static ROMUserInfo UserInfo;

        public static boolean isNSMBRom = true;
        public static String romInternalName;
        public static String romGamecode;

        public final static int SpriteCount = 326;

        public static ArrayList<String> fileBackups = new ArrayList<String>();
		
        public static DSFile arm9binFile;
        public static DSFile arm9ovFile;
		public static Overlay[] arm9ovs;

        public static DSFile arm7binFile;
        public static DSFile arm7ovFile;
		public static Overlay[] arm7ovs;

        public static DSFile bannerFile;
        public static DSFile rsaSigFile;
        public static DSFile headerFile;

        public static long arm9RAMAddress;

        //Download play-friendly mode.
        public static boolean dlpMode = false;

        public static void load(DSFilesystem fs) throws Exception
        {
            filename = fs.getRomPath();
            FS = fs;
            if(fs instanceof NitroROMFilesystem)
                romfile = new File(filename);

			arm9binFile = FS.getDSFileByName("arm9.bin");			
			arm9ovFile = FS.getDSFileByName("arm9ovt.bin");	
			arm9ovs = loadOvTable(arm9ovFile);
			arm7binFile = FS.getDSFileByName("arm7.bin");			
			arm7ovFile = FS.getDSFileByName("arm7ovt.bin");			
			arm7ovs = loadOvTable(arm7ovFile);
			rsaSigFile = FS.getDSFileByName("rsasig.bin");			
			headerFile = FS.getDSFileByName("header.bin");

            arm9RAMAddress = headerFile.getUintAt(0x28);
			
            ByteArrayInputStream header = new ByteArrayInputStream(headerFile.getContents());
            byte[] intName = new byte[12];
            header.read(intName);
            romInternalName = new String(intName, "US-ASCII");
            byte[] gameCode = new byte[4];
            header.read(gameCode);
            romGamecode = new String(gameCode, "US-ASCII");

            if (romGamecode.equals("A2DE"))
                Region = Origin.US;
            else if (romGamecode.equals("A2DP"))
                Region = Origin.EU;
            else if (romGamecode.equals("A2DJ"))
                Region = Origin.JP;
            else if (romGamecode.equals("A2DK"))
                Region = Origin.KR;
            else
            {
                isNSMBRom = false;
                Region = Origin.UNK;
            }

            if (isNSMBRom)
            {
                //UserInfo = new ROMUserInfo(filename);
                LoadOverlay0();
            }
        }

        private static long readUint(ByteArrayInputStream b)
        {
        	long res = 0;
        	for (int i = 0; i < 4; i++)
            {
                res |= b.read()&0xFF << 8 * i;
            }
        	return res;
        }
        private static int readUshort(ByteArrayInputStream b)
        {
        	int res = 0;
        	for (int i = 0; i < 2; i++)
            {
                res |= b.read()&0xFF << 8 * i;
            }
        	return res;
        }
        private static Overlay[] loadOvTable(DSFile table) throws IOException
        {
        	Overlay[] ovs = new Overlay[table.fileSize()/32];

            ByteArrayInputStream tbl = new ByteArrayInputStream(table.getContents());

            int i = 0;
            while (tbl.read()>=0)
            {
            	tbl.reset();
                long ovId = readUint(tbl);
                /*long ramAddr = */readUint(tbl);
                /*long ramSize =*/ readUint(tbl);
                /*long bssSize = */readUint(tbl);
                /*long staticInitStart = */readUint(tbl);
                /*long staticInitEnd = */readUint(tbl);
                int fileID = readUshort(tbl);
                tbl.skip(6); //unused 0's
				
				ovs[(int) ovId] = new Overlay(FS.getDSFileById(fileID), table, (long)i*32);
				tbl.mark(0);
                i++;
            }
            
            return ovs;
        }
        
        public static void close() throws Exception
        {
            if (FS == null) return;
            FS.close();
        }

        public static void SaveOverlay0() throws Exception
        {
            Overlay ov = arm9ovs[0];
            ov.decompress();
            ov.f.beginEdit(FS);
            ov.f.replace(Overlay0, FS);
            ov.f.endEdit(FS);
        }

        public static void LoadOverlay0()
        {
            if (arm9ovs.length == 0)
                return;
            Overlay ov = arm9ovs[0];
            Overlay0 = ov.getDecompressedContents();
        }

        public static void writeBackupSetting() // Don't care
        {
            /*String setting = "";
            if (fileBackups.size() > 0)
                setting = filename;
            for(String level : fileBackups)
                setting += ";" + level;
            //Properties.Settings.Default.BackupFiles = setting;
            //Properties.Settings.Default.Save();*/
        }

        public static DSFile getLevelFile(String filename)
        {
            return FS.getDSFileByName(filename + ".bin");
        }

        public static DSFile getBGDatFile(String filename)
        {
            return FS.getDSFileByName(filename + "_bgdat.bin");
        }

        public enum Origin {
            US(0), EU(1), JP(2), KR(3), UNK(4);
            
            private final int value;
            private Origin(int value)
            {
            	this.value = value;
            }
            public int getValue()
            {
            	return value;
            }
        }

        public static Origin Region = Origin.US;

        public enum Data  {
            Number_FileOffset(0),
            Table_TS_UNT_HD(1),
            Table_TS_UNT(2),
            Table_TS_CHK(3),
            Table_TS_ANIM_NCG(4),
            Table_BG_NCG(5),
            Table_TS_NCG(6),
            Table_FG_NCG(7),
            Table_FG_NSC(8),
            Table_BG_NSC(9),
            Table_BG_NCL(10),
            Table_TS_NCL(11),
            Table_FG_NCL(12),
            Table_TS_PNL(13),
            Table_Jyotyu_NCL(14),
            File_Jyotyu_CHK(15),
            File_Modifiers(16),
            Table_Sprite_CLASSID(17);
            private final int value;
            private Data(int value)
            {
            	this.value = value;
            }
            public int getValue()
            {
            	return value;
            }
        }

        public static int[][] Offsets = {
                                           {131, 135, 131, 131}, //File Offset (Overlay Count)
                                           {0x2F8E4, 0x2F0F8, 0x2ECE4, 0x2EDA4}, //TS_UNT_HD
                                           {0x2FA14, 0x2F228, 0x2EE14, 0x2EED4}, //TS_UNT
                                           {0x2FB44, 0x2F358, 0x2EF44, 0x2F004}, //TS_CHK
                                           {0x2FC74, 0x2F488, 0x2F074, 0x2F134}, //TS_ANIM_NCG
                                           {0x30D74, 0x30588, 0x30174, 0x30234}, //BG_NCG
                                           {0x30EA4, 0x306B8, 0x302A4, 0x30364}, //TS_NCG
                                           {0x30FD4, 0x307E8, 0x303D4, 0x30494}, //FG_NCG
                                           {0x31104, 0x30918, 0x30504, 0x305C4}, //FG_NSC
                                           {0x31234, 0x30A48, 0x30634, 0x306F4}, //BG_NSC
                                           {0x31364, 0x30B78, 0x30764, 0x30824}, //BG_NCL
                                           {0x31494, 0x30CA8, 0x30894, 0x30954}, //TS_NCL
                                           {0x315C4, 0x30DD8, 0x309C4, 0x30A84}, //FG_NCL
                                           {0x316F4, 0x30F08, 0x30AF4, 0x30BB4}, //TS_PNL
                                           {0x30CD8, 0x304EC, 0x300D8, 0x30198}, //Jyotyu_NCL
                                           {0x2FDA4, 0x2F5B8, 0x2F1A4, 0x2FC74}, //Jyotyu_CHK
                                           {0x2C930, 0x2BDF0, 0x2BD30, 0x2BDF0}, //Modifiers
                                           {0x29BD8, 0x00000, 0x00000, 0x00000}, //Sprite Class IDs
                                           {0x2CBBC, 0, 0, 0}, //weird table¿?
                                       };

        public static int[] FileSizes = {
                                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //Don't include tables
                                            0x400, //Jyotyu_CHK
                                            ROM.SpriteCount*2, //Modifiers
                                        };

        public static int[] MusicNumbers = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 
                                            80, 81, 82, 83, 86, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111};

        //All the code below is a fucking mess...

        public static int GetFileIDFromTable(int id, Data datatype) {
            return GetFileIDFromTable(id, GetOffset(datatype));
        }

        public static int GetFileIDFromTable(int id, int tableoffset)
        {
            int off = tableoffset + (id << 2);
            return (int)((Overlay0[off]&0xFF | (Overlay0[off + 1]&0xFF << 8)) + GetOffset(Data.Number_FileOffset));
        }

        public static int GetClassIDFromTable(int id)
        {
            int off = GetOffset(Data.Table_Sprite_CLASSID) + (id << 1);
            return (int)((Overlay0[off]&0xFF | (Overlay0[off + 1]&0xFF << 8)));
        }

        public static void SetFileIDFromTable(int id, Data datatype, int fid)
        {
            SetFileIDFromTable(id, GetOffset(datatype), fid);
        }

        public static String getDataForSprite(int id)
        {
            int offs = 0x2CBBC + id * 20;
            String s = "";

            for(int i = 0; i < 20; i++)
                s += String.format("%02x", Overlay0[offs++]) + " ";

            return s;
        }

        public static void SetFileIDFromTable(int id, int tableoffset, int fid)
        {
            int off = tableoffset + (id << 2);
            fid -= (int) GetOffset(Data.Number_FileOffset);
            Overlay0[off] = (byte)(fid & 0xFF);
            Overlay0[off + 1] = (byte)(fid >> 8);
        }

        public static int GetOffset(Data datatype) {
            return Offsets[datatype.getValue()][Region.getValue()];
        }

        public static byte[] GetInlineFile(Data datatype) {
            byte[] output = new byte[FileSizes[datatype.getValue()]];
            System.arraycopy(Overlay0, GetOffset(datatype), output, 0, output.length);
            return output;
        }

        public static void ReplaceInlineFile(Data datatype, byte[] NewFile) throws Exception {
            System.arraycopy(NewFile, 0, Overlay0, GetOffset(datatype), NewFile.length);
            SaveOverlay0();
        }
        public static class IntRef { public int value = 0; }
        public static class LongRef { public long value = 0; }
        public static class BooleanRef { public boolean value = false; }
        public static byte[] DecompressOverlay(byte[] sourcedata)
        {
            long DataVar1, DataVar2;
            //long last 8-5 bytes
            DataVar1 = (long)((sourcedata[sourcedata.length - 8]&0xFF) | ((sourcedata[sourcedata.length - 7]&0xFF) << 8) | ((sourcedata[sourcedata.length - 6]&0xFF) << 16) | ((sourcedata[sourcedata.length - 5]&0xFF) << 24));
            //long last 4 bytes
            DataVar2 = (long)((sourcedata[sourcedata.length - 4]&0xFF) | ((sourcedata[sourcedata.length - 3]&0xFF) << 8) | ((sourcedata[sourcedata.length - 2]&0xFF) << 16) | ((sourcedata[sourcedata.length - 1]&0xFF) << 24));

            byte[] memory = new byte[(int) (sourcedata.length + DataVar2)];
            System.arraycopy(sourcedata, 0, memory, 0, sourcedata.length);

            LongRef r0, r1, r2, r3, r5, r6, r7, r12;
            r0 = new LongRef();
            r1 = new LongRef();
            r2 = new LongRef();
            r3 = new LongRef();
            r5 = new LongRef();
            r6 = new LongRef();
            r7 = new LongRef();
            r12 = new LongRef();
            BooleanRef N, V;
            N = new BooleanRef();
            V = new BooleanRef();
            r0.value = (long)sourcedata.length;

            if (r0.value == 0) {
                return null;
            }
            r1.value = DataVar1;
            r2.value = DataVar2;
            r2.value = r0.value + r2.value; //length + datavar2.value -> decompressed length
            r3.value = r0.value - (r1.value >> 0x18); //delete the latest 3 bits??
            r1.value &= 0xFFFFFF; //save the latest 3 bits
            r1.value = r0.value- r1.value;
            int dumbgoto = 0xa958;
            while(true)
            {
            	switch(dumbgoto)
            	{
            		case 0xa958:
            			if (r3.value <= r1.value) { //if r1.value is 0 they will be equal
                            dumbgoto = 0xa9B8; //return the memory buffer
                            break;
                        }
                        r3.value -= 1;
                        r5.value = memory[(int) r3.value]&0xFF;
                        r6.value = 8;
                        dumbgoto = 0xa968;
            			break;
            		case 0xa968:
            			SubS(r6, r6.value, 1, N, V);
        	            if (N != V) {
        	                dumbgoto = 0xa958;
        	                break;
        	            }
        	            if ((r5.value & 0x80) != 0) {
        	                dumbgoto = 0xa984;
        	                break;
        	            }
        	            r3.value -= 1;
        	            r0.value = memory[(int) r3.value]&0xFF;
        	            r2.value -= 1;
        	            memory[(int) r2.value] = (byte)r0.value;
        	            dumbgoto = 0xa9AC;
            			break;
            		case 0xa984:
                        r3.value -= 1;
                        r1.value = memory[(int) r3.value]&0xFF;
                        r3.value -= 1;
                        r7.value = memory[(int) r3.value]&0xFF;
                        r7.value |= ((r12.value << 8)&0xFFFFFFFFL);
                        r7.value &= 0xFFF;
                        r7.value += 2;
                        r12.value += 0x20;
                        dumbgoto = 0xa99C;
                        break;
            		case 0xa99C:
            			r0.value = memory[(int) (r2.value + r7.value)]&0xFF;
                        r2.value -= 1;
                        memory[(int) r2.value] = (byte)(r0.value);
                        SubS(r12, r12.value, 0x10, N, V);
                        if (N == V) {
                            dumbgoto = 0xa99C;
                            break;
                        }
                        dumbgoto = 0xa9AC;
            			break;
            		case 0xa9AC:
            			r5.value <<= 1;
            			r5.value &= 0xFFFFFFFFL;
                        if (r3.value > r1.value) {
                            dumbgoto = 0xa968;
                            break;
                        }
                        dumbgoto = 0xa9B8;
                        break;
            		case 0xa9B8:
            			return memory;
                        
            	}
            }
        }

        private static void SubS(/*out*/ LongRef dest, final long v1, final long v2, /*out*/ BooleanRef N, /*out*/ BooleanRef V) {
            dest.value = v1 - v2;
            dest.value &=0xFFFFFFFFL;
            N.value = (dest.value & 2147483648L) != 0;
            V.value = ((((v1 & 2147483648L) != 0) && ((v2 & 2147483648L) == 0) && ((dest.value & 2147483648L) == 0)) || ((v1 & 2147483648L) == 0) && ((v2 & 2147483648L) != 0) && ((dest.value & 2147483648L) != 0));
        }

        
		//TODO: Optimize it with KMP search maybe?
        public static void LZ77_Compress_Search(byte[] data, int pos, /*out*/ IntRef match, /*out*/ IntRef length)
        {
            int maxMatchDiff = 4096;
            int maxMatchLen = 18;
            match.value = 0;
            length.value = 0;

            int start = pos - maxMatchDiff;
            if (start < 0) start = 0;

            for (int thisMatch = start; thisMatch < pos; thisMatch++)
            {
                int thisLength = 0;
                while(thisLength < maxMatchLen
                    && thisMatch + thisLength < pos 
                    && pos + thisLength < data.length
                    && data[pos+thisLength] == data[thisMatch+thisLength])
                    thisLength++;

                if(thisLength > length.value)
                {
                    match.value = thisMatch;
                    length.value = thisLength;
                }

                //We can't improve the max match length again...
                if(length.value == maxMatchLen)
                    return;
            }
        }

        @SuppressWarnings("unused")
		public static byte[] LZ77_Compress(byte[] data, boolean header) throws IOException
        {
            ByteArrayOutputStream res = new ByteArrayOutputStream();
            if (header && false) //0x37375A4C
            {
            	byte[] b = {0x4C, 0x5A, 0x37, 0x37};
            	res.write(b);
            } 
            
            res.write((data.length << 8) | 0x10);

            byte[] tempBuffer = new byte[16];

            //Current byte to compress.
            int current = 0;

            while (current < data.length)
            {
                int tempBufferCursor = 0;
                byte blockFlags = 0;
                for (int i = 0; i < 8; i++)

                {
                    //Not sure if this is needed. The DS probably ignores this data.
                    if (current >= data.length)
                    {
                        tempBuffer[tempBufferCursor++] = 0;
                        continue;
                    }

                    IntRef searchPos = new IntRef();
                    IntRef searchLen = new IntRef();
                    LZ77_Compress_Search(data, current, /*out*/ searchPos, /*out*/ searchLen);
                    int searchDisp = current - searchPos.value - 1;
                    if (searchLen.value > 2) //We found a big match, let's write a compressed block.
                    {
                        blockFlags |= (byte)(1 << (7 - i));
                        tempBuffer[tempBufferCursor++] = (byte)((((searchLen.value - 3) & 0xF) << 4) + ((searchDisp >> 8) & 0xF));
                        tempBuffer[tempBufferCursor++] = (byte)(searchDisp & 0xFF);
                        current += searchLen.value;
                    }
                    else
                    {
                        tempBuffer[tempBufferCursor++] = data[current++];
                    }
                }

                res.write(blockFlags);
                for (int i = 0; i < tempBufferCursor; i++)
                    res.write(tempBuffer[i]);
            }

            return res.toByteArray();
        }


        public static byte[] LZ77_FastCompress(byte[] source)
        {
            int DataLen = 4;
            DataLen += source.length;
            DataLen += (int)Math.ceil((double)source.length / 8);
            byte[] dest = new byte[DataLen];

            dest[0] = 0;
            dest[1] = (byte)(source.length & 0xFF);
            dest[2] = (byte)((source.length >> 8) & 0xFF);
            dest[3] = (byte)((source.length >> 16) & 0xFF);

            int FilePos = 4;
            int UntilNext = 0;

            for (int SrcPos = 0; SrcPos < source.length; SrcPos++)
            {
                if (UntilNext == 0)
                {
                    dest[FilePos] = 0;
                    FilePos++;
                    UntilNext = 8;
                }
                dest[FilePos] = source[SrcPos];
                FilePos++;
                UntilNext -= 1;
            }

            return dest;
        }

        public static int LZ77_GetDecompressedSize(byte[] source)
        {
            // This code converted from Elitemap 
            int DataLen;
            DataLen = source[1]&0xFF | (source[2]&0xFF << 8) | (source[3]&0xFF << 16);
            return DataLen;
        }
        public static int LZ77_GetDecompressedSizeWithHeader(byte[] source)
        {
            // This code converted from Elitemap 
            int DataLen;
            DataLen = source[5]&0xFF | (source[6]&0xFF << 8) | (source[7]&0xFF << 16);
            return DataLen;
        }
        
        public static byte[] LZ77_Decompress(byte[] source)
        {
            // This code converted from Elitemap 
            int DataLen;
            DataLen = source[1]&0xFF | (source[2]&0xFF << 8) | (source[3]&0xFF << 16);
            byte[] dest = new byte[DataLen];
            int i, j, xin, xout;
            xin = 4;
            xout = 0;
            int length, offset, windowOffset, data;
            byte d;
            while (DataLen > 0)
            {
                d = source[xin++];
                if (d != 0)
                {
                    for (i = 0; i < 8; i++)
                    {
                        if ((d & 0x80) != 0)
                        {
                            data = ((source[xin]&0xFF << 8) | source[xin + 1]&0xFF);
                            xin += 2;
                            length = (data >> 12) + 3;
                            offset = data & 0xFFF;
                            windowOffset = xout - offset - 1;
                            for (j = 0; j < length; j++)
                            {
                                dest[xout++] = dest[windowOffset++];
                                DataLen--;
                                if (DataLen == 0)
                                {
                                    return dest;
                                }
                            }
                        }
                        else
                        {
                            dest[xout++] = source[xin++];
                            DataLen--;
                            if (DataLen == 0)
                            {
                                return dest;
                            }
                        }
                        d <<= 1;
                    }
                }
                else
                {
                    for (i = 0; i < 8; i++)
                    {
                        dest[xout++] = source[xin++];
                        DataLen--;
                        if (DataLen == 0)
                        {
                            return dest;
                        }
                    }
                }
            }
            return dest;
        }

        public static byte[] LZ77_DecompressWithHeader(byte[] source)
        {
            // This code converted from Elitemap 
            int DataLen;
            DataLen = source[5]&0xFF | (source[6]&0xFF << 8) | (source[7]&0xFF << 16);
            byte[] dest = new byte[DataLen];
            int i, j, xin, xout;
            xin = 8;
            xout = 0;
            int length, offset, windowOffset, data;
            byte d;
            while (DataLen > 0)
            {
                d = source[xin++];
                if (d != 0)
                {
                    for (i = 0; i < 8; i++)
                    {
                        if ((d & 0x80) != 0)
                        {
                            data = ((source[xin]&0xFF << 8) | source[xin + 1]&0xFF);
                            xin += 2;
                            length = (data >> 12) + 3;
                            offset = data & 0xFFF;
                            windowOffset = xout - offset - 1;
                            for (j = 0; j < length; j++)
                            {
                                dest[xout++] = dest[windowOffset++];
                                DataLen--;
                                if (DataLen == 0)
                                {
                                    return dest;
                                }
                            }
                        }
                        else
                        {
                            dest[xout++] = source[xin++];
                            DataLen--;
                            if (DataLen == 0)
                            {
                                return dest;
                            }
                        }
                        d <<= 1;
                    }
                }
                else
                {
                    for (i = 0; i < 8; i++)
                    {
                        dest[xout++] = source[xin++];
                        DataLen--;
                        if (DataLen == 0)
                        {
                            return dest;
                        }
                    }
                }
            }
            return dest;
        }

        private static int[] CRC16Table = {
            0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
            0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
            0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
            0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
            0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
            0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
            0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
            0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
            0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
            0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
            0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
            0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
            0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
            0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
            0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
            0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
            0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
            0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
            0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
            0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
            0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
            0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
            0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
            0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
            0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
            0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
            0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
            0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
            0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
            0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
            0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
            0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
        };

        // from ndstool
        public static int CalcCRC16(byte[] data)
        {
            int crc = 0xFFFF;

            for (int i = 0; i < data.length; i++) {
                crc = (int)((crc >> 8) ^ CRC16Table[(crc ^ (data[i]&0xFF)) & 0xFF]);
            }

            return crc;
        }
    }

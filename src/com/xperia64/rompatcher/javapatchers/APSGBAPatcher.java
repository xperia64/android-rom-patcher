/*******************************************************************************
 * This file is part of ROM Patcher.
 * 
 * Copyright (c) 2014 xperia64.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 * 	Paul Kratt - main MultiPatch application for Mac OSX
 * 	byuu - UPS and BPS patchers
 * 	Neil Corlett - IPS patcher
 * 	Daniel Ekstr'm - PPF patcher
 * 	Josh MacDonald - XDelta
 * 	Colin Percival - BSDiff
 * 	xperia64 - port to Android and IPS32 support
 ******************************************************************************/
package com.xperia64.rompatcher.javapatchers;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.util.Arrays;


public class APSGBAPatcher {
	
	
	// I am literally just porting VB5 to Java. That's why this is so terrible.
		private final byte[] lSignature  = {0x41, 0x50, 0x53, 0x31};
		private final int lChunkBytes = 65536; //// 64k	
		private final int lIntegerOffset = 65536;
		private final int lMaxInteger = (lIntegerOffset / 2) - 1;

		private int[] lCrcTable = new int[256];

		private byte[] bByteArray1 = new byte[lChunkBytes];
		private byte[] bByteArray2 = new byte[lChunkBytes];

		private class tPatchData
		{
			int lOffset;
			short iCrc16_1;
			short iCrc16_2;
		}

		public void crcTableInit() // This method is correct.
		{
			final int Poly = 0x1021;
			int i;
			int j;
			int lCrc;
			int lTemp;
		    
		    for( i = 0; i<lCrcTable.length; i++)
		    {
		        lTemp = i * 0x100;
		        lCrc = 0;
		        
		        for(j=0; j<8; j++)
		        {
		            if (((lCrc ^ lTemp) & 0x8000)!=0)
		            	lCrc = ((lCrc * 2) ^ Poly) & 0xFFFF;
	            	else
	            		lCrc = (lCrc * 2) & 0xFFFF;
		            lTemp = (lTemp * 2) & 0xFFFF;
		        }
		        lCrcTable[i] = lCrc;   
		    }
		}
		private int crc16(byte[] Data)
		{
			int i;
		    
		    int Crc16 =  0xFFFF;
		    
		    for(i = 0; i<0x10000; i++)
		    {
		    	Crc16 = (short) (((Crc16<<8) ^ lCrcTable[((Crc16>>8)^(Data[i]))&0xFF]) & 0xFFFF);
		    }
		    
		    if(Crc16 > lMaxInteger)
		        Crc16 = (short) (Crc16 - lIntegerOffset);
		    return Crc16;
		}
		
		private void TruncateFile(String FileName, int Size) throws IOException
		{
			File f = new File(FileName);
			FileChannel outChan;
			outChan = new FileOutputStream(f, true).getChannel();
			outChan.truncate(Size);
			outChan.close();
		}
		public short b2ls(byte[] buffer)
		{
			return (short) ((buffer[0] & 0xFF) | (buffer[1] & 0xFF) << 8);
		}
		public int b2li(byte[] buffer) 
		{
		    return (buffer[0] & 0xFF) | (buffer[1] & 0xFF) << 8 | (buffer[2] & 0xFF) << 16 | (buffer[3] & 0xFF) << 24;
		}

		public int ApplyPatch(String PatchFile, String Original, boolean ignoreCRCErrors) throws IOException
		{
			int lFileSize1;
			int lFileSize2;
			int iCrc16;
			long lBytesLeft;
			tPatchData tPatch = null;
			boolean fIsOriginal = false;
			boolean fIsModified = false;

		    RandomAccessFile rPatch = new RandomAccessFile(PatchFile, "r");
			byte[] inttemp = new byte[4];
			byte[] shorttemp = new byte[2];
			rPatch.read(inttemp);
			if(!Arrays.equals(inttemp, lSignature))
			{
				return -1;
			}
			
			rPatch.read(inttemp);
			lFileSize1 = b2li(inttemp);
			rPatch.read(inttemp);
			lFileSize2 = b2li(inttemp);
		    lBytesLeft = rPatch.length()-12;
			RandomAccessFile rOrig = new RandomAccessFile(Original, "rw");
			
		    while(lBytesLeft > 0)
		    {
		    	tPatch = new tPatchData();
		        rPatch.read(inttemp);
		        tPatch.lOffset = b2li(inttemp);
		        rPatch.read(shorttemp);
		        tPatch.iCrc16_1 = b2ls(shorttemp);
		        rPatch.read(shorttemp);
		        tPatch.iCrc16_2 = b2ls(shorttemp);
		        rOrig.seek(tPatch.lOffset);
		        rOrig.read(bByteArray1);
		        rPatch.read(bByteArray2);
		        lBytesLeft -= lChunkBytes;
		        lBytesLeft -= 8;
		        if(lBytesLeft<lChunkBytes)
		        	for(int i = (int) lBytesLeft; i<lChunkBytes; i++)
		        		bByteArray1[i]=0;
		        
		        iCrc16 = crc16(bByteArray1);
		        
		        for(int i = 0; i<lChunkBytes; i++)
		        	bByteArray1[i]^=bByteArray2[i];

		        if(iCrc16 == tPatch.iCrc16_1 ) // Called when patching a clean file
		        {
		        	if(fIsModified&&fIsOriginal)
		        		return -2;

		        	rOrig.seek(tPatch.lOffset);
		        	rOrig.write(bByteArray1);
		        	fIsOriginal = true;

		        }else if( iCrc16 == tPatch.iCrc16_2 ) // Called when restoring the file to its original state
		        {   
		        	if(fIsModified&&fIsOriginal)
		        		return -3;
		        	rOrig.seek(tPatch.lOffset);
		        	rOrig.write(bByteArray1);
		        	fIsModified = true;

		        }else if(ignoreCRCErrors)
		        {
		        	// Assume we are patching the original
		        	rOrig.seek(tPatch.lOffset);
		        	rOrig.write(bByteArray1);
		        	fIsOriginal = true;
		        }else{
		        	return -4;
		        }
		    }
		    if(fIsOriginal)
		    {
		    	TruncateFile(Original, lFileSize1);
		    }else{
		    	TruncateFile(Original, lFileSize2);
		    }
		    rOrig.close();
		    rPatch.close();
		    return 0;
		}
}
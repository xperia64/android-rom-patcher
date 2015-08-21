package com.xperia64.rompatcher.javapatchers.nsmbe.streamutils;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

public class FileStream extends Stream {

	RandomAccessFile derp;
	
	public FileStream(String name) throws FileNotFoundException
	{
		derp = new RandomAccessFile(name, "rw");
	}
	@Override
	public void seek(long pos) throws IOException {
		// TODO Auto-generated method stub
		derp.seek(pos);
	}

	@Override
	public int read() throws IOException {
		// TODO Auto-generated method stub
		return derp.read();
	}

	@Override
	public void read(byte[] data, long offset, long length) throws IOException {
		// TODO Auto-generated method stub
		derp.read(data, (int)offset, (int)length);
	}

	@Override
	public void write(byte[] data, long offset, long length) throws IOException {
		// TODO Auto-generated method stub
		derp.write(data, (int)offset, (int)length);
	}

	@Override
	public byte[] array() throws IOException {
		// TODO Auto-generated method stub
		byte[] temp = new byte[(int) derp.length()];
		derp.read(temp);
		return temp;
	}

	@Override
	public void setLength(long len) throws IOException {
		// TODO Auto-generated method stub
		derp.setLength(len);
	}

	@Override
	public void close() throws IOException {
		// TODO Auto-generated method stub
		derp.close();
	}
	@Override
	public int position() throws IOException {
		// TODO Auto-generated method stub
		return (int) derp.getFilePointer();
	}

}

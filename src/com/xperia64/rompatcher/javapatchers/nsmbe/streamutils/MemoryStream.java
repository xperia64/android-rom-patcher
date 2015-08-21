package com.xperia64.rompatcher.javapatchers.nsmbe.streamutils;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class MemoryStream extends Stream {

	 private SeekableByteArrayInputStream bis;
	 private SeekableByteArrayOutputStream bos;
	 //private boolean updateBis = false;
	 @Override
	 public int position() throws IOException
	 {
		 return (int) bis.getFilePointer();
	 }
	@Override
	public void seek(long pos) throws IOException {
		bis.seek(pos);
		bos.seek(pos);
	}
	@Override
	public int read() throws IOException {
		// TODO Auto-generated method stub
		return bis.read();
	}
	@Override
	public void read(byte[] data, long offset, long length) throws IOException {
		// TODO Auto-generated method stub
		bis.read(data, (int)offset, (int)length);
	}
	@Override
	public void write(byte[] data, long offset, long length) throws IOException {
		// TODO Auto-generated method stub
		bos.write(data, (int)offset,(int)length);
		bis = new SeekableByteArrayInputStream(bos.toByteArray());
		bis.seek(bos.getFilePointer());
	}
	@Override
	public byte[] array() {
		// TODO Auto-generated method stub
		return bos.toByteArray();
	}
	@Override
	public void close() throws IOException
	{
		bis.close();
		bos.close();
	}
	@Override
	public void setLength(long len) throws IOException
	{
		byte[] tmp = new byte[(int) len];
		bis.reset();
		for(int i = 0; i<tmp.length; i++)
		{
			if(bis.available()>0)
			{
				tmp[i]=(byte) bis.read();
			}else{
				tmp[i]=0;
			}
		}
		bis = new SeekableByteArrayInputStream(tmp);
		bos = new SeekableByteArrayOutputStream();
		bos.write(tmp);
		
	}
	/**
	 * Trivial wrapper around java.io.ByteArrayOutputStream that allows for seeking
	 * @author Marius Milner
	 *
	 */
	public static class SeekableByteArrayOutputStream extends ByteArrayOutputStream
	implements Seekable {
		protected int maxCount;
		
		public SeekableByteArrayOutputStream() {
			super();
		}

		public SeekableByteArrayOutputStream(int size) {
			super(size);
		}

		@Override
		public synchronized void reset() {
			super.reset();
			maxCount = 0;
		}

		@Override
		public int size() {
			if (count > maxCount)
				return count;
			else
				return maxCount;
		}

		@Override
		public synchronized byte[] toByteArray() {
			// Temporarily swap in maxCount so super method isn't confused
			int c = count;
			if (count > maxCount)
				maxCount = count;
			else
				count = maxCount;
			byte[] ba = super.toByteArray();
			count = c;
			return ba;
		}

		public synchronized long getFilePointer() throws IOException {
			return count;
		}

		public synchronized void seek(long pos) throws IOException {
			if (count > maxCount)
				maxCount = count;
			if (pos > maxCount) {
				throw new IllegalArgumentException(
						"Cannot seek past end of written data: seek=" + pos +
						", pos=" + pos + " of " + maxCount);
			}
			count = (int) pos;
		}
		public synchronized void align(int m)
		{
	            while (count % m != 0)
	                write(0);
		}

	}

	public static class SeekableByteArrayInputStream extends ByteArrayInputStream
	implements Seekable {

	public SeekableByteArrayInputStream(byte[] buf) {
		super(buf);
	}

	public SeekableByteArrayInputStream(byte[] buf, int offset, int length) {
		super(buf, offset, length);
	}

	public synchronized long getFilePointer() throws IOException {
		return pos;
	}

	public synchronized void seek(long where) throws IOException {
		if (pos > buf.length) {
			throw new IllegalArgumentException(
					"Cannot seek past end of data: seek=" + where +
					", pos=" + pos + " of " + buf.length);
		}
		pos = (int) where;
	}

}


}

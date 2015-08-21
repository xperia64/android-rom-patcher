package com.xperia64.rompatcher.javapatchers.nsmbe.streamutils;

import java.io.IOException;

public abstract class Stream {
	public abstract void seek(long pos) throws IOException;
	public abstract int read() throws IOException;
	public abstract void read(byte[] data, long offset, long length) throws IOException;
	public abstract void write(byte[] data, long offset, long length) throws IOException;
	public abstract byte[] array() throws IOException;
	public abstract void setLength(long len) throws IOException;
	public abstract void close() throws IOException;
	public abstract int position() throws IOException;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "libsmw.h"

mapper_t mapper=lorom;
int sa1banks[8]={0<<20, 1<<20, -1, -1, 2<<20, 3<<20, -1, -1};
unsigned char * romdata=NULL;
int romlen;
static bool header;
static FILE * thisfile;

const char * openromerror;

#define CONV_UNHEADERED 0
#define CONV_HEADERED 1
#define CONV_PCTOSNES 2
#define CONV_SNESTOPC 0
#define CONV_LOROM 0
int convaddr(int addr, int mode)
{
	bool header=mode&1;
	bool ispc=mode&2;
	if (ispc)
	{
		if (header) addr-=512;
		addr=pctosnes(addr);
	}
	else
	{
		addr=snestopc(addr);
		if (header) addr+=512;
	}
	return addr;
}

int ratsstart(int snesaddr)
{
	int pcaddr=snestopc(snesaddr);
	if (pcaddr<0x7FFF8) return -1;
	unsigned char * start=romdata+pcaddr-0x10000;
	for (int i=0x10000;i>=0;i--)
	{
		if (!strncmp((char*)start+i, "STAR", 4) &&
				(start[i+4]^start[i+6])==0xFF && (start[i+5]^start[i+7])==0xFF)
		{
			if ((start[i+4]|(start[i+5]<<8))>0x10000-i-8-1) return pctosnes(start-romdata+i);
			else return -1;
		}
	}
	return -1;
}

void resizerats(int snesaddr, int newlen)
{
	int pos=snestopc(ratsstart(snesaddr));
	if (pos<0) return;
	if (newlen!=1) newlen--;
	romdata[pos+4]=  newlen    &0xFF;
	romdata[pos+5]= (newlen>>8)&0xFF;
	romdata[pos+6]=( newlen    &0xFF)^0xFF;
	romdata[pos+7]=((newlen>>8)&0xFF)^0xFF;
}

static void handleprot(int loc, char * name, int len, unsigned char * contents)
{
	if (!strncmp(name, "PROT", 4))
	{
		strncpy(name, "NULL", 4);//to block recursion, in case someone is an idiot
		if (len%3) return;
		len/=3;
		for (int i=0;i<len;i++) removerats((contents[(i*3)+0]    )|(contents[(i*3)+1]<<8 )|(contents[(i*3)+2]<<16));
	}
}

void removerats(int snesaddr)
{
	int addr=ratsstart(snesaddr);
	if (addr<0) return;
	WalkMetadata(addr+8, handleprot);
	addr=snestopc(addr);
	for (int i=(romdata[addr+4]|(romdata[addr+5]<<8))+8;i>=0;i--) romdata[addr+i]=0;
}

static inline int trypcfreespace(int start, int end, int size, int banksize, int minalign)
{
	while (start+size<=end)
	{
		if (
				((start+8)&~banksize)!=((start+size-1)&~banksize&0xFFFFFF)//if the contents won't fit in this bank...
			&&
				(start&banksize&0xFFFFF8)!=(banksize&0xFFFFF8)//and the RATS tag can't fit in the bank either...
			)
		{
			start&=~banksize&0xFFFFFF;//round it down to the start of the bank,
			start|=banksize&0xFFFFF8;//then round it up to the end minus the RATS tag...
			continue;
		}
		if (minalign)
		{
			start&=~minalign&0xFFFFFF;
			start|=minalign&0xFFFFF8;
		}
		if (!strncmp((char*)romdata+start, "STAR", 4) &&
				(romdata[start+4]^romdata[start+6])==0xFF && (romdata[start+5]^romdata[start+7])==0xFF)
		{
			start+=(romdata[start+4]|(romdata[start+5]<<8))+1+8;
			continue;
		}
		bool bad=false;
		for (int i=0;i<size;i++)
		{
			if (romdata[start+i]!=0x00)
			{
				start+=i;
				if (!i) start++;//this could check for a rats tag instead, but somehow I think this will give better performance.
				bad=true;
				break;
			}
		}
		if (bad) continue;
		size-=8;
		if (size) size--;//rats tags eat one byte more than specified for some reason
		romdata[start+0]='S';
		romdata[start+1]='T';
		romdata[start+2]='A';
		romdata[start+3]='R';
		romdata[start+4]=  size    &0xFF;
		romdata[start+5]= (size>>8)&0xFF;
		romdata[start+6]=( size    &0xFF)^0xFF;
		romdata[start+7]=((size>>8)&0xFF)^0xFF;
		return start+8;
	}
	return -1;
}

//This function finds a block of freespace. -1 means "no freespace found", anything else is a PC address.
//isforcode=true tells it to favor banks 40+, false tells it to avoid them entirely.
//It automatically adds a RATS tag.

int getpcfreespace(int size, bool isforcode, bool autoexpand, bool respectbankborders, bool align)
{
	if (!size) return 0x1234;//in case someone protects zero bytes for some dumb reason.
		//You can write zero bytes to anywhere, so I'll just return something that removerats will ignore.
	if (size>0x10000) return -1;
	size+=8;
	if (mapper==lorom)
	{
		if (size>0x8008 && respectbankborders) return -1;
	rebootlorom:
		if (romlen>0x200000 && !isforcode)
		{
			int pos=trypcfreespace(0x200000-8, (romlen<0x400000)?romlen:0x400000, size,
					respectbankborders?0x7FFF:0xFFFFFF, align?0x7FFF:(respectbankborders || size<32768)?0:0x7FFF);
			if (pos>=0) return pos;
		}
		int pos=trypcfreespace(0x80000, (romlen<0x200000)?romlen:0x200000, size,
				respectbankborders?0x7FFF:0xFFFFFF, align?0x7FFF:(respectbankborders || size<32768)?0:0x7FFF);
		if (pos>=0) return pos;
		if (autoexpand)
		{
			if(0);
			else if (romlen==0x080000)
			{
				romlen=0x100000;
				romdata[snestopc(0x00FFD7)]=0x0A;
			}
			else if (romlen==0x100000)
			{
				romlen=0x200000;
				romdata[snestopc(0x00FFD7)]=0x0B;
			}
			else if (isforcode) return -1;//no point creating freespace that can't be used
			//else if (romlen==0x200000)//I don't support 3mb roms more than needed. I'll just expand 2mb ROMs to 4mb directly. They screw up my checksums.
			//{
			//	romlen=0x300000;
			//	romdata[snestopc(0x00FFD7)]=0x0C;
			//}
			else if (romlen==0x200000 || romlen==0x300000)
			{
				romlen=0x400000;
				romdata[snestopc(0x00FFD7)]=0x0C;
				//for (int i=0x380000;i<0x400000;i+=16)
				//{
				//	memcpy((char*)romdata+i, "Unusable area\0\0", 16);
				//}
			}
			else return -1;
			autoexpand=false;
			goto rebootlorom;
		}
	}
	if (mapper==hirom)
	{
		if (isforcode) return -1;
		return trypcfreespace(0, romlen, size, 0xFFFF, align?0xFFFF:0);
	}
	if (mapper==sfxrom)
	{
		if (isforcode) return -1;
		return trypcfreespace(0, romlen, size, 0x7FFF, align?0x7FFF:0);
	}
	if (mapper==sa1rom)
	{
	rebootsa1rom:
		int nextbank=-1;
		for (int i=0;i<8;i++)
		{
			if (i&2) continue;
			if (sa1banks[i]+0x100000>romlen)
			{
				if (sa1banks[i]<=romlen && sa1banks[i]+0x100000>romlen) nextbank=sa1banks[i];
				continue;
			}
			int pos=trypcfreespace(sa1banks[i]?sa1banks[i]:0x80000, sa1banks[i]+0x100000, size, 0x7FFF, align?0x7FFF:0);
			if (pos>=0) return pos;
		}
		if (autoexpand && nextbank>=0)
		{
			unsigned char x7FD7[]={0, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D};
			romlen=nextbank+0x100000;
			romdata[0x7FD7]=x7FD7[romlen>>20];
			autoexpand=false;
			goto rebootsa1rom;
		}
	}
	return -1;
}

void WalkRatsTags(void(*func)(int loc, int len))
{
	int pos=snestopc(0x108000);
	while (pos<romlen)
	{
		if (!strncmp((char*)romdata+pos, "STAR", 4) &&
					(romdata[pos+4]^romdata[pos+6])==0xFF && (romdata[pos+5]^romdata[pos+7])==0xFF)
		{
			func(pctosnes(pos+8), (romdata[pos+4]|(romdata[pos+5]<<8))+1);
			pos+=(romdata[pos+4]|(romdata[pos+5]<<8))+1+8;
		}
		else pos++;
	}
}

void WalkMetadata(int loc, void(*func)(int loc, char * name, int len, unsigned char * contents))
{
	int pcoff=snestopc(loc);
	if (strncmp((char*)romdata+pcoff-8, "STAR", 4)) return;
	unsigned char * metadata=romdata+pcoff;
	while (isupper(metadata[0]) && isupper(metadata[1]) && isupper(metadata[2]) && isupper(metadata[3]))
	{
		if (!strncmp((char*)metadata, "STOP", 4))
		{
			metadata=romdata+pcoff;
			while (isupper(metadata[0]) && isupper(metadata[1]) && isupper(metadata[2]) && isupper(metadata[3]))
			{
				if (!strncmp((char*)metadata, "STOP", 4))
				{
					break;
				}
				func(pctosnes(metadata-romdata), (char*)metadata, metadata[4], metadata+5);
				metadata+=5+metadata[4];
			}
			break;
		}
		metadata+=5+metadata[4];
	}
}

int getsnesfreespace(int size, bool isforcode, bool autoexpand, bool respectbankborders, bool align)
{
	return pctosnes(getpcfreespace(size, isforcode, autoexpand, respectbankborders, align));
}

bool openrom(const char * filename, bool confirm)
{
	closerom();
	thisfile=fopen(filename, "r+b");
	if (!thisfile)
	{
		openromerror="Couldn't open ROM";
		return false;
	}
	fseek(thisfile, 0, SEEK_END);
	header=false;
	if (strlen(filename)>4)
	{
		const char * fnameend=strchr(filename, '\0')-4;
		header=(!stricmp(fnameend, ".smc"));
	}
	romlen=ftell(thisfile)-(header*512);
	if (romlen<0) romlen=0;
	fseek(thisfile, header*512, SEEK_SET);
	romdata=(unsigned char*)malloc(sizeof(unsigned char)*16*1024*1024);
	int truelen=fread((char*)romdata, 1, romlen, thisfile);
	if (truelen!=romlen)
	{
		openromerror="Couldn't open ROM";
		free(romdata);
		return false;
	}
	memset((char*)romdata+romlen, 0x00, 16*1024*1024-romlen);
	if (confirm && snestopc(0x00FFC0)+21<(int)romlen && strncmp((char*)romdata+snestopc(0x00FFC0), "SUPER MARIOWORLD     ", 21))
	{
		closerom(false);
		openromerror=header?"Doesn't look like an SMW ROM (maybe its extension is wrong?)":"Doesn't look like an SMW ROM (maybe it's headered?)";
		return false;
	}
	return true;
}

void closerom(bool save)
{
	if (thisfile && save && romlen)
	{
		fseek(thisfile, header*512, SEEK_SET);
		fwrite((char*)romdata, 1, romlen, thisfile);
	}
	if (thisfile) fclose(thisfile);
	if (romdata) free(romdata);
	thisfile=NULL;
	romdata=NULL;
	romlen=0;
	return;
}

static unsigned int getchecksum()
{
/*
//from snes9x:
	uint32 sum1 = 0;
	uint32 sum2 = 0;
	if(0==CalculatedChecksum)
	{
		int power2 = 0;
		int size = CalculatedSize;
		
		while (size >>= 1)
			power2++;
	
		size = 1 << power2;
		uint32 remainder = CalculatedSize - size;
	
	
		int i;
	
		for (i = 0; i < size; i++)
			sum1 += ROM [i];
	
		for (i = 0; i < (int) remainder; i++)
			sum2 += ROM [size + i];
	
		int sub = 0;
		if (Settings.BS&& ROMType!=0xE5)
		{
			if (Memory.HiROM)
			{
				for (i = 0; i < 48; i++)
					sub += ROM[0xffb0 + i];
			}
			else if (Memory.LoROM)
			{
				for (i = 0; i < 48; i++)
					sub += ROM[0x7fb0 + i];
			}
			sum1 -= sub;
		}


	    if (remainder)
    		{
				sum1 += sum2 * (size / remainder);
    		}
	
	
    sum1 &= 0xffff;
    Memory.CalculatedChecksum=sum1;
	}
*/
	unsigned int checksum=0;
	for (int i=0;i<romlen;i++) checksum+=romdata[i];//this one is correct for most cases, and I don't care about the rest.
	return checksum&0xFFFF;
}

bool goodchecksum()
{
	int checksum=getchecksum();
	return ((romdata[snestopc(0x00FFDE)]^romdata[snestopc(0x00FFDC)])==0xFF) && ((romdata[snestopc(0x00FFDF)]^romdata[snestopc(0x00FFDD)])==0xFF) &&
					((romdata[snestopc(0x00FFDE)]&0xFF)==(checksum&0xFF)) && ((romdata[snestopc(0x00FFDF)]&0xFF)==((checksum>>8)&0xFF));
}

void fixchecksum()
{
	int checksum=getchecksum();
	romdata[snestopc(0x00FFDE)]=  checksum    &255     ;
	romdata[snestopc(0x00FFDF)]= (checksum>>8)&255     ;
	romdata[snestopc(0x00FFDC)]=( checksum    &255)^255;
	romdata[snestopc(0x00FFDD)]=((checksum>>8)&255)^255;
}

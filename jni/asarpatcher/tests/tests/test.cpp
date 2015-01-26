#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../libstr.h"
#include "../autoarray.h"

#define die() do { fclose(fopen("fail", "wb")); return 1; } while(0)
#define dief(...) do { printf(__VA_ARGS__); die(); } while(0)

#define min(a, b) ((a)<(b)?(a):(b))

int main(int argc, char * argv[])
{
	char * smwrom=(char*)malloc(512*1024);
	FILE * smwromf=fopen("smw.sfc", "rb");
	fread(smwrom, 1, 512*1024, smwromf);
	fclose(smwromf);
	char * expectedrom=(char*)malloc(16777216);
	char * truerom=(char*)malloc(16777216);
	char * fname=argv[1];
	char line[256];
	char cmd[256];
	FILE * asmfile=fopen(fname, "rt");
	int pos=0;
	int len=0;
	FILE * rom=fopen("a.sfc", "wb");
	
	int numiter=1;
	
	bool shouldfail=false;
	while (true)
	{
		fgets(line, 250, asmfile);
		if (line[0]!=';' || line[1]!='@') break;
		*strchr(line, '\n')=0;
		autoptr<char**> words=split(line+2, " ");
		for (int i=0;words[i];i++)
		{
			char * word;
			int num=strtol(words[i], &word, 16);
			if (*word) num=-1;
			word=words[i];
			if (!strcmp(word, "+"))
			{
				if (line[2]=='+')
				{
					memcpy(expectedrom, smwrom, 512*1024);
					fwrite(smwrom, 1, 512*1024, rom);
					len=512*1024;
				}
			}
			if ((strlen(word)==5 || strlen(word)==6) && num>=0) pos=num;
			if (strlen(word)==2 && num>=0) expectedrom[pos++]=num;
			if (word[0]=='#')
			{
				numiter=atoi(word+1);
			}
			if (!strcmp(word, "err")) shouldfail=true;
			if (pos>len) len=pos;
		}
	}
	
	//this is rather hacky and fragile, but it works. it's not like anyone's gonna use this thing
	char * asmdata=(char*)malloc(65536);
	strcpy(asmdata, line);
	char * asmdataend=strchr(asmdata, '\0');
	asmdataend[fread(asmdataend, 1, 65536, asmfile)]='\0';
	FILE * azmfile=fopen("a.azm", "wt");
	while (asmdata[0]=='\n') asmdata++;
	fwrite(asmdata, 1, strlen(asmdata), azmfile);
	fclose(azmfile);
	
	fclose(asmfile);
	fclose(rom);
	
	sprintf(cmd, "asar a.azm a.sfc > err.log 2>&1", fname);
	for (int i=0;i<numiter;i++) system(cmd);
	FILE * err=fopen("err.log", "rt");
	fseek(err, 0, SEEK_END);
	if (ftell(err) && !shouldfail)
	{
		fseek(err, 0, SEEK_SET);
		fgets(line, 250, err);
		dief("%s: Insertion error: %s", fname, line);
	}
	if (!ftell(err) && shouldfail) dief("%s: No insertion error\n", fname);
	fclose(err);
	
	rom=fopen("a.sfc", "rb");
	fseek(rom, 0, SEEK_END);
	int truelen=ftell(rom);
	fseek(rom, 0, SEEK_SET);
	fread(truerom, 1, truelen, rom);
	bool fail=false;
	for (int i=0;i<min(len, truelen);i++)
	{
		if (truerom[i]!=expectedrom[i] && !(i>=0x07FDC && i<=0x07FDF && (expectedrom[i]==0x00 || expectedrom[i]==smwrom[i])))
		{
			printf("%s: Mismatch at %.5X: Expected %.2X, got %.2X\n", fname, i, (unsigned char)expectedrom[i], (unsigned char)truerom[i]);
			fail=true;
		}
	}
	if (truelen!=len) dief("%s: Bad ROM length (expected %X, got %X)\n", fname, len, ftell(rom));
	fclose(rom);
	if (fail) die();
}

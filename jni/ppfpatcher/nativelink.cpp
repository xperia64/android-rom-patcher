/* 
	Native JNI linkage
						*/
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



//////////////////////////////////////////////////////////////////////
// Macros for little to big Endian conversion.
int ignoreChecksum=0;
#ifdef __BIG_ENDIAN__

#define Endian16_Swap(value)	(value = (((((unsigned short) value) << 8) & 0xFF00)  | \
                                          ((((unsigned short) value) >> 8) & 0x00FF)))

#define Endian32_Swap(value)    (value = (((((unsigned long) value) << 24) & 0xFF000000)  | \
                                          ((((unsigned long) value) <<  8) & 0x00FF0000)  | \
                                          ((((unsigned long) value) >>  8) & 0x0000FF00)  | \
                                          ((((unsigned long) value) >> 24) & 0x000000FF)))

#define Endian64_Swap(value)	(value = (((((unsigned long long) value) << 56) & 0xFF00000000000000ULL)  | \
                                          ((((unsigned long long) value) << 40) & 0x00FF000000000000ULL)  | \
                                          ((((unsigned long long) value) << 24) & 0x0000FF0000000000ULL)  | \
                                          ((((unsigned long long) value) <<  8) & 0x000000FF00000000ULL)  | \
                                          ((((unsigned long long) value) >>  8) & 0x00000000FF000000ULL)  | \
                                          ((((unsigned long long) value) >> 24) & 0x0000000000FF0000ULL)  | \
                                          ((((unsigned long long) value) >> 40) & 0x000000000000FF00ULL)  | \
                                          ((((unsigned long long) value) >> 56) & 0x00000000000000FFULL)))

#else

#define	Endian16_Swap(value)
#define	Endian32_Swap(value)
#define	Endian64_Swap(value)

#endif /* __BIG_ENDIAN__ */

//////////////////////////////////////////////////////////////////////
// Used global variables.
FILE *ppf, *bin;
char binblock[1024], ppfblock[1024];
unsigned char ppfmem[512];
#define APPLY 1
#define UNDO 2

//////////////////////////////////////////////////////////////////////
// Used prototypes.
int	PPFVersion(FILE *ppf);
int	OpenFiles(char* file1, char* file2);
int	ShowFileId(FILE *ppf, int ppfver);
void 	ApplyPPF1Patch(FILE *ppf, FILE *bin);
int 	ApplyPPF2Patch(FILE *ppf, FILE *bin);
int 	ApplyPPF3Patch(FILE *ppf, FILE *bin, char mode);


//////////////////////////////////////////////////////////////////////
// Applies a PPF1.0 patch.
void ApplyPPF1Patch(FILE *ppf, FILE *bin){
	char desc[50];
	int pos;
	unsigned int count, seekpos;
	unsigned char anz;


	fseeko(ppf, 6,SEEK_SET);  /* Read Desc.line  64 no work */
	fread(&desc, 1, 50, ppf); desc[50]=0;
	printf("Patchfile is a PPF1.0 patch. Patch Information:\n");
	printf("Description : %s\n",desc);
	printf("File_id.diz : no\n");

	printf("Patching... "); fflush(stdout);
	fseeko(ppf, 0, SEEK_END);
	count=ftell(ppf);
	count-=56;
	seekpos=56;
	printf("Patching ... ");

	do{
		printf("reading...\b\b\b\b\b\b\b\b\b\b"); fflush(stdout);
		fseeko(ppf, seekpos, SEEK_SET);
		fread(&pos, 1, 4, ppf);
                Endian32_Swap (pos);			// <Hu Kares> little to big endian
		fread(&anz, 1, 1, ppf);
		fread(&ppfmem, 1, anz, ppf);
		fseeko(bin, pos, SEEK_SET);
		printf("writing...\b\b\b\b\b\b\b\b\b\b"); fflush(stdout);
		fwrite(&ppfmem, 1, anz, bin);
		seekpos=seekpos+5+anz;
		count=count-5-anz;
	} while(count!=0);

	printf("successful.\n");

}

//////////////////////////////////////////////////////////////////////
// Applies a PPF2.0 patch.
int ApplyPPF2Patch(FILE *ppf, FILE *bin){
		char desc[50], in;
		unsigned int binlen, obinlen, count, seekpos;
		int idlen, pos;
		unsigned char anz;



		fseeko(ppf, 6,SEEK_SET);
		fread(&desc, 1, 50, ppf); desc[50]=0;
		printf("Patchfile is a PPF2.0 patch. Patch Information:\n");
		printf("Description : %s\n",desc);
		printf("File_id.diz : ");
		idlen=ShowFileId(ppf, 2);
		if(!idlen) printf("not available\n");

		fseeko(ppf, 56, SEEK_SET);
		fread(&obinlen, 1, 4, ppf);
                Endian32_Swap (obinlen);		// <Hu Kares> little to big endian
        fseeko(bin, 0, SEEK_END);
        binlen=ftell(bin);
        if(obinlen!=binlen){
			return -1;
			/*printf("The size of the bin file isn't correct, continue ? (y/n): "); fflush(stdout);
			in=getc(stdin);
			if(in!='y'&&in!='Y'){
				printf("Aborted...\n");
				return;
			}*/
		}

		fflush(stdin);
		fseeko(ppf, 60, SEEK_SET);
		fread(&ppfblock, 1, 1024, ppf);
		fseeko(bin, 0x9320, SEEK_SET);
		fread(&binblock, 1, 1024, bin);
		in=memcmp(ppfblock, binblock, 1024);
		if(in!=0){
			/*printf("Binblock/Patchvalidation failed. continue ? (y/n): "); fflush(stdout);

#if defined(__APPLE__) || defined (MACOSX)

                        if(obinlen!=binlen) {		// <Hu Kares> required, since fflush doesn't flush '\n'!
                            in=getc(stdin);
                        }

#endif /* __APPLE__ || MACOSX *//*

			in=getc(stdin);
			if(in!='y'&&in!='Y'){
				printf("Aborted...\n");
				return;
			}*/
			return -2;
		}

		printf("Patching... "); fflush(stdout);
		fseeko(ppf, 0, SEEK_END);
		count=ftell(ppf);
		seekpos=1084;
		count-=1084;
		if(idlen) count-=idlen+38;

        do{
			printf("reading...\b\b\b\b\b\b\b\b\b\b"); fflush(stdout);
			fseeko(ppf, seekpos, SEEK_SET);
			fread(&pos, 1, 4, ppf);
                        Endian32_Swap (pos);		// <Hu Kares> little to big endian
			fread(&anz, 1, 1, ppf);
			fread(&ppfmem, 1, anz, ppf);
			fseeko(bin, pos, SEEK_SET);
			printf("writing...\b\b\b\b\b\b\b\b\b\b"); fflush(stdout);
			fwrite(&ppfmem, 1, anz, bin);
			seekpos=seekpos+5+anz;
			count=count-5-anz;
        } while(count!=0);

		printf("successful.\n");
		return 0;
}
//////////////////////////////////////////////////////////////////////
// Applies a PPF3.0 patch.
int ApplyPPF3Patch(FILE *ppf, FILE *bin, char mode){
	char desc[50], imagetype=0, in;
        unsigned char	undo=0, blockcheck=0;
	int idlen;
	off_t offset, count;			// <Hu Kares> count has to be 64 bit!
	unsigned int seekpos;
	unsigned char anz=0;


	fseeko(ppf, 6,SEEK_SET);  /* Read Desc.line */
	fread(&desc, 1, 50, ppf); desc[50]=0;
	printf("Patchfile is a PPF3.0 patch. Patch Information:\n");
	printf("Description : %s\n",desc);
	printf("File_id.diz : ");

	idlen=ShowFileId(ppf, 3);
	if(!idlen) printf("not available\n");

	fseeko(ppf, 56, SEEK_SET);
	fread(&imagetype, 1, 1, ppf);
	fseeko(ppf, 57, SEEK_SET);
	fread(&blockcheck, 1, 1, ppf);
	fseeko(ppf, 58, SEEK_SET);
	fread(&undo, 1, 1, ppf);

	if(mode==UNDO){
		if(!undo){
			printf("Error: no undo data available\n");
			return -4;
		}
	}

	if(blockcheck){
		fflush(stdin);
		fseeko(ppf, 60, SEEK_SET);
		fread(&ppfblock, 1, 1024, ppf);

		if(imagetype){
			fseeko(bin, 0x80A0, SEEK_SET);
		} else {
			fseeko(bin, 0x9320, SEEK_SET);
		}
		fread(&binblock, 1, 1024, bin);
		in=memcmp(ppfblock, binblock, 1024);
		if(in!=0){
			/*printf("Binblock/Patchvalidation failed. continue ? (y/n): "); fflush(stdout);
			in=getc(stdin);
			if(in!='y'&&in!='Y'){
				printf("Aborted...\n");
				return;
			}*/
			if(!ignoreChecksum)
			{
				return -3;
			}
			
		}
	}

	fseeko(ppf, 0, SEEK_END);
	count=ftello(ppf);				// <Hu Kares> 64 bit!
	fseeko(ppf, 0, SEEK_SET);
	
	if(blockcheck){
		seekpos=1084;
		count-=1084;
	} else {
		seekpos=60;
		count-=60;
	}

	if(idlen) count-=(idlen+18+16+2);
	

	printf("Patching ... ");
	fseeko(ppf, seekpos, SEEK_SET);
	do{
		printf("reading...\b\b\b\b\b\b\b\b\b\b"); fflush(stdout);
		fread(&offset, 1, 8, ppf);
                Endian64_Swap(offset);			// <Hu Kares> little to big endian
		fread(&anz, 1, 1, ppf);

		if(mode==APPLY){
			fread(&ppfmem, 1, anz, ppf);
			if(undo) fseeko(ppf, anz, SEEK_CUR);
		}
                else {
                    if(mode==UNDO){
                            fseeko(ppf, anz, SEEK_CUR);
                            fread(&ppfmem, 1, anz, ppf);
                    }
                }
    
		printf("writing...\b\b\b\b\b\b\b\b\b\b"); fflush(stdout);		
		fseeko(bin, offset, SEEK_SET);
		fwrite(&ppfmem, 1, anz, bin);
		count-=(anz+9);
		if(undo) count-=anz;

	} while(count!=0);

		printf("successful.\n");
		return 0;

}


//////////////////////////////////////////////////////////////////////
// Shows File_Id.diz of a PPF2.0 / PPF3.0 patch.
// Input: 2 = PPF2.0
// Input: 3 = PPF3.0
// Return 0 = Error/no fileid.
// Return>0 = Length of fileid.
int ShowFileId(FILE *ppf, int ppfver){
	char buffer2[3073];
	unsigned int idmagic;
	int lenidx=0, idlen=0, orglen=0;


	if(ppfver==2){
		lenidx=4;
	} else {
		lenidx=2;
	}

	fseeko(ppf,-(lenidx+4),SEEK_END);
	fread(&idmagic, 1, 4, ppf);
        Endian32_Swap (idmagic);			// <Hu Kares> little to big endian
	if(idmagic!='ZID.'){
		return(0);
	} else {
		fseeko(ppf,-lenidx,SEEK_END);
		fread(&idlen, 1, lenidx, ppf);
                Endian32_Swap (idlen);			// <Hu Kares> little to big endian
                orglen = idlen;
                if (idlen > 3072) {			// <Hu Kares> to be secure: avoid segmentation fault!
                    idlen = 3072;
                }
		fseeko(ppf,-(lenidx+16+idlen),SEEK_END);
		fread(&buffer2, 1, idlen, ppf);
		buffer2[idlen]=0;
		printf("available\n%s\n",buffer2);
	}

	return(orglen);
}

//////////////////////////////////////////////////////////////////////
// Check what PPF version we have.
// Return: 0 - File is no PPF.
// Return: 1 - File is a PPF1.0
// Return: 2 - File is a PPF2.0
// Return: 3 - File is a PPF3.0
int PPFVersion(FILE *ppf){
	unsigned int magic;

	fseeko(ppf,0,SEEK_SET);
	fread(&magic, 1, 4, ppf);
        Endian32_Swap (magic);				// <Hu Kares> little to big endian
	switch(magic){
			case '1FPP'		:	return(1);
			case '2FPP'		:	return(2);
			case '3FPP'		:	return(3);
			default			:   printf("Error: patchfile is no ppf patch\n"); break;
	}

	return(0);
}


//////////////////////////////////////////////////////////////////////
// Open all needed files.
// Return: 0 - Successful
// Return: 1 - Failed.
int OpenFiles(const char* file1, const char* file2){

	bin=fopen(file1, "rb+");
	if(!bin){
		printf("Error: cannot open file '%s' ",file1);
		return(1);
	}

	ppf=fopen(file2,  "rb");
	if(!ppf){
		printf("Error: cannot open file '%s' ",file2);
		fclose(bin);
		return(1);
	}

	return(0);
}


extern "C" /* specify the C calling convention */  
JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_ppfPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath,

	 jint jignoreChecksum)          /* argument #2 */ 

{ 
	ignoreChecksum=(int) jignoreChecksum;
     const char *str1 = env->GetStringUTFChars(romPath, 0); 
	 const char *str2 = env->GetStringUTFChars(patchPath, 0); 
	 OpenFiles(str1, str2);
	 int v = PPFVersion(ppf);
	 int e=1;
	 switch(v){
	 case 1:
		 ApplyPPF1Patch(ppf, bin);
		 e=0;
	 case 2:
		 e=ApplyPPF2Patch(ppf, bin);
	 case 3:
		 e=ApplyPPF3Patch(ppf, bin, APPLY);
	 }
     env->ReleaseStringUTFChars(romPath, str1); 
	env->ReleaseStringUTFChars(patchPath, str2); 
     return e;

} 

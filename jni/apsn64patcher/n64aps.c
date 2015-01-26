/* Apply an APS (Advanced Patch System) File for N64 Images
 * (C)1998 Silo / BlackBag
 *
 * Version 1.2 981217
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


#define MESSAGE   "\nN64APS v1.2 (BETA) Build 981217\n"
#define COPYRIGHT "(C)1998 Silo/BlackBag (Silo@BlackBag.org)\n\n"

#define BUFFERSIZE 255

char Magic[]="APS10";
#define MagicLength 5

#define TYPE_N64 1
unsigned char PatchType=TYPE_N64;

#define DESCRIPTION_LEN 50

#define ENCODINGMETHOD 0 // Very Simplistic Method

unsigned char EncodingMethod=ENCODINGMETHOD;

FILE	*APSFile;
FILE	*ORGFile;
int		 Quiet=0;

void
syntax (void)
{
	printf ("%s", MESSAGE);
	printf ("%s", COPYRIGHT);
	printf ("N64APS <options> <Original File> <APS File>\n");
    printf (" -f                 : Force Patching Over Incorrect Image\n");
	printf (" -q                 : Quiet Mode\n");
	fflush (stdout);
}

int CheckFile (char *Filename,char *mode)
{
	FILE	*fp;

	fp=fopen (Filename,mode);
	if (fp == NULL)
		return (0);
	else
	{
		fclose (fp);
		if (mode[0] == 'w')
			unlink (Filename);
		return (1);
	}
}

void
ReadStdHeader ()
{
	char aMagic[MagicLength];
	char Description[DESCRIPTION_LEN+1];

	fread (aMagic,1,MagicLength,APSFile);
	if (strncmp (aMagic,Magic,MagicLength) != 0)
	{
		printf ("Not a Valid Patch File\n");
		fclose (ORGFile);
		fclose (APSFile);
		exit (1);
	}
	fread (&PatchType,sizeof (PatchType),1,APSFile);
	if (PatchType != 1) // N64 Patch
	{
		printf ("Unable to Process Patch File\n");
		fclose (ORGFile);
		fclose (APSFile);
		exit (1);
	}
	fread (&EncodingMethod,sizeof (EncodingMethod),1,APSFile);
	if (EncodingMethod != 0) // Simple Encoding
	{
		printf ("Unknown or New Encoding Method\n");
		fclose (ORGFile);
		fclose (APSFile);
		exit (1);
	}

	fread (Description,1,DESCRIPTION_LEN,APSFile);
	Description[DESCRIPTION_LEN]=0;
	if (!Quiet)
	{
		printf ("Description : %s\n",Description);
		fflush (stdout);
	}
}

void
ReadN64Header (int Force)
{
	unsigned long MagicTest;
	unsigned char Buffer[8];
	unsigned char APSBuffer[8];
	int c;
	unsigned char CartID[2],Temp;
	unsigned char Teritory,APSTeritory;
    unsigned char APSFormat;

	fseek (ORGFile,0,SEEK_SET);
	fread (&MagicTest,sizeof (MagicTest),1,ORGFile);
	fread (Buffer,1,1,APSFile);
    APSFormat=Buffer[0];

	if (((MagicTest == 0x12408037) && (Buffer[0]==1)) || ((MagicTest != 0x12408037 && (Buffer[0]==0))))// 0 for Doctor Format, 1 for Everything Else
	{
		printf ("Image is in the wrong format\n");
		fclose (ORGFile);
		fclose (APSFile);
		exit (1);
	}

	fseek (ORGFile,60, SEEK_SET); // Cart ID
	fread (CartID,1,2,ORGFile);
	fread (Buffer,1,2,APSFile);
	if (MagicTest == 0x12408037) // Doc
	{
		Temp=CartID[0];
		CartID[0]=CartID[1];
		CartID[1]=Temp;
	}
	if ((Buffer[0] != CartID[0]) || (Buffer[1] != CartID[1]))
	{
		printf ("Not the Same Image\n");
		fclose (ORGFile);
		fclose (APSFile);
		exit (1);
	}

	if (MagicTest == 0x12408037)
		fseek (ORGFile,63,SEEK_SET); // Teritory
	else
		fseek (ORGFile,62,SEEK_SET);

	fread (&Teritory,sizeof (Teritory),1,ORGFile);
	fread (&APSTeritory,sizeof (APSTeritory),1,APSFile);
	if (Teritory != APSTeritory)
	{
		printf ("Wrong Country\n");
		if (!Force)
		{
			fclose (ORGFile);
			fclose (APSFile);
			exit (1);
		}
	}

	fseek (ORGFile,0x10, SEEK_SET); // CRC Header Position
	fread (Buffer,1,8,ORGFile);
	fread (APSBuffer,1,8,APSFile);

	if (MagicTest == 0x12408037) // Doc
    {
        Temp=Buffer[0];
        Buffer[0]=Buffer[1];
        Buffer[1]=Temp;
        Temp=Buffer[2];
        Buffer[2]=Buffer[3];
        Buffer[3]=Temp;
        Temp=Buffer[4];
        Buffer[4]=Buffer[5];
        Buffer[5]=Temp;
        Temp=Buffer[6];
        Buffer[6]=Buffer[7];
        Buffer[7]=Temp;
    }

	if ((APSBuffer[0] != Buffer[0]) || (APSBuffer[1] != Buffer[1]) ||
        (APSBuffer[2] != Buffer[2]) || (APSBuffer[3] != Buffer[3]) ||
     	(APSBuffer[4] != Buffer[4]) || (APSBuffer[5] != Buffer[5]) ||
	    (APSBuffer[6] != Buffer[6]) || (APSBuffer[7] != Buffer[7]))
	{
		if (!Quiet) { printf ("Incorrect Image\n"); fflush (stdout); }
		if (!Force)
		{
			fclose (ORGFile);
			fclose (APSFile);
			exit (1);
		}
	}

	fseek (ORGFile,0,SEEK_SET);

	c=fgetc(APSFile);
	c=fgetc(APSFile);
	c=fgetc(APSFile);
	c=fgetc(APSFile);
	c=fgetc(APSFile);
}


void
ReadSizeHeader (char *File1)
{
	long OrigSize;
	long APSOrigSize;
	unsigned char t;
	long i;

	fseek (ORGFile,0,SEEK_END);

	OrigSize = ftell (ORGFile);

	fread (&APSOrigSize,sizeof (APSOrigSize),1,APSFile);

	if (OrigSize != APSOrigSize) // Do File Resize
	{
		if (APSOrigSize < OrigSize)
		{
			int x;
			fclose (ORGFile);
			x = open (File1, O_WRONLY);
			if (ftruncate (x, APSOrigSize) != 0)
			{
				printf ("Trunacte Failed\n");
			}
			close (x);
			ORGFile = fopen (File1,"rb+");
		}
		else
		{
			t = 0;
			for (i=0;i<(APSOrigSize-OrigSize);i++) fputc (t,ORGFile);
		}
	}

	fseek (ORGFile,0,SEEK_SET);
}


void
ReadPatch ()
{
	int APSReadLen;
	int Finished=0;
	unsigned char Buffer[256];
	long Offset;
	unsigned char Size;

	while (!Finished)
	{
		APSReadLen = fread (&Offset,sizeof (Offset),1,APSFile);
		if (APSReadLen == 0)
		{
			Finished=1;
		}
		else
		{
			fread (&Size,sizeof (Size),1,APSFile);
			if (Size != 0)
			{
				fread (Buffer,1,Size,APSFile);
				if ((fseek (ORGFile,Offset,SEEK_SET)) != 0)
				{
					printf ("Seek Failed\n");
					fflush (stdout);
					exit (1);
				}
				fwrite (Buffer,1,Size,ORGFile);
			}
			else
			{
				unsigned char data;
				unsigned char len;
				int i;

				fread (&data,sizeof(data),1,APSFile);
				fread (&len,sizeof(data),1,APSFile);

				if ((fseek (ORGFile,Offset,SEEK_SET)) != 0)
				{
					printf ("Seek Failed\n");
					fflush (stdout);
					exit (1);
				}

				for (i=0;i<len;i++) fputc (data,ORGFile);
			}
		}
	}
}


int
main (int argc, char **argv)
{
	int		Result;
	char	File1[1024];
	char	File2[1024];
	int		Force=0;
	while ((Result = getopt (argc, argv, "1:2:fq")) != -1)
	{
		switch (Result)
		{
		case 'f': // Force
			Force=1;
			break;
		case 'q': // Quiet
			Quiet=1;
			break;
		case '?':
			printf ("Unknown option %c\n", Result);
			syntax();
			return (1);
			break;
		}
	}
	if (argc - optind != 2)
	{
		syntax();
		return (1);
	}

	strcpy (File1,argv[optind]);
	strcpy (File2,argv[optind+1]);
	if (!CheckFile (File1,"rb+")) return(1);
	if (!CheckFile (File2,"rb")) return(1);

	ORGFile = fopen (File1,"rb+");
	APSFile = fopen (File2,"rb");

	if (!Quiet)
	{
		printf ("%s", MESSAGE);
		printf ("%s", COPYRIGHT);
		fflush (stdout);
	}
	ReadStdHeader ();

	ReadN64Header (Force);

	ReadSizeHeader (File1);

	ReadPatch ();
	//fclose (NEWFile);
	fclose (ORGFile);
	fclose (APSFile);
	return (0);
}

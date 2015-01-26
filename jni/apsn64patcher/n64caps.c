/* Create APS (Advanced Patch System) for N64 Images
 * (C)1998 Silo / BlackBag
 *
 * Version 1.2 981217
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef TRUE
#define TRUE  1
#define FALSE !TRUE
#endif

#define MESSAGE   "\nN64CAPS v1.2 (BETA) Build 981217\n"
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
FILE	*NEWFile;
int		Quiet=FALSE;
int		ChangeFound;

void
syntax (void)
{
	printf ("%s", MESSAGE);
	printf ("%s", COPYRIGHT);
	printf ("N64CAPS <options> <Original File> <Modified File> <Output APS File>\n");
	printf (" -d %c<Image Title>%c : Description\n", 34, 34);
	printf (" -q                 : Quiet Mode\n");
	fflush (stdout);
}

int CheckFile (char *Filename,char *mode,int Image)
{
	FILE	*fp;
	unsigned long MagicTest;

	fp=fopen (Filename,mode);
	if (fp == NULL)
		return (FALSE);
	else
	{
		if (Image)
		{
			fread (&MagicTest,sizeof (MagicTest),1,fp);

			fclose (fp);
			if (mode[0] == 'w')
				unlink (Filename);

			if (MagicTest != 0x12408037 && MagicTest != 0x40123780) // Invalid Image
			{
				if (!Quiet)
				{
					printf ("%s is an Invalid Image\n",Filename);
					fflush (stdout);
				}
				return (FALSE);
			}
			return (TRUE);
		}
	}
	return (TRUE);
}

void
WriteStdHeader (char *Desc)
{
	char Description[DESCRIPTION_LEN];

	fwrite (Magic,1,MagicLength,APSFile);
	fwrite (&PatchType,sizeof (PatchType),1,APSFile);
	fwrite (&EncodingMethod,sizeof (EncodingMethod),1,APSFile);

	memset (Description,' ',DESCRIPTION_LEN);
	strcpy (Description,Desc);
	Description[strlen(Desc)]=' ';

	fwrite (Description,1,DESCRIPTION_LEN,APSFile);

}

void
WriteN64Header ()
{
	unsigned long MagicTest;
	unsigned char Buffer[8];
	unsigned char Teritory;
	unsigned char CartID[2],Temp;

	fread (&MagicTest,sizeof (MagicTest),1,ORGFile);

	if (MagicTest == 0x12408037) // 0 for Doctor Format, 1 for Everything Else
		fputc (0,APSFile);
	else
		fputc (1,APSFile);


	fseek  (ORGFile,60,SEEK_SET);
	fread  (CartID,1,2,ORGFile);
	if (MagicTest == 0x12408037) // Doc
	{
		Temp=CartID[0];
		CartID[0]=CartID[1];
		CartID[1]=Temp;
	}
	fwrite (CartID,1,2,APSFile);

	if (MagicTest == 0x12408037) // Doc
		fseek (ORGFile,63,SEEK_SET);
	else
		fseek (ORGFile,62,SEEK_SET);
	fread  (&Teritory,sizeof (Teritory),1,ORGFile);
	fwrite (&Teritory,sizeof (Teritory),1,APSFile);

	fseek (ORGFile,0x10, SEEK_SET); // CRC Header Position
	fread (Buffer,1,8,ORGFile);

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

	fwrite (Buffer,1,8,APSFile);

	fseek (ORGFile,0,SEEK_SET);

	fputc (0,APSFile); // PAD
	fputc (0,APSFile); // PAD
	fputc (0,APSFile); // PAD
	fputc (0,APSFile); // PAD
	fputc (0,APSFile); // PAD
}


void
WriteSizeHeader ()
{
	long NEWSize,ORGSize;

	fseek (NEWFile,0,SEEK_END);
	fseek (ORGFile,0,SEEK_END);

	NEWSize = ftell (NEWFile);
	ORGSize = ftell (ORGFile);

	fwrite (&NEWSize,sizeof (NEWSize),1,APSFile);

	if (ORGSize != NEWSize) ChangeFound=TRUE;

	fseek (NEWFile,0,SEEK_SET);
	fseek (ORGFile,0,SEEK_SET);
}


void
WritePatch ()
{
	long ORGReadLen;
	long NEWReadLen;
	int Finished=FALSE;
	unsigned char ORGBuffer[BUFFERSIZE];
	unsigned char NEWBuffer[BUFFERSIZE];
	long FilePos;
	int i;
	long ChangedStart=0;
	long ChangedOffset=0;
	int ChangedLen=0;
	int State;

	fseek (ORGFile,0,SEEK_SET);
	fseek (NEWFile,0,SEEK_SET);

	FilePos = 0;

	while (!Finished)
	{
		ORGReadLen = fread (ORGBuffer,1,BUFFERSIZE,ORGFile);
		NEWReadLen = fread (NEWBuffer,1,BUFFERSIZE,NEWFile);

		if (ORGReadLen != NEWReadLen)
		{
			int a;

			for (a=ORGReadLen;a<NEWReadLen;a++) ORGBuffer[a]=0;
		}

		i=0;
		State=0;

		if (NEWReadLen != 0)
		{
		while (i<NEWReadLen)
		{
			switch (State)
			{
			case 0:
				if (NEWBuffer[i] != ORGBuffer[i])
				{
					State=1;
					ChangedStart=FilePos+i;
					ChangedOffset=i;
					ChangedLen=1;
					ChangeFound=TRUE;
				}
				i++;
				break;
			case 1:
				if (NEWBuffer[i] != ORGBuffer[i])
				{
					ChangedLen++;
					i++;
				}
				else
					State=2;
				break;
			case 2:
				fwrite (&ChangedStart,sizeof (ChangedStart),1,APSFile);
				fputc  ((ChangedLen&0xff),APSFile);
				fwrite (NEWBuffer+(ChangedOffset),1,ChangedLen,APSFile);
				State=0;
				break;
			}
		}
		}

		if (State!=0)
		{
			if (ChangedLen==0) ChangedLen=255;
			fwrite (&ChangedStart,sizeof (ChangedStart),1,APSFile);
			fputc ((ChangedLen&0xff),APSFile);
			fwrite (NEWBuffer+(ChangedOffset),1,ChangedLen,APSFile);
		}

		if (NEWReadLen==0) Finished=TRUE;
		FilePos+=NEWReadLen;
	}
}


int
main (int argc, char *argv[])
{
	int		Result;
	char	File1[256];
	char	File2[256];
	char	OutFile[256];
	char	Description[81];

	ChangeFound = FALSE;

	Description[0]=0;

	while ((Result = getopt (argc, argv, "d:q")) != -1)
	{
		switch (Result)
		{
		case 'd': // Description
			strcpy (Description, optarg);
			break;
		case 'q': // Quiet
			Quiet=TRUE;
			break;
		case '?':
			printf ("Unknown option %c\n", Result);
			syntax();
			return (1);
			break;
		}
	}

	if ((argc - optind) != 3)
	{
		syntax();
		return (1);
	}

	strcpy (File1,argv[optind]);
	strcpy (File2,argv[optind+1]);
	strcpy (OutFile,argv[optind+2]);

	if (!CheckFile (File1,"rb",TRUE)) return(1);
	if (!CheckFile (File2,"rb",TRUE)) return(1);
	if (!CheckFile (OutFile,"wb",FALSE)) return(1);

	APSFile = fopen (OutFile,"wb");
	ORGFile = fopen (File1,"rb");
	NEWFile = fopen (File2,"rb");

	if (!Quiet)
	{
		printf ("%s", MESSAGE);
		printf ("%s", COPYRIGHT);
		printf ("Writing Headers...");
		fflush (stdout);
	}

	WriteStdHeader (Description);

	WriteN64Header ();

	WriteSizeHeader ();

	if (!Quiet) { printf ("Done\nFinding Changes..."); fflush (stdout); }

	WritePatch ();

	if (!Quiet) { printf ("Done\n"); fflush (stdout); }

	fclose (NEWFile);
	fclose (ORGFile);
	fclose (APSFile);

	if (!Quiet && !ChangeFound)
	{
		printf ("No Changes Found\n");
		fflush (stdout);
		unlink (OutFile);
	}

	return (0);
}

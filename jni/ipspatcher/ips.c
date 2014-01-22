#include "ips.h"

/* 
 * Reset ips patch structure
 */
void IPSReset(struct IPSPatch *ips)
{
	ips->patch = NULL;	
	
	ips->rom      = NULL;
	ips->romSize  = 0;
	
	ips->record.offset  = 0;
	ips->record.size    = 0;
	ips->record.rleData = 0;
}

/*
 * Open and check patch file
 */
int8_t IPSOpenPatch(struct IPSPatch *ips, const char *fileName)
{
	char   header[5];
	size_t nRead;
	
	ips->patch = fopen(fileName, "rb");
	if(ips->patch == NULL)
	{
		return IPS_ERROR;
	}
	
	fseek(ips->patch, 0, SEEK_SET);
	
	/* Read header and check it */
	nRead = fread(header, 1, 5, ips->patch);
	if(nRead != 5)
	{
		return IPS_ERROR_FILE_TYPE;
	}
	
	if((header[0] != 'P') ||
	   (header[1] != 'A') ||
	   (header[2] != 'T') ||
	   (header[3] != 'C') ||
	   (header[4] != 'H'))
	{
		return IPS_ERROR_FILE_TYPE;
	}
	
	return IPS_OK;
}

/*
 * Open rom and make a backup of it 
 */
int8_t IPSOpenInOUT(struct IPSPatch *ips,
                    const char *romName)
{
	int      err;
	char     backupFileName[520];
	uint32_t backupFileExtOffset;
	FILE     *backup;
	long     nTotal;
	size_t   nRead;
	char     buffer[256];

	if(romName == NULL)
	{
		return IPS_ERROR;
	}

	/* Create backup file name */
	strncpy(backupFileName, romName, 512);
		
	backupFileExtOffset = strlen(romName);
	if(backupFileExtOffset > 512)
	{
		backupFileExtOffset = 512;
	}
		
	backupFileName[backupFileExtOffset++] = '.';
	backupFileName[backupFileExtOffset++] = 'b';
	backupFileName[backupFileExtOffset++] = 'a';
	backupFileName[backupFileExtOffset++] = 'k';
	backupFileName[backupFileExtOffset++] = '\0';
		
	/* Open rom */
	ips->rom = fopen(romName, "rb+");
	if(ips->rom == NULL)
	{
		return IPS_ERROR;
	}
	
	/* Get its size */
	fseek(ips->rom, 0, SEEK_END);
	nTotal  = ftell(ips->rom);
	fseek(ips->rom, 0, SEEK_SET);
	nTotal -= ftell(ips->rom);
	
	ips->romSize = (uint32_t)nTotal;
	if(nTotal <= 0)
	{
		return IPS_ERROR;
	}

	/* Open backup */
	backup = fopen(backupFileName, "wb");
	if(backup == NULL)
	{
		return IPS_ERROR;
	}
	
	/* A copy rom data to it */
	fseek(backup, 0, SEEK_SET);
	for(nTotal=0; nTotal<ips->romSize; nTotal+=nRead)
	{
		nRead = fread(buffer, 1, 256, ips->rom);
		fwrite(buffer, 1, nRead, backup);
	}		
	fclose(backup);

	/* Reset rom file pointer */
	fseek(ips->rom, 0, SEEK_SET);
	
	return IPS_OK;
}

/*
 * Open rom and patch
 */
int8_t IPSOpen(struct IPSPatch *ips,
               const char *patchName,
               const char *romName)
{
	int8_t err;
	
	ips->patch = NULL;
	
	ips->rom     = NULL;
	ips->romSize = 0;
		
	ips->record.offset = 0;
	ips->record.size   = 0;
	
	err = IPSOpenPatch(ips, patchName);
	if(err != IPS_OK)
	{
		goto error;
	}
	
	err = IPSOpenInOUT(ips, romName);
	if(err != IPS_OK)
	{
		goto error;
	}
		
	return IPS_OK;
error:
	if(ips->patch != NULL)
	{
		fclose(ips->patch);
	}
	if(ips->rom != NULL)
	{
		fclose(ips->rom);
	}
	ips->patch = NULL;
	ips->rom   = NULL;

	return IPS_ERROR_OPEN;
}

/*
 * Close patch and rom files 
 */
void IPSClose(struct IPSPatch *ips)
{
	if(ips->rom!= NULL)
	{
		fclose(ips->rom);
		ips->rom = NULL;
	}
	
	if(ips->patch != NULL)
	{
		fclose(ips->patch);
		ips->patch = NULL;
	}
}

/*
 * Read IPS record from file
 */
int8_t IPSReadRecord(struct IPSPatch *ips)
{
	uint8_t buffer[5];
	size_t  nRead;
	
	nRead = fread(buffer, 1, 3, ips->patch);
	if(nRead < 3)
	{
		return IPS_ERROR_READ;
	}
	/* Is it the end of the patch ? */
	if((buffer[0] == 'E') &&
	   (buffer[1] == 'O') &&
	   (buffer[2] == 'F'))
	{
		return IPS_PATCH_END;
	}
	
	/* Retrieve rom offset */
	ips->record.offset = (buffer[0] << 16) | 
	                     (buffer[1] <<  8) |
	                     (buffer[2]      );
	
	/* Data size */
	nRead = fread(buffer, 1, 2, ips->patch);
	if(nRead < 2)
	{
		return IPS_ERROR_READ;
	}
	ips->record.size = (buffer[0] << 8) | 
	                   (buffer[1]     );

	/* We have a RLE section if data size is zero */
	if(ips->record.size == 0)
	{
		ips->record.offset |= IPS_RECORD_RLE << 24;
		
		/* Read RLE size */
		nRead = fread(buffer, 1, 2, ips->patch);
		if(nRead < 2)
		{
			return IPS_ERROR_READ;
		}
		ips->record.size = (buffer[0] << 8) | 
		                   (buffer[1]     );
		
		/* Read byte data to copy */
		nRead = fread(&(ips->record.rleData), 1, 1, ips->patch);
		if(nRead < 1)
		{
			return IPS_ERROR_READ;
		}
	}
	
	return IPS_OK;
}

/*
 * Process current record
 */
int8_t IPSProcessRecord (struct IPSPatch *ips)
{
	uint32_t i;
	uint8_t  byte;
	uint32_t offset;

	offset = IPS_RECORD_OFFSET(ips->record);
	if(offset > ips->romSize)
	{
		ips->romSize += ips->record.size;
		//return IPS_ERROR_OFFSET;
	}

	fseek(ips->rom, offset, SEEK_SET);
	
	if(IPS_RECORD_INFO(ips->record) == IPS_RECORD_RLE)
	{
		for(i=0; i<ips->record.size; ++i)
		{
			fwrite(&(ips->record.rleData), 1, 1, ips->rom);
		}
	}
	else
	{	
		for(i=0; i<ips->record.size; ++i)
		{
			fread (&byte, 1, 1, ips->patch);
			fwrite(&byte, 1, 1, ips->rom);
		}
	}

	/* Update rom size if needed */
	if((offset + ips->record.size) > ips->romSize)
	{
		ips->romSize += ips->record.size;
	}
	
	return IPS_OK;
}

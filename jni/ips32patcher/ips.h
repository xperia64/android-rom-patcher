#ifndef _IPS_H_
#define _IPS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define IPS32_OK 1
#define IPS32_ERROR -1
#define IPS32_ERROR_FILE_TYPE -2
#define IPS32_ERROR_OPEN -3
#define IPS32_ERROR_SAVE -4
#define IPS32_ERROR_READ -5
#define IPS32_ERROR_PROCESS -6
#define IPS32_ERROR_OFFSET -7
#define IPS32_PATCH_END  0

#define IPS32_RECORD_OFFSET(record) ((record).offset & 0xffffffff)
// Don't need this since with have rleSwitch:
//#define IPS32_RECORD_INFO(record) (((record).offset >> 24)&0xff)
#define IPS32_RECORD_RLE 1

/*
 * Structures
 */

struct IPSRecord
{
	uint32_t offset;
	uint32_t size;
	uint8_t	rleSwitch;
	uint32_t  rleData;
};

struct IPSPatch
{
	FILE             *rom;
	uint32_t         romSize;
	FILE             *patch;
	struct IPSRecord record;
};

/* 
 * Reset ips patch structure
 */
void IPSReset(struct IPSPatch *ips);

/*
 * Open rom and patch
 */
int8_t IPSOpen(struct IPSPatch *ips,
               const char *patchName,
               const char *romName);
   
/*
 * Close patch and rom files 
 */           
void IPSClose(struct IPSPatch *ips);

/*
 * Read IPS record from file
 */
int8_t IPSReadRecord(struct IPSPatch *ips);

/*
 * Process current record
 */
int8_t IPSProcessRecord (struct IPSPatch *ips);

#endif /* _IPS_H_ */

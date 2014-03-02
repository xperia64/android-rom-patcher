////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "fakecrc - Fake the CRC32 of a file"
#define COPYR "Copyright (C) 2010 Neill Corlett"
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "banner.h"

////////////////////////////////////////////////////////////////////////////////

static uint32_t crctable_f[256];
static uint32_t crctable_r[256];

static void crc_init(void) {
    uint32_t i, j, k;
    for(i = 0; i < 256; i++) {
        j = i;
        for(k = 0; k < 8; k++) {
            j = (j >> 1) ^ ((j & 1) ? 0x80000000 : 0x6DB88320);
        }
        crctable_f[i] = j;
        j = i << 24;
        for(k = 0; k < 8; k++) {
            j = (j << 1) ^ ((((int32_t)j) < 0) ? 0x00000001 : 0xDB710640);
        }
        crctable_r[i] = j;
    }
}

static uint32_t crc_nextbyte(uint32_t crc, uint8_t byte) {
    return (crc >> 8) ^ crctable_f[(crc ^ byte) & 0xFF];
}

static uint32_t crc_prevbyte(uint32_t crc, uint8_t byte) {
    return (crc << 8) ^ byte ^ crctable_r[crc >> 24];
}

////////////////////////////////////////////////////////////////////////////////
//
// Compute CRC forward for a section of a file
//
static uint8_t crc_buffer[4096];

// Returns nonzero on error
static int crc_f(uint32_t crc, FILE* f, off_t start, off_t end, uint32_t* result) {
    if(start < 0) { printf("error - start is negative\n"); return 1; }
    if(end   < 0) { printf("error - end is negative\n"); return 1; }
    if(start > end) { printf("error - start > end\n"); return 1; }

    if(fseeko(f, start, SEEK_SET) != 0) {
        printf("seek error\n");
        return 1;
    }
    while(start < end) {
        off_t i;
        off_t diff = end - start;
        if(diff > ((off_t)sizeof(crc_buffer))) { diff = sizeof(crc_buffer); }

        if(fread(crc_buffer, 1, (size_t)diff, f) != (size_t)diff) {
            printf("read error\n");
            return 1;
        }
        for(i = 0; i < diff; i++) {
            crc = crc_nextbyte(crc, crc_buffer[i]);
        }

        start += diff;
    }
    if(result) { *result = crc; }
    return 0;
}

//
// Compute CRC reverse for a section of a file
//
// Returns nonzero on error
static int crc_r(uint32_t crc, FILE* f, off_t start, off_t end, uint32_t* result) {
    if(start < 0) { printf("error - start is negative\n"); return 1; }
    if(end   < 0) { printf("error - end is negative\n"); return 1; }
    if(start > end) { printf("error - start > end\n"); return 1; }

    while(start < end) {
        off_t i;
        off_t diff = end - start;
        if(diff > ((off_t)sizeof(crc_buffer))) { diff = sizeof(crc_buffer); }
        end -= diff;

        if(fseeko(f, end, SEEK_SET) != 0) {
            printf("seek error\n");
            return 1;
        }
        if(fread(crc_buffer, 1, (size_t)diff, f) != (size_t)diff) {
            printf("read error\n");
            return 1;
        }

        for(i = diff; i > 0; i--) {
            crc = crc_prevbyte(crc, crc_buffer[i - 1]);
        }
    }
    if(result) { *result = crc; }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns the 4 data bytes necessary to go from current->desired
//
static uint32_t crc32_preimage(uint32_t current_crc, uint32_t desired_crc) {
    uint32_t x = desired_crc ^ 0x2144DF1Clu;
    return current_crc ^
        (  x               & 0x47FF01FFlu) ^ (((x<< 1)|(x>>31)) & 0x8F0003C1lu) ^
        (((x<< 2)|(x>>30)) & 0xFE0FFE03lu) ^ (((x<< 3)|(x>>29)) & 0x3FE003C0lu) ^
        (((x<< 4)|(x>>28)) & 0x7830FE00lu) ^ (((x<< 5)|(x>>27)) & 0xF01F03DFlu) ^
        (((x<< 6)|(x>>26)) & 0x00C001C0lu) ^ (((x<< 7)|(x>>25)) & 0x3E00FC40lu) ^
        (((x<< 8)|(x>>24)) & 0x7F0FFEC0lu) ^ (((x<< 9)|(x>>23)) & 0xF9EF03FFlu) ^
        (((x<<10)|(x>>22)) & 0x0FDFFE00lu) ^ (((x<<11)|(x>>21)) & 0x07CF05C0lu) ^
        (((x<<12)|(x>>20)) & 0x30E00C00lu) ^ (((x<<13)|(x>>19)) & 0x7ECF0000lu) ^
        (((x<<14)|(x>>18)) & 0xC1E001FFlu) ^ (((x<<15)|(x>>17)) & 0x78CF03C0lu) ^
        (((x<<16)|(x>>16)) & 0xF61FFFFFlu) ^ (((x<<17)|(x>>15)) & 0x0F310000lu) ^
        (((x<<18)|(x>>14)) & 0x39E0FE00lu) ^ (((x<<19)|(x>>13)) & 0x4F37FC00lu) ^
        (((x<<20)|(x>>12)) & 0x8610003Flu) ^ (((x<<21)|(x>>11)) & 0xCF30FFFFlu) ^
        (((x<<22)|(x>>10)) & 0x79EFFFC0lu) ^ (((x<<23)|(x>> 9)) & 0xCF2001FFlu) ^
        (((x<<24)|(x>> 8)) & 0x793FFC00lu) ^ (((x<<25)|(x>> 7)) & 0xCF0F01FFlu) ^
        (((x<<26)|(x>> 6)) & 0x7AE003C0lu) ^ (((x<<27)|(x>> 5)) & 0xC9C0003Flu) ^
        (((x<<28)|(x>> 4)) & 0x78000000lu) ^ (((x<<29)|(x>> 3)) & 0xD70FFE3Flu) ^
        (((x<<30)|(x>> 2)) & 0x4E1F0200lu) ^ (((x<<31)|(x>> 1)) & 0xC03FFC3Flu);
}

////////////////////////////////////////////////////////////////////////////////

int main(
    int argc,
    char **argv
) {
    int returncode = 0;

    FILE *f = NULL;
    uint32_t i;
    uint32_t desired_crc = 0;

    off_t file_size;
    off_t patch_offset;

    uint32_t pre_crc  = 0;
    uint32_t post_crc = 0;
    uint32_t preimage = 0;

    uint8_t initial_bytes[4];
    uint8_t patched_bytes[4];

    normalize_argv0(argv[0]);

    crc_init();

    if(argc != 2 && argc != 3 && argc != 4) {
        banner();
        printf(
            "Usage:\n"
            "To obtain the CRC32 of a file:\n"
            "    %s file\n"
            "To modify the CRC32 of a file:\n"
            "    %s file desired_crc [offset]\n"
            "Patches 4 consecutive bytes at the given offset to force the file's CRC32.\n"
            "If no offset is given, the last 4 bytes of the file are used.\n",
            argv[0],
            argv[0]
        );
        goto error;
    }

    if(argc == 2) {
        //
        // Just get the file's CRC
        //
        f = fopen(argv[1], "rb");
        if(!f) { goto error_f; }

        if(fseeko(f, 0, SEEK_END) != 0) { goto error_f; }
        file_size = ftello(f);
        if(fseeko(f, 0, SEEK_SET) != 0) { goto error_f; }

        if(crc_f(0, f, 0, file_size, &pre_crc)) { goto error; }

        printf("0x%08lX\n", (unsigned long)pre_crc);
        goto done;
    }

    if(argc >= 3) {
        desired_crc = strtoul(argv[2], NULL, 0);
    }

    f = fopen(argv[1], "r+b");
    if(!f) { goto error_f; }

    if(fseeko(f, 0, SEEK_END) != 0) { goto error_f; }
    file_size = ftello(f);
    if(fseeko(f, 0, SEEK_SET) != 0) { goto error_f; }

    if(file_size < 4) {
        printf("error: file must be at least 4 bytes\n");
        goto error;
    }
    patch_offset = file_size - 4;
    if(argc >= 4) {
        patch_offset = strtoofft(argv[3], NULL, 0);
        if(patch_offset < 0) {
            printf("patch offset must not be negative\n");
            goto error;
        }
    }
    if(patch_offset > (file_size - 4)) {
        printf("offset 0x");
        fprinthex(stdout, patch_offset, 1);
        printf(" is out of range of the file\n");
        goto error;
    }

    //
    // Compute pre- and post-crc
    //
    if(crc_f(0x00000000 , f, 0               , patch_offset, &pre_crc )) { goto error; }
    if(crc_r(desired_crc, f, patch_offset + 4, file_size   , &post_crc)) { goto error; }

    //
    // Compute preimage from pre to post
    //
    preimage = crc32_preimage(pre_crc, post_crc);

    patched_bytes[0] = (uint8_t)(preimage >>  0);
    patched_bytes[1] = (uint8_t)(preimage >>  8);
    patched_bytes[2] = (uint8_t)(preimage >> 16);
    patched_bytes[3] = (uint8_t)(preimage >> 24);
    //
    // Write preimage at the patch offset
    //
    if(fseeko(f, patch_offset, SEEK_SET) != 0) { goto error_f; }
    if(fread (initial_bytes, 1, 4, f)    != 4) { goto error_f; }
    if(fseeko(f, patch_offset, SEEK_SET) != 0) { goto error_f; }
    if(fwrite(patched_bytes, 1, 4, f)    != 4) { goto error_f; }
    fflush(f);
    //
    // Done
    //
    for(i = 0; i < 4; i++) {
        printf("0x");
        fprinthex(stdout, patch_offset + i, 1);
        printf(": 0x%02X -> 0x%02X\n", initial_bytes[i], patched_bytes[i]);
    }
    printf("Done!  CRC32 is now 0x%08lX\n", (unsigned long)desired_crc);

    goto done;

error_f:
    printfileerror(f, argv[1]);
error:
    returncode = 1;

done:
    if(f != NULL) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

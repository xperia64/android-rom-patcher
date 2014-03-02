////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "cdpatch - CD-XA image insert/extract utility"
#define COPYR "Copyright (C) 2001,2011 Neill Corlett"
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

static const uint32_t max_path_depth = 256;

////////////////////////////////////////////////////////////////////////////////
//
// Program options
//
struct cdpatch_options {
    int8_t insert;
    int8_t extract;
    const char* binname;
    const char* basedir;
    int8_t big;
    int8_t little;
    int8_t boot;
    int8_t enforce_fscheck;
    int8_t overwrite;
    int8_t verbose;
    int8_t recurse;
    const char** files;
    int files_count;
};

static const char* fsoverride = " (use -f to override)";

////////////////////////////////////////////////////////////////////////////////

static int exists(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if(f) { fclose(f); return 1; }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

static const char* oom = "Error: Out of memory\n";

////////////////////////////////////////////////////////////////////////////////

static uint32_t get32lsb(const uint8_t* p) {
    return
        (((uint32_t)(p[0])) <<  0) |
        (((uint32_t)(p[1])) <<  8) |
        (((uint32_t)(p[2])) << 16) |
        (((uint32_t)(p[3])) << 24);
}

static uint32_t get32msb(const uint8_t* p) {
    return
        (((uint32_t)(p[0])) << 24) |
        (((uint32_t)(p[1])) << 16) |
        (((uint32_t)(p[2])) <<  8) |
        (((uint32_t)(p[3])) <<  0);
}

static void set32lsb(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)(value >>  0);
    p[1] = (uint8_t)(value >>  8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void set32msb(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >>  8);
    p[3] = (uint8_t)(value >>  0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns nonzero if any bytes in the array are nonzero
//
static int anynonzero(const uint8_t* data, size_t len) {
    for(; len; len--) {
        if(*data++) { return 1; }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert ISO9660 file size to sector count, rounding up
//
static uint32_t sectorcount(uint32_t size) {
    return (size >> 11) + ((size & 0x7FF) != 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// LUTs for computing ECC/EDC
//
static uint8_t  ecc_f_lut[256];
static uint8_t  ecc_b_lut[256];
static uint32_t edc_lut  [256];

static void eccedc_init(void) {
    uint32_t i, j, edc;
    for(i = 0; i < 256; i++) {
        j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
        ecc_f_lut[i    ] = (uint8_t)j;
        ecc_b_lut[i ^ j] = (uint8_t)i;
        edc = i;
        for(j = 0; j < 8; j++) {
            edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
        }
        edc_lut[i] = edc;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Compute EDC for a block
//
static void edc_computeblock(const uint8_t* src, size_t size, uint8_t* dest) {
    uint32_t edc = 0;
    while(size--) {
        edc = (edc >> 8) ^ edc_lut[(edc ^ (*src++)) & 0xFF];
    }
    set32lsb(dest, edc);
}

////////////////////////////////////////////////////////////////////////////////
//
// Compute ECC for a block (can do either P or Q)
//
static void ecc_computeblock(
    uint8_t* src,
    uint32_t major_count,
    uint32_t minor_count,
    uint32_t major_mult,
    uint32_t minor_inc,
    uint8_t* dest
) {
    uint32_t size = major_count * minor_count;
    uint32_t major, minor;
    for(major = 0; major < major_count; major++) {
        uint32_t index = (major >> 1) * major_mult + (major & 1);
        uint8_t ecc_a = 0;
        uint8_t ecc_b = 0;
        for(minor = 0; minor < minor_count; minor++) {
            uint8_t temp = src[index];
            index += minor_inc;
            if(index >= size) index -= size;
            ecc_a ^= temp;
            ecc_b ^= temp;
            ecc_a = ecc_f_lut[ecc_a];
        }
        ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
        dest[major              ] = ecc_a;
        dest[major + major_count] = ecc_a ^ ecc_b;
    }
}

//
// Generate ECC P and Q codes for a block
//
static void ecc_generate(uint8_t* sector, int zeroaddress) {
    uint8_t saved_address[4];
    //
    // Save the address and zero it out, if necessary
    //
    if(zeroaddress) {
        memmove(saved_address, sector + 12, 4);
        memset(sector + 12, 0, 4);
    }
    //
    // Compute ECC P code
    //
    ecc_computeblock(sector + 0xC, 86, 24,  2, 86, sector + 0x81C);
    //
    // Compute ECC Q code
    //
    ecc_computeblock(sector + 0xC, 52, 43, 86, 88, sector + 0x8C8);
    //
    // Restore the address, if necessary
    //
    if(zeroaddress) {
        memmove(sector + 12, saved_address, 4);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// CD sync header
//
static const uint8_t sync_header[12] = {
    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00
};

////////////////////////////////////////////////////////////////////////////////
//
// Generate ECC/EDC information for a sector (must be 2352 = 0x930 bytes)
//
static void eccedc_generate(uint8_t* sector) {
    //
    // Generate sync
    //
    memmove(sector, sync_header, sizeof(sync_header));
    switch(sector[0x0F]) {
    case 0x00:
        //
        // Mode 0: no data; generate zeroes
        //
        memset(sector + 0x10, 0, 0x920);
        break;

    case 0x01:
        //
        // Mode 1:
        //
        // Compute EDC
        //
        edc_computeblock(sector + 0x00, 0x810, sector + 0x810);
        //
        // Zero out reserved area
        //
        memset(sector + 0x814, 0, 8);
        //
        // Generate ECC P/Q codes
        //
        ecc_generate(sector, 0);
        break;

    case 0x02:
        //
        // Mode 2:
        //
        // Make sure XA flags match
        //
        memmove(sector + 0x14, sector + 0x10, 4);

        if(!(sector[0x12] & 0x20)) {
            //
            // Form 1: Compute EDC
            //
            edc_computeblock(sector + 0x10, 0x808, sector + 0x818);
            //
            // Generate ECC P/Q codes
            //
            ecc_generate(sector, 1);

        } else {
            //
            // Form 2: Compute EDC
            //
            edc_computeblock(sector + 0x10, 0x91C, sector + 0x92C);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Verify EDC for a sector (must be 2352 = 0x930 bytes)
// Returns 0 on success
//
static int edc_verify(const uint8_t* sector) {
    uint8_t myedc[4];
    //
    // Verify sync
    //
    if(memcmp(sector, sync_header, sizeof(sync_header))) { return 1; }

    switch(sector[0x0F]) {
    case 0x00:
        //
        // Mode 0: no data; everything had better be zero
        //
        return anynonzero(sector + 0x10, 0x920);

    case 0x01:
        //
        // Mode 1
        //
        edc_computeblock(sector + 0x00, 0x810, myedc);
        return memcmp(myedc, sector + 0x810, 4);

    case 0x02:
        //
        // Mode 2: Verify that the XA type is correctly copied twice
        //
        if(memcmp(sector + 0x10, sector + 0x14, 4)) { return 1; }

        if(!(sector[0x12] & 0x20)) {
            //
            // Form 1
            //
            edc_computeblock(sector + 0x10, 0x808, myedc);
            return memcmp(myedc, sector + 0x818, 4);

        } else {
            //
            // Form 2
            //
            edc_computeblock(sector + 0x10, 0x91C, myedc);
            return memcmp(myedc, sector + 0x92C, 4);
        }
    }
    //
    // Invalid mode
    //
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

struct cacheentry {
    uint8_t* data;
    uint32_t sector;
    uint8_t valid;
};

enum { CACHE_ENTRIES = 4 };

enum {
    BINTYPE_UNKNOWN = 0,
    BINTYPE_2048    = 1,
    BINTYPE_2352    = 2
};

struct binfile {
    FILE* f;
    const char* name;
    int type;
    uint32_t sectors;
    struct cacheentry cache[CACHE_ENTRIES];
};

static void bin_quit(struct binfile* bin) {
    size_t i;
    if(bin->f) { fclose(bin->f); }
    for(i = 0; i < CACHE_ENTRIES; i++) {
        if(bin->cache[i].data) { free(bin->cache[i].data); }
    }
}

static int bin_init(struct binfile* bin) {
    size_t i;
    memset(bin, 0, sizeof(struct binfile));
    for(i = 0; i < CACHE_ENTRIES; i++) {
        bin->cache[i].data = malloc(2352);
        if(!bin->cache[i].data) {
            printf("%s", oom);
            bin_quit(bin);
            return 1;
        }
    }
    return 0;
}

static void cache_mtf(struct binfile* bin, size_t entry) {
    if(entry) {
        struct cacheentry tmp = bin->cache[entry];
        memmove(bin->cache + 1, bin->cache, sizeof(struct cacheentry) * entry);
        bin->cache[0] = tmp;
    }
}

static uint8_t* cache_find(struct binfile* bin, uint32_t sector) {
    size_t i;
    for(i = 0; i < CACHE_ENTRIES; i++) {
        if((bin->cache[i].sector == sector) && bin->cache[i].valid) {
            cache_mtf(bin, i);
            return bin->cache[0].data;
        }
    }
    return NULL;
}

static uint8_t* cache_allocbegin(struct binfile* bin, uint32_t sector) {
    bin->cache[CACHE_ENTRIES - 1].valid = 0;
    bin->cache[CACHE_ENTRIES - 1].sector = sector;
    return bin->cache[CACHE_ENTRIES - 1].data;
}

static void cache_allocend(struct binfile* bin) {
    cache_mtf(bin, CACHE_ENTRIES - 1);
    bin->cache[0].valid = 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Detect whether image is ISO or BIN
// Returns nonzero on error
//
static int bintype_detect(struct binfile* bin) {
    uint8_t* sector = NULL;
    off_t size;

    bin->type = BINTYPE_UNKNOWN;

    if(fseeko(bin->f, 0, SEEK_END) != 0) { goto error_bin; }
    size = ftello(bin->f);
    if(size == -1) { goto error_bin; }
    if(fseeko(bin->f, 0, SEEK_SET) != 0) { goto error_bin; }

    if(size <= 0 || ((size % 2352) != 0 && (size % 2048) != 0)) {
        //
        // Size is zero or not cleanly divisible
        //
        printf("Error: %s: Unable to determine BIN or ISO format based on size\n",
            bin->name
        );
        goto error;

    } else if((size % 2352) != 0) {
        //
        // If indivisible by 2352, assume ISO
        //
        bin->type = BINTYPE_2048;

    } else if((size % 2048) != 0) {
        //
        // If indivisible by 2048, assume BIN
        //
        bin->type = BINTYPE_2352;

    } else {
        //
        // If divisible by both, read the first 2352 bytes and see if it's a
        // valid raw sector.  If so, assume BIN.
        //
        sector = malloc(2352);
        if(!sector) { goto error_mem; }

        if(fread(sector, 1, 2352, bin->f) != 2352) { goto error_bin; }
        bin->type = edc_verify(sector) ? BINTYPE_2048 : BINTYPE_2352;
    }

    //
    // Figure out the number of sectors
    //
    {   off_t sectorsize = (bin->type == BINTYPE_2048) ? 2048 : 2352;
        bin->sectors =
            (sizeof(off_t) > 4) ? (
                ((((off_t)(size / sectorsize)) >> 31) > 1) ?
                    ((uint32_t)(0xFFFFFFFFLU)) :
                    ((uint32_t)(size / sectorsize))
            ) : (((uint32_t)size) / ((uint32_t)sectorsize));
    }

    goto done;

error_mem:
    printf("%s", oom);
    goto error;
error_bin:
    printfileerror(bin->f, bin->name);
    goto error;
error:
    bin->type = BINTYPE_UNKNOWN;
    goto done;
done:
    if(sector) { free(sector); }
    return (bin->type == BINTYPE_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Ensure that the sector number is within the seekable range for the bin file;
// returns nonzero on error
//
int check_bin_sector_range(const struct binfile* bin, uint32_t sector) {
    if(sector >= bin->sectors) {
        printf("Error: Sector %lu is out of range\n", (unsigned long)sector);
        return 1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns NULL on failure
// The returned buffer is valid until the next call
//
static uint8_t* read_raw_sector(struct binfile* bin, uint32_t sector) {
    uint8_t* data = NULL;

    if(check_bin_sector_range(bin, sector)) { goto error; }

    if(bin->type != BINTYPE_2352) {
        printf("Error: Tried to read raw sector from ISO\n");
        goto error;
    }

    data = cache_find(bin, sector);
    if(!data) {
        data = cache_allocbegin(bin, sector);
        if(fseeko(bin->f, 2352 * ((off_t)sector), SEEK_SET) != 0) {
            goto error_f;
        }
        if(fread(data, 1, 2352, bin->f) != 2352) { goto error_f; }
        cache_allocend(bin);
    }
    return data;

error_f:
    printf("At sector %lu: ", (unsigned long)sector);
    printfileerror(bin->f, bin->name);
    goto error;
error:
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns NULL on failure
// The returned buffer is valid until the next call
//
static uint8_t* read_cooked_sector(
    struct binfile* bin,
    uint32_t sector,
    const struct cdpatch_options* opt
) {
    uint8_t* data = NULL;

    if(check_bin_sector_range(bin, sector)) { goto error; }

    data = cache_find(bin, sector);
    if(!data) {
        data = cache_allocbegin(bin, sector);
        if(bin->type == BINTYPE_2048) {
            if(fseeko(bin->f, 2048 * ((off_t)sector), SEEK_SET) != 0) {
                goto error_f;
            }
            if(fread(data, 1, 2048, bin->f) != 2048) { goto error_f; }
        } else {
            if(fseeko(bin->f, 2352 * ((off_t)sector), SEEK_SET) != 0) {
                goto error_f;
            }
            if(fread(data, 1, 2352, bin->f) != 2352) { goto error_f; }
            //
            // Verify the EDC
            //
            if(edc_verify(data)) {
                if(opt->enforce_fscheck || opt->verbose) {
                    printf("%s: CD sector %lu is corrupt%s\n",
                        opt->enforce_fscheck ? "Error" : "Warning",
                        (unsigned long)sector,
                        opt->enforce_fscheck ? fsoverride : ""
                    );
                }
                if(opt->enforce_fscheck) { goto error; }
            }
        }
        cache_allocend(bin);
    }

    if(bin->type == BINTYPE_2352) {
        //
        // Figure out where the data actually resides in the sector
        //
        switch(data[0xF]) {
        case 1:
            data += 0x10;
            break;
        case 2:
            if(data[0x12] & 0x20) {
                if(opt->enforce_fscheck || opt->verbose) {
                    printf("%s: Attempted to read Form 2 sector %lu%s\n",
                        opt->enforce_fscheck ? "Error" : "Warning",
                        (unsigned long)sector,
                        opt->enforce_fscheck ? fsoverride : ""
                    );
                }
                if(opt->enforce_fscheck) { goto error; }
            }
            data += 0x18;
            break;
        default:
            if(opt->enforce_fscheck || opt->verbose) {
                printf("%s: Invalid mode 0x%02X at sector %lu%s\n",
                    opt->enforce_fscheck ? "Error" : "Warning",
                    (int)data[0xF], (unsigned long)sector,
                    opt->enforce_fscheck ? fsoverride : ""
                );
                if(opt->enforce_fscheck) { goto error; }
            }
            data += 0x10; // assume mode 1
            break;
        }
    }

    return data;

error_f:
    printf("At sector %lu: ", (unsigned long)sector);
    printfileerror(bin->f, bin->name);
    goto error;
error:
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Allocate space for a cooked sector, with the understanding that we will
// overwrite all of it
//
// Returns NULL on failure
// The returned buffer is valid until the next call
//
static uint8_t* alloc_cooked_sector(
    struct binfile* bin,
    uint32_t sector,
    const struct cdpatch_options* opt
) {
    if(bin->type == BINTYPE_2048) {
        uint8_t* data;
        if(check_bin_sector_range(bin, sector)) { return NULL; }
        //
        // Just allocate space and not initialize it
        //
        data = cache_find(bin, sector);
        if(!data) {
            data = cache_allocbegin(bin, sector);
            cache_allocend(bin);
        }
        return data;
    } else {
        //
        // We need to read the sector anyway, so just read it
        //
        return read_cooked_sector(bin, sector, opt);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns 0 on success
//
static int writeback_raw_sector(struct binfile* bin, uint32_t sector) {
    int returncode = 0;
    const uint8_t* data;

    if(check_bin_sector_range(bin, sector)) { goto error; }

    if(bin->type != BINTYPE_2352) {
        printf("Error: Tried to write raw sector to ISO\n");
        goto error;
    }

    data = cache_find(bin, sector);
    if(!data) {
        printf("Error: Sector not in cache\n");
        goto error;
    }

    //
    // Seek and write
    //
    if(fseeko(bin->f, 2352 * ((off_t)sector), SEEK_SET) != 0) { goto error_f; }
    if(fwrite(data, 1, 2352, bin->f) != 2352) { goto error_f; }
    fflush(bin->f);

    goto done;

error_f:
    printf("At sector %lu: ", (unsigned long)sector);
    printfileerror(bin->f, bin->name);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns 0 on success
//
static int writeback_cooked_sector(
    struct binfile* bin,
    uint32_t sector,
    int redoflags,
    int last
) {
    int returncode = 0;
    uint8_t* data;

    if(check_bin_sector_range(bin, sector)) { goto error; }

    data = cache_find(bin, sector);
    if(!data) {
        printf("Error: Sector not in cache\n");
        goto error;
    }

    if(bin->type == BINTYPE_2048) {
        //
        // If this is an ISO file, just seek and write directly
        //
        if(fseeko(bin->f, 2048 * ((off_t)sector), SEEK_SET) != 0) { goto error_f; }
        if(fwrite(data, 1, 2048, bin->f) != 2048) { goto error_f; }

    } else {
        //
        // If mode 2, and we care about the XA flags, set those up
        //
        if(redoflags && (data[0xF] == 2)) {
            data[0x10] = data[0x14] = 0;
            data[0x11] = data[0x15] = 0;
            data[0x12] = data[0x16] = 0x08 | (last ? 0x81 : 0x00);
            data[0x13] = data[0x17] = 0;
        }
        //
        // Regenerate ECC/EDC
        //
        eccedc_generate(data);
        //
        // Seek and write
        //
        if(fseeko(bin->f, 2352 * ((off_t)sector), SEEK_SET) != 0) { goto error_f; }
        if(fwrite(data, 1, 2352, bin->f) != 2352) { goto error_f; }

    }

    fflush(bin->f);
    goto done;

error_f:
    printf("At sector %lu: ", (unsigned long)sector);
    printfileerror(bin->f, bin->name);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////
//
// Read or write arbitrary cooked data; returns nonzero on error
//
static int rw_cooked_data(
    struct binfile* bin,
    uint32_t sector,
    uint32_t offset,
    uint8_t* data,
    size_t size,
    const struct cdpatch_options* opt,
    int write
) {
    int returncode = 0;
    //
    // Normalize sector/offset, and verify the range
    //
    if((offset >> 11) > (0xFFFFFFFFLU - sector)) { goto error_range; }
    sector += offset >> 11;
    offset &= 0x7FF;
    if(check_bin_sector_range(bin, sector)) { goto error; }

    if(bin->type == BINTYPE_2048) {
        //
        // Read or write the image file directly
        //
        if(fseeko(
            bin->f, 2048 * ((off_t)sector) + ((off_t)offset), SEEK_SET
        ) != 0) { goto error_f; }
        if(write) {
            if(fwrite(data, 1, size, bin->f) != size) { goto error_f; }
        } else {
            if(fread (data, 1, size, bin->f) != size) { goto error_f; }
        }
    } else {
        //
        // Read or write sector-by-sector
        //
        for(; size; sector++) {
            size_t insector = 0x800 - offset;
            size_t z = size < insector ? size : insector;
            uint8_t* d = read_cooked_sector(bin, sector, opt);
            if(!d) { goto error; }
            if(write) {
                memmove(d + offset, data, z);
                if(writeback_cooked_sector(bin, sector, 0, 0)) { goto error; }
            } else {
                memmove(data, d + offset, z);
            }
            data += z;
            size -= z;
            offset = 0;
            if(size && (sector == 0xFFFFFFFFLU)) { goto error_range; }
        }
    }
    goto done;

error_f:
    printf("At sector %lu: ", (unsigned long)sector);
    printfileerror(bin->f, bin->name);
    goto error;
error_range:
    printf("Error: %s out of range: sector=%lu offset=%lu\n",
        write ? "Write" : "Read", (unsigned long)sector, (unsigned long)offset
    );
    goto error;
error:
    returncode = 1;
done:
    return returncode;
}

static int read_cooked_data(
    struct binfile* bin,
    uint32_t sector,
    uint32_t offset,
    uint8_t* data,
    size_t size,
    const struct cdpatch_options* opt
) {
    return rw_cooked_data(bin, sector, offset, data, size, opt, 0);
}

static int write_cooked_data(
    struct binfile* bin,
    uint32_t sector,
    uint32_t offset,
    const uint8_t* data,
    size_t size,
    const struct cdpatch_options* opt
) {
    return rw_cooked_data(bin, sector, offset, (uint8_t*)data, size, opt, 1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Use RIFF if:
// - The sector is mode 0, or invalid
// - The sector is mode 2 and any of the flag bits besides 'data', 'last sector
//   of data record', or 'last sector of file' are set.
//
static int should_use_riff_format(const uint8_t* sector) {
    if(sector[0xF] == 1) { return 0; }
    if(sector[0xF] == 2) {
        return
            ( sector[0x10]         != 0) ||
            ( sector[0x11]         != 0) ||
            ((sector[0x12] & 0x76) != 0) ||
            ( sector[0x13]         != 0);
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns nonzero on error
//
static int extract_file(
    struct binfile* bin,
    uint32_t sector,
    uint32_t filesize,
    time_t modtime,
    const char* filename,
    const struct cdpatch_options* opt
) {
    int returncode = 0;
    FILE* f = NULL;
    uint8_t* data = NULL;

    if(!opt->overwrite && exists(filename)) {
        printf("Error: %s already exists (use -o to override)\n", filename);
        goto error;
    }
    f = fopen(filename, "wb");
    if(!f) { goto error_f; }

    //
    // Check for RIFF format
    //
    if(bin->type == BINTYPE_2352) {
        data = read_raw_sector(bin, sector);
        if(!data) { goto error; }
    }
    if(data && should_use_riff_format(data)) {
        //
        // Extract in RIFF format
        //
        uint32_t sectors = sectorcount(filesize);
        if(sectors > 1826091LU) {
            sectors = 1826091LU; // Exceeds uint32_t - just cap it
        }
        if(opt->verbose) {
            printf(
                "Extract %7lu %10lu %s (CDXA)\n",
                (unsigned long)sector,
                (unsigned long)(2352 * sectors + 0x2C),
                filename
            );
        }
        //
        // Write header
        //
        {   uint8_t hdr[0x2C];
            memset(hdr, 0, sizeof(hdr));
            memmove (hdr + 0x00, "RIFF", 4);
            set32lsb(hdr + 0x04, 2352 * sectors + 0x2C);
            memmove (hdr + 0x08, "CDXA", 4);
            set32lsb(hdr + 0x0C, 2352 * sectors);
            if(fwrite(hdr, 1, 0x2C, f) != 0x2C) { goto error_f; }
        }
        //
        // Write contents
        //
        for(; sectors; sector++, sectors--) {
            data = read_raw_sector(bin, sector);
            if(!data) { goto error; }
            if(edc_verify(data)) {
                if(opt->enforce_fscheck || opt->verbose) {
                    printf("%s: %s: CD sector %lu is corrupt%s\n",
                        opt->enforce_fscheck ? "Error" : "Warning",
                        filename, (unsigned long)sector,
                        opt->enforce_fscheck ? fsoverride : ""
                    );
                }
                if(opt->enforce_fscheck) { goto error; }
            }
            if(fwrite(data, 1, 2352, f) != 2352) { goto error_f; }
        }
    } else {
        if(opt->verbose) {
            printf(
                "Extract %7lu %10lu %s\n",
                (unsigned long)sector,
                (unsigned long)filesize,
                filename
            );
        }
        //
        // Extract normally
        //
        for(; filesize; sector++) {
            size_t remain = filesize < 2048 ? filesize : 2048;
            data = read_cooked_sector(bin, sector, opt);
            if(!data) { goto error; }
            if(fwrite(data, 1, remain, f) != remain) { goto error_f; }
            filesize -= remain;
        }
    }
    if(f) { fclose(f); f = NULL; }

    //
    // Set modification time, if it was valid
    //
    if(modtime != ((time_t)(-1))) {
        struct utimbuf b;
        b.actime  = modtime;
        b.modtime = modtime;
        if(utime((char*)filename, &b) != 0) {
            // Silently fail - preserving modtime shouldn't be considered critical
        }
    }

    goto done;

error_f:
    printfileerror(f, filename);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    if(f) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns nonzero on error
// Outputs the new size in *filesize
//
static int insert_file(
    struct binfile* bin,
    uint32_t sector,
    uint32_t* filesize,
    const char* filename,
    const struct cdpatch_options* opt
) {
    int returncode = 0;
    uint8_t* data = NULL;
    FILE* f = NULL;
    uint32_t newfilesize;
    uint32_t sectors = sectorcount(*filesize);

    f = fopen(filename, "rb");
    if(!f) { goto error_f; }

    //
    // Get the replacement file size
    //
    {   off_t size;
        if(fseeko(f, 0, SEEK_END) != 0) { goto error_f; }
        size = ftello(f);
        if(size == -1) { goto error_f; }
        if(fseeko(f, 0, SEEK_SET) != 0) { goto error_f; }
        // Verify size is not out of uint32_t range
        if(sizeof(size) > 4 && (size >> 31) > 1) {
            printf("Error: %s: Too large for ISO9660 (> 4GiB)\n", filename);
            goto error;
        }
        newfilesize = (uint32_t)size;
    }

    //
    // Check for RIFF format
    //
    if(bin->type == BINTYPE_2352) {
        data = read_raw_sector(bin, sector);
        if(!data) { goto error; }
    }
    if(data && should_use_riff_format(data)) {
        //
        // Insert in RIFF format
        //
        if(sectors > 1826091LU) {
            sectors = 1826091LU; // Exceeds uint32_t - just cap it
        }
        if(opt->verbose) {
            printf(
                "Insert %7lu %10lu %s (CDXA)\n",
                (unsigned long)sector,
                (unsigned long)newfilesize,
                filename
            );
        }
        if(
            (newfilesize < 0x2C) ||
            ((newfilesize - 0x2C) % 2352) != 0
        ) {
            printf("Error: %s: CDXA file has invalid size\n", filename);
            goto error;
        }
        //
        // Make sure we're not expanding the file, at all
        //
        if(newfilesize > (2352 * sectors + 0x2C)) {
            printf("Error: %s: Cannot expand CDXA data beyond %lu bytes\n",
                filename, (unsigned long)(2352 * sectors)
            );
            goto error;
        }
        //
        // Read and verify header
        //
        {   uint8_t hdr[0x2C];
            if(fread(hdr, 1, 0x2C, f) != 0x2C) { goto error_f; }
            if(
                memcmp(hdr + 0x00, "RIFF", 4) ||
                memcmp(hdr + 0x08, "CDXA", 4) ||
                anynonzero(hdr + 0x10, 0x1C)
            ) {
                printf("Error: %s: RIFF header is invalid\n", filename);
                goto error;
            }
            if(
                get32lsb(hdr + 0x04) != (newfilesize       ) ||
                get32lsb(hdr + 0x0C) != (newfilesize - 0x2C)
            ) {
                printf("Error: %s: RIFF header mismatches actual file size\n",
                    filename
                );
                goto error;
            }
        }
        //
        // Read contents
        //
        newfilesize -= 0x2C;
        *filesize = (newfilesize / 2352) * 2048;
        for(; newfilesize >= 2352; sector++) {
            data = read_raw_sector(bin, sector);
            if(!data) { goto error; }

            // Ignore sync and address
            if(fread(data + 0x00F, 1, 0x00F, f) != 0x00F) { goto error_f; }
            // Read mode and everything else
            if(fread(data + 0x00F, 1, 0x921, f) != 0x921) { goto error_f; }
            // Regenerate sync and ECC/EDC
            memmove(data, sync_header, sizeof(sync_header));
            eccedc_generate(data);

            if(writeback_raw_sector(bin, sector)) { goto error; }
            newfilesize -= 2352;
        }

    } else {
        //
        // Make sure we're not expanding the file too much
        //
        uint32_t bytelimit = (sectors >= 0x200000LU) ?
            ((uint32_t)(0xFFFFFFFFLU)) :
            ((uint32_t)(sectors << 11));
        if(newfilesize > bytelimit) {
            printf("Error: %s: Cannot expand file beyond %lu bytes\n",
                filename, (unsigned long)bytelimit
            );
            goto error;
        }
        if(opt->verbose) {
            printf(
                "Insert %7lu %10lu %s\n",
                (unsigned long)sector,
                (unsigned long)newfilesize,
                filename
            );
        }
        //
        // Insert normally
        //
        *filesize = newfilesize;
        for(; newfilesize; sector++) {
            size_t remain = newfilesize < 2048 ? newfilesize : 2048;

            data = alloc_cooked_sector(bin, sector, opt);
            if(!data) { goto error; }

            if(fread(data, 1, remain, f) != remain) { goto error_f; }
            if(remain < 2048) { memset(data + remain, 0, 2048 - remain); }

            if(writeback_cooked_sector(bin, sector, 1, newfilesize <= 2048)) {
                goto error;
            }
            newfilesize -= remain;
        }
    }

    goto done;

error_f:
    printfileerror(f, filename);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    if(f) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////
//
// ISO9660 data structure offsets
//
enum {
    PD_type                    =    0,
    PD_id                      =    1,
    PD_version                 =    6,
    PD_unused1                 =    7,
    PD_system_id               =    8,
    PD_volume_id               =   40,
    PD_unused2                 =   72,
    PD_volume_space_size       =   80,
    PD_unused3                 =   88,
    PD_volume_set_size         =  120,
    PD_volume_sequence_number  =  124,
    PD_logical_block_size      =  128,
    PD_path_table_size         =  132,
    PD_type_l_path_table       =  140,
    PD_opt_type_l_path_table   =  144,
    PD_type_m_path_table       =  148,
    PD_opt_type_m_path_table   =  152,
    PD_root_dir_record         =  156,
    PD_volume_set_id           =  190,
    PD_publisher_id            =  318,
    PD_preparer_id             =  446,
    PD_application_id          =  574,
    PD_copyright_file_id       =  702,
    PD_abstract_file_id        =  739,
    PD_bibliographic_file_id   =  776,
    PD_creation_date           =  813,
    PD_modification_date       =  830,
    PD_expiration_date         =  847,
    PD_effective_date          =  864,
    PD_file_structure_version  =  881,
    PD_unused4                 =  882,
    PD_application_data        =  883,
    PD_unused5                 = 1395
};

enum {
    DR_length                  =    0,
    DR_ext_attr_length         =    1,
    DR_extent                  =    2,
    DR_size                    =   10,
    DR_date                    =   18,
    DR_flags                   =   25,
    DR_file_unit_size          =   26,
    DR_interleave              =   27,
    DR_volume_sequence_number  =   28,
    DR_name_len                =   32,
    DR_name                    =   33
};

////////////////////////////////////////////////////////////////////////////////

static void printsafestring(const uint8_t* src, size_t size) {
    size_t i;
    for(i = 0; i < size; i++) {
        int c = src[i];
        if(!c) { break; }
        if(isprint(c)) {
            fputc(c, stdout);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Node: Info about a ISO9660 directory or file
//
struct node {
    //
    // Location of the ISO DR representing this node
    //
    uint32_t dr_sector;
    uint32_t dr_ofs;
    //
    // Info copied from the ISO DR
    //
    uint8_t name[256];
    int8_t isdir;
    uint32_t sector;
    uint32_t size;
    time_t modtime;
    //
    // Internal data
    //
    uint32_t depth;
    struct node* up;
    //
    // Current state, if this is a directory and we're reading it
    //
    uint32_t dirread;
};

static void rewindnode(struct node* n) { n->dirread = 0; }

static void freesubnodes(struct node* n, struct node* root) {
    while(n && n != root) {
        struct node* up = n->up;
        free(n);
        n = up;
    }
}

static void printnodename(struct node* n) {
    struct node* prev = NULL;
    //
    // Special case root directory
    //
    if(n && !n->up) {
        printf("/");
    } else {
        int num = 0;
        //
        // Momentarily reverse node order
        //
        while(n) {
            struct node* up = n->up;
            n->up = prev;
            prev = n;
            n = up;
        }
        n = prev;
        prev = NULL;
        //
        // Reverse node order again while printing names
        //
        while(n) {
            struct node* up = n->up;
            n->up = prev;
            if(strcmp((const char*)(n->name), ".")) {
                if(num++) { printf("/"); }
                printsafestring(n->name, strlen((const char*)(n->name)));
            }
            prev = n;
            n = up;
        }
    }
}

static size_t nodenamelen(const struct node* n) {
    size_t len = 0;
    for(; n && n->up; n = n->up) {
        len += strlen((const char*)(n->name));
        if(n->up->up) {
            len++; // directory separator
        }
    }
    return len;
}

static void copynodename(char* end, const struct node* n) {
    for(; n && n->up; n = n->up) {
        size_t len = strlen((const char*)(n->name));
        memmove(end - len, n->name, len);
        end -= len;
        if(n->up->up) {
            *(--end) = '/';
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Read an ISO9660 directory record
//
// Returns the number of bytes in the record, 0 on end, or -1 on error
//
static int read_iso_dr(
    struct binfile* bin,
    struct node* n,
    uint32_t sector,
    uint32_t ofs,
    uint32_t size,
    const struct cdpatch_options* opt
) {
    uint8_t dr[256];
    uint32_t little;
    uint32_t big;
    size_t i, namelen;

    n->dr_sector = sector;
    n->dr_ofs    = ofs;
    n->name[0]   = 0;
    n->isdir     = 0;
    n->sector    = 0;
    n->size      = 0;
    n->modtime   = (time_t)(-1);
    rewindnode(n);

    if(ofs >= size) { return 0; }
    //
    // Read DR length
    //
    if(read_cooked_data(bin, sector, ofs, dr, 1, opt)) { return -1; }
    if(dr[DR_length] == 0) { return 0; }
    if(dr[DR_length] > (size - ofs)) {
        dr[DR_length] = (uint8_t)(size - ofs);
        goto malformed;
    }
    if(dr[DR_length] < DR_name) { goto malformed; }
    //
    // Read DR
    //
    if(read_cooked_data(bin, sector, ofs+1, dr+1, dr[DR_length]-1, opt)) {
        return -1;
    }
    if(dr[DR_name_len] > (dr[DR_length] - DR_name)) { goto malformed; }
    if(dr[DR_name_len] == 0) { goto malformed; }
    namelen = dr[DR_name_len];

    //
    // Get name (but don't normalize yet)
    //
    memmove(n->name, dr + DR_name, namelen);
    n->name[namelen] = 0;
    //
    // Get directory flag
    //
    n->isdir = (dr[DR_flags] & 0x02) != 0;

    //
    // Normalize name, checking it for errors in the process
    //
    if(namelen == 1 && n->name[0] == 0) {
        n->name[0] = '.';
    } else if(namelen == 1 && n->name[0] == 1) {
        n->name[0] = '.';
        n->name[1] = '.';
        n->name[2] = 0;
        namelen = 2;
    } else {
        for(i = 0; i < namelen; i++) {
            uint8_t c = n->name[i];
            if(
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                (c == '_')
            ) {
                // acceptable character
            } else if(c >= 'A' && c <= 'Z') {
                // uppercase alpha - convert to lowercase
                c += ('a' - 'A');
            } else if(c == '.') {
                // if the next character is the end, or version number, then
                // eat the dot
                if(n->name[i + 1] == 0 || n->name[i + 1] == ';') {
                    memmove(n->name + i, n->name + i + 1, namelen - i);
                    namelen--;
                    i--;
                    continue;
                }
            } else if(c == ';') {
                // semicolon - strip the rest of the name if ";1"
                if(i == (namelen - 2) && n->name[i + 1] == '1') {
                    c = 0;
                    namelen = i;
                }
            } else {
                // not ok - just replace with an underscore
                c = '_';
            }
            n->name[i] = c;
        }
    }
    //
    // Get sector
    //
    little = get32lsb(dr + DR_extent + 0);
    big    = get32msb(dr + DR_extent + 4);
    if(little == big || opt->little) {
        n->sector = little;
    } else if(opt->big) {
        n->sector = big;
    } else {
        goto error_mismatch;
    }
    //
    // Get size
    //
    little = get32lsb(dr + DR_size + 0);
    big    = get32msb(dr + DR_size + 4);
    if(little == big || opt->little) {
        n->size = little;
    } else if(opt->big) {
        n->size = big;
    } else {
        goto error_mismatch;
    }
    //
    // Get modified time
    //
    {   int32_t year   = ((uint8_t)(dr[DR_date + 0]));
        int32_t month  = ((uint8_t)(dr[DR_date + 1]));
        int32_t day    = ((uint8_t)(dr[DR_date + 2]));
        int32_t hour   = ((uint8_t)(dr[DR_date + 3]));
        int32_t minute = ((uint8_t)(dr[DR_date + 4]));
        int32_t second = ((uint8_t)(dr[DR_date + 5]));
        int32_t gmtofs = (( int8_t)(dr[DR_date + 6]));
        year -= 70; // year = Years since 1970
        if(year < 0) {
            // Before 1970: Invalid
            n->modtime = (time_t)(-1);
        } else {
            int32_t i;
            // Calculate modtime = days since 1970
            n->modtime = year;
            n->modtime *= 365;
            // Adjust for leap years
            if(year > 2) {
                n->modtime += (year + 1) / 4;
            }
            if(((((year+2) % 4)) == 0) && (month > 2)) {
                n->modtime += 1;
            }
            // Add days from previous months
            for(i = 1; i < month; i++) {
                static const int32_t md[] = {
                    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
                };
                n->modtime += md[i - 1];
            }
            // Add days from this month
            n->modtime += day - 1;
            // Multiply out modtime from days into seconds
            n->modtime *= 24;
            n->modtime += hour;
            n->modtime *= 60;
            n->modtime += minute;
            n->modtime *= 60;
            n->modtime += second;
            // Adjust modtime into GMT using the provided offset
            if(gmtofs >= -52 && gmtofs <= 52) {
                n->modtime -= gmtofs * 15 * 60;
            }
        }
    }
    //
    // Success: Return the length of the directory record
    //
    return dr[DR_length];

malformed:
    if(opt->enforce_fscheck || opt->verbose) {
        printf("%s: ",
            opt->enforce_fscheck ? "Error" : "Warning"
        );
        printnodename(n);
        printf(": Malformed directory record%s\n",
            opt->enforce_fscheck ? fsoverride : ""
        );
    }
    if(opt->enforce_fscheck) { return -1; }
    return dr[DR_length];
error_mismatch:
    printf("Error: ");
    printnodename(n);
    printf(": Mismatched little vs. big-endian metadata\n");
    printf("Use -le or -be to override\n");
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Get the next directory entry in a directory
// Returns nonzero on error
// Returns zero, but NULL node, if nothing was found
//
static int findnext(
    struct binfile* bin,
    struct node* n,
    struct node** out,
    const struct cdpatch_options* opt
) {
    int returncode = 0;
    //
    // Allocate a new node
    //
    *out = malloc(sizeof(struct node));
    if(!(*out)) { goto error_mem; }
    (*out)->up = n;
    (*out)->depth = n->depth + 1;
    //
    // Read node data from DR
    //
    returncode = read_iso_dr(bin, *out, n->sector, n->dirread, n->size, opt);
    if(returncode < 0) { goto error; }
    if(returncode == 0) { goto notfound; }
    //
    // Advance
    //
    n->dirread += returncode;
    returncode = 0;
    goto done;

error_mem:
    printf("%s", oom);
    goto error;
error:
    returncode = 1;
    goto notfound;
notfound:
    if(*out) { free(*out); *out = NULL; }
    goto done;
done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

static int pathsep(char c) { return c == '/' || c == '\\'; }
static int pathend(char c) { return c == 0 || pathsep(c); }

////////////////////////////////////////////////////////////////////////////////
//
// Pass f=NULL to get the boot area
//
static void dofile(
    struct binfile* bin,
    struct node* filenode,
    const struct cdpatch_options* opt,
    int automatic,
    uint32_t* numerrors,
    uint32_t* numsuccesses
) {
    //
    // Construct base filename from base name and node name
    //
    char* filename = NULL;
    size_t basedirlen = strlen(opt->basedir);
    size_t addsep = !(basedirlen && pathsep(opt->basedir[basedirlen - 1]));
    size_t filenamelen = basedirlen + addsep +
        (filenode ? nodenamelen(filenode) : 4);

    filename = malloc(filenamelen + 1);
    if(!filename) { goto error_mem; }

    memmove(filename, opt->basedir, basedirlen);
    if(addsep) { filename[basedirlen] = '/'; }
    if(filenode) {
        copynodename(filename + filenamelen, filenode);
    } else {
        memmove(filename + filenamelen - 4, "boot", 4);
    }
    filename[filenamelen] = 0;

    if(opt->extract) {
        //
        // Ensure that all parent directories are created
        //
        char* t = filename;
        while(*t) {
            while(pathsep(*t)) { t++; }
            while(!pathend(*t)) { t++; }
            if(pathsep(*t)) {
                char c = *t;
                *t = 0;
                mkdir(filename); // OK if it doesn't succeed
                *t = c;
            }
        }
        //
        // Extract
        //
        if(filenode) {
            if(extract_file(
                bin,
                filenode->sector,
                filenode->size,
                filenode->modtime,
                filename,
                opt
            )) { goto error; }
        } else {
            if(extract_file(bin, 0, 32768, (time_t)(-1), filename, opt)) {
                goto error;
            }
        }
        (*numsuccesses)++;

    } else if(opt->insert && (!automatic || exists(filename))) {
        //
        // Insert
        //
        if(filenode) {
            uint32_t newsize = filenode->size;
            if(insert_file(bin, filenode->sector, &newsize, filename, opt)) {
                goto error;
            }
            //
            // If the size changed, update the size in the DR
            //
            if(newsize != filenode->size) {
                uint8_t sz[8];
                set32lsb(sz + 0, newsize);
                set32msb(sz + 4, newsize);
                if(write_cooked_data(
                    bin,
                    filenode->dr_sector,
                    filenode->dr_ofs + DR_size,
                    sz,
                    8,
                    opt
                )) { goto error; }
            }
        } else {
            uint32_t newsize = 32768;
            if(insert_file(bin, 0, &newsize, filename, opt)) {
                goto error;
            }
        }
        (*numsuccesses)++;
    }

    goto done;

error_mem:
    printf("%s", oom);
    goto error;
error:
    (*numerrors)++;
    goto done;
done:
    if(filename) { free(filename); }
}

////////////////////////////////////////////////////////////////////////////////
//
// Recursively walk a directory
//
static void walkdirectory(
    struct binfile* bin,
    struct node* root,
    const struct cdpatch_options* opt,
    uint32_t* numerrors,
    uint32_t* numsuccesses
) {
    struct node* n = root;
    struct node* f = NULL;

    rewindnode(n);
    for(;;) {
        if(findnext(bin, n, &f, opt)) { goto error; }
        if(!f) {
            if(n == root) {
                // Done with entire tree walk
                break;
            } else {
                // Back to previous directory
                f = n;
                n = n->up;
                freesubnodes(f, n);
                f = NULL;
            }
        } else if(!f->name[0]) {
            //
            // Probably a malformed entry which was ignored
            //

        } else if(f->isdir) {
            if(
                strcmp((const char*)(f->name), ".") &&
                strcmp((const char*)(f->name), "..")
            ) {
                // Ensure this directory didn't appear anywhere up the chain
                struct node* t;
                for(t = n; t != root; t = t->up) {
                    if(t->sector == f->sector) { break; }
                }
                if(t->sector == f->sector) {
                    if(opt->enforce_fscheck || opt->verbose) {
                        printf("%s: Infinite recursion: ",
                            opt->enforce_fscheck ? "Error" : "Warning"
                        );
                        printnodename(f);
                        printf(" points to ");
                        printnodename(t);
                        printf("%s\n",
                            opt->enforce_fscheck ? fsoverride : ""
                        );
                    }
                    if(opt->enforce_fscheck) { goto error; }
                    // Ignore the error, but we still can't descend into it
                } else {
                    // Check depth limit.
                    // Allow out-of-ISO9660 spec, but impose our own limit
                    if(f->depth > max_path_depth) {
                        printf("Error: ");
                        printnodename(f);
                        printf(": Path is too deep\n");
                        goto error;
                    }
                    // Descend into this directory
                    n = f;
                }
            }
        } else {
            //
            // This is a file
            //
            dofile(bin, f, opt, 1, numerrors, numsuccesses);
        }
        freesubnodes(f, n);
        f = NULL;
    }
    goto done;
error:
    (*numerrors)++;
    goto done;
done:
    freesubnodes(f, n);
    freesubnodes(n, root);
}

////////////////////////////////////////////////////////////////////////////////

static int arenamesequal(const char* a, const char* b) {
    for(;;) {
        int ca = (*a++) & 0xff;
        int cb = (*b++) & 0xff;
        if(pathend(ca) && pathend(cb)) { return 1; }
        if(pathend(ca)) { return 0; }
        if(pathend(cb)) { return 0; }
        ca = tolower(ca);
        cb = tolower(cb);
        if(ca != cb) { return 0; }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Find a file or directory in the ISO9660 filesystem
// Returns NULL if not found
//
static struct node* findnode(
    struct binfile* bin,
    struct node* root,
    const char* filename,
    const struct cdpatch_options* opt
) {
    struct node* n = root;
    struct node* f = NULL;
    const char* p = filename;
    const char* next = p;

    while(pathsep(*p)) { p++; }

    for(; *p; p = next) {
        //
        // Figure out the next path component, if any
        //
        next = p;
        while(!pathend(*next)) { next++; }
        while(pathsep(*next)) { next++; }
        //
        // Special case "." and ".."
        //
        if(p[0] == '.' && pathend(p[1])) {
            continue;
        }
        if(p[0] == '.' && p[1] == '.' && pathend(p[2])) {
            if(n != root && n->up) {
                f = n;
                n = n->up;
                freesubnodes(f, n);
                f = NULL;
            }
            continue;
        }
        //
        // Find n = path component p in directory node n
        //
        rewindnode(n);
        for(;;) {
            if(findnext(bin, n, &f, opt)) { goto error; }
            if(!f) { goto error_pathnotfound; }
            if(
                arenamesequal(p, (const char*)(f->name)) &&
                (f->isdir || !(*next))
            ) {
                n = f;
                break;
            }
            freesubnodes(f, n);
            f = NULL;
        }
    }

    return n;

error_pathnotfound:
    printf("Error: %s: Path not found in ISO\n", filename);
    goto error;
error:
    freesubnodes(f, n);
    freesubnodes(n, root);
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static void visit_arg(
    struct binfile* bin,
    struct node* root,
    const char* filename,
    const struct cdpatch_options* opt,
    uint32_t* numerrors,
    uint32_t* numsuccesses
) {
    struct node* n = NULL;

    //
    // First, find the node in the ISO9660 filesystem
    //
    n = findnode(bin, root, filename, opt);
    if(!n) { goto error; }

    if(n->isdir) {
        //
        // If it's a directory:
        //
        // If we're not recursing, that's an error
        //
        if(!opt->recurse) {
            printf("Error: %s: Is a directory\n", filename);
            goto error;
        }
        //
        // Walk the directory
        //
        walkdirectory(bin, n, opt, numerrors, numsuccesses);

    } else {
        //
        // If it's a file, visit it
        //
        dofile(bin, n, opt, 0, numerrors, numsuccesses);
    }

    goto done;

error:
    (*numerrors)++;
    goto done;
done:
    freesubnodes(n, root);
}

////////////////////////////////////////////////////////////////////////////////

static int cdpatch(const struct cdpatch_options* opt) {
    int returncode = 0;
    struct binfile bin;
    struct node* root = NULL;
    int i;
    uint32_t numerrors    = 0;
    uint32_t numsuccesses = 0;

    if(bin_init(&bin)) { goto error; }
    bin.name = opt->binname;

    //
    // Attempt to open bin/iso file
    //
    bin.f = fopen(bin.name, opt->insert ? "r+b" : "rb");
    if(!bin.f) { goto error_bin; }

    if(bintype_detect(&bin)) { goto error; }

    if(opt->verbose) {
        printf("Image file: %s\n", opt->binname);
        printf("Format: ");
        switch(bin.type) {
        case BINTYPE_2048: printf("ISO (2048-byte sectors)\n"); break;
        case BINTYPE_2352: printf("BIN (2352-byte sectors)\n"); break;
        }
    }

    //
    // Read primary descriptor
    //
    {   uint8_t* data = read_cooked_sector(&bin, 16, opt);
        if(!data) { goto error; }
        if(opt->verbose) {
            printf("System ID: ");
            printsafestring(data + PD_system_id, 32);
            printf("\nVolume ID: ");
            printsafestring(data + PD_volume_id, 32);
            printf("\n");
        }
    }

    //
    // Insert or extract boot area, if desired
    //
    if(opt->boot) {
        dofile(&bin, NULL, opt, 0, &numerrors, &numsuccesses);
    }

    //
    // Retrive root directory info
    //
    root = malloc(sizeof(struct node));
    if(!root) { goto error_mem; }
    root->depth = 0;
    root->up = NULL;
    i = read_iso_dr(&bin, root, 16, PD_root_dir_record, 2048, opt);
    if(i < 0) { goto error; }
    if(i == 0) {
        printf("Error: Root directory descriptor is missing\n");
        goto error;
    }

    //
    // Visit each of the arguments
    //
    for(i = 0; i < opt->files_count; i++) {
        const char* file = opt->files[i];
        if(*file) { // If non-empty
            visit_arg(&bin, root, file, opt, &numerrors, &numsuccesses);
        }
    }

    if(numsuccesses || !numerrors) {
        printf("%lu file%s %s\n",
            (unsigned long)numsuccesses,
            numsuccesses != 1 ? "s" : "",
            opt->extract ? "extracted" : "inserted"
        );
    }
    if(numerrors) {
        printf("%lu error%s encountered\n",
            (unsigned long)numerrors,
            numerrors != 1 ? "s" : ""
        );
    }

    returncode = (numerrors != 0);
    goto done;

error_mem:
    printf("%s", oom);
    goto error;
error_bin:
    printfileerror(bin.f, bin.name);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    if(root) { free(root); }
    bin_quit(&bin);
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

static int checkboth(int a, int b, const char* opa, const char* opb) {
    if(a && b) {
        printf("Error: Cannot specify both %s and %s\n", opa, opb);
        return 1;
    }
    return 0;
}

static int checkeither(int a, int b, const char* opa, const char* opb) {
    if((!a) && (!b)) {
        printf("Error: Must specify either %s or %s\n", opa, opb);
        return 1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int returncode = 0;
    static const char* default_files[1] = { "." };
    static const int   default_files_count = 1;
    const char* warn_missing = NULL;

    struct cdpatch_options opt;
    int i;

    normalize_argv0(argv[0]);

    memset(&opt, 0, sizeof(opt));
    //
    // Enforce filesystem checks by default (unless overridden with -f)
    //
    opt.enforce_fscheck = 1;

    //
    // Check options
    //
    if(argc == 1) { goto usage; }
    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            // An option
            if(!strcmp(argv[i], "--")) {
                // No more options
                i++;
                break;
            } else if(!strcmp(argv[i], "-i")) {
                if(opt.insert) { goto error_dup; }
                if(i >= (argc - 1)) { goto error_missing; }
                if(argv[i+1][0] == '-') { warn_missing = argv[i]; }
                opt.insert = 1;
                opt.binname = argv[++i];
                continue;
            } else if(!strcmp(argv[i], "-x")) {
                if(opt.extract) { goto error_dup; }
                if(i >= (argc - 1)) { goto error_missing; }
                if(argv[i+1][0] == '-') { warn_missing = argv[i]; }
                opt.extract = 1;
                opt.binname = argv[++i];
                continue;
            } else if(!strcmp(argv[i], "-d")) {
                if(opt.basedir) { goto error_dup; }
                if(i >= (argc - 1)) { goto error_missing; }
                if(argv[i+1][0] == '-') { warn_missing = argv[i]; }
                opt.basedir = argv[++i];
                continue;
            } else if(!strcmp(argv[i], "-be")) {
                opt.big = 1;
                continue;
            } else if(!strcmp(argv[i], "-le")) {
                opt.little = 1;
                continue;
            } else if(!strcmp(argv[i], "-boot")) {
                opt.boot = 1;
                continue;
            } else if(!strcmp(argv[i], "-f")) {
                opt.enforce_fscheck = 0;
                continue;
            } else if(!strcmp(argv[i], "-o")) {
                opt.overwrite = 1;
                continue;
            } else if(!strcmp(argv[i], "-v")) {
                opt.verbose = 1;
                continue;
            } else if(!strcmp(argv[i], "-r")) {
                opt.recurse = 1;
                continue;
            }
            printf("Unknown option: %s\n", argv[i]);
            goto error_usage;
        } else {
            // Not an option - stop here
            break;
        }
    }

    if(checkeither(opt.insert, opt.extract  ,"-i","-x")) { goto error_usage; }
    warn_missing = NULL;
    if(checkboth  (opt.big   , opt.little   ,"-be","-le")) { goto error_usage; }
    if(checkboth  (opt.insert, opt.extract  ,"-i","-x")) { goto error_usage; }
    if(checkboth  (opt.insert, opt.overwrite,"-i","-o")) { goto error_usage; }

    //
    // If base directory wasn't specified, default to "."
    //
    if(!opt.basedir) { opt.basedir = "."; }

    //
    // If no files or -boot were specified, default to "-r ."
    //
    if(i >= argc && !opt.boot) {
        opt.recurse = 1;
        opt.files       = default_files;
        opt.files_count = default_files_count;
    } else {
        opt.files       = (const char**)(argv + i);
        opt.files_count =               (argc - i);
    }

    //
    // Initialize ECC/EDC tables
    //
    eccedc_init();

    //
    // Go
    //
    returncode = cdpatch(&opt);

    goto done;

error_dup:
    printf("Error: Specified %s twice\n", argv[i]);
    goto error_usage;
error_missing:
    printf("Error: Missing parameter for %s\n", argv[i]);
    goto error_usage;
error_usage:
    if(warn_missing) {
        printf("(Missing parameter after %s?)\n", warn_missing);
    }
    printf("\n");
    goto usage;
usage:
    banner();
    printf(
        "Usage:\n"
        "  To insert:  %s -i bin_or_iso [options] [files...]\n"
        "  To extract: %s -x bin_or_iso [options] [files...]\n"
        "\nOptions:\n"
        "  -be       Favor big-endian values in ISO9660 metadata\n"
        "  -boot     Insert or extract boot area\n"
        "  -d dir    Set the base directory for inserted or extracted files\n"
        "            (defaults to .)\n"
        "  -f        Skip filesystem consistency checks\n"
        "  -le       Favor little-endian values in ISO9660 metadata\n"
        "  -o        Force overwrite when extracting files\n"
        "  -r        Recurse into subdirectories\n"
        "  -v        Verbose\n",
        argv[0],
        argv[0]
    );
    goto error;

error:
    returncode = 1;
    goto done;

done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

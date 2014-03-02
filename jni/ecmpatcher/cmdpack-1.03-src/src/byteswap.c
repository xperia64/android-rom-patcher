////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "byteswap - Swap byte order of files"
#define COPYR "Copyright (C) 2011 Neill Corlett"
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

static int byteswap(
    const char* filename,
    uint8_t* buf,
    size_t buffersize,
    size_t recordsize,
    int quiet
) {
    int returncode = 0;
    FILE* f = NULL;

    f = fopen(filename, "r+b");
    if(!f) { goto error_f; }

    for(;;) {
        size_t rec;
        size_t size;
        //
        // Read
        //
        clearerr(f);
        size = fread(buf, 1, buffersize, f);
        if(size == 0) {
            // Check for error
            if(ferror(f)) {
                goto error_f;
            }
            // Otherwise, done
            break;
        }
        //
        // Seek backwards
        //
        if(fseeko(f, -((off_t)size), SEEK_CUR) != 0) {
            goto error_f;
        }

        if(size > buffersize) { size = buffersize; }
        if(size < buffersize) {
            //
            // Round up to the next record, filling with zeroes as we go
            //
            size_t extra = size % recordsize;
            if(extra) {
                for(; extra < recordsize; extra++) {
                    buf[size++] = 0;
                }
            }
        }
        //
        // Reverse each record in memory
        //
        for(rec = 0; rec < size; rec += recordsize) {
            size_t a = rec;
            size_t b = rec + recordsize - 1;
            for(; a < b; a++, b--) {
                uint8_t t = buf[a];
                buf[a] = buf[b];
                buf[b] =  t;
            }
        }
        //
        // Write
        //
        if(fwrite(buf, 1, size, f) != size) {
            goto error_f;
        }
        fflush(f);
    }

    if(!quiet) {
        printf("%s: Done\n", filename);
    }
    goto done;

error_f:
    printfileerror(f, filename);
    goto error;

error:
    returncode = 1;

done:
    if(f != NULL) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int returncode = 0;
    unsigned long recordsize = 2;
    unsigned long recordsize_max = (sizeof(size_t) < sizeof(unsigned long)) ?
        ((unsigned long)((size_t)(-1))) : LONG_MAX;
    int quiet = 0;
    int i;
    uint8_t* buf = NULL;
    size_t buffersize = 0;
    size_t buffersize_min = 8192;

    normalize_argv0(argv[0]);

    //
    // Check options
    //
    if(argc == 1) { goto usage; }
    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            // An option
            if(argv[i][1] == '-' && argv[i][2] == 0) {
                // No more options
                i++;
                break;
            } else if(argv[i][1] == 'q' && argv[i][2] == 0) {
                // Quiet
                quiet = 1;
                continue;
            } else if(argv[i][1] == 's' && argv[i][2] == 0) {
                // Record size
                i++;
                if(i >= argc) { goto usage; }
                if(argv[i][0] == '-') {
                    printf("Error: Record size must not be negative\n");
                    goto error;
                }
                if(!isdigit((int)(argv[i][0]))) { goto usage; }
                recordsize = strtoul(argv[i], NULL, 0);
                continue;
            }
            printf("Unknown option: %s\n", argv[i]);
            goto usage;
        } else {
            // Not an option - stop here
            break;
        }
    }
    if(i >= argc) {
        printf("Error: No files were specified\n");
        goto usage;
    }

    if(recordsize > recordsize_max || recordsize > LONG_MAX) {
        printf("Error: Record size is too large\n");
        goto error;
    }
    if(recordsize < 2) {
        printf("Error: Record size must be at least 2\n");
        goto error;
    }

    //
    // Figure out how big the buffer should be
    //
    buffersize = ((size_t)(recordsize));
    if(buffersize < buffersize_min) {
        buffersize = buffersize_min - (
            buffersize_min % ((size_t)(recordsize))
        );
    }

    //
    // Allocate buffer
    //
    buf = malloc(buffersize);
    if(!buf) {
        printf("Error: Out of memory\n");
        goto error;
    }

    //
    // Process files
    //
    for(; i < argc; i++) {
        if(byteswap(argv[i], buf, buffersize, (size_t)recordsize, quiet)) {
            returncode = 1;
        }
    }
    goto done;

usage:
    banner();
    printf("Swaps the byte order of each record in a file, in place.\n");
    printf("\n");
    printf("Usage: %s [-q] [-s recordsize] files...\n", argv[0]);
    printf(" -q: Quiet\n");
    printf(" -s: Size, in bytes, of each record to swap (default: 2)\n");
    goto error;

error:
    returncode = 1;

done:
    if(buf != NULL) { free(buf); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

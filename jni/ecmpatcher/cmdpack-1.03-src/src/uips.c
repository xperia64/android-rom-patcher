////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "uips - Universal IPS patch create/apply utility"
#define COPYR "Copyright (C) 1999,2010 Neill Corlett"
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

static const off_t IPS_EOF   = 0x00454F46L;
static const off_t IPS_LIMIT = 0x01000000L;

//
// Wrapper for fopen that does various things
//
static FILE *my_fopen(const char *filename, const char *mode, off_t *size) {
    FILE *f = fopen(filename, mode);
    if(!f) { goto error; }
    if(size) {
        if(fseeko(f, 0, SEEK_END) == -1) { goto error; }
        *size = ftello(f);
        if(*size < 0) { goto error; }
        if(fseeko(f, 0, SEEK_SET) == -1) { goto error; }
    }
    return f;
error:
    printfileerror(f, filename);
    if(f) { fclose(f); }
    return NULL;
}

//
// Read a number from a file, MSB first
// Returns -1 on end of file
//
static int32_t readvalue(FILE *f, size_t nbytes) {
    int32_t v = 0;
    while(nbytes--) {
        int c = fgetc(f);
        if(c == EOF) {
            return -1;
        }
        v = (v << 8) | (c & 0xFF);
    }
    return v;
}

//
// Write a number to a file, MSB first
//
static void writevalue(int32_t value, FILE *f, size_t nbytes) {
    size_t i = nbytes << 3;
    while(nbytes--) {
        i -= 8;
        fputc(value >> i, f);
    }
}

//
// Search for the next difference between the target file and a number of
// source files
//
static off_t get_next_difference(
          off_t      ofs,
          FILE     **source_file,
    const off_t     *source_size,
          size_t     source_nfiles,
          FILE      *target_file,
          off_t      target_size
) {
    size_t i;
    if(ofs >= target_size) {
        return target_size;
    }
    fseeko(target_file, ofs, SEEK_SET);
    for(i = 0; i < source_nfiles; i++) {
        if(ofs >= source_size[i]) {
            return ofs;
        }
    }
    for(i = 0; i < source_nfiles; i++) {
        fseeko(source_file[i], ofs, SEEK_SET);
    }
    for(;;) {
        int tc = fgetc(target_file);
        if(tc == EOF) {
            return target_size;
        }
        for(i = 0; i < source_nfiles; i++) {
            if(fgetc(source_file[i]) != tc) {
                return ofs;
            }
        }
        ofs++;
    }
}

//
// Search for the end of a difference block
//
static off_t get_difference_end(
          off_t      ofs,
          off_t      similar_limit,
          FILE     **source_file,
    const off_t     *source_size,
          size_t     source_nfiles,
          FILE      *target_file,
          off_t      target_size
) {
    size_t i;
    off_t similar_rl = 0;
    if(ofs >= target_size) {
        return target_size;
    }
    fseeko(target_file, ofs, SEEK_SET);
    for(i = 0; i < source_nfiles; i++) {
        if(ofs >= source_size[i]) {
            return target_size;
        }
    }
    for(i = 0; i < source_nfiles; i++) {
        fseeko(source_file[i], ofs, SEEK_SET);
    }
    for(;;) {
        char is_different = 0;
        int tc = fgetc(target_file);
        if(tc == EOF) {
            return target_size - similar_rl;
        }
        for(i = 0; i < source_nfiles; i++) {
            int fc = fgetc(source_file[i]);
            if((fc == EOF) || (fc != tc)) {
                is_different = 1;
            }
        }
        ofs++;
        if(is_different) {
            similar_rl = 0;
        } else {
            similar_rl++;
            if(similar_rl == similar_limit) {
                break;
            }
        }
    }
    return ofs - similar_rl;
}

//
// Encode a difference block into a patch file
//
static void encode_patch_block(
    FILE* patch_file,
    FILE* target_file,
    off_t ofs,
    off_t ofs_end
) {
    while(ofs < ofs_end) {
        off_t ofs_block_end;
        off_t rl;
        int c;
        //
        // Avoid accidental "EOF" marker
        //
        if(ofs == IPS_EOF) {
            ofs--;
        }
        //
        // Write the offset to the patch file
        //
        writevalue(ofs, patch_file, 3);
        fseeko(target_file, ofs, SEEK_SET);
        //
        // If there is a beginning run of at least 9 bytes, use it
        //
        c = fgetc(target_file);
        rl = 1;
        while(
            (fgetc(target_file) == c) &&
            (rl < 0xFFFF) &&
            ((ofs + rl) < ofs_end)
        ) {
            rl++;
        }
        //
        // Encode a run, if the run was long enough
        //
        if(rl >= 9) {
            writevalue( 0, patch_file, 2);
            writevalue(rl, patch_file, 2);
            writevalue( c, patch_file, 1);
            ofs += rl;
            continue;
        }
        //
        // Search for the end of the block.
        // The block ends if there's an internal run of at least 14, or an ending
        // run of at least 9, or the block length == 0xFFFF, or the block reaches
        // ofs_end.
        //
        fseeko(target_file, ofs, SEEK_SET);
        ofs_block_end = ofs;
        c = -1;
        while(
            (ofs_block_end < ofs_end) &&
            ((ofs_block_end - ofs) < 0xFFFF)
        ) {
            int c2 = fgetc(target_file);
            ofs_block_end++;
            if(c == c2) {
                rl++;
                if(rl == 14) {
                    ofs_block_end -= 14;
                    break;
                }
            } else {
                rl = 1;
                c = c2;
            }
        }
        //
        // Look for a sufficiently long ending run
        //
        if((ofs_block_end == ofs_end) && (rl >= 9)) {
            ofs_block_end -= rl;
            if(ofs_block_end == IPS_EOF) {
                ofs_block_end++;
            }
        }
        //
        // Encode a regular patch block
        //
        writevalue(ofs_block_end - ofs, patch_file, 2);
        fseeko(target_file, ofs, SEEK_SET);
        while(ofs < ofs_block_end) {
            fputc(fgetc(target_file), patch_file);
            ofs++;
        }
    }
}

//
// Create a patch given a list of source filenames and a target filename.
// Returns 0 on success.
//
static int create_patch(
    const char  *patch_filename,
    size_t       source_nfiles,
    const char **source_filename,
    const char  *target_filename
) {
    int      returncode = 0;
    FILE    *patch_file  = NULL;
    FILE   **source_file = NULL;
    off_t   *source_size = NULL;
    FILE    *target_file = NULL;
    off_t    target_size;
    off_t    ofs;
    size_t   i;
    char     will_truncate = 0;
    //
    // Allocate memory for list of source file streams and sizes
    //
    if(
        source_nfiles > (((size_t)(-1)) / sizeof(*source_file)) ||
        source_nfiles > (((size_t)(-1)) / sizeof(*source_size)) ||
        ((source_file = malloc(sizeof(*source_file) * source_nfiles)) == NULL) ||
        ((source_size = malloc(sizeof(*source_size) * source_nfiles)) == NULL)
    ) {
        printf("Out of memory\n");
        goto err;
    }
    for(i = 0; i < source_nfiles; i++) { source_file[i] = NULL; }
    //
    // Open target file
    //
    target_file = my_fopen(target_filename, "rb", &target_size);
    if(!target_file) { goto err; }
    //
    // Open source files
    //
    for(i = 0; i < source_nfiles; i++) {
        source_file[i] = my_fopen(source_filename[i], "rb", source_size + i);
        if(!source_file[i]) { goto err; }
        if(source_size[i] > target_size) {
            will_truncate = 1;
        }
    }
    //
    // Create patch file
    //
    patch_file = my_fopen(patch_filename, "wb", NULL);
    if(!patch_file) { goto err; }
    printf("Creating %s...\n", patch_filename);
    //
    // Write "PATCH" signature
    //
    if(fwrite("PATCH", 1, 5, patch_file) != 5) { goto err_patch_file; }
    //
    // Main patch creation loop
    //
    ofs = 0;
    for(;;) {
        off_t ofs_end;
        //
        // Search for next difference
        //
        ofs = get_next_difference(
            ofs,
            source_file,
            source_size,
            source_nfiles,
            target_file,
            target_size
        );
        if(ofs == target_size) {
            break;
        }
        if(ofs >= IPS_LIMIT) {
            printf("Warning: Differences beyond 16MiB were ignored\n");
            break;
        }
        //
        // Determine the length of the difference block
        //
        ofs_end = get_difference_end(
            ofs,
            5,
            source_file,
            source_size,
            source_nfiles,
            target_file,
            target_size
        );
        //
        // Encode the difference block into the patch file
        //
        encode_patch_block(patch_file, target_file, ofs, ofs_end);
        ofs = ofs_end;
    }
    //
    // Write EOF marker
    //
    writevalue(IPS_EOF, patch_file, 3);
    if(will_truncate) {
        if(target_size >= IPS_LIMIT) {
            printf("Warning: Can't truncate beyond 16MiB\n");
        } else {
            writevalue(target_size, patch_file, 3);
        }
    }
    //
    // Finished
    //
    printf("Done\n");
    goto no_err;

err_patch_file: printfileerror(patch_file, patch_filename); goto err;

err:
    returncode = 1;
no_err:
    if(patch_file) { fclose(patch_file); }
    for(i = 0; i < source_nfiles; i++) {
        if(source_file[i]) { fclose(source_file[i]); }
    }
    if(target_file) { fclose(target_file); }
    if(source_file) { free(source_file); }
    if(source_size) { free(source_size); }
    return returncode;
}

//
// Apply a patch to a given target.
// Returns 0 on success.
//
static int apply_patch(
    const char* patch_filename,
    const char* target_filename
) {
    int returncode = 0;
    FILE *patch_file  = NULL;
    FILE *target_file = NULL;
    off_t target_size;
    off_t ofs;
    //
    // Open patch file
    //
    patch_file = my_fopen(patch_filename, "rb", NULL);
    if(!patch_file) { goto err; }
    //
    // Verify first five characters
    //
    if(
        (fgetc(patch_file) != 'P') ||
        (fgetc(patch_file) != 'A') ||
        (fgetc(patch_file) != 'T') ||
        (fgetc(patch_file) != 'C') ||
        (fgetc(patch_file) != 'H')
    ) {
        printf("%s: Invalid patch file format\n", patch_filename);
        goto err;
    }
    //
    // Open target file
    //
    target_file = my_fopen(target_filename, "r+b", &target_size);
    if(!target_file) { goto err; }
    printf("Applying %s...\n", patch_filename);
    //
    // Main patch application loop
    //
    for(;;) {
        off_t ofs, len;
        off_t rlen  = 0;
        int   rchar = 0;
        //
        // Read the beginning of a patch record
        //
        ofs = readvalue(patch_file, 3);
        if(ofs == -1) { goto err_eof; }
        if(ofs == IPS_EOF) {
            break;
        }
        len = readvalue(patch_file, 2);
        if(len == -1) { goto err_eof; }
        if(!len) {
            rlen = readvalue(patch_file, 2);
            if(rlen == -1) { goto err_eof; }
            rchar = fgetc(patch_file);
            if(rchar == EOF) { goto err_eof; }
        }
        //
        // Seek to the appropriate position in the target file
        //
        if(ofs <= target_size) {
            fseeko(target_file, ofs, SEEK_SET);
        } else {
            fseeko(target_file, 0, SEEK_END);
            while(target_size < ofs) {
                fputc(0, target_file);
                target_size++;
            }
        }
        //
        // Apply patch block
        //
        if(len) {
            ofs += len;
            if(ofs > target_size) {
                target_size = ofs;
            }
            while(len--) {
                rchar = fgetc(patch_file);
                if(rchar == EOF) { goto err_eof; }
                fputc(rchar, target_file);
            }
        } else {
            ofs += rlen;
            if(ofs > target_size) {
                target_size = ofs;
            }
            while(rlen--) {
                fputc(rchar, target_file);
            }
        }
    }
    //
    // Perform truncation if necessary
    //
    fclose(target_file);
    target_file = NULL;
    ofs = readvalue(patch_file, 3);
    if(ofs != -1 && ofs < target_size) {
        if(truncate(target_filename, ofs) != 0) {
            printf("Warning: Truncate failed\n");
        }
    }
    //
    // Finished
    //
    printf("Done\n");
    goto no_err;

err_eof:
    printf(
        "Error: %s: Unexpected end-of-file; patch incomplete\n",
        patch_filename
    );

err:
    returncode = 1;

no_err:
    if(target_file) { fclose(target_file); }
    if( patch_file) { fclose( patch_file); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

int main(
    int argc,
    char **argv
) {
    int returncode = 1;
    char cmd;

    normalize_argv0(argv[0]);

    if(argc < 2) {
        banner();
        goto usage;
    }
    cmd = argv[1][0];
    if(cmd && argv[1][1]) { cmd = 0; }
    switch(cmd) {
    case 'c':
    case 'C':
        if(argc < 5) {
            goto usage;
        }
        returncode = create_patch(
            argv[2],
            argc - 4,
            (const char**)(argv + 3),
            argv[argc - 1]
        );
        break;
    case 'a':
    case 'A':
        if(argc != 4) {
            goto usage;
        }
        returncode = apply_patch(argv[2], argv[3]);
        break;
    default:
        printf("Unknown command: %s\n", argv[1]);
        goto usage;
    }

    goto done;

usage:
    printf(
        "Usage:\n"
        "To create an IPS patch:\n"
        "  %s c patch_file source_file(s) target_file\n"
        "To apply an IPS patch:\n"
        "  %s a patch_file target_file\n",
        argv[0], argv[0]
    );
    returncode = 1;

done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

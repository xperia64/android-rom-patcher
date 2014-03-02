////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "bin2iso - Convert CD .BIN to .ISO"
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
//
// Sector buffer
//
uint8_t buffer[2352];

////////////////////////////////////////////////////////////////////////////////

static const char* w(int force) { return force ? "Warning" : "Error"; }

int main(int argc, char** argv) {
    int returncode = 0;
    const char* binfilename = NULL;
    const char* isofilename = NULL;
    FILE* binfile = NULL;
    FILE* isofile = NULL;
    int force = 0;
    time_t tstart = time(NULL) - 1;
    off_t binfilelength = 0;
    off_t binfilepos = 0;
    char warning_shown_sync  = 0;
    char warning_shown_mode  = 0;
    char warning_shown_form2 = 0;

    normalize_argv0(argv[0]);

    //
    // Process command line
    //
    {   int fn = 0;
        int i;
        for(i = 1; i < argc; i++) {
            if(argv[i][0] == '-' && !fn) {
                // An option
                if(argv[i][1] == '-' && argv[i][2] == 0) {
                    // No more options
                    fn = 1;
                    continue;
                }
                if(argv[i][1] == 'f' && argv[i][2] == 0) {
                    // Force
                    force = 1;
                    continue;
                }
                printf("Unknown option: %s\n", argv[i]);
                goto usage;
            } else {
                // A filename
                switch(fn) {
                case 0:
                case 1: binfilename = argv[i]; fn = 2; break;
                case 2: isofilename = argv[i]; fn = 3; break;
                default: goto usage;
                }
            }
        }
    }
    if(!binfilename || !isofilename) { goto usage; }

    //
    // Make sure the iso doesn't already exist
    //
    {   int exists = 0;
        FILE* test = fopen(isofilename, "rb");
        exists = (test != NULL);
        if(test) {
            fclose(test);
        }
        if(exists) {
            printf("%s: %s already exists.\n", w(force), isofilename);
            if(!force) { goto error_override; }
            printf("Overwriting.\n");
        }
    }

    //
    // Open bin for reading
    //
    binfile = fopen(binfilename, "rb");
    if(!binfile) { goto error_bin; }
    //
    // Verify the file length
    //
    if(fseeko(binfile, 0, SEEK_END) != 0) { goto error_bin; }
    binfilelength = ftello(binfile);
    if(!binfilelength) {
        printf("Error: %s: Empty file; nothing to convert.\n", binfilename);
        goto error;
    }
    if((binfilelength % ((off_t)2352)) != 0) {
        printf("%s: %s: File size is not a multiple of 2352.\n", w(force), binfilename);
        printf("Likely not a BIN file.\n");
        if(!force) { goto error_override; }
    }

    //
    // Open iso for writing
    //
    isofile = fopen(isofilename, "wb");
    if(!isofile) { goto error_iso; }

    //
    // Convert
    //
    if(fseeko(binfile, 0, SEEK_SET) != 0) { goto error_bin; }
    for(binfilepos = 0;; binfilepos += 2352) {
        size_t data_ofs = 0x10;
        size_t s;
        time_t t = time(NULL);

        //
        // Progress indicator
        //
        if(t != tstart) {
            off_t a = binfilepos    / ((off_t)2352);
            off_t b = binfilelength / ((off_t)2352);
            off_t percent = b ? ((((off_t)100) * a) / b) : 0;
            fprintf(stderr, "%d%%...\r", (int)percent);
            tstart = t;
        }

        s = fread(buffer, 1, 2352, binfile);
        if(ferror(binfile)) { goto error_bin; }
        if(s == 0 && feof(binfile)) {
            // Done converting
            break;
        }
        if(s > 2352) { s = 2352; }
        if(s < 2352) {
            printf("%s: %s: Last sector is incomplete\n", w(force), binfilename);
            if(!force) { goto error_override; }
            // Zero-fill the remaining data
            memset(buffer + s, 0, 2352 - s);
        }
        //
        // Check for sync
        //
        if(memcmp(buffer, "\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00", 12)) {
            if(!warning_shown_sync) {
                printf("%s: %s: Sync area missing at 0x", w(force), binfilename);
                fprinthex(stdout, binfilepos, 1);
                printf("\n");
                printf("Likely not a BIN file.\n");
            }
            if(!force) { goto error_override; }
            warning_shown_sync = 1;
        }
        //
        // Determine sector mode
        //
        switch(buffer[0xF]) {
        case 0x00:
            //
            // Mode 0: technically legal, all zeroes
            //
            data_ofs = 0x10;
            break;

        case 0x01:
            //
            // Mode 1: ok
            //
            data_ofs = 0x10;
            break;

        case 0x02:
            //
            // Mode 2: Check form
            //
            data_ofs = 0x18;
            if(
                ((buffer[0x12] & 0x20) != 0) ||
                ((buffer[0x16] & 0x20) != 0)
            ) {
                if(!warning_shown_form2) {
                    printf("%s: %s: Form 2 sector at 0x", w(force), binfilename);
                    fprinthex(stdout, binfilepos, 1);
                    printf("\n");
                    printf("Information will be lost.\n");
                }
                if(!force) { goto error_override; }
                warning_shown_form2 = 1;
            }
            break;

        default:
            //
            // Unknown mode
            //
            if(!warning_shown_mode) {
                printf("%s: %s: Unknown sector mode 0x%x at 0x", w(force), binfilename, buffer[0xF]);
                fprinthex(stdout, binfilepos, 1);
                printf("\n");
            }
            if(!force) { goto error_override; }
            data_ofs = 0x10;
            if(!warning_shown_mode) {
                printf("Assuming mode 1.\n");
            }
            warning_shown_mode = 1;
            break;
        }
        //
        // Write sector to ISO
        //
        s = fwrite(buffer + data_ofs, 1, 2048, isofile);
        if(ferror(isofile)) { goto error_iso; }
        if(s != 2048) { goto error_iso; }
    }
    printf("%s -> %s done\n", binfilename, isofilename);
    goto done;

usage:
    banner();
    printf(
        "Usage: %s [-f] input_file output_file\n"
        " -f: Force conversion and bypass checks\n",
        argv[0]
    );
    goto error;

error_bin: printfileerror(binfile, binfilename); goto error;
error_iso: printfileerror(isofile, isofilename); goto error;

error_override:
    printf("Use -f to override.\n");

error:
    returncode = 1;

done:
    if(binfile) { fclose(binfile); }
    if(isofile) {
        fclose(isofile);
        // If there was an error, remove the partial ISO file
        if(returncode && isofilename) {
            remove(isofilename);
        }
    }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "bincomp - Compare binary files"
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

static void dumpline(off_t ofs, uint16_t mask, uint8_t* bytes, off_t limit) {
    off_t i;
    fprinthex(stdout, ofs, 8);
    printf(":  ");
    for(i = 0; i < 16; i++) {
        if((mask >> i) & 1) {
            printf("%02X", (int)bytes[i]);
        } else {
            printf((ofs + i) < limit ? ". " : "  ");
        }
    }
    printf("  ");
    for(i = 0; i < 16; i++) {
        if((mask >> i) & 1) {
            printf("%02X", (int)bytes[16 + i]);
        } else {
            printf((ofs + i) < limit ? ". " : "  ");
        }
    }
    printf("\n");
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int returncode = 0;
    FILE* fa = NULL;
    FILE* fb = NULL;
    off_t ofs;
    int8_t is_different = 0;
    int longform = 0;
    off_t    line_ofs = -1;
    uint16_t line_mask = 0;
    uint8_t  line_bytes[32];

    normalize_argv0(argv[0]);

    if(argc == 4) {
        if(strcmp(argv[3], "-l")) { goto usage; }
        longform = 1;
    } else if(argc == 3) {
        longform = 0;
    } else {
        goto usage;
    }

    if(!strcmp(argv[1], argv[2])) {
        printf("You specified the same file\n");
        goto done;
    }

    fa = fopen(argv[1], "rb");
    if(!fa) { goto error_fa; }

    fb = fopen(argv[2], "rb");
    if(!fb) { goto error_fb; }

    for(ofs = 0;; ofs++) {
        int ca;
        int cb;

        if(line_mask && ((ofs | 0xF) != (line_ofs | 0xF))) {
            dumpline(line_ofs, line_mask, line_bytes, line_ofs + 0x10);
            line_mask = 0;
        }

        ca = fgetc(fa);
        if(ca == EOF && ferror(fa)) { goto error_fa; }

        cb = fgetc(fb);
        if(cb == EOF && ferror(fb)) { goto error_fb; }

        is_different |= (ca != cb);

        if(ca == EOF && cb != EOF) {
            if(line_mask) { dumpline(line_ofs, line_mask, line_bytes, ofs); }
            printf("%s is longer than %s\n", argv[2], argv[1]);
            break;
        }
        if(cb == EOF && ca != EOF) {
            if(line_mask) { dumpline(line_ofs, line_mask, line_bytes, ofs); }
            printf("%s is longer than %s\n", argv[1], argv[2]);
            break;
        }
        if(ca == EOF && cb == EOF) {
            if(line_mask) { dumpline(line_ofs, line_mask, line_bytes, ofs); }
            break;
        }
        if(ca != cb) {
            if(longform) {
                fprinthex(stdout, ofs, 8);
                printf(": %02X %02X\n", ca, cb);
            } else {
                line_ofs = (ofs | 0xF) ^ 0xF;
                line_mask |= 1 << (ofs & 0xF);
                line_bytes[     (ofs & 0xF)] = (uint8_t)ca;
                line_bytes[16 + (ofs & 0xF)] = (uint8_t)cb;
            }
        }

    }
    if(!is_different) {
        printf("Files match\n");
    }

    returncode = is_different;
    goto done;

error_fa: printfileerror(fa, argv[1]); goto error;
error_fb: printfileerror(fb, argv[2]); goto error;

usage:
    banner();
    printf(
        "Usage: %s file1 file2 [-l]\n"
        "  -l   Use long format\n",
        argv[0]
    );
    goto error;

error:
    returncode = 1;

done:
    if(fa != NULL) { fclose(fa); }
    if(fb != NULL) { fclose(fb); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "zerofill - Create a large, empty file"
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
// Convert a time_t to a struct tm, safely
//
static struct tm my_localtime(time_t t) {
    struct tm *p_tm = localtime(&t);
    if(!p_tm) { t = 0; p_tm = localtime(&t); }
    if(!p_tm) {
        struct tm fallback;
        memset(&fallback, 0, sizeof(fallback));
        return fallback;
    }
    return (*p_tm);
}

////////////////////////////////////////////////////////////////////////////////

static void fprinttime(FILE* f, time_t t) {
    size_t r;
    char buf[64];
    struct tm my_tm = my_localtime(t);
    memset(buf, 0, sizeof(buf));
    r = strftime(buf, sizeof(buf)-1, "%c", &my_tm);
    if(r < 1) {
        strncpy(buf, "(invalid date)", sizeof(buf)-1);
    }
    fputs(buf, f);
}

////////////////////////////////////////////////////////////////////////////////

static void progress(
    FILE* f,
    off_t written,
    off_t size,
    time_t time_cur,
    time_t time_start
) {
    time_t elapsed = time_cur - time_start;

    {   off_t divider;
        off_t percent;
        divider = size / 100;
        if(divider < 1) { divider = 1; }
        percent = (size == 0) ? 100 : (written / divider);
        if(percent < 0) { percent = 0; }
        if(percent > 100) { percent = 100; }
        fprintf(f, "%3d%%", (int)percent);
    }

    if(elapsed >= 5) {
        off_t rate = written / ((off_t)(elapsed > 1 ? elapsed : 1));
        off_t sec_remain = (size - written) / (rate > 1 ? rate : 1);
        if(sec_remain <= 1000000) {
            time_t eta = time_cur + sec_remain;
            fputs(" (", f);
            fprintdec(f, rate);
            fputs(" byte/sec, ETA ", f);
            fprinttime(f, eta);
            fputs(")          ", f);
        }
    }

    fputs("\r", f);
    fflush(f);
}

////////////////////////////////////////////////////////////////////////////////

static uint8_t buf[0x8000LU];

int main(int argc, char **argv) {
    int returncode = 0;
    int force = 0;
    int quiet = 0;
    const char* filename = NULL;
    FILE* f = NULL;
    off_t size = 0;
    off_t written = 0;
    time_t time_start = time(NULL);
    time_t time_cur   = 0;

    normalize_argv0(argv[0]);

    //
    // Check command line
    //
    {   int argn = 0;
        int i;
        for(i = 1; i < argc; i++) {
            if(argv[i][0] == '-' && argn == 0) {
                // An option
                if(argv[i][1] != 0 && argv[i][2] == 0) {
                    switch(argv[i][1]) {
                    case '-':
                        // No more options
                        argn = 1;
                        continue;
                    case 'f':
                        // Force
                        force = 1;
                        continue;
                    case 'q':
                        // Quiet
                        quiet = 1;
                        continue;
                    }
                }
                printf("Unknown option: %s\n", argv[i]);
                goto usage;
            } else {
                switch(argn) {
                case 0:
                case 1:
                    size = strtoofft(argv[i], NULL, 0);
                    argn = 2;
                    break;
                case 2:
                    filename = argv[i];
                    argn = 3;
                    break;
                default:
                    goto usage;
                }
            }
        }
        if(argn != 3) { goto usage; }
    }

    if(size < 0) {
        printf("Error: Size must not be negative\n");
        goto error;
    }

    if(!force) {
        //
        // Make sure the file doesn't already exist
        //
        f = fopen(filename, "rb");
        if(f) {
            printf("Error: %s already exists (use -f to overwrite)\n", filename);
            goto error;
        }
    }

    f = fopen(filename, "wb");
    if(!f) { goto fileerror; }

    if(!quiet) {
        printf("Writing ");
        fprintdec(stdout, size);
        printf(" bytes to file: %s\n", filename);
    }

    memset(buf, 0, sizeof(buf));

    while(written < size) {
        off_t remain;
        size_t s;

        //
        // Do progress indicator
        //
        if(!quiet) {
            time_t t = time(NULL);
            if(t != time_cur) {
                time_cur = t;
                progress(stderr, written, size, time_cur, time_start);
            }
        }

        //
        // Write a block
        //
        remain = size - written;
        if(remain > ((off_t)(sizeof(buf)))) {
            remain = ((off_t)(sizeof(buf)));
        }
        s = fwrite(buf, 1, (size_t)remain, f);
        if(s != ((size_t)remain)) {
            if(!quiet) {
                fputs("\n", stdout);
            }
            goto fileerror;
        }
        written += remain;
    }

    //
    // Done
    //
    if(!quiet) {
        time_cur = time(NULL);
        progress(stderr, written, size, time_cur, time_start);
        printf("\nDone\n");
    }

    goto end;

fileerror:
    printfileerror(f, filename);
    goto error;

usage:
    banner();
    printf("Usage: %s [-f] [-q] size filename\n", argv[0]);
    printf(" -f: Force overwrite\n");
    printf(" -q: Quiet\n");
    goto error;

error:
    returncode = 1;

end:
    if(f != NULL) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

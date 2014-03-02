////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "byteshuf - Shuffle or unshuffle bytes in a file"
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

static int shuffle(
    int shuffling,
    const char* mainfile,
    const char** subfiles,
    size_t subfiles_count,
    int overwrite
) {
    int returncode = 0;
    size_t i;
    int* chars = calloc(1, sizeof(int) * subfiles_count);
    FILE* mf = NULL;
    FILE** sf = calloc(1, sizeof(FILE*) * subfiles_count);
    if(!sf || !chars) {
        printf("Error: Out of memory\n");
        goto error;
    }
    if(shuffling && !overwrite) {
        //
        // Ensure main file doesn't already exist
        //
        mf = fopen(mainfile, "rb");
        if(mf) {
            printf("Error: %s already exists (use -o to overwrite)\n",
                mainfile
            );
            goto error;
        }
    }
    if(!shuffling && !overwrite) {
        //
        // Ensure sub files don't already exist
        //
        for(i = 0; i < subfiles_count; i++) {
            sf[i] = fopen(subfiles[i], "rb");
            if(sf[i]) {
                printf("Error: %s already exists (use -o to overwrite)\n",
                    subfiles[i]
                );
                goto error;
            }
        }
    }
    if(shuffling) {
        size_t num_chars_last = subfiles_count;

        //
        // Open sub files
        //
        for(i = 0; i < subfiles_count; i++) {
            sf[i] = fopen(subfiles[i], "rb");
            if(!sf[i]) { goto error_sf_i; }
            clearerr(sf[i]);
        }
        //
        // Open main file
        //
        mf = fopen(mainfile, "wb");
        if(!mf) { goto error_mf; }
        clearerr(mf);

        for(;;) {
            size_t num_chars = 0;
            for(i = 0; i < subfiles_count; i++) {
                chars[i] = fgetc(sf[i]);
                if(chars[i] == EOF) {
                    if(ferror(sf[i])) { goto error_sf_i; }
                    chars[i] = 0;
                } else {
                    chars[i] &= 0xFF;
                    num_chars = i + 1;
                }
            }
            if(!num_chars) { break; }
            for(i = num_chars_last; i < subfiles_count; i++) {
                if(fputc(0, mf) == EOF) { goto error_mf; }
            }
            for(i = 0; i < num_chars; i++) {
                if(fputc(chars[i], mf) == EOF) { goto error_mf; }
            }
            num_chars_last = num_chars;
        }
    } else {
        //
        // Open main file
        //
        mf = fopen(mainfile, "rb");
        if(!mf) { goto error_mf; }
        clearerr(mf);
        //
        // Open sub files
        //
        for(i = 0; i < subfiles_count; i++) {
            sf[i] = fopen(subfiles[i], "wb");
            if(!sf[i]) { goto error_sf_i; }
            clearerr(sf[i]);
        }

        for(;;) {
            for(i = 0; i < subfiles_count; i++) {
                int c = fgetc(mf);
                if(c == EOF) {
                    if(ferror(mf)) { goto error_mf; }
                    break;
                }
                if(fputc(c & 0xFF, sf[i]) == EOF) { goto error_sf_i; }
            }
            if(i < subfiles_count) { break; }
        }
    }
    goto done;

error_mf:
    printfileerror(mf, mainfile);
    goto error;
error_sf_i:
    printfileerror(sf[i], subfiles[i]);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    if(mf) { fclose(mf); }
    if(sf) {
        for(i = 0; i < subfiles_count; i++) {
            if(sf[i]) { fclose(sf[i]); }
        }
        free(sf);
    }
    if(chars) { free(chars); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

static int checkboth(int a, int b, const char* ab) {
    if(a && b) {
        printf("Error: Cannot specify both -%c and -%c\n", ab[0], ab[1]);
        return 1;
    }
    return 0;
}

static int checkeither(int a, int b, const char* ab) {
    if((!a) && (!b)) {
        printf("Error: Must specify either -%c or -%c\n", ab[0], ab[1]);
        return 1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int returncode = 0;
    struct {
        int8_t shuffle;
        int8_t unshuffle;
        int8_t overwrite;
        const char* mainfile;
        const char** files;
        size_t files_count;
    } opt;
    int i;

    normalize_argv0(argv[0]);

    memset(&opt, 0, sizeof(opt));

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
            } else if(argv[i][1] == 's' && argv[i][2] == 0) {
                if(opt.shuffle) { goto error_dup; }
                if(i >= (argc - 1)) { goto error_missing; }
                opt.shuffle = 1;
                opt.mainfile = argv[++i];
                continue;
            } else if(argv[i][1] == 'u' && argv[i][2] == 0) {
                if(opt.unshuffle) { goto error_dup; }
                if(i >= (argc - 1)) { goto error_missing; }
                opt.unshuffle = 1;
                opt.mainfile = argv[++i];
                continue;
            } else if(argv[i][1] == 'o' && argv[i][2] == 0) {
                opt.overwrite = 1;
                continue;
            }
            printf("Error: Unknown option: %s\n", argv[i]);
            goto error_usage;
        } else {
            // Not an option - stop here
            break;
        }
    }

    if(checkeither(opt.shuffle, opt.unshuffle, "su")) { goto error_usage; }
    if(checkboth  (opt.shuffle, opt.unshuffle, "su")) { goto error_usage; }
    //
    // At least two files must be specified
    //
    opt.files       = (const char**)(argv + i);
    opt.files_count =               (argc - i);
    if(opt.files_count < 2) {
        printf("Error: Must specify at least two subfiles\n");
        goto error_usage;
    }

    //
    // Go
    //
    if(opt.shuffle) {
        returncode = shuffle(
            1, opt.mainfile, opt.files, opt.files_count, opt.overwrite
        );
    } else if(opt.unshuffle) {
        returncode = shuffle(
            0, opt.mainfile, opt.files, opt.files_count, opt.overwrite
        );
    }
    goto done;

error_dup:
    printf("Error: Specified %s twice\n", argv[i]);
    goto error_usage;
error_missing:
    printf("Error: Missing parameter for %s\n", argv[i]);
    goto error_usage;
error_usage:
    printf("\n");
    goto usage;
usage:
    banner();
    printf(
        "Usage:\n"
        "  To unshuffle: %s [-o] -u source      [subfiles...]\n"
        "  To shuffle:   %s [-o] -s destination [subfiles...]\n"
        "\n"
        "Options:\n"
        "  -o       Force overwrite\n"
        "\n"
        "For example, \"%s -u abc def0 def1\" will split all the even bytes from\n"
        "\"abc\" into \"def0\", and the odd bytes into \"def1\".\n",
        argv[0], argv[0], argv[0]
    );
    goto error;

error:
    returncode = 1;
    goto done;

done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

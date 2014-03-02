////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "subfile - Extract a portion of a file"
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

static uint8_t buf[0x8000LU];

int main(int argc, char **argv) {
    int returncode = 0;
    FILE *inf  = NULL;
    FILE *outf = NULL;
    off_t offset = 0;
    off_t length = 0;
    off_t in_length;
    const char *infilename = NULL;
    const char *outfilename = NULL;

    normalize_argv0(argv[0]);

    if(argc != 5) {
        banner();
        printf("Usage: %s infile offset length outfile\n", argv[0]);
        goto error;
    }
    infilename  = argv[1];
    offset      = strtoofft(argv[2], NULL, 0);
    length      = strtoofft(argv[3], NULL, 0);
    outfilename = argv[4];

    if(offset < 0) { printf("Offset must not be negative\n"); goto error; }
    if(length < 0) { printf("Length must not be negative\n"); goto error; }

    inf = fopen(infilename, "rb");
    if(!inf) { goto infileerror; }
    clearerr(inf);

    //
    // Verify offset and length against the size of the input file
    //
    if(fseeko(inf, 0, SEEK_END) != 0) { goto infileerror; }
    in_length = ftello(inf);
    if(in_length < 0) { goto infileerror; }
    if(offset > in_length) {
        printf("Offset is past the end of %s\n", infilename);
        goto error;
    }
    if(length > in_length) {
        printf("Length is past the end of %s\n", infilename);
        goto error;
    }
    if(offset > (in_length - length)) {
        printf("Offset+length is past the end of %s\n", infilename);
        goto error;
    }

    //
    // Seek to the desired offset
    //
    if(fseeko(inf, offset, SEEK_SET) != 0) { goto infileerror; }

    //
    // Ensure the output file doesn't already exist
    //
    outf = fopen(outfilename, "rb");
    if(outf) {
        printf("Error: %s already exists; cannot overwrite\n", outfilename);
        goto error;
    }

    outf = fopen(outfilename, "wb");
    if(!outf) { goto outfileerror; }
    clearerr(outf);

    while(length > 0) {
        size_t r, w;
        off_t l = length;
        if(l > ((off_t)sizeof(buf))) { l = sizeof(buf); }
        r = fread(buf, 1, (size_t)l, inf);
        if(r != ((size_t)l)) { goto infileerror; }
        w = fwrite(buf, 1, r, outf);
        if(w != r) { goto outfileerror; }
        length -= r;
    }

    goto end;

infileerror: printfileerror(inf, infilename); goto error;
outfileerror: printfileerror(outf, outfilename); goto error;

error:
    returncode = 1;

end:
    if(inf  != NULL) { fclose(inf ); }
    if(outf != NULL) { fclose(outf); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

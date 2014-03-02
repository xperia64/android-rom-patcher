////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "rels - Relative Searcher"
#define COPYR "Copyright (C) 2002,2010 Neill Corlett"
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

void report(
    off_t          filepos,
    const uint8_t* src,
    size_t         len,
    const char*    type
) {
    fprinthex(stdout, filepos, 8);
    printf(": ");
    while(len--) {
        uint8_t c = *src++;
        printf("%02X ", c);
    }
    printf("(%s)\n", type);
}

////////////////////////////////////////////////////////////////////////////////

int matchtest(
    const uint8_t* string,
    const uint8_t* buffer,
    size_t         increment,
    int            shift
) {
    int16_t stringstart;
    int16_t bufferstart;
    for(;;) {
        uint8_t c = *string;
        if(!c) { return 1; }
        if(c != '.') {
            stringstart = ((int16_t)( c     )) & 0xFF;
            bufferstart = ((int16_t)(*buffer)) & 0xFF;
            break;
        }
        string++;
        buffer += increment;
    }
    for(;;) {
        uint8_t c = *string;
        if(!c) { break; }
        if(c != '.') {
            int16_t stringnow = ((int16_t)( c     )) & 0xFF;
            int16_t buffernow = ((int16_t)(*buffer)) & 0xFF;
            stringnow -= stringstart;
            buffernow -= bufferstart;
            stringnow <<= shift;
            if(stringnow != buffernow) { return 0; }
        }
        string++;
        buffer += increment;
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

off_t relsearch(
    const uint8_t* string,
    const char*    filename
) {
    FILE*    f = NULL;
    uint8_t* buffer = NULL;

    size_t   stringlen;
    size_t   buffersize;
    off_t    matchesfound = 0;
    off_t    bufferbase;
    size_t   bufferpos;
    size_t   bufferlen;

    //
    // Examine length of search string
    //
    stringlen = strlen((const char*)string);
    // Avoid overflow
    if(stringlen > (((size_t)(-1)) / 2)) {
        printf("String is too long\n"); // very rare case
        goto done;                      // very rare case
    }
    //
    // Allocate buffer
    //
    buffersize = 2 * stringlen - 1;
    if(buffersize < 4096) {
        buffersize = 4096;
    }
    buffer = malloc(buffersize);
    if(!buffer) {
        printf("Out of memory\n");
        goto done;
    }

    f = fopen(filename, "rb");
    if(!f) { goto error_f; }

    printf("%s: ", filename);
    bufferbase = 0;
    bufferpos = 0;
    bufferlen = 0;
    for(;;) {
        size_t readsize;
        if(bufferlen && ((buffersize - bufferpos) < (2 * stringlen - 1))) {
            memmove(buffer, buffer + bufferpos, bufferlen);
            bufferbase += bufferpos;
            bufferpos = 0;
        }
        readsize = buffersize - (bufferpos + bufferlen);
        if((readsize > 0) && (!feof(f))) {
            readsize = fread(buffer + (bufferpos + bufferlen), 1, readsize, f);
            bufferlen += readsize;
        }
        if(bufferlen < stringlen) break;
             if(matchtest(string, buffer + bufferpos, 1, 0)) { if(matchesfound < 1) printf("\n"); matchesfound++; report(bufferbase + bufferpos, buffer + bufferpos, stringlen, "normal"); }
        else if(matchtest(string, buffer + bufferpos, 1, 1)) { if(matchesfound < 1) printf("\n"); matchesfound++; report(bufferbase + bufferpos, buffer + bufferpos, stringlen, "double"); }
        if(bufferlen >= (2 * stringlen - 1)) {
                 if(matchtest(string, buffer + bufferpos, 2, 0)) { if(matchesfound < 1) printf("\n"); matchesfound++; report(bufferbase + bufferpos, buffer + bufferpos, 2 * stringlen - 1, "wide"); }
            else if(matchtest(string, buffer + bufferpos, 2, 1)) { if(matchesfound < 1) printf("\n"); matchesfound++; report(bufferbase + bufferpos, buffer + bufferpos, 2 * stringlen - 1, "wide double"); }
        }
        bufferpos++;
        bufferlen--;
    }
    printf(
        "%lu match%s found\n",
        (unsigned long)matchesfound,
        (matchesfound == 1) ? "" : "es"
    );

    goto done;

error_f:
    printfileerror(f, filename);

done:
    if(f      != NULL) { fclose(f); }
    if(buffer != NULL) { free(buffer); }

    return matchesfound;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    const uint8_t* string;
    int i;
    off_t total = 0;

    normalize_argv0(argv[0]);

    if(argc < 3) {
        banner();
        printf(
            "Usage: %s string files\n"
            "\n"
            "Search string may include '.' characters as wildcards, but must include at\n"
            "least two non-wildcard characters.\n",
            argv[0]
        );
        return 1;
    }
    string = (const uint8_t*)(argv[1]);
    {   size_t nwc = 0;
        const uint8_t* s = string;
        for(;;) {
            uint8_t c = *s++;
            if(!c) { break; }
            if(c != '.') { nwc++; }
        }
        if(nwc < 2) {
            printf(
                "Search string must contain at least two non-wildcard characters\n"
            );
            return 1;
        }
    }
    printf("Searching for \"%s\":\n", string);
    for(i = 2; i < argc; i++) {
        total += relsearch(string, argv[i]);
    }
    printf(
        "Total: %lu %s found\n",
        (unsigned long)total,
        (total == 1) ? "match" : "matches"
    );
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

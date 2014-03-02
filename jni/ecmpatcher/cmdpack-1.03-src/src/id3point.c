////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "id3point - Pointless ID3v1 Tagger"
#define COPYR "Copyright (C) 2001,2010 Neill Corlett"
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

enum {
    TITLE_START    =   3,
    TITLE_LEN      =  30,
    ARTIST_START   =  33,
    ARTIST_LEN     =  30,
    ALBUM_START    =  63,
    ALBUM_LEN      =  30,
    YEAR_START     =  93,
    YEAR_LEN       =   4,
    COMMENT_START  =  97,
    COMMENT_LEN    =  29,
    TRACKNUM_START = 126,
    GENRE_START    = 127
};

////////////////////////////////////////////////////////////////////////////////

static void usage(const char *name) {
    banner();
    printf(
        "Usage: %s [options] mp3file(s)\n"
        "\n"
        "Options:\n"
        "  -t title     Set ID3 title       -t-   Clear ID3 title\n"
        "  -a artist    Set ID3 artist      -a-   Clear ID3 artist\n"
        "  -m album     Set ID3 album       -m-   Clear ID3 album\n"
        "  -y year      Set ID3 year        -y-   Clear ID3 year\n"
        "  -c comment   Set ID3 comment     -c-   Clear ID3 comment\n"
        "  -k track#    Set ID3 track #     -k-   Clear ID3 track #\n"
        "  -g genre#    Set ID3 genre #     -g-   Clear ID3 genre #\n"
        "  -tf          Set ID3 title based on the filename\n"
        "\n"
        "Notes:\n"
        "  - Only ID3v1 tags are supported.\n"
        "  - The track number increments automatically for each additional file.\n"
        "  - The -tf option will turn \"Artist - Title.mp3\" into \"Title\", but\n"
        "    be sure to check the titles by hand in case it gets confused.\n",
        name
    );
}

////////////////////////////////////////////////////////////////////////////////

static void cleanstring(
    char *s,
    int size
) {
    int l = strlen(s) + 1;
    int z = size - l;
    if(z > 0) memset(s + l, 0, z);
}

////////////////////////////////////////////////////////////////////////////////

int main(
    int argc,
    char **argv
) {
    FILE *f = NULL;
    int returncode = 0;
    int n;
    int8_t set_title    = 0; char title  [TITLE_LEN  +1] = {0};
    int8_t set_artist   = 0; char artist [ARTIST_LEN +1] = {0};
    int8_t set_album    = 0; char album  [ALBUM_LEN  +1] = {0};
    int8_t set_year     = 0; char year   [YEAR_LEN   +1] = {0};
    int8_t set_comment  = 0; char comment[COMMENT_LEN+1] = {0};
    int8_t set_tracknum = 0; int tracknum = 0;
    int8_t set_genre    = 0; int genre    = 0;

    normalize_argv0(argv[0]);

    for(n = 1; n < argc; n++) {
        if(argv[n][0] != '-') break;
        if(!strcmp(argv[n], "--")) { n++; break; }
        if(!strcmp(argv[n], "-t")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_title    = 1; strncpy(title  , argv[n], TITLE_LEN  ); }
        if(!strcmp(argv[n], "-a")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_artist   = 1; strncpy(artist , argv[n], ARTIST_LEN ); }
        if(!strcmp(argv[n], "-m")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_album    = 1; strncpy(album  , argv[n], ALBUM_LEN  ); }
        if(!strcmp(argv[n], "-y")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_year     = 1; strncpy(year   , argv[n], YEAR_LEN   ); }
        if(!strcmp(argv[n], "-c")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_comment  = 1; strncpy(comment, argv[n], COMMENT_LEN); }
        if(!strcmp(argv[n], "-k")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_tracknum = 1; tracknum = atoi(argv[n]); }
        if(!strcmp(argv[n], "-g")) { n++; if(n >= argc) { usage(argv[0]); goto error; } set_genre    = 1; genre    = atoi(argv[n]); }
        if(!strcmp(argv[n], "-t-")) { set_title    = 1; title  [0] = 0; }
        if(!strcmp(argv[n], "-a-")) { set_artist   = 1; artist [0] = 0; }
        if(!strcmp(argv[n], "-m-")) { set_album    = 1; album  [0] = 0; }
        if(!strcmp(argv[n], "-y-")) { set_year     = 1; year   [0] = 0; }
        if(!strcmp(argv[n], "-c-")) { set_comment  = 1; comment[0] = 0; }
        if(!strcmp(argv[n], "-k-")) { set_tracknum = 1; tracknum = 0; }
        if(!strcmp(argv[n], "-g-")) { set_genre    = 1; genre    = 255; }
        if(!strcmp(argv[n], "-tf")) { set_title    = -1; }
    }

    if(n >= argc) {
        usage(argv[0]);
        goto error;
    }

    cleanstring(title  , sizeof(title  ));
    cleanstring(artist , sizeof(artist ));
    cleanstring(album  , sizeof(album  ));
    cleanstring(year   , sizeof(year   ));
    cleanstring(comment, sizeof(comment));

    for(; n < argc; n++) {
        char *mp3filename = argv[n];
        printf("%s: ", mp3filename);

        if(f) { fclose(f); f = NULL; }
        f = fopen(mp3filename, "r+b");
        if(!f) {
            printf("%s\n", strerror(errno));
            returncode = 1;
            continue;
        }

        //
        // File must be at least 128 bytes
        //
        fseeko(f, 0, SEEK_END);
        if(ftello(f) < 128) {
            fclose(f); f = NULL;
            printf("file is too short\n");
            returncode = 1;
            continue;
        }

        //
        // See if there's an ID3 tag, and if not, create one
        //
        fseeko(f, -128, SEEK_END);
        if(
            (fgetc(f) != 'T') ||
            (fgetc(f) != 'A') ||
            (fgetc(f) != 'G')
        ) {
            int i;
            printf("(creating tag) ");
            fseeko(f, 0, SEEK_END);
            fputc('T', f);
            fputc('A', f);
            fputc('G', f);
            for(i = 3; i < 127; i++) fputc(0, f);
            fputc(255, f);
        }

        //
        // Get title from the filename if we need to
        //
        if(set_title == -1) {
            char *m;
            m = strrchr(mp3filename, '-');
            if(!m) {
                m = mp3filename;
            } else {
                m++;
            }
            //
            // Skip spaces/underscores
            //
            while(((*m) == ' ') || ((*m) == '_')) m++;
            //
            // Skip track numbers
            //
            while((*m >= '0') && (*m <= '9')) m++;
            //
            // Skip more spaces/underscores
            //
            while(((*m) == ' ') || ((*m) == '_')) m++;
            strncpy(title, m, TITLE_LEN);
            m = strchr(title, '.');
            if(m) *m = 0;
            while((m = strchr (title, '_')) != NULL) { *m = ' '; }
            while(strlen(title) && title[strlen(title) - 1] == ' ') {
                title[strlen(title) - 1] = 0; // rare case: difficult to test in DOS
            }
            cleanstring(title, sizeof(title));
        }
        //
        // Set various things
        //
        if(set_title   ) { fseeko(f, -128 + TITLE_START   , SEEK_END); fwrite(title  , TITLE_LEN  , 1, f); }
        if(set_artist  ) { fseeko(f, -128 + ARTIST_START  , SEEK_END); fwrite(artist , ARTIST_LEN , 1, f); }
        if(set_album   ) { fseeko(f, -128 + ALBUM_START   , SEEK_END); fwrite(album  , ALBUM_LEN  , 1, f); }
        if(set_year    ) { fseeko(f, -128 + YEAR_START    , SEEK_END); fwrite(year   , YEAR_LEN   , 1, f); }
        if(set_comment ) { fseeko(f, -128 + COMMENT_START , SEEK_END); fwrite(comment, COMMENT_LEN, 1, f); }
        if(set_tracknum) { fseeko(f, -128 + TRACKNUM_START, SEEK_END); fputc(tracknum, f); }
        if(set_genre   ) { fseeko(f, -128 + GENRE_START   , SEEK_END); fputc(genre, f); }

        fclose(f); f = NULL;

        printf("ok\n");
        if(tracknum) {
            tracknum++;
        }
    }

    goto done;

error:
    returncode = 1;

done:

    if(f) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

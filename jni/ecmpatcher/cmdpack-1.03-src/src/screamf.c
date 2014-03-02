////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "screamf - .AMF to .S3M converter"
#define COPYR "Copyright (C) 1996,2010 Neill Corlett"
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

// S3M parapointer limit
static const off_t S3MFILESIZELIMIT    = (off_t)0xFFFFFLU;

// Pattern size limit
static const off_t S3MPATTERNSIZELIMIT = (off_t) 0xFFFFLU;

////////////////////////////////////////////////////////////////////////////////

struct INSTRUMENT {
    char     name    [33];
    char     filename[14];
    uint8_t  sampled;
    uint32_t samplelength;
    uint32_t loopstart;
    uint32_t loopend;
    uint16_t c4spd;
    uint8_t  defaultvol;
    uint8_t* sampledata;
};

struct PLACE {
    int8_t  note;
    int8_t  octave;
    int8_t  instrument;
    int8_t  effect;
    int16_t volume;
    int16_t parameter;
};

struct TRACK {
    struct PLACE place[64];
};

struct MODULE {
    char     title[33];
    uint8_t  insnum;
    uint8_t  ordnum;
    size_t   tracknum; // AMF only
    uint8_t  channels;
    int8_t   panposition[32];
    uint8_t  tempo;
    uint8_t  speed;
};

////////////////////////////////////////////////////////////////////////////////

static void fput32lsb(uint32_t c, FILE* stream) {
    fputc(((c      ) & 0xFF), stream);
    fputc(((c >>  8) & 0xFF), stream);
    fputc(((c >> 16) & 0xFF), stream);
    fputc(((c >> 24) & 0xFF), stream);
}

static void fput16lsb(uint16_t c, FILE* stream) {
    fputc(((c      ) & 0xFF), stream);
    fputc(((c >>  8) & 0xFF), stream);
}

static uint32_t fget32lsb(FILE* stream) {
    uint32_t t;
    t  = (((uint32_t)(fgetc(stream) & 0xFF)) <<  0);
    t |= (((uint32_t)(fgetc(stream) & 0xFF)) <<  8);
    t |= (((uint32_t)(fgetc(stream) & 0xFF)) << 16);
    t |= (((uint32_t)(fgetc(stream) & 0xFF)) << 24);
    return t;
}

static uint16_t fget16lsb(FILE* stream) {
    uint16_t t;
    t  = (((uint16_t)(fgetc(stream) & 0xFF)) <<  0);
    t |= (((uint16_t)(fgetc(stream) & 0xFF)) <<  8);
    return t;
}

//
// Align file offset to a paragraph (16 bytes)
//
static void alignpara(FILE* f) {
    while((ftello(f) & 0xF) != 0) {
        fputc(0, f);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns nonzero on error
//
static int amf_decodetrack(
    FILE* amffile,
    struct TRACK* track,
    uint8_t insnum
) {
    int16_t i,c,n,row,num,effect;
    struct PLACE* place = track->place;

    //
    // Initialize track to empty/unused
    //
    for(i = 0; i < 64; i++) {
        place[i].note       = -1;
        place[i].octave     = -1;
        place[i].instrument = -1;
        place[i].volume     = -1;
        place[i].effect     = -1;
        place[i].parameter  = -1;
    }
    n      = fgetc(amffile);
    num    = fgetc(amffile); // lost?
    effect = fgetc(amffile); // lost?
    for(c = 0; c < n; c++) {
        row    = fgetc(amffile);
        num    = fgetc(amffile);
        effect = fgetc(amffile);

        if(row == 0xFF) { break; }
        if(row >= 64) {
            printf("row out of bounds");
            goto errorat;
        }

        if(num == 0x80) {
            for(i = row; i < 64; i++) {
                if(effect >= insnum) {
                    printf("instrument out of bounds");
                    goto errorat;
                }
                place[i].instrument = (int8_t)effect;
            }
        } else if(num > 0x80) {
            place[row].effect    = num & 0x7F;
            place[row].parameter = (int8_t)effect;
        } else {
            place[row].note   = num % 12;
            place[row].octave = num / 12;
            place[row].volume = effect;
        }
    }
    //
    // Postprocess the track
    //
    for(i = 0; i < 64; i++) {
        //
        // remove phantom instruments
        //
        if(place[i].note == -1) {
            place[i].instrument = -1;
        }
        //
        // Convert effects
        //
        switch(place[i].effect) {
            case -1:
                break;
            case 1: // Set Speed [A]
                place[i].effect='A'-64;
                break;
            case 2: // Volume Slide [D]
                place[i].effect='D'-64;
                //
                // Convert to S3M style
                //
                if(place[i].parameter < 0) {
                    place[i].parameter=-(place[i].parameter);
                } else {
                    place[i].parameter<<=4;
                }
                break;
            case 3: // Volume Change
                // (What's this doing in here?)
                place[i].volume    = place[i].parameter;
                place[i].effect    = -1;
                place[i].parameter = -1;
                break;
            case 4: // Portamento [E]+, [F]-
                if(place[i].parameter < 0) {
                    place[i].effect='F'-64;
                    place[i].parameter=-(place[i].parameter);
                } else {
                    place[i].effect='E'-64;
                }
                break;
            case 6: // Tone Portamento [G]
                place[i].effect='G'-64;
                break;
            case 7: // Tremolo [R]
                place[i].effect='R'-64;
                break;
            case 8: // [J]
                place[i].effect='J'-64;
                break;
            case 9: // [H]
                place[i].effect='H'-64;
                break;
            case 10: // [L] (negative)
                place[i].effect='L'-64;
                place[i].parameter=-(place[i].parameter);
                break;
            case 11: // [K] (negative)
                place[i].effect='K'-64;
                place[i].parameter=-(place[i].parameter);
                break;
            case 12: // [C]
                place[i].effect='C'-64;
                break;
            case 13: // [B]
                place[i].effect='B'-64;
                break;
            case 15: // [Q]
                place[i].effect='Q'-64;
                break;
            case 16: // Set Sample Offset [O]
                place[i].effect='O'-64;
                break;
            case 17: // Find Volume Slide [DxF]+, [DFx]-
                place[i].effect='D'-64;
                //
                // convert to S3M style
                //
                if(place[i].parameter<(-14))place[i].parameter=(-14);
                if(place[i].parameter>( 14))place[i].parameter=( 14);
                if(place[i].parameter<0) {
                    place[i].parameter=-(place[i].parameter);
                    place[i].parameter|=0xF0;
                } else if(place[i].parameter>0) {
                    place[i].parameter<<=4;
                    place[i].parameter|=0x0F;
                } else {
                    place[i].effect=-1;
                    place[i].parameter=-1;
                }
                break;
            case 18: // Fine Portamento [EF]+, [FF]-
                if(place[i].parameter<0) {
                    place[i].effect='F'-64;
                    place[i].parameter=-(place[i].parameter);
                } else {
                    place[i].effect='E'-64;
                }
                if(place[i].parameter>0xF)place[i].parameter=0xF;
                place[i].parameter|=0xF0;
                break;
            case 21: // Set Tempo [T]
                place[i].effect='T'-64;
                break;
            case 22: // Extra Fine Portamento [EE]+, [FE]-
                if(place[i].parameter<0) {
                    place[i].effect='F'-64;
                    place[i].parameter=-(place[i].parameter);
                } else {
                    place[i].effect='E'-64;
                }
                if(place[i].parameter>0xF)place[i].parameter=0xF;
                place[i].parameter|=0xE0;
                break;
            case 23: // [X]
                place[i].effect='X'-64;
                break;
            default: // Remove the unknown effect
                printf(
                    "Unknown effect %02X-%02X\n",
                    place[i].effect,
                    place[i].parameter
                );
                place[i].effect=-1;
                break;
        }
    }
    return 0;

errorat:
    printf(" @ 0x");
    fprinthex(stdout, ftello(amffile), 1);
    printf("\n");
    return 1;
}

static void amf_decodeinstrument(
    FILE* amffile,
    struct INSTRUMENT* instrument
) {
    size_t i;

    instrument->sampled = fgetc(amffile);

    for(i = 0; i < 32; i++) {
        instrument->name[i] = fgetc(amffile);
    }
    instrument->name[i] = 0;

    for(i = 0; i < 13; i++) {
        instrument->filename[i] = fgetc(amffile);
    }
    instrument->filename[i] = 0;

                               fget32lsb(amffile); // TODO: unknown
    instrument->samplelength = fget32lsb(amffile);
    instrument->c4spd        = fget16lsb(amffile);
    instrument->defaultvol   = fgetc    (amffile);

    instrument->loopstart    = fget32lsb(amffile);
    instrument->loopend      = fget32lsb(amffile);
}

static void s3m_writeinstrument(
    const struct INSTRUMENT* instrument,
    FILE* f
) {
    fputc(instrument->sampled, f);
    fwrite(instrument->filename, 1, 13, f);
    fput16lsb(0, f); // sample pointer - zero by default
    fput32lsb(instrument->samplelength, f);
    fput32lsb(instrument->loopstart, f);
    fput32lsb(instrument->loopend, f);
    fputc(instrument->defaultvol, f);
    fputc(0, f);
    fputc(0, f);
    if(
        instrument->loopstart != 0 ||
        instrument->loopend   != 0
    ) {
        fputc(1, f);
    } else {
        fputc(0, f);
    }
    fput32lsb(instrument->c4spd, f);
    //
    // Next 12 bytes are reserved
    //
    fput32lsb(0, f);
    fput32lsb(0, f);
    fput32lsb(0, f);
    fwrite(instrument->name, 1, 28, f);
    fputc('S', f);
    fputc('C', f);
    fputc('R', f);
    fputc('S', f);
}

////////////////////////////////////////////////////////////////////////////////

static uint8_t s3morderlist[256];

int main(int argc, char** argv) {
    int returncode = 0;

    FILE*     amffile = NULL;
    FILE*     s3mfile = NULL;

    const char* amffilename = "";
    const char* s3mfilename = "";

    uint16_t*          ordertable      = NULL;
    uint16_t*          tracktable      = NULL;
    struct INSTRUMENT* instrument      = NULL;
    uint16_t*          s3mpatterntable = NULL;
    uint8_t            s3mpatterns;

    struct TRACK**     amftrack        = NULL;
    size_t             amftracknum     = 0;

    char      amfheader[4];
    struct MODULE module;
    off_t     fpos_tracktable;
    off_t     fpos_instrument;
    off_t     fpos_samples;
    off_t     s3minstrumentstart;
    off_t     s3mpatternpointers;

    uint32_t i, j, k;

    normalize_argv0(argv[0]);

    if(argc != 3) {
        banner();
        printf(
            "Usage: %s inputfile.amf outputfile.s3m\n",
            argv[0]
        );
        goto error;
    }
    amffilename = argv[1];
    amffile = fopen(amffilename, "rb");
    if(!amffile) { goto error_amffile; }
    amfheader[0] = fgetc(amffile);
    amfheader[1] = fgetc(amffile);
    amfheader[2] = fgetc(amffile);
    amfheader[3] = 0;
    if(strcmp(amfheader, "AMF")) {
        printf("'%s' is not an AMF file\n",amffilename);
        goto conversionfailed;
    }
    if(fgetc(amffile) != 0xE) {
        printf("'%s' - unrecognized AMF version\n",amffilename);
        goto conversionfailed;
    }
    //
    // Ensure the S3M file doesn't already exist
    //
    s3mfilename = argv[2];
    s3mfile = fopen(s3mfilename, "rb");
    if(s3mfile != NULL) {
        printf("%s already exists; refusing to overwrite\n", s3mfilename);
        goto conversionfailed;
    }

    for(i = 0; i < 32; i++) {
        module.title[i] = fgetc(amffile);
    }
    module.title[i] = 0;

    module.insnum   = fgetc    (amffile);
    module.ordnum   = fgetc    (amffile);
    module.tracknum = fget16lsb(amffile);
    module.channels = fgetc    (amffile);
    for(i = 0; i < 32; i++) {
        module.panposition[i] = fgetc(amffile);
    }
    module.tempo    = fgetc    (amffile);
    module.speed    = fgetc    (amffile);

    if(module.insnum == 0) {
        printf("Module apparently has zero instruments\n");
        goto conversionfailed;
    }
    if(module.ordnum == 0) {
        printf("Module apparently has zero orders\n");
        goto conversionfailed;
    }
    if(module.tracknum == 0) {
        printf("Module apparently has zero tracks\n");
        goto conversionfailed;
    }
    if(module.channels == 0) {
        printf("Module apparently has zero channels\n");
        goto conversionfailed;
    }
    if(module.channels > 32) {
        printf("Module reports too many channels (%u); max is 32\n", module.channels);
        goto conversionfailed;
    }

    //
    // Print out general information
    //
    printf(
        "Title..........%s\n"
        "Channels.......%lu\n"
        "Orders.........%lu\n"
        "Track entries..%lu\n"
        "Instruments....%lu\n"
        "Tempo..........%lu bpm\n"
        "Speed..........%lu\n\n",
                       module.title,
        (unsigned long)module.channels,
        (unsigned long)module.ordnum,
        (unsigned long)module.tracknum,
        (unsigned long)module.insnum,
        (unsigned long)module.tempo,
        (unsigned long)module.speed
    );

    //
    // Allocate memory for all kinds of things, except AMF tracks
    //
    ordertable = malloc(sizeof(*ordertable) * module.channels * module.ordnum);
    if(!ordertable) { printf("Out of memory\n"); goto conversionfailed; }

    if(module.tracknum > (((size_t)(-1)) / sizeof(*tracktable))) {
        printf("Too many tracks (%lu)\n", (unsigned long)module.tracknum);
        goto conversionfailed;
    }
    tracktable = malloc(sizeof(*tracktable) * module.tracknum);
    if(!tracktable) { printf("Out of memory\n"); goto conversionfailed; }

    instrument = malloc(sizeof(*instrument) * module.insnum);
    if(!instrument) { printf("Out of memory\n"); goto conversionfailed; }

    //
    // Determine the number of real tracks present
    //
    printf("Reading and decoding AMF...\n");
    fpos_instrument = (off_t)0x4B + ((((off_t)module.channels + 1) * (off_t)module.ordnum) * 2);
    fpos_tracktable = (off_t)fpos_instrument + (off_t)65 * (off_t)module.insnum;
    fseeko(amffile, fpos_tracktable, SEEK_SET);
    for(i = 0; i < module.tracknum; i++) {
        tracktable[i] = fget16lsb(amffile);
        if(tracktable[i] > amftracknum) {
            amftracknum = tracktable[i];
        }
        // make zero-based
        if(tracktable[i] > 0) { tracktable[i]--; }
    }

    //
    // Allocate space for real tracks
    //
    if(amftracknum > (((size_t)(-1)) / sizeof(*amftrack))) {
        printf("Too many amftracks (%lu)\n", (unsigned long)amftracknum);
        goto conversionfailed;
    }
    amftrack = malloc(sizeof(*amftrack) * amftracknum);
    if(!amftrack) { printf("Out of memory\n"); goto conversionfailed; }

    for(i = 0; i < amftracknum; i++) { amftrack[i] = NULL; }
    for(i = 0; i < amftracknum; i++) {
        amftrack[i] = malloc(sizeof(*amftrack[i]));
        if(!amftrack[i]) { printf("Out of memory\n"); goto conversionfailed; }
    }

    //
    // Decode the AMF tracks
    //
    for(i = 0; i < amftracknum; i++) {
        if(amf_decodetrack(amffile, amftrack[i], module.insnum)) {
            goto conversionfailed;
        }
    }
    //
    // Save this place, this is where the samples are stored
    //
    fpos_samples = ftello(amffile);
    //
    // Now decode the orderlist, replacing track tags with real track numbers
    //
    fseeko(amffile, 0x4B, SEEK_SET);
    for(i = 0; i < module.ordnum; i++) {
        fget16lsb(amffile);
        for(j = 0; j < module.channels; j++) {
            uint16_t x = fget16lsb(amffile);
            ordertable[i*module.channels+j] = tracktable[x ? x-1 : 0];
        }
    }
    //
    // Now decode the instrument headers
    //
    for(i = 0; i < module.insnum; i++) {
        amf_decodeinstrument(amffile, instrument + i);
    }
    printf("Processing patterns...");

    //
    // Rearrange the patterns in a more S3M-friendly way
    //
    s3mpatterntable = malloc(sizeof(*s3mpatterntable) * module.ordnum * module.channels);
    if(!s3mpatterntable) { printf("Out of memory\n"); goto conversionfailed; }

    //
    // Start numbering S3M patterns with 0
    //
    s3mpatterns = 0;
    //
    // Go through the entire AMF orderlist
    //
    for(i = 0; i < module.ordnum; i++) {
        //
        // Is there already a S3M pattern with this track configuration?
        //
        for(j = 0; j < s3mpatterns; j++) {
            if(!memcmp(
                s3mpatterntable + j * module.channels,
                ordertable      + i * module.channels,
                sizeof(*s3mpatterntable) * module.channels
            )) {
                break;
            }
        }
        s3morderlist[i] = (uint8_t)j;
        if(j == s3mpatterns) {
            // We need to create a new pattern
            memmove(
                s3mpatterntable + j * module.channels,
                ordertable      + i * module.channels,
                sizeof(*s3mpatterntable) * module.channels
            );
            if(s3mpatterns >= 0xFF) {
                printf("Too many patterns!\n");
                goto conversionfailed;
            }
            s3mpatterns++;
        }
    }
    printf(" (%u patterns used)\n", s3mpatterns);
    //
    // Now s3mpatterns holds the number of S3M patterns required.
    // (plus, we know how to set up each pattern now)
    //

    //
    // Try creating the S3M file
    //
    s3mfile = fopen(s3mfilename, "wb");
    if(!s3mfile) { goto error_s3mfile; }
    //
    // Begin writing it
    //
    printf("Writing S3M header information...\n");
    fwrite(module.title, 1, 28, s3mfile);
    fput32lsb(0x101A, s3mfile);
    //
    // OrdNum has to be even in the S3M file, so adjust it
    //
    fput16lsb((module.ordnum + 1) & (~1), s3mfile);

    fput16lsb(module.insnum, s3mfile);
    fput16lsb(s3mpatterns, s3mfile);
    fput16lsb(64, s3mfile); // flags
    fput16lsb(0x1301, s3mfile); // created with Scream Tracker 3.01
    fput16lsb(2, s3mfile); // unsigned samples
    fputc('S', s3mfile);
    fputc('C', s3mfile);
    fputc('R', s3mfile);
    fputc('M', s3mfile);
    fputc(0x40, s3mfile); // global volume
    fputc(module.speed, s3mfile); // initial speed
    fputc(module.tempo, s3mfile); // initial tempo
    fputc(0xB0, s3mfile); // master volume
    fputc(16, s3mfile); // ultraclick removal level=8
    fputc(0, s3mfile); // don't bother with pan positions
    //
    // next 10 bytes are reserved
    //
    fputc(0, s3mfile);fputc(0, s3mfile);fputc(0, s3mfile);fputc(0, s3mfile);
    fputc(0, s3mfile);fputc(0, s3mfile);fputc(0, s3mfile);fputc(0, s3mfile);
    fputc(0, s3mfile);fputc(0, s3mfile);
    //
    // Now set up the default LRLR... panning
    //
    for(i = 0; i < module.channels; i++) {
        fputc(((i & 1) << 3) | (i >> 1), s3mfile);
    }
    for(i = module.channels; i < 32; i++) {
        fputc(0xFF, s3mfile);
    }
    //
    // Write out the orderlist
    //
    for(i = 0; i < module.ordnum;i++) {
        fputc(s3morderlist[i], s3mfile);
    }
    //
    // Adjust for that even-ordnum thing
    //
    if(module.ordnum & 1) {
        fputc(0xFF, s3mfile);
        module.ordnum++;
    }
    //
    // Precompute and write out where all the instrument headers are going to be
    //
    s3minstrumentstart = (
        ((0x60 + module.ordnum + 2 * module.insnum + 2 * s3mpatterns) + 0xF) & (~0xF)
    );
    for(i = 0; i < module.insnum; i++) {
        fput16lsb((uint16_t)((s3minstrumentstart + 80 * i) >> 4), s3mfile);
    }
    //
    // Zero out the pattern parapointers for now
    //
    s3mpatternpointers = ftello(s3mfile);
    for(i = 0; i < s3mpatterns; i++) {
        fput16lsb(0, s3mfile);
    }
    //
    // Wait until the boundary is an even paragraph
    //
    alignpara(s3mfile);
    //
    // Write out all instrument headers (making up sample locations)
    //
    for(i = 0; i < module.insnum;i++) {
        s3m_writeinstrument(instrument + i, s3mfile);
    }

    //
    // Squeeze some extra bytes out of each track
    //
    for(i = 0; i < amftracknum; i++) {
        struct PLACE* place = amftrack[i]->place;
        for(j = 0; j < 64; j++) {
            if(
                place[j].note != -1 &&
                place[j].instrument != -1 &&
                place[j].volume == instrument[place[j].instrument].defaultvol
            ) {
                place[j].volume = -1;
            }
        }
    }

    //
    // Now encode and write patterns
    //
    printf("Writing patterns...\n");
    for(i = 0; i < s3mpatterns; i++) {
        off_t pattern_start;
        off_t pattern_end;

        //
        // Align to the next paragraph offset
        //
        alignpara(s3mfile);

        //
        // Obtain the pattern pointer
        //
        pattern_start = ftello(s3mfile);
        if(pattern_start > S3MFILESIZELIMIT) {
            printf("S3M file is too large!\n");
            goto conversionfailed;
        }

        //
        // Go back and write the pattern parapointer
        //
        fseeko(s3mfile, s3mpatternpointers + 2 * i, SEEK_SET);
        fput16lsb((uint16_t)(pattern_start >> 4), s3mfile);

        fseeko(s3mfile, pattern_start, SEEK_SET);

        // Dummy size - fill it in later
        fput16lsb(0, s3mfile);

        for(j = 0; j < 64; j++) {
            for(k = 0; k < module.channels; k++) {
                struct PLACE* thisplace = amftrack[
                    s3mpatterntable[module.channels * i + k]
                ]->place + j;

                uint8_t s3mbytewhat = 0;

                if(thisplace->note   != -1) { s3mbytewhat |= 0x20; }
                if(thisplace->volume != -1) { s3mbytewhat |= 0x40; }
                if(thisplace->effect != -1) { s3mbytewhat |= 0x80; }

                if(s3mbytewhat != 0) {
                    s3mbytewhat |= k;
                    fputc(s3mbytewhat, s3mfile);
                    if((s3mbytewhat & 0x20) != 0) {
                        if(thisplace->octave > 0) {
                            fputc(((thisplace->note)|
                                (((thisplace->octave)-1)<<4)), s3mfile);
                            fputc(thisplace->instrument+1, s3mfile);
                        } else { // note cut
                            fputc(254, s3mfile);
                            fputc(0, s3mfile);
                        }
                    }
                    if((s3mbytewhat & 0x40) != 0) {
                        fputc(thisplace->volume   , s3mfile);
                    }
                    if((s3mbytewhat & 0x80) != 0) {
                        fputc(thisplace->effect   , s3mfile);
                        fputc(thisplace->parameter, s3mfile);
                    }
                }
            }
            fputc(0, s3mfile);
        }

        //
        // Mark end of pattern
        //
        pattern_end = ftello(s3mfile);
        if((pattern_end - pattern_start) > S3MPATTERNSIZELIMIT) {
            printf("S3M pattern is too large!\n");
            goto conversionfailed;
        }

        //
        // Go back and write the pattern size
        //
        fseeko(s3mfile, pattern_start, SEEK_SET);
        fput16lsb((uint16_t)(pattern_end - pattern_start), s3mfile);

        fseeko(s3mfile, pattern_end, SEEK_SET);
    }

    //
    // Now write out the samples
    //
    printf("Writing samples...\n");
    fseeko(amffile, fpos_samples, SEEK_SET);
    for(i = 0; i < module.insnum; i++) {
        if(instrument[i].sampled == 1) {
            off_t sample_start;

            //
            // Align to the next paragraph offset
            //
            alignpara(s3mfile);

            //
            // Obtain the sample pointer
            //
            sample_start = ftello(s3mfile);
            if(sample_start > S3MFILESIZELIMIT) {
                printf("S3M file is too large!\n");
                goto conversionfailed;
            }

            //
            // Go back and write the sample pointer in the instrument
            //
            fseeko(s3mfile, s3minstrumentstart + 80 * i + 14, SEEK_SET);
            fput16lsb((uint16_t)(sample_start >> 4), s3mfile);

            fseeko(s3mfile, sample_start, SEEK_SET);

            //
            // Copy the actual sample
            //
            for(j = 0; j < instrument[i].samplelength; j++) {
                fputc(fgetc(amffile), s3mfile);
            }
        }
    }
    printf("Done!\n");
    //
    // Success (hopefully)
    //
    goto done;

conversionfailed:
    printf("Conversion failed\n");
    goto error;

error_amffile: printfileerror(amffile, amffilename); goto error;
error_s3mfile: printfileerror(s3mfile, s3mfilename); goto error;

error:
    returncode = 1;

done:
    //
    // Free up all allocated memory
    //
    if(ordertable      != NULL) { free(ordertable     ); }
    if(tracktable      != NULL) { free(tracktable     ); }
    if(instrument      != NULL) { free(instrument     ); }
    if(s3mpatterntable != NULL) { free(s3mpatterntable); }
    if(amftrack != NULL) {
        for(i = 0; i < amftracknum; i++) {
            if(amftrack[i] != NULL) { free(amftrack[i]); }
        }
        free(amftrack);
    }
    //
    // Close both files
    //
    if(amffile != NULL) { fclose(amffile); }
    if(s3mfile != NULL) { fclose(s3mfile); }

    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

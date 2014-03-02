////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "vb2rip - VB2 sound format ripping utility"
#define COPYR "Copyright (C) 2001,2011 Neill Corlett"
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

static const char* oom = "Error: Out of memory\n";

////////////////////////////////////////////////////////////////////////////////

struct vb2rip_options {
    const char* fmt;
    const char* outname;
    size_t   inputfilecount;
    int8_t   use_channels;
    int8_t   use_honor_endflag;
    int8_t   use_maxbytes;
    int8_t   use_samplerate;
    int8_t   use_offset;
    int8_t   use_interleave;
    int8_t   use_skip;
    int8_t   channels;
    int8_t   honor_endflag;
    off_t    maxbytes;
    uint32_t samplerate;
    off_t    offset;
    off_t    interleave;
    off_t    skip;
};

////////////////////////////////////////////////////////////////////////////////

static uint32_t get32lsb(const uint8_t *data) {
    return
        (((uint32_t)(data[0]))      ) |
        (((uint32_t)(data[1])) <<  8) |
        (((uint32_t)(data[2])) << 16) |
        (((uint32_t)(data[3])) << 24);
}

static void put32lsb(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value      );
    data[1] = (uint8_t)(value >>  8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

////////////////////////////////////////////////////////////////////////////////

static FILE *wav_create(
    const char* filename,
    uint8_t channels,
    uint32_t samplerate
) {
    // .WAV file header
    uint8_t wavhdr[0x2C] = {
        0x52,0x49,0x46,0x46,0x00,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
        0x10,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x44,0xAC,0x00,0x00,0x10,0xB1,0x02,0x00,
        0x04,0x00,0x10,0x00,0x64,0x61,0x74,0x61,0x00,0x00,0x00,0x00
    };
    FILE *f = fopen(filename, "wb");
    if(!f) { goto error; }
    wavhdr[0x16] = channels;
    put32lsb(wavhdr + 0x18, samplerate);
    put32lsb(wavhdr + 0x1C, samplerate * channels * 2);
    if(fwrite(wavhdr, 1, sizeof(wavhdr), f) != sizeof(wavhdr)) { goto error; }
    return f;
error:
    printfileerror(f, filename);
    if(f) { fclose(f); }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns nonzero on error
//
static int wav_close(FILE *f) {
    off_t tl = ftello(f);
    uint8_t buf[4];
    if(tl == -1) { goto error; }
    fflush(f);
    if(fseeko(f, 0x04, SEEK_SET) != 0) { goto error; }
    put32lsb(buf, (uint32_t)(tl - 0x08));
    if(fwrite(buf, 1, 4, f) != 4) { goto error; }
    fflush(f);
    if(fseeko(f, 0x28, SEEK_SET) != 0) { goto error; }
    put32lsb(buf, (uint32_t)(tl - 0x2C));
    if(fwrite(buf, 1, 4, f) != 4) { goto error; }
    fflush(f);
    if(fseeko(f, tl, SEEK_SET) != 0) { goto error; }
    fflush(f);
    fclose(f);
    return 0;
error:
    printfileerror(f, NULL);
    fclose(f);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode a line of samples
//
static void decodeline(
    const uint8_t* src,
    int16_t* outbuf,
    int32_t* p1,
    int32_t* p2
) {
    static int32_t coef[5][2] = {
        {  0,   0},
        { 60,   0},
        {115, -52},
        { 98, -55},
        {122, -60}
    };
    int32_t c;
    size_t i;
    uint8_t fm = *src++;
    uint32_t filter    = (fm >> 4) & 0xF;
    uint32_t magnitude = (fm     ) & 0xF;
    if(magnitude > 12 || filter > 4) {
        magnitude = 12;
        filter = 0;
    }
    src++;
    for(i = 0; i < 14; i++) {
        uint32_t d = *src++;
        int32_t d1 = (d & 0x0F) << (12 + 16);
        int32_t d2 = (d & 0xF0) << ( 8 + 16);
        d1 >>= magnitude + 16;
        d2 >>= magnitude + 16;

        c = d1 +
            ((((*p1) * coef[filter][0]) +
              ((*p2) * coef[filter][1])) >> 6);
        if(c < -32768) c = -32768;
        if(c >  32767) c =  32767;

        *outbuf++ = (int16_t)c;
        (*p2) = (*p1);
        (*p1) = c;

        c = d2 +
            ((((*p1) * coef[filter][0]) +
              ((*p2) * coef[filter][1])) >> 6);
        if(c < -32768) c = -32768;
        if(c >  32767) c =  32767;

        *outbuf++ = (int16_t)c;
        (*p2) = (*p1);
        (*p1) = c;
    }
}

////////////////////////////////////////////////////////////////////////////////

static int cmpcase(const char* a, const char* b) {
    for(;;) {
        int ca = (*a++) & 0xFF;
        int cb = (*b++) & 0xFF;
        int la = tolower(ca) & 0xFF;
        int lb = tolower(cb) & 0xFF;
        if(la != lb) { return (la - lb); }
        if(!la) { return 0; }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// callback which should detect the beginning of a new block
//
typedef off_t (*blockdetect_t)(FILE* f, off_t ofs, off_t size);

////////////////////////////////////////////////////////////////////////////////
//
// returns the number of errors
//
static int rip(
    FILE *inf,
    struct vb2rip_options opts,
    blockdetect_t blockdetect
) {
    int returncode = 1;
    char *outname = NULL;
    uint8_t *block   = NULL;
    uint8_t *block_r = NULL;
    FILE *outf = NULL;
    off_t in_start;
    off_t in_end;
    off_t in_pos;
    off_t out_indicator_pos;
    size_t readsize;
    int8_t endflag = 0;
    int32_t p1[2] = {0, 0}; // per channel
    int32_t p2[2] = {0, 0}; // per channel

    in_pos = in_start = ftello(inf);
    if(in_start == -1) { goto error_in; }

    if(fseeko(inf, 0, SEEK_END) != 0) { goto error_in; }
    in_end = ftello(inf);
    if(in_end == -1) { goto error_in; }
    if(fseeko(inf, in_start, SEEK_SET) != 0) { goto error_in; }

    // use a default for the output filename if it doesn't exist
    if(!opts.outname) { opts.outname = "out"; }

    {   size_t outname_l = strlen(opts.outname);
        outname = malloc(outname_l + 4 + 1);
        if(!outname) { printf("%s", oom); goto error; }
        strcpy(outname, opts.outname);
        if(
            (outname_l < 4) ||
            cmpcase(outname + outname_l - 4, ".wav")
        ) {
            strcat(outname, ".wav");
        }
    }
    printf("writing %s: ", outname);
    fflush(stdout);

    // fill in and validate all parts of opts

    // channel count
    if(!opts.use_channels) { opts.channels = 1; }
    if(opts.channels < 1 || opts.channels > 2) {
        printf("invalid channel count (%d)", (int)opts.channels);
        goto error;
    }

    // honor end flag
    if(!opts.use_honor_endflag) { opts.honor_endflag = 1; }

    // max bytes will be used/checked later

    // sample rate
    if(!opts.use_samplerate) { opts.samplerate = 44100; }

    // offset
    if(!opts.use_offset) { opts.offset = 0; }

    // interleave size
    // defaults to 16K for stereo, 16 bytes (the minimum) for mono
    if(!opts.use_interleave) {
        opts.interleave = (opts.channels == 2) ? 0x4000 : 0x10;
    }
    if((opts.interleave < 1) || ((opts.interleave) & 0xF)) {
        printf("invalid interleave size (");
        fprintdec(stdout, opts.interleave);
        printf("); must be a positive multiple of 16");
        goto error;
    }

    // skip
    if(!opts.use_skip) { opts.skip = 0; }

    // apply the starting offset if necessary
    in_start += opts.offset;
    in_pos = in_start;

    // max bytes
    if(opts.use_maxbytes) {
        if(opts.maxbytes < (in_end - in_start)) {
            in_end = in_start + opts.maxbytes;
        }
    }

    //
    // allocate input blocks
    //
    readsize = (size_t)(opts.interleave < 0x8000 ? opts.interleave : 0x8000);
    block = malloc(readsize);
    if(!block) { printf("%s", oom); goto error; }
    if(opts.channels == 2) {
        block_r = malloc(readsize);
        if(!block_r) { printf("%s", oom); goto error; }
    }

    //
    // now, open the output wav file
    //
    outf = wav_create(outname, opts.channels, opts.samplerate);
    if(!outf) { goto error; }

    out_indicator_pos = ftello(outf);
    if(out_indicator_pos == -1) { goto error_out; }

    endflag = 0;
    for(in_pos = in_start;
        in_pos < in_end &&
        (in_pos + (opts.channels * opts.interleave)) <= in_end &&
        !endflag;
    ) {
        off_t b;

        //
        // See if there's a new first block here
        //
        if((in_pos != in_start) && (blockdetect)) {
            off_t size = opts.channels * opts.interleave;
            if(size > (in_end - in_pos)) {
                size = in_end - in_pos;
            }
            if(blockdetect(inf, in_pos, size) != 0) { break; }
        }

        //
        // Get through this block
        //
        for(b = 0; b < opts.interleave && !endflag; ) {
            int16_t outbuf  [28];
            int16_t outbuf_r[28];
            uint8_t outline[28 * 2 * 2];
            size_t i, j;
            size_t bpc =
                (opts.interleave - b) < ((off_t)readsize) ?
                (size_t)(opts.interleave - b) : readsize;

            //
            // Read left
            //
            if(fseeko(inf, in_pos + b, SEEK_SET) != 0) { goto error_in; }
            clearerr(inf);
            i = fread(block, 1, bpc, inf);
            if(i != bpc) {
                if(ferror(inf)) { goto error_in; }
                break;
            }

            //
            // Read right
            //
            if(opts.channels == 2) {
                if(fseeko(inf, in_pos + opts.interleave + b, SEEK_SET) != 0) {
                    goto error_in;
                }
                clearerr(inf);
                i = fread(block_r, 1, bpc, inf);
                if(i != bpc) {
                    if(ferror(inf)) { goto error_in; }
                    break;
                }
            }

            //
            // If we're honoring the end flag, look for it, and cut this block
            // short if it's there
            //
            if(opts.honor_endflag) {
                for(i = 0; i < bpc; i += 0x10) {
                    if(
                        (block[i + 1] & 1) ||
                        (block_r && (block_r[i + 1] & 1))
                    ) {
                        endflag = 1;
                        bpc = i + 0x10;
                        break;
                    }
                }
            }

            if(opts.channels == 1) {
                for(i = 0; i < bpc; i += 0x10) {
                    decodeline(block + i, outbuf, p1, p2);
                    for(j = 0; j < 28; j++) {
                        outline[2 * j + 0] = (uint8_t)(outbuf[j]     );
                        outline[2 * j + 1] = (uint8_t)(outbuf[j] >> 8);
                    }
                    if(fwrite(outline, 1, 28 * 2, outf) != (28 * 2)) {
                        goto error_out;
                    }
                }
            } else {
                for(i = 0; i < bpc; i += 0x10) {
                    decodeline(block   + i, outbuf  , p1+0, p2+0);
                    decodeline(block_r + i, outbuf_r, p1+1, p2+1);
                    for(j = 0; j < 28; j++) {
                        outline[4 * j + 0] = (uint8_t)(outbuf  [j]     );
                        outline[4 * j + 1] = (uint8_t)(outbuf  [j] >> 8);
                        outline[4 * j + 2] = (uint8_t)(outbuf_r[j]     );
                        outline[4 * j + 3] = (uint8_t)(outbuf_r[j] >> 8);
                    }
                    if(fwrite(outline, 1, 28 * 2 * 2, outf) != (28 * 2 * 2)) {
                        goto error_out;
                    }
                }
            }

            b += bpc;
        }

        // if we couldn't process the whole block, stop here
        if(b < opts.interleave) { break; }

        // progress indicator
        if((out_indicator_pos >> 20) != (ftello(outf) >> 20)) {
            out_indicator_pos = ftello(outf);
            printf(".");
            fflush(stdout);
        }

        // seek to next block
        in_pos += (opts.channels * opts.interleave) + opts.skip;
    }

    printf("ok");

    goto success;

error_in:  printfileerror(inf , NULL   ); goto error;
error_out: printfileerror(outf, outname); goto error;
error:   returncode = 1; goto end;
success: returncode = 0; goto end;
end:
    if(outf) { wav_close(outf); }
    if(outname) { free(outname); }
    if(block  ) { free(block  ); }
    if(block_r) { free(block_r); }

    if(inf) { fseeko(inf, in_pos, SEEK_SET); }

    printf("\n");
    fflush(stdout);
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////
//
// Format: 8
//
// Used by: Suikoden
//
////////////////////////////////////////////////////////////////////////////////

static int fmt_8_detect(FILE *inf, const char* ext) {
    int score = 50;

    // base the entire test on the file extension for now
    (void)inf;
    if(!cmpcase(ext, "8")) { return 90; }

    return score;
}

static int fmt_8_rip(
    FILE *inf,
    struct vb2rip_options opts
) {
    // set .8-specific options
    if(!opts.use_channels) {
        opts.use_channels = 1;
        opts.channels = 2;
    }
    if(!opts.use_honor_endflag) {
        opts.use_honor_endflag = 1;
        opts.honor_endflag = 1;
    }
    if(!opts.use_samplerate) {
        opts.use_samplerate = 1;
        opts.samplerate = 44100;
    }
    if(!opts.use_interleave) {
        opts.use_interleave = 1;
        opts.interleave = 0x4000;
    }

    // now pass these options to rip
    return rip(inf, opts, NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Format: MSA
//
// Used by: Psyvariar Complete Edition, maybe other PS2 games.
//
////////////////////////////////////////////////////////////////////////////////

static int fmt_msa_detect(FILE *inf, const char* ext) {
    int score = 50;

    // base the entire test on the file extension for now
    (void)inf;
    if(!cmpcase(ext, "msa")) { return 90; }

    return score;
}

static int fmt_msa_rip(
    FILE *inf,
    struct vb2rip_options opts
) {
    uint8_t hdr[0x14];

    if(opts.use_offset) {
        if(fseeko(inf, opts.offset, SEEK_CUR) != 0) { goto error_in; }
    }
    opts.use_offset = 0;

    if(fread(hdr, 1, 0x14, inf) != 0x14) { goto error_in; }

    if(!opts.use_channels) {
        opts.use_channels = 1;
        opts.channels = 2;
    }
    if(!opts.use_honor_endflag) {
        opts.use_honor_endflag = 1;
        opts.honor_endflag = 1;
    }
    if(!opts.use_samplerate) {
        opts.use_samplerate = 1;
        opts.samplerate = get32lsb(hdr + 0x10);
        if(opts.samplerate < 8000 || opts.samplerate > 48000) {
            opts.samplerate = 44100;
        }
    }
    if(!opts.use_interleave) {
        opts.use_interleave = 1;
        opts.interleave = 0x4000;
    }

    return rip(inf, opts, NULL);

error_in:
    printfileerror(inf, NULL);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Format: Raw
//
// Used for experimentation.
//
////////////////////////////////////////////////////////////////////////////////

static int fmt_raw_detect(FILE *inf, const char* ext) {
    int score = 50;
    (void)inf;
    (void)ext;
    // can never be quite sure if a file is "raw" or not
    return score;
}

static int fmt_raw_rip(FILE *inf, struct vb2rip_options opts) {
    // just pass these options directly to rip
    return rip(inf, opts, NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Format: VB2
//
// Used by: DDR, Castlevania Chronicles, other Konami games
//
////////////////////////////////////////////////////////////////////////////////

static int fmt_vb2_detect(FILE *inf, const char* ext) {
    int score = 50;

    (void)inf;

    // if it ends in vb2, we're pretty sure
    if(!cmpcase(ext, "vb2")) { return 90; }

    // otherwise, it's not so clear - would have to search around for the
    // start of the data

    return score;
}

//
// Tell if a block of bytes is all zero
//
static int iszeroes(const uint8_t *buf, uint32_t n) {
    uint32_t i;
    for(i = 0; i < n; i++) {
        if(buf[i]) { return 0; }
    }
    return 1;
}

//
// Detect the beginning of a vb2 music block
// Returns the interleave size if successful
// Returns 0 if no beginning was detected
//
static off_t blockdetect_vb2(FILE* f, off_t ofs, off_t size) {
    uint8_t buf0[0x20];
    uint8_t buf2[0x20];
    uint8_t buf4[0x20];
    uint8_t buf8[0x20];
    if(size >= 0x2000) {
        if(fseeko(f, ofs + 0x0000, SEEK_SET) != 0) { return 0; }
        if(fread(buf0, 1, 0x20, f) != 0x20) { return 0; }
    }
    if(size >= 0x4000) {
        if(fseeko(f, ofs + 0x2000, SEEK_SET) != 0) { return 0; }
        if(fread(buf2, 1, 0x20, f) != 0x20) { return 0; }
    }
    if(size >= 0x8000) {
        if(fseeko(f, ofs + 0x4000, SEEK_SET) != 0) { return 0; }
        if(fread(buf4, 1, 0x20, f) != 0x20) { return 0; }
    }
    if(size >= 0x10000) {
        if(fseeko(f, ofs + 0x8000, SEEK_SET) != 0) { return 0; }
        if(fread(buf8, 1, 0x20, f) != 0x20) { return 0; }
    }
    //
    // DDR, various versions
    //
    if(size >= 0x8000) {
        if(
            (buf0[0x01] == 0x00) &&
            (buf0[0x11] == 0x06) &&
            (buf4[0x01] == 0x00) &&
            (buf4[0x11] == 0x06)
        ) { return 0x4000; }
    }
    //
    // DDRMAX2
    //
    if(size >= 0x4000) {
        if(
            iszeroes(buf0 + 0x00, 0x10) &&
            ((buf0[0x11] == 0x06) || (buf0[0x11] == 0x04)) &&
            iszeroes(buf2 + 0x00, 0x10) &&
            ((buf2[0x11] == 0x06) || (buf2[0x11] == 0x04))
        ) { return 0x2000; }
    }
    //
    // Castlevania Chronicles (J)
    //
    if(size >= 0x8000) {
        if(
            ((buf0[0x00] == 0x00) && ( iszeroes(buf0 + 0x02, 0x0E))) &&
            ((buf4[0x00] == 0x00) && ( iszeroes(buf4 + 0x02, 0x0E))) &&
            ((buf0[0x10] != 0x00) || (!iszeroes(buf0 + 0x12, 0x0E))) &&
            ((buf4[0x10] != 0x00) || (!iszeroes(buf4 + 0x12, 0x0E))) &&
            ((buf0[0x11] == 0x02) || (buf0[0x11] == 0x06)) &&
            ((buf4[0x11] == 0x02) || (buf4[0x11] == 0x06)) &&
             (buf0[0x01] == buf4[0x01]) &&
             (buf0[0x11] == buf4[0x11])
        ) { return 0x4000; }
    }
    //
    // Castlevania Chronicles (US)
    //
    if(size >= 0x10000) {
        if(
            ((buf0[0x00] == 0x00) && ( iszeroes(buf0 + 0x02, 0x0E))) &&
            ((buf8[0x00] == 0x00) && ( iszeroes(buf8 + 0x02, 0x0E))) &&
            ((buf0[0x10] != 0x00) || (!iszeroes(buf0 + 0x12, 0x0E))) &&
            ((buf8[0x10] != 0x00) || (!iszeroes(buf8 + 0x12, 0x0E))) &&
            ((buf0[0x11] == 0x02) || (buf0[0x11] == 0x06)) &&
            ((buf8[0x11] == 0x02) || (buf8[0x11] == 0x06)) &&
             (buf0[0x01] == buf8[0x01]) &&
             (buf0[0x11] == buf8[0x11])
        ) { return 0x8000; }
    }

    // all tests failed
    return 0;
}

static int fmt_vb2_rip(
    FILE *inf,
    struct vb2rip_options opts
) {
    int errors = 0;
    off_t bytesskipped = 0;
    off_t startpos;
    off_t addpos;
    off_t in_size;
    off_t searchstartpos = -1;
    off_t lastsongendedat = 0;
    int songnum = 0;
    const char* orig_outname;
    char *outname = NULL;

    orig_outname = opts.outname;
    if(!orig_outname) orig_outname = "out";

    outname = malloc(strlen(orig_outname) + 20);
    if(!outname) { printf("%s", oom); goto error; }

    startpos = ftello(inf);
    if(startpos == -1) { goto error_in; }

    if(fseeko(inf, 0, SEEK_END) != 0) { goto error_in; }
    in_size = ftello(inf);
    if(in_size == -1) { goto error_in; }
    if(fseeko(inf, startpos, SEEK_SET) != 0) { goto error_in; }

    if(!opts.use_channels) {
        opts.use_channels = 1;
        opts.channels = 2;
    }
    if(!opts.use_honor_endflag) {
        opts.use_honor_endflag = 1;
        opts.honor_endflag = 1;
    }

    addpos = 0;
    while((startpos + addpos) < in_size) {
        off_t interleave = blockdetect_vb2(
            inf,
            startpos + addpos,
            in_size - (startpos + addpos)
        );
        if(!interleave) {
            if(addpos >= lastsongendedat) {
                if(searchstartpos < 0) {
                    searchstartpos = startpos + addpos;
                    printf("searching at 0x");
                    fprinthex(stdout, searchstartpos, 1);
                    printf("...");
                    fflush(stdout);
                }
                bytesskipped += 0x800;
            }
            addpos += 0x800;
            continue;
        }
        if(searchstartpos >= 0) {
            printf("skipped ");
            fprintdec(stdout, (startpos + addpos) - searchstartpos);
            printf(" bytes\n");
            searchstartpos = -1;
        }
        opts.use_interleave = 1;
        opts.interleave = interleave;

        sprintf(outname, "%s%04ld", orig_outname, (long)songnum);
        opts.outname = outname;
        songnum++;

        if(fseeko(inf, startpos + addpos, SEEK_SET) != 0) { goto error_in; }

        errors += rip(inf, opts, blockdetect_vb2);

        //
        // rather than advancing completely beyond this song, we should seek back
        // 2*interleave to make sure the last block isn't short/malformed.
        // this fixes DDRMAX/DDRMAX2
        //
        lastsongendedat = ((ftello(inf) - startpos) | 0x7FF) ^ 0x7FF;
        {   off_t newaddpos = lastsongendedat - 2 * interleave;
            if(newaddpos <= addpos) { newaddpos = addpos + 0x800; }
            addpos = newaddpos;
        }
    }

    if(searchstartpos >= 0) {
        printf("nothing found\n");
    }

    printf("total bytes skipped: ");
    fprintdec(stdout, bytesskipped);
    printf("\n");

    goto end;
error_in: printfileerror(inf, NULL); goto error;
error:
    errors++;
end:
    if(outname) free(outname);
    return errors;
}

////////////////////////////////////////////////////////////////////////////////
//
// Format: XA2
//
// Used by: Extreme-G 3, possibly other PS2
//
////////////////////////////////////////////////////////////////////////////////

static int fmt_xa2_detect(FILE *inf, const char* ext) {
    int score = 50;
    (void)inf;
    // base the entire test on the file extension for now
    if(!cmpcase(ext, "xa2")) { return 90; }
    return score;
}

static int fmt_xa2_rip(
    FILE *inf,
    struct vb2rip_options opts
) {
    int errors = 0;
    static uint8_t hdr[0x800]; // TODO: not very clean
    uint32_t streams;
    off_t interleave;
    uint32_t sn;
    off_t startpos;
    const char* orig_outname;

    orig_outname = opts.outname;
    if(!orig_outname) { orig_outname = "out"; }

    // get the header
    memset(hdr, 0, sizeof(hdr));
    if(fread(hdr, 1, 0x800, inf) != 0x800) { goto error_in; }
    startpos = ftello(inf);
    if(startpos == -1) { goto error_in; }

    streams    = get32lsb(hdr);
    interleave = get32lsb(hdr + 4);
    if(streams < 1) {
        printf("invalid stream count present: %ld\n", (long)streams);
        return 1;
    }
    if(
        (interleave <= 0) ||
        (interleave & (interleave - 1))
    ) {
        printf("invalid interleave value present: 0x");
        fprinthex(stdout, interleave, 1);
        printf("\n");
        return 1;
    }

    if(!opts.use_channels) {
        opts.use_channels = 1;
        opts.channels = (streams >= 2) ? 2 : 1;
    }
    if(!opts.use_honor_endflag) {
        opts.use_honor_endflag = 1;
        opts.honor_endflag = 1;
    }
    if(!opts.use_samplerate) {
        opts.use_samplerate = 1;
        opts.samplerate = 44100;
    }
    if(!opts.use_interleave) {
        opts.use_interleave = 1;
        opts.interleave = interleave;
    }

    for(sn = 0; sn < streams; sn += opts.channels) {
        char *outname = malloc(strlen(orig_outname) + 20);
        if(!outname) { printf("%s", oom); errors++; break; }
        sprintf(outname, "%s%ld", orig_outname, (long)sn);

        opts.outname = outname;

        // if there's only one stream left, rip it as a mono
        if(sn == (streams - 1)) { opts.channels = 1; }

        // seek to the appropriate spot
        if(fseeko(inf, startpos + sn * opts.interleave, SEEK_SET) != 0) {
            goto error_in;
        }

        // set the appropriate skip value to skip over the other streams
        opts.use_skip = 1;
        opts.skip = (streams - opts.channels) * opts.interleave;

        errors += rip(inf, opts, NULL);

        free(outname);
    }

    return errors;

error_in:
    printfileerror(inf, NULL);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Table of recognized formats
//

typedef int (*fmt_rip_t)(FILE *inf, struct vb2rip_options opts);
typedef int (*fmt_detect_t)(FILE *inf, const char* ext);

struct FMT_TABLE {
    const char* name;
    fmt_rip_t rip;
    fmt_detect_t detect;
    const char* desc;
};

static const struct FMT_TABLE fmt_table[] = {
    { "raw", fmt_raw_rip, fmt_raw_detect, "Raw data (for experimentation)" },
    { "vb2", fmt_vb2_rip, fmt_vb2_detect, "Konami multi-song .BIN/.VB2 file (Dance Dance Revolution, etc.)" },
    { "8"  , fmt_8_rip  , fmt_8_detect  , ".8 file (Suikoden)" },
    { "msa", fmt_msa_rip, fmt_msa_detect, ".MSA file (Psyvariar, possibly other PS2 games)" },
    { "xa2", fmt_xa2_rip, fmt_xa2_detect, ".XA2 file (Extreme-G 3, possibly other PS2 games)" },
    { NULL, NULL, NULL, NULL }
};

////////////////////////////////////////////////////////////////////////////////

static void fmt_printtable(void) {
    const struct FMT_TABLE *t;
    printf("  Format   Description\n");
    printf("  ------------------------------------------------------------\n");
    for(t = fmt_table; t->name; t++) {
        printf("  %-9s%s\n", t->name, t->desc);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// determine if a given format name is recognized
//
static int fmt_isvalid(const char* fmt) {
    const struct FMT_TABLE *t;
    for(t = fmt_table; t->name; t++) {
        if(!strcmp(t->name, fmt)) return 1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// detect the format of a file if none was given
// (must return at least some valid string pointer)
//
static const char* fmt_detect(FILE *inf, const char* ext) {
    const struct FMT_TABLE *t;
    const char* fmt_highest = "raw";
    int fmt_highest_score = 50;
    for(t = fmt_table; t->name; t++) {
        if(t->detect) {
            int score = t->detect(inf, ext);
            if(score > fmt_highest_score) {
                fmt_highest = t->name;
                fmt_highest_score = score;
            }
        }
    }
    return fmt_highest;
}

////////////////////////////////////////////////////////////////////////////////
//
// rip the actual music from a file
// returns the number of errors that occurred
// fmt and outname must be valid in opts
//
static int fmt_rip(FILE *inf, struct vb2rip_options opts) {
    const struct FMT_TABLE *t;

    if(!opts.fmt) { opts.fmt = "raw"; }
    if(!opts.outname) { opts.outname = "out"; }

    if(opts.use_offset) {
        if(fseeko(inf, opts.offset, SEEK_CUR) != 0) {
            goto error_in;
        }
        opts.use_offset = 0;
        opts.offset = 0;
    }

    for(t = fmt_table; t->name; t++) {
        if(!strcmp(t->name, opts.fmt)) {
            return t->rip(inf, opts);
        }
    }

    printf("unknown format '%s'\n", opts.fmt);
    return 1;

error_in:
    printfileerror(inf, NULL);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

void usage(const char* progname) {
    banner();
    printf("Usage: %s [options] inputfile(s)\n\n", progname);
    printf(
        "Options:\n"
        "  -fmt format     Specify the input file format\n"
        "  -o path         Specify the output filename (if one input file is given)\n"
        "                    or specify the output directory (if several are given)\n"
        "  -mono           Treat input file as monaural\n"
        "  -stereo         Treat input file as stereo\n"
        "  -rate n         Specify the sample rate\n"
        "  -interleave n   Specify the block interleave size\n"
        "  -skip n         Skip this many bytes after each block\n"
        "  -offset n       Start at the given offset in the input file\n"
        "  -endflag        Stop decoding when the sample end flag is reached\n"
        "  -noendflag      Ignore the sample end flag\n"
        "  -maxbytes n     Set the maximum number of input bytes to decode\n"
        "\n"
    );
    printf("Supported input formats:\n");
    fmt_printtable();
}

////////////////////////////////////////////////////////////////////////////////
//
// returns how many argv spots were used
// returns 0 on error
// if opts is NULL, there are no side effects
//
static int decodeoption(
    struct vb2rip_options* opts,
    int argc,
    char **argv
) {
    if(argc < 1) return 0;
    if(!strcmp(argv[0], "-fmt")) {
        if(argc < 2) { goto needextra; }
        if(!fmt_isvalid(argv[1])) {
            printf("unknown format '%s'\n", argv[1]);
            return 0;
        }
        opts->fmt = argv[1];
        return 2;
    } else if(!strcmp(argv[0], "-o")) {
        if(argc < 2) { goto needextra; }
        opts->outname = argv[1];
        return 2;
    } else if(!strcmp(argv[0], "-mono")) {
        opts->use_channels = 1;
        opts->channels = 1;
        return 1;
    } else if(!strcmp(argv[0], "-stereo")) {
        opts->use_channels = 1;
        opts->channels = 2;
        return 1;
    } else if(!strcmp(argv[0], "-rate")) {
        if(argc < 2) { goto needextra; }
        opts->use_samplerate = 1;
        opts->samplerate = strtoul(argv[1], NULL, 0);
        return 2;
    } else if(!strcmp(argv[0], "-interleave")) {
        if(argc < 2) { goto needextra; }
        opts->use_interleave = 1;
        opts->interleave = strtoofft(argv[1], NULL, 0);
        return 2;
    } else if(!strcmp(argv[0], "-skip")) {
        if(argc < 2) { goto needextra; }
        opts->use_skip = 1;
        opts->skip = strtoofft(argv[1], NULL, 0);
        return 2;
    } else if(!strcmp(argv[0], "-offset")) {
        if(argc < 2) { goto needextra; }
        opts->use_offset = 1;
        opts->offset = strtoofft(argv[1], NULL, 0);
        return 2;
    } else if(!strcmp(argv[0], "-noendflag")) {
        opts->use_honor_endflag = 1;
        opts->honor_endflag = 0;
        return 1;
    } else if(!strcmp(argv[0], "-endflag")) {
        opts->use_honor_endflag = 1;
        opts->honor_endflag = 1;
        return 1;
    } else if(!strcmp(argv[0], "-maxbytes")) {
        if(argc < 2) { goto needextra; }
        opts->use_maxbytes = 1;
        opts->maxbytes = strtoofft(argv[1], NULL, 0);
        return 2;
    }
    printf("unknown option '%s'\n", argv[0]);
    return 0;
needextra:
    printf("option '%s' needs an extra parameter\n", argv[0]);
    return 0;
}

//
// returns nonzero on error
//
static int getoptions(
    struct vb2rip_options* opts,
    int argc,
    char **argv
) {
    int endopts = 0;
    int i;
    for(i = 1; i < argc; i++) {
        if(endopts || (argv[i][0] != '-')) {
            opts->inputfilecount++;
        } else if(!strcmp(argv[i], "--")) {
            endopts = 1;
        } else {
            int advance = decodeoption(opts, argc - i, argv + i);
            if(advance < 1) return 1;
            i += (advance - 1);
        }
    }
    // success
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// returns the number of errors that occurred
//
static int processfile(const char* infilename, struct vb2rip_options opts) {
    int returncode = 1;
    const char* fmt;
    FILE* f = NULL;
    char* innameonly = NULL;
    char* outname = NULL;
    char separator = '/';

    if(!infilename) { printf("no input filename given\n"); goto error; }

    // allocate memory for filename strings
    innameonly = malloc(strlen(infilename) + 1);
    if(!innameonly) { printf("%s", oom); goto error; }
    {   int maxoutl = strlen(infilename);
        if(opts.outname) { maxoutl += strlen(opts.outname); }
        maxoutl += 10;
        outname = malloc(maxoutl);
        if(!outname) { printf("%s", oom); goto error; }
    }

    // count directory separators in both input and output names
    {   const char* t;
        int sepunix = 0;
        int sepdos  = 0;
        if(infilename) {
            for(t = infilename; *t; t++) {
                switch(*t) {
                case '/' : sepunix++; break;
                case '\\': sepdos++; break;
                }
            }
        }
        if(opts.outname) {
            for(t = opts.outname; *t; t++) {
                switch(*t) {
                case '/' : sepunix++; break;
                case '\\': sepdos++; break;
                }
            }
        }
        // if one beats the other, use it; otherwise leave at the default
        if(sepunix > sepdos ) { separator = '/'; }
        if(sepdos  > sepunix) { separator = '\\'; }
    }

    // strip the input filename to its name only
    {   const char* t, *base;
        base = infilename;
        for(t = infilename; *t; t++) {
            switch(*t) {
            case '/' : case '\\': base = t + 1; break;
            }
        }
        strcpy(innameonly, base);
        { char *p = strrchr(innameonly, '.');
            if(p) *p = 0;
        }
    }

    // if no output name was specified at all, then we use innameonly
    if(!opts.outname) {
        strcpy(outname, innameonly);
    // if we only have one input file, we use outname directly
    } else if(opts.inputfilecount == 1) {
        strcpy(outname, opts.outname);
    // otherwise, outname is actually a path to which we'll add innameonly
    } else {
        int l = strlen(opts.outname);
        strcpy(outname, opts.outname);
        if(l > 0) {
            char c = outname[l - 1];
            if(c != '/' && c != '\\') {
                outname[l] = separator;
                outname[l + 1] = 0;
            }
        }
        strcat(outname, innameonly);
    }

    // now we actually open the input file
    printf("reading %s: ", infilename);
    fflush(stdout);

    f = fopen(infilename, "rb");
    if(!f) { goto error_in; }

    // determine the format if necessary, or use the override
    fmt = opts.fmt;
    if(opts.use_offset) {
        if(fseeko(f, opts.offset, SEEK_SET) != 0) { goto error_in; }
        opts.use_offset = 0;
        opts.offset = 0;
    }
    if(fmt) {
        printf("(format set to: %s)", fmt);
    } else {
        const char* ext = strrchr(infilename, '.');
        if(ext) { ext++; }
        else { ext = ""; }
        fmt = fmt_detect(f, ext);
        printf("(format detected as: %s)", fmt);
    }

    // set fmt and outname in the options for fmt_rip
    opts.fmt = fmt;
    opts.outname = outname;

    printf("\n");

    // call fmt_rip to do the actual ripping work
    if(fmt_rip(f, opts)) { goto error; }
    goto success;

error_in: printfileerror(f, infilename); goto error;
error:   returncode = 1; goto end;
success: returncode = 0; goto end;
end:
    if(f) { fclose(f); }
    if(innameonly) { free(innameonly); }
    if(outname) { free(outname); }

    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    struct vb2rip_options opts;
    const char* progname = argv[0];
    int i;
    int endopts = 0;
    int errors = 0;

    // get options from command line
    memset(&opts, 0, sizeof(opts));
    if(getoptions(&opts, argc, argv)) return 1;

    // make sure at least one input file was specified
    if((opts.inputfilecount) < 1) {
        usage(progname);
        return 1;
    }

    // now process each file
    endopts = 0;
    for(i = 1; i < argc; i++) {
        if(endopts || (argv[i][0] != '-')) {
            errors += processfile(argv[i], opts);
        } else if(!strcmp(argv[i], "--")) {
            endopts = 1;
        } else {
            struct vb2rip_options dummy;
            int advance = decodeoption(&dummy, argc - i, argv + i);
            if(advance < 1) return 1;
            i += (advance - 1);
        }
    }

    return errors;
}

////////////////////////////////////////////////////////////////////////////////

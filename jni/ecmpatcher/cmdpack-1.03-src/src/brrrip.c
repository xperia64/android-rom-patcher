////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "brrrip - Rip SNES BRR sound samples"
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

static void set32lsb(uint8_t* dest, uint32_t n) {
    dest[0] = (uint8_t)(n >>  0);
    dest[1] = (uint8_t)(n >>  8);
    dest[2] = (uint8_t)(n >> 16);
    dest[3] = (uint8_t)(n >> 24);
}

////////////////////////////////////////////////////////////////////////////////

static int writewav(
    FILE* src,
    const char* srcfilename,
    off_t startpos,
    off_t endpos,
    off_t samplerate
) {
    int returncode = 0;
    static uint8_t wavhdr[] = {
        0x52,0x49,0x46,0x46,0x16,0x63,0x01,0x00,
        0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
        0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,
        0x44,0xAC,0x00,0x00,0x88,0x58,0x01,0x00,
        0x02,0x00,0x10,0x00,0x64,0x61,0x74,0x61,
        0x40,0x62,0x01,0x00
    };
    FILE *wav = NULL;
    char wavfilename[24];
    int32_t p1 = 0;
    int32_t p2 = 0;

    if(fseeko(src, startpos, SEEK_SET) != 0) { goto error_src; }

    sprintf(wavfilename, "%06lx.wav", (unsigned long)((uint32_t)(startpos)));

    fprinthex(stdout, startpos, 6);
    printf("..");
    fprinthex(stdout, endpos  , 6);
    printf(" -> %s: ", wavfilename);
    fflush(stdout);

    wav = fopen(wavfilename, "wb");
    if(!wav) { goto error_wav; }

    set32lsb(wavhdr + 0x04, (uint32_t)(0x24+32*((endpos - startpos) / 9)));
    set32lsb(wavhdr + 0x28, (uint32_t)(     32*((endpos - startpos) / 9)));
    set32lsb(wavhdr + 0x18, (uint32_t)(samplerate    ));
    set32lsb(wavhdr + 0x1C, (uint32_t)(samplerate * 2));
    if(fwrite(wavhdr, 1, 0x2C, wav) != 0x2C) { goto error_wav; }

    for(; startpos < endpos; startpos += 9) {
        static const int8_t coef1[4] = { -128, -68,  -6, -13 };
        static const int8_t coef2[4] = {    0,   0, -15, -13 };
        int32_t c1, c2;
        uint8_t block[9];
        uint8_t shift;
        size_t s = fread(block, 1, 9, src);
        if(s != 9) { goto error_src; }
        shift = block[0] >> 4;
        c1 = coef1[(block[0] >> 2) & 3];
        c2 = coef2[(block[0] >> 2) & 3];
        for(s = 0; s < 16; s++) {
            int32_t sample = block[1 + (s >> 1)];
            sample <<= 24 + 4 * (s & 1);
            sample >>= 28;
            if(shift <= 12) {
                sample <<= shift;
                sample >>= 1;
            } else {
                sample &= -0x800;
            }
            sample += (p1 << 1) + ((c1 * p1) >> 6);
            sample += (c2 * p2) >> 4;
            if(sample < -32768) { sample = -32768; }
            if(sample >  32767) { sample =  32767; }
            fputc((sample     ) & 0xFF, wav);
            fputc((sample >> 8) & 0xFF, wav);
            p2 = p1;
            p1 = sample;
        }
    }

    printf("ok\n");
    goto done;

error_wav: printfileerror(wav, wavfilename); goto error;
error_src: printfileerror(src, srcfilename); goto error;
error:
    returncode = 1;
    goto done;
done:
    if(wav) { fclose(wav); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

static int isvalidblock(const uint8_t* block) {
    if(block[0] >= 0xD0) { return 0; }
    if( (!block[1]) && (!block[2]) && (!block[3]) && (!block[4]) &&
        (!block[5]) && (!block[6]) && (!block[7]) && (!block[8])
    ) { return 0; }
    return 1;
}

////////////////////////////////////////////////////////////////////////////////

static int analyze(
    FILE* f,
    const char* filename,
    off_t* nsamples,
    off_t samplerate,
    off_t minblocks
) {
    int returncode = 0;

    clearerr(f);

    for(;;) {
        uint8_t block[9];
        size_t s;
        off_t startpos, endpos;

        startpos = ftello(f);
        if(startpos == -1) { goto error_f; }

        //
        // Find a sample beginning
        //
        for(;; startpos += 9) {
            s = fread(block, 1, 9, f);
            if(s != 9) {
                if(ferror(f)) { goto error_f; }
                goto done;
            }
            if(!isvalidblock(block)) { continue; }
            break;
        }

        endpos = ftello(f);
        if(endpos == -1) { goto error_f; }

        //
        // Find a sample end
        //
        for(;; endpos += 9) {
            s = fread(block, 1, 9, f);
            if(s != 9) {
                if(ferror(f)) { goto error_f; }
                break;
            }
            if(!isvalidblock(block)) { break; } // end before this block
            if(block[0] & 1) { endpos += 9; break; } // end after this block
        }

        //
        // If the sample is long enough, write it out
        //
        if(((endpos - startpos) / 9) >= minblocks) {
            if(writewav(f, filename, startpos, endpos, samplerate)) {
                goto error;
            }
            (*nsamples)++;
        }

        if(fseeko(f, endpos, SEEK_SET) != 0) { goto error_f; }
    }

error_f:
    printfileerror(f, filename);
    goto error;
error:
    returncode = 1;
    goto done;
done:
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int returncode = 0;
    off_t samplerate = 16000;
    off_t minblocks = 50;
    off_t nsamples = 0;
    FILE *f = NULL;
    int i;

    normalize_argv0(argv[0]);

    if(argc <  2) { goto usage; }
    if(argc >= 3) { samplerate = strtoofft(argv[2], NULL, 0); }
    if(argc >= 4) { minblocks  = strtoofft(argv[3], NULL, 0); }
    if(argc >= 5) { goto usage; }

    f = fopen(argv[1], "rb");
    if(!f) { goto error_f; }

    for(i = 0; i < 9; i++) {
        printf("Pass %d/9:\n", (int)(i + 1));
        if(fseeko(f, i, SEEK_SET) != 0) { goto error_f; }
        if(analyze(f, argv[1], &nsamples, samplerate, minblocks)) {
            goto error;
        }
    }

    goto done;
usage:
    banner();
    printf("Usage: %s romfile [samplerate [minblocks]]\n", argv[0]);
    goto error;
error_f:
    printfileerror(f, argv[1]);
error:
    returncode = 1;
    goto done;
done:
    if(nsamples) {
        printf("Ripped ");
        fprintdec(stdout, nsamples);
        printf(" sample%s\n", nsamples != 1 ? "s" : "");
    }
    if(f) { fclose(f); }
    return returncode;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "hax65816 - Simple 65816 disassembler"
#define COPYR "Copyright (C) 1998,2010 Neill Corlett"
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

static char aluop[8][4] =
{"ora", "and", "eor", "adc", "sta", "lda", "cmp", "sbc" };
static char rmwop[8][4] =
{"asl", "rol", "lsr", "ror", "???", "???", "dec", "inc" };

static int8_t mflag_default = 1;
static int8_t xflag_default = 0;
static int8_t flag_return = 1;
static int8_t flag_guess  = 1;
static int8_t flag_follow = 1;

static uint32_t dasm_address, ins_address;
static int8_t mflag, xflag;

static int getbyte(void);

////////////////////////////////////////////////////////////////////////////////

static int fetchlist[10];
static int unfetchstack[10];
static int fetchn, unfetchn;

static int fetchbyte(void) {
    int b;
    dasm_address++;
    if(unfetchn) b = unfetchstack[--unfetchn]; else b = getbyte();
    fetchlist[fetchn++] = b;
    return b;
}

static void unfetchbyte(int b) {
    dasm_address--;
    fetchn--;
    if(fetchn < 0) fetchn = 0;
    unfetchstack[unfetchn++] = b;
}

enum { INS_COLUMN = 23 };

static void ins(const char* fmt, ...) {
    static char outputstr[100];
    int outputn = INS_COLUMN;
    va_list ap;
    va_start(ap, fmt);
    for(;;) {
        char c = *fmt++;
        if(!c) break;
        if(c == '%') {
            c = *fmt++;
            switch(c) {
            case '%': {
                outputstr[outputn++] = '%';
                break; }
            case 'A': {
                int n = (va_arg(ap, int) >> 5) & 7;
                outputstr[outputn++] = aluop[n][0];
                outputstr[outputn++] = aluop[n][1];
                outputstr[outputn++] = aluop[n][2];
                break; }
            case 'M': {
                int n = (va_arg(ap, int) >> 5) & 7;
                outputstr[outputn++] = rmwop[n][0];
                outputstr[outputn++] = rmwop[n][1];
                outputstr[outputn++] = rmwop[n][2];
                break; }
            case 'B': {
                int n = fetchbyte();
                outputstr[outputn++] = '$';
                if(n < 0) {
                    outputstr[outputn++] = '-';
                    outputstr[outputn++] = '-';
                } else {
                    sprintf(
                        outputstr + outputn,
                        "%02lX", (unsigned long)n
                    );
                    outputn += 2;
                }
                break; }
            case 'R': {
                int n = fetchbyte();
                outputstr[outputn++] = '$';
                if(n < 0) {
                    outputstr[outputn++] = '-';
                    outputstr[outputn++] = '-';
                    outputstr[outputn++] = '-';
                    outputstr[outputn++] = '-';
                } else {
                    n = ((int8_t)(n)) +
                        dasm_address;
                    sprintf(
                        outputstr + outputn,
                        "%04lX", (unsigned long)(n & 0xFFFF)
                    );
                    outputn += 4;
                }
                break; }
            case 'Z': {
                int n1 = fetchbyte();
                int n2 = fetchbyte();
                outputstr[outputn++] = '$';
                if((n1 < 0) || (n2 < 0)) {
                    for(n1 = 0; n1 < 6; n1++) {
                        outputstr[outputn++] = '-';
                    }
                } else {
                    int n = (n2 << 8) | (n1 & 0xFF);
                    n = ((int16_t)(n)) +
                        dasm_address;
                    sprintf(
                        outputstr + outputn,
                        "%04lX", (unsigned long)(n & 0xFFFF)
                    );
                    outputn += 6;
                }
                break; }
            case 'G': {
                int n = va_arg(ap, int);
                outputstr[outputn++] = '$';
                if(n < 0) {
                    outputstr[outputn++] = '-';
                    outputstr[outputn++] = '-';
                } else {
                    sprintf(
                        outputstr + outputn,
                        "%02lX", (unsigned long)(n)
                    );
                    outputn += 2;
                }
                break; }
            case 'W': {
                int q, n[2];
                n[1] = fetchbyte();
                n[0] = fetchbyte();
                outputstr[outputn++] = '$';
                for(q = 0; q < 2; q++) {
                    if(n[q] < 0) {
                        outputstr[outputn++] = '-';
                        outputstr[outputn++] = '-';
                    } else {
                        sprintf(
                            outputstr + outputn,
                            "%02lX", (unsigned long)(n[q])
                        );
                        outputn += 2;
                    }
                }
                break; }
            case 'L': {
                int q, n[3];
                n[2] = fetchbyte();
                n[1] = fetchbyte();
                n[0] = fetchbyte();
                outputstr[outputn++] = '$';
                for(q = 0; q < 3; q++) {
                    if(n[q] < 0) {
                        outputstr[outputn++] = '-';
                        outputstr[outputn++] = '-';
                    } else {
                        sprintf(
                            outputstr + outputn,
                            "%02lX", (unsigned long)(n[q])
                        );
                        outputn += 2;
                    }
                }
                break; }
            case 'I': case 'X': {
                int q, n[3];
                int8_t* sflag = (c == 'I') ? (&mflag) : (&xflag);
                outputstr[outputn++] = '$';
                if(*sflag) {
                    n[0] = fetchbyte();
                    n[1] = fetchbyte();
                    //
                    // BRK/COP/WDM/STP after this?
                    //
                    switch(n[1]) {
                    case 0x00: case 0x02:
                    case 0x42: case 0xDB:
                        if(flag_follow && flag_guess
                        ) {
                            (*sflag) = 0;
                            q = 2;
                            break;
                        }
                    default:
                        unfetchbyte(n[1]);
                        q = 1;
                        break;
                    }
                } else {
                    n[0] = fetchbyte();
                    n[1] = fetchbyte();
                    n[2] = fetchbyte();
                    //
                    // BRK/COP/WDM/STP after this?
                    //
                    switch(n[2]) {
                    case 0x00: case 0x02:
                    case 0x42: case 0xDB:
                        if(flag_follow && flag_guess
                        ) {
                            (*sflag) = 1;
                            unfetchbyte(n[2]);
                            unfetchbyte(n[1]);
                            q = 1;
                            break;
                        }
                    default:
                        unfetchbyte(n[2]);
                        q = 2;
                        break;
                    }
                }
                while(q--) {
                    if(n[q] < 0) {
                        outputstr[outputn++] = '-';
                        outputstr[outputn++] = '-';
                    } else {
                        sprintf(
                            outputstr + outputn,
                            "%02lX", (unsigned long)(n[q])
                        );
                        outputn += 2;
                    }
                }
                break; }
            }
        } else {
            outputstr[outputn++] = c;
        }
    }
    outputstr[outputn] = 0;
    sprintf(outputstr, "%02lX/%04lX:",
        (unsigned long)(ins_address >> 16),
        (unsigned long)(ins_address & 0xFFFF)
    );
    memset(outputstr + 8, ' ', INS_COLUMN - 8);

    outputn = 9;
    {   int i;
        for(i = 0; i < fetchn; i++) {
            int n = fetchlist[i];
            if(n < 0) {
                outputstr[outputn  ] = '-';
                outputstr[outputn+1] = '-';
            } else {
                sprintf(outputstr + outputn, "%02X", n);
                outputstr[outputn+2] = ' ';
            }
            outputn += 3;
        }
    }
    fetchn = 0;
    printf("%s\n", outputstr);

    va_end(ap);
}

static void disassemble_one(void) {
    int opcode;
    ins_address = dasm_address;
    opcode = fetchbyte();
    if(opcode < 0) return;

    if       (opcode == 0x00) { ins("brk %B");
    } else if(opcode == 0x20) { ins("jsr %W");
    } else if(opcode == 0x40) { ins("rti"); if(flag_return) { mflag = mflag_default; xflag = xflag_default; }
    } else if(opcode == 0x60) { ins("rts"); if(flag_return) { mflag = mflag_default; xflag = xflag_default; }
    } else if(opcode == 0x80) { ins("bra %R");
    } else if(opcode == 0xA0) { ins("ldy #%X");
    } else if(opcode == 0xC0) { ins("cpy #%X");
    } else if(opcode == 0xE0) { ins("cpx #%X");
    } else if(opcode == 0x10) { ins("bpl %R");
    } else if(opcode == 0x30) { ins("bmi %R");
    } else if(opcode == 0x50) { ins("bvc %R");
    } else if(opcode == 0x70) { ins("bvs %R");
    } else if(opcode == 0x90) { ins("bcc %R");
    } else if(opcode == 0xB0) { ins("bcs %R");
    } else if(opcode == 0xD0) { ins("bne %R");
    } else if(opcode == 0xF0) { ins("beq %R");

    } else if((opcode & 0x1F) == 0x01) { ins("%A (%B,x)"  , opcode);
    } else if((opcode & 0x1F) == 0x11) { ins("%A (%B),y"  , opcode);

    } else if(opcode == 0x02) { ins("cop %B");
    } else if(opcode == 0x22) { ins("jsr %L");
    } else if(opcode == 0x42) { ins("wdm %B");
    } else if(opcode == 0x62) { ins("per %Z");
    } else if(opcode == 0x82) { ins("brl %Z");
    } else if(opcode == 0xA2) { ins("ldx #%X");
    } else if(opcode == 0xC2) {
        int n = fetchbyte();
        if(flag_follow) if(n >= 0) {
            if(n & 0x10) xflag = 0;
            if(n & 0x20) mflag = 0;
        }
        ins("rep #%G", n);
    } else if(opcode == 0xE2) {
        int n = fetchbyte();
        if(flag_follow) if(n >= 0) {
            if(n & 0x10) xflag = 1;
            if(n & 0x20) mflag = 1;
        }
        ins("sep #%G", n);
    } else if((opcode & 0x1F) == 0x12) { ins("%A (%B)"    , opcode);

    } else if((opcode & 0x1F) == 0x03) { ins("%A %B,s"    , opcode);
    } else if((opcode & 0x1F) == 0x13) { ins("%A (%B,s),y", opcode);

    } else if(opcode == 0x04) { ins("tsb %B");
    } else if(opcode == 0x24) { ins("bit %B");
    } else if(opcode == 0x44) { ins("mvp %B,%B");
    } else if(opcode == 0x64) { ins("stz %B");
    } else if(opcode == 0x84) { ins("sty %B");
    } else if(opcode == 0xA4) { ins("ldy %B");
    } else if(opcode == 0xC4) { ins("cpy %B");
    } else if(opcode == 0xE4) { ins("cpx %B");
    } else if(opcode == 0x14) { ins("trb %B");
    } else if(opcode == 0x34) { ins("bit %B,x");
    } else if(opcode == 0x54) { ins("mvn %B,%B");
    } else if(opcode == 0x74) { ins("stz %B,x");
    } else if(opcode == 0x94) { ins("sty %B,x");
    } else if(opcode == 0xB4) { ins("ldy %B,x");
    } else if(opcode == 0xD4) { ins("pei (%B)");
    } else if(opcode == 0xF4) { ins("pea %W");

    } else if((opcode & 0x1F) == 0x05) { ins("%A %B"      , opcode);
    } else if((opcode & 0x1F) == 0x15) { ins("%A %B,x"    , opcode);

    } else if(((opcode & 0x1F) == 0x06) && ((opcode & 0xC0) != 0x80)) {
        ins("%M %B", opcode);
    } else if(opcode == 0x86) { ins("stx %B");
    } else if(opcode == 0xA6) { ins("ldx %B");
    } else if(((opcode & 0x1F) == 0x16) && ((opcode & 0xC0) != 0x80)) {
        ins("%M %B,x", opcode);
    } else if(opcode == 0x96) { ins("stx %B,y");
    } else if(opcode == 0xB6) { ins("ldx %B,y");

    } else if((opcode & 0x1F) == 0x07) { ins("%A [%B]"    , opcode);
    } else if((opcode & 0x1F) == 0x17) { ins("%A [%B],y"  , opcode);

    } else if(opcode == 0x08) { ins("php");
    } else if(opcode == 0x28) { ins("plp");
    } else if(opcode == 0x48) { ins("pha");
    } else if(opcode == 0x68) { ins("pla");
    } else if(opcode == 0x88) { ins("dey");
    } else if(opcode == 0xA8) { ins("tay");
    } else if(opcode == 0xC8) { ins("iny");
    } else if(opcode == 0xE8) { ins("inx");
    } else if(opcode == 0x18) { ins("clc");
    } else if(opcode == 0x38) { ins("sec");
    } else if(opcode == 0x58) { ins("cli");
    } else if(opcode == 0x78) { ins("sei");
    } else if(opcode == 0x98) { ins("tya");
    } else if(opcode == 0xB8) { ins("clv");
    } else if(opcode == 0xD8) { ins("cld");
    } else if(opcode == 0xF8) { ins("sed");

    } else if((opcode & 0x1F) == 0x09) {
        if(opcode == 0x89) ins("bit #%I");
        else ins("%A #%I", opcode);
    } else if((opcode & 0x1F) == 0x19) { ins("%A %W,y"    , opcode);

    } else if(opcode == 0x0A) { ins("asl");
    } else if(opcode == 0x2A) { ins("rol");
    } else if(opcode == 0x4A) { ins("lsr");
    } else if(opcode == 0x6A) { ins("ror");
    } else if(opcode == 0x8A) { ins("txa");
    } else if(opcode == 0xAA) { ins("tax");
    } else if(opcode == 0xCA) { ins("dex");
    } else if(opcode == 0xEA) { ins("nop");
    } else if(opcode == 0x1A) { ins("inc");
    } else if(opcode == 0x3A) { ins("dec");
    } else if(opcode == 0x5A) { ins("phy");
    } else if(opcode == 0x7A) { ins("ply");
    } else if(opcode == 0x9A) { ins("txs");
    } else if(opcode == 0xBA) { ins("tsx");
    } else if(opcode == 0xDA) { ins("phx");
    } else if(opcode == 0xFA) { ins("plx");

    } else if(opcode == 0x0B) { ins("phd");
    } else if(opcode == 0x2B) { ins("pld");
    } else if(opcode == 0x4B) { ins("phk");
    } else if(opcode == 0x6B) { ins("rtl"); if(flag_return) { mflag = mflag_default; xflag = xflag_default; }
    } else if(opcode == 0x8B) { ins("phb");
    } else if(opcode == 0xAB) { ins("plb");
    } else if(opcode == 0xCB) { ins("wai");
    } else if(opcode == 0xEB) { ins("xba");
    } else if(opcode == 0x1B) { ins("tcs");
    } else if(opcode == 0x3B) { ins("tsc");
    } else if(opcode == 0x5B) { ins("tcd");
    } else if(opcode == 0x7B) { ins("tdc");
    } else if(opcode == 0x9B) { ins("txy");
    } else if(opcode == 0xBB) { ins("tyx");
    } else if(opcode == 0xDB) { ins("stp");
    } else if(opcode == 0xFB) { ins("xce");

    } else if(opcode == 0x0C) { ins("tsb %W");
    } else if(opcode == 0x2C) { ins("bit %W");
    } else if(opcode == 0x4C) { ins("jmp %W");
    } else if(opcode == 0x6C) { ins("jmp (%W)");
    } else if(opcode == 0x8C) { ins("sty %W");
    } else if(opcode == 0xAC) { ins("ldy %W");
    } else if(opcode == 0xCC) { ins("cpy %W");
    } else if(opcode == 0xEC) { ins("cpx %W");
    } else if(opcode == 0x1C) { ins("trb %W");
    } else if(opcode == 0x3C) { ins("bit %W,x");
    } else if(opcode == 0x5C) { ins("jmp %L");
    } else if(opcode == 0x7C) { ins("jmp (%W,x)");
    } else if(opcode == 0x9C) { ins("stz %W");
    } else if(opcode == 0xBC) { ins("ldy %W,x");
    } else if(opcode == 0xDC) { ins("jmp [%W]");
    } else if(opcode == 0xFC) { ins("jsr (%W,x)");

    } else if((opcode & 0x1F) == 0x0D) { ins("%A %W"      , opcode);
    } else if((opcode & 0x1F) == 0x1D) { ins("%A %W,x"    , opcode);

    } else if(((opcode & 0x1F) == 0x0E) && ((opcode & 0xC0) != 0x80)) {
        ins("%M %W", opcode);
    } else if(opcode == 0x8E) { ins("stx %W");
    } else if(opcode == 0xAE) { ins("ldx %W");
    } else if(((opcode & 0x1F) == 0x1E) && ((opcode & 0xC0) != 0x80)) {
        ins("%M %W,x", opcode);
    } else if(opcode == 0x9E) { ins("stz %W,x");
    } else if(opcode == 0xBE) { ins("ldx %W,y");

    } else if((opcode & 0x1F) == 0x0F) { ins("%A %L"      , opcode);
    } else if((opcode & 0x1F) == 0x1F) { ins("%A %L,x"    , opcode);
    } else { ins("???"); }
}

////////////////////////////////////////////////////////////////////////////////

static FILE* infile = NULL;
static uint32_t infile_bytes_left;

static int getbyte(void) {
    if(infile_bytes_left) {
        int n = fgetc(infile);
        if(n == EOF) {
            infile_bytes_left = 0;
            return -1;
        } else {
            infile_bytes_left--;
            return (n & 0xFF);
        }
    }
    return -1;
}

static void disasm_range(uint32_t fileoffset, uint32_t len, uint32_t addr) {
    fseeko(infile, fileoffset, SEEK_SET);
    dasm_address = addr;
    infile_bytes_left = len;
    mflag = mflag_default;
    xflag = xflag_default;
    fetchn = unfetchn = 0;
    while(infile_bytes_left > 0) {
        disassemble_one();
    }
}

static uint32_t gethex(const char* s) {
    if(!s[0]) return 0;
    if(s[0] == '$') s++;
    if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    return (uint32_t)strtoul(s, NULL, 16);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    const char* infilename;
    uint32_t arg_start;
    uint32_t arg_len;
    uint32_t arg_addr;
    int option_argn = 4;

    normalize_argv0(argv[0]);

    if(argc < 4) {
        banner();
        printf(
            "Usage: %s imagefile start address [length] [options]\n"
            "Output is written to stdout. All values must be given in hex.\n"
            "If no length is given, disassembly will stop at the end of the bank.\n"
            "Options:\n"
            "  -m0         Assume M flag = 0\n"
            "  -m1         Assume M flag = 1 (default)\n"
            "  -x0         Assume X flag = 0 (default)\n"
            "  -x1         Assume X flag = 1\n"
            "  -noreturn   Disable flag reset after RTS/RTL/RTI\n"
            "  -noguess    Disable flag guess on BRK/COP/WDM/STP\n"
            "  -nofollow   Disable REP/SEP following (not recommended)\n",
            argv[0]
        );
        return 1;
    }

    infilename = argv[1];
    arg_start = gethex(argv[2]);
    arg_addr  = gethex(argv[3]);

    arg_len = ((arg_addr & 0xFFFF) ^ 0xFFFF) + 1;

    if((argc >= 5) && (argv[4][0] != '-')) {
        arg_len = gethex(argv[4]);
        option_argn = 5;
    }

    while(option_argn < argc) {
        const char* s = argv[option_argn];
        if       (!strcmp(s, "-m0")) { mflag_default = 0;
        } else if(!strcmp(s, "-m1")) { mflag_default = 1;
        } else if(!strcmp(s, "-x0")) { xflag_default = 0;
        } else if(!strcmp(s, "-x1")) { xflag_default = 1;
        } else if(!strcmp(s, "-noreturn")) { flag_return = 0;
        } else if(!strcmp(s, "-noguess" )) { flag_guess  = 0;
        } else if(!strcmp(s, "-nofollow")) { flag_follow = 0;
        } else {
            printf("unknown option: %s\n", s);
            return 1;
        }
        option_argn++;
    }

    printf(
        "Disassembly of %s\n"
        "Starting at offset $%lX for $%lX bytes\n"
        "65816 address starts at $%lX\n"
        "return=%s guess=%s follow=%s\n"
        "\n",
        infilename,
        (unsigned long)arg_start,
        (unsigned long)arg_len,
        (unsigned long)arg_addr,
        flag_return ? "on" : "off",
        flag_guess  ? "on" : "off",
        flag_follow ? "on" : "off"
    );

    infile = fopen(infilename, "rb");
    if(!infile) {
        fprintf(stderr, "Error: %s: %s\n", infilename, strerror(errno));
        return 1;
    }

    disasm_range(arg_start, arg_len, arg_addr);

    fclose(infile);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "wordadd - Addition word puzzle solver"
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
//
// Count trailing zeroes
//
static uint8_t ctztable[1024];

static void init_ctztable(void) {
    size_t i, j, k;
    ctztable[0] = 0;
    for(i = 1; i < 1024; i++) {
        for(k = 0, j = i; !(j & 1); k++, j >>= 1) { }
        ctztable[i] = k;
    }
}

////////////////////////////////////////////////////////////////////////////////

enum {
    DIGIT   = 0x0F,
    KNOWN   = 0x10,
    NONZERO = 0x20
};

static char**   lines;
static size_t n_lines;

static uint8_t digitmap[256];
static uint16_t available = 0x3FF;

static uint32_t solutions = 0;
static char printresults = 0;

static void init_puzzle(void) {
    size_t i;

    //
    // Reset digit maps
    //
    memset(digitmap, 0, 256);
    //
    // Digits not present: 0
    //
    digitmap[0] = KNOWN | 0;
    //
    // 0-9 are literal
    //
    for(i = 0; i < 10; i++) {
        digitmap['0' + i] = KNOWN | i;
    }
    //
    // First letter of each word must be nonzero
    //
    for(i = 0; i < n_lines; i++) {
        if(lines[i][0]) {
            digitmap[(uint8_t)lines[i][0]] |= NONZERO;
        }
    }
}

void print_puzzle(void) {
    size_t i;
    for(i = 0; i < n_lines; i++) {
        if(i == (n_lines - 1)) {
            printf(" = ");
        } else if(i > 0) {
            printf(" + ");
        }
        printf("%s", lines[i]);
    }
    printf("\n");
}

void print_solution(void) {
    size_t i, j;
    for(i = 0; i < n_lines; i++) {
        if(i == (n_lines - 1)) {
            printf(" = ");
        } else if(i > 0) {
            printf(" + ");
        }
        for(j = 0; lines[i][j]; j++) {
            fputc('0' + (digitmap[(uint8_t)(lines[i][j])] & DIGIT), stdout);
        }
    }
    printf("\n");
}

////////////////////////////////////////////////////////////////////////////////

void solve_puzzle(
    size_t line,
    size_t place, // 0 = rightmost
    int carry_in,
    char final
) {
    //
    // Obtain the current letter, or 0 if none
    //
    size_t len = strlen(lines[line]);
    uint8_t letter =
        (place < len) ?
        (letter = lines[line][(len - 1) - place]) : 0;

    if(letter) { final = 0; }


    if(line == (n_lines - 1)) {
        //
        // This is the sum line
        //

        if(final) {
            //
            // We have finished the addition
            //
            if(carry_in == 0) {
                //
                // Sum matches: Found a solution
                //
                if(printresults) { print_solution(); }
                solutions++;
            }
        } else {
            //
            // Get next carry digit
            //
            int carry_out = carry_in / 10;
            carry_in %= 10;

            if(digitmap[letter] & KNOWN) {
                //
                // Letter is mapped
                //
                if((digitmap[letter] & DIGIT) == carry_in) {
                    //
                    // Letter is mapped to the correct digit; move on
                    //
                    solve_puzzle(0, place + 1, carry_out, 1);
                }
            } else {
                //
                // Letter is not mapped yet.
                // We will need to map it to carry_in.
                //
                int mask = 1 << carry_in;
                if(
                    (available & mask) &&
                    (carry_in || !(digitmap[letter] & NONZERO))
                ) {
                    //
                    // Digit we need is available; map it and move on
                    //
                    digitmap[letter] |=   KNOWN | carry_in;
                    available ^= mask;

                    solve_puzzle(0, place + 1, carry_out, 1);

                    digitmap[letter] &= ~(KNOWN | DIGIT);
                    available ^= mask;
                }
            }
        }
    } else {
        //
        // This is not the sum line
        //
        if(digitmap[letter] & KNOWN) {

            //
            // Letter is mapped; add it and move on
            //
            solve_puzzle(line + 1, place, carry_in + (digitmap[letter] & DIGIT), final);

        } else {
            //
            // Letter is not mapped
            //

            //
            // Try all available digits, and count the solutions
            //
            int t = available;
            if(digitmap[letter] & NONZERO) { t &= ~1; }
            for(; t; t &= t - 1) {
                int digit = ctztable[t];
                int mask = 1 << digit;

                digitmap[letter] |=   KNOWN | digit;
                available ^= mask;

                solve_puzzle(line + 1, place, carry_in + digit, final);

                digitmap[letter] &= ~(KNOWN | DIGIT);
                available ^= mask;
            }
        }
    }
}

static void solve_puzzle_all(void) {
    solve_puzzle(0, 0, 0, 1);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int i;
    // Bitmask of letters A-Z that were used
    uint32_t letters = 0;

    int lettercount;
    int avdigitcount;

    if(argc < 3) {
        banner();
        printf(
            "Usage: %s words... sum\n"
            "Examples:\n"
            "    %s BEEF BACON MEATS\n",
            argv[0],
            argv[0]
        );
        return 1;
    }

    for(i = 1; i < argc; i++) {
        char* s = argv[i];
        size_t j;
        for(j = 0; s[j]; j++) {
            int c = ((unsigned char)(s[j]));
            static const char* allletters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            static const char* alldigits  = "0123456789";
            if(isalpha(c)) {
                const char* t;
                s[j] = c = toupper(c);
                t = strchr(allletters, c);
                if(t) {
                    letters |= ((uint32_t)1) << (t - allletters);
                    continue;
                }
            } else if(isdigit(c)) {
                const char* t = strchr(alldigits, c);
                if(t) {
                    //
                    // Allow digits, but don't allow letters to use that digit
                    //
                    available &= ~(1 << (t - alldigits));
                    continue;
                }
            }
            printf("Not a word: \"%s\"\n", s);
            return 1;
        }
    }
    lines   = argv + 1;
    n_lines = argc - 1;

    //
    // Count letters
    //
    lettercount = 0;
    for(i = 0; i < 26; i++) {
        lettercount += (letters >> i) & 1;
    }
    if(lettercount < 1) {
        printf("Not enough letters\n");
        return 1;
    }
    if(lettercount > 10) {
        printf("Too many letters (%d)\n", lettercount);
        return 1;
    }

    //
    // Count available digits
    //
    avdigitcount = 0;
    for(i = 0; i < 10; i++) {
        avdigitcount += (available >> i) & 1;
    }
    if(lettercount > avdigitcount) {
        printf(
            "There are more letters (%d) than available digits (%d)\n",
            lettercount,
            avdigitcount
        );
        return 1;
    }

    //
    // Solve
    //
    init_ctztable();
    init_puzzle();
    print_puzzle();
    printresults = 1;
    solve_puzzle_all();
    printf("Solutions: %lu\n", (unsigned long)solutions);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

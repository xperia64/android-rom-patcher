////////////////////////////////////////////////////////////////////////////////
//
#define TITLE "pecompat - Maximize compatibility of a Win32 PE file"
#define COPYR "Copyright (C) 2012 Neill Corlett"
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
// Seek in a way that doesn't involve seeking past the end of the file
//
void safe_seek(FILE* f, unsigned long offset) {
    long limit;

    clearerr(f);
    fseek(f, 0, SEEK_END);
    if(ferror(f)) { return; }

    limit = ftell(f);
    if(ferror(f)) { return; }

    if(limit >= 0) {
        if(offset > ((unsigned long)limit)) {
            fgetc(f); // force EOF
            return;
        }
    }

    fseek(f, offset, SEEK_SET);
}

////////////////////////////////////////////////////////////////////////////////

unsigned get16lsb(FILE* f) {
    int c0, c1;
    clearerr(f);
    c0 = fgetc(f);
    if(ferror(f) || feof(f)) { return 0; }
    c1 = fgetc(f);
    if(ferror(f) || feof(f)) { return 0; }
    return
        (((unsigned)(c1 & 0xff)) <<  8) +
        (((unsigned)(c0 & 0xff))      );
}

unsigned get16lsb_at(FILE* f, unsigned long offset) {
    safe_seek(f, offset);
    if(ferror(f) || feof(f)) { return 0; }
    return get16lsb(f);
}

unsigned long get32lsb(FILE* f) {
    int c0, c1, c2, c3;
    clearerr(f);
    c0 = fgetc(f);
    if(ferror(f) || feof(f)) { return 0; }
    c1 = fgetc(f);
    if(ferror(f) || feof(f)) { return 0; }
    c2 = fgetc(f);
    if(ferror(f) || feof(f)) { return 0; }
    c3 = fgetc(f);
    if(ferror(f) || feof(f)) { return 0; }
    return
        (((unsigned long)(c3 & 0xff)) << 24) +
        (((unsigned long)(c2 & 0xff)) << 16) +
        (((unsigned long)(c1 & 0xff)) <<  8) +
        (((unsigned long)(c0 & 0xff))      );
}

unsigned long get32lsb_at(FILE* f, unsigned long offset) {
    safe_seek(f, offset);
    if(ferror(f) || feof(f)) { return 0; }
    return get32lsb(f);
}

int put16lsb(unsigned value, FILE* f) {
    clearerr(f);
    fputc(((value >>  0) & 0xff), f);
    if(ferror(f) || feof(f)) { return 1; }
    fputc(((value >>  8) & 0xff), f);
    if(ferror(f) || feof(f)) { return 1; }
    fflush(f);
    return 0;
}

int put16lsb_at(unsigned value, FILE* f, unsigned long offset) {
    safe_seek(f, offset);
    if(ferror(f) || feof(f)) { return 0; }
    return put16lsb(value, f);
}

int put32lsb(unsigned long value, FILE* f) {
    clearerr(f);
    fputc(((value >>  0) & 0xff), f);
    if(ferror(f) || feof(f)) { return 1; }
    fputc(((value >>  8) & 0xff), f);
    if(ferror(f) || feof(f)) { return 1; }
    fputc(((value >> 16) & 0xff), f);
    if(ferror(f) || feof(f)) { return 1; }
    fputc(((value >> 24) & 0xff), f);
    if(ferror(f) || feof(f)) { return 1; }
    fflush(f);
    return 0;
}

int put32lsb_at(unsigned long value, FILE* f, unsigned long offset) {
    safe_seek(f, offset);
    if(ferror(f) || feof(f)) { return 0; }
    return put32lsb(value, f);
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert virtual address to file offset
// Returns 0 if not found
//
unsigned long virtual_to_fileoffset(FILE* f, unsigned long v) {
    unsigned long pe;
    unsigned long imagebase;
    unsigned long obj;
    unsigned long nobjs;
    unsigned long i;

    pe = get32lsb_at(f, 0x3c);
    if(ferror(f) || feof(f)) { return 0; }

    imagebase = get32lsb_at(f, pe + 0x34);
    if(ferror(f) || feof(f)) { return 0; }

    nobjs = get16lsb_at(f, pe + 0x06);
    if(ferror(f) || feof(f)) { return 0; }

    obj = pe + 0x18 + get16lsb_at(f, pe + 0x14);
    if(ferror(f) || feof(f)) { return 0; }

    for(i = 0; i < nobjs; i++) {
        unsigned long record = 0x28 * i + obj;
        unsigned long vsize;
        unsigned long vaddr;
        unsigned long psize;
        unsigned long fileoffset;
        unsigned long size;
        vsize   = get32lsb_at(f, record + 0x08);
        if(ferror(f) || feof(f)) { return 0; }
        vaddr   = get32lsb_at(f, record + 0x0c) + imagebase;
        if(ferror(f) || feof(f)) { return 0; }
        psize   = get32lsb_at(f, record + 0x10);
        if(ferror(f) || feof(f)) { return 0; }
        fileoffset = get32lsb_at(f, record + 0x14);
        if(ferror(f) || feof(f)) { return 0; }
        size = vsize;
        if(size > psize) { size = psize; }
        if((v >= vaddr) && (v < (vaddr + size))) {
            return (v - vaddr) + fileoffset;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

void dumpstring(FILE* f, unsigned long offset) {
    safe_seek(f, offset);
    if(ferror(f) || feof(f)) { return; }
    for(;;) {
        int c = fgetc(f);
        if((c == 0) || (c == EOF) || (!isprint(c))) { break; }
        fputc(c, stdout);
    }
}

////////////////////////////////////////////////////////////////////////////////

int fixpe(const char* filename) {
    int returnvalue = 1;
    FILE* f = NULL;
    unsigned long pe_header;
    unsigned cpu_type;

    unsigned long old_pe_checksum;
    unsigned long new_pe_checksum;

    f = fopen(filename, "r+b");
    if(!f) { goto fileerror; }

    //
    // Check MZ header
    //
    {   unsigned mz = get16lsb_at(f, 0);
        if(ferror(f) || feof(f)) { goto fileerror; }
        if(mz != 0x5a4d) {
            printf("%s: Error: Incorrect MZ signature\n", filename);
            goto error;
        }
    }

    //
    // Check PE header
    //
    pe_header = get32lsb_at(f, 0x3c);
    if(ferror(f)) { goto fileerror; }
    if(feof(f)) {
        printf("%s: Error: Too short, not a PE file\n", filename);
        goto error;
    }
    {   unsigned long pe = get32lsb_at(f, pe_header);
        if(ferror(f)) { goto fileerror; }
        if(feof(f)) {
            printf("%s: Error: PE signature not found\n", filename);
            goto error;
        }
        if(pe != 0x00004550) {
            printf("%s: Error: Incorrect PE signature\n", filename);
            goto error;
        }
    }

    //
    // Check CPU type (but don't patch it)
    //
    cpu_type = get16lsb_at(f, pe_header+0x04);
    if(ferror(f) || feof(f)) { goto fileerror; }
    switch(cpu_type) {
    case 0x014c:
        // 80386 - OK
        break;
    case 0x014d:
    case 0x014e:
        // 80486 or 80586 - warn
        printf("%s: Warning: CPU type of 0x%x is not strictly 80386 compatible\n", filename, cpu_type);
        break;
    case 0x8664:
        // x86-64 - will not work
        printf("%s: Error: x86-64 architecture; not supported\n", filename);
        goto error;
    default:
        // Unknown architecture
        printf("%s: Error: Unknown CPU architecture 0x%x\n", filename, cpu_type);
        goto error;
    }

    //
    // Patch the linker timestamp
    //
    {   unsigned long old_timestamp;
        unsigned long new_timestamp = 0;
        old_timestamp = get32lsb_at(f, pe_header+0x08);
        if(ferror(f) || feof(f)) { goto fileerror; }
        if(old_timestamp != new_timestamp) {
            printf("%s: Patching linker timestamp: 0x%lx -> 0x%lx\n",
                filename,
                old_timestamp,
                new_timestamp
            );
            if(put32lsb_at(new_timestamp, f, pe_header+0x08)) { goto fileerror; }
        }
    }

    //
    // Patch the OS version
    //
    {   unsigned old_os_major;
        unsigned old_os_minor;
        unsigned new_os_major = 4;
        unsigned new_os_minor = 0;
        old_os_major  = get16lsb_at(f, pe_header+0x40);
        if(ferror(f) || feof(f)) { goto fileerror; }
        old_os_minor  = get16lsb_at(f, pe_header+0x42);
        if(ferror(f) || feof(f)) { goto fileerror; }
        if(
            old_os_major != new_os_major ||
            old_os_minor != new_os_minor
        ) {
            printf("%s: Patching OS version: %u.%u -> %u.%u\n",
                filename,
                old_os_major, old_os_minor,
                new_os_major, new_os_minor
            );
            if(put16lsb_at(new_os_major, f, pe_header+0x40)) { goto fileerror; }
            if(put16lsb_at(new_os_minor, f, pe_header+0x42)) { goto fileerror; }
        }
    }

    //
    // Patch the subsystem version
    //
    {   unsigned old_sub_major;
        unsigned old_sub_minor;
        unsigned new_sub_major = 4;
        unsigned new_sub_minor = 0;
        old_sub_major  = get16lsb_at(f, pe_header+0x48);
        if(ferror(f) || feof(f)) { goto fileerror; }
        old_sub_minor  = get16lsb_at(f, pe_header+0x4a);
        if(ferror(f) || feof(f)) { goto fileerror; }
        if(
            old_sub_major != new_sub_major ||
            old_sub_minor != new_sub_minor
        ) {
            printf("%s: Patching subsystem version: %u.%u -> %u.%u\n",
                filename,
                old_sub_major, old_sub_minor,
                new_sub_major, new_sub_minor
            );
            if(put16lsb_at(new_sub_major, f, pe_header+0x48)) { goto fileerror; }
            if(put16lsb_at(new_sub_minor, f, pe_header+0x4a)) { goto fileerror; }
        }
    }

    //
    // Grab the old PE checksum
    //
    old_pe_checksum = get32lsb_at(f, pe_header+0x58);
    if(ferror(f) || feof(f)) { goto fileerror; }

    //
    // Calculate the new PE checksum
    //
    new_pe_checksum = 0;
    {   unsigned long offset = 0;

        safe_seek(f, 0);
        if(ferror(f) || feof(f)) { goto fileerror; }

        for(;;) {
            unsigned long len = 0;
            int c0, c1;
            c0 = fgetc(f);
            if(ferror(f)) { goto fileerror; }
            if(c0 == EOF) { break; } else { len++; }
            c1 = fgetc(f);
            if(ferror(f)) { goto fileerror; }
            if(c1 == EOF) { c1 = 0; } else { len++; }
            //
            // Treat existing checksum field as zero for purposes of checksumming
            //
            if(
                offset >= (pe_header + 0x58) &&
                offset <  (pe_header + 0x5c)
            ) { c0 = c1 = 0; }
            //
            // Add word with carry
            //
            {   unsigned long word = 
                    (((unsigned long)(c0 & 0xff)) << 0) +
                    (((unsigned long)(c1 & 0xff)) << 8);
                new_pe_checksum += word;
                new_pe_checksum += (new_pe_checksum >> 16);
                new_pe_checksum &= ((unsigned long)(0xffff));
            }
            offset += len;
        }
        //
        // Finally, add the file size
        //
        new_pe_checksum += offset;
    }

    //
    // Patch checksum if necessary
    //
    if(old_pe_checksum != new_pe_checksum) {
        printf("%s: Patching checksum: 0x%lx -> 0x%lx\n",
            filename,
            old_pe_checksum,
            new_pe_checksum
        );
        if(put32lsb_at(new_pe_checksum, f, pe_header+0x58)) { goto fileerror; }
    }

    //
    // Look at the import table
    //
    {   unsigned long imagebase;
        unsigned long import_table_vaddr;
        unsigned long import_table_size;
        unsigned long import_table_fileoffset;

        unsigned long i;
        unsigned long warned_functions = 0;
        unsigned long max_warned_functions = 5;

        imagebase          = get32lsb_at(f, pe_header+0x34);
        if(ferror(f) || feof(f)) { goto fileerror; }
        import_table_vaddr = get32lsb_at(f, pe_header+0x80) + imagebase;
        if(ferror(f) || feof(f)) { goto fileerror; }
        import_table_size  = get32lsb_at(f, pe_header+0x84);
        if(ferror(f) || feof(f)) { goto fileerror; }

        import_table_fileoffset = virtual_to_fileoffset(f, import_table_vaddr);
        if(ferror(f) || feof(f)) { goto fileerror; }

        if(!import_table_fileoffset) {
            printf("%s: Error: Unable to find import table\n", filename);
            goto error;
        }

        for(i = 0; i < import_table_size; i += 0x14) {
            unsigned long function_list_vaddr;
            unsigned long dll_name_vaddr;
            unsigned long function_list_fileoffset;
            unsigned long dll_name_fileoffset;
            function_list_vaddr = get32lsb_at(f, import_table_fileoffset + i + 0x00);
            if(ferror(f) || feof(f)) { goto fileerror; }
            dll_name_vaddr      = get32lsb_at(f, import_table_fileoffset + i + 0x0c);
            if(ferror(f) || feof(f)) { goto fileerror; }

            //
            // These lists seem to be NULL-terminated
            //
            if(function_list_vaddr == 0) { break; }
            if(dll_name_vaddr      == 0) { break; }

            function_list_vaddr += imagebase;
            dll_name_vaddr      += imagebase;

            function_list_fileoffset = virtual_to_fileoffset(f, function_list_vaddr);
            if(ferror(f) || feof(f)) { goto fileerror; }
            dll_name_fileoffset      = virtual_to_fileoffset(f, dll_name_vaddr);
            if(ferror(f) || feof(f)) { goto fileerror; }

            if(!function_list_fileoffset) {
                printf("%s: Error: Malformed import table: Unable to map function list address 0x%lx\n",
                    filename,
                    function_list_vaddr
                );
                goto error;
            }
            if(!dll_name_fileoffset) {
                printf("%s: Error: Malformed import table: Unable to map DLL filename address 0x%lx\n",
                    filename,
                    dll_name_vaddr
                );
                goto error;
            }

            for(;;) {
                int c0 = 0, c1 = 0;
                unsigned long methodname_vaddr;
                unsigned long methodname_fileoffset;

                methodname_vaddr = get32lsb_at(f, function_list_fileoffset);
                if(ferror(f) || feof(f)) { goto fileerror; }

                if(methodname_vaddr == 0) { break; }
                methodname_vaddr += imagebase;

                function_list_fileoffset += 4;

                methodname_fileoffset = virtual_to_fileoffset(f, methodname_vaddr);
                if(ferror(f) || feof(f)) { goto fileerror; }
                if(!methodname_fileoffset) {
                    printf("%s: Error: Malformed import table: Unable to map method name address 0x%lx\n",
                        filename,
                        methodname_vaddr
                    );
                    goto error;
                }

                //
                // Method name ending in 'W'?
                //
                safe_seek(f, methodname_fileoffset + 2);
                if(ferror(f) || feof(f)) { goto fileerror; }

                for(;;) {
                    c0 = c1;
                    c1 = fgetc(f);
                    if(c1 == 0 || c1 == EOF) { break; }
                }
                if(c0 == 'W') {
                    if(warned_functions < max_warned_functions) {
                        if(warned_functions == 0) {
                            printf("%s: Warning: The following Unicode imports are used:\n",
                                filename
                            );
                        }
                        printf("  ");
                        dumpstring(f, dll_name_fileoffset);
                        printf(": ");
                        dumpstring(f, methodname_fileoffset + 2);
                        printf("\n");
                    }
                    warned_functions++;
                }
            }
        }
        if(warned_functions > max_warned_functions) {
            printf("  ...plus %lu more\n", (warned_functions - max_warned_functions));
        }
    }

    printf("%s: Done\n", filename);

    returnvalue = 0;
    goto done;
fileerror:
    if((!f) || ferror(f)) {
        printf("%s: Error: %s\n", filename, strerror(errno));
    } else if(feof(f)) {
        printf("%s: Error: Unexpected end of file\n", filename);
    } else {
        printf("%s: Error: Unknown error\n", filename);
    }
    goto error;
error:
    returnvalue = 1;
done:
    if(f) { fclose(f); }
    return returnvalue;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    int error = 0;
    int i;

    normalize_argv0(argv[0]);

    if(argc < 2) {
        banner();
        printf("Usage: %s pe_exe_file(s)\n", argv[0]);
        return 1;
    }

    for(i = 1; i < argc; i++) {
        if(fixpe(argv[i])) { error = 1; }
    }

    return error;
}

////////////////////////////////////////////////////////////////////////////////

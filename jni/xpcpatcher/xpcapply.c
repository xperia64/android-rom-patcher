/***************************************************************
 * xpctools                                                    *
 * a set of tools to create and manage ExecROM patchfiles      *
 *                                                             *
 * (c) 2004 A&L Software                                       *
 *                                                             *
 * This program may be freely copied, modified and distributed *
 * under the terms of GNU GPL license version 2 or later.      *
 ***************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xpctools.h"

#define ASKNONE		1
#define ASKSOME		2
#define ASKALL		3

int main(int argc, char **argv) {
	xpc_t *xpc;
	char *inrom = NULL;
	char *outrom = NULL;
	char *xpcfile = NULL;
	FILE *fi, *fo;
	unsigned char buffer[8192];
	int apply = ASKNONE;
	int i, j, k;

	setprogramname(argv[0]);
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-o")) {
			if (i < argc-1) outrom = argv[++i]; else goto help;
		} else if (!strcmp(argv[i], "-ask")) {
			apply = ASKSOME;
		} else if (!strcmp(argv[i], "-askall")) {
			apply = ASKALL;
		} else if (!inrom) {
			inrom = argv[i];
		} else if (!xpcfile) {
			xpcfile = argv[i];
		} else goto help;
	}
	if (!inrom || argc > 6 || argc < 2 || (argc == 2 && !strcmp(argv[1], "--help"))) {
help:		fprintf(stderr, "xpctools - xpcapply\n"
				"(c) 2004 A&L Software\n"
				"This program may be freely copied, modified and distributed\n"
				"under the terms of GNU GPL license version 2 or later.\n\n"
				"usage: xpcapply romfile [xpcfile] [-o modifiedromfile] [-ask|-askall]\n"
				"if xpcfile is ommited or \"-\", it will be read from stdin\n"
				"if output ROM file is ommited, original ROM file itself will be modified\n");
		exit(1);
	}
	if (!xpcfile) {
		if (apply != ASKNONE) {
			fprintf(stderr, "%s: can't use interactive mode when reading patchfile from stdin\n", programname);
			exit(1);
		}
		xpcfile = "-";
	}
	if (outrom) {
		if (!(fi = fopen(inrom, "rb"))) {
			fprintf(stderr, "%s: can't open %s\n", programname, inrom);
			exit(1);
		}
		if (!(fo = fopen(outrom, "wb"))) {
			fprintf(stderr, "%s: can't create %s\n", programname, outrom);
			exit(1);
		}
		while (!feof(fi)) {
			i = fread(buffer, 1, 8192, fi);
			if (fwrite(buffer, 1, i, fo) != i) {
				fprintf(stderr, "%s: can't write %s\n", programname, outrom);
				exit(1);
			}
		}
		fclose(fi);
		fclose(fo);
		inrom = outrom;
	}
	xpc = readxpc(xpcfile);
	if (!(fi = fopen(inrom, "r+b"))) {
		fprintf(stderr, "%s: can't open %s\n", programname, inrom);
		exit(1);
	}
	fseek(fi, 0L, SEEK_END);
	if ((ftell(fi) & 0x1fff) != 0) {
		fprintf(stderr, "%s: file %s doesn't have size multiple of 8192 bytes\n", programname, inrom);
		exit(1);
	}
	fseek(fi, 0L, SEEK_SET);
	for (i = 0; i < xpc->numblocks; i++) {
		if ((apply == ASKSOME && !xpc->xpcblock[i]->essential) || apply == ASKALL) {
			printf("%s? (Y/N) ", xpc->xpcblock[i]->description);
			fflush(stdout);
			fflush(stdin);
			fgets(buffer, 8191, stdin);
			if (*buffer == 'N' || *buffer == 'n') continue;
		}
		for (j = 0; j < xpc->xpcblock[i]->numpatches; j++) {
			fseek(fi, xpc->xpcblock[i]->patches[j]->page*8192+xpc->xpcblock[i]->patches[j]->address, SEEK_SET);
			for (k = 0; k < xpc->xpcblock[i]->patches[j]->size; k++) {
				if (fputc(xpc->xpcblock[i]->patches[j]->data[k], fi) != xpc->xpcblock[i]->patches[j]->data[k]) {
					fprintf(stderr, "%s: error writing %s\n", programname, inrom);
					exit(1);
				}
			}
		}
	}
	fclose(fi);
	return 0;
}

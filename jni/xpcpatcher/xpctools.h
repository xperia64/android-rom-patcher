/***************************************************************
 * xpctools                                                    *
 * a set of tools to create and manage ExecROM patchfiles      *
 *                                                             *
 * (c) 2004 A&L Software                                       *
 *                                                             *
 * This program may be freely copied, modified and distributed *
 * under the terms of GNU GPL license version 2 or later.      *
 ***************************************************************/

typedef unsigned char uchar;

typedef struct {
	uchar size;
	uchar page;
	int address;
	uchar data[256];
} xpc_patch_t;

typedef struct {
	uchar numpatches;
	uchar essential;
	char *description;
	xpc_patch_t *patches[255];
} xpc_block_t;

typedef struct {
	uchar version;
	int numblocks;
	xpc_block_t *xpcblock[8192];
} xpc_t;

char *programname;

void setprogramname(char *name) {
	programname = name;
}

void freexpc(xpc_t *txpc) {
	int i, j;

	for (i = 0; i < txpc->numblocks; i++) {
		free(txpc->xpcblock[i]->description);
		for (j = 0; j < txpc->xpcblock[i]->numpatches; j++) {
			free(txpc->xpcblock[i]->patches[j]);
		}
		free(txpc->xpcblock[i]);
	}
	free(txpc);
}

xpc_t *newxpc() {
	xpc_t *txpc;

	if (!(txpc = (xpc_t *)malloc(sizeof(xpc_t)))) return NULL;
	txpc->version = 0x10;
	txpc->numblocks = 0;
	return txpc;
}

void writexpc(char *nam, xpc_t *txpc) {
	char *name = nam;
	FILE *f;
	int i, j, k;

	if (!strcmp(nam, "-")) {
		f = stdout;
		nam = "stdout";
	} else {
		f = fopen(name, "wb");
	}
	if (!f) {
		fprintf(stderr, "%s: cannot open file %s for writing\n", programname, name);
		exit(1);
	}
	fwrite("ExecROM patchfile\x1a", 18, 1, f);
	fputc(txpc->version, f);
	for (i = 0; i < txpc->numblocks; i++) {
		fputc(txpc->xpcblock[i]->numpatches, f);
		fputc(txpc->xpcblock[i]->essential, f);
		fputs(txpc->xpcblock[i]->description, f);
		fputc('$', f);
		for (j = 0; j < txpc->xpcblock[i]->numpatches; j++) {
			fputc(txpc->xpcblock[i]->patches[j]->size, f);
			fputc(txpc->xpcblock[i]->patches[j]->address & 0xff, f);
			fputc(txpc->xpcblock[i]->patches[j]->address >> 8, f);
			fputc(txpc->xpcblock[i]->patches[j]->page, f);
			for (k = 0; k < txpc->xpcblock[i]->patches[j]->size; k++) {
				fputc(txpc->xpcblock[i]->patches[j]->data[k], f);
			}
		}
	}
	fputc(0, f);
	fclose(f);
}

xpc_t *readxpc(char *nam) {
	char sig[19];
	char *name = nam;
	char patchname[1024];
	xpc_t *txpc;
	int n, i, blockidx = 0, patchidx, j;
	FILE *f;

	if (!strcmp(nam, "-")) {
		f = stdin;
		nam = "stdin";
	} else {
		f = fopen(name, "rb");
	}
	if (!f) {
		fprintf(stderr, "%s: cannot open file %s for reading\n", programname, name);
		exit(1);
	}
	fread(sig, 19, 1, f);
	if (memcmp(sig, "ExecROM patchfile\x1a", 18)) {
		fprintf(stderr, "%s: file %s isn't an ExecROM patchfile\n", programname, name);
		fclose(f);
		exit(1);
	}
	if (!(txpc = newxpc())) {
nomem:		fprintf(stderr, "%s: out of memory reading patchfile\n", programname);
		exit(1);
	}
	txpc->version = (unsigned char)sig[18];
	for (;;) {
		if (feof(f)) {
corrupted:		fprintf(stderr, "%s: unexpected end of patchfile\n", programname);
			exit(1);
		}
		if (!(n = fgetc(f))) break;
		txpc->numblocks++;
		if (!(txpc->xpcblock[blockidx] = (xpc_block_t *)malloc(sizeof(xpc_block_t)))) goto nomem;
		txpc->xpcblock[blockidx]->numpatches = n;
		if (feof(f)) goto corrupted;
		txpc->xpcblock[blockidx]->essential = fgetc(f);
		i = 0;
		while ((n = fgetc(f)) != '$') {
			if (feof(f)) goto corrupted;
			patchname[i++] = n;
		}
		patchname[i] = 0;
		txpc->xpcblock[blockidx]->description = strdup(patchname);
		patchidx = 0;
		if (feof(f)) goto corrupted;
		for (i = 0; i < txpc->xpcblock[blockidx]->numpatches; i++) {
			if (!(txpc->xpcblock[blockidx]->patches[patchidx] = (xpc_patch_t *)malloc(sizeof(xpc_patch_t)))) goto nomem;
			txpc->xpcblock[blockidx]->patches[patchidx]->size = fgetc(f);
			if (feof(f)) goto corrupted;
			txpc->xpcblock[blockidx]->patches[patchidx]->address = fgetc(f);
			if (feof(f)) goto corrupted;
			txpc->xpcblock[blockidx]->patches[patchidx]->address += 256*fgetc(f);
			if (feof(f)) goto corrupted;
			txpc->xpcblock[blockidx]->patches[patchidx]->page = fgetc(f);
			if (feof(f)) goto corrupted;
			for (j = 0; j < txpc->xpcblock[blockidx]->patches[patchidx]->size; j++) {
				txpc->xpcblock[blockidx]->patches[patchidx]->data[j] = fgetc(f);
				if (feof(f)) goto corrupted;
			}
			patchidx++;
		}
		blockidx++;
	}
	fclose(f);
	return txpc;
}

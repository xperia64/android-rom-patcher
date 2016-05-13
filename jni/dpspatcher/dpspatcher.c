	

    /*
    * DPS Patcher (c) Marc de Falco
    * It's a simple patch system for patching rom without preserving size.
    */
    #include <stdio.h>
    #include <stdlib.h>
     
    #define PATCH_UNSTABLE 1
    #define STR_SIZE 0x40
    #define ENCLOSED_DATA 1
    #define COPY_DATA 0
     
int dpspatch(int argc, char **argv) {
      FILE *fin, *fout, *fpatch;
      char patch_name[STR_SIZE];
      char patch_author[STR_SIZE];
      char patch_version[STR_SIZE];
      unsigned char patch_flag;
      unsigned char dps_version;
      unsigned int original_size;
     
      if (argc != 4) {
        printf("Bad parameters. Usage :\n%s <ORIGINAL ROM FILE> <PATCHED ROM FILE> <PATCH>\n",
               argv[0]);
    #ifdef WINDOWS
        sleep(100);
    #endif
        return -1;
      }
     
      fin = fopen(argv[1], "rb");
      fout = fopen(argv[2], "wb");
      fpatch = fopen(argv[3], "rb");
     
      fread(patch_name, sizeof(char), STR_SIZE, fpatch);
      fread(patch_author, sizeof(char), STR_SIZE, fpatch);
      fread(patch_version, sizeof(char), STR_SIZE, fpatch);
     
      fread(&patch_flag, sizeof(unsigned char), 1, fpatch);
      fread(&dps_version, sizeof(unsigned char), 1, fpatch);
      fread(&original_size, sizeof(unsigned int), 1, fpatch);
     
      fseek(fin, 0, SEEK_END);
     
      if (ftell(fin) != original_size) {
        #ifdef DEBUG
		  printf("%08x %08x\n", ftell(fin), original_size);
          printf("Size of original rom is incorrect, please make sure you use an original untrimmed rom.\n");
		#endif
	#ifdef WINDOWS
          sleep(100);
    #endif
          fclose(fin);
          fclose(fout);
          fclose(fpatch);
          return -2;
      }
     
      fseek(fin, 0, SEEK_SET);
     
      printf("Patch info : \n");
      printf("\t%s Version %s\n", patch_name, patch_version);
      printf("\tby %s\n", patch_author);
     
	  int ret=0;
      if (patch_flag & PATCH_UNSTABLE)
	  {
		  ret=-3;
          printf("WARNING : this is an UNSTABLE patch and it is not crash free.\n");
	  }
     
      printf("Starting patching process\n");
     
      while (!feof(fpatch)) {
        unsigned char mode = fgetc(fpatch);
        unsigned int offset;
        int i;
     
     
        fread(&offset, sizeof(unsigned int), 1, fpatch);
        fseek(fout, offset, 0);
     
        switch(mode) {
        case ENCLOSED_DATA:
          {
            unsigned int length;   
            fread(&length, sizeof(unsigned int), 1, fpatch);
    #ifdef DEBUG
            printf("Enclosed data @ %08x len : %dbytes\n", offset, length);
    #endif
     
            for (i = 0; i < length; i++)
              fputc(fgetc(fpatch), fout);
          }
          break;
        case COPY_DATA:
          {
            unsigned int copy_offset, length;
            fread(&copy_offset, sizeof(unsigned int), 1, fpatch);
            fread(&length, sizeof(unsigned int), 1, fpatch);
    #ifdef DEBUG
            printf("Copy data @ %08x -> @ %08x len : %dbytes\n", copy_offset, offset, length);
    #endif
     
            fseek(fin, copy_offset, 0);
            for (i = 0; i < length; i++)
              fputc(fgetc(fin), fout);
          }      
        }
     
      }
     
      printf("Patching complete\n");
	  return ret;
    }


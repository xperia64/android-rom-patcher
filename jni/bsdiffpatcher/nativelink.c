/* 
	Native JNI linkage
						*/
#include <jni.h>
#include <stdio.h>

extern int bspatch_perform(char* oldfile, char* newfile, char* patchfile);

int Java_com_xperia64_rompatcher_MainActivity_bsdiffPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath, jstring outputFile)
{
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	const char * szOutputFile = (*env)->GetStringUTFChars(env, outputFile, &isCopy);
	FILE*  InFile = fopen(szPatchPath, "rb");
	FILE*  SrcFile = fopen(szRomPath, "rb");
	FILE* OutFile = fopen(szOutputFile, "wb");
	int r = bspatch_perform((char*)szRomPath, (char*)szOutputFile, (char*)szPatchPath);
	fclose(OutFile);
	fclose(SrcFile);
	fclose(InFile);

	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	(*env)->ReleaseStringUTFChars(env, outputFile, szOutputFile); 
	return r;
}

/* 
	Native JNI linkage
						*/
#include <jni.h>
#include <stdio.h>

extern int    main    (int argc, char** argv);
int Java_com_xperia64_rompatcher_MainActivity_xpcPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath, jstring outputFile)
{
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	const char * szOutputFile = (*env)->GetStringUTFChars(env, outputFile, &isCopy);
	char** filez = NULL;
	filez = malloc(sizeof(char*) * 5);
	filez[0]="xpcapply"; // argv
	filez[1]=szRomPath;
	filez[2]=szPatchPath;
	filez[3]="-o";
	filez[4]=szOutputFile;
	int r = main(5,filez);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	(*env)->ReleaseStringUTFChars(env, outputFile, szOutputFile); 
	free(filez);
	return r;
}
/* 
	Native JNI linkage
						*/
#include <jni.h>
#include <stdio.h>
#include <glib.h>
extern gint    main    (gint argc, gchar** argv);
int Java_com_xperia64_rompatcher_MainActivity_xdelta1PatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath, jstring outputFile)
{
	jboolean isCopy;
	const gchar * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const gchar * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	const gchar * szOutputFile = (*env)->GetStringUTFChars(env, outputFile, &isCopy);
	gchar** filez = NULL;
	filez = malloc(sizeof(gchar*) * 5);
	filez[0]="xdelta1"; // argv
	filez[1]="patch";
	filez[2]=szPatchPath;
	filez[3]=szRomPath;
	filez[4]=szOutputFile;
	int r = (int)main(5,filez);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	(*env)->ReleaseStringUTFChars(env, outputFile, szOutputFile); 
	free(filez);
	return r;
}


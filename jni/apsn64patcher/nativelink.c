/* 
	Native JNI linkage
						*/
#include <jni.h>
#include <stdlib.h>

extern int main (int argc, char **argv);

int Java_com_xperia64_rompatcher_MainActivity_apsN64PatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath)
{
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	char** filez;
	filez = malloc(sizeof(char*) * 4);
	filez[0]="apspatch";
	filez[1]="-f";
	filez[2]=(char*)szRomPath;
	filez[3]=(char*)szPatchPath;
	int err = main(4,filez);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	free(filez);
	return err;
}
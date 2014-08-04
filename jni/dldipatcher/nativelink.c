/* 
	Native JNI linkage
						*/
#include <jni.h>

extern int dldipatch(int argc, char **argv);

int Java_com_xperia64_rompatcher_MainActivity_dldiPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath)
{
	jboolean isCopy;
	char** filez;
	filez = malloc(sizeof(char*) * 4);
	const char * command = "dlditool"; // Arbitrary
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	filez[0]=command;
	filez[1]=szPatchPath;
	filez[2]=szRomPath;
	int err=0;
	err=dldipatch(3, filez);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	return err;
}
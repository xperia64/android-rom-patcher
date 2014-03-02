/* 
	Native JNI linkage
						*/
#include <jni.h>

extern int dpspatch(int argc, char **argv);

int Java_com_xperia64_rompatcher_MainActivity_dpsPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath, jstring outFile)
{
	jboolean isCopy;
	char** filez;
	filez = malloc(sizeof(char*) * 4);
	const char * command = "dpspatch"; // Arbitrary
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szOutFile = (*env)->GetStringUTFChars(env, outFile, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	filez[0]=command;
	filez[1]=szRomPath;
	filez[2]=szOutFile;
	filez[3]=szPatchPath;
	int err=0;
	err=dpspatch(4, filez);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, outFile, szOutFile); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	return err;
}
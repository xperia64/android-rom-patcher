/* 
	Native JNI linkage
						*/
#include <jni.h>
#include <stdlib.h>

extern int main(int argc, char** argv);

int Java_com_xperia64_rompatcher_MainActivity_ecmPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring outFile, jint bkup)
{
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szOutFile = (*env)->GetStringUTFChars(env, outFile, &isCopy);
	char** filez;
	int num = bkup ? 3 : 2;
	filez = malloc(sizeof(char*) * num);
	filez[0]="unecm";
	filez[1]=(char*)szRomPath;
	if(bkup)
	{
		filez[2]=(char*)szOutFile;
	}
	int err = main(num, filez);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, outFile, szOutFile); 
	free(filez);
	return err;
}
/* 
	Native JNI linkage
						*/
#include "libups.hpp"
#include <jni.h>

using namespace nall;


extern "C" /* specify the C calling convention */  

JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_upsPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath,
	 jstring outputFile,
	 jint jignoreChecksum)          /* argument #2 */ 

{ 

     const char *str1 = env->GetStringUTFChars(romPath, 0); 
	 const char *str2 = env->GetStringUTFChars(patchPath, 0); 
     const char *str3 = env->GetStringUTFChars(outputFile, 0); 
	 UPS ups;
	int e = ups.apply(str1, str3, str2, (int)jignoreChecksum);
     env->ReleaseStringUTFChars(romPath, str1); 
	env->ReleaseStringUTFChars(patchPath, str2); 
	env->ReleaseStringUTFChars(outputFile, str3); 
     return e;

} 
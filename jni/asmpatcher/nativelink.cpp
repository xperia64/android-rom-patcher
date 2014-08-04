/* 
	Native JNI linkage
						*/
#ifdef ANDROID
#include <jni.h>
#endif
extern int main(int argc, char **argv);

extern "C" /* specify the C calling convention */  
JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_asmPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath)          /* argument #2 */ 

{ 

    char *str1 = (char*) env->GetStringUTFChars(romPath, 0); 
	char *str2 = (char*)env->GetStringUTFChars(patchPath, 0); 
	int e;
	char** args;
	args = new char * [3]; // could be entered during runtime
	args[0]="xkas";
	args[1]=str2;
	args[2]=str1;
	e=main(3,args);
    env->ReleaseStringUTFChars(romPath, str1); 
	env->ReleaseStringUTFChars(patchPath, str2); 
     return e;

} 

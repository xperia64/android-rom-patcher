#include <jni.h>
extern int main(int argc, char * argv[]);

extern "C" /* specify the C calling convention */  
JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_asarPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath,
	 jint jignoreChecksum)          /* argument #2 */ 

{ 

     char *str1 = (char*)env->GetStringUTFChars(romPath, 0); 
	 char *str2 = (char*)env->GetStringUTFChars(patchPath, 0); 
	 int numargs = 4;
     if(jignoreChecksum)
		 numargs++;
	 char* args[numargs];
	 args[0]="asar";
	 if(jignoreChecksum)
	 {
		 args[1]="-nocheck";
		 args[2]="-pause=no";
		 args[3]=str2;
		 args[4]=str1;
	 }else{
		 args[1]="-pause=no";
		 args[2]=str2;
		 args[3]=str1;
	 }
	 int e = main(numargs, args);
     env->ReleaseStringUTFChars(romPath, str1); 
	env->ReleaseStringUTFChars(patchPath, str2); 
     return e;

} 
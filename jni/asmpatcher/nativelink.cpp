/* 
	Native JNI linkage
						*/
#ifdef ANDROID
#include <jni.h>
#endif
//extern int main(int argc, char **argv);

extern "C" /* specify the C calling convention */  
JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_asmPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath)          /* argument #2 */ 

{ 
	/*args = (char**)(malloc(3*sizeof(char*))); // could be entered during runtime
	args[0]=(char*)malloc((5)*sizeof(char));
	args[1]=(char*)malloc((strlen(str1)*sizeof(char)));
	args[2]= (char*)malloc((strlen(str2)*sizeof(char)));
	args[0] = "xkas";
	args[1] = str1;
	args[2] = str2;*/


    char *str1 = (char*) env->GetStringUTFChars(romPath, 0); 
	char *str2 = (char*)env->GetStringUTFChars(patchPath, 0); /*
	int e = 0;
	char** args;
	args = new char * [3]; // could be entered during runtime
	args[0]="xkas";
	args[1]=str2;
	args[2]=str1;
	//e=main(3,args);
     return e;*/
	env->ReleaseStringUTFChars(patchPath, str2);
	env->ReleaseStringUTFChars(romPath, str1);
	return 0;

} 

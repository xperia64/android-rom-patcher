/* 
	Native JNI linkage
						*/
#include "ips.h"
#include <jni.h>



int Java_com_xperia64_rompatcher_MainActivity_ipsPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath)
{
	struct IPSPatch patch;
	IPSReset(&patch);
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);

	int8_t err;
	err=IPS_ERROR;
	err = IPSOpen(&patch,
			              szPatchPath,
                          szRomPath);
			if(err == IPS_OK)
			{
				/* Process patch */
				while(err == IPS_OK)
				{
					err = IPSReadRecord(&patch);
					if(err == IPS_OK)
					{
						err = IPSProcessRecord (&patch);
					}
				}
			}else{
			return err;
			}
				IPSClose(&patch);
	IPSClose(&patch);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	return err;
}
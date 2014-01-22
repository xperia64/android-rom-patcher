/*******************************************************************************
 * This file is part of ROM Patcher.
 * 
 * Copyright (c) 2014 xperia64.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 * 	Paul Kratt - main MultiPatch application for Mac OSX
 * 	byuu - UPS and BPS patchers
 * 	Neil Corlett - IPS patcher
 * 	Daniel Ekstr'm - PPF patcher
 * 	Josh MacDonald - XDelta
 * 	Colin Percival - BSDiff
 * 	xperia64 - port to Android and IPS32 support
 ******************************************************************************/

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
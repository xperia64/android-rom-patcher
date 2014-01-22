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
#include "libups.hpp"
#include <jni.h>

using namespace nall;


extern "C" /* specify the C calling convention */  

JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_upsPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath,
	 jstring outputFile)          /* argument #2 */ 

{ 

     const char *str1 = env->GetStringUTFChars(romPath, 0); 
	 const char *str2 = env->GetStringUTFChars(patchPath, 0); 
     const char *str3 = env->GetStringUTFChars(outputFile, 0); 
	 UPS ups;
	int e = ups.apply(str1, str3, str2);
     env->ReleaseStringUTFChars(romPath, str1); 
	env->ReleaseStringUTFChars(patchPath, str2); 
	env->ReleaseStringUTFChars(outputFile, str3); 
     return e;

} 
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
#import "nall/bps/patch.hpp"
#import "nall/bps/linear.hpp"
#import "nall/bps/delta.hpp"
#ifdef ANDROID
#include <jni.h>
#endif
#include "nativelink.hpp"
using namespace nall;


extern "C" /* specify the C calling convention */  
#ifdef ANDROID
JNIEXPORT int JNICALL Java_com_xperia64_rompatcher_MainActivity_bpsPatchRom ( 

     JNIEnv *env,        /* interface pointer */ 

     jobject obj,        /* "this" pointer */ 

     jstring romPath,             /* argument #1 */ 

     jstring patchPath,
	 jstring outputFile)          /* argument #2 */ 

{ 

     const char *str1 = env->GetStringUTFChars(romPath, 0); 
	 const char *str2 = env->GetStringUTFChars(patchPath, 0); 
     const char *str3 = env->GetStringUTFChars(outputFile, 0); 
    nall::bpspatch bps;
    bps.modify(str2);
    bps.source(str1);
    bps.target(str3);
    nall::bpspatch::result bpsResult = bps.apply();
	int e=1;
	if (bpsResult==nall::bpspatch::success)
	{
		e=0;
	}else if(bpsResult==nall::bpspatch::patch_too_small)
	{
		e=-1;
	}else if(bpsResult==nall::bpspatch::patch_invalid_header)
	{
		e=-2;
	}else if(bpsResult==nall::bpspatch::source_too_small)
	{
		e=-3;
	}else if(bpsResult==nall::bpspatch::target_too_small)
	{
		e=-4;
	}else if(bpsResult==nall::bpspatch::source_checksum_invalid)
	{
		e=-5;
	}else if(bpsResult==nall::bpspatch::target_checksum_invalid)
	{
		e=-6;
	}else if(bpsResult==nall::bpspatch::patch_checksum_invalid)
	{
		e=-7;
	}else if(bpsResult==nall::bpspatch::unknown)
	{
		e=-8;
	}else{
		e=1;
	}
     env->ReleaseStringUTFChars(romPath, str1); 
	env->ReleaseStringUTFChars(patchPath, str2); 
	env->ReleaseStringUTFChars(outputFile, str3); 
     return e;

} 
#else
	void bpsPatchRom(const char* romPath, const char* patchPath, const char* outputFile)
{
	nall::bpspatch bps;
    bps.modify(patchPath);
    bps.source(romPath);
    bps.target(outputFile);
    nall::bpspatch::result bpsResult = bps.apply();
	return;
}
#endif
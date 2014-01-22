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
#include <jni.h>
#include <stdio.h>

extern int bspatch_perform(char* oldfile, char* newfile, char* patchfile);

int Java_com_xperia64_rompatcher_MainActivity_bsdiffPatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath, jstring outputFile)
{
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	const char * szOutputFile = (*env)->GetStringUTFChars(env, outputFile, &isCopy);
	FILE*  InFile = fopen(szPatchPath, "rb");
	FILE*  SrcFile = fopen(szRomPath, "rb");
	FILE* OutFile = fopen(szOutputFile, "wb");
	int r = bspatch_perform(szRomPath, szOutputFile, szPatchPath);
	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	(*env)->ReleaseStringUTFChars(env, outputFile, szOutputFile); 
	return r;
}

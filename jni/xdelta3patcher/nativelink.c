/* 
	Native JNI linkage
						*/
#include "xdelta3.h"
#include <jni.h>
#include <android/log.h>
#include <stdio.h>

#define  LOG_TAG    "xdelta3"

//#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)

extern int code(int encode, FILE* InFile, FILE* SrcFile, FILE* OutFile, int BufSize);

int Java_com_xperia64_rompatcher_MainActivity_xdelta3PatchRom(JNIEnv * env, jobject this, jstring romPath, jstring patchPath, jstring outputFile)
{
	jboolean isCopy;
	const char * szRomPath = (*env)->GetStringUTFChars(env, romPath, &isCopy);
	const char * szPatchPath = (*env)->GetStringUTFChars(env, patchPath, &isCopy);
	const char * szOutputFile = (*env)->GetStringUTFChars(env, outputFile, &isCopy);
	FILE*  InFile = fopen(szPatchPath, "rb");
	FILE*  SrcFile = fopen(szRomPath, "rb");
	FILE* OutFile = fopen(szOutputFile, "wb");
	int ret = code (0, InFile, SrcFile, OutFile, 0x1500);
	fclose(OutFile);
	fclose(SrcFile);
	fclose(InFile);

	(*env)->ReleaseStringUTFChars(env, romPath, szRomPath); 
	(*env)->ReleaseStringUTFChars(env, patchPath, szPatchPath); 
	(*env)->ReleaseStringUTFChars(env, outputFile, szOutputFile); 
	return ret;
}

int code (int encode, FILE* InFile, FILE* SrcFile, FILE* OutFile, int BufSize)
{
    int r, ret;
    xd3_stream stream;
    xd3_config config;
    xd3_source source;
    void* Input_Buf;
    int Input_Buf_Read;
    
    if (BufSize < XD3_ALLOCSIZE)
        BufSize = XD3_ALLOCSIZE;
    
    memset (&stream, 0, sizeof (stream));
    memset (&source, 0, sizeof (source));
    
    xd3_init_config(&config, XD3_ADLER32);
    config.winsize = BufSize;
    xd3_config_stream(&stream, &config);
    
    if (SrcFile)
    {
        source.blksize = BufSize;
        source.curblk = malloc(source.blksize);
        
        /* Load 1st block of stream. */
        r = fseek(SrcFile, 0, SEEK_SET);
        if (r)
            return r;
        source.onblk = fread((void*)source.curblk, 1, source.blksize, SrcFile);
        source.curblkno = 0;
        /* Set the stream. */
        xd3_set_source(&stream, &source);
    }
    
    Input_Buf = malloc(BufSize);
    
    fseek(InFile, 0, SEEK_SET);
    do
    {
        Input_Buf_Read = fread(Input_Buf, 1, BufSize, InFile);
        if (Input_Buf_Read < BufSize)
        {
            xd3_set_flags(&stream, XD3_FLUSH | stream.flags);
        }
        xd3_avail_input(&stream, Input_Buf, Input_Buf_Read);
        
    process:
        if (encode)
            ret = xd3_encode_input(&stream);
        else
            ret = xd3_decode_input(&stream);
        
        switch (ret)
        {
            case XD3_INPUT:
            {
                LOGE("XD3_INPUT\n");
                continue;
            }
                
            case XD3_OUTPUT:
            {
                LOGE("XD3_OUTPUT\n");
                r = fwrite(stream.next_out, 1, stream.avail_out, OutFile);
                if (r != (int)stream.avail_out)
                    return r;
                xd3_consume_output(&stream);
                goto process;
            }
                
            case XD3_GETSRCBLK:
            {
                LOGE("XD3_GETSRCBLK %qd\n", source.getblkno);
                if (SrcFile)
                {
                    r = fseek(SrcFile, source.blksize * source.getblkno, SEEK_SET);
                    if (r)
                        return r;
                    source.onblk = fread((void*)source.curblk, 1,
                                         source.blksize, SrcFile);
                    source.curblkno = source.getblkno;
                }
                goto process;
            }
                
            case XD3_GOTHEADER:
            {
                LOGE("XD3_GOTHEADER\n");
                goto process;
            }
                
            case XD3_WINSTART:
            {
                LOGE("XD3_WINSTART\n");
                goto process;
            }
                
            case XD3_WINFINISH:
            {
                LOGE("XD3_WINFINISH\n");
                goto process;
            }
                
            default:
            {
                LOGE("!!! INVALID %s %d !!!\n",
                         stream.msg, ret);
                return ret;
            }
                
        }
        
    }
    while (Input_Buf_Read == BufSize);
    
    free(Input_Buf);
    
    free((void*)source.curblk);
    xd3_close_stream(&stream);
    xd3_free_stream(&stream);
    
    return 0;
};

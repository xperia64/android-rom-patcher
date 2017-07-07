LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lzma

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/api
LOCAL_C_INCLUDES += $(LOCAL_PATH)/lzma
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/check
LOCAL_C_INCLUDES += $(LOCAL_PATH)/simple
LOCAL_C_INCLUDES += $(LOCAL_PATH)/delta
LOCAL_C_INCLUDES += $(LOCAL_PATH)/lz
LOCAL_C_INCLUDES += $(LOCAL_PATH)/rangecoder

LOCAL_SRC_FILES := \
	tuklib_physmem.c \
	tuklib_cpucores.c common/common.c \
	common/block_util.c common/easy_preset.c \
	common/filter_common.c \
	common/hardware_physmem.c \
	common/index.c common/stream_flags_common.c \
	common/vli_size.c \
	common/hardware_cputhreads.c common/alone_encoder.c \
	common/block_buffer_encoder.c \
	common/block_encoder.c \
	common/block_header_encoder.c common/easy_buffer_encoder.c \
	common/easy_encoder.c common/easy_encoder_memusage.c \
	common/filter_buffer_encoder.c common/filter_encoder.c \
	common/filter_flags_encoder.c \
	common/index_encoder.c \
	common/stream_buffer_encoder.c common/stream_encoder.c \
	common/stream_flags_encoder.c common/vli_encoder.c \
	common/outqueue.c common/stream_encoder_mt.c \
	common/alone_decoder.c \
	common/auto_decoder.c common/block_buffer_decoder.c \
	common/block_decoder.c \
	common/block_header_decoder.c common/easy_decoder_memusage.c \
	common/filter_buffer_decoder.c common/filter_decoder.c \
	common/filter_flags_decoder.c \
	common/index_decoder.c common/index_hash.c \
	common/stream_buffer_decoder.c common/stream_decoder.c \
	common/stream_flags_decoder.c \
	common/vli_decoder.c check/check.c \
	check/crc32_table.c \
	check/crc32_fast.c \
	check/crc64_table.c \
	check/crc64_fast.c \
	check/sha256.c lz/lz_encoder.c \
	lz/lz_encoder_mf.c lz/lz_decoder.c \
	lzma/lzma_encoder_presets.c \
	lzma/lzma_encoder.c \
	lzma/lzma_encoder_optimum_fast.c \
	lzma/lzma_encoder_optimum_normal.c lzma/fastpos_table.c \
	lzma/lzma_decoder.c lzma/lzma2_encoder.c \
	lzma/lzma2_decoder.c \
	rangecoder/price_table.c \
	delta/delta_common.c \
	delta/delta_encoder.c \
	delta/delta_decoder.c \
	simple/simple_coder.c \
	simple/simple_encoder.c \
	simple/simple_decoder.c \
	simple/x86.c simple/powerpc.c \
	simple/ia64.c simple/arm.c simple/armthumb.c simple/sparc.c

LOCAL_CFLAGS := -O2 -DHAVE_INTTYPES_H -DHAVE_STDINT_H -DHAVE_LIMITS_H -DHAVE_STDBOOL_H -std=c99 -DMYTHREAD_POSIX -DHAVE_COMMON_H -DHAVE_DECODER_LZMA1 -DHAVE_DECODER_LZMA2 -DHAVE_DECODER_DELTA

include $(BUILD_SHARED_LIBRARY)  

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := retro_dino

ifeq ($(TARGET_ARCH),arm)
  LOCAL_CFLAGS += -DANDROID_ARM
  LOCAL_ARM_MODE := arm
endif

ifeq ($(TARGET_ARCH),x86)
  LOCAL_CFLAGS += -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
  LOCAL_CFLAGS += -DANDROID_MIPS
endif

LOCAL_SRC_FILES += $(wildcard ../../../*.cpp) $(wildcard ../../../*/*.cpp) $(wildcard ../../../vorbis/*.c) $(wildcard ../../../ogg/*.c)
LOCAL_CPPFLAGS += -Wall -std=gnu++11 -fexceptions -DOV_EXCLUDE_STATIC_CALLBACKS -Wno-literal-suffix
LOCAL_CFLAGS += -O2 -ffast-math -D_GLIBCXX_HAS_GTHREADS -DANDROID
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../.. $(LOCAL_PATH)/../../../vorbis
LOCAL_LDLIBS += -lz -llog

include $(BUILD_SHARED_LIBRARY)


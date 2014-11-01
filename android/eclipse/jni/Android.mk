LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)


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

CORE_DIR := ../../..

LOCAL_MODULE := retro_dino

LOCAL_SRC_FILES = $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CPPFLAGS := -Wall -std=gnu++11 -fexceptions -DOV_EXCLUDE_STATIC_CALLBACKS -Wno-literal-suffix
LOCAL_CFLAGS := -O2 -ffast-math -D_GLIBCXX_HAS_GTHREADS -DANDROID
LOCAL_C_INCLUDES += $(INCFLAGS)
LOCAL_LDLIBS += -lz -llog

include $(BUILD_SHARED_LIBRARY)


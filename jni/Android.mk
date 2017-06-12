LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	LOCAL_CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

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

CORE_DIR := ../

LOCAL_MODULE := retro

include $(CORE_DIR)/Makefile.common

LOCAL_SRC_FILES = $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CPPFLAGS := -DHAVE_ZLIB -Wall -std=gnu++11 -fexceptions -DOV_EXCLUDE_STATIC_CALLBACKS -Wno-literal-suffix $(INCFLAGS) -std=c++11
LOCAL_CFLAGS := -O2 -ffast-math -DHAVE_ZLIB -D_GLIBCXX_HAS_GTHREADS -DANDROID $(INCFLAGS)
LOCAL_C_INCLUDES += $(INCFLAGS)
LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)


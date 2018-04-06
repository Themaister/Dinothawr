LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

HAVE_NEON := 0

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  HAVE_NEON := 1
endif

include $(CORE_DIR)/Makefile.common

COREFLAGS := -DHAVE_ZLIB -Wall -DOV_EXCLUDE_STATIC_CALLBACKS -ffast-math -D_GLIBCXX_HAS_GTHREADS -DANDROID $(INCFLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	LOCAL_CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_CXX) $(SOURCES_C) $(SOURCES_ASM)
LOCAL_CPPFLAGS     := -std=gnu++11 $(COREFLAGS)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_LDLIBS       := -lz -latomic
LOCAL_CPP_FEATURES := exceptions

ifeq ($(HAVE_NEON),1)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)


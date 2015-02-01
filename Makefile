DEBUG = 0
USE_CXX03 = 0

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

LIBRETRO := dinothawr
LIBDIR = $(DESTDIR)/usr/lib/libretro
ASSETDIR = $(DESTDIR)/usr/share/dinothawr

ifeq ($(USE_CXX03),1)
CXXFLAGS += -DUSE_CXX03
#note that this actually doesn't enable C++03 yet
CXX11 = -std=c++11
CXX0X = -std=c++0x
else
CXX11 = -std=c++11
CXX0X = -std=c++0x
endif

ifneq (,$(findstring unix,$(platform)))
   TARGET := $(LIBRETRO)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), osx)
   TARGET := $(LIBRETRO)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   LDFLAGS += -stdlib=libc++
   CXXFLAGS += $(CXX11) $(LDFLAGS)
   OSXVER = `sw_vers -productVersion | cut -d. -f 2`
   OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
ifeq ($(OSX_LT_MAVERICKS),"YES")
   fpic += -mmacosx-version-min=10.7
endif
ifndef ($(NOUNIVERSAL))
   CFLAGS += $(ARCHFLAGS)
   CXXFLAGS += $(ARCHFLAGS)
   LDFLAGS += $(ARCHFLAGS)
endif
else ifeq ($(platform), ios)
   TARGET := $(LIBRETRO)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   LDFLAGS += -stdlib=libc++

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcrun -sdk iphoneos -show-sdk-path)
endif

   CC = clang -arch armv7 -isysroot $(IOSSDK)
   CXX = clang++ -arch armv7 -isysroot $(IOSSDK)
   CXXFLAGS += $(CXX11) $(LDFLAGS)
   OSXVER = `sw_vers -productVersion | cut -d. -f 2`
   OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
ifeq ($(OSX_LT_MAVERICKS),"YES")
   CC += -miphoneos-version-min=5.0
   CXX += -miphoneos-version-min=5.0
   CXXFLAGS += -miphoneos-version-min=5.0
endif
else ifeq ($(platform), theos_ios)
TARGET_CXX=clang++
DEPLOYMENT_IOSVERSION = 5.0
TARGET = iphone:latest:$(DEPLOYMENT_IOSVERSION)
ARCHS = armv7 armv7s
TARGET_IPHONEOS_DEPLOYMENT_VERSION=$(DEPLOYMENT_IOSVERSION)
THEOS_BUILD_DIR := objs
include $(THEOS)/makefiles/common.mk

LIBRARY_NAME = $(TARGET_NAME)_libretro_ios

else ifeq ($(platform), qnx)
	# QNX
	TARGET := $(TARGET_NAME)_libretro_qnx.so
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=link.T
	CC = qcc -Vgcc_notarmv7le
	CXX = QCC -Vgcc_notarmv7le_cpp
	AR = QCC -Vgcc_ntoarmv7le
	CXXFLAGS += -D__BLACKBERRY_QNX__
	CXXFLAGS += -DARM

else ifeq ($(platform), wii)
   TARGET := $(TARGET_NAME)_libretro_wii.a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   PLATFORM_DEFINES += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
	STATIC_LINKING = 1
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_psp1.a
   CC = psp-gcc$(EXE_EXT)
   CXX = psp-g++$(EXE_EXT)
   AR = psp-ar$(EXE_EXT)
   PLATFORM_DEFINES += -DPSP -G0 -DLSB_FIRST -std=gnu++0x
	STATIC_LINKING = 1
else
   CC = gcc
   CXX = g++
   TARGET := $(LIBRETRO)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -static -s -Wl,--version-script=link.T -Wl,--no-undefined
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
   CFLAGS += -O0 -g
else
   CXXFLAGS += -O3 -DNDEBUG
   CFLAGS += -O3 -DNDEBUG
endif

ifneq ($(platform), osx)
ifneq ($(platform), ios)
ifneq ($(platform), theos_ios)
CXXFLAGS += $(CXX0X)
endif
endif
endif

CORE_DIR := .

include Makefile.common

HEADERS := $(INCFLAGS)
OBJECTS := $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)
CXXFLAGS += -ffast-math -Wall -pedantic $(fpic) -I. -DOV_EXCLUDE_STATIC_CALLBACKS
CFLAGS += -ffast-math $(fpic) -I. -Ivorbis

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

ifeq ($(platform), theos_ios)
COMMON_FLAGS := -DIOS $(COMMON_DEFINES) $(INCFLAGS) -I$(THEOS_INCLUDE_PATH) -Wno-error
$(LIBRARY_NAME)_CFLAGS += $(CFLAGS)  $(COMMON_FLAGS)
$(LIBRARY_NAME)_CPPFLAGS += $(CXXFLAGS) $(COMMON_FLAGS)
${LIBRARY_NAME}_FILES = $(SOURCES_CXX) $(SOURCES_C)
${LIBRARY_NAME}_LIBRARIES = z
ADDITIONAL_CCFLAGS = -std=c++11 -stdlib=libc++
ADDITIONAL_LDFLAGS = -std=c++11 -stdlib=libc++
include $(THEOS_MAKE_PATH)/library.mk
else
all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) $(fpic) $(SHARED) $(LDFLAGS) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS) -lm -lz
endif

clean:
	rm -f $(OBJECTS) $(TARGET)

install: all
	mkdir -p $(LIBDIR) || /bin/true
	install -m755 $(TARGET) $(LIBDIR)/dinothawr_libretro.so
	install -d -m755 $(ASSETDIR)
	cp -r dinothawr/* $(ASSETDIR)

.PHONY: clean install
endif

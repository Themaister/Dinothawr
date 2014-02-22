DEBUG = 0

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

ifneq (,$(findstring unix,$(platform)))
   TARGET := $(LIBRETRO)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), osx)
   TARGET := $(LIBRETRO)_libretro.dylib
   fpic := -fPIC -mmacosx-version-min=10.7
   SHARED := -dynamiclib
   LDFLAGS += -stdlib=libc++
   CXXFLAGS += -std=c++11 $(LDFLAGS)
else ifeq ($(platform), ios)
   TARGET := $(LIBRETRO)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   LDFLAGS += -stdlib=libc++
   CC = clang -arch armv7 -isysroot $(IOSSDK) -miphoneos-version-min=5.0
   CXX = clang++ -arch armv7 -isysroot $(IOSSDK) -miphoneos-version-min=5.0
   CXXFLAGS += -std=c++11 $(LDFLAGS) -miphoneos-version-min=5.0
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
   CXXFLAGS += -O3
   CFLAGS += -O3
endif

ifneq ($(platform), osx)
ifneq ($(platform), ios)
CXXFLAGS += -std=gnu++0x
CFLAGS += -std=gnu99
endif
endif

HEADERS := $(wildcard *.hpp) $(wildcard */*.hpp) $(wildcard vorbis/*.h) $(wildcard ogg/*.h)
SOURCES := $(wildcard *.cpp) $(wildcard */*.cpp)
CSOURCES := $(wildcard ogg/*.c) $(wildcard vorbis/*.c)
OBJECTS := $(SOURCES:.cpp=.o) $(CSOURCES:.c=.o)
CXXFLAGS += -ffast-math -Wall -pedantic $(fpic) -I. -DOV_EXCLUDE_STATIC_CALLBACKS
CFLAGS += -ffast-math $(fpic) -I. -Ivorbis

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) $(fpic) $(SHARED) $(LDFLAGS) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS) -lm -lz
endif

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

install: all
	mkdir -p $(LIBDIR) || /bin/true
	install -m755 $(TARGET) $(LIBDIR)/dinothawr_libretro.so
	install -d -m755 $(ASSETDIR)
	cp -r dinothawr/* $(ASSETDIR)

.PHONY: clean install


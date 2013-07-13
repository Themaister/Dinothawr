
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

ifeq ($(platform), unix)
   TARGET := $(LIBRETRO)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CXXFLAGS += $(shell pkg-config vorbisfile --cflags)
   LIBS += $(shell pkg-config vorbisfile --libs)
else ifeq ($(platform), osx)
   TARGET := $(LIBRETRO)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   CXXFLAGS += $(shell pkg-config vorbisfile --cflags)
   LIBS += $(shell pkg-config vorbisfile --libs)
else
   CXX = g++
   TARGET := $(LIBRETRO)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -static -s -Wl,--version-script=link.T -Wl,--no-undefined
   LIBS += -lvorbisfile -lvorbis -logg -lz 
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
else
   CXXFLAGS += -O3
endif

HEADERS := $(wildcard *.hpp) $(wildcard */*.hpp)
SOURCES := $(wildcard *.cpp) $(wildcard */*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
CXXFLAGS += -std=gnu++11 -Wall -pedantic $(fpic)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS) -lm -lz

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

install: all
	mkdir -p $(LIBDIR) || /bin/true
	install -m755 $(TARGET) $(LIBDIR)/libretro-dinothawr.so
	install -d -m755 $(ASSETDIR)
	cp -r dinothawr/* $(ASSETDIR)

.PHONY: clean install


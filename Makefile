
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

LIBDIR = /usr/lib/libretro
ASSETDIR = /usr/share/dinothawr

ifeq ($(platform), unix)
   TARGET := libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), osx)
   TARGET := libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else
   CC = gcc
   TARGET := retro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -static -s -Wl,--version-script=link.T -Wl,--no-undefined
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
else
   CXXFLAGS += -O3
endif

HEADERS := $(wildcard *.hpp) $(wildcard */*.hpp)

SOURCES := $(wildcard *.cpp) $(wildcard */*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
CXXFLAGS += -std=gnu++0x -Wall -pedantic $(fpic)

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
	cp -r maps/* $(ASSETDIR)

.PHONY: clean install


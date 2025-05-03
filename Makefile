
RAYLIB_PATH ?= ../raylib
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))
PLATFORM := DESKTOP
MODE := DEBUG

INCLUDE_DIRS := -I./ -I./include/ -I$(RAYLIB_PATH)/src
LDFLAGS := -L$(RAYLIB_PATH)/src
LDLIBS := -lraylib
CFLAGS := -std=c99 -Wall

BUILD_DIR ?= ./build
OUT_NAME := rafd
OUT_EXT := exe

release: MODE := RELEASE
web: MODE := RELEASE
web: PLATFORM := WEB

ifeq ($(PLATFORM), DESKTOP)
  LDLIBS := $(LDLIBS) -lgdi32 -lopengl32 -lwinmm
  ifeq ($(MODE), DEBUG)
    CFLAGS := $(CFLAGS) -g -O0
  else
    CFLAGS := $(CFLAGS) -O3 
  endif
endif

$(BUILD_DIR)/$(OUT_NAME).$(OUT_EXT): $(OBJECTS) 
ifeq (, $(wildcard $(BUILD_DIR)))
	mkdir $(BUILD_DIR)
	cp -r fonts $(BUILD_DIR)
	cp -r shaders $(BUILD_DIR)
	cp def.afdd $(BUILD_DIR)
endif
	clang $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	clang $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

run: $(BUILD_DIR)/$(OUT_NAME).$(OUT_EXT)
	$^

clean:
	-rm *.o
	-rm $(OUT_NAME).exe

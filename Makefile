RAYLIB_PATH ?= ../raylib
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

INCLUDE_DIRS := -I./ -I./include/ -I$(RAYLIB_PATH)/src
LDFLAGS := -L$(RAYLIB_PATH)/src
LDLIBS := -lraylib
CFLAGS := -std=c99 -Wall -Wextra

BUILD_DIR ?= ./build
OUT_NAME := rafd
OUT_EXT := exe

$(BUILD_DIR)/$(OUT_NAME).$(OUT_EXT): debug

debug: CFLAGS += -g -O0 -DDEBUG
release: CFLAGS += -O3
debug release: LDLIBS += -lgdi32 -lopengl32 -lwinmm
debug release: CFLAGS += -DPLATFORM_DESKTOP
debug release: $(OBJECTS) 
ifeq (, $(wildcard $(BUILD_DIR)))
	mkdir $(BUILD_DIR)
	cp -r fonts $(BUILD_DIR)
	cp -r shaders $(BUILD_DIR)
	cp def.afdd $(BUILD_DIR)
endif
	clang $(CFLAGS) -o $(BUILD_DIR)/$(OUT_NAME).$(OUT_EXT) $^ $(LDFLAGS) $(LDLIBS)

web: CFLAGS += -O3 -DPLATFORM_WEB
web: OUT_EXT := html
web:
	echo "todo"

%.o: %.c
	clang $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

run: $(BUILD_DIR)/$(OUT_NAME).$(OUT_EXT)
	$^

clean:
	-rm *.o
ifneq (, $(wildcard $(BUILD_DIR)))
	-rm -r $(BUILD_DIR)
endif

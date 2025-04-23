
RAYLIB_PATH ?= ../raylib
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))
INCLUDE_DIRS := -I./ -I./include/ -I$(RAYLIB_PATH)/src
LDFLAGS := -L$(RAYLIB_PATH)/src
LDLIBS := -lraylib -lgdi32 -lopengl32 -lwinmm
CFLAGS := -g -O0 -std=c99 
EXE_NAME := rafd

$(EXE_NAME): $(OBJECTS) 
	clang $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	clang $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

run: $(EXE_NAME)
	$^

clean:
	-rm *.o
	-rm $(EXE_NAME).exe

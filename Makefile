
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

main: $(OBJECTS) 
	clang -g -O0 -o $@ $^

%.o: %.c
	clang -g -O0 -c -o $@ $<

run: main
	$^

clean:
	rm main.exe
	rm *.o

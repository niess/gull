CFLAGS := -O0 -g -std=c99 -pedantic -fPIC -Wall
LIBS := -lm

.PHONY: example lib clean

lib: lib/libgull.so
	@rm -f *.o
	
clean:
	@rm -rf example lib *.o
	
example: src/example.c
	@gcc -o $@ $(CFLAGS) $< -Llib -lgull

lib/lib%.so: src/%.c include/%.h
	@mkdir -p lib
	@gcc -o $@ $(CFLAGS) -shared $< $(LIBS)

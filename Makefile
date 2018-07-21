CFLAGS := -O2 -std=c99 -pedantic -fPIC -Wall
INCLUDE := -Iinclude
LIBS := -lm

.PHONY: examples lib clean

lib: lib/libgull.so
	@rm -f *.o

clean:
	@rm -rf bin lib *.o

examples: bin/example-basic

lib/lib%.so: src/%.c include/%.h
	@mkdir -p lib
	@gcc -o $@ $(CFLAGS) $(INCLUDE) -shared $< $(LIBS)

bin/example-%: examples/example-%.c lib
	@mkdir -p bin
	@gcc -o $@ $(CFLAGS) $(INCLUDE) $< -Llib -Wl,-rpath $(PWD)/lib -lgull

CFLAGS = -O3 -std=c99 -pedantic -Wall
INCLUDE = -Iinclude
LIBS = -lm

SOEXT = so
SYS = $(shell uname -s)
ifeq ($(SYS), Darwin)
	SOEXT = dylib
endif

.PHONY: examples lib clean

lib: lib/libgull.$(SOEXT)
	@rm -f *.o

clean:
	@rm -rf bin lib *.o

examples: bin/example-basic

SHARED = -shared
RPATH  = '-Wl,-rpath,$$ORIGIN/../lib'
ifeq ($(SYS), Darwin)
	SHARED = -dynamiclib -Wl,-install_name,@rpath/libgull.$(SOEXT)
	RPATH  = -Wl,-rpath,@loader_path/../lib
endif

lib/lib%.$(SOEXT): src/%.c include/%.h
	@mkdir -p lib
	@gcc -o $@ $(CFLAGS) -fPIC $(INCLUDE) $(LDFLAGS) $(SHARED) $< $(LIBS)

bin/example-%: examples/example-%.c lib
	@mkdir -p bin
	@gcc -o $@ $(CFLAGS) $(INCLUDE) $< -Llib $(RPATH) -lgull

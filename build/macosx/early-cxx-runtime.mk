# Compiler-support runtime only; this does not build all of libstdc++.
ZR_PREFIX = /opt/mac/bin/powerpc-apple-darwin8-
ZR_GCC = /opt/mac/lib/gcc/powerpc-apple-darwin8/4.0.1
ZR_SUPC = $(ZR_RUNTIME_SOURCE)/libstdc++-v3/libsupc++
ZR_FLAGS = -isystem $(ZR_GCC)/include -isysroot $(ZR_MACOS_SDK) \
  -mmacosx-version-min=10.0 -mcpu=G3 -mno-altivec -mlong-double-64 -fPIC -O2
ZR_OBJECTS = $(patsubst $(ZR_SUPC)/%.cc,%.o,$(wildcard $(ZR_SUPC)/*.cc)) cp-demangle.o

.PHONY: all
all: libsupc++-10.0.a

%.o: $(ZR_SUPC)/%.cc
	$(ZR_PREFIX)g++ $(ZR_FLAGS) -nostdinc++ -I$(ZR_SUPC) \
	  -I$(ZR_RUNTIME_HEADERS) -I$(ZR_RUNTIME_HEADERS)/powerpc-apple-darwin8 \
	  -I$(ZR_RUNTIME_SOURCE)/gcc -c $< -o $@

cp-demangle.o: $(ZR_RUNTIME_SOURCE)/libiberty/cp-demangle.c
	$(ZR_PREFIX)gcc $(ZR_FLAGS) -DIN_GLIBCPP_V3 -DHAVE_STDLIB_H -DHAVE_STRING_H \
	  -I$(ZR_RUNTIME_SOURCE)/include -I$(ZR_RUNTIME_SOURCE)/libiberty -c $< -o $@

libsupc++-10.0.a: $(ZR_OBJECTS)
	$(ZR_PREFIX)ar cr $@ $(ZR_OBJECTS)
	$(ZR_PREFIX)ranlib $@

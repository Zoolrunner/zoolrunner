# Matching GCC 4 standard-library sources, configured for early Darwin.
ZR_PREFIX = /opt/mac/bin/powerpc-apple-darwin8-
ZR_GCC = /opt/mac/lib/gcc/powerpc-apple-darwin8/4.0.1
ZR_FLAGS = -isystem $(ZR_GCC)/include -isysroot $(ZR_MACOS_SDK) \
  -mmacosx-version-min=10.0 -mcpu=G3 -mno-altivec -mlong-double-64 -fPIC -O2
ZR_CXXFLAGS = $(ZR_FLAGS) -nostdinc++ \
  -I$(ZR_STDLIB_OUTPUT)/include -I$(ZR_STDLIB_OUTPUT)/include/powerpc-apple-darwin8 \
  -I$(ZR_STDLIB_OUTPUT)/include/backward -I$(ZR_STDLIB_SOURCE)/src \
  -I$(ZR_STDLIB_SOURCE)/config/locale/generic -fno-implicit-templates
ZR_SOURCES = $(wildcard $(ZR_STDLIB_SOURCE)/src/*.cc) \
  $(wildcard $(ZR_STDLIB_SOURCE)/config/locale/generic/*.cc) \
  $(ZR_STDLIB_SOURCE)/config/io/basic_file_stdio.cc
ZR_OBJECTS = $(notdir $(ZR_SOURCES:.cc=.o)) atomicity.o eprintf.o supc-merged.o
vpath %.cc $(ZR_STDLIB_SOURCE)/src $(ZR_STDLIB_SOURCE)/config/locale/generic \
  $(ZR_STDLIB_SOURCE)/config/io

.PHONY: all
all: libzoolcxx.dylib libgcc-10.0.a

%.o: %.cc
	$(ZR_PREFIX)g++ $(ZR_CXXFLAGS) -c $< -o $@

concept-inst.o: $(ZR_STDLIB_SOURCE)/src/concept-inst.cc
	$(ZR_PREFIX)g++ $(ZR_CXXFLAGS) -D_GLIBCXX_CONCEPT_CHECKS -fimplicit-templates -c $< -o $@

atomicity.cc: $(ZR_STDLIB_SOURCE)/config/cpu/powerpc/atomicity.h
	cp $< $@

eprintf.o: $(ZR_SCRIPTS)/compat/early-eprintf.c
	$(ZR_PREFIX)gcc $(ZR_FLAGS) -c $< -o $@

supc-merged.o: $(ZR_STDLIB_OUTPUT)/libsupc++-10.0.a
	$(ZR_PPC_EARLY_LINKER) -r -arch ppc -all_load $< -o $@

libgcc-10.0.a: eprintf.o
	cp $(ZR_GCC)/libgcc.a $@
	$(ZR_PREFIX)ar d $@ _eprintf.o
	$(ZR_PREFIX)ar r $@ eprintf.o
	$(ZR_PREFIX)ranlib $@

libzoolcxx.dylib: $(ZR_OBJECTS)
	$(ZR_PPC_EARLY_LINKER) -dynamic -dylib -single_module -flat_namespace \
	  -arch ppc -macosx_version_min 10.0 -syslibroot $(ZR_MACOS_SDK) \
	  -install_name @executable_path/libzoolcxx.dylib \
	  /opt/mac/lib/zoolrunner-cheetah/dylib1.o $(ZR_OBJECTS) \
	  $(ZR_GCC)/libgcc_eh.a $(ZR_GCC)/libgcc.a \
	  -L$(ZR_MACOS_SDK)/usr/lib -lSystem -o $@

AUTOMAKE_OPTIONS=gnu

if BUILD_JAVA_INTERFACE
lib_LTLIBRARIES = libfastbitjni.la
libfastbitjni_la_SOURCES = FastBit.c
include_HEADERS = FastBit.h ../src/capi.h
libfastbitjni_la_DEPENDENCIES = ../src/libfastbit.la
libfastbitjni_la_LIBADD = ../src/libfastbit.la
endif
#
../src/libfastbit.la::
	cd ../src && $(MAKE) libfastbit.la
#
clean: clean-am
	rm -f *~ *.class core core.[0-9]*
	rm -rf tmp

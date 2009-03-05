AUTOMAKE_OPTIONS=gnu
EXTRA_DIST=README build.xml milky.java \
gov/lbl/fastbit/FastBit.java \
gov/lbl/fastbit/FastBitStringReader.java \
gov/lbl/fastbit/FastBitStringReaderException.java \
gov/lbl/fastbit/FastBitStringWriter.java \
gov/lbl/fastbit/FastBitStringWriterException.java \
tests/lib/commons-logging.jar \
tests/lib/junit-4.4.jar \
tests/lib/log4j-1.2.15.jar \
tests/README \
tests/log4j.properties \
tests/TestFastBitJava.java

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

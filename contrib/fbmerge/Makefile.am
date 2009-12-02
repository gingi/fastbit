AUTOMAKE_OPTIONS=gnu
EXTRA_DIST = fbmerge.sln fbmerge.vcproj

#if CONFIG_CONTRIB
bin_PROGRAMS = fbmerge
fbmerge_SOURCES = fbmerge.cpp
fbmerge_DEPENDENCIES = ../../src/libfastbit.la
fbmerge_LDADD = ../../src/libfastbit.la
#endif
#
../../src/libfastbit.la::
	cd ../../src && $(MAKE) libfastbit.la
distclean: distclean-am
	-rm -rf ./$(DEPDIR) .ibis
	-rm -f Makefile

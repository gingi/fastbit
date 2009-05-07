AUTOMAKE_OPTIONS=gnu
ACLOCAL_AMFLAGS=-I tests/m4
SUBDIRS = src examples java tests

EXTRA_DIST = win/README win/Win.mak win/MinGW.mak win/ardea.vcproj \
	win/ibis.sln win/ibis.vcproj win/thula.vcproj win/tcapi.vcproj \
	win/trydll.cpp win/dll.vcproj win/trydll.vcproj win/java.vcproj \
	win/FlexLexer.h

dist_doc_DATA = doc/dataDistributions.html \
	doc/index.html doc/quickstart.html doc/dataLoading.html \
	doc/ibisCommandLine.html doc/indexSpec.html doc/footer.html

docs:
	cd src && $(MAKE) docs

# three levels of thoroughness are available for checking the
# correctness of FastBit functions: quick-check, check, full-check.
# The normal check may take about ten minutes on a typical Linux machine.
more-check:
	cd tests && $(MAKE) more-check

full-check:
	cd tests && $(MAKE) full-check

clean: clean-recursive
	-rm -f win/*.suo win/*.ncb win/fastbit.def win/*.obj win/*.lib win/*.exe win/*.exe.*
	-rm -fr autom4t4.cache doc/html doc/latex win/Debug win/Release win/dll win/*.vcproj.*

distclean: distclean-recursive
	-rm -f $(am__CONFIG_DISTCLEAN_FILES) Makefile win/*.suo win/*.ncb win/*.obj win/*.exe win/*.exe.*
	-rm -fr doc/html doc/latex win/Debug win/Release win/dll win/*.vcproj.*

maintainer-clean: maintainer-clean-am
	-rm -rf ./$(DEPDIR) autom4te.cache
	-rm -f Makefile

#dist-hook:
#	-rm -rf `find $(distdir) \( -name CVS -o \name .svn \)`

.PHONY: docs more-check full-check

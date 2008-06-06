AUTOMAKE_OPTIONS=gnu
SUBDIRS = src examples java tests
windir = $(prefix)/share/win
javadir = $(prefix)/share/java
dist_win_DATA = win/README win/Win.mak win/MinGW.mak win/ardea.vcproj \
	win/ibis.sln win/ibis.vcproj win/thula.vcproj win/tcapi.vcproj \
	win/trydll.cpp win/dll.vcproj win/trydll.vcproj win/java.vcproj
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
	-rm -f win/ibis.suo win/ibis.ncb win/fastbit.def
	-rm -fr doc/html doc/latex win/Debug win/Release win/dll

distclean: distclean-recursive
	-rm -f $(am__CONFIG_DISTCLEAN_FILES) Makefile win/ibis.suo win/ibis.ncb
	-rm -fr doc/html doc/latex win/Debug win/Release

maintainer-clean: maintainer-clean-am
	-rm -rf ./$(DEPDIR) autom4te.cache
	-rm -f Makefile

.PHONY: docs all-tests

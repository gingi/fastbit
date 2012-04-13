AUTOMAKE_OPTIONS=gnu
ACLOCAL_AMFLAGS=-I tests/m4

if BUILD_CONTRIB
   EXTRA_BUILD=contrib
endif
SUBDIRS = src examples java tests $(EXTRA_BUILD)

bin_SCRIPTS = tests/scripts/fastbit-config

EXTRA_DIST = runconfig tests/scripts/fastbit-config.in \
	win/README win/Win.mak win/MinGW.mak win/xMinGW.mak win/ibis.sln \
	win/ibis.vcproj win/thula.vcproj win/tcapi.vcproj \
	win/trydll.cpp win/dll.vcproj win/trydll.vcproj win/java.vcproj \
	win/ardea.vcproj win/setqgen.vcproj win/rara.vcproj win/FlexLexer.h \
	win/static_pthread_init.c win/static_pthread.patch \
	contrib/README contrib/Makefile.am contrib/Makefile.in \
	contrib/fbmerge/fbmerge.cpp \
	contrib/fbmerge/Makefile.am contrib/fbmerge/Makefile.in \
	contrib/fbmerge/fbmerge.sln contrib/fbmerge/fbmerge.vcproj

dist_doc_DATA = doc/dataDistributions.html doc/header.html doc/contact.html \
	doc/index.html doc/quickstart.html doc/dataLoading.html doc/rara.html \
	doc/ibisCommandLine.html doc/indexSpec.html doc/footer.html

docs:
	cd src && $(MAKE) docs

# three levels of thoroughness are available for checking the
# correctness of FastBit functions: check, more-check, and full-check.
# The normal check may take about a minute on a typical Linux machine,
# more-check takes about an hour, while full-check takes several hours.
more-check:
	cd tests && $(MAKE) more-check

full-check:
	cd tests && $(MAKE) full-check

clean-local:
	-rm -f win/*.suo win/*.ncb win/*/*.suo win/*/*.ncb win/*.obj win/*.lib win/*.exe win/*.exe.* win/*/*.obj win/*/*.lib win/*/*.exe win/*/*.exe.* win/fastbit.def
	-rm -fr autom4te.cache doc/html doc/latex win/Debug win/Release win/*/Debug win/*/Release win/dll win/*.vcproj.* win/*/*.vcproj.*

distclean-local:
	-rm -f win/*.suo win/*.ncb win/*.obj win/*.exe win/*.exe.* win/*/*.suo win/*/*.ncb win/*/*.obj win/*/*.exe win/*/*.exe.*
	-rm -fr autom4te.cache doc/html doc/latex win/Debug win/Release win/*/Debug win/*/Release win/dll win/*.vcproj.* win/*/*.vcproj.*

.PHONY: docs more-check full-check

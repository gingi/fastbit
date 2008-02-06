AUTOMAKE_OPTIONS=gnu

lib_LTLIBRARIES = libfastbit.la
libfastbit_la_SOURCES = array_t.cpp part.cpp ibin.cpp query.cpp index.cpp column.cpp meshQuery.cpp bord.cpp party.cpp predicate.tab.cpp ixambit.cpp bitvector.cpp bitvector64.cpp irange.cpp fileManager.cpp mensa.cpp irelic.cpp parti.cpp iroster.cpp colValues.cpp ixpack.cpp category.cpp ixpale.cpp bundle.cpp qExpr.cpp predicate.yy.cpp ixzone.cpp ixbylt.cpp tafel.cpp util.cpp icegale.cpp ixzona.cpp ixfuzz.cpp ixfuge.cpp isbiad.cpp ifade.cpp idbak2.cpp isapid.cpp icentre.cpp islice.cpp imesa.cpp idirekte.cpp idbak.cpp capi.cpp ikeywords.cpp icmoins.cpp resource.cpp rids.cpp
libfastbit_la_LDFLAGS = -versioninfo $(LIB_VERSION_INFO)

include_HEADERS = config.h array_t.h bitvector.h bitvector64.h bord.h bundle.h capi.h category.h colValues.h column.h const.h fileManager.h horometer.h ibin.h ibis.h idirekte.h ikeywords.h index.h irelic.h iroster.h mensa.h meshQuery.h part.h predicate.h predicate.tab.h qExpr.h query.h resource.h rids.h tab.h table.h tafel.h twister.h util.h

dist_data_DATA=predicate.l predicate.y Doxyfile

# use a useless print statement to prevent the use of implicit rule
# to generating executable named predicate
predicate: predicate.tab.h predicate.tab.cpp predicate.yy.cpp
	@echo Updated predicate.tab.cpp and predicate.yy.cpp

docs: Doxyfile
	perl -pi -e 's/(PROJECT_NUMBER\s*=\s*).+/\1$(PACKAGE_VERSION)/' Doxyfile
	doxygen
# targets that are not files
.PHONY: docs
# # rules to generate .h and .cpp files from predicate.y and predicate.l
predicate.h: predicate.tab.h predicate.yy.cpp
# using flex and bison -- must use bison 1.875b or newer
predicate.tab.cpp: predicate.tab.h
	touch predicate.tab.cpp
predicate.tab.h: predicate.y
	bison -d -b predicate predicate.y
	echo '///' >> predicate.tab.h
	echo '/// Grammar of query conditions.' >> predicate.tab.h
	sed -e 's/predicate.tab.c/predicate.tab.cpp/' predicate.tab.c > predicate.tab.cpp
	rm predicate.tab.c
predicate.yy.cpp: predicate.l
	flex predicate.l
	sed -e 's/lex.yy.c/predicate.yy.cpp/' lex.yy.c > predicate.yy.cpp
	rm lex.yy.c


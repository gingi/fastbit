AUTOMAKE_OPTIONS=gnu

lib_LTLIBRARIES = libfastbit.la
libfastbit_la_SOURCES = parth3d.cpp parth3da.cpp parth3db.cpp parth3dw.cpp parth2d.cpp parth.cpp part.cpp array_t.cpp ibin.cpp query.cpp utilidor.cpp index.cpp column.cpp meshQuery.cpp bord.cpp party.cpp whereParser.cc ixambit.cpp bitvector.cpp bitvector64.cpp irange.cpp fileManager.cpp mensa.cpp irelic.cpp parti.cpp iroster.cpp colValues.cpp ixpack.cpp category.cpp ixpale.cpp bundle.cpp qExpr.cpp whereLexer.cc ixzone.cpp ixbylt.cpp tafel.cpp util.cpp icegale.cpp ixzona.cpp ixfuzz.cpp ixfuge.cpp isbiad.cpp ifade.cpp idbak2.cpp isapid.cpp icentre.cpp islice.cpp imesa.cpp joinin.cpp idirekte.cpp idbak.cpp capi.cpp ikeywords.cpp icmoins.cpp resource.cpp rids.cpp countQuery.cpp whereClause.cpp selectClause.cpp selectParser.cc selectLexer.cc
libfastbit_la_LDFLAGS = -version-info $(LIB_VERSION_INFO)

include_HEADERS = fastbit-config.h array_t.h bitvector.h bitvector64.h bord.h bundle.h capi.h category.h colValues.h column.h const.h countQuery.h fileManager.h horometer.h ibin.h ibis.h idirekte.h ikeywords.h index.h irelic.h iroster.h join.h joinin.h mensa.h meshQuery.h part.h whereClause.h whereLexer.h whereParser.hh qExpr.h query.h resource.h rids.h tab.h table.h tafel.h twister.h util.h utilidor.h location.hh position.hh stack.hh selectClause.h selectLexer.h selectParser.hh

EXTRA_DIST=whereLexer.ll whereParser.yy selectLexer.ll selectParser.yy Doxyfile

# use a useless print statement to prevent the use of implicit rule
# to generating executable named predicate
parsers: whereLexer.cc whereParser.cc selectLexer.cc selectParser.cc
	@echo Updated selectLexer.cc selectParser.cc whereLexer.cc and whereParser.cc

docs: Doxyfile
	perl -pi -e 's/(PROJECT_NUMBER\s*=\s*).+/\1$(PACKAGE_VERSION)/' Doxyfile
	doxygen
# targets that are not files
.PHONY: docs
# # rules to generate .h and .cpp files from predicate.y and predicate.l
location.hh position.hh stack.hh: whereParser.hh
whereParser.cc: whereParser.hh
	touch whereParser.cc
whereParser.hh: whereParser.yy
	bison -d -o whereParser.cc whereParser.yy
whereLexer.cc: whereLexer.ll whereParser.hh
	flex -o whereLexer.cc whereLexer.ll
selectParser.cc: selectParser.hh
	touch selectParser.cc
selectParser.hh: selectParser.yy
	bison -d -o selectParser.cc selectParser.yy
selectLexer.cc: selectLexer.ll selectParser.hh
	flex -o selectLexer.cc selectLexer.ll


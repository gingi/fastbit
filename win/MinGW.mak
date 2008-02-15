# $Id: MinGW.mak,v 1.1 2008/02/06 18:10:05 kewu Exp $
# Makefile for mingw32-make on windows using MinGW g++ port
#
CXX=g++.exe
OPT=-g -O0
INC=-I ../src -I "pthreads-w32-2-8-0-release"
DEF=-DFILEMANAGER_UNLOAD_TIME=3
LIB=-Lpthreads-w32-2-8-0-release -lpthreadGC2 -lm

CCFLAGS=$(DEF) $(INC) $(OPT)
#
OBJ =  array_t.o \
 bitvector.o \
 bitvector64.o \
 bundle.o \
 capi.o \
 category.o \
 colValues.o \
 column.o \
 fileManager.o \
 ibin.o \
 bord.o \
 tafel.o \
 mensa.o \
 party.o \
 part.o \
 parti.o \
 icegale.o \
 icentre.o \
 icmoins.o \
 idbak.o \
 idbak2.o \
 idirekte.o \
 ifade.o \
 ikeywords.o \
 imesa.o \
 index.o \
 irange.o \
 irelic.o \
 iroster.o \
 isapid.o \
 isbiad.o \
 islice.o \
 ixambit.o \
 ixbylt.o \
 ixfuge.o \
 ixfuzz.o \
 ixpack.o \
 ixpale.o \
 ixzona.o \
 ixzone.o \
 meshQuery.o \
 predicate.tab.o \
 predicate.yy.o \
 qExpr.o \
 query.o \
 resource.o \
 rids.o \
 util.o

#
ibis: ibis.exe
all: ibis.exe ardea.exe thula.exe

lib: libfastbit.a
libfastbit.a: $(OBJ)
	ar ruv libfastbit.a $(OBJ)

ibis.exe: ibis.o libfastbit.a
	$(CXX) -o $@ ibis.o libfastbit.a $(LIB)

thula: thula.exe
thula.exe: thula.o libfastbit.a
	$(CXX) $(LIB) -o $@ thula.o libfastbit.a $(LIB)

ardea: ardea.exe
ardea.exe: ardea.o libfastbit.a
	$(CXX) $(LIB) -o $@ ardea.o libfastbit.a $(LIB)

dll: fastbit.dll
fastbit.a: fastbit.dll
fastbit.dll: $(FRC)
	make -f MinGW.mak DEF="$(DEF) -D_USRDLL -DDLL_EXPORTS" $(OBJ)
	$(CXX) -shared -o $@ $(OBJ) $(LIB)
	dlltool -z fastbit.def $(OBJ)
	dlltool -k --dllname fastbit.dll --output-lib fastbit.a --def fastbit.def
# -Wl,-soname,$@
trydll: trydll.exe
trydll.exe: trydll.cpp fastbit.dll
	$(CXX) $(CCFLAGS) -D_USRDLL -o $@ trydll.cpp fastbit.a $(LIB)

tcapi: tcapi.exe
tcapi.exe: ../examples/tcapi.c ../src/capi.h fastbit.dll
	$(CXX) $(CCFLAGS) -D_USRDLL -o $@ ../examples/tcapi.c fastbit.a $(LIB)

clean:
	rm -f *.o core b?_? *.dll *.lib *.exe *.a *.so *.suo *.ncb *.exp *.pdb
clean-all: clean
	rm -rf Debug Release dll tmp pthreads-w32-2-8-0-release pthreads-w32-2-8-0-release.tar.gz

pthreads-w32: pthreads-w32-2-8-0-release/libpthreadGC2.a
pthreads-w32-2-8-0-release.tar.gz:
	wget ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
pthreads-w32-2-8-0-release: pthreads-w32-2-8-0-release.tar.gz
	tar xzf pthreads-w32-2-8-0-release.tar.gz
pthreads-w32-2-8-0-release/libpthreadGC2.a: pthreads-w32-2-8-0-release
	cd pthreads-w32-2-8-0-release && make clean PTW32_FLAGS=-DPTW32_BUILD GC

force:

#suffixes
.SUFFIXES: .o .cpp .h
# #
# # rules to generate .h and .cpp files from predicate.y and predicate.l
# predicate: predicate.tab.h predicate.tab.cpp predicate.yy.cpp
# predicate.tab.cpp predicate.tab.h: predicate.y
# 	yacc -d -b predicate predicate.y
# 	mv predicate.tab.c predicate.tab.cpp
# #	touch predicate.tab.cpp
# predicate.yy.cpp: predicate.l
# 	lex predicate.l
# 	sed -e 's/^yylex/int yylex/' lex.yy.c > predicate.yy.cpp
# 	rm lex.yy.c
# predicate.h: predicate.tab.h predicate.yy.cpp
# suffixes based rules
.cpp.o:
	$(CXX) $(CCFLAGS) -c $<
############################################################
# dependencies generated with g++ -MM
ardea.o: ../examples/ardea.cpp ../src/table.h ../src/const.h ../src/resource.h ../src/util.h
	$(CXX) $(CCFLAGS) -c -o ardea.o ../examples/ardea.cpp
array_t.o: ../src/array_t.cpp ../src/array_t.h ../src/fileManager.h ../src/util.h ../src/const.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o array_t.o ../src/array_t.cpp
bit64.o: bit64.cpp ../src/bitvector64.h ../src/array_t.h ../src/fileManager.h ../src/util.h ../src/const.h \
  ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o bit64.o bit64.cpp
bitvector.o: ../src/bitvector.cpp ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/util.h \
  ../src/const.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o bitvector.o ../src/bitvector.cpp
bitvector64.o: ../src/bitvector64.cpp ../src/bitvector64.h ../src/array_t.h ../src/fileManager.h \
  ../src/util.h ../src/const.h ../src/horometer.h ../src/bitvector.h
	$(CXX) $(CCFLAGS) -c -o bitvector64.o ../src/bitvector64.cpp
bord.o: ../src/bord.cpp ../src/tab.h ../src/table.h ../src/const.h ../src/bord.h ../src/util.h ../src/part.h ../src/column.h \
  ../src/qExpr.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/query.h ../src/bundle.h ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o bord.o ../src/bord.cpp
bundle.o: ../src/bundle.cpp ../src/bundle.h ../src/util.h ../src/const.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/query.h ../src/part.h ../src/column.h ../src/table.h ../src/qExpr.h ../src/bitvector.h \
  ../src/resource.h ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o bundle.o ../src/bundle.cpp
capi.o: ../src/capi.cpp ../src/capi.h ../src/part.h ../src/column.h ../src/table.h ../src/const.h ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h ../src/query.h \
  ../src/bundle.h ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o capi.o ../src/capi.cpp
category.o: ../src/category.cpp ../src/part.h ../src/column.h ../src/table.h ../src/const.h ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h ../src/category.h \
  ../src/irelic.h ../src/index.h ../src/ikeywords.h
	$(CXX) $(CCFLAGS) -c -o category.o ../src/category.cpp
colValues.o: ../src/colValues.cpp ../src/bundle.h ../src/util.h ../src/const.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/query.h ../src/part.h ../src/column.h ../src/table.h ../src/qExpr.h \
  ../src/bitvector.h ../src/resource.h ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o colValues.o ../src/colValues.cpp
column.o: ../src/column.cpp ../src/resource.h ../src/util.h ../src/const.h ../src/category.h ../src/irelic.h \
  ../src/index.h ../src/qExpr.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/column.h ../src/table.h ../src/part.h
	$(CXX) $(CCFLAGS) -c -o column.o ../src/column.cpp
fileManager.o: ../src/fileManager.cpp ../src/fileManager.h ../src/util.h ../src/const.h ../src/resource.h \
  ../src/array_t.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o fileManager.o ../src/fileManager.cpp
ibin.o: ../src/ibin.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h \
  ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o ibin.o ../src/ibin.cpp
ibis.o: ../examples/ibis.cpp ../src/ibis.h ../src/meshQuery.h ../src/query.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/const.h ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/resource.h ../src/bundle.h ../src/colValues.h ../src/rids.h
	$(CXX) $(CCFLAGS) -c -o ibis.o ../examples/ibis.cpp
icegale.o: ../src/icegale.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o icegale.o ../src/icegale.cpp
icentre.o: ../src/icentre.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o icentre.o ../src/icentre.cpp
icmoins.o: ../src/icmoins.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o icmoins.o ../src/icmoins.cpp
idbak.o: ../src/idbak.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o idbak.o ../src/idbak.cpp
idbak2.o: ../src/idbak2.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o idbak2.o ../src/idbak2.cpp
idirekte.o: ../src/idirekte.cpp ../src/idirekte.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o idirekte.o ../src/idirekte.cpp
ifade.o: ../src/ifade.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ifade.o ../src/ifade.cpp
ikeywords.o: ../src/ikeywords.cpp ../src/ikeywords.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/category.h ../src/irelic.h \
  ../src/column.h ../src/table.h ../src/iroster.h ../src/part.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ikeywords.o ../src/ikeywords.cpp
imesa.o: ../src/imesa.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o imesa.o ../src/imesa.cpp
index.o: ../src/index.cpp ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/ibin.h ../src/irelic.h ../src/idirekte.h ../src/ikeywords.h \
  ../src/category.h ../src/column.h ../src/table.h ../src/part.h ../src/resource.h ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o index.o ../src/index.cpp
irange.o: ../src/irange.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o irange.o ../src/irange.cpp
irelic.o: ../src/irelic.cpp ../src/bitvector64.h ../src/array_t.h ../src/fileManager.h ../src/util.h ../src/const.h \
  ../src/horometer.h ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/bitvector.h ../src/part.h ../src/column.h \
  ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o irelic.o ../src/irelic.cpp
iroster.o: ../src/iroster.cpp ../src/iroster.h ../src/array_t.h ../src/fileManager.h ../src/util.h ../src/const.h \
  ../src/horometer.h ../src/column.h ../src/table.h ../src/qExpr.h ../src/bitvector.h ../src/part.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o iroster.o ../src/iroster.cpp
isapid.o: ../src/isapid.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o isapid.o ../src/isapid.cpp
isbiad.o: ../src/isbiad.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o isbiad.o ../src/isbiad.cpp
islice.o: ../src/islice.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o islice.o ../src/islice.cpp
ixambit.o: ../src/ixambit.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixambit.o ../src/ixambit.cpp
ixbylt.o: ../src/ixbylt.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixbylt.o ../src/ixbylt.cpp
ixfuge.o: ../src/ixfuge.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h ../src/bitvector.h ../src/column.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixfuge.o ../src/ixfuge.cpp
ixfuzz.o: ../src/ixfuzz.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixfuzz.o ../src/ixfuzz.cpp
ixpack.o: ../src/ixpack.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixpack.o ../src/ixpack.cpp
ixpale.o: ../src/ixpale.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixpale.o ../src/ixpale.cpp
ixzona.o: ../src/ixzona.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixzona.o ../src/ixzona.cpp
ixzone.o: ../src/ixzone.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h ../src/util.h ../src/const.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o ixzone.o ../src/ixzone.cpp
mensa.o: ../src/mensa.cpp ../src/tab.h ../src/table.h ../src/const.h ../src/bord.h ../src/util.h ../src/part.h ../src/column.h \
  ../src/qExpr.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/mensa.h ../src/query.h ../src/index.h
	$(CXX) $(CCFLAGS) -c -o mensa.o ../src/mensa.cpp
meshQuery.o: ../src/meshQuery.cpp ../src/meshQuery.h ../src/query.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/const.h ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o meshQuery.o ../src/meshQuery.cpp
part.o: ../src/part.cpp ../src/qExpr.h ../src/util.h ../src/const.h ../src/category.h ../src/irelic.h ../src/index.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/column.h ../src/table.h \
  ../src/query.h ../src/part.h ../src/resource.h ../src/iroster.h
	$(CXX) $(CCFLAGS) -c -o part.o ../src/part.cpp
parti.o: ../src/parti.cpp ../src/part.h ../src/column.h ../src/table.h ../src/const.h ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h ../src/category.h \
  ../src/irelic.h ../src/index.h
	$(CXX) $(CCFLAGS) -c -o parti.o ../src/parti.cpp
party.o: ../src/party.cpp ../src/part.h ../src/column.h ../src/table.h ../src/const.h ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h ../src/iroster.h \
  ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o party.o ../src/party.cpp
predicate.tab.o: ../src/predicate.tab.cpp ../src/predicate.tab.h ../src/util.h ../src/const.h ../src/qExpr.h \
  ../src/predicate.h
	$(CXX) $(CCFLAGS) -c -o predicate.tab.o ../src/predicate.tab.cpp
predicate.yy.o: ../src/predicate.yy.cpp ../src/predicate.h ../src/qExpr.h ../src/util.h ../src/const.h \
  ../src/predicate.tab.h
	$(CXX) $(CCFLAGS) -c -o predicate.yy.o ../src/predicate.yy.cpp
qExpr.o: ../src/qExpr.cpp ../src/util.h ../src/const.h ../src/qExpr.h
	$(CXX) $(CCFLAGS) -c -o qExpr.o ../src/qExpr.cpp
query.o: ../src/query.cpp ../src/query.h ../src/part.h ../src/column.h ../src/table.h ../src/const.h ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/resource.h ../src/predicate.h \
  ../src/bundle.h ../src/colValues.h ../src/ibin.h ../src/index.h ../src/iroster.h ../src/irelic.h ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o query.o ../src/query.cpp
resource.o: ../src/resource.cpp ../src/util.h ../src/const.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o resource.o ../src/resource.cpp
rids.o: ../src/rids.cpp ../src/rids.h ../src/util.h ../src/const.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o rids.o ../src/rids.cpp
tafel.o: ../src/tafel.cpp ../src/tafel.h ../src/table.h ../src/const.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/horometer.h ../src/part.h ../src/column.h ../src/qExpr.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o tafel.o ../src/tafel.cpp
thula.o: ../examples/thula.cpp ../src/table.h ../src/const.h ../src/resource.h ../src/util.h
	$(CXX) $(CCFLAGS) -c -o thula.o ../examples/thula.cpp
util.o: ../src/util.cpp ../src/util.h ../src/const.h ../src/array_t.h ../src/fileManager.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o util.o ../src/util.cpp
trydll.exe: ../src/ibis.h ../src/meshQuery.h ../src/query.h \
  ../src/part.h ../src/column.h ../src/table.h ../src/const.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/bundle.h ../src/colValues.h ../src/rids.h

# $Id$
# Makefile for nmake on windows using microsoft compiler visual C++ 7
#
VC=C:\Tools\VS\VC
CXX=$(VC)\BIN\cl
LINK=$(VC)\BIN\link
OPT=/MD /EHsc /GR /O2 /W1 /arch:SSE2
#OPT=/MD /EHsc /GR /Ox
INC=-I ..\src -I "C:\Tools\pthread\include" -I .
DEF=/D WIN32 /D _CONSOLE /D FASTBIT_UNLOAD_LIMIT=3 /D WITHOUT_FASTBIT_CONFIG_H
#/INCREMENTAL:NO /NOLOGO /D WINVER=0x0501
LIB=/LIBPATH:"$(VC)\Lib" /LIBPATH:"C:\Program Files\Microsoft SDKs\Windows\v6.0A\lib" /LIBPATH:"C:\Tools\pthread\lib" /SUBSYSTEM:CONSOLE pthreadVC2.lib psapi.lib advapi32.lib
# ******
# for VisualStudio .Net and earlier, the following libpath is needed
# /LIBPATH:"$(VC)\PlatformSDK\Lib"
# to replace the second libpath argument above
# ******
CCFLAGS=/nologo $(DEF) $(INC) $(OPT)
#
OBJ =  parth3d.obj parth3da.obj parth3db.obj parth3dw.obj \
 array_t.obj \
 bitvector.obj \
 bitvector64.obj \
 bundle.obj \
 capi.obj \
 category.obj \
 colValues.obj \
 countQuery.obj \
 column.obj \
 fileManager.obj \
 ibin.obj \
 joinin.obj \
 bord.obj \
 tafel.obj \
 mensa.obj \
 party.obj \
 part.obj \
 parth.obj \
 parth2d.obj \
 parti.obj \
 icegale.obj \
 icentre.obj \
 icmoins.obj \
 idbak.obj \
 idbak2.obj \
 idirekte.obj \
 ifade.obj \
 ikeywords.obj \
 imesa.obj \
 index.obj \
 irange.obj \
 irelic.obj \
 iroster.obj \
 isapid.obj \
 isbiad.obj \
 islice.obj \
 ixambit.obj \
 ixbylt.obj \
 ixfuge.obj \
 ixfuzz.obj \
 ixpack.obj \
 ixpale.obj \
 ixzona.obj \
 ixzone.obj \
 meshQuery.obj \
 selectClause.obj \
 selectLexer.obj \
 selectParser.obj \
 whereClause.obj \
 whereLexer.obj \
 whereParser.obj \
 qExpr.obj \
 query.obj \
 resource.obj \
 rids.obj \
 utilidor.obj \
 util.obj

#
ibis: ibis.exe
all: ibis.exe ardea.exe thula.exe tcapi.exe

lib: fastbit.lib
fastbit.lib: $(OBJ)
	lib /out:fastbit.lib $(OBJ)

ibis.exe: ibis.obj fastbit.lib
	$(LINK) /NOLOGO /out:$@ $(LIB) ibis.obj fastbit.lib

thula: thula.exe
thula.exe: thula.obj fastbit.lib
	$(LINK) /NOLOGO $(LIB) /out:$@ thula.obj fastbit.lib

ardea: ardea.exe
ardea.exe: ardea.obj fastbit.lib
	$(LINK) /NOLOGO $(LIB) /out:$@ ardea.obj fastbit.lib

tcapi: tcapi.exe
tcapi.exe: tcapi.obj fastbit.lib
	$(LINK) /NOLOGO $(LIB) /out:$@ tcapi.obj fastbit.lib

#	$(MAKE) /f Win.mak DEF="$(DEF) /D _USRDLL /D DLL_EXPORTS" $(OBJ)
# To compile C++ Interface of FastBit, replace _USRDLL with CXX_USE_DLL
dll: fastbit.dll
fastbit.dll: $(FRC)
	$(MAKE) /f Win.mak DEF="$(DEF) /D _USRDLL /D DLL_EXPORTS" $(OBJ)
	$(LINK) /NOLOGO /DLL /OUT:$@ $(LIB) $(OBJ)

trydll: trydll.exe
trydll.exe: trydll.obj fastbit.dll
	$(LINK) /NOLOGO /out:$@ $(LIB) trydll.obj fastbit.lib

clean:
	del *.obj core b?_? fastbit.dll *.lib *.exe *.suo *.ncb *.exp *.pdb *.manifest
#	rmdir Debug Release
force:

#suffixes
.SUFFIXES: .obj .cpp .cc .hh .h
# #
# suffixes based rules
.cpp.obj:
	$(CXX) $(CCFLAGS) -c $<
############################################################
# dependencies generated with g++ -MM
array_t.obj: ../src/array_t.cpp ../src/array_t.h ../src/fileManager.h \
  ../src/util.h ../src/const.h  ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o array_t.o ../src/array_t.cpp
bitvector.obj: ../src/bitvector.cpp ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/const.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o bitvector.o ../src/bitvector.cpp
bitvector64.obj: ../src/bitvector64.cpp ../src/bitvector64.h \
  ../src/array_t.h ../src/fileManager.h ../src/util.h ../src/const.h \
  ../src/horometer.h ../src/bitvector.h
	$(CXX) $(CCFLAGS) -c -o bitvector64.o ../src/bitvector64.cpp
bord.obj: ../src/bord.cpp ../src/tab.h ../src/table.h ../src/const.h \
  ../src/bord.h ../src/util.h ../src/part.h \
  ../src/column.h ../src/qExpr.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/query.h ../src/whereClause.h ../src/bundle.h \
  ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o bord.o ../src/bord.cpp
bundle.obj: ../src/bundle.cpp ../src/bundle.h ../src/util.h ../src/const.h \
  ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/query.h ../src/part.h ../src/column.h \
  ../src/table.h ../src/qExpr.h ../src/bitvector.h ../src/resource.h \
  ../src/utilidor.h ../src/whereClause.h ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o bundle.o ../src/bundle.cpp
capi.obj: ../src/capi.cpp ../src/capi.h  \
  ../src/part.h ../src/column.h ../src/table.h ../src/const.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/query.h ../src/whereClause.h ../src/bundle.h \
  ../src/colValues.h ../src/tafel.h
	$(CXX) $(CCFLAGS) -c -o capi.o ../src/capi.cpp
category.obj: ../src/category.cpp ../src/part.h ../src/column.h \
  ../src/table.h ../src/const.h  ../src/qExpr.h \
  ../src/util.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h \
  ../src/category.h ../src/irelic.h ../src/index.h ../src/ikeywords.h
	$(CXX) $(CCFLAGS) -c -o category.o ../src/category.cpp
colValues.obj: ../src/colValues.cpp ../src/bundle.h ../src/util.h \
  ../src/const.h  ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/query.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/qExpr.h ../src/bitvector.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h \
  ../src/colValues.h
	$(CXX) $(CCFLAGS) -c -o colValues.o ../src/colValues.cpp
column.obj: ../src/column.cpp ../src/resource.h ../src/util.h \
  ../src/const.h  ../src/category.h \
  ../src/irelic.h ../src/index.h ../src/qExpr.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/column.h ../src/table.h ../src/part.h ../src/utilidor.h \
  ../src/iroster.h
	$(CXX) $(CCFLAGS) -c -o column.o ../src/column.cpp
countQuery.obj: ../src/countQuery.cpp ../src/countQuery.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/const.h  \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/whereClause.h ../src/selectClause.h \
  ../src/query.h
	$(CXX) $(CCFLAGS) -c ../src/countQuery.cpp
fileManager.obj: ../src/fileManager.cpp ../src/fileManager.h ../src/util.h \
  ../src/const.h  ../src/resource.h ../src/array_t.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o fileManager.o ../src/fileManager.cpp
ibin.obj: ../src/ibin.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h \
  ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o ibin.o ../src/ibin.cpp
icegale.obj: ../src/icegale.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o icegale.o ../src/icegale.cpp
icentre.obj: ../src/icentre.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o icentre.o ../src/icentre.cpp
icmoins.obj: ../src/icmoins.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o icmoins.o ../src/icmoins.cpp
idbak.obj: ../src/idbak.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o idbak.o ../src/idbak.cpp
idbak2.obj: ../src/idbak2.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o idbak2.o ../src/idbak2.cpp
idirekte.obj: ../src/idirekte.cpp ../src/idirekte.h ../src/index.h \
  ../src/qExpr.h ../src/util.h ../src/const.h  \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o idirekte.o ../src/idirekte.cpp
ifade.obj: ../src/ifade.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ifade.o ../src/ifade.cpp
ikeywords.obj: ../src/ikeywords.cpp ../src/ikeywords.h ../src/index.h \
  ../src/qExpr.h ../src/util.h ../src/const.h  \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/category.h ../src/irelic.h ../src/column.h \
  ../src/table.h ../src/iroster.h ../src/part.h ../src/resource.h \
  ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ikeywords.o ../src/ikeywords.cpp
imesa.obj: ../src/imesa.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o imesa.o ../src/imesa.cpp
index.obj: ../src/index.cpp ../src/index.h ../src/qExpr.h ../src/util.h \
  ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/ibin.h \
  ../src/idirekte.h ../src/ikeywords.h ../src/category.h ../src/irelic.h \
  ../src/column.h ../src/table.h ../src/part.h ../src/resource.h \
  ../src/utilidor.h ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o index.o ../src/index.cpp
irange.obj: ../src/irange.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o irange.o ../src/irange.cpp
irelic.obj: ../src/irelic.cpp ../src/bitvector64.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/const.h \
  ../src/horometer.h ../src/irelic.h \
  ../src/index.h ../src/qExpr.h ../src/bitvector.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o irelic.o ../src/irelic.cpp
iroster.obj: ../src/iroster.cpp ../src/iroster.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/const.h \
  ../src/horometer.h ../src/column.h \
  ../src/table.h ../src/qExpr.h ../src/bitvector.h ../src/part.h \
  ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o iroster.o ../src/iroster.cpp
isapid.obj: ../src/isapid.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o isapid.o ../src/isapid.cpp
isbiad.obj: ../src/isbiad.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o isbiad.o ../src/isbiad.cpp
islice.obj: ../src/islice.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o islice.o ../src/islice.cpp
ixambit.obj: ../src/ixambit.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixambit.o ../src/ixambit.cpp
ixbylt.obj: ../src/ixbylt.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixbylt.o ../src/ixbylt.cpp
ixfuge.obj: ../src/ixfuge.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixfuge.o ../src/ixfuge.cpp
ixfuzz.obj: ../src/ixfuzz.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixfuzz.o ../src/ixfuzz.cpp
ixpack.obj: ../src/ixpack.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixpack.o ../src/ixpack.cpp
ixpale.obj: ../src/ixpale.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixpale.o ../src/ixpale.cpp
ixzona.obj: ../src/ixzona.cpp ../src/irelic.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixzona.o ../src/ixzona.cpp
ixzone.obj: ../src/ixzone.cpp ../src/ibin.h ../src/index.h ../src/qExpr.h \
  ../src/util.h ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o ixzone.o ../src/ixzone.cpp
joinin.obj: ../src/joinin.cpp ../src/joinin.h ../src/join.h ../src/table.h \
  ../src/const.h  ../src/part.h ../src/column.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/query.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o joinin.o ../src/joinin.cpp
mensa.obj: ../src/mensa.cpp ../src/tab.h ../src/table.h ../src/const.h \
  ../src/bord.h ../src/util.h ../src/part.h \
  ../src/column.h ../src/qExpr.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/mensa.h ../src/countQuery.h \
  ../src/whereClause.h ../src/index.h ../src/category.h ../src/irelic.h \
  ../src/selectClause.h
	$(CXX) $(CCFLAGS) -c -o mensa.o ../src/mensa.cpp
meshQuery.obj: ../src/meshQuery.cpp ../src/meshQuery.h ../src/query.h \
  ../src/part.h ../src/column.h ../src/table.h ../src/const.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o meshQuery.o ../src/meshQuery.cpp
part.obj: ../src/part.cpp ../src/qExpr.h ../src/util.h ../src/const.h \
  ../src/category.h ../src/irelic.h \
  ../src/index.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/column.h ../src/table.h ../src/query.h \
  ../src/part.h ../src/resource.h ../src/utilidor.h ../src/whereClause.h \
  ../src/countQuery.h ../src/iroster.h ../src/twister.h
	$(CXX) $(CCFLAGS) -c -o part.o ../src/part.cpp
parth.obj: ../src/parth.cpp ../src/index.h ../src/qExpr.h ../src/util.h \
  ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/countQuery.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o parth.o ../src/parth.cpp
parth2d.obj: ../src/parth2d.cpp ../src/index.h ../src/qExpr.h ../src/util.h \
  ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/countQuery.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o parth2d.o ../src/parth2d.cpp
parth3d.obj: ../src/parth3d.cpp ../src/index.h ../src/qExpr.h ../src/util.h \
  ../src/const.h  ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/countQuery.h ../src/part.h ../src/column.h ../src/table.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o parth3d.o ../src/parth3d.cpp
parth3da.obj: ../src/parth3da.cpp ../src/countQuery.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/const.h  \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o parth3da.o ../src/parth3da.cpp
parth3db.obj: ../src/parth3db.cpp ../src/countQuery.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/const.h  \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o parth3db.o ../src/parth3db.cpp
parth3dw.obj: ../src/parth3dw.cpp ../src/countQuery.h ../src/part.h \
  ../src/column.h ../src/table.h ../src/const.h  \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/utilidor.h ../src/whereClause.h
	$(CXX) $(CCFLAGS) -c -o parth3dw.o ../src/parth3dw.cpp
parti.obj: ../src/parti.cpp ../src/part.h ../src/column.h ../src/table.h \
  ../src/const.h  ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h \
  ../src/category.h ../src/irelic.h ../src/index.h ../src/selectClause.h
	$(CXX) $(CCFLAGS) -c -o parti.o ../src/parti.cpp
party.obj: ../src/party.cpp ../src/part.h ../src/column.h ../src/table.h \
  ../src/const.h  ../src/qExpr.h ../src/util.h \
  ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h ../src/iroster.h \
  ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o party.o ../src/party.cpp
qExpr.obj: ../src/qExpr.cpp ../src/util.h ../src/const.h \
  ../src/part.h ../src/column.h ../src/table.h \
  ../src/qExpr.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o qExpr.o ../src/qExpr.cpp
query.obj: ../src/query.cpp ../src/query.h ../src/part.h ../src/column.h \
  ../src/table.h ../src/const.h  ../src/qExpr.h \
  ../src/util.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h \
  ../src/whereClause.h ../src/bundle.h ../src/colValues.h ../src/ibin.h \
  ../src/index.h ../src/iroster.h ../src/irelic.h ../src/bitvector64.h
	$(CXX) $(CCFLAGS) -c -o query.o ../src/query.cpp
resource.obj: ../src/resource.cpp ../src/util.h ../src/const.h \
   ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o resource.o ../src/resource.cpp
rids.obj: ../src/rids.cpp ../src/rids.h ../src/utilidor.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/const.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o rids.o ../src/rids.cpp
selectClause.obj: ../src/selectClause.cpp ../src/part.h ../src/column.h \
  ../src/table.h ../src/const.h  ../src/qExpr.h \
  ../src/util.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h \
  ../src/selectLexer.h ../src/selectParser.hh ../src/stack.hh \
  ../src/selectClause.h ../src/location.hh ../src/position.hh \
  ./FlexLexer.h
	$(CXX) $(CCFLAGS) -c -o selectClause.o ../src/selectClause.cpp
tafel.obj: ../src/tafel.cpp ../src/tafel.h ../src/table.h ../src/const.h \
  ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/horometer.h ../src/part.h \
  ../src/column.h ../src/qExpr.h ../src/resource.h ../src/utilidor.h
	$(CXX) $(CCFLAGS) -c -o tafel.o ../src/tafel.cpp
util.obj: ../src/util.cpp ../src/util.h ../src/const.h \
   ../src/horometer.h ../src/resource.h
	$(CXX) $(CCFLAGS) -c -o util.o ../src/util.cpp
utilidor.obj: ../src/utilidor.cpp ../src/utilidor.h ../src/array_t.h \
  ../src/fileManager.h ../src/util.h ../src/const.h ../src/horometer.h
	$(CXX) $(CCFLAGS) -c -o utilidor.o ../src/utilidor.cpp
whereClause.obj: ../src/whereClause.cpp ../src/part.h ../src/column.h \
  ../src/table.h ../src/const.h  ../src/qExpr.h \
  ../src/util.h ../src/bitvector.h ../src/array_t.h ../src/fileManager.h \
  ../src/horometer.h ../src/resource.h ../src/utilidor.h \
  ../src/whereLexer.h ../src/whereParser.hh ../src/stack.hh \
  ../src/whereClause.h ../src/location.hh ../src/position.hh \
  ./FlexLexer.h ../src/selectClause.h
	$(CXX) $(CCFLAGS) -c -o whereClause.o ../src/whereClause.cpp
selectLexer.obj: ../src/selectLexer.cc ./FlexLexer.h ../src/selectLexer.h \
  ../src/selectParser.hh ../src/stack.hh ../src/selectClause.h \
  ../src/qExpr.h ../src/util.h ../src/const.h  \
  ../src/location.hh ../src/position.hh
	$(CXX) $(CCFLAGS) -c ../src/selectLexer.cc
selectParser.obj: ../src/selectParser.cc ../src/selectParser.hh \
  ../src/stack.hh ../src/selectClause.h ../src/qExpr.h ../src/util.h \
  ../src/const.h  ../src/location.hh \
  ../src/position.hh ../src/selectLexer.h ./FlexLexer.h
	$(CXX) $(CCFLAGS) -c ../src/selectParser.cc
whereLexer.obj: ../src/whereLexer.cc ./FlexLexer.h ../src/whereLexer.h \
  ../src/whereParser.hh ../src/stack.hh ../src/whereClause.h \
  ../src/qExpr.h ../src/util.h ../src/const.h  \
  ../src/location.hh ../src/position.hh
	$(CXX) $(CCFLAGS) -c ../src/whereLexer.cc
whereParser.obj: ../src/whereParser.cc ../src/whereParser.hh \
  ../src/stack.hh ../src/whereClause.h ../src/qExpr.h ../src/util.h \
  ../src/const.h ../src/location.hh \
  ../src/position.hh ../src/whereLexer.h ./FlexLexer.h
	$(CXX) $(CCFLAGS) -c ../src/whereParser.cc
ardea.obj: ../examples/ardea.cpp ../src/table.h ../src/const.h \
   ../src/resource.h ../src/util.h
	$(CXX) $(CCFLAGS) -c ../examples/ardea.cpp
ibis.obj: ../examples/ibis.cpp ../src/ibis.h ../src/countQuery.h \
  ../src/part.h ../src/column.h ../src/table.h ../src/const.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h \
  ../src/meshQuery.h ../src/query.h ../src/bundle.h ../src/colValues.h \
  ../src/join.h ../src/rids.h ../src/mensa.h
	$(CXX) $(CCFLAGS) -c ../examples/ibis.cpp
rara.obj: ../examples/rara.cpp ../src/ibis.h ../src/countQuery.h \
  ../src/part.h ../src/column.h ../src/table.h ../src/const.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h \
  ../src/array_t.h ../src/fileManager.h ../src/horometer.h \
  ../src/resource.h ../src/utilidor.h ../src/whereClause.h \
  ../src/meshQuery.h ../src/query.h ../src/bundle.h ../src/colValues.h \
  ../src/join.h ../src/rids.h
	$(CXX) $(CCFLAGS) -c ../examples/rara.cpp
thula.obj: ../examples/thula.cpp ../src/table.h ../src/const.h \
  ../src/resource.h ../src/util.h ../src/mensa.h \
  ../src/table.h ../src/fileManager.h
	$(CXX) $(CCFLAGS) -c ../examples/thula.cpp
tcapi.obj: ../examples/tcapi.c ../src/capi.h
	$(CXX) $(CCFLAGS) -c ../examples/tcapi.c

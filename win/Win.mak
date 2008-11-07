# $Id$
# Makefile for nmake on windows using microsoft compiler visual C++ 7
#
VC=C:\Tools\VS\VC
CXX=$(VC)\BIN\cl
LINK=$(VC)\BIN\link
OPT=/MD /EHsc /GR /O2 /W1 /arch:SSE2
#OPT=/MD /EHsc /GR /Ox
INC=-I ..\src -I "C:\Tools\pthread\include"
DEF=/D WIN32 /D _CONSOLE /D FILEMANAGER_UNLOAD_TIME=3
#/INCREMENTAL:NO /NOLOGO
LIB=/LIBPATH:"$(VC)\Lib" /LIBPATH:"C:\Program Files\Microsoft SDKs\Windows\v6.0A\lib" /LIBPATH:"C:\Tools\pthread\lib" /SUBSYSTEM:CONSOLE pthreadVC2.lib psapi.lib advapi32.lib
# ******
# for VisualStudio .Net and earlier, the following libpath is needed
# /LIBPATH:"$(VC)\PlatformSDK\Lib"
# to replace the second libpath argument above
# ******
CCFLAGS=/nologo $(DEF) $(INC) $(OPT)
#
OBJ =  array_t.obj \
 bitvector.obj \
 bitvector64.obj \
 bundle.obj \
 capi.obj \
 category.obj \
 colValues.obj \
 column.obj \
 fileManager.obj \
 ibin.obj \
 joinin.obj \
 bord.obj \
 tafel.obj \
 mensa.obj \
 party.obj \
 part.obj \
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
ardea.obj: ..\examples\ardea.cpp ..\src\table.h ..\src\const.h ..\src\resource.h ..\src\util.h
	$(CXX) $(CCFLAGS) -c ..\examples\ardea.cpp
array_t.obj: ..\src\array_t.cpp ..\src\array_t.h ..\src\fileManager.h ..\src\util.h ..\src\const.h ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\array_t.cpp
bit64.obj: bit64.cpp ..\src\bitvector64.h ..\src\array_t.h ..\src\fileManager.h ..\src\util.h ..\src\const.h \
  ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c bit64.cpp
bitvector.obj: ..\src\bitvector.cpp ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\util.h \
  ..\src\const.h ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\bitvector.cpp
bitvector64.obj: ..\src\bitvector64.cpp ..\src\bitvector64.h ..\src\array_t.h ..\src\fileManager.h \
  ..\src\util.h ..\src\const.h ..\src\horometer.h ..\src\bitvector.h
	$(CXX) $(CCFLAGS) -c ..\src\bitvector64.cpp
bord.obj: ..\src\bord.cpp ..\src\tab.h ..\src\table.h ..\src\const.h ..\src\bord.h ..\src\util.h ..\src\part.h ..\src\column.h \
  ..\src\qExpr.h ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h \
  ..\src\query.h ..\src\bundle.h ..\src\colValues.h
	$(CXX) $(CCFLAGS) -c ..\src\bord.cpp
bundle.obj: ..\src\bundle.cpp ..\src\bundle.h ..\src\util.h ..\src\const.h ..\src\array_t.h ..\src\fileManager.h \
  ..\src\horometer.h ..\src\query.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\qExpr.h ..\src\bitvector.h \
  ..\src\resource.h ..\src\colValues.h
	$(CXX) $(CCFLAGS) -c ..\src\bundle.cpp
capi.obj: ..\src\capi.cpp ..\src\capi.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\const.h ..\src\qExpr.h ..\src\util.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h ..\src\query.h \
  ..\src\bundle.h ..\src\colValues.h
	$(CXX) $(CCFLAGS) -c ..\src\capi.cpp
category.obj: ..\src\category.cpp ..\src\part.h ..\src\column.h ..\src\table.h ..\src\const.h ..\src\qExpr.h ..\src\util.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h ..\src\category.h \
  ..\src\irelic.h ..\src\index.h ..\src\ikeywords.h
	$(CXX) $(CCFLAGS) -c ..\src\category.cpp
colValues.obj: ..\src\colValues.cpp ..\src\bundle.h ..\src\util.h ..\src\const.h ..\src\array_t.h \
  ..\src\fileManager.h ..\src\horometer.h ..\src\query.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\qExpr.h \
  ..\src\bitvector.h ..\src\resource.h ..\src\colValues.h
	$(CXX) $(CCFLAGS) -c ..\src\colValues.cpp
column.obj: ..\src\column.cpp ..\src\resource.h ..\src\util.h ..\src\const.h ..\src\category.h ..\src\irelic.h \
  ..\src\index.h ..\src\qExpr.h ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h \
  ..\src\column.h ..\src\table.h ..\src\part.h
	$(CXX) $(CCFLAGS) -c ..\src\column.cpp
fileManager.obj: ..\src\fileManager.cpp ..\src\fileManager.h ..\src\util.h ..\src\const.h ..\src\resource.h \
  ..\src\array_t.h ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\fileManager.cpp
ibin.obj: ..\src\ibin.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h \
  ..\src\bitvector64.h
	$(CXX) $(CCFLAGS) -c ..\src\ibin.cpp
ibis.obj: ..\examples\ibis.cpp ..\src\ibis.h ..\src\meshQuery.h ..\src\query.h ..\src\part.h ..\src\column.h ..\src\table.h \
  ..\src\const.h ..\src\qExpr.h ..\src\util.h ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h \
  ..\src\resource.h ..\src\bundle.h ..\src\colValues.h ..\src\rids.h
	$(CXX) $(CCFLAGS) -c ..\examples\ibis.cpp
# -o ibis.obj
icegale.obj: ..\src\icegale.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\icegale.cpp
icentre.obj: ..\src\icentre.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\icentre.cpp
icmoins.obj: ..\src\icmoins.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\icmoins.cpp
idbak.obj: ..\src\idbak.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\idbak.cpp
idbak2.obj: ..\src\idbak2.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\idbak2.cpp
idirekte.obj: ..\src\idirekte.cpp ..\src\idirekte.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h \
  ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\idirekte.cpp
ifade.obj: ..\src\ifade.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ifade.cpp
ikeywords.obj: ..\src\ikeywords.cpp ..\src\ikeywords.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\category.h ..\src\irelic.h \
  ..\src\column.h ..\src\table.h ..\src\iroster.h ..\src\part.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ikeywords.cpp
imesa.obj: ..\src\imesa.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\imesa.cpp
index.obj: ..\src\index.cpp ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h ..\src\array_t.h \
  ..\src\fileManager.h ..\src\horometer.h ..\src\ibin.h ..\src\irelic.h ..\src\idirekte.h ..\src\ikeywords.h \
  ..\src\category.h ..\src\column.h ..\src\table.h ..\src\part.h ..\src\resource.h ..\src\bitvector64.h
	$(CXX) $(CCFLAGS) -c ..\src\index.cpp
irange.obj: ..\src\irange.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\irange.cpp
irelic.obj: ..\src\irelic.cpp ..\src\bitvector64.h ..\src\array_t.h ..\src\fileManager.h ..\src\util.h ..\src\const.h \
  ..\src\horometer.h ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\bitvector.h ..\src\part.h ..\src\column.h \
  ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\irelic.cpp
iroster.obj: ..\src\iroster.cpp ..\src\iroster.h ..\src\array_t.h ..\src\fileManager.h ..\src\util.h ..\src\const.h \
  ..\src\horometer.h ..\src\column.h ..\src\table.h ..\src\qExpr.h ..\src\bitvector.h ..\src\part.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\iroster.cpp
isapid.obj: ..\src\isapid.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\isapid.cpp
isbiad.obj: ..\src\isbiad.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\isbiad.cpp
islice.obj: ..\src\islice.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\islice.cpp
ixambit.obj: ..\src\ixambit.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixambit.cpp
ixbylt.obj: ..\src\ixbylt.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixbylt.cpp
ixfuge.obj: ..\src\ixfuge.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h \
  ..\src\util.h ..\src\const.h ..\src\bitvector.h ..\src\column.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h \
  ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixfuge.cpp
ixfuzz.obj: ..\src\ixfuzz.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixfuzz.cpp
ixpack.obj: ..\src\ixpack.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixpack.cpp
ixpale.obj: ..\src\ixpale.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixpale.cpp
ixzona.obj: ..\src\ixzona.cpp ..\src\irelic.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixzona.cpp
ixzone.obj: ..\src\ixzone.cpp ..\src\ibin.h ..\src\index.h ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\bitvector.h \
  ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\ixzone.cpp
mensa.obj: ..\src\mensa.cpp ..\src\tab.h ..\src\table.h ..\src\const.h ..\src\bord.h ..\src\util.h ..\src\part.h ..\src\column.h \
  ..\src\qExpr.h ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h \
  ..\src\mensa.h ..\src\query.h ..\src\index.h
	$(CXX) $(CCFLAGS) -c ..\src\mensa.cpp
meshQuery.obj: ..\src\meshQuery.cpp ..\src\meshQuery.h ..\src\query.h ..\src\part.h ..\src\column.h ..\src\table.h \
  ..\src\const.h ..\src\qExpr.h ..\src\util.h ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h \
  ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\meshQuery.cpp
part.obj: ..\src\part.cpp ..\src\qExpr.h ..\src\util.h ..\src\const.h ..\src\category.h ..\src\irelic.h ..\src\index.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\column.h ..\src\table.h \
  ..\src\query.h ..\src\part.h ..\src\resource.h ..\src\iroster.h
	$(CXX) $(CCFLAGS) -c ..\src\part.cpp
parti.obj: ..\src\parti.cpp ..\src\part.h ..\src\column.h ..\src\table.h ..\src\const.h ..\src\qExpr.h ..\src\util.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h ..\src\category.h \
  ..\src\irelic.h ..\src\index.h
	$(CXX) $(CCFLAGS) -c ..\src\parti.cpp
party.obj: ..\src\party.cpp ..\src\part.h ..\src\column.h ..\src\table.h ..\src\const.h ..\src\qExpr.h ..\src\util.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h ..\src\iroster.h \
  ..\src\bitvector64.h
	$(CXX) $(CCFLAGS) -c ..\src\party.cpp
whereClause.obj: ..\src\whereClause.cpp ..\src\whereParser.hh ..\src\util.h ..\src\const.h ..\src\qExpr.h \
  ..\src\whereClause.h
	$(CXX) $(CCFLAGS) -I . -c ..\src\whereClause.cpp
whereParser.obj: ..\src\whereParser.cc ..\src\whereParser.hh ..\src\util.h ..\src\const.h ..\src\qExpr.h \
  ..\src\whereClause.h
	$(CXX) $(CCFLAGS) -I . -c ..\src\whereParser.cc
whereLexer.obj: ..\src\whereLexer.cc ..\src\whereClause.h ..\src\qExpr.h ..\src\util.h ..\src\const.h \
  ..\src\whereParser.hh
	$(CXX) $(CCFLAGS) -I . -c ..\src\whereLexer.cc
qExpr.obj: ..\src\qExpr.cpp ..\src\util.h ..\src\const.h ..\src\qExpr.h
	$(CXX) $(CCFLAGS) -c ..\src\qExpr.cpp
query.obj: ..\src\query.cpp ..\src\query.h ..\src\part.h ..\src\column.h ..\src\table.h ..\src\const.h ..\src\qExpr.h ..\src\util.h \
  ..\src\bitvector.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h ..\src\resource.h ..\src\whereClause.h \
  ..\src\bundle.h ..\src\colValues.h ..\src\ibin.h ..\src\index.h ..\src\iroster.h ..\src\irelic.h ..\src\bitvector64.h
	$(CXX) $(CCFLAGS) -c ..\src\query.cpp
resource.obj: ..\src\resource.cpp ..\src\util.h ..\src\const.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\resource.cpp
rids.obj: ..\src\rids.cpp ..\src\rids.h ..\src\util.h ..\src\const.h ..\src\array_t.h ..\src\fileManager.h \
  ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\rids.cpp
tafel.obj: ..\src\tafel.cpp ..\src\tafel.h ..\src\table.h ..\src\const.h ..\src\bitvector.h ..\src\array_t.h \
  ..\src\fileManager.h ..\src\util.h ..\src\horometer.h ..\src\part.h ..\src\column.h ..\src\qExpr.h ..\src\resource.h
	$(CXX) $(CCFLAGS) -c ..\src\tafel.cpp
thula.obj: ..\examples\thula.cpp ..\src\table.h ..\src\const.h ..\src\resource.h ..\src\util.h
	$(CXX) $(CCFLAGS) -c ..\examples\thula.cpp
util.obj: ..\src\util.cpp ..\src\util.h ..\src\const.h ..\src\array_t.h ..\src\fileManager.h ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\util.cpp
tcapi.obj: ..\examples\tcapi.c ..\src\capi.h
	$(CXX) $(CCFLAGS) -c ..\examples\tcapi.c
trydll.obj: trydll.cpp ../src/ibis.h ../src/meshQuery.h ../src/query.h \
  ../src/part.h ../src/column.h ../src/table.h ../src/const.h \
  ../src/qExpr.h ../src/util.h ../src/bitvector.h ../src/array_t.h \
  ../src/fileManager.h ../src/horometer.h ../src/resource.h \
  ../src/bundle.h ../src/colValues.h ../src/rids.h
	$(CXX) $(CCFLAGS) /D CXX_USE_DLL -c trydll.cpp
utilidor.obj: ..\src\utilidor.cpp ..\src\utilidor.h ..\src\array_t.h \
  ..\src\fileManager.h ..\src\util.h ..\src\const.h ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\utilidor.cpp
joinin.obj: ..\src\joinin.cpp ..\src\joinin.h ..\src\join.h ..\src\table.h \
  ..\src\const.h ..\src\part.h ..\src\column.h ..\src\fileManager.h \
  ..\src\qExpr.h ..\src\util.h ..\src\bitvector.h ..\src\array_t.h \
  ..\src\resource.h ..\src\query.h ..\src\utilidor.h ..\src\horometer.h
	$(CXX) $(CCFLAGS) -c ..\src\joinin.cpp

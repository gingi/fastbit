UTOMAKE_OPTIONS=gnu
EXTRA_PROGRAMS = readcsv smatch inRange setqgen
check_SCRIPTS = scripts/matchCounts.pl
readcsv_SOURCES = readcsv.cpp
smatch_SOURCES = smatch.cpp
smatch_CPPFLAGS = -I../src
smatch_DEPENDENCIES = ../src/libfastbit.la
smatch_LDADD = ../src/libfastbit.la
inRange_SOURCES = inRange.cpp
inRange_CPPFLAGS = -I../src
inRange_DEPENDENCIES = ../src/libfastbit.la
inRange_LDADD = ../src/libfastbit.la
setqgen_SOURCES = setqgen.cpp
setqgen_CPPFLAGS = -I../src
setqgen_DEPENDENCIES = ../src/libfastbit.la
setqgen_LDADD = ../src/libfastbit.la
#
EXDIR=../examples
TESTDIR=$(PWD)/tmp
#
IBISEXE=$(EXDIR)/ibis$(EXEEXT)
ARDEAEXE=$(EXDIR)/ardea$(EXEEXT)
THULAEXE=$(EXDIR)/thula$(EXEEXT)
TCAPIEXE=$(EXDIR)/tcapi$(EXEEXT)
#
EXTRA_DIST=test0.csv test1.csv maurel.csv marksdb.csv \
    scripts/fastbit-config.in scripts/hcompare.pl \
    scripts/matchCounts.pl scripts/printWarning.pl scripts/query-count.list \
    m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 \
    vault/hist0 vault/hist1 vault/hist2 vault/hist3 vault/hist4 \
    vault/text-0 vault/thula-0 vault/thula-1 vault/tcapi-3 vault/inRange-1 \
    vault/marksdb-1 vault/marksdb-3 vault/marksdb-4 vault/smatch-1 \
    vault/small-1 vault/small-2 vault/small-3 vault/small-4 vault/small-5 vault/small-6 vault/small-7 \
    vault/join-1 vault/join-2 vault/join-3 vault/join-4 vault/join-5 vault/join-6 vault/join-7 vault/join-8

#
# tests with different levels of thoroughness
check: clean-tmp check-am
	@$(MAKE) readcsv$(EXEEXT) smatch${EXEEXT} inRange${EXEEXT} setqgen${EXEEXT} > /dev/null
	@$(MAKE) check-thula && $(MAKE) check-tcapi && \
	$(MAKE) check-text && $(MAKE) check-marksdb && \
	$(MAKE) really-small && $(MAKE) check-maurel && \
	$(MAKE) check-ibis && $(MAKE) check-join
more-check: check check-sq1 small-tests
full-check: more-check check-sq2 large-tests
#
# small scale tests, may take about ten minutes on a typical Linux machine
small-tests: append-test sanity-check match-counts
#
# the following target excercises larger tests and also excercieses
# obselete index options
# the tests run on the same set of data files and can not be run in parallel
large-tests: large-append-test
	 $(MAKE) multilevel && $(MAKE) large-sanity-check
#
check-thula: $(THULAEXE) $(TESTDIR)/t1/-part.txt $(TESTDIR)/t2/t2a/-part.txt $(TESTDIR)/t2/t2b/-part.txt $(TESTDIR)/t2/t2c/-part.txt
	@rm -f $(TESTDIR)/thula-0 $(TESTDIR)/thula-1
	@$(THULAEXE) -d $(TESTDIR)/t1/ -w "a = 0" | if [ `fgrep " produced 2 hits out of 200 records" - | wc -l` -eq 1 ] ; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "a = b and -1 < c < 10" | if [ `fgrep " produced 20 hits out of 200 records" - | wc -l` -eq 1 ] ; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "-2 <= c < 10" -s "min(a/b), max(a/b), avg(a/b)" -x $(TESTDIR)/thula-0 > /dev/null && if [ `diff -w vault/thula-0 $(TESTDIR)/thula-0 | wc -l` -eq 0 ] ; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "a < c and b > 30" -s "b, min(b/c*c), max(sqrt(a/c)^2*c), avg(a/b)" -x $(TESTDIR)/thula-1 > /dev/null && if [ `diff -w vault/thula-1 $(TESTDIR)/thula-1 | wc -l` -eq 0 ] ; then echo Passed $@ case 4; else echo Did NOT pass $@ case 4; fi
	@$(THULAEXE) -d $(TESTDIR)/t2 -w "a between -1 and 1" | if [ `fgrep "t2) contains 14 rows but no columns" - | wc -l` -eq 1 ] ; then echo Passed $@ case 5; else echo Did NOT pass $@ case 5; fi
	@echo
check-tcapi: $(TESTDIR)/t1/-part.txt $(TESTDIR)/t0/-part.txt $(TCAPIEXE)
	@$(TCAPIEXE) $(TESTDIR)/t1/ "a = 0" | if [ `fgrep " produced 2 hits" - | wc -l` -eq 1 ] ; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(TCAPIEXE) $(TESTDIR)/t1 "a = b and -2 <= c < 10" | if [ `fgrep " produced 20 hits" - | wc -l` -eq 1 ] ; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(TCAPIEXE) $(TESTDIR)/t0 "c < 10" a l | tail -1 > $(TESTDIR)/tcapi-3 ; if [ `diff -w vault/tcapi-3 $(TESTDIR)/tcapi-3 | wc -l` -eq 0 ] ; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@echo
check-ibis: $(IBISEXE) $(TESTDIR)/t1/-part.txt $(TESTDIR)/rowlist $(TESTDIR)/t2/t2a/-part.txt $(TESTDIR)/t2/t2b/-part.txt $(TESTDIR)/t2/t2c/-part.txt
	@rm -f $(TESTDIR)/hist0 $(TESTDIR)/hist1 $(TESTDIR)/hist2
	@$(IBISEXE) -d $(TESTDIR)/t1/ -q "where a = 0 OR a = -1 OR a = -2" | if [ `fgrep " produced 2 hits, " - | wc -l` -eq 1 ] ; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "select count(*) as cnt where a = b and -3 < c < exp(log(9.5))" | if [ `fgrep "20" - | fgrep -v between | wc -c` -eq 3 ] ; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -y $(TESTDIR)/rowlist -q "where a=0" | if [ `fgrep " produced 1 hit, " - | wc -l` -eq 1 ] ; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "where -1 < c < 10" | if [ `fgrep " produced 19 hits, " - | wc -l` -eq 1 ] ; then echo Passed $@ case 4; else echo Did NOT pass $@ case 4; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -k "a < 2 || c < 3" -q "where a=0" | if [ `fgrep " produced 2 hits, " - | wc -l` -eq 1 ] ; then echo Passed $@ case 5; else echo Did NOT pass $@ case 5; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "where c < 2" | if [ `fgrep " produced 4 hits, " - | wc -l` -eq 1 ] ; then echo Passed $@ case 6; else echo Did NOT pass $@ case 6; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "c : c>80" | perl -p -e 's/ in Partition .*//' | if [ `diff -w vault/hist0 - | wc -l` -eq 0 ] ; then echo Passed $@ case 7; else echo Did NOT pass $@ case 7; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "joint a b : c>50" > $(TESTDIR)/hist1 && if [ `diff -w vault/hist1 $(TESTDIR)/hist1 | wc -l` -eq 0 ] ; then echo Passed $@ case 8; else echo Did NOT pass $@ case 8; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "joint c a b : c>50" > $(TESTDIR)/hist2 && if [ `diff -w vault/hist2 $(TESTDIR)/hist2 | wc -l` -eq 0 ] ; then echo Passed $@ case 9; else echo Did NOT pass $@ case 9; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "weighted a b : c>80" > $(TESTDIR)/hist3 && if [ `diff -w vault/hist3 $(TESTDIR)/hist3 | wc -l` -eq 0 ] ; then echo Passed $@ case 10; else echo Did NOT pass $@ case 10; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "select c where b > 90 order by c" -r -v 2 | if [ `fgrep "Successfully verified 18 RIDs" - | wc -l` -gt 1 ] ; then echo Passed $@ case 11; else echo Did NOT pass $@ case 11; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -v -t 5 >| $(TESTDIR)/check-ibis-12.log && if [ `fgrep "found no error" $(TESTDIR)/check-ibis-12.log | wc -l` -eq 1 ] ; then echo Passed $@ case 12; else echo Did NOT pass $@ case 12; fi
	@$(IBISEXE) -r $(TESTDIR)/t2/t2c -v 2 | if [ `fgrep 'reorder' - | fgrep 'completed successfully' - | wc -l` -eq 1 ] ; then echo Passed $@ case 13; else  echo Did NOT pass $@ case 13; fi
	@$(IBISEXE) -d $(TESTDIR)/t2 -q "select floor(b/10) as sb, avg(c), countdistinct(c) where a < 25" -o $(TESTDIR)/hist4 | if [ `scripts/printWarning.pl - | wc -l` -eq 0 ] ; then echo Passed $@ case 14; else echo Did NOT pass $@ case 14; fi
	@if [ ` diff -w vault/hist4 $(TESTDIR)/hist4 | wc -l ` -eq 0 ] ; then echo Passed $@ case 15; else echo Did NOT pass $@ case 15; fi
	@echo
check-text: $(IBISEXE) smatch${EXEEXT} inRange${EXEEXT} $(TESTDIR)/t0/-part.txt
	@$(IBISEXE) -d $(TESTDIR)/t0/ -b -v -v > $(TESTDIR)/check-text.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a, b where -5 < c < 10" -o $(TESTDIR)/t0-0 >> $(TESTDIR)/check-text.log 2>&1
	@if [ `awk -F ', ' '{if ($$1 != $$2) print $$0}' $(TESTDIR)/t0-0 | wc -l` -le 2 ] ; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a, b where c between 10 and 20" -o $(TESTDIR)/t0-1 >> $(TESTDIR)/check-text.log 2>&1
	@if [ `awk -F ', ' '{if ($$1 != $$2) print $$0}' $(TESTDIR)/t0-1 | wc -l` -le 2 ] ; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a where c in (11)" -o $(TESTDIR)/t0-2 >> $(TESTDIR)/check-text.log 2>&1
	@if [ `tail -1 $(TESTDIR)/t0-2 | fgrep 88 - | wc -l` -eq 1 ] ; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a, min(c) where -4 < c < 10" -o $(TESTDIR)/t0-3 >> $(TESTDIR)/check-text.log 2>&1
	@fgrep -v 'a, b' $(TESTDIR)/t0-0 | cut -d',' -f1 - > $(TESTDIR)/t0-4
	@fgrep -v 'a, b' $(TESTDIR)/t0-3 | cut -d',' -f1 - > $(TESTDIR)/t0-5
	@if [ `diff $(TESTDIR)/t0-4 $(TESTDIR)/t0-5 | wc -l` -eq 0 ] ; then echo Passed $@ case 4; else echo Did NOT pass $@ case 4; fi
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select count(*), floor(c/10), 10-floor(c/7) as b, max(c) as mx, avg(c) as av where a in ('1', '11', '21') or c < 29 order by b, av desc" -o $(TESTDIR)/text-0 >> $(TESTDIR)/check-text.log 2>&1
	@if [ `diff -w vault/text-0 $(TESTDIR)/text-0 | wc -l` -eq 0 ] ; then echo Passed $@ case 5; else echo Did NOT pass $@ case 5; fi
	@if [ `$(IBISEXE) -d $(TESTDIR)/t0 -yank "c < 10" -z -v | fgrep 'doDeletion purgeInactive(t0) returned 90' | wc -l` -eq 1 ] ; then echo Passed $@ case 6; else echo Did NOT pass $@ case 6; fi
	@if [ `$(IBISEXE) -d $(TESTDIR)/t0 -q "select AVG(c) where -3 < c < 20 || a = '79'" | fgrep '15, 11' | wc -l` -eq 1 ] ; then echo Passed $@ case 7; else echo Did NOT pass $@ case 7; fi
	@if [ `./smatch${EXEEXT} $(TESTDIR)/smatch | fgrep -v std::exception | diff - vault/smatch-1 | wc -l` -eq 0 ] ; then echo Passed $@ case 8; else echo Did NOT pass $@ case 8; fi
	@if [ `./inRange${EXEEXT} $(TESTDIR)/inRange | fgrep -v std::exception | diff - vault/inRange-1 | wc -l` -eq 0 ] ; then echo Passed $@ case 9; else echo Did NOT pass $@ case 9; fi
	@if [ `scripts/printWarning.pl $(TESTDIR)/check-text.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/check-text.log; fi
	@echo
check-marksdb: $(TESTDIR)/marksdb/-part.txt $(IBISEXE)
	@$(IBISEXE) -d $(TESTDIR)/marksdb -b -v -v > $(TESTDIR)/check-marksdb.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/marksdb -q "select lat,lon,3956 * 2 * asin(sqrt(pow(sin((fabs(-10) - fabs(lat)) * 0.0174/2), 2) + cos(fabs(-10) * 0.0174) * cos(fabs(lat) * 0.0174) * pow(sin((112 - lon) * 0.0174/2), 2))) as d where time = 77328 order by d, lon limit 0, 120" -o $(TESTDIR)/marksdb-1 >> $(TESTDIR)/check-marksdb.log 2>&1
	@if [ `scripts/hcompare.pl vault/marksdb-1 $(TESTDIR)/marksdb-1 | fgrep 'acceptable' | wc -l` -eq 1 ] ; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(IBISEXE) -d $(TESTDIR)/marksdb -q "select lat,lon,3956 * 2 * asin(sqrt(pow(sin((fabs(-10) - fabs(lat)) * 0.0174/2), 2) + cos(fabs(-10) * 0.0174) * cos(fabs(lat) * 0.0174) * pow(sin((112 - lon) * 0.0174/2), 2))) as d where (time < 0 || time = 77328) and d <= 12 order by d, lon limit 150" -o $(TESTDIR)/marksdb-2 >> $(TESTDIR)/check-marksdb.log 2>&1
	@if [ `scripts/hcompare.pl $(TESTDIR)/marksdb-1 $(TESTDIR)/marksdb-2 | fgrep acceptable | wc -l` -eq 1 ] ; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(IBISEXE) -d $(TESTDIR)/marksdb -q "select lat,lon where time = 77328 and 3956 * 2 * asin(sqrt(pow(sin((fabs(-10) - fabs(lat)) * 0.0174/2), 2) + cos(fabs(-10) * 0.0174) * cos(fabs(lat) * 0.0174) * pow(sin((112 - lon) * 0.0174/2), 2))) <= 15 order by lat,lon DESC" -o $(TESTDIR)/marksdb-3 >> $(TESTDIR)/check-marksdb.log 2>&1
	@if [ `diff -w vault/marksdb-3 $(TESTDIR)/marksdb-3 | wc -l` -eq 0 ] ; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@$(IBISEXE) -d $(TESTDIR)/marksdb -q "select floor(lat), floor(lon), avg(3956 * 2 * asin(sqrt(pow(sin((fabs(-10) - fabs(lat)) * 0.0174/2), 2) + cos(fabs(-10) * 0.0174) * cos(fabs(lat) * 0.0174) * pow(sin((112 - lon) * 0.0174/2), 2)))) where time = 77328" -o $(TESTDIR)/marksdb-4 >> $(TESTDIR)/check-marksdb.log
	@if [ `scripts/hcompare.pl vault/marksdb-4 $(TESTDIR)/marksdb-4 cnt | fgrep acceptable | wc -l` -eq 1 ] ; then echo Passed $@ case 4; else echo Did NOT pass $@ case 4; fi
	@if [ `scripts/printWarning.pl $(TESTDIR)/check-marksdb.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/check-marksdb.log; fi
	@echo
check-maurel: $(TESTDIR)/maurel/-part.txt $(IBISEXE)
	@echo "Output from tests and data contributed by Alexandre Maurel (`/bin/date`)" > $(TESTDIR)/check-maurel.log
	@$(IBISEXE) -d $(TESTDIR)/maurel -q "select Inhibitor where Inhibitor = 'Imatinib'" -output-with-header $(TESTDIR)/maurel-1a -v -v >> $(TESTDIR)/check-maurel.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/maurel -q "select Inhibitor where Inhibitor in ('Imatinib')" -output-with-header $(TESTDIR)/maurel-1b -v -v >> $(TESTDIR)/check-maurel.log 2>&1
	@if [ -f $(TESTDIR)/maurel-1a -a -f $(TESTDIR)/maurel-1b -a `diff -w $(TESTDIR)/maurel-1a $(TESTDIR)/maurel-1b | wc -l` -eq 0 ]; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(IBISEXE) -no-estimation -d $(TESTDIR)/maurel -q "select Col, Array where (((((( ! Array == 'D09') AND ( ! Array == 'H07')) AND ( ! Array == 'B04')) AND ( ! (Array == 'B04' AND Visual_QC == 'OK'))) AND Visual_QC == 'OK') AND 2 < Col)" -output-with-header $(TESTDIR)/maurel-2a -v -v >> $(TESTDIR)/check-maurel.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/maurel -q "select Col, Array where (((((( ! Array == 'D09') AND ( ! Array == 'H07')) AND ( ! Array == 'B04')) AND ( ! (Array == 'B04' AND Visual_QC == 'OK'))) AND Visual_QC == 'OK') AND 2 < Col)" -output-with-header $(TESTDIR)/maurel-2b -v -v >> $(TESTDIR)/check-maurel.log 2>&1
	@if [ -f $(TESTDIR)/maurel-2a -a -f $(TESTDIR)/maurel-2b -a `diff -w $(TESTDIR)/maurel-2a $(TESTDIR)/maurel-2b | wc -l` -eq 0 ]; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(IBISEXE) -no-estimation -d $(TESTDIR)/maurel -q "select Col, countdistinct(Row), avg(cycle*exposure_time) where  (Visual_QC = 'OK') AND ((! ( Array  = 'D09')) AND (! ( Array  = 'H07')) AND (Array != 'B04')) AND (Col > 2) AND (! ( Array  = 'B04' AND Visual_QC  = 'OK'))" -output-with-header $(TESTDIR)/maurel-3a -v -v >> $(TESTDIR)/check-maurel.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/maurel -q "select Col, countdistinct(Row), avg(cycle*exposure_time) where  (Visual_QC = 'OK') AND ((! ( Array  = 'D09')) AND (! ( Array  = 'H07')) AND !(Array = 'B04')) AND (Col > 2) AND (! ( Array  = 'B04' AND Visual_QC  = 'OK'))" -output-with-header $(TESTDIR)/maurel-3b -v -v >> $(TESTDIR)/check-maurel.log 2>&1
	@if [ -f $(TESTDIR)/maurel-3a -a -f $(TESTDIR)/maurel-3b -a `diff -w $(TESTDIR)/maurel-3a $(TESTDIR)/maurel-3b | wc -l` -eq 0 ]; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@if [ `scripts/printWarning.pl $(TESTDIR)/check-maurel.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/check-maurel.log; fi
	@echo
#
really-small: $(TESTDIR)/wee1/-part.txt $(TESTDIR)/wee2/-part.txt $(TESTDIR)/wee3/-part.txt $(IBISEXE)
	@date >| $(TESTDIR)/really-small.log
	@$(IBISEXE) -d $(TESTDIR)/wee1 -v -q "select distinct(c_1) as n, c_2 where c_1 > 0" -output $(TESTDIR)/small-1 >> $(TESTDIR)/really-small.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/wee2 -v -q "select distinct(c_1) as n, c_2 where c_1 > 0" -output $(TESTDIR)/small-2 >> $(TESTDIR)/really-small.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/wee3 -v -q "select distinct(c_1) as n, c_2 where c_1 > 0" -output $(TESTDIR)/small-3 >> $(TESTDIR)/really-small.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/wee3 -v -q "select distinct(c_1), stdev(c_2) where c_1 > 0" -output $(TESTDIR)/small-4 >> $(TESTDIR)/really-small.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/wee3 -v -q "select distinct(c_1), stdev(c_2) where c_3 = '1010'" -output $(TESTDIR)/small-5 >> $(TESTDIR)/really-small.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/wee3 -v -q "select distinct(c_1), stdev(c_2) where c_3 != '1010'" -output $(TESTDIR)/small-6 >> $(TESTDIR)/really-small.log 2>&1
	@$(IBISEXE) -d $(TESTDIR)/wee3 -v -q "select distinct(c_1), stdev(c_2) where c_3 like '101_'" -output $(TESTDIR)/small-7 >> $(TESTDIR)/really-small.log 2>&1
	@if [ `scripts/printWarning.pl $(TESTDIR)/really-small.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/really-small.log; fi
	@if [ -f $(TESTDIR)/small-1 -a -f vault/small-1 -a `diff -w $(TESTDIR)/small-1 vault/small-1 | wc -l` -eq 0 ]; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@if [ -f $(TESTDIR)/small-2 -a -f vault/small-2 -a `diff -w $(TESTDIR)/small-2 vault/small-2 | wc -l` -eq 0 ]; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@if [ -f $(TESTDIR)/small-3 -a -f vault/small-3 -a `diff -w $(TESTDIR)/small-3 vault/small-3 | wc -l` -eq 0 ]; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@if [ -f $(TESTDIR)/small-4 -a -f vault/small-4 -a `diff -w $(TESTDIR)/small-4 vault/small-4 | wc -l` -eq 0 ]; then echo Passed $@ case 4; else echo Did NOT pass $@ case 4; fi
	@if [ -f $(TESTDIR)/small-5 -a -f vault/small-5 -a `diff -w $(TESTDIR)/small-5 vault/small-5 | wc -l` -eq 0 ]; then echo Passed $@ case 5; else echo Did NOT pass $@ case 5; fi
	@if [ -f $(TESTDIR)/small-6 -a -f vault/small-6 -a `diff -w $(TESTDIR)/small-6 vault/small-6 | wc -l` -eq 0 ]; then echo Passed $@ case 6; else echo Did NOT pass $@ case 6; fi
	@if [ -f $(TESTDIR)/small-7 -a -f vault/small-7 -a `diff -w $(TESTDIR)/small-7 vault/small-7 | wc -l` -eq 0 ]; then echo Passed $@ case 7; else echo Did NOT pass $@ case 7; fi
	@echo
check-join: $(IBISEXE) $(TESTDIR)/t2/t2a/-part.txt $(TESTDIR)/t2/t2b/-part.txt $(TESTDIR)/t2/t2c/-part.txt
	@date >| $(TESTDIR)/check-join.log
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2b -q "from t2a join t2b using a where t2a.a < 10 and t2b.b > 10" | if [ `fgrep ' produced 0 hit' - | wc -l` -eq 1 ] ; then echo Passed $@ case 1; else echo Did NOT pass $@ case 1; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -q "from t2a as s where s.a = t2a.a and s.a < 10 and t2a.b > 5" | if [ `fgrep ' produced 4 hits' - | wc -l` -eq 1 ] ; then echo Passed $@ case 2; else echo Did NOT pass $@ case 2; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -q "select s.a from t2a as s where s.a = t2a.a and s.a < 10 and t2a.b > 5" -o $(TESTDIR)/join-1 | if [ `fgrep ' produced a table with 4 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 3; else echo Did NOT pass $@ case 3; fi
	@if [ -f $(TESTDIR)/join-1 -a -f vault/join-1 -a `diff -w $(TESTDIR)/join-1 vault/join-1 | wc -l` -eq 0 ]; then echo Passed $@ case 4; else echo Did NOT pass $@ case 4; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2b -q "select s.a from t2a as s, t2b as t where s.a = t.a and s.a < 10 and t.b > 5" -o $(TESTDIR)/join-2 | if [ `fgrep ' produced a table with 8 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 5; else echo Did NOT pass $@ case 5; fi
	@if [ -f $(TESTDIR)/join-2 -a -f vault/join-2 -a `diff -w $(TESTDIR)/join-2 vault/join-2 | wc -l` -eq 0 ]; then echo Passed $@ case 6; else echo Did NOT pass $@ case 6; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2c -q "select s.a, t.b+s.c/4 from t2a as s, t2c as t where s.a = t.a and s.a < 10 and t.b < 5" -o $(TESTDIR)/join-3 | if [ `fgrep ' produced a table with 20 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 7; else echo Did NOT pass $@ case 7; fi
	@if [ -f $(TESTDIR)/join-3 -a -f vault/join-3 -a `diff -w $(TESTDIR)/join-3 vault/join-3 | wc -l` -eq 0 ]; then echo Passed $@ case 8; else echo Did NOT pass $@ case 8; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2c -q "select floor(t.b+s.c/4), avg(t.b) from t2a as s, t2c as t where s.a = t.a and s.a < 10 and t.b < 5" -o $(TESTDIR)/join-4 | if [ `fgrep ' produced a table with 4 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 9; else echo Did NOT pass $@ case 9; fi
	@if [ -f $(TESTDIR)/join-4 -a -f vault/join-4 -a `diff -w $(TESTDIR)/join-4 vault/join-4 | wc -l` -eq 0 ]; then echo Passed $@ case 10; else echo Did NOT pass $@ case 10; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -q "select s.a, t2a.a from t2a as s where s.a - t2a.a between 0 and 1 and s.a < 10 and t2a.b > 5" -o $(TESTDIR)/join-5 | if [ `fgrep ' produced a table with 7 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 11; else echo Did NOT pass $@ case 11; fi
	@if [ -f $(TESTDIR)/join-5 -a -f vault/join-5 -a `diff -w $(TESTDIR)/join-5 vault/join-5 | wc -l` -eq 0 ]; then echo Passed $@ case 12; else echo Did NOT pass $@ case 12; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2b -q "select s.a-t.a, distinct(s.a) from t2a as s, t2b as t where t.a - s.a between 0 and 1 and s.a < 10 and t.b > 5" -o $(TESTDIR)/join-6 | if [ `fgrep ' produced a table with 2 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 13; else echo Did NOT pass $@ case 13; fi
	@if [ -f $(TESTDIR)/join-6 -a -f vault/join-6 -a `diff -w $(TESTDIR)/join-6 vault/join-6 | wc -l` -eq 0 ]; then echo Passed $@ case 14; else echo Did NOT pass $@ case 14; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2c -q "select s.a, t.b+s.c/4 from t2a as s, t2c as t where s.a-0.1 < t.a < s.a+1.1 and s.a < 10 and t.b < 5" -o $(TESTDIR)/join-7 | if [ `fgrep ' produced a table with 36 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 15; else echo Did NOT pass $@ case 15; fi
	@if [ -f $(TESTDIR)/join-7 -a -f vault/join-7 -a `diff -w $(TESTDIR)/join-7 vault/join-7 | wc -l` -eq 0 ]; then echo Passed $@ case 16; else echo Did NOT pass $@ case 16; fi
	@$(IBISEXE) -d $(TESTDIR)/t2/t2a -d $(TESTDIR)/t2/t2c -q "select floor(t.b+s.c/4), sum(t.b) from t2a as s, t2c as t where s.a+0.5 <= t.a <= s.a+2.5 and s.a < 10 and t.b < 5" -o $(TESTDIR)/join-8 | if [ `fgrep ' produced a table with 3 rows' - | wc -l` -eq 1 ] ; then echo Passed $@ case 17; else echo Did NOT pass $@ case 17; fi
	@if [ -f $(TESTDIR)/join-8 -a -f vault/join-8 -a `diff -w $(TESTDIR)/join-8 vault/join-8 | wc -l` -eq 0 ]; then echo Passed $@ case 18; else echo Did NOT pass $@ case 18; fi
	@if [ `scripts/printWarning.pl $(TESTDIR)/check-join.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/check-join.log; fi
#
$(TESTDIR)/t0/-part.txt: $(ARDEAEXE) test0.csv
	@rm -rf $(TESTDIR)/t0
	@$(ARDEAEXE) -d $(TESTDIR)/t0 -m "a:key, b:text, c:ushort" -t test0.csv > /dev/null
$(TESTDIR)/t1/-part.txt: $(ARDEAEXE) test0.csv
	@rm -rf $(TESTDIR)/t1
	@$(ARDEAEXE) -d $(TESTDIR)/t1 -m "a:byte, b:float, c:ushort" -t test0.csv > /dev/null
	@$(ARDEAEXE) -d $(TESTDIR)/t1 -m "a:byte, b:float, c:ushort" -t test0.csv > /dev/null
$(TESTDIR)/rowlist: $(TESTDIR)/t1/-part.txt
	@echo 0 > $(TESTDIR)/rowlist; echo 99 >> $(TESTDIR)/rowlist;
$(TESTDIR)/t2/t2a/-part.txt: $(ARDEAEXE) test0.csv
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2a -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
$(TESTDIR)/t2/t2b/-part.txt: $(ARDEAEXE) test0.csv
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2b -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2b -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
$(TESTDIR)/t2/t2c/-part.txt: $(ARDEAEXE) test0.csv
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2c -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2c -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2c -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
	@$(ARDEAEXE) -d $(TESTDIR)/t2/t2c -m "a:byte, b:float, c:ubyte" -t test0.csv > /dev/null
# really small data partitions (wee == very small)
$(TESTDIR)/wee1/-part.txt: $(ARDEAEXE)
	@$(ARDEAEXE) -d $(TESTDIR)/wee1 -m "c_1:s, c_2:f, c_3:k" -r "105, 1.1," > /dev/null
$(TESTDIR)/wee2/-part.txt: $(ARDEAEXE)
	@$(ARDEAEXE) -d $(TESTDIR)/wee2 -m "c_1:s, c_2:f, c_3:k" -r "105, , 1010" -r "105, 1.001, 1010" > /dev/null
$(TESTDIR)/wee3/-part.txt: $(ARDEAEXE)
	@$(ARDEAEXE) -d $(TESTDIR)/wee3 -m "c_1:s, c_2:f, c_3:k" -r "105, 1.0001, 1010" -r "105, 1.00001" -r "105, 1.00001, 1012" > /dev/null
# a simple internal sanity check
sanity-check: $(IBISEXE) $(TESTDIR)/append.log
	@echo
	@if [ -f $(TESTDIR)/append.rc ] ; then \
	$(IBISEXE) -c $(TESTDIR)/append.rc -v -t 2 > $(TESTDIR)/sanity-check.log; \
	else \
	$(IBISEXE) -c $(TESTDIR)/test.rc -v -t 5 > $(TESTDIR)/sanity-check.log; \
	fi
	@if [ `fgrep 'self tests' $(TESTDIR)/sanity-check.log | fgrep 'found no error' | wc -l` -lt 2 -o `scripts/printWarning.pl $(TESTDIR)/sanity-check.log | wc -l` -gt 0 ]; then echo Did NOT pass $@.  Please look through $(TESTDIR)/sanity-check.log.; else echo Passed internal sanity checks.; fi
	@echo
# a test of append operations, 10 copies of data in test0 and test1
append-test: $(TESTDIR)/append.log
	@echo
	@if [ `fgrep ' committing 100' $(TESTDIR)/append.log | fgrep '0/test' | wc -l` -lt 20 ]; then echo Did NOT pass $@..  Please look through $(TESTDIR)/append.log.; else echo Passed append tests.; fi
$(TESTDIR)/append.log:
	$(MAKE) do-small-append
# make 1024 copies of the results of append-test
large-append-test: do-large-append
	@rm -rf $(TESTDIR)/at0 $(TESTDIR)/at1
	@echo
	@if [ `fgrep ' committing 512000' $(TESTDIR)/append.log | wc -l` -lt 2 ]; then echo Did NOT pass $@..  Please look through $(TESTDIR)/append.log.; else echo Passed large append tests.; fi
	@echo
#
ENCODINGS="equality" "range" "interval" "equality ncomp=2" \
	"range ncomp=2" "interval ncomp=2" "equality ncomp=3" \
	"range ncomp=3" "interval ncomp=3" "interval-equality" \
	"range-equality" "equality-equality"
BINNINGS="none" "nbins=10" "nbins=20 equal-weight" "nbins=23 scale=simple" \
	"nbins=30 scale=log" "nbins=40 scale=linear" "precision=1" \
	"nbins=10 reorder" "nbins=27 equal-weight reorder" \
	"nbins=39 scale=simple reorder" "nbins=37 scale=log reorder" \
	"nbins=40 scale=linear reorder" "precision=2 reorder" \
	"start=0, end=1000, nbins=100, scale=simple" \
	"(start=0, end=100, nbins=10, scale=log)(start=100, end=1000, nbins=10, scale=linear)"
#
large-sanity-check: $(IBISEXE) $(TESTDIR)/append.log $(TESTDIR)/smallmem.rc
	@echo "starting $@ with 185 different combinations of indexing options"
	@echo > $(TESTDIR)/large-sanity-check.log
	@jcase=0; for e in $(ENCODINGS) ; do \
	for b in $(BINNINGS) ; do \
	perl -pi -e "s/index = .*/index = <binning $$b\/> <encoding $$e\/>/" $(TESTDIR)/?/*/-part.txt; \
	rm -f $(TESTDIR)/?/*/*.idx $(TESTDIR)/?/*/*.bin; \
	echo >> $(TESTDIR)/large-sanity-check.log; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt >> $(TESTDIR)/large-sanity-check.log; \
	if [ -f $(TESTDIR)/append.rc ] ; then \
	 $(IBISEXE) -c $(TESTDIR)/append.rc -v -b -z >> $(TESTDIR)/large-sanity-check.log; \
	 $(IBISEXE) -c $(TESTDIR)/append.rc -c $(TESTDIR)/smallmem.rc -v -t 2 >> $(TESTDIR)/large-sanity-check.log; \
	test_status=$$?; \
	else \
	 $(IBISEXE) -c $(TESTDIR)/test.rc -v -t 5 >> $(TESTDIR)/large-sanity-check.log; \
	test_status=$$?; \
	fi; jcase=`expr 1 + $${jcase}`; \
	if [ $${test_status} -eq 0 ] ; then echo "  Completed case $${jcase} of $@"; \
	else echo "  Did NOT complete case $${jcase} of $@: $(IBISEXE) exited with $${test_status}"; fi \
	done; \
	done
	@jcase=180; for i in "range/range" "range/bin" "bin/range" "bin/bin" "bitslice" ; do \
	perl -pi -e "s#index = .*#index = $$i#" $(TESTDIR)/?/*/-part.txt; \
	rm -f $(TESTDIR)/?/*/*.idx $(TESTDIR)/?/*/*.bin; \
	echo >> $(TESTDIR)/large-sanity-check.log; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt >> $(TESTDIR)/large-sanity-check.log; \
	if [ -f $(TESTDIR)/append.rc ] ; then \
	 $(IBISEXE) -c $(TESTDIR)/append.rc -v -t 2 >> $(TESTDIR)/large-sanity-check.log; \
	test_status=$$?; \
	else \
	 $(IBISEXE) -c $(TESTDIR)/test.rc -v -t 5 >> $(TESTDIR)/large-sanity-check.log; \
	test_status=$$?; \
	fi; jcase=`expr 1 + $${jcase}`; \
	if [ $${test_status} -eq 0 ] ; then echo "  Completed case $${jcase} of $@"; \
	else echo "  Did NOT complete case $${jcase} of $@: $(IBISEXE) exited with $${test_status}"; fi \
	done;
	@numpasses=`fgrep 'self tests' $(TESTDIR)/large-sanity-check.log | fgrep 'found no error' | wc -l` && if [ $${numpasses} -lt 370 ]; then echo Did NOT pass $@: passed $${numpasses} out 370 cases..  Please look through $(TESTDIR)/large-sanity-check.log.; else echo Passed large internal sanity checks.; fi
	@if [ `scripts/printWarning.pl $(TESTDIR)/large-sanity-check.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/large-sanity-check.log; fi
	@echo
#
match-counts: scripts/matchCounts.pl scripts/query-count.list $(IBISEXE) $(TESTDIR)/test.rc
	@ncases=0; npassed=0; for e in $(ENCODINGS) ; do \
	for b in $(BINNINGS) ; do \
	perl -pi -e "s/index = .*/index = <binning $$b\/> <encoding $$e\/>/" $(TESTDIR)/0/test?/-part.txt; \
	rm -f $(TESTDIR)/0/test?/*.idx $(TESTDIR)/0/test?/*.bin; \
	ncases=`expr 1 + $$ncases`; echo; echo "$@ case # $${ncases}"; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt; \
	$(IBISEXE) -c $(TESTDIR)/test.rc -b -z; \
	scripts/matchCounts.pl "$(IBISEXE) -c $(TESTDIR)/test.rc -v 2" scripts/query-count.list $(TESTDIR) && npassed=`expr 1 + $$npassed`; \
	done; done; \
	perl -pi -e "s/index = .*/index = none/" $(TESTDIR)/0/test?/-part.txt; \
	rm -f $(TESTDIR)/0/test?/*.idx $(TESTDIR)/0/test?/*.bin; \
	ncases=`expr 1 + $$ncases`; echo; echo "$@ case # $${ncases}"; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt; \
	$(IBISEXE) -c $(TESTDIR)/test.rc -b -z; \
	scripts/matchCounts.pl "$(IBISEXE) -c $(TESTDIR)/test.rc -v 2" scripts/query-count.list $(TESTDIR) && npassed=`expr 1 + $$npassed`; echo; \
	if [ $${npassed} -lt $${ncases} ]; then \
	echo Did NOT pass `expr $${ncases} - $${npassed}` out of $${ncases} $@ test cases; \
	else \
	echo Passed $${npassed} out of $${ncases} $@ test cases; \
	fi
	@echo
#
multilevel: scripts/matchCounts.pl scripts/query-count.list $(IBISEXE) $(TESTDIR)/test.rc
	@for i in "range/range" "range/bin" "bin/range" "bin/bin" "bitslice" "none" ; do \
	perl -pi -e "s#index = .*#index = $$i#" $(TESTDIR)/0/test?/-part.txt; \
	rm -f $(TESTDIR)/0/test?/*.idx $(TESTDIR)/0/test?/*.bin; \
	echo; \
	fgrep 'index = ' $(TESTDIR)/0/test1/-part.txt; \
	scripts/matchCounts.pl "$(IBISEXE) -c $(TESTDIR)/test.rc -v 2" scripts/query-count.list $(TESTDIR); \
	done;
	@echo
# testing with enough rows to fill the disk cache
check-sq1: $(THULAEXE) $(TESTDIR)/sq1
	@$(THULAEXE) -d $(TESTDIR)/sq1 -v -t 100 >> $(TESTDIR)/$@.log 2>&1
	@if [ `scripts/printWarning.pl $(TESTDIR)/$@.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/$@.log; fi
$(TESTDIR)/sq1: setqgen$(EXEEXT)
	@setqgen$(EXEEXT) $(TESTDIR)/sq1 -1 -1 >| $(TESTDIR)/check-sq1.log 2>&1
# testing with 1 billion rows, will encounter failures
check-sq2: $(THULAEXE) $(TESTDIR)/sq2
	@$(THULAEXE) -d $(TESTDIR)/sq2 -v -t 100 >> $(TESTDIR)/$@.log 2>&1
	@if [ `scripts/printWarning.pl $(TESTDIR)/$@.log | wc -l` -eq 0 ]; then echo Log file for $@ contains no warning; else echo Log file for $@ contains some warnings, please examine the file $(TESTDIR)/$@.log; fi
$(TESTDIR)/sq: setqgen$(EXEEXT)
	@setqgen$(EXEEXT) $(TESTDIR)/sq 1e9 3e7 >| $(TESTDIR)/check-sq2.log 2>&1
#
$(TESTDIR):
	@test -d $(TESTDIR) || mkdir -p $(TESTDIR)
$(TESTDIR)/0: $(TESTDIR)
	@test -d $(TESTDIR)/0 || mkdir $(TESTDIR)/0
$(TESTDIR)/1: $(TESTDIR)
	@test -d $(TESTDIR)/1 || mkdir $(TESTDIR)/1
$(TESTDIR)/2: $(TESTDIR)
	@test -d $(TESTDIR)/2 || mkdir $(TESTDIR)/2
$(TESTDIR)/marksdb/-part.txt: readcsv$(EXEEXT) marksdb.csv
	-rm -rf $(TESTDIR)/marksdb
	-./readcsv marksdb.csv $(TESTDIR)/marksdb > /dev/null
$(TESTDIR)/maurel/-part.txt: readcsv$(EXEEXT) maurel.csv
	-rm -rf $(TESTDIR)/maurel
	-./readcsv maurel.csv $(TESTDIR)/maurel > /dev/null
$(TESTDIR)/0/test0/-part.txt: $(TESTDIR)/0 test0.csv readcsv$(EXEEXT)
	-./readcsv test0.csv $(TESTDIR)/0/test0 > /dev/null
$(TESTDIR)/0/test1/-part.txt: $(TESTDIR)/0 test1.csv readcsv$(EXEEXT)
	-./readcsv test1.csv $(TESTDIR)/0/test1 > /dev/null
$(TESTDIR)/test.rc: $(TESTDIR)/0/test0/-part.txt $(TESTDIR)/0/test1/-part.txt
	@echo "activeDir=$(TESTDIR)/0" > $(TESTDIR)/test.rc
	@echo "CacheDir=$(TESTDIR)/queryCache" >> $(TESTDIR)/test.rc
$(TESTDIR)/append.rc: $(TESTDIR)
	@echo "activeDir=$(TESTDIR)/1" > $(TESTDIR)/append.rc
	@echo "backupDir=$(TESTDIR)/2" >> $(TESTDIR)/append.rc
	@echo "CacheDir=$(TESTDIR)/queryCache" >> $(TESTDIR)/append.rc
	@echo "useBackupDir=1" >> $(TESTDIR)/append.rc
	@echo "preferMMapIndex=1" >> $(TESTDIR)/append.rc
$(TESTDIR)/smallmem.rc: $(TESTDIR)
#	@echo "fileManager.maxBytes=1.3GB" > $(TESTDIR)/smallmem.rc
	@echo "fileManager.maxBytes=300MB" > $(TESTDIR)/smallmem.rc
# the above configuration statement instructs ibis to limit the amount
# of memory used to 300MB, which is not sufficient for many of the
# operations such as index building.  However, it has been useful to
# test the error handling functions to include this memory restriction.
#
#
clean: clean-am
	-rm -f *~ core core.[0-9]*
	-rm -rf $(TESTDIR) star2002*
clean-tmp:
	-rm -rf $(TESTDIR) .ibis
distclean: distclean-am
	-rm -rf ./$(DEPDIR) readcsv$(EXEEXT) smatch${EXEEXT} inRange${EXEEXT}
	-rm -f Makefile
#
# append the data in test0.csv and test1.csv 10 times
do-small-append: $(TESTDIR)/append.rc $(TESTDIR)/0/test0/-part.txt $(TESTDIR)/0/test1/-part.txt $(IBISEXE)
	@rm -rf $(TESTDIR)/append.log $(TESTDIR)/1 $(TESTDIR)/2
	@/bin/date > $(TESTDIR)/append.log
	@echo Append the test data 10 times >> $(TESTDIR)/append.log
	@for i in 0 1 2 3 4 5 6 7 8 9 ; do \
	$(IBISEXE) -c $(TESTDIR)/append.rc -v -append $(TESTDIR)/0/test0 appendTest0 >> $(TESTDIR)/append.log; \
	$(IBISEXE) -c $(TESTDIR)/append.rc -v -append $(TESTDIR)/0/test1 appendTest1 >> $(TESTDIR)/append.log; \
	done
#
# take the results of do-small-append, double them 10 times
$(TESTDIR)/at0: $(TESTDIR)
	@test -d $(TESTDIR)/at0 || mkdir $(TESTDIR)/at0
$(TESTDIR)/at1: $(TESTDIR)
	@test -d $(TESTDIR)/at1 || mkdir $(TESTDIR)/at1
do-large-append: do-small-append $(TESTDIR)/at0 $(TESTDIR)/at1
	@echo >> $(TESTDIR)/append.log
	@/bin/date >> $(TESTDIR)/append.log
	@echo Double the test data 10 times >> $(TESTDIR)/append.log
	@for i in 0 1 2 3 4 5 6 7 8 9 ; do \
	/bin/cp -pr $(TESTDIR)/2/appendTest0/* $(TESTDIR)/at0; \
	/bin/cp -pr $(TESTDIR)/2/appendTest1/* $(TESTDIR)/at1; \
	$(IBISEXE) -c $(TESTDIR)/append.rc -v -append $(TESTDIR)/at0 appendTest0 >> $(TESTDIR)/append.log; \
	$(IBISEXE) -c $(TESTDIR)/append.rc -v -append $(TESTDIR)/at1 appendTest1 >> $(TESTDIR)/append.log; \
	done
	@$(IBISEXE) -v -reorder $(TESTDIR)/1/appendTest0 \
	-reorder $(TESTDIR)/2/appendTest0 >> $(TESTDIR)/append.log
#
$(IBISEXE):
	@cd $(EXDIR) && $(MAKE) ibis$(EXEEXT)
$(ARDEAEXE):
	@cd $(EXDIR) && $(MAKE) ardea$(EXEEXT)
$(THULAEXE):
	@cd $(EXDIR) && $(MAKE) thula$(EXEEXT)
$(TCAPIEXE):
	@cd $(EXDIR) && $(MAKE) tcapi$(EXEEXT)
#
../src/libfastbit.la::
	cd ../src && $(MAKE) libfastbit.la
#
# these targets do not refer to files
.PHONY: clean clean-tmp multilevel match-counts large-sanity-check large-tests
.PHONY: large-append-test append-test sanity-check full-check more-check check
.PHONY: check-ibis check-thula check-tcapi do-small-append do-large-append
.PHONY: check-marksdb check-maurel small-tests really-small check-sq1 check-sq2

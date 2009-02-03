UTOMAKE_OPTIONS=gnu
EXTRA_PROGRAMS = readcsv
readcsv_SOURCES = readcsv.cpp
check_SCRIPTS = scripts/matchCounts.pl
#
EXDIR=../examples
TESTDIR=$(PWD)/tmp
#
IBISEXE=$(EXDIR)/ibis$(EXEEXT)
ARDEAEXE=$(EXDIR)/ardea$(EXEEXT)
THULAEXE=$(EXDIR)/thula$(EXEEXT)
TCAPIEXE=$(EXDIR)/tcapi$(EXEEXT)
#
dist_data_DATA=test0.csv test1.csv hist0 hist1 hist2 thula-0 thula-1 tcapi-3 \
	scripts/matchCounts.pl scripts/printWarning.pl scripts/query-count.list
#
# tests with different levels of thoroughness
check: clean check-am
	${MAKE} check-ibis && ${MAKE} check-thula && ${MAKE} check-tcapi && ${MAKE} check-text
more-check: check small-tests
full-check: more-check large-tests
#
# small scale tests, may take about ten minutes on a typical Linux machine
small-tests: append-test sanity-check match-counts
#
# the following target excercises larger tests and also excercieses
# obselete index options
# the tests run on the same set of data files and can not be run in parallel
large-tests: large-append-test
	 ${MAKE} multilevel && ${MAKE} large-sanity-check
#
check-thula: $(TESTDIR)/t1/-part.txt $(THULAEXE)
	@rm -f $(TESTDIR)/thula-0 $(TESTDIR)/thula-1
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "a = 0" | if [ `fgrep " produced 2 hits out of 200 records" - | wc -l` -eq 1 ] ; then echo $@ passed test 1; else echo $@ did NOT pass test 1; fi
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "a = b and c < 10" | if [ `fgrep " produced 20 hits out of 200 records" - | wc -l` -eq 1 ] ; then echo $@ passed test 2; else echo $@ did NOT pass test 2; fi
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "c < 10" -s "min(a/b), max(a/b), avg(a/b)" -x $(TESTDIR)/thula-0 > /dev/null && if [ `diff -w thula-0 $(TESTDIR)/thula-0 | wc -l` -eq 0 ] ; then echo $@ passed test 3; else echo $@ did NOT pass test 3; fi
	@$(THULAEXE) -d $(TESTDIR)/t1 -w "a < c and b > 30" -s "b, min(b/c*c), max(sqrt(a/c)^2*c), avg(a/b)" -x $(TESTDIR)/thula-1 > /dev/null && if [ `diff -w thula-1 $(TESTDIR)/thula-1 | wc -l` -eq 0 ] ; then echo $@ passed test 4; else echo $@ did NOT pass test 4; fi
	@echo
check-tcapi: $(TESTDIR)/t1/-part.txt $(TESTDIR)/t0/-part.txt $(TCAPIEXE)
	@$(TCAPIEXE) $(TESTDIR)/t1 "a = 0" | if [ `fgrep " produced 2 hits" - | wc -l` -eq 1 ] ; then echo $@ passed test 1; else echo $@ did NOT pass test 1; fi
	@$(TCAPIEXE) $(TESTDIR)/t1 "a = b and c < 10" | if [ `fgrep " produced 20 hits" - | wc -l` -eq 1 ] ; then echo $@ passed test 2; else echo $@ did NOT pass test 2; fi
	@$(TCAPIEXE) $(TESTDIR)/t0 "c < 10" a l | tail -1 - > $(TESTDIR)/tcapi-3 ; if [ `diff -w tcapi-3 $(TESTDIR)/tcapi-3 | wc -l` -eq 0 ] ; then echo $@ passed test 3; else echo $@ did NOT pass test 3; fi
	@echo
check-ibis: $(IBISEXE) $(TESTDIR)/t1/-part.txt $(TESTDIR)/rowlist
	@rm -f $(TESTDIR)/hist0 $(TESTDIR)/hist1 $(TESTDIR)/hist2
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "where a = 0" | if [ `fgrep " produced 2 hits, " - | wc -l` -eq 1 ] ; then echo $@ passed test 1; else echo $@ did NOT pass test 1; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "where a = b and c < exp(log(9.5))" | if [ `fgrep " produced 20 hits, " - | wc -l` -eq 1 ] ; then echo $@ passed test 2; else echo $@ did NOT pass test 2; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -y $(TESTDIR)/rowlist -q "where a=0" | if [ `fgrep " produced 1 hit, " - | wc -l` -eq 1 ] ; then echo $@ passed test 3; else echo $@ did NOT pass test 3; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "where c < 10" | if [ `fgrep " produced 19 hits, " - | wc -l` -eq 1 ] ; then echo $@ passed test 4; else echo $@ did NOT pass test 4; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -k "a < 2 || c < 3" -q "where a=0" | if [ `fgrep " produced 2 hits, " - | wc -l` -eq 1 ] ; then echo $@ passed test 5; else echo $@ did NOT pass test 5; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -q "where c < 2" | if [ `fgrep " produced 4 hits, " - | wc -l` -eq 1 ] ; then echo $@ passed test 6; else echo $@ did NOT pass test 6; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "c : c>80" > $(TESTDIR)/hist0 && perl -pi -e 's/ in Partition .*//' $(TESTDIR)/hist0 && if [ `diff -w hist0 $(TESTDIR)/hist0 | wc -l` -eq 0 ] ; then echo $@ passed test 7; else echo $@ did NOT pass test 7; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "joint a b : c>50" > $(TESTDIR)/hist1 && if [ `diff -w hist1 $(TESTDIR)/hist1 | wc -l` -eq 0 ] ; then echo $@ passed test 8; else echo $@ did NOT pass test 8; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -p "joint c a b : c>50" > $(TESTDIR)/hist2 && if [ `diff -w hist2 $(TESTDIR)/hist2 | wc -l` -eq 0 ] ; then echo $@ passed test 9; else echo $@ did NOT pass test 9; fi
	@$(IBISEXE) -d $(TESTDIR)/t1 -v -t 5 >| $(TESTDIR)/check-ibis-10.log && if [ `fgrep "found no error" $(TESTDIR)/check-ibis-10.log | wc -l` -eq 1 ] ; then echo $@ passed test 10; else echo $@ did NOT pass test 10; fi
	@echo
check-text: $(IBISEXE) $(TESTDIR)/t0/-part.txt
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a, b where c < 10" -o $(TESTDIR)/t0-0 > /dev/null
	@if [ `awk -F ', ' '{if ($$1 != $$2) print $$0}' $(TESTDIR)/t0-0 | wc -l` -le 2 ] ; then echo $@ passed test 1; else echo $@ did NOT pass test 1; fi
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a, b where c between 10 and 20" -o $(TESTDIR)/t0-1 > /dev/null
	@if [ `awk -F ', ' '{if ($$1 != $$2) print $$0}' $(TESTDIR)/t0-1 | wc -l` -le 2 ] ; then echo $@ passed test 2; else echo $@ did NOT pass test 2; fi
	@$(IBISEXE) -d $(TESTDIR)/t0 -q "select a where c in (11)" -o $(TESTDIR)/t0-2 > /dev/null
	@if [ `tail -1 $(TESTDIR)/t0-2 | fgrep 88 - | wc -l` -eq 1 ] ; then echo $@ passed test 3; else echo $@ did NOT pass test 3; fi
	@echo
$(TESTDIR)/t1/-part.txt: $(ARDEAEXE) test0.csv
	rm -rf $(TESTDIR)/t1
	$(ARDEAEXE) -d $(TESTDIR)/t1 -m "a:int, b:float, c:short" -t test0.csv
	$(ARDEAEXE) -d $(TESTDIR)/t1 -m "a:int, b:float, c:short" -t test0.csv
$(TESTDIR)/t0/-part.txt: $(ARDEAEXE) test0.csv
	rm -rf $(TESTDIR)/t0
	$(ARDEAEXE) -d $(TESTDIR)/t0 -m "a:key, b:text, c:short" -t test0.csv
$(TESTDIR)/rowlist: $(TESTDIR)/t1/-part.txt
	echo 0 > $(TESTDIR)/rowlist; echo 99 >> $(TESTDIR)/rowlist;
# a simple internal sanity check
sanity-check: $(IBISEXE) ${TESTDIR}/append.log
	@echo
	@if [ -f ${TESTDIR}/append.rc ] ; then \
	$(IBISEXE) -c ${TESTDIR}/append.rc -v -t 2 > ${TESTDIR}/sanity-check.log; \
	else \
	$(IBISEXE) -c ${TESTDIR}/test.rc -v -t 5 > ${TESTDIR}/sanity-check.log; \
	fi
	@if [ `fgrep 'self tests' ${TESTDIR}/sanity-check.log | fgrep 'found no error' | wc -l` -lt 2 ]; then echo Errors found during internal sanity checks.  Please look through ${TESTDIR}/sanity-check.log.; else echo Passed internal sanity checks.; fi
	@echo
# a test of append operations, 10 copies of data in test0 and test1
append-test: ${TESTDIR}/append.log
	@echo
	@if [ `fgrep ' 0 error' ${TESTDIR}/append.log | fgrep appendTest | wc -l` -lt 20 ]; then echo Errors found during test of append operations.  Please look through ${TESTDIR}/append.log.; else echo Passed append tests.; fi
${TESTDIR}/append.log:
	$(MAKE) do-small-append
# make 1024 copies of the results of append-test
large-append-test: do-large-append
	rm -rf ${TESTDIR}/at0 ${TESTDIR}/at1
	@echo
	@if [ `fgrep ' 0 error' ${TESTDIR}/append.log | fgrep appendTest | wc -l` -lt 40 ]; then echo Errors found during test of append operations.  Please look through ${TESTDIR}/append.log.; else echo Passed large append tests.; fi
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
large-sanity-check: $(IBISEXE) ${TESTDIR}/append.log ${TESTDIR}/smallmem.rc
	@echo > ${TESTDIR}/large-sanity-check.log
	for e in $(ENCODINGS) ; do \
	for b in $(BINNINGS) ; do \
	perl -pi -e "s/index = .*/index = <binning $$b\/> <encoding $$e\/>/" ${TESTDIR}/?/*/-part.txt; \
	rm -f ${TESTDIR}/?/*/*.idx ${TESTDIR}/?/*/*.bin; \
	echo >> ${TESTDIR}/large-sanity-check.log; \
	fgrep 'index = ' ${TESTDIR}/0/test1/-part.txt; \
	fgrep 'index = ' ${TESTDIR}/0/test1/-part.txt >> ${TESTDIR}/large-sanity-check.log; \
	if [ -f ${TESTDIR}/append.rc ] ; then \
	 $(IBISEXE) -c ${TESTDIR}/append.rc -v -b -z >> ${TESTDIR}/large-sanity-check.log; \
	 $(IBISEXE) -c ${TESTDIR}/append.rc -c ${TESTDIR}/smallmem.rc -v -t 2 >> ${TESTDIR}/large-sanity-check.log; \
	else \
	 $(IBISEXE) -c ${TESTDIR}/test.rc -v -t 5 >> ${TESTDIR}/large-sanity-check.log; \
	fi; \
	done; \
	done
	for i in "range/range" "range/bin" "bin/range" "bin/bin" "bitslice" ; do \
	perl -pi -e "s#index = .*#index = $$i#" ${TESTDIR}/?/*/-part.txt; \
	rm -f ${TESTDIR}/?/*/*.idx ${TESTDIR}/?/*/*.bin; \
	echo >> ${TESTDIR}/large-sanity-check.log; \
	fgrep 'index = ' ${TESTDIR}/0/test1/-part.txt; \
	fgrep 'index = ' ${TESTDIR}/0/test1/-part.txt >> ${TESTDIR}/large-sanity-check.log; \
	if [ -f ${TESTDIR}/append.rc ] ; then \
	 $(IBISEXE) -c ${TESTDIR}/append.rc -v -t 2 >> ${TESTDIR}/large-sanity-check.log; \
	else \
	 $(IBISEXE) -c ${TESTDIR}/test.rc -v -t 5 >> ${TESTDIR}/large-sanity-check.log; \
	fi; \
	done;
	@if [ `fgrep 'self tests' ${TESTDIR}/large-sanity-check.log | fgrep 'found no error' | wc -l` -lt 370 ]; then echo Errors found during internal sanity checks.  Please look through ${TESTDIR}/large-sanity-check.log.; else echo Passed large internal sanity checks.; fi
	@echo
#
match-counts: scripts/matchCounts.pl scripts/query-count.list $(IBISEXE) ${TESTDIR}/test.rc
	ncases=0; npassed=0; for e in $(ENCODINGS) ; do \
	for b in $(BINNINGS) ; do \
	perl -pi -e "s/index = .*/index = <binning $$b\/> <encoding $$e\/>/" ${TESTDIR}/0/test?/-part.txt; \
	rm -f ${TESTDIR}/0/test?/*.idx ${TESTDIR}/0/test?/*.bin; \
	ncases=`expr 1 + $$ncases`; echo; echo "test case # $${ncases}"; \
	fgrep 'index = ' ${TESTDIR}/0/test1/-part.txt; \
	$(IBISEXE) -c $(TESTDIR)/test.rc -b -z; \
	scripts/matchCounts.pl "$(IBISEXE) -c ${TESTDIR}/test.rc -v 2" scripts/query-count.list ${TESTDIR} && npassed=`expr 1 + $$npassed`; \
	done; done; \
	echo; echo $@: passed $${npassed} out of $${ncases} test cases; echo
	@echo
#
multilevel: scripts/matchCounts.pl scripts/query-count.list $(IBISEXE) ${TESTDIR}/test.rc
	for i in "range/range" "range/bin" "bin/range" "bin/bin" "bitslice" ; do \
	perl -pi -e "s#index = .*#index = $$i#" ${TESTDIR}/0/test?/-part.txt; \
	rm -f ${TESTDIR}/0/test?/*.idx ${TESTDIR}/0/test?/*.bin; \
	echo; \
	fgrep 'index = ' ${TESTDIR}/0/test1/-part.txt; \
	scripts/matchCounts.pl "$(IBISEXE) -c ${TESTDIR}/test.rc -v 2" scripts/query-count.list ${TESTDIR}; \
	done;
	@echo
#
${TESTDIR}:
	@test -d ${TESTDIR} || mkdir -p ${TESTDIR}
${TESTDIR}/0: ${TESTDIR}
	@test -d ${TESTDIR}/0 || mkdir ${TESTDIR}/0
${TESTDIR}/1: ${TESTDIR}
	@test -d ${TESTDIR}/1 || mkdir ${TESTDIR}/1
${TESTDIR}/2: ${TESTDIR}
	@test -d ${TESTDIR}/2 || mkdir ${TESTDIR}/2
${TESTDIR}/0/test0/-part.txt: ${TESTDIR}/0 test0.csv readcsv
	./readcsv test0.csv ${TESTDIR}/0/test0
${TESTDIR}/0/test1/-part.txt: ${TESTDIR}/0 test1.csv readcsv
	./readcsv test1.csv ${TESTDIR}/0/test1
${TESTDIR}/test.rc: ${TESTDIR}/0/test0/-part.txt ${TESTDIR}/0/test1/-part.txt
	@echo "activeDir=${TESTDIR}/0" > ${TESTDIR}/test.rc
	@echo "CacheDir=${TESTDIR}/queryCache" >> ${TESTDIR}/test.rc
${TESTDIR}/append.rc: ${TESTDIR}
	@echo "activeDir=${TESTDIR}/1" > ${TESTDIR}/append.rc
	@echo "backupDir=${TESTDIR}/2" >> ${TESTDIR}/append.rc
	@echo "CacheDir=${TESTDIR}/queryCache" >> ${TESTDIR}/append.rc
	@echo "useBackupDir=1" >> ${TESTDIR}/append.rc
	@echo "preferMMapIndex=1" >> ${TESTDIR}/append.rc
${TESTDIR}/smallmem.rc: ${TESTDIR}
#	@echo "fileManager.maxBytes=1.3GB" > ${TESTDIR}/smallmem.rc
	@echo "fileManager.maxBytes=160MB" > ${TESTDIR}/smallmem.rc
# the above configuration statement instructs ibis to limit the amount
# of memory used to 160MB, which is not sufficient for many of the
# operations such index building.  However, it has been useful to test
# the error handling functions to include this memory restriction.
#
#
clean: clean-am
	rm -f *\\~ core core.[0-9]*
	rm -rf ${TESTDIR} .ibis readcsv$(EXEEXT) star2002*
distclean: distclean-am
	-rm -rf ./$(DEPDIR) ${TESTDIR} .ibis readcsv$(EXEEXT)
	-rm -f Makefile
#
# append the data in test0.csv and test1.csv 10 times
do-small-append: ${TESTDIR}/append.rc ${TESTDIR}/0/test0/-part.txt ${TESTDIR}/0/test1/-part.txt $(IBISEXE)
	@rm -rf ${TESTDIR}/append.log ${TESTDIR}/1 ${TESTDIR}/2
	@/bin/date > ${TESTDIR}/append.log
	@echo Append the test data 10 times >> ${TESTDIR}/append.log
	for i in 0 1 2 3 4 5 6 7 8 9 ; do \
	$(IBISEXE) -c ${TESTDIR}/append.rc -v -append ${TESTDIR}/0/test0 appendTest0 >> ${TESTDIR}/append.log; \
	$(IBISEXE) -c ${TESTDIR}/append.rc -v -append ${TESTDIR}/0/test1 appendTest1 >> ${TESTDIR}/append.log; \
	done
#
# take the results of do-small-append, double them 10 times
${TESTDIR}/at0: ${TESTDIR}
	test -d ${TESTDIR}/at0 || mkdir ${TESTDIR}/at0
${TESTDIR}/at1: ${TESTDIR}
	test -d ${TESTDIR}/at1 || mkdir ${TESTDIR}/at1
do-large-append: do-small-append ${TESTDIR}/at0 ${TESTDIR}/at1
	@echo >> ${TESTDIR}/append.log
	@/bin/date >> ${TESTDIR}/append.log
	@echo Double the test data 10 times >> ${TESTDIR}/append.log
	for i in 0 1 2 3 4 5 6 7 8 9 ; do \
	/bin/cp -pr ${TESTDIR}/2/appendTest0/* ${TESTDIR}/at0; \
	/bin/cp -pr ${TESTDIR}/2/appendTest1/* ${TESTDIR}/at1; \
	$(IBISEXE) -c ${TESTDIR}/append.rc -v -append ${TESTDIR}/at0 appendTest0 >> ${TESTDIR}/append.log; \
	$(IBISEXE) -c ${TESTDIR}/append.rc -v -append ${TESTDIR}/at1 appendTest1 >> ${TESTDIR}/append.log; \
	done
#
$(IBISEXE):
	@cd $(EXDIR) && $(MAKE) ibis$(EXEEXT)
$(ARDEAEXE):
	@cd $(EXDIR) && $(MAKE) ardea$(EXEEXT)
$(THULAEXE):
	@cd $(EXDIR) && $(MAKE) thula$(EXEEXT)
$(TCAPIEXE):
	@cd $(EXDIR) && $(MAKE) tcapi$(EXEEXT)
# these targets do not refer to files
.PHONY: clean multilevel match-counts large-sanity-check large-tests small-tests
.PHONY: large-append-test append-test sanity-check full-check more-check check
.PHONY: check-ibis check-thula check-tcapi do-small-append do-large-append

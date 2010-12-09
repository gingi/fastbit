/***************************************************************
 *
 * setqbgen.c - data generator for the set query benchmark
 * <http://www.cs.umb.edu/~poneil/SetQBM.pdf>
 *
 * usage: setqgen <root-data-dir> <#rows> <#rows-per-dir>
 *
 * It generates the data as 32-bit integers for FastBit.  If more than one
 * directory is needed, it will generate a set of subdirectories in the
 * root-data-dir with the names that are a concatenation of the
 * root-data-dir name and hexadecimal version of the partition number.
 ***************************************************************/ 
#include <ibis.h>	/* ibis::init */
#include <string.h>	/* strrchr */
#include <stdlib.h>
#include <stdio.h>
#include <cmath>	/* fmod, ceil */
#include <iomanip>	/* setprecision, setfill */
#include <memory>	/* std::auto_ptr */

/* number of numeric columns to generate data for SETQ is always 13, with
   one as a sequence number */
#define NUMCOLS 12

/** column cardinalities:  # distinct values*/
const int colcard[]={2,4,5,10,25,100,1000,10000,40000,100000,250000,500000};
/** column names */
const char *colname[]={"K2", "K4", "K5", "K10", "K25", "K100", "K1K", "K10K",
		       "K40K", "K100K", "K250K", "K500K", "KSEQ"};

/** A simple random number generator.  It generates the same sequence of
    numbers each time. */
inline int simplerand(void) {
    /* constants for random numbers */
    const static double MODULUS = 2147483647.0;
    const static double MULTIPLIER = 16807.0;
    /* the actual random value */
    static double seed = 1.0;
    seed = std::fmod(MULTIPLIER*seed, MODULUS);
    int ret = (int)seed;
    return ret;
} /* simplerand */

static void fillRow(ibis::table::row& val, uint64_t seq) {
    val.uintsvalues[3]   = (uint32_t)seq;
    val.uintsvalues[2]   = (simplerand() % colcard[11]) + 1;
    val.uintsvalues[1]   = (simplerand() % colcard[10]) + 1;
    val.uintsvalues[0]   = (simplerand() % colcard[9]) + 1;
    val.ushortsvalues[2] = (simplerand() % colcard[8]) + 1;
    val.ushortsvalues[1] = (simplerand() % colcard[7]) + 1;
    val.ushortsvalues[0] = (simplerand() % colcard[6]) + 1;
    val.ubytesvalues[5]  = (simplerand() % colcard[5]) + 1;
    val.ubytesvalues[4]  = (simplerand() % colcard[4]) + 1;
    val.ubytesvalues[3]  = (simplerand() % colcard[3]) + 1;
    val.ubytesvalues[2]  = (simplerand() % colcard[2]) + 1;
    val.ubytesvalues[1]  = (simplerand() % colcard[1]) + 1;
    val.ubytesvalues[0]  = (simplerand() % colcard[0]) + 1;
} // fillRow

static void initColumns(ibis::tablex& tab, ibis::table::row& val) {
    tab.addColumn(colname[0], ibis::UBYTE);
    tab.addColumn(colname[1], ibis::UBYTE);
    tab.addColumn(colname[2], ibis::UBYTE);
    tab.addColumn(colname[3], ibis::UBYTE);
    tab.addColumn(colname[4], ibis::UBYTE);
    tab.addColumn(colname[5], ibis::UBYTE);
    tab.addColumn(colname[6], ibis::USHORT);
    tab.addColumn(colname[7], ibis::USHORT);
    tab.addColumn(colname[8], ibis::USHORT);
    tab.addColumn(colname[9], ibis::UINT);
    tab.addColumn(colname[10], ibis::UINT);
    tab.addColumn(colname[11], ibis::UINT);
    tab.addColumn(colname[12], ibis::UINT, colname[12],
		  "<binning precsion=2 reorder/><encoding equality/>");

    val.clear();
    val.ubytesnames.resize(6);
    val.ubytesvalues.resize(6);
    val.ushortsnames.resize(3);
    val.ushortsvalues.resize(3);
    val.uintsnames.resize(4);
    val.uintsvalues.resize(4);
} // initColumns

int main(int argc, char **argv) {
    const int totcols = NUMCOLS+1;
    int64_t maxrow, nrpd;
    int nparts, ndigits, ierr;

    /* get the number of rows to generate */
    if (argc < 3) {
	fprintf(stderr,
		"Usage: setqgen <fastbit-data-dir> <#rows> [<#rows-per-dir>] \n"
		"\tIf the fourth argument is not provided, all rows will be "
		"placed in one directory\n");
	return -1;
    }

    ibis::init(argc>4?argv[4]:(const char*)0); // initialize the file manager
    ibis::util::timer mytimer(*argv, 0);
    maxrow = atof(argv[2]);
    if (maxrow <= 0) { // determine the number of rows based on cache size
	maxrow = ibis::fileManager::currentCacheSize();
	// the queries in doTest of thula.cpp needs 4 sets of doubles, wich
	// amounts to 32 bytes per row, the Set Query Benchmark data takes
	// 28 bytes per row, the choice below allows all intermediate
	// results to fit in the memory cache
	maxrow = ibis::util::compactValue(maxrow / 80.0, maxrow / 60.0);
    }
    if (maxrow < 100) /* generate at least 100 rows */
	maxrow = 100;
    if (argc > 3) {
	nrpd = atof(argv[3]);
	if (nrpd < 1)
	    nrpd = ibis::util::compactValue(maxrow / 10.0, 1e7);
    }
    else {
	nrpd = (maxrow > 10000000 ? 10000000 : maxrow);
    }
    std::cout << argv[0] << " " << argv[1] << " " << maxrow << " " << nrpd
	      << std::endl;
    nparts = maxrow / nrpd;
    nparts += (maxrow > nparts*nrpd);
    ierr = nparts;
    for (ndigits = 1, ierr >>= 4; ierr > 0; ierr >>= 4, ++ ndigits);
    if (ibis::gVerbose < 1)
	ibis::gVerbose = 1;

    ibis::table::row val;
    std::auto_ptr<ibis::tablex> tab(ibis::tablex::create());
    initColumns(*tab, val);
    tab->reserveSpace(nrpd);
    const uint32_t cap = tab->capacity();

    for (uint64_t irow = 1; irow <= maxrow;) {
	const uint64_t krow = (irow + nrpd < maxrow+1 ? irow+nrpd : maxrow+1);
	std::string dir;
	if (nparts > 1) {
	    const char* str = strrchr(argv[1], '/');
	    if (str != 0) {
		++ str;
	    }
	    else {
		str = argv[1];
	    }
	    std::ostringstream oss;
	    oss << argv[1] << FASTBIT_DIRSEP << str << std::hex
		<< std::setprecision(ndigits) << std::setw(ndigits)
		<< std::setfill('0') << irow / nrpd;
	    dir = oss.str();
	}
	else {
	    dir = argv[1];
	}

	while (irow < krow) {
	    LOGGER(irow % 100000 == 0)
		<< " . " << irow;

	    fillRow(val, irow);
	    ierr = tab->appendRow(val);
	    LOGGER(ierr != totcols && ibis::gVerbose >= 0)
		<< "Warning -- " << *argv << " failed to add values of row "
		<< irow << " to the in-memory table, appendRow returned "
		<< ierr;
	    if (tab->mRows() >= cap) {
		ierr= tab->write(dir.c_str());
		LOGGER(ierr < 0)
		    << "Warning -- " << *argv << " failed to write "
		    << tab->mRows() << " rows to directory "
		    << dir << ", the function write returned " << ierr;

		tab->clearData();
	    }
	    ++ irow;
	}

	if (tab->mRows() > 0) {
	    ierr = tab->write(dir.c_str());
	    LOGGER(ierr < 0)
		<< "Warning -- " << *argv << " failed to write "
		<< tab->mRows() << " rows to directory "
		<< dir << ", the function write returned " << ierr;

	    tab->clearData();
	}
    }

    return 0;
} /* main */


// $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2012 the Regents of the University of California
/** @file thula.cpp

This is a simple test program for the querying functions of ibis::table.
The data must already be on disk and all data directories are treated as
one ibis::table.

Command line arguments
[-c conf-file] [-d directory_containing_a_dataset] [-s select-clause]
[-w where-clause] [-f from-clause] [-v[=| ]verbose_level] [-help]

@note All data directories specified through options -d and -c are treated
as partitions of one data table.

@note Only the last from clause and the last select clause will be used.

@note Multiple where clauses are executed one after another.

Egretta Thula is the Latin name for Snowy Egret, one of John's favorite
birds, see
http://msnucleus.org/watersheds/elizabeth/duck_island.htm for some pictures.
    @ingroup FastBitExamples
*/
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "table.h"	// ibis::table
#include "resource.h"	// ibis::gParameters
#include "mensa.h"	// ibis::mensa::select2
#include <set>		// std::set
#include <iomanip>	// std::setprecision
#include <memory>	// std::auto_ptr
#include <cmath>	// std::floor

// local data types
typedef std::set< const char*, ibis::lessi > qList;

// the export file, used to dump select records or the whole table
std::ofstream xfile;
// the number of test cases to use
int testing = 0;

// printout the usage string
static void usage(const char* name) {
    std::cout << "usage:\n" << name << " [-c conf-file] "
	"[-d directory_containing_a_dataset] [-s select-clause] "
	"[-w where-clause] [-o order-by-clasue] [-f from-clause] "
	"[-v[=| ]verbose_level] [-t[=| ]#-of-cases]"
	"\n\nPerforms a projection of rows satisfying the specified "
	"conditions, a very limited version of SQL"
	"\n  SELECT select-clause FROM from-clause WHERE where-clause."
	"\nEach where-clause will be used in turn."
	"\n\n-- both select clause and where clause may contain "
	"arithmetic expressions."
	"\n-- data in all directories specified by -c and -d "
	"options are considered as one table!"
	"\n-- when multiple select clauses are specified, only "
	"the last one is used."
	"\n-- a from clause specifies what data partitions "
	"participate in the query.  It may contain wild characters '_' and '%'."
	"  When multiple from clauses are specified, only the last one is used."
	      << std::endl;
} // usage

// function to parse the command line arguments
static void parse_args(int argc, char** argv, ibis::table*& tbl,
		       qList& qcnd, const char*& sel, const char*& frm,
		       const char*& ord) {
#if defined(DEBUG) || defined(_DEBUG)
#if DEBUG + 0 > 10 || _DEBUG + 0 > 10
    ibis::gVerbose = INT_MAX;
#elif DEBUG + 0 > 0
    ibis::gVerbose += 7 * DEBUG;
#elif _DEBUG + 0 > 0
    ibis::gVerbose += 5 * _DEBUG;
#else
    ibis::gVerbose += 3;
#endif
#endif
    std::vector<const char*> dirs;

    sel = 0;
    frm = 0;
    ord = 0;
    for (int i=1; i<argc; ++i) {
	if (*argv[i] == '-') { // normal arguments starting with -
	    switch (argv[i][1]) {
	    default:
	    case 'h':
	    case 'H':
		usage(*argv);
		exit(0);
	    case 'c':
	    case 'C':
		if (i+1 < argc) {
		    ++ i;
		    ibis::gParameters().read(argv[i]);
		}
		break;
	    case 'd':
	    case 'D':
		if (i+1 < argc) {
		    ++ i;
		    dirs.push_back(argv[i]);
		}
		break;
	    case 'f':
	    case 'F':
		if (i+1 < argc) {
		    ++ i;
		    frm = argv[i];
		}
		break;
	    case 'q':
	    case 'Q':
	    case 'w':
	    case 'W':
		if (i+1 < argc) {
		    ++ i;
		    qcnd.insert(argv[i]);
		}
		break;
	    case 'o':
	    case 'O':
		if (i+1 < argc) {
		    ++ i;
		    ord = argv[i];
		}
		break;
	    case 's':
	    case 'S':
		if (i+1 < argc) {
		    ++ i;
		    sel = argv[i];
		}
		break;
	    case 'v':
	    case 'V': {
		char *ptr = strchr(argv[i], '=');
		if (ptr == 0) {
		    if (i+1 < argc) {
			if (isdigit(*argv[i+1])) {
			    ibis::gVerbose += atoi(argv[i+1]);
			    i = i + 1;
			}
			else {
			    ++ ibis::gVerbose;
			}
		    }
		    else {
			++ ibis::gVerbose;
		    }
		}
		else {
		    ibis::gVerbose += atoi(++ptr);
		}
		break;}
	    case 't':
	    case 'T': {
		char *ptr = strchr(argv[i], '=');
		if (ptr == 0) {
		    if (i+1 < argc) {
			if (isdigit(*argv[i+1])) {
			    testing += atoi(argv[i+1]);
			    i = i + 1;
			}
			else {
			    ++ testing;
			}
		    }
		    else {
			++ testing;
		    }
		}
		else {
		    testing += atoi(++ptr);
		}
		break;}
	    case 'x':
	    case 'X': 
		if (i+1 < argc) {
		    ++ i;
		    xfile.open(argv[i], std::ios_base::out|std::ios_base::app);
		    if (!xfile)
			std::cerr << *argv << " failed to open \"" << argv[i]
				  << "\" for writing output records"
				  << std::endl;
		}
		break;
	    } // switch (argv[i][1])
	} // normal arguments
	else { // assume to be a set of query conditioins
	    qcnd.insert(argv[i]);
	}
    } // for (inti=1; ...)

    // add the data partitions from configuartion files first
    tbl = ibis::table::create(0);
    // add data partitions from explicitly specified directories
    for (std::vector<const char*>::const_iterator it = dirs.begin();
	 it != dirs.end(); ++ it) {
	if (tbl != 0)
	    tbl->addPartition(*it);
	else
	    tbl = ibis::table::create(*it);
    }
    if (tbl == 0) {
	usage(argv[0]);
	exit(-2);
    }

    if (ibis::gVerbose > 1) {
	tbl->describe(std::cout);
    }
    if (ibis::gVerbose > 1 && ! qcnd.empty()) {
	std::cout << argv[0] << "\nSelect " << (sel ? sel : "count(*)")
		  << "\nFrom " << (frm ? frm : tbl->name()) << "\nWhere -- ";
	for (qList::const_iterator it = qcnd.begin(); it != qcnd.end(); ++it)
	    std::cout  << "\n      " << *it;
	std::cout << std::endl;
    }
} // parse_args

static void clearBuffers(const ibis::table::typeList& tps,
			 std::vector<void*>& buffers) {
    const size_t nc = (tps.size() <= buffers.size() ?
		       tps.size() : buffers.size());
    for (size_t j = 0; j < nc; ++ j) {
	switch (tps[j]) {
	case ibis::BYTE: {
	    signed char* tmp = static_cast<signed char*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::UBYTE: {
	    unsigned char* tmp = static_cast<unsigned char*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::SHORT: {
	    int16_t* tmp = static_cast<int16_t*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::USHORT: {
	    uint16_t* tmp = static_cast<uint16_t*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::INT: {
	    int32_t* tmp = static_cast<int32_t*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::UINT: {
	    uint32_t* tmp = static_cast<uint32_t*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::LONG: {
	    int64_t* tmp = static_cast<int64_t*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::ULONG: {
	    uint64_t* tmp = static_cast<uint64_t*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::FLOAT: {
	    float* tmp = static_cast<float*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::DOUBLE: {
	    double* tmp = static_cast<double*>(buffers[j]);
	    delete [] tmp;
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
		std::vector<std::string>* tmp =
		    static_cast<std::vector<std::string>*>(buffers[j]);
		delete tmp;
		break;}
	default: {
	    break;}
	}
    }
} // clearBuffers

static void dumpIth(size_t i, ibis::TYPE_T t, void* buf) {
    switch (t) {
    case ibis::BYTE: {
	const signed char* tmp = static_cast<const signed char*>(buf);
	std::cout << (int)tmp[i];
	break;}
    case ibis::UBYTE: {
	const unsigned char* tmp = static_cast<const unsigned char*>(buf);
	std::cout << (unsigned)tmp[i];
	break;}
    case ibis::SHORT: {
	const int16_t* tmp = static_cast<const int16_t*>(buf);
	std::cout << tmp[i];
	break;}
    case ibis::USHORT: {
	const uint16_t* tmp = static_cast<const uint16_t*>(buf);
	std::cout << tmp[i];
	break;}
    case ibis::INT: {
	const int32_t* tmp = static_cast<const int32_t*>(buf);
	std::cout << tmp[i];
	break;}
    case ibis::UINT: {
	const uint32_t* tmp = static_cast<const uint32_t*>(buf);
	std::cout << tmp[i];
	break;}
    case ibis::LONG: {
	const int64_t* tmp = static_cast<const int64_t*>(buf);
	std::cout << tmp[i];
	break;}
    case ibis::ULONG: {
	const uint64_t* tmp = static_cast<const uint64_t*>(buf);
	std::cout << tmp[i];
	break;}
    case ibis::FLOAT: {
	const float* tmp = static_cast<const float*>(buf);
	std::cout << std::setprecision(7) << tmp[i];
	break;}
    case ibis::DOUBLE: {
	const double* tmp = static_cast<const double*>(buf);
	std::cout << std::setprecision(15) << tmp[i];
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	const std::vector<std::string>* tmp =
	    static_cast<const std::vector<std::string>*>(buf);
	std::cout << '"' << (*tmp)[i] << '"';
	break;}
    default: {
	break;}
    }
} // dumpIth

// Print the first few rows of a table.  This is meant as an example that
// attempts to read all records into memory.  It is likely faster than
// funtion printValues, but it may be more likely to run out of memory.
static int printValues1(const ibis::table& tbl) {
    if (ibis::gVerbose < 0) return 0;

    const size_t nr = static_cast<size_t>(tbl.nRows());
    if (nr != tbl.nRows()) {
	std::cout << "printValues is unlikely to be able to do it job "
	    "because the number of rows (" << tbl.nRows()
		  << ") is too large for it read all records into memory"
		  << std::endl;
	return -1;
    }

    ibis::table::stringList nms = tbl.columnNames();
    ibis::table::typeList tps = tbl.columnTypes();
    std::vector<void*> buffers(nms.size(), 0);
    for (size_t i = 0; i < nms.size(); ++ i) {
	switch (tps[i]) {
	case ibis::BYTE: {
	    char* buf = new char[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsBytes(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::UBYTE: {
	    unsigned char* buf = new unsigned char[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsUBytes(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::SHORT: {
	    int16_t* buf = new int16_t[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsShorts(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::USHORT: {
	    uint16_t* buf = new uint16_t[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsUShorts(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::INT: {
	    int32_t* buf = new int32_t[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsInts(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::UINT: {
	    uint32_t* buf = new uint32_t[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsUInts(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::LONG: {
	    int64_t* buf = new int64_t[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsLongs(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::ULONG: {
	    uint64_t* buf = new uint64_t[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsULongs(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::FLOAT: {
	    float* buf = new float[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsFloats(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::DOUBLE: {
	    double* buf = new double[nr];
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsDoubles(nms[i], buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
	    std::vector<std::string>* buf = new std::vector<std::string>();
	    if (buf == 0) { // run out of memory
		clearBuffers(tps, buffers);
		return -1;
	    }
	    int64_t ierr = tbl.getColumnAsStrings(nms[i], *buf);
	    if (ierr < 0 || ((size_t) ierr) < nr) {
		clearBuffers(tps, buffers);
		return -2;
	    }
	    buffers[i] = buf;
	    break;}
	default: break;
	}
    }
    if (nms.size() != tbl.nColumns() || nms.size() == 0) return -3;

    size_t nprt = 10;
    if (ibis::gVerbose > 30) {
	nprt = nr;
    }
    else if ((1U << ibis::gVerbose) > nprt) {
	nprt = (1U << ibis::gVerbose);
    }
    if (nprt > nr)
	nprt = nr;

    if (nprt > 0) {
	std::cout << nms[0];
	for (size_t j = 1; j < nms.size(); ++ j)
	    std::cout << ", " << nms[j];
	std::cout << "\n";
    }
    for (size_t i = 0; i < nprt; ++ i) {
	dumpIth(i, tps[0], buffers[0]);
	for (size_t j = 1; j < nms.size(); ++ j) {
	    std::cout << ", ";
	    dumpIth(i, tps[j], buffers[j]);
	}
	std::cout << "\n";
    }
    clearBuffers(tps, buffers);

    if (nprt < nr)
	std::cout << "-- " << (nr - nprt) << " skipped...\n";
    //std::cout << std::endl;
    return 0;
} // printValues1

// This version uses ibis::cursor to print the first few rows.  It is
// likely to be slower than printValues1, but is likely to use less memory
// and less prone to failure.
static int printValues2(const ibis::table& tbl) {
    ibis::table::cursor *cur = tbl.createCursor();
    if (cur == 0) return -1;
    uint64_t nr = tbl.nRows();
    size_t nprt = 10;
    if (ibis::gVerbose > 30) {
	nprt = static_cast<size_t>(nr);
    }
    else if ((1U << ibis::gVerbose) > nprt) {
	nprt = (1U << ibis::gVerbose);
    }
    if (nprt > nr)
	nprt = static_cast<size_t>(nr);
    if (nprt > 0) {
	ibis::table::stringList nms = tbl.columnNames();
	std::cout << nms[0];
	for (size_t j = 1; j < nms.size(); ++ j)
	    std::cout << ", " << nms[j];
	std::cout << "\n";
    }
    int ierr = 0;
    for (size_t i = 0; i < nprt; ++ i) {
	ierr = cur->fetch(); // make the next row ready
	if (ierr == 0) {
	    cur->dump(std::cout, ", ");
	}
	else {
	    std::cout << "printValues2 failed to fetch row " << i << std::endl;
	    ierr = -2;
	    nprt = i;
	    break;
	}
    }
    delete cur; // clean up the cursor

    if (nprt < nr)
	std::cout << "-- " << (nr - nprt) << " skipped...\n";
    //std::cout << std::endl;
    return ierr;
} // printValues2

static void printValues(const ibis::table& tbl) {
    if (tbl.nColumns() == 0 || tbl.nRows() == 0) return;
    int ierr = printValues1(tbl); // try to faster version first
    if (ierr < 0) { // try to the slower version
	ierr = printValues2(tbl);
	if (ierr < 0)
	    std::cout << "printValues failed with error code " << ierr
		      << std::endl;
    }
} // printValues

// evaluate a single query, print out the number of hits
void doQuery(const ibis::table& tbl, const char* wstr, const char* sstr,
	     const char* fstr, const char* ostr) {
    if (wstr == 0 || *wstr == 0) return;

    std::string mesg;
    mesg = "doQuery(";
    mesg += wstr;
    mesg += ')';
    ibis::util::timer atimer(mesg.c_str(), 1);

    uint64_t n0, n1;
    if (ibis::gVerbose > 1 && fstr == 0) {
	tbl.estimate(wstr, n0, n1);
	std::cout << mesg << " -- the estimated number of hits on "
		  << tbl.name() << " is ";
	if (n1 > n0)
	    std::cout << "between " << n0 << " and " << n1 << "\n";
	else
	    std::cout << n1 << "\n";
	if (n1 == 0U) return;
    }
    // function select returns a table containing the selected values
    ibis::table *sel = 0;
    if (fstr == 0 || *fstr == 0) {
	sel = tbl.select(sstr, wstr);
    }
    else {
	const ibis::mensa* mns = dynamic_cast<const ibis::mensa*>(&tbl);
	if (mns != 0) {
	    sel = mns->select2(sstr, wstr, fstr);
	}
	else {
	    std::cout << "Warning -- " << mesg << " can not cast an "
		"abstract ibis::table to the necessary concrete class.  "
		"Will ignore the from clause " << fstr << std::endl;
	    sel = tbl.select(sstr, wstr);
	}
    }
    if (sel == 0) {
	std::cout << mesg << " failed to produce any result"
		  << std::endl;
	return;
    }

    n0 = sel->nRows();
    n1 = tbl.nRows();
    std::cout << mesg << " evaluated on " << tbl.name()
	      << " produced " << n0 << " hit" << (n0>1 ? "s" : "")
	      << " out of " << n1 << " record" << (n1>1 ? "s" : "")
	      << "\n";
    if (ibis::gVerbose >= 0 || (xfile.is_open() && xfile.good())) {
	std::cout << "-- begin printing the result table --\n";
	sel->describe(std::cout); // ask the table to describe itself

	if (n0 > 0 && sel->nColumns() > 0) {
	    if (ostr != 0) {
	        sel->orderby(ostr);
	    }
	    if (xfile.is_open() && xfile.good()) {
		sel->dump(xfile);
	    }
	    else {
		try {
		    size_t nprt = 10;
		    if (ibis::gVerbose > 30) {
			nprt = static_cast<size_t>(n0);
		    }
		    else if ((1U << ibis::gVerbose) > nprt) {
			nprt = (1U << ibis::gVerbose);
		    }
		    if (nprt > n0)
			nprt = static_cast<size_t>(n0);
		    sel->dump(std::cout, nprt);
		}
		catch (...) {
		    printValues(*sel);
		}
	    }
	}
	std::cout  << "-- end printing --\n";
    }
    std::cout << std::endl;

    // exercise the class function ibis::table::select
    if (ibis::gVerbose > 2 && sstr != 0 && *sstr != 0 && n0 > 0 &&
	sel->nColumns() > 0 && (fstr == 0 || *fstr == 0)) {
	std::cout << "\n-- +++ extra test for class function "
	    "ibis::table::select +++ --\n";
	std::vector<const ibis::part*> parts;
	int ierr = tbl.getPartitions(parts);
	if (ierr <= 0) {
	    std::cout << "Warning -- " << mesg << " tbl.getPartitions failed "
		"with error code " << ierr << ", can not proceed with the "
		"test on class function ibis::table::select\n" << std::endl;
	}
	else {
	    std::auto_ptr<ibis::table>
		sel2(ibis::table::select(parts, sstr, wstr));
	    if (sel2.get() == 0) {
		std::cout << "Warning -- " << mesg
			  << "class function ibis::table::select failed\n"
			  << std::endl;
	    }
	    else if (sel2->nRows() != n0 ||
		     sel2->nColumns() != sel->nColumns()) {
		std::cout << "Warning -- " << mesg << " class function "
		    "ibis::table::select return a table with " << sel2->nRows()
			  << " row" << (sel2->nRows()>1?"s":"") << " and "
			  << sel2->nColumns() << " column"
			  << (sel2->nColumns()>1?"s":"") << ", but the member "
		    "function version returned a table of " << n0 << " x "
			  << sel->nColumns() << "\n" << std::endl;
	    }
	    else {
		std::cout << mesg << " passed the test on class function "
		    "ibis::table::select\n" << std::endl;
	    }
	}
    }

    // exercise function groupby on the table sel
    if (sel->nColumns() > 0 && ibis::gVerbose > 0 && sstr != 0 && *sstr != 0
	&& strchr(sstr, '(') == 0) {
	std::cout << "\n-- *** extra test for function groupby *** --\n";
	ibis::table* gb = 0;
	ibis::nameList nl(sstr);
	if (nl.size() > 0) {
	    std::vector<std::string> strs;
	    ibis::table::stringList strc;
	    if (nl.size() == 1) {
#if defined(_DEBUG) || defined(DEBUG)
		strs.resize(1);
		strs[0] = sstr;
#else
		strs.resize(10);
		strs[0] = sstr;
		strs[1] = "min(";     strs[1] += sstr; strs[1] += ')';
		strs[2] = "max(";     strs[2] += sstr; strs[2] += ')';
		strs[3] = "sum(";     strs[3] += sstr; strs[3] += ')';
		strs[4] = "avg(";     strs[4] += sstr; strs[4] += ')';
		strs[5] = "varpop(";  strs[5] += sstr; strs[5] += ')';
		strs[6] = "varsamp("; strs[6] += sstr; strs[6] += ')';
		strs[7] = "stdpop(";  strs[7] += sstr; strs[7] += ')';
		strs[8] = "stdsamp("; strs[8] += sstr; strs[8] += ')';
		strs[9] = "distinct(";strs[9] += sstr; strs[9] += ')';
#endif
	    }
	    else if (nl.size() == 2) {
		strs.resize(10);
		strs[0] = nl[0];
		const char* nm2 = nl[1];
		strs[1] = "min(";     strs[1] += nm2; strs[1] += ')';
		strs[2] = "max(";     strs[2] += nm2; strs[2] += ')';
		strs[3] = "sum(";     strs[3] += nm2; strs[3] += ')';
		strs[4] = "avg(";     strs[4] += nm2; strs[4] += ')';
		strs[5] = "varpop(";  strs[5] += nm2; strs[5] += ')';
		strs[6] = "varsamp("; strs[6] += nm2; strs[6] += ')';
		strs[7] = "stdpop(";  strs[7] += nm2; strs[7] += ')';
		strs[8] = "stdsamp("; strs[8] += nm2; strs[8] += ')';
		strs[9] = "distinct(";strs[9] += nm2; strs[9] += ')';
	    }
	    else {
		strs.resize(nl.size()+8);
		strs[0] = nl[0];
		const char* nm2 = 0;
		size_t i;
		for (i = 1; i < nl.size(); ++ i) {
		    nm2 = nl[i];
		    strs[i] = "avg("; strs[i] += nm2; strs[i] += ')';
		}
		strs[i] = "min(";     strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "max(";     strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "sum(";     strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "varpop(";  strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "varsamp("; strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "stdpop(";  strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "stdsamp("; strs[i] += nm2; strs[i] += ')'; ++ i; 
		strs[i] = "distinct(";strs[i] += nm2; strs[i] += ')'; 
	    }

	    strc.resize(strs.size());
	    for (size_t i = 0; i < strs.size(); ++ i)
		strc[i] = strs[i].c_str();

	    gb = sel->groupby(strc);
	    if (gb == 0) {
		std::cout << "groupby(" << strs[0];
		for (size_t i = 1; i < strs.size(); ++ i)
		    std::cout << ", " << strs[i];
		std::cout << ") failed on table " << sel->name()
			  << std::endl;
	    }
	}

	if (gb != 0) {
	    std::cout << "-- begin output of group by operation --\n";
	    gb->describe(std::cout);

	    if (gb->nRows() > 0 && gb->nColumns() > 0) {
		if (xfile.is_open() && xfile.good()) {
		    gb->dump(xfile);
		}
		else {
		    try {
			gb->dump(std::cout);
		    }
		    catch (...) {
			printValues(*gb);
		    }
		}
	    }
	    std::cout << "--  end  output of group by operation --\n"
		      << std::endl;
	    delete gb;
	}
    }
    delete sel;
} // doQuery

// Construct a random set of tests that may read a large number of records
// into memory.
void doTest(const ibis::table& tbl) {
    std::cout << "Entering doTest with table " << tbl.name() << " ("
	      << tbl.nRows() << " row" << (tbl.nRows()>1?"s":"") << " and "
	      << tbl.nColumns() << " column" << (tbl.nColumns()>1?"s":"")
	      << ")\n";
    if (tbl.nColumns() <= 1 || tbl.nRows() < 10) {
	std::cout << " -- table too small to do anything useful here"
		  << std::endl;
	return;
    }

    const ibis::table::stringList& cols(tbl.columnNames());
    for (int j = 0; j < testing; ++ j) {
	LOGGER(ibis::gVerbose > 2)
	    << "Info -- doTest iteration " << j << " starting with "
	    << ibis::util::groupby1000(ibis::fileManager::bytesInUse())
	    << " bytes in memory cache and "
	    << ibis::util::groupby1000(ibis::fileManager::bytesFree())
	    << " bytes free";

	// select a random column to build a where clause
	int iw = std::floor(ibis::util::rand() * cols.size());
	std::string selmm = "min(";
	selmm += cols[iw];
	selmm += ") as a, max(";
	selmm += cols[iw];
	selmm += ") as b";
	std::auto_ptr<ibis::table> minmax(tbl.select(selmm.c_str(), "1=1"));
	if (minmax.get() == 0) {
	    std::cerr << "Warning -- doTest iteration " << j
		      << " failed to compute the minimum and the maximum of "
		      << cols[iw] << "\n" << std::endl;
	    continue;
	}
	std::vector<double> wmin, wmax;
	int64_t ierr = minmax->getColumnAsDoubles("a", wmin);
	if (ierr < 1) {
	    std::cerr << "Warning -- doTest iteration " << j
		      << " failed to retrieve the minimum value of "
		      << cols[iw] << "\n" << std::endl;
	    continue;
	}
	ierr = minmax->getColumnAsDoubles("b", wmax);
	if (ierr < 1) {
	    std::cerr << "Warning -- doTest iteration " << j
		      << " failed to retrieve the maximum value of "
		      << cols[iw] << "\n" << std::endl;
	    continue;
	}

	std::ostringstream where;
	where << cols[iw] << " <= "
	      << wmin[0] + ibis::util::rand() * (1.0 + wmax[0] - wmin[0]);
	// choose four random columns for the select clause
	std::ostringstream sel;
	sel << "floor(" << cols[(int)(ibis::util::rand() * cols.size())]
	    << "/80), sum(" << cols[(int)(ibis::util::rand() * cols.size())]
	    << "), stdev(" << cols[(int)(ibis::util::rand() * cols.size())]
	    << "), count(" << cols[(int)(ibis::util::rand() * cols.size())]
	    << ") as count0";
	std::auto_ptr<ibis::table>
	    res(tbl.select(sel.str().c_str(), where.str().c_str()));
	if (res.get() == 0) {
	    std::cerr << "Warning -- doTest iteration " << j
		      << " failed to produce a table for \"select " << sel.str()
		      << " from " << tbl.name() << " where " << where.str()
		      << "\"\n" << std::endl;
	    continue;
	}

	std::cout << "doTest iteration " << j << " produced " << res->nRows()
		  << " row" << (res->nRows()>1?"s":"") << std::endl;
	const uint64_t nprt = (res->nRows()>5 ? 5 : res->nRows());
	if (nprt > 0) {
	    res->orderby("count0");
	    res->reverseRows();
	    res->describe(std::cout);
	    if (nprt > 1)
		std::cout << "  the " << nprt
			  << " heaviest rows are as follws:\n";
	    else
		std::cout << "  the heaviest row is as follws:\n";
	    res->dump(std::cout, nprt);
	}
	std::cout << "\n" << std::endl;
    }

    if (testing > 1) {
	ibis::util::logger lg;
	lg() << "doTest completed with the following content in "
	    "the memory cache\n";
	ibis::fileManager::instance().printStatus(lg());
	lg() << "\n";
    }
} // doTest

int main(int argc, char** argv) {
    ibis::table* tbl = 0;
    const char* sel; // only one select clause
    const char* frm; // only one string to select different data partitions
    const char* ord; // only one order clause
    qList qcnd; // list of query conditions (where clauses)

    parse_args(argc, argv, tbl, qcnd, sel, frm, ord);
    if (tbl == 0) {
	std::clog << *argv << " must have at least one data table."
		  << std::endl;
	exit(-1);
    }
    if (qcnd.empty() && xfile.is_open() && xfile.good()) {
	int ierr = tbl->dump(xfile);
	if (ibis::gVerbose >= 0) {
	    if (ierr != 0)
		std::cerr << *argv << " tbl->dump() returned error code "
			  << ierr << std::endl;
	    else
		std::cout << *argv << " successfully exported the content of "
			  << tbl->name() << std::endl;
	}
    }
    else if (qcnd.empty() && testing <= 0) {
	std::clog << *argv << " must have at least one query specified."
		  << std::endl;
	exit(-2);
    }

    // to print the elapsed time from this point on when -v is specified
    ibis::util::timer mytimer(*argv, 1);
    if (testing > 0)
	doTest(*tbl);

    for (qList::const_iterator qit = qcnd.begin();
	 qit != qcnd.end(); ++ qit) {
	doQuery(*tbl, *qit, sel, frm, ord);
    }

    delete tbl;
    return 0;
} // main

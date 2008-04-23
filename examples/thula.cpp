// $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2008 the Regents of the University of California
/** @file thula.cpp

This is a simple test program for the querying functions of
ibis::table.  It assumes that the data is already on disk.

Egretta Thula is the Latin name for Snowy Egret, one of John's favorite
birds.  They nest near his house in Fremont, CA (see
http://msnucleus.org/watersheds/elizabeth/duck_island.htm).
*/
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "table.h"	// ibis::table
#include "resource.h"	// ibis::gParameters
#include <set>		// std::set

// local data types
typedef std::set< const char*, ibis::lessi > qList;

// printout the usage string
static void usage(const char* name) {
    std::cout << "usage:\n" << name << " [-c conf-file] "
	      << "[-d directory_containing_a_dataset] [-s select-clause] "
	      << "[-w where-clause] [-v[=| ]verbose_level]" << std::endl;
} // usage

// Adds a table defined in the named directory.
static void addTables(ibis::tableList& tlist, const char* dir) {
    ibis::table *tbl = ibis::table::create(dir);
    if (tbl == 0) return;
    if (tbl->nRows() != 0 && tbl->nColumns() != 0)
	tlist.add(tbl);
    delete tbl;
} // addTables

// function to parse the command line arguments
static void parse_args(int argc, char** argv, ibis::tableList& tlist,
		       qList& qcnd, const char*& sel) {
    sel = 0;
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
		    // try to sift through the configuration files to find
		    // a data table
		    addTables(tlist, static_cast<const char*>(0));
		}
		break;
	    case 'd':
	    case 'D':
		if (i+1 < argc) {
		    ++ i;
		    addTables(tlist, argv[i]);
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
	    } // switch (argv[i][1])
	} // normal arguments
	else { // assume to be a set of query conditioins
	    qcnd.insert(argv[i]);
	}
    } // for (inti=1; ...)

    if (tlist.empty()) {
	ibis::table *t = ibis::table::create(0);
	if (t != 0) {
	    if (t->nRows() != 0 && t->nColumns() != 0)
		tlist.add(t);
	    delete t;
	}
    }
    if (tlist.empty() || qcnd.empty()) {
	usage(argv[0]);
	exit(-2);
    }

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
    std::cout << "" << argv[0] << "\nqueries: ";
    for (qList::const_iterator it = qcnd.begin(); it != qcnd.end(); ++it)
	std::cout  << " " << *it;
    std::cout << std::endl;
    if (ibis::gVerbose > 0) {
	if (tlist.size()) {
	    std::cout << "Table" << (tlist.size()>1 ? "s:\n" : ":\n");
	    for (ibis::tableList::iterator it = tlist.begin();
		 it != tlist.end(); ++it)
		std::cout << (*it).first << "\n";
	    std::cout << std::endl;
	}
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
	std::cout << tmp[i];
	break;}
    case ibis::DOUBLE: {
	const double* tmp = static_cast<const double*>(buf);
	std::cout << tmp[i];
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
    int ierr;
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
void doQuery(const ibis::table& tbl, const char* wstr, const char* sstr) {
    if (wstr == 0 || *wstr == 0) return;
    if (sstr != 0 && *sstr != 0 && strchr(sstr, '(') != 0) {
	std::cerr << "doQuery(" << wstr << ", " << sstr
		  << ") -- can not process select clause with functions\n"
		  << std::endl;
	return;
    }

    uint64_t n0, n1;
    if (ibis::gVerbose > 0) {
	tbl.estimate(wstr, n0, n1);
	std::cout << "doQuery(" << wstr
		  << ") -- the estimated number of hits on "
		  << tbl.name() << " is ";
	if (n1 > n0)
	    std::cout << "between " << n0 << " and " << n1 << "\n";
	else
	    std::cout << n1 << "\n";
	if (n1 == 0U) return;
    }
    // function select returns a table containing the selected values
    ibis::table *sel = tbl.select(sstr, wstr);
    if (sel == 0) {
	std::cout << "doQuery(" << wstr << ") failed to produce any result"
		  << std::endl;
	return;
    }

    n0 = sel->nRows();
    n1 = tbl.nRows();
    std::cout << "doQuery(" << wstr << ") evaluated on " << tbl.name()
	      << " produced " << n0 << " hit" << (n0>1 ? "s" : "")
	      << " out of " << n1 << " record" << (n1>1 ? "s" : "")
	      << "\n";
    if (ibis::gVerbose > 0) {
	std::cout << "-- begin printing the result table --\n";
	sel->describe(std::cout); // ask the table to describe itself

	if (ibis::gVerbose > 0 && n0 > 0 && sel->nColumns() > 0) {
	    sel->orderby(sstr);
	    if (ibis::gVerbose > 2)
		sel->dump(std::cout);
	    else
		printValues(*sel);
	}
	std::cout  << "-- end printing --\n";
    }
    std::cout << std::endl;

    // test the function groupby
    if (sel->nColumns() > 0) {
	ibis::table* gb = 0;
	ibis::nameList nl(sstr);
	if (nl.size() > 0) {
	    std::vector<std::string> strs;
	    ibis::table::stringList strc;
	    if (nl.size() == 1) {
		strs.resize(4);
		strs[0] = "min("; strs[0] += sstr; strs[0] += ')';
		strs[1] = "max("; strs[1] += sstr; strs[1] += ')';
		strs[2] = "sum("; strs[2] += sstr; strs[2] += ')';
		strs[3] = "avg("; strs[3] += sstr; strs[3] += ')';
	    }
	    else if (nl.size() == 2) {
		strs.resize(5);
		strs[0] = nl[0];
		const char* nm2 = nl[1];
		strs[1] = "min("; strs[1] += nm2; strs[1] += ')';
		strs[2] = "max("; strs[2] += nm2; strs[2] += ')';
		strs[3] = "sum("; strs[3] += nm2; strs[3] += ')';
		strs[4] = "avg("; strs[4] += nm2; strs[4] += ')';
	    }
	    else {
		strs.resize(nl.size()+3);
		strs[0] = nl[0];
		const char* nm2;
		size_t i;
		for (i = 1; i < nl.size(); ++ i) {
		    nm2 = nl[i];
		    strs[i] = "avg("; strs[i] += nm2; strs[i] += ')';
		}
		strs[i] = "min("; strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "max("; strs[i] += nm2; strs[i] += ')'; ++ i;
		strs[i] = "sum("; strs[i] += nm2; strs[i] += ')';
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
		if (ibis::gVerbose > 2)
		    gb->dump(std::cout);
		else if (ibis::gVerbose > 0)
		    printValues(*gb);
	    }
	    std::cout << "--  end  output of group by operation --\n"
		      << std::endl;
	    delete gb;
	}
    }
    delete sel;
} // doQuery

int main(int argc, char** argv) {
    ibis::tableList tlist;
    qList qcnd; // list of properties
    const char* sel;

    parse_args(argc, argv, tlist, qcnd, sel);
    if (tlist.empty()) {
	std::clog << *argv << " must have at least one data table."
		  << std::endl;
	exit(-1);
    }
    if (qcnd.empty()) {
	std::clog << *argv << " must have at least one query specified."
		  << std::endl;
	exit(-2);
    }

    for (ibis::tableList::iterator it = tlist.begin();
	 it != tlist.end();
	 ++it) {
	if ((*it).second->nRows() == 0 ||
	    (*it).second->nColumns() == 0) continue;

	for (qList::const_iterator qit = qcnd.begin();
	     qit != qcnd.end(); ++ qit) {
	    doQuery(*((*it).second), *qit, sel);
	}
    }
    return 0;
} // main

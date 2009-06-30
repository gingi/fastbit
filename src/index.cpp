// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
//
// This file contains the implementation of the classes defined in index.h
// The primary function from the database point of view is a functioin
// called estimate.  It evaluates a given range condition and produces
// two bit vectors representing the range where the actual solution lies.
// The bulk of the code is devoted to maintain and update the indices.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "index.h"
#include "ibin.h"
#include "irelic.h"
#include "idirekte.h"
#include "ikeywords.h"
#include "part.h"
#include "column.h"
#include "resource.h"
#include "bitvector64.h"
#include <queue>	// priority queue
#include <algorithm>	// std::sort
#include <sstream>	// std::ostringstream
#include <typeinfo>	// typeid

namespace ibis {
#if defined(TEST_SUMBINS_OPTIONS)
    //a temporary variable for testing the various options in sumBits
    extern int _sumBits_option;
#endif
}

namespace std { // specialize the std::less struct
    template <> struct less<std::pair<ibis::bitvector*, bool> > {
	bool operator()
	    (const std::pair<ibis::bitvector*, bool> &x,
	     const std::pair<ibis::bitvector*, bool> &y) const {
	    return (x.first->bytes() > y.first->bytes());
	}
    };

    template <> struct less<ibis::bitvector* > {
	bool operator()(const ibis::bitvector *x,
			const ibis::bitvector *y) const {
	    return (x->bytes() < y->bytes());
	}
    };
}

////////////////////////////////////////////////////////////////////////
// functions from ibis::index

/// It creates a specific concrete index object.  If this function
/// fails to read the specified index file, it attempts to create a
/// new index based on the current data file and index specification.
/// The new index will be written under the old name.
///
/// This function returns nil if it fails to create an index.  It captures
/// and absorbs exceptions in most cases.
///
/// @param c a pointer to a ibis::column object.  This argument must be
/// present.
///
/// @param dfname data file name, may also be the name of the index file,
/// or the directory containing the data file.  If the name ends with
/// '.idx' is treated as an index file, and the content of the file is
/// read.  If the name does not end with '.idx', it is assumed to be the
/// data file name, unless it is determined to be a directory name.  If it
/// is a directory name, the data file is assumed to be in the specified
/// directory with the same name as the column.  Once a data file is found,
/// the content of the data file is read to construct a new index according
/// to the return value of function indexSpec.  The argument dfname can be
/// nil, in which case, the data file name is constructed by concatenate
/// the return of partition()->currentDataDir() and the column name.
///
/// @note Set @c name to null to build a brand new index and discard
/// the existing index.
///
/// @param spec the index specification.  This string contains the
/// parameters for how to create an index.  The most general form is
///\verbatim
/// <binning .../> <encoding .../> <compression .../>.
///\endverbatim
/// Here is one example (it is the default for some integer columns)
///\verbatim
/// <binning none /> <encoding equality />
///\endverbatim
/// FastBit always compresses every bitmap it ever generates.  The
/// compression option is to instruct it to uncompress some bitmaps or
/// not compress indices at all.  The compress option is usually not
/// used.
///
/// If the argument @c spec is not specified, this function checks the
/// specification in the following order.
/// <ol>
/// <li> use the index specification for the column being indexed;
/// <li> use the index specification for the table containing the
/// column being indexed;
/// <li> use the most specific index specification relates to the
/// column be indexed in the global resources (gParameters).
/// </ol>
/// It stops looking as soon as it finds the first non-empty string.
/// To override any general index specification, one must provide a
/// complete index specification string.
///
/// @param inEntirety If this value is greater than zero, this function
/// will attempt to read the whole index file in one shot.  In cases where
/// there is enough memory to hold all indexes in-memory, this option
/// may reduce I/O overhead.  Additionally, it places all bitmaps in an
/// index consecutively in memory, which may also speed up memory accesses
/// when the bitmaps are used to answer queries.  Of course, the drawback
/// is that this option requires more memory than necessary and may
/// actually take more time to read the data files since many of the
/// bitmaps may not be actually needed.  The default value of this argument
/// is 0, which allows bitmaps to be read into memory as needed.
///
/// @note An index can not be built correctly if it does not fit in memory!
/// This is the most likely reason for failure in this function.  If this
/// does happen, try to build indexes one at a time, use a machine with
/// more memory, or break up a large partition into a number of smaller
/// ones.  Normally, we recommand one to not put much more than 100 million
/// rows in a data partition.
ibis::index* ibis::index::create(const ibis::column* c, const char* dfname,
				 const char* spec, int inEntirety) {
    ibis::index* ind = 0;
    if (c == 0) // can not procede
	return ind;
    if (c->partition() == 0)
	return ind;
    if (c->partition()->nRows() == 0)
	return ind;

    if (spec == 0 || *spec == static_cast<char>(0))
	spec = c->indexSpec(); // index spec of the column
    if (spec == 0 || *spec == static_cast<char>(0))
	spec = c->partition()->indexSpec(); // index spec of the table
    if (spec == 0 || *spec == static_cast<char>(0)) {
	// attempt to retrieve the value of tableName.columnName.index for
	// the index specification in the global resource
	std::string idxnm(c->partition()->name());
	idxnm += '.';
	idxnm += c->name();
	idxnm += ".index";
	spec = ibis::gParameters()[idxnm.c_str()];
    }
    if (spec) {
	// no index is to be used if the index specification start
	// with "noindex", "null" or "none".
	if (strncmp(spec, "noindex", 7) == 0 ||
	    strncmp(spec, "null", 4) == 0 ||
	    strncmp(spec, "none", 4) == 0) {
	    return ind;
	}
    }
    else {
	switch (c->type()) {
	case ibis::ULONG:
	case ibis::LONG:
	case ibis::USHORT:
	case ibis::SHORT:
	case ibis::UINT:
	case ibis::INT:
	case ibis::FLOAT:
	case ibis::DOUBLE:
	case ibis::UBYTE:
	case ibis::BYTE:
	case ibis::CATEGORY:
	case ibis::TEXT: {
	    spec = "default";
	    break;}
	default: {
	    c->logWarning("createIndex", "not able to "
			  "generate for this column type");
	    return ind;}
	}
    }
    const bool usebin = (strstr(spec, "bin") != 0 &&
			 strstr(spec, "none") == 0);
    if (ibis::gVerbose > 3)
	c->logMessage("index::create", "invoking the index "
		      "factory with spec=`%s' and source=%s)", spec,
		      (dfname ? dfname : c->partition()->currentDataDir()));

    bool isRead = false;
    uint32_t ncomp = 0;
    ibis::horometer timer;
    if (ibis::gVerbose > 1)
	timer.start();
    const char* ptr = strstr(spec, "ncomp=");
    if (ptr != 0) {
	ptr += 6;
	while (isspace(*ptr)) { // skip till the first digit
	    ++ ptr;
	}
	if (*ptr) {
	    if (isdigit(*ptr)) { // string --> number
		ncomp = atoi(ptr);
		if (ncomp == 0) {
		    if (errno == EINVAL)
			c->logMessage("createIndex",
				      "atio(%s) failed", ptr);
		    ncomp = 2; // default to 2
		}
	    }
	    else {
		ncomp = 1;
	    }
	}
    }
    try {
	if (dfname != 0 && *dfname != 0) { // first attempt to read the index
	    int ierr;
	    std::string file;
	    ibis::fileManager::storage* st=0;
	    file = dfname;
	    Stat_T st0;
	    if (UnixStat(dfname, &st0)) { // file/directory doesn't exist
		ierr = strlen(dfname);
		if (ierr <= 4 || dfname[ierr-1] != 'x' || dfname[ierr-2] != 'd'
		    || dfname[ierr-3] != 'i' || dfname[ierr-4] != '.') {
		    // dfname doesn't end with .idx extension
		    bool isFile = false;
		    int len = strlen(c->name());
		    if (ierr >= len) {
			const char* tail = dfname + (ierr - len);
			isFile = (strcmp(tail, c->name()) == 0);
		    }
		    if (isFile) { // dfname is the data file name
			file += ".idx";
		    }
		    else { // assume to be a nonexistent dir
			return ind;
		    }
		}
	    }
	    else if ((st0.st_mode & S_IFDIR) == S_IFDIR) { // an existing dir
		file += FASTBIT_DIRSEP;
		file += c->name();
		file += ".idx";
	    }
	    else { // assumes to be an existing file
		file += ".idx";
	    }

	    char buf[12];
	    const char* header = 0;
	    {
		bool useGetFile = (inEntirety != 0);
		if (! useGetFile) {
		    std::string key(c->partition()->name());
		    key += ".";
		    key += c->name();
		    key += ".preferMMapIndex";
		    useGetFile = ibis::gParameters().isTrue(key.c_str());
		}
		if (useGetFile) {
		    // manage the index file as a whole
		    ierr = ibis::fileManager::instance().tryGetFile
			(file.c_str(), &st,
			 (inEntirety > 0 ?
			  ibis::fileManager::PREFER_READ :
			  ibis::fileManager::MMAP_LARGE_FILES));
		    if (ierr != 0) {
			LOGGER(ibis::gVerbose > 7)
			    << "index::create tryGetFile(" << file
			    << " failed with return code " << ierr;
			st = 0;
		    }
		    if (st)
			header = st->begin();
		}
	    }
	    if (header == 0) {
		// attempt to read the file using read(2)
		int fdes = UnixOpen(file.c_str(), OPEN_READONLY);
		if (fdes >= 0) {
#if defined(_WIN32) && defined(_MSC_VER)
		    (void)_setmode(fdes, _O_BINARY);
#endif
		    if (8 == UnixRead(fdes, static_cast<void*>(buf), 8)) {
			header = buf;
		    }
		    UnixClose(fdes);
		}
	    }
	    if (header) { // verify header
		const bool check = (header[0] == '#' && header[1] == 'I' &&
				    header[2] == 'B' && header[3] == 'I' &&
				    header[4] == 'S' &&
				    header[6] == (char)sizeof(int32_t) &&
				    header[7] == static_cast<char>(0));
		if (!check) {
		    c->logWarning("readIndex", "index file \"%s\" "
				  "contains an incorrect header "
				  "(%c%c%c%c%c:%i.%i.%i)",
				  file.c_str(),
				  header[0], header[1], header[2],
				  header[3], header[4],
				  (int)header[5], (int)header[6],
				  (int)header[7]);
		    header = 0;
		}
	    }

	    if (header) { // reconstruct index from st
		isRead = true;
		switch (static_cast<ibis::index::INDEX_TYPE>(header[5])) {
		case ibis::index::BINNING: // ibis::bin
		    if (st) {
			ind = new ibis::bin(c, st);
		    }
		    else {
			ind = new ibis::bin(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::RANGE: // new range index
		    if (st) {
			ind = new ibis::range(c, st);
		    }
		    else {
			ind = new ibis::range(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::AMBIT: // multilevel range index
		    if (st) {
			ind = new ibis::ambit(c, st);
		    }
		    else {
			ind = new ibis::ambit(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::PALE: // two-level bin/range index
		    if (st) {
			ind = new ibis::pale(c, st);
		    }
		    else {
			ind = new ibis::pale(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::PACK: // two-level range/bin index
		    if (st) {
			ind = new ibis::pack(c, st);
		    }
		    else {
			ind = new ibis::pack(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::ZONE: // two-level bin/bin index
		    if (st) {
			ind = new ibis::zone(c, st);
		    }
		    else {
			ind = new ibis::zone(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::MESA: // one-level (interval) encoded index
		    if (st) {
			ind = new ibis::mesa(c, st);
		    }
		    else {
			ind = new ibis::mesa(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::RELIC: // the basic bitmap index
		    if (st) {
			ind = new ibis::relic(c, st);
		    }
		    else {
			ind = new ibis::relic(c, file.c_str());
		    }
		    break;
		case ibis::index::SLICE: // the bit-sliced index
		    if (st) {
			ind = new ibis::slice(c, st);
		    }
		    else {
			ind = new ibis::slice(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::FADE: // multicomponent range-encoded
		    if (st) {
			ind = new ibis::fade(c, st);
		    }
		    else {
			ind = new ibis::fade(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::SAPID: // multicomponent equality-encoded
		    if (st) {
			ind = new ibis::sapid(c, st);
		    }
		    else {
			ind = new ibis::sapid(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::SBIAD: // multicomponent interval-encoded
		    if (st) {
			ind = new ibis::sbiad(c, st);
		    }
		    else {
			ind = new ibis::sbiad(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::EGALE:
		    // multicomponent equality code on bins
		    if (st) {
			ind = new ibis::egale(c, st);
		    }
		    else {
			ind = new ibis::egale(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::MOINS:
		    // multicomponent equality code on bins
		    if (st) {
			ind = new ibis::moins(c, st);
		    }
		    else {
			ind = new ibis::moins(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::ENTRE:
		    // multicomponent equality code on bins
		    if (st) {
			ind = new ibis::entre(c, st);
		    }
		    else {
			ind = new ibis::entre(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::BAK:
		    // equality code on reduced precision values
		    if (st) {
			ind = new ibis::bak(c, st);
		    }
		    else {
			ind = new ibis::bak(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::BAK2:
		    // equality code on reduced precision values, split bins
		    if (st) {
			ind = new ibis::bak2(c, st);
		    }
		    else {
			ind = new ibis::bak2(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::KEYWORDS:
		    // boolean term-document matrix
		    if (st) {
			ind = new ibis::keywords(c, st);
		    }
		    else {
			const ibis::column* idcol =
			    reinterpret_cast<const ibis::text*>(c)->
			    IDColumnForKeywordIndex();
			ind = new ibis::keywords(c, idcol, file.c_str());
		    }
		    break;
		case ibis::index::DIREKTE:
		    if (st) {
			ind = new ibis::direkte(c, st);
		    }
		    else {
			ind = new ibis::direkte(0);
			ind->col = c;
			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::BYLT:
		    if (st) {
			ind = new ibis::bylt(c, st);
		    }
		    else {
			ind = new ibis::bylt(c, file.c_str());
			// 			ind = new ibis::bylt(0);
			// 			ind->col = c;
			// 			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::ZONA:
		    if (st) {
			ind = new ibis::zona(c, st);
		    }
		    else {
			ind = new ibis::zona(c, file.c_str());
			// 			ind = new ibis::zona(0);
			// 			ind->col = c;
			// 			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::FUZZ:
		    if (st) {
			ind = new ibis::fuzz(c, st);
		    }
		    else {
			ind = new ibis::fuzz(c, file.c_str());
			// 			ind = new ibis::fuzz(0);
			// 			ind->col = c;
			// 			ind->read(file.c_str());
		    }
		    break;
		case ibis::index::FUGE:
		    if (st) {
			ind = new ibis::fuge(c, st);
		    }
		    else {
			ind = new ibis::fuge(c, file.c_str());
		    }
		    break;
		default:
		    c->logWarning("readIndex", "ibis::index::create(%s) "
				  "index type (%d) is not supported",
				  file.c_str(), (int)header[5]);
		}
	    }
	    else if (c->type() == ibis::CATEGORY) {
		// there should be an index already!
		return ind;
	    }
	    else if (c->type() == ibis::TEXT) {
		const ibis::column* idcol =
		    reinterpret_cast<const ibis::text*>(c)->
		    IDColumnForKeywordIndex();
		ind = new ibis::keywords(c, idcol, file.c_str());
		return ind;
	    }
	    else { // need to build a new index from attribute values
		file.erase(file.size()-4); // remove ".idx" extension
		bool dflt = false;
		if (c->type() == ibis::CATEGORY) {
		    dflt = true;
		}
		else if (spec == 0) {
		    dflt = true;
		}
		else if (*spec == 0) {
		    dflt = true;
		}
		else if (strstr(spec, "automatic") != 0 ||
			 strstr(spec, "default") != 0) {
		    dflt = true;
		}
		else {
		    dflt = (0 != isspace(*spec));
		}

		if (dflt) {
		    switch (c->type()) {
		    case ibis::ULONG:
		    case ibis::LONG:
		    case ibis::USHORT:
		    case ibis::SHORT:
		    case ibis::UINT:
		    case ibis::INT: {
			double amin = c->lowerBound();
			double amax = c->upperBound();
			if (amin >= amax && amin >= 0) {
			    c->computeMinMax(c->partition()->currentDataDir(),
					     amin, amax);
			}
			if (amax - amin < 1e3 ||
			    amax - amin < c->partition()->nRows()*0.1) {
			    if (amin >= 0.0 && amin <= ceil(amax*0.01))
				ind = new ibis::direkte(c, file.c_str());
			    else if (amax >= amin+1e2)
				ind = new ibis::fuzz(c, file.c_str());
			    else
				ind = new ibis::relic(c, file.c_str());
			}
			else {
			    ind = new ibis::bin(c, file.c_str());
			}
			break;}
		    case ibis::FLOAT:
		    case ibis::DOUBLE: {
			ind = new ibis::bin(c, file.c_str());
			break;}
		    case ibis::UBYTE:
		    case ibis::BYTE:
		    case ibis::CATEGORY: {
			ind = new ibis::relic(c, file.c_str());
			break;}
		    case ibis::TEXT: {
			const ibis::column* idcol =
			    reinterpret_cast<const ibis::text*>(c)->
			    IDColumnForKeywordIndex();
			ind = new ibis::keywords(c, idcol, file.c_str());
			break;}
		    default: {
			c->logWarning("createIndex", "not able to "
				      "generate for this column type");
			break;}
		    }
		}
		else if (ncomp > 1 || strstr(spec, "mcbin") != 0 ||
			 strstr(spec, "multicomponent") != 0) {
		    INDEX_TYPE t = SAPID; // default to equality encoding
		    if (strstr(spec, "equal")) {
			t = SAPID;
		    }
		    else if (strstr(spec, "range")) {
			t = FADE;
		    }
		    else if (strstr(spec, "interval")) {
			t = SBIAD;
		    }
		    switch (t) {
		    default:
		    case SBIAD:
			if (usebin)
			    ind = new ibis::entre(c, file.c_str(), ncomp);
			else
			    ind = new ibis::sbiad(c, file.c_str(), ncomp);
			break;
		    case SAPID:
			if (usebin)
			    ind = new ibis::egale(c, file.c_str(), ncomp);
			else
			    ind = new ibis::sapid(c, file.c_str(), ncomp);
			break;
		    case FADE:
			if (usebin)
			    ind = new ibis::moins(c, file.c_str(), ncomp);
			else
			    ind = new ibis::fade(c, file.c_str(), ncomp);
			break;
		    }
		}
		else if (!usebin) { // <binning none> is specified explicitly
		    INDEX_TYPE t = RELIC;
		    const char* str = strstr(spec, "<encoding ");
		    if (str) {
			str += 10; // skip "<encoding "
			if (strstr(str, "range/equality") ||
			    strstr(str, "range-equality")) {
			    t = BYLT;
			}
			else if (strstr(str, "equality/equality") ||
				 strstr(str, "equality-equality")) {
			    t = ZONA;
			}
			else if (strstr(str, "interval/equality") ||
				 strstr(str, "interval-equality")) {
			    t = FUZZ;
			}
			else if (strstr(str, "equal")) {
			    t = SAPID;
			}
			else if (strstr(str, "interval")) {
			    t = SBIAD;
			}
			else if (strstr(str, "range")) {
			    t = FADE;
			}
			else if (strstr(str, "binary")) {
			    t = SLICE;
			}
		    }
		    else if (stricmp(spec, "index=simple") == 0 ||
			     stricmp(spec, "index=basic") == 0 ||
			     strstr(spec, "relic") != 0) {
			t = RELIC;
		    }
		    else if (strstr(spec, "slice") != 0 ||
			     strstr(spec, "bitslice") != 0 ||
			     strstr(spec, "binary") != 0) {
			t = SLICE;
		    }
		    else {
			t = SAPID;
		    }
		    switch (t) {
		    default:
		    case SAPID:
			if (ncomp > 1)
			    ind = new ibis::sapid(c, file.c_str(), ncomp);
			else if ((c->type() != ibis::FLOAT &&
				  c->type() != ibis::DOUBLE &&
				  c->type() != ibis::TEXT) &&
				 c->lowerBound() >= 0.0 &&
				 c->lowerBound() <=
				 ceil(c->upperBound()*0.01) &&
				 c->upperBound() <= c->partition()->nRows())
			    ind = new ibis::direkte(c, file.c_str());
			else
			    ind = new ibis::relic(c, file.c_str());
			break;
		    case RELIC:
			ind = new ibis::relic(c, file.c_str());
			break;
		    case FADE:
			ind = new ibis::fade(c, file.c_str(), ncomp);
			break;
		    case SBIAD:
			ind = new ibis::sbiad(c, file.c_str(), ncomp);
			break;
		    case SLICE:
			ind = new ibis::slice(c, file.c_str());
			break;
		    case BYLT:
			ind = new ibis::bylt(c, file.c_str());
			break;
		    case ZONA:
			ind = new ibis::zona(c, file.c_str());
			break;
		    case FUZZ:
			ind = new ibis::fuzz(c, file.c_str());
			break;
		    }
		}
		else if (strstr(spec, "slice") != 0 ||
			 strstr(spec, "bitslice") != 0 ||
			 strstr(spec, "binary") != 0) { // ibis::slice
		    ind = new ibis::slice(c, file.c_str());
		}
		else if (stricmp(spec, "index=simple") == 0 ||
			 stricmp(spec, "index=basic") == 0 ||
			 strstr(spec, "relic") != 0) {
		    if ((c->type() != ibis::FLOAT &&
			 c->type() != ibis::DOUBLE &&
			 c->type() != ibis::TEXT) &&
			c->lowerBound() >= 0.0 &&
			c->lowerBound() <= ceil(c->upperBound()*0.01) &&
			c->upperBound() <= c->partition()->nRows())
			ind = new ibis::direkte(c, file.c_str());
		    else
			ind = new ibis::relic(c, file.c_str());
		}
		else if (strstr(spec, "fade") != 0 ||
			 strstr(spec, "multi-range") != 0) {
		    ind = new ibis::fade(c, file.c_str());
		}
		else if (strstr(spec, "sapid") != 0 ||
			 strstr(spec, "multi-equal") != 0) {
		    ind = new ibis::sapid(c, file.c_str());
		}
		else if (strstr(spec, "sbiad") != 0 ||
			 strstr(spec, "multi-interval") != 0) {
		    ind = new ibis::sbiad(c, file.c_str());
		}
		else if (strstr(spec, "egale") != 0) {
		    ind = new ibis::egale(c, file.c_str());
		}
		else if (strstr(spec, "moins") != 0) {
		    ind = new ibis::moins(c, file.c_str());
		}
		else if (strstr(spec, "entre") != 0) {
		    ind = new ibis::entre(c, file.c_str());
		}
		else if (strstr(spec, "ambit") != 0 ||
			 strstr(spec, "range/range") != 0 ||
			 strstr(spec, "range-range") != 0) {
		    ibis::bin tmp(c, file.c_str());
		    ind = new ibis::ambit(tmp);
		    // ind = new ibis::ambit(c, file.c_str());
		}
		else if (strstr(spec, "pale") != 0 ||
			 strstr(spec, "bin/range") != 0 ||
			 strstr(spec, "equality-range") != 0) {
		    ibis::bin tmp(c, file.c_str());
		    ind = new ibis::pale(tmp);
		}
		else if (strstr(spec, "pack") != 0 ||
			 strstr(spec, "range/bin") != 0 ||
			 strstr(spec, "range/equality") != 0 ||
			 strstr(spec, "range-equality") != 0) {
		    ibis::bin tmp(c, file.c_str());
		    ind = new ibis::pack(tmp);
		}
		else if (strstr(spec, "zone") != 0 ||
			 strstr(spec, "bin/bin") != 0 ||
			 strstr(spec, "equality/equality") != 0 ||
			 strstr(spec, "equality-equality") != 0) {
		    ibis::bin tmp(c, file.c_str());
		    ind = new ibis::zone(tmp);
		}
		else if (strstr(spec, "interval/equality") != 0 ||
			 strstr(spec, "interval-equality") != 0) {
		    ind = new ibis::fuge(c, file.c_str());
		}
		else if (strstr(spec, "bak2") != 0) {
		    ind = new ibis::bak2(c, file.c_str());
		}
		else if (strstr(spec, "bak") != 0) {
		    ind = new ibis::bak(c, file.c_str());
		}
		else if (strstr(spec, "mesa") != 0 ||
			 strstr(spec, "interval") != 0 ||
			 strstr(spec, "2sided") != 0) {
		    ibis::bin tmp(c, file.c_str());
		    ind = new ibis::mesa(tmp);
		}
		else if (strstr(spec, "range") != 0 ||
			 strstr(spec, "cumulative") != 0) {
		    ind = new ibis::range(c, file.c_str());
		}
		else {
		    if (strstr(spec, "bin") == 0)
			c->logWarning("createIndex", "can not process bin "
				      "spec \"%s\", use simple bins", spec);
		    ind = new ibis::bin(c, file.c_str());
		}

		if (ind == 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "ibis::index::create failed to create an index for "
			<< c->name() << " with " << file;
		}
		else if (ind->getNRows() == 0) {
		    delete ind;
		    ind = 0;
		    LOGGER(ibis::gVerbose > 0)
			<< "ibis::index::create create an empty index for "
			<< c->name() << " with " << file;
		}
		else if (ind->getNRows() == c->partition()->nRows()) {
		    // have built a valid index, write out its content
		    file.erase(file.rfind(FASTBIT_DIRSEP));
		    try {
			ind->write(file.c_str());
		    }
		    catch (...) {
			c->logWarning("createIndex",
				      "failed to write the index to %s",
				      file.c_str());
			file += FASTBIT_DIRSEP;
			file += c->name();
			file += ".idx";
			remove(file.c_str());
		    }
		}
		else {
		    LOGGER(ibis::gVerbose > 0)
			<< "ibis::index::create created an index with "
			<< ind->getNRows() << " row"
			<< (ind->getNRows() > 1 ? "s" : "") << " from "
			<< file << ", but the data partition has "
			<< c->partition()->nRows() << " row"
			<< (c->partition()->nRows() > 1 ? "s" : "");
		}
	    }
	} // if (dfname != 0)
	else if (c->type() == ibis::CATEGORY) {
	    return ind;
	}
	else if (c->type() == ibis::TEXT) {
	    const ibis::column* idcol =
		reinterpret_cast<const ibis::text*>(c)->
		IDColumnForKeywordIndex();
	    ind = new ibis::keywords(c, idcol);
	    return ind;
	}
	else { // build a new index from attribute values
	    bool dflt = false;
	    if (c->type() == ibis::CATEGORY) {
		dflt = true;
	    }
	    else if (spec == 0) {
		dflt = true;
	    }
	    else if (*spec == 0) {
		dflt = true;
	    }
	    else if (strstr(spec, "automatic") != 0 ||
		     strstr(spec, "default") != 0) {
		dflt = true;
	    }
	    else {
		dflt = (0 != isspace(*spec));
	    }

	    if (dflt) {
		switch (c->type()) {
		case ibis::ULONG:
		case ibis::LONG:
		case ibis::USHORT:
		case ibis::SHORT:
		case ibis::UINT:
		case ibis::INT: {
		    double amin = c->lowerBound();
		    double amax = c->upperBound();
		    if (amin >= amax && amin >= 0) {
			c->computeMinMax(c->partition()->currentDataDir(),
					 amin, amax);
		    }
		    if (amax - amin < 1e3 ||
			amax - amin < 0.1 * c->partition()->nRows()) {
			if (amin >= 0.0 && amin <= ceil(amax*0.01))
			    ind = new ibis::direkte(c);
			else if (amax >= amin+1e2)
			    ind = new ibis::fuzz(c);
			else
			    ind = new ibis::relic(c);
		    }
		    else {
			ind = new ibis::bin(c);
		    }
		    break;}
		case ibis::FLOAT:
		case ibis::DOUBLE: {
		    ind = new ibis::bin(c);
		    break;}
		case ibis::UBYTE:
		case ibis::BYTE:
		case ibis::CATEGORY: {
		    ind = new ibis::relic(c);
		    break;}
		case ibis::TEXT: {
		    const ibis::column* idcol =
			reinterpret_cast<const ibis::text*>(c)->
			IDColumnForKeywordIndex();
		    ind = new ibis::keywords(c, idcol);
		    break;}
		default: {
		    c->logWarning("createIndex", "no default index type for "
				  "this column");
		    break;}
		}
	    }
	    else if (ncomp > 1 || strstr(spec, "mcbin") != 0 ||
		     strstr(spec, "multicomponent") != 0) {
		INDEX_TYPE t = SAPID; // default to equality encoding
		if (strstr(spec, "equal")) {
		    t = SAPID;
		}
		else if (strstr(spec, "range")) {
		    t = FADE;
		}
		else if (strstr(spec, "interval")) {
		    t = SBIAD;
		}
		switch (t) {
		default:
		case SBIAD:
		    if (usebin)
			ind = new ibis::entre(c, static_cast<const char*>(0),
					      ncomp);
		    else
			ind = new ibis::sbiad(c, static_cast<const char*>(0),
					      ncomp);
		    break;
		case SAPID:
		    if (usebin)
			ind = new ibis::egale(c, static_cast<const char*>(0),
					      ncomp);
		    else
			ind = new ibis::sapid(c, static_cast<const char*>(0),
					      ncomp);
		    break;
		case FADE:
		    if (usebin)
			ind = new ibis::moins(c, static_cast<const char*>(0),
					      ncomp);
		    else
			ind = new ibis::fade(c, static_cast<const char*>(0),
					     ncomp);
		    break;
		}
	    }
	    else if (!usebin) { // <binning none> is specified explicitly
		INDEX_TYPE t = RELIC;
		const char* str = strstr(spec, "<encoding ");
		if (str) {
		    str += 10; // skip "<encoding "
		    if (strstr(str, "range/equality") ||
			strstr(str, "range-equality")) {
			t = BYLT;
		    }
		    else if (strstr(str, "equality/equality") ||
			     strstr(str, "equality-equality")) {
			t = ZONA;
		    }
		    else if (strstr(str, "interval/equality") ||
			     strstr(str, "interval-equality")) {
			t = FUZZ;
		    }
		    else if (strstr(str, "equal")) {
			t = SAPID;
		    }
		    else if (strstr(str, "interval")) {
			t = SBIAD;
		    }
		    else if (strstr(str, "range")) {
			t = FADE;
		    }
		    else if (strstr(str, "binary")) {
			t = SLICE;
		    }
		}
		else if (stricmp(spec, "index=simple") == 0 ||
			 stricmp(spec, "index=basic") == 0 ||
			 strstr(spec, "relic") != 0) {
		    t = RELIC;
		}
		else if (strstr(spec, "slice") != 0 ||
			 strstr(spec, "bitslice") != 0 ||
			 strstr(spec, "binary") != 0) {
		    t = SLICE;
		}
		else {
		    t = SAPID;
		}
		switch (t) {
		default:
		case SAPID:
		    if (ncomp > 1)
			ind = new ibis::sapid(c, static_cast<const char*>(0),
					      ncomp);
		    else if ((c->type() != ibis::FLOAT &&
			      c->type() != ibis::DOUBLE &&
			      c->type() != ibis::TEXT) &&
			     c->lowerBound() >= 0.0 &&
			     c->lowerBound() <= ceil(c->upperBound()*0.01) &&
			     c->upperBound() <= c->partition()->nRows())
			ind = new ibis::direkte(c);
		    else
			ind = new ibis::relic(c);
		    break;
		case RELIC:
		    ind = new ibis::relic(c);
		    break;
		case FADE:
		    ind = new ibis::fade(c, static_cast<const char*>(0),
					 ncomp);
		    break;
		case SBIAD:
		    ind = new ibis::sbiad(c, static_cast<const char*>(0),
					  ncomp);
		    break;
		case SLICE:
		    ind = new ibis::slice(c);
		    break;
		case BYLT:
		    ind = new ibis::bylt(c);
		    break;
		case ZONA:
		    ind = new ibis::zona(c);
		    break;
		case FUZZ:
		    ind = new ibis::fuzz(c);
		    break;
		}
	    }
	    else if (strstr(spec, "slice") != 0 ||
		     strstr(spec, "bitslice") != 0 ||
		     strstr(spec, "binary") != 0) { // ibis::slice
		ind = new ibis::slice(c);
	    }
	    else if (strstr(spec, "simple") != 0 ||
		     strstr(spec, "basic") != 0 ||
		     strstr(spec, "bitmap") != 0 ||
		     strstr(spec, "relic") != 0) { // ibis::relic
		if ((c->type() != ibis::FLOAT &&
		     c->type() != ibis::DOUBLE &&
		     c->type() != ibis::TEXT) &&
		    c->lowerBound() >= 0.0 &&
		    c->lowerBound() <= ceil(c->upperBound()*0.01) &&
		    c->upperBound() <= c->partition()->nRows())
		    ind = new ibis::direkte(c);
		else
		    ind = new ibis::relic(c);
	    }
	    else if (strstr(spec, "fade") != 0 ||
		     strstr(spec, "multi-range") != 0) {
		ind = new ibis::fade(c);
	    }
	    else if (strstr(spec, "sapid") != 0 ||
		     strstr(spec, "multi-equal") != 0) {
		ind = new ibis::sapid(c);
	    }
	    else if (strstr(spec, "sbiad") != 0 ||
		     strstr(spec, "multi-interval") != 0) {
		ind = new ibis::sbiad(c);
	    }
	    else if (strstr(spec, "egale") != 0) {
		ind = new ibis::egale(c);
	    }
	    else if (strstr(spec, "moins") != 0) {
		ind = new ibis::moins(c);
	    }
	    else if (strstr(spec, "entre") != 0) {
		ind = new ibis::entre(c);
	    }
	    else if (strstr(spec, "ambit") != 0 ||
		     strstr(spec, "range/range") != 0 ||
		     strstr(spec, "range-range") != 0) {
		ibis::bin tmp(c);
		ind = new ibis::ambit(tmp);
		// ind = new ibis::ambit(c);
	    }
	    else if (strstr(spec, "pale") != 0 ||
		     strstr(spec, "bin/range") != 0 ||
		     strstr(spec, "equality-range") != 0) {
		ibis::bin tmp(c);
		ind = new ibis::pale(tmp);
	    }
	    else if (strstr(spec, "pack") != 0 ||
		     strstr(spec, "range/bin") != 0 ||
		     strstr(spec, "range/equality") != 0 ||
		     strstr(spec, "range-equality") != 0) {
		ibis::bin tmp(c);
		ind = new ibis::pack(tmp);
	    }
	    else if (strstr(spec, "zone") != 0 ||
		     strstr(spec, "bin/bin") != 0 ||
		     strstr(spec, "equality/equality") != 0 ||
		     strstr(spec, "equality-equality") != 0) {
		ibis::bin tmp(c);
		ind = new ibis::zone(tmp);
	    }
	    else if (strstr(spec, "interval/equality") != 0 ||
		     strstr(spec, "interval-equality") != 0) {
		ind = new ibis::fuge(c);
	    }
	    else if (strstr(spec, "bak2") != 0) {
		ind = new ibis::bak2(c);
	    }
	    else if (strstr(spec, "bak") != 0) {
		ind = new ibis::bak(c);
	    }
	    else if (strstr(spec, "mesa") != 0 ||
		     strstr(spec, "interval") != 0 ||
		     strstr(spec, "2sided") != 0) {
		ind = new ibis::mesa(c);
	    }
	    else if (strstr(spec, "range") != 0 ||
		     strstr(spec, "cumulative") != 0) { // ibis::range
		ind = new ibis::range(c);
	    }
	    else {
		if (strstr(spec, "bin") == 0)
		    c->logMessage("createIndex", "can not process bin "
				  "spec \"%s\", use simple bins", spec);
		ind = new ibis::bin(c);
	    }

	    if (ind == 0) {
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::index::create failed to create an index for "
		    << c->name();
	    }
	    else if (ind->getNRows() == 0) {
		delete ind;
		ind = 0;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::index::create create an empty index for "
		    << c->name();
	    }
	    else if (ind->getNRows() == c->partition()->nRows()) {
		// having built a valid index, write out its content
		try {
		    ind->write(c->partition()->currentDataDir());
		}
		catch (...) {
		    c->logWarning("createIndex",
				  "failed to write the index to %s",
				  c->partition()->currentDataDir());
		    std::string idxname = c->partition()->currentDataDir();
		    idxname += FASTBIT_DIRSEP;
		    idxname += c->name();
		    idxname += ".idx";
		    remove(idxname.c_str());
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::index::create created an index with "
		    << ind->getNRows() << " row"
		    << (ind->getNRows() > 1 ? "s" : "")
		    << ", but the data partition has "
		    << c->partition()->nRows() << " row"
		    << (c->partition()->nRows() > 1 ? "s" : "");
	    }
	} // building a new index
    }
    catch (const char* s) {
	ibis::util::logMessage("Warning", "ibis::index::create(%s) received "
			       "a string exception -- %s", c->name(), s);
	delete ind;
	ind = 0;
    }
    catch (const ibis::bad_alloc& e) {
	ibis::util::logMessage("Warning", "ibis::index::create(%s) failed "
			       "to create a new index -- %s", c->name(),
			       e.what());
	delete ind;
	ind = 0;
    }
    catch (const std::exception& e) {
	ibis::util::logMessage("Warning", "ibis::index::create(%s) failed "
			       "to create a new index -- %s", c->name(),
			       e.what());
	delete ind;
	ind = 0;
    }
    catch (...) {
	ibis::util::logMessage("Warning", "ibis::index::create(%s) failed "
			       "to create a new index -- unknown error",
			       c->name());
	delete ind;
	ind = 0;
    }

    if (ind == 0) {
	if (c->type() == ibis::TEXT) {
	    LOGGER(ibis::gVerbose > 1)
		<< "index::create -- can not create an index for "
		<< c->partition()->name() << "." << c->name()
		<< ", this is of data value (string) require an "
		"additional term-document matrix file";
	}
	else {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- failed to create an index for "
		<< c->partition()->name() << "." << c->name()
		<< ", can still process queries if the raw data "
		"is available";
	}
    }
    else if (ibis::gVerbose > 1) {
	timer.stop();
	if (isRead) {
	    c->logMessage("readIndex", "the %s index was read (%s) in %g "
			  "sec(CPU), %g sec(elapsed)", ind->name(), dfname,
			  timer.CPUTime(), timer.realTime());
	}
	else {
	    c->logMessage("createIndex", "creating a (%s) index took %g "
			  "sec(CPU), %g sec(elapsed)", ind->name(),
			  timer.CPUTime(), timer.realTime());
	}
	if (ibis::gVerbose > 3) {
	    ibis::util::logger lg;
	    ind->print(lg.buffer());
	}
    }
    return ind;
} // ibis::index::create

/// Free the objectes pointed to by the pointers.
void ibis::index::clear() {
    for (uint32_t i = 0; i < bits.size(); ++ i) {
	delete bits[i];
	bits[i] = 0;
    }
    bits.clear();
    offsets.clear();
    delete [] fname;
    nrows = 0;

    // reassign the internal storage tracking variables to null
    fname = 0;
    str = 0;
    // the pointer str can only be from a file and must be managed by the
    // fileManager and can not be deleted
} // ibis::index::clear

// read the first eight bytes of the file and verify that it is the expected
// header of an index file
bool ibis::index::isIndex(const char* f, ibis::index::INDEX_TYPE t) {
    char buf[12];
    char* header = 0;
    // attempt to read the file using read(2)
    int fdes = UnixOpen(f, OPEN_READONLY);
    if (fdes >= 0) {
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdes, _O_BINARY);
#endif
	if (8 == UnixRead(fdes, static_cast<void*>(buf), 8)) {
	    header = buf;
	}
	UnixClose(fdes);
    }

    if (header) { // verify header
	const bool check =
	    (header[0] == '#' && header[1] == 'I' &&
	     header[2] == 'B' && header[3] == 'I' &&
	     header[4] == 'S' && t ==
	     static_cast<ibis::index::INDEX_TYPE>(header[5]) &&
	     header[6] == static_cast<char>(sizeof(int32_t)) &&
	     header[7] == static_cast<char>(0));
	if (!check) {
	    ibis::util::logMessage("readIndex", "index file \"%s\" contains "
				   "an incorrect header "
				   "(%c%c%c%c%c:%i.%i.%i)",
				   f, header[0], header[1], header[2],
				   header[3], header[4], (int)header[5],
				   (int)header[6], (int)header[7]);
	}
	return check;
    }
    return false;
} // ibis::index::isIndex

// generates data file name from "f"
void ibis::index::dataFileName(const char* f, std::string& iname) const {
    if (f == 0) {
	iname = col->partition()->currentDataDir();
	iname += FASTBIT_DIRSEP;
	iname += col->name();
    }
    else {
	uint32_t j = strlen(f);
	bool isFile = false;
	uint32_t i = strlen(col->name());
	if (j >= i) {
	    const char* tail = f + (j - i);
	    isFile = (strcmp(tail, col->name()) == 0);
	}
	if (isFile) {
	    iname = f;
	}
	else if (j > 4 && f[j-1] == 'x' && f[j-2] == 'd' &&
		 f[j-3] == 'i' && f[j-4] == '.') { // index file name
	    iname = f;
	    iname.erase(j-4);
	}
	else { // check the existence of the file or direcotry
	    Stat_T st0;
	    if (UnixStat(f, &st0)) { // assume to be a file
		iname = f;
	    }
	    else if ((st0.st_mode & S_IFDIR) == S_IFDIR) {
		// named directory exist
		iname = f;
		iname += FASTBIT_DIRSEP;
		iname += col->name();
	    }
	    else if (j > 4 && f[j-1] == 'x' && f[j-2] == 'd' &&
		     f[j-3] == 'i' && f[j-4] == '.') { // index file name
		iname = f;
		iname.erase(j-4);
	    }
	    else { // given name is the data file name
		iname = f;
	    }
	}
    }
} // dataFileName

// generates index file name from "f"
void ibis::index::indexFileName(const char* f, std::string& iname) const {
    if (f == 0 || *f == 0) {
	iname = col->partition()->currentDataDir();
	iname += FASTBIT_DIRSEP;
	iname += col->name();
	iname += ".idx";
    }
    else {
	Stat_T st0;
	if (UnixStat(f, &st0)) { // stat fails, use the name
	    iname = f;
	}
	else if ((st0.st_mode & S_IFDIR) == S_IFDIR) {
	    // named directory exist
	    iname = f;
	    iname += FASTBIT_DIRSEP;
	    iname += col->name();
	    iname += ".idx";
	}
	else {
	    // use given name as the index file name if it is not
	    // exactly the name as the column name
	    uint32_t j = strlen(f);
	    if (f[j-1] == 'x' && f[j-2] == 'd' && f[j-3] == 'i' &&
		f[j-4] == '.') {
		iname = f;
	    }
	    else {
		bool isDFile = false;
		uint32_t i = strlen(col->name());
		const char* tail = f + (j - i);
		if (j > i) {
		    isDFile = ((tail[-1] == FASTBIT_DIRSEP) &&
			       (strcmp(tail, col->name()) == 0));
		}
		else if (j == i) {
		    isDFile = (strcmp(tail, col->name()) == 0);
		}
		if (isDFile) {
		    iname = f;
		    iname += ".idx";
		}
		else {
		    iname = f;
		}
	    }
	}
    }
} // indexFileName

/// Generate the index file name for the composite index fromed on two
/// columns.  May use argument "dir" if it is not null.
void ibis::index::indexFileName(std::string& iname,
				const ibis::column *col1,
				const ibis::column *col2,
				const char* dir) {
    if (dir == 0 || *dir == 0) {
	iname = col1->partition()->currentDataDir();
	iname += FASTBIT_DIRSEP;
	iname += col1->name();
	iname += '-';
	iname += col2->name();
	iname += ".idx";
    }
    else {
	Stat_T st0;
	if (UnixStat(dir, &st0)) { // stat fails, use the name
	    iname = dir;
	    uint32_t j = iname.rfind(FASTBIT_DIRSEP);
	    if (j < iname.size()) {
		++ j;
		iname.erase(j);
	    }
	    else if (iname.size() > 0) {
		iname += FASTBIT_DIRSEP;
	    }
	}
	else if ((st0.st_mode & S_IFDIR) == S_IFDIR) {
	    // named directory exist
	    iname = dir;
	    if (iname[iname.size()-1] != FASTBIT_DIRSEP)
		iname += FASTBIT_DIRSEP;
	}
	else {
	    iname = dir;
	    uint32_t j = iname.rfind(FASTBIT_DIRSEP);
	    if (j < iname.size()) {
		++ j;
		iname.erase(j);
	    }
	    else if (iname.size() > 0) {
		iname += FASTBIT_DIRSEP;
	    }
	}
	iname += col1->name();
	iname += '-';
	iname += col2->name();
	iname += ".idx";
    }
} // indexFileName

// actually go through values and determine the min/max values
void ibis::index::computeMinMax(const char* f, double& min,
				double& max) const {
    std::string fnm;
    dataFileName(f, fnm); // generate the correct data file name
    switch (col->type()) {
    case ibis::UINT: {
	array_t<uint32_t> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	uint32_t imin = val[0];
	uint32_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	min = imin;
	max = imax;
	break;}
    case ibis::INT: {
	array_t<int32_t> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	int32_t imin = val[0];
	int32_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	min = imin;
	max = imax;
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	uint16_t imin = val[0];
	uint16_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	min = imin;
	max = imax;
	break;}
    case ibis::SHORT: {
	array_t<int16_t> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	int16_t imin = val[0];
	int16_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	min = imin;
	max = imax;
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	unsigned char imin = val[0];
	unsigned char imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	min = imin;
	max = imax;
	break;}
    case ibis::BYTE: {
	array_t<signed char> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	signed char imin = val[0];
	signed char imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	min = imin;
	max = imax;
	break;}
    case ibis::FLOAT: {
	array_t<float> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	min = val[0];
	max = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (min > val[i])
		min = val[i];
	    else if (max < val[i])
		max = val[i];
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> val;
	int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr != 0) {
	    col->logWarning("computeMinMax", "unable to retrieve file %s",
			    fnm.c_str());
	    return;
	}

	min = val[0];
	max = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (min > val[i])
		min = val[i];
	    else if (max < val[i])
		max = val[i];
	}
	break;}
    default:
	col->logMessage("computeMinMax", "not able to compute min/max or "
			"no need for min/max");
	min = 0;
	max = 0;
    } // switch(m_type)
} // ibis::index::computeMinMax

/// Map the locations of the values of one column.  Given a file containing
/// the values of a column, this function maps the position of each
/// individual values and stores the result in a set of bitmaps.
///@note
/// IMPROTANT ASSUMPTION.
/// A value of any supported type is supposed to be able to fit in a
/// double with no rounding, no approximation and no overflow.
void ibis::index::mapValues(const char* f, VMap& bmap) const {
    horometer timer;
    if (ibis::gVerbose > 4)
	timer.start();
    uint32_t i, j, k, nev = col->partition()->nRows();
    std::string fnm; // name of the data file

    bmap.clear();
    dataFileName(f, fnm);
    k = ibis::util::getFileSize(fnm.c_str());
    if (k > 0) {
	if (ibis::gVerbose > 1)
	    col->logMessage("mapValues", "attempting to map the positions "
			    "of every value in \"%s\"", fnm.c_str());
    }
    else {
	if (nev > 0) {
	    if (col->type() == ibis::CATEGORY) {
		if (col->partition()->getState() ==
		    ibis::part::PRETRANSITION_STATE) {
		    ibis::bitvector *tmp = new ibis::bitvector;
		    tmp->set(1, nev);
		    bmap[1] = tmp;
		}
	    }
	    else if (ibis::gVerbose > 4) {
		col->logMessage("mapValues", "data file \"%s\"does not "
				"exist or is empty", fnm.c_str());
	    }
	}
	return;
    }

    int ierr = 0;
    VMap::iterator it;
    ibis::bitvector mask;
    col->getNullMask(mask);
#if defined(MAPVALUES_EXCLUDE_INACTIVE)
    if (col->partition() != 0) {
	ibis::bitvector tmp(col->partition()->getNullMask());
	tmp.adjustSize(col->partition()->nRows(), col->partition()->nRows());
	mask.adjustSize(0, tmp.size());
	mask &= tmp;
    }
#endif
    // need to do different things for different columns
    switch (col->type()) {
    case ibis::TEXT:
    case ibis::UINT:
    case ibis::CATEGORY: {// if data file exists, must be unsigned int
	array_t<uint32_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	uint32_t nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices that are definitely within nev
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else { // a list of indices that may be larger than nev
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::BYTE: {// (1-byte) integer values
	array_t<signed char> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::UBYTE: {// (1-byte) integer values
	array_t<unsigned char> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::SHORT: {// (2-byte) integer values
	array_t<int16_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::USHORT: {// (2-byte) integer values
	array_t<uint16_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::ULONG: {// if data file exists, must be unsigned int64_t
	array_t<uint64_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	uint32_t nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices that are definitely within nev
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else { // a list of indices that may be larger than nev
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    case ibis::LONG: {// signed int64_t
	array_t<int64_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	nev = val.size();
	if (ierr < 0 || val.size() == 0)
	    break;

	if (nev > mask.size())
	    mask.adjustSize(nev, nev);
	ibis::bitvector::indexSet iset = mask.firstIndexSet();
	unsigned nind = iset.nIndices();
	const ibis::bitvector::word_t *iix = iset.indices();
	while (nind) { // build the list of bitmaps
	    if (iset.isRange()) { // a range
		k = (iix[1] < nev ? iix[1] : nev);
		for (i = *iix; i < k; ++i) {
		    it = bmap.find(val[i]);
		    if (it != bmap.end()) {
			(*it).second->setBit(i, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(i, 1);
			bmap[val[i]] = tmp;
		    }
		}
	    }
	    else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		// a list of indices
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    it = bmap.find(val[k]);
		    if (it != bmap.end()) {
			(*it).second->setBit(k, 1);
		    }
		    else {
			ibis::bitvector* tmp = new ibis::bitvector();
			tmp->setBit(k, 1);
			bmap[val[k]] = tmp;
		    }
		}
	    }
	    else {
		for (i = 0; i < nind; ++i) {
		    k = iix[i];
		    if (k < nev) {
			it = bmap.find(val[k]);
			if (it != bmap.end()) {
			    (*it).second->setBit(k, 1);
			}
			else {
			    ibis::bitvector* tmp = new ibis::bitvector();
			    tmp->setBit(k, 1);
			    bmap[val[k]] = tmp;
			}
		    }
		}
	    }
	    ++iset;
	    nind = iset.nIndices();
	    if (*iix >= nev) nind = 0;
	} // while (nind)
	break;}

    default:
	col->logWarning("index::mapValues", "unable to maps values of "
			"this type of column");
	return;
    }

    if (ierr < 0) {
	col->logWarning("index::mapValues", "unable to read %s, ierr=%d",
			fnm.c_str(), ierr);
	return;
    }
    else if (nev == 0) {
	col->logWarning("index::mapValues", "data file %s is empty",
			fnm.c_str());
	return;
    }

    // make sure all bit vectors are the same size
    if (mask.size() > nev)
	nev = mask.size();
    j = nev - 1;
    for (it = bmap.begin(); it != bmap.end(); ++it) {
	if ((*it).second->size() < nev) {
	    (*it).second->setBit(j, 0);
	}
	else if ((*it).second->size() > nev) {
	    col->logWarning("index::mapValues", "bitvector for value %.9g "
			    "contains %lu bits while %lu are expected -- "
			    "removing the extra bits",
			    (*it).first,
			    static_cast<long unsigned>((*it).second->size()),
			    static_cast<long unsigned>(nev));
	    (*it).second->adjustSize(nev, nev);
	}
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	col->logMessage("index::mapValues", "mapping %lu values from file %s "
			"generated %lu bitvectors of %lu-bit each "
			"in %g sec(elapsed)",
			static_cast<long unsigned>(nev), fnm.c_str(),
			static_cast<long unsigned>(bmap.size()),
			static_cast<long unsigned>(nev), timer.realTime());
	if (ibis::gVerbose > 30 || ((1U<<ibis::gVerbose)>bmap.size())) {
	    ibis::util::logger lg;
	    lg.buffer() << "value, count (extracted from the bitvector)\n";
	    for (it = bmap.begin(); it != bmap.end(); ++it)
		lg.buffer() << (*it).first << ",\t" << (*it).second->cnt()
			  << "\n";
	}
    }
    else if (ibis::gVerbose > 2) {
	col->logMessage("index::mapValues", "mapping file %s generated %lu "
			"bitvectors of %lu-bit each", fnm.c_str(),
			static_cast<long unsigned>(bmap.size()),
			static_cast<long unsigned>(nev));
    }
} // ibis::index::mapValues

template <typename E>
void ibis::index::mapValues(const array_t<E>& val, VMap& bmap) {
    bmap.clear();
    if (val.size() == 0) {
	if (ibis::gVerbose > 2)
	    ibis::util::logMessage("index::mapValues",
				   "the input array is empty");
	return;
    }

    uint32_t nev = val.size();
    VMap::iterator it;
    ibis::horometer timer;
    timer.start();
    for (uint32_t i = 0; i < nev; ++i) {
	it = bmap.find(val[i]);
	if (it != bmap.end()) {
	    (*it).second->setBit(i, 1);
	}
	else {
	    ibis::bitvector* tmp = new ibis::bitvector();
	    tmp->setBit(i, 1);
	    bmap[val[i]] = tmp;
	}
    }
    const uint32_t j = nev - 1;
    for (it = bmap.begin(); it != bmap.end(); ++it) {
	if ((*it).second->size() < nev) {
	    (*it).second->setBit(j, 0);
	}
	else if ((*it).second->size() > nev) {
	    ibis::util::logMessage
		("index::mapValues", "bitvector for value %.9g "
		 "contains %lu bits while %lu are expected -- "
		 "removing the extra bits",
		 (*it).first, static_cast<long unsigned>((*it).second->size()),
		 static_cast<long unsigned>(nev));
	    (*it).second->adjustSize(nev, nev);
	}
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage
	    ("index::mapValues", "mapping an array[%lu] generated "
	     "%lu bitvectors of %lu-bit each in %g sec(elapsed)",
	     static_cast<long unsigned>(nev),
	     static_cast<long unsigned>(bmap.size()),
	     static_cast<long unsigned>(nev), timer.realTime());
	if (ibis::gVerbose > 30 || ((1U<<ibis::gVerbose)>bmap.size())) {
	    ibis::util::logger lg;
	    lg.buffer() << "value, count (extracted from the bitvector)\n";
	    for (it = bmap.begin(); it != bmap.end(); ++it)
		lg.buffer() << (*it).first << ",\t" << (*it).second->cnt()
			  << "\n";
	}
    }
    else if (ibis::gVerbose > 2) {
	ibis::util::logMessage
	    ("index::mapValues", "mapping an array[%lu] found "
	     "%lu unique values", static_cast<long unsigned>(nev),
	     static_cast<long unsigned>(bmap.size()));
    }
} // ibis::index::mapValues

/// A brute-force approach to get an accurate distribution.
long ibis::index::getDistribution
(std::vector<double>& bds, std::vector<uint32_t>& cts) const {
    bds.clear();
    cts.clear();

    histogram hist;
    mapValues(0, hist);
    bds.reserve(hist.size());
    cts.reserve(hist.size());
    histogram::const_iterator it = hist.begin();
    cts.push_back((*it).second);
    for (++ it; it != hist.end(); ++ it) {
	bds.push_back((*it).first);
	cts.push_back((*it).second);
    }
    return cts.size();
} // ibis::index::getDistribution

/// A brute-force approach to get an accurate cumulative distribution.
long ibis::index::getCumulativeDistribution
(std::vector<double>& bds, std::vector<uint32_t>& cts) const {
    bds.clear();
    cts.clear();

    histogram hist;
    mapValues(0, hist);
    bds.reserve(hist.size());
    cts.reserve(hist.size());
    histogram::const_iterator it = hist.begin();
    cts.push_back((*it).second);
    uint32_t sum = (*it).second;
    for (++ it; it != hist.end(); ++ it) {
	sum += (*it).second;
	bds.push_back((*it).first);
	cts.push_back(sum);
    }
    if (bds.size() > 0) {
	double tmp = bds.back();
	bds.push_back(ibis::util::compactValue(tmp, (tmp>0?tmp+tmp:0.0)));
    }

    return cts.size();
} // ibis::index::getCumulativeDistribution

/// Compute a histogram of a column.  Given a property file containing the
/// values of a column, this function counts the occurances of each
/// distinct values.  Argument @c count is the number of samples to be
/// used for building the histogram.  If it is zero or greater than half of
/// the number of values in the data files, all values are used, otherwise,
/// approximately @c count values will be sampled with nearly uniform
/// distances from each other.
///
/// @note IMPROTANT ASSUMPTION.
/// A value of any supported type is supposed to be able to fit in a
/// double with no rounding, no approximation and no overflow.
void ibis::index::mapValues(const char* f, histogram& hist,
			    uint32_t count) const {
    uint32_t i, k, nev = col->partition()->nRows();
// TODO: implement a different algorith, like sort the values first, to
// make memory usage more predictable.  The numerous dynamically allocated
// elements used by histogram really could slow down this function!

    horometer timer;
    std::string fnm; // name of the data file
    dataFileName(f, fnm);
    if (ibis::gVerbose > 4) {
	timer.start();
	col->logMessage("mapValues", "attempting to generate a histogram "
			"for data in \"%s\"", fnm.c_str());
    }

    histogram::iterator it;
    ibis::bitvector mask;
    col->getNullMask(mask);
    if (count > 0 && (mask.size() > 10000000 || count <
		      (mask.size() >> (col->elementSize() <= 4 ? 11 : 10)))) {
	ibis::bitvector pgm; // page mask
	const unsigned ntot = mask.size();
	const unsigned multip = ((ntot >> 12)>count ? (ntot >> 10)/count : 4);
	const unsigned stride = 1024 * multip;
	if (ibis::gVerbose > 2)
	    col->logMessage("mapValues", "will sample 1024 values out of "
			    "every %u (total %lu)",
			    static_cast<unsigned>(stride),
			    static_cast<long unsigned>(ntot));
	for (i = 0; i < ntot; i += stride) {
	    unsigned skip = static_cast<unsigned>(ibis::util::rand() * multip);
	    if (skip > 0)
		pgm.appendFill(0, 1024*skip);
	    pgm.appendFill(1, 1024);
	    if (skip+2 < multip)
		pgm.appendFill(0, stride-1024*(skip+1));
	}
	pgm.adjustSize(0, mask.size());
	mask &= pgm;
    }

    // need to do different things for different columns
    switch (col->type()) {
    case ibis::TEXT:
    case ibis::UINT: {// unsigned int
	array_t<uint32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(static_cast<double>(val[i]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(static_cast<double>(val[k]));
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(static_cast<double>(val[i]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(static_cast<double>(val[k]));
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(static_cast<double>(val[i]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(static_cast<double>(val[k]));
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::BYTE: {// (1-byte) integer values
	array_t<signed char> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::UBYTE: {// (1-byte) integer values
	array_t<unsigned char> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::SHORT: {// (2-byte) integer values
	array_t<int16_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::USHORT: {// (2-byte) integer values
	array_t<uint16_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::LONG: {// (8-byte) integer values
	array_t<int64_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
#if defined(DEBUG) && DEBUG+0>1
			    LOGGER(ibis::gVerbose > 5)
				<< "DEBUG -- index::mapValues adding value "
				<< val[i] << " to the histogram of size "
				<< hist.size();
#endif
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::ULONG: {// unsigned (8-byte) integer values
	array_t<uint64_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    nev = val.size();
	    if (nev > mask.size())
		mask.adjustSize(nev, nev);
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) { // count the occurances
		if (iset.isRange()) { // a range
		    k = (iix[1] < nev ? iix[1] : nev);
		    for (i = *iix; i < k; ++i) {
			it = hist.find(val[i]);
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[i]] = 1;
			}
		    }
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nev) {
		    // a list of indices within the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			it = hist.find(static_cast<double>(val[k]));
			if (it != hist.end()) {
			    ++ ((*it).second);
			}
			else {
			    hist[val[k]] = 1;
			}
		    }
		}
		else { // some indices may be out of the valid range
		    for (i = 0; i < nind; ++i) {
			k = iix[i];
			if (k < nev) {
			    it = hist.find(val[k]);
			    if (it != hist.end()) {
				++ ((*it).second);
			    }
			    else {
				hist[val[k]] = 1;
			    }
			}
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nev) nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("index::mapValues", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::CATEGORY: // no need for a separate index
	col->logWarning("index::mapValues", "no value to compute a "
			"histogram -- use the basic bitmap index for the "
			"same information");
	hist.clear();
	break;
    default:
	col->logWarning("index::mapValues", "unable to create a histogram "
			"for this type of column");
	break;
    }

    if (ibis::gVerbose > 4) {
	timer.stop();
	col->logMessage("index::mapValues", "generated histogram (%lu "
			"distinct value%s) in %g sec(elapsed)",
			static_cast<long unsigned>(hist.size()),
			(hist.size()>1?"s":""), timer.realTime());
	if (ibis::gVerbose > 30 || ((1U<<ibis::gVerbose)>hist.size())) {
	    ibis::util::logger lg;
	    lg.buffer() << "value, count\n";
	    for (it = hist.begin(); it != hist.end(); ++it)
		lg.buffer() << (*it).first << ",\t" << (*it).second << "\n";
	}
    }
    else if (ibis::gVerbose > 2) {
	col->logMessage("index::mapValues", "generated histogram (%lu "
			"distinct value%s)",
			static_cast<long unsigned>(hist.size()),
			(hist.size()>1?"s":""));
    }
} // ibis::index::mapValues

template <typename E>
void ibis::index::mapValues(const array_t<E>& val, histogram& hist,
			    uint32_t count) {
    if (val.empty()) return;
    horometer timer;
    const uint32_t nev = val.size();
    uint32_t stride = 1;
    histogram::iterator it;

    if (count > 0 && count+count <= nev)
	stride = static_cast<uint32_t>(0.5 + static_cast<double>(nev)
				       / static_cast<double>(count));
    if (ibis::gVerbose > 4) {
	timer.start();
	ibis::util::logMessage("index::mapValues", "starting to count the "
			       "frequencies of %s[%lu] with stride %lu",
			       typeid(E).name(),
			       static_cast<long unsigned>(nev),
			       static_cast<long unsigned>(stride));
    }
    if (stride <= 2) {
	for (uint32_t i = 0; i < nev; ++i) {
	    it = hist.find(static_cast<double>(val[i]));
	    if (it != hist.end()) {
		++ ((*it).second);
	    }
	    else {
		hist[val[i]] = 1;
	    }
	}
    }
    else { // initial stride > 2
	uint32_t cnt = 0;
	for (uint32_t i = 0; i < nev; i += stride) {
	    it = hist.find(static_cast<double>(val[i]));
	    if (it != hist.end()) {
		++ ((*it).second);
	    }
	    else {
		hist[val[i]] = 1;
	    }
	    ++ cnt;
	    if (cnt < count) // adjust stride as needed
		stride = (nev-i > count-cnt ? (nev-i)/(count-cnt) : 1);
	    else
		break;
	}
    }

    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage
	    ("index::mapValues", "generated histogram (%lu "
	     "distinct value%s) in %g sec(elapsed)",
	     static_cast<long unsigned>(hist.size()), (hist.size()>1?"s":""),
	     timer.realTime());
	if (ibis::gVerbose > 30 || ((1U<<ibis::gVerbose)>hist.size())) {
	    ibis::util::logger lg;
	    lg.buffer() << "value, count\n";
	    for (it = hist.begin(); it != hist.end(); ++it)
		lg.buffer() << (*it).first << ",\t" << (*it).second << "\n";
	}
    }
    else if (ibis::gVerbose > 2) {
	ibis::util::logMessage("index::mapValues", "generated histogram (%lu "
			       "distinct value%s)",
			       static_cast<long unsigned>(hist.size()),
			       (hist.size()>1?"s":""));
    }
} // ibis::index::mapValues

template <typename E>
void ibis::index::mapValues(const array_t<E>& val, array_t<E>& bounds,
			    std::vector<uint32_t>& cnts) {
    if (val.size() == 0)
	return;
    bool existing = ! bounds.empty();
    for (uint32_t i = 1; i < bounds.size() && existing; ++ i)
	existing = (bounds[i] > bounds[i-1]);
    if (! existing) { // need to generate boundaries
	E amin = val.front();
	E amax = val.front();
	for (uint32_t i = 1; i < val.size(); ++ i) {
	    if (amin > val[i])
		amin = val[i];
	    else if (amax < val[i])
		amax = val[i];
	}
	E diff = (amax - amin) / 1024;
	if (diff > 0) {
	    uint32_t cnt = static_cast<uint32_t>((amax - amin) / diff);
	    bounds.reserve(cnt);
	    for (uint32_t i = 1; i <= cnt; ++ i)
		bounds.push_back(amin + i * diff);
	}
	else {
	    uint32_t cnt = static_cast<uint32_t>(amax - amin);
	    bounds.reserve(cnt);
	    for (uint32_t i = 1; i <= cnt; ++ i)
		bounds.push_back(amin + i);
	}
    }

    const uint32_t nbounds = bounds.size();
    if (cnts.size() != nbounds+1) {
	cnts.resize(nbounds+1);
	for (uint32_t i = 0; i <= nbounds; ++ i)
	    cnts[i] = 0;
    }

    for (uint32_t i = 0; i < val.size(); ++ i) {
	uint32_t j1 = bounds.find(val[i]);
	if (j1 < nbounds)
	    j1 += (val[i] == bounds[j1]);
	else
	    j1 = nbounds;
	++ cnts[j1];
    }
} // ibis::index::mapValues

/// Compute a two-dimensional histogram.  Given two arrays of the same
/// size, count the number of appearance of each combinations defined by @c
/// bnd1 and @c bnd2.  If the arrays @c bnd1 or @c bnd2 contain values in
/// ascending order, their values are directly used in this function.  The
/// empty one will be replaced by a linear division of actual range into
/// 256 bins.  The array @c counts stores the 2-D bins in raster-scan order
/// with the second variable, @c val2, as the faster varying variables.
/// More specifically, the bins for variable 1 are defined as:
/// \verbatim
/// (..., bnd1[0]) [bnd1[1], bin1[2]) [bnd1[2], bin1[3) ... [bnd1.back(), ...)
/// \endverbatim
/// Note that '[' denote the left boundary is inclusive and ')' denote the
/// right boundary is exclusive.
/// Similarly, the bins for variable 2 are
/// \verbatim
/// (..., bnd2[0]) [bnd2[1], bin2[2]) [bnd2[2], bin2[3) ... [bnd2.back(), ...)
/// \endverbatim
/// The @c counts are for the following bins
/// \verbatim
/// (..., bin1[0]) (.... bin2[0])
/// (..., bin1[0]) [bin2[0], bin2[1])
/// (..., bin1[0]) [bin2[1], bin2[2])
/// ...
/// (..., bin1[0]) [bin2.back(), ...)
/// [bin1[0], bin1[1]) (..., bin2[0])
/// [bin1[0], bin1[1]) [bin2[0], bin2[1])
/// [bin1[0], bin1[1]) [bin2[1], bin2[2])
/// ...
/// [bin1[0], bin1[1]) [bin2.back(), ...)
/// ...
/// \endverbatim
template <typename E1, typename E2>
void ibis::index::mapValues(const array_t<E1>& val1, const array_t<E2>& val2,
			    array_t<E1>& bnd1, array_t<E2>& bnd2,
			    std::vector<uint32_t>& cnts) {
    if (val1.size() == 0 || val2.size() == 0 || val1.size() != val2.size())
	return;
    bool sorted = ! bnd1.empty();
    for (uint32_t i = 1; i < bnd1.size() && sorted; ++ i)
	sorted = (bnd1[i] > bnd1[i-1]);
    if (bnd1.size() == 0 || ! sorted) { // need to generate boundaries
	E1 amin = val1.front();
	E1 amax = val1.front();
	for (uint32_t i = 1; i < val1.size(); ++ i) {
	    if (amin > val1[i])
		amin = val1[i];
	    else if (amax < val1[i])
		amax = val1[i];
	}
	E1 diff = (amax - amin) / 255;
	if (diff > 0) {
	    uint32_t cnt = static_cast<uint32_t>((amax - amin) / diff);
	    bnd1.reserve(cnt);
	    for (uint32_t i = 1; i <= cnt; ++ i)
		bnd1.push_back(amin + i * diff);
	}
	else {
	    uint32_t cnt = static_cast<uint32_t>(amax - amin);
	    bnd1.reserve(cnt);
	    for (uint32_t i = 1; i <= cnt; ++ i)
		bnd1.push_back(amin + i);
	}
    }
    sorted = ! bnd2.empty();
    for (uint32_t i = 1; i < bnd2.size() && sorted; ++ i)
	sorted = (bnd2[i] > bnd2[i-1]);
    if (bnd2.size() == 0 || ! sorted) { // need to generate boundaries
	E2 amin = val2.front();
	E2 amax = val2.front();
	for (uint32_t i = 1; i < val2.size(); ++ i) {
	    if (amin > val2[i])
		amin = val2[i];
	    else if (amax < val2[i])
		amax = val2[i];
	}
	E2 diff = (amax - amin) / 255;
	if (diff > 0) {
	    uint32_t cnt = static_cast<uint32_t>((amax - amin) / diff);
	    bnd2.reserve(cnt);
	    for (uint32_t i = 1; i <= cnt; ++ i)
		bnd2.push_back(amin + i * diff);
	}
	else {
	    uint32_t cnt = static_cast<uint32_t>(amax - amin);
	    bnd2.reserve(cnt);
	    for (uint32_t i = 1; i <= cnt; ++ i)
		bnd2.push_back(amin + i);
	}
    }

    const uint32_t nbnd1 = bnd1.size();
    const uint32_t nbnd2 = bnd2.size();
    const uint32_t nb2p1 = bnd2.size() + 1;
    if (cnts.size() != (nbnd1+1) * nb2p1) {
	cnts.resize(nb2p1 * (nbnd1+1));
	for (uint32_t i = 0; i < nb2p1 * (nbnd1+1); ++ i)
	    cnts[i] = 0;
    }

    for (uint32_t i = 0; i < val1.size(); ++ i) {
	uint32_t j1 = bnd1.find(val1[i]);
	uint32_t j2 = bnd2.find(val2[i]);
	if (j1 < nbnd1)
	    j1 += (val1[i] == bnd1[j1]);
	else
	    j1 = nbnd1;
	if (j2 < nbnd2)
	    j2 += (val2[i] == bnd2[j2]);
	else
	    j2 = nbnd2;
	++ cnts[j1*nb2p1 + j2];
    }
} // ibis::index::mapValues

/// @note The array @c bdry stores the dividers.  The first group is [0,
/// bdry[0]), the second group is [bdry[0], bdry[1]), and so on.   Ths
/// function uses the size of array @c bdry to determine the number of
/// groups to use.
void ibis::index::divideCounts(array_t<uint32_t>& bdry,
			       const array_t<uint32_t>& cnt) {
    if (bdry.empty())
	return;

    const uint32_t nb = bdry.size();
    const uint32_t ncnt = cnt.size();
    uint32_t i, j, avg=0, top=0;
    if (nb*3/2 >= ncnt) {
	bdry.resize(ncnt);
	for (i=0; i<ncnt; ++i)
	    bdry[i] = i + 1;
	return;
    }

    array_t<uint32_t> weight(nb); // a temperory array
    for (i=0; i<ncnt; ++i) {
	avg += cnt[i];
	if (top < cnt[i])
	    top = cnt[i];
    }
    avg = (avg + (nb>>1)) / nb; // round to the nearest integer
    if (top < avg) { // no isolated values with high counts
	top = cnt[0];
	i = 1;
	j = 0;
	while (i < ncnt && j < nb) {
	    if (top + cnt[i] < avg) {
		top += cnt[i];
	    }
	    else if (top + cnt[i] == avg) {
		weight[j] = avg;
		bdry[j] = i + 1;
		++ j;
		++ i;
		if (i < ncnt)
		    top = cnt[i];
		else
		    top = 0;
	    }
	    else if (j > 0 && weight[j-1] > avg) {
		// previous bin is somewhat heavy, prefer a lighter bin
		if (top > 0.9*avg) {
		    weight[j] = top;
		    bdry[j] = i;
		    ++ j;
		    top = cnt[i];
		}
		else if (top + cnt[i] < 1.2*avg) {
		    weight[j] = top + cnt[i];
		    bdry[j] = i + 1;
		    ++ j;
		    ++ i;
		    if (i < ncnt)
			top = cnt[i];
		    else
			top = 0;
		}
		else if (top > 0.7*avg) {
		    weight[j] = top;
		    bdry[j] = i;
		    ++ j;
		    top = cnt[i];
		}
		else if (top + cnt[i] < 1.4*avg) {
		    weight[j] = top + cnt[i];
		    bdry[j] = i + 1;
		    ++ j;
		    ++ i;
		    if (i < ncnt)
			top = cnt[i];
		    else
			top = 0;
		}
		else {
		    weight[j] = top;
		    bdry[j] = i;
		    ++ j;
		    top = cnt[i];
		}
	    }
	    // the next part attempts to put the current group into a bin
	    // that is slightly slighter
	    else if (top + cnt[i] < 1.1*avg) {
		weight[j] = top + cnt[i];
		bdry[j] = i + 1;
		++ j;
		++ i;
		if (i < ncnt)
		    top = cnt[i];
		else
		    top = 0;
	    }
	    else if (top > 0.8*avg) {
		weight[j] = top;
		bdry[j] = i;
		++ j;
		top = cnt[i];
	    }
	    else if (top + cnt[i] < 1.3*avg) {
		weight[j] = top + cnt[i];
		bdry[j] = i + 1;
		++ j;
		++ i;
		if (i < ncnt)
		    top = cnt[i];
		else
		    top = 0;
	    }
	    else if (top > 0.6*avg) {
		weight[j] = top;
		bdry[j] = i;
		++ j;
		top = cnt[i];
	    }
	    else {
		weight[j] = top + cnt[i];
		bdry[j] = i + 1;
		++ j;
		++ i;
		if (i < ncnt)
		    top = cnt[i];
		else
		    top = 0;
	    }
	    ++i;
	} // while (i < ncnt && j < nb)
	if (top > 0) { // deal with events that have not been put in to a bin
	    if (j < nb) { // the last bin
		weight[j] = top;
		bdry[j] = ncnt;
		++ j;
	    }
	    else { // count the remaining events
		while (i < ncnt) {
		    top += cnt[i];
		    ++i;
		}
		if (weight[j-1] + top < (avg<<1)) { // merge with the last bin
		    weight[j-1] += top;
		    bdry[j-1] = ncnt;
		}
		else { // make a new bin
		    weight.push_back(top);
		    bdry.push_back(ncnt);
		    j = bdry.size();
		}
	    }
	}
	if (j < nb) { // have put too many events into first j bins
	    bool dosplit = false;
	    do {
		// attempt to find the last heaviest bin
		for (i = 1, top = 0; i < j; ++i) {
		    if (weight[i] >= weight[top])
			top = i;
		}
		// attempt to split bin i and later
		dosplit = false;
		for (i = top; i < j; ++i) {
		    if (i > 0)
			dosplit = (bdry[i]>bdry[i-1]+1);
		    else
			dosplit = (bdry[0]>1);
		    if (dosplit) { // move the last value to the next bin
			-- bdry[i];
			weight[i] -= cnt[bdry[i]];
			if (i+1 < j) {
			    weight[i+1] += cnt[bdry[i]];
			}
			else { // make a new bin
			    weight[i+1] = cnt[bdry[i]];
			    bdry[i+1] = ncnt;
			}
		    }
		}
		j += (dosplit == true);
	    } while (j < nb && dosplit);
	    if (j < nb) {
		bdry.resize(j);
		weight.resize(j);
	    }
	}

	// attempt to move the bin boundaries around to get more uniform bins
	bool doadjust = (bdry.size()>2);
	while (doadjust) {
	    if (ibis::gVerbose > 12) {
		array_t<uint32_t>::const_iterator it;
		ibis::util::logger lg;
		lg.buffer() << "divideCounts(): smoothing --\n bounds("
			    << bdry.size() << ") = [";
		for (it = bdry.begin(); it != bdry.end(); ++it)
		    lg.buffer() << " " << *it;
		lg.buffer() << "]\nweights(" << bdry.size() << ") = [";
		for (it = weight.begin(); it != weight.end(); ++it)
		    lg.buffer() << " " << *it;
		lg.buffer() << "]\n";
	    }

	    // locate the largest difference between two borders
	    int diff = weight[1] - weight[0];
	    j = 1;
	    for (i = 2; i < bdry.size(); ++i) {
		int tmp = weight[i] - weight[i-1];
		if ((diff>=0?diff:-diff) < (tmp>=0?tmp:-tmp)) {
		    diff = tmp;
		    j = i;
		}
	    }
	    doadjust = false;
	    // can the border be moved to reduce the difference
	    if (diff > 0) { // weight[j] > weight[j-1]
		if (weight[j-1]+cnt[bdry[j-1]] < weight[j]) {
		    diff >>= 1;
		    doadjust = true;
		    if (cnt[bdry[j-1]] > static_cast<uint32_t>(diff)) {
			// move only one value
			weight[j-1] += cnt[bdry[j-1]];
			weight[j] -= cnt[bdry[j-1]];
			++ bdry[j-1];
		    }
		    else { // may move more than one value
			for (i=bdry[j-1]+1, top=cnt[bdry[j-1]];
			     top <= static_cast<uint32_t>(diff); ++i)
			    top += cnt[i];
			--i;
			top -= cnt[i];
			weight[j-1] += top;
			weight[j] -= top;
			bdry[j-1] = i;
		    }
		}
		else if (j>1 && weight[j-1]+cnt[bdry[j-1]]-cnt[bdry[j-2]] <
			 weight[j]) {
		    doadjust = true;
		    for (i = j-1; doadjust && i>1; --i) {
			if (weight[i-1]+cnt[bdry[i-1]] < weight[j]) {
			    break;
			}
			else {
			    doadjust = ((weight[i-1]+cnt[bdry[i-1]]-
					 cnt[bdry[i-2]]) < weight[j]);
			}
		    }
		    if (i == 1 && doadjust)
			doadjust = (weight[0]+cnt[bdry[0]] < weight[j]);
		    if (doadjust) { // actually change the borders
			while (i <= j) {
			    weight[i-1] += cnt[bdry[i-1]];
			    weight[i] -= cnt[bdry[i-1]];
			    ++ bdry[i-1];
			    ++ i;
			}
		    }
		}
	    }
	    else if (diff < 0) { // weight[j] < weight[j-1]
		if (weight[j-1] > weight[j]+cnt[bdry[j-1]-1]) {
		    doadjust = true;
		    diff = -diff / 2;
		    if (cnt[bdry[j-1]-1] > static_cast<uint32_t>(diff)) {
			// move only one value
			-- bdry[j-1];
			weight[j] += cnt[bdry[j-1]];
			weight[j-1] -= cnt[bdry[j-1]];
		    }
		    else { // may move more than one value
			for (i=bdry[j-1]-2, top=cnt[bdry[j-1]-1];
			     top+cnt[i] <= static_cast<uint32_t>(diff); --i)
			    top += cnt[i];
			++i;
			bdry[j-1] = i;
			weight[j] += top;
			weight[j-1] -= top;
		    }
		}
		else if (weight[j-1]-cnt[bdry[j-1]-1] >
			 weight[j]-cnt[bdry[j]-1]) {
		    doadjust = (j+1 < weight.size());
		    for (i = j+1; doadjust && i < weight.size(); ++i) {
			if (weight[j-1] > weight[i]+cnt[bdry[i-1]-1]) {
			    break;
			}
			else {
			    doadjust = ((i+1 < weight.size()) &&
					(weight[i-1]-cnt[bdry[i-1]-1] >
					 weight[i]-cnt[bdry[i]-1]));
			}
		    }
		    if (doadjust) { // actually change the borders
			while (i >= j) {
			    -- bdry[i-1];
			    weight[i] += cnt[bdry[i-1]];
			    weight[i-1] -= cnt[bdry[i-1]];
			    -- i;
			}
		    }
		}
	    }
	} // while (doadjust)
    }
    else { // got some values with very large counts
	// first locate the values with heavy counts
	for (i = 0, j = 0; i < ncnt && j < nb; ++i) {
	    if (cnt[i] >= avg) {
		weight[j] = i; // mark the position of the heavy counts
		++ j;
		if (ibis::gVerbose > 4)
		    ibis::util::logMessage
			("index::divideCounts", "treating bin %lu "
			 "as heavy (weight = %lu)",
			 static_cast<long unsigned>(i),
			 static_cast<long unsigned>(cnt[i]));
	    }
	}
	if (i < ncnt || j >= nb) {
	    // special case -- all values have equal counts
	    avg = ncnt / nb;
	    j = ncnt % nb;
	    for (i = 0, top = 0; i < j; ++i) {
		top += avg+1;
		bdry[i] = top;
	    }
	    for (; i < nb; ++i) {
		top += avg;
		bdry[i] = top;
	    }
	    return;
	}

	// count the number of events in-between the heavy ones and decide
	// how many bins they will get
	// cnt2 stores the number of events
	// nb2  stores the number of bins to use
	weight.resize(j);
	array_t<uint32_t> cnt2(j+1);
	cnt2[0] = 0;
	avg = 0;
	for (i = 0; i < weight[0]; ++ i)
	    cnt2[0] += cnt[i];
	avg += cnt2[0];
	for (i = 1; i < j; ++ i) {
	    cnt2[i] = 0;
	    for (uint32_t ki = weight[i-1]+1; ki < weight[i]; ++ ki)
		cnt2[i] += cnt[ki];
	    avg += cnt2[i];
	}
	cnt2[j] = 0;
	for (i=weight.back()+1; i<ncnt; ++i)
	    cnt2[j] += cnt[i];
	avg += cnt2[j];
	avg = ((avg > nb-j) ? (avg + ((nb-j)>>1)) / (nb-j) : 1);
	top = (avg >> 1);

	// initial assignment of the number of bins to use
	array_t<uint32_t> nb2(j+1);
	for (i = 0; i <= j; ++ i) {
	    nb2[i] = (top + cnt2[i]) / avg; // round to nearest integer
	    if (nb2[i] == 0 && cnt2[i] > 0) {
		nb2[i] = 1;
	    }
	    else if (i == j) {
		if (nb2[i] > ncnt - weight.back() - 1)
		    nb2[i] = ncnt - weight.back() - 1;
	    }
	    else if (i > 0) {
		if (nb2[i] > weight[i] - weight[i-1] - 1)
		    nb2[i] = weight[i] - weight[i-1] - 1;
	    }
	    else if (i == 0 && nb2[0] > weight[0]) {
		nb2[0] = weight[0];
	    }
	}

	// attempt to make sure the total number of bins is exactly nb
	// avg stores the total number of bins
	for (i=0, avg=j; i<j+1; ++i)
	    avg += nb2[i];
	while (avg > nb) { // need to reduce the number of bins
	    top = 0;
	    double frac = DBL_MAX;
	    // find the interval containing the most bins
	    if (nb2[0] > 1)
		frac = static_cast<double>(cnt2[0]) / nb2[0];
	    for (i = 1; i <= j; ++i) {
		if (nb2[i] > 1) {
		    if (frac < DBL_MAX) {
			if (frac*nb2[i] < cnt2[i]) {
			    top = i;
			    frac = static_cast<double>(cnt2[i]) / nb2[i];
			}
			else if (frac*nb2[i]==cnt2[i] &&
				 cnt2[i]>cnt2[top]) {
			    top = i;
			    frac = static_cast<double>(cnt2[i]) / nb2[i];
			}
		    }
		    else {
			top = i;
			frac = static_cast<double>(cnt2[i]) / nb2[i];
		    }
		}
	    }
	    if (frac == DBL_MAX) { // can not use less bins
		break;
	    }
	    // reduce the number of bins in the range top
	    -- nb2[top];
	    -- avg;
	} // while (avg > nb)
	while (avg < nb) { // need to use more bins
	    top = 0;
	    double frac = (nb2[0] < weight[0] ?
			   (nb2[0]>0 ? cnt2[0]/nb2[0] : cnt2[0]) : 0.0);
	    // find the crowdest bin
	    for (i = 1; i <= j; ++ i) {
		if (nb2[i] > 0 && nb2[i] <
		    (i<j ? nb2[i] > weight[i] - weight[i-1] - 1 :
		     ncnt - weight.back() - 1)) {
		    if (frac*nb2[i] > cnt2[i]) {
			top = i;
			frac = static_cast<double>(cnt2[i]) / nb2[i];
		    }
		    else if (frac*nb2[i]==cnt2[i] && cnt2[i]>cnt2[top]) {
			top = i;
			frac = static_cast<double>(cnt2[i]) / nb2[i];
		    }
		    else if (frac <= 0.0) {
			top = i;
			frac = static_cast<double>(cnt2[i]) / nb2[i];
		    }
		}
	    }
	    if (frac == 0.0) { // can not use more bins
		break;
	    }
	    // increase the number of bins in the range top
	    ++ nb2[top];
	    ++ avg;
	} // while (avg < nb)

	// actually establish the boundaries
	if (nb2[0] > 1) {
	    bdry.resize(nb2[0]);
	    const array_t<uint32_t> tmp(cnt, 0, weight[0]);
	    if (ibis::gVerbose > 7)
		ibis::util::logMessage
		    ("divideCounts", "attempting to divide [0, %lu) into %lu "
		     "bins", static_cast<long unsigned>(weight[0]),
		     static_cast<long unsigned>(nb2[0]));
	    divideCounts(bdry, tmp);
	}
	else if (nb2[0] == 1) {
	    bdry[0] = weight[0];
	    bdry.resize(1);
	}
	else {
	    bdry.clear();
	}
	for (i=0; i<j; ++i) {
	    uint32_t off = weight[i] + 1;
	    bdry.push_back(off);
	    if (nb2[i+1] > 1) {
		const array_t<uint32_t> tmp(cnt, off,
					    (i+1<j?weight[i+1]:ncnt)-off);
		array_t<uint32_t> bnd(nb2[i+1]);
		if (ibis::gVerbose > 7)
		    ibis::util::logMessage
			("divideCounts", "attempting to divide [%lu, %lu) "
			 "into %lu bins", static_cast<long unsigned>(off),
			 static_cast<long unsigned>(i+1<j?weight[i+1]:ncnt),
			 static_cast<long unsigned>(nb2[i+1]));
		divideCounts(bnd, tmp);
		for (array_t<uint32_t>::const_iterator it = bnd.begin();
		     it != bnd.end(); ++it)
		    bdry.push_back(off + *it);
	    }
	    else if (nb2[i+1] == 1) {
		bdry.push_back(i+1<j?weight[i+1]:ncnt);
	    }
	}
    }

    if (ibis::gVerbose > 8) {
	ibis::util::logger lg;
	lg.buffer()
	    << "divideCounts() binning result (i, cnt[i], sum cnt[i])\n";
	for (i = 0, top=0; i < bdry[0]; ++i) {
	    top += cnt[i];
	    lg.buffer() << i << "\t" << cnt[i] << "\t" << top << "\n";
	}
	if (bdry[0] > 0) {
	    lg.buffer() << "-^- bin 0 -^-\n";
	}
	else {
	    lg.buffer() << "WARNING: divideCounts() bin: 0 is empty\n";
	}
	for (j = 1; j < bdry.size(); ++j) {
	    for (i = bdry[j-1], top = 0; i < bdry[j]; ++i) {
		top += cnt[i];
		if (i < (bdry[j-1]+(1<<ibis::gVerbose))) {
		    lg.buffer() << i << "\t" << cnt[i] << "\t" << top << "\n";
		}
		else if (i+1 == bdry[j]) {
		    if (i > (bdry[j-1]+(1<<ibis::gVerbose)))
			lg.buffer() << "...\n";
		    lg.buffer() << i << "\t" << cnt[i] << "\t" << top << "\n";
		}
	    }
	    if (bdry[j] > bdry[j-1]) {
		lg.buffer() << "-^- bin " << j << "\n";
	    }
	    else {
		lg.buffer() << "WARNING: divideCounts() bin: " << j << " ["
			    << bdry[j-1] << ", " << bdry[j] << ") is empty\n";
	    }
	}
    }
    else {
	weight.resize(bdry.size()); // actually compute the weights
	for (i = 0; i < bdry.size(); ++i) {
	    weight[i] = 0;
	    for (j = (i==0 ? 0 : bdry[i-1]); j < bdry[i]; ++j)
		weight[i] += cnt[j];
	    if (weight[i] == 0) {
		ibis::util::logMessage("divideCounts",
				       "bin:%lu [%lu, %lu) is empty",
				       static_cast<long unsigned>(i),
				       static_cast<long unsigned>(i==0 ? 0 :
								  bdry[i-1]),
				       static_cast<long unsigned>(bdry[i]));
	    }
	}

	if (ibis::gVerbose > 6) {
	    array_t<uint32_t>::const_iterator it;
	    ibis::util::logger lg;
	    lg.buffer() << "divideCounts():\n    cnt(" << ncnt << ") = [";
	    if (ncnt < 256) {
		for (it = cnt.begin(); it != cnt.end(); ++it)
		    lg.buffer() << " " << *it;
	    }
	    else {
		for (i = 0; i < 128; ++i)
		    lg.buffer() << " " << cnt[i];
		lg.buffer() << " ... " << cnt.back();
	    }
	    lg.buffer() << "];\nbounds(" << bdry.size() << ") = [";
	    if (bdry.size() < 256) {
		for (it = bdry.begin(); it != bdry.end(); ++it)
		    lg.buffer() << " " << *it;
	    }
	    else {
		for (i = 0; i < 128; ++i)
		    lg.buffer() << " " << bdry[i];
		lg.buffer() << " ... " << bdry.back();
	    }
	    lg.buffer() << "]\nweights(" << bdry.size() << ") = [";
	    if (weight.size() < 256) {
		for (it = weight.begin(); it != weight.end(); ++it)
		    lg.buffer() << " " << *it;
	    }
	    else {
		for (i = 0; i < 128; ++ i)
		    lg.buffer() << " " << weight[i];
		lg.buffer() << " ... " << weight.back();
	    }
	    lg.buffer() << "]\n";
	}
    }
} // ibis::index::divideCounts

// activate all bitvectors
void ibis::index::activate() const {
    ibis::column::mutexLock lock(col, "index::activate");

    const uint32_t nobs = bits.size();
    bool missing = false; // any bits[i] missing (is 0)?
    for (uint32_t i = 0; i < nobs && ! missing; ++ i)
	missing = (bits[i] == 0);
    if (missing == false) return;

    if (offsets.size() <= nobs) {
	col->logWarning("index::activate", "no records of offsets, can "
			"not regenerate the bitvectors");
    }
    else if (str) { // using a ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose > 8)
	    << "ibis::column[" << col->name()
	    << "]::index::activate(all) from ibis::fileManager::storage(0x"
	    << str << ")";

	for (uint32_t i = 0; i < nobs; ++i) {
	    if (bits[i] == 0 && offsets[i+1] > offsets[i]) {
#if defined(DEBUG)
		if (ibis::gVerbose > 5) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "index::activate -- activating bitvector "
			<< i << " from a raw storage ("
			<< static_cast<const void*>(str->begin())
			<< "), offsets[" << i << "]= " << offsets[i]
			<< ", offsets[" << i+1 << "]= " << offsets[i+1];
		}
#endif
		array_t<ibis::bitvector::word_t>
		    a(str, offsets[i], (offsets[i+1]-offsets[i]) /
		      sizeof(ibis::bitvector::word_t));
		bits[i] = new ibis::bitvector(a);
		bits[i]->setSize(nrows);
	    }
	}
    }
    else if (fname) { // using the named file directly
	int fdes = UnixOpen(fname, OPEN_READONLY);
	if (fdes >= 0) {
	    LOGGER(ibis::gVerbose > 8)
		<< "ibis::column[" << col->name()
		<< "]::index::activate(all) from file \"" << fname << "\"";

#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fdes, _O_BINARY);
#endif
	    uint32_t i = 0;
	    while (i < nobs) {
		// skip to next empty bit vector
		while (i < nobs && bits[i] != 0)
		    ++ i;
		// the last bitvector to activate. can not be larger
		// than j
		uint32_t aj = (i<nobs ? i + 1 : nobs);
		while (aj < nobs && bits[aj] == 0)
		    ++ aj;
		if (offsets[aj] > offsets[i]) {
		    const uint32_t start = offsets[i];
		    ibis::fileManager::storage *a0 = new
			ibis::fileManager::storage(fdes, start,
						   offsets[aj]);
		    while (i < aj) {
			if (offsets[i+1] > offsets[i]) {
			    array_t<ibis::bitvector::word_t>
				a1(a0, offsets[i]-start,
				   (offsets[i+1]-offsets[i])/
				   sizeof(ibis::bitvector::word_t));
			    bits[i] = new ibis::bitvector(a1);
			    bits[i]->setSize(nrows);
#if defined(DEBUG)
			    if (ibis::gVerbose > 5) {
				LOGGER(ibis::gVerbose >= 0)
				    << "index::activate -- "
				    "activating bitvector " << i
				    << "by reading file " << fname
				    << "offsets[" << i << "]= " << offsets[i]
				    << ", offsets[" << i+1 << "]= "
				    << offsets[i+1];
			    }
#endif
			}
			++ i;
		    }
		}
		i = aj; // always advance i
	    }
	    UnixClose(fdes);
	}
	else {
	    col->logWarning("activate", "failed to open file \"%s\" ... %s",
			    fname, strerror(errno));
	}
    }
    else {
	col->logWarning("index::activate", "can not regenerate bitvectors "
			"because neither str or fname is specified");
    }
} // ibis::index::activate

// activate the ith bitvector
void ibis::index::activate(uint32_t i) const {
    if (i >= bits.size()) return;	// index out of range
    ibis::column::mutexLock lock(col, "index::activate");

    if (bits[i] != 0) return;	// already active
    if (offsets.size() <= bits.size()) {
	col->logWarning("index::activate", "no records of offsets, can not "
			"regenerate bitvector %lu",
			static_cast<long unsigned>(i));
    }
    else if (offsets[i+1] <= offsets[i]) {
	return;
    }
    if (str) { // using a ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose > 8)
	    << "ibis::column[" << col->name() << "]::index::activate("
	    << i << ") from ibis::fileManager::storage(0x" << str << ")";

	array_t<ibis::bitvector::word_t>
	    a(str, offsets[i], (offsets[i+1]-offsets[i]) /
	      sizeof(ibis::bitvector::word_t));
	bits[i] = new ibis::bitvector(a);
	bits[i]->setSize(nrows);
#if defined(DEBUG)
	if (ibis::gVerbose > 5) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "index::activate(" << i
		<< ") constructed a bitvector from range ["
		<< offsets[i] << ", " << offsets[i+1] << ") of a storage at "
		<< static_cast<const void*>(str->begin()) << "\nbits[" << i
		<< "]=" << *(bits[i]);
	}
#endif
    }
    else if (fname) { // using the named file directly
	int fdes = UnixOpen(fname, OPEN_READONLY);
	if (fdes >= 0) {
	    LOGGER(ibis::gVerbose > 8)
		<< "ibis::column[" << col->name() << "]::index::activate("
		<< i << ") from file \"" << fname << "\"";

#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fdes, _O_BINARY);
#endif
	    array_t<ibis::bitvector::word_t> a0(fdes, offsets[i],
						offsets[i+1]);
	    bits[i] = new ibis::bitvector(a0);
	    UnixClose(fdes);
	    bits[i]->setSize(nrows);
#if defined(DEBUG)
	    if (ibis::gVerbose > 5) {
		LOGGER(ibis::gVerbose >= 0)
		    << "index::activate(" << i
		    << ") constructed a bitvector from range ["
		    << offsets[i] << ", " << offsets[i+1] << ") of file "
		    << fname << "\nbits[" << i << "]=" << *(bits[i]);
	    }
#endif
	}
	else {
	    col->logWarning("activate",
			    "failed to open file \"%s\" ... %s",
			    fname, strerror(errno));
	}
    }
    else {
	col->logWarning("index::activate", "can not regenerate bitvector %lu "
			"because neither str or fname is specified",
			static_cast<long unsigned>(i));
    }
} // ibis::index::activate

// activate bitvectors [i, j).
void ibis::index::activate(uint32_t i, uint32_t j) const {
    if (j > bits.size())
	j = bits.size();
    if (i >= j || i >= bits.size()) // empty range
	return;
    ibis::column::mutexLock lock(col, "index::activate");

    bool incore = (bits[i] != 0);
    for (uint32_t k = i+1; k < j && incore; ++ k)
	incore = (bits[k] != 0);
    if (incore) return; // all bitvectors active

    if (offsets.size() <= bits.size()) {
	col->logWarning("index::activate", "no records of offsets, can not "
			"regenerate bitvectors %lu:%lu",
			static_cast<long unsigned>(i),
			static_cast<long unsigned>(j));
    }
    else if (str) { // using an ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose > 8)
	    << "ibis::column[" << col->name() << "]::index::activate("
	    << i << ", " << j << ") from ibis::fileManager::storage(0x"
	    << str << ")";

	while (i < j) {
	    if (bits[i] == 0 && offsets[i+1] > offsets[i]) {
		array_t<ibis::bitvector::word_t>
		    a(str, offsets[i], (offsets[i+1]-offsets[i]) /
		      sizeof(ibis::bitvector::word_t));
		bits[i] = new ibis::bitvector(a);
		bits[i]->setSize(nrows);
#if defined(DEBUG)
		if (ibis::gVerbose > 5) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "index::activate(" << i
			<< ") constructed a bitvector from range ["
			<< offsets[i] << ", " << offsets[i+1]
			<< ") of a storage at "
			<< static_cast<const void*>(str->begin());
		}
#endif
	    }
	    ++ i;
	}
    }
    else if (fname) { // using the named file directly
	if (offsets[j] > offsets[i]) {
	    int fdes = UnixOpen(fname, OPEN_READONLY);
	    if (fdes >= 0) {
		LOGGER(ibis::gVerbose > 8)
		    << "ibis::column[" << col->name()
		    << "]::index::activate(" << i << ", " << j
		    << ") from file \"" << fname << "\"";

#if defined(_WIN32) && defined(_MSC_VER)
		(void)_setmode(fdes, _O_BINARY);
#endif
		while (i < j) {
		    // skip to next empty bit vector
		    while (i < j && bits[i] != 0)
			++ i;
		    // the last bitvector to activate. can not be larger
		    // than j
		    uint32_t aj = (i<j ? i + 1 : j);
		    while (aj < j && bits[aj] == 0)
			++ aj;
		    if (offsets[aj] > offsets[i]) {
			const uint32_t start = offsets[i];
			ibis::fileManager::storage *a0 = new
			    ibis::fileManager::storage(fdes, start,
						       offsets[aj]);
			while (i < aj) {
			    if (offsets[i+1] > offsets[i]) {
				array_t<ibis::bitvector::word_t>
				    a1(a0, offsets[i]-start,
				       (offsets[i+1]-offsets[i])/
				       sizeof(ibis::bitvector::word_t));
				bits[i] = new ibis::bitvector(a1);
				bits[i]->setSize(nrows);
#if defined(DEBUG)
				if (ibis::gVerbose > 5) {
				    LOGGER(ibis::gVerbose >= 0)
					<< "index::activate(" << i
					<< ") constructed a bitvector "
					"from range ["
					<< offsets[i] << ", "
					<< offsets[i+1]
					<< ") of file " << fname;
				}
#endif
			    }
			    ++ i;
			}
		    }
		    i = aj; // always advance i
		}
		UnixClose(fdes);
	    }
	    else {
		col->logWarning("activate",
				"failed to open file \"%s\" ... %s",
				fname, strerror(errno));
	    }
	}
    }
    else {
	col->logWarning("index::activate", "can not regenerate bitvector %lu "
			"because neither str or fname is specified",
			static_cast<long unsigned>(i));
    }
} // ibis::index::activate

/// The most important difference between this function and @c sumBins is
/// that this function always use @c bits[ib] through @c bits[ie-1].  This
/// is similar to the function @c addBits.
void ibis::index::addBins(uint32_t ib, uint32_t ie,
			  ibis::bitvector& res) const {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index[" << col->name() << "]::addBins(" << ib << ", "
	<< ie << ", res(" << res.cnt() << ", " << res.size() << ")) ...";
    const uint32_t nobs = bits.size();
    if (res.cnt() >= nrows) return; // useless to add more bits
    if (res.size() != nrows)
	res.adjustSize(0, nrows);
    if (ie > nobs)
	ie = nobs;
    if (ib >= ie)
	return;
    if (ib == 0 && ie == nobs) {
	res.set(1, nrows);
	return;
    }

    activate(ib, ie);
    for (; ib < ie && ib < nobs && bits[ib] == 0; ++ ib);
    const uint32_t na = ie - ib;
    if (na <= 2) { // some special cases
	if (na == 1) {
	    if (bits[ib])
		res |= *(bits[ib]);
	}
	else if (na > 1) { // two consecutive bitmaps in the range
	    if (bits[ib])
		res |= (*(bits[ib]));
	    if (bits[ib+1])
		res |= *(bits[ib+1]);
	}
	else {
	    res.set(0, nrows);
	}
	return;
    }

    horometer timer;
    uint32_t bytes = 0;
    // compute the total size of all bitmaps
    for (uint32_t i = ib; i < ie; ++i) {
	if (bits[i])
	    bytes += bits[i]->bytes();
    }
    // based on extensive testing, we have settled on the following
    // combination
    // (1) if the total size of the first two are greater than the
    // uncompressed size of one bitmap, use option 1.  Because the
    // operation of the first two will produce an uncompressed result,
    // it will sum together all other bits with to the uncompressed
    // result generated already.
    // (2) if total size (bytes) times log 2 of number of bitmaps
    // is less than or equal to the size of an uncompressed
    // bitmap, use option 3, else use option 4.
    if (ibis::gVerbose > 4) {
	ibis::util::logMessage("index", "addBins(%lu, %lu) will operate on "
			       "%lu out of %lu bitmaps using the combined "
			       "option", static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       static_cast<long unsigned>(na),
			       static_cast<long unsigned>(nobs));
	timer.start();
    }
    const uint32_t uncomp = (ibis::bitvector::bitsPerLiteral() == 8 ?
			     nrows * 2 / 15 :
			     nrows * 4 / 31);
    const uint32_t sum2 = (bits[ib] ? bits[ib]->bytes() : 0U) +
	(bits[ib+1] ? bits[ib+1]->bytes() : 0U);
    if (sum2 >= uncomp) {
	LOGGER(ibis::gVerbose > 5)
	    << "ibis::inex::addBins(" << ib << ", " << ie
	    << ") takes a simple loop to OR the bitmaps";
	for (uint32_t i = ib; i < ie; ++i) {
	    if (bits[i])
		res |= *(bits[i]);
	}
    }
    else if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
	LOGGER(ibis::gVerbose > 5)
	    << "ibis::inex::addBins(" << ib << ", " << ie
	    << ") uses a priority queue to OR the bitmaps";
	typedef std::pair<ibis::bitvector*, bool> _elem;
	// put all bitmaps in a priority queue
	std::priority_queue<_elem> que;
	_elem op1, op2, tmp;
	tmp.first = 0;

	// populate the priority queue with the original input
	for (uint32_t i = ib; i < ie; ++i) {
	    if (bits[i]) {
		op1.first = bits[i];
		op1.second = false;
		que.push(op1);
	    }
	}

	try {
	    while (! que.empty()) {
		op1 = que.top();
		que.pop();
		if (que.empty()) {
		    res.copy(*(op1.first));
		    if (op1.second) delete op1.first;
		    break;
		}

		op2 = que.top();
		que.pop();
		tmp.second = true;
		tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
		LOGGER(ibis::gVerbose >= 0)
		    << "addBins-using priority queue: " << op1.first->bytes()
		    << (op1.second ? "(transient), " : ", ")
		    << op2.first->bytes()
		    << (op2.second ? "(transient) >> " : " >> ")
		    << tmp.first->bytes();
#endif
		if (op1.second)
		    delete op1.first;
		if (op2.second)
		    delete op2.first;
		if (! que.empty()) {
		    que.push(tmp);
		    tmp.first = 0;
		}
	    }
	    if (tmp.first != 0) {
		if (tmp.second) {
		    res |= *(tmp.first);
		    delete tmp.first;
		    tmp.first = 0;
		}
		else {
		    res |= *(tmp.first);
		}
	    }
	}
	catch (...) { // need to free the pointers
	    delete tmp.first;
	    while (! que.empty()) {
		tmp = que.top();
		if (tmp.second)
		    delete tmp.first;
		que.pop();
	    }
	    throw;
	}
    }
    else if (sum2 <= (uncomp >> 2)) {
	LOGGER(ibis::gVerbose > 5)
	    << "ibis::inex::addBins(" << ib << ", " << ie
	    << ") decompresses the result bitmap before ORing the bitmaps";
	// use uncompressed res
	while (ib < ie && bits[ib] == 0)
	    ++ ib;
	if (ib < ie) {
	    if (bits[ib])
		res |= *(bits[ib]);
	    ++ ib;
	}
	res.decompress();
	for (uint32_t i = ib; i < ie; ++ i) {
	    if (bits[i])
		res |= *(bits[i]);
	}
    }
    else {
	LOGGER(ibis::gVerbose > 5)
	    << "ibis::inex::addBins(" << ib << ", " << ie
	    << ") takes a simple loop to OR the bitmaps";
	for (uint32_t i = ib; i < ie; ++ i)
	    if (bits[i])
		res |= *(bits[i]);
    }

    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("index", "addBins operated on %u bitmap%s "
			       "(%lu in %lu out) took %g sec(CPU), "
			       "%g sec(elapsed).",
			       static_cast<unsigned>(na), (na>1?"s":""),
			       static_cast<long unsigned>(bytes),
			       static_cast<long unsigned>(res.bytes()),
			       timer.CPUTime(), timer.realTime());
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "addBins(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::addBins

/// This is basically a copy of the function @c sumBins (without the 4th
/// arguments).  There are two changes: (1) if @c res has the same number
/// of bits as @c tot, the new sum is added to the existing bitvector, and
/// (2) when it computes the sum through complements, it performs a
/// subtraction from @c tot.
void ibis::index::addBins(uint32_t ib, uint32_t ie, ibis::bitvector& res,
			  const ibis::bitvector& tot) const {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index[" << col->name() << "]::addBins(" << ib
	<< ", " << ie << ", res(" << res.cnt() << ", " << res.size()
	<< "), tot(" << tot.cnt() << ", " << tot.size() << ")) ...";
    if (res.size() != tot.size())
	res.adjustSize(0U, tot.size());
    if (ib >= ie) { // no bitmap in the range
	return;
    }

    typedef std::pair<ibis::bitvector*, bool> _elem;
    const uint32_t nobs = bits.size();
    if (ie > nobs) ie = nobs;
    bool straight = true;
    if (offsets.empty()) { // all bitvectors must be in memory
	try {
	    offsets.resize(nobs+1);
	    offsets[0] = 0;
	    for (uint32_t i = 0; i < nobs; ++ i)
		offsets[i+1] = offsets[i] +
		    (bits[i] ? bits[i]->bytes() : 0U);
	}
	catch (...) {
	    offsets.clear();
	}
    }
    if (offsets.size() > nobs) {
	const uint32_t all = offsets[nobs] - offsets[0];
	const uint32_t mid = offsets[ie] - offsets[ib];
	straight = (mid <= (all >> 1));
    }
    else {
	straight = (ie-ib <= (nobs >> 1));
    }

    if (str || fname) { // try to activate the needed bitmaps
	if (straight) {
	    activate(ib, ie);
	}
	else {
	    activate(0, ib);
	    activate(ie, nobs);
	}
    }

    const uint32_t na = (straight ? ie-ib : nobs + ib - ie);
    if (na <= 2) { // some special cases
	if (ib >= ie) { // no bitmap in the range
	    res.set(0, nrows);
	}
	else if (ib == 0 && ie == nobs) { // every bitmap in the range
	    res |= tot;
	}
	else if (na == 1) {
	    if (straight) { // only one bitmap in the range
		if (bits[ib])
		    res |= (*(bits[ib]));
	    }
	    else if (ib == 0) { // last one is outside
		if (bits[ie]) {
		    ibis::bitvector tmp(tot);
		    tmp -= *(bits[ie]);
		    res |= tmp;
		}
		else {
		    res |= tot;
		}
	    }
	    else { // the first one is outside
		ibis::bitvector tmp(tot);
		if (bits[0])
		    tmp -= *(bits[0]);
		res |= tmp;
	    }
	}
	else if (straight) { // two consecutive bitmaps in the range
	    if (bits[ib])
		res |= *(bits[ib]);
	    if (bits[ib+1])
		res |= *(bits[ib+1]);
	}
	else if (ib == 0) { // two bitmaps at the end are outside
	    ibis::bitvector tmp(tot);
	    if (bits[ie])
		tmp -= (*(bits[ie]));
	    if (bits[nobs-1])
		tmp -= *(bits[nobs-1]);
	    res |= tmp;
	}
	else if (ib == 1) { // two outside bitmaps are split
	    ibis::bitvector tmp(tot);
	    if (bits[0])
		tmp -= (*(bits[0]));
	    if (bits[ie])
		tmp -= *(bits[ie]);
	    res |= tmp;
	}
	else if (ib == 2) { // two outside bitmaps are at the beginning
	    ibis::bitvector tmp(tot);
	    if (bits[0])
		tmp -= (*(bits[0]));
	    if (bits[1])
		tmp -= *(bits[1]);
	    res |= tmp;
	}
	return;
    }

    horometer timer;
    uint32_t bytes = 0;
    // based on extensive testing, we have settled on the following
    // combination
    // (1) if the two size of the first two are greater than the
    // uncompressed size of one bitmap, use option 1.  Because the
    // operation of the first two will produce an uncompressed result,
    // it will sum together all other bits with to the uncompressed
    // result generated already.
    // (2) if total size (bytes) times log 2 of number of bitmaps
    // is less than or equal to the size of an uncompressed
    // bitmap, use option 3, else use option 4.
    if (ibis::gVerbose > 4) {
	ibis::util::logMessage("index", "addBins(%lu, %lu) will operate on "
			       "%lu out of %lu bitmaps using the combined "
			       "option", static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       static_cast<long unsigned>(na),
			       static_cast<long unsigned>(nobs));
	timer.start();
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i) {
		if (bits[i])
		    bytes += bits[i]->bytes();
	    }
	}
	else {
	    for (uint32_t i = 0; i < ib; ++i) {
		if (bits[i])
		    bytes += bits[i]->bytes();
	    }
	    for (uint32_t i = ie; i < nobs; ++i) {
		if (bits[i])
		    bytes += bits[i]->bytes();
	    }
	}
    }
    const uint32_t uncomp = (ibis::bitvector::bitsPerLiteral() == 8 ?
			     nrows * 2 / 15 : nrows * 4 / 31);
    if (straight) {
	uint32_t sum2 = (bits[ib] ? bits[ib]->bytes() : 0U) +
	    (bits[ib+1] ? bits[ib+1]->bytes() : 0U);
	if (sum2 >= uncomp) { // let the automatic decompression work
	    LOGGER(ibis::gVerbose > 5)
		<< "ibis::inex::addBins(" << ib << ", " << ie
		<< ") takes a simple loop to OR the bitmaps";
	    for (uint32_t i = ib; i < ie; ++i) {
		if (bits[i])
		    res |= *(bits[i]);
	    }
	}
	else {
	    // need to compute the total size of all bitmaps
	    if (bytes == 0) {
		for (uint32_t i = ib; i < ie; ++i) {
		    if (bits[i])
			bytes += bits[i]->bytes();
		}
	    }
	    if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::addBins(" << ib << ", " << ie
		    << ") uses a priority queue to OR the bitmaps";
		// put all bitmaps in a priority queue
		std::priority_queue<_elem> que;
		_elem op1, op2, tmp;
		tmp.first = 0;

		// populate the priority queue with the original input
		for (uint32_t i = ib; i < ie; ++i) {
		    if (bits[i]) {
			op1.first = bits[i];
			op1.second = false;
			que.push(op1);
		    }
		}

		try {
		    while (! que.empty()) {
			op1 = que.top();
			que.pop();
			if (que.empty()) {
			    res.copy(*(op1.first));
			    if (op1.second) delete op1.first;
			    break;
			}

			op2 = que.top();
			que.pop();
			tmp.second = true;
			tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			LOGGER(ibis::gVerbose >= 0)
			    << "addBins-using priority queue: "
			    << op1.first->bytes()
			    << (op1.second ? "(transient), " : ", ")
			    << op2.first->bytes()
			    << (op2.second ? "(transient) >> " : " >> ")
			    << tmp.first->bytes();
#endif
			if (op1.second) delete op1.first;
			if (op2.second) delete op2.first;
			if (! que.empty()) {
			    que.push(tmp);
			    tmp.first = 0;
			}
		    }
		    if (tmp.first != 0) {
			if (tmp.second) {
			    res |= *(tmp.first);
			    delete tmp.first;
			    tmp.first = 0;
			}
			else {
			    res |= *(tmp.first);
			}
		    }
		}
		catch (...) { // need to free the pointers
		    delete tmp.first;
		    while (! que.empty()) {
			tmp = que.top();
			if (tmp.second)
			    delete tmp.first;
			que.pop();
		    }
		    throw;
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::addBins(" << ib << ", " << ie
		    << ") decompresses the result before ORing the bitmaps";
		res.decompress(); // explicit decompression needed
		for (uint32_t i = ib; i < ie; ++i) {
		    if (bits[i])
			res |= *(bits[i]);
		}
	    }
	}
    } // if (straight)
    else { // use complements
	uint32_t sum2;
	ibis::bitvector sum;
	if (ib > 1) {
	    activate(0, 2);
	    if (bits[0] && bits[1])
		sum2 = bits[0]->bytes() + (bits[1] ? bits[1]->bytes() : 0U);
	    else if (bits[0])
		sum2 = bits[0]->bytes();
	    else if (bits[1])
		sum2 = bits[1]->bytes();
	    else
		sum2 = 0;
	}
	else if (ib == 1) {
	    if (bits[0]) {
		if (bits[ie])
		    sum2 = bits[0]->bytes() + bits[ie]->bytes();
		else
		    sum2 = bits[0]->bytes();
	    }
	    else if (bits[ie]) {
		sum2 = bits[ie]->bytes();
	    }
	    else {
		sum2 = 0;
	    }
	}
	else {
	    sum2 = (bits[ie] ? bits[ie]->bytes() : 0U) +
		(bits[ie+1] ? bits[ie+1]->bytes() : 0U);
	}
	if (sum2 >= uncomp) { // take advantage of built-in decopression
	    LOGGER(ibis::gVerbose > 5)
		<< "ibis::inex::addBins(" << ib << ", " << ie
		<< ") takes a simple loop to OR the bitmaps (complement)";
	    if (ib > 1) {
		if (bits[0])
		    sum.copy(*(bits[0]));
		else
		    sum.set(0, nrows);
		for (uint32_t i = 1; i < ib; ++i)
		    if (bits[i])
			sum |= *(bits[i]);
	    }
	    else if (ib == 1) {
		if (bits[0])
		    sum.copy(*(bits[0]));
		else
		    sum.set(0, nrows);
	    }
	    else {
		for (; bits[ie] == 0 && ie < nobs; ++ ie);
		if (ie < nobs)
		    sum.copy(*(bits[ie]));
		else
		    sum.set(0, nrows);
	    }
	    for (uint32_t i = ie; i < nobs; ++i) {
		if (bits[i])
		    sum |= *(bits[i]);
	    }
	}
	else { // need to look at the total size
	    if (bytes == 0) {
		for (uint32_t i = 0; i < ib; ++i) {
		    if (bits[i])
			bytes += bits[i]->bytes();
		}
		for (uint32_t i = ie; i < nobs; ++i) {
		    if (bits[i])
			bytes += bits[i]->bytes();
		}
	    }
	    if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::addBins(" << ib << ", " << ie
		    << ") uses a priority queue to OR the bitmaps (complement)";
		// use priority queue for all bitmaps
		std::priority_queue<_elem> que;
		_elem op1, op2, tmp;
		tmp.first = 0;

		// populate the priority queue with the original input
		for (uint32_t i = 0; i < ib; ++i) {
		    if (bits[i]) {
			op1.first = bits[i];
			op1.second = false;
			que.push(op1);
		    }
		}
		for (uint32_t i = ie; i < nobs; ++i) {
		    if (bits[i]) {
			op1.first = bits[i];
			op1.second = false;
			que.push(op1);
		    }
		}

		try {
		    while (! que.empty()) {
			op1 = que.top();
			que.pop();
			if (que.empty()) {
			    res.copy(*(op1.first));
			    if (op1.second) delete op1.first;
			    break;
			}

			op2 = que.top();
			que.pop();
			tmp.second = true;
			tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			LOGGER(ibis::gVerbose >= 0)
			    << "addBins-using priority queue: "
			    << op1.first->bytes()
			    << (op1.second ? "(transient), " : ", ")
			    << op2.first->bytes()
			    << (op2.second ? "(transient) >> " : " >> ")
			    << tmp.first->bytes();
#endif
			if (op1.second) delete op1.first;
			if (op2.second) delete op2.first;
			if (! que.empty()) {
			    que.push(tmp);
			    tmp.first = 0;
			}
		    }
		    if (tmp.first != 0) {
			if (tmp.second) {
			    sum.swap(*(tmp.first));
			    delete tmp.first;
			    tmp.first = 0;
			}
			else {
			    sum.copy(*(tmp.first));
			}
		    }
		}
		catch (...) { // need to free the pointers
		    delete tmp.first;
		    while (! que.empty()) {
			tmp = que.top();
			if (tmp.second)
			    delete tmp.first;
			que.pop();
		    }
		    throw;
		}
	    }
	    else if (sum2 <= (uncomp >> 2)){
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::addBins(" << ib << ", " << ie
		    << ") decompresses the result before ORing the "
		    "bitmaps (complement)";
		// uncompress the first bitmap generated
		if (ib > 1) {
		    if (bits[0])
			sum.copy(*(bits[0]));
		    else
			sum.set(0, nrows);
		    if (bits[1]) {
			sum |= *(bits[1]);
		    }
		    sum.decompress();
		    for (uint32_t i = 2; i < ib; ++i)
			if (bits[i])
			    sum |= *(bits[i]);
		}
		else if (ib == 1) {
		    if (bits[0])
			sum.copy(*(bits[0]));
		    else
			sum.set(0, nrows);
		    sum.decompress();
		}
		else {
		    for (; bits[ie] == 0 && ie < nobs; ++ ie);
		    if (ie < nobs) {
			sum.copy(*(bits[ie]));
			++ ie;
			if (ie < nobs)
			    sum.decompress();
		    }
		    else {
			sum.set(0, nrows);
		    }
		}
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bits[i])
			sum |= *(bits[i]);
	    }
	    else if (ib > 0) {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::addBins(" << ib << ", " << ie
		    << ") decompresses the result before ORing the "
		    "bitmaps (complement)";
		if (bits[0])
		    sum.copy(*(bits[0]));
		else
		    sum.set(0, nrows);
		sum.decompress();
		for (uint32_t i = 1; i < ib; ++i)
		    if (bits[i])
			sum |= *(bits[i]);
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bits[i])
			sum |= *(bits[i]);
	    }
	    else {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::addBins(" << ib << ", " << ie
		    << ") decompresses the result before ORing the "
		    "bitmaps (complement)";
		for (; bits[ie] == 0 && ie < nobs; ++ ie);
		if (ie < nobs) {
		    sum.copy(*(bits[ie]));
		    ++ ie;
		    if (ie < nobs)
			sum.decompress();
		}
		else {
		    sum.set(0, nrows);
		}
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bits[i])
			sum |= *(bits[i]);
	    }
	}
	// need to flip because we have been using complement
	ibis::bitvector tmp(tot);
	tmp -= sum;
	res |= tmp;
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("index", "addBins operated on %u bitmap%s "
			       "(%lu in %lu out) took %g sec(CPU), "
			       "%g sec(elapsed).",
			       static_cast<unsigned>(na), (na>1?"s":""),
			       static_cast<long unsigned>(bytes),
			       static_cast<long unsigned>(res.bytes()),
			       timer.CPUTime(), timer.realTime());
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "addBins(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::addBins

/// Sum up <code>bits[ib:ie-1]</code> and place the result in res.  The
/// bitmaps (bits) are held by this index object and may be regenerated as
/// needed.  It uses the combined strategy that was determined in previous
/// tests.  The basic rule is as follows.
/// - If there are two or less bit vectors, use |= operator directly.
/// - Compute the total size of the bitmaps involved.
/// - If the total size times log(number of bitvectors involved) is less
///   than the size of an uncompressed bitvector, use a priority queue to
///   store all input bitvectors and intermediate solutions,
/// - or else, decompress the first bitvector and use inplace bitwise OR
///   operator to complete the operations.
void ibis::index::sumBins(uint32_t ib, uint32_t ie,
			  ibis::bitvector& res) const {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index[" << col->name() << "]::sumBins(" << ib << ", "
	<< ie << ", res(" << res.cnt() << ", " << res.size() << ")) ...";
    const uint32_t nobs = bits.size();
    if (ie > nobs) ie = nobs;
    if (ib >= ie) {
	res.set(0, nrows); // no bitmap in the range
	return;
    }

    typedef std::pair<ibis::bitvector*, bool> _elem;
    bool straight = true;
    if (offsets.size() <= nobs) { // all bitvectors must be in memory
	try {
	    offsets.resize(nobs+1);
	    offsets[0] = 0;
	    for (uint32_t i = 0; i < nobs; ++ i)
		offsets[i+1] = offsets[i] +
		    (bits[i] ? bits[i]->bytes() : 0U);
	}
	catch (...) {
	    offsets.clear();
	}
    }
    if (offsets.size() > nobs) {
	const uint32_t all = offsets[nobs] - offsets[0];
	const uint32_t mid = offsets[ie] - offsets[ib];
	if (mid == 0U) {
	    res.set(0, nrows);
	    return;
	}
	else if (all == mid) {
	    res.set(1, nrows);
	    return;
	}
	straight = (mid <= (all >> 1));
    }
    else {
	straight = (ie-ib <= (nobs >> 1));
    }

    if (str || fname) { // try to activate the needed bitmaps
	if (straight) {
	    activate(ib, ie);
	}
	else {
	    activate(0, ib);
	    activate(ie, nobs);
	}
    }
    const uint32_t na = (straight ? ie-ib : nobs + ib - ie);
    if (na <= 2) { // some special cases
	if (ib >= ie) { // no bitmap in the range
	    res.set(0, nrows);
	}
	else if (ib == 0 && ie == nobs) { // every bitmap in the range
	    res.set(1, nrows);
	}
	else if (na == 1) {
	    if (straight) { // only one bitmap in the range
		if (bits[ib]) {
		    res.copy(*(bits[ib]));
		}
		else
		    res.set(0, nrows);
	    }
	    else if (ib == 0) { // last one is outside
		if (bits[ie]) {
		    res.copy(*(bits[ie]));
		    res.flip();
		}
		else {
		    res.set(1, nrows);
		}
	    }
	    else { // the first one is outside
		if (bits[0]) {
		    res.copy(*(bits[0]));
		    res.flip();
		}
		else {
		    res.set(1, nrows);
		}
	    }
	}
	else if (straight) { // two consecutive bitmaps in the range
	    if (bits[ib]) {
		ibis::bitvector tmp(*(bits[ib]));
		res.swap(tmp);
		if (bits[ib+1])
		    res |= *(bits[ib+1]);
	    }
	    else if (bits[ib+1]) {
		res.copy(*(bits[ib+1]));
	    }
	    else {
		res.set(0, nrows);
	    }
	}
	else if (ib == 0) { // two bitmaps at the end are outside
	    if (bits[ie]) {
		res.copy(*(bits[ie]));
		if (bits[nobs-1])
		    res |= *(bits[nobs-1]);
		res.flip();
	    }
	    else if (bits[nobs-1]) {
		res.copy(*(bits[nobs-1]));
		res.flip();
	    }
	    else {
		res.set(1, nrows);
	    }
	}
	else if (ib == 1) { // two outside bitmaps are split
	    if (bits[0])
		res.copy(*(bits[0]));
	    else
		res.set(0, nrows);
	    if (bits[ie])
		res |= *(bits[ie]);
	    res.flip();
	}
	else if (ib == 2) { // two outside bitmaps are at the beginning
	    if (bits[0])
		res.copy(*(bits[0]));
	    else
		res.set(0, nrows);
	    if (bits[1])
		res |= *(bits[1]);
	    res.flip();
	}
	return;
    }

    horometer timer;
    uint32_t bytes = 0;
    // based on extensive testing, we have settled on the following
    // combination
    // (1) if the two size of the first two are greater than the
    // uncompressed size of one bitmap, use option 1.  Because the
    // operation of the first two will produce an uncompressed result,
    // it will sum together all other bits with to the uncompressed
    // result generated already.
    // (2) if total size (bytes) times log 2 of number of bitmaps
    // is less than or equal to the size of an uncompressed
    // bitmap, use option 3, else use option 4.
    if (ibis::gVerbose > 4) {
	ibis::util::logMessage("index", "sumBins(%lu, %lu) will operate on "
			       "%lu out of %lu bitmaps using the combined "
			       "option", static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       static_cast<long unsigned>(na),
			       static_cast<long unsigned>(nobs));
	timer.start();
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i) {
		if (bits[i])
		    bytes += bits[i]->bytes();
	    }
	}
	else {
	    for (uint32_t i = 0; i < ib; ++i) {
		if (bits[i])
		    bytes += bits[i]->bytes();
	    }
	    for (uint32_t i = ie; i < nobs; ++i) {
		if (bits[i])
		    bytes += bits[i]->bytes();
	    }
	}
    }
    const uint32_t uncomp = (ibis::bitvector::bitsPerLiteral() == 8 ?
			     nrows * 2 / 15 : nrows * 4 / 31);
    if (straight) {
	uint32_t sum2 = (bits[ib] ? bits[ib]->bytes() : 0U) +
	    (bits[ib+1] ? bits[ib+1]->bytes() : 0U);
	if (sum2 >= uncomp) {
	    LOGGER(ibis::gVerbose > 5)
		<< "ibis::inex::sumBins(" << ib << ", " << ie
		<< ") performs bitwise OR with a simple loop";
	    if (bits[ib]) {
		res.copy(*(bits[ib]));
		if (bits[ib+1])
		    res |= *(bits[ib+1]);
	    }
	    else if (bits[ib+1]) {
		res.copy(*(bits[ib+1]));
	    }
	    else {
		res.set(0, nrows);
	    }
	    for (uint32_t i = ib+2; i < ie; ++i) {
		if (bits[i])
		    res |= *(bits[i]);
	    }
	}
	else {
	    // need to compute the total size of all bitmaps
	    if (bytes == 0) {
		for (uint32_t i = ib; i < ie; ++i) {
		    if (bits[i])
			bytes += bits[i]->bytes();
		}
	    }
	    if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a priority queue";
		// put all bitmaps in a priority queue
		std::priority_queue<_elem> que;
		_elem op1, op2, tmp;
		tmp.first = 0;

		// populate the priority queue with the original input
		for (uint32_t i = ib; i < ie; ++i) {
		    if (bits[i]) {
			op1.first = bits[i];
			op1.second = false;
			que.push(op1);
		    }
		}

		try {
		    while (! que.empty()) {
			op1 = que.top();
			que.pop();
			if (que.empty()) {
			    res.copy(*(op1.first));
			    if (op1.second) delete op1.first;
			    break;
			}

			op2 = que.top();
			que.pop();
			tmp.second = true;
			tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			LOGGER(ibis::gVerbose >= 0)
			    << "sumBins-using priority queue: "
			    << op1.first->bytes()
			    << (op1.second ? "(transient), " : ", ")
			    << op2.first->bytes()
			    << (op2.second ? "(transient) >> " : " >> ")
			    << tmp.first->bytes();
#endif
			if (op1.second) delete op1.first;
			if (op2.second) delete op2.first;
			if (! que.empty()) {
			    que.push(tmp);
			    tmp.first = 0;
			}
		    }
		    if (tmp.first != 0) {
			if (tmp.second) {
			    res.swap(*(tmp.first));
			    delete tmp.first;
			    tmp.first = 0;
			}
			else {
			    res.copy(*(tmp.first));
			}
		    }
		}
		catch (...) { // need to free the pointers
		    delete tmp.first;
		    while (! que.empty()) {
			tmp = que.top();
			if (tmp.second)
			    delete tmp.first;
			que.pop();
		    }
		    throw;
		}
	    }
	    else if (sum2 <= (uncomp >> 2)) {
		// use uncompressed res
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a decompressed result";
		if (bits[ib]) {
		    res.copy(*(bits[ib]));
		    if (bits[ib+1])
			res |= *(bits[ib+1]);
		}
		else if (bits[ib+1]) {
		    res.copy(*(bits[ib+1]));
		}
		else {
		    res.set(0, nrows);
		}
		res.decompress();
		for (uint32_t i = ib+2; i < ie; ++i) {
		    if (bits[i])
			res |= *(bits[i]);
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a simple loop";
		uint32_t i = ib;
		for (; bits[i] == 0 && i < ie; ++ i); // skip leading nulls
		if (i < ie) {
		    res.copy(*(bits[i]));
		    res.decompress();
		    for (++i; i < ie; ++i) {
			if (bits[i])
			    res |= *(bits[i]);
		    }
		}
		else {
		    res.set(0, nrows);
		}
	    }
	}
    } // if (straight)
    else { // use complements
	uint32_t sum2;
	if (ib > 1) {
	    sum2 = (bits[0] ? bits[0]->bytes() : 0U) +
		(bits[1] ? bits[1]->bytes() : 0U);
	}
	else if (ib == 1) {
	    sum2 = (bits[0] ? bits[0]->bytes() : 0U) +
		(bits[ie] ? bits[ie]->bytes() : 0U);
	}
	else {
	    sum2 = (bits[ie] ? bits[ie]->bytes() : 0U) +
		(bits[ie+1] ? bits[ie+1]->bytes() : 0U);
	}
	if (sum2 >= uncomp) { // take advantage of automate decopression
	    LOGGER(ibis::gVerbose > 5)
		<< "ibis::inex::sumBins(" << ib << ", " << ie
		<< ") performs bitwise OR with a simple loop (complement)";
	    if (ib > 1) {
		if (bits[0])
		    res.copy(*(bits[0]));
		else
		    res.set(0, nrows);
		for (uint32_t i = 1; i < ib; ++i)
		    if (bits[i])
			res |= *(bits[i]);
	    }
	    else if (ib == 1) {
		if (bits[0])
		    res.copy(*(bits[0]));
		else
		    res.set(0, nrows);
	    }
	    else {
		for (; bits[ie] == 0 && ie < nobs; ++ ie);
		if (ie < nobs)
		    res.copy(*(bits[ie]));
		else
		    res.set(0, nrows);
	    }
	    for (uint32_t i = ie; i < nobs; ++i) {
		if (bits[i])
		    res |= *(bits[i]);
	    }
	}
	else { // need to look at the total size
	    if (bytes == 0) {
		for (uint32_t i = 0; i < ib; ++i) {
		    if (bits[i])
			bytes += bits[i]->bytes();
		}
		for (uint32_t i = ie; i < nobs; ++i) {
		    if (bits[i])
			bytes += bits[i]->bytes();
		}
	    }
	    if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a priority queue "
		    "(complement)";
		// use priority queue for all bitmaps
		std::priority_queue<_elem> que;
		_elem op1, op2, tmp;
		tmp.first = 0;

		// populate the priority queue with the original input
		for (uint32_t i = 0; i < ib; ++i) {
		    if (bits[i]) {
			op1.first = bits[i];
			op1.second = false;
			que.push(op1);
		    }
		}
		for (uint32_t i = ie; i < nobs; ++i) {
		    if (bits[i]) {
			op1.first = bits[i];
			op1.second = false;
			que.push(op1);
		    }
		}

		try {
		    while (! que.empty()) {
			op1 = que.top();
			que.pop();
			if (que.empty()) {
			    res.copy(*(op1.first));
			    if (op1.second) delete op1.first;
			    break;
			}

			op2 = que.top();
			que.pop();
			tmp.second = true;
			tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			LOGGER(ibis::gVerbose >= 0)
			    << "sumBins-using priority queue: "
			    << op1.first->bytes()
			    << (op1.second ? "(transient), " : ", ")
			    << op2.first->bytes()
			    << (op2.second ? "(transient) >> " : " >> ")
			    << tmp.first->bytes();
#endif
			if (op1.second) delete op1.first;
			if (op2.second) delete op2.first;
			if (! que.empty()) {
			    que.push(tmp);
			    tmp.first = 0;
			}
		    }
		    if (tmp.first != 0) {
			if (tmp.second) {
			    res.swap(*(tmp.first));
			    delete tmp.first;
			    tmp.first = 0;
			}
			else {
			    res.copy(*(tmp.first));
			}
		    }
		}
		catch (...) { // need to free the pointers
		    delete tmp.first;
		    while (! que.empty()) {
			tmp = que.top();
			if (tmp.second)
			    delete tmp.first;
			que.pop();
		    }
		    throw;
		}
	    }
	    else if (sum2 <= (uncomp >> 2)){
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a decompressed result "
		    "(complement)";
		// uncompress the first bitmap generated
		if (ib > 1) {
		    if (bits[0]) {
			res.copy(*(bits[0]));
			if (bits[1])
			    res |= *(bits[1]);
		    }
		    else if (bits[1]) {
			res.copy(*(bits[1]));
		    }
		    if (res.size() != nrows)
			res.set(0, nrows);
		    res.decompress();
		    for (uint32_t i = 2; i < ib; ++i)
			if (bits[i])
			    res |= *(bits[i]);
		}
		else if (ib == 1) {
		    if (bits[0])
			res.copy(*(bits[0]));
		    else
			res.set(0, nrows);
		    res.decompress();
		}
		else {
		    for (; bits[ie] == 0 && ie < nobs; ++ ie);
		    if (ie < nobs) {
			res.copy(*(bits[ie]));
			++ ie;
			if (ie < nobs)
			    res.decompress();
		    }
		    else {
			res.set(0, nrows);
		    }
		}
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bits[i])
			res |= *(bits[i]);
	    }
	    else if (ib > 0) {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a decompressed result "
		    "(complement)";
		if (bits[0])
		    res.copy(*(bits[0]));
		else
		    res.set(0, nrows);
		res.decompress();
		for (uint32_t i = 1; i < ib; ++i)
		    if (bits[i])
			res |= *(bits[i]);
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bits[i])
			res |= *(bits[i]);
	    }
	    else {
		LOGGER(ibis::gVerbose > 5)
		    << "ibis::inex::sumBins(" << ib << ", " << ie
		    << ") performs bitwise OR with a decompressed result "
		    "(complement)";
		for (; bits[ie] == 0 && ie < nobs; ++ ie);
		if (ie < nobs) {
		    res.copy(*(bits[ie]));
		    ++ ie;
		    if (ie < nobs)
			res.decompress();
		}
		else {
		    res.set(0, nrows);
		}
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bits[i])
			res |= *(bits[i]);
	    }
	}
	res.flip(); // need to flip because we have been using complement
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("index", "sumBins operated on %u bitmap%s "
			       "(%lu in %lu out) took %g sec(CPU), "
			       "%g sec(elapsed).",
			       static_cast<unsigned>(na), (na>1?"s":""),
			       static_cast<long unsigned>(bytes),
			       static_cast<long unsigned>(res.bytes()),
			       timer.CPUTime(), timer.realTime());
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "sumBins(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::sumBins

/// This function attempts to take advantage of existing results of a
/// previously computed sum.
/// - On input, res = sum_{i=ib0}^{ie0} bits[i].
/// - On exit, res = sum_{i=ib}^{ie} bits[i].
void ibis::index::sumBins(uint32_t ib, uint32_t ie, ibis::bitvector& res,
			  uint32_t ib0, uint32_t ie0) const {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index[" << col->name() << "]::sumBins(" << ib
	<< ", " << ie << ", res(" << res.cnt() << ", " << res.size()
	<< "), " << ib0 << ", " << ie0 << ") ...";
    if (ie > bits.size())
	ie = bits.size();
    if (ib0 > ie || ie0 < ib || ib0 >= ie0 ||
	res.size() != nrows) {	// no overlap
	sumBins(ib, ie, res);
    }
    else { // [ib, ie] overlaps [ib0, ie0]
	const uint32_t ib1 = (ib0 >= ib ? ib0 : ib);
	const uint32_t ie1 = (ie0 <= ie ? ie0 : ie);
	bool local; // do the operations here (true) or call sumBins
	if (offsets.size() > bits.size()) {
	    uint32_t change = (offsets[ib1] - offsets[ib0>=ib ? ib : ib0])
		+ (offsets[ie0 <= ie ? ie : ie0] - offsets[ie1]);
	    uint32_t direct = offsets[ie] - offsets[ib];
	    local = (change <= direct);
	}
	else {
	    local = ((ib0 >= ib ? ib0 - ib : ib - ib0) +
		     (ie0 <= ie ? ie - ie0 : ie0 - ie) < ie - ib);
	}

	if (local) { // evaluate new sum here
	    if (ib0 < ib) { // take away bits[ib0:ib]
		activate(ib0, ib);
		for (uint32_t i = ib0; i < ib; ++ i)
		    if (bits[i])
			res -= *(bits[i]);
	    }
	    else if (ib0 > ib) { // add bits[ib:ib0]
		activate(ib, ib0);
		for (uint32_t i = ib; i < ib0; ++ i) {
		    if (bits[i])
			res |= *(bits[i]);
		}
	    }
	    if (ie0 > ie) { // subtract bits[ie:ie0]
		activate(ie, ie0);
		for (uint32_t i = ie; i < ie0; ++ i)
		    if (bits[i])
			res -= *(bits[i]);
	    }
	    else if (ie0 < ie) { // add bits[ie0:ie]
		activate(ie0, ie);
		for (uint32_t i = ie0; i < ie; ++ i) {
		    if (bits[i])
			res |= *(bits[i]);
		}
	    }
	}
	else { // evalute the new sum directly
	    sumBins(ib, ie, res);
	}
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "sumBins(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::sumBins

/// Add the @c bts[ib:ie-1] to @c res.  Since the set of bit vectors are
/// explicitly given, there is no need to perform activation.  To minimize
/// the burden of deciding which bit vectors to activate, this function
/// always use the @c bts[ib] through @c bts[ie-1].
/// @note  The caller need to activate the bit vectors!
/// @note  This function still has to check whether a particular bts[i] is
/// a null pointer before using the bit vector.
void ibis::index::addBits(const std::vector<ibis::bitvector*>& bts,
			  uint32_t ib, uint32_t ie, ibis::bitvector& res) {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index::addBits(" << bts.size()
	<< "-bitvector set, " << ib << ", " << ie << ", res("
	<< res.cnt() << ", " << res.size() << ")) ...";
    const uint32_t nobs = bts.size();
    if (res.cnt() >= res.size()) return; // useless to add more bit
    while (ib < nobs && bts[ib] == 0) // skip the leading nulls
	++ ib;
    if (ie > nobs)
	ie = nobs;
    if (ib >= ie || ib >= nobs) {
	return;
    }
    else if (ib == 0 && ie == nobs) {
	res.set(1, res.size());
	return;
    }

    horometer timer;
    bool decmp = false;
    if (ibis::gVerbose > 4)
	timer.start();
    if (res.size() != bts[ib]->size()) {
	res.copy(*(bts[ib]));
	++ ib;
    }

    // first determine whether to decompres the result
    if (ie-ib>64) {
	decmp = true;
    }
    else if (ie - ib > 3) {
	uint32_t tot = 0;
	for (uint32_t i = ib; i < ie; ++i)
	    if (bts[i])
		tot += bts[i]->bytes();
	if (tot > (res.size() >> 2))
	    decmp = true;
	else if (tot > (res.size() >> 3) && ie-ib > 4)
	    decmp = true;
    }
    if (decmp) { // use decompressed res
	if (ibis::gVerbose > 5)
	    ibis::util::logMessage("index", "addBits(%lu, %lu) using "
				   "uncompressed bitvector",
				   static_cast<long unsigned>(ib),
				   static_cast<long unsigned>(ie));
	res.decompress(); // decompress res
	for (uint32_t i = ib; i < ie; ++i)
	    res |= *(bts[i]);
	res.compress();
    }
    else if (ie > ib + 2) { // use compressed res
	typedef std::pair<ibis::bitvector*, bool> _elem;
	std::priority_queue<_elem> que;
	_elem op1, op2, tmp;
	tmp.first = 0;
	if (ibis::gVerbose > 5) 
	    ibis::util::logMessage("index", "addBits(%lu, %lu) using "
				   "compressed bitvector (with a priority "
				   "queue)", static_cast<long unsigned>(ib),
				   static_cast<long unsigned>(ie));

	// populate the priority queue with the original input
	for (uint32_t i = ib; i < ie; ++i) {
	    if (bts[i]) {
		op1.first = bts[i];
		op1.second = false;
		que.push(op1);
	    }
	}

	try {
	    while (! que.empty()) {
		op1 = que.top();
		que.pop();
		if (que.empty()) {
		    res.copy(*(op1.first));
		    if (op1.second) delete op1.first;
		    break;
		}

		op2 = que.top();
		que.pop();
		tmp.second = true;
		tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
		LOGGER(ibis::gVerbose >= 0)
		    << "addBits-using priority queue: "
		    << op1.first->bytes()
		    << (op1.second ? "(transient), " : ", ")
		    << op2.first->bytes()
		    << (op2.second ? "(transient) >> " : " >> ")
		    << tmp.first->bytes();
#endif
		if (op1.second)
		    delete op1.first;
		if (op2.second)
		    delete op2.first;
		if (! que.empty()) {
		    que.push(tmp);
		    tmp.first = 0;
		}
	    }
	    if (tmp.first != 0) {
		if (tmp.second) {
		    res |= *(tmp.first);
		    delete tmp.first;
		    tmp.first = 0;
		}
		else {
		    res |= *(tmp.first);
		}
	    }
	}
	catch (...) { // need to free the pointers
	    delete tmp.first;
	    while (! que.empty()) {
		tmp = que.top();
		if (tmp.second)
		    delete tmp.first;
		que.pop();
	    }
	    throw;
	}
    }
    else if (ie > ib + 1) {
	if (bts[ib])
	    res |= *(bts[ib]);
	if (bts[ib+1])
	    res |= *(bts[ib+1]);
    }
    else if (bts[ib]) {
	res |= *(bts[ib]);
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("index", "addBits(%lu, %lu) took %g sec(CPU), "
			       "%g sec(elapsed).",
			       static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       timer.CPUTime(), timer.realTime());
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "addBits(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::addBits

/// Sum up @c bts[ib:ie-1] and place the result in @c res.
/// @note This function may either use bts[ib:ie-1] or bts[0:ib-1] and
/// bts[ie:nobs-1] depending which set has more bit vectors!  This
/// requires the caller to activate the appropriate set.
/// @note This function always uses the operator |=.
/// Tests show that using the function @c setBit is always slower.
void ibis::index::sumBits(const std::vector<ibis::bitvector*>& bts,
			  uint32_t ib, uint32_t ie, ibis::bitvector& res) {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index::sumBits(" << bts.size()
	<< "-bitvector set, " << ib << ", " << ie << ", res("
	<< res.cnt() << ", " << res.size() << ")) ...";
    typedef std::pair<ibis::bitvector*, bool> _elem;
    const uint32_t nobs = bts.size();
    if (ie > nobs) ie = nobs;
    const bool straight = (2*(ie-ib) <= nobs);
    const uint32_t na = (straight ? ie-ib : nobs + ib - ie);
    // need to figure out the size of bit vectors
    uint32_t sz = 0;
    for (unsigned i = 0; i < nobs && sz == 0; ++i)
	if (bts[i] != 0)
	    sz = bts[i]->size();

    if (ib >= ie) {
	res.set(0, sz); // no bitmap in the range
	return;
    }
    else if (na <= 2) { // some special cases
	if (ib == 0 && ie == nobs) { // every bitmap in the range
	    res.set(1, bts[0]->size());
	}
	else if (na == 1) {
	    if (straight) { // only one bitmap in the range
		if (bts[ib]) {
		    res.copy(*(bts[ib]));
		}
		else {
		    res.set(0, sz);
		}
	    }
	    else if (ib == 0) { // last one is outside
		if (bts[ie]) {
		    res.copy(*(bts[ie]));
		    res.flip();
		}
		else {
		    res.set(1, sz);
		}
	    }
	    else { // the first one is outside
		if (bts[0] != 0) {
		    res.copy(*(bts[0]));
		    res.flip();
		}
		else {
		    res.set(1, sz);
		}
	    }
	}
	else if (straight) { // two consecutive bitmaps in the range
	    if (bts[ib]) {
		res.copy(*(bts[ib]));
		if (bts[ib+1])
		    res |= *(bts[ib+1]);
	    }
	    else if (bts[ib+1]) {
		res.copy(*(bts[ib+1]));
	    }
	    else {
		res.set(0, sz);
	    }
	}
	else if (ib == 0) { // two bitmaps at the end are outside
	    if (bts[ie]) {
		res.copy(*(bts[ie]));
		if (bts[nobs-1])
		    res |= *(bts[nobs-1]);
		res.flip();
	    }
	    else if (bts[nobs-1]) {
		res.copy(*(bts[nobs-1]));
		res.flip();
	    }
	    else {
		res.set(1, sz);
	    }
	}
	else if (ib == 1) { // two outside bitmaps are split
	    res.copy(*(bts[0]));
	    if (bts[ie])
		res |= *(bts[ie]);
	    res.flip();
	}
	else if (ib == 2) { // two outside bitmaps are at the beginning
	    res.copy(*(bts[0]));
	    if (bts[1])
		res |= *(bts[1]);
	    res.flip();
	}
	return;
    }

    horometer timer;
    uint32_t bytes = 0;

#if defined(TEST_SUMBINS_OPTIONS)
    if (ibis::gVerbose > 4 || ibis::_sumBits_option != 0) {
	ibis::util::logMessage("index", "sumBits(%lu, %lu) will operate on "
			       "%lu out of %lu bitmaps using option %d",
			       static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       static_cast<long unsigned>(na),
			       static_cast<long unsigned>(nobs),
			       ibis::_sumBits_option);
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i)
		bytes += bts[i]->bytes();
	}
	else {
	    for (uint32_t i = 0; i < ib; ++i)
		bytes += bts[i]->bytes();
	    for (uint32_t i = ie; i < nobs; ++i)
		bytes += bts[i]->bytes();
	}
	timer.start();
    }

    switch (ibis::_sumBits_option) {
    case 1: // compressed or in natural order
	if (2*(ie-ib) <= nobs) {
	    res.copy(*(bts[ib]));
	    for (uint32_t i = ib+1; i < ie; ++i)
		res |= *(bts[i]);
	}
	else { // use complement
	    if (ib > 0) {
		res.copy(*(bts[0]));
		for (uint32_t i = 1; i < ib; ++i)
		    res |= *(bts[i]);
	    }
	    else {
		res.copy(*(bts[ie]));
		++ ie;
	    }
	    for (uint32_t i = ie; i < nobs; ++i)
		res |= *(bts[i]);
	    res.flip();
	}
	break;
    case 2: {// compressed or, sort input bitmap according to size
	std::vector<ibis::bitvector*> ind;
	ind.reserve(na);
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i)
		ind.push_back(bts[i]);
	}
	else { // use complement
	    for (uint32_t i = 0; i < ib; ++i)
		ind.push_back(bts[i]);
	    for (uint32_t i = ie; i < nobs; ++i)
		ind.push_back(bts[i]);
	}
	// sort ind according the size of the bitvectors
	// make use the specialized version of std::less
	std::less<ibis::bitvector*> cmp;
	std::sort(ind.begin(), ind.end(), cmp);
	// evaluate according the order ind
	res.copy(*(ind[0]));
	for (uint32_t i = 1; i < na; ++i) {
	    res |= *(ind[i]);
#if defined(DEBUG)
	    LOGGER(ibis::gVerbose >= 0)
		<< "sumBits-option 2: " << i << ", " << ind[i]->bytes();
#endif
	}
	if (! straight)
	    res.flip();
	break;
    }
    case 3: {// compressed or, put all bitmaps on a priority queue
	std::priority_queue<_elem> que;
	_elem op1, op2, tmp;
	tmp.first = 0;

	// populate the priority queue with the original input
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i) {
		op1.first = bts[i];
		op1.second = false;
		que.push(op1);
	    }
	}
	else { // use complement
	    for (uint32_t i = 0; i < ib; ++i) {
		op1.first = bts[i];
		op1.second = false;
		que.push(op1);
	    }
	    for (uint32_t i = ie; i < nobs; ++i) {
		op1.first = bts[i];
		op1.second = false;
		que.push(op1);
	    }
	}

	try {
	    while (! que.empty()) {
		op1 = que.top();
		que.pop();
		if (que.empty()) {
		    res.copy(*(op1.first));
		    if (op1.second) delete op1.first;
		    break;
		}

		op2 = que.top();
		que.pop();
		tmp.second = true;
		tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
		LOGGER(ibis::gVerbose >= 0)
		    << "sumBits-option 3: " << op1.first->bytes()
		    << (op1.second ? "(transient), " : ", ")
		    << op2.first->bytes()
		    << (op2.second ? "(transient) >> " : " >> ")
		    << tmp.first->bytes();
#endif
		if (op1.second) delete op1.first;
		if (op2.second) delete op2.first;
		if (! que.empty()) {
		    que.push(tmp);
		    tmp.first = 0;
		}
	    }
	    if (tmp.first != 0) {
		if (tmp.second) {
		    res.swap(*(tmp.first));
		    delete tmp.first;
		    tmp.first = 0;
		}
		else {
		    res.copy(*(tmp.first));
		}
	    }
	}
	catch (...) { // need to free the pointers
	    delete tmp.first;
	    while (! que.empty()) {
		tmp = que.top();
		if (tmp.second)
		    delete tmp.first;
		que.pop();
	    }
	    throw;
	}

	if (! straight)
	    res.flip();
	break;
    }
    case 4: {// uncompressed res, start with either from or end
	if (straight) {
	    if (bts[ib]->bytes() >= bts[ie-1]->bytes()) {
		res.copy(*(bts[ib]));
		++ ib;
	    }
	    else {
		-- ie;
		res.copy(*(bts[ie]));
	    }
	    res.decompress();
	    for (uint32_t i = ib; i < ie; ++i)
		res |= *(bts[i]);
	    res.compress();
	}
	else if (ib > 0) {
	    if (bts[0]->bytes() >= bts[ib-1]->bytes()) {
		res.copy(*(bts[0]));
		res.decompress();
		for (uint32_t i = 1; i < ib; ++i)
		    res |= *(bts[i]);
	    }
	    else {
		-- ib;
		res.copy(*(bts[ib]));
		res.decompress();
		for (uint32_t i = 0; i < ib; ++i)
		    res |= *(bts[i]);
	    }
	    for (uint32_t i = ie; i < nobs; ++i)
		res |= *(bts[i]);
	    res.compress();
	    res.flip();
	}
	else if (bts[ie]->bytes() >= bts[nobs-1]->bytes()) {
	    res.copy(*(bts[ie]));
	    res.decompress();
	    for (uint32_t i = ie+1; i < nobs; ++i)
		res |= *(bts[i]);
	    res.compress();
	    res.flip();
	}
	else {
	    res.copy(*(bts[nobs-1]));
	    res.decompress();
	    for (uint32_t i = ie; i < nobs-1; ++i)
		res |= *(bts[i]);
	    res.compress();
	    res.flip();
	}
	break;
    }
    case 5: {// uncompressed res, start with the heaviest bitmap
	std::vector<uint32_t> ind;
	ind.reserve(na);
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i)
		ind.push_back(i);
	}
	else { // use complement
	    for (uint32_t i = 0; i < ib; ++i)
		ind.push_back(i);
	    for (uint32_t i = ie; i < nobs; ++i)
		ind.push_back(i);
	}
	uint32_t j = 0;
	for (uint32_t i = 1; i < na; ++i) {
	    if (bts[ind[i]]->bytes() > bts[ind[j]]->bytes())
		j = i;
	}
	res.copy(*(bts[ind[j]]));
	res.decompress();
	ind[j] = ind[0];
	for (uint32_t i = 1; i < na; ++i)
	    res |= *(bts[ind[i]]);
	res.compress();
	if (! straight)
	    res.flip();
	break;
    }
    case 6: {
	// based on the timing results of 1 - 5, here is the rule for
	// choosing which scheme to use
	// (1) if the two size of the first two are greater than the
	// uncompressed size of one bitmap, use option 1.  Because the
	// operation of the first two will produce an uncompressed result,
	// it will sum together all other bts with to the uncompressed
	// result generated already.
	// (2) if total size (bytes) times square root of number of bitmaps
	// is less than or equal to twice the size of an uncompressed
	// bitmap, use option 3, else use option 4.
	const uint32_t uncomp = (ibis::bitvector::btsPerLiteral() == 8 ?
				 sz * 2 / 15 :
				 sz * 4 / 31);
	if (straight) {
	    uint32_t sum2 = bts[ib]->bytes() + bts[ib+1]->bytes();
	    if (sum2 >= uncomp) {
		ibis::bitvector *tmp;
		tmp = *(bts[ib]) | *(bts[ib+1]);
		res.swap(*tmp);
		delete tmp;
		for (uint32_t i = ib+2; i < ie; ++i) {
		    res |= *(bts[i]);
		}
	    }
	    else {
		// need to compute the total size of all bitmaps
		if (bytes == 0) {
		    for (uint32_t i = ib; i < ie; ++i)
			bytes += bts[i]->bytes();
		}
		if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		    // put all bitmaps in a priority queue
		    std::priority_queue<_elem> que;
		    _elem op1, op2, tmp;
		    tmp.first = 0;

		    // populate the priority queue with the original input
		    for (uint32_t i = ib; i < ie; ++i) {
			op1.first = bts[i];
			op1.second = false;
			que.push(op1);
		    }

		    try {
			while (! que.empty()) {
			    op1 = que.top();
			    que.pop();
			    if (que.empty()) {
				res.copy(*(op1.first));
				if (op1.second) delete op1.first;
				break;
			    }

			    op2 = que.top();
			    que.pop();
			    tmp.second = true;
			    tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			    LOGGER(ibis::gVerbose >= 0)
				<< "sumBits-using priority queue: "
				<< op1.first->bytes()
				<< (op1.second ? "(transient), " : ", ")
				<< op2.first->bytes()
				<< (op2.second ? "(transient) >> " : " >> ")
				<< tmp.first->bytes();
#endif
			    if (op1.second) delete op1.first;
			    if (op2.second) delete op2.first;
			    if (! que.empty()) {
				que.push(tmp);
				tmp.first = 0;
			    }
			}
			if (tmp.first != 0) {
			    if (tmp.second) {
				res.swap(*(tmp.first));
				delete tmp.first;
			    }
			    else {
				res.copy(*(tmp.first));
			    }
			}
		    }
		    catch (...) { // need to free the pointers
			delete tmp.first;
			while (! que.empty()) {
			    tmp = que.top();
			    if (tmp.second)
				delete tmp.first;
			    que.pop();
			}
			throw;
		    }
		}
		else if (sum2 <= (uncomp >> 2)) {
		    // use uncompressed res
		    ibis::bitvector *tmp;
		    tmp = *(bts[ib]) | *(bts[ib+1]);
		    res.swap(*tmp);
		    delete tmp;
		    res.decompress();
		    for (uint32_t i = ib+2; i < ie; ++i)
			res |= *(bts[i]);
		}
		else {
		    res.copy(*(bts[ib]));
		    res.decompress();
		    for (uint32_t i = ib + 1; i < ie; ++i)
			res |= *(bts[i]);
		}
	    }
	} // if (straight)
	else { // use complements
	    uint32_t sum2;
	    if (ib > 1) {
		sum2 = bts[0]->bytes() + bts[1]->bytes();
	    }
	    else if (ib == 1) {
		sum2 = bts[0]->bytes() + bts[ie]->bytes();
	    }
	    else {
		sum2 = bts[ie]->bytes() + bts[ie+1]->bytes();
	    }
	    if (sum2 >= uncomp) { // take advantage of automate decopression
		if (ib > 1) {
		    ibis::bitvector *tmp;
		    tmp = *(bts[0]) | *(bts[1]);
		    res.swap(*tmp);
		    delete tmp;
		    for (uint32_t i = 2; i < ib; ++i)
			res |= *(bts[i]);
		}
		else if (ib == 1) {
		    ibis::bitvector *tmp;
		    tmp = *(bts[0]) | *(bts[ie]);
		    res.swap(*tmp);
		    delete tmp;
		    ++ ie;
		}
		else {
		    ibis::bitvector *tmp;
		    tmp = *(bts[ie]) | *(bts[ie+1]);
		    res.swap(*tmp);
		    delete tmp;
		    ie += 2;
		}
		for (uint32_t i = ie; i < nobs; ++i)
		    res |= *(bts[i]);
	    }
	    else { // need to look at the total size
		if (bytes == 0) {
		    for (uint32_t i = 0; i < ib; ++i)
			bytes += bts[i]->bytes();
		    for (uint32_t i = ie; i < nobs; ++i)
			bytes += bts[i]->bytes();
		}
		if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		    // use priority queue for all bitmaps
		    std::priority_queue<_elem> que;
		    _elem op1, op2, tmp;
		    tmp.first = 0;

		    // populate the priority queue with the original input
		    for (uint32_t i = 0; i < ib; ++i) {
			op1.first = bts[i];
			op1.second = false;
			que.push(op1);
		    }
		    for (uint32_t i = ie; i < nobs; ++i) {
			op1.first = bts[i];
			op1.second = false;
			que.push(op1);
		    }

		    try {
			while (! que.empty()) {
			    op1 = que.top();
			    que.pop();
			    if (que.empty()) {
				res.copy(*(op1.first));
				if (op1.second) delete op1.first;
				break;
			    }

			    op2 = que.top();
			    que.pop();
			    tmp.second = true;
			    tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			    LOGGER(ibis::gVerbose >= 0)
				<< "sumBits-using priority queue: "
				<< op1.first->bytes()
				<< (op1.second ? "(transient), " : ", ")
				<< op2.first->bytes()
				<< (op2.second ? "(transient) >> " : " >> ")
				<< tmp.first->bytes();
#endif
			    if (op1.second) delete op1.first;
			    if (op2.second) delete op2.first;
			    if (! que.empty()) {
				que.push(tmp);
				tmp.first = 0;
			    }
			}
			if (tmp.first != 0) {
			    if (tmp.second) {
				res.swap(*(tmp.first));
				delete tmp.first;
				tmp.first = 0;
			    }
			    else {
				res.copy(*(tmp.first));
			    }
			}
		    }
		    catch (...) { // need to free the pointers
			delete tmp.first;
			while (! que.empty()) {
			    tmp = que.top();
			    if (tmp.second)
				delete tmp.first;
			    que.pop();
			}
			throw;
		    }
		}
		else if (sum2 <= (uncomp >> 2)){
		    // uncompress the first bitmap generated
		    if (ib > 1) {
			ibis::bitvector *tmp;
			tmp = *(bts[0]) | *(bts[1]);
			res.swap(*tmp);
			delete tmp;
			res.decompress();
			for (uint32_t i = 2; i < ib; ++i)
			    res |= *(bts[i]);
		    }
		    else if (ib == 1) {
			ibis::bitvector *tmp;
			tmp = *(bts[0]) | *(bts[ie]);
			res.swap(*tmp);
			delete tmp;
			res.decompress();
			++ ie;
		    }
		    else {
			ibis::bitvector *tmp;
			tmp = *(bts[ie]) | *(bts[ie+1]);
			res.swap(*tmp);
			delete tmp;
			res.decompress();
			ie += 2;
		    }
		    for (uint32_t i = ie; i < nobs; ++i)
			res |= *(bts[i]);
		}
		else if (ib > 0) {
		    if (bts[0]->bytes() >= bts[ib-1]->bytes()) {
			res.copy(*(bts[0]));
			res.decompress();
			for (uint32_t i = 1; i < ib; ++i)
			    res |= *(bts[i]);
		    }
		    else {
			-- ib;
			res.copy(*(bts[ib]));
			res.decompress();
			for (uint32_t i = 0; i < ib; ++i)
			    res |= *(bts[i]);
		    }
		    for (uint32_t i = ie; i < nobs; ++i)
			res |= *(bts[i]);
		}
		else if (bts[ie]->bytes() >= bts[nobs-1]->bytes()) {
		    res.copy(*(bts[ie]));
		    res.decompress();
		    for (uint32_t i = ie+1; i < nobs; ++i)
			res |= *(bts[i]);
		}
		else {
		    res.copy(*(bts[nobs-1]));
		    res.decompress();
		    for (uint32_t i = ie; i < nobs-1; ++i)
			res |= *(bts[i]);
		}
	    }
	    res.flip(); // need to flip because we have been using complement
	}
	break;
    }
    default:
	if (straight) { // sum less than half of the bitvectors
	    bool decmp = false;
	    // first determine whether to decompres the result
	    if (ie-ib>64) {
		decmp = true;
	    }
	    else if (ie - ib > 3) {
		uint32_t tot = 0;
		for (uint32_t i = ib; i < ie; ++i)
		    tot += bts[i]->bytes();
		if (tot > (sz >> 2))
		    decmp = true;
		else if (tot > (sz >> 3) && ie-ib > 4)
		    decmp = true;
	    }
	    if (decmp) { // use decompressed res
		if (ibis::gVerbose > 5) {
		    double sb = 0;
		    for (uint32_t i = ib; i < ie; ++i)
			sb += bts[i]->bytes();
		    ibis::util::logMessage("index", "sumBits(%lu, %lu) using "
					   "uncompressed bitvector, total "
					   "input bitmap size is %lG bytes",
					   static_cast<long unsigned>(ib),
					   static_cast<long unsigned>(ie), sb);
		}
		res.copy(*(bts[ib]));
		res.decompress();
		for (uint32_t i = ib+1; i < ie; ++i)
		    res |= *(bts[i]);
	    }
	    else if (ie > ib + 2) { // use compressed res
		if (ibis::gVerbose > 5) {
		    double sb = 0;
		    for (uint32_t i = ib; i < ie; ++i)
			sb += bts[i]->bytes();
		    ibis::util::logMessage("index", "sumBits(%lu, %lu) using "
					   "compressed bitvector, total "
					   "input bitmap size is %lG bytes",
					   static_cast<long unsigned>(ib),
					   static_cast<long unsigned>(ie), sb);
		}
		// first determine an good evaluation order (ind)
		std::vector<uint32_t> ind;
		uint32_t i, j, k;
		ind.reserve(ie-ib);
		for (i = ib; i < ie; ++i)
		    ind.push_back(i);
		// sort ind according the size of bitvectors (insertion sort)
		for (i = 0; i < ie-ib-1; ++i) {
		    k = i + 1;
		    for (j = k+1; j < ie-ib; ++j)
			if (bts[ind[j]]->bytes() < bts[ind[k]]->bytes())
			    k = j;
		    if (bts[ind[i]]->bytes() > bts[ind[k]]->bytes()) {
			j = ind[i];
			ind[i] = ind[k];
			ind[k] = j;
		    }
		    else {
			++ i;
			if (bts[ind[i]]->bytes() > bts[ind[k]]->bytes()) {
			    j = ind[i];
			    ind[i] = ind[k];
			    ind[k] = j;
			}
		    }
		}
		// evaluate according the order ind
		res.copy(*(bts[ind[0]]));
		for (i = 1; i < ie-ib; ++i)
		    res |= *(bts[ind[i]]);
	    }
	    else if (ie > ib + 1) {
		if (ibis::gVerbose > 5) {
		    double sb = bts[ib]->bytes();
		    sb += bts[ib+1]->bytes();
		    ibis::util::logMessage("index", "sumBits(%lu, %lu) using "
					   "compressed bitvector, total "
					   "input bitmap size is %lG bytes",
					   static_cast<long unsigned>(ib),
					   static_cast<long unsigned>(ie), sb);
		}
		res.copy(*(bts[ib]));
		res |= *(bts[ib+1]);
	    }
	    else {
		res.copy(*(bts[ib]));
	    }
	}
	else if (nobs - ie + ib > 64) { // use uncompressed res
	    if (ibis::gVerbose > 5) {
		double sb = 0;
		for (uint32_t i = 0; i < ib; ++i)
		    sb += bts[i]->bytes();
		for (uint32_t i = ie; i < nobs; ++i)
		    sb += bts[i]->bytes();
		ibis::util::logMessage("index", "sumBits(%lu, %lu) using "
				       "uncompressed bitvecector, total "
				       "input bitmap size is %lG bytes",
				       static_cast<long unsigned>(ib),
				       static_cast<long unsigned>(ie), sb);
	    }
	    if (ib > 0) {
		-- ib;
		res.copy(*(bts[ib]));
	    }
	    else {
		res.copy(*(bts[ie]));
		++ ie;
	    }
	    res.decompress();
	    for (uint32_t i = 0; i < ib; ++i)
		res |= *(bts[i]);
	    for (uint32_t i = ie; i < nobs; ++i)
		res |= *(bts[i]);
	    res.compress();
	    res.flip();
	}
	else { // need to check the sizes of bitvectors to be added
	    std::vector<uint32_t> ind;
	    bool decmp = false;
	    for (uint32_t i=0; i < ib; ++i)
		ind.push_back(i);
	    for (uint32_t i=ie; i < nobs; ++i)
		ind.push_back(i);
	    if (ind.size() > 64) {
		decmp = true;
	    }
	    else if (ind.size() > 3) {
		uint32_t tot = 0;
		for (uint32_t i = 0; i < ind.size(); ++i)
		    tot += bts[ind[i]]->bytes();
		if (tot > (sz >> 2))
		    decmp = true;
		else if (tot > (sz >> 3) && ind.size() > 8)
		    decmp = true;
	    }
	    if (decmp) {
		if (ibis::gVerbose > 5) {
		    double sb = 0;
		    uint32_t j = 0;
		    uint32_t large=0, tmp;
		    for (uint32_t i = 0; i < ind.size(); ++i) {
			tmp = bts[ind[i]]->bytes();
			if (tmp > large) {
			    large = tmp;
			    j = i;
			}
			sb += tmp;
		    }
		    if (j != 0) {
			uint32_t k = ind[0];
			ind[0] = ind[j];
			ind[j] = k;
		    }
		    ibis::util::logMessage("index", "sumBits(%lu, %lu) using "
					   "uncompressed bitvecector, total "
					   "input bitmap size is %lG bytes",
					   static_cast<long unsigned>(ib),
					   static_cast<long unsigned>(ie), sb);
		}
		res.copy(*(bts[ind[0]]));
		res.decompress();
		for (uint32_t i = 1; i < ind.size(); ++i)
		    res |= *(bts[ind[i]]);
		res.compress();
	    }
	    else {
		const uint32_t nb = ind.size();
		if (ibis::gVerbose > 5) {
		    double sb = 0;
		    for (uint32_t i = 0; i < nb; ++i)
			sb += bts[ind[i]]->bytes();
		    ibis::util::logMessage("index", "sumBits(%lu, %lu) using "
					   "compressed bitvector, total "
					   "input bitmap size is %lG bytes",
					   static_cast<long unsigned>(ib),
					   static_cast<long unsigned>(ie), sb);
		}
		uint32_t i, j, k;
		// sort the ind array (insertion sort)
		for (i = 0; i < nb-1; ++i) {
		    k = i + 1;
		    for (j = k + 1; j < nb; ++j)
			if (bts[ind[j]]->bytes() < bts[ind[k]]->bytes())
			    k = j;
		    if (bts[ind[i]]->bytes() > bts[ind[k]]->bytes()) {
			j = ind[i];
			ind[i] = ind[k];
			ind[k] = j;
		    }
		    else {
			++ i;
			if (bts[ind[i]]->bytes() > bts[ind[k]]->bytes()) {
			    j = ind[i];
			    ind[i] = ind[k];
			    ind[k] = j;
			}
		    }
		}
		// evaluate in the order specified by ind
		res.copy(*(bts[ind[0]]));
		for (i = 1; i < nb; ++i)
		    res |= *(bts[ind[i]]);
	    }
	    res.flip();
	}
	break;
    } // switch (ibis::_sumBits_option)
    if (ibis::gVerbose > 4 || ibis::_sumBits_option != 0) {
	timer.stop();
	ibis::util::logMessage("index", "sumBits operated on %lu bitmap%s "
			       "using option %d (%lu in %lu out) took "
			       "%g sec(CPU), %g sec(elapsed).",
			       static_cast<long unsigned>(na),
			       (na>1?"s":""), ibis::_sumBits_option,
			       bytes, res.bytes(), timer.CPUTime(),
			       timer.realTime());
    }
#else
    // based on extensive testing, we have settled on the following
    // combination
    // (1) if the two size of the first two are greater than the
    // uncompressed size of one bitmap, use option 1.  Because the
    // operation of the first two will produce an uncompressed result,
    // it will sum together all other bts with to the uncompressed
    // result generated already.
    // (2) if total size (bytes) times log 2 of number of bitmaps
    // is less than or equal to the size of an uncompressed
    // bitmap, use option 3, else use option 4.
    if (ibis::gVerbose > 4) {
	ibis::util::logMessage("index", "sumBits(%lu, %lu) will operate on "
			       "%lu out of %lu bitmaps using the combined "
			       "option", static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       static_cast<long unsigned>(na),
			       static_cast<long unsigned>(nobs));
	timer.start();
	if (straight) {
	    for (uint32_t i = ib; i < ie; ++i) {
		if (bts[i])
		    bytes += bts[i]->bytes();
	    }
	}
	else {
	    for (uint32_t i = 0; i < ib; ++i) {
		if (bts[i])
		    bytes += bts[i]->bytes();
	    }
	    for (uint32_t i = ie; i < nobs; ++i) {
		if (bts[i])
		    bytes += bts[i]->bytes();
	    }
	}
    }
    const uint32_t uncomp = (ibis::bitvector::bitsPerLiteral() == 8 ?
			     sz * 2 / 15 :
			     sz * 4 / 31);
    if (straight) {
	uint32_t sum2 = (bts[ib] ? bts[ib]->bytes() : 0U) +
	    (bts[ib+1] ? bts[ib+1]->bytes() : 0U);
	if (sum2 >= uncomp) {
	    uint32_t i;
	    for (i = ib; i < ie && bts[i] == 0; ++ i);
	    if (i < ie)
		res.copy(*(bts[i]));
	    else
		res.set(0, sz);
	    for (++ i; i < ie; ++ i) {
		if (bts[i])
		    res |= *(bts[i]);
	    }
	}
	else {
	    // need to compute the total size of all bitmaps
	    if (bytes == 0) {
		for (uint32_t i = ib; i < ie; ++i)
		    if (bts[i])
			bytes += bts[i]->bytes();
	    }
	    if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		// put all bitmaps in a priority queue
		std::priority_queue<_elem> que;
		_elem op1, op2, tmp;
		tmp.first = 0;

		// populate the priority queue with the original input
		for (uint32_t i = ib; i < ie; ++i) {
		    if (bts[i]) {
			op1.first = bts[i];
			op1.second = false;
			que.push(op1);
		    }
		}

		try {
		    while (! que.empty()) {
			op1 = que.top();
			que.pop();
			if (que.empty()) {
			    res.copy(*(op1.first));
			    if (op1.second) delete op1.first;
			    break;
			}

			op2 = que.top();
			que.pop();
			tmp.second = true;
			tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			LOGGER(ibis::gVerbose >= 0)
			    << "sumBits-using priority queue: "
			    << op1.first->bytes()
			    << (op1.second ? "(transient), " : ", ")
			    << op2.first->bytes()
			    << (op2.second ? "(transient) >> " : " >> ")
			    << tmp.first->bytes() << std::endl;
#endif
			if (op1.second)
			    delete op1.first;
			if (op2.second)
			    delete op2.first;
			if (! que.empty()) {
			    que.push(tmp);
			    tmp.first = 0;
			}
		    }
		    if (tmp.first != 0) {
			if (tmp.second) {
			    res.swap(*(tmp.first));
			    delete tmp.first;
			}
			else {
			    res.copy(*(tmp.first));
			}
		    }
		}
		catch (...) { // need to free the pointers
		    delete tmp.first;
		    while (! que.empty()) {
			tmp = que.top();
			if (tmp.second)
			    delete tmp.first;
			que.pop();
		    }
		    throw;
		}
	    }
	    else {
		uint32_t i;
		for (i = ib; i < ie && bts[i] == 0; ++ i);
		if (i < ie) {
		    res.copy(*(bts[i]));
		    res.decompress();
		    for (++ i; i < ie; ++ i)
			if (bts[i])
			    res |= *(bts[i]);
		}
		else {
		    res.set(0, sz);
		}
	    }
	}
    } // if (straight)
    else { // use complements
	uint32_t sum2;
	if (ib > 1) {
	    sum2 = bts[0]->bytes() + (bts[1] ? bts[1]->bytes() : 0U);
	}
	else if (ib == 1) {
	    sum2 = bts[0]->bytes() + (bts[ie] ? bts[ie]->bytes() : 0U);
	}
	else {
	    sum2 = (bts[ie] ? bts[ie]->bytes() : 0U) +
		(bts[ie+1] ? bts[ie+1]->bytes() : 0U);
	}
	if (sum2 >= uncomp) { // take advantage of automate decopression
	    if (ib > 1) {
		res.copy(*(bts[0]));
		for (uint32_t i = 1; i < ib; ++i)
		    if (bts[i])
			res |= *(bts[i]);
	    }
	    else if (ib == 1) {
		res.copy(*(bts[0]));
	    }
	    else {
		while (ie < nobs && bts[ie] == 0)
		    ++ ie;
		if (ie < nobs) {
		    res.copy(*(bts[ie]));
		    ++ ie;
		}
	    }
	    for (uint32_t i = ie; i < nobs; ++i)
		if (bts[i])
		    res |= *(bts[i]);
	}
	else { // need to look at the total size
	    if (bytes == 0) {
		for (uint32_t i = 0; i < ib; ++i)
		    if (bts[i])
			bytes += bts[i]->bytes();
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bts[i])
			bytes += bts[i]->bytes();
	    }
	    if (bytes*static_cast<double>(na)*na <= log(2.0)*uncomp) {
		// use priority queue for all bitmaps
		std::priority_queue<_elem> que;
		_elem op1, op2, tmp;
		tmp.first = 0;

		// populate the priority queue with the original input
		for (uint32_t i = 0; i < ib; ++i) {
		    if (bts[i]) {
			op1.first = bts[i];
			op1.second = false;
			que.push(op1);
		    }
		}
		for (uint32_t i = ie; i < nobs; ++i) {
		    if (bts[i]) {
			op1.first = bts[i];
			op1.second = false;
			que.push(op1);
		    }
		}

		try {
		    while (! que.empty()) {
			op1 = que.top();
			que.pop();
			if (que.empty()) {
			    res.copy(*(op1.first));
			    if (op1.second) delete op1.first;
			    break;
			}

			op2 = que.top();
			que.pop();
			tmp.second = true;
			tmp.first = *(op1.first) | *(op2.first);
#if defined(DEBUG)
			LOGGER(ibis::gVerbose >= 0)
			    << "sumBits-using priority queue: "
			    << op1.first->bytes()
			    << (op1.second ? "(transient), " : ", ")
			    << op2.first->bytes()
			    << (op2.second ? "(transient) >> " : " >> ")
			    << tmp.first->bytes();
#endif
			if (op1.second)
			    delete op1.first;
			if (op2.second)
			    delete op2.first;
			if (! que.empty()) {
			    que.push(tmp);
			    tmp.first = 0;
			}
		    }
		    if (tmp.first != 0) {
			if (tmp.second) {
			    res.swap(*(tmp.first));
			    delete tmp.first;
			}
			else {
			    res.copy(*(tmp.first));
			}
		    }
		}
		catch (...) { // need to free the pointers
		    delete tmp.first;
		    while (! que.empty()) {
			tmp = que.top();
			if (tmp.second)
			    delete tmp.first;
			que.pop();
		    }
		    throw;
		}
	    }
	    else if (sum2 <= (uncomp >> 2)){
		// uncompress the first bitmap generated
		if (ib > 1) {
		    res.copy(*(bts[0]));
		    res.decompress();
		    for (uint32_t i = 1; i < ib; ++i)
			if (bts[i])
			    res |= *(bts[i]);
		}
		else if (ib == 1) {
		    res.copy(*(bts[0]));
		    res.decompress();
		}
		else {
		    while (ie < nobs && bts[ie] == 0)
			++ ie;
		    if (ie < nobs) {
			res.copy(*(bts[ie]));
			res.decompress();
			++ ie;
		    }
		    else {
			res.set(0, sz);
		    }
		}
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bts[i])
			res |= *(bts[i]);
	    }
	    else if (ib > 0) {
		res.copy(*(bts[0]));
		res.decompress();
		for (uint32_t i = 1; i < ib; ++i)
		    if (bts[i])
			res |= *(bts[i]);
		for (uint32_t i = ie; i < nobs; ++i)
		    if (bts[i])
			res |= *(bts[i]);
	    }
	    else {
		while (ie < nobs && bts[ie] == 0)
		    ++ ie;
		if (ie < nobs) {
		    res.copy(*(bts[ie]));
		    res.decompress();
		    for (uint32_t i = ie+1; i < nobs; ++i)
			if (bts[i])
			    res |= *(bts[i]);
		}
		else {
		    res.set(0, sz);
		}
	    }
	}
	res.flip(); // need to flip because we have been using complement
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("index", "sumBits operated on %lu bitmap%s "
			       "(%lu B in %lu B out) took %g sec(CPU), "
			       "%g sec(elapsed).",
			       static_cast<long unsigned>(na), (na>1?"s":""),
			       static_cast<long unsigned>(bytes),
			       static_cast<long unsigned>(res.bytes()),
			       timer.CPUTime(), timer.realTime());
    }
#endif
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "sumBits(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::sumBits

/// Sum up @c bts[ib:ie-1] and add the result to @c res.  It is assumed
/// that all bts add up to @c tot.  In the other version of sumBits
/// without this argument @c tot, it was assumed that all bitmaps add up to
/// a bit vector of all ones.  The decision of whether to use bts[ib:ie-1]
/// directly or use the subtractive version (using bts[0:ib-1] and
/// bts[ie:nobs-1]) are based on the number of bit vectors.
void ibis::index::sumBits(const std::vector<ibis::bitvector*>& bts,
			  const ibis::bitvector& tot, uint32_t ib,
			  uint32_t ie, ibis::bitvector& res) {
    LOGGER(ibis::gVerbose > 7)
	<< "ibis::index::sumBits(" << bts.size()
	<< "-bitvector set, tot(" << tot.cnt() << ", " << tot.size()
	<< "), " << ib << ", " << ie << "res(" << res.cnt() << ", "
	<< res.size() << ")) ...";
    const uint32_t uncomp = (ibis::bitvector::bitsPerLiteral() == 8 ?
			     tot.size() * 2 / 15 : tot.size() * 4 / 31);
    const uint32_t nobs = bts.size();
    if (ie > nobs)
	ie = nobs;
    if (ib >= ie || ib >= nobs) {
	return;
    }
    horometer timer;
    if (ibis::gVerbose > 4)
	timer.start();

    if (res.size() != tot.size())
	res.set(0, tot.size());
    if ((ie-ib)*2 <= nobs) { // direct evaluation: less than half of bitmaps
	const uint32_t nb = ie - ib;
	if (ie-ib > 24) {
	    res.decompress();
	}
	else if (nb > 3) {
	    uint32_t tb = 0;
	    for (uint32_t i = ib; i < ie; ++ i)
		if (bts[i])
		    tb += bts[i]->bytes();
	    if (nb * log(static_cast<double>(nb)) >
		uncomp / static_cast<double>(tb))
		res.decompress();
	}
	for (uint32_t i=ib; i<ie; ++i)
	    if (bts[i])
		res |= *(bts[i]);
    }
    else if (ib == 0 && ie >= nobs) { // all bitmaps
	res |= tot;
    }
    else { // more than half (but not all)
	ibis::bitvector tmp;
	while (ib > 0 && bts[ib-1] == 0)
	    -- ib;
	if (bts[ib]) {
	    tmp.copy(*(bts[ib]));
	    if (ib > 0)
		-- ib;
	}
	else {
	    while (ie < nobs && bts[ie] == 0)
		++ ie;
	    if (ie < nobs) {
		tmp.copy(*(bts[ie]));
		++ ie;
	    }
	    else {
		tmp.set(0, tot.size());
	    }
	}
	const uint32_t nb = nobs - ie + ib;
	if (nb > 24) {
	    tmp.decompress();
	}
	else if (nb > 3) {
	    uint32_t tb = 0;
	    for (uint32_t i = 0; i < ib; ++ i)
		if (bts[i])
		    tb += bts[i]->bytes();
	    for (uint32_t i = ie; i < nobs; ++ i)
		if (bts[i])
		    tb += bts[i]->bytes();
	    if (nb * log(static_cast<double>(nb))
		> uncomp / static_cast<double>(tb))
		tmp.decompress();
	}
	for (uint32_t i = 0; i < ib; ++ i)
	    if (bts[i])
		tmp |= *(bts[i]);
	for (uint32_t i = ie; i < nobs; ++ i)
	    if (bts[i])
		tmp |= *(bts[i]);
	ibis::bitvector diff(tot);
	diff -= tmp;
	res |= diff;
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("index", "sumBits(%lu, %lu) took %g sec(CPU), "
			       "%g sec(elapsed).",
			       static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       timer.CPUTime(), timer.realTime());
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) >= res.bytes()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "sumBits(" << ib << ", " << ie << "):" << res;
    }
#endif
} // ibis::index::sumBits

/// Fill the array bases with the values that cover the range [0, card).
/// Assumes at least two components.  For one component case use indices
/// defined explicit for one component cases.
void ibis::index::setBases(array_t<uint32_t>& bases, uint32_t card,
			   uint32_t nbase) {
    if (card < 4) { // very low cardinality, use only one component
	bases.resize(1);
	bases[0] = card;
	return;
    }

    if (nbase > 2) {
	// more than two components, make sure each base size is no less
	// than 2
	uint32_t b = static_cast<uint32_t>
	    (log(static_cast<double>(card)) / log(2.0));
	if (b <= 1 && card >= 4)
	    b = 2;
	if (b < nbase)
	    nbase = b;
    }
    else if (nbase == 2 && card < 4) {
	nbase = 1;
    }
    if (nbase > 2) { // more than two components
	uint32_t b = static_cast<uint32_t>(ceil(pow(card, 1.0/nbase)));
	bases.resize(nbase);
	uint32_t tot = 1;
	for (uint32_t i = 0; i < nbase; ++i) {
	    bases[i] = b;
	    tot *= b;
	}
	for (uint32_t i = 0; i < nbase; ++i) {
	    if ((tot/b)*(b-1) >= card) {
		bases[nbase-i-1] = b - 1;
		tot /= b;
		tot *= b - 1;
	    }
	    else {
		break; // do not examine the rest of the bases
	    }
	}
	// remove the last few bases that are one
	while (nbase > 0 && bases[nbase-1] == 1)
	    -- nbase;
	bases.resize(nbase);
    }
    else if (card > 2 && nbase > 1) { // assume two components
	uint32_t b = static_cast<uint32_t>(ceil(sqrt(static_cast<double>(card))));
	bases.resize(2);
	bases[0] = static_cast<uint32_t>
	    (ceil(static_cast<double>(card)/static_cast<double>(b)));
	bases[1] = b;
	double tmp = 0.5 * (bases[0] + bases[1]);
	tmp = tmp*tmp - card;
	tmp = sqrt(tmp);
	tmp = tmp -  0.5 * (bases[1] - bases[0]);
	if (tmp > 0) {
	    bases[0] -= static_cast<int>(tmp);
	    bases[1] += static_cast<int>(tmp);
	}
	if (bases[1] > bases[0]) {
	    b = bases[0];
	    bases[0] = bases[1];
	    bases[1] = b;
	}
	if (bases[1] < 2) { // should be only one component
	    bases.resize(1);
	}
    }
    else { // only one component
	bases.resize(1);
	bases[0] = card;
    }
} // ibis::index::setBases

/// Decide whether to uncompress the bitmaps.
void ibis::index::optionalUnpack(std::vector<ibis::bitvector*>& bts,
				 const char *opt) {
    const uint32_t nobs = bts.size();
    const char *ptr = 0;
    if (opt != 0)
	ptr = strstr(opt, "<compressing ");
    if (ptr != 0) {
	ptr += 13;
	while (isspace(*ptr))
	    ++ ptr;
	if (strnicmp(ptr, "uncompress", 10) == 0) {
	    switch (ptr[10]) {
	    case 'a':
	    case 'A': { // uncompressAll
		for (uint32_t i = 0; i < nobs; ++i) {
		    if (bts[i])
			bts[i]->decompress();
		}
		break;
	    }
	    case 'd':
	    case 'D': { // uncompressDense
		double dens = 0.125;
		ptr = strchr(ptr, '>');
		if (ptr != 0) {
		    ++ ptr;
		    dens = atof(ptr);
		    if (dens <= 0.0)
			dens = 0.125;
		}
		for (uint32_t i = 0; i < nobs; ++i) {
		    if (bts[i]) {
			//bts[i]->compress();
			if (bts[i]->cnt() >
			    static_cast<uint32_t>(dens * bts[i]->size()))
			    bts[i]->decompress();
		    }
		}
		break;
	    }
	    case 'l':
	    case 'L': { // uncompressLarge
		double cr = 0.75;
		ptr = strchr(ptr, '>');
		if (ptr != 0) {
		    ++ ptr;
		    cr = atof(ptr);
		    if (cr <= 0.0)
			cr = 0.75;
		}
		for (uint32_t i = 0; i < nobs; ++i) {
		    if (bts[i]) {
			//bts[i]->compress();
			if (bts[i]->bytes() > static_cast<uint32_t>
			    (ceil(cr * (bts[i]->size()>>3))))
			    bts[i]->decompress();
		    }
		}
		break;
	    }
	    default: break; // do nothing
	    }
	}
    }
    else { // check ibis::gParameters
	std::string uA = col->partition()->name();
	uA += ".";
	uA += col->name();
	uA += ".uncompress";
	std::string uL = uA;
	uL += "LargeBitvector";
	uA += "All";
	if (ibis::gParameters().isTrue(uA.c_str())) {
	    // decompress the bitvectors as requested
	    for (uint32_t i = 0; i < nobs; ++i) {
		if (bts[i])
		    bts[i]->decompress();
	    }
	}
	else if (ibis::gParameters().isTrue(uL.c_str())) {
	    // decompress the bitvectors as requested -- decompress those
	    // with compression ratios larger than 1/3
	    uint32_t bar0 = nrows / 24;
	    for (uint32_t i = 0; i < nobs; ++i) {
		if (bts[i]) {
		    //bts[i]->compress();
		    if (bts[i]->bytes() > bar0)
			bts[i]->decompress();
		}
	    }
	}
	else {
	    // decompress very heavy bitvectors, > 8/9
	    uint32_t bar1 = nrows / 9;
	    for (uint32_t i = 0; i < nobs; ++i) {
		if (bts[i]) {
		    //bts[i]->compress();
		    if (bts[i]->bytes() > bar1)
			bts[i]->decompress();
		}
	    }
	}
    }
} // ibis::index::optionalUnpack

/// A trivial implementation to indicate the index can not determine any row.
void ibis::index::estimate(const ibis::qDiscreteRange& expr,
			   ibis::bitvector& lower,
			   ibis::bitvector& upper) const {
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- using a dummy version of ibis::index::estimate to "
	"evaluate a qDiscreteRange on column " << expr.colName();
    if (col && col->partition()) {
	lower.set(0, col->partition()->nRows());
	upper.set(1, col->partition()->nRows());
    }
} // ibis::index::estimate

uint32_t ibis::index::estimate(const ibis::qDiscreteRange& expr) const {
    LOGGER(ibis::gVerbose > 1)
	<< " Note -- using a dummy version of ibis::index::estimate to "
	"evaluate a qDiscreteRange on column " << expr.colName();
    return (col && col->partition() ? col->partition()->nRows() : 0U);
} // ibis::index::estimate

float ibis::index::undecidable(const ibis::qDiscreteRange& expr,
			       ibis::bitvector& iffy) const {
    LOGGER(ibis::gVerbose > 2)
	<< "Note -- using a dummy version of ibis::index::undecidable to "
	"evaluate a qDiscreteRange on column " << expr.colName();
    if (col && col->partition())
	iffy.set(1, col->partition()->nRows());
    return 0.5;
} // ibis::index::undecidable

// Provided as dummy implementation so that the derived classes are not
// force to implement these functions.  It indicates that every row is
// undecidable by the index.
void ibis::index::estimate(const ibis::index& idx2,
			   const ibis::rangeJoin& expr,
			   ibis::bitvector64& lower,
			   ibis::bitvector64& upper) const {
    if (col == 0) return;

    LOGGER(ibis::gVerbose > 2)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process " << expr;

    ibis::bitvector64::word_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    nb *= nb;
    lower.set(0, nb);
    upper.set(1, nb);
} // ibis::index::estimate

void ibis::index::estimate(const ibis::index& idx2,
			   const ibis::rangeJoin& expr,
			   const ibis::bitvector& mask,
			   ibis::bitvector64& lower,
			   ibis::bitvector64& upper) const {
    if (col == 0) return;

    LOGGER(ibis::gVerbose > 2)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process " << expr;

    ibis::bitvector64::word_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    nb *= nb;
    lower.set(0, nb);
    upper.clear();
    ibis::util::outerProduct(mask, mask, upper);
} // ibis::index::estimate

void ibis::index::estimate(const ibis::index& idx2,
			   const ibis::rangeJoin& expr,
			   const ibis::bitvector& mask,
			   const ibis::qRange* const range1,
			   const ibis::qRange* const range2,
			   ibis::bitvector64& lower,
			   ibis::bitvector64& upper) const {
    if (col == 0) return;
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process " << expr;

    ibis::bitvector64::word_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    nb *= nb;
    lower.set(0, nb);
    upper.clear();
    ibis::util::outerProduct(mask, mask, upper);
} // ibis::index::estimate

void ibis::index::estimate(const ibis::rangeJoin& expr,
			   const ibis::bitvector& mask,
			   const ibis::qRange* const range1,
			   const ibis::qRange* const range2,
			   ibis::bitvector64& lower,
			   ibis::bitvector64& upper) const {
    if (col == 0) return;
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process %s" << expr;

    ibis::bitvector64::word_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    nb *= nb;
    lower.set(0, nb);
    upper.clear();
    ibis::util::outerProduct(mask, mask, upper);
} // ibis::index::estimate

int64_t ibis::index::estimate(const ibis::index& idx2,
			      const ibis::rangeJoin& expr) const {
    if (col == 0) return -1;
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process %s" << expr;

    int64_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    nb *= nb;
    return nb;
} // ibis::index::estimate

int64_t ibis::index::estimate(const ibis::index& idx2,
			      const ibis::rangeJoin& expr,
			      const ibis::bitvector& mask) const {
    if (col == 0) return -1;
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process %s" << expr;

    int64_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    if (nb > mask.cnt())
	nb = mask.cnt();
    nb *= nb;
    return nb;
} // ibis::index::estimate

int64_t ibis::index::estimate(const ibis::index& idx2,
			      const ibis::rangeJoin& expr,
			      const ibis::bitvector& mask,
			      const ibis::qRange* const range1,
			      const ibis::qRange* const range2) const {
    if (col == 0) return -1;
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process %s" << expr;

    int64_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    if (nb > mask.cnt())
	nb = mask.cnt();
    nb *= nb;
    return nb;
} // ibis::index::estimate

int64_t ibis::index::estimate(const ibis::rangeJoin& expr,
			      const ibis::bitvector& mask,
			      const ibis::qRange* const range1,
			      const ibis::qRange* const range2) const {
    if (col == 0) return -1;
    LOGGER(ibis::gVerbose > 1)
	<< "Note -- index::estimate is using a dummy estimate "
	"function to process %s" << expr;

    int64_t nb = static_cast<ibis::bitvector64::word_t>
	(col->partition()->nRows());
    if (nb > mask.cnt())
	nb = mask.cnt();
    nb *= nb;
    return nb;
} // ibis::index::estimate

// explicit instantiation of templated functions
template void ibis::index::mapValues(const array_t<char>&, VMap&);
template void ibis::index::mapValues(const array_t<unsigned char>&, VMap&);
template void ibis::index::mapValues(const array_t<int16_t>&, VMap&);
template void ibis::index::mapValues(const array_t<uint16_t>&, VMap&);
template void ibis::index::mapValues(const array_t<int32_t>&, VMap&);
template void ibis::index::mapValues(const array_t<uint32_t>&, VMap&);
template void ibis::index::mapValues(const array_t<int64_t>&, VMap&);
template void ibis::index::mapValues(const array_t<uint64_t>&, VMap&);
template void ibis::index::mapValues(const array_t<float>&, VMap&);
template void ibis::index::mapValues(const array_t<double>&, VMap&);

template void
ibis::index::mapValues(const array_t<char>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<unsigned char>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<int16_t>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<uint16_t>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<int32_t>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<uint32_t>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<int64_t>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<uint64_t>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<float>&, histogram&, uint32_t);
template void
ibis::index::mapValues(const array_t<double>&, histogram&, uint32_t);

template void
ibis::index::mapValues(const array_t<char>&, array_t<char>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<unsigned char>&, array_t<unsigned char>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<int16_t>&, array_t<int16_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<uint16_t>&, array_t<uint16_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<int32_t>&, array_t<int32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<uint32_t>&, array_t<uint32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<float>&, array_t<float>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<double>&, array_t<double>&,
		       std::vector<uint32_t>&);

template void
ibis::index::mapValues(const array_t<int32_t>&, const array_t<int32_t>&,
		       array_t<int32_t>&, array_t<int32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<int32_t>&, const array_t<uint32_t>&,
		       array_t<int32_t>&, array_t<uint32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<int32_t>&, const array_t<float>&,
		       array_t<int32_t>&, array_t<float>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<int32_t>&, const array_t<double>&,
		       array_t<int32_t>&, array_t<double>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<uint32_t>&, const array_t<int32_t>&,
		       array_t<uint32_t>&, array_t<int32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<uint32_t>&, const array_t<uint32_t>&,
		       array_t<uint32_t>&, array_t<uint32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<uint32_t>&, const array_t<float>&,
		       array_t<uint32_t>&, array_t<float>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<uint32_t>&, const array_t<double>&,
		       array_t<uint32_t>&, array_t<double>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<float>&, const array_t<int32_t>&,
		       array_t<float>&, array_t<int32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<float>&, const array_t<uint32_t>&,
		       array_t<float>&, array_t<uint32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<float>&, const array_t<float>&,
		       array_t<float>&, array_t<float>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<float>&, const array_t<double>&,
		       array_t<float>&, array_t<double>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<double>&, const array_t<int32_t>&,
		       array_t<double>&, array_t<int32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<double>&, const array_t<uint32_t>&,
		       array_t<double>&, array_t<uint32_t>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<double>&, const array_t<float>&,
		       array_t<double>&, array_t<float>&,
		       std::vector<uint32_t>&);
template void
ibis::index::mapValues(const array_t<double>&, const array_t<double>&,
		       array_t<double>&, array_t<double>&,
		       std::vector<uint32_t>&);

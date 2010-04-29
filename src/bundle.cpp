//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2010 the Regents of the University of California
//
// This file contains the implementation details of the classes defined in
// bundle.h
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "bundle.h"
#include "utilidor.h"	// ibis::util::reorder
#include <sstream>	// std::ostringstream

//////////////////////////////////////////////////////////////////////
// functions of class bundle
/// Create a new bundle from previously stored information.
ibis::bundle* ibis::bundle::create(const ibis::query& q,
				   const ibis::bitvector& hits) {
    if (hits.size() == 0 || hits.cnt() == 0)
	return 0;
    ibis::horometer timer;
    if (ibis::gVerbose > 2)
	timer.start();

    ibis::bundle* bdl = 0;
    if (q.components().empty())
	bdl = new ibis::bundle0(q, hits);
    else if (q.components().size() == 1)
	bdl = new ibis::bundle1(q, hits);
    else
	bdl = new ibis::bundles(q, hits);

    if (ibis::gVerbose > 2) {
	timer.stop();
	q.logMessage("createBundle", "time to generate the bundle: "
		     "%g sec(CPU), %g sec(elapsed)", timer.CPUTime(),
		     timer.realTime());
    }
    return bdl;
} // ibis::bundle::create

/// Create new bundle from a hit vector.  Write info to q.dir().
ibis::bundle* ibis::bundle::create(const ibis::query& q) {
    ibis::horometer timer;
    if (ibis::gVerbose > 2)
	timer.start();

    ibis::bundle* bdl = 0;
    if (q.components().empty())
	bdl = new ibis::bundle0(q);
    else if (q.components().size() == 1)
	bdl = new ibis::bundle1(q);
    else
	bdl = new ibis::bundles(q);

    if (ibis::gVerbose > 2) {
	timer.stop();
	q.logMessage("createBundle", "time to generate the bundle: "
		     "%g sec(CPU), %g sec(elapsed)", timer.CPUTime(),
		     timer.realTime());
    }
    return bdl;
} // ibis::bundle::create

/// Create a bundle using the values passed in through vals.  It assumes
/// that the caller has processed the select clause and provided all the
/// necessary values in vals.  Currently, the only column that does not
/// require any actual input value is count(*).
ibis::bundle* ibis::bundle::create(const ibis::part& tbl,
				   const ibis::selectClause& sel,
				   const std::vector<void*>& vals) {
    const uint32_t nc = (vals.size() <= sel.size() ? vals.size() : sel.size());
    ibis::bundle* res = 0;
    if (nc > 1) {
	res = new ibis::bundles(tbl, sel, vals);
    }
    else if (nc == 1) {
	res = new ibis::bundle1(tbl, sel, vals);
    }
    return res;
} // ibis::bundle::create

/// Sort RIDs in the range of [i, j)
void ibis::bundle::sortRIDs(uint32_t i, uint32_t j) {
    std::less<ibis::rid_t> cmp;
    if (i+32 >= j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1) {
	    for (uint32_t i2=i; i2<i1; ++i2) {
		if (cmp((*rids)[i2+1], (*rids)[i2]))
		    swapRIDs(i2, i2+1);
	    }
	}
    }
    else { // use quick sort
	ibis::rid_t tmp = (*rids)[(i+j)/2];
	uint32_t i1 = i;
	uint32_t i2 = j-1;
	bool left = cmp((*rids)[i1], tmp);
	bool right = !cmp((*rids)[i2], tmp);
	while (i1 < i2) {
	    if (left && right) {
		// both i1 and i2 are in the right position
		++ i1; --i2;
		left = cmp((*rids)[i1], tmp);
		right = !cmp((*rids)[i2], tmp);
	    }
	    else if (right) {
		// i2 is in the right position
		-- i2;
		right = !cmp((*rids)[i2], tmp);
	    }
	    else if (left) {
		// i1 is in the right position
		++ i1;
		left = cmp((*rids)[i1], tmp);
	    }
	    else { // both in the wrong position, swap them
		swapRIDs(i2, i1);
		++ i1; -- i2;
		left = cmp((*rids)[i1], tmp);
		right = !cmp((*rids)[i2], tmp);
	    }
	}
	i1 += left; // if left is true, rids[i1] should be on the left
	// everything below i1 is less than tmp
	if (i1 > i) {
	    sortRIDs(i, i1);
	    sortRIDs(i1, j);
	}
	else { // nothing has been swapped, i.e., tmp is the smallest
	    while (i1 < j &&
		   0 == memcmp(&tmp, &((*rids)[i1]), sizeof(ibis::rid_t)))
		++i1;
	    if (i1+i1 < i+j) {
		swapRIDs(i1, (i+j)/2);
		++i1;
	    }
	    sortRIDs(i1, j);
	}
    }
} // ibis::bundle::sortRIDs

/// Read the RIDs related to the ith bundle.
const ibis::RIDSet* ibis::bundle::readRIDs(const char* dir,
					   const uint32_t i) {
    if (dir == 0) return 0;

    char fn[PATH_MAX];
    uint32_t len = strlen(dir);
    if (len+8 >= PATH_MAX) {
	ibis::util::logMessage("Error", "ibis::bundle::readRIDs -- "
			       "argument dir (%s) too long", dir);
	throw "ibis::bundle::readRIDs -- argument dir too long";
    }

    if (dir[len-1] == FASTBIT_DIRSEP) {
	strcpy(fn, dir);
	strcat(fn, "bundles");
    }
    else {
	++len;
	sprintf(fn, "%s%cbundles", dir, FASTBIT_DIRSEP);
    }
    ibis::fileManager::storage* bdlstore=0;
    int ierr = ibis::fileManager::instance().getFile(fn, &bdlstore);
    if (ierr != 0) {
	ibis::util::logMessage("Warning", "ibis::bundle::readRIDs failed "
			       "to retrieve the bundle file %s", fn);
	return 0;
    }
    uint32_t ncol, nbdl, offset;
    bdlstore->beginUse(); // obtain a read lock on dblstore
    { // get the first two numbers out of bdlstore
	ibis::array_t<uint32_t> tmp(bdlstore, 0, 2);
	nbdl = tmp[0];
	ncol = tmp[1];
    }
    { // verify the file contains the right number of bytes
	ibis::array_t<uint32_t> sizes(bdlstore, 2*sizeof(uint32_t), ncol);
	uint32_t expected = sizeof(uint32_t)*(ncol+3+nbdl);
	for (uint32_t i0 = 0; i0 < ncol; ++i0)
	    expected += sizes[i0] * nbdl;
	if (expected != bdlstore->bytes()) {
	    ibis::util::logMessage
		("Warning", "ibis::bundle::readRIDs -- according to the "
		 "header, %lu bytes are expected, but the file %s "
		 "contains %lu", static_cast<long unsigned>(expected), fn,
		 static_cast<long unsigned>(bdlstore->bytes()));
	    return 0;
	}
	offset = expected - sizeof(uint32_t)*(nbdl+1);
    }

    ibis::array_t<uint32_t> starts(bdlstore, offset, nbdl+1);
    bdlstore->endUse(); // this function no longer needs the read lock
    if (i < nbdl) { // open the rid file and read the selected segment
	ibis::RIDSet* res = new ibis::RIDSet;
	strcpy(fn+len, "-rids");
	int fdes = UnixOpen(fn, OPEN_READONLY);
	if (fdes < 0) {
	    if (errno != ENOENT || ibis::gVerbose > 10)
		ibis::util::logMessage
		    ("Warning", "ibis::bundle::readRIDs -- failed to "
		     "open file \"%s\" ... %s", fn,
		     (errno ? strerror(errno) : "no free stdio stream"));
	    delete res;
	    return 0;
	}
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdes, _O_BINARY);
#endif
	offset = sizeof(ibis::rid_t) * starts[i];
	if (offset != static_cast<uint32_t>(UnixSeek(fdes, offset,
						     SEEK_SET))) {
	    ibis::util::logMessage("Warning", "ibis::bundle::readRIDs "
				   "-- failed to fseek to %lu in file %s ",
				   static_cast<long unsigned>(offset), fn);
	    delete res;
	    return 0;
	}

	len = starts[i+1] - starts[i];
	res->resize(len);
	offset = UnixRead(fdes, res->begin(), sizeof(ibis::rid_t)*len);
	ibis::fileManager::instance().recordPages
	    (sizeof(ibis::rid_t)*starts[i], sizeof(ibis::rid_t)*starts[i+1]);
	UnixClose(fdes);
	if (offset != sizeof(ibis::rid_t)*len) {
	    ibis::util::logMessage("Warning", "ibis::bundle::readRIDs -- "
				   "expected to read %lu RIDs but got %lu ",
				   static_cast<long unsigned>(len),
				   static_cast<long unsigned>(nbdl));
	    delete res;
	    return 0;
	}
	else {
	    return res;
	}
    }
    else {
	return 0;
    }
} // ibis::bundle::readRIDs

/// Return the maximal int value.
int32_t ibis::bundle::getInt(uint32_t, uint32_t) const {
    return 0x7FFFFFFF;
} // ibis::bundle::getInt

/// Return the maximal unsigned int value.
uint32_t ibis::bundle::getUInt(uint32_t, uint32_t) const {
    return 0xFFFFFFFFU;
} // ibis::bundle::getUInt

/// Return the maximal int value.
int64_t ibis::bundle::getLong(uint32_t, uint32_t) const {
    return 0x7FFFFFFFFFFFFFFFLL;
} // ibis::bundle::getLong

/// Return the maximal unsigned int value.
uint64_t ibis::bundle::getULong(uint32_t, uint32_t) const {
    return 0xFFFFFFFFFFFFFFFFULL;
} // ibis::bundle::getULong

/// Return the maximal float value.
float ibis::bundle::getFloat(uint32_t, uint32_t) const {
    return FLT_MAX;
} // ibis::bundle::getFloat

/// Return the maximum double value.
double ibis::bundle::getDouble(uint32_t, uint32_t) const {
    return DBL_MAX;
} // ibis::bundle::getDouble

/// Return an empty string.  Could have thrown an exception, but that
/// seemed to be a little too heavy handed.
std::string ibis::bundle::getString(uint32_t, uint32_t) const {
    std::string ret;
    return ret;
} // ibis::bundle::getString

uint32_t ibis::bundle::rowCounts(array_t<uint32_t>& cnt) const {
    cnt.clear();
    if (starts == 0) return 0;

    const uint32_t ng = starts->size()-1;
    cnt.resize(ng);
    for (unsigned i = 0; i < ng; ++ i)
	cnt[i] = (*starts)[i+1] - (*starts)[i];
    return ng;
} // ibis::bundle::rowCounts

//////////////////////////////////////////////////////////////////////
// functions for ibis::bundle0
// print the bundle values along with the RIDs
void ibis::bundle0::printAll(std::ostream& out) const {
    ibis::util::ioLock lock;
    if (rids) {
	// print all RIDs one on a line
	ibis::RIDSet::const_iterator it;
	if (ibis::gVerbose > 2)
	    out << "IDs of all qualified rows for bundle " << id
		<< " (one per line)" << std::endl;
	for (it = rids->begin(); it != rids->end(); ++it) {
	    out << *it << std::endl;;
	}
	out << std::endl;
    }
    else if (ibis::gVerbose > 1) {
	out << "No RIDS for bundle " << id << std::endl;
    }
} // ibis::bundle0::printAll

//////////////////////////////////////////////////////////////////////
// functions for ibis::bundle1
//
/// Constructor.  It attempt to read to read a bundle from files first.  If
/// that fails, it attempts to create a bundle based on the current hits.
ibis::bundle1::bundle1(const ibis::query& q) : bundle(q) {
    if (q.getNumHits() == 0)
	return;

    char bdlfile[PATH_MAX];
    const ibis::part* tbl = q.partition();
    if (q.dir()) {
	strcpy(bdlfile, q.dir());
	strcat(bdlfile, "bundles");
    }
    else {
	bdlfile[0] = 0;
    }
    const ibis::selectClause& cmps = q.components();
    if (cmps.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- ibis::bundle1 can not continue with an empty "
	    "select clause";
	throw "ibis::bundle1 can not work with empty select clauses";
    }
    LOGGER(cmps.size() != 1 && ibis::gVerbose > 0)
	<< "Warning -- ibis::bundle1 will only use the 1st terms of "
	<< cmps.size();
    ibis::column* c = tbl->getColumn(cmps.argName(0));
    if (c == 0) {
	ibis::util::logMessage("Warning", "ibis::bundle1::ctor name %s "
			       "is not a column in table ", cmps.argName(0));
	return;
    }

    if (ibis::util::getFileSize(bdlfile) > 0) {
	// file bundles exists, read it
	if (rids == 0) {
	    rids = q.readRIDs(); // read RIDs
	    if (rids != 0 && static_cast<long>(rids->size()) !=
		q.getNumHits()) {
		delete rids;
		rids = 0;
	    }
	}
	ibis::fileManager::storage* bdlstore = 0;
	int ierr = ibis::fileManager::instance().getFile(bdlfile, &bdlstore);
	if (ierr != 0) {
	    ibis::util::logMessage("Error", "ibis::bundle1::ctor failed "
				   "to retrieve bundle file %s", bdlfile);
	    throw ibis::bad_alloc("failed to retrieve bundle file");
	}
	// no need to explicitly call beginUse() because array_t sizes will
	// hold a read lock long enough until starts holds another one for
	// the life time of this object
	ibis::array_t<uint32_t> sizes(bdlstore, 0, 3);
	uint32_t expected = sizeof(uint32_t)*(sizes[0]+4) + sizes[0]*sizes[2];
	if (bdlstore->bytes() == expected) {
	    if (cmps.getAggregator(0) == ibis::selectClause::NIL) {
		col = ibis::colValues::create
		    (c, bdlstore, 3*sizeof(uint32_t), sizes[0]);
	    }
	    else {
		switch (cmps.getAggregator(0)) {
		case ibis::selectClause::AVG:
		case ibis::selectClause::SUM:
	        case ibis::selectClause::VARPOP:
	        case ibis::selectClause::VARSAMP:
	        case ibis::selectClause::STDPOP:
	        case ibis::selectClause::STDSAMP:
		    col = new ibis::colDoubles
			(c, bdlstore, 3*sizeof(uint32_t), sizes[0]);
		    break;
		default:
		    col = ibis::colValues::create
			(c, bdlstore, 3*sizeof(uint32_t), sizes[0]);
		    break;
		}
	    }
	    starts = new ibis::array_t<uint32_t>
		(bdlstore, 3*sizeof(uint32_t)+sizes[0]*sizes[2], sizes[0]+1);
	    infile = true;
	}
	else {
	    ibis::util::logMessage
		("Warning", "ibis::bundle1::ctor -- according to the "
		 "header, %lu bytes are expected, but the file %s "
		 "contains %lu", static_cast<long unsigned>(expected),
		 bdlfile, static_cast<long unsigned>(bdlstore->bytes()));
	}
    }

    if (starts == 0) { // use the current hit vector
	const ibis::bitvector* hits = q.getHitVector();
	if (hits != 0 && hits->cnt() > 0) {
	    if (rids == 0) {
		rids = tbl->getRIDs(*hits);
		if (rids != 0 && rids->size() != hits->cnt()) {
		    delete rids;
		    rids = 0;
		}
	    }
	    if (cmps.getAggregator(0) == ibis::selectClause::NIL) {
		col = ibis::colValues::create(c, *hits);
	    }
	    else {
		switch (cmps.getAggregator(0)) {
		case ibis::selectClause::AVG:
		case ibis::selectClause::SUM:
	        case ibis::selectClause::VARPOP:
	        case ibis::selectClause::VARSAMP:
	        case ibis::selectClause::STDPOP:
	        case ibis::selectClause::STDSAMP:
		    col = new ibis::colDoubles(c, *hits);
		    break;
		default:
		    col = ibis::colValues::create(c, *hits);
		    break;
		}
	    }
	    if (col->size() != hits->cnt()) {
		ibis::util::logMessage
		    ("Warning", "ibis::bundle1::ctor got %lu values "
		     "but expected %lu",
		     static_cast<long unsigned>(col->size()),
		     static_cast<long unsigned>(hits->cnt()));
		delete col;
		col = 0;
		throw ibis::bad_alloc("incorrect number of bundles");
	    }
	}
	sort();
    }

    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "query[" << q.id()
		    << "]::bundle1 -- generated the bundle\n";
	if (rids == 0) {
	    if ((1U << ibis::gVerbose) > col->size() ||
		ibis::gVerbose > 30)
		print(lg.buffer());
	}
	else if ((1U << ibis::gVerbose) > rids->size() ||
		 ibis::gVerbose > 30) {
	    if (ibis::gVerbose > 8)
		printAll(lg.buffer());
	    else
		print(lg.buffer());
	}
    }
} // ibis::bundle1::bundle1

/// Constructor.  It creates a bundle using the rows selected by hits.
ibis::bundle1::bundle1(const ibis::query& q, const ibis::bitvector& hits)
    : bundle(q, hits) {
    if (hits.cnt() == 0)
	return;

    const ibis::part* tbl = q.partition();
    if (rids == 0) {
	rids = tbl->getRIDs(hits);
	if (rids != 0 && rids->size() != hits.cnt()) {
	    delete rids;
	    rids = 0;
	}
    }
    const ibis::selectClause& cmps = q.components();
    ibis::column* c = tbl->getColumn(cmps.argName(0));
    if (c != 0) {
	if (cmps.getAggregator(0) == ibis::selectClause::NIL) {
	    // use column type
	    col = ibis::colValues::create(c, hits);
	}
	else { // a function, treat AVG and SUM as double
	    switch (cmps.getAggregator(0)) {
	    case ibis::selectClause::AVG:
	    case ibis::selectClause::SUM:
	    case ibis::selectClause::VARPOP:
	    case ibis::selectClause::VARSAMP:
	    case ibis::selectClause::STDPOP:
	    case ibis::selectClause::STDSAMP:
		col = new ibis::colDoubles(c, hits);
		break;
	    default:
		col = ibis::colValues::create(c, hits);
		break;
	    }
	}
	if (col->size() != hits.cnt()) {
	    ibis::util::logMessage
		("Warning", "ibis::bundle1::ctor got %lu value, but "
		 "expected %lu",
		 static_cast<long unsigned>(col->size()),
		 static_cast<long unsigned>(hits.cnt()));
	    delete col;
	    col = 0;
	    throw ibis::bad_alloc("incorrect number of bundles");
	}
    }
    else {
	ibis::util::logMessage("Error", "ibis::bundle1::ctor name \"%s\" "
			       "is not a column in table %s",
			       cmps.argName(0), tbl->name());
	throw ibis::bad_alloc("not a valid column name");
    }
    sort();

    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "query[" << q.id()
		    << "]::bundle1 -- generated the bundle\n";
	if (rids == 0) {
	    if ((1U << ibis::gVerbose) > col->size() ||
		ibis::gVerbose > 30)
		print(lg.buffer());
	}
	else if ((1U << ibis::gVerbose) > rids->size() ||
		 ibis::gVerbose > 30) {
	    if (ibis::gVerbose > 8)
		printAll(lg.buffer());
	    else
		print(lg.buffer());
	}
    }
} // ibis::bundle1::bundle1

/// Constructor.  It creates the bundle using the values passed in through
/// vals.  It will use the values in vals[0] and take on the first column
/// name in cmps that is not '*'.
ibis::bundle1::bundle1(const ibis::part& tbl, const ibis::selectClause& cmps,
		       const std::vector<void*>& vals)
    : bundle(cmps) {
    if (cmps.size() == 0 || vals.size() == 0)
	return;

    id = tbl.name();
    uint32_t icol = 0;
    while (icol < cmps.size() && *(cmps.argName(icol)) == '*') ++ icol;
    if (icol >= cmps.size()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- bundle1::ctor failed to locate a valid column "
	    "name in " << cmps;
	throw "bundle1::ctor can not find a column name";
    }
    ibis::column* c = tbl.getColumn(cmps.argName(icol));
    if (c != 0 && vals[0] != 0) {
	if (cmps.getAggregator(icol) == ibis::selectClause::NIL) {
	    // use column type
	    col = ibis::colValues::create(c, vals[0]);
	}
	else { // a function, treat AVG and SUM as double
	    switch (cmps.getAggregator(icol)) {
	    case ibis::selectClause::AVG:
	    case ibis::selectClause::SUM:
	    case ibis::selectClause::VARPOP:
	    case ibis::selectClause::VARSAMP:
	    case ibis::selectClause::STDPOP:
	    case ibis::selectClause::STDSAMP:
		col = new ibis::colDoubles(c, vals[0]);
		break;
	    default:
		col = ibis::colValues::create(c, vals[0]);
		break;
	    }
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
		<< "bundle1 constructor skipping a unknown column ("
		<< cmps.argName(0) << ") or a column without data ("
		<< vals[0] << ")";
	return;
    }
    sort();

    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "ibis::bundle1 -- generated the bundle for \"" << *cmps
		    << "\"\n";
	if ((1U << ibis::gVerbose) > col->size() || ibis::gVerbose > 30)
	    print(lg.buffer());
    }
} // ibis::bundle1::bundle1

// print out the bundles (without RIDs)
void ibis::bundle1::print(std::ostream& out) const {
    if (col == 0)
	return;

    uint32_t nbdl = col->size();
    if (ibis::gVerbose > 2)
	out << "Bundle1 " << id << " has " << nbdl
	    << (col->canSort() ? " distinct" : "")
	    << (nbdl > 1 ? " values" : " value")
	    << std::endl;
    if (starts != 0) {
	out << (*col)->name() << " (with counts)\n";
	for (uint32_t i=0; i < nbdl; ++i) {
	    col->write(out, i);
	    out << ",\t" << (*starts)[i+1] - (*starts)[i] << "\n";
	}
    }
    else {
	out << *comps << "\n";
	for (uint32_t i=0; i < nbdl; ++i) {
	    col->write(out, i);
	    out << "\n";
	}
    }
} // ibis::bundle1::print

// print out the bundles (with RIDs)
void ibis::bundle1::printAll(std::ostream& out) const {
    if (col == 0)
	return;

    if (rids != 0 && starts != 0) {
	ibis::util::ioLock lock;
	uint32_t nbdl = col->size();
	if (ibis::gVerbose > 2)
	    out << "Bundle " << id << " has " << nbdl
		<< (col->canSort() ? " distinct" : "")
		<< (nbdl > 1 ? " values" : " value")
		<< " from " << rids->size()
		<< (rids->size()>1 ? " rows" : " row") << std::endl;
	out << *comps << " : followed by RIDs\n";
	for (uint32_t i=0; i < nbdl; ++i) {
	    col->write(out, i);
	    out << ",\t";
	    for (uint32_t j=(*starts)[i]; j < (*starts)[i+1]; ++j)
		out << (*rids)[j] << (j+1<(*starts)[i+1] ? ", " : "\n");
	}
    }
    else {
	print(out);
	return;
    }
} // ibis::bundle1::printAll

/// Sort the rows.  Remove the duplicate elements and generate the
/// starts.
void ibis::bundle1::sort() {
    if (col == 0) return;

    const uint32_t nrow = col->size();
    col->nosharing();
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- bundle1[" << id << "]::sort starting with "
		    << nrow << " row" << (nrow > 1 ? "s" : "");
	for (uint32_t j = 0; j < nrow; ++ j) {
	    lg.buffer() << "\n";
	    col->write(lg.buffer(), j);
	}
    }
#endif

    if (nrow < 2) { // not much to do
	starts = new array_t<uint32_t>((uint32_t)2);
	(*starts)[1] = nrow;
	(*starts)[0] = 0;
    }
    else if (comps.getAggregator(0) == ibis::selectClause::NIL) {// no function
	// sort according to the values
	col->sort(0, nrow, this);
	// determine the starting positions of the identical segments
	starts = col->segment();

	uint32_t nGroups = starts->size() - 1;
	if (nGroups < nrow) {    // erase the dupliate elements
	    col->reduce(*starts);
	    if (rids && rids->size() == nrow) {
		for (uint32_t i=nGroups; i>0; --i)
		    sortRIDs((*starts)[i-1], (*starts)[i]);
	    }
	}
    }
    else { // a function is involved
	starts = new array_t<uint32_t>((uint32_t)2);
	(*starts)[1] = nrow;
	(*starts)[0] = 0;
	col->reduce(*starts, comps.getAggregator(0));
    }
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    if (ibis::gVerbose > 5) {
	const size_t nGroups = starts->size()-1;
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- bundle1[" << id << "]::sort ending "
		    << nGroups << " row" << (nGroups > 1 ? "s" : "");
	for (uint32_t j = 0; j < nGroups; ++ j) {
	    lg.buffer() << "\n";
	    col->write(lg.buffer(), j);
	}
    }
#endif
} // ibis::bundle1::sort

/// Change from ascending order to descending order.  Most lines of the
/// code deals with the re-ordering of the RIDs.
void ibis::bundle1::reverse() {
    if (col == 0 || starts == 0) return;
    if (starts->size() <= 2) return;
    const uint32_t ngroups = starts->size() - 1;

    col->nosharing();
    if (rids != 0) { // has a rid list
	array_t<uint32_t> cnts(ngroups);
	for (uint32_t i = 0; i < ngroups; ++ i)
	    cnts[i] = (*starts)[i+1] - (*starts)[i];
	for (uint32_t i = 0; i+i < ngroups; ++ i) {
	    const uint32_t j = ngroups - i - 1;
	    {
		uint32_t tmp;
		tmp = (*starts)[i];
		(*starts)[i] = (*starts)[j];
		(*starts)[j] = tmp;
	    }
	    {
		uint32_t tmp;
		tmp = cnts[i];
		cnts[i] = cnts[j];
		cnts[j] = tmp;
	    }
	    col->swap(i, j);
	}

	ibis::RIDSet tmpids;
	tmpids.reserve(rids->size());
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    for (uint32_t j = 0; j < cnts[i]; ++ j)
		tmpids.push_back((*rids)[(*starts)[i]+j]);
	}
	rids->swap(tmpids);
	(*starts)[0] = 0;
	for (uint32_t i = 0; i <= ngroups; ++ i)
	    (*starts)[i+1] = (*starts)[i] + cnts[i];
    }
    else {
	// turn starts into counts
	for (uint32_t i = 0; i < ngroups; ++ i)
	    (*starts)[i] = (*starts)[i+1] - (*starts)[i];
	for (uint32_t i = 0; i < ngroups/2; ++ i) {
	    const uint32_t j = ngroups-1-i;
	    uint32_t tmp = (*starts)[i];
	    (*starts)[i] = (*starts)[j];
	    (*starts)[j] = tmp;
	    col->swap(i, j);
	}
	// turn counts back into starts
	uint32_t sum = 0;
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    uint32_t tmp = (*starts)[i];
	    (*starts)[i] = sum;
	    sum += tmp;
	}
    }
} // ibis::bundle1::reverse

/// This single-argument version keep the first few records.
long ibis::bundle1::truncate(uint32_t keep) {
    if (col == 0 || starts == 0) return -2L;
    if (starts->size() <= 2) return -3L;
    const uint32_t ngroups = starts->size()-1;
    if (keep >= ngroups) return ngroups;
    if (keep == 0) {
	starts->clear();
	col->truncate(0);
	return 0;
    }

    if (rids != 0) {
        rids->resize((*starts)[keep]);
    }
    infile = false;
    starts->resize(keep+1);
    return col->truncate(keep);
} // ibis::bundle1::truncate

/// This two-argument version keeps a few records starting at a
/// user-specified row number.  Note that the row number starts with 0,
/// i.e., the first row has the row number 0.
long ibis::bundle1::truncate(uint32_t keep, uint32_t start) {
    if (col == 0 || starts == 0) return -2L;
    if (starts->size() <= 2) return -3L;
    const uint32_t ngroups = starts->size()-1;
    if (start >= ngroups || keep == 0) {
	starts->clear();
	col->truncate(0);
	return 0;
    }
    else if (keep >= ngroups && start == 0) {
	return ngroups;
    }

    const uint32_t end = (keep+start < ngroups ? keep+start : ngroups);
    keep = end - start;
    if (rids != 0) {
	rids->truncate((*starts)[end]-(*starts)[start], (*starts)[start]);
    }
    infile = false;
    starts->truncate(keep+1, start);
    if (start != 0) {
	const uint32_t offset = starts->front();
	for (array_t<uint32_t>::iterator it = starts->begin();
	     it != starts->end(); ++ it)
	    *it -= offset;
    }
    return col->truncate(keep, start);
} // ibis::bundle1::truncate

void ibis::bundle1::write(const ibis::query& theQ) const {
    if (theQ.dir() == 0) return;
    if (col == 0) return;
    if (infile) return;
    uint32_t tmp = col->size();
    if (starts->size() != tmp+1) {
	ibis::util::logMessage
	    ("Warning", "ibis::bundle1::write invalid bundle "
	     "(starts->size(%lu) != col->size(%lu)+1)",
	     static_cast<long unsigned>(starts->size()),
	     static_cast<long unsigned>(tmp));
	return;
    }

    if (rids != 0)
	theQ.writeRIDs(rids); // write the RIDs

    uint32_t len = strlen(theQ.dir());
    char* fn = new char[len+16];
    strcpy(fn, theQ.dir());
    strcat(fn, "bundles");
    FILE* fptr = fopen(fn, "wb");
    if (fptr == 0) {
	ibis::util::logMessage("Warning", "ibis::bundle1::write -- unable "
			       "to open file \"%s\" ... %s", fn,
			       (errno ? strerror(errno) :
				"no free stdio stream"));
	return;
    }

    int32_t ierr = fwrite(&tmp, sizeof(uint32_t), 1, fptr);
    tmp = 1;
    ierr = fwrite(&tmp, sizeof(uint32_t), 1, fptr);
    tmp = col->elementSize();
    ierr = fwrite(&tmp, sizeof(uint32_t), 1, fptr);
    ierr = col->write(fptr);
    ierr = fwrite(starts->begin(), sizeof(uint32_t), starts->size(), fptr);
    ierr = fclose(fptr);
    delete [] fn;
    infile = true;
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    sync();
#endif
} // ibis::bundle1::write

/// Retrieve the value of i-th row j-th column as a 32-bit integer.
/// Return the maximal value defined in the class numeric_limits if either
/// i or j is out of bounds.
int32_t ibis::bundle1::getInt(uint32_t i, uint32_t j) const {
    int32_t ret = 0x7FFFFFFF;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	ret = col->getInt(i);
    }
    return ret;
} // ibis::bundle1::getInt

/// Retrieve the value of i-th row j-th column as a 32-bit unsigned
/// integer.  Return the maximal value defined in the class numeric_limits
/// if either i or j is out of bounds.
uint32_t ibis::bundle1::getUInt(uint32_t i, uint32_t j) const {
    uint32_t ret = 0xFFFFFFFFU;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	ret = col->getUInt(i);
    }
    return ret;
} // ibis::bundle1::getUInt

/// Retrieve the value of i-th row j-th column as a 64-bit integer.
/// Return the maximal value defined in the class numeric_limits if either
/// i or j is out of bounds.
int64_t ibis::bundle1::getLong(uint32_t i, uint32_t j) const {
    int64_t ret = 0x7FFFFFFFFFFFFFFFLL;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	ret = col->getLong(i);
    }
    return ret;
} // ibis::bundle1::getLong

/// Retrieve the value of i-th row j-th column as a 64-bit unsigned
/// integer.  Return the maximal value defined in the class numeric_limits
/// if either i or j is out of bounds.
uint64_t ibis::bundle1::getULong(uint32_t i, uint32_t j) const {
    uint64_t ret = 0xFFFFFFFFFFFFFFFFULL;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	ret = col->getULong(i);
    }
    return ret;
} // ibis::bundle1::getULong

/// Retrieve the value of i-th row j-th column as a 32-bit floating-point
/// value.  Return the maximal value defined in the class numeric_limits if
/// wither i or j is out of bounds.
float ibis::bundle1::getFloat(uint32_t i, uint32_t j) const {
    float ret = FLT_MAX;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	ret = col->getFloat(i);
    }
    return ret;
} // ibis::bundle1::getFloat

/// Retrieve the value of i-th row j-th column as a 64-bit floating-point
/// value.  Return the maximal value defined in the class numeric_limits if
/// either i or j is out of bounds.
double ibis::bundle1::getDouble(uint32_t i, uint32_t j) const {
    double ret = DBL_MAX;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	ret = col->getDouble(i);
    }
    return ret;
} // ibis::bundle1::getDouble

/// Retrieve the value of i-th row j-th column as a string.  Returns an
/// empty string if either i or j is out of bounds, which can not be
/// distinguished from an actual empty string.  This function converts a
/// value to its string representation through @c std::ostringstream.
std::string ibis::bundle1::getString(uint32_t i, uint32_t j) const {
    std::ostringstream oss;
    if (col != 0 && i < col->size() && j == 0) { // indices i and j are valid
	col->write(oss, i);
    }
    return oss.str();
} // ibis::bundle1::getString

//////////////////////////////////////////////////////////////////////
// functions of ibis::bundles
//
/// Constructor.  It will use the hit vector of the query to generate a new
/// bundle, if it is not able to read the existing bundles.
ibis::bundles::bundles(const ibis::query& q) : bundle(q) {
    if (q.getNumHits() == 0)
	return;

    char bdlfile[PATH_MAX];
    const ibis::part* tbl = q.partition();
    if (q.dir() != 0) {
	strcpy(bdlfile, q.dir());
	strcat(bdlfile, "bundles");
    }
    else {
	bdlfile[0] = 0;
    }
    const ibis::selectClause& cmps = q.components();
    const uint32_t ncol = cmps.size();
    if (q.dir() != 0 && ibis::util::getFileSize(bdlfile) > 0) {
	// file bundles exists, attempt to read in its content
	if (rids == 0) {
	    rids = q.readRIDs();
	    if (rids != 0 && static_cast<long>(rids->size()) !=
		q.getNumHits()) {
		delete rids;
		rids = 0;
	    }
	}
	ibis::fileManager::storage* bdlstore=0;
	int ierr = ibis::fileManager::instance().getFile(bdlfile, &bdlstore);
	if (ierr != 0) {
	    ibis::util::logMessage("Error", "ibis::bundles::ctor failed "
				   "to retrieve bundle file %s",
				   bdlfile);
	    throw ibis::bad_alloc("failed to retrieve bundle file");
	}
	// no need to explicitly call beginUse() because array_t sizes will
	// hold a read lock long enough until starts holds another one for
	// the life time of this object
	array_t<uint32_t> sizes(bdlstore, 0, ncol+2);
	uint32_t expected = sizeof(uint32_t)*(3+sizes[0]+sizes[1]);
	for (uint32_t i = 2; i < 2+ncol; ++i)
	    expected += sizes[i] * sizes[0];
	if (ncol == sizes[1] && expected == bdlstore->bytes()) {
	    // go through every selected column to construct the colValues
	    uint32_t start = sizeof(uint32_t)*(ncol+2);
	    for (uint32_t i=0; i < ncol; ++i) {
		const ibis::column* cptr = tbl->getColumn(cmps.argName(i));
		if (cptr != 0) {
		    ibis::colValues* tmp;
		    switch (cmps.getAggregator(i)) {
		    case ibis::selectClause::AVG:
		    case ibis::selectClause::SUM:
	    	    case ibis::selectClause::VARPOP:
	            case ibis::selectClause::VARSAMP:
	            case ibis::selectClause::STDPOP:
	            case ibis::selectClause::STDSAMP:
			tmp = new ibis::colDoubles
			    (cptr, bdlstore, start, sizes[0]);
			break;
		    default:
			tmp = ibis::colValues::create
			    (cptr, bdlstore, start, sizes[0]);
			break;
		    }
		    cols.push_back(tmp);
		    start += sizes[2+i] * sizes[0];
		}
		else {
		    ibis::util::logMessage("Error", "ibis::bundles::ctor "
					   "\"%s\" is not the name of a "
					   "column in table %s",
					   cmps.argName(i), tbl->name());
		    throw ibis::bad_alloc("unknown column name");
		}
	    }
	    starts = new ibis::array_t<uint32_t>(bdlstore, start, sizes[0]+1);
	    infile = true;
	}
	else {
	    ibis::util::logMessage
		("Warning", "ibis::bundles::ctor -- according to the "
		 "header, %lu bytes are expected, but the file %s "
		 "contains %lu", static_cast<long unsigned>(expected),
		 bdlfile, static_cast<long unsigned>(bdlstore->bytes()));
	}
    }

    if (starts == 0) {
	// use the current hit vector of the query to generate the bundle
	const ibis::bitvector* hits = q.getHitVector();
	if (hits != 0) {
	    if (rids == 0) {
		rids = tbl->getRIDs(*hits);
		if (rids != 0 && rids->size() != hits->cnt()) {
		    delete rids;
		    rids = 0;
		}
	    }
	}
	else {
	    ibis::util::logMessage("Error", "ibis::bundles::ctor -- query %s"
				   " contains an invalid hit vector, call "
				   "evaluate to generate a valid hit vector",
				   q.id());
	    throw ibis::bad_alloc("ibis::bundles::ctor -- no hit vector");
	}
	for (uint32_t i=0; i < ncol; ++i) {
	    const ibis::column* cptr = tbl->getColumn(cmps.argName(i));
	    if (cptr != 0) {
		ibis::colValues* tmp;
		switch (cmps.getAggregator(i)) {
		case ibis::selectClause::AVG:
		case ibis::selectClause::SUM:
	    	case ibis::selectClause::VARPOP:
	        case ibis::selectClause::VARSAMP:
	        case ibis::selectClause::STDPOP:
	        case ibis::selectClause::STDSAMP:
		    tmp = new ibis::colDoubles(cptr, *hits);
		    break;
		default:
		    tmp = ibis::colValues::create(cptr, *hits);
		    break;
		}
		cols.push_back(tmp);
	    }
	    else {
		ibis::util::logMessage("Error", "ibis::bundles::ctor "
				       "\"%s\" is not the name of a "
				       "column in table %s",
				       cmps.argName(i), tbl->name());
		throw ibis::bad_alloc("unknown column name");
	    }
	}

	sort();
    }

    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "query[" << q.id()
		    << "]::bundles -- generated the bundle\n";
	if (rids == 0) {
	    if ((1U << ibis::gVerbose) > cols[0]->size() ||
		ibis::gVerbose > 30)
		print(lg.buffer());
	}
	else if ((1U << ibis::gVerbose) > rids->size() ||
		 ibis::gVerbose > 30) {
	    if (ibis::gVerbose > 8)
		printAll(lg.buffer());
	    else
		print(lg.buffer());
	}
    }
} // ibis::bundles::bundles

/// Constructor.  It creates a bundle using the hits provided by the caller
/// (instead of from the query object q).
ibis::bundles::bundles(const ibis::query& q, const ibis::bitvector& hits)
    : bundle(q, hits) {
    if (hits.cnt() == 0)
	return;

    // need to retrieve the named columns
    const ibis::part* tbl = q.partition();
    const ibis::selectClause& cmps = q.components();
    const uint32_t ncol = cmps.size();
    for (uint32_t i=0; i < ncol; ++i) {
	const ibis::column* cptr = tbl->getColumn(cmps.argName(i));
	if (cptr != 0) {
	    ibis::colValues* tmp;
	    switch (cmps.getAggregator(i)) {
	    case ibis::selectClause::AVG:
	    case ibis::selectClause::SUM:
	    case ibis::selectClause::VARPOP:
	    case ibis::selectClause::VARSAMP:
	    case ibis::selectClause::STDPOP:
	    case ibis::selectClause::STDSAMP:
		tmp = new ibis::colDoubles(cptr, hits);
		break;
	    default:
		tmp = ibis::colValues::create(cptr, hits);
		break;
	    }
	    cols.push_back(tmp);
	}
	else {
	    ibis::util::logMessage("Error", "ibis::bundles::ctr \"%s\" is "
				   "not the name of a column in table %s",
				   cmps[i], tbl->name());
	    throw ibis::bad_alloc("unknown column name");
	}
    }
    if (rids == 0) {
	rids = tbl->getRIDs(hits);
	if (rids != 0 && rids->size() != hits.cnt()) {
	    delete rids;
	    rids = 0;
	}
    }
    sort();

    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "query[" << q.id()
		    << "]::bundle1 -- generated the bundle\n";
	if (rids == 0) {
	    if ((1U << ibis::gVerbose) > cols[0]->size() ||
		ibis::gVerbose > 30)
		print(lg.buffer());
	}
	else if ((1U << ibis::gVerbose) > rids->size() ||
		 ibis::gVerbose > 30) {
	    if (ibis::gVerbose > 8)
		printAll(lg.buffer());
	    else
		print(lg.buffer());
	}
    }
} // ibis::bundles::bundles

/// Constructor.  It creates a bundle from the values provided by the
/// caller.
ibis::bundles::bundles(const ibis::part& tbl, const ibis::selectClause& cmps,
		       const std::vector<void*>& vals)
    : bundle(cmps) {
    id = tbl.name();
    for (unsigned ic = 0,  iv = 0; ic < cmps.size() && iv < vals.size();
	 ++ ic) {
	const char* cn = cmps.argName(ic);
	ibis::column* c = tbl.getColumn(cn);
	if (c != 0) {
	    if (vals[iv] != 0) {
		ibis::colValues* cv = 0;
		switch (cmps.getAggregator(ic)) {
		case ibis::selectClause::AVG:
		case ibis::selectClause::SUM:
		case ibis::selectClause::VARPOP:
		case ibis::selectClause::VARSAMP:
		case ibis::selectClause::STDPOP:
		case ibis::selectClause::STDSAMP:
		    cv = new ibis::colDoubles(c, vals[iv]);
		    break;
		default:
		    cv = ibis::colValues::create(c, vals[iv]);
		    break;
		}
		if (cv != 0)
		    cols.push_back(cv);
		LOGGER(cv == 0 && ibis::gVerbose > 1)
		    << "Warning -- bundles(" << tbl.name() << ", "
		    << cmps << ", vals[" << vals.size()
		    << "]) failed to create colValues object from column "
		    << c->name() << " and vals[" << iv << "]";
	    }
	    else {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- bundles(" << tbl.name() << ", "
		    << cmps << ", vals[" << vals.size()
		    << "]) can not associate column " << c->name()
		    << " with a nil pointer for values";
	    }
	    ++ iv;
	}
	else if (*cn != '*') {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- bundles(" << tbl.name() << ", " << cmps
		<< ", vals[" << vals.size() << "]) can not find a column named "
		<< (cn ? cn : "");
	}
    }
    sort();

    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "ibis::bundles -- generated the bundle for \"" << *cmps
		    << "\"\n";
	if ((1U << ibis::gVerbose) > cols.size() || ibis::gVerbose > 30)
	    print(lg.buffer());
    }
} // ibis::bundles::bundles

// print out the bundles (no RIDs)
void ibis::bundles::print(std::ostream& out) const {
    // caller must hold an ioLock ibis::util::ioLock lock;
    const uint32_t ncol = cols.size();
    const uint32_t size = (cols[0] != 0 ? cols[0]->size() : 0);
    bool distinct = true;
    for (uint32_t i = 0; i < ncol && distinct; ++ i) {
	if (cols[i] == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- bundles::print can not proceed because cols["
		<< i << "] is nil";
	    return;
	}
	distinct = cols[i]->canSort();
    }
    if (ibis::gVerbose > 2)
	out << "Bundle " << id << " contains " << size
	    << (distinct ? " distinct " : " ") << ncol << "-tuple"
	    << (size > 1 ? "s" : "") << std::endl;
    if (starts != 0) {
	for (uint32_t i = 0; i < ncol; ++ i) {
	    if (i > 0) out << ", ";
	    out << (*(cols[i]))->name();
	}
	out << " (with counts)\n";
	for (uint32_t i=0; i<size; ++i) {
	    for (uint32_t ii=0; ii<ncol; ++ii) {
		cols[ii]->write(out, i);
		out << ", ";
	    }
	    out << "\t" << (*starts)[i+1]-(*starts)[i] << "\n";
	}
    }
    else {
	out << *comps << "\n";
	for (uint32_t i=0; i<size; ++i) {
	    for (uint32_t ii=0; ii<ncol; ++ii) {
		cols[ii]->write(out, i);
		out << (ii+1<ncol ? ", " : "\n");
	    }
	}
    }
} // ibis::bundles::print

// print out the bundles (with RIDs)
void ibis::bundles::printAll(std::ostream& out) const {
    if (rids == 0 || starts == 0) {
	print(out);
	return;
    }

    bool distinct = true;
    const uint32_t ncol = cols.size();
    for (uint32_t i = 0; i < ncol && distinct; ++ i) {
	if (cols[i] == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- bundles::printAll can not proceed because cols["
		<< i << "] is nil";
	    return;
	}
	distinct = cols[i]->canSort();
    }
    const uint32_t size = cols[0]->size();
    ibis::util::ioLock lock;
    if (ibis::gVerbose > 2)
	out << "Bundle " << id << " contains " << size
	    << (distinct ? " distinct " : " ") << ncol << "-tuple"
	    << (size > 1 ? "s" : "") << " from "
	    << rids->size() << (rids->size()>1 ? " rows" : " row")
	    << std::endl;
    out << *comps << "\n";
    for (uint32_t i=0; i<size; ++i) {
	for (uint32_t ii=0; ii<ncol; ++ii) {
	    cols[ii]->write(out, i);
	    out << ", ";
	}
	out << ",\t";
	for (uint32_t j=(*starts)[i]; j < (*starts)[i+1]; ++j) {
	    out << (*rids)[j] << (j+1<(*starts)[i+1] ? ", " : "\n");
	}
    }
} // ibis::bundles::printAll

/// Sort the columns.  Remove the duplicate elements and generate the
/// starts.  This function allows aggregation functions to appear in
/// arbitrary positions in the select clause.
void ibis::bundles::sort() {
    const uint32_t ncol = cols.size();
    const uint32_t nplain = comps.nPlain();
    const uint32_t nHits = (cols[0] != 0 ? cols[0]->size() : 0);
    uint32_t nGroups = nHits;
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- bundles[" << id << "]::sort starting with "
		    << ncol << " columns and " << nHits << " row"
		    << (nHits > 1 ? "s" : "");
	for (uint32_t j = 0; j < nHits; ++ j) {
	    lg.buffer() << "\n";
	    for (uint32_t i = 0; i < ncol; ++ i) {
		if (i > 0) lg.buffer() << ", ";
		cols[i]->write(lg.buffer(), j);
	    }
	}
    }
#endif
    if (nHits < 2) { // not much to do
	starts = new ibis::array_t<uint32_t>(2);
	(*starts)[1] = nHits;
	(*starts)[0] = 0;
    }
    else if (nplain == ncol) { // no functions
	for (uint32_t i = 0; i < ncol; ++ i)
	    cols[i]->nosharing();
	// sort according to the values of the first column
	cols[0]->sort(0, nHits, this, cols.begin()+1, cols.end());
	starts = cols[0]->segment();

	nGroups = starts->size() - 1;
	// go through the rest of the columns if necessary
	for (uint32_t i=1; i<ncol && nGroups<nHits; ++i) {
	    uint32_t i1 = i + 1;
	    for (uint32_t i2=0; i2<nGroups; ++i2) { // sort one group at a time
		cols[i]->sort((*starts)[i2], (*starts)[i2+1], this,
			      cols.begin()+i1, cols.end());
	    }
	    array_t<uint32_t>* tmp = cols[i]->segment(starts);
	    delete starts;
	    starts = tmp;
	    nGroups = starts->size() - 1;
	}

	if (nGroups < nHits) {// erase the dupliate elements
	    for (uint32_t i2=0; i2<ncol; ++i2)
		cols[i2]->reduce(*starts);
	}
    }
    else if (nplain == 0) { // no column to sort
	for (uint32_t i = 0; i < ncol; ++ i)
	    cols[i]->nosharing();
	delete starts;
	starts = new array_t<uint32_t>(2);
	(*starts)[0] = 0;
	(*starts)[1] = nHits;
	nGroups = 1;
	for (uint32_t i = 0; i < ncol; ++ i) {
	    cols[i]->reduce(*starts, comps.getAggregator(i));
	}
    }
    else { // one or more columns to sort
	for (uint32_t i = 0; i < ncol; ++ i)
	    cols[i]->nosharing();

	ibis::colList cols2(ncol);
	std::vector<ibis::selectClause::AGREGADO> ops(ncol);
	// move aggregation functions to the end of the list
	for (uint32_t i1 = 0, i2 = 0, iplain = 0, iaggr = nplain;
	     i1 < comps.size(); ++ i1) {
	    if (comps.getAggregator(i1) == ibis::selectClause::NIL) {
		cols2[iplain] = cols[i2];
		ops[iplain] = ibis::selectClause::NIL;
		++ iplain;
		++ i2;
	    }
	    else if (strcmp(comps.argName(i1), "*") != 0) {
		cols2[iaggr] = cols[i2];
		ops[iaggr] = comps.getAggregator(i1);
		++ iaggr;
		++ i2;
	    }
	}
	cols2.swap(cols);

 	// sort according to the values of the first column
	cols[0]->sort(0, nHits, this, cols.begin()+1, cols.end());
	starts = cols[0]->segment();
	nGroups = starts->size() - 1;

	// go through the rest of the columns if necessary
	for (uint32_t i=1; i<nplain && nGroups<nHits; ++i) {
	    uint32_t i1 = i + 1;
	    for (uint32_t i2=0; i2<nGroups; ++i2) { // sort one group at a time
		cols[i]->sort((*starts)[i2], (*starts)[i2+1], this,
			      cols.begin()+i1, cols.end());
	    }
	    array_t<uint32_t>* tmp = cols[i]->segment(starts);
	    delete starts;
	    starts = tmp;
	    nGroups = starts->size() - 1;
	}

	if (nGroups < nHits) {// erase the dupliate elements
	    for (uint32_t i2 = 0; i2 < nplain; ++ i2) {
		cols[i2]->reduce(*starts);
	    }
	    for (uint32_t i2 = nplain; i2 < ncol; ++ i2) {
		cols[i2]->reduce(*starts, ops[i2]);
	    }
	}

	// restore the input order of the columns
	cols2.swap(cols);
   }

    // sort RIDs and perform sanity check
    if (nGroups < nHits && rids && rids->size() == nHits) {
	for (uint32_t i1=nGroups; i1>0; --i1)
	    sortRIDs((*starts)[i1-1], (*starts)[i1]);
    }
    for (uint32_t i1 = 0; i1 < ncol; ++i1) {
	if (cols[i1]->size() != nGroups) {
	    ibis::util::logMessage
		("Warning", "bundles::sort -- column # %lu (%s) is expected "
		 "to have %lu values, but it actually has %lu",
		 static_cast<long unsigned>(i1), (*(cols[i1]))->name(),
		 static_cast<long unsigned>(nGroups),
		 static_cast<long unsigned>(cols[i1]->size()));
	}
    }
#if _DEBUG+0>2 || DEBUG+0>1
    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- bundles[" << id << "]::sort ending "
		    << ncol << " columns and " << nGroups << " row"
		    << (nGroups > 1 ? "s" : "");
	for (uint32_t j = 0; j < nGroups; ++ j) {
	    lg.buffer() << "\n";
	    for (uint32_t i = 0; i < ncol; ++ i) {
		if (i > 0) lg.buffer() << ", ";
		cols[i]->write(lg.buffer(), j);
	    }
	}
    }
#endif
} // ibis::bundles::sort

/// Reorder the bundles according to the keys (names) given.  If the
/// argument direction is a negative number, the rows are reversed after
/// sorting.  If no sorting is done, the reversal of rows is still
/// performed.
void ibis::bundles::reorder(const char *names, int direction) {
    if (names == 0 || *names == 0) return;
    if (starts == 0) return;
    if (starts->size() <= 2) return; // one group, no need to sort

    ibis::nameList sortkeys; // the new keys for sorting
    sortkeys.select(names); // preserve the order of the sort keys

    bool nosort = true;
    for (unsigned j = 0; nosort && j < sortkeys.size() && j < cols.size(); ++ j)
	nosort = (stricmp(sortkeys[j], comps.argName(j)) == 0);
    if (nosort) { // no need to sort
	if (direction < 0)
	    reverse();
	return;
    }
    // make sure all columns are ready for modification
    for (uint32_t i = 0; i < cols.size(); ++ i)
	cols[i]->nosharing();

    // verify the variable names.  Note that for functions, it only looks
    // at the attribute names not the actual funtion to match with the
    // select clause, this is not a complete verification.
    const uint32_t ngroups = starts->size() - 1;
    if (rids != 0) {
	// turn a single list of RIDs into a number of smaller lists so
	// that the smaller lists can be re-ordered along with the other
	// values
	array_t< ibis::RIDSet* > rid2;
	rid2.reserve(ngroups);
	for (uint32_t i = 0; i < ngroups; ++ i)
	    rid2.push_back(new array_t<ibis::rid_t>
			   (*rids, (*starts)[i], (*starts)[i+1]-(*starts)[i]));

	if (sortkeys.size() > 1) { // multiple keys
	    array_t<uint32_t> gb;
	    gb.reserve(ngroups);
	    gb.push_back(0);
	    gb.push_back(ngroups);
	    for (uint32_t i = 0;
		 i < sortkeys.size() && gb.size() <= ngroups;
		 ++ i) {
		const uint32_t j = comps.find(sortkeys[i]);
		if (j >= comps.size()) continue;

		array_t<uint32_t> ind0; // indices over all ngroups
		ind0.reserve(ngroups);
		for (uint32_t g = 0; g < gb.size()-1; ++ g) {
		    if (gb[g+1] > gb[g]+1) { // more than one group
			array_t<uint32_t> ind1; // indices for group g
			cols[j]->sort(gb[g], gb[g+1], ind1);
			ind0.insert(ind0.end(), ind1.begin(), ind1.end());
		    }
		    else { // a single group
			ind0.push_back(gb[g]);
		    }
		}
		for (uint32_t k = 0; k < cols.size(); ++ k)
		    cols[k]->reorder(ind0);
		ibis::util::reorder(rid2, ind0);

		{
		    array_t<uint32_t> *tmp = cols[j]->segment(&gb);
		    gb.swap(*tmp);
		    delete tmp;
		}
	    }

	    if (direction < 0) { // reverse the order
		for (uint32_t j = 0; j < cols.size(); ++ j)
		    for (uint32_t i = 0; i < ngroups/2; ++ i)
			cols[j]->swap(i, ngroups-1-i);
		for (uint32_t i = 0; i < ngroups/2; ++ i) {
		    const uint32_t j = ngroups - 1 - i;
		    ibis::RIDSet *tmp = rid2[i];
		    rid2[i] = rid2[j];
		    rid2[j] = tmp;
		}
	    }
	}
	else { // a single key
	    const uint32_t j = comps.find(sortkeys[0]);
	    if (j < comps.size()) {
		array_t<uint32_t> ind;
		cols[j]->sort(0, ngroups, ind);
		if (direction < 0) { // reverse the order of ind
		    for (uint32_t i = 0; i < ngroups/2; ++ i) {
			const uint32_t itmp = ind[i];
			ind[i] = ind[ngroups-1-i];
			ind[ngroups-1-i] = itmp;
		    }
		}
		for (uint32_t i = 0; i < cols.size(); ++ i)
		    cols[i]->reorder(ind);
		ibis::util::reorder(rid2, ind);
	    }
	}

	// time to put the smaller lists together again, and update starts
	ibis::RIDSet rid1;
	rid1.reserve(rids->size());
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    rid1.insert(rid1.end(), rid2[i]->begin(), rid2[i]->end());
	    (*starts)[i+1] = (*starts)[i] + rid2[i]->size();
	    delete rid2[i];
	}
	rids->swap(rid1);
    }
    else { // no rids
	// turn starts into counts
	for (uint32_t i = 0; i < ngroups; ++ i)
	    (*starts)[i] = (*starts)[i+1] - (*starts)[i];
	starts->resize(ngroups);
	if (sortkeys.size() > 1) {
	    ibis::array_t<uint32_t> gb;
	    gb.reserve(ngroups);
	    gb.push_back(0);
	    gb.push_back(ngroups);
	    for (uint32_t i = 0;
		 i < sortkeys.size() && gb.size() <= ngroups;
		 ++ i) {
		const uint32_t j = comps.find(sortkeys[i]);
		if (j >= comps.size()) continue;

		ibis::array_t<uint32_t> ind0; // indices over all ngroups
		ind0.reserve(ngroups);
		for (uint32_t g = 0; g < gb.size()-1; ++ g) {
		    if (gb[g+1] > gb[g]+1) { // more than one group
			ibis::array_t<uint32_t> ind1; // indices for group g
			cols[j]->sort(gb[g], gb[g+1], ind1);
			ind0.insert(ind0.end(), ind1.begin(), ind1.end());
		    }
		    else { // a single group
			ind0.push_back(gb[g]);
		    }
		}
		for (uint32_t k = 0; k < cols.size(); ++ k)
		    cols[k]->reorder(ind0);
		ibis::util::reorder(*(starts), ind0);

		{
		    ibis::array_t<uint32_t> *tmp = cols[j]->segment(&gb);
		    gb.swap(*tmp);
		    delete tmp;
		}
	    }

	    if (direction < 0) { // reverse the order
		for (uint32_t j = 0; j < cols.size(); ++ j)
		    for (uint32_t i = 0; i < ngroups/2; ++ i)
			cols[j]->swap(i, ngroups-1-i);
		for (uint32_t i = 0; i < ngroups/2; ++ i) {
		    const uint32_t j = ngroups - 1 - i;
		    const uint32_t tmp = (*starts)[i];
		    (*starts)[i] = (*starts)[j];
		    (*starts)[j] = tmp;
		}
	    }
	}
	else {
	    const uint32_t j = comps.find(sortkeys[0]);
	    if (j < comps.size()) {
		ibis::array_t<uint32_t> ind;
		cols[j]->sort(0, ngroups, ind);
		if (direction < 0) { // reverse the order of ind
		    for (uint32_t i = 0; i < ngroups/2; ++ i) {
			const uint32_t itmp = ind[i];
			ind[i] = ind[ngroups-1-i];
			ind[ngroups-1-i] = itmp;
		    }
		}
		for (uint32_t i = 0; i < cols.size(); ++ i)
		    cols[i]->reorder(ind);
		ibis::util::reorder(*(starts), ind);
	    }
	}

	/// turn counts back into starting positions (starts)
	uint32_t cumu = 0;
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    uint32_t tmp = (*starts)[i];
	    (*starts)[i] = cumu;
	    cumu += tmp;
	}
	starts->push_back(cumu);
    }
    // new content, definitely not in file
    infile = false;
} // ibis::bundles::reorder

// Change from ascending order to descending order.  Most lines of the code
// deals with the re-ordering of the RIDs.
void ibis::bundles::reverse() {
    if (starts == 0) return;
    if (starts->size() <= 2) return;
    const uint32_t ngroups = starts->size() - 1;

    // make sure all columns are ready for modification
    for (uint32_t i = 0; i < cols.size(); ++ i)
	cols[i]->nosharing();
    if (rids != 0) { // has a rid list
	array_t<uint32_t> cnts(ngroups);
	for (uint32_t i = 0; i < ngroups; ++ i)
	    cnts[i] = (*starts)[i+1] - (*starts)[i];
	for (uint32_t i = 0; i+i < ngroups; ++ i) {
	    const uint32_t j = ngroups - i - 1;
	    {
		uint32_t tmp;
		tmp = (*starts)[i];
		(*starts)[i] = (*starts)[j];
		(*starts)[j] = tmp;
	    }
	    {
		uint32_t tmp;
		tmp = cnts[i];
		cnts[i] = cnts[j];
		cnts[j] = tmp;
	    }
	    for (ibis::colList::iterator it = cols.begin();
		 it != cols.end(); ++ it)
		(*it)->swap(i, j);
	}

	ibis::RIDSet tmpids;
	tmpids.reserve(rids->size());
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    for (uint32_t j = 0; j < cnts[i]; ++ j)
		tmpids.push_back((*rids)[(*starts)[i]+j]);
	}
	rids->swap(tmpids);
	(*starts)[0] = 0;
	for (uint32_t i = 0; i < ngroups; ++ i)
	    (*starts)[i+1] = (*starts)[i] + cnts[i];
    }
    else {
	for (ibis::colList::iterator it = cols.begin();
	     it != cols.end(); ++ it)
	    for (uint32_t i = 0; i < ngroups/2; ++ i)
		(*it)->swap(i, ngroups-1-i);
	// turn starts into counts
	for (uint32_t i = 0; i < ngroups; ++ i)
	    (*starts)[i] = (*starts)[i+1] - (*starts)[i];
	// swap counts
	for (uint32_t i = 0; i < ngroups/2; ++ i) {
	    const uint32_t j = ngroups - 1 - i;
	    const uint32_t tmp = (*starts)[i];
	    (*starts)[i] = (*starts)[j];
	    (*starts)[j] = tmp;
	}
	// turn counts back into starts
	uint32_t cumu = 0;
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    const uint32_t tmp = (*starts)[i];
	    (*starts)[i] = cumu;
	    cumu += tmp;
	}
	if (cumu != (*starts)[ngroups] && ibis::gVerbose >= 0) {
	    ibis::util::logMessage
		("Warning", "ibis::bundles::reverse internal error, "
		 "cumu (%lu) and (*starts)[%lu] (%lu) are expected to "
		 "be equal but are not", static_cast<long unsigned>(cumu),
		 static_cast<long unsigned>(ngroups),
		 static_cast<long unsigned>((*starts)[ngroups]));
	}
    }
} // ibis::bundles::reverse

/// This single-arugment version of the function truncate keeps the first
/// few rows.
long ibis::bundles::truncate(uint32_t keep) {
    if (starts == 0) return -2L;
    if (starts->size() <= 2) return -3L;
    const uint32_t ngroups = starts->size() - 1;
    if (ngroups <= keep)
	return ngroups;

    if (rids != 0)
	rids->resize((*starts)[keep]);
    starts->resize(keep+1);
    for (uint32_t i = 0; i < cols.size(); ++ i)
	cols[i]->truncate(keep);
    infile = false;
    return keep;
} // ibis::bundles::truncate

/// This two-argument version of the function keeps a few rows after a
/// specified starting point.
long ibis::bundles::truncate(uint32_t keep, uint32_t start) {
    if (cols.empty() || starts == 0) return -2L;
    if (starts->size() <= 2) return -3L;
    const uint32_t ngroups = starts->size()-1;
    if (start >= ngroups || keep == 0) {
	starts->clear();
	for (uint32_t i = 0; i < cols.size(); ++ i)
	    cols[i]->truncate(0);
	return 0;
    }
    else if (keep >= ngroups && start == 0) {
	return ngroups;
    }

    const uint32_t end = (keep+start < ngroups ? keep+start : ngroups);
    keep = end - start;
    if (rids != 0) {
	rids->truncate((*starts)[end]-(*starts)[start], (*starts)[start]);
    }
    infile = false;
    starts->truncate(keep+1, start);
    if (start != 0) {
	const uint32_t offset = starts->front();
	for (array_t<uint32_t>::iterator it = starts->begin();
	     it != starts->end(); ++ it)
	    *it -= offset;
    }
    for (uint32_t i = 0; i < cols.size(); ++ i)
	cols[i]->truncate(keep, start);
    return keep;
} // ibis::bundles::truncate

/// Reorder the bundles according to the keys (names) given.  Keep only the
/// first @c keep elements.  If @c direction < 0, keep the largest ones,
/// otherwise keep the smallest ones.
long ibis::bundles::truncate(const char *names, int direction, uint32_t keep) {
    if (names == 0 || *names == 0) return -1L;
    if (starts == 0) return -2L;
    if (starts->size() <= 2) return -3L;
    if (keep == 0) return -4L;

    ibis::nameList sortkeys; // the new keys for sorting
    sortkeys.select(names); // preserve the order of the sort keys
    if (sortkeys.size() == 0) {
	if (direction < 0)
	    reverse();
	return size();
    }

    // make sure all columns are ready for modification
    for (uint32_t i = 0; i < cols.size(); ++ i)
	cols[i]->nosharing();
    // Note that for functions, it only looks at the attribute names not
    // the actual funtion to match with the select clause, this is not a
    // complete verification.
    uint32_t ngroups = starts->size() - 1;
    if (rids != 0) {
	// turn a single list of RIDs into a number of smaller lists so
	// that the smaller lists can be re-ordered along with the other
	// values
	array_t< ibis::RIDSet* > rid2;
	rid2.reserve(ngroups);
	for (uint32_t i = 0; i < ngroups; ++ i)
	    rid2.push_back(new array_t<ibis::rid_t>
			   (*rids, (*starts)[i], (*starts)[i+1]-(*starts)[i]));

	if (sortkeys.size() > 1) {
	    array_t<uint32_t> gb;
	    uint32_t i = 0;
	    uint32_t j = comps.find(sortkeys[0]);
	    while (j >= comps.size() && i < sortkeys.size()) {
		++ i;
		j = comps.find(sortkeys[i]);
	    }
	    if (i >= sortkeys.size())
		return truncate(keep);

	    array_t<uint32_t> ind0; // indices over all ngroups
	    ind0.reserve(keep);
	    // deal with the first sort key
	    if (direction >= 0)
		cols[j]->bottomk(keep, ind0);
	    else
		cols[j]->topk(keep, ind0);
	    for (uint32_t ii = 0; ii < cols.size(); ++ ii)
		cols[ii]->reorder(ind0);
	    ibis::util::reorder(rid2, ind0);
	    ngroups = ind0.size();
	    { // segment cols[j]
		array_t<uint32_t> *tmp = cols[j]->segment(&gb);
		gb.swap(*tmp);
		delete tmp;
	    }

	    for (++ i; // starting with 
		 i < sortkeys.size() && gb.size() <= ngroups;
		 ++ i) {
		j = comps.find(sortkeys[i]);
		if (j >= comps.size()) continue;

		for (uint32_t g = 0; g < gb.size()-1; ++ g) {
		    if (gb[g+1] > gb[g]+1) { // more than one group
			array_t<uint32_t> ind1; // indices for group g
			cols[j]->sort(gb[g], gb[g+1], ind1);
			ind0.insert(ind0.end(), ind1.begin(), ind1.end());
		    }
		    else { // a single group
			ind0.push_back(gb[i]);
		    }
		}
		for (uint32_t k = 0; k < cols.size(); ++ k)
		    cols[k]->reorder(ind0);
		ibis::util::reorder(rid2, ind0);

		{
		    array_t<uint32_t> *tmp = cols[j]->segment(&gb);
		    gb.swap(*tmp);
		    delete tmp;
		}
	    }
	}
	else {
	    const uint32_t j = comps.find(sortkeys[0]);
	    if (j < comps.size()) {
		array_t<uint32_t> ind;
		if (direction >= 0)
		    cols[j]->bottomk(keep, ind);
		else
		    cols[j]->topk(keep, ind);
		for (uint32_t i = 0; i < cols.size(); ++ i)
		    cols[i]->reorder(ind);
		ibis::util::reorder(rid2, ind);
		ngroups = ind.size();
	    }
	}

	if (direction < 0) { // reverse the order
	    for (uint32_t j = 0; j < cols.size(); ++ j)
		for (uint32_t i = 0; i < ngroups/2; ++ i)
		    cols[j]->swap(i, ngroups-1-i);
	    for (uint32_t i = 0; i < ngroups/2; ++ i) {
		const uint32_t j = ngroups - 1 - i;
		ibis::RIDSet *tmp = rid2[i];
		rid2[i] = rid2[j];
		rid2[j] = tmp;
	    }
	}

	// time to put the smaller lists together again, also updates starts
	ibis::RIDSet rid1;
	rid1.reserve(rids->size());
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    rid1.insert(rid1.end(), rid2[i]->begin(), rid2[i]->end());
	    (*starts)[i+1] = (*starts)[i] + rid2[i]->size();
	    delete rid2[i];
	}
	rids->swap(rid1);
    }
    else { // no rids
	// turn starts into counts
	for (uint32_t i = 0; i < ngroups; ++ i)
	    (*starts)[i] = (*starts)[i+1] - (*starts)[i];
	starts->resize(ngroups);
	if (sortkeys.size() > 1) {
	    array_t<uint32_t> gb;
	    uint32_t i = 0;
	    uint32_t j0 = comps.find(sortkeys[0]);
	    while (j0 >= comps.size() && i < sortkeys.size()) {
		++ i;
		j0 = comps.find(sortkeys[i]);
	    }
	    if (i >= sortkeys.size())
		return truncate(keep);

	    array_t<uint32_t> ind0; // indices over all ngroups
	    ind0.reserve(keep);
	    // deal with the first sort key
	    if (direction >= 0)
		cols[j0]->bottomk(keep, ind0);
	    else
		cols[j0]->topk(keep, ind0);
	    for (uint32_t ii = 0; ii < cols.size(); ++ ii)
		cols[ii]->reorder(ind0);
	    ibis::util::reorder(*starts, ind0);
	    ngroups = ind0.size();
	    { // segment cols[j0]
		array_t<uint32_t> *tmp = cols[j0]->segment(&gb);
		gb.swap(*tmp);
		delete tmp;
	    }

	    for (++ i;
		 i < sortkeys.size() && gb.size() <= ngroups;
		 ++ i) {
		const uint32_t j1 = comps.find(sortkeys[i]);
		if (j1 >= comps.size()) continue;

		for (uint32_t g = 0; g < gb.size()-1; ++ g) {
		    if (gb[g+1] > gb[g]+1) { // more than one group
			array_t<uint32_t> ind1; // indices for group g
			cols[j1]->sort(gb[g], gb[g+1], ind1);
			ind0.insert(ind0.end(), ind1.begin(), ind1.end());
		    }
		    else { // a single group
			ind0.push_back(gb[g]);
		    }
		}
		for (uint32_t k = 0; k < cols.size(); ++ k)
		    cols[k]->reorder(ind0);
		ibis::util::reorder(*(starts), ind0);

		{
		    array_t<uint32_t> *tmp = cols[j1]->segment(&gb);
		    gb.swap(*tmp);
		    delete tmp;
		}
	    }
	}
	else {
	    const uint32_t j = comps.find(sortkeys[0]);
	    if (j < comps.size()) {
		array_t<uint32_t> ind;
		if (direction >= 0)
		    cols[j]->bottomk(keep, ind);
		else
		    cols[j]->topk(keep, ind);
		for (uint32_t i = 0; i < cols.size(); ++ i)
		    cols[i]->reorder(ind);
		ibis::util::reorder(*(starts), ind);
		ngroups = ind.size();
	    }
	}

	if (direction < 0) { // reverse the order
	    for (uint32_t j = 0; j < cols.size(); ++ j)
		for (uint32_t i = 0; i < ngroups/2; ++ i)
		    cols[j]->swap(i, ngroups-1-i);
	    for (uint32_t i = 0; i < ngroups/2; ++ i) {
		const uint32_t j = ngroups - 1 - i;
		const uint32_t tmp = (*starts)[i];
		(*starts)[i] = (*starts)[j];
		(*starts)[j] = tmp;
	    }
	}

	/// turn counts back into starting positions (starts)
	uint32_t cumu = 0;
	for (uint32_t i = 0; i < ngroups; ++ i) {
	    uint32_t tmp = (*starts)[i];
	    (*starts)[i] = cumu;
	    cumu += tmp;
	}
	starts->push_back(cumu);
    }

    // truncate arrays
    if (ngroups > keep) {
	if (rids != 0)
	    rids->resize((*starts)[keep]);
	starts->resize(keep+1);
	for (uint32_t i = 0; i < cols.size(); ++ i)
	    cols[i]->truncate(keep);
    }
    // new content, definitely not in file yet
    infile = false;
    return size();
} // ibis::bundles::truncate

void ibis::bundles::clear() {
    for (ibis::colList::iterator it = cols.begin(); it != cols.end(); ++it)
	delete *it;
    cols.clear();
}

void ibis::bundles::write(const ibis::query& theQ) const {
    if (theQ.dir() == 0) return;
    if (cols.size() == 0) return;
    if (infile) return;
    if (starts == 0) return;
    if (cols[0]->size() == 0) return;
    if (cols[0]->size()+1 != starts->size()) {
	ibis::util::logMessage("Warning", "ibis::bundles::write invalid "
			       "bundle (starts->size(%lu) != "
			       "cols[0]->size(%lu)+1)",
			       static_cast<long unsigned>(starts->size()),
			       static_cast<long unsigned>(cols[0]->size()));
	return;
    }

    if (rids != 0)
	theQ.writeRIDs(rids); // write the RIDs

    uint32_t len = strlen(theQ.dir());
    char* fn = new char[len+16];
    strcpy(fn, theQ.dir());
    strcat(fn, "bundles");
    FILE* fptr = fopen(fn, "wb");
    if (fptr == 0) {
	ibis::util::logMessage("Warning", "ibis::bundles::write -- unable "
			       "to open file \"%s\" ... %s", fn,
			       (errno ? strerror(errno) : 
				"no free stdio stream"));
	return;
    }

    uint32_t i1, ncol = cols.size(), tmp = cols[0]->size();
    int32_t ierr = fwrite(&tmp, sizeof(uint32_t), 1, fptr);
    ierr = fwrite(&ncol, sizeof(uint32_t), 1, fptr);
    for (i1 = 0; i1 < ncol; ++ i1) { // element sizes
	tmp = cols[i1]->elementSize(); // ibis::colValue::elementSize
	ierr = fwrite(&tmp, sizeof(uint32_t), 1, fptr);
	if (cols[i1]->size() != cols[0]->size())
	    ibis::util::logMessage
		("Warning", "invalid ibis::bundle object "
		 "(cols[i1]->size(%lu) != cols[0]->size(%lu))",
		 static_cast<long unsigned>(cols[i1]->size()),
		 static_cast<long unsigned>(cols[0]->size()));
    }

    for (i1 = 0; i1 < ncol; ++ i1) { // the actual values
	ierr = cols[i1]->write(fptr);
    }

    // the starting positions
    ierr = fwrite(starts->begin(), sizeof(uint32_t), starts->size(),
		  fptr);
    ierr = fclose(fptr);
    delete [] fn;
    infile = true;
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    sync();
#endif
} // ibis::bundles::write

/// Retrieve the value of i-th row j-th column as a 32-bit integer.
/// Return the maximal value defined in the class numeric_limits if indices
/// i and j are out of bounds.
int32_t ibis::bundles::getInt(uint32_t i, uint32_t j) const {
    int32_t ret = 0x7FFFFFFF;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	ret = cols[j]->getInt(i);
    }
    return ret;
} // ibis::bundles::getInt

/// Retrieve the value of i-th row j-th column as a 32-bit unsigned integer.
/// Return the maximal value defined in the class numeric_limits if indices
/// i and j are out of bounds.
uint32_t ibis::bundles::getUInt(uint32_t i, uint32_t j) const {
    uint32_t ret = 0xFFFFFFFFU;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	ret = cols[j]->getUInt(i);
    }
    return ret;
} // ibis::bundles::getUInt

/// Retrieve the value of i-th row j-th column as a 64-bit integer.
/// Return the maximal value defined in the class numeric_limits if indices
/// i and j are out of bounds.
int64_t ibis::bundles::getLong(uint32_t i, uint32_t j) const {
    int64_t ret = 0x7FFFFFFFFFFFFFFFLL;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	ret = cols[j]->getLong(i);
    }
    return ret;
} // ibis::bundles::getLong

/// Retrieve the value of i-th row j-th column as a 64-bit unsigned integer.
/// Return the maximal value defined in the class numeric_limits if indices
/// i and j are out of bounds.
uint64_t ibis::bundles::getULong(uint32_t i, uint32_t j) const {
    uint64_t ret = 0xFFFFFFFFFFFFFFFFULL;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	ret = cols[j]->getULong(i);
    }
    return ret;
} // ibis::bundles::getULong

/// Retrieve the value of i-th row j-th column as a 32-bit floating-point
/// number.
/// Return the maximal value defined in the class numeric_limits if indices
/// i and j are out of bounds.
float ibis::bundles::getFloat(uint32_t i, uint32_t j) const {
    float ret = FLT_MAX;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	ret = cols[j]->getFloat(i);
    }
    return ret;
} // ibis::bundles::getFloat

/// Retrieve the value of i-th row j-th column as a 64-bit floating-point
/// number. 
/// Return the maximal value defined in the class numeric_limits if indices
/// i and j are out of bounds.
double ibis::bundles::getDouble(uint32_t i, uint32_t j) const {
    double ret = DBL_MAX;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	ret = cols[j]->getDouble(i);
    }
    return ret;
} // ibis::bundles::getDouble

/// Retrieve the value of i-th row j-th column as a string.
/// Convert the value to its string representation through @c
/// std::ostringstream.
std::string ibis::bundles::getString(uint32_t i, uint32_t j) const {
    std::ostringstream oss;
    if (j < cols.size() && i < cols[j]->size()) { // indices i and j are valid
	cols[j]->write(oss, i);
    }
    return oss.str();
} // ibis::bundles::getString

ibis::query::result::result(ibis::query& q)
    : que_(q), bdl_(0), sel(q.components()), bid_(0), lib_(0) {
    if (q.getState() == ibis::query::UNINITIALIZED ||
	q.getState() == ibis::query::SET_COMPONENTS) {
	throw ibis::bad_alloc("Can not construct query::result on "
			      "an incomplete query");
    }
    if (sel.size() == 0) {
	throw ibis::bad_alloc("Can not construct query::result on "
			      "a query without a select clause");
    }
    if (q.getState() == ibis::query::SPECIFIED ||
	q.getState() == ibis::query::QUICK_ESTIMATE) {
	int ierr = q.evaluate();
	if (ierr < 0) {
	    ibis::util::logMessage
		("Error", "ibis::query::result constructor failed "
		 "to evaluate query %s", q.id());
	    throw ibis::bad_alloc("Can not evaluate query");
	}
    }
    bdl_ = ibis::bundle::create(q);
    if (bdl_ == 0) {
	ibis::util::logMessage
	    ("Error", "ibis::query::result constructor failed "
	     "to create a bundle object from query %s", q.id());
	throw ibis::bad_alloc("failed to create a result set from query");
    }
} // ibis::query::result::result

ibis::query::result::~result() {
    delete bdl_;
    bdl_ = 0;
    bid_ = 0;
    lib_ = 0;
} // ibis::query::result::~result

bool ibis::query::result::next() {
    bool ret = false;
    if (bdl_ == 0)
	return ret;
    const uint32_t bsize = bdl_->size();
    if (bid_ < bsize) {
	ret = true;
	if (lib_ > 0) {
	    -- lib_;
	}
	else { // need to move on to the next bundle
	    lib_ = bdl_->numRowsInBundle(bid_) - 1;
	    ++ bid_;
	}
    }
    else if (bid_ == bsize) {
	if (lib_ > 0) {
	    -- lib_;
	    ret = true;
	}
	else {
	    ++ bid_;
	}
    }
    return ret;
} // ibis::query::result::next

void ibis::query::result::reset() {
    bid_ = 0;
    lib_ = 0;
} // ibis::query::result::reset

int32_t ibis::query::result::getInt(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getInt(ind);
} // ibis::query::result::getInt

uint32_t ibis::query::result::getUInt(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getUInt(ind);
} // ibis::query::result::getUInt

int64_t ibis::query::result::getLong(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getLong(ind);
} // ibis::query::result::getLong

uint64_t ibis::query::result::getULong(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getULong(ind);
} // ibis::query::result::getULong

float ibis::query::result::getFloat(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getFloat(ind);
} // ibis::query::result::getFloat

double ibis::query::result::getDouble(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getDouble(ind);
} // ibis::query::result::getDouble

std::string ibis::query::result::getString(const char *cname) const {
    uint32_t ind = sel.find(cname);
    return getString(ind);
} // ibis::query::result::getDouble

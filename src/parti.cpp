//File: $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2000-2012 the Regents of the University of California
//
// Implement the ibis::part functions that modify a partition.
////////////////////////////////////////////////////////////////////////
// Because these functions modifies a partition, they may require mutex
// locks to function correctly!
////////////////////////////////////////////////////////////////////////
#include "part.h"	// ibis::part definition, ibis header files
#include "category.h"
#include "selectClause.h"	// for parsing arithmetic expressions

#include <sstream>	// std::ostringstream
#include <typeinfo>	// typeid
#include <limits>	// std::numeric_limits
#include <algorithm>	// std::reverse

/// Sort rows with the lowest cardinality column first.  The lowest
/// cardinality column is ordered first.  Only integer-valued columns are
/// used in sorting.  Returns the number of rows reordered when successful,
/// otherwise return a negative number and the base data is corrupt!
///
/// @warning <b>Danger</b>: This function does not update null masks!
long ibis::part::reorder() {
    if (nRows() == 0 || nColumns() == 0 || activeDir == 0 ||
	amask.cnt() != amask.size()) return 0;
    ibis::table::stringList keys;
    gatherSortKeys(keys);
    if (keys.empty()) {
	return -1;
    }
    else {
	std::vector<bool> direc;
	return reorder(keys, direc);
    }
} // ibis::part::reorder

void ibis::part::gatherSortKeys(ibis::table::stringList& names) {
    // first gather all integer-valued columns
    typedef std::vector<column*> colVector;
    colVector keys; // sort according to the keys
    array_t<uint64_t> ranges;
    for (columnList::iterator it = columns.begin(); it != columns.end();
	 ++ it) {
	if ((*it).second->isInteger()) {
	    if ((*it).second->upperBound() > (*it).second->lowerBound()) {
		uint64_t width = static_cast<uint64_t>
		    ((*it).second->upperBound() - (*it).second->lowerBound());
		keys.push_back((*it).second);
		ranges.push_back(width);
	    }
	    else {
		(*it).second->computeMinMax();
		if ((*it).second->upperBound() > (*it).second->lowerBound()) {
		    uint64_t width = static_cast<uint64_t>
			((*it).second->upperBound() -
			 (*it).second->lowerBound());
		    keys.push_back((*it).second);
		    ranges.push_back(width);
		}
	    }
	}
    }

    if (keys.size() > 1) {
	names.resize(keys.size());
	array_t<uint32_t> ind;
	ranges.stableSort(ind);
	for (unsigned i = 0; i < ind.size(); ++ i)
	    names[i] = keys[ind[i]]->name();
    }
    else if (keys.size() == 1) {
	names.push_back(keys.front()->name());
    }
} // ibis::part::getherSortKeys

long ibis::part::reorder(const ibis::table::stringList& names) {
    std::vector<bool> direc;
    return reorder(names, direc);
} // ibis::part::reorder

/// Sort rows according the values of the columns specified in @c names.
long ibis::part::reorder(const ibis::table::stringList& names,
			 const std::vector<bool>& directions) {
    if (nRows() == 0 || nColumns() == 0 || activeDir == 0) return 0;
    std::string evt = "part[";
    evt += m_name;
    evt += "]::reorder";
    ibis::util::timer mytimer(evt.c_str(), 1);

    long ierr = purgeInactive();
    if (ierr <= 0) return ierr;

    // first gather all columns with numerical values
    typedef std::vector<column*> colVector;
    std::set<const char*, ibis::lessi> used;
    colVector keys, load; // sort according to the keys
    for (ibis::table::stringList::const_iterator nit = names.begin();
	 nit != names.end(); ++ nit) {
	ibis::part::columnList::iterator it = columns.find(*nit);
	if (it != columns.end()) {
	    used.insert((*it).first);
	    if (! (*it).second->isNumeric()) {
		load.push_back((*it).second);
	    }
	    else if ((*it).second->upperBound() > (*it).second->lowerBound()) {
		keys.push_back((*it).second);
	    }
	    else {
		(*it).second->computeMinMax();
		if ((*it).second->upperBound() > (*it).second->lowerBound()) {
		    keys.push_back((*it).second);
		}
		else {
		     load.push_back((*it).second);
		}
	    }
	}
    }

    if (keys.empty()) { // no keys specified
	if (names.empty()) {
	    LOGGER(ibis::gVerbose > 0)
		<< evt << " -- user did not specify ordering keys, "
		"will attempt to use all integer columns as ordering keys";
	    return reorder();
	}
	else if (ibis::gVerbose > 0) {
	    std::ostringstream oss;
	    oss << names[0];
	    for (unsigned i = 1; i < names.size(); ++ i)
		oss << ", " << names[i];
	    logMessage("reorder", "user specified ordering keys \"%s\" "
		       "does not match any numerical columns with more "
		       "than one distinct value, can not continue",
		       oss.str().c_str());
	}
	return 0;
    }
    if (ibis::gVerbose > 0) {
	std::ostringstream oss;
	oss << evt << '(' << keys[0]->name();
	for (unsigned i = 1; i < keys.size(); ++ i)
	    oss << ", " << keys[i]->name();
	oss << ')';
	evt = oss.str();
    }
    LOGGER(ibis::gVerbose > 2)
	<< evt << " start sorting ...";

    writeLock lock(this, evt.c_str()); // can't process other operations
    for (columnList::const_iterator it = columns.begin();
	 it != columns.end();
	 ++ it) { // purge all index files
	if (! it->second->isNumeric()) return -1L;
	ibis::bitvector msk;
	it->second->getNullMask(msk);
	if (msk.cnt() != msk.size()) return -2L;

	(*it).second->unloadIndex();
	(*it).second->purgeIndexFile();
    }
    if (backupDir != 0 && *backupDir != 0)
	ibis::fileManager::instance().flushDir(backupDir);
    if (activeDir != 0 && *activeDir != 0)
	ibis::fileManager::instance().flushDir(activeDir);

    for (ibis::part::columnList::const_iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	std::set<const char*, ibis::lessi>::const_iterator uit =
	    used.find((*it).first);
	if (uit == used.end())
	    load.push_back((*it).second);
    }

    // the sorting loop
    ierr = nRows();
    array_t<uint32_t> ind1;
    {
	array_t<uint32_t> starts, ind0;
	for (uint32_t i = 0; i < keys.size(); ++ i) {
	    std::string sname;
	    const bool asc = (directions.size()>i?directions[i]:true);
	    const char *fname = keys[i]->dataFileName(sname);
	    switch (keys[i]->type()) {
	    case ibis::DOUBLE:
		ierr = reorderValues<double>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::FLOAT:
		ierr = reorderValues<float>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::ULONG:
		ierr = reorderValues<uint64_t>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::LONG:
		ierr = reorderValues<int64_t>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::UINT:
		ierr = reorderValues<uint32_t>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::INT:
		ierr = reorderValues<int32_t>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::USHORT:
		ierr = reorderValues<uint16_t>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::SHORT:
		ierr = reorderValues<int16_t>(fname, starts, ind0, ind1, asc);
		break;
	    case ibis::UBYTE:
		ierr = reorderValues<unsigned char>
		    (fname, starts, ind0, ind1, asc);
		break;
	    case ibis::BYTE:
		ierr = reorderValues<char>(fname, starts, ind0, ind1, asc);
		break;
	    default:
		logWarning("reorder", "column %s type %d is not supported as "
			   "sort key",
			   keys[i]->name(), static_cast<int>(keys[i]->type()));
		continue;
	    }

	    if (ierr == static_cast<long>(nRows())) {
		ind1.swap(ind0);
	    }
	    else {
		logError("reorder", "failed to reorder column %s, ierr=%ld.  "
			 "data files are no longer consistent!",
			 keys[i]->name(), ierr);
	    }
	}
    }

#if DEBUG+0 > 0 || _DEBUG+0 > 0
    {
	ibis::util::logger lg(4);
	lg() << "part[" << name() << "]::reorder --\n";
	std::vector<bool> marks(ind1.size(), false);
	for (uint32_t i = 0; i < ind1.size(); ++ i) {
	    if (ibis::gVerbose > 6)
		lg() << "ind[" << i << "]=" << ind1[i] << "\n";
	    if (ind1[i] < ind1.size())
		marks[ind1[i]] = true;
	}
	bool isperm = true;
	for (uint32_t i = 0; isperm && i < ind1.size(); ++ i)
	    isperm = marks[i];
	if (isperm)
	    lg() << "array ind IS a permutation\n";
	else
	    lg() << "array ind is NOT a permutation\n";
    }
#endif
    for (columnList::const_iterator it = columns.begin();
	 it != columns.end();
	 ++ it) { // update the m_sorted flag of each column
	(*it).second->isSorted((*it).second == keys[0]);
    }

    LOGGER(ibis::gVerbose > 2 && load.size() > 0)
	<< evt << " start moving unsorted columns ...";
    for (uint32_t i = 0; i < load.size(); ++ i) {
	std::string sname;
	const char *fname = load[i]->dataFileName(sname);
	switch (load[i]->type()) {
	case ibis::DOUBLE:
	    ierr = writeValues<double>(fname, ind1);
	    break;
	case ibis::FLOAT:
	    ierr = writeValues<float>(fname, ind1);
	    break;
	case ibis::ULONG:
	    ierr = writeValues<uint64_t>(fname, ind1);
	    break;
	case ibis::LONG:
	    ierr = writeValues<int64_t>(fname, ind1);
	    break;
	case ibis::UINT:
	    ierr = writeValues<uint32_t>(fname, ind1);
	    break;
	case ibis::INT:
	    ierr = writeValues<int32_t>(fname, ind1);
	    break;
	case ibis::USHORT:
	    ierr = writeValues<uint16_t>(fname, ind1);
	    break;
	case ibis::SHORT:
	    ierr = writeValues<int16_t>(fname, ind1);
	    break;
	case ibis::UBYTE:
	    ierr = writeValues<unsigned char>(fname, ind1);
	    break;
	case ibis::BYTE:
	    ierr = writeValues<char>(fname, ind1);
	    break;
	default:
	    logWarning("reorder", "column %s type %s is not supported in "
		       "this implementation", keys[i]->name(),
		       ibis::TYPESTRING[static_cast<int>(keys[i]->type())]);
	    continue;
	}
	LOGGER(ierr < 0 && ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to write data to " << fname
	    << " for column " << load[i]->name() << " (type "
	    << ibis::TYPESTRING[load[i]->type()] << "), ierr = " << ierr;
    }

    if (rids != 0 && rids->size() == nEvents) {
	delete rids;
	rids = 0;
	std::string fname(activeDir);
	fname += FASTBIT_DIRSEP;
	fname += "-rids";
	ierr = writeValues<uint64_t>(fname.c_str(), ind1);
	LOGGER(ierr < 0 && ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to write data to " << fname
	    << ", ierr = " << ierr;
	if (ierr > 0 && static_cast<unsigned>(ierr) == nEvents) {
	    rids = new ibis::RIDSet;
	    ierr = ibis::fileManager::instance().getFile(fname.c_str(), *rids);
	    LOGGER(ierr < 0 && ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " failed to read " << fname
		<< " after reordering, ierr = " << ierr;
	}
    }

    m_desc += " -- ";
    m_desc += evt;
    if (ibis::gVerbose >= 0) {
	char currtime[30];
	ibis::util::getLocalTime(currtime);
	m_desc += " on ";
	m_desc += currtime;
    }
    writeMetaData(nEvents, columns, activeDir);
    LOGGER(ibis::gVerbose > 1 && ierr >= 0)
	<< evt << " completed successfully";
    return ierr;
} // ibis::part::reorder

/// Writes elementary data tyles.  Can not handle string values correctly.
template <typename T>
long ibis::part::writeValues(const char *fname,
			     const array_t<uint32_t>& ind) {
    int fdes = UnixOpen(fname, OPEN_READWRITE, OPEN_FILEMODE);
    if (fdes < 0) {
	if (ibis::gVerbose > 1)
	    logWarning("writeValues", "failed to open %s for writing "
		       "reordered values", fname);
	return -1; // couldn't open file for writing
    }
    long pos = UnixSeek(fdes, 0L, SEEK_END);
    if (pos != static_cast<long>(ind.size() * sizeof(T))) {
	if (ibis::gVerbose > 1)
	    logMessage("writeValues", "expected size of %s is %ld, "
		       "actual size is %ld", fname,
		       static_cast<long>(ind.size() * sizeof(T)),
		       pos);
	UnixClose(fdes);
	return -2;
    }

#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    array_t<T> vals;
    vals.read(fdes, 0, pos);
    if (vals.size() != ind.size()) {
	if (ibis::gVerbose > 1)
	    logMessage("writeValues", "failed to read %lu elements from %s, "
		       "actually read %lu",
		       static_cast<long unsigned>(ind.size()), fname,
		       static_cast<long unsigned>(vals.size()));
	UnixClose(fdes);
	return -3;
    }

    // write the values out in the new order
    UnixSeek(fdes, 0, SEEK_SET);
    const unsigned block = PREFERRED_BLOCK_SIZE / sizeof(T);
    array_t<T> buf(block);
    for (uint32_t i = 0; i < vals.size(); i += block) {
	const unsigned asize = (i+block<=vals.size() ? block : vals.size()-i);
	for (uint32_t j = 0; j < asize; ++ j)
	    buf[j] = vals[ind[i+j]];
	LOGGER((long)(asize * sizeof(T)) >
	       UnixWrite(fdes, buf.begin(), asize * sizeof(T)) &&
	       ibis::gVerbose > 1)
	    << "Warning -- part[" << name() << "]::writeValues failed to write "
	    << asize << " value" << (asize>1?"s":"") << " of type "
	    << typeid(T).name();
    }
    UnixClose(fdes);
    return vals.size();
} // ibis::part::writeValues

/// Reorders elementary data types.  Can not handle string valued data.
/// This function opens the data file in read-write mode and modify the
/// content of the underlying data file.
template <typename T>
long ibis::part::reorderValues(const char *fname,
			       array_t<uint32_t>& starts,
			       array_t<uint32_t>& indout,
			       const array_t<uint32_t>& indin,
			       bool ascending) {
    const long unsigned nrows = nRows();
    ibis::horometer timer;
    if (ibis::gVerbose > 2)
	timer.start();

    std::string evt = "part[";
    evt += m_name;
    evt += "]::reorderValues<";
    evt += typeid(T).name();
    evt += ">(";
    evt += fname;
    evt += ')';
    int fdes = UnixOpen(fname, OPEN_READWRITE, OPEN_FILEMODE);
    if (fdes < 0) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- failed to open file " << fname
	    << " for writing reordered values";
	return -1; // couldn't open file for writing
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    ibis::util::guard gfdes = ibis::util::makeGuard(UnixClose, fdes);
    long pos = UnixSeek(fdes, 0L, SEEK_END);
    if (pos != static_cast<long>(nrows * sizeof(T))) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- expected size of " << fname << " is "
	    << nrows * sizeof(T) << ", actual size is " << pos;
	return -2;
    }

    array_t<T> vals;
    vals.read(fdes, 0, pos);
    if (vals.size() != nrows || (indin.size() != vals.size() &&
				 ! indin.empty())) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- failed to read " << nrows << " elements from "
	    << fname << ", actually read " << vals.size();
	return -3;
    }
    if (indin.empty() || starts.size() < 2 || starts[0] != 0
	|| starts.back() != vals.size()) {
	starts.resize(2);
	starts[0] = 0;
	starts[1] = vals.size();
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- (re)set array starts to contain [0, " << vals.size()
	    << "]";
    }

    // sort vals one segment at a time
    const uint32_t nseg = starts.size() - 1;
    if (nseg > nrows) { // no sorting necessary
	indout.resize(nrows);
	for (uint32_t i = 0; i < nrows; ++i)
	    indout[i] = indin[i];
    }
    else if (nseg > 1) { // need sorting some blocks
	indout.resize(nrows);
	array_t<uint32_t> starts2;

	for (uint32_t iseg = 0; iseg < nseg; ++ iseg) {
	    const uint32_t segstart = starts[iseg];
	    const uint32_t segsize = starts[iseg+1]-starts[iseg];
	    if (segsize > 1) { // segment has move than one element
		array_t<T> tmp(segsize); // copy segement to this array
		array_t<uint32_t> ind0;
		for (unsigned i = 0; i < segsize; ++ i)
		    tmp[i] = vals[indin[i+segstart]];
		tmp.sort(ind0); // sort segment
		if (! ascending)
		    std::reverse(ind0.begin(), ind0.end());

		starts2.push_back(segstart);
		T last = tmp[ind0[0]];
		indout[segstart] = indin[ind0[0] + segstart];
		for (unsigned i = 1; i < segsize; ++ i) {
		    indout[i+segstart] = indin[ind0[i] + segstart];
		    if (tmp[ind0[i]] > last) {
			starts2.push_back(i + segstart);
			last = tmp[ind0[i]];
		    }
		}
	    }
	    else { // segement has only one element
		starts2.push_back(segstart);
		indout[segstart] = indin[segstart];
	    }
	}
	starts2.push_back(nrows);
	starts.swap(starts2);
    }
    else { // all in one block
	vals.sort(indout);
	if (! ascending)
	    std::reverse(indout.begin(), indout.end());

	starts.clear();
	starts.push_back(0U);
	T last = vals[indout[0]];
	for (uint32_t i = 1; i < nrows; ++ i) {
	    if (vals[indout[i]] > last) {
		starts.push_back(i);
		last = vals[indout[i]];
	    }
	}
	starts.push_back(nrows);
    }

    // write the values out in the new order
    UnixSeek(fdes, 0, SEEK_SET); // rewind
    const unsigned block = PREFERRED_BLOCK_SIZE / sizeof(T);
    array_t<T> buf(block);
    for (uint32_t i = 0; i < nrows; i += block) {
	const unsigned asize = (i+block<=vals.size() ? block : vals.size()-i);
	for (unsigned j = 0; j < asize; ++ j)
	    buf[j] = vals[indout[i+j]];
	LOGGER((long)(asize*sizeof(T)) !=
	       UnixWrite(fdes, buf.begin(), asize * sizeof(T)) &&
	       ibis::gVerbose > 0)
	    << "Warning -- " << evt << " failed to write " << asize
	    << " value" << (asize>1?"s":"") << " of type "
	    << typeid(T).name();
    }
    UnixClose(fdes);
    gfdes.dismiss(); // no longer need the guard on fdes
    if (ibis::gVerbose > 2) {
	timer.stop();
	LOGGER(ibis::gVerbose > 2)
	    << evt << " -- wrote " << nrows << " reordered value"
	    << (nrows>1 ? "s" : "") << " (# seg " << (starts.size()-1)
	    << ") to " << fname << " in " << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed)";
    }
    return nrows;
} // ibis::part::reorderValues


/// Append data in dir to the current database.  Return the number of rows
/// actually added.
///
/// @note If there is a backup data directory, it is possible to rollback
/// the append operation before commit.
long ibis::part::append(const char* dir) {
    long ierr = 0;
    if (dir == 0 || *dir == 0)
	return ierr;
    if (activeDir == 0 || *activeDir == 0 || readonly)
	return -1;

    std::string evt = "part[";
    evt += m_name;
    evt += "]::append(";
    evt += dir;
    evt += ')';
    ibis::util::mutexLock lock(&mutex, evt.c_str());
    ibis::util::timer mytimer(evt.c_str());
    // can only do this in RECEIVING state and have received something
    if (state == STABLE_STATE)
	state = RECEIVING_STATE;
    if (state != RECEIVING_STATE) {
	logWarning("append", "can not accept data from %s while in state %d",
		   dir, static_cast<int>(state));
	return ierr;
    }

    try {
	if (backupDir != 0 && *backupDir != 0 && activeDir != backupDir &&
	    strcmp(activeDir, backupDir) != 0) {
	    ibis::fileManager::instance().flushDir(backupDir);
	    ierr = append2(dir);
	}
	else {
	    ibis::fileManager::instance().flushDir(activeDir);
	    ierr = append1(dir);
	}
    }
    catch (const char* s) { // revert to previous state
	logWarning("append", "received the following error message, "
		   "will reverse changes made so far.\n%s", s);
	state = UNKNOWN_STATE;
	makeBackupCopy();
	ierr = -2021;
    }
    catch (...) {
	logWarning("append", "received a unknown exception, "
		   "will reverse changes made so far.");
	state = UNKNOWN_STATE;
	makeBackupCopy();
	ierr = -2020;
 	throw; // can not handle unknown error -- rethrow exception
    }

    return ierr;
} // ibis::part::append

/// Perform append operation using only only one data directory.  Must wait
/// for all queries on the partition to finish before preceding.
long ibis::part::append1(const char *dir) {
    // can not handle dir == activeDir
    if (strcmp(dir, activeDir) == 0)
	return -1;

    long ierr = 0;
    uint32_t ntot = 0;
    {   // need an exclusive lock to allow file manager to close all
	// open files
	writeLock rw(this, "append");
	unloadIndexes();	// remove all indices
	delete rids;	// remove the RID list
	ibis::fileManager::instance().flushDir(activeDir);
	columnList::iterator it;
	for (it = columns.begin(); it != columns.end(); ++it)
	    delete (*it).second;
	columns.clear();
    }

    // assign backupDir so that appendToBackup will work correctly
    if (backupDir != activeDir)
	delete [] backupDir;
    backupDir = activeDir; activeDir = 0;

    // do the work of copying data
    ierr = appendToBackup(dir);

    // reset backupDir to null
    activeDir = backupDir; backupDir = 0;

    // retrieve the new column list
    readMetaData(nEvents, columns, activeDir);
    if (ntot > 0 && ntot != nEvents) {
	logWarning("append", "expected %lu rows, but the table.tdc "
		   "file says %lu", static_cast<long unsigned>(ierr),
		   static_cast<long unsigned>(nEvents));
	return -2022;
    }
    // retrieve the new RID list
    std::string fn(activeDir);
    fn += FASTBIT_DIRSEP;
    fn += "-rids";
    rids = new array_t<ibis::rid_t>;
    if (0 != ibis::fileManager::instance().getFile(fn.c_str(),*rids)) {
	if (nEvents > 0 && ibis::gVerbose > 4)
	    logMessage("append", "unable to read rid file \"%s\" ... %s",
		       fn.c_str(), strerror(errno));

	std::string fillrids(m_name);
	fillrids += ".fillRIDs";
	if (nEvents > 0 &&
	    ibis::gParameters().isTrue(fillrids.c_str()))
	    fillRIDs(fn.c_str());
    }

    switchTime = time(0);
    state = STABLE_STATE; // switched successfully
    writeMetaData(nEvents, columns, activeDir); // update the TDC file

    if (nEvents > 0) { // update the mask for the partition
	amask.adjustSize(nEvents, nEvents);
	if (amask.cnt() < amask.size()) {
	    std::string mskfile(activeDir);
	    if (! mskfile.empty())
		mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    amask.write(mskfile.c_str());
	    ibis::fileManager::instance().flushFile(mskfile.c_str());
	}
    }
    ibis::fileManager::instance().flushDir(activeDir);
    if (ibis::gVerbose > -1) {
	logMessage("append", "committed to use the "
		   "updated dataset with %lu rows and %lu "
		   "columns", static_cast<long unsigned>(nEvents),
		   static_cast<long unsigned>(columns.size()));
	if (ibis::gVerbose > 3) {
	    ibis::util::logger lg;
	    print(lg());
	}
    }
    return ierr;
} // ibis::part::append1

/// Perform append operation with two data directories.  It appends the
/// data to the backup directory first, then swap the roles of the two
/// directories.
long ibis::part::append2(const char *dir) {
    long ierr = 0;
    uint32_t ntot = 0;
    // only need to copy files if the files are not already in the
    // activeDir
    if (strcmp(dir, activeDir)) {
	ierr = verifyBackupDir(); // make sure the backup is there
	if (ierr != 0) {
	    if (nEvents > 0) {
		state = UNKNOWN_STATE;
		doBackup(); // actually copy the files
	    }
	    else {
		ibis::util::removeDir(backupDir, true);
	    }
	}
	state = PRETRANSITION_STATE;
	ierr = appendToBackup(dir);
	if (ierr < 0) {
	    logWarning("append", "appendToBackup(%s) returned with "
		       "%ld, restore the content of backupDir",
		       dir, ierr);
	    state = UNKNOWN_STATE;
	    makeBackupCopy();
	    ierr = -2023;
	    return ierr;
	}
	else if (ierr == 0) {
	    if (ibis::gVerbose > 1)
		logMessage("append", "appendToBackup(%s) appended no "
			   "new rows", dir);
	    state = STABLE_STATE;
	    return ierr;
	}

	// make sure that the number of RIDs is as expected
	std::string fn(backupDir);
	fn += FASTBIT_DIRSEP;
	fn += "-rids";
	uint32_t nrids = ibis::util::getFileSize(fn.c_str()) /
	    sizeof(ibis::rid_t);
	ntot = nEvents + ierr;
	if (nrids > 0 && nrids != ntot) {
	    logWarning("append", "expected to have %lu rids after "
		       "switch, but get %lu",
		       static_cast<long unsigned>(ntot),
		       static_cast<long unsigned>(nrids));
	    state = UNKNOWN_STATE;
	    makeBackupCopy();
	    ierr = -2024;
	    return ierr;
	}
    }

    {   // need an exclusive lock to allow file manager to close all
	// open files and switch the roles of the activeDir and the
	// backupDir
	writeLock rw(this, "append");
	if (strcmp(dir, activeDir)) {
	    unloadIndexes();	// remove all indices
	    delete rids;	// remove the RID list
	    ibis::fileManager::instance().flushDir(activeDir);
	    columnList::iterator it;
	    for (it = columns.begin(); it != columns.end(); ++it)
		delete (*it).second;
	    columns.clear();

	    // switch the directory name and read the rids
	    char* tstr = activeDir;
	    activeDir = backupDir;
	    backupDir = tstr;
	}

	// retrieve the new column list
	readMetaData(nEvents, columns, activeDir);
	if (ntot > 0 && ntot != nEvents) {
	    logWarning("append", "expected %lu rows, but the table.tdc "
		       "file says %lu", static_cast<long unsigned>(ierr),
		       static_cast<long unsigned>(nEvents));
	    return -2025;
	}
	// retrieve the new RID list
	std::string fn(activeDir);
	fn += FASTBIT_DIRSEP;
	fn += "-rids";
	rids = new array_t<ibis::rid_t>;
	if (0 != ibis::fileManager::instance().
	    getFile(fn.c_str(),*rids)) {
	    if (nEvents > 0 && ibis::gVerbose > 4)
		logMessage("append", "unable to read rid file \"%s\" ... %s",
			   fn.c_str(), strerror(errno));

	    std::string fillrids(m_name);
	    fillrids += ".fillRIDs";
	    if (nEvents > 0 &&
		ibis::gParameters().isTrue(fillrids.c_str()))
		fillRIDs(fn.c_str());
	}

	switchTime = time(0);
	state = TRANSITION_STATE; // switched successfully
	writeMetaData(nEvents, columns, activeDir); // update the TDC file

	// update the mask for the partition
	amask.adjustSize(nEvents, nEvents);
	if (amask.cnt() < amask.size()) {
	    std::string mskfile(activeDir);
	    if (! mskfile.empty())
		mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    amask.write(mskfile.c_str());
	    ibis::fileManager::instance().flushFile(mskfile.c_str());
	}
    }
    if (ibis::gVerbose > -1) {
	logMessage("append", "switched (with possibility of rollback) to use "
		   "the updated dataset with %lu rows and %lu columns",
		   static_cast<long unsigned>(nEvents),
		   static_cast<long unsigned>(columns.size()));
	if (ibis::gVerbose > 3) {
	    ibis::util::logger lg;
	    print(lg());
	}
    }
    return ierr;
} // ibis::part::append2

/// Rollback(revert) to previous data set.  Can only undo the last append
/// operation on the data partition.
long ibis::part::rollback() {
    long ierr = 0;
    if (backupDir == 0 || *backupDir == 0 || activeDir == 0 || readonly)
	return ierr;

    ibis::util::mutexLock lock(&mutex, "part::rollback");
    if (state != TRANSITION_STATE)
	return ierr;

    try {
	// process no more queries, clear RID list, close all open files
	writeLock rw(this, "rollback");
	unloadIndexes();	// remove all indices
	delete rids;	// remove the RID list
	ibis::fileManager::instance().clear();

	// switch the directory name, and read the table and the rids
	char* tmp = activeDir;
	activeDir = backupDir;
	backupDir = tmp;
	int jerr = readMetaData(nEvents, columns, activeDir);
	if (jerr <= 0) {
	    logWarning("rollback", "the TDC file in \"%s\" contains no "
		       "valid entry.  Simply remove directory %s",
		       activeDir, backupDir);
	    rids = 0;
	    ierr = 0;
	    ibis::util::removeDir(backupDir);
	    ibis::util::removeDir(activeDir, true);
	    return ierr;
	}

	std::string fn(activeDir);
	fn += FASTBIT_DIRSEP;
	fn += "-rids";
	rids = new ibis::RIDSet;
	jerr = ibis::fileManager::instance().getFile(fn.c_str(), *rids);
	if (jerr) {
	    if (ibis::gVerbose > 3)
		logMessage("rollback", "the file manager failed to "
			   "read the rids from file \"%s\"", fn.c_str());
	    delete rids;
	    rids = 0;
	}

	if (rids != 0 && nEvents != rids->size())
	    nEvents = rids->size();
	if (ibis::gVerbose > -1) { // switched successfully
	    logMessage("rollback", "switched to use the "
		       "previous dataset with %lu rows, %lu "
		       "columns", static_cast<long unsigned>(nEvents),
		       static_cast<long unsigned>(columns.size()));
	    if (ibis::gVerbose > 3) {
		ibis::util::logger lg;
		print(lg());
	    }
	}
	amask.adjustSize(nEvents, nEvents);
	if (amask.cnt() < amask.size()) {
	    std::string mskfile(activeDir);
	    if (! mskfile.empty())
		mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    amask.write(mskfile.c_str());
	    ibis::fileManager::instance().flushFile(mskfile.c_str());
	}
	else {
	    std::string mskfile(activeDir);
	    if (! mskfile.empty())
		mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    remove(mskfile.c_str());
	}
	state = UNKNOWN_STATE;
	makeBackupCopy();
    }
    catch (const char* s) { // revert to previous state
	logWarning("rollback", "received the following error message, "
		   "will reverse changes made so far.\n%s", s);
	state = UNKNOWN_STATE;
	makeBackupCopy();
	ierr = -2031;
    }
    catch (...) {
	logWarning("rollback", "received a unknown exception, "
		   "will reverse changes made so far.");
	state = UNKNOWN_STATE;
	makeBackupCopy();
	ierr = -2030;
 	throw; // can not handle unknown error -- rethrow exception
    }

    return ierr;
} // ibis::part::rollback

/// Commit the active database.  No longer able to rollback afterward.
/// Return the number of records committed.
long ibis::part::commit(const char* dir) {
    long ierr = 0;
    if (state == STABLE_STATE || readonly)
	return ierr;
    if (backupDir == 0 || *backupDir == 0 || activeDir == 0)
	return ierr;
    if (dir == 0)
	return ierr;
    if (*dir == 0)
	return ierr;

    if (state == RECEIVING_STATE) {// switch to new data first
	ierr = append(dir);
	if (ierr < 0) {
	    logWarning("commit", "function append(%s) returned %ld",
		       dir, ierr);
	    return ierr;
	}
    }

    ibis::util::mutexLock lock(&mutex, "part::commit");
    try {
	ierr = appendToBackup(dir); // make the backup copy
	state = POSTTRANSITION_STATE;
	if (verifyBackupDir() == 0) {
	    ibis::fileManager::instance().flushDir(backupDir);
	    state = STABLE_STATE;
	    // rewrite the table.tdc files to show the correct state
	    writeMetaData(nEvents, columns, activeDir);
	    writeMetaData(nEvents, columns, backupDir);

	    if (amask.cnt() < amask.size()) {
		std::string mskfile(backupDir);
		if (! mskfile.empty())
		    mskfile += FASTBIT_DIRSEP;
		mskfile += "-part.msk";
		amask.write(mskfile.c_str());
		ibis::fileManager::instance().flushFile(mskfile.c_str());
	    }
	    LOGGER(ibis::gVerbose > 0)
		<< "part[" << name() << "]::commit -- successfully "
		"integrated new data from " << dir << ", nrows=" << nEvents;
	}
	else {
	    logWarning("commit", "failed to integrate new data into "
		       "the backup directory, will copy all files from "
		       "%s to %s.", activeDir, backupDir);
	    state = UNKNOWN_STATE;
	    makeBackupCopy();
	}
    }
    catch (const char* s) { // revert to previous state
	logWarning("commit", "received the following error message, "
		   "will reverse changes made so far.\n%s", s);
	state = UNKNOWN_STATE;
	makeBackupCopy();
	ierr = -2001;
    }
    catch (...) {
	logWarning("commit", "received a unknown exception, "
		   "will reverse changes made so far.");
	state = UNKNOWN_STATE;
	makeBackupCopy();
	ierr = -2000;
 	throw; // can not handle unknown error -- rethrow exception
    }

    return ierr;
} // ibis::part::commit

/// Append data in @c dir to the partition in the backup directory.
/// Return the number of rows actually appended.
long ibis::part::appendToBackup(const char* dir) {
    long ierr = 0;
    if (dir == 0 || *dir == 0)
	return ierr;
    if (backupDir == 0 || *backupDir == 0 || readonly)
	return -1;
    if (strcmp(dir, backupDir) == 0)
	return -1;

    uint32_t napp;
    columnList clist; // combined list of attributes
    columnList::iterator cit, pit;

    ibis::fileManager::instance().flushDir(backupDir);
    ierr = readMetaData(napp, clist, dir); // read table.tdc in dir
    if (ierr <= 0 || napp == 0) {
	if (ibis::gVerbose > 0)
	    logMessage("appendToBackup", "no data in the specified source "
		       "directory (%s), maybe missing table.tdc",
		       dir);
	return ierr;
    }

    bool has_rids = true;
    if (nEvents > 0) {
	if (rids == 0)
	    has_rids = false;
	else if (rids->empty())
	    has_rids = false;
    }
    std::string fn;
    fn = dir;
    fn += FASTBIT_DIRSEP;
    fn += "-rids";
    long tmp = ibis::util::getFileSize(fn.c_str());
    if (tmp > 0) {
	tmp /= sizeof(ibis::rid_t);
	if (static_cast<uint32_t>(tmp) != napp) {
	    logWarning("appendToBackup", "table.tdc file indicates that "
		       "directory %s has %lu rows, but there are %ld "
		       "rids.  Assume %ld rows are available.", dir,
		       static_cast<long unsigned>(napp), tmp, tmp);
	    napp = tmp;
	}
	has_rids = true;
    }
    else {
	has_rids = false;
    }

    if (ibis::gVerbose > 1)
	logMessage("appendToBackup", "starting to append new data in \"%s\""
		   "(%lu rows) to %s", dir, static_cast<long unsigned>(napp),
		   backupDir);
    ibis::horometer timer;
    if (ibis::gVerbose > 0)
	timer.start();

    // meta tags were not included when fetching raw data, need to add them
    for (ibis::resource::vList::const_iterator mit = metaList.begin();
	 mit != metaList.end();
	 ++ mit) {
	cit = clist.find((*mit).first);
	if (cit == clist.end()) { // need to add a new column
	    ibis::category* prop =
		new ibis::category(this, (*mit).first, (*mit).second,
				   dir, napp);
	    clist[prop->name()] = prop;
	}
    }

    // integerate the two column list, the combined list is stored in clist
    for (cit = clist.begin(); cit != clist.end(); ++cit) {
	pit = columns.find((*cit).first);
	if (pit != columns.end()) { // update the min/max pair
	    if ((*pit).second->upperBound() >
		(*pit).second->lowerBound()) {
		if ((*pit).second->upperBound() >
		    (*cit).second->upperBound())
		    (*cit).second->upperBound((*pit).second->upperBound());
		if ((*pit).second->lowerBound() <
		    (*cit).second->lowerBound())
		    (*cit).second->lowerBound((*pit).second->lowerBound());
	    }
	}
    }
    for (pit = columns.begin(); pit != columns.end(); ++pit) {
	cit = clist.find((*pit).first);
	if (cit == clist.end()) { // attribute in columns but not in clist
	    ibis::column* prop = 0;
	    if ((*pit).second->type() == ibis::CATEGORY) {
		prop = new ibis::category(*((*pit).second));
	    }
	    else if ((*pit).second->type() == ibis::TEXT) {
		prop = new ibis::text(*((*pit).second));
	    }
	    else {
		prop = new ibis::column(*((*pit).second));
	    }
	    clist[prop->name()] = prop;
	}
    }
    if (ibis::gVerbose > 6) {
	ibis::util::logger lg;
	lg() << "part::appendToBackup -- The combined (new) "
	    "attribute list (" << clist.size() << ")\n";
	for (cit = clist.begin(); cit != clist.end(); ++cit)
	    lg() << *((*cit).second) << "\n";
    }

    ibis::fileManager::buffer<char> mybuf;
    char* buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    // ibis::fileManager::increaseUse(nbuf, "appendToBackup");
    uint32_t nold = nEvents;
    if (state == TRANSITION_STATE)
	nold -= napp;

    if (has_rids) {
	// integrate the RID lists, temporarily create a RID column
	ibis::column* m_rids = new ibis::column(this, ibis::OID, "-rids");
	ierr = m_rids->append(backupDir, dir, nold, napp, nbuf, buf);
	delete m_rids;
	if ((uint32_t)ierr != napp) {
	    logWarning("appendToBackup", "expected %lu new RIDs but got %ld.  "
		       "Removing file rids.", static_cast<long unsigned>(napp),
		       ierr);
	    fn = backupDir;
	    fn += FASTBIT_DIRSEP;
	    fn += "-rids";
	    remove(fn.c_str());
	    fn += ".srt";
	    remove(fn.c_str());
	}
	else if (ibis::gVerbose > 1) {
	    logMessage("appendToBackup", "completed appending %lu RIDs",
		       static_cast<long unsigned>(napp));
	}
    }
    else if (ibis::gVerbose > 5) {
	logMessage("appendToBackup", "no RID column");
    }
    ierr = napp;

    // go through each column in the combined column list
    for (cit = clist.begin(); cit != clist.end(); ++cit) {
	if (ibis::gVerbose > 14)
	    logMessage("appendToBackup", "processing %s (%s)", (*cit).first,
		       ibis::TYPESTRING[(*cit).second->type()]);
	tmp = (*cit).second->append(backupDir, dir, nold, napp,
				    nbuf, buf);
	if (tmp != ierr)
	    logWarning("appendToBackup", "expected to add %ld elements "
		       "of \"%s\", but actually added %ld", ierr,
		       (*cit).first, tmp);
	else if (ibis::gVerbose > 3)
	    logMessage("appendToBackup", "completed processing %s",
		       (*cit).first);

	// the lower and upper bounds have not been set, set them the
	// actual min and max values
	if (tmp == ierr && (*cit).second->elementSize() > 0 &&
	    (*cit).second->lowerBound() > (*cit).second->upperBound())
	    (*cit).second->computeMinMax(backupDir);
    }

    // ibis::fileManager::decreaseUse(nbuf, "appendToBackup");
    if (ibis::gVerbose > 0) {
	timer.stop();
	logMessage("appendToBackup", "completed integrating %lu rows into %s, "
		   "took %g sec(CPU), %g sec(elapsed) ",
		   static_cast<long unsigned>(napp), backupDir,
		   timer.CPUTime(), timer.realTime());
    }

    // rewrite table.tdc in the backup directory
    writeMetaData(nold+napp, clist, backupDir);
    // clear clist
    for (cit = clist.begin(); cit != clist.end(); ++ cit)
	delete (*cit).second;
    clist.clear();
    return ierr;
} // ibis::part::appendToBackup

/// Mark the rows identified in @c rows as inactive.  Return the number of
/// rows inactive or error code.
///
/// @note Inactive rows will no longer participate in future query
/// evaluations.
long ibis::part::deactivate(const ibis::bitvector& rows) {
    if (readonly)
	return -1;

    std::string mskfile(activeDir);
    if (! mskfile.empty())
	mskfile += FASTBIT_DIRSEP;
    mskfile += "-part.msk";

    writeLock lock(this, "deactivate");
    amask.adjustSize(rows.size(), rows.size());
    amask -= rows;
    if (amask.cnt() < amask.size()) {
	amask.write(mskfile.c_str());
	ibis::fileManager::instance().flushFile(mskfile.c_str());
    }
    LOGGER(ibis::gVerbose > 0)
	<< "part[" << (m_name?m_name:"?") << "]::deactivate marked "
	<< rows.cnt() << " row" << (rows.cnt()>1?"s":"")
	<< " as inactive, leaving " << amask.cnt() << " active row"
	<< (amask.cnt()>1?"s":"") << " out of " << amask.size();
    return (amask.size() - amask.cnt());
} // ibis::part::deactivate

/// Mark the rows identified in @c rows as active.
long ibis::part::reactivate(const ibis::bitvector& rows) {
    if (readonly)
	return -1;

    std::string mskfile(activeDir);
    if (! mskfile.empty())
	mskfile += FASTBIT_DIRSEP;
    mskfile += "-part.msk";

    writeLock lock(this, "reactivate");
    amask.adjustSize(rows.size(), rows.size());
    amask |= rows;
    if (amask.cnt() < amask.size())
	amask.write(mskfile.c_str());
    else
	remove(mskfile.c_str());
    ibis::fileManager::instance().flushFile(mskfile.c_str());
    LOGGER(ibis::gVerbose > 0)
	<< "part[" << (m_name?m_name:"?") << "]::reactivate marked "
	<< rows.cnt() << " row" << (rows.cnt()>1?"s":"")
	<< " as active, leaving " << amask.cnt() << " active row"
	<< (amask.cnt()>1?"s":"") << " out of " << amask.size();
    return amask.cnt();
} // ibis::part::reactivate

/// Mark the specified rows as inactive.
/// The integers in array rows are simply the row numbers.  Note rows are
/// numbered starting from 0.  Return the number of
/// rows inactive or error code.
///
/// @note Inactive rows will no longer participate in future query
/// evaluations.
long ibis::part::deactivate(const std::vector<uint32_t>& rows) {
    if (readonly)
	return -1;

    if (rows.empty() || nEvents == 0) return 0;

    ibis::bitvector msk;
    numbersToBitvector(rows, msk);
    if (msk.cnt() > 0)
	return deactivate(msk);
    else
	return 0;
} // ibis::part::deactivate

/// Mark all rows satisfying the specified conditions as inactive.
/// All rows satisfying the specified conditions will be made inactive.
/// Return the number of rows inactive or error code.
///
/// @note All inactive rows will no longer participate in any future query
/// processing.
long ibis::part::deactivate(const char* conds) {
    if (readonly)
	return -1;
    if (conds == 0 || *conds == 0 || nEvents == 0)
	return 0;

    ibis::bitvector msk;
    stringToBitvector(conds, msk);
    LOGGER(ibis::gVerbose > 1)
	<< "part[" << (m_name?m_name:"?") << "]::deactivate translated \""
	<< conds << "\" into " << msk.cnt() << " row" << (msk.cnt()>1?"s":"");
    if (msk.cnt() > 0)
	return deactivate(msk);
    else
	return 0;
} // ibis::part::deactivate

/// Make sure the specified rows are active.  Return the total number
/// of active rows or error code.
long ibis::part::reactivate(const std::vector<uint32_t>& rows) {
    if (readonly)
	return -1;
    if (rows.empty() || nEvents == 0)
	return 0;

    ibis::bitvector msk;
    numbersToBitvector(rows, msk);
    if (msk.cnt() > 0)
	return reactivate(msk);
    else
	return amask.cnt();
} // ibis::part::reactivate

/// Make sure the rows satisfying the specified conditionis are active.
long ibis::part::reactivate(const char* conds) {
    if (readonly)
	return -1;
    if (conds == 0 || *conds == 0 || nEvents == 0)
	return 0;

    ibis::bitvector msk;
    stringToBitvector(conds, msk);
    LOGGER(ibis::gVerbose > 1)
	<< "part[" << (m_name?m_name:"?") << "]::reactivate translated \""
	<< conds << "\" into " << msk.cnt() << " row" << (msk.cnt()>1?"s":"");
    if (msk.cnt() > 0)
	return reactivate(msk);
    else
	return amask.cnt();
} // ibis::part::reactivate

/// Purge all inactive rows from the partition.
/// Return the number of rows left or error code.
/// @note This operations is permanent and irreversible!
long ibis::part::purgeInactive() {
    if (readonly)
	return -1;
    int ierr = 0; 
    ibis::util::mutexLock lock(&mutex, "part::purgeInactive");
    if (amask.cnt() == amask.size()) return nEvents;

    LOGGER(ibis::gVerbose > 0)
	<< "part[" << (m_name?m_name:"?") << "]::purgeInactive to remove "
	<< amask.size()-amask.cnt() << " out of " << amask.size() << " row"
	<< (amask.size()>1?"s":"");

    ibis::fileManager::buffer<char> buf_;
    char *mybuf = buf_.address();
    uint32_t nbuf = buf_.size();

    if (backupDir != 0 && *backupDir != 0) { // has backup dir
	ibis::fileManager::instance().flushDir(backupDir);
	for (columnList::iterator it = columns.begin();
	     it != columns.end();
	     ++ it) {
	    ibis::column& col = *((*it).second);
	    const long itmp = col.saveSelected(amask, backupDir, mybuf, nbuf);
	    if (itmp < 0 && ibis::gVerbose > 1)
		logMessage("purgeInactive", "saving selected values for "
			   "column %s failed with error code %ld",
			   col.name(), itmp);
	    if (itmp < 0) {
		ierr = itmp;
		break;
	    }
	    else if (ierr == 0 && itmp == static_cast<long>(amask.cnt())) {
		ierr = itmp;
	    }
	}

	if (ierr == (long) amask.cnt()) { // wrote selected values successfully
	    if (rids != 0 && rids->size() == nEvents) {
		ibis::column rcol(this, ibis::OID, "-rids");
		rcol.saveSelected(amask, backupDir, mybuf, nbuf);
	    }
	    std::string mskfile(backupDir);
	    mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    remove(mskfile.c_str());
	    writeMetaData(amask.cnt(), columns, backupDir);

	    writeLock rw(this, "append");
	    unloadIndexes();	// remove all indices
	    delete rids;	// remove the RID list
	    rids = 0;
	    ibis::fileManager::instance().flushDir(activeDir);
	    for (columnList::iterator it = columns.begin();
		 it != columns.end(); ++it)
		delete (*it).second;
	    columns.clear();
	    amask.set(1, nEvents);
	    mskfile = activeDir;
	    mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    remove(mskfile.c_str());

	    // switch the directory name and read the rids
	    char* tstr = activeDir;
	    activeDir = backupDir;
	    backupDir = tstr;
	    readMetaData(nEvents, columns, activeDir);
	    readRIDs();
	}
	else {
	    logWarning("purgeInactive", "operations to permanently remove "
		       "inactive rows in %s have failed, will restore the "
		       "content from %s", backupDir, activeDir);
	}

	makeBackupCopy();
    }
    else { // only have one directory
	writeLock lock(this, "purgeInactive");
	ibis::fileManager::instance().flushDir(activeDir);
	for (columnList::iterator it = columns.begin();
	     it != columns.end();
	     ++ it) {
	    ibis::column& col = *((*it).second);
	    const long itmp = col.saveSelected(amask, activeDir, mybuf, nbuf);
	    if (itmp < 0 && ibis::gVerbose > 1)
		logMessage("purgeInactive", "saving selected values for "
			   "column %s failed with error code %ld",
			   col.name(), itmp);
	    if (itmp < 0) {
		ierr = itmp;
		break;
	    }
	    else if (ierr == 0 && itmp == static_cast<long>(amask.cnt())) {
		ierr = itmp;
	    }
	}

	if (ierr == (long) amask.cnt()) {
	    if (rids != 0 && rids->size() == nEvents) {
		ibis::column rcol(this, ibis::OID, "-rids");
		rcol.saveSelected(amask, activeDir, mybuf, nbuf);

		delete rids;
		rids = 0;
		readRIDs();
	    }
	    nEvents = amask.cnt();
	    amask.set(1, nEvents);
	    std::string mskfile(activeDir);
	    if (! mskfile.empty())
		mskfile += FASTBIT_DIRSEP;
	    mskfile += "-part.msk";
	    remove(mskfile.c_str());
	    writeMetaData(nEvents, columns, activeDir);
	}
	else {
	    logWarning("purgeInactive", "operations to permanently remove "
		       "inactive rows in %s have failed, the data files are "
		       "no longer consistent", activeDir);
	}
    }

    return ierr;
} // ibis::part::purgeInactive

/// Empty all unused resources in cache.
/// This function attempts to unload all the indexes and then remove all
/// unused files from the file manager.
void ibis::part::emptyCache() const {
    if (myCleaner != 0)
	(*myCleaner)(); // invoke the cleaner
    if (backupDir != 0 && *backupDir != 0)
	ibis::fileManager::instance().flushDir(backupDir);
    if (activeDir != 0 && *activeDir != 0)
	ibis::fileManager::instance().flushDir(activeDir);
} // ibis::part::emptyCache

/// Add a column computed with the given arithmetic expression.
/// The arithmetic expression is evaluated in double and casted to the
/// specified type.
long ibis::part::addColumn(const char* aexpr, const char* cname,
			   ibis::TYPE_T ctype) {
    if (readonly || aexpr == 0 || cname == 0 || *aexpr == 0 || *cname == 0)
	return -1L;

    ibis::selectClause xpr(aexpr);
    if (xpr.aggSize() != 1) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- part[" << name()
	    << "]::addColumn expects to parse \"" << aexpr
	    << "\" into a single arithmetic expression, but it got "
	    << xpr.aggSize();
	return -2L;
    }

    ibis::bitvector mask;
    xpr.getNullMask(*this, mask);
    return addColumn(xpr.aggExpr(0), mask, cname, ctype);
} // ibis::part::addColumn

/// Add a column computed with the given arithmetic expression.
long ibis::part::addColumn(const ibis::math::term* xpr,
			   ibis::bitvector& mask, const char* cname,
			   ibis::TYPE_T ctype) {
    if (readonly || xpr == 0 || cname == 0 || *cname == 0)
	return -1L;

    array_t<double> vals;
    long ierr = calculate(*xpr, mask, vals);
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- part[" << name() << "]::addColumn(" << xpr
	    << ") failed to evaluate the arithmetic expression, ierr = "
	    << ierr;
	return -3L;
    }
    else if (static_cast<unsigned long>(ierr) != mask.cnt()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- part[" << name() << "]::addColumn(" << xpr
	    << ") expected to receive " << mask.cnt() << " values, but got "
	    << ierr;
	return -4L;
    }

    std::ostringstream oss;
    oss << "Select " << *xpr << " From " << name();
    ibis::column *xcol =
	new ibis::column(this, ctype, cname, oss.str().c_str());
    switch (ctype) {
    default:
    case ibis::DOUBLE: {
	if (vals.size() == mask.size()) {
	    ierr = xcol->writeData(activeDir, 0U, mask.size(), mask,
				   vals.begin(), 0);
	}
	else {
	    ierr = xcol->castAndWrite(vals, mask, FASTBIT_DOUBLE_NULL);
	}
	break;}
    case ibis::FLOAT: {
	ierr = xcol->castAndWrite(vals, mask, FASTBIT_FLOAT_NULL);
	break;}
    case ibis::ULONG: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFLL));
	break;}
    case ibis::LONG: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<int64_t>(0x7FFFFFFFFFFFFFFFLL));
	break;}
    case ibis::UINT: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<uint32_t>(0xFFFFFFFFL));
	break;}
    case ibis::INT: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<int32_t>(0x7FFFFFFFL));
	break;}
    case ibis::USHORT: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<uint16_t>(0xFFFF));
	break;}
    case ibis::SHORT: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<int16_t>(0x7FFF));
	break;}
    case ibis::UBYTE: {
	ierr = xcol->castAndWrite(vals, mask,
				  static_cast<unsigned char>(0));
	break;}
    case ibis::BYTE: {
	ierr = xcol->castAndWrite(vals, mask, static_cast<char>(0));
	break;}
    } // switch
    if (ierr == static_cast<long>(mask.size())) { // success
	LOGGER(ibis::gVerbose > 2)
	    << "part[" << name() << "]::addColumn successfully wrote "
	    << ierr << " value" << (ierr > 1 ? "s" : "") << " for "
	    << cname << "(" << oss.str() << ")";
	ibis::util::mutexLock lock(&mutex, "part::addColumn");
	columns[xcol->name()] = xcol;
	ierr = 0;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- part[" << name()
	    << "]::addColumn failed to write" << mask.size() << " values for "
	    << cname << ", only wrote " << ierr;
	delete xcol;
	ierr = -5;
    }
    return ierr;
} // ibis::part::addColumn

/// Check the time stamp on the metadata files to update the in-memory
/// metadata information.
int ibis::part::updateData() {
    emptyCache();
    if (activeDir == 0 || *activeDir == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- part[" << name() << "]::updateData can not proceed "
	    "because the activeDir is not defined";
	return -1;
    }

    softWriteLock lock(this, "updateData");
    if (lock.isLocked() == false) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- part[" << name() << "]::updateData can not proceed, "
	    "must free all queries and stop other accesses before continuing";
	return -2;
    }
    LOGGER(ibis::gVerbose > 3)
	<< "part[" << name() << "]::updateData to check time stamps on "
	<< activeDir << "/-part.txt and " << activeDir << "/-part.msk";

    int ierr;
    time_t t0;
    Stat_T st;
    std::string fn = activeDir;
    fn += FASTBIT_DIRSEP;
    fn += "-part.txt";
    ierr = UnixStat(fn.c_str(), &st);
    if (ierr != 0) {
	fn.erase(fn.size() - 9);
	fn += "table.tdc";
	ierr = UnixStat(fn.c_str(), &st);
    }
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- part[" << name() << "]::updateData failed to "
	    "determine the status of -part.txt, errno = " << errno
	    << " (" << strerror(errno) << ')';
	return -3;
    }
    t0 = (st.st_ctime >= st.st_mtime ? st.st_ctime : st.st_mtime);
    fn.erase(fn.size()-9);
    fn += "-part.msk";
    ierr = UnixStat(fn.c_str(), &st);
    if (ierr == 0) {
	if (t0 < st.st_ctime)
	    t0 = st.st_ctime;
	else if (t0 < st.st_mtime)
	    t0 = st.st_mtime;
    }
    if (switchTime > t0) return 0; // up-to-date

    if (ierr == 0 && (switchTime <= st.st_ctime || switchTime <= st.st_mtime))
	amask.read(fn.c_str()); // re-read the mask file
    readMetaData(nEvents, columns, activeDir); // re-read column information
    amask.adjustSize(nEvents, nEvents);
    return 0;
} // ibis::part::updateData

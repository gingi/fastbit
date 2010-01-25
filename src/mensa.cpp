// File $Id$	
// author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tab.h"	// ibis::tabula and ibis::tabele
#include "bord.h"	// ibis::bord
#include "mensa.h"	// ibis::mensa
#include "part.h"	// ibis::part
#include "countQuery.h"	// ibis::countQuery
#include "index.h"	// ibis::index
#include "category.h"	// ibis::text
#include "selectClause.h"//ibis::selectClause

#include <algorithm>	// std::sort
#include <sstream>	// std::ostringstream
#include <limits>	// std::numeric_limits
#include <cmath>	// std::floor

/// This function expects a valid data directory to find data partitions.
/// If the incoming directory is not a valid string, it will use
/// ibis::gParameter() to find data partitions.
ibis::mensa::mensa(const char* dir) : nrows(0) {
    if (dir != 0 && *dir != 0)
	ibis::util::gatherParts(parts, dir, true);
    if (parts.empty())
	ibis::util::gatherParts(parts, ibis::gParameters(), true);
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it)->combineNames(naty);
	nrows += (*it)->nRows();
    }
    if (name_.empty() && ! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = "T-";
	name_ += (*it)->name();
	if (desc_.empty()) {
	    if (dir != 0 && *dir != 0)
		desc_ = dir;
	    else
		desc_ = "data specified in RC file";
	}
    }
    if (ibis::gVerbose > 0 && ! name_.empty()) {
	ibis::util::logger lg;
	lg.buffer() << "ibis::mensa -- constructed table "
		    << name_ << " (" << desc_ << ") from ";
	if (dir != 0 && *dir != 0)
	    lg.buffer() << "directory " << dir;
	else
	    lg.buffer() << "RC file entries";
	lg.buffer() << ".  It consists of " << parts.size() << " partition"
		  << (parts.size()>1 ? "s" : "") << " with "
		  << naty.size() << " column"
		  << (naty.size()>1 ? "s" : "") << " and "
		  << nrows << " row" << (nrows>1 ? "s" : "");
    }
} // constructor with one directory as argument

/// This function expects a pair of data directories to define data
/// partitions.  If either dir1 and dir2 is not valid, it will attempt to
/// find data partitions using global parameters ibis::gParameters().
ibis::mensa::mensa(const char* dir1, const char* dir2) : nrows(0) {
    if (*dir1 == 0 && *dir2 == 0) return;
    if (dir1 != 0 && *dir1 != 0) {
	ibis::util::gatherParts(parts, dir1, dir2, true);
    }
    if (parts.empty())
	ibis::util::gatherParts(parts, ibis::gParameters(), true);
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it)->combineNames(naty);
	nrows += (*it)->nRows();
    }
    if (name_.empty() && ! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = "T-";
	name_ += (*it)->name();
	if (desc_.empty()) {
	    if (dir1 != 0 && *dir1 != 0) {
		desc_ = dir1;
		if (dir2 != 0 && *dir2 != 0) {
		    desc_ += " + ";
		    desc_ += dir2;
		}
	    }
	    else {
		desc_ = "data specified in RC file";
	    }
	}
    }
    if (ibis::gVerbose > 0 && ! name_.empty()) {
	ibis::util::logger lg;
	lg.buffer() << "ibis::mensa -- constructed table "
		    << name_ << " (" << desc_ << ") from ";
	if (dir1 != 0 && *dir1 != 0) {
	    if (dir2 != 0 && *dir2 != 0)
		lg.buffer() << "directories " << dir1 << " + " << dir2;
	    else
		lg.buffer() << "directory " << dir1;
	}
	else {
	    lg.buffer() << "RC file entries";
	}
	lg.buffer() << ".  It consists of " << parts.size() << " partition"
		    << (parts.size()>1 ? "s" : "") << " with "
		    << naty.size() << " column"
		    << (naty.size()>1 ? "s" : "") << " and "
		    << nrows << "row" << (nrows>1 ? "s" : "");
    }
} // constructor with two directories as arguments

/// Add data partitions defined in the named directory.  It uses opendir
/// and friends to traverse the subdirectories, which means it will only
/// able to descend to subdirectories on unix and compatible systems.
int ibis::mensa::addPartition(const char* dir) {
    const uint32_t npold = parts.size();
    const uint32_t ncold = naty.size();
    const uint64_t nrold = nrows;
    unsigned int newparts = 0;
    if (dir != 0 && *dir != 0)
	newparts = ibis::util::gatherParts(parts, dir, true);
    else
	newparts = ibis::util::gatherParts(parts, ibis::gParameters(), true);
    if (newparts == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "ibis::mensa::addPartition(" << dir
	    << ") did not find any valid data partition";
	return -2;
    }
    LOGGER(ibis::gVerbose > 1)
	<< "ibis::mensa::addPartition(" << dir << ") found " << newparts
	<< " new data partition" << (newparts > 1 ? "s" : "");

    nrows = 0;
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it)->combineNames(naty);
	nrows += (*it)->nRows();
    }

    if (name_.empty() && ! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = "T-";
	name_ += (*it)->name();
	if (desc_.empty()) {
	    if (dir != 0 && *dir != 0)
		desc_ = dir;
	    else
		desc_ = "data specified in RC file";
	}
    }
    LOGGER(ibis::gVerbose > 0)
	<< "ibis::mensa::addPartition(" << dir
	<< ") increases the number partitions from " << npold << " to "
	<< parts.size() << ", the number of rows from " << nrold << " to "
	<< nrows << ", and the number of columns from " << ncold << " to "
	<< naty.size();
    return newparts;
} // ibis::mensa::addPartition

int ibis::mensa::getPartitions(std::vector<const ibis::part*> &lst) const {
    lst.resize(parts.size());
    for (uint32_t i = 0; i < parts.size(); ++ i)
	lst[i] = parts[i];
    return parts.size();
} // ibis::mensa::getPartitions

void ibis::mensa::clear() {
    const uint32_t np = parts.size();
    LOGGER(ibis::gVerbose > 2 && np > 0)
	<< "ibis::mensa::clear -- clearing the existing content of "
	<< np << " partition" << (np>1 ? "s" : "") << " with "
	<< naty.size() << " column" << (naty.size()>1 ? "s" : "")
	<< " and " << nrows << " row" << (nrows>1 ? "s" : "");

    nrows = 0;
    naty.clear();
    name_.erase();
    desc_.erase();
    for (uint32_t j = 0; j < np; ++ j)
	delete parts[j];
} // ibis::mensa::clear

ibis::table::stringList ibis::mensa::columnNames() const {
    ibis::table::stringList res(naty.size());
    uint32_t i = 0;
    for (ibis::table::namesTypes::const_iterator it = naty.begin();
	 it != naty.end(); ++ it, ++ i)
	res[i] = (*it).first;
    return res;
} // ibis::mensa::columnNames
ibis::table::typeList ibis::mensa::columnTypes() const {
    ibis::table::typeList res(naty.size());
    uint32_t i = 0;
    for (ibis::table::namesTypes::const_iterator it = naty.begin();
	 it != naty.end(); ++ it, ++ i)
	res[i] = (*it).second;
    return res;
} // ibis::mensa::columnTypes

void ibis::mensa::describe(std::ostream& out) const {
    out << "Table (on disk) " << name_ << " (" << desc_ << ") consists of "
	<< parts.size() << " partition" << (parts.size()>1 ? "s" : "")
	<< " with "<< naty.size() << " column" << (naty.size()>1 ? "s" : "")
	<< " and " << nrows << " row" << (nrows>1 ? "s" : "");
    for (ibis::table::namesTypes::const_iterator it = naty.begin();
	 it != naty.end(); ++ it)
	out << "\n" << (*it).first << "\t" << ibis::TYPESTRING[(*it).second];
    out << std::endl;
} // ibis::mensa::describe

void ibis::mensa::dumpNames(std::ostream& out, const char* del) const {
    if (naty.empty()) return;

    ibis::table::namesTypes::const_iterator it = naty.begin();
    out << (*it).first;
    for (++ it; it != naty.end(); ++ it)
	out << del << (*it).first;
    out << std::endl;
} // ibis::mensa::dumpNames

const char* ibis::mensa::indexSpec(const char* colname) const {
    if (parts.empty()) {
	return 0;
    }
    else if (colname == 0 || *colname == 0) {
	return parts[0]->indexSpec();
    }
    else {
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::column* col = (*it)->getColumn(colname);
	    if (col != 0)
		return col->indexSpec();
	}
	return parts[0]->indexSpec();
    }
} // ibis::mensa::indexSpec

void ibis::mensa::indexSpec(const char* opt, const char* colname) {
    for (ibis::partList::iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	if (colname == 0 || *colname == 0) {
	    (*it)->indexSpec(opt);
	}
	else {
	    ibis::column* col = (*it)->getColumn(colname);
	    if (col != 0)
		col->indexSpec(opt);
	}
    }
} // ibis::mensa::indexSpec

int ibis::mensa::buildIndex(const char* colname, const char* option) {
    if (colname == 0 || *colname == 0) return -1;

    int ierr = 0;
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	const ibis::column* col = (*it)->getColumn(colname);
	if (col != 0) {
	    ibis::index* ind = ibis::index::create(col, 0, option);
	    if (ind != 0) {
		++ ierr;
		delete ind;
	    }
	    else {
		LOGGER(ibis::gVerbose > 1)
		    << "ibis::mensa::buildIndex(" << colname << ", "
		    << (option != 0 ? option : col->indexSpec()) << ") failed";
	    }
	}
    }

    if (ierr == 0)
	ierr = -2;
    else if ((unsigned)ierr < parts.size())
	ierr = 1;
    else
	ierr = 0;
    return ierr;
} // ibis::mensa::buildIndex

int ibis::mensa::buildIndexes(const char* opt) {
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it)->loadIndexes(opt);
	(*it)->unloadIndexes();
    }
    return 0;
} // ibis::mensa::buildIndexes

void ibis::mensa::estimate(const char* cond,
			   uint64_t& nmin, uint64_t& nmax) const {
    nmin = 0;
    nmax = 0;
    ibis::countQuery qq;
    int ierr = qq.setWhereClause(cond);
    if (ierr < 0) {
	nmax = nRows();
	return;
    }

    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	if (ierr >= 0) {
	    ierr = qq.setPartition(*it);
	    if (ierr >= 0) {
		ierr = qq.estimate();
		if (ierr >= 0) {
		    nmin += qq.getMinNumHits();
		    nmax += qq.getMaxNumHits();
		}
		else {
		    nmax += (*it)->nRows();
		}
	    }
	    else {
		nmax += (*it)->nRows();
	    }
	    ierr = 0;
	}
	else {
	    nmax += (*it)->nRows();
	}
    }
} // ibis::mensa::estimate

void ibis::mensa::estimate(const ibis::qExpr* cond,
			   uint64_t& nmin, uint64_t& nmax) const {
    nmin = 0;
    nmax = 0;
    ibis::countQuery qq;
    int ierr = qq.setWhereClause(cond);
    if (ierr < 0) {
	nmax = nRows();
	return;
    }

    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	if (ierr >= 0) {
	    ierr = qq.setPartition(*it);
	    if (ierr >= 0) {
		ierr = qq.estimate();
		if (ierr >= 0) {
		    nmin += qq.getMinNumHits();
		    nmax += qq.getMaxNumHits();
		}
		else {
		    nmax += (*it)->nRows();
		}
	    }
	    else {
		nmax += (*it)->nRows();
	    }
	    ierr = 0;
	}
	else {
	    nmax += (*it)->nRows();
	}
    }
} // ibis::mensa::estimate

ibis::table* ibis::mensa::select(const char* sel, const char* cond) const {
    if (cond == 0 || *cond == 0) return 0;
    if (sel != 0) // skip leading space
	while (isspace(*sel)) ++ sel;
    if (sel == 0 || *sel == 0) {
	int64_t nhits = computeHits(cond);
	if (nhits < 0) {
	    return 0;
	}
	else {
	    std::string des = name_;
	    if (! desc_.empty()) {
		des += " -- ";
		des += desc_;
	    }
	    return new ibis::tabula(cond, des.c_str(), nhits);
	}
    }
    else if (stricmp(sel, "count(*)") == 0) { // count(*)
	int64_t nhits = computeHits(cond);
	if (nhits < 0) {
	    return 0;
	}
	else {
	    std::string des = name_;
	    if (! desc_.empty()) {
		des += " -- ";
		des += desc_;
	    }
	    return new ibis::tabele(cond, des.c_str(), nhits);
	}
    }
    else {
	// handle the non-trivial case in a separate function
	return ibis::table::select
	    (reinterpret_cast<const std::vector<const ibis::part*>&>(parts),
	     sel, cond);
    }
} // ibis::mensa::select

/// A variation of the function select defined in ibis::table.  It accepts
/// an extra argument for caller to specify a list of names of data
/// partitions that will participate in the select operation.  The argument
/// pts may contain wild characters accepted by SQL function 'LIKE', more
/// specifically, '_' and '%'.  If the argument pts is a nil pointer or an
/// empty string
ibis::table* ibis::mensa::select2(const char* sel, const char* cond,
				  const char* pts) const {
    if (cond == 0 || *cond == 0 || pts == 0 || *pts == 0) return 0;
    while (isspace(*pts)) ++ pts;
    while (isspace(*cond)) ++ cond;
    if (sel != 0) // skip leading space
	while (isspace(*sel)) ++ sel;
    if (*pts == 0) return 0;
    if (*pts == '%' || *pts == '*') return select(sel, cond);

    ibis::nameList patterns(pts);
    if (patterns.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::mensa::select2 can not find any valid data partition "
	    "names to use";
	return 0;
    }

    std::vector<const ibis::part*> mylist;
    for (unsigned k = 0; k < parts.size(); ++ k) {
	for (unsigned j = 0; j < patterns.size(); ++ j) {
	    const char* pat = patterns[j];
	    if (stricmp(pat, parts[k]->name()) == 0) {
		mylist.push_back(parts[k]);
		break;
	    }
	    else if (ibis::util::strMatch(parts[k]->name(), pat)) {
		mylist.push_back(parts[k]);
		break;
	    }
	}
    }
    if (mylist.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::mensa::select2 cannot find any data partitions "
	    "matching \"" << pts << "\"";
	return 0;
    }

    if (sel == 0 || *sel == 0) {
	int64_t nhits = ibis::table::computeHits(mylist, cond);
	if (nhits < 0) {
	    return 0;
	}
	else {
	    std::string des = name_;
	    if (! desc_.empty()) {
		des += " -- ";
		des += desc_;
	    }
	    return new ibis::tabula(cond, des.c_str(), nhits);
	}
    }
    else if (strnicmp(sel, "count(", 6) == 0) { // count(*)
	int64_t nhits = ibis::table::computeHits(mylist, cond);
	if (nhits < 0) {
	    return 0;
	}
	else {
	    std::string des = name_;
	    if (! desc_.empty()) {
		des += " -- ";
		des += desc_;
	    }
	    return new ibis::tabele(cond, des.c_str(), nhits);
	}
    }
    else {
	return ibis::table::select(mylist, sel, cond);
    }
    return 0;
} // ibis::mensa::select2

/// Reordering the rows using the specified columns.  Each data partition
/// is reordered separately.
void ibis::mensa::orderby(const ibis::table::stringList& names) {
    for (ibis::partList::iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	long ierr = (*it)->reorder(names);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "ibis::mensa::orderby -- reordering partition "
		<< (*it)->name() << " encountered error " << ierr;
	}
    }
} // ibis::mensa::orderby

int64_t ibis::mensa::getColumnAsBytes(const char* cn, char* vals) const {
    {
	ibis::table::namesTypes::const_iterator nit = naty.find(cn);
	if (nit == naty.end())
	    return -1;
	else if ((*nit).second != ibis::BYTE &&
		 (*nit).second != ibis::UBYTE)
	    return -2;
    }

    uint32_t ierr = 0;
    array_t<char> tmp;
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	const ibis::part& dp = **it;
	if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "ibis::mensa::getColumnAsBytes encounters an "
		"integer overflow problem (number of elements assigned "
		"so far is " << ierr << ")";
	    break;
	}

	const ibis::column* col = dp.getColumn(cn);
	if (col != 0 && col->getValuesArray(&tmp) >= 0)
	    memcpy(vals+ierr, tmp.begin(), dp.nRows());
	ierr += dp.nRows();
    }
    return ierr;
} // ibis::mensa::getColumnAsBytes

int64_t ibis::mensa::getColumnAsUBytes(const char* cn,
				       unsigned char* vals) const {
    {
	ibis::table::namesTypes::const_iterator nit = naty.find(cn);
	if (nit == naty.end())
	    return -1;
	else if ((*nit).second != ibis::BYTE &&
		 (*nit).second != ibis::UBYTE)
	    return -2;
    }

    uint32_t ierr = 0;
    array_t<unsigned char> tmp;
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	const ibis::part& dp = **it;
	if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "ibis::mensa::getColumnAsUBytes encounters an "
		"integer overflow problem (number of elements assigned "
		"so far is " << ierr << ")";
	    break;
	}

	const ibis::column* col = dp.getColumn(cn);
	if (col != 0 && col->getValuesArray(&tmp) >= 0)
	    memcpy(vals+ierr, tmp.begin(), dp.nRows());
	ierr += dp.nRows();
    }
    return ierr;
} // ibis::mensa::getColumnAsUBytes

int64_t ibis::mensa::getColumnAsShorts(const char* cn, int16_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(int16_t));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsShorts

int64_t ibis::mensa::getColumnAsUShorts(const char* cn, uint16_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsUShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsUShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(uint16_t));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsUShorts

int64_t ibis::mensa::getColumnAsInts(const char* cn, int32_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT:
    case ibis::UINT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(int32_t));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsInts

int64_t ibis::mensa::getColumnAsUInts(const char* cn, uint32_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsUInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT:
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsUInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(uint32_t));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsUInts

/// @note All integers 4-byte or shorter in length can be safely converted
/// into int64_t.  Values in uint64_t are treated as signed integers, which
/// may create incorrect values.
int64_t ibis::mensa::getColumnAsLongs(const char* cn, int64_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	array_t<int64_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(int64_t));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsLongs

/// @note All integers can be converted to uint64_t, however, negative 
/// integers will be treated as unsigned integers.
int64_t ibis::mensa::getColumnAsULongs(const char* cn, uint64_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT:
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	array_t<uint64_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(uint64_t));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsULongs

/// @note Integers two-byte or less in length can be converted safely to
/// floats.
int64_t ibis::mensa::getColumnAsFloats(const char* cn, float* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(float));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsFloats

/// @note Integers four-byte or less in length can be converted to double
/// safely.  Float values may also be converted into doubles.
int64_t ibis::mensa::getColumnAsDoubles(const char* cn, double* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(double));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsDoubles

int64_t ibis::mensa::getColumnAsDoubles(const char* cn,
					std::vector<double>& vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;
    try {
	vals.resize(nrows);
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::mensa::getColumnAsDoubles failed to "
	    "allocate the output std::vector<double>";
	return -3;
    }

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0)
		for (uint32_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    vals.resize(ierr);
    return ierr;
} // ibis::mensa::getColumnAsDoubles

/// @note Any data type can be converted to strings, however, the
/// conversion may take a significant amount of time.
int64_t ibis::mensa::getColumnAsStrings(const char* cn,
					std::vector<std::string>& vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    uint32_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << static_cast<int>(tmp[i]);
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << static_cast<unsigned>(tmp[i]);
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::LONG: {
	array_t<int64_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getValuesArray(&tmp) >= 0) {
		for (uint32_t i=0; i < tmp.size(); ++ i) {
		    std::ostringstream oss;
		    oss << tmp[i];
		    vals[ierr+i] = oss.str();
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::CATEGORY:
    case ibis::TEXT: {
	std::string tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0) {
		for (uint32_t i=0; i < dp.nRows(); ++ i) {
		    static_cast<const ibis::text*>(col)->getString(i, tmp);
		    vals.push_back(tmp);
		}
	    }
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsStrings

long ibis::mensa::getHistogram(const char* constraints,
			       const char* cname,
			       double begin, double end, double stride,
			       std::vector<uint32_t>& counts) const {
    long ierr = -1;
    if (cname == 0 || *cname == 0 || (begin >= end && !(stride < 0.0)) ||
	(begin <= end && !(stride > 0.0))) return ierr;
    if (sizeof(uint32_t) == sizeof(uint32_t)) {
	counts.clear();
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it)->get1DDistribution
		(constraints, cname, begin, end, stride,
		 reinterpret_cast<std::vector<uint32_t>&>(counts));
	    if (ierr < 0) return ierr;
	}
    }
    else {
	const uint32_t nbins = 1 + 
	    static_cast<uint32_t>(std::floor((end - begin) / stride));
	counts.resize(nbins);
	for (uint32_t i = 0; i < nbins; ++ i)
	    counts[i] = 0;

	std::vector<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    tmp.clear();
	    ierr = (*it)->get1DDistribution(constraints, cname, begin,
					    end, stride, tmp);
	    if (ierr < 0) return ierr;
	    for (uint32_t i = 0; i < nbins; ++ i)
		counts[i] += tmp[i];
	}
    }
    return ierr;
} // ibis::mensa::getHistogram

long ibis::mensa::getHistogram2D(const char* constraints,
				 const char* cname1,
				 double begin1, double end1, double stride1,
				 const char* cname2,
				 double begin2, double end2, double stride2,
				 std::vector<uint32_t>& counts) const {
    long ierr = -1;
    if (cname1 == 0 || cname2 == 0 || (begin1 >= end1 && !(stride1 < 0.0)) ||
	(begin1 <= end1 && !(stride1 > 0.0)) ||
	*cname1 == 0 || *cname2 == 0 ||  (begin2 >= end2 && !(stride2 < 0.0)) ||
	(begin2 <= end2 && !(stride2 > 0.0))) return ierr;

    if (sizeof(uint32_t) == sizeof(uint32_t)) {
	counts.clear();

	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it)->get2DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2,
		 reinterpret_cast<std::vector<uint32_t>&>(counts));
	    if (ierr < 0) return ierr;
	}
    }
    else {
	const uint32_t nbins =
	    (1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	    (1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2)));
	counts.resize(nbins);
	for (uint32_t i = 0; i < nbins; ++ i)
	    counts[i] = 0;

	std::vector<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it)->get2DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2, tmp);
	    if (ierr < 0) return ierr;

	    for (uint32_t i = 0; i < nbins; ++ i)
		counts[i] += tmp[i];
	}
    }
    return ierr;
} // ibis::mensa::getHistogram2D

long ibis::mensa::getHistogram3D(const char* constraints,
				 const char* cname1,
				 double begin1, double end1, double stride1,
				 const char* cname2,
				 double begin2, double end2, double stride2,
				 const char* cname3,
				 double begin3, double end3, double stride3,
				 std::vector<uint32_t>& counts) const {
    long ierr = -1;
    if (cname1 == 0 || cname2 == 0 || cname3 == 0 ||
	*cname1 == 0 || *cname2 == 0 || *cname3 == 0 ||
	(begin1 >= end1 && !(stride1 < 0.0)) ||
	(begin1 <= end1 && !(stride1 > 0.0)) ||
	(begin2 >= end2 && !(stride2 < 0.0)) ||
	(begin2 <= end2 && !(stride2 > 0.0)) ||
	(begin3 >= end3 && !(stride3 < 0.0)) ||
	(begin3 <= end3 && !(stride3 > 0.0))) return -1;

    if (sizeof(uint32_t) == sizeof(uint32_t)) {
	counts.clear();

	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it)->get3DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2, cname3, begin3, end3, stride3,
		 reinterpret_cast<std::vector<uint32_t>&>(counts));
	    if (ierr < 0) return ierr;
	}
    }
    else {
	const uint32_t nbins =
	    (1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	    (1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2))) *
	    (1 + static_cast<uint32_t>(std::floor((end3 - begin3) / stride3)));
	counts.resize(nbins);
	for (uint32_t i = 0; i < nbins; ++ i)
	    counts[i] = 0;

	std::vector<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it)->get3DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2, cname3, begin3, end3, stride3, tmp);
	    if (ierr < 0) return ierr;

	    for (uint32_t i = 0; i < nbins; ++ i)
		counts[i] += tmp[i];
	}
    }

    return ierr;
} // ibis::mensa::getHistogram3D

int ibis::mensa::dump(std::ostream& out, const char* del) const {
    ibis::mensa::cursor cur(*this);
    while (0 == cur.fetch()) {
	int ierr = cur.dumpBlock(out, del);
	if (ierr < 0)  {
	    out << " ... ierr = " << ierr << std::endl;
	    return ierr;
	}
    }
    return 0;
} // ibis::mensa::dump

ibis::table::cursor* ibis::mensa::createCursor() const {
    return new ibis::mensa::cursor(*this);
} // ibis::mensa::createCursor

/// Constructure a cursor object for row-wise data access to a @c
/// ibis::mensa object.
ibis::mensa::cursor::cursor(const ibis::mensa& t)
    : buffer(t.nColumns()), tab(t), curPart(0), pBegin(0), bBegin(0),
      bEnd(0), curRow(-1) {
    if (curPart >= t.parts.size()) return; // no data partition
    if (buffer.empty()) return;

    // linearize the t.naty to buffer, build a mapping between column names
    // and the position in the buffer
    uint32_t j = 0;
    long unsigned row_width = 0;
    for (ibis::table::namesTypes::const_iterator it = t.naty.begin();
	 it != t.naty.end(); ++ it, ++ j) {
	buffer[j].cname = (*it).first;
	buffer[j].ctype = (*it).second;
	buffer[j].cval = 0;
	bufmap[(*it).first] = j;
	switch ((*it).second) {
	case ibis::BYTE:
	case ibis::UBYTE:
	    ++ row_width; break;
	case ibis::SHORT:
	case ibis::USHORT:
	    row_width += 2; break;
	case ibis::INT:
	case ibis::UINT:
	case ibis::FLOAT:
	    row_width += 4; break;
	case ibis::LONG:
	case ibis::ULONG:
	case ibis::DOUBLE:
	    row_width += 8; break;
	default:
	    row_width += 16; break;
	}
    }
    if (row_width == 0) row_width = 1024 * t.naty.size();
    row_width = ibis::fileManager::bytesFree() / row_width;
    j = 0;
    while (row_width > 0) {
	row_width >>= 1;
	++ j;
    }
    -- j;
    if (j > 30) // maximum block size 1 billion
	preferred_block_size = 0x40000000;
    else if (j > 10) // read enough rows to take up half of the free memory
	preferred_block_size = (1 << j);
    else // minimum block size 1 thousand
	preferred_block_size = 1024;

    LOGGER(ibis::gVerbose > 2)
	<< "ibis::mensa::cursor constructed for table " << t.name()
	<< " with preferred block size " << preferred_block_size;
} // ibis::mensa::cursor::cursor

/// Fill the buffer for variable number @c i.  On success, return 0,
/// otherwise return a negative value.
/// @note The member variable @c cval in the buffer for a string valued
/// column is not the usual ibis::fileManager::storage object, instead
/// it is simply the pointer to the ibis::column object.  The string
/// variable is retrieved through the column object one at a time using the
/// function @c getString.
int ibis::mensa::cursor::fillBuffer(uint32_t i) const {
    if (curPart >= tab.parts.size() || tab.parts[curPart] == 0) return -1;
    const ibis::part& apart = *(tab.parts[curPart]);
    // has to do a look up based on the column name, becaus the ith column
    // of the data partition may not be the correct one (some columns may
    // be missing)
    const ibis::column* col = apart.getColumn(buffer[i].cname);
    if (col == 0)
	return -2;
    if (buffer[i].ctype == ibis::CATEGORY || buffer[i].ctype == ibis::TEXT
	|| buffer[i].ctype == ibis::BLOB) {
	buffer[i].cval = const_cast<void*>(static_cast<const void*>(col));
	return 0;
    }

    int ierr = 0;
    ibis::bitvector mask;
    if (bBegin > pBegin)
	mask.appendFill(0, static_cast<ibis::bitvector::word_t>(bBegin-pBegin));
    mask.adjustSize(static_cast<ibis::bitvector::word_t>(bEnd-pBegin),
		    static_cast<ibis::bitvector::word_t>(apart.nRows()));
    switch (buffer[i].ctype) {
    case ibis::BYTE: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<signed char>();
	ierr = col->selectValues
	    (mask, static_cast<array_t< signed char > *>(buffer[i].cval));
	break;}
    case ibis::UBYTE: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<unsigned char>();
	ierr = col->selectValues
	    (mask, static_cast<array_t< unsigned char > *>(buffer[i].cval));
	break;}
    case ibis::SHORT: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<int16_t>();
	ierr = col->selectValues
	    (mask, static_cast<array_t< int16_t > *>(buffer[i].cval));
	break;}
    case ibis::USHORT: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<uint16_t>();
	ierr = col->selectValues
	    (mask, static_cast<array_t< uint16_t >* >(buffer[i].cval));
	break;}
    case ibis::INT: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<int32_t>;
	ierr = col->selectValues
	    (mask, static_cast< array_t< int32_t >* >(buffer[i].cval));
	break;}
    case ibis::UINT: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<uint32_t>;
	ierr = col->selectValues
	    (mask, static_cast< array_t< uint32_t >* >(buffer[i].cval));
	break;}
    case ibis::LONG: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<int64_t>;
	ierr = col->selectValues
	    (mask, static_cast< array_t<int64_t>* >(buffer[i].cval));
	break;}
    case ibis::ULONG: {
     if (buffer[i].cval == 0)
	 buffer[i].cval = new array_t<uint64_t>;
     ierr = col->selectValues
	 (mask, static_cast< array_t< uint64_t >* >(buffer[i].cval));
     break;}
    case ibis::FLOAT: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<float>;
	ierr = col->selectValues
	    (mask, static_cast< array_t< float >* >(buffer[i].cval));
	break;}
    case ibis::DOUBLE: {
	if (buffer[i].cval == 0)
	    buffer[i].cval = new array_t<double>;
	ierr = col->selectValues
	    (mask, static_cast< array_t< double >* >(buffer[i].cval));
	break;}
    default:
	LOGGER(ibis::gVerbose > 0)
	    << "mensa::cursor::fillBuffer(" << i
	    << ") can not handle column " << col->name() << " type "
	    << ibis::TYPESTRING[buffer[i].ctype];
	ierr = -2;
	break;
    } // switch

    return ierr;
} // ibis::mensa::cursor::fillBuffer

/// Fill the buffers for every column.  If all column buffers are not
/// empty, then they are assumed to contain the expected content already.
/// Otherwise, it calls fillBuffer on each column.
int ibis::mensa::cursor::fillBuffers() const {
    uint32_t cnt = 0;
    for (uint32_t i = 0; i < buffer.size(); ++ i) {
	if (buffer[i].cval != 0) {
	    switch (buffer[i].ctype) {
	    case ibis::BYTE: {
		cnt += (static_cast<const array_t< signed char > *>
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::UBYTE: {
		cnt += (static_cast<const array_t<unsigned char> *>
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::SHORT: {
		cnt += (static_cast<const array_t< int16_t > *>
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::USHORT: {
		cnt += (static_cast<const array_t< uint16_t >* >
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::INT: {
		cnt += (static_cast<const array_t< int32_t >* >
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::UINT: {
		cnt += (static_cast<const array_t< uint32_t >* >
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::LONG: {
		cnt += (static_cast<const array_t<int64_t>* >
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::ULONG: {
		cnt += (static_cast<const array_t< uint64_t >* >
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::FLOAT: {
		cnt += (static_cast<const array_t< float >* >
			(buffer[i].cval)->empty() == false);
		break;}
	    case ibis::DOUBLE: {
		cnt += (static_cast<const array_t< double >* >
			(buffer[i].cval)->empty() == false);
		break;}
	    default:
		++ cnt;
		break;
	    } // switch
	}
    }
    if (cnt >= buffer.size()) return 1;

    std::string evt = "mensa[";
    evt += tab.name();
    evt += "]::cursor::fillBuffers";
    int ierr;
    ibis::util::timer mytimer(evt.c_str(), 4);
    for (uint32_t i = 0; i < buffer.size(); ++ i) {
	ierr = fillBuffer(i);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to fill buffer for column "
		<< i << "(" << buffer[i].cname << ", "
		<< ibis::TYPESTRING[buffer[i].ctype] << ") of partition "
		<< tab.parts[curPart]->name() << " with pBegin " << pBegin
		<< ", bBegin " << bBegin << ", and bEnd " << bEnd
		<< ", ierr = " << ierr;
	    return ierr;
	}
    }
    return 0;
} // ibis::mensa::cursor::fillBuffers

/// Mark all existing buffer as empty.
void ibis::mensa::cursor::clearBuffers() {
    for (uint32_t i = 0; i < buffer.size(); ++ i) {
	if (buffer[i].cval == 0) continue;

	switch (buffer[i].ctype) {
	default:
	    buffer[i].cval = 0;
	    break;
	case ibis::BYTE:
	    static_cast<array_t<char>*>(buffer[i].cval)->clear();
	    break;
	case ibis::UBYTE:
	    static_cast<array_t<unsigned char>*>(buffer[i].cval)->clear();
	    break;
	case ibis::SHORT:
	    static_cast<array_t<int16_t>*>(buffer[i].cval)->clear();
	    break;
	case ibis::USHORT:
	    static_cast<array_t<uint16_t>*>(buffer[i].cval)->clear();
	    break;
	case ibis::INT:
	    static_cast<array_t<int32_t>*>(buffer[i].cval)->clear();
	    break;
	case ibis::UINT:
	    static_cast<array_t<uint32_t>*>(buffer[i].cval)->clear();
	    break;
	case ibis::LONG:
	    static_cast<array_t<int64_t>*>(buffer[i].cval)->clear();
	    break;
	case ibis::ULONG:
	    static_cast<array_t<uint64_t>*>(buffer[i].cval)->clear();
	    break;
	case ibis::FLOAT:
	    static_cast<array_t<float>*>(buffer[i].cval)->clear();
	    break;
	case ibis::DOUBLE:
	    static_cast<array_t<double>*>(buffer[i].cval)->clear();
	    break;
	}
    }
} // ibis::mensa::cursor::clearBuffers

/// Points the the next row.
int ibis::mensa::cursor::fetch() {
    if (curPart >= tab.parts.size()) return -1;

    ++ curRow;
    if (static_cast<uint64_t>(curRow) >= bEnd) { // reach end of the block
	clearBuffers();
	if (bEnd >= pBegin + tab.parts[curPart]->nRows()) {
	    pBegin += tab.parts[curPart]->nRows();
	    ++ curPart;
	    if (curPart >= tab.parts.size()) return -1;

	    bBegin = pBegin;
	    bEnd = pBegin +
		(preferred_block_size <= tab.parts[curPart]->nRows() ?
		 preferred_block_size : tab.parts[curPart]->nRows());
	}
	else {
	    bBegin = bEnd;
	    bEnd += preferred_block_size;
	    const uint64_t pEnd = pBegin + tab.parts[curPart]->nRows();
	    if (bEnd > pEnd)
		bEnd = pEnd;
	}
	return fillBuffers();
    }
    return 0;
} // ibis::mensa::cursor::fetch

/// Pointers to the specified row.
int ibis::mensa::cursor::fetch(uint64_t irow) {
    if (curPart >= tab.parts.size()) return -1;
    if (bEnd <= irow)
	clearBuffers();

    while (pBegin + tab.parts[curPart]->nRows() <= irow &&
	   curPart < tab.parts.size()) {
	pBegin += tab.parts[curPart]->nRows();
	++ curPart;
    }
    if (curPart < tab.parts.size()) {
	curRow = irow;
	bBegin = irow;
	bEnd = irow + preferred_block_size;
	if (bEnd > pBegin + tab.parts[curPart]->nRows())
	    bEnd = pBegin + tab.parts[curPart]->nRows();
	return fillBuffers();
    }
    else {
	curRow = pBegin;
	return -1;
    }
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(ibis::table::row& res) {
    int ierr = fetch();
    if (ierr < 0) return ierr;

    fillRow(res);
    return 0;
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(uint64_t irow, ibis::table::row& res) {
    int ierr = fetch(irow);
    if (ierr < 0) return ierr;

    fillRow(res);
    return 0;
} // ibis::mensa::cursor::fetch

void ibis::mensa::cursor::fillRow(ibis::table::row& res) const {
    res.clear();
    const uint32_t il = static_cast<uint32_t>(curRow - bBegin);
    for (uint32_t j = 0; j < buffer.size(); ++ j) {
	switch (buffer[j].ctype) {
	case ibis::BYTE: {
	    res.bytesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.bytesvalues.push_back
		    ((*static_cast<const array_t<char>*>(buffer[j].cval))[il]);
	    }
	    else {
		res.bytesvalues.push_back(0x7F);
	    }
	    break;}
	case ibis::UBYTE: {
	    res.ubytesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.ubytesvalues.push_back
		    ((*static_cast<const array_t<unsigned char>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.ubytesvalues.push_back(0xFFU);
	    }
	    break;}
	case ibis::SHORT: {
	    res.shortsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.shortsvalues.push_back
		    ((*static_cast<const array_t<int16_t>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.shortsvalues.push_back(0x7FFF);
	    }
	    break;}
	case ibis::USHORT: {
	    res.ushortsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.ushortsvalues.push_back
		    ((*static_cast<const array_t<uint16_t>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.ushortsvalues.push_back(0xFFFFU);
	    }
	    break;}
	case ibis::INT: {
	    res.intsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.intsvalues.push_back
		    ((*static_cast<const array_t<int32_t>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.intsvalues.push_back(0x7FFFFFFF);
	    }
	    break;}
	case ibis::UINT: {
	    res.uintsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.uintsvalues.push_back
		    ((*static_cast<const array_t<uint32_t>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.uintsvalues.push_back(0xFFFFFFFFU);
	    }
	    break;}
	case ibis::LONG: {
	    res.longsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.longsvalues.push_back
		    ((*static_cast<const array_t<int64_t>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.longsvalues.push_back(0x7FFFFFFFFFFFFFFFLL);
	    }
	    break;}
	case ibis::ULONG: {
	    res.ulongsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.ulongsvalues.push_back
		    ((*static_cast<const array_t<uint64_t>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.ulongsvalues.push_back(0xFFFFFFFFFFFFFFFFULL);
	    }
	    break;}
	case ibis::FLOAT: {
	    res.floatsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.floatsvalues.push_back
		    ((*static_cast<const array_t<float>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.floatsvalues.push_back
		    (std::numeric_limits<float>::quiet_NaN());
	    }
	    break;}
	case ibis::DOUBLE: {
	    res.doublesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.doublesvalues.push_back
		    ((*static_cast<const array_t<double>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.doublesvalues.push_back
		    (std::numeric_limits<double>::quiet_NaN());
	    }
	    break;}
	case ibis::BLOB: {
	    std::string val;
	    res.blobsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		unsigned char *buf = 0;
		uint32_t sz = 0;
		int ierr = reinterpret_cast<const ibis::blob*>(buffer[j].cval)
		    ->getBlob(curRow-pBegin, buf, sz);
		if (ierr >= 0 && sz > 0 && buf != 0)
		    val.insert(val.end(), buf, buf+sz);
		delete buf;
	    }
	    res.blobsvalues.push_back(val);
	    break;}
	case ibis::TEXT: {
	    res.textsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		std::string tmp;
		reinterpret_cast<const ibis::text*>(buffer[j].cval)
		    ->getString(curRow-pBegin, tmp);
		res.textsvalues.push_back(tmp);
	    }
	    else {
		res.textsvalues.push_back("");
	    }
	    break;}
	case ibis::CATEGORY: {
	    res.catsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		std::string tmp;
		reinterpret_cast<const ibis::text*>(buffer[j].cval)
		    ->getString(curRow - pBegin, tmp);
		res.catsvalues.push_back(tmp);
	    }
	    else {
		res.catsvalues.push_back("");
	    }
	    break;}
	default: {
	    if (ibis::gVerbose > 1)
		ibis::util::logMessage
		    ("Warning", "ibis::mensa::cursor::fillRow is not expected "
		     "to encounter data type %s (column name %s)",
		     ibis::TYPESTRING[(int)buffer[j].ctype], buffer[j].cname);
	    break;}
	}
    }
} // ibis::mensa::cursor::fillRow

/**
   Print the current row.  Assumes the caller has performed the fetch
   operation.

   Return values:
   -   0  -- normal (successful) completion.
   -  -1  -- cursor objection not initialized, call fetch first.
   -  -2  -- unable to load data into memory.
   -  -4  -- error in the output stream.
 */
int ibis::mensa::cursor::dump(std::ostream& out, const char* del) const {
    if (tab.nColumns() == 0) return 0;
    if (curRow < 0 || curPart >= tab.parts.size())
	return -1;
    int ierr;
//     if (static_cast<uint64_t>(curRow) == bBegin) {
// 	// first time accessing the data partition
// 	ierr = fillBuffers();
// 	if (ierr < 0) {
// 	    LOGGER(ibis::gVerbose > 1)
// 		<< "ibis::mensa[" << tab.name() << "]::cursor::dump "
// 		"call to fillBuffers() failed with ierr = " << ierr
// 		<< " at partition " << tab.parts[curPart]->name()
// 		<< ", pBegin " << pBegin << ", bBegin " << bBegin
// 		<< ", bEnd " << bEnd;
// 	    return -2;
// 	}
//     }

    const uint32_t i = static_cast<uint32_t>(curRow - bBegin);
    ierr = dumpIJ(out, i, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (uint32_t j = 1; j < tab.nColumns(); ++ j) {
	out << del;
	ierr = dumpIJ(out, i, j);
	if (ierr < 0)
	    return ierr;
    }
    out << "\n";
    if (! out)
	ierr = -4;
    return ierr;
} // ibis::mensa::cursor::dump

/// Print out the content of the current block.  Also move the pointer to
/// the last row of the block.
int ibis::mensa::cursor::dumpBlock(std::ostream& out, const char* del) {
    if (tab.nColumns() == 0) return 0;
    if (curPart >= tab.parts.size()) return 0;
    if (curRow < 0) // not initialized
	return -1;
    int ierr;
    if (static_cast<uint64_t>(curRow) == bBegin) {
	// first time accessing the data partition
	ierr = fillBuffers();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "ibis::mensa[" << tab.name() << "]::cursor::dumpBlock "
		"call to fillBuffers() failed with ierr = " << ierr
		<< " at partition " << tab.parts[curPart]->name()
		<< ", pBegin " << pBegin << ", bBegin " << bBegin
		<< ", bEnd " << bEnd;
	    return -2;
	}
    }

    uint32_t i = static_cast<uint32_t>(curRow - bBegin);
    // print the first row with error checking
    ierr = dumpIJ(out, i, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (uint32_t j = 1; j < tab.nColumns(); ++ j) {
	out << del;
	ierr = dumpIJ(out, i, j);
	if (ierr < 0)
	    return -4;
    }
    out << "\n";
    if (! out)
	ierr = -4;
    // print the rest of the rows without error checking
    const uint32_t nelem = static_cast<uint32_t>(bEnd - bBegin);
    for (++ i; i < nelem; ++ i) {
	(void) dumpIJ(out, i, 0U);
	for (uint32_t j = 1; j < buffer.size(); ++ j) {
	    out << del;
	    (void) dumpIJ(out, i, j);
	}
	out << "\n";
    }

    // move the position of the cursor to the last row of the block
    curRow = bEnd-1;
    return (out ? 0 : -4);
} // ibis::mensa::cursor::dumpBlock

/// Print the first nr rows of the table to the specified output stream.
int ibis::mensa::cursor::dumpSome(std::ostream &out, uint64_t nr,
				  const char *del) {
    int ierr = 0;
    if (nr > tab.nRows()) nr = tab.nRows();
    if (curRow < 0) {
	ierr = fetch();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "ibis::mensa[" << tab.name() << "]::cursor::dumpSome "
		"call to fetch(of the block) failed with ierr = " << ierr
		<< " at partition " << tab.parts[curPart]->name()
		<< ", pBegin " << pBegin << ", bBegin " << bBegin
		<< ", bEnd " << bEnd;
	    return -1;
	}
    }

    while (static_cast<uint64_t>(curRow) < nr) {
	if (bEnd <= nr) {
	    ierr = dumpBlock(out, del);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "ibis::mensa[" << tab.name() << "]::cursor::dumpSome "
		    "call to fillBlock() failed with ierr = " << ierr
		    << " at partition " << tab.parts[curPart]->name()
		    << ", pBegin " << pBegin << ", bBegin " << bBegin
		    << ", bEnd " << bEnd;
		return -3;
	    }
	    (void) fetch();
	}
	else {
	    while (static_cast<uint64_t>(curRow) < nr) {
		ierr = dump(out, del);
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 1)
			<< "ibis::mensa[" << tab.name()
			<< "]::cursor::dumpSome call to dump() "
			"failed with ierr = " << ierr
			<< " at partition " << tab.parts[curPart]->name()
			<< ", pBegin " << pBegin << ", bBegin " << bBegin
			<< ", bEnd " << bEnd;
		    return -4;
		}
		ierr = fetch();
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 1)
			<< "ibis::mensa[" << tab.name()
			<< "]::cursor::dumpSome call to fetch(row " << curRow
			<< ") failed with ierr = " << ierr
			<< " at partition " << tab.parts[curPart]->name()
			<< ", pBegin " << pBegin << ", bBegin " << bBegin
			<< ", bEnd " << bEnd;
		    return -5;
		}
	    }
	}
    }
    if (static_cast<uint64_t>(curRow) < tab.nRows())
	out << "\t... " << tab.nRows() - curRow << " remaining in table "
	    << tab.name();
    return ierr;
} // ibis::mensa::cursor::dumpSome

/// Print the ith element in the current block for column j.
/// @note This function does not perform array bounds check.
int ibis::mensa::cursor::dumpIJ(std::ostream& out, uint32_t i,
				uint32_t j) const {
    int ierr = 0;
    switch (buffer[j].ctype) {
    default: {
	ierr = -2;
	break;}
    case ibis::BYTE: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<signed char> *tmp 
	    = static_cast<const array_t<signed char>*>(buffer[j].cval);
	out << (int) ((*tmp)[i]);
	break;}
    case ibis::UBYTE: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<unsigned char> *tmp 
	    = static_cast<const array_t<unsigned char>*>(buffer[j].cval);
	out << (unsigned int) ((*tmp)[i]);
	break;}
    case ibis::SHORT: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<int16_t> *tmp 
	    = static_cast<const array_t<int16_t>*>(buffer[j].cval);
	out << (*tmp)[i];
	break;}
    case ibis::USHORT: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<uint16_t> *tmp 
	    = static_cast<const array_t<uint16_t>*>(buffer[j].cval);
	out << (*tmp)[i];
	break;}
    case ibis::INT: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<int32_t> *tmp 
	    = static_cast<const array_t<int32_t>*>(buffer[j].cval);
	out << (*tmp)[i];
	break;}
    case ibis::UINT: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<uint32_t> *tmp 
	    = static_cast<const array_t<uint32_t>*>(buffer[j].cval);
	out << (*tmp)[i];
	break;}
    case ibis::LONG: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<int64_t> *tmp 
	    = static_cast<const array_t<int64_t>*>(buffer[j].cval);
	out << (*tmp)[i];
	break;}
    case ibis::ULONG: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<uint64_t> *tmp 
	    = static_cast<const array_t<uint64_t>*>(buffer[j].cval);
	out << (*tmp)[i];
	break;}
    case ibis::FLOAT: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<float> *tmp 
	    = static_cast<const array_t<float>*>(buffer[j].cval);
	out << std::setprecision(8) << (*tmp)[i];
	break;}
    case ibis::DOUBLE: {
	if (buffer[j].cval == 0) return -1; // null value
	const array_t<double> *tmp 
	    = static_cast<const array_t<double>*>(buffer[j].cval);
	out << std::setprecision(18) << (*tmp)[i];
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	if (curPart < tab.parts.size()) {
	    const ibis::column* col = 0;
	    if (buffer[j].cval != 0) {
		col = reinterpret_cast<const ibis::column*>(buffer[j].cval);
	    }
	    else {
		col = tab.parts[curPart]->getColumn(buffer[j].cname);
		buffer[j].cval =
		    const_cast<void*>(static_cast<const void*>(col));
	    }
	    if (col != 0) {
		std::string val;
		static_cast<const ibis::text*>(col)
		    ->ibis::text::getString
		    (static_cast<uint32_t>(i+bBegin-pBegin), val);
#if DEBUG+0 > 1 || _DEBUG+0 > 1
		LOGGER(ibis::gVerbose > 5)
		    << "DEBUG -- mensa::cursor::dump(" << i << ", " << j
		    << ") printing string " << val << " to position "
		    << static_cast<off_t>(out.tellp()) << " of output stream "
		    << static_cast<void*>(&out);
#endif
		out << '"' << val << '"';
	    }
	    else {
		ierr = -3;
	    }
	}
	break;}
    case ibis::BLOB: {
	if (curPart < tab.parts.size()) {
	    const ibis::blob *blo;
	    if (buffer[j].cval != 0) {
		blo = reinterpret_cast<const ibis::blob*>(buffer[j].cval);
	    }
	    else {
		blo = dynamic_cast<const ibis::blob*>
		    (tab.parts[curPart]->getColumn(buffer[j].cname));
		buffer[j].cval =
		    const_cast<void*>(static_cast<const void*>(blo));
		LOGGER(blo == 0 && ibis::gVerbose > 0)
		    << "mensa::cursor::dumpIJ(" << i << ", " << j
		    << ") failed to find a column with name "
		    << buffer[j].cname << " with type blob";
	    }

	    if (blo != 0) {
		unsigned char *buf = 0;
		uint32_t sz = 0;
		ierr = blo->getBlob(static_cast<uint32_t>(i+bBegin-pBegin),
				    buf, sz);
		if (ierr >= 0 && sz > 0 && buf != 0) {
		    out << "0x" << std::hex;
		    for (uint32_t k = 0; k < sz; ++ k)
			out << std::setprecision(2) << (uint16_t)(buf[k]);
		    out << std::dec;
		}
	    }
	    else {
		ierr = -5;
	    }
	}
	break;}
    }
    if (ierr >= 0 && ! out) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- mensa::cursor::dumpIJ(" << i << ", " << j
	    << ") failed to write to the output stream at offset "
	    << static_cast<off_t>(out.tellp());
	ierr = -4;
    }
    return ierr;
} // ibis::mensa::cursor::dumpIJ

int ibis::mensa::cursor::getColumnAsByte(uint32_t j, char& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j); // first time accessing the data partition
    if (ierr < 0 || buffer[j].cval == 0) // missing column in this partition
	return -2;

    switch (buffer[j].ctype) {
    case ibis::UBYTE:
    case ibis::BYTE: {
	val = (*(static_cast<array_t<signed char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsByte

int ibis::mensa::cursor::getColumnAsUByte(uint32_t j,
					  unsigned char& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j); // first time accessing the data partition
    if (ierr < 0 || buffer[j].cval == 0) // missing column in this partition
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUByte

int ibis::mensa::cursor::getColumnAsShort(uint32_t j, int16_t& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (*(static_cast<array_t<signed char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (*(static_cast<array_t<int16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsShort

int ibis::mensa::cursor::getColumnAsUShort(uint32_t j, uint16_t& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUShort

int ibis::mensa::cursor::getColumnAsInt(uint32_t j, int32_t& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (*(static_cast<array_t<signed char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (*(static_cast<array_t<int16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	val = (*(static_cast<array_t<int32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsInt

int ibis::mensa::cursor::getColumnAsUInt(uint32_t j, uint32_t& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	val = (*(static_cast<array_t<uint32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUInt

int ibis::mensa::cursor::getColumnAsLong(uint32_t j, int64_t& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (*(static_cast<array_t<signed char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (*(static_cast<array_t<int16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::INT: {
	val = (*(static_cast<array_t<int32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UINT: {
	val = (*(static_cast<array_t<uint32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	val = (*(static_cast<array_t<int64_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsLong

int ibis::mensa::cursor::getColumnAsULong(uint32_t j, uint64_t& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	val = (*(static_cast<array_t<uint32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	val = (*(static_cast<array_t<uint64_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsULong

int ibis::mensa::cursor::getColumnAsFloat(uint32_t j, float& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (*(static_cast<array_t<signed char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (*(static_cast<array_t<int16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	val = (*(static_cast<array_t<float>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsFloat

int ibis::mensa::cursor::getColumnAsDouble(uint32_t j, double& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (*(static_cast<array_t<signed char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (*(static_cast<array_t<int16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::INT: {
	val = (*(static_cast<array_t<int32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::UINT: {
	val = (*(static_cast<array_t<uint32_t>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	val = (*(static_cast<array_t<double>*>(buffer[j].cval)))
	    [curRow - bBegin];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsDouble

int ibis::mensa::cursor::getColumnAsString(uint32_t j, std::string& val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    const uint32_t irow = static_cast<uint32_t>(curRow-bBegin);
    std::ostringstream oss;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	oss << static_cast<int>
	    ((*(static_cast<array_t<char>*>(buffer[j].cval)))[irow]);
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	oss << static_cast<unsigned>
	    ((*(static_cast<array_t<unsigned char>*>(buffer[j].cval)))[irow]);
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::SHORT: {
	oss << (*(static_cast<array_t<int16_t>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::USHORT: {
	oss << (*(static_cast<array_t<uint16_t>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::INT: {
	oss << (*(static_cast<array_t<int32_t>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::UINT: {
	oss << (*(static_cast<array_t<uint32_t>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::LONG: {
	oss << (*(static_cast<array_t<int64_t>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::ULONG: {
	oss << (*(static_cast<array_t<uint64_t>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	oss << (*(static_cast<array_t<float>*>(buffer[j].cval)))[irow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	oss << (*(static_cast<array_t<double>*>(buffer[j].cval)))[irow];
	ierr = 0;
	val = oss.str();
	break;}
    case ibis::CATEGORY:
    case ibis::TEXT: {
	const ibis::column* col =
	    tab.parts[curPart]->getColumn(buffer[j].cname);
	if (col != 0) {
	    static_cast<const ibis::text*>(col)->
		getString(static_cast<uint32_t>(curRow-pBegin), val);
	    ierr = 0;
	}
	else {
	    ierr = -1;
	}
	break;}
    case ibis::BLOB: {
	unsigned char *buf = 0;
	uint32_t sz = 0;
	const ibis::blob* blo = dynamic_cast<const ibis::blob*>
	    (tab.parts[curPart]->getColumn(buffer[j].cname));
	if (blo != 0) {
	    ierr = blo->getBlob(static_cast<uint32_t>(curRow-pBegin), buf, sz);
	    if (ierr >= 0) {
		if (sz > 0) {
		    oss << "0x" << std::hex;
		    for (uint32_t i = 0; i < sz; ++ i)
			oss << std::setprecision(2) << buf[i];
		    oss << std::dec;
		    val = oss.str();
		}
	    }
	}
	else {
	    ierr = -3;
	}
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsString

/// Destructor for bufferElement.
ibis::mensa::cursor::bufferElement::~bufferElement() {
    switch (ctype) {
    default:
	break;
    case ibis::BLOB:
	delete static_cast<array_t<unsigned char>*>(cval);
	break;
    case ibis::BYTE:
	delete static_cast<array_t<char>*>(cval);
	break;
    case ibis::UBYTE:
	delete static_cast<array_t<unsigned char>*>(cval);
	break;
    case ibis::SHORT:
	delete static_cast<array_t<int16_t>*>(cval);
	break;
    case ibis::USHORT:
	delete static_cast<array_t<uint16_t>*>(cval);
	break;
    case ibis::INT:
	delete static_cast<array_t<int32_t>*>(cval);
	break;
    case ibis::UINT:
	delete static_cast<array_t<uint32_t>*>(cval);
	break;
    case ibis::LONG:
	delete static_cast<array_t<int64_t>*>(cval);
	break;
    case ibis::ULONG:
	delete static_cast<array_t<uint64_t>*>(cval);
	break;
    case ibis::FLOAT:
	delete static_cast<array_t<float>*>(cval);
	break;
    case ibis::DOUBLE:
	delete static_cast<array_t<double>*>(cval);
	break;
    }
} // ibis::mensa::cursor::bufferElement::~bufferElement

/// Ibis::liga does not own the data partitions and does not free the
/// resources in those partitions.
ibis::liga::~liga () {
    parts.clear();
} // ibis::liga::~liga

/// Create an object from an externally managed data partition.
ibis::liga::liga(ibis::part& p) : ibis::mensa() {
    if (p.nRows() == 0 || p.nColumns() == 0) return;
    parts.resize(1);
    parts[0] = &p;
    p.combineNames(naty);
    nrows = p.nRows();

    name_ = "T-";
    desc_ = "a simple container of data partition ";
    std::ostringstream oss;
    oss << "with " << p.nRows() << " row" << (p.nRows()>1 ? "s" : "")
	<< " and " << p.nColumns() << " column"
	<< (p.nColumns()>1 ? "s" : "");
    if (p.name() != 0 && *(p.name()) != 0) {
	name_ += p.name();
	desc_ += p.name();
    }
    else if (p.description() != 0 && *(p.description()) != 0) {
	unsigned sum =
	    ibis::util::checksum(p.description(), strlen(p.description()));
	std::string tmp;
	ibis::util::int2string(tmp, sum);
	name_ += tmp;
	desc_ += p.description();
    }
    else { // produce a random name from the size of the data partition
	const unsigned v2 = (p.nColumns() ^ ibis::fileManager::iBeat());
	std::string tmp;
	ibis::util::int2string(tmp, p.nRows(), v2);
	desc_ += oss.str();
    }
    LOGGER(ibis::gVerbose > 0)
	<< "ibis::liga -- constructed table " << name_ << " (" << desc_
	<< ") from a partition " << oss.str();
} // ibis::liga::liga

/// Create an object from an external list of data partitions.  Note that
/// this object does not own the partitions and is not reponsible for
/// freeing the partitions.  It merely provide a container for the
/// partitions so that one can use the ibis::table API.
ibis::liga::liga(const ibis::partList &l) : ibis::mensa() {
    if (l.empty()) return;

    parts.insert(parts.end(), l.begin(), l.end());
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it)->combineNames(naty);
	nrows += (*it)->nRows();
    }
    if (! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = "T-";
	name_ += (*it)->name();
	if (desc_.empty()) {
	    uint32_t mp = ((l.size() >> ibis::gVerbose) <= 1 ?
			 l.size() :
			 (ibis::gVerbose > 2 ? (1 << ibis::gVerbose) : 5));
	    if (mp > l.size()) mp = l.size();
	    uint32_t jp = 1;
	    desc_ = "a simple list of partition";
	    if (l.size() > 1) desc_ += "s";
	    desc_ += ": ";
	    desc_ += parts[0]->name();
	    while (jp < mp) {
		desc_ += (jp+1 < parts.size() ? ", " : " and ");
		desc_ += parts[jp]->name();
	    }
	    if (jp < parts.size()) {
		std::ostringstream oss;
		oss << ", ... (" << parts.size()-jp << " skipped)";
		desc_ += oss.str();
	    }
	}
    }
    if (ibis::gVerbose > 0 && ! name_.empty()) {
	ibis::util::logger lg;
	lg.buffer() << "ibis::liga -- constructed table "
		    << name_ << " (" << desc_ << ") from a list of "
		    << l.size() << " data partition"
		    << (l.size()>1 ? "s" : "")
		    << ", with " << naty.size() << " column"
		    << (naty.size()>1 ? "s" : "") << " and "
		    << nrows << " row" << (nrows>1 ? "s" : "");
    }
} // ibis::liga::liga

ibis::table* ibis::table::create(ibis::part& p) {
    return new ibis::liga(p);
} // ibis::table::create

ibis::table* ibis::table::create(const ibis::partList& pl) {
    return new ibis::liga(pl);
} // ibis::table::create

/// If the incoming directory name is nil or an empty string, it attempts
/// to use the directories specified in the configuration files.
ibis::table* ibis::table::create(const char* dir) {
    return new ibis::mensa(dir);
} // ibis::table::create

ibis::table* ibis::table::create(const char* dir1, const char* dir2) {
    if (dir1 == 0 || *dir1 == 0)
	return 0;
    else if (dir2 == 0 || *dir2 == 0)
	return new ibis::mensa(dir1);
    else
	return new ibis::mensa(dir1, dir2);
} // ibis::table::create

void ibis::table::parseNames(char* in, ibis::table::stringList& out) {
    char* ptr1 = in;
    char* ptr2;
    while (*ptr1 != 0 && isspace(*ptr1) != 0) ++ ptr1; // leading space
    // since SQL names can not contain space, quotes must be for the whole
    // list of names
    if (*ptr1 == '\'') {
	++ ptr1; // skip opening quote
	ptr2 = strchr(ptr1, '\'');
	if (ptr2 > ptr1)
	    *ptr2 = 0; // string terminates here
    }
    else if (*ptr1 == '"') {
	++ ptr1;
	ptr2 = strchr(ptr1, '"');
	if (ptr2 > ptr1)
	    *ptr2 = 0;
    }

    while (*ptr1 != 0) {
	for (ptr2 = ptr1; *ptr2 == '_' || isalnum(*ptr2) != 0; ++ ptr2);
	while (*ptr2 == '(') {
	    int nesting = 1;
	    for (++ ptr2; *ptr2 != 0 && nesting > 0; ++ ptr2) {
		nesting -= (*ptr2 == ')');
		nesting += (*ptr2 == '(');
	    }
	    while (*ptr2 != 0 && *ptr2 != ',' && *ptr2 != ';' && *ptr2 != '(')
		++ ptr2;
	}
	if (*ptr2 == 0) {
	    out.push_back(ptr1);
	}
	else if (ispunct(*ptr2) || isspace(*ptr2)) {
	    *ptr2 = 0;
	    out.push_back(ptr1);
	    ++ ptr2;
	}
	else {
	    if (ibis::gVerbose > 0)
		ibis::util::logMessage
		    ("ibis::table::parseNames", "can not part string \"%s\" "
		     "into a column name or a function, skip till first "
		     "character after the next comma or space", ptr1);

	    while (*ptr2 != 0 && ispunct(*ptr2) == 0 && isspace(*ptr2) == 0)
		++ ptr2;
	    if (*ptr2 != 0) ++ ptr2;
	    while (*ptr2 != 0 && isspace(*ptr2) != 0) ++ ptr2;
	}
	// skip spaces and punctuations
	for (ptr1 = ptr2; *ptr1 && (ispunct(*ptr1) || isspace(*ptr1)); ++ ptr1);
    }
} // ibis::table::parseNames

ibis::table* ibis::table::groupby(const char* str) const {
    stringList lst;
    char* buf = 0;
    if (str != 0 && *str != 0) {
	buf = new char[strlen(str)+1];
	strcpy(buf, str);
	parseNames(buf, lst);
    }
    ibis::table* res = groupby(lst);
    delete [] buf;
    return res;
} // ibis::table::groupby

void ibis::table::orderby(const char* str) {
    stringList lst;
    char* buf = 0;
    if (str != 0 && *str != 0) {
	buf = new char[strlen(str)+1];
	strcpy(buf, str);
	parseNames(buf, lst);
    }
    orderby(lst);
    delete [] buf;
} // ibis::table::orderby

/// This implementation of the member function uses the class function
/// ibis::table::select that takes the similar arguments along with the
/// full list of data partitions to work with.  This function returns a nil
/// table when the select clause is empty or nil.
ibis::table*
ibis::table::select(const char* sel, const ibis::qExpr* cond) const {
    if (sel == 0 || *sel == 0 || cond == 0 || nRows() == 0 || nColumns() == 0)
	return 0;

    std::vector<const ibis::part*> parts;
    int ierr = getPartitions(parts);
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- ibis::table::select failed to getPartitions, ierr="
	    << ierr;
	return 0;
    }

    try {
	return ibis::table::select(parts, sel, cond);
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- ibis::table::select absorbed an exception, "
	    "will return a nil pointer";
	return 0;
    }
} // ibis::table::select

/// Upon successful completion of this function, it produces an in-memory
/// data partition holding the selected data records.  It will fail in a
/// unpredictable way if the selected records can not fit in the available
/// memory.
///
/// It expects the arguments sel and cond to be valid and non-trivial.  It
/// will return a nil pointer if those arguments are nil pointers or empty
/// strings or blank spaces.
ibis::table* ibis::table::select(const std::vector<const ibis::part*>& mylist,
				 const char *sel, const char *cond) {
    if (sel == 0 || cond == 0 || *sel == 0 || *cond == 0 || mylist.empty())
	return 0;

    try {
	ibis::whereClause wc(cond);
	if (wc.empty()) return 0; // failed to parse the where clause
	return ibis::table::select(mylist, sel, wc.getExpr());
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- ibis::table::select absorbed an exception, "
	    "will return a nil pointer";
	return 0;
    }
} // ibis::table::select

/// Upon successful completion of this function, it produce an in-memory
/// data partition holding the selected data records.  It will fail in an
/// unpredictable way if the selected records can not fit in available
/// memory.
///
/// It expects the arguments sel and cond to be valid and non-trivial.  It
/// will return a nil pointer if those arguments are nil pointers or empty
/// strings or blank spaces.
ibis::table* ibis::table::select(const std::vector<const ibis::part*>& mylist,
				 const char *sel, const ibis::qExpr *cond) {
    if (sel == 0 || cond == 0 || *sel == 0 || mylist.empty())
	return 0;

    std::string mesg = "table::select";
    if (ibis::gVerbose > 0) {
	std::ostringstream oss;
	oss << "(" << mylist.size() << " data partition"
	    << (mylist.size() > 1 ? "s" : "") << ", " << sel
	    << ", qExpr @" << static_cast<const void*>(cond) << ')';
	mesg += oss.str();
    }

    ibis::util::timer atimer(mesg.c_str(), 2);
    ibis::selectClause tms;
    long int ierr;
    try {
	ierr = tms.parse(sel);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- " << mesg << " failed to parse \""
		<< sel << "\" (ierr=" << ierr
		<< "), return a nil pointer to indicate error";
	    return 0;
	}
	    
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << mesg << " failed to parse \""
	    << sel << "\", return a nil pointer to indicate error";
	return 0;
    }
    if (tms.size() == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " translated select clause \"" << sel
	    << "\" into an empty internal expression list";

	int64_t nhits = ibis::table::computeHits(mylist, cond);
	if (nhits < 0) {
	    return 0;
	}
	else {
	    std::ostringstream oss;
	    oss << "select count(*) from ";
	    for (std::vector<const ibis::part*>::const_iterator it =
		     mylist.begin();
		 it != mylist.end(); ++ it) {
		if (it != mylist.begin())
		    oss << ", ";
		oss << (*it)->name();
	    }
	    oss << " where ";
	    cond->printFull(oss);
	    std::string nm = ibis::util::shortName(oss.str());
	    return new ibis::tabula(nm.c_str(), oss.str().c_str(), nhits);
	}
    }

    // a single query object is used for different data partitions
    ibis::countQuery qq;
    ierr = qq.setWhereClause(cond);
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " failed to assign externally "
	    "provided query expression \"" << *cond
	    << "\" to a ibis::countQuery object, ierr=" << ierr;
	return 0;
    }

    // produce a list of names, types, and buffers to hold the initial selection
    std::vector<std::string> nls;
    ibis::table::typeList    tls;
    ibis::bord::bufferList   buff;
    std::vector<uint32_t>    tmstouse;
    uint32_t                 nplain = 0;
    if (tms.size() > 0) { // sort the names of variables to compute
	std::map<const char*, uint32_t> uniquenames;
	for (uint32_t i = 0; i < tms.size(); ++ i) {
	    nplain += (tms.getAggregator(i) == ibis::selectClause::NIL);

	    const char* tname = tms.argName(i);
	    if (tms.getAggregator(i) == ibis::selectClause::CNT &&
		*tname == '*') // skip count(*)
		continue;

	    if (uniquenames.find(tname) == uniquenames.end())
		uniquenames[tname] = i;
	}
	if (uniquenames.size() >= tms.size()) {
	    nls.resize(tms.size());
	    tls.resize(tms.size());
	    buff.resize(tms.size());
	    tmstouse.resize(tms.size());
	    for (uint32_t i = 0; i < tms.size(); ++ i) {
		tls[i] = ibis::UNKNOWN_TYPE;
		nls[i] = tms.argName(i);
		buff[i] = 0;
		tmstouse[i] = i;
	    }
	}
	else if (uniquenames.size() > 1) {
	    const uint32_t nnames = uniquenames.size();
	    ibis::array_t<uint32_t> pos;
	    pos.reserve(nnames);
	    for (std::map<const char*, uint32_t>::const_iterator it =
		     uniquenames.begin(); it != uniquenames.end(); ++ it)
		pos.push_back(it->second);
	    std::sort(pos.begin(), pos.end());
	    for (std::map<const char*, uint32_t>::iterator it =
		     uniquenames.begin(); it != uniquenames.end(); ++ it)
		it->second = pos.find(it->second);
	    nls.resize(nnames);
	    tls.resize(nnames);
	    buff.resize(nnames);
	    tmstouse.resize(nnames);
	    for (uint32_t i = 0; i < nnames; ++i) {
		nls[i] = tms.argName(pos[i]);
		tls[i] = ibis::UNKNOWN_TYPE;
		buff[i] = 0;
		tmstouse[i] = pos[i];
	    }
	}
	else { // only one unique name
	    nls.resize(1);
	    tls.resize(1);
	    buff.resize(1);
	    tmstouse.resize(1);
	    nls[0] = tms.argName(0);
	    tls[0] = ibis::UNKNOWN_TYPE;
	    buff[0] = 0;
	    tmstouse[0] = 0;
	}
    }

    uint32_t nh = 0;
    // main loop through each data partition, fill the initial selection
    for (std::vector<const ibis::part*>::const_iterator it = mylist.begin();
	 it != mylist.end(); ++ it) {
	if (tmstouse.size() >= tms.size())
	    ierr = tms.verify(**it);
	else
	    ierr = tms.verifySome(**it, tmstouse);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- select clause (" << sel
		<< ") contains variables that are not in data partition "
		<< (*it)->name();
	    continue;
	}
	ierr = qq.setSelectClause(&tms);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- failed to modify the select clause of "
		<< "the countQuery object (" << qq.getWhereClause()
		<< ") on data partition " << (*it)->name();
	    continue;
	}

	ierr = qq.setPartition(*it);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- query.setPartition(" << (*it)->name()
		<< ") failed with error code " << ierr;
	    continue;
	}

	ierr = qq.evaluate();
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- failed to process query on data partition "
		<< (*it)->name();
	    continue;
	}

	const ibis::bitvector* hits = qq.getHitVector();
	if (hits == 0 || hits->cnt() == 0) continue;

	const uint32_t nqq = hits->cnt();
	for (uint32_t i = 0; i < tmstouse.size(); ++ i) {
	    const uint32_t itm = tmstouse[i];
	    const ibis::math::term *aterm = tms.at(itm);
	    if (aterm->termType() != ibis::math::VARIABLE) {
		if (aterm->termType() == ibis::math::UNDEFINED) {
		    LOGGER(ibis::gVerbose > 1)
			<< mesg << " -- can not handle a math::term "
			"of undefined type";
		}
		else {
		    if (tls[i] == ibis::UNKNOWN_TYPE)
			tls[i] = ibis::DOUBLE;

		    array_t<double> tmp;
		    ierr = (*it)->calculate(*aterm, *hits, tmp);
		    ibis::util::addIncoreData
			(buff[i], tmp, nh,
			 std::numeric_limits<double>::quiet_NaN());
		}
		continue;
	    }

	    const ibis::column* col = (*it)->getColumn(tms.argName(itm));
	    if (col == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << mesg << " -- \"" << tms.argName(itm)
		    << "\" is not a column of partition " << (*it)->name();
		continue;
	    }

	    if (tls[i] == ibis::UNKNOWN_TYPE)
		tls[i] = col->type();
	    switch (col->type()) {
	    case ibis::BYTE:
	    case ibis::UBYTE: {
		array_t<char>* tmp = col->selectBytes(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData(buff[i], *tmp, nh,
						  static_cast<char>(0x7F));
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::SHORT:
	    case ibis::USHORT: {
		array_t<int16_t>* tmp = col->selectShorts(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData(buff[i], *tmp, nh,
						  static_cast<int16_t>(0x7FFF));
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::UINT:
	    case ibis::INT: {
		array_t<int32_t>* tmp = col->selectInts(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh,
			     static_cast<int32_t>(0x7FFFFFFF));
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::LONG:
	    case ibis::ULONG: {
		array_t<int64_t>* tmp = col->selectLongs(*hits);
		if (tmp != 0) {
		    if (tmp != 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh, static_cast<int64_t>
			     (0x7FFFFFFFFFFFFFFFLL));
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::FLOAT: {
		array_t<float>* tmp = col->selectFloats(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh,
			     std::numeric_limits<float>::quiet_NaN());
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* tmp = col->selectDoubles(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh,
			     std::numeric_limits<double>::quiet_NaN());
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::TEXT: {
		std::vector<std::string>* tmp = col->selectStrings(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addStrings(buff[i], *tmp, nh);
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::CATEGORY: {
		std::vector<std::string>* tmp = col->selectStrings(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			ibis::util::addStrings(buff[i], *tmp, nh);
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    default: {
		LOGGER(ibis::gVerbose > 1)
		    << mesg << " -- unable to process column " << tms[itm]
		    << " (type " << ibis::TYPESTRING[(int)tls[i]] << ")";
		break;}
	    }
	}
	nh += nqq;
    }
    if (nh == 0) { // return an empty table of type tabula
	return new ibis::tabula(nh);
    }

    // convert the selection into a in-memory data partition
    std::string tn = ibis::util::shortName(mesg);
    ibis::table::stringList  nlsptr(nls.size());
    std::vector<std::string> desc(nls.size());
    ibis::table::stringList  cdesc(nls.size());
    for (uint32_t i = 0; i < nls.size(); ++ i) {
	const uint32_t itm = tmstouse[i];
	desc[i] = tms.termName(itm);
	cdesc[i] = desc[i].c_str();
	nlsptr[i] = (nplain >= tms.size() ? desc[i].c_str() : nls[i].c_str());
    }

    ibis::bord *brd1 =
	new ibis::bord(tn.c_str(), mesg.c_str(), nh, nlsptr, tls, buff, &cdesc);
    if (nplain >= tms.size() || brd1 == 0)
	return brd1;

    ibis::table *brd2 = brd1->groupby(tms);
    delete brd1;
    return brd2;
} // ibis::table::select

/// It iterates through all data partitions to compute the number of hits.
int64_t ibis::table::computeHits(const std::vector<const ibis::part*>& pts,
				 const char* cond) {
    if (cond == 0 || *cond == 0)
	return -1;

    int ierr;
    uint64_t nhits = 0;
    ibis::countQuery qq;
    ierr = qq.setWhereClause(cond);
    if (ierr < 0)
	return ierr;

    for (std::vector<const ibis::part*>::const_iterator it = pts.begin();
	 it != pts.end(); ++ it) {
	ierr = qq.setPartition(*it);
	if (ierr < 0) continue;
	ierr = qq.evaluate();
	if (ierr == 0) {
	    nhits += qq.getNumHits();
	}
	else if (ibis::gVerbose > 1) {
	    ibis::util::logger lg;
	    lg.buffer() << "Warning -- table::computeHits failed to evaluate \""
			<< cond << "\" on data partition " << (*it)->name()
			<< ", query::evaluate returned " << ierr;
	}
    }
    return nhits;
} // ibis::table::computeHits

/// It iterates through all data partitions to compute the number of hits.
int64_t ibis::table::computeHits(const std::vector<const ibis::part*>& pts,
				 const ibis::qExpr* cond) {
    if (cond == 0)
	return -1;

    int ierr;
    uint64_t nhits = 0;
    ibis::countQuery qq;
    ierr = qq.setWhereClause(cond);
    if (ierr < 0)
	return ierr;

    for (std::vector<const ibis::part*>::const_iterator it = pts.begin();
	 it != pts.end(); ++ it) {
	ierr = qq.setPartition(*it);
	if (ierr < 0) continue;
	ierr = qq.evaluate();
	if (ierr == 0) {
	    nhits += qq.getNumHits();
	}
	else if (ibis::gVerbose > 1) {
	    ibis::util::logger lg;
	    lg.buffer() << "Warning -- table::computeHits failed to evaluate \""
			<< *cond << "\" on data partition " << (*it)->name()
			<< ", query::evaluate returned " << ierr;
	}
    }
    return nhits;
} // ibis::table::computeHits

template <typename T>
void ibis::util::addIncoreData(void*& to, const array_t<T>& from,
			       uint32_t nold, const T special) {
    const uint32_t nqq = from.size();

    if (to == 0)
	to = new array_t<T>();
    array_t<T>& target = * (static_cast<array_t<T>*>(to));
    target.reserve(nold+nqq);
    if (nold > target.size())
	target.insert(target.end(), nold-target.size(), special);
    if (nqq > 0)
	target.insert(target.end(), from.begin(), from.end());
} // ibis::util::addIncoreData

void ibis::util::addStrings(void*& to, const std::vector<std::string>& from,
			    uint32_t nold) {
    const uint32_t nqq = from.size();
    if (to == 0)
	to = new std::vector<std::string>();
    std::vector<std::string>& target =
	* (static_cast<std::vector<std::string>*>(to));
    target.reserve(nold+nqq);
    if (nold > target.size()) {
	const std::string dummy;
	target.insert(target.end(), nold-target.size(), dummy);
    }
    if (nqq > 0)
	target.insert(target.end(), from.begin(), from.end());
} // ibis::util::addStrings

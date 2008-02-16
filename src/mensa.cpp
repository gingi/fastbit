// File $Id$
// Author: John Wu <John.Wu@ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2008 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tab.h"	// ibis::tabula and ibis::tabele
#include "bord.h"	// ibis::bord
#include "mensa.h"	// ibis::mensa
#include "part.h"	// ibis::part
#include "query.h"	// ibis::query
#include "index.h"	// ibis::index

#include <sstream>	// std::ostringstream
#include <limits>	// std::numeric_limits
#include <cmath>	// std::floor

ibis::mensa::mensa(const char* dir) : nrows(0) {
    if (dir != 0 && *dir != 0)
	ibis::util::tablesFromDir(parts, dir);
    if (parts.empty())
	ibis::util::tablesFromResources(parts, ibis::gParameters());
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it).second->combineNames(naty);
	nrows += (*it).second->nRows();
    }
    if (name_.empty() && ! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = (*it).first;
	if (desc_.empty()) {
	    if (dir != 0 && *dir != 0)
		desc_ = dir;
	    else
		desc_ = "data specified in RC file";
	}
    }
    if (ibis::gVerbose > 0) {
	ibis::util::logger lg(0);
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

ibis::mensa::mensa(const char* dir1, const char* dir2) : nrows(0) {
    if (dir1 != 0 && *dir1 != 0) {
	ibis::util::tablesFromDir(parts, dir1, dir2);
    }
    if (parts.empty())
	ibis::util::tablesFromResources(parts, ibis::gParameters());
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it).second->combineNames(naty);
	nrows += (*it).second->nRows();
    }
    if (name_.empty() && ! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = (*it).first;
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
    if (ibis::gVerbose > 0) {
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

void ibis::mensa::clear() {
    LOGGER(3) << "ibis::mensa::clear -- clearing the existing content of "
	      << parts.size() << " partition"
	      << (parts.size()>1 ? "s" : "") << " with "
	      << naty.size() << " column"
	      << (naty.size()>1 ? "s" : "") << " and "
	      << nrows << " row" << (nrows>1 ? "s" : "");

    nrows = 0;
    naty.clear();
    name_.erase();
    desc_.erase();
    while (! parts.empty()) {
	ibis::partList::iterator it = parts.begin();
	ibis::part *tmp = (*it).second;
	parts.erase(it);
	delete tmp;
    }
} // ibis::mensa::clear

ibis::table::stringList ibis::mensa::columnNames() const {
    ibis::table::stringList res(naty.size());
    size_t i = 0;
    for (ibis::table::namesTypes::const_iterator it = naty.begin();
	 it != naty.end(); ++ it, ++ i)
	res[i] = (*it).first;
    return res;
} // ibis::mensa::columnNames
ibis::table::typeList ibis::mensa::columnTypes() const {
    ibis::table::typeList res(naty.size());
    size_t i = 0;
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

const char* ibis::mensa::indexSpec(const char* colname) const {
    if (parts.empty()) {
	return 0;
    }
    else if (colname == 0 || *colname == 0) {
	ibis::partList::const_iterator it = parts.begin();
	return (*it).second->indexSpec();
    }
    else {
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::column* col = (*it).second->getColumn(colname);
	    if (col != 0)
		return col->indexSpec();
	}
	return (*(parts.begin())).second->indexSpec();
    }
} // ibis::mensa::indexSpec

void ibis::mensa::indexSpec(const char* opt, const char* colname) {
    for (ibis::partList::iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	if (colname == 0 || *colname == 0) {
	    (*it).second->indexSpec(opt);
	}
	else {
	    ibis::column* col = (*it).second->getColumn(colname);
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
	const ibis::column* col = (*it).second->getColumn(colname);
	if (col != 0) {
	    ibis::index* ind = ibis::index::create(col, 0, option);
	    if (ind != 0) {
		++ ierr;
		delete ind;
	    }
	    else {
		LOGGER(2) << "ibis::mensa::buildIndex(" << colname << ", "
			  << (option != 0 ? option : col->indexSpec())
			  << ") failed";
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
	(*it).second->loadIndex(opt);
	(*it).second->unloadIndex();
    }
    return 0;
} // ibis::mensa::buildIndexes

void ibis::mensa::estimate(const char* cond,
			   uint64_t& nmin, uint64_t& nmax) const {
    nmin = 0;
    nmax = 0;
    ibis::query q(ibis::util::userName());
    int ierr = q.setWhereClause(cond);
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	if (ierr >= 0) {
	    ierr = q.setTable((*it).second);
	    if (ierr >= 0) {
		ierr = q.estimate();
		if (ierr >= 0) {
		    nmin += q.getMinNumHits();
		    nmax += q.getMaxNumHits();
		}
		else {
		    nmax += (*it).second->nRows();
		}
	    }
	    else {
		nmax += (*it).second->nRows();
	    }
	    ierr = 0;
	}
	else {
	    nmax += (*it).second->nRows();
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
    else if (strnicmp(sel, "count(", 6) == 0) { // count(*)
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
	// collect all values from different data partitions
	ibis::selected nms(sel);
	if (nms.size() == 0) {
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

	ibis::query qq(ibis::util::userName());
	int ierr = qq.setWhereClause(cond);
	if (ierr < 0) return 0;

	std::vector<std::string> nls(nms.size());
	ibis::table::typeList tls(nms.size());
	ibis::bord::bufferList buff(nms.size());
	for (size_t i = 0; i < nms.size(); ++ i) {
	    nls[i] = nms.getTerm(i);
	    tls[i] = ibis::UNKNOWN_TYPE;
	    buff[i] = 0;
	}

	size_t nh = 0;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = qq.setTable((*it).second);
	    if (ierr < 0) continue;

	    ierr = qq.evaluate();
	    if (ierr != 0) continue;

	    const ibis::bitvector* mask = qq.getHitVector();
	    if (mask == 0 || mask->cnt() == 0) continue;

	    const size_t nqq = mask->cnt();
	    for (size_t i = 0; i < nms.size(); ++ i) {
		const ibis::column* col = (*it).second->getColumn(nms[i]);
		if (col == 0) continue;

		if (tls[i] == ibis::UNKNOWN_TYPE)
		    tls[i] = col->type();
		switch (tls[i]) {
		case ibis::BYTE:
		case ibis::UBYTE: {
		    array_t<char>* tmp = col->selectBytes(*mask);
		    if (tmp != 0)
			addIncoreData(buff[i], *tmp, nh,
				      static_cast<char>(0x7F));
		    break;}
		case ibis::SHORT:
		case ibis::USHORT: {
		    array_t<int16_t>* tmp = col->selectShorts(*mask);
		    if (tmp != 0)
			addIncoreData(buff[i], *tmp, nh,
				      static_cast<int16_t>(0x7FFF));
		    break;}
		case ibis::UINT:
		case ibis::INT: {
		    array_t<int32_t>* tmp = col->selectInts(*mask);
		    if (tmp != 0)
			addIncoreData(buff[i], *tmp, nh,
				      static_cast<int32_t>(0x7FFFFFFF));
		    break;}
		case ibis::LONG:
		case ibis::ULONG: {
		    array_t<int64_t>* tmp = col->selectLongs(*mask);
		    if (tmp != 0)
			addIncoreData(buff[i], *tmp, nh, static_cast<int64_t>
				      (0x7FFFFFFFFFFFFFFFLL));
		    break;}
		case ibis::FLOAT: {
		    array_t<float>* tmp = col->selectFloats(*mask);
		    if (tmp != 0)
			addIncoreData(buff[i], *tmp, nh,
				      std::numeric_limits<float>::quiet_NaN());
		    break;}
		case ibis::DOUBLE: {
		    array_t<double>* tmp = col->selectDoubles(*mask);
		    if (tmp != 0)
			addIncoreData
			    (buff[i], *tmp, nh,
			     std::numeric_limits<double>::quiet_NaN());
		    break;}
		case ibis::TEXT:
		case ibis::CATEGORY: {
		    std::vector<std::string>* tmp = col->selectStrings(*mask);
		    if (tmp != 0)
			addStrings(buff[i], *tmp, nh);
		    break;}
		default: {
		    LOGGER(2) << "ibis::mensa::select is not able to "
			"process column " << nms[i] << " (type "
			      << ibis::TYPESTRING[(int)tls[i]] << ")";
		    break;}
		}
	    }
	    nh += nqq;
	}

	std::string de = "SELECT ";
	de += sel;
	de += " FROM ";
	de += name_;
	de += " WHERE ";
	de += cond;
	std::string tn = ibis::util::shortName(de);
	ibis::table::stringList nlsptr(nls.size());
	for (size_t i = 0; i < nls.size(); ++ i)
	    nlsptr[i] = nls[i].c_str();
	return new ibis::bord(tn.c_str(), de.c_str(), nh, nlsptr, tls, buff);
    }
} // ibis::mensa::select

template <typename T>
void ibis::mensa::addIncoreData(void*& to, const array_t<T>& from,
				size_t nold, const T special) const {
    const size_t nqq = from.size();

    if (to == 0)
	to = new array_t<T>();
    array_t<T>& target = * (static_cast<array_t<T>*>(to));
    target.reserve(nold+nqq);
    if (nold > target.size())
	target.insert(target.end(), nold-target.size(), special);
    if (nqq > 0)
	target.insert(target.end(), from.begin(), from.end());
} // ibis::mensa::addIncoreData

void ibis::mensa::addStrings(void*& to, const std::vector<std::string>& from,
			     size_t nold) const {
    const size_t nqq = from.size();
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
} // ibis::mensa::addStrings

int64_t ibis::mensa::computeHits(const char* cond) const {
    if (cond == 0 || *cond == 0)
	return nrows;

    int ierr;
    uint64_t nhits = 0;
    ibis::query qq(ibis::util::userName());
    ierr = qq.setWhereClause(cond);
    if (ierr < 0)
	return ierr;

    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	ierr = qq.setTable((*it).second);
	if (ierr < 0) continue;
	ierr = qq.evaluate();
	if (ierr == 0)
	    nhits += qq.getNumHits();
    }
    return nhits;
} // ibis::mensa::computeHits

/// Reordering the rows using the specified columns.  Each data partition
/// is reordered separately.
void ibis::mensa::orderby(const ibis::table::stringList& names) {
    for (ibis::partList::iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	long ierr = (*it).second->reorder(names);
	if (ierr < 0) {
	    LOGGER(0)
		<< "ibis::mensa::orderby -- reordering partition "
		<< (*it).first << " encountered error " << ierr;
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

    size_t ierr = 0;
    array_t<char> tmp;
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	const ibis::part& dp = *((*it).second);
	if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
	    LOGGER(0)
		<< "ibis::mensa::getColumnAsBytes encounters an "
		"integer overflow problem (number of elements assigned "
		"so far is " << ierr << ")";
	    break;
	}

	const ibis::column* col = dp.getColumn(cn);
	if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    array_t<unsigned char> tmp;
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	const ibis::part& dp = *((*it).second);
	if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
	    LOGGER(0)
		<< "ibis::mensa::getColumnAsUBytes encounters an "
		"integer overflow problem (number of elements assigned "
		"so far is " << ierr << ")";
	    break;
	}

	const ibis::column* col = dp.getColumn(cn);
	if (col != 0 && col->getRawData(tmp) >= 0)
	    memcpy(vals+ierr, tmp.begin(), dp.nRows());
	ierr += dp.nRows();
    }
    return ierr;
} // ibis::mensa::getColumnAsUBytes

int64_t ibis::mensa::getColumnAsShorts(const char* cn, int16_t* vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsUShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsUShorts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsUInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsUInts encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsLongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsULongs encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsFloats encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
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

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::SHORT: {
	array_t<int16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::UINT: {
	array_t<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i)
		    vals[ierr+i] = tmp[i];
	    }
	    ierr += dp.nRows();
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsDoubles encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0)
		memcpy(vals+ierr, tmp.begin(), dp.nRows()*sizeof(double));
	    ierr += dp.nRows();
	}
	break;}
    default:
	return -2;
    }
    return ierr;
} // ibis::mensa::getColumnAsDoubles

/// @note Any data type can be converted to strings, however, the
/// conversion may take a significant amount of time.
int64_t ibis::mensa::getColumnAsStrings(const char* cn,
					std::vector<std::string>& vals) const {
    ibis::table::namesTypes::const_iterator nit = naty.find(cn);
    if (nit == naty.end())
	return -1;

    size_t ierr = 0;
    switch ((*nit).second) {
    case ibis::BYTE: {
	array_t<char> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0 && col->getRawData(tmp) >= 0) {
		for (size_t i=0; i < tmp.size(); ++ i) {
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
	    const ibis::part& dp = *((*it).second);
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(0)
		    << "ibis::mensa::getColumnAsStrings encounters an "
		    "integer overflow problem (number of elements assigned "
		    "so far is " << ierr << ")";
		break;
	    }

	    const ibis::column* col = dp.getColumn(cn);
	    if (col != 0) {
		for (size_t i=0; i < dp.nRows(); ++ i) {
		    col->getString(i, tmp);
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
			       std::vector<size_t>& counts) const {
    long ierr = -1;
    if (cname == 0 || *cname == 0 || (begin >= end && !(stride < 0.0)) ||
	(begin <= end && !(stride > 0.0))) return ierr;
    if (sizeof(size_t) == sizeof(uint32_t)) {
	counts.clear();
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it).second->get1DDistribution
		(constraints, cname, begin, end, stride,
		 reinterpret_cast<std::vector<uint32_t>&>(counts));
	    if (ierr < 0) return ierr;
	}
    }
    else {
	const size_t nbins = 1 + 
	    static_cast<uint32_t>(std::floor((end - begin) / stride));
	counts.resize(nbins);
	for (size_t i = 0; i < nbins; ++ i)
	    counts[i] = 0;

	std::vector<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    tmp.clear();
	    ierr = (*it).second->get1DDistribution(constraints, cname, begin,
						   end, stride, tmp);
	    if (ierr < 0) return ierr;
	    for (size_t i = 0; i < nbins; ++ i)
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
				 std::vector<size_t>& counts) const {
    long ierr = -1;
    if (cname1 == 0 || cname2 == 0 || (begin1 >= end1 && !(stride1 < 0.0)) ||
	(begin1 <= end1 && !(stride1 > 0.0)) ||
	*cname1 == 0 || *cname2 == 0 ||  (begin2 >= end2 && !(stride2 < 0.0)) ||
	(begin2 <= end2 && !(stride2 > 0.0))) return ierr;

    if (sizeof(size_t) == sizeof(uint32_t)) {
	counts.clear();

	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it).second->get2DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2,
		 reinterpret_cast<std::vector<uint32_t>&>(counts));
	    if (ierr < 0) return ierr;
	}
    }
    else {
	const size_t nbins =
	    (1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	    (1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2)));
	counts.resize(nbins);
	for (size_t i = 0; i < nbins; ++ i)
	    counts[i] = 0;

	std::vector<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it).second->get2DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2, tmp);
	    if (ierr < 0) return ierr;

	    for (size_t i = 0; i < nbins; ++ i)
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
				 std::vector<size_t>& counts) const {
    long ierr = -1;
    if (cname1 == 0 || cname2 == 0 || cname3 == 0 ||
	*cname1 == 0 || *cname2 == 0 || *cname3 == 0 ||
	(begin1 >= end1 && !(stride1 < 0.0)) ||
	(begin1 <= end1 && !(stride1 > 0.0)) ||
	(begin2 >= end2 && !(stride2 < 0.0)) ||
	(begin2 <= end2 && !(stride2 > 0.0)) ||
	(begin3 >= end3 && !(stride3 < 0.0)) ||
	(begin3 <= end3 && !(stride3 > 0.0))) return -1;

    if (sizeof(size_t) == sizeof(uint32_t)) {
	counts.clear();

	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it).second->get3DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2, cname3, begin3, end3, stride3,
		 reinterpret_cast<std::vector<uint32_t>&>(counts));
	    if (ierr < 0) return ierr;
	}
    }
    else {
	const size_t nbins =
	    (1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	    (1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2))) *
	    (1 + static_cast<uint32_t>(std::floor((end3 - begin3) / stride3)));
	counts.resize(nbins);
	for (size_t i = 0; i < nbins; ++ i)
	    counts[i] = 0;

	std::vector<uint32_t> tmp;
	for (ibis::partList::const_iterator it = parts.begin();
	     it != parts.end(); ++ it) {
	    ierr = (*it).second->get3DDistribution
		(constraints, cname1, begin1, end1, stride1, cname2, begin2,
		 end2, stride2, cname3, begin3, end3, stride3, tmp);
	    if (ierr < 0) return ierr;

	    for (size_t i = 0; i < nbins; ++ i)
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
    : buffer(t.nColumns()), tab(t), curRow(-1), curBlock(0),
      curPart(t.parts.begin()) {
    if (curPart == t.parts.end()) return; // no data partition
    if (buffer.empty()) return;

    // linearize the t.naty to buffer, build a mapping between column names
    // and the position in the buffer
    size_t j = 0;
    for (ibis::table::namesTypes::const_iterator it = t.naty.begin();
	 it != t.naty.end(); ++ it, ++ j) {
	buffer[j].cname = (*it).first;
	buffer[j].ctype = (*it).second;
	buffer[j].cval = 0;
	bufmap[(*it).first] = j;
    }
} // ibis::mensa::cursor::cursor

/// Fill the buffer for variable number @c i.  On success, return 0,
/// otherwise return a negative value.
/// @note The member variable @c cval in the buffer for a string valued
/// column is not the usual ibis::fileManager::storage object, instead
/// it is simply the pointer to the ibis::column object.  The string
/// variable is retrieved through the column object one at a time using the
/// function @c getString.
int ibis::mensa::cursor::fillBuffer(size_t i) const {
    if (buffer[i].cval != 0) return 0; // already filled
    // has to do a look up based on the column name, becaus the ith column
    // of the data partition may not be the correct one (some columns may
    // be missing)
    const ibis::column* col = (*curPart).second->getColumn(buffer[i].cname);
    if (col == 0)
	return -2;

    if (buffer[i].ctype != ibis::CATEGORY && buffer[i].ctype != ibis::TEXT) {
	buffer[i].cval = col->getRawData();
	if (buffer[i].cval != 0) {
	    buffer[i].cval->beginUse();
	    return 0;
	}
	else {
	    return -1;
	}
    }
    else {
	buffer[i].cval = reinterpret_cast<ibis::fileManager::storage*>
	    (const_cast<ibis::column*>(col));
	return 0;
    }
} // ibis::mensa::cursor::fillBuffer

/// Fill the buffers for every column.
void ibis::mensa::cursor::fillBuffers() const {
    for (size_t i = 0; i < buffer.size(); ++ i) {
	if (buffer[i].cval != 0) {
	    buffer[i].cval->endUse();
	}
	const ibis::column* col =
	    (*curPart).second->getColumn(buffer[i].cname);
	if (col != 0) {
	    if (col->type() != ibis::CATEGORY && col->type() != ibis::TEXT) {
		buffer[i].cval = col->getRawData();
		if (buffer[i].cval != 0)
		    buffer[i].cval->beginUse();
	    }
	    else {
		buffer[i].cval = reinterpret_cast<ibis::fileManager::storage*>
		    (const_cast<ibis::column*>(col));
	    }
	}
	else {
	    buffer[i].cval = 0;
	}
    }
} // ibis::mensa::cursor::fillBuffers

void ibis::mensa::cursor::clearBuffers() {
    for (size_t i = 0; i < buffer.size(); ++ i) {
	if (buffer[i].cval != 0 && buffer[i].ctype != ibis::CATEGORY &&
	    buffer[i].ctype != ibis::TEXT)
	    buffer[i].cval->endUse();
	buffer[i].cval = 0;
    }
} // ibis::mensa::cursor::clearBuffers

int ibis::mensa::cursor::fetch() {
    if (curPart == tab.parts.end()) return -1;

    ++ curRow;
    if (static_cast<uint64_t>(curRow) >=
	curBlock + (*curPart).second->nRows()) {
	curBlock += (*curPart).second->nRows();
	clearBuffers();
	++ curPart;
    }
    return (0 - (curPart == tab.parts.end()));
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(uint64_t irow) {
    if (curPart == tab.parts.end()) return -1;
    if (curBlock + (*curPart).second->nRows() <= irow)
	clearBuffers();

    while (curBlock + (*curPart).second->nRows() <= irow &&
	   curPart != tab.parts.end()) {
	curBlock += (*curPart).second->nRows();
	++ curPart;
    }
    if (curPart != tab.parts.end()) {
	curRow = irow;
	return 0;
    }
    else {
	curRow = curBlock;
	return -1;
    }
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(ibis::table::row& res) {
    if (curPart == tab.parts.end()) return -1;

    ++ curRow;
    if (static_cast<uint64_t>(curRow) >
	curBlock + (*curPart).second->nRows()) {
	curBlock += (*curPart).second->nRows();
	fillBuffers();
	++ curPart;
    }
    if (curPart != tab.parts.end()) {
	fillRow(res);
	return 0;
    }
    else {
	return -1;
    }
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(uint64_t irow, ibis::table::row& res) {
    if (curPart == tab.parts.end()) return -1;
    ibis::partList::const_iterator oldPart = curPart;
    while (curBlock + (*curPart).second->nRows() <= irow &&
	   curPart != tab.parts.end()) {
	curBlock += (*curPart).second->nRows();
	++ curPart;
    }
    if (oldPart != curPart)
	fillBuffers();

    if (curPart != tab.parts.end()) {
	curRow = static_cast<int64_t>(irow);
	fillRow(res);
	return 0;
    }
    else {
	curRow = curBlock;
	return -1;
    }
} // ibis::mensa::cursor::fetch

void ibis::mensa::cursor::fillRow(ibis::table::row& res) const {
    res.clear();
    const size_t il = static_cast<size_t>(curRow - curBlock);
    for (size_t j = 0; j < buffer.size(); ++ j) {
	switch (buffer[j].ctype) {
	case ibis::BYTE: {
	    res.bytesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.bytesvalues.push_back(buffer[j].cval->begin()[il]);
	    }
	    else {
		res.bytesvalues.push_back(0x7F);
	    }
	    break;}
	case ibis::UBYTE: {
	    res.ubytesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.ubytesvalues.push_back
		    (reinterpret_cast<const unsigned char*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.ubytesvalues.push_back(0xFFU);
	    }
	    break;}
	case ibis::SHORT: {
	    res.shortsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.shortsvalues.push_back
		    (reinterpret_cast<const int16_t*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.shortsvalues.push_back(0x7FFF);
	    }
	    break;}
	case ibis::USHORT: {
	    res.ushortsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.ushortsvalues.push_back
		    (reinterpret_cast<const uint16_t*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.ushortsvalues.push_back(0xFFFFU);
	    }
	    break;}
	case ibis::INT: {
	    res.intsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.intsvalues.push_back
		    (reinterpret_cast<const int32_t*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.intsvalues.push_back(0x7FFFFFFF);
	    }
	    break;}
	case ibis::UINT: {
	    res.uintsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.uintsvalues.push_back
		    (reinterpret_cast<const uint32_t*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.uintsvalues.push_back(0xFFFFFFFFU);
	    }
	    break;}
	case ibis::LONG: {
	    res.longsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.longsvalues.push_back
		    (reinterpret_cast<const int64_t*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.longsvalues.push_back(0x7FFFFFFFFFFFFFFFLL);
	    }
	    break;}
	case ibis::ULONG: {
	    res.ulongsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.ulongsvalues.push_back
		    (reinterpret_cast<const uint64_t*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.ulongsvalues.push_back(0xFFFFFFFFFFFFFFFFULL);
	    }
	    break;}
	case ibis::FLOAT: {
	    res.floatsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.floatsvalues.push_back
		    (reinterpret_cast<const float*>
		     (buffer[j].cval->begin())[il]);
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
		    (reinterpret_cast<const double*>
		     (buffer[j].cval->begin())[il]);
	    }
	    else {
		res.doublesvalues.push_back
		    (std::numeric_limits<double>::quiet_NaN());
	    }
	    break;}
	case ibis::TEXT: {
	    res.textsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.textsvalues.push_back
		    ((* reinterpret_cast<const std::vector<std::string>*>
		      (buffer[j].cval))[il]);
	    }
	    else {
		res.textsvalues.push_back("");
	    }
	    break;}
	case ibis::CATEGORY: {
	    res.catsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval != 0) {
		res.catsvalues.push_back
		    ((* reinterpret_cast<const std::vector<std::string>*>
		      (buffer[j].cval))[il]);
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
   return values:
   0  -- normal (successful) completion
  -1  -- no data in-memory
  -2  -- unknown data type
  -3  -- string-valued columns not currently supported
  -4  -- error in the output stream
 */
int ibis::mensa::cursor::dump(std::ostream& out, const char* del) const {
    if (tab.nColumns() == 0) return 0;
    if (curRow < 0 || curPart == tab.parts.end())
	return -1;
    if (static_cast<uint64_t>(curRow) == curBlock)
	// first time accessing the data partition
	fillBuffers();

    const size_t i = static_cast<size_t>(curRow - curBlock);
    int ierr = dumpIJ(out, i, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (size_t j = 1; j < tab.nColumns(); ++ j) {
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
    if (curPart == tab.parts.end()) return 0;
    if (curRow < 0) // not initialized
	return -1;
    if (static_cast<uint64_t>(curRow) == curBlock)
	// first time accessing the data partition
	fillBuffers();

    size_t i = static_cast<size_t>(curRow - curBlock);
    // print the first row with error checking
    int ierr = dumpIJ(out, i, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (size_t j = 1; j < tab.nColumns(); ++ j) {
	out << del;
	ierr = dumpIJ(out, i, j);
	if (ierr < 0)
	    return ierr;
    }
    out << "\n";
    if (! out)
	ierr = -4;
    // print the rest of the rows without error checking
    for (++ i; i < (*curPart).second->nRows(); ++ i) {
	(void) dumpIJ(out, i, 0U);
	for (size_t j = 1; j < buffer.size(); ++ j) {
	    out << del;
	    (void) dumpIJ(out, i, j);
	}
	out << "\n";
    }

    // move the position of the cursor to the last row of the block
    curRow += ((*curPart).second->nRows() - 1);
    return (out ? 0 : -4);
} // ibis::mensa::cursor::dumpBlock

int ibis::mensa::cursor::getColumnAsByte(size_t j, char* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j); // first time accessing the data partition
    if (ierr < 0 || buffer[j].cval == 0) // missing column in this partition
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsByte

int ibis::mensa::cursor::getColumnAsUByte(size_t j, unsigned char* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j); // first time accessing the data partition
    if (ierr < 0 || buffer[j].cval == 0) // missing column in this partition
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUByte

int ibis::mensa::cursor::getColumnAsShort(size_t j, int16_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>
		 (buffer[j].cval->begin()) + (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsShort

int ibis::mensa::cursor::getColumnAsUShort(size_t j, uint16_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUShort

int ibis::mensa::cursor::getColumnAsInt(size_t j, int32_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsInt

int ibis::mensa::cursor::getColumnAsUInt(size_t j, uint32_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUInt

int ibis::mensa::cursor::getColumnAsLong(size_t j, int64_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::INT: {
	*val = *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	*val = *(reinterpret_cast<int64_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsLong

int ibis::mensa::cursor::getColumnAsULong(size_t j, uint64_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	*val = *(reinterpret_cast<uint64_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsULong

int ibis::mensa::cursor::getColumnAsFloat(size_t j, float* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	*val = *(reinterpret_cast<float*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsFloat

int ibis::mensa::cursor::getColumnAsDouble(size_t j, double* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::INT: {
	*val = *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	*val = *(reinterpret_cast<double*>(buffer[j].cval->begin()) +
		 (curRow - curBlock));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsDouble

int ibis::mensa::cursor::getColumnAsString(size_t j, std::string& val) const {
    if (curRow < 0 || curPart == tab.parts.end() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == curBlock)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    const uint32_t irow = static_cast<uint32_t>(curRow-curBlock);
    std::ostringstream oss;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	oss << static_cast<int>(*(buffer[j].cval->begin() +
				  irow));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	oss << static_cast<unsigned>
	    (*(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
	       irow));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	oss << *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::USHORT: {
	oss << *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::INT: {
	oss << *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::UINT: {
	oss << *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::LONG: {
	oss << *(reinterpret_cast<int64_t*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::ULONG: {
	oss << *(reinterpret_cast<uint64_t*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	oss << *(reinterpret_cast<float*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	oss << *(reinterpret_cast<double*>(buffer[j].cval->begin()) +
		 irow);
	ierr = 0;
	break;}
    case ibis::CATEGORY:
    case ibis::TEXT: {
	const ibis::column* col =
	    (*curPart).second->getColumn(buffer[j].cname);
	if (col != 0) {
	    col->getString(irow, val);
	    ierr = 0;
	}
	else {
	    ierr = -1;
	}
	break;}
    default: {
	ierr = -1;
	break;}
    }
    val = oss.str();
    return ierr;
} // ibis::mensa::cursor::getColumnAsString

///@note If the incoming directory name is nil or an empty string, it
///attempts to use the directories specified in the configuration files.
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

void ibis::table::parseNames(char* in, ibis::table::stringList& out) const {
    char* ptr1 = in;
    while (*ptr1 != 0 && isspace(*ptr1) != 0) ++ ptr1; // leading space
    // since SQL names can not contain space, quotes must be for the whole
    // list of names
    if (*ptr1 == '\'') {
	++ ptr1; // skip opening quote
	char* ptr2 = strchr(ptr1, '\'');
	if (ptr2 > ptr1)
	    *ptr2 = 0; // string terminates here
    }
    else if (*ptr1 == '"') {
	++ ptr1;
	char* ptr2 = strchr(ptr1, '"');
	if (ptr2 > ptr1)
	    *ptr2 = 0;
    }

    while (*ptr1 != 0) {
	char* ptr2;
	for (ptr2 = ptr1; *ptr2 != 0 && isalnum(*ptr2) != 0; ++ ptr2);
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
	ptr1 = ptr2;
    }
} // ibis::table::parseNames

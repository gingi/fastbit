// File $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
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
#include "category.h"	// ibis::text
#include "selectClause.h"//ibis::selectClause

#include <sstream>	// std::ostringstream
#include <limits>	// std::numeric_limits
#include <cmath>	// std::floor

/// This function expects a valid data directory to find data partitions.
/// If the incoming directory is not a valid string, it will use
/// ibis::gParameter() to find data partitions.
ibis::mensa::mensa(const char* dir) : nrows(0) {
    if (dir != 0 && *dir != 0)
	ibis::util::tablesFromDir(parts, dir);
    if (parts.empty())
	ibis::util::tablesFromResources(parts, ibis::gParameters());
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

/// This function expects a pair of data directories to define data
/// partitions.  If either dir1 and dir2 is not valid, it will attempt to
/// find data partitions using global parameters ibis::gParameters().
ibis::mensa::mensa(const char* dir1, const char* dir2) : nrows(0) {
    if (*dir1 == 0 && *dir2 == 0) return;
    if (dir1 != 0 && *dir1 != 0) {
	ibis::util::tablesFromDir(parts, dir1, dir2);
    }
    if (parts.empty())
	ibis::util::tablesFromResources(parts, ibis::gParameters());
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

int ibis::mensa::addPartition(const char* dir) {
    const size_t npold = parts.size();
    const size_t ncold = naty.size();
    const uint64_t nrold = nrows;
    unsigned int newparts = 0;
    if (dir != 0 && *dir != 0)
	newparts = ibis::util::tablesFromDir(parts, dir);
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
    return (naty.size() > ncold && ncold > 0);
} // ibis::mensa::addPartition

void ibis::mensa::clear() {
    LOGGER(ibis::gVerbose > 2)
	<< "ibis::mensa::clear -- clearing the existing content of "
	<< parts.size() << " partition"
	<< (parts.size()>1 ? "s" : "") << " with "
	<< naty.size() << " column"
	<< (naty.size()>1 ? "s" : "") << " and "
	<< nrows << " row" << (nrows>1 ? "s" : "");

    nrows = 0;
    naty.clear();
    name_.erase();
    desc_.erase();
    size_t np = parts.size();
    for (size_t j = 0; j < np; ++ j)
	delete parts[j];
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
	(*it)->loadIndex(opt);
	(*it)->unloadIndex();
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
	    ierr = q.setPartition(*it);
	    if (ierr >= 0) {
		ierr = q.estimate();
		if (ierr >= 0) {
		    nmin += q.getMinNumHits();
		    nmax += q.getMaxNumHits();
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
	// handle the non-trivial case in a separate function
	return doSelect(sel, cond, parts);
    }
} // ibis::mensa::select

ibis::table* ibis::mensa::select2(const char* sel, const char* cond,
				  const char* pts) const {
    if (cond == 0 || *cond == 0) return 0;
    if (sel != 0) // skip leading space
	while (isspace(*sel)) ++ sel;
    if (pts == 0 || *pts == 0) return select(sel, cond);

    ibis::nameList patterns(pts);
    if (patterns.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::mensa::select2 can not find any valid data partition "
	    "names to use";
	return 0;
    }

    ibis::partList mylist;
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
	int64_t nhits = computeHits(cond, mylist);
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
	int64_t nhits = computeHits(cond, mylist);
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
	return doSelect(sel, cond, mylist);
    }
    return 0;
} // ibis::mensa::select2

template <typename T>
void ibis::mensa::addIncoreData(void*& to, const array_t<T>& from,
				size_t nold, const T special) {
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
			     size_t nold) {
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

/// This function expects the first two arguments to be valid and
/// non-trivial.  If will not produce the correct answer if those arguments
/// are nil pointers or empty strings or blank spaces.
ibis::table* ibis::mensa::doSelect(const char *sel, const char *cond,
				   const ibis::partList& mylist) {
    if (sel == 0 || cond == 0 || *sel == 0 || *cond == 0 || mylist.empty())
	return 0;

    ibis::selectClause tms(sel);
    if (tms.size() == 0) {
	int64_t nhits = computeHits(cond, mylist);
	if (nhits < 0) {
	    return 0;
	}
	else {
	    std::string des = "from data partitions --";
	    for (ibis::partList::const_iterator it = mylist.begin();
		 it != mylist.end(); ++ it) {
		des += " ";
		des += (*it)->name();
	    }
	    return new ibis::tabula(cond, des.c_str(), nhits);
	}
    }

    // a single query object is used for different data partitions
    ibis::query qq(ibis::util::userName());
    std::string mesg = "ibis::mensa::doSelect(\"";
    mesg += sel;
    mesg += "\", ";
    mesg += qq.id();
    mesg += ")";
    ibis::util::timer atimer(mesg.c_str(), 2);
    long int ierr = qq.setWhereClause(cond);
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << mesg << " -- can not parse \"" << cond
	    << "\" into a valid query expression";
	return 0;
    }

    // list of names, types, and buffers to hold the temporary content
    std::vector<std::string> nls(tms.size());
    ibis::table::typeList tls(tms.size());
    ibis::bord::bufferList buff(tms.size());
    for (size_t i = 0; i < tms.size(); ++ i) {
	tls[i] = ibis::UNKNOWN_TYPE;
	nls[i] = tms.getName(i);
	buff[i] = 0;
    }

    size_t nh = 0;
    // main loop through each data partition
    for (ibis::partList::const_iterator it = mylist.begin();
	 it != mylist.end(); ++ it) {
	ierr = qq.setPartition(*it);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- query[" << cond << "].setPartition("
		<< (*it)->name() << ") failed with error " << ierr;
	    continue;
	}

	ierr = tms.verify(**it);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- select clause (" << sel
		<< ") contains variables that are not in data partition "
		<< (*it)->name();
	    continue;
	}

	ierr = qq.evaluate();
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- failed to evaluate conditions ("
		<< cond << ") on data partition " << (*it)->name();
	    continue;
	}

	const ibis::bitvector* hits = qq.getHitVector();
	if (hits == 0 || hits->cnt() == 0) continue;

	const size_t nqq = hits->cnt();
	for (size_t i = 0; i < tms.size(); ++ i) {
	    const ibis::math::term *aterm = tms.at(i);
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
		    addIncoreData(buff[i], tmp, nh,
				  std::numeric_limits<double>::quiet_NaN());
		}
		continue;
	    }

	    const ibis::column* col = (*it)->getColumn(tms.getName(i));
	    if (col == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << mesg << " -- \"" << tms.getName(i)
		    << "\" is not a column of partition " << (*it)->name();
		continue;
	    }

	    if (tls[i] == ibis::UNKNOWN_TYPE)
		tls[i] = col->type();
	    switch (tls[i]) {
	    case ibis::BYTE:
	    case ibis::UBYTE: {
		array_t<char>* tmp = col->selectBytes(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			addIncoreData(buff[i], *tmp, nh,
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
			addIncoreData(buff[i], *tmp, nh,
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
			addIncoreData(buff[i], *tmp, nh,
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
			addIncoreData(buff[i], *tmp, nh, static_cast<int64_t>
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
			addIncoreData(buff[i], *tmp, nh,
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
			addIncoreData(buff[i], *tmp, nh,
				      std::numeric_limits<double>::quiet_NaN());
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    case ibis::TEXT:
	    case ibis::CATEGORY: {
		std::vector<std::string>* tmp = col->selectStrings(*hits);
		if (tmp != 0) {
		    if (nh > 0) {
			addStrings(buff[i], *tmp, nh);
			delete tmp;
		    }
		    else {
			buff[i] = tmp;
		    }
		}
		break;}
	    default: {
		LOGGER(ibis::gVerbose > 1)
		    << mesg << " -- unable to process column " << tms[i]
		    << " (type " << ibis::TYPESTRING[(int)tls[i]] << ")";
		break;}
	    }
	}
	nh += nqq;
    }

    std::string de = "SELECT ";
    de += sel;
    de += " FROM ";
    if (mylist.size() > 0) {
	ibis::partList::const_iterator it = mylist.begin();
	de += (*it)->name();
	for (++ it; it != mylist.end(); ++ it) {
	    de += ", ";
	    de += (*it)->name();
	}
    }
    else {
	de += "???";
    }
    de += " WHERE ";
    de += cond;
    std::string tn = ibis::util::shortName(de);
    ibis::table::stringList nlsptr(nls.size());
    std::vector<std::string> desc(nls.size());
    ibis::table::stringList cdesc(nls.size());
    size_t nplain = 0;
    for (size_t i = 0; i < nls.size(); ++ i) {
	nlsptr[i] = nls[i].c_str();
	nplain += (tms.getAggregator(i) == ibis::selectClause::NIL);
	tms.describe(i, desc[i]);
	cdesc[i] = desc[i].c_str();
    }

    ibis::bord *brd1 =
	new ibis::bord(tn.c_str(), de.c_str(), nh, nlsptr, tls, buff, &cdesc);
    if (nplain >= tms.size())
	return brd1;

    std::vector<std::string> aggr(nls.size());
    ibis::table::stringList caggr(nls.size());
    for (size_t i = 0; i < nls.size(); ++ i) {
	switch (tms.getAggregator(i)) {
	default:
	case ibis::selectClause::NIL:
	    aggr[i] = nls[i];
	    break;
	case ibis::selectClause::AVG:
	    aggr[i] = "AVG(";
	    aggr[i] += nls[i];
	    aggr[i] += ")";
	    break;
	case ibis::selectClause::CNT:
	    aggr[i] = "COUNT(";
	    aggr[i] += nls[i];
	    aggr[i] += ")";
	    break;
	case ibis::selectClause::MAX:
	    aggr[i] = "MAX(";
	    aggr[i] += nls[i];
	    aggr[i] += ")";
	    break;
	case ibis::selectClause::MIN:
	    aggr[i] = "MIN(";
	    aggr[i] += nls[i];
	    aggr[i] += ")";
	    break;
	case ibis::selectClause::SUM:
	    aggr[i] = "SUM(";
	    aggr[i] += nls[i];
	    aggr[i] += ")";
	    break;
	}
	caggr[i] = aggr[i].c_str();
    }
    ibis::table *brd2 = brd1->groupby(caggr);
    delete brd1;
    return brd2;
} // ibis::mensa::doSelect

int64_t ibis::mensa::computeHits(const char* cond, const ibis::partList& pts) {
    if (cond == 0 || *cond == 0)
	return -1;

    int ierr;
    uint64_t nhits = 0;
    ibis::query qq(ibis::util::userName());
    ierr = qq.setWhereClause(cond);
    if (ierr < 0)
	return ierr;

    for (ibis::partList::const_iterator it = pts.begin();
	 it != pts.end(); ++ it) {
	ierr = qq.setPartition(*it);
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

    size_t ierr = 0;
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
	const ibis::part& dp = **it;
	if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
	    LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    const ibis::part& dp = **it;
	    if (ierr + static_cast<int64_t>(dp.nRows()) < ierr) {
		LOGGER(ibis::gVerbose >= 0)
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
	    ierr = (*it)->get1DDistribution
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
	    ierr = (*it)->get1DDistribution(constraints, cname, begin,
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
	    ierr = (*it)->get2DDistribution
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
	    ierr = (*it)->get2DDistribution
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
	    ierr = (*it)->get3DDistribution
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
	    ierr = (*it)->get3DDistribution
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
    : buffer(t.nColumns()), tab(t), curPart(0), pBegin(0), bBegin(0),
      bEnd(0), curRow(-1) {
    if (curPart >= t.parts.size()) return; // no data partition
    if (buffer.empty()) return;

    // linearize the t.naty to buffer, build a mapping between column names
    // and the position in the buffer
    size_t j = 0;
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
	    break;
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

template <typename T>
int ibis::mensa::cursor::getSelected
(const ibis::column &col, const ibis::bitvector &mask,
 ibis::fileManager::storage *&vals) const {
    array_t<T> tmp;
    int ierr;
    if (col.selectValues(mask, tmp) < 0) {
	vals = 0;
	ierr = -1;
    }
    else {
	vals = tmp.getStorage();
	vals->beginUse();
	ierr = 0;
    }
    return ierr;
} // ibis::mensa::cursor::getSelected

/// Fill the buffer for variable number @c i.  On success, return 0,
/// otherwise return a negative value.
/// @note The member variable @c cval in the buffer for a string valued
/// column is not the usual ibis::fileManager::storage object, instead
/// it is simply the pointer to the ibis::column object.  The string
/// variable is retrieved through the column object one at a time using the
/// function @c getString.
int ibis::mensa::cursor::fillBuffer(size_t i) const {
    if (buffer[i].cval != 0) return 0; // already filled
    if (curPart >= tab.parts.size() || tab.parts[curPart] == 0) return -1;
    const ibis::part& apart = *(tab.parts[curPart]);
    // has to do a look up based on the column name, becaus the ith column
    // of the data partition may not be the correct one (some columns may
    // be missing)
    const ibis::column* col = apart.getColumn(buffer[i].cname);
    if (col == 0)
	return -2;

    if (buffer[i].ctype != ibis::CATEGORY && buffer[i].ctype != ibis::TEXT) {
	if (bBegin+apart.nRows() <= bEnd) { // read whole partition
	    buffer[i].cval = col->getRawData();
	    if (buffer[i].cval != 0) {
		buffer[i].cval->beginUse();
		return 0;
	    }
	    else {
		return -1;
	    }
	}
	else { // read part of a partition
	    int ierr;
	    ibis::bitvector mask;
	    if (bBegin > pBegin)
		mask.appendFill(0, static_cast<ibis::bitvector::word_t>
				(bBegin-pBegin));
	    mask.adjustSize(static_cast<ibis::bitvector::word_t>(bEnd-pBegin),
			    static_cast<ibis::bitvector::word_t>
			    (apart.nRows()));
	    switch (buffer[i].ctype) {
	    case ibis::BYTE:
		ierr = getSelected<char>(*col, mask, buffer[i].cval);
		break;
	    case ibis::UBYTE:
		ierr = getSelected<unsigned char>(*col, mask, buffer[i].cval);
		break;
	    case ibis::SHORT:
		ierr = getSelected<int16_t>(*col, mask, buffer[i].cval);
		break;
	    case ibis::USHORT:
		ierr = getSelected<uint16_t>(*col, mask, buffer[i].cval);
		break;
	    case ibis::INT:
		ierr = getSelected<int32_t>(*col, mask, buffer[i].cval);
		break;
	    case ibis::UINT:
		ierr = getSelected<uint32_t>(*col, mask, buffer[i].cval);
		break;
	    case ibis::LONG:
		ierr = getSelected<int64_t>(*col, mask, buffer[i].cval);
		break;
	    case ibis::ULONG:
		ierr = getSelected<uint64_t>(*col, mask, buffer[i].cval);
		break;
	    case ibis::FLOAT:
		ierr = getSelected<float>(*col, mask, buffer[i].cval);
		break;
	    case ibis::DOUBLE:
		ierr = getSelected<double>(*col, mask, buffer[i].cval);
		break;
	    default:
		ierr = -2;
		break;
	    } // switch
	    return ierr;
	}
    }
    else {
	buffer[i].cval = reinterpret_cast<ibis::fileManager::storage*>
	    (const_cast<ibis::column*>(col));
	return 0;
    }
} // ibis::mensa::cursor::fillBuffer

/// Fill the buffers for every column.
int ibis::mensa::cursor::fillBuffers() const {
    int ierr;
    for (size_t i = 0; i < buffer.size(); ++ i) {
	if (buffer[i].cval != 0 && buffer[i].ctype != ibis::CATEGORY &&
	    buffer[i].ctype != ibis::TEXT) {
	    buffer[i].cval->endUse();
	    if (buffer[i].cval->unnamed() && buffer[i].cval->inUse() == 0)
		delete buffer[i].cval;
	    buffer[i].cval = 0;
	}
	ierr = fillBuffer(i);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "ibis::mensa[" << tab.name() << "]::cursor::fillBuffers "
		"failed to fill buffer for column " << i << "("
		<< buffer[i].cname << ", " << ibis::TYPESTRING[buffer[i].ctype]
		<< ") of partition " << tab.parts[curPart]->name()
		<< " with pBegin " << pBegin << ", bBegin " << bBegin
		<< ", and bEnd " << bEnd << ", ierr = " << ierr;
	    return ierr;
	}
    }
    return 0;
} // ibis::mensa::cursor::fillBuffers

void ibis::mensa::cursor::clearBuffers() {
    for (size_t i = 0; i < buffer.size(); ++ i) {
	if (buffer[i].cval != 0 && buffer[i].ctype != ibis::CATEGORY &&
	    buffer[i].ctype != ibis::TEXT) {
	    buffer[i].cval->endUse();
	    if (buffer[i].cval->unnamed() && buffer[i].cval->inUse() == 0)
		delete buffer[i].cval;
	}
	buffer[i].cval = 0;
    }
} // ibis::mensa::cursor::clearBuffers

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
    }
    return 0;
} // ibis::mensa::cursor::fetch

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
	return 0;
    }
    else {
	curRow = pBegin;
	return -1;
    }
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(ibis::table::row& res) {
    int ierr = fetch();
    if (ierr != 0) return ierr;

    if (curRow == bBegin) {
	ierr = fillBuffers();
	if (ierr < 0)
	    return ierr;
    }
    fillRow(res);
    return 0;
} // ibis::mensa::cursor::fetch

int ibis::mensa::cursor::fetch(uint64_t irow, ibis::table::row& res) {
    int ierr = fetch(irow);
    if (ierr != 0) return ierr;

    if (curRow == bBegin) {
	ierr = fillBuffers();
	if (ierr < 0)
	    return ierr;
    }
    fillRow(res);
    return 0;
} // ibis::mensa::cursor::fetch

void ibis::mensa::cursor::fillRow(ibis::table::row& res) const {
    res.clear();
    const size_t il = static_cast<size_t>(curRow - bBegin);
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
		reinterpret_cast<const ibis::category*>(buffer[j].cval)
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
   return values:
   0  -- normal (successful) completion
  -1  -- cursor objection not initialized, call fetch first
  -2  -- unable to load data into memory
  -4  -- error in the output stream
 */
int ibis::mensa::cursor::dump(std::ostream& out, const char* del) const {
    if (tab.nColumns() == 0) return 0;
    if (curRow < 0 || curPart >= tab.parts.size())
	return -1;
    int ierr;
    if (static_cast<uint64_t>(curRow) == bBegin) {
	// first time accessing the data partition
	ierr = fillBuffers();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "ibis::mensa[" << tab.name() << "]::cursor::dump "
		"call to fillBuffers() failed with ierr = " << ierr
		<< " at partition " << tab.parts[curPart]->name()
		<< ", pBegin " << pBegin << ", bBegin " << bBegin
		<< ", bEnd " << bEnd;
	    return -2;
	}
    }

    const size_t i = static_cast<size_t>(curRow - bBegin);
    ierr = dumpIJ(out, i, 0U);
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

    size_t i = static_cast<size_t>(curRow - bBegin);
    // print the first row with error checking
    ierr = dumpIJ(out, i, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (size_t j = 1; j < tab.nColumns(); ++ j) {
	out << del;
	ierr = dumpIJ(out, i, j);
	if (ierr < 0)
	    return -4;
    }
    out << "\n";
    if (! out)
	ierr = -4;
    // print the rest of the rows without error checking
    const size_t nelem = static_cast<size_t>(bEnd - bBegin);
    for (++ i; i < nelem; ++ i) {
	(void) dumpIJ(out, i, 0U);
	for (size_t j = 1; j < buffer.size(); ++ j) {
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
    int ierr;
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
	if (curRow == bBegin) {
	    ierr = fillBuffers();
	    if (ierr < 0) {
		return -2;
	    }
	}
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
    LOGGER(ibis::gVerbose >= 0 && static_cast<uint64_t>(curRow) < tab.nRows())
	<< "\t... " << tab.nRows() - curRow << " remaining in table "
	<< tab.name();
    return ierr;
} // ibis::mensa::cursor::dumpSome

/// Print the ith element in the current block for column j.
/// @note This function does not perform array bounds check.
int ibis::mensa::cursor::dumpIJ(std::ostream& out, size_t i, size_t j) const {
    int ierr = 0;
    switch (buffer[j].ctype) {
    default: {
	ierr = -2;
	break;}
    case ibis::BYTE: {
	if (buffer[j].cval == 0) return 0; // null value
	const signed char *tmp 
	    = reinterpret_cast<const signed char*>(buffer[j].cval->begin());
	out << (int) tmp[i];
	break;}
    case ibis::UBYTE: {
	if (buffer[j].cval == 0) return 0; // null value
	const unsigned char *tmp 
	    = reinterpret_cast<const unsigned char*>(buffer[j].cval->begin());
	out << (unsigned int) tmp[i];
	break;}
    case ibis::SHORT: {
	if (buffer[j].cval == 0) return 0; // null value
	const int16_t *tmp 
	    = reinterpret_cast<const int16_t*>(buffer[j].cval->begin());
	out << tmp[i];
	break;}
    case ibis::USHORT: {
	if (buffer[j].cval == 0) return 0; // null value
	const uint16_t *tmp 
	    = reinterpret_cast<const uint16_t*>(buffer[j].cval->begin());
	out << tmp[i];
	break;}
    case ibis::INT: {
	if (buffer[j].cval == 0) return 0; // null value
	const int32_t *tmp 
	    = reinterpret_cast<const int32_t*>(buffer[j].cval->begin());
	out << tmp[i];
	break;}
    case ibis::UINT: {
	if (buffer[j].cval == 0) return 0; // null value
	const uint32_t *tmp 
	    = reinterpret_cast<const uint32_t*>(buffer[j].cval->begin());
	out << tmp[i];
	break;}
    case ibis::LONG: {
	if (buffer[j].cval == 0) return 0; // null value
	const int64_t *tmp 
	    = reinterpret_cast<const int64_t*>(buffer[j].cval->begin());
	out << tmp[i];
	break;}
    case ibis::ULONG: {
	if (buffer[j].cval == 0) return 0; // null value
	const uint64_t *tmp 
	    = reinterpret_cast<const uint64_t*>(buffer[j].cval->begin());
	out << tmp[i];
	break;}
    case ibis::FLOAT: {
	if (buffer[j].cval == 0) return 0; // null value
	const float *tmp 
	    = reinterpret_cast<const float*>(buffer[j].cval->begin());
	out << std::setprecision(8) << tmp[i];
	break;}
    case ibis::DOUBLE: {
	if (buffer[j].cval == 0) return 0; // null value
	const double *tmp 
	    = reinterpret_cast<const double*>(buffer[j].cval->begin());
	out << std::setprecision(18) << tmp[i];
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
	    }
	    if (col != 0) {
		std::string val;
		static_cast<const ibis::text*>(col)
		    ->ibis::text::getString
		    (static_cast<uint32_t>(i+bBegin-pBegin), val);
		out << '"' << val << '"';
	    }
	}
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::dumpIJ

int ibis::mensa::cursor::getColumnAsByte(size_t j, char* val) const {
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
	*val = *(buffer[j].cval->begin() + (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsByte

int ibis::mensa::cursor::getColumnAsUByte(size_t j, unsigned char* val) const {
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
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUByte

int ibis::mensa::cursor::getColumnAsShort(size_t j, int16_t* val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>
		 (buffer[j].cval->begin()) + (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsShort

int ibis::mensa::cursor::getColumnAsUShort(size_t j, uint16_t* val) const {
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
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUShort

int ibis::mensa::cursor::getColumnAsInt(size_t j, int32_t* val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsInt

int ibis::mensa::cursor::getColumnAsUInt(size_t j, uint32_t* val) const {
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
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsUInt

int ibis::mensa::cursor::getColumnAsLong(size_t j, int64_t* val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::INT: {
	*val = *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	*val = *(reinterpret_cast<int64_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsLong

int ibis::mensa::cursor::getColumnAsULong(size_t j, uint64_t* val) const {
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
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	*val = *(reinterpret_cast<uint64_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsULong

int ibis::mensa::cursor::getColumnAsFloat(size_t j, float* val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	*val = *(reinterpret_cast<float*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsFloat

int ibis::mensa::cursor::getColumnAsDouble(size_t j, double* val) const {
    if (curRow < 0 || curPart >= tab.parts.size() || j > tab.nColumns())
	return -1;
    int ierr = 0;
    if (static_cast<uint64_t>(curRow) == bBegin)
	ierr = fillBuffer(j);
    if (ierr < 0 || buffer[j].cval == 0)
	return -2;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = *(buffer[j].cval->begin() + (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = *(reinterpret_cast<unsigned char*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = *(reinterpret_cast<int16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = *(reinterpret_cast<uint16_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::INT: {
	*val = *(reinterpret_cast<int32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::UINT: {
	*val = *(reinterpret_cast<uint32_t*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	*val = *(reinterpret_cast<double*>(buffer[j].cval->begin()) +
		 (curRow - bBegin));
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::mensa::cursor::getColumnAsDouble

int ibis::mensa::cursor::getColumnAsString(size_t j, std::string& val) const {
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
	    tab.parts[curPart]->getColumn(buffer[j].cname);
	if (col != 0) {
	    col->getString(static_cast<uint32_t>(curRow-pBegin), val);
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

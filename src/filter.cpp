// File $Id$	
// author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2010 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tab.h"	// ibis::tabula
#include "bord.h"	// ibis::bord
#include "mensa.h"	// ibis::mensa
#include "countQuery.h"	// ibis::countQuery
#include "filter.h"	// ibis::filter

#include <algorithm>	// std::sort
#include <sstream>	// std::ostringstream
#include <limits>	// std::numeric_limits

/// The user supplies all three clauses of a SQL select statement.  The
/// objects are copied if they are not empty.
///
/// @note This constructor makes a copy of the container for the data
/// partitions, but not the data partitioins themselves.  In the
/// destructor, only the container is freed, not the data partitions.
ibis::filter::filter(const ibis::whereClause &w, const ibis::partList &p,
		     const ibis::selectClause &s)
    : wc_(w.empty() ? 0 : new whereClause(w)),
      parts_(p.empty() ? 0 : new partList(p)),
      aliases_(s.empty() ? 0 : new selectClause(s)) {
} // constructor

void ibis::filter::roughCount(uint64_t& nmin, uint64_t& nmax) const {
    const ibis::partList &myparts = (parts_ != 0 ? *parts_ : ibis::datasets);
    nmin = 0;
    nmax = 0;
    if (wc_ == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "ibis::filter::roughCount assumes all rows are hits because no "
	    "query condition is specified";
	for (ibis::partList::const_iterator it = myparts.begin();
	     it != myparts.end(); ++ it)
	    nmax += (*it)->nRows();
	nmin = nmax;
	return;
    }

    ibis::countQuery qq;
    int ierr = qq.setWhereClause(wc_->getExpr());
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::filter::roughCount failed to assign the "
	    "where clause, assume all rows may be hits";
	for (ibis::partList::const_iterator it = myparts.begin();
	     it != myparts.end(); ++ it)
	    nmax += (*it)->nRows();
	return;
    }
    if (aliases_ != 0) {
	ierr = qq.setSelectClause(aliases_);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::filter::roughCount failed to assign the "
		"select clause, assume all rows may be hits";
	    for (ibis::partList::const_iterator it = myparts.begin();
		 it != myparts.end(); ++ it)
		nmax += (*it)->nRows();
	    return;
	}
    }

    for (ibis::partList::const_iterator it = myparts.begin();
	 it != myparts.end(); ++ it) {
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
    }
} // ibis::filter::roughCount

int64_t ibis::filter::count() const {
    int64_t nhits = 0;
    const ibis::partList &myparts = (parts_ != 0 ? *parts_ : ibis::datasets);
    nhits = 0;
    if (wc_ == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "ibis::filter::count assumes all rows are hits because no "
	    "query condition is specified";
	for (ibis::partList::const_iterator it = myparts.begin();
	     it != myparts.end(); ++ it)
	    nhits += (*it)->nRows();
	return nhits;
    }

    ibis::countQuery qq;
    int ierr = qq.setWhereClause(wc_->getExpr());
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::filter::count failed to assign the "
	    "where clause";
	nhits = ierr;
	return nhits;
    }
    if (aliases_ != 0) {
	ierr = qq.setSelectClause(aliases_);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::filter::count failed to assign the "
		"select clause";
	    nhits = ierr;
	    return nhits;
	}
    }

    for (ibis::partList::const_iterator it = myparts.begin();
	 it != myparts.end(); ++ it) {
	ierr = qq.setPartition(*it);
	if (ierr >= 0) {
	    ierr = qq.evaluate();
	    if (ierr >= 0) {
		nhits += qq.getNumHits();
	    }
	    else {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- ibis::filter::count failed to evaluate "
		    << qq.getWhereClause() << " on " << (*it)->name()
		    << ", ierr = " << ierr;
	    }
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::filter::count failed to assign "
		<< qq.getWhereClause() << " on " << (*it)->name()
		<< ", ierr = " << ierr;
	}
    }
    return nhits;
} // ibis::filter::count

ibis::table* ibis::filter::select(const char* sel) const {
    const ibis::partList &myparts = (parts_ != 0 ? *parts_ : ibis::datasets);
    if (wc_ == 0) {
	// empty where clause, SQL standard dictates it to select everyont
	return new ibis::liga(myparts);
    }
    if (sel == 0 || *sel == 0) {
	if (aliases_ != 0) {
	    return ibis::filter::filt(*aliases_, myparts, *wc_);
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- ibis::filter::select can not proceed without "
		"a select clause";
	    return 0;
	}
    }

    ibis::selectClause sc(sel);
    if (sc.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- ibis::filter::select parsed \"" << sel
	    << "\" into an empty select clause";
	return 0;
    }
    return ibis::filter::filt(sc, myparts, *wc_);
} // ibis::filter::select

/// Select the rows satisfying the where clause and store the results
/// in a table object.  It concatenates the results from different data
/// partitions in the order of the data partitions given in mylist.
///
/// It expects all three arguments to be valid and non-trivial.  It will
/// return a nil pointer if those arguments are nil pointers or empty.
ibis::table* ibis::filter::filt(const ibis::selectClause &tms,
				const ibis::partList &mylist,
				const ibis::whereClause &cond) {
    if (tms.empty() || mylist.empty() || cond.empty())
	return 0;

    std::string mesg = "filter::filt";
    if (ibis::gVerbose > 0) {
	std::ostringstream oss;
	oss << "(select " << tms << " from " << mylist.size()
	    << " data partition" << (mylist.size() > 1 ? "s" : "")
	    << " where " << cond << ')';
	mesg += oss.str();
    }

    ibis::util::timer atimer(mesg.c_str(), 2);
    long int ierr;
    // a single query object is used for different data partitions
    ibis::countQuery qq;
    ierr = qq.setWhereClause(cond.getExpr());
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " failed to assign externally "
	    "provided query expression \"" << cond
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
	else if (*(tms.argName(0)) != '*') { // only one unique name
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
    for (ibis::partList::const_iterator it = mylist.begin();
	 it != mylist.end(); ++ it) {
	if (tmstouse.size() >= tms.size())
	    ierr = tms.verify(**it);
	else
	    ierr = tms.verifySome(**it, tmstouse);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- select clause (" << tms
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

		    ibis::array_t<double> tmp;
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
		ibis::array_t<char>* tmp = col->selectBytes(*hits);
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
		ibis::array_t<int16_t>* tmp = col->selectShorts(*hits);
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
		ibis::array_t<int32_t>* tmp = col->selectInts(*hits);
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
		ibis::array_t<int64_t>* tmp = col->selectLongs(*hits);
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
		ibis::array_t<float>* tmp = col->selectFloats(*hits);
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
		ibis::array_t<double>* tmp = col->selectDoubles(*hits);
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

    std::string tn = ibis::util::shortName(mesg);
    if (nh == 0) { // return an empty table of type tabula
	return new ibis::tabula(tn.c_str(), mesg.c_str(), nh);
    }
    else if (tmstouse.empty()) { // count(*)
	return new ibis::tabele(tn.c_str(), mesg.c_str(), nh, tms.termName(0));
    }

    // convert the selection into a in-memory data partition
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
	new ibis::bord(tn.c_str(), mesg.c_str(), nh, buff, tls, nlsptr, &cdesc);
    if (nplain >= tms.size() || brd1 == 0)
	return brd1;

    ibis::table *brd2 = brd1->groupby(tms);
    delete brd1;
    return brd2;
} // ibis::filter::filt

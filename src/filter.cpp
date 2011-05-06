// File $Id$	
// author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2011 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tab.h"	// ibis::tabula
#include "bord.h"	// ibis::bord
#include "mensa.h"	// ibis::mensa
#include "countQuery.h"	// ibis::countQuery
#include "filter.h"	// ibis::filter

#include <memory>	// std::auto_ptr
#include <algorithm>	// std::sort
#include <sstream>	// std::ostringstream
#include <limits>	// std::numeric_limits

/// The incoming where clause is applied to all known data partitions in
/// ibis::datasets.
ibis::filter::filter(const ibis::whereClause* w)
    : wc_(w != 0 && w->empty() == false ? new whereClause(*w) : 0),
      parts_(0), sel_(0) {
} // constructor

/// The user supplies all three clauses of a SQL select statement.  The
/// objects are copied if they are not empty.
///
/// @note This constructor makes a copy of the container for the data
/// partitions, but not the data partitions themselves.  In the
/// destructor, only the container is freed, not the data partitions.
ibis::filter::filter(const ibis::selectClause* s, const ibis::partList* p,
		     const ibis::whereClause* w)
    : wc_(w == 0 || w->empty() ? 0 : new whereClause(*w)),
      parts_(p == 0 || p->empty() ? 0 : new partList(*p)),
      sel_(s == 0 || s->empty() ? 0 : new selectClause(*s)) {
} // constructor

ibis::filter::~filter() {
    delete sel_;
    delete parts_;
    delete wc_;
} // ibis::filter::~filter

void ibis::filter::roughCount(uint64_t& nmin, uint64_t& nmax) const {
    const ibis::partList &myparts = (parts_ != 0 ? *parts_ : ibis::datasets);
    nmin = 0;
    nmax = 0;
    if (wc_ == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "filter::roughCount assumes all rows are hits because no "
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
	    << "Warning -- filter::roughCount failed to assign the "
	    "where clause, assume all rows may be hits";
	for (ibis::partList::const_iterator it = myparts.begin();
	     it != myparts.end(); ++ it)
	    nmax += (*it)->nRows();
	return;
    }
    if (sel_ != 0) {
	ierr = qq.setSelectClause(sel_);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- filter::roughCount failed to assign the "
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
	    << "filter::count assumes all rows are hits because no "
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
	    << "Warning -- filter::count failed to assign the "
	    "where clause";
	nhits = ierr;
	return nhits;
    }
    if (sel_ != 0) {
	ierr = qq.setSelectClause(sel_);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- filter::count failed to assign the "
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
		    << "Warning -- filter::count failed to evaluate "
		    << qq.getWhereClause() << " on " << (*it)->name()
		    << ", ierr = " << ierr;
	    }
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- filter::count failed to assign "
		<< qq.getWhereClause() << " on " << (*it)->name()
		<< ", ierr = " << ierr;
	}
    }
    return nhits;
} // ibis::filter::count

ibis::table* ibis::filter::select() const {
    const ibis::partList &myparts = (parts_ != 0 ? *parts_ : ibis::datasets);
    if (wc_ == 0) {
	// empty where clause, SQL standard dictates it to select every row
	return new ibis::liga(myparts);
    }
    if (sel_ == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- filter::select can not proceed without "
	    "a select clause";
	return 0;
    }
    try {
	return ibis::filter::filt(*sel_, myparts, *wc_);
    }
    catch (const ibis::bad_alloc &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed a bad_alloc exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const std::exception &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed a std::exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const char *s) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed a string exception ("
		 << s << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (...) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed an unknown exception, "
		"will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    return 0;
} // ibis::filter::select

ibis::table*
ibis::filter::select(const ibis::table::stringList& colnames) const {
    ibis::selectClause sc(colnames);
    if (sc.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- filter::select can not proceed with an empty "
	    "select clause";
	return 0;
    }
    try {
	return ibis::filter::filt(sc, *parts_, *wc_);
    }
    catch (const ibis::bad_alloc &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed a bad_alloc exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const std::exception &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed a std::exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const char *s) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed a string exception ("
		 << s << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (...) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- filter::select absorbed an unknown exception, "
		"will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    return 0;
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
    ibis::table::bufferList  buff;
    std::vector<uint32_t>    tmstouse;
    uint32_t                 nplain = 0;
    if (tms.size() > 0) { // sort the names of variables to compute
	std::set<const char*, ibis::lessi> uniquenames;
	for (uint32_t i = 0; i < tms.size(); ++ i) {
	    if (tms.getAggregator(i) != ibis::selectClause::CNT ||
		strcmp(tms.argName(i), "*") != 0) {// not count(*)
		const char* tname = tms.argName(i);
		nplain += (tms.getAggregator(i) == ibis::selectClause::NIL);
		if (uniquenames.find(tname) == uniquenames.end()) {
		    uniquenames.insert(tname);
		    tmstouse.push_back(i);
		    nls.push_back(tname);
		}
	    }
	}
	if (! uniquenames.empty()) {
	    tls.resize(nls.size());
	    buff.resize(nls.size());
	    for (uint32_t i = 0; i < nls.size(); ++ i) {
		tls[i] = ibis::UNKNOWN_TYPE;
		buff[i] = 0;
	    }
#if _DEBUG+0>1 || DEBUG+0>0
	    if (ibis::gVerbose > 5) {
		ibis::util::logger lg;
		lg() << "DEBUG: " << mesg << " uniquenames["
			    << uniquenames.size() << "]=";
		for (std::set<const char*, ibis::lessi>::const_iterator it =
			 uniquenames.begin(); it != uniquenames.end(); ++ it) {
		    lg() << (it != uniquenames.begin() ? ", " : "")
				<< *it;
		}
	    }
#endif
	}
    }
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg() << mesg << " -- processing a select clause with " << tms.size()
	     << " term" << (tms.size()>1?"s":"") << ", " << nplain
	     << " of which " << (nplain>1?"are":"is") << " plain";
	if (ibis::gVerbose > 3)
	    lg() << " (buff[" << buff.size() << "] and tls[" << tls.size()
		 << ")";
    }

    // create the guard after buff and tls have got their correct sizes
    ibis::util::guard gbuff
	= ibis::util::makeGuard(ibis::table::freeBuffers,
				ibis::util::ref(buff),
				ibis::util::ref(tls));
    uint32_t nh = 0;
    // main loop through each data partition, fill the initial selection
    for (ibis::partList::const_iterator it = mylist.begin();
	 it != mylist.end(); ++ it) {
	LOGGER(ibis::gVerbose > 2)
	    << mesg << " -- processing query conditions \"" << cond
	    << "\" on data partition " << (*it)->name();
	if (tmstouse.size() >= tms.size())
	    ierr = tms.verify(**it);
	else
	    ierr = tms.verifySome(tmstouse, **it);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- select clause (" << tms
		<< ") contains variables that are not in data partition "
		<< (*it)->name();
	    ierr = -11;
	    continue;
	}
	ierr = qq.setSelectClause(&tms);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- failed to modify the select clause of "
		<< "the countQuery object (" << qq.getWhereClause()
		<< ") on data partition " << (*it)->name();
	    ierr = -12;
	    continue;
	}

	ierr = qq.setPartition(*it);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- query.setPartition(" << (*it)->name()
		<< ") failed with error code " << ierr;
	    ierr = -13;
	    continue;
	}

	ierr = qq.evaluate();
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- failed to process query on data partition "
		<< (*it)->name();
	    ierr = -14;
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
		    ierr = -15;
		}
		else {
		    if (tls[i] == ibis::UNKNOWN_TYPE)
			tls[i] = ibis::DOUBLE;

		    ibis::array_t<double> tmp;
		    ierr = (*it)->calculate(*aterm, *hits, tmp);
		    ibis::util::addIncoreData
			(buff[i], tmp, nh, FASTBIT_DOUBLE_NULL);
		}
		continue;
	    }

	    const ibis::column* col = (*it)->getColumn(tms.argName(itm));
	    if (col == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << mesg << " -- \"" << tms.argName(itm)
		    << "\" is not a column of partition " << (*it)->name();
		ierr = -16;
		continue;
	    }

	    if (tls[i] == ibis::UNKNOWN_TYPE)
		tls[i] = col->type();
	    LOGGER(ibis::gVerbose > 3)
		<< mesg << " -- adding " << nqq << " element"
		<< (nqq>1?"s":"") << " from column " << col->name()
		<< " of partition " << (*it)->name() << ", nh = " << nh;
	    switch (col->type()) {
	    case ibis::BYTE:
	    case ibis::UBYTE: {
		std::auto_ptr< ibis::array_t<signed char> >
		    tmp(col->selectBytes(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh, static_cast<signed char>(0x7F));
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::SHORT:
	    case ibis::USHORT: {
		std::auto_ptr< ibis::array_t<int16_t> >
		    tmp(col->selectShorts(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData(buff[i], *tmp, nh,
						  static_cast<int16_t>(0x7FFF));
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::UINT:
	    case ibis::INT: {
		std::auto_ptr< ibis::array_t<int32_t> >
		    tmp(col->selectInts(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh,
			     static_cast<int32_t>(0x7FFFFFFF));
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::LONG:
	    case ibis::ULONG: {
		std::auto_ptr<ibis::array_t<int64_t> >
		    tmp(col->selectLongs(*hits));
		if (tmp.get() != 0) {
		    if (nh != 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh, static_cast<int64_t>
			     (0x7FFFFFFFFFFFFFFFLL));
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::FLOAT: {
		std::auto_ptr< ibis::array_t<float> >
		    tmp(col->selectFloats(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh, FASTBIT_FLOAT_NULL);
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::DOUBLE: {
		std::auto_ptr< ibis::array_t<double> >
		    tmp(col->selectDoubles(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addIncoreData
			    (buff[i], *tmp, nh, FASTBIT_DOUBLE_NULL);
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::TEXT: {
		std::auto_ptr< std::vector<std::string> >
		    tmp(col->selectStrings(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addStrings(buff[i], *tmp, nh);
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    case ibis::CATEGORY: {
		std::auto_ptr< std::vector<std::string> >
		    tmp(col->selectStrings(*hits));
		if (tmp.get() != 0) {
		    if (nh > 0) {
			ibis::util::addStrings(buff[i], *tmp, nh);
		    }
		    else {
			buff[i] = tmp.release();
		    }
		}
		break;}
	    default: {
		LOGGER(ibis::gVerbose > 1)
		    << mesg << " -- unable to process column " << tms[itm]
		    << " (type " << ibis::TYPESTRING[(int)tls[i]] << ")";
		ierr = -17;
		break;}
	    }
	}
	nh += nqq;
    }

    std::string tn = ibis::util::shortName(mesg);
    if (nh == 0) {
	if (ierr >= 0) { // return an empty table of type tabula
	    return new ibis::tabula(tn.c_str(), mesg.c_str(), nh);
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << mesg << " failed to produce any result "
		"due to error, last error code was " << ierr;
	    return 0;
	}
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
	// if nplain >= tms.size(), then use the external names, otherwise
	// use the internal names
	nlsptr[i] = (nplain >= tms.size() ? desc[i].c_str() : nls[i].c_str());
    }

    std::auto_ptr<ibis::bord> brd1
	(new ibis::bord(tn.c_str(), mesg.c_str(), nh, buff, tls, nlsptr,
			&cdesc));
    // need to dismiss the guard after buff has been transfered to the new
    // table object
    if (brd1.get() != 0)
	gbuff.dismiss();
    if (nplain >= tms.size() || brd1.get() == 0)
	return brd1.release();

    std::auto_ptr<ibis::table> brd2(brd1->groupby(tms));
    return brd2.release();
} // ibis::filter::filt

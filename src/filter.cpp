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
ibis::filter::filter(const ibis::selectClause* s, const ibis::constPartList* p,
		     const ibis::whereClause* w)
    : wc_(w == 0 || w->empty() ? 0 : new whereClause(*w)),
      parts_(p == 0 || p->empty() ? 0 : new constPartList(*p)),
      sel_(s == 0 || s->empty() ? 0 : new selectClause(*s)) {
} // constructor

ibis::filter::~filter() {
    delete sel_;
    delete parts_;
    delete wc_;
} // ibis::filter::~filter

void ibis::filter::roughCount(uint64_t& nmin, uint64_t& nmax) const {
    const ibis::constPartList &myparts =
	(parts_ != 0 ? *parts_ :
	 reinterpret_cast<const constPartList&>(ibis::datasets));
    nmin = 0;
    nmax = 0;
    if (wc_ == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "filter::roughCount assumes all rows are hits because no "
	    "query condition is specified";
	for (ibis::constPartList::const_iterator it = myparts.begin();
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
	for (ibis::constPartList::const_iterator it = myparts.begin();
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
	    for (ibis::constPartList::const_iterator it = myparts.begin();
		 it != myparts.end(); ++ it)
		nmax += (*it)->nRows();
	    return;
	}
    }

    for (ibis::constPartList::const_iterator it = myparts.begin();
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
    const ibis::constPartList &myparts =
	(parts_ != 0 ? *parts_ :
	 reinterpret_cast<const constPartList&>(ibis::datasets));
    nhits = 0;
    if (wc_ == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "filter::count assumes all rows are hits because no "
	    "query condition is specified";
	for (ibis::constPartList::const_iterator it = myparts.begin();
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

    for (ibis::constPartList::const_iterator it = myparts.begin();
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
    const ibis::constPartList &myparts =
	(parts_ != 0 ? *parts_ :
	 reinterpret_cast<const ibis::constPartList&>(ibis::datasets));
    if (sel_ == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- filter::select can not proceed without "
	    "a select clause";
	return 0;
    }
    try {
	if (wc_ == 0 || wc_->empty())
	    return ibis::filter::filt0(*sel_, myparts);

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
    const ibis::constPartList &myparts =
	(parts_ ? *parts_ :
	 reinterpret_cast<const constPartList&>(ibis::datasets));
    ibis::selectClause sc(colnames);
    if (sc.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- filter::select can not proceed with an empty "
	    "select clause";
	return 0;
    }
    try {
	if (wc_ == 0 || wc_->empty())
	    return ibis::filter::filt0(sc, myparts);

	if (sc.aggSize() == 1 &&
	    wc_->getExpr()->getType() == ibis::qExpr::RANGE &&
	    0 == stricmp(sc.aggName(0),
			 static_cast<const ibis::qContinuousRange*>
			 (wc_->getExpr())->colName()))
	    return ibis::filter::filt1(sc, myparts, *wc_);

	return ibis::filter::filt(sc, myparts, *wc_);
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
				const ibis::constPartList &plist,
				const ibis::whereClause &cond) {
    if (plist.empty())
	return new ibis::tabula();
    if (tms.empty())
	return new
	    ibis::tabula(ibis::table::computeHits(plist, cond.getExpr()));
    if (cond.empty())
	return filt0(tms, plist);
    if (tms.aggSize() == 1 &&
	cond->getType() == ibis::qExpr::RANGE &&
	0 == stricmp(tms.aggName(0),
		     static_cast<const ibis::qContinuousRange*>
		     (cond.getExpr())->colName()))
	return ibis::filter::filt1(tms, plist, cond);

    std::string mesg = "filter::filt";
    if (ibis::gVerbose > 1) {
	mesg += "(SELECT ";
	std::ostringstream oss;
	oss << tms;
	if (oss.str().size() <= 20) {
	    mesg += oss.str();
	}
	else {
	    for (unsigned j = 0; j < 20; ++ j)
		mesg += oss.str()[j];
	    mesg += " ...";
	}
	oss.clear();
	oss.str("");
	oss << " FROM " << plist.size() << " data partition"
	    << (plist.size() > 1 ? "s" : "")
	    << " WHERE " << cond;
	if (oss.str().size() <= 35) {
	    mesg += oss.str();
	}
	else {
	    for (unsigned j = 0; j < 35; ++ j)
		mesg += oss.str()[j];
	    mesg += " ...";
	}
	mesg += ')';
    }

    long int ierr = 0;
    ibis::util::timer atimer(mesg.c_str(), 2);
    // a single query object is used for different data partitions
    ibis::countQuery qq;
    ierr = qq.setWhereClause(cond.getExpr());
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " failed to assign externally "
	    "provided query expression \"" << cond
	    << "\" to a countQuery object, ierr=" << ierr;
	return 0;
    }

    std::string tn = ibis::util::shortName(mesg);
    std::auto_ptr<ibis::bord> brd1
	(new ibis::bord(tn.c_str(), mesg.c_str(), tms, *(plist.front())));
    const uint32_t nplain = tms.numGroupbyKeys();
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg() << mesg << " -- processing a select clause with " << tms.aggSize()
	     << " term" << (tms.aggSize()>1?"s":"") << ", " << nplain
	     << " of which " << (nplain>1?"are":"is") << " plain";
	if (ibis::gVerbose > 4) {
	    lg() << "\nTemporary data will be stored in the following:\n";
	    brd1->describe(lg());
	}
    }

    // main loop through each data partition, fill the initial selection
    for (ibis::constPartList::const_iterator it = plist.begin();
	 it != plist.end(); ++ it) {
	LOGGER(ibis::gVerbose > 2)
	    << mesg << " -- processing query conditions \"" << cond
	    << "\" on data partition " << (*it)->name();
	ierr = tms.verify(**it);
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
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- query.setPartition(" << (*it)->name()
		<< ") failed with error code " << ierr;
	    ierr = -13;
	    continue;
	}

	ierr = qq.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- failed to process query on data partition "
		<< (*it)->name();
	    ierr = -14;
	    continue;
	}

	const ibis::bitvector* hits = qq.getHitVector();
	if (hits == 0 || hits->cnt() == 0) continue;

	ierr = brd1->append(tms, **it, *hits);
	LOGGER(ierr < 0 && ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " failed to append " << hits->cnt()
	    << " row" << (hits->cnt() > 1 ? "s" : "") << " from "
	    << (*it)->name();
    }

    if (brd1.get() == 0) return 0;
    if (ibis::gVerbose > 2 && brd1.get() != 0) {
	ibis::util::logger lg;
	lg() << mesg << " -- creates an in-memory data partition with "
	     << brd1->nRows() << " row" << (brd1->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd1->describe(lg());
	}
    }
    if (brd1->nRows() == 0) {
	if (ierr >= 0) { // return an empty table of type tabula
	    return new ibis::tabula(tn.c_str(), mesg.c_str(), 0);
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << mesg << " failed to produce any result, "
		"the last error code was " << ierr;
	    return 0;
	}
    }
    else if (brd1->nColumns() == 0) { // count(*)
	return new ibis::tabele(tn.c_str(), mesg.c_str(), brd1->nRows(),
				tms.termName(0));
    }

    if (nplain >= tms.aggSize()) {
	brd1->renameColumns(tms);
	return brd1.release();
    }

    std::auto_ptr<ibis::table> brd2(brd1->groupby(tms));
    if (ibis::gVerbose > 2 && brd2.get() != 0) {
	ibis::util::logger lg;
	lg() << mesg << " -- produces an in-memory data partition with "
	     << brd2->nRows() << " row" << (brd2->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd2->describe(lg());
	}
    }
    return brd2.release();
} // ibis::filter::filt

/// Select all rows from each data partition and place them in a table
/// object.  It concatenates the results from different data partitions in
/// the order of the data partitions given in mylist.
///
/// It expects both incoming arguments to be valid and non-trivial.  It
/// will return a nil pointer if those arguments are nil pointers or empty.
ibis::table* ibis::filter::filt0(const ibis::selectClause &tms,
				 const ibis::constPartList &plist) {
    long int ierr = 0;
    if (tms.empty() || plist.empty())
	return 0;

    std::string mesg = "filter::filt0";
    if (ibis::gVerbose > 1) {
	mesg += "(SELECT ";
	std::ostringstream oss;
	oss << tms;
	if (oss.str().size() <= 20) {
	    mesg += oss.str();
	}
	else {
	    for (unsigned j = 0; j < 20; ++ j)
		mesg += oss.str()[j];
	    mesg += " ...";
	}
	oss.clear();
	oss.str("");
	oss << " FROM " << plist.size() << " data partition"
	    << (plist.size() > 1 ? "s" : "");
	if (oss.str().size() <= 35) {
	    mesg += oss.str();
	}
	else {
	    for (unsigned j = 0; j < 35; ++ j)
		mesg += oss.str()[j];
	    mesg += " ...";
	}
	mesg += ')';
    }

    ibis::util::timer atimer(mesg.c_str(), 2);
    std::string tn = ibis::util::shortName(mesg);
    std::auto_ptr<ibis::bord> brd1
	(new ibis::bord(tn.c_str(), mesg.c_str(), tms, *(plist.front())));
    const uint32_t nplain = tms.numGroupbyKeys();
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg() << mesg << " -- processing a select clause with " << tms.aggSize()
	     << " term" << (tms.aggSize()>1?"s":"") << ", " << nplain
	     << " of which " << (nplain>1?"are":"is") << " plain";
	if (ibis::gVerbose > 4) {
	    lg() << "\nTemporary data will be stored in the following:\n";
	    brd1->describe(lg());
	}
    }

    // main loop through each data partition, fill the initial selection
    for (ibis::constPartList::const_iterator it = plist.begin();
	 it != plist.end(); ++ it) {
	LOGGER(ibis::gVerbose > 2)
	    << mesg << " -- processing data partition " << (*it)->name();
	ierr = tms.verify(**it);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- select clause (" << tms
		<< ") contains variables that are not in data partition "
		<< (*it)->name();
	    ierr = -11;
	    continue;
	}

	const ibis::bitvector& msk = (*it)->getNullMask();
	ierr = brd1->append(tms, **it, msk);
	LOGGER(ierr < 0 && ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " failed to append " << msk.cnt()
	    << " row" << (msk.cnt() > 1 ? "s" : "") << " from "
	    << (*it)->name();
    }

    if (brd1.get() == 0) return 0;
    if (ibis::gVerbose > 2 && brd1.get() != 0) {
	ibis::util::logger lg;
	lg() << mesg << " -- creates an in-memory data partition with "
	     << brd1->nRows() << " row" << (brd1->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd1->describe(lg());
	}
    }
    if (brd1->nRows() == 0) {
	if (ierr >= 0) { // return an empty table of type tabula
	    return new ibis::tabula(tn.c_str(), mesg.c_str(), 0);
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << mesg << " failed to produce any result, "
		"the last error code was " << ierr;
	    return 0;
	}
    }
    else if (brd1->nColumns() == 0) { // count(*)
	return new ibis::tabele(tn.c_str(), mesg.c_str(), brd1->nRows(),
				tms.termName(0));
    }

    if (nplain >= tms.aggSize()) {
	brd1->renameColumns(tms);
	return brd1.release();
    }

    std::auto_ptr<ibis::table> brd2(brd1->groupby(tms));
    if (ibis::gVerbose > 2 && brd2.get() != 0) {
	ibis::util::logger lg;
	lg() << mesg << " -- produces an in-memory data partition with "
	     << brd2->nRows() << " row" << (brd2->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd2->describe(lg());
	}
    }
    return brd2.release();
} // ibis::filter::filt0

/// Select the rows satisfying the where clause and store the results
/// in a table object.  It concatenates the results from different data
/// partitions in the order of the data partitions given in mylist.
///
/// This version is intended to work with only one column of raw data in
/// the select clause and one term in the where clause, the where clause
/// must be a simple range expression, and the column involved in these
/// clauses are expected to be the same.  If any of these conditions is not
/// satisfied, it returns a nil pointer.
ibis::table* ibis::filter::filt1(const ibis::selectClause &tms,
				 const ibis::constPartList &plist,
				 const ibis::whereClause &cond) {
    long int ierr = 0;
    if (tms.aggSize() != 1 || plist.empty() ||
	cond->getType() != ibis::qExpr::RANGE)
	return 0;
    if (0 != stricmp(tms.aggName(0),
		     static_cast<const ibis::qContinuousRange*>
		     (cond.getExpr())->colName()))
	return 0;

    std::string mesg = "filter::filt1";
    if (ibis::gVerbose > 1) {
	mesg += "(SELECT ";
	std::ostringstream oss;
	oss << tms;
	if (oss.str().size() <= 20) {
	    mesg += oss.str();
	}
	else {
	    for (unsigned j = 0; j < 20; ++ j)
		mesg += oss.str()[j];
	    mesg += " ...";
	}
	oss.clear();
	oss.str("");
	oss << " FROM " << plist.size() << " data partition"
	    << (plist.size() > 1 ? "s" : "")
	    << " WHERE " << cond;
	if (oss.str().size() <= 35) {
	    mesg += oss.str();
	}
	else {
	    for (unsigned j = 0; j < 30; ++ j)
		mesg += oss.str()[j];
	    mesg += " ...";
	}
	mesg += ')';
    }

    ibis::util::timer atimer(mesg.c_str(), 2);
    std::string tn = ibis::util::shortName(mesg);
    std::auto_ptr<ibis::bord> brd1
	(new ibis::bord(tn.c_str(), mesg.c_str(), tms, *(plist.front())));
    const uint32_t nplain = tms.numGroupbyKeys();
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg() << mesg << " -- processing a select clause with " << tms.aggSize()
	     << " term" << (tms.aggSize()>1?"s":"") << ", " << nplain
	     << " of which " << (nplain>1?"are":"is") << " plain";
	if (ibis::gVerbose > 4) {
	    lg() << "\nTemporary data will be stored in the following:\n";
	    brd1->describe(lg());
	}
    }

    // main loop through each data partition, fill the initial selection
    for (ibis::constPartList::const_iterator it = plist.begin();
	 it != plist.end(); ++ it) {
	LOGGER(ibis::gVerbose > 2)
	    << mesg << " -- processing query conditions \"" << cond
	    << "\" on data partition " << (*it)->name();
	ierr = tms.verify(**it);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< mesg << " -- select clause (" << tms
		<< ") contains variables that are not in data partition "
		<< (*it)->name();
	    ierr = -11;
	    continue;
	}
	ierr = brd1->append(tms, **it,
			    *static_cast<const ibis::qContinuousRange*>
			    (cond.getExpr()));
	LOGGER(ierr < 0 && ibis::gVerbose > 0)
	    << "Warning -- " << mesg << " failed to append rows satisfying "
	    << cond << " from " << (*it)->name();
    }

    if (brd1.get() == 0) return 0;
    if (ibis::gVerbose > 2 && brd1.get() != 0) {
	ibis::util::logger lg;
	lg() << mesg << " -- creates an in-memory data partition with "
	     << brd1->nRows() << " row" << (brd1->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd1->describe(lg());
	}
    }
    if (brd1->nRows() == 0) {
	if (ierr >= 0) { // return an empty table of type tabula
	    return new ibis::tabula(tn.c_str(), mesg.c_str(), 0);
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << mesg << " failed to produce any result, "
		"the last error code was " << ierr;
	    return 0;
	}
    }
    else if (brd1->nColumns() == 0) { // count(*)
	return new ibis::tabele(tn.c_str(), mesg.c_str(), brd1->nRows(),
				tms.termName(0));
    }

    if (nplain >= tms.aggSize()) {
	brd1->renameColumns(tms);
	return brd1.release();
    }

    std::auto_ptr<ibis::table> brd2(brd1->groupby(tms));
    if (ibis::gVerbose > 2 && brd2.get() != 0) {
	ibis::util::logger lg;
	lg() << mesg << " -- produces an in-memory data partition with "
	     << brd2->nRows() << " row" << (brd2->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd2->describe(lg());
	}
    }
    return brd2.release();
} // ibis::filter::filt1

/// Upon successful completion of this function, it produces an in-memory
/// data partition holding the selected data records.  It will fail in a
/// unpredictable way if the selected records can not fit in the available
/// memory.
///
/// It expects the arguments sel and cond to be valid and non-trivial.  It
/// will return a nil pointer if those arguments are nil pointers or empty
/// strings or blank spaces.
ibis::table* ibis::table::select(const ibis::constPartList& mylist,
				 const char *sel, const char *cond) {
    try {
	if (mylist.empty())
	    return new ibis::tabula(); // return an empty unnamed table

	if (sel == 0 || *sel == 0)
	    return new ibis::tabula(ibis::table::computeHits(mylist, cond));

	ibis::selectClause sc(sel);
	if (sc.empty())
	    return new ibis::tabula(ibis::table::computeHits(mylist, cond));

	if (cond == 0 || *cond == 0)
	    return ibis::filter::filt0(sc, mylist);

	ibis::whereClause wc(cond);
	if (wc.empty())
	    return ibis::filter::filt0(sc, mylist);

	return ibis::filter::filt(sc, mylist, wc);
    }
    catch (const ibis::bad_alloc &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed a bad_alloc exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const std::exception &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed a std::exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const char *s) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed a string exception ("
		 << s << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (...) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed an unknown exception, "
		"will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    return 0;
} // ibis::table::select

/// Upon successful completion of this function, it produces an in-memory
/// data partition holding the selected data records.  It will fail in an
/// unpredictable way if the selected records can not fit in available
/// memory.
///
/// It expects the arguments sel and cond to be valid and non-trivial.  It
/// will return a nil pointer if those arguments are nil pointers or empty
/// strings or blank spaces.
ibis::table* ibis::table::select(const ibis::constPartList& plist,
				 const char *sel, const ibis::qExpr *cond) {
    try {
	if (plist.empty())
	    return new ibis::tabula(); // return an empty unnamed table

	if (sel == 0 || *sel == 0)
	    return new ibis::tabula(ibis::table::computeHits(plist, cond));

	ibis::selectClause sc(sel);
	if (sc.empty())
	    return new ibis::tabula(ibis::table::computeHits(plist, cond));

	if (cond == 0)
	    return ibis::filter::filt0(sc, plist);

	ibis::whereClause wc;
	wc.setExpr(cond);
	return ibis::filter::filt(sc, plist, wc);
    }
    catch (const ibis::bad_alloc &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed a bad_alloc exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const std::exception &e) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed a std::exception ("
		 << e.what() << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (const char *s) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed a string exception ("
		 << s << "), will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    catch (...) {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- table::select absorbed an unknown exception, "
		"will return a nil pointer";
	    if (ibis::gVerbose > 0)
		ibis::fileManager::instance().printStatus(lg());
	}
    }
    return 0;
} // ibis::table::select

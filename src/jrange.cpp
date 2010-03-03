// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2010 the Regents of the University of California
#include "jrange.h"
#include "tab.h"	// ibis::tabula
#include "bord.h"	// ibis::bord, ibis::bord::bufferList
#include "countQuery.h"	// ibis::countQuery
#include "utilidor.h"	// ibis::util::sortMerge
#include <stdexcept>	// std::exception

/// Constructor.
ibis::jRange::jRange(const ibis::part& partr, const ibis::part& parts,
		     const ibis::column& colr, const ibis::column& cols,
		     double delta1, double delta2,
		     const ibis::qExpr* condr, const ibis::qExpr* conds,
		     const char* desc)
    : partr_(partr), parts_(parts), colr_(colr), cols_(cols),
      delta1_(delta1), delta2_(delta2), valr_(0), vals_(0), nrows(-1) {
    if (desc == 0 || *desc == 0) {
	std::ostringstream oss;
	oss << "From " << partr.name() << " Join " << parts.name()
	    << " On " << partr.name() << '.' << colr.name() << " Between "
	    << parts.name() << '.' << cols.name() << " - " << delta1
	    << " And " << parts.name() << '.' << cols.name() << " + "
	    << delta2 << " Where ...";
	desc_ = oss.str();
    }
    else {
	desc_ = desc;
    }
    int ierr;
    if (condr != 0) {
	ibis::countQuery que(&partr);
	ierr = que.setWhereClause(condr);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jRange(" << desc_ << ") could apply "
		<< condr << " on partition " << partr.name()
		<< ", ierr = " << ierr;
	    throw "ibis::jRange failed to apply conditions on partr";
	}
	ierr = que.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jRange(" << desc_
		<< ") could not evaluate " << que.getWhereClause()
		<< " on partition " << partr.name() << ", ierr = " << ierr;
	    throw "ibis::jRange failed to evaluate constraints on partr";
	}
	maskr_.copy(*que.getHitVector());
    }
    else {
	colr.getNullMask(maskr_);
    }
    if (conds != 0) {
	ibis::countQuery que(&parts);
	ierr = que.setWhereClause(conds);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jRange(" << desc_ << ") could apply "
		<< conds << " on partition " << parts.name()
		<< ", ierr = " << ierr;
	    throw "ibis::jRange failed to apply conditions on parts";
	}
	ierr = que.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jRange(" << desc_
		<< ") could not evaluate " << que.getWhereClause()
		<< " on partition " << parts.name() << ", ierr = " << ierr;
	    throw "ibis::jRange failed to evaluate constraints on parts";
	}
	masks_.copy(*que.getHitVector());
    }
    else {
	cols.getNullMask(masks_);
    }
    LOGGER(ibis::gVerbose > 2)
	<< "ibis::jRange(" << desc_ << ") construction complete";
} // ibis::jRange::jRange

/// Destructor.
ibis::jRange::~jRange() {
}

/// Estimate the number of hits.  Do nothing useful at this time.
void ibis::jRange::roughCount(uint64_t& nmin, uint64_t& nmax) const {
    nmin = 0;
    nmax = maskr_.cnt();
    nmax *= masks_.cnt();
} // ibis::jRange::roughCount

int64_t ibis::jRange::count() const {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::jRange(" << desc_ << ")::count not yet implemented";
    return nrows;
} // ibis::jRange::count

ibis::table*
ibis::jRange::select(const char *sel) const {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::jRange(" << desc_ << ")::select not yet implemented";
    return 0;
} // ibis::jRange::select

ibis::table*
ibis::jRange::select(const std::vector<const char*>& colnames) const {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::jRange(" << desc_ << ")::select not yet implemented";
    return 0;
} // ibis::jRange::select

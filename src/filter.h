// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2010 the Regents of the University of California
#ifndef IBIS_FILTER_H
#define IBIS_FILTER_H
/**@file
   @brief FastBit Filter class.

   This is the simplest version of a query.  Following the older code, this
   class attempts to apply the same where clause on all known data
   partitions and produce a concatenated result set.
 */

#include "quaere.h"		// ibis::quaere
#include "whereClause.h"	// ibis::whereClause
#include "selectClause.h"	// ibis::selectClause

namespace ibis {
    class filter;	// forward definition
} // namespace ibis

/// A simple filtering query.  The where clause does not contain any table
/// names.  Following the convention used in older version of the query
/// class, the same where clause is applied to all known data partitions.
class ibis::filter : public ibis::quaere {
public:
    /// Constructor.  The incoming where clause is applied to all known
    /// data partitions in ibis::datasets.
    explicit filter(const ibis::whereClause& w)
	: wc_(w.empty() ? 0 : new whereClause(w)), parts_(0), aliases_(0) {};
    /// Constructor.
    filter(const ibis::whereClause &w, const ibis::partList &p,
	   const ibis::selectClause &s);
    /// Destructor.
    ~filter() {delete wc_; delete parts_; delete aliases_;}

    virtual void    roughCount(uint64_t& nmin, uint64_t& nmax) const;
    virtual int64_t count() const;
    virtual table*  select(const char *sel) const;

    static table*   filt(const ibis::selectClause &sel,
			 const ibis::partList &pl,
			 const ibis::whereClause &wc);

protected:
    /// The where clause.
    const ibis::whereClause *wc_;
    /// A list of data partitions to query.
    const ibis::partList *parts_;
    /// The select clause.  Primarily used to spply aliases.  If the
    /// function select is called with an empty select clause, then this
    /// variable will be used as the substitute.
    const ibis::selectClause *aliases_;

    /// Default constructor.  Nothing can be done without explicitly
    /// accessing the member variables.
    filter() : wc_(0), parts_(0), aliases_(0) {}

private:
    filter(const filter&); // no copying
    filter& operator=(const filter&); // no assignment
}; // class ibis::filter
#endif

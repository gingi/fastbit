// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2010 the Regents of the University of California
#ifndef IBIS_QUAERE_H
#define IBIS_QUAERE_H
/**@file
   @brief FastBit Quaere Interface.

   This is the public interface to a set of functions that performs query
   operations.  It is intended to replace query.h.
 */

#include "part.h"	// ibis::part

namespace ibis {
    class quaere;	// forward definition
} // namespace ibis

/// An abstract query interface.  It provides three key functions, specify
/// a query, compute the number of hits, and produce a table to represent
/// the selection.  The task of specifying a query is done with various
/// versions of function create.  There are two functions to compute the
/// number of results, estimate and evaluate.
///
/// @warning This is an experimental feature of FastBit.  The current
/// design is very limited and is likely to go through major revisions
/// soon.  Feel free to express your opinions at
/// fastbit-users@hpcrdm.lbl.gov.
///
/// @note The word quaere is the latin equivalent of query.  Once the
/// implementation of this class stablizes, we intend to swap the names
/// quaere and query.
class ibis::quaere {
public:
    static quaere* create(const char* sel, const char* from, const char* where);
    /// The natural join.
    static quaere* create(const ibis::part& partr, const ibis::part& parts,
			  const char* colname, const char* condr = 0,
			  const char* conds = 0);

    /// Provide an estimate of the number of hits.  It never fails.  In
    /// the worst case, it will simply set the minimum (nmin) to 0 and the
    /// maximum (nmax) to the maximum possible number of results.
    virtual void roughCount(uint64_t& nmin, uint64_t& nmax) const = 0;
    /// Compute the number of results.  This function provide the exact
    /// answer.  If it fails to do so, it will return a negative number to
    /// indicate error.
    virtual int64_t count() const = 0;

    /// Produce a projection of the joined table.  The column names
    /// specified should be of the form "part-name.column-name".  If a dot
    /// ('.') is not present or the string before the dot is not the name
    /// of one of the two partitions, the whole string is taken to be a
    /// column name.  In which case, we first look in partition partr for
    /// the named column, then in partition parts.  A nil pointer will be
    /// returned if some names can not be found in the two partitions.
    virtual table* select(const char *sel) const =0;

    virtual ~quaere() {};

protected:
    quaere() {} //< Default constructor.  Can only be used by derived classes.

private:
    quaere(const quaere&); // no copying
    quaere& operator=(const quaere&); // no assignment
}; // class ibis::quaere
#endif

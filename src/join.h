// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008-2010 the Regents of the University of California
#ifndef IBIS_JOIN_H
#define IBIS_JOIN_H
/**@file
   @brief FastBit Join Interface.

   This is the public interface to a set of functions that performs join
   operations.
 */

#include "table.h"	// public data types used by FastBit
#include "part.h"	// ibis::part

namespace ibis {
    class join;	// forward definition
} // namespace ibis

/// An abstract join interface.  It provides three key functions,
/// specify a join, evaluate the number of hits, and iterate through the
/// results.  The task of specifying a join is done with various versions
/// of function create.  There are two functions to compute the number of
/// results, estimate and evaluate.  The iterator for the result of a join
/// is encapsulated in the class ibis::join::result.
///
/// @warning This is an experimental feature of FastBit.  The current
/// design is very limited and is likely to go through major revisions
/// soon.  Feel free to express your opinions at
/// fastbit-users@hpcrdm.lbl.gov.
class ibis::join {
public:
    /// The natural join.  This is equivalent to SQL statement
    ///
    /// "From partr Join parts Using(colname) Where condr And conds"
    ///
    /// Note that conditions specified in condr is for partr only, and
    /// conds is for parts only.  If no conditions are specified, all valid
    /// records in the partition will participate in the natural join.
    static join* create(const ibis::part& partr, const ibis::part& parts,
			const char* colname, const char* condr = 0,
			const char* conds = 0);

    /// Provide an estimate of the number of results.  It never fails.  In
    /// the worst case, it will simply set the minimum (nmin) to 0 and the
    /// maximum (nmax) to the maximum possible number of results.
    virtual void estimate(uint64_t& nmin, uint64_t& nmax) =0;
    /// Compute the number of results.  This function provide the exact
    /// answer.  If it fails to do so, it will return a negative number to
    /// indicate error.
    virtual int64_t evaluate() =0;

    /// Produce a projection of the joined table.  The column names
    /// specified should be of the form "part-name.column-name".  If a dot
    /// ('.') is not present or the string before the dot is not the name
    /// of one of the two partitions, the whole string is taken to be a
    /// column name.  In which case, we first look in partition partr for
    /// the named column, then in partition parts.  A nil pointer will be
    /// returned if some names can not be found in the two partitions.
    virtual table* select(const std::vector<const char*>& colnames) =0;

    virtual ~join() {};

    template <typename T>
    static table*
    fillEquiJoinTable(size_t nrows,
		      const std::string &desc,
		      const ibis::array_t<T>& rjcol,
		      const ibis::table::typeList& rtypes,
		      const std::vector<void*>& rbuff,
		      const ibis::array_t<T>& sjcol,
		      const ibis::table::typeList& stypes,
		      const std::vector<void*>& sbuff,
		      const ibis::table::stringList& cnamet,
		      const std::vector<uint32_t>& cnpos);
    static table*
    fillEquiJoinTable(size_t nrows,
		      const std::string &desc,
		      const std::vector<std::string>& rjcol,
		      const ibis::table::typeList& rtypes,
		      const std::vector<void*>& rbuff,
		      const std::vector<std::string>& sjcol,
		      const ibis::table::typeList& stypes,
		      const std::vector<void*>& sbuff,
		      const ibis::table::stringList& cnamet,
		      const std::vector<uint32_t>& cnpos);

protected:
    join() {} //< Default constructor.  Can only be used by derived classes.

private:
    join(const join&); // no copying
    join& operator=(const join&); // no assignment
}; // class ibis::join
#endif

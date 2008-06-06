// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008 the Regents of the University of California
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
/// @note This is an experimental feature of FastBit.  The current design
/// is very limited and is likely to go through major revisions soon.  Feel
/// free to express your opinions at fastbit-users@hpcrdm.lbl.gov.
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

    class result; // An iterator for the results of join
    /// Produce a projection of the joined table.  The column names
    /// specified should be of the form "part-name.column-name".  If a dot
    /// ('.') is not present or the string before the dot is not the name
    /// of one of the two partitions, the whole string is taken to be a
    /// column name.  In which case, we first look in partition partr for
    /// the named column, then in partition parts.  A nil pointer will be
    /// returned if some names can not be found in the two partitions.
    virtual result* select(const std::vector<const char*>& colnames) =0;

    virtual ~join() {};

protected:
    join() {} //< Default constructor.  Can only be used by derived classes.

private:
    join(const join&); // no copying
    join& operator=(const join&); // no assignment
}; // class ibis::join

/// An abstract base class for results of a join operation.  It primarily
/// provides functions to iterate through the results.
class ibis::join::result {
public:
    virtual ~result() {};

    virtual uint64_t nRows() const=0;
    virtual size_t nColumns() const=0;

    /// Return column names.
    virtual std::vector<std::string> columnNames() const=0;
    /// Return data types of all columns.
    virtual ibis::table::typeList columnTypes() const=0;
    /// Print the column names and type.
    virtual void describe(std::ostream& out) const=0;

    /// Make the next row of the data set available for retrieval.  Returns
    /// 0 if successful, returns a negative number to indicate error.
    virtual int fetch() =0;

    /// Print out the values of the current row.
    virtual int dump(std::ostream& out, const char* del=", ") const=0;

    /// @{
    /// Retrieve the value of the named column.
    /// @note Note the cost of name lookup is likely to dominate the total
    /// cost of such a function.
    virtual int getColumnAsByte(const char* cname, char*) const=0;
    virtual int getColumnAsUByte(const char* cname, unsigned char*) const=0;
    virtual int getColumnAsShort(const char* cname, int16_t*) const=0;
    virtual int getColumnAsUShort(const char* cname, uint16_t*) const=0;
    virtual int getColumnAsInt(const char* cname, int32_t*) const=0;
    virtual int getColumnAsUInt(const char* cname, uint32_t*) const=0;
    virtual int getColumnAsLong(const char* cname, int64_t*) const=0;
    virtual int getColumnAsULong(const char* cname, uint64_t*) const=0;
    virtual int getColumnAsFloat(const char* cname, float*) const=0;
    virtual int getColumnAsDouble(const char* cname, double*) const=0;
    virtual int getColumnAsString(const char* cname, std::string&) const=0;
    /// @}

    /// @{
    /// This version of getColumnAsTTT directly use the column number, i.e.,
    /// the position of a column in the list returned by function @c
    /// columnNames or @c columnTypes.  This version of the data access
    /// function may be able to avoid the name lookup and reduce the
    /// execution time.
    virtual int getColumnAsByte(size_t cnum, char* val) const=0;
    virtual int getColumnAsUByte(size_t cnum, unsigned char* val) const=0;
    virtual int getColumnAsShort(size_t cnum, int16_t* val) const=0;
    virtual int getColumnAsUShort(size_t cnum, uint16_t* val) const=0;
    virtual int getColumnAsInt(size_t cnum, int32_t* val) const=0;
    virtual int getColumnAsUInt(size_t cnum, uint32_t* val) const=0;
    virtual int getColumnAsLong(size_t cnum, int64_t* val) const=0;
    virtual int getColumnAsULong(size_t cnum, uint64_t* val) const=0;
    virtual int getColumnAsFloat(size_t cnum, float* val) const=0;
    virtual int getColumnAsDouble(size_t cnum, double* val) const=0;
    virtual int getColumnAsString(size_t cnum, std::string& val) const=0;
    /// @}

protected:
    result() {} // Default constructor.  May only be used by a derived class.

private:
    result(const result&); // no copying
    result& operator=(const result&); // no assignment
}; // class ibis::join::result
#endif

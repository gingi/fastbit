// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2010 the Regents of the University of California
/** @file
    Declares ibis::selectClause class.
*/
#ifndef IBIS_SELECTCLAUSE_H
#define IBIS_SELECTCLAUSE_H
#include "qExpr.h"
#include "table.h"

namespace ibis {
    class selectLexer;
    class selectParser;
}

/// A class to represent the select clause.  It parses a string into a list
/// of arithmetic expressions and aggregation functions.
///
/// The terms in a select clause are to be separated by coma ',' and each
/// term may be an arithmetic expression or an aggregation function over an
/// arithmetic expression, e.g., "age, avg(income)" and "temperature,
/// sqrt(vx*vx+vy*vy+vz*vz) as speed, max(duration * speed)".  An
/// arithmetic expression may contain any valid combination of numbers and
/// column names connected with operators +, -, *, /, %, ^, ** and standard
/// functions with one and two arguements (defined in math.h).  The
/// supported aggregation functions are:
///
/// - count(*): count the number of rows in each group.
/// - countdistinct(expression): count the number of distinct values.
///   computed by the expression, equivalent to SQL expression
///   'count(distinct expression)'.
/// - avg(expression): compute the average of the expression, (note that
///   the computation is always performed in double-precision
///   floating-point values).
/// - sum(expression): compute the sum of the expression.
/// - max(expression): compute the maximum value of the expression.
/// - min(expression): compute the minimum value of the expression.
/// - median(expression): compute the median of the expression.  Note that
///   if the arithmetic expression is a simple column name, the value
///   retuned by this function has the same type as the column.  In cases
///   requires the computation of the average of two values, the average is
///   computed using the arithmetic in the type of the column.  This means
///   the median of a set of integers is always an integer, which can be
///   slightly different from what one might expect.  Arithmetic
///   expressions are evaluated in double-precision arithmetic, their
///   median values will also be double-precision floating-point numbers.
/// - var(expression): compute the sample variance, i.e., the sum of
///   squared differences from the mean divided by the number of rows in
///   the group minus 1.  This function name may also appears as varsamp or
///   variance.
/// - varp(expression): compute the population variance, i.e., the sum of
///   squared differences from the mean divided by the number of rows in
///   the group.  This function name may also appears as varpop because the
///   origianal contributor of this function, Jan, used varpop.
/// - stdev(expression): compute the sample standard deviation, i.e., the
///   square root of the sum of squared differences from the mean divided
///   by the number of rows minus 1.  Thisfunction name may also appears as
///   stdsamp or stddev.
/// - stdevp(expression): compute the population standard deviation, i.e.,
///   the square root of the sum of squared differences from the mean
///   divided by the number of rows.  This function name may also appears
///   as stdpop.
///
/// @note All select operations excludes null values.  In most SQL
/// implementations, the function 'count(*)' includes the null values.
/// However, in FastBit, null values are always excluded.  For example, the
/// return value for 'count(*)' in the following two select statements may
/// be different if there are any null values in column A,
/// @code
/// select count(*) from ...;
/// select avg(A), count(*) from ...;
/// @endcode
/// In the first case, the number reported is purely determined by the
/// where clause.  However, in the second case, because the select clause
/// also involves the column A, all of null values of A are excluded,
/// therefore 'count(*)' reports the number of rows actually used to
/// compute the average in function 'avg(A)'.  In other SQL
/// implementations, the number of rows used to compute the average is
/// reported through 'count(A)'.
class ibis::selectClause {
public:
    /// Parse a new string as a select clause.
    explicit selectClause(const char *cl=0);
    /// Parse a list of strings.
    selectClause(const ibis::table::stringList&);
    ~selectClause();

    /// Copy constructor.  Deep copy.
    selectClause(const selectClause&);

    /// Parse a new string.
    int parse(const char *cl);

    /// Return a pointer to the string form of the select clause.
    const char* getString(void) const {return clause_.c_str();}
    /// Dereferences to the string form of the select clause.
    const char* operator*(void) const {return clause_.c_str();}

    bool empty() const {return terms_.empty();}
    uint32_t size() const {return terms_.size();}
    /// A vector of arithematic expressions.
    typedef std::vector<ibis::math::term*> mathTerms;
    const mathTerms& getTerms() const {return terms_;}
    /// Fetch the ith term of the select clause, with array bound checking.
    const ibis::math::term* operator[](unsigned i) const {
	if (i < terms_.size())
	    return terms_[i];
	else
	    return 0;
    }
    /// Fetch the ith term of the select clause, without array bound checking.
    const ibis::math::term* at(unsigned i) const {return terms_[i];}

    /// Print the content.
    void print(std::ostream&) const;
    /// Remove the current content.
    void clear();

    int find(const char*) const;
    /// Name inside the aggregation function.
    const char* argName(unsigned i) const {return names_[i].c_str();}
    /// Name given to the whole aggregation function.
    const char* termName(unsigned i) const {return xnames_[i].c_str();}
    void describe(unsigned i, std::string &str) const;
    uint32_t nPlain() const;

    /// Aggregation functions.  @note "Agregado" is Spanish for aggregate.
    enum AGREGADO {NIL, AVG, CNT, MAX, MIN, SUM, DISTINCT,
		   VARPOP, VARSAMP, STDPOP, STDSAMP, MEDIAN};
    /// Return the aggregation function used for the ith term.
    AGREGADO getAggregator(uint32_t i) const {return aggr_[i];}

    int verify(const ibis::part&);
    int verifySome(const ibis::part&, const std::vector<uint32_t>&);
    void getNullMask(const ibis::part&, ibis::bitvector&) const;

    /// Assignment operator.
    selectClause& operator=(const selectClause& rhs) {
	selectClause tmp(rhs);
	swap(tmp);
	return *this;
    }
    /// Swap the content of two select clauses.
    void swap(selectClause& rhs) {
	terms_.swap(rhs.terms_);
	aggr_.swap(rhs.aggr_);
	alias_.swap(rhs.alias_);
	names_.swap(rhs.names_);
	xnames_.swap(rhs.xnames_);
	clause_.swap(rhs.clause_);
    }

protected:
    /// Arithmetic expressions.
    mathTerms terms_;
    /// Aggregation functions.
    std::vector<AGREGADO> aggr_;
    typedef std::map<std::string, unsigned> StringToInt;
    /// Aliases.
    StringToInt alias_;
    /// Names of given to the variables inside the aggregation functions.
    std::vector<std::string> names_;
    /// Names of given to the aggregation operations.
    std::vector<std::string> xnames_;

    std::string clause_;	///< String version of the select clause.

    ibis::selectLexer *lexer;	///< A pointer for the parser.

    friend class ibis::selectParser;

    /// Sort out the names for the terms.
    void fillNames();
    /// The actual work-horse to do the verification.
    int _verify(const ibis::part&, const ibis::math::term&) const;
}; // class ibis::selectClause

/// Number of terms without aggregation functions.
inline uint32_t ibis::selectClause::nPlain() const {
    uint32_t ret = 0;
    for (uint32_t j = 0; j < aggr_.size(); ++j)
	ret += (aggr_[j] == NIL);
    return ret;
} // ibis::selectClause::nPlain

namespace std {
    inline ostream& operator<<(ostream&, const ibis::selectClause&);
}

inline std::ostream& std::operator<<(std::ostream& out,
				     const ibis::selectClause &sel) {
    sel.print(out);
    return out;
} // std::operator<<
#endif

// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2008 the Regents of the University of California
/** \file Declares ibis::selectClause class. */
#ifndef IBIS_SELECTCLAUSE_H
#define IBIS_SELECTCLAUSE_H
#include "qExpr.h"

namespace ibis {
    class selectClause;
    class selectLexer;
    class selectParser;
}

/// A class to encapsulate the parsing of a string into an internal
/// representation suitable for computation.
class ibis::selectClause {
public:
    /// Parse a new string as a select clause.
    explicit selectClause(const char *cl=0);
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
    size_t size() const {return terms_.size();}
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
    const char* getName(unsigned i) const {return names_[i].c_str();}
    void describe(unsigned i, std::string &str) const;

    /// Aggregation functions.  @note "Agregado" is Spanish for aggregate.
    enum AGREGADO {NIL, AVG, CNT, MAX, MIN, SUM};
    AGREGADO getAggregator(size_t i) const {return aggr_[i];}

    /// Make sure all the variables are present in the specified data
    /// partition.  Returns the number of variables that are not. 
    int verify(const ibis::part&) const;
    void getNullMask(const ibis::part&, ibis::bitvector&) const;

    /// Assignment operator.
    selectClause& operator=(const selectClause& rhs) {
	selectClause tmp(rhs);
	swap(tmp);
    }
    /// Swap the content of two select clauses.
    void swap(selectClause& rhs) {
	aggr_.swap(rhs.aggr_);
	alias_.swap(rhs.alias_);
	terms_.swap(rhs.terms_);
	clause_.swap(rhs.clause_);
    }

protected:
    mathTerms terms_;
    std::vector<AGREGADO> aggr_;
    typedef std::map<std::string, unsigned> StringToInt;
    StringToInt alias_;
    std::vector<std::string> names_;

    std::string clause_;	// string version of the select clause

    ibis::selectLexer *lexer;	// hold a pointer for the parser

    friend class ibis::selectParser;

    void fillNames();
    static int _verify(const ibis::part&, const ibis::math::term&);
};
#endif

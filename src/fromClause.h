// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2009-2010 the Regents of the University of California
/** @file
    Declares ibis::fromClause class.
*/
#ifndef IBIS_FROMCLAUSE_H
#define IBIS_FROMCLAUSE_H
#include "qExpr.h"
#include "table.h"

namespace ibis {
    class fromLexer;
    class fromParser;
}

/// A class to represent the from clause.  It parses a string into a list
/// of names, a list of aliases and a join expression.
///
class ibis::fromClause {
public:
    /// Parse a new string as a from clause.
    explicit fromClause(const char *cl=0);
    /// Parse a list of strings.
    fromClause(const ibis::table::stringList&);
    ~fromClause();
    fromClause(const fromClause&);

    /// Parse a new string.
    int parse(const char *cl);

    /// Return a pointer to the string form of the from clause.
    const char* getString(void) const {return clause_.c_str();}
    /// Dereferences to the string form of the from clause.
    const char* operator*(void) const {return clause_.c_str();}

    bool empty() const {return names_.empty();}
    uint32_t size() const {return names_.size()+aliases_.size();}
    void getNames(ibis::table::stringList&) const;

    /// Print the content.
    void print(std::ostream&) const;
    /// Remove the current content.
    void clear();

    const char* find(const char*) const;

    /// Assignment operator.
    fromClause& operator=(const fromClause& rhs) {
	fromClause tmp(rhs);
	swap(tmp);
	return *this;
    }
    /// Swap the content of two from clauses.
    void swap(fromClause& rhs) {
	names_.swap(rhs.names_);
	aliases_.swap(rhs.aliases_);
	clause_.swap(rhs.clause_);
	ordered_.swap(rhs.ordered_);
	ibis::compRange *tmp = jcond_;
	jcond_ = rhs.jcond_;
	rhs.jcond_ = tmp;
    }

protected:
    /// The names of data tables.
    std::vector<std::string> names_;
    /// The aliases.
    std::vector<std::string> aliases_;
    /// The ordered version of the names.
    std::map<const char*, size_t, ibis::lessi> ordered_;
    /// The join condition.  An empty join condition indicates a natural
    /// join that will use the first shared column.  A natural join may
    /// also specify a join column, which will be stored as term 3 of
    /// jcond_.  If this variable is a nil pointer, no join has been
    /// specified.
    ibis::compRange* jcond_;

    std::string clause_;	///< String version of the from clause.
    ibis::fromLexer *lexer;	///< A pointer for the parser.

    friend class ibis::fromParser;
}; // class ibis::fromClause

namespace std {
    inline ostream& operator<<(ostream& out, const ibis::fromClause& fc) {
	fc.print(out);
	return out;
    } // std::operator<<
}
#endif

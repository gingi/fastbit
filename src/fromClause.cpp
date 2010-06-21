// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2009-2010 the Regents of the University of California
#include "part.h"
#include "qExpr.h"
#include "fromLexer.h"
#include "fromClause.h"

ibis::fromClause::fromClause(const char *cl) : jcond_(0), lexer(0) {
    if (cl == 0 || *cl == 0) return;
    LOGGER(ibis::gVerbose > 5)
	<< "fromClause::ctor creating a new from clause with \"" << cl
	<< "\"";
    parse(cl);
} // ibis::fromClause::fromClause

ibis::fromClause::fromClause(const ibis::table::stringList &sl)
    : jcond_(0), lexer(0) {
    for (size_t j = 0; j < sl.size(); ++ j) {
	if (sl[j] != 0 && *(sl[j]) != 0) {
	    if (! clause_.empty())
		clause_ += ", ";
	    clause_ += sl[j];
	}
    }
    if (clause_.empty()) return;
    LOGGER(ibis::gVerbose > 5)
	<< "fromClause::ctor creating a new from clause with \"" << clause_
	<< "\"";
    parse(clause_.c_str());
} // ibis::fromClause::fromClause

/// Copy constructor.  Deep copy.
ibis::fromClause::fromClause(const ibis::fromClause& rhs)
    : names_(rhs.names_), aliases_(rhs.aliases_),
      jcond_(0), clause_(rhs.clause_), lexer(0) {
    if (rhs.jcond_ != 0)
	jcond_ = static_cast<ibis::compRange*>(rhs.jcond_->dup());
    for (size_t j = 0; j < names_.size(); ++ j) {
	if (! names_[j].empty() &&
	    ordered_.find(names_[j].c_str()) == ordered_.end())
	    ordered_[names_[j].c_str()] = j;
	if (! aliases_[j].empty() &&
	    ordered_.find(aliases_[j].c_str()) == ordered_.end())
	    ordered_[aliases_[j].c_str()] = j;
    }
} // ibis::fromClause::fromClause

ibis::fromClause::~fromClause() {
    clear();
}

void ibis::fromClause::clear() {
    names_.clear();
    ordered_.clear();
    aliases_.clear();
    delete jcond_;
    jcond_ = 0;
    clause_.clear();
} // ibis::fromClause::clear

/// Clear the existing content.  A minimal amount of sanity check is also
/// performed.
int ibis::fromClause::parse(const char *cl) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	if (cl != clause_.c_str()) {
	    clear();
	    clause_ = cl;
	}
	std::istringstream iss(clause_);
	ibis::util::logger lg;
	fromLexer lx(&iss, &(lg.buffer()));
	fromParser parser(*this);
	lexer = &lx;
#if DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#elif _DEBUG+0 > 2
	parser.set_debug_level(_DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());
	ierr = parser.parse();
	lexer = 0;
    }
    if (ierr == 0) {
	// if (jcond_ != 0)
	//     ibis::qExpr::simplify(jcond_);
	if (jcond_ != 0 &&
	    (names_.size() != 2 || aliases_.size() != 2)) {
	    ierr = -300;
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- fromClause expects no more than two table "
		"names, but got " << names_.size() << " table name"
		<< (names_.size()>1?"s":"") << " and " << aliases_.size()
		<< " alias" << (aliases_.size()>1?"es":"");
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- fromClause::parse failed to parse string \""
	    << clause_ << "\"";
    }
    if (ierr < 0) clear();
    for (size_t j = 0; j < names_.size(); ++ j) {
	if (! names_[j].empty() &&
	    ordered_.find(names_[j].c_str()) == ordered_.end())
	    ordered_[names_[j].c_str()] = j;
	if (! aliases_[j].empty()) {
	    if (ordered_.find(aliases_[j].c_str()) == ordered_.end()) {
		ordered_[aliases_[j].c_str()] = j;
	    }
	    else {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- fromClause::parse(" << cl
		    << ") detected duplicate alias " << aliases_[j]
		    << ", only the first one will be in effective";
	    }
	}
    }
    return ierr;
} // ibis::fromClause::parse

/// Fill the array nms with the known names.  It returns both actual table
/// names and their aliases in alphabetic order.
void ibis::fromClause::getNames(ibis::table::stringList& nms) const {
    nms.clear();
    nms.reserve(ordered_.size());
    for (std::map<const char*, size_t, ibis::lessi>::const_iterator it
	     = ordered_.begin();
	 it != ordered_.end(); ++ it)
	nms.push_back(it->first);
} // ibis::fromClause::getNames

/// Write a string version of the from clause to the specified output stream.
void ibis::fromClause::print(std::ostream& out) const {
    if (jcond_ == 0) { // no join condition, simply print the table names
	for (size_t j = 0; j < names_.size(); ++j) {
	    if (j > 0)
		out << ", ";
	    out << names_[j];
	    if (! aliases_[j].empty())
		out << " as " << aliases_[j];
	}
    }
    else if (jcond_->getTerm3() != 0 && jcond_->getLeft() == 0 &&
	     jcond_->getRight() == 0) { // join ... using(term 3)
	out << names_[0];
	if (! aliases_[0].empty())
	    out << " as " << aliases_[0];
	out << " join " << names_[1];
	if (! aliases_[1].empty())
	    out << " as " << aliases_[1];
	out << " using " << *(jcond_->getTerm3());
    }
    else if (jcond_->getLeft() == 0 &&
	     jcond_->getRight() == 0) { // join (no explicit join column)
	out << names_[0];
	if (! aliases_[0].empty())
	    out << " as " << aliases_[0];
	out << " join " << names_[1];
	if (! aliases_[1].empty())
	    out << " as " << aliases_[1];	
    }
    else { // join ... on 
	out << names_[0];
	if (! aliases_[0].empty())
	    out << " as " << aliases_[0];
	out << " join " << names_[1];
	if (! aliases_[1].empty())
	    out << " as " << aliases_[1];
	out << " on " << *jcond_;
    }
} // ibis::fromClause::print

/// Given an alias find its real name.  A nil pointer will be returned if
/// the incoming argument is neither an alias nor an actual table name
/// mentioned in the from clause.
const char* ibis::fromClause::realName(const char* al) const {
    if (al == 0 || *al == 0) return 0;
    std::map<const char*, size_t, ibis::lessi>::const_iterator it
	= ordered_.find(al);
    if (it != ordered_.end()) {
	if (it->second < names_.size()) {
	    return names_[it->second].c_str();
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- fromClause::realName(" << al << ") encountered "
		"an internal error, the name points to element "
		<< it->second << ", but there only " << names_.size()
		<< " name" << (names_.size() > 1 ? "s" : "");
	    return 0;
	}
    }
    else {
	LOGGER(ibis::gVerbose > 4)
	    << "Warning -- fromClause::realName(" << al
	    << ") failed to find a name for " << al;
	return 0;
    }
} // ibis::fromClause::realName

/// Given a name find its alias.  A nil pointer will be returned if
/// the incoming argument is neither an alias nor an actual table name
/// mentioned in the from clause.
const char* ibis::fromClause::alias(const char* al) const {
    if (al == 0 || *al == 0) return 0;
    std::map<const char*, size_t, ibis::lessi>::const_iterator it
	= ordered_.find(al);
    if (it != ordered_.end()) {
	if (it->second < aliases_.size()) {
	    return aliases_[it->second].c_str();
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- fromClause::alias(" << al << ") encountered "
		"an internal error, the name points to element "
		<< it->second << ", but there only " << aliases_.size()
		<< " alias" << (names_.size() > 1 ? "es" : "");
	    return 0;
	}
    }
    else {
	LOGGER(ibis::gVerbose > 4)
	    << "Warning -- fromClause::alias(" << al
	    << ") failed to find an alias for " << al;
	return 0;
    }
} // ibis::fromClause::alias

size_t ibis::fromClause::position(const char* al) const {
    if (al == 0 || *al == 0)
	return names_.size();
    std::map<const char*, size_t, ibis::lessi>::const_iterator it
	= ordered_.find(al);
    if (it != ordered_.end()) {
	return it->second;
    }
    else {
	return names_.size();
    }
} // ibis::fromClause::position

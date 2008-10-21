// $Id: selectClause.cpp,v 1.1 2008/10/21 16:04:14 kewu Exp $
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2008 the Regents of the University of California
#include "part.h"
#include "qExpr.h"
#include "selectLexer.h"
#include "selectClause.h"

ibis::selectClause::selectClause(const char *cl) : clause_(cl), lexer(0) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	std::istringstream iss(clause_);
	ibis::util::logger lg(0);
	selectLexer lx(&iss, &(lg.buffer()));
	selectParser parser(*this);
	lexer = &lx;
#if defined(DEBUG) && DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());
	ierr = parser.parse();
	if (ierr == 0) {
	    for (size_t it = 0; it < terms_.size(); ++ it) {
		ibis::qExpr *tmp = terms_[it];
		ibis::qExpr::simplify(tmp);
		if (tmp != terms_[it]) {
		    delete terms_[it];
		    terms_[it] = static_cast<ibis::math::term*>(tmp);
		}
	    }
	    fillNames();
	}
	lexer = 0;
    }
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "selectWhere(" << cl << ") failed to parse the string";
	clear();
    }
} // ibis::selectClause::selectClause

ibis::selectClause::selectClause(const ibis::selectClause& rhs)
    : terms_(rhs.terms_.size()), aggr_(rhs.aggr_.size()),
      clause_(rhs.clause_), lexer(0) {
    for (size_t i = 0; i < rhs.terms_.size(); ++ i) {
	terms_[i] = rhs.terms_[i]->dup();
	aggr_[i] = rhs.aggr_[i];
    }
    alias_.insert(rhs.alias_.begin(), rhs.alias_.end());
} // ibis::selectClause::selectClause

ibis::selectClause::~selectClause() {
    clear();
}

void ibis::selectClause::clear() {
    for (size_t i = 0; i < terms_.size(); ++ i)
	delete terms_[i];
    terms_.clear();
    aggr_.clear();
    alias_.clear();
} // ibis::selectClause::clear

int ibis::selectClause::parse(const char *cl) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	clear();
	clause_ = cl;
	std::istringstream iss(clause_);
	ibis::util::logger lg(0);
	selectLexer lx(&iss, &(lg.buffer()));
	selectParser parser(*this);
	lexer = &lx;
#if defined(DEBUG) && DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());
	ierr = parser.parse();
	if (ierr == 0) {
	    for (size_t it = 0; it < terms_.size(); ++ it) {
		ibis::qExpr *tmp = terms_[it];
		ibis::qExpr::simplify(tmp);
		if (tmp != terms_[it]) {
		    delete terms_[it];
		    terms_[it] = static_cast<ibis::math::term*>(tmp);
		}
	    }
	    fillNames();
	}
	lexer = 0;
    }
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "selectWhere::parse(" << cl << ") failed to parse the string";
	clear();
    }
    return ierr;
} // ibis::selectClause::parse

/// Fill the names array with the appropriate names for each term of the
/// select clause.
void ibis::selectClause::fillNames() {
    names_.clear();
    if (terms_.size() == 0) return;

    names_.resize(terms_.size());
    // go through the aliases first
    for (StringToInt::const_iterator it = alias_.begin();
	 it != alias_.end(); ++ it)
	names_[it->second] = it->first;

    // fill those without a specified name
    for (size_t j = 0; j < terms_.size(); ++ j) {
	if (names_[j].empty()) {
	    std::ostringstream oss;
	    switch (aggr_[j]) {
	    default:
		oss << *(terms_[j]);
		break;
	    case AVG:
		oss << "AVG(" << *(terms_[j]) << ")";
		break;
	    case CNT:
		oss << "COUNT(" << *(terms_[j]) << ")";
		break;
	    case MAX:
		oss << "MAX(" << *(terms_[j]) << ")";
		break;
	    case MIN:
		oss << "MIN(" << *(terms_[j]) << ")";
		break;
	    case SUM:
		oss << "SUM(" << *(terms_[j]) << ")";
		break;
	    }	
	    names_[j] = oss.str();
	}
    }
} // ibis::selectClause::fillNames

void ibis::selectClause::print(std::ostream& out) const {
    std::vector<const std::string*> aliases(terms_.size(), 0);
    for (StringToInt::const_iterator it = alias_.begin();
	 it != alias_.end(); ++ it) {
	aliases[(*it).second] = &(it->first);
    }

    for (size_t i = 0; i < terms_.size(); ++ i) {
	switch (aggr_[i]) {
	default:
	    out << *(terms_[i]);
	    break;
	case AVG:
	    out << "AVG(" << *(terms_[i]) << ")";
	    break;
	case CNT:
	    out << "COUNT(" << *(terms_[i]) << ")";
	    break;
	case MAX:
	    out << "MAX(" << *(terms_[i]) << ")";
	    break;
	case MIN:
	    out << "MIN(" << *(terms_[i]) << ")";
	    break;
	case SUM:
	    out << "SUM(" << *(terms_[i]) << ")";
	    break;
	}
	if (aliases[i] != 0)
	    out << " AS " << *(aliases[i]);
	if (i+1 < terms_.size())
	    out << ", ";
    }
} // ibis::selectClause::print

int ibis::selectClause::verify(const ibis::part& part0) const {
    int ierr = 0;
    for (size_t j = 0; j < terms_.size(); ++ j)
	ierr += ibis::selectClause::_verify(part0, *(terms_[j]));
    return ierr;
} // ibis::selectClause::verify

int ibis::selectClause::_verify(const ibis::part& part0,
				const ibis::math::term& xp0) {
    int ierr = 0;

    if (xp0.termType() == ibis::math::VARIABLE) {
	const ibis::math::variable& var =
	    static_cast<const ibis::math::variable&>(xp0);
	const ibis::column* col = part0.getColumn(var.variableName());
	if (col == 0) {
	    ++ ierr;
	    LOGGER(ibis::gVerbose > 0)
		<< "ibis::selectClause::verify -- data partition "
		<< part0.name() << " does not contain a column named "
		<< var.variableName();
	}
    }
    else {
	if (xp0.getLeft() != 0)
	    ierr = _verify(part0, *static_cast<const ibis::math::term*>(xp0.getLeft()));
	if (xp0.getRight() != 0)
	    ierr += _verify(part0, *static_cast<const ibis::math::term*>(xp0.getRight()));
    }

    return ierr;
} // ibis::selectClause::_verify

void ibis::selectClause::getNullMask(const ibis::part& part0,
				     ibis::bitvector& mask) const {
    mask.copy(part0.getMask());
    if (terms_.size() > 0) {
	ibis::part::barrel bar(&part0);
	for (size_t j = 0; j < terms_.size(); ++ j)
	    bar.recordVariable(terms_[j]);
	ibis::bitvector tmp;
	bar.getNullMask(tmp);
	mask &= tmp;
    }
} // ibis::selectClause::getNullMask

int ibis::selectClause::find(const char* key) const {
    int ret = -1;
    if (key != 0 && *key != 0) {
	StringToInt::const_iterator it = alias_.find(key);
	if (it != alias_.end()) {
	    ret = it->second;
	}
	else {
	    for (ret = 0; ret < static_cast<int>(names_.size()); ++ ret) {
		if (stricmp(names_[ret].c_str(), key) == 0)
		    break;
	    }
	    if (ret >= static_cast<int>(names_.size()))
		ret = -1;
	}
    }
    return ret;
} // ibis::selectClause::find

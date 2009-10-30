// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
#include "part.h"
#include "qExpr.h"
#include "selectLexer.h"
#include "selectClause.h"

ibis::selectClause::selectClause(const char *cl) : clause_(cl), lexer(0) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	std::istringstream iss(clause_);
	ibis::util::logger lg;
	selectLexer lx(&iss, &(lg.buffer()));
	selectParser parser(*this);
	lexer = &lx;
#if defined(DEBUG) && DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());
	ierr = parser.parse();
	if (ierr == 0) {
	    for (uint32_t it = 0; it < terms_.size(); ++ it) {
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
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- selectWhere failed to parse string \"" << cl << "\"";
	clear();
    }
} // ibis::selectClause::selectClause

ibis::selectClause::selectClause(const ibis::selectClause& rhs)
    : terms_(rhs.terms_.size()), aggr_(rhs.aggr_.size()),
      clause_(rhs.clause_), lexer(0) {
    for (uint32_t i = 0; i < rhs.terms_.size(); ++ i) {
	terms_[i] = rhs.terms_[i]->dup();
	aggr_[i] = rhs.aggr_[i];
    }
    alias_.insert(rhs.alias_.begin(), rhs.alias_.end());
} // ibis::selectClause::selectClause

ibis::selectClause::~selectClause() {
    clear();
}

void ibis::selectClause::clear() {
    for (uint32_t i = 0; i < terms_.size(); ++ i)
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
	ibis::util::logger lg;
	selectLexer lx(&iss, &(lg.buffer()));
	selectParser parser(*this);
	lexer = &lx;
#if defined(DEBUG) && DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());
	ierr = parser.parse();
	if (ierr == 0) {
	    for (uint32_t it = 0; it < terms_.size(); ++ it) {
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
	    << "Warning -- selectWhere::parse failed to parse string\""
	    << cl << "\"";
	clear();
    }
    return ierr;
} // ibis::selectClause::parse

/// Write the string form of the ith term into str.
void ibis::selectClause::describe(unsigned i, std::string &str) const {
    if (i >= terms_.size()) return;
    std::ostringstream oss;
    switch (aggr_[i]) {
    default:
	oss << *(terms_[i]);
	break;
    case AVG:
	oss << "AVG(" << *(terms_[i]) << ")";
	break;
    case CNT:
	oss << "COUNT(" << *(terms_[i]) << ")";
	break;
    case MAX:
	oss << "MAX(" << *(terms_[i]) << ")";
	break;
    case MIN:
	oss << "MIN(" << *(terms_[i]) << ")";
	break;
    case SUM:
	oss << "SUM(" << *(terms_[i]) << ")";
	break;
    case VARPOP:
	oss << "VARPOP(" << *(terms_[i]) << ")";
	break;
    case VARSAMP:
	oss << "VARSAMP(" << *(terms_[i]) << ")";
	break;
    case STDPOP:
	oss << "STDPOP(" << *(terms_[i]) << ")";
	break;
    case STDSAMP:
	oss << "STDSAMP(" << *(terms_[i]) << ")";
	break;
    case DISTINCT:
	oss << "DISTINCT(" << *(terms_[i]) << ")";
	break;
    }

    str = oss.str();
} // ibis::selectClause::describe

/// Fill array names_.  If an alias is present, it is used, if the term
/// is a variable, the variable name is used, otherwise, a name of the form
/// "shhh" is generated where "hhh" is the hexadecimal number.
void ibis::selectClause::fillNames() {
    names_.clear();
    if (terms_.size() == 0) return;

    uint32_t prec = 0;
    for (uint32_t j = terms_.size(); j > 0; j >>= 4)
	++ prec;

    names_.resize(terms_.size());
    // go through the aliases first
    for (StringToInt::const_iterator it = alias_.begin();
	 it != alias_.end(); ++ it)
	names_[it->second] = it->first;

    // fill those without a specified name
    for (uint32_t j = 0; j < terms_.size(); ++ j) {
	if (names_[j].empty()) {
	    if (terms_[j]->termType() == ibis::math::VARIABLE) {
		names_[j] = static_cast<const ibis::math::variable*>(terms_[j])
		    ->variableName();
	    }
	    else {
		std::ostringstream oss;
		oss << "s" << std::setprecision(prec) << j;
		names_[j] = oss.str();
	    }
	}
    }
} // ibis::selectClause::fillNames

/// Locate the position of the string.  Upon successful completion, it
/// returns the position of the term with the matching name, otherwise, it
/// returns -1.  The incoming argument may be an alias, a column name, or
/// the exact form of the arithmetic expression.  In case, it is the whole
/// arithmetic expression, it must be exactly the same as the original term
/// passed to the constructor of this class.  The comparison is done with
/// case-insensitive string comparison.
int ibis::selectClause::find(const char* key) const {
    int ret = -1;
    if (key != 0 && *key != 0) {
	StringToInt::const_iterator it = alias_.find(key);
	if (it != alias_.end()) {
	    ret = it->second;
	}
	else {
	    // try to match short-hand names
	    for (ret = 0; ret < static_cast<int>(names_.size()); ++ ret) {
		if (stricmp(names_[ret].c_str(), key) == 0)
		    break;
	    }
	    // try to match the string version of each arithmetic expression
	    if (ret >= static_cast<int>(names_.size())) {
		for (unsigned int i = 0; i < terms_.size(); ++ i) {
		    std::ostringstream oss;
		    switch (aggr_[i]) {
		    default:
			oss << *(terms_[i]);
			break;
		    case AVG:
			oss << "AVG(" << *(terms_[i]) << ")";
			break;
		    case CNT:
			oss << "COUNT(" << *(terms_[i]) << ")";
			break;
		    case MAX:
			oss << "MAX(" << *(terms_[i]) << ")";
			break;
		    case MIN:
			oss << "MIN(" << *(terms_[i]) << ")";
			break;
		    case SUM:
			oss << "SUM(" << *(terms_[i]) << ")";
			break;
		    case VARPOP:
			oss << "VARPOP(" << *(terms_[i]) << ")";
			break;
		    case VARSAMP:
			oss << "VARSAMP(" << *(terms_[i]) << ")";
			break;
		    case STDPOP:
			oss << "STDPOP(" << *(terms_[i]) << ")";
			break;
		    case STDSAMP:
			oss << "STDSAMP(" << *(terms_[i]) << ")";
			break;
		    case DISTINCT:
			oss << "DISTINCT(" << *(terms_[i]) << ")";
			break;
		    }
		    if (stricmp(oss.str().c_str(), key) == 0) {
			ret = i;
			break;
		    }
		}
	    }
	    if (ret >= static_cast<int>(names_.size()))
		ret = -1;
	}
    }
    return ret;
} // ibis::selectClause::find

/// Write a string version of the select clause to the specified output stream.
void ibis::selectClause::print(std::ostream& out) const {
    std::vector<const std::string*> aliases(terms_.size(), 0);
    for (StringToInt::const_iterator it = alias_.begin();
	 it != alias_.end(); ++ it) {
	aliases[(*it).second] = &(it->first);
    }

    for (uint32_t i = 0; i < terms_.size(); ++ i) {
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
	case VARPOP:
	    out << "VARPOP(" << *(terms_[i]) << ")";
	    break;
	case VARSAMP:
	    out << "VARSAMP(" << *(terms_[i]) << ")";
	    break;
	case STDPOP:
	    out << "STDPOP(" << *(terms_[i]) << ")";
	    break;
	case STDSAMP:
	    out << "STDSAMP(" << *(terms_[i]) << ")";
	    break;
	case DISTINCT:
	    out << "DISTINCT(" << *(terms_[i]) << ")";
	    break;
	}
	if (aliases[i] != 0)
	    out << " AS " << *(aliases[i]);
	if (i+1 < terms_.size())
	    out << ", ";
    }
} // ibis::selectClause::print

/// Are all the variables are present in the specified data partition?
/// Returns the number of variables that are not.  This function also
/// simplifies the arithmetic expression if
/// ibis::math::preserveInputExpression is not set.
///
/// @note Simplifying the arithmetic expressions typically reduces the time
/// needed for evaluations, but may introduces a different set of round-off
/// erros in the evaluation process than the original expression.
int ibis::selectClause::verify(const ibis::part& part0) {
    int ierr = 0;
    for (uint32_t j = 0; j < terms_.size(); ++ j) {
	if (ibis::math::preserveInputExpressions == false) {
	    ibis::math::term *tmp = terms_[j]->reduce();
	    if (tmp != terms_[j]) {
		delete terms_[j];
		terms_[j] = tmp;
	    }
	}
	ierr += ibis::selectClause::_verify(part0, *(terms_[j]));
    }
    return ierr;
} // ibis::selectClause::verify

int ibis::selectClause::verifySome(const ibis::part& part0,
				   const std::vector<uint32_t>& touse) {
    int ierr = 0;
    for (uint32_t j = 0; j < touse.size(); ++ j) {
	if (ibis::math::preserveInputExpressions == false) {
	    ibis::math::term *tmp = terms_[touse[j]]->reduce();
	    if (tmp != terms_[touse[j]]) {
		delete terms_[touse[j]];
		terms_[touse[j]] = tmp;
	    }
	}
	ierr += ibis::selectClause::_verify(part0, *(terms_[touse[j]]));
    }
    return ierr;
} // ibis::selectClause::verifySome

int ibis::selectClause::_verify(const ibis::part& part0,
				const ibis::math::term& xp0) const {
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
    else if (xp0.termType() == ibis::math::UNDEFINED) {
	++ ierr;
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::selectClause::verify -- ibis::math::term has a "
	    "undefined type";
    }
    else {
	if (xp0.getLeft() != 0)
	    ierr += _verify(part0, *static_cast<const ibis::math::term*>
			   (xp0.getLeft()));
	if (xp0.getRight() != 0)
	    ierr += _verify(part0, *static_cast<const ibis::math::term*>
			    (xp0.getRight()));
    }

    return ierr;
} // ibis::selectClause::_verify

void ibis::selectClause::getNullMask(const ibis::part& part0,
				     ibis::bitvector& mask) const {
    mask.copy(part0.getNullMask());
    if (terms_.size() > 0) {
	ibis::part::barrel bar(&part0);
	for (uint32_t j = 0; j < terms_.size(); ++ j)
	    bar.recordVariable(terms_[j]);
	ibis::bitvector tmp;
	bar.getNullMask(tmp);
	mask &= tmp;
    }
} // ibis::selectClause::getNullMask

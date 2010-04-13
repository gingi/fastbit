// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2010 the Regents of the University of California
#include "part.h"
#include "qExpr.h"
#include "selectLexer.h"
#include "selectClause.h"

ibis::selectClause::selectClause(const char *cl) : lexer(0) {
    if (cl == 0 || *cl == 0) return;
    LOGGER(ibis::gVerbose > 5)
	<< "selectClause::ctor creating a new select clause with \"" << cl
	<< "\"";

    int ierr = 0;
    clause_ = cl;
    std::istringstream iss(clause_);
    ibis::util::logger lg;
    selectLexer lx(&iss, &(lg.buffer()));
    selectParser parser(*this);
    lexer = &lx;
#if DEBUG+0 > 2
    parser.set_debug_level(DEBUG-1);
#elif _DEBUG+0 > 2
    parser.set_debug_level(_DEBUG-1);
#endif
    parser.set_debug_stream(lg.buffer());
    ierr = parser.parse();
    lexer = 0;

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
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- selectClause::ctor failed to parse string \""
	    << clause_ << "\"";
	clear();
    }
} // ibis::selectClause::selectClause

ibis::selectClause::selectClause(const ibis::table::stringList &sl) : lexer(0) {
    for (size_t j = 0; j < sl.size(); ++ j) {
	if (sl[j] != 0 && *(sl[j]) != 0) {
	    if (! clause_.empty())
		clause_ += ", ";
	    clause_ += sl[j];
	}
    }
    if (clause_.empty()) return;
    LOGGER(ibis::gVerbose > 5)
	<< "selectClause::ctor creating a new select clause with \"" << clause_
	<< "\"";

    int ierr = 0;
    std::istringstream iss(clause_);
    ibis::util::logger lg;
    selectLexer lx(&iss, &(lg.buffer()));
    selectParser parser(*this);
    lexer = &lx;
#if DEBUG+0 > 2
    parser.set_debug_level(DEBUG-1);
#elif _DEBUG+0 > 2
    parser.set_debug_level(_DEBUG-1);
#endif
    parser.set_debug_stream(lg.buffer());
    ierr = parser.parse();
    lexer = 0;

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
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- selectClause::ctor failed to parse string \""
	    << clause_ << "\"";
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
	LOGGER(ibis::gVerbose > 5)
	    << "selectClause::parse cleared existing content before parsing \""
	    << cl << "\"";

	clear();
	clause_ = cl;
	std::istringstream iss(clause_);
	ibis::util::logger lg;
	selectLexer lx(&iss, &(lg.buffer()));
	selectParser parser(*this);
	lexer = &lx;
#if DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#elif _DEBUG+0 > 2
	parser.set_debug_level(_DEBUG-1);
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
	    << "Warning -- selectClause::parse failed to parse string \""
	    << cl << "\"";
	clear();
    }
    return ierr;
} // ibis::selectClause::parse

/// Write the string form of the ith term into str.
void ibis::selectClause::describe(unsigned i, std::string &str) const {
    if (i >= terms_.size()) return;
    if (! xnames_[i].empty()) {
	str = xnames_[i];
    }
    else if (! names_[i].empty()) {
	switch (aggr_[i]) {
	default:
	    str = names_[i];
	    break;
	case AVG:
	    str = "AVG(";
	    str += names_[i];
	    str += ')';
	    break;
	case CNT:
	    str = "COUNT(";
	    str += names_[i];
	    str += ')';
	    break;
	case MAX:
	    str = "MAX(";
	    str += names_[i];
	    str += ')';
	    break;
	case MIN:
	    str = "MIN(";
	    str += names_[i];
	    str += ')';
	    break;
	case SUM:
	    str = "SUM(";
	    str += names_[i];
	    str += ')';
	    break;
	case VARPOP:
	    str = "VARPOP(";
	    str += names_[i];
	    str += ')';
	    break;
	case VARSAMP:
	    str = "VARSAMP(";
	    str += names_[i];
	    str += ')';
	    break;
	case STDPOP:
	    str = "STDPOP(";
	    str += names_[i];
	    str += ')';
	    break;
	case STDSAMP:
	    str = "STDSAMP(";
	    str += names_[i];
	    str += ')';
	    break;
	case DISTINCT:
	    str = "COUNTDISTINCT(";
	    str += names_[i];
	    str += ')';
	    break;
	}
    }
    else if (terms_[i] != 0) {
	std::ostringstream oss;
	switch (aggr_[i]) {
	default:
	    oss << *(terms_[i]);
	    break;
	case AVG:
	    oss << "AVG(" << *(terms_[i]) << ')';
	    break;
	case CNT:
	    oss << "COUNT(" << *(terms_[i]) << ')';
	    break;
	case MAX:
	    oss << "MAX(" << *(terms_[i]) << ')';
	    break;
	case MIN:
	    oss << "MIN(" << *(terms_[i]) << ')';
	    break;
	case SUM:
	    oss << "SUM(" << *(terms_[i]) << ')';
	    break;
	case VARPOP:
	    oss << "VARPOP(" << *(terms_[i]) << ')';
	    break;
	case VARSAMP:
	    oss << "VARSAMP(" << *(terms_[i]) << ')';
	    break;
	case STDPOP:
	    oss << "STDPOP(" << *(terms_[i]) << ')';
	    break;
	case STDSAMP:
	    oss << "STDSAMP(" << *(terms_[i]) << ')';
	    break;
	case DISTINCT:
	    oss << "COUNTDISTINCT(" << *(terms_[i]) << ')';
	    break;
	}

	str = oss.str();
    }
} // ibis::selectClause::describe

/// Fill array names_ and xnames_.  An alias for an aggregation operation
/// is used as the external name for the whole term.  This function
/// resolves all external names first to establish all aliases, and then
/// resolve the names of the arguments to the aggregation functions.  The
/// arithmetic expressions without external names are given names of the
/// form "shhh", where "hhh" is the hexadecimal number.
void ibis::selectClause::fillNames() {
    names_.clear();
    xnames_.clear();
    if (terms_.empty()) return;

    uint32_t prec = 0; // number of hexadecimal to use
    for (uint32_t j = terms_.size(); j > 0; j >>= 4)
	++ prec;

    names_.resize(terms_.size());
    xnames_.resize(terms_.size());
    // go through the aliases first to assign xnames_
    for (StringToInt::const_iterator it = alias_.begin();
	 it != alias_.end(); ++ it)
	xnames_[it->second] = it->first;

    // fill the argument name and then the external name
    for (uint32_t j = 0; j < terms_.size(); ++ j) {
	if (terms_[j]->termType() == ibis::math::VARIABLE) {
	    names_[j] = static_cast<const ibis::math::variable*>(terms_[j])
		->variableName();
	}
	else {
	    std::ostringstream oss;
	    oss << 's' << std::hex << std::setprecision(prec) << j;
	    names_[j] = oss.str();
	}

	if (xnames_[j].empty() && aggr_[j] == ibis::selectClause::NIL)
	    xnames_[j] = names_[j];

	if (xnames_[j].empty()) {
	    std::ostringstream oss;
	    switch (aggr_[j]) {
	    default:
		oss << 's';
		break;
	    case AVG:
		oss << "avg";
		break;
	    case CNT:
		oss << "count";
		break;
	    case MAX:
		oss << "max";
		break;
	    case MIN:
		oss << "min";
		break;
	    case SUM:
		oss << "sum";
		break;
	    case VARPOP:
		oss << "var";
		break;
	    case VARSAMP:
		oss << "var";
		break;
	    case STDPOP:
		oss << "std";
		break;
	    case STDSAMP:
		oss << "std";
		break;
	    case DISTINCT:
		oss << "count";
		break;
	    }
	    oss << std::hex << std::setprecision(prec) << j;
	    xnames_[j] = oss.str();
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
	    // try to match names of the terms one at a time
	    for (ret = 0; ret < static_cast<int>(names_.size()); ++ ret) {
		if (stricmp(xnames_[ret].c_str(), key) == 0)
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
			oss << "AVG(" << *(terms_[i]) << ')';
			break;
		    case CNT:
			oss << "COUNT(" << *(terms_[i]) << ')';
			break;
		    case MAX:
			oss << "MAX(" << *(terms_[i]) << ')';
			break;
		    case MIN:
			oss << "MIN(" << *(terms_[i]) << ')';
			break;
		    case SUM:
			oss << "SUM(" << *(terms_[i]) << ')';
			break;
		    case VARPOP:
			oss << "VARPOP(" << *(terms_[i]) << ')';
			break;
		    case VARSAMP:
			oss << "VARSAMP(" << *(terms_[i]) << ')';
			break;
		    case STDPOP:
			oss << "STDPOP(" << *(terms_[i]) << ')';
			break;
		    case STDSAMP:
			oss << "STDSAMP(" << *(terms_[i]) << ')';
			break;
		    case DISTINCT:
			oss << "COUNTDISTINCT(" << *(terms_[i]) << ')';
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
	    out << "AVG(" << *(terms_[i]) << ')';
	    break;
	case CNT:
	    out << "COUNT(" << *(terms_[i]) << ')';
	    break;
	case MAX:
	    out << "MAX(" << *(terms_[i]) << ')';
	    break;
	case MIN:
	    out << "MIN(" << *(terms_[i]) << ')';
	    break;
	case SUM:
	    out << "SUM(" << *(terms_[i]) << ')';
	    break;
	case VARPOP:
	    out << "VARPOP(" << *(terms_[i]) << ')';
	    break;
	case VARSAMP:
	    out << "VARSAMP(" << *(terms_[i]) << ')';
	    break;
	case STDPOP:
	    out << "STDPOP(" << *(terms_[i]) << ')';
	    break;
	case STDSAMP:
	    out << "STDSAMP(" << *(terms_[i]) << ')';
	    break;
	case DISTINCT:
	    out << "COUNTDISTINCT(" << *(terms_[i]) << ')';
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
int ibis::selectClause::verify(const ibis::part& part0) const {
    int ierr = 0;
    for (uint32_t j = 0; j < terms_.size(); ++ j) {
	if (ibis::math::preserveInputExpressions == false) {
	    ibis::math::term *tmp = terms_[j]->reduce();
	    if (tmp != terms_[j]) {
		delete const_cast<ibis::math::term*>(terms_[j]);
		const_cast<mathTerms&>(terms_)[j] = tmp;
	    }
	}
	ierr += ibis::selectClause::_verify(part0, *(terms_[j]));
    }
    return ierr;
} // ibis::selectClause::verify

int ibis::selectClause::verifySome(const ibis::part& part0,
				   const std::vector<uint32_t>& touse) const {
    int ierr = 0;
    for (uint32_t j = 0; j < touse.size(); ++ j) {
	if (ibis::math::preserveInputExpressions == false) {
	    ibis::math::term *tmp = terms_[touse[j]]->reduce();
	    if (tmp != terms_[touse[j]]) {
		delete const_cast<ibis::math::term*>(terms_[touse[j]]);
		const_cast<mathTerms&>(terms_)[touse[j]] = tmp;
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
	if (*(var.variableName()) != '*') {
	    if (part0.getColumn(var.variableName()) == 0) {
		++ ierr;
		LOGGER(ibis::gVerbose > 0)
		    << "selectClause::verify -- data partition "
		    << part0.name() << " does not contain a column named "
		    << var.variableName();
	    }
	}
    }
    else if (xp0.termType() == ibis::math::UNDEFINED) {
	++ ierr;
	LOGGER(ibis::gVerbose > 0)
	    << "selectClause::verify -- ibis::math::term has a "
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
    if (terms_.size() > 0) {
	ibis::part::barrel bar(&part0);
	for (uint32_t j = 0; j < terms_.size(); ++ j)
	    bar.recordVariable(terms_[j]);
	if (bar.size() > 0) {
	    bar.getNullMask(mask);
	}
	else {
	    mask.copy(part0.getNullMask());
	}
    }
    else {
	mask.copy(part0.getNullMask());
    }
} // ibis::selectClause::getNullMask

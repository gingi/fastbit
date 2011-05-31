// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2011 the Regents of the University of California
#include "part.h"
#include "qExpr.h"
#include "selectLexer.h"
#include "selectClause.h"

ibis::selectClause::selectClause(const char *cl) : lexer(0) {
    if (cl == 0 || *cl == 0) return;
    int ierr = parse(cl);
    LOGGER(ierr < 0 && ibis::gVerbose >= 0)
	<< "selectClause::ctor failed to parse \"" << cl
	<< "\", function parse returned " << ierr;
} // ibis::selectClause::selectClause

ibis::selectClause::selectClause(const ibis::table::stringList &sl) : lexer(0) {
    std::string cl;
    for (size_t j = 0; j < sl.size(); ++ j) {
	if (sl[j] != 0 && *(sl[j]) != 0) {
	    if (! clause_.empty())
		cl += ", ";
	    cl += sl[j];
	}
    }
    if (cl.empty()) return;
    int ierr = parse(cl.c_str());
    LOGGER(ierr < 0 && ibis::gVerbose >= 0)
	<< "selectClause::ctor failed to parse \"" << cl
	<< "\", function parse returned " << ierr;
} // ibis::selectClause::selectClause

ibis::selectClause::selectClause(const ibis::selectClause& rhs)
    : atms_(rhs.atms_.size()), aggr_(rhs.aggr_), xtms_(rhs.xtms_.size()),
      names_(rhs.names_), xnames_(rhs.xnames_), clause_(rhs.clause_), lexer(0) {
    for (uint32_t i = 0; i < rhs.atms_.size(); ++ i) {
	atms_[i] = rhs.atms_[i]->dup();
	aggr_[i] = rhs.aggr_[i];
    }
    for (uint32_t j = 0; j < rhs.xtms_.size(); ++ j)
	xtms_[j] = rhs.xtms_[j]->dup();
    xalias_.insert(rhs.xalias_.begin(), rhs.xalias_.end());
} // ibis::selectClause::selectClause

ibis::selectClause::~selectClause() {
    clear();
}

void ibis::selectClause::clear() {
    for (uint32_t j = 0; j < xtms_.size(); ++ j)
	delete xtms_[j];
    xtms_.clear();
    for (uint32_t i = 0; i < atms_.size(); ++ i)
	delete atms_[i];
    atms_.clear();
    aggr_.clear();
    xalias_.clear();
} // ibis::selectClause::clear

int ibis::selectClause::parse(const char *cl) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	clear();
	LOGGER(ibis::gVerbose > 5)
	    << "selectClause::parse cleared existing content before parsing \""
	    << cl << "\"";

	clause_ = cl;
	std::istringstream iss(clause_);
	ibis::util::logger lg;
	selectLexer lx(&iss, &(lg()));
	selectParser parser(*this);
	lexer = &lx;
#if DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#elif _DEBUG+0 > 2
	parser.set_debug_level(_DEBUG-1);
#endif
	parser.set_debug_stream(lg());
	ierr = parser.parse();
	if (ierr == 0) {
	    // for (uint32_t it = 0; it < atms_.size(); ++ it) {
	    // 	ibis::qExpr *tmp = atms_[it];
	    // 	ibis::qExpr::simplify(tmp);
	    // 	if (tmp != atms_[it]) {
	    // 	    delete atms_[it];
	    // 	    atms_[it] = static_cast<ibis::math::term*>(tmp);
	    // 	}
	    // }
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

/// Write the string form of the ith term.  The result is placed in the
/// second argument str.
void ibis::selectClause::describe(unsigned i, std::string &str) const {
    if (i >= xtms_.size()) return;
    if (atms_[i] != 0) {
	std::ostringstream oss;
	switch (aggr_[i]) {
	default:
	    oss << *(atms_[i]);
	    break;
	case AVG:
	    oss << "AVG(" << *(atms_[i]) << ')';
	    break;
	case CNT:
	    oss << "COUNT(" << *(atms_[i]) << ')';
	    break;
	case MAX:
	    oss << "MAX(" << *(atms_[i]) << ')';
	    break;
	case MIN:
	    oss << "MIN(" << *(atms_[i]) << ')';
	    break;
	case SUM:
	    oss << "SUM(" << *(atms_[i]) << ')';
	    break;
	case VARPOP:
	    oss << "VARPOP(" << *(atms_[i]) << ')';
	    break;
	case VARSAMP:
	    oss << "VARSAMP(" << *(atms_[i]) << ')';
	    break;
	case STDPOP:
	    oss << "STDPOP(" << *(atms_[i]) << ')';
	    break;
	case STDSAMP:
	    oss << "STDSAMP(" << *(atms_[i]) << ')';
	    break;
	case DISTINCT:
	    oss << "COUNTDISTINCT(" << *(atms_[i]) << ')';
	    break;
	}

	str = oss.str();
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
    else if (! xnames_[i].empty()) {
	str = xnames_[i];
    }
} // ibis::selectClause::describe

/// Fill array names_ and xnames_.  An alias for an aggregation operation
/// is used as the external name for the whole term.  This function
/// resolves all external names first to establish all aliases, and then
/// resolve the names of the arguments to the aggregation functions.  The
/// arithmetic expressions without external names are given names of the
/// form "__hhh", where "hhh" is a hexadecimal number.
void ibis::selectClause::fillNames() {
    names_.clear();
    xnames_.clear();
    if (atms_.empty()) return;

    names_.resize(atms_.size());
    xnames_.resize(atms_.size());
    // go through the aliases first to assign xnames_
    for (StringToInt::const_iterator it = xalias_.begin();
	 it != xalias_.end(); ++ it)
	xnames_[it->second] = it->first;

    // fill the argument name and then the external name
    for (uint32_t j = 0; j < atms_.size(); ++ j) {
	if (atms_[j]->termType() == ibis::math::VARIABLE) {
	    names_[j] = static_cast<const ibis::math::variable*>(atms_[j])
		->variableName();
	}
	else {
	    std::ostringstream oss;
	    oss << "__" << std::hex << j;
	    names_[j] = oss.str();
	}

	if (xnames_[j].empty() && aggr_[j] == ibis::selectClause::NIL_AGGR) {
	    xnames_[j] = names_[j];
	    for (unsigned i = 0; i < names_[j].size(); ++ i)
		if (! isalnum(xnames_[j][i]))
		    xnames_[j][i] = '_';
	}

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
	    case DISTINCT:
		oss << "distinct";
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
	    case MEDIAN:
		oss << "med";
		break;
	    }
	    oss << std::hex << j;
	    xnames_[j] = oss.str();
	}
    }
} // ibis::selectClause::fillNames

/// Record an aggregation function.  Return a math term of the type
/// variable to caller so the caller can continue to build up a larger
/// expression.  For simplicity, the variable name is simply "__hhh", where
/// "hhh" is the size of aggr_ in hexadecimal.
ibis::math::variable*
ibis::selectClause::addAgregado(ibis::selectClause::AGREGADO agr,
				ibis::math::term *expr) {
    const unsigned pos = atms_.size();
    aggr_.push_back(agr);
    atms_.push_back(expr);
    if (expr->termType() != ibis::math::VARIABLE) {
	std::ostringstream oss;
	oss << "__" << std::hex << pos;
	ordered_[oss.str()] = pos;
	return new ibis::selectClause::variable(oss.str().c_str(), *this);
    }
    else {
	ibis::math::variable *var = static_cast<ibis::math::variable*>(expr);
	ordered_[var->variableName()] = pos;
	return var->dup();
    }
} // ibis::selectClause::addAgregado

/// Determine if the name refers to a term in the list of aggregation
/// functions.  A name to a aggregation function will be named by
/// ibis::selctClause::addAgregado.  If the return value is less than the
/// size of atms_, then the name is considered referring to a aggregation
/// function, otherwise, it is a literal name from the user.
uint64_t ibis::selectClause::decodeAName(const char* nm) const {
    uint64_t ret = std::numeric_limits<uint64_t>::max();
    if (nm == 0) return ret;
    if (nm[0] != '_' || nm[1] != '_') return ret;

    int ierr = ibis::util::decode16(ret, nm+2);
    if (ierr < 0)
	return atms_.size();
    return ret;
} // ibis::selectClause::decodeAName

/// Add a top-level term.  It invokes ibis::selectClause::addRecursive to
/// do the actual work.  The final expression returned by addRecursive is
/// added to  xtms_.
void ibis::selectClause::addTerm(ibis::math::term *tm) {
    if (tm == 0) return;

    xtms_.push_back(addRecursive(tm));
} // ibis::selectClause::addTerm

/// Does the math expression contain any aggregation operations?
bool ibis::selectClause::hasAggregation(const ibis::math::term *tm) const {
    switch (tm->termType()) {
    default:
    case ibis::math::NUMBER:
    case ibis::math::STRING:
	return false;
    case ibis::math::VARIABLE:
	return (dynamic_cast<const ibis::selectClause::variable *>(tm) != 0);
    case ibis::math::STDFUNCTION1:
    case ibis::math::CUSTOMFUNCTION1:
	return hasAggregation(reinterpret_cast<const ibis::math::term*>(tm->getLeft()));
    case ibis::math::OPERATOR:
    case ibis::math::STDFUNCTION2:
    case ibis::math::CUSTOMFUNCTION2:
	bool res = hasAggregation(reinterpret_cast<const ibis::math::term*>(tm->getLeft()));
	if (! res)
	    res = hasAggregation(reinterpret_cast<const ibis::math::term*>(tm->getRight()));
	return res;
    }
} // ibis::selectClause::hasAggregation

ibis::math::term* ibis::selectClause::addRecursive(ibis::math::term*& tm) {
    if (tm == 0) return tm;

    switch (tm->termType()) {
    default:
    case ibis::math::NUMBER:
    case ibis::math::STRING:
	break; // nothing to do
    case ibis::math::VARIABLE: {
	ibis::selectClause::variable *var =
	    dynamic_cast<ibis::selectClause::variable *>(tm);
	if (var == 0) { // a bare variable
	    const char* vname =
		static_cast<ibis::math::variable*>(tm)->variableName();
	    if (ordered_.find(vname) == ordered_.end()) {
		const unsigned pos = atms_.size();
		aggr_.push_back(ibis::selectClause::NIL_AGGR);
		atms_.push_back(tm->dup());
		ordered_[vname] = pos;
	    }
	}
	break;}
    case ibis::math::STDFUNCTION1:
    case ibis::math::CUSTOMFUNCTION1: {
	ibis::math::term *nxt =
	    reinterpret_cast<ibis::math::term*>(tm->getLeft());
	if (nxt == 0) {
	    return nxt;
	}
	else if (hasAggregation(nxt)) {
	    ibis::math::term *tmp = addRecursive(nxt);
	    if (tmp != nxt)
		tm->getLeft() = tmp;
	}
	else {
	    const unsigned pos = atms_.size();
	    aggr_.push_back(ibis::selectClause::NIL_AGGR);
	    atms_.push_back(tm);
	    std::ostringstream oss;
	    oss << "__" << pos;
	    ordered_[oss.str()] = pos;
	    return new ibis::selectClause::variable(oss.str().c_str(), *this);
	}
	break;}
    case ibis::math::OPERATOR:
    case ibis::math::STDFUNCTION2:
    case ibis::math::CUSTOMFUNCTION2: {
	ibis::math::term *left =
	    reinterpret_cast<ibis::math::term*>(tm->getLeft());
	ibis::math::term *right =
	    reinterpret_cast<ibis::math::term*>(tm->getRight());
	if (left == 0 || right == 0) {
	    return 0;
	}
	else if (dynamic_cast<ibis::selectClause::variable*>(left) != 0) {
	    if (dynamic_cast<ibis::selectClause::variable*>(right) == 0) {
		tm->getRight() = addRecursive(right);
	    }
	}
	else if (dynamic_cast<ibis::selectClause::variable*>(right) != 0) {
	    tm->getLeft() = addRecursive(left);
	}
	else if (hasAggregation(tm)) {
	    tm->getLeft() = addRecursive(left);
	    tm->getRight() = addRecursive(right);
	}
	else {
	    const unsigned pos = atms_.size();
	    aggr_.push_back(ibis::selectClause::NIL_AGGR);
	    atms_.push_back(tm);
	    std::ostringstream oss;
	    oss << "__" << pos;
	    ordered_[oss.str()] = pos;
	    return new ibis::selectClause::variable(oss.str().c_str(), *this);
	}
	break;}
    }
    return tm;
} // ibis::selectClause::addRecursive

/// Locate the position of the string.  Upon successful completion, it
/// returns the position of the term with the matching name, otherwise, it
/// returns -1.  The incoming argument may be an alias, a column name, or
/// the exact form of the arithmetic expression.  In case it is an
/// arithmetic expression, it must be exactly the same as the original term
/// passed to the constructor of this class including spaces.  The
/// comparison is done with case-insensitive string comparison.
int ibis::selectClause::find(const char* key) const {
    int ret = -1;
    if (key != 0 && *key != 0) {
	StringToInt::const_iterator it = xalias_.find(key);
	if (it != xalias_.end()) {
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
		for (unsigned int i = 0; i < atms_.size(); ++ i) {
		    std::ostringstream oss;
		    switch (aggr_[i]) {
		    default:
			oss << *(atms_[i]);
			break;
		    case AVG:
			oss << "AVG(" << *(atms_[i]) << ')';
			break;
		    case CNT:
			oss << "COUNT(" << *(atms_[i]) << ')';
			break;
		    case MAX:
			oss << "MAX(" << *(atms_[i]) << ')';
			break;
		    case MIN:
			oss << "MIN(" << *(atms_[i]) << ')';
			break;
		    case SUM:
			oss << "SUM(" << *(atms_[i]) << ')';
			break;
		    case VARPOP:
			oss << "VARPOP(" << *(atms_[i]) << ')';
			break;
		    case VARSAMP:
			oss << "VARSAMP(" << *(atms_[i]) << ')';
			break;
		    case STDPOP:
			oss << "STDPOP(" << *(atms_[i]) << ')';
			break;
		    case STDSAMP:
			oss << "STDSAMP(" << *(atms_[i]) << ')';
			break;
		    case DISTINCT:
			oss << "COUNTDISTINCT(" << *(atms_[i]) << ')';
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
    std::vector<const std::string*> aliases(xtms_.size(), 0);
    for (StringToInt::const_iterator it = xalias_.begin();
	 it != xalias_.end(); ++ it) {
	aliases[(*it).second] = &(it->first);
    }

    for (uint32_t i = 0; i < xtms_.size(); ++ i) {
	if (i > 0)
	    out << ", ";
	out << *(atms_[i]);
	if (aliases[i] != 0)
	    out << " AS " << *(aliases[i]);
    }
} // ibis::selectClause::print

void ibis::selectClause::getNullMask(const ibis::part& part0,
				     ibis::bitvector& mask) const {
    if (atms_.size() > 0) {
	ibis::part::barrel bar(&part0);
	for (uint32_t j = 0; j < atms_.size(); ++ j)
	    bar.recordVariable(atms_[j]);
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

/// Verify the select clause is valid against the given data partition.
/// Returns the number of variables that are not in the data partition.
/// This function also simplifies the arithmetic expression if
/// ibis::math::preserveInputExpression is not set.
///
/// @note Simplifying the arithmetic expressions typically reduces the time
/// needed for evaluations, but may introduce a different set of round-off
/// erros in the evaluation process than the original expression.  Set the
/// variable ibis::math::preserveInputExpression to true to avoid this
/// change in error round-off property.
int ibis::selectClause::verify(const ibis::part& part0) const {
    int ierr = 0;
    for (uint32_t j = 0; j < atms_.size(); ++ j) {
	if (ibis::math::preserveInputExpressions == false) {
	    ibis::math::term *tmp = atms_[j]->reduce();
	    if (tmp != atms_[j]) {
		delete const_cast<ibis::math::term*>(atms_[j]);
		const_cast<mathTerms&>(atms_)[j] = tmp;
	    }
	}
	ierr += verifyTerm(*(atms_[j]), part0, this);
    }
    return ierr;
} // ibis::selectClause::verify

/// Verify the selected terms.  Return the number of terms containing
/// unknown names.
int ibis::selectClause::verifySome(const std::vector<uint32_t>& touse,
				   const ibis::part& part0) const {
    int ierr = 0;
    for (uint32_t j = 0; j < touse.size(); ++ j) {
	if (ibis::math::preserveInputExpressions == false) {
	    ibis::math::term *tmp = atms_[touse[j]]->reduce();
	    if (tmp != atms_[touse[j]]) {
		delete const_cast<ibis::math::term*>(atms_[touse[j]]);
		const_cast<mathTerms&>(atms_)[touse[j]] = tmp;
	    }
	}
	ierr += verifyTerm(*(atms_[touse[j]]), part0, this);
    }
    return ierr;
} // ibis::selectClause::verifySome

/// Verify the specified term has valid column names.  It returns the
/// number of terms not in the given data partition.
int ibis::selectClause::verifyTerm(const ibis::math::term& xp0,
				   const ibis::part& part0,
				   const ibis::selectClause* sel0) {
    int ierr = 0;

    if (xp0.termType() == ibis::math::VARIABLE) {
	const ibis::math::variable& var =
	    static_cast<const ibis::math::variable&>(xp0);
	if (*(var.variableName()) != '*') {
	    if (part0.getColumn(var.variableName()) == 0) {
		bool alias = false;
		if (sel0 != 0) {
		    int as = sel0->find(var.variableName());
		    if (as >= 0 && (unsigned)as < sel0->size())
			alias = (part0.getColumn(sel0->argName(as)) != 0);
		}
		if (! alias) {
		    ++ ierr;
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- selectClause::verifyTerm can NOT find "
			"a column named " << var.variableName()
			<< " in data partition " << part0.name();
		}
	    }
	}
    }
    else if (xp0.termType() == ibis::math::UNDEF_TERM) {
	++ ierr;
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- selectClause::verifyTerm can not work with a "
	    "math::term of undefined type";
    }
    else {
	if (xp0.getLeft() != 0)
	    ierr += verifyTerm(*static_cast<const ibis::math::term*>
			       (xp0.getLeft()), part0, sel0);
	if (xp0.getRight() != 0)
	    ierr += verifyTerm(*static_cast<const ibis::math::term*>
			       (xp0.getRight()), part0, sel0);
    }

    return ierr;
} // ibis::selectClause::verifyTerm

void ibis::selectClause::variable::print(std::ostream& out) const {
    const uint64_t itrm = sc_.decodeAName(name);
    if (itrm >= sc_.atms_.size()) {
	// assume to be a bare arithmetic expression
	out << *(sc_.atms_[itrm]);
	return;
    }

    switch (sc_.aggr_[itrm]) {
    default:
    case ibis::selectClause::NIL_AGGR:
	out << *(sc_.atms_[itrm]);
	break;
    case ibis::selectClause::AVG:
	out << "AVG(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::CNT:
	out << "COUNT(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::MAX:
	out << "MAX(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::MIN:
	out << "MIN(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::SUM:
	out << "SUM(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::VARPOP:
	out << "VARPOP(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::VARSAMP:
	out << "VARSAMP(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::STDPOP:
	out << "STDPOP(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::STDSAMP:
	out << "STDSAMP(" << *(sc_.atms_[itrm]) << ')';
	break;
    case ibis::selectClause::DISTINCT:
	out << "COUNTDISTINCT(" << *(sc_.atms_[itrm]) << ')';
	break;
    }
} // ibis::selectClause::variable::print

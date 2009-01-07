// $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
#include "part.h"	// ibis::part, used by verify and amplify
#include "whereLexer.h"
#include "whereClause.h"

ibis::whereClause::~whereClause() {
    delete expr_;
} // destructor

ibis::whereClause::whereClause(const char* cl) : expr_(0) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	clause_ = cl;
	std::istringstream iss(clause_);
	ibis::util::logger lg(0);
	whereLexer lx(&iss, &(lg.buffer()));
	whereParser parser(*this);
	lexer = &lx;
#if defined(DEBUG) && DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());
	ierr = parser.parse();
	lexer = 0;
    }
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- whereClause(" << cl
	    << ") failed to parse the string into an expression tree";
	if (expr_ != 0) {
	    delete expr_;
	    expr_ = 0;
	}
    }
} // constructor

ibis::whereClause::whereClause(const ibis::whereClause& rhs)
    : clause_(rhs.clause_), expr_(0) {
    if (rhs.expr_ != 0)
	expr_ = rhs.expr_->dup();
} // copy constructor

ibis::whereClause& ibis::whereClause::operator=(const ibis::whereClause& rhs) {
    ibis::whereClause tmp(rhs);
    swap(tmp);
    return *this;
} // assignment operator

int ibis::whereClause::parse(const char* cl) {
    int ierr = 0;
    if (cl != 0 && *cl != 0) {
	clause_ = cl;
	std::istringstream iss(clause_);
	ibis::util::logger lg(0);
	whereLexer lx(&iss, &(lg.buffer()));
	whereParser parser(*this);
	lexer = &lx;
#if defined(DEBUG) && DEBUG+0 > 2
	parser.set_debug_level(DEBUG-1);
#endif
	parser.set_debug_stream(lg.buffer());

	delete expr_;
	expr_ = 0;

	ierr = parser.parse();
	lexer = 0;
    }
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- whereClause(" << cl
	    << ") failed to parse the string into an expression tree";
	if (expr_ != 0) {
	    delete expr_;
	    expr_ = 0;
	}
    }
    return ierr;
} // ibis::whereClause::parse

void ibis::whereClause::clear() throw () {
    clause_.clear();
    delete expr_;
    expr_ = 0;
} // ibis::whereClause::clear

int ibis::whereClause::verify(const ibis::part& part0) {
    if (expr_ != 0) {
	ibis::qExpr::simplify(expr_);
	amplify(part0);
	return _verify(part0, expr_);
    }
    else {
	return 0;
    }
} // ibis::whereClause::verify

/// The actual work horse of the verify function.  This function check each
/// variable name specified in the query conditions to make sure they all
/// appear as names of the columns of the given data partition.  It returns
/// the number of names NOT in the data partition.  It is also the function
/// that converts expressions of the form "string1 = string2" to string
/// lookups when one of the strings is a name of a string-valued column.
int ibis::whereClause::_verify(const ibis::part& part0, ibis::qExpr *&xp0) {
    int ierr = 0;

    switch (xp0->getType()) {
    case ibis::qExpr::RANGE: {
	ibis::qContinuousRange* range =
	    static_cast<ibis::qContinuousRange*>(xp0);
	if (range->colName()) { // allow name to be NULL
	    const ibis::column* col = part0.getColumn(range->colName());
	    if (col == 0) {
		++ ierr;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::whereClause::verify -- data partition "
		    << part0.name() << " does not contain a column named "
		    << range->colName();
	    }
	    else if (col->type() == ibis::FLOAT) {
		// reduce the precision of the bounds
		range->leftBound() =
		    static_cast<float>(range->leftBound());
		range->rightBound() =
		    static_cast<float>(range->rightBound());
	    }
	}
	break;}
    case ibis::qExpr::STRING: {
	const ibis::qString* str =
	    static_cast<const ibis::qString*>(xp0);
	if (str->leftString()) { // allow name to be NULL
	    const ibis::column* col =
		part0.getColumn(str->leftString());
	    if (col == 0) {
		++ ierr;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::whereClause::verify -- data partition "
		    << part0.name() << " does not contain a column named "
		    << str->leftString();
	    }
	}
	break;}
    case ibis::qExpr::MATHTERM: {
	ibis::math::term* math =
	    static_cast<ibis::math::term*>(xp0);
	if (math->termType() == ibis::math::VARIABLE) {
	    const ibis::math::variable* var =
		static_cast<const ibis::math::variable*>(math);
	    const ibis::column* col =
		part0.getColumn(var->variableName());
	    if (col == 0) {
		++ ierr;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::whereClause::verify -- data partition "
		    << part0.name() << " does not contain a column named "
		    << var->variableName();
	    }
	}
	ibis::qExpr *tmp = math->getLeft();
	if (tmp != 0)
	    ierr += _verify(part0, tmp);
	tmp = math->getRight();
	if (tmp != 0)
	    ierr += _verify(part0, tmp);
	break;}
    case ibis::qExpr::COMPRANGE: {
	// compRange have three terms rather than two
	if (reinterpret_cast<ibis::compRange*>(xp0)
	    ->maybeStringCompare()) {
	    const ibis::math::variable *v1 =
		reinterpret_cast<const ibis::math::variable*>
		(xp0->getLeft());
	    const ibis::math::variable *v2 =
		reinterpret_cast<const ibis::math::variable*>
		(xp0->getRight());
	    const ibis::column *c1 =
		part0.getColumn(v1->variableName());
	    const ibis::column *c2 =
		part0.getColumn(v2->variableName());
	    if (c1 != 0) {
		if (c2 == 0) {
		    if (c1->type() == ibis::TEXT ||
			c1->type() == ibis::CATEGORY) {
			LOGGER(ibis::gVerbose > 3)
			    << "ibis::whereClause::verify -- replacing ("
			    << v1->variableName() << " = "
			    << v2->variableName() << ") with ("
			    << v1->variableName() << " = \""
			    << v2->variableName() << "\")";
			ibis::qString *tmp = new
			    ibis::qString(v1->variableName(),
					  v2->variableName());
			delete xp0;
			xp0 = tmp;
		    }
		    else {
			++ ierr;
			LOGGER(ibis::gVerbose > 0)
			    << "ibis::whereClause::verify -- expected column \""
			    << v1->variableName() << "\" to be of string type, "
			    << "but it is %s" << ibis::TYPESTRING[c1->type()];
		    }
		}
	    }
	    else if (c2 != 0) {
		if (c2->type() == ibis::TEXT ||
		    c2->type() == ibis::CATEGORY) {
		    LOGGER(ibis::gVerbose > 3)
			<< "ibis::whereClause::verify -- replacing ("
			<< v2->variableName() << " = " << v1->variableName()
			<< ") with (" << v2->variableName() << " = \""
			<< v1->variableName() << "\")";
		    ibis::qString *tmp = new
			ibis::qString(v2->variableName(),
				      v1->variableName());
		    delete xp0;
		    xp0 = tmp;
		}
		else {
		    ++ ierr;
		    LOGGER(ibis::gVerbose > 0)
			<< "ibis::whereClause::verify -- expected column \""
			<<  v2->variableName() <<  "\" to be of string type, "
			<< "but it is " << ibis::TYPESTRING[c2->type()];
		}
	    }
	    else {
		ierr += 2;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::whereClause::verify -- neither "
		    << v1->variableName() << " or " << v2->variableName()
		    << " are columns names of table " << part0.name();
	    }
	}
	else {
	    if (xp0->getLeft() != 0)
		ierr = _verify(part0, xp0->getLeft());
	    if (xp0->getRight() != 0)
		ierr += _verify(part0, xp0->getRight());
	    if (reinterpret_cast<ibis::compRange*>(xp0)->getTerm3() != 0) {
		ibis::qExpr *cr = reinterpret_cast<ibis::compRange*>
		    (xp0)->getTerm3();
		ierr += _verify(part0, cr);
	    }
	}
	break;}
    case ibis::qExpr::DRANGE : {
	ibis::qDiscreteRange *range =
	    reinterpret_cast<ibis::qDiscreteRange*>(xp0);
	if (range->colName()) { // allow name to be NULL
	    const ibis::column* col = part0.getColumn(range->colName());
	    if (col == 0) {
		++ ierr;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::whereClause::verify -- data partition "
		    << part0.name() << " does not contain a column named "
		    << range->colName();
	    }
	    else if (col->type() == ibis::FLOAT) {
		// reduce the precision of the bounds
		std::vector<double>& val = range->getValues();
		for (std::vector<double>::iterator it = val.begin();
		     it != val.end(); ++ it)
		    *it = static_cast<float>(*it);
	    }
	}
	break;}
    case ibis::qExpr::MSTRING : {
	ibis::qMultiString *range =
	    reinterpret_cast<ibis::qMultiString*>(xp0);
	if (range->colName()) { // allow name to be NULL
	    const ibis::column* col = part0.getColumn(range->colName());
	    if (col == 0) {
		++ ierr;
		LOGGER(ibis::gVerbose > 0)
		    << "ibis::whereClause::verify -- data partition "
		    << part0.name() << " does not contain a column named "
		    << range->colName();
	    }
	}
	break;}
    case ibis::qExpr::JOIN : {
	ibis::rangeJoin *rj = reinterpret_cast<ibis::rangeJoin*>(xp0);
	const ibis::column* c1 = part0.getColumn(rj->getName1());
	if (c1 == 0) {
	    ++ ierr;
	    LOGGER(ibis::gVerbose > 0)
		<< "ibis::whereClause::verify -- data partition "
		<< part0.name() << " does not contain a column named "
		<< rj->getName1();
	}
	const ibis::column* c2 = part0.getColumn(rj->getName2());
	if (c2 == 0) {
	    ++ ierr;
	    LOGGER(ibis::gVerbose > 0)
		<< "ibis::whereClause::verify -- data partition "
		<< part0.name() << " does not contain a column named "
		<< rj->getName2();
	}
	ibis::qExpr *t = rj->getRange();
	ierr += _verify(part0, t);
	break;}
    default: {
	if (xp0->getLeft() != 0)
	    ierr = _verify(part0, xp0->getLeft());
	if (xp0->getRight() != 0)
	    ierr += _verify(part0, xp0->getRight());
	break;}
    } // end switch

    return ierr;
} // ibis::whereClause::_verify

/// @note This name is intentionally vague to discourage its use.  It might
/// be completely removed in a later release.
void ibis::whereClause::amplify(const ibis::part& part0) {
    std::vector<const ibis::rangeJoin*> terms;
    expr_->extractJoins(terms);
    if (terms.empty()) // no join terms to use
	return;

    LOGGER(ibis::gVerbose >= 7)
	<< "ibis::whereClause::amplify -- current query expression\n" << *expr_;

    for (uint32_t i = 0; i < terms.size(); ++ i) {
	const ibis::rangeJoin* jn = terms[i];
	double delta = 0.0;
	if (jn->getRange()) {
	    const ibis::math::term *tm = jn->getRange();
	    if (tm != 0) {
		if (tm->termType() != ibis::math::NUMBER)
		    continue;
		else
		    delta = tm->eval();
	    }
	}

	const char *nm1 = jn->getName1();
	const char *nm2 = jn->getName2();
	const ibis::column *col1 = part0.getColumn(nm1);
	const ibis::column *col2 = part0.getColumn(nm2);
	if (col1 == 0 || col2 == 0)
	    continue;

	double cmin1 = col1->getActualMin();
	double cmax1 = col1->getActualMax();
	double cmin2 = col2->getActualMin();
	double cmax2 = col2->getActualMax();
	ibis::qRange* cur1 = expr_->findRange(nm1);
	ibis::qRange* cur2 = expr_->findRange(nm2);
	if (cur1) {
	    double tmp = cur1->leftBound();
	    if (tmp > cmin1)
		cmin1 = tmp;
	    tmp = cur1->rightBound();
	    if (tmp < cmax1)
		cmax1 = tmp;
	}
	if (cur2) {
	    double tmp = cur2->leftBound();
	    if (tmp > cmin2)
		cmin2 = tmp;
	    tmp = cur2->rightBound();
	    if (tmp < cmax2)
		cmax2 = tmp;
	}

	if (cmin1 < cmin2-delta || cmax1 > cmax2+delta) {
	    double bd1 = (cmin1 >= cmin2-delta ? cmin1 : cmin2-delta);
	    double bd2 = (cmax1 <= cmax2+delta ? cmax1 : cmax2+delta);
	    if (cur1) { // reduce the range of an existing range condition
		cur1->restrictRange(bd1, bd2);
	    }
	    else { // add an addition term of nm1
		ibis::qContinuousRange *qcr =
		    new ibis::qContinuousRange(bd1, ibis::qExpr::OP_LE,
					       nm1, ibis::qExpr::OP_LE, bd2);
		ibis::qExpr *qop = new ibis::qExpr(ibis::qExpr::LOGICAL_AND,
						   qcr, expr_->getRight());
		expr_->getRight() = qop;
	    }
	}

	if (cmin2 < cmin1-delta || cmax2 > cmax1+delta) {
	    double bd1 = (cmin2 >= cmin1-delta ? cmin2 : cmin1-delta);
	    double bd2 = (cmax2 <= cmax1+delta ? cmax2 : cmax1+delta);
	    if (cur2) {
		cur2->restrictRange(bd1, bd2);
	    }
	    else {
		ibis::qContinuousRange *qcr =
		    new ibis::qContinuousRange(bd1, ibis::qExpr::OP_LE,
					       nm2, ibis::qExpr::OP_LE, bd2);
		ibis::qExpr *qop = new ibis::qExpr(ibis::qExpr::LOGICAL_AND,
						   qcr, expr_->getLeft());
		expr_->getLeft() = qop;
	    }
	}
    }

    ibis::qExpr::simplify(expr_);
    if (expr_ != 0) // may become nil pointer after simplify
	LOGGER(ibis::gVerbose >= 7)
	    << "ibis::whereClause::amplify -- "
	    "query expression with additional constraints\n" << *expr_;
} // ibis::whereClause::amplify

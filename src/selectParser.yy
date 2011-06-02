/* $Id$ -*- mode: c++ -*- */
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2011 the Regents of the University of California

%code top {
/** \file Defines the parser for the select clause accepted by FastBit
    IBIS.  The definitions are processed through bison.
*/
#include <iostream>
}
%code requires {
#include "selectClause.h"	// class selectClause
}

/* bison declarations */
%require "2.3"
%debug
%error-verbose
 /*%start START*/
%defines
%skeleton "lalr1.cc"
%name-prefix="ibis"
%define "parser_class_name" "selectParser"
%locations
     /*%expect 1*/
%initial-action
{ // initialize location object
    @$.begin.filename = @$.end.filename = &(driver.clause_);
};

%parse-param {class ibis::selectClause& driver}

%union {
    int			integerVal;
    double		doubleVal;
    std::string 	*stringVal;
    ibis::math::term	*selectNode;
};

%token              END       0 "end of input"
%token <integerVal> ASOP	"as"
%token <integerVal> BITOROP	"|"
%token <integerVal> BITANDOP	"&"
%token <integerVal> ADDOP	"+"
%token <integerVal> MINUSOP	"-"
%token <integerVal> MULTOP	"*"
%token <integerVal> DIVOP	"/"
%token <integerVal> REMOP	"%"
%token <integerVal> EXPOP	"**"
%token <doubleVal> NUMBER	"numerical value"
%token <stringVal> NOUNSTR	"name"

%nonassoc ASOP
%left BITOROP
%left BITANDOP
%left ADDOP MINUSOP
%left MULTOP DIVOP REMOP
%right EXPOP

%type <selectNode> mathExpr

%destructor { delete $$; } NOUNSTR
%destructor { delete $$; } mathExpr

%{
#include "selectLexer.h"

#undef yylex
#define yylex driver.lexer->lex
%}

%% /* Grammar rules */
slist: sterm | sterm slist;
sterm: mathExpr ',' {
    driver.addTerm($1, 0);
}
| mathExpr END {
    driver.addTerm($1, 0);
}
| mathExpr NOUNSTR ',' {
    driver.addTerm($1, $2);
    delete $2;
}
| mathExpr NOUNSTR END {
    driver.addTerm($1, $2);
    delete $2;
}
| mathExpr ASOP NOUNSTR ',' {
    driver.addTerm($1, $3);
    delete $3;
}
| mathExpr ASOP NOUNSTR END {
    driver.addTerm($1, $3);
    delete $3;
}
;

mathExpr:
mathExpr ADDOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " + " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::PLUS);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr MINUSOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " - " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MINUS);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr MULTOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " * " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MULTIPLY);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr DIVOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " / " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::DIVIDE);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr REMOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " % " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::REMAINDER);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr EXPOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " ^ " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::POWER);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr BITANDOP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " & " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITAND);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| mathExpr BITOROP mathExpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " | " << *$3;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITOR);
    opr->setRight($3);
    opr->setLeft($1);
    $$ = opr;
}
| NOUNSTR '(' MULTOP ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << "(*)";
#endif
    ibis::math::term *fun = 0;
    if (stricmp($1->c_str(), "count") == 0) { // aggregation count
	ibis::math::variable *var = new ibis::math::variable("*");
	fun = driver.addAgregado(ibis::selectClause::CNT, var);
    }
    else {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- only operator COUNT supports * as the argument, "
	    "but received " << *$1;
	throw "invalid use of (*) as an argument";
    }
    delete $1;
    $$ = fun;
}
| NOUNSTR '(' mathExpr ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << "("
	<< *$3 << ")";
#endif
    ibis::math::term *fun = 0;
    if (stricmp($1->c_str(), "avg") == 0) { // aggregation avg
	fun = driver.addAgregado(ibis::selectClause::AVG, $3);
    }
    else if (stricmp($1->c_str(), "count") == 0) { // aggregation count
	fun = driver.addAgregado(ibis::selectClause::CNT, $3);
    }
    else if (stricmp($1->c_str(), "max") == 0) { // aggregation max
	fun = driver.addAgregado(ibis::selectClause::MAX, $3);
    }
    else if (stricmp($1->c_str(), "min") == 0) { // aggregation min
	fun = driver.addAgregado(ibis::selectClause::MIN, $3);
    }
    else if (stricmp($1->c_str(), "sum") == 0) { // aggregation sum
	fun = driver.addAgregado(ibis::selectClause::SUM, $3);
    }
    else if (stricmp($1->c_str(), "median") == 0) { // aggregation median
	fun = driver.addAgregado(ibis::selectClause::MEDIAN, $3);
    }
    else if (stricmp($1->c_str(), "distinct") == 0 ||
	     stricmp($1->c_str(), "countdistinct") == 0) { // count distinct values
	fun = driver.addAgregado(ibis::selectClause::DISTINCT, $3);
    }
    else if (stricmp($1->c_str(), "varp") == 0 ||
	     stricmp($1->c_str(), "varpop") == 0) { // population variance
	fun = driver.addAgregado(ibis::selectClause::VARPOP, $3);
    }
    else if (stricmp($1->c_str(), "var") == 0 ||
	     stricmp($1->c_str(), "varsamp") == 0 ||
	     stricmp($1->c_str(), "variance") == 0) { // sample variance
	fun = driver.addAgregado(ibis::selectClause::VARSAMP, $3);
    }
    else if (stricmp($1->c_str(), "stdevp") == 0 ||
	     stricmp($1->c_str(), "stdpop") == 0) { // population standard deviation
	fun = driver.addAgregado(ibis::selectClause::STDPOP, $3);
    }
    else if (stricmp($1->c_str(), "std") == 0 ||
	     stricmp($1->c_str(), "stdev") == 0 ||
	     stricmp($1->c_str(), "stdsamp") == 0) { // sample standard deviation
	fun = driver.addAgregado(ibis::selectClause::STDSAMP, $3);
    }
    else { // standard math function
	fun = new ibis::math::stdFunction1($1->c_str());
	fun->setLeft($3);
    }
    delete $1;
    $$ = fun;
}
| NOUNSTR '(' mathExpr ',' mathExpr ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << "("
	<< *$3 << ", " << *$5 << ")";
#endif
    ibis::math::stdFunction2 *fun =
	new ibis::math::stdFunction2($1->c_str());
    fun->setRight($5);
    fun->setLeft($3);
    $$ = fun;
    delete $1;
}
| MINUSOP mathExpr %prec EXPOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *$2;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight($2);
    $$ = opr;
}
| ADDOP mathExpr %prec EXPOP {
    $$ = $2;
}
| '(' mathExpr ')' {
    $$ = $2;
}
| NOUNSTR {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *$1;
#endif
    $$ = new ibis::math::variable($1->c_str());
    delete $1;
}
| NUMBER {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << $1;
#endif
    $$ = new ibis::math::number($1);
}
;

%%
void ibis::selectParser::error(const ibis::selectParser::location_type& l,
			       const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::selectParser encountered " << m << " at location "
	<< l;
}

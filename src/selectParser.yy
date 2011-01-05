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
%token <integerVal> AVGOP	"avg"
%token <integerVal> CNTOP	"count"
%token <integerVal> MINOP	"min"
%token <integerVal> MAXOP	"max"
%token <integerVal> SUMOP	"sum"
%token <integerVal> VARPOPOP	"varpop"
%token <integerVal> VARSAMPOP	"varsamp"
%token <integerVal> STDPOPOP	"stdpop"
%token <integerVal> STDSAMPOP	"stdsamp"
%token <integerVal> DISTINCTOP	"distinct"
%token <integerVal> MEDIANOP	"median"
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
sterm: AVGOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
| AVGOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
| CNTOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| CNTOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| CNTOP '(' MULTOP ')' ',' {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| CNTOP '(' MULTOP ')' END {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| MAXOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
| MAXOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
| MINOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
| MINOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
| SUMOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
| SUMOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
| VARPOPOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
| VARPOPOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
| VARSAMPOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
| VARSAMPOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
| STDPOPOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
| STDPOPOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
| STDSAMPOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
| STDSAMPOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
| DISTINCTOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
| DISTINCTOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
| MEDIANOP '(' mathExpr ')' ',' {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
| MEDIANOP '(' mathExpr ')' END {
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
| mathExpr ',' {
    driver.terms_.push_back($1);
    driver.aggr_.push_back(ibis::selectClause::NIL);
}
| mathExpr END {
    driver.terms_.push_back($1);
    driver.aggr_.push_back(ibis::selectClause::NIL);
}
| AVGOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
| AVGOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
| CNTOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| CNTOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| CNTOP '(' MULTOP ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| CNTOP '(' MULTOP ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
| MAXOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
| MAXOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
| MINOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
| MINOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
| SUMOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
| SUMOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
| mathExpr ASOP NOUNSTR ',' {
    driver.alias_[*$3] = driver.terms_.size();
    driver.terms_.push_back($1);
    driver.aggr_.push_back(ibis::selectClause::NIL);
}
| mathExpr ASOP NOUNSTR END {
    driver.alias_[*$3] = driver.terms_.size();
    driver.terms_.push_back($1);
    driver.aggr_.push_back(ibis::selectClause::NIL);
}
| DISTINCTOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
| DISTINCTOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
| MEDIANOP '(' mathExpr ')' ASOP NOUNSTR ',' {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
| MEDIANOP '(' mathExpr ')' ASOP NOUNSTR END {
    driver.alias_[*$6] = driver.terms_.size();
    driver.terms_.push_back($3);
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
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
| NOUNSTR '(' mathExpr ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << "("
	<< *$3 << ")";
#endif
    ibis::math::stdFunction1 *fun =
	new ibis::math::stdFunction1($1->c_str());
    delete $1;
    fun->setLeft($3);
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
	<< "Warning -- ibis::selectParser encountered " << m << " at location " << l;
}

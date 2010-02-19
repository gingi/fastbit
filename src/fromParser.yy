/* $Id$ -*- mode: c++ -*- */
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2009-2010 the Regents of the University of California

%code top {
/** \file Defines the parser for the from clause accepted by FastBit
    IBIS.  The definitions are processed through bison.
*/
#include <iostream>
}
%code requires {
#include "fromClause.h"	// class fromClause
}

/* bison declarations */
%require "2.3"
%debug
%error-verbose
 /*%start START*/
%defines
%skeleton "lalr1.cc"
%name-prefix="ibis"
%define "parser_class_name" "fromParser"
%locations
     /*%expect 1*/
%initial-action
{ // initialize location object
    @$.begin.filename = @$.end.filename = &(driver.clause_);
};

%parse-param {class ibis::fromClause& driver}

%union {
    int		integerVal;
    double	doubleVal;
    std::string	*stringVal;
    ibis::qExpr	*fromNode;
};

%token              END       0 "end of input"
%token <integerVal> ASOP	"as"
%token <integerVal> ONOP	"on"
%token <integerVal> JOINOP	"join"
%token <integerVal> USINGOP	"using"
%token <integerVal> BETWEENOP	"between"
%token <integerVal> ANDOP	"and"
%token <integerVal> LEOP	"<="
%token <integerVal> GEOP	">="
%token <integerVal> LTOP	"<"
%token <integerVal> GTOP	">"
%token <integerVal> EQOP	"=="
%token <integerVal> NEQOP	"!="
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
%nonassoc ONOP
%nonassoc JOINOP
%nonassoc USINGOP
%left BITOROP
%left BITANDOP
%left ADDOP MINUSOP
%left MULTOP DIVOP REMOP
%right EXPOP

%type <fromNode> compRange compRange2 compRange3 mathExpr

%destructor { delete $$; } NOUNSTR
%destructor { delete $$; } compRange2 compRange3 mathExpr

%{
#include "fromLexer.h"

#undef yylex
#define yylex driver.lexer->lex
%}

%% /* Grammar rules */
flist: fterm | fterm flist;
fterm: NOUNSTR ',' {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
}
| NOUNSTR END {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
}
| NOUNSTR NOUNSTR ',' {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$2);
}
| NOUNSTR NOUNSTR END {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$2);
}| NOUNSTR ASOP NOUNSTR ',' {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
}
| NOUNSTR ASOP NOUNSTR END {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
}
| NOUNSTR JOINOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back("");
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back("");
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back(*$7);
}
| NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back(*$5);
}
| NOUNSTR JOINOP NOUNSTR USINGOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back("");
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($5->c_str()));
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR USINGOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back("");
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($7->c_str()));
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR USINGOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back(*$7);
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($9->c_str()));
}
| NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR USINGOP NOUNSTR END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back(*$5);
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($7->c_str()));
}
| NOUNSTR JOINOP NOUNSTR USINGOP '(' NOUNSTR ')' END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back("");
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($6->c_str()));
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR USINGOP '(' NOUNSTR ')' END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back("");
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($8->c_str()));
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR USINGOP '(' NOUNSTR ')' END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back(*$7);
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($10->c_str()));
}
| NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR USINGOP '(' NOUNSTR ')' END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back(*$5);
    driver.jcond_ = new ibis::compRange();
    driver.jcond_->setTerm3(new ibis::math::variable($8->c_str()));
}
| NOUNSTR JOINOP NOUNSTR ONOP compRange END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>($5);
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR ONOP compRange END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>($7);
}
| NOUNSTR ASOP NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR ONOP compRange END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back(*$3);
    driver.names_.push_back(*$5);
    driver.aliases_.push_back(*$7);
    driver.jcond_ = dynamic_cast<ibis::compRange*>($9);
}
| NOUNSTR JOINOP NOUNSTR ASOP NOUNSTR ONOP compRange END {
    driver.names_.push_back(*$1);
    driver.aliases_.push_back("");
    driver.names_.push_back(*$3);
    driver.aliases_.push_back(*$5);
    driver.jcond_ = dynamic_cast<ibis::compRange*>($7);
}
;

compRange: compRange2 | compRange3;
compRange2:
mathExpr EQOP mathExpr {
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " = "
	<< *me2;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2);
}
| mathExpr NEQOP mathExpr {
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " != "
	<< *me2;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2));
}
| mathExpr LTOP mathExpr {
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2);
}
| mathExpr LEOP mathExpr {
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2);
}
| mathExpr GTOP mathExpr {
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_GT, me2);
}
| mathExpr GEOP mathExpr {
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_GE, me2);
}
;

compRange3:
mathExpr LTOP mathExpr LTOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2 << " < " << *me3;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LT, me3);
}
| mathExpr LTOP mathExpr LEOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2 << " <= " << *me3;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LE, me3);
}
| mathExpr LEOP mathExpr LTOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2 << " < " << *me3;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LT, me3);
}
| mathExpr LEOP mathExpr LEOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2 << " <= " << *me3;
#endif
    $$ = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LE, me3);
}
| mathExpr GTOP mathExpr GTOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2 << " > " << *me3;
#endif
    $$ = new ibis::compRange(me3, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LT, me1);
}
| mathExpr GTOP mathExpr GEOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2 << " >= " << *me3;
#endif
    $$ = new ibis::compRange(me3, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LT, me1);
}
| mathExpr GEOP mathExpr GTOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2 << " > " << *me3;
#endif
    $$ = new ibis::compRange(me3, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LE, me1);
}
| mathExpr GEOP mathExpr GEOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2 << " >= " << *me3;
#endif
    $$ = new ibis::compRange(me3, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LE, me1);
}
| mathExpr BETWEENOP mathExpr ANDOP mathExpr {
    ibis::math::term *me3 = static_cast<ibis::math::term*>($5);
    ibis::math::term *me2 = static_cast<ibis::math::term*>($3);
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " BETWEEN "
	<< *me2 << " AND " << *me3;
#endif
    $$ = new ibis::compRange(me2, ibis::qExpr::OP_LE, me1,
			     ibis::qExpr::OP_LE, me3);
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
void ibis::fromParser::error(const ibis::fromParser::location_type& l,
			     const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::fromParser encountered " << m << " at location " << l;
}

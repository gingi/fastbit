/* $Id$ -*- mode: c++ -*- */
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California

%{
/** \file Defines the parser for the where clause accepted by FastBit IBIS.
    The definitions are processed through bison.
*/

#include "whereClause.h"	// class whereClause
%}

/* bison declarations */
%require "2.3"
 /*%debug*/
 /*%error-verbose*/
%start START
%defines
%skeleton "lalr1.cc"
%name-prefix="ibis"
%define "parser_class_name" "whereParser"
%locations
     /*%expect 1*/
%initial-action
{ // initialize location object
    @$.begin.filename = @$.end.filename = &(driver.clause_);
};

%parse-param {class ibis::whereClause& driver}

%union {
    int		integerVal;
    double	doubleVal;
    std::string *stringVal;
    ibis::qExpr *whereNode;
}

%token              END       0 "end of input"
%token <integerVal> NOTOP	"not"
%token <integerVal> LEOP	"<="
%token <integerVal> GEOP	">="
%token <integerVal> LTOP	"<"
%token <integerVal> GTOP	">"
%token <integerVal> EQOP	"=="
%token <integerVal> NEQOP	"!="
%token <integerVal> ANDOP	"and"
%token <integerVal> ANDNOTOP	"&!"
%token <integerVal> OROP	"or"
%token <integerVal> XOROP	"xor"
%token <integerVal> BETWEENOP	"between"
%token <integerVal> INOP	"in"
%token <integerVal> LIKEOP	"like"
%token <integerVal> JOINOP	"self-join"
%token <integerVal> ANYOP	"any"
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
%token <stringVal> NUMSEQ	"number sequence"
%token <stringVal> STRSEQ	"string sequence"
%token <stringVal> STRLIT	"string literal"

%left OROP
%left XOROP
%left ANDOP ANDNOTOP
%left EQOP NEQOP
%left BITOROP
%left BITANDOP
%left ADDOP MINUSOP
%left MULTOP DIVOP REMOP
%right EXPOP
%right NOTOP
%nonassoc INOP
%nonassoc ANYOP
%nonassoc JOINOP

%type <whereNode> qexpr simpleRange compRange2 compRange3 mathExpr


%destructor { delete $$; } NOUNSTR NUMSEQ STRSEQ
%destructor { delete $$; } qexpr simpleRange compRange2 compRange3 mathExpr

%{
#include "whereLexer.h"

#undef yylex
#define yylex driver.lexer->lex
%}

%% /* Grammar rules */
qexpr:
qexpr OROP qexpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " || " << *$3;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_OR);
    $$->setRight($3);
    $$->setLeft($1);
}
| qexpr XOROP qexpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " ^ " << *$3;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_XOR);
    $$->setRight($3);
    $$->setLeft($1);
}
| qexpr ANDOP qexpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " && " << *$3;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
    $$->setRight($3);
    $$->setLeft($1);
}
| qexpr ANDNOTOP qexpr {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1
	<< " &~ " << *$3;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_MINUS);
    $$->setRight($3);
    $$->setLeft($1);
}
| NOTOP qexpr %prec NOTOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ! " << *$2;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft($2);
}
| '(' qexpr ')' {
    $$ = $2;
}
| simpleRange
| compRange2
| compRange3
;

simpleRange:
NOUNSTR INOP NUMSEQ {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$3 << ")";
#endif
    $$ = new ibis::qDiscreteRange($1->c_str(), $3->c_str());
    delete $3;
    delete $1;
}
| NOUNSTR INOP '(' NUMBER ',' NUMBER ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< $4 << ", " << $6 << ")";
#endif
    std::vector<double> vals(2);
    vals[0] = $4;
    vals[1] = $6;
    $$ = new ibis::qDiscreteRange($1->c_str(), vals);
    delete $1;
}
| NOUNSTR INOP '(' NUMBER ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< $4 << ")";
#endif
    $$ = new ibis::qContinuousRange($1->c_str(), ibis::qExpr::OP_EQ, $4);
    delete $1;
}
| NOUNSTR NOTOP INOP NUMSEQ {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$4 << ")";
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qDiscreteRange($1->c_str(), $4->c_str()));
    delete $4;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' NUMBER ',' NUMBER ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< $5 << ", " << $7 << ")";
#endif
    std::vector<double> vals(2);
    vals[0] = $5;
    vals[1] = $7;
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qDiscreteRange($1->c_str(), vals));
    delete $1;
}
| NOUNSTR NOTOP INOP '(' NUMBER ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< $5 << ")";
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qContinuousRange($1->c_str(), ibis::qExpr::OP_EQ, $5));
    delete $1;
}
| NOUNSTR INOP STRSEQ {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$3 << ")";
#endif
    $$ = new ibis::qMultiString($1->c_str(), $3->c_str());
    delete $3;
    delete $1;
}
| NOUNSTR INOP '(' NOUNSTR ',' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$4 << ", " << *$6 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$4;
    val += "\", \"";
    val += *$6;
    val += '"';
    $$ = new ibis::qMultiString($1->c_str(), val.c_str());
    delete $6;
    delete $4;
    delete $1;
}
| NOUNSTR INOP '(' STRLIT ',' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$4 << ", " << *$6 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$4;
    val += "\", \"";
    val += *$6;
    val += '"';
    $$ = new ibis::qMultiString($1->c_str(), val.c_str());
    delete $6;
    delete $4;
    delete $1;
}
| NOUNSTR INOP '(' NOUNSTR ',' STRLIT ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$4 << ", " << *$6 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$4;
    val += "\", \"";
    val += *$6;
    val += '"';
    $$ = new ibis::qMultiString($1->c_str(), val.c_str());
    delete $6;
    delete $4;
    delete $1;
}
| NOUNSTR INOP '(' STRLIT ',' STRLIT ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$4 << ", " << *$6 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$4;
    val += "\", \"";
    val += *$6;
    val += '"';
    $$ = new ibis::qMultiString($1->c_str(), val.c_str());
    delete $6;
    delete $4;
    delete $1;
}
| NOUNSTR INOP '(' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$4 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$4;
    val += '"';
    $$ = new ibis::qMultiString($1->c_str(), val.c_str());
    delete $4;
    delete $1;
}
| NOUNSTR INOP '(' STRLIT ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " IN ("
	<< *$4 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$4;
    val += '"';
    $$ = new ibis::qMultiString($1->c_str(), val.c_str());
    delete $4;
    delete $1;
}
| NOUNSTR LIKEOP NOUNSTR {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " LIKE ("
	<< *$3 << ")";
#endif
    $$ = new ibis::qLike($1->c_str(), $3->c_str());
    delete $3;
    delete $1;
}
| NOUNSTR LIKEOP STRLIT {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " LIKE ("
	<< *$3 << ")";
#endif
    $$ = new ibis::qLike($1->c_str(), $3->c_str());
    delete $3;
    delete $1;
}
| NOUNSTR NOTOP INOP STRSEQ {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$4 << ")";
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), $4->c_str()));
    delete $4;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' NOUNSTR ',' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$5 << ", " << *$7 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$5;
    val += "\", \"";
    val += *$7;
    val += '"';
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), val.c_str()));
    delete $7;
    delete $5;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' STRLIT ',' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$5 << ", " << *$7 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$5;
    val += "\", \"";
    val += *$7;
    val += '"';
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), val.c_str()));
    delete $7;
    delete $5;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' NOUNSTR ',' STRLIT ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$5 << ", " << *$7 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$5;
    val += "\", \"";
    val += *$7;
    val += '"';
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), val.c_str()));
    delete $7;
    delete $5;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' STRLIT ',' STRLIT ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$5 << ", " << *$7 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$5;
    val += "\", \"";
    val += *$7;
    val += '"';
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), val.c_str()));
    delete $7;
    delete $5;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$5 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$5;
    val += '"';
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), val.c_str()));
    delete $5;
    delete $1;
}
| NOUNSTR NOTOP INOP '(' STRLIT ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << " NOT IN ("
	<< *$5 << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *$5;
    val += '"';
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qMultiString($1->c_str(), val.c_str()));
    delete $5;
    delete $1;
}
| JOINOP '(' NOUNSTR ',' NOUNSTR ')' {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- JOIN(" << *$3 << ", "
	<< *$5 << ")";
#endif
    $$ = new ibis::rangeJoin($3->c_str(), $5->c_str());
    delete $5;
    delete $3;
}
| JOINOP '(' NOUNSTR ',' NOUNSTR ',' mathExpr ')' {
    ibis::math::term *me = static_cast<ibis::math::term*>($7);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- JOIN(" << *$3 << ", "
	<< *$5 << ", " << *me << ")";
#endif
    $$ = new ibis::rangeJoin($3->c_str(), $5->c_str(), me);
    delete $5;
    delete $3;
}
| ANYOP '(' NOUNSTR ')' EQOP NUMBER {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ANY(" << *$3 << ") = "
	<< $6 << ")";
#endif
    $$ = new ibis::qAnyAny($3->c_str(), $6);
    delete $3;
}
| ANYOP '(' NOUNSTR ')' INOP NUMSEQ {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ANY(" << *$3 << ") = "
	<< *$6 << ")";
#endif
    $$ = new ibis::qAnyAny($3->c_str(), $6->c_str());
    delete $6;
    delete $3;
}
;

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
| STRLIT EQOP NOUNSTR %prec INOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$3 << " = "
	<< *$1;
#endif
    $$ = new ibis::qString($3->c_str(), $1->c_str());
    delete $3;
    delete $1;
}
| mathExpr EQOP STRLIT {
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " = "
	<< *$3;
#endif
    ibis::math::variable *var = dynamic_cast<ibis::math::variable*>(me1);
    if (var != 0) {
	$$ = new ibis::qString(var->variableName(), $3->c_str());
	delete $3;
	delete var;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "whereParser.yy: rule mathExpr == 'string literal' is a "
	    "kludge for Name == 'string literal'.  The mathExpr on the "
	    "left can only be variable name, currently " << *me1;
	delete $3;
	delete me1;
	throw "The rule on line 419 in whereParser.yy expects a simple "
	    "variable name on the left-hand side";
    }
}
| STRLIT NEQOP NOUNSTR %prec INOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$3 << " = "
	<< *$1;
#endif
    $$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    $$->setLeft(new ibis::qString($3->c_str(), $1->c_str()));
    delete $3;
    delete $1;
}
| mathExpr NEQOP STRLIT {
    ibis::math::term *me1 = static_cast<ibis::math::term*>($1);
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " != "
	<< *$3;
#endif
    ibis::math::variable *var = dynamic_cast<ibis::math::variable*>(me1);
    if (var != 0) {
	$$ = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
	$$->setLeft(new ibis::qString(var->variableName(), $3->c_str()));
	delete $3;
	delete var;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "whereParser.yy: rule mathExpr != 'string literal' is a "
	    "kludge for Name != 'string literal'.  The mathExpr on the "
	    "left can only be variable name, currently " << *me1;
	delete $3;
	delete me1;
	throw "The rule on line 419 in whereParser.yy expects a simple "
	    "variable name on the left-hand side";
    }
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
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
    $$ = static_cast<ibis::qExpr*>(opr);
}
| NOUNSTR '(' mathExpr ')' %prec JOINOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << "("
	<< *$3 << ")";
#endif
    ibis::math::stdFunction1 *fun =
	new ibis::math::stdFunction1($1->c_str());
    delete $1;
    fun->setLeft($3);
    $$ = static_cast<ibis::qExpr*>(fun);
}
| NOUNSTR '(' mathExpr ',' mathExpr ')' %prec JOINOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *$1 << "("
	<< *$3 << ", " << *$5 << ")";
#endif
    ibis::math::stdFunction2 *fun =
	new ibis::math::stdFunction2($1->c_str());
    fun->setRight($5);
    fun->setLeft($3);
    $$ = static_cast<ibis::qExpr*>(fun);
    delete $1;
}
| MINUSOP mathExpr %prec NOTOP {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *$2;
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight($2);
    $$ = static_cast<ibis::qExpr*>(opr);
}
| ADDOP mathExpr %prec NOTOP {
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
    ibis::math::variable *var =
	new ibis::math::variable($1->c_str());
    $$ = static_cast<ibis::qExpr*>(var);
    delete $1;
}
| NUMBER {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << $1;
#endif
    ibis::math::number *num = new ibis::math::number($1);
    $$ = static_cast<ibis::qExpr*>(num);
}
;

START : qexpr END { /* pass qexpr to the driver */
    driver.expr_ = $1;
}
| qexpr ';' { /* pass qexpr to the driver */
    driver.expr_ = $1;
}
;

%%
void ibis::whereParser::error(const ibis::whereParser::location_type& l,
			      const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::whereParser encountered " << m << " at location " << l;
}

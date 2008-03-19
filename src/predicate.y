%{
/* $Id$*/
/*
  Defines the grammar of the query conditions.

  A set of query conditions is a set of range conditions joined together
  by logical oeprators.
 */
#include "util.h"
#include "qExpr.h"
#include "predicate.h"

#include <stack>
#include <deque>
#include <vector>

#include <string.h>
#include <stdio.h>

int yylex();

// variables with local file scope
static std::stack<ibis::qExpr*, std::deque<ibis::qExpr*> > qexpr_stack;
static ibis::qExpr *pn1 = 0;	// for query expression
%}

%union {
    float fval;
    int   ival;
    char *sval;
}

%token <sval> NOUNSTR
%token <sval> NUMSEQ
%token <sval> NUMSTR
%token <sval> NUMHEX
%token <sval> STRSEQ
%token <sval> LITSTR
%token <ival> NOTOP
%token <ival> LEOP
%token <ival> GEOP
%token <ival> LTOP
%token <ival> GTOP
%token <ival> EQOP
%token <ival> NEQOP
%token <ival> ANDOP
%token <ival> ANDNOTOP
%token <ival> BETWEENOP
%token <ival> INOP
%token <ival> JOINOP
%token <ival> ANYOP
%token <ival> OROP
%token <ival> XOROP
%token <ival> BITOROP
%token <ival> BITANDOP
%token <ival> MINUSOP
%token <ival> MULTOP
%token <ival> DIVOP
%token <ival> REMOP
%token <ival> EXPOP

%left OROP
%left XOROP
%left ANDOP ANDNOTOP
%left BITOROP
%left BITANDOP
%left ADDOP MINUSOP
%left MULTOP DIVOP REMOP
%left EXPOP
%nonassoc STRING_EXPRESSION
%nonassoc INOP
%nonassoc ANYOP
%nonassoc JOINOP
%nonassoc UNOT

%glr-parser

%%
qexpr:  qexpr OROP qexpr   {
    // logical OR
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_OR);
    pn1->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    pn1->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(pn1);
}

|  qexpr XOROP qexpr  {
    // logical XOR
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_XOR);
    pn1->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    pn1->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(pn1);
}

|  qexpr ANDOP qexpr  {
    // logical AND
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
    pn1->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    pn1->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(pn1);
}

|  qexpr ANDNOTOP qexpr  {
    // logical AND NOT
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_MINUS);
    pn1->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    pn1->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(pn1);
}

|  NOTOP  qexpr %prec UNOT   {
    // logical negation
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    pn1->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(pn1);
}

|  '(' qexpr ')'

|   simpleRange1
/*
|   simpleRange2
*/
|   computedRange1

|   computedRange2
;

simpleRange1:
LITSTR EQOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got qString: " << $3 << " == " << $1;
#endif
    pn1 = new ibis::qString($3, $1);
    qexpr_stack.push(pn1);
}

| NOUNSTR EQOP LITSTR %prec STRING_EXPRESSION
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got qString: " << $1 << " == " << $3;
#endif
    pn1 = new ibis::qString($1, $3);
    qexpr_stack.push(pn1);
}

| NOUNSTR INOP NUMSEQ
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << $1 << " IN (" << $3 << ")";
#endif
    pn1 = new ibis::qDiscreteRange($1, $3);
    qexpr_stack.push(pn1);
}

| NOUNSTR INOP STRSEQ
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << $1 << " IN (" << $3 << ")";
#endif
    pn1 = new ibis::qMultiString($1, $3);
    qexpr_stack.push(pn1);
}

| NOUNSTR NOTOP INOP NUMSEQ
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << $1 << " NOT IN (" << $4 << ")";
#endif
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    pn1->setLeft(new ibis::qDiscreteRange($1, $4));
    qexpr_stack.push(pn1);
}

| NOUNSTR NOTOP INOP STRSEQ
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << $1 << " NOT IN (" << $4 << ")";
#endif
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    pn1->setLeft(new ibis::qMultiString($1, $4));
    qexpr_stack.push(pn1);
}

| JOINOP '(' NOUNSTR ',' NOUNSTR ')'
{   // equal join of to variables
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "join(" << $3 << ", " << $5 << ")";
#endif
    pn1 = new ibis::rangeJoin($3, $5);
    qexpr_stack.push(pn1);
}

| JOINOP '(' NOUNSTR ',' NOUNSTR ',' mathexpr ')'
{   // range join of to variables
    ibis::compRange::term *ex = static_cast<ibis::compRange::term*>
	(qexpr_stack.top());
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "join(" << $3 << ", " << $5
	<< ", " << *ex << ")";
#endif
    pn1 = new ibis::rangeJoin($3, $5, ex);
    qexpr_stack.pop(); // remove the expression used in the range join
    qexpr_stack.push(pn1);
}

| ANYOP '(' NOUNSTR ')' EQOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "any(" << $3 << ") = " << $6;
#endif
    pn1 = new ibis::qAnyAny($3, $6);
    qexpr_stack.push(pn1);
}

| ANYOP '(' NOUNSTR ')' INOP NUMSEQ
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "any(" << $3 << ") in " << $6;
#endif
    pn1 = new ibis::qAnyAny($3, $6);
    qexpr_stack.push(pn1);
}
;
/*
| NOUNSTR GTOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " > " << $1;
#endif
    pn1 = new ibis::qRange($3, ibis::qExpr::OP_LT, $1,
			   ibis::qExpr::OP_UNDEFINED, (const char*)0);
    qexpr_stack.push(pn1);
}
| NOUNSTR GEOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " >= " << $1;
#endif
    pn1 = new ibis::qRange($3, ibis::qExpr::OP_LE, $1,
			   ibis::qExpr::OP_UNDEFINED, (const char*)0);
    qexpr_stack.push(pn1);
}
| NOUNSTR LTOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " < " << $1;
#endif
    pn1 = new ibis::qRange((const char*)0, ibis::qExpr::OP_UNDEFINED, $1,
			   ibis::qExpr::OP_LT, $3);
    qexpr_stack.push(pn1);
}
| NOUNSTR LEOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " <= " << $1;
#endif
    pn1 = new ibis::qRange((const char*)0, ibis::qExpr::OP_UNDEFINED, $1,
			   ibis::qExpr::OP_LE, $3);
    qexpr_stack.push(pn1);
}
| NOUNSTR EQOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " == " << $1;
#endif
    pn1 = new ibis::qRange($3, ibis::qExpr::OP_EQ, $1,
			   ibis::qExpr::OP_UNDEFINED, (const char*)0);
    qexpr_stack.push(pn1);
}
| NOUNSTR NEQOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " != " << $1;
#endif
    ibis::qExpr *pn2 = new ibis::qRange($3, ibis::qExpr::OP_EQ, $1,
					ibis::qExpr::OP_UNDEFINED,
					(const char*)0);
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    pn1->setLeft(pn2);
    qexpr_stack.push(pn1);
}
| NUMSTR GTOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " < " << $1;
#endif
    pn1 = new ibis::qRange((const char*)0, ibis::qExpr::OP_UNDEFINED, $3,
			   ibis::qExpr::OP_LT, $1);
    qexpr_stack.push(pn1);
}
| NUMSTR GEOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " <= " << $1;
#endif
    pn1 = new ibis::qRange((const char*)0, ibis::qExpr::OP_UNDEFINED, $3,
			   ibis::qExpr::OP_LE, $1);
    qexpr_stack.push(pn1);
}
| NUMSTR LTOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " > " << $1;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_LT, $3,
			   ibis::qExpr::OP_UNDEFINED, (const char*)0);
    qexpr_stack.push(pn1);
}
| NUMSTR LEOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " >= " << $1;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_LE, $3,
			   ibis::qExpr::OP_UNDEFINED, (const char*)0);
    qexpr_stack.push(pn1);
}
| NUMSTR EQOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " == " << $1;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_EQ, $3,
			   ibis::qExpr::OP_UNDEFINED, (const char*)0);
    qexpr_stack.push(pn1);
}
| NUMSTR NEQOP NOUNSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $3 << " != " << $1;
#endif
    ibis::qExpr *pn2 = new ibis::qRange($1, ibis::qExpr::OP_EQ, $3,
					ibis::qExpr::OP_UNDEFINED,
					(const char*)0);
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    pn1->setLeft(pn2);
    qexpr_stack.push(pn1);
}
;

simpleRange2:
NUMSTR LTOP NOUNSTR LTOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $1 << " < " << $3
	<< " < " << $5;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_LT, $3,
			   ibis::qExpr::OP_LT, $5);
    qexpr_stack.push(pn1);
}
| NUMSTR LEOP NOUNSTR LEOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $1 << " <= " << $3
	<< " <= " << $5;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_LE, $3,
			   ibis::qExpr::OP_LE, $5);
    qexpr_stack.push(pn1);
}
| NUMSTR LEOP NOUNSTR LTOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $1 << " <= " << $3
	<< " < " << $5;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_LE, $3,
			   ibis::qExpr::OP_LT, $5);
    qexpr_stack.push(pn1);
}
| NUMSTR LTOP NOUNSTR LEOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $1 << " < " << $3
	<< " <= " << $5;
#endif
    pn1 = new ibis::qRange($1, ibis::qExpr::OP_LT, $3,
			   ibis::qExpr::OP_LE, $5);
    qexpr_stack.push(pn1);
}
| NUMSTR GTOP NOUNSTR GTOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $5 << " < " << $3
	<< " < " << $1;
#endif
    pn1 = new ibis::qRange($5, ibis::qExpr::OP_LT, $3,
			   ibis::qExpr::OP_LT, $1);
    qexpr_stack.push(pn1);
}
| NUMSTR GEOP NOUNSTR GEOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $5 << " <= " << $3
	<< " <= " << $1;
#endif
    pn1 = new ibis::qRange($5, ibis::qExpr::OP_LE, $3,
			   ibis::qExpr::OP_LE, $1);
    qexpr_stack.push(pn1);
}
| NUMSTR GEOP NOUNSTR GTOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $5 << " <= "
	<< $3 << " < " << $1;
#endif
    pn1 = new ibis::qRange($5, ibis::qExpr::OP_LT, $3,
			   ibis::qExpr::OP_LE, $1);
    qexpr_stack.push(pn1);
}
| NUMSTR GTOP NOUNSTR GEOP NUMSTR
{
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << "RANGE: " << $5 << " < " << $3
	<< " <= " << $1;
#endif
    pn1 = new ibis::qRange($5, ibis::qExpr::OP_LE, $3,
			   ibis::qExpr::OP_LT, $1);
    qexpr_stack.push(pn1);
}
;
*/
computedRange1:
mathexpr LTOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t2, ibis::qExpr::OP_LT, t1);
    qexpr_stack.push(pn1);
}
| mathexpr LEOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t2, ibis::qExpr::OP_LE, t1);
    qexpr_stack.push(pn1);
}
| mathexpr GTOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t2, ibis::qExpr::OP_GT, t1);
    qexpr_stack.push(pn1);
}
| mathexpr GEOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t2, ibis::qExpr::OP_GE, t1);
    qexpr_stack.push(pn1);
}
| mathexpr EQOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t2, ibis::qExpr::OP_EQ, t1);
    qexpr_stack.push(pn1);
}
| mathexpr NEQOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::qExpr* pn2 = new ibis::compRange(t2, ibis::qExpr::OP_EQ, t1);
    pn1 = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    pn1->setLeft(pn2);
    qexpr_stack.push(pn1);
}
;

computedRange2:
mathexpr LTOP mathexpr LTOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_LT, t2,
			      ibis::qExpr::OP_LT, t1);
    qexpr_stack.push(pn1);
}
| mathexpr LTOP mathexpr LEOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_LT, t2,
			      ibis::qExpr::OP_LE, t1);
    qexpr_stack.push(pn1);
}
| mathexpr LEOP mathexpr LTOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_LE, t2,
			      ibis::qExpr::OP_LT, t1);
    qexpr_stack.push(pn1);
}
| mathexpr LEOP mathexpr LEOP mathexpr
{
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_LE, t2,
			      ibis::qExpr::OP_LE, t1);
    qexpr_stack.push(pn1);
}
| mathexpr GTOP mathexpr GTOP mathexpr
{	// v1 > v2 > v3
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_GT, t2,
			      ibis::qExpr::OP_GT, t1);
    qexpr_stack.push(pn1);
}
| mathexpr GTOP mathexpr GEOP mathexpr
{	// v1 > v2 >= v3
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_GT, t2,
			      ibis::qExpr::OP_GE, t1);
    qexpr_stack.push(pn1);
}
| mathexpr GEOP mathexpr GTOP mathexpr
{	// v1 >= v2 > v3
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_GE, t2,
			      ibis::qExpr::OP_GT, t1);
    qexpr_stack.push(pn1);
}
| mathexpr GEOP mathexpr GEOP mathexpr
{	// v1 >= v2 >= v3
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t3, ibis::qExpr::OP_GE, t2,
			      ibis::qExpr::OP_GE, t1);
    qexpr_stack.push(pn1);
}
| mathexpr BETWEENOP mathexpr ANDOP mathexpr
{   // a two-sided range in SQL terminology
    ibis::compRange::term *t1 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t2 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    ibis::compRange::term *t3 =
	static_cast<ibis::compRange::term*>(qexpr_stack.top());
    qexpr_stack.pop();
    pn1 = new ibis::compRange(t2, ibis::qExpr::OP_LE, t3,
			      ibis::qExpr::OP_LE, t1);
    qexpr_stack.push(pn1);
}
;

mathexpr:
mathexpr ADDOP mathexpr
{   // addition
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Operator +";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::PLUS);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| mathexpr MINUSOP mathexpr
{   // subtraction
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Binary Operator -";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::MINUS);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| mathexpr BITANDOP mathexpr
{   // bitwise AND
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Bitwise AND operation ";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::BITAND);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| mathexpr BITOROP mathexpr
{   // bitwise OR
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Bitwise OR Operation ";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::BITOR);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| NOUNSTR '(' mathexpr ')'
{   // a new unary function
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Unary Function: " << $1;
#endif
    ibis::compRange::stdFunction1 *fnc =
	new ibis::compRange::stdFunction1($1);
    fnc->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(fnc);
}
| NOUNSTR '(' mathexpr ',' mathexpr ')'
{   // a new binary function
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Binary Function: " << $1;
#endif
    ibis::compRange::stdFunction2 *fnc =
	new ibis::compRange::stdFunction2($1);
    fnc->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    fnc->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(fnc);
}
| MINUSOP '(' mathexpr ')'  %prec UNOT
{   // a new unary operator
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Unary Operator -";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::NEGATE);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| '(' mathexpr ')'
| mathTerm1
;

mathTerm1:
mathexpr MULTOP mathexpr
{   // a new MULTIPLY operator
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Operator *";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::MULTIPLY);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| mathexpr DIVOP mathexpr
{   // a new DIVIDE operator
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Operator /";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::DIVIDE);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| mathexpr REMOP mathexpr
{   // a new remainder (%) operator
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Operator %";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::REMAINDER);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
| mathTerm2
;

mathTerm2:
mathexpr EXPOP mathTerm3
{   // a new POWER operator
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Operator ^";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::POWER);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
/*
| mathexpr XOROP mathTerm3 %prec EXPOP
{   // a new POWER operator
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0) << "DEBUG - parser got " << " Operator ^";
#endif
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::POWER);
    opr->setRight(qexpr_stack.top());
    qexpr_stack.pop();
    opr->setLeft(qexpr_stack.top());
    qexpr_stack.pop();
    qexpr_stack.push(opr);
}
*/
| mathTerm3
;

mathTerm3:
NOUNSTR
{   // a new variable name
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Variable: " << $1;
#endif
    ibis::compRange::variable *var = new ibis::compRange::variable($1);
    qexpr_stack.push(var);
}
| NUMSTR
{
    // a new number constant
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Constant: " << $1;
#endif
    ibis::compRange::number *var = new ibis::compRange::number($1);
    qexpr_stack.push(var);
}
| NUMHEX
{
    // a new number constant in hexadecimal
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Constant: " << $1;
#endif
    unsigned int val;
    const char *buf = $1;
    long ierr = sscanf(buf+2, "%x", &val);
    if (ierr <= 0) {
	val = 0;
	LOGGER(ibis::gVerbose >= 0)
	    << "parseQuery -- failed to extact a hexadecimal "
	    "integer from string \"" << buf << "\", assume zero (0)";
    }
    ibis::compRange::number *var = new ibis::compRange::number(val);
    qexpr_stack.push(var);
}
| MINUSOP NOUNSTR  %prec UNOT
{   // a new variable
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Variable: - " << $2;
#endif
    ibis::compRange::variable *var = new ibis::compRange::variable($2);
    ibis::compRange::bediener *opr =
	new ibis::compRange::bediener(ibis::compRange::NEGATE);
    opr->setRight(var);
    qexpr_stack.push(opr);
}
| MINUSOP NUMSTR  %prec UNOT
{   // a new number constant
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Constant: - " << $2;
#endif
    ibis::compRange::number *var = new ibis::compRange::number($2);
    var->negate(); // take into account of the negative sign
    qexpr_stack.push(var);
}
| ADDOP NUMSTR  %prec UNOT
{   // a new number constant
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG - parser got " << " Constant: + " << $2;
#endif
    ibis::compRange::number *var = new ibis::compRange::number($2);
    qexpr_stack.push(var);
}
;
%%

// initialization of global variables related to the string to be parsed.
// a mutex lock is required to ensure that only one instance of these
// variables are in active use at any given moment.
std::vector<char*> parse_str_vec;
char *parse_string = 0; // string to be parsed
int parse_length = 0; // length of the string to be parsed
int parse_offset = 0; // current position being processed

// report error
void yyerror(const char *s) {
    char theError[1024];
    sprintf(theError,  "While parsing \"%s\": %s at \"%s\" (%d of %d)",
	    parse_string, s, yytext, parse_offset, parse_length);
    LOGGER(ibis::gVerbose >= 0) << theError;
    if (strcmp(s, "syntax error")==0)
	throw theError;
}

int yywrap() {return 1;}

// delete parse_str_vec array and the content of parse_string
void yyparse_cleanup() {
    for (std::vector<char*>::iterator it=parse_str_vec.begin();
	 it!=parse_str_vec.end(); ++it) {
	delete[] *it;
    }
    parse_str_vec.clear();
    delete [] parse_string;
    parse_string = 0;
}

#if defined(YYBISON)
extern struct yy_buffer_state* yy_scan_bytes(const char*, int);
extern void yy_delete_buffer(struct yy_buffer_state*);

/// Parse a string into a query expression.
/// The query expression is stored in a binary tree (ibis::qExpr).  There
/// is a mutual exclusion lock to serialize accesses to this function.  The
/// implication is that only one query can be parsed at any given time.
ibis::qExpr* ibis::parseQuery(const char* str) {
    ibis::qExpr* root = 0;
    static pthread_mutex_t yaccMutex = PTHREAD_MUTEX_INITIALIZER;
    if (str) {
	// there can only be one active parser
	ibis::util::mutexLock lock(&yaccMutex, str);
	parse_string = ibis::util::strnewdup(str);
	parse_length = strlen(str);
	struct yy_buffer_state *yyst =
	    yy_scan_bytes(parse_string, parse_length);
	pn1 = 0;
#if defined(DEBUG) && DEBUG + 0 > 1
	LOGGER(ibis::gVerbose >= 0)
	    << "DEBUG - parser starts parsing \"" << str << "\"";
#endif
	try {
	    yyparse(); // yacc parse function
#if defined(DEBUG) && DEBUG + 0 > 1
	    LOGGER(ibis::gVerbose >= 0) << "DEBUG - parser produced " << *pn1;
#endif
	    root = pn1;
	}
	catch (const char* s) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "parseQuery -- received a string exception -- " << s;
	    delete pn1;
	}
	catch (const std::exception& e) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "parseQuery -- received a std::exception -- " << e.what();
	    delete pn1;
	}
	catch (...) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "parseQuery -- received a unexepcted exception";
	    delete pn1;
	}
	yy_delete_buffer(yyst);
	yyparse_cleanup();
    }
    else {
	LOGGER(ibis::gVerbose >= 0) << "parseQuery: incoming string is nil";
    }
    return root;
} // parseQuery()
#else
/// Parse a string into a query expression.
/// The query expression is stored in a binary tree (ibis::qExpr).  There
/// is a mutual exclusion lock to serialize accesses to this function.  The
/// implication is that only one query can be parsed at any given time.
ibis::qExpr* ibis::parseQuery(const char* str) {
    ibis::qExpr* root = 0;
    static pthread_mutex_t yaccMutex = PTHREAD_MUTEX_INITIALIZER;
    if (str) {
	// there can only be one active parser
	ibis::util::mutexLock lock(&yaccMutex, str);
	parse_string = ibis::util::strnewdup(str);
	parse_length = strlen(parse_string);
	parse_offset = 0;
	pn1 = 0;
#if defined(DEBUG) && DEBUG + 0 > 1
	LOGGER(ibis::gVerbose >= 0)
	    << "DEBUG - parser starts parsing \"" << parse_string << "\"";
#endif
	try {
	    yyparse(); // yacc parse function
#if defined(DEBUG) && DEBUG + 0 > 1
	    LOGGER(ibis::gVerbose >= 0) << "DEBUG - parser produced " << *pn1;
#endif
	    yyparse_cleanup();
	    root = pn1;
	}
	catch (const char* s) {
	    LOGGER(ibis::gVerbose >= 0) << "parseQuery received exception\n" << s;
	    yyparse_cleanup();
	    delete pn1;
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 0) << "parseQuery: incoming string is nil";
    }
    return root;
} // ibis::parseQuery
#endif

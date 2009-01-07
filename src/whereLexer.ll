/* $Id$ -*- mode: c++ -*-

   Author: John Wu <John.Wu at acm.org>
   Lawrence Berkeley National Laboratory
   Copyright 2007-2009 the Regents of the University of California
 */

%{ /* C++ declarations */
/** \file Defines the tokenlizer using Flex C++ template. */

#include "whereLexer.h"		// definition of YY_DECL
#include "whereParser.hh"	// class ibis::wherParser

typedef ibis::whereParser::token token;
typedef ibis::whereParser::token_type token_type;

#define yyterminate() return token::END
#define YY_USER_ACTION  yylloc->columns(yyleng);
%}

/* Flex declarations and options */
%option c++
%option stack
%option nounistd
%option never-interactive
%option prefix="_whereLexer_"
 /*%option case-insensitive*/

/* regular expressions used to shorten the definitions 

the following definition of a name is somehow bad
DIGIT	[0-9]
ALPHA	[_a-zA-Z]
NAME	{ALPHA}((->)?[{DIGIT}{ALPHA}:.]+)*(\[[^\]]+\])?

this version works -- guess I can not use {ALPHA}
NAME	[_a-zA-Z]((->)?[_a-zA-Z0-9.:\[\]]+)*
*/
WS	[ \t\v\n]
SEP	[ \t\v\n,;]
NUMBER	[-+]?([0-9]+[.]?|[0-9]*[.][0-9]+)([eE][-+]?[0-9]+)?
QUOTED	\"([^\"\\]*(\\.[^\"\\]*)*)\"|\'([^\'\\]*(\\.[^\'\\]*)*)\'
NAME	[_a-zA-Z]((->)?[0-9A-Za-z_:.]+)*(\[[^\]]+\])?

%%
%{
    yylloc->step();
%}
		   /* section defining the tokens */
"!"   {return token::NOTOP;}
"~"   {return token::NOTOP;}
"<="  {return token::LEOP;}
"!="  {return token::NEQOP;}
"<>"  {return token::NEQOP;}
"<"   {return token::LTOP;}
">="  {return token::GEOP;}
">"   {return token::GTOP;}
"="   {return token::EQOP;}
"=="  {return token::EQOP;}
"||"  {return token::OROP;}
"&&"  {return token::ANDOP;}
"&!"  {return token::ANDNOTOP;}
"&~"  {return token::ANDNOTOP;}
"|"   {return token::BITOROP;}
"&"   {return token::BITANDOP;}
"-"   {return token::MINUSOP;}
"+"   {return token::ADDOP;}
"*"   {return token::MULTOP;}
"/"   {return token::DIVOP;}
"%"   {return token::REMOP;}
"^"   {return token::EXPOP;}
"**"  {return token::EXPOP;}
[nN][oO][tT] {return token::NOTOP;}
[jJ][oO][iI][nN] {return token::JOINOP;}
[iI][nN] {return token::INOP;}
[oO][rR] {return token::OROP;}
[aA][nN][dD] {return token::ANDOP;}
[aA][nN][yY] {return token::ANYOP;}
[xX][oO][rR] {return token::XOROP;}
[mM][iI][nN][uU][sS] {return token::ANDNOTOP;}
[aA][nN][dD][nN][oO][tT] {return token::ANDNOTOP;}
[bB][eE][tT][wW][eE][eE][nN] {return token::BETWEENOP;}

{NAME} { /* a name, unquoted string */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
 	<< __FILE__ << ":" << __LINE__ << " got a name: " << yytext;
#endif
    yylval->stringVal = new std::string(yytext, yyleng);
    return token::NOUNSTR;
}

{NUMBER} { /* a floating-point number */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
 	<< __FILE__ << ":" << __LINE__ << " got a number: " << yytext;
#endif
    yylval->doubleVal = atof(yytext);
    return token::NUMBER;
}

0[xX][0-9a-fA-F]+ { /* a hexidacimal string */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
 	<< __FILE__ << ":" << __LINE__ << " got a hexadecimal number: " << yytext;
#endif
    (void) sscanf(yytext+2, "%x", &(yylval->integerVal));
    return token::NUMBER;
}

{QUOTED} { /* a quoted string literal */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
 	<< __FILE__ << ":" << __LINE__ << " got a quoted string: " << yytext;
#endif
    yylval->stringVal = new std::string(yytext+1, yyleng-2);
    return token::NOUNSTR;
}

\({WS}*{NUMBER}({SEP}+{NUMBER})*{WS}*\) { /* a number series */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
 	<< __FILE__ << ":" << __LINE__ << " got a number sequence: " << yytext;
#endif
    yylval->stringVal = new std::string(yytext+1, yyleng-2);
    return token::NUMSEQ;
}

\({WS}*({QUOTED}|{NAME}){WS}*({QUOTED}|{NAME})({SEP}+({QUOTED}|{NAME}))+{WS}*\) {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
 	<< __FILE__ << ":" << __LINE__ << " got a string sequence: " << yytext;
#endif
    yylval->stringVal = new std::string(yytext+1, yyleng-2);
    return token::STRSEQ;
}

{WS}+ ; /* do nothing for blank space */

. { /* pass the character to the parser as a token */
    return static_cast<token_type>(*yytext);
}

%%
/* additional c++ code to complete the definition of class whereLexer */
ibis::whereLexer::whereLexer(std::istream* in, std::ostream* out)
    : ::_wLexer(in, out) {
#if defined(DEBUG) && DEBUG + 0 > 1
    yy_flex_debug = true;
#endif
}

ibis::whereLexer::~whereLexer() {
}

/* function needed by the super-class of ibis::whereLexer */
#ifdef yylex
#undef yylex
#endif

int ::_wLexer::yylex() {
    return 0;
} // ::_wLexer::yylex

int ::_wLexer::yywrap() {
    return 1;
} // ::_wLexer::yywrap

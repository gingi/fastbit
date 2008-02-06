%option never-interactive case-insensitive pointer noyywrap
%{
/* $Id$ */
/*
   Defines the tokens recognized.
 */
#undef lex_input
#undef input
#undef unput
#include "predicate.h" // defines lex_input() and unput()
#include "predicate.tab.h"

#include <string.h>

%}
%%
"<="  {return LEOP;}
"!="  {return NEQOP;}
"<>"  {return NEQOP;}
"<"   {return LTOP;}
">="  {return GEOP;}
">"   {return GTOP;}
"="   {return EQOP;}
"=="  {return EQOP;}
"|"   {return BITOROP;}
"||"  {return OROP;}
"&"   {return BITANDOP;}
"&&"  {return ANDOP;}
"&!"  {return ANDNOTOP;}
"-"   {return MINUSOP;}
"^"   {return XOROP;}
"+"   {return ADDOP;}
"*"   {return MULTOP;}
"/"   {return DIVOP;}
"%"   {return REMOP;}
"**"  {return EXPOP;}
"!"   {return NOTOP;}
"~"   {return NOTOP;}
[nN][oO][tT] {return NOTOP;}
[jJ][oO][iI][nN] {return JOINOP;}
[iI][nN] {return INOP;}
[oO][rR] {return OROP;}
[aA][nN][dD] {return ANDOP;}
[aA][nN][yY] {return ANYOP;}
[xX][oO][rR] {return XOROP;}
[mM][iI][nN][uU][sS] {return ANDNOTOP;}
[aA][nN][dD][nN][oO][tT] {return ANDNOTOP;}
[bB][eE][tT][wW][eE][eE][nN] {return BETWEENOP;}

[A-Za-z_]((->)?[0-9A-Za-z_.:\[\]]+)*   { /* name string */
    /* a name */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0) << "got a name: " << yytext;
#endif
    char* str = new char[strlen(yytext)+1];
    parse_str_vec.push_back(str);
    strcpy(str, yytext);
    yylval.sval = str;
    return NOUNSTR;
}

\([\t\n ]*[-+]?(([0-9]+(\.[0-9]*)?)|(\.[0-9]+))([eE][-+]?[0-9]+)?([\n\t, ]+[-+]?(([0-9]+(\.[0-9]*)?)|(\.[0-9]+))([eE][-+]?[0-9]+)?)*[\t\n ]*\) {
    /* a series of numbers in parentheses */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0) << "got a series of numbers: " << yytext;
#endif
    unsigned len = strlen(yytext);
    char* str = new char[len];
    parse_str_vec.push_back(str);
    strcpy(str, yytext+1);
    str[len-2] = 0; // remove ')'
    yylval.sval = str;
    return NUMSEQ;
}

[-+]?(([0-9]+(\.[0-9]*)?)|(\.[0-9]+))([eE][-+]?[0-9]+)? {
    /* a number in string format */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0) << "got a number string: " << yytext;
#endif
    char* str = new char[strlen(yytext)+1];
    parse_str_vec.push_back(str);
    strcpy(str, yytext);
    yylval.sval = str;
    return NUMSTR;
}

0x[0-9a-fA-F]+ {
    /* a hexadecimal number */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0) << "got a hex numnber: " << yytext;
#endif
    char* str = new char[strlen(yytext)+1];
    parse_str_vec.push_back(str);
    strcpy(str, yytext);
    yylval.sval = str;
    return NUMHEX;
}

\((\"([^\"\\]*(\\.[^\"\\]*)*)\"|\'([^\'\\]*(\\.[^\'\\]*)*)\')([\n\t, ]+(\"([^\"\\]*(\\.[^\"\\]*)*)\"|\'([^\'\\]*(\\.[^\'\\]*)*)\'))*\) {
    /* a list of quoted strings in parentheses */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0) << "got a list of quoted strings: " << yytext;
#endif
    unsigned len = strlen(yytext);
    char* str = new char[len];
    parse_str_vec.push_back(str);
    strcpy(str, yytext+1);
    str[len-2] = 0; // remove ')'
    yylval.sval = str;
    return STRSEQ;
}

\"([^\"\\]*(\\.[^\"\\]*)*)\"|\'([^\'\\]*(\\.[^\'\\]*)*)\' {
    /* a quoted string -- LITSTR is a shorthand for literal string */
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0) << "got a quoted string: " << yytext;
#endif
    char* str = new char[strlen(yytext)+1];
    parse_str_vec.push_back(str);
    strcpy(str, yytext);
    yylval.sval = str;
    return LITSTR;
}

[\n\t ] ; /* skip space */
"[\n]"  {return 0;}
.      return yytext[0];
%%

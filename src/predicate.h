// File: $Id$
// Author: John Wu <John.Wu@nersc.gov>
//      Lawrence Berkeley National Laboratory
// Copyright 1998-2008 the Regents of the University of California
#ifndef IBIS_PREDICATE_H
#define IBIS_PREDICATE_H
///@file
/// Replaces some default lex functions.
///
/// The replacement functions all it to work on a string named @c
/// parse_string (size @c parse_length).  ALL lex/yacc related variables
/// are in global namespace, only parseQuery is in the usual ibis
/// namespace.
///
/// @note
/// The only function that should be used elsewhere is parseQuery
///@verbatim
/// ibis::qExpr* ibis::parseQuery(const char* str)
///@endverbatim
#include "qExpr.h"

#include <stdio.h> // BUFSIZ
#include <stack>
#include <deque>
#include <vector>

int yylex();
int  yyparse();
void yyerror(const char *s);
void yyparse_cleanup();
namespace ibis {
    /// Parse a query string.
    qExpr* parseQuery(const char* str);
}

extern std::vector<char*> parse_str_vec;
extern char *parse_string;
extern int parse_length, parse_offset;

#if defined(YYBISON) || defined(FLEX_SCANNER)
// FLEX is instructed to use pointer instead of array
extern char *yytext;
#define YY_INPUT(buf,result,max_size) {\
	if (parse_offset <= parse_length) {\
	buf[0] = parse_string[parse_offset++];\
	result = 1;}\
	else {result = YY_NULL;}}

#else
// on solaris machines, the lex/yacc uses array
# define YYLMAX BUFSIZ
extern char yytext[YYLMAX];
int yywrap();

// Overwrite the default lex_input that reads from file.  This version uses
// the string stored in global variable parse_string.
inline int lex_input() {
    if (parse_offset <= parse_length) {
	return parse_string[parse_offset++];
    }
    else {
	yyerror("attempt to move beyond EOS");
	return 0;
    }
}

inline void unput(int){
    if (parse_offset > 0) --parse_offset;
    else yyerror("attempt to move before BOS");
}
#endif
#endif //  IBIS_PREDICATE_H

/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison GLR parsers in C

   Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NOUNSTR = 258,
     NUMSEQ = 259,
     NUMSTR = 260,
     NUMHEX = 261,
     STRSEQ = 262,
     LITSTR = 263,
     NOTOP = 264,
     LEOP = 265,
     GEOP = 266,
     LTOP = 267,
     GTOP = 268,
     EQOP = 269,
     NEQOP = 270,
     ANDOP = 271,
     ANDNOTOP = 272,
     BETWEENOP = 273,
     INOP = 274,
     JOINOP = 275,
     ANYOP = 276,
     OROP = 277,
     XOROP = 278,
     BITOROP = 279,
     BITANDOP = 280,
     MINUSOP = 281,
     MULTOP = 282,
     DIVOP = 283,
     REMOP = 284,
     EXPOP = 285,
     ADDOP = 286,
     STRING_EXPRESSION = 287,
     UNOT = 288
   };
#endif


/* Copy the first part of user declarations.  */
#line 1 "predicate.y"

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


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE 
#line 27 "predicate.y"
{
    float fval;
    int   ival;
    char *sval;
}
/* Line 2604 of glr.c.  */
#line 114 "predicate.tab.h"
	YYSTYPE;
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{

  char yydummy;

} YYLTYPE;
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;



///
/// Grammar of query conditions.

/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++

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

// Take the name prefix into account.
#define yylex   ibislex

#include "whereParser.hh"

/* User implementation prologue.  */
#line 90 "whereParser.yy"

#include "whereLexer.h"

#undef yylex
#define yylex driver.lexer->lex


/* Line 317 of lalr1.cc.  */
#line 51 "whereParser.cc"

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* FIXME: INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#define YYUSE(e) ((void) (e))

/* A pseudo ostream that takes yydebug_ into account.  */
# define YYCDEBUG							\
  for (bool yydebugcond_ = yydebug_; yydebugcond_; yydebugcond_ = false)	\
    (*yycdebug_)

/* Enable debugging if requested.  */
#if YYDEBUG

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)	\
do {							\
  if (yydebug_)						\
    {							\
      *yycdebug_ << Title << ' ';			\
      yy_symbol_print_ ((Type), (Value), (Location));	\
      *yycdebug_ << std::endl;				\
    }							\
} while (false)

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug_)				\
    yy_reduce_print_ (Rule);		\
} while (false)

# define YY_STACK_PRINT()		\
do {					\
  if (yydebug_)				\
    yystack_print_ ();			\
} while (false)

#else /* !YYDEBUG */

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_REDUCE_PRINT(Rule)
# define YY_STACK_PRINT()

#endif /* !YYDEBUG */

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab

namespace ibis
{
#if YYERROR_VERBOSE

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  whereParser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr = "";
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              /* Fall through.  */
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

#endif

  /// Build a parser object.
  whereParser::whereParser (class ibis::whereClause& driver_yyarg)
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
      driver (driver_yyarg)
  {
  }

  whereParser::~whereParser ()
  {
  }

#if YYDEBUG
  /*--------------------------------.
  | Print this symbol on YYOUTPUT.  |
  `--------------------------------*/

  inline void
  whereParser::yy_symbol_value_print_ (int yytype,
			   const semantic_type* yyvaluep, const location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yyvaluep);
    switch (yytype)
      {
         default:
	  break;
      }
  }


  void
  whereParser::yy_symbol_print_ (int yytype,
			   const semantic_type* yyvaluep, const location_type* yylocationp)
  {
    *yycdebug_ << (yytype < yyntokens_ ? "token" : "nterm")
	       << ' ' << yytname_[yytype] << " ("
	       << *yylocationp << ": ";
    yy_symbol_value_print_ (yytype, yyvaluep, yylocationp);
    *yycdebug_ << ')';
  }
#endif /* ! YYDEBUG */

  void
  whereParser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
      {
        case 28: /* "\"name\"" */
#line 87 "whereParser.yy"
	{ delete (yyvaluep->stringVal); };
#line 211 "whereParser.cc"
	break;
      case 29: /* "\"number sequence\"" */
#line 87 "whereParser.yy"
	{ delete (yyvaluep->stringVal); };
#line 216 "whereParser.cc"
	break;
      case 30: /* "\"string sequence\"" */
#line 87 "whereParser.yy"
	{ delete (yyvaluep->stringVal); };
#line 221 "whereParser.cc"
	break;
      case 37: /* "qexpr" */
#line 88 "whereParser.yy"
	{ delete (yyvaluep->whereNode); };
#line 226 "whereParser.cc"
	break;
      case 38: /* "simpleRange" */
#line 88 "whereParser.yy"
	{ delete (yyvaluep->whereNode); };
#line 231 "whereParser.cc"
	break;
      case 39: /* "compRange2" */
#line 88 "whereParser.yy"
	{ delete (yyvaluep->whereNode); };
#line 236 "whereParser.cc"
	break;
      case 40: /* "compRange3" */
#line 88 "whereParser.yy"
	{ delete (yyvaluep->whereNode); };
#line 241 "whereParser.cc"
	break;
      case 41: /* "mathExpr" */
#line 88 "whereParser.yy"
	{ delete (yyvaluep->whereNode); };
#line 246 "whereParser.cc"
	break;

	default:
	  break;
      }
  }

  void
  whereParser::yypop_ (unsigned int n)
  {
    yystate_stack_.pop (n);
    yysemantic_stack_.pop (n);
    yylocation_stack_.pop (n);
  }

  std::ostream&
  whereParser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  whereParser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  whereParser::debug_level_type
  whereParser::debug_level () const
  {
    return yydebug_;
  }

  void
  whereParser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }


  int
  whereParser::parse ()
  {
    /// Look-ahead and look-ahead in internal form.
    int yychar = yyempty_;
    int yytoken = 0;

    /* State.  */
    int yyn;
    int yylen = 0;
    int yystate = 0;

    /* Error handling.  */
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// Semantic value of the look-ahead.
    semantic_type yylval;
    /// Location of the look-ahead.
    location_type yylloc;
    /// The locations where the error started and ended.
    location yyerror_range[2];

    /// $$.
    semantic_type yyval;
    /// @$.
    location_type yyloc;

    int yyresult;

    YYCDEBUG << "Starting parse" << std::endl;


    /* User initialization code.  */
    #line 26 "whereParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}
  /* Line 547 of yacc.c.  */
#line 327 "whereParser.cc"
    /* Initialize the stacks.  The initial state will be pushed in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystate_stack_ = state_stack_type (0);
    yysemantic_stack_ = semantic_stack_type (0);
    yylocation_stack_ = location_stack_type (0);
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* New state.  */
  yynewstate:
    yystate_stack_.push (yystate);
    YYCDEBUG << "Entering state " << yystate << std::endl;
    goto yybackup;

    /* Backup.  */
  yybackup:

    /* Try to take a decision without look-ahead.  */
    yyn = yypact_[yystate];
    if (yyn == yypact_ninf_)
      goto yydefault;

    /* Read a look-ahead token.  */
    if (yychar == yyempty_)
      {
	YYCDEBUG << "Reading a token: ";
	yychar = yylex (&yylval, &yylloc);
      }


    /* Convert token to internal form.  */
    if (yychar <= yyeof_)
      {
	yychar = yytoken = yyeof_;
	YYCDEBUG << "Now at end of input." << std::endl;
      }
    else
      {
	yytoken = yytranslate_ (yychar);
	YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
      }

    /* If the proper action on seeing token YYTOKEN is to reduce or to
       detect an error, take that action.  */
    yyn += yytoken;
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yytoken)
      goto yydefault;

    /* Reduce or error.  */
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
	if (yyn == 0 || yyn == yytable_ninf_)
	goto yyerrlab;
	yyn = -yyn;
	goto yyreduce;
      }

    /* Accept?  */
    if (yyn == yyfinal_)
      goto yyacceptlab;

    /* Shift the look-ahead token.  */
    YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

    /* Discard the token being shifted unless it is eof.  */
    if (yychar != yyeof_)
      yychar = yyempty_;

    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* Count tokens shifted since error; after three, turn off error
       status.  */
    if (yyerrstatus_)
      --yyerrstatus_;

    yystate = yyn;
    goto yynewstate;

  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystate];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;

  /*-----------------------------.
  | yyreduce -- Do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    /* If YYLEN is nonzero, implement the default value of the action:
       `$$ = $1'.  Otherwise, use the top of the stack.

       Otherwise, the following line sets YYVAL to garbage.
       This behavior is undocumented and Bison
       users should not rely upon it.  */
    if (yylen)
      yyval = yysemantic_stack_[yylen - 1];
    else
      yyval = yysemantic_stack_[0];

    {
      slice<location_type, location_stack_type> slice (yylocation_stack_, yylen);
      YYLLOC_DEFAULT (yyloc, slice, yylen);
    }
    YY_REDUCE_PRINT (yyn);
    switch (yyn)
      {
	  case 2:
#line 99 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " || " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_OR);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
;}
    break;

  case 3:
#line 109 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " ^ " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_XOR);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
;}
    break;

  case 4:
#line 119 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " && " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
;}
    break;

  case 5:
#line 129 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " &~ " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_MINUS);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
;}
    break;

  case 6:
#line 139 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ! " << *(yysemantic_stack_[(2) - (2)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft((yysemantic_stack_[(2) - (2)].whereNode));
;}
    break;

  case 7:
#line 147 "whereParser.yy"
    {
    (yyval.whereNode) = (yysemantic_stack_[(3) - (2)].whereNode);
;}
    break;

  case 11:
#line 156 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qDiscreteRange((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
;}
    break;

  case 12:
#line 166 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(7) - (1)].stringVal) << " IN ("
	<< (yysemantic_stack_[(7) - (4)].doubleVal) << ", " << (yysemantic_stack_[(7) - (6)].doubleVal) << ")";
#endif
    std::vector<double> vals(2);
    vals[0] = (yysemantic_stack_[(7) - (4)].doubleVal);
    vals[1] = (yysemantic_stack_[(7) - (6)].doubleVal);
    (yyval.whereNode) = new ibis::qDiscreteRange((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), vals);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
;}
    break;

  case 13:
#line 178 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(5) - (1)].stringVal) << " IN ("
	<< (yysemantic_stack_[(5) - (4)].doubleVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qContinuousRange((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), ibis::qExpr::OP_EQ, (yysemantic_stack_[(5) - (4)].doubleVal));
    delete (yysemantic_stack_[(5) - (1)].stringVal);
;}
    break;

  case 14:
#line 187 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(4) - (4)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qDiscreteRange((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), (yysemantic_stack_[(4) - (4)].stringVal)->c_str()));
    delete (yysemantic_stack_[(4) - (4)].stringVal);
    delete (yysemantic_stack_[(4) - (1)].stringVal);
;}
    break;

  case 15:
#line 198 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(8) - (1)].stringVal) << " NOT IN ("
	<< (yysemantic_stack_[(8) - (5)].doubleVal) << ", " << (yysemantic_stack_[(8) - (7)].doubleVal) << ")";
#endif
    std::vector<double> vals(2);
    vals[0] = (yysemantic_stack_[(8) - (5)].doubleVal);
    vals[1] = (yysemantic_stack_[(8) - (7)].doubleVal);
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qDiscreteRange((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), vals));
    delete (yysemantic_stack_[(8) - (1)].stringVal);
;}
    break;

  case 16:
#line 211 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << " NOT IN ("
	<< (yysemantic_stack_[(6) - (5)].doubleVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qContinuousRange((yysemantic_stack_[(6) - (1)].stringVal)->c_str(), ibis::qExpr::OP_EQ, (yysemantic_stack_[(6) - (5)].doubleVal)));
    delete (yysemantic_stack_[(6) - (1)].stringVal);
;}
    break;

  case 17:
#line 221 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
;}
    break;

  case 18:
#line 231 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(7) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(7) - (4)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(7) - (6)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
;}
    break;

  case 19:
#line 248 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(7) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(7) - (4)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(7) - (6)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
;}
    break;

  case 20:
#line 265 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(7) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(7) - (4)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(7) - (6)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
;}
    break;

  case 21:
#line 282 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(7) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(7) - (4)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(7) - (6)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
;}
    break;

  case 22:
#line 299 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(5) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(5) - (4)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(5) - (4)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(5) - (4)].stringVal);
    delete (yysemantic_stack_[(5) - (1)].stringVal);
;}
    break;

  case 23:
#line 313 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(5) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(5) - (4)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(5) - (4)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qMultiString((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(5) - (4)].stringVal);
    delete (yysemantic_stack_[(5) - (1)].stringVal);
;}
    break;

  case 24:
#line 327 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " LIKE ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qLike((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
;}
    break;

  case 25:
#line 337 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " LIKE ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qLike((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
;}
    break;

  case 26:
#line 347 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(4) - (4)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), (yysemantic_stack_[(4) - (4)].stringVal)->c_str()));
    delete (yysemantic_stack_[(4) - (4)].stringVal);
    delete (yysemantic_stack_[(4) - (1)].stringVal);
;}
    break;

  case 27:
#line 358 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(8) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(8) - (5)].stringVal) << ", " << *(yysemantic_stack_[(8) - (7)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(8) - (5)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(8) - (7)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
;}
    break;

  case 28:
#line 376 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(8) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(8) - (5)].stringVal) << ", " << *(yysemantic_stack_[(8) - (7)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(8) - (5)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(8) - (7)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
;}
    break;

  case 29:
#line 394 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(8) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(8) - (5)].stringVal) << ", " << *(yysemantic_stack_[(8) - (7)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(8) - (5)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(8) - (7)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
;}
    break;

  case 30:
#line 412 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(8) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(8) - (5)].stringVal) << ", " << *(yysemantic_stack_[(8) - (7)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(8) - (5)].stringVal);
    val += "\", \"";
    val += *(yysemantic_stack_[(8) - (7)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
;}
    break;

  case 31:
#line 430 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(6) - (5)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(6) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(6) - (5)].stringVal);
    delete (yysemantic_stack_[(6) - (1)].stringVal);
;}
    break;

  case 32:
#line 445 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif
    std::string val;
    val = '"'; /* add quote to keep strings intact */
    val += *(yysemantic_stack_[(6) - (5)].stringVal);
    val += '"';
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qMultiString((yysemantic_stack_[(6) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(6) - (5)].stringVal);
    delete (yysemantic_stack_[(6) - (1)].stringVal);
;}
    break;

  case 33:
#line 460 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ANY(" << *(yysemantic_stack_[(6) - (3)].stringVal) << ") = "
	<< (yysemantic_stack_[(6) - (6)].doubleVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qAnyAny((yysemantic_stack_[(6) - (3)].stringVal)->c_str(), (yysemantic_stack_[(6) - (6)].doubleVal));
    delete (yysemantic_stack_[(6) - (3)].stringVal);
;}
    break;

  case 34:
#line 469 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ANY(" << *(yysemantic_stack_[(6) - (3)].stringVal) << ") = "
	<< *(yysemantic_stack_[(6) - (6)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qAnyAny((yysemantic_stack_[(6) - (3)].stringVal)->c_str(), (yysemantic_stack_[(6) - (6)].stringVal)->c_str());
    delete (yysemantic_stack_[(6) - (6)].stringVal);
    delete (yysemantic_stack_[(6) - (3)].stringVal);
;}
    break;

  case 35:
#line 482 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " = "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2);
;}
    break;

  case 36:
#line 492 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " != "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2));
;}
    break;

  case 37:
#line 503 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2);
;}
    break;

  case 38:
#line 513 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2);
;}
    break;

  case 39:
#line 523 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_GT, me2);
;}
    break;

  case 40:
#line 533 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_GE, me2);
;}
    break;

  case 41:
#line 543 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (3)].stringVal) << " = "
	<< *(yysemantic_stack_[(3) - (1)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qString((yysemantic_stack_[(3) - (3)].stringVal)->c_str(), (yysemantic_stack_[(3) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
;}
    break;

  case 42:
#line 553 "whereParser.yy"
    {
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " = "
	<< *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    ibis::math::variable *var = dynamic_cast<ibis::math::variable*>(me1);
    if (var != 0) {
	(yyval.whereNode) = new ibis::qString(var->variableName(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
	delete (yysemantic_stack_[(3) - (3)].stringVal);
	delete var;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "whereParser.yy: rule mathExpr == 'string literal' is a "
	    "kludge for Name == 'string literal'.  The mathExpr on the "
	    "left can only be variable name, currently " << *me1;
	delete (yysemantic_stack_[(3) - (3)].stringVal);
	delete me1;
	throw "The rule on line 419 in whereParser.yy expects a simple "
	    "variable name on the left-hand side";
    }
;}
    break;

  case 43:
#line 577 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (3)].stringVal) << " = "
	<< *(yysemantic_stack_[(3) - (1)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qString((yysemantic_stack_[(3) - (3)].stringVal)->c_str(), (yysemantic_stack_[(3) - (1)].stringVal)->c_str()));
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
;}
    break;

  case 44:
#line 588 "whereParser.yy"
    {
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " != "
	<< *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    ibis::math::variable *var = dynamic_cast<ibis::math::variable*>(me1);
    if (var != 0) {
	(yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
	(yyval.whereNode)->setLeft(new ibis::qString(var->variableName(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str()));
	delete (yysemantic_stack_[(3) - (3)].stringVal);
	delete var;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "whereParser.yy: rule mathExpr != 'string literal' is a "
	    "kludge for Name != 'string literal'.  The mathExpr on the "
	    "left can only be variable name, currently " << *me1;
	delete (yysemantic_stack_[(3) - (3)].stringVal);
	delete me1;
	throw "The rule on line 419 in whereParser.yy expects a simple "
	    "variable name on the left-hand side";
    }
;}
    break;

  case 45:
#line 616 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2 << " < " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LT, me3);
;}
    break;

  case 46:
#line 628 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2 << " <= " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LE, me3);
;}
    break;

  case 47:
#line 640 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2 << " < " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LT, me3);
;}
    break;

  case 48:
#line 652 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2 << " <= " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LE, me3);
;}
    break;

  case 49:
#line 664 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2 << " > " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me3, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LT, me1);
;}
    break;

  case 50:
#line 676 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2 << " >= " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me3, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LT, me1);
;}
    break;

  case 51:
#line 688 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2 << " > " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me3, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LE, me1);
;}
    break;

  case 52:
#line 700 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2 << " >= " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me3, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LE, me1);
;}
    break;

  case 53:
#line 712 "whereParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].whereNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " BETWEEN "
	<< *me2 << " AND " << *me3;
#endif
    (yyval.whereNode) = new ibis::compRange(me2, ibis::qExpr::OP_LE, me1,
			     ibis::qExpr::OP_LE, me3);
;}
    break;

  case 54:
#line 727 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " + " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::PLUS);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 55:
#line 739 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " - " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MINUS);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 56:
#line 751 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " * " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MULTIPLY);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 57:
#line 763 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " / " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::DIVIDE);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 58:
#line 775 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " % " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::REMAINDER);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 59:
#line 787 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " ^ " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::POWER);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 60:
#line 799 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " & " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITAND);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 61:
#line 811 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " | " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITOR);
    opr->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 62:
#line 823 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(4) - (3)].whereNode) << ")";
#endif
    ibis::math::stdFunction1 *fun =
	new ibis::math::stdFunction1((yysemantic_stack_[(4) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(4) - (1)].stringVal);
    fun->setLeft((yysemantic_stack_[(4) - (3)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(fun);
;}
    break;

  case 63:
#line 835 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(6) - (3)].whereNode) << ", " << *(yysemantic_stack_[(6) - (5)].whereNode) << ")";
#endif
    ibis::math::stdFunction2 *fun =
	new ibis::math::stdFunction2((yysemantic_stack_[(6) - (1)].stringVal)->c_str());
    fun->setRight((yysemantic_stack_[(6) - (5)].whereNode));
    fun->setLeft((yysemantic_stack_[(6) - (3)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(fun);
    delete (yysemantic_stack_[(6) - (1)].stringVal);
;}
    break;

  case 64:
#line 848 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *(yysemantic_stack_[(2) - (2)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight((yysemantic_stack_[(2) - (2)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
;}
    break;

  case 65:
#line 858 "whereParser.yy"
    {
    (yyval.whereNode) = (yysemantic_stack_[(2) - (2)].whereNode);
;}
    break;

  case 66:
#line 861 "whereParser.yy"
    {
    (yyval.whereNode) = (yysemantic_stack_[(3) - (2)].whereNode);
;}
    break;

  case 67:
#line 864 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    ibis::math::variable *var =
	new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    (yyval.whereNode) = static_cast<ibis::qExpr*>(var);
    delete (yysemantic_stack_[(1) - (1)].stringVal);
;}
    break;

  case 68:
#line 874 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    ibis::math::number *num = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(num);
;}
    break;

  case 69:
#line 884 "whereParser.yy"
    { /* pass qexpr to the driver */
    driver.expr_ = (yysemantic_stack_[(2) - (1)].whereNode);
;}
    break;

  case 70:
#line 887 "whereParser.yy"
    { /* pass qexpr to the driver */
    driver.expr_ = (yysemantic_stack_[(2) - (1)].whereNode);
;}
    break;


    /* Line 675 of lalr1.cc.  */
#line 1482 "whereParser.cc"
	default: break;
      }
    YY_SYMBOL_PRINT ("-> $$ =", yyr1_[yyn], &yyval, &yyloc);

    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();

    yysemantic_stack_.push (yyval);
    yylocation_stack_.push (yyloc);

    /* Shift the result of the reduction.  */
    yyn = yyr1_[yyn];
    yystate = yypgoto_[yyn - yyntokens_] + yystate_stack_[0];
    if (0 <= yystate && yystate <= yylast_
	&& yycheck_[yystate] == yystate_stack_[0])
      yystate = yytable_[yystate];
    else
      yystate = yydefgoto_[yyn - yyntokens_];
    goto yynewstate;

  /*------------------------------------.
  | yyerrlab -- here on detecting error |
  `------------------------------------*/
  yyerrlab:
    /* If not already recovering from an error, report this error.  */
    if (!yyerrstatus_)
      {
	++yynerrs_;
	error (yylloc, yysyntax_error_ (yystate));
      }

    yyerror_range[0] = yylloc;
    if (yyerrstatus_ == 3)
      {
	/* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

	if (yychar <= yyeof_)
	  {
	  /* Return failure if at end of input.  */
	  if (yychar == yyeof_)
	    YYABORT;
	  }
	else
	  {
	    yydestruct_ ("Error: discarding", yytoken, &yylval, &yylloc);
	    yychar = yyempty_;
	  }
      }

    /* Else will try to reuse look-ahead token after shifting the error
       token.  */
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:

    /* Pacify compilers like GCC when the user code never invokes
       YYERROR and the label yyerrorlab therefore never appears in user
       code.  */
    if (false)
      goto yyerrorlab;

    yyerror_range[0] = yylocation_stack_[yylen - 1];
    /* Do not reclaim the symbols of the rule which action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    yystate = yystate_stack_[0];
    goto yyerrlab1;

  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;	/* Each real token shifted decrements this.  */

    for (;;)
      {
	yyn = yypact_[yystate];
	if (yyn != yypact_ninf_)
	{
	  yyn += yyterror_;
	  if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
	    {
	      yyn = yytable_[yyn];
	      if (0 < yyn)
		break;
	    }
	}

	/* Pop the current state because it cannot handle the error token.  */
	if (yystate_stack_.height () == 1)
	YYABORT;

	yyerror_range[0] = yylocation_stack_[0];
	yydestruct_ ("Error: popping",
		     yystos_[yystate],
		     &yysemantic_stack_[0], &yylocation_stack_[0]);
	yypop_ ();
	yystate = yystate_stack_[0];
	YY_STACK_PRINT ();
      }

    if (yyn == yyfinal_)
      goto yyacceptlab;

    yyerror_range[1] = yylloc;
    // Using YYLLOC is tempting, but would change the location of
    // the look-ahead.  YYLOC is available though.
    YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yyloc);

    /* Shift the error token.  */
    YY_SYMBOL_PRINT ("Shifting", yystos_[yyn],
		   &yysemantic_stack_[0], &yylocation_stack_[0]);

    yystate = yyn;
    goto yynewstate;

    /* Accept.  */
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;

    /* Abort.  */
  yyabortlab:
    yyresult = 1;
    goto yyreturn;

  yyreturn:
    if (yychar != yyeof_ && yychar != yyempty_)
      yydestruct_ ("Cleanup: discarding lookahead", yytoken, &yylval, &yylloc);

    /* Do not reclaim the symbols of the rule which action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (yystate_stack_.height () != 1)
      {
	yydestruct_ ("Cleanup: popping",
		   yystos_[yystate_stack_[0]],
		   &yysemantic_stack_[0],
		   &yylocation_stack_[0]);
	yypop_ ();
      }

    return yyresult;
  }

  // Generate an error message.
  std::string
  whereParser::yysyntax_error_ (int yystate)
  {
    std::string res;
    YYUSE (yystate);
#if YYERROR_VERBOSE
    int yyn = yypact_[yystate];
    if (yypact_ninf_ < yyn && yyn <= yylast_)
      {
	/* Start YYX at -YYN if negative to avoid negative indexes in
	   YYCHECK.  */
	int yyxbegin = yyn < 0 ? -yyn : 0;

	/* Stay within bounds of both yycheck and yytname.  */
	int yychecklim = yylast_ - yyn + 1;
	int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
	int count = 0;
	for (int x = yyxbegin; x < yyxend; ++x)
	  if (yycheck_[x + yyn] == x && x != yyterror_)
	    ++count;

	// FIXME: This method of building the message is not compatible
	// with internationalization.  It should work like yacc.c does it.
	// That is, first build a string that looks like this:
	// "syntax error, unexpected %s or %s or %s"
	// Then, invoke YY_ on this string.
	// Finally, use the string as a format to output
	// yytname_[tok], etc.
	// Until this gets fixed, this message appears in English only.
	res = "syntax error, unexpected ";
	res += yytnamerr_ (yytname_[tok]);
	if (count < 5)
	  {
	    count = 0;
	    for (int x = yyxbegin; x < yyxend; ++x)
	      if (yycheck_[x + yyn] == x && x != yyterror_)
		{
		  res += (!count++) ? ", expecting " : " or ";
		  res += yytnamerr_ (yytname_[x]);
		}
	  }
      }
    else
#endif
      res = YY_("syntax error");
    return res;
  }


  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
  const signed char whereParser::yypact_ninf_ = -31;
  const short int
  whereParser::yypact_[] =
  {
        57,    57,   -30,   220,   220,   -31,    97,    93,    57,    12,
     -31,   -31,   -31,   100,     4,   -31,   -17,   -16,   220,   -31,
     -31,     3,    29,   181,   220,    -8,     2,    -4,    48,   -31,
      57,    57,    57,    57,   -31,   220,   220,   220,   220,    55,
     218,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     -31,    47,   196,   178,   -31,   -31,   -14,   -31,   -31,   180,
     -31,   -31,   -31,   -31,   -31,   -31,   213,   117,   111,   142,
     134,   164,   -31,   153,   -31,   153,   172,   233,   120,    40,
      40,    90,    90,    90,    90,    -5,   -31,   -31,     0,    77,
     118,   147,   -31,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,    76,    89,   235,   237,   239,   -31,   112,
     -31,   197,   -31,   232,   211,   153,   153,   153,   153,   153,
     153,   153,   153,   153,   -31,   -31,   -31,   121,   -31,   234,
     -31,   236,   137,   194,   205,   210,   228,   -31,   241,   242,
     243,   244,   245,   -31,   -31,   -31,   -31,   -31,   -31,   -31,
     -31,   -31,   -31
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  whereParser::yydefact_[] =
  {
         0,     0,     0,     0,     0,    68,    67,     0,     0,     0,
       8,     9,    10,     0,     0,     6,     0,    67,     0,    65,
      64,     0,     0,     0,     0,     0,     0,     0,     0,    69,
       0,     0,     0,     0,    70,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       1,     0,     0,     0,    11,    17,     0,    24,    25,     0,
      41,    43,     7,    66,     4,     5,     2,     3,    38,    40,
      37,    39,    42,    35,    44,    36,     0,    61,    60,    54,
      55,    56,    57,    58,    59,     0,    14,    26,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    13,     0,
      22,     0,    23,     0,     0,    48,    47,    52,    51,    46,
      45,    50,    49,    53,    33,    34,    16,     0,    31,     0,
      32,     0,     0,     0,     0,     0,     0,    63,     0,     0,
       0,     0,     0,    12,    18,    20,    19,    21,    15,    27,
      29,    28,    30
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  whereParser::yypgoto_[] =
  {
       -31,    18,   -31,   -31,   -31,    -3,   -31
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  whereParser::yydefgoto_[] =
  {
        -1,     9,    10,    11,    12,    13,    14
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If zero, do what YYDEFACT says.  */
  const signed char whereParser::yytable_ninf_ = -1;
  const unsigned char
  whereParser::yytable_[] =
  {
        19,    20,    16,   103,    50,    28,    30,    31,    32,    33,
     104,    51,    29,    89,    90,    52,    24,    91,    53,    15,
      60,    59,    30,    31,    32,    33,    27,   105,   106,    62,
      61,   107,    68,    69,    70,    71,    73,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    34,    64,    65,
      66,    67,    35,    36,    37,    38,    39,    40,    54,    55,
       1,    56,    41,    46,    47,    48,    49,    42,    43,    44,
      45,    46,    47,    48,    49,     2,     3,     4,     3,     4,
      85,    63,     5,    17,     5,     6,    72,    18,     7,     8,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
      21,    25,    26,   124,    35,    36,    37,    38,    39,    40,
     108,   109,    22,    23,    41,    94,    49,    95,   125,    42,
      43,    44,    45,    46,    47,    48,    49,    30,    31,    24,
      42,    43,    44,    45,    46,    47,    48,    49,    98,   132,
      99,    44,    45,    46,    47,    48,    49,    96,   138,    97,
       0,   110,   111,    42,    43,    44,    45,    46,    47,    48,
      49,    42,    43,    44,    45,    46,    47,    48,    49,   100,
     143,   101,    42,    43,    44,    45,    46,    47,    48,    49,
     112,   113,   102,    42,    43,    44,    45,    46,    47,    48,
      49,    42,    43,    44,    45,    46,    47,    48,    49,    42,
      43,    44,    45,    46,    47,    48,    49,    86,    87,    57,
      88,     0,    58,    92,    93,    42,    43,    44,    45,    46,
      47,    48,    49,    30,    31,   133,    33,   144,   134,    63,
      42,    43,    44,    45,    46,    47,    48,    49,   145,     3,
       4,     3,     4,   146,   137,     5,    17,     5,    17,    74,
      18,     0,    18,    43,    44,    45,    46,    47,    48,    49,
     135,   147,   139,   136,   141,   140,     0,   142,   126,   127,
     128,   129,   130,   131,   148,   149,   150,   151,   152
  };

  /* YYCHECK.  */
  const signed char
  whereParser::yycheck_[] =
  {
         3,     4,    32,     8,     0,     8,    10,    11,    12,    13,
      15,    28,     0,    27,    28,    18,    32,    31,    15,     1,
      28,    24,    10,    11,    12,    13,     8,    27,    28,    33,
      28,    31,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    35,    30,    31,
      32,    33,     4,     5,     6,     7,     8,     9,    29,    30,
       3,    32,    14,    23,    24,    25,    26,    19,    20,    21,
      22,    23,    24,    25,    26,    18,    21,    22,    21,    22,
      33,    33,    27,    28,    27,    28,    31,    32,    31,    32,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
       3,     8,     9,    27,     4,     5,     6,     7,     8,     9,
      33,    34,    15,    16,    14,     4,    26,     6,    29,    19,
      20,    21,    22,    23,    24,    25,    26,    10,    11,    32,
      19,    20,    21,    22,    23,    24,    25,    26,     4,    27,
       6,    21,    22,    23,    24,    25,    26,     5,    27,     7,
      -1,    33,    34,    19,    20,    21,    22,    23,    24,    25,
      26,    19,    20,    21,    22,    23,    24,    25,    26,     5,
      33,     7,    19,    20,    21,    22,    23,    24,    25,    26,
      33,    34,    10,    19,    20,    21,    22,    23,    24,    25,
      26,    19,    20,    21,    22,    23,    24,    25,    26,    19,
      20,    21,    22,    23,    24,    25,    26,    29,    30,    28,
      32,    -1,    31,    33,    34,    19,    20,    21,    22,    23,
      24,    25,    26,    10,    11,    28,    13,    33,    31,    33,
      19,    20,    21,    22,    23,    24,    25,    26,    33,    21,
      22,    21,    22,    33,    33,    27,    28,    27,    28,    31,
      32,    -1,    32,    20,    21,    22,    23,    24,    25,    26,
      28,    33,    28,    31,    28,    31,    -1,    31,    33,    34,
      33,    34,    33,    34,    33,    33,    33,    33,    33
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  whereParser::yystos_[] =
  {
         0,     3,    18,    21,    22,    27,    28,    31,    32,    37,
      38,    39,    40,    41,    42,    37,    32,    28,    32,    41,
      41,     3,    15,    16,    32,     8,     9,    37,    41,     0,
      10,    11,    12,    13,    35,     4,     5,     6,     7,     8,
       9,    14,    19,    20,    21,    22,    23,    24,    25,    26,
       0,    28,    41,    15,    29,    30,    32,    28,    31,    41,
      28,    28,    33,    33,    37,    37,    37,    37,    41,    41,
      41,    41,    31,    41,    31,    41,    41,    41,    41,    41,
      41,    41,    41,    41,    41,    33,    29,    30,    32,    27,
      28,    31,    33,    34,     4,     6,     5,     7,     4,     6,
       5,     7,    10,     8,    15,    27,    28,    31,    33,    34,
      33,    34,    33,    34,    41,    41,    41,    41,    41,    41,
      41,    41,    41,    41,    27,    29,    33,    34,    33,    34,
      33,    34,    27,    28,    31,    28,    31,    33,    27,    28,
      31,    28,    31,    33,    33,    33,    33,    33,    33,    33,
      33,    33,    33
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  whereParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,    40,    41,    44,    59
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  whereParser::yyr1_[] =
  {
         0,    36,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    38,    38,    38,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,    38,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    40,    40,    40,    40,    40,
      40,    40,    40,    40,    41,    41,    41,    41,    41,    41,
      41,    41,    41,    41,    41,    41,    41,    41,    41,    42,
      42
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  whereParser::yyr2_[] =
  {
         0,     2,     3,     3,     3,     3,     2,     3,     1,     1,
       1,     3,     7,     5,     4,     8,     6,     3,     7,     7,
       7,     7,     5,     5,     3,     3,     4,     8,     8,     8,
       8,     6,     6,     6,     6,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     3,     3,     3,     3,     3,     3,
       3,     3,     4,     6,     2,     2,     3,     1,     1,     2,
       2
  };

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const whereParser::yytname_[] =
  {
    "\"end of input\"", "error", "$undefined", "\"not\"", "\"<=\"",
  "\">=\"", "\"<\"", "\">\"", "\"==\"", "\"!=\"", "\"and\"", "\"&!\"",
  "\"or\"", "\"xor\"", "\"between\"", "\"in\"", "\"like\"",
  "\"self-join\"", "\"any\"", "\"|\"", "\"&\"", "\"+\"", "\"-\"", "\"*\"",
  "\"/\"", "\"%\"", "\"**\"", "\"numerical value\"", "\"name\"",
  "\"number sequence\"", "\"string sequence\"", "\"string literal\"",
  "'('", "')'", "','", "';'", "$accept", "qexpr", "simpleRange",
  "compRange2", "compRange3", "mathExpr", "START", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const whereParser::rhs_number_type
  whereParser::yyrhs_[] =
  {
        42,     0,    -1,    37,    12,    37,    -1,    37,    13,    37,
      -1,    37,    10,    37,    -1,    37,    11,    37,    -1,     3,
      37,    -1,    32,    37,    33,    -1,    38,    -1,    39,    -1,
      40,    -1,    28,    15,    29,    -1,    28,    15,    32,    27,
      34,    27,    33,    -1,    28,    15,    32,    27,    33,    -1,
      28,     3,    15,    29,    -1,    28,     3,    15,    32,    27,
      34,    27,    33,    -1,    28,     3,    15,    32,    27,    33,
      -1,    28,    15,    30,    -1,    28,    15,    32,    28,    34,
      28,    33,    -1,    28,    15,    32,    31,    34,    28,    33,
      -1,    28,    15,    32,    28,    34,    31,    33,    -1,    28,
      15,    32,    31,    34,    31,    33,    -1,    28,    15,    32,
      28,    33,    -1,    28,    15,    32,    31,    33,    -1,    28,
      16,    28,    -1,    28,    16,    31,    -1,    28,     3,    15,
      30,    -1,    28,     3,    15,    32,    28,    34,    28,    33,
      -1,    28,     3,    15,    32,    31,    34,    28,    33,    -1,
      28,     3,    15,    32,    28,    34,    31,    33,    -1,    28,
       3,    15,    32,    31,    34,    31,    33,    -1,    28,     3,
      15,    32,    28,    33,    -1,    28,     3,    15,    32,    31,
      33,    -1,    18,    32,    28,    33,     8,    27,    -1,    18,
      32,    28,    33,    15,    29,    -1,    41,     8,    41,    -1,
      41,     9,    41,    -1,    41,     6,    41,    -1,    41,     4,
      41,    -1,    41,     7,    41,    -1,    41,     5,    41,    -1,
      31,     8,    28,    -1,    41,     8,    31,    -1,    31,     9,
      28,    -1,    41,     9,    31,    -1,    41,     6,    41,     6,
      41,    -1,    41,     6,    41,     4,    41,    -1,    41,     4,
      41,     6,    41,    -1,    41,     4,    41,     4,    41,    -1,
      41,     7,    41,     7,    41,    -1,    41,     7,    41,     5,
      41,    -1,    41,     5,    41,     7,    41,    -1,    41,     5,
      41,     5,    41,    -1,    41,    14,    41,    10,    41,    -1,
      41,    21,    41,    -1,    41,    22,    41,    -1,    41,    23,
      41,    -1,    41,    24,    41,    -1,    41,    25,    41,    -1,
      41,    26,    41,    -1,    41,    20,    41,    -1,    41,    19,
      41,    -1,    28,    32,    41,    33,    -1,    28,    32,    41,
      34,    41,    33,    -1,    22,    41,    -1,    21,    41,    -1,
      32,    41,    33,    -1,    28,    -1,    27,    -1,    37,     0,
      -1,    37,    35,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  whereParser::yyprhs_[] =
  {
         0,     0,     3,     7,    11,    15,    19,    22,    26,    28,
      30,    32,    36,    44,    50,    55,    64,    71,    75,    83,
      91,    99,   107,   113,   119,   123,   127,   132,   141,   150,
     159,   168,   175,   182,   189,   196,   200,   204,   208,   212,
     216,   220,   224,   228,   232,   236,   242,   248,   254,   260,
     266,   272,   278,   284,   290,   294,   298,   302,   306,   310,
     314,   318,   322,   327,   334,   337,   340,   344,   346,   348,
     351
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  whereParser::yyrline_[] =
  {
         0,    99,    99,   109,   119,   129,   139,   147,   150,   151,
     152,   156,   166,   178,   187,   198,   211,   221,   231,   248,
     265,   282,   299,   313,   327,   337,   347,   358,   376,   394,
     412,   430,   445,   460,   469,   482,   492,   503,   513,   523,
     533,   543,   553,   577,   588,   616,   628,   640,   652,   664,
     676,   688,   700,   712,   727,   739,   751,   763,   775,   787,
     799,   811,   823,   835,   848,   858,   861,   864,   874,   884,
     887
  };

  // Print the state stack on the debug stream.
  void
  whereParser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (state_stack_type::const_iterator i = yystate_stack_.begin ();
	 i != yystate_stack_.end (); ++i)
      *yycdebug_ << ' ' << *i;
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  whereParser::yy_reduce_print_ (int yyrule)
  {
    unsigned int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    /* Print the symbols being reduced, and their result.  */
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
	       << " (line " << yylno << "), ";
    /* The symbols being reduced.  */
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
		       yyrhs_[yyprhs_[yyrule] + yyi],
		       &(yysemantic_stack_[(yynrhs) - (yyi + 1)]),
		       &(yylocation_stack_[(yynrhs) - (yyi + 1)]));
  }
#endif // YYDEBUG

  /* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
  whereParser::token_number_type
  whereParser::yytranslate_ (int t)
  {
    static
    const token_number_type
    translate_table[] =
    {
           0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      32,    33,     2,     2,    34,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    35,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int whereParser::yyeof_ = 0;
  const int whereParser::yylast_ = 278;
  const int whereParser::yynnts_ = 7;
  const int whereParser::yyempty_ = -2;
  const int whereParser::yyfinal_ = 50;
  const int whereParser::yyterror_ = 1;
  const int whereParser::yyerrcode_ = 256;
  const int whereParser::yyntokens_ = 36;

  const unsigned int whereParser::yyuser_token_number_max_ = 286;
  const whereParser::token_number_type whereParser::yyundef_token_ = 2;

} // namespace ibis

#line 892 "whereParser.yy"

void ibis::whereParser::error(const ibis::whereParser::location_type& l,
			      const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::whereParser encountered " << m << " at location " << l;
}


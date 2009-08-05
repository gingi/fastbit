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

#include "selectParser.hh"

/* User implementation prologue.  */
#line 69 "selectParser.yy"

#include "selectLexer.h"

#undef yylex
#define yylex driver.lexer->lex


/* Line 317 of lalr1.cc.  */
#line 51 "selectParser.cc"

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
  selectParser::yytnamerr_ (const char *yystr)
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
  selectParser::selectParser (class ibis::selectClause& driver_yyarg)
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
      driver (driver_yyarg)
  {
  }

  selectParser::~selectParser ()
  {
  }

#if YYDEBUG
  /*--------------------------------.
  | Print this symbol on YYOUTPUT.  |
  `--------------------------------*/

  inline void
  selectParser::yy_symbol_value_print_ (int yytype,
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
  selectParser::yy_symbol_print_ (int yytype,
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
  selectParser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
      {
        case 18: /* "\"name\"" */
#line 66 "selectParser.yy"
	{ delete (yyvaluep->stringVal); };
#line 211 "selectParser.cc"
	break;
      case 25: /* "mathExpr" */
#line 67 "selectParser.yy"
	{ delete (yyvaluep->selectNode); };
#line 216 "selectParser.cc"
	break;

	default:
	  break;
      }
  }

  void
  selectParser::yypop_ (unsigned int n)
  {
    yystate_stack_.pop (n);
    yysemantic_stack_.pop (n);
    yylocation_stack_.pop (n);
  }

  std::ostream&
  selectParser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  selectParser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  selectParser::debug_level_type
  selectParser::debug_level () const
  {
    return yydebug_;
  }

  void
  selectParser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }


  int
  selectParser::parse ()
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
    #line 26 "selectParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}
  /* Line 547 of yacc.c.  */
#line 297 "selectParser.cc"
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
	  case 4:
#line 78 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 5:
#line 82 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 6:
#line 86 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 7:
#line 90 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 8:
#line 94 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 9:
#line 98 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 10:
#line 102 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 11:
#line 106 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 12:
#line 110 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 13:
#line 114 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 14:
#line 118 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 15:
#line 122 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 16:
#line 126 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 17:
#line 130 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 18:
#line 134 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 19:
#line 139 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 20:
#line 144 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 21:
#line 149 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 22:
#line 154 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 23:
#line 159 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 24:
#line 164 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 25:
#line 169 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 26:
#line 174 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 27:
#line 179 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 28:
#line 184 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 29:
#line 189 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 30:
#line 194 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 31:
#line 199 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 32:
#line 207 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " + " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::PLUS);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 33:
#line 219 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " - " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MINUS);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 34:
#line 231 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " * " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MULTIPLY);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 35:
#line 243 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " / " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::DIVIDE);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 36:
#line 255 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " % " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::REMAINDER);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 37:
#line 267 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " ^ " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::POWER);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 38:
#line 279 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " & " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITAND);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 39:
#line 291 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].selectNode)
	<< " | " << *(yysemantic_stack_[(3) - (3)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITOR);
    opr->setRight((yysemantic_stack_[(3) - (3)].selectNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 40:
#line 303 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(4) - (3)].selectNode) << ")";
#endif
    ibis::math::stdFunction1 *fun =
	new ibis::math::stdFunction1((yysemantic_stack_[(4) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(4) - (1)].stringVal);
    fun->setLeft((yysemantic_stack_[(4) - (3)].selectNode));
    (yyval.selectNode) = fun;
;}
    break;

  case 41:
#line 315 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(6) - (3)].selectNode) << ", " << *(yysemantic_stack_[(6) - (5)].selectNode) << ")";
#endif
    ibis::math::stdFunction2 *fun =
	new ibis::math::stdFunction2((yysemantic_stack_[(6) - (1)].stringVal)->c_str());
    fun->setRight((yysemantic_stack_[(6) - (5)].selectNode));
    fun->setLeft((yysemantic_stack_[(6) - (3)].selectNode));
    (yyval.selectNode) = fun;
    delete (yysemantic_stack_[(6) - (1)].stringVal);
;}
    break;

  case 42:
#line 328 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *(yysemantic_stack_[(2) - (2)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight((yysemantic_stack_[(2) - (2)].selectNode));
    (yyval.selectNode) = opr;
;}
    break;

  case 43:
#line 338 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(2) - (2)].selectNode);
;}
    break;

  case 44:
#line 341 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(3) - (2)].selectNode);
;}
    break;

  case 45:
#line 344 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.selectNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
;}
    break;

  case 46:
#line 352 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.selectNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
;}
    break;


    /* Line 675 of lalr1.cc.  */
#line 864 "selectParser.cc"
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
	error (yylloc, yysyntax_error_ (yystate, yytoken));
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
  selectParser::yysyntax_error_ (int yystate, int tok)
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
  const signed char selectParser::yypact_ninf_ = -18;
  const short int
  selectParser::yypact_[] =
  {
        79,   -17,   -16,   -14,   -13,   -12,   185,   185,   -18,    -1,
     185,    19,    79,    29,   185,   181,   185,   185,   185,   -18,
     -18,   185,   103,   -18,   -18,   -18,     2,   185,   185,   185,
     185,   185,   185,   185,   185,   -18,   115,    11,   127,   139,
     151,   163,    90,   -18,    36,   195,   201,    49,    49,    50,
      50,    50,   -18,    13,    14,    30,    46,    52,    53,   -18,
     185,   -18,   -18,   -18,    54,   -18,   -18,    57,   -18,   -18,
      58,   -18,   -18,    70,   -18,   -18,    71,   -18,   -18,    74,
     -18,   175,    37,    47,    48,    59,    60,    61,   -18,   -18,
     -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,
     -18
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  selectParser::yydefact_[] =
  {
         0,     0,     0,     0,     0,     0,     0,     0,    46,    45,
       0,     0,     2,     0,     0,     0,     0,     0,     0,    43,
      42,     0,     0,     1,     3,    17,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    16,     0,     0,     0,     0,
       0,     0,     0,    44,     0,    39,    38,    32,    33,    34,
      35,    36,    37,     0,     0,     0,     0,     0,     0,    40,
       0,    31,    30,     5,     0,     4,     9,     0,     8,     7,
       0,     6,    13,     0,    12,    11,     0,    10,    15,     0,
      14,     0,     0,     0,     0,     0,     0,     0,    41,    19,
      18,    23,    22,    21,    20,    27,    26,    25,    24,    29,
      28
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yypgoto_[] =
  {
       -18,    65,   -18,    -6
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yydefgoto_[] =
  {
        -1,    11,    12,    13
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If zero, do what YYDEFACT says.  */
  const signed char selectParser::yytable_ninf_ = -1;
  const unsigned char
  selectParser::yytable_[] =
  {
        19,    20,    14,    15,    22,    16,    17,    18,    36,    38,
      39,    40,    41,    63,    66,    42,    64,    67,    21,    23,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    25,
      69,    54,    26,    70,    65,    68,    61,    89,    27,    28,
      29,    30,    31,    32,    33,    34,    72,    91,    93,    73,
      35,    71,    75,    78,    81,    76,    79,    62,    90,    95,
      97,    99,    31,    32,    33,    34,    34,    74,    92,    94,
       0,     0,    82,    77,    80,    83,    84,    24,     0,     0,
      96,    98,   100,     1,     2,     3,     4,     5,    85,    86,
       6,     7,    87,     0,     0,     0,     8,     9,    10,    27,
      28,    29,    30,    31,    32,    33,    34,     0,     0,     0,
      59,    60,    27,    28,    29,    30,    31,    32,    33,    34,
       0,     0,     0,    43,    27,    28,    29,    30,    31,    32,
      33,    34,     0,     0,     0,    53,    27,    28,    29,    30,
      31,    32,    33,    34,     0,     0,     0,    55,    27,    28,
      29,    30,    31,    32,    33,    34,     0,     0,     0,    56,
      27,    28,    29,    30,    31,    32,    33,    34,     0,     0,
       0,    57,    27,    28,    29,    30,    31,    32,    33,    34,
       0,     0,     0,    58,    27,    28,    29,    30,    31,    32,
      33,    34,     6,     7,    37,    88,     6,     7,     8,     9,
      10,     0,     8,     9,    10,    28,    29,    30,    31,    32,
      33,    34,    29,    30,    31,    32,    33,    34
  };

  /* YYCHECK.  */
  const signed char
  selectParser::yycheck_[] =
  {
         6,     7,    19,    19,    10,    19,    19,    19,    14,    15,
      16,    17,    18,     0,     0,    21,     3,     3,    19,     0,
      18,    27,    28,    29,    30,    31,    32,    33,    34,     0,
       0,    20,     3,     3,    21,    21,     0,     0,     9,    10,
      11,    12,    13,    14,    15,    16,     0,     0,     0,     3,
      21,    21,     0,     0,    60,     3,     3,    21,    21,     0,
       0,     0,    13,    14,    15,    16,    16,    21,    21,    21,
      -1,    -1,    18,    21,    21,    18,    18,    12,    -1,    -1,
      21,    21,    21,     4,     5,     6,     7,     8,    18,    18,
      11,    12,    18,    -1,    -1,    -1,    17,    18,    19,     9,
      10,    11,    12,    13,    14,    15,    16,    -1,    -1,    -1,
      20,    21,     9,    10,    11,    12,    13,    14,    15,    16,
      -1,    -1,    -1,    20,     9,    10,    11,    12,    13,    14,
      15,    16,    -1,    -1,    -1,    20,     9,    10,    11,    12,
      13,    14,    15,    16,    -1,    -1,    -1,    20,     9,    10,
      11,    12,    13,    14,    15,    16,    -1,    -1,    -1,    20,
       9,    10,    11,    12,    13,    14,    15,    16,    -1,    -1,
      -1,    20,     9,    10,    11,    12,    13,    14,    15,    16,
      -1,    -1,    -1,    20,     9,    10,    11,    12,    13,    14,
      15,    16,    11,    12,    13,    20,    11,    12,    17,    18,
      19,    -1,    17,    18,    19,    10,    11,    12,    13,    14,
      15,    16,    11,    12,    13,    14,    15,    16
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  selectParser::yystos_[] =
  {
         0,     4,     5,     6,     7,     8,    11,    12,    17,    18,
      19,    23,    24,    25,    19,    19,    19,    19,    19,    25,
      25,    19,    25,     0,    23,     0,     3,     9,    10,    11,
      12,    13,    14,    15,    16,    21,    25,    13,    25,    25,
      25,    25,    25,    20,    18,    25,    25,    25,    25,    25,
      25,    25,    25,    20,    20,    20,    20,    20,    20,    20,
      21,     0,    21,     0,     3,    21,     0,     3,    21,     0,
       3,    21,     0,     3,    21,     0,     3,    21,     0,     3,
      21,    25,    18,    18,    18,    18,    18,    18,    20,     0,
      21,     0,    21,     0,    21,     0,    21,     0,    21,     0,
      21
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  selectParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,    40,
      41,    44
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  selectParser::yyr1_[] =
  {
         0,    22,    23,    23,    24,    24,    24,    24,    24,    24,
      24,    24,    24,    24,    24,    24,    24,    24,    24,    24,
      24,    24,    24,    24,    24,    24,    24,    24,    24,    24,
      24,    24,    25,    25,    25,    25,    25,    25,    25,    25,
      25,    25,    25,    25,    25,    25,    25
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  selectParser::yyr2_[] =
  {
         0,     2,     1,     2,     5,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     5,     5,     2,     2,     7,     7,
       7,     7,     7,     7,     7,     7,     7,     7,     7,     7,
       4,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       4,     6,     2,     2,     3,     1,     1
  };

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const selectParser::yytname_[] =
  {
    "\"end of input\"", "error", "$undefined", "\"as\"", "\"avg\"",
  "\"count\"", "\"min\"", "\"max\"", "\"sum\"", "\"|\"", "\"&\"", "\"+\"",
  "\"-\"", "\"*\"", "\"/\"", "\"%\"", "\"**\"", "\"numerical value\"",
  "\"name\"", "'('", "')'", "','", "$accept", "slist", "sterm", "mathExpr", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const selectParser::rhs_number_type
  selectParser::yyrhs_[] =
  {
        23,     0,    -1,    24,    -1,    24,    23,    -1,     4,    19,
      25,    20,    21,    -1,     4,    19,    25,    20,     0,    -1,
       5,    19,    25,    20,    21,    -1,     5,    19,    25,    20,
       0,    -1,     5,    19,    13,    20,    21,    -1,     5,    19,
      13,    20,     0,    -1,     7,    19,    25,    20,    21,    -1,
       7,    19,    25,    20,     0,    -1,     6,    19,    25,    20,
      21,    -1,     6,    19,    25,    20,     0,    -1,     8,    19,
      25,    20,    21,    -1,     8,    19,    25,    20,     0,    -1,
      25,    21,    -1,    25,     0,    -1,     4,    19,    25,    20,
       3,    18,    21,    -1,     4,    19,    25,    20,     3,    18,
       0,    -1,     5,    19,    25,    20,     3,    18,    21,    -1,
       5,    19,    25,    20,     3,    18,     0,    -1,     5,    19,
      13,    20,     3,    18,    21,    -1,     5,    19,    13,    20,
       3,    18,     0,    -1,     7,    19,    25,    20,     3,    18,
      21,    -1,     7,    19,    25,    20,     3,    18,     0,    -1,
       6,    19,    25,    20,     3,    18,    21,    -1,     6,    19,
      25,    20,     3,    18,     0,    -1,     8,    19,    25,    20,
       3,    18,    21,    -1,     8,    19,    25,    20,     3,    18,
       0,    -1,    25,     3,    18,    21,    -1,    25,     3,    18,
       0,    -1,    25,    11,    25,    -1,    25,    12,    25,    -1,
      25,    13,    25,    -1,    25,    14,    25,    -1,    25,    15,
      25,    -1,    25,    16,    25,    -1,    25,    10,    25,    -1,
      25,     9,    25,    -1,    18,    19,    25,    20,    -1,    18,
      19,    25,    21,    25,    20,    -1,    12,    25,    -1,    11,
      25,    -1,    19,    25,    20,    -1,    18,    -1,    17,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned char
  selectParser::yyprhs_[] =
  {
         0,     0,     3,     5,     8,    14,    20,    26,    32,    38,
      44,    50,    56,    62,    68,    74,    80,    83,    86,    94,
     102,   110,   118,   126,   134,   142,   150,   158,   166,   174,
     182,   187,   192,   196,   200,   204,   208,   212,   216,   220,
     224,   229,   236,   239,   242,   246,   248
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  selectParser::yyrline_[] =
  {
         0,    77,    77,    77,    78,    82,    86,    90,    94,    98,
     102,   106,   110,   114,   118,   122,   126,   130,   134,   139,
     144,   149,   154,   159,   164,   169,   174,   179,   184,   189,
     194,   199,   207,   219,   231,   243,   255,   267,   279,   291,
     303,   315,   328,   338,   341,   344,   352
  };

  // Print the state stack on the debug stream.
  void
  selectParser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (state_stack_type::const_iterator i = yystate_stack_.begin ();
	 i != yystate_stack_.end (); ++i)
      *yycdebug_ << ' ' << *i;
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  selectParser::yy_reduce_print_ (int yyrule)
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
  selectParser::token_number_type
  selectParser::yytranslate_ (int t)
  {
    static
    const token_number_type
    translate_table[] =
    {
           0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      19,    20,     2,     2,    21,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int selectParser::yyeof_ = 0;
  const int selectParser::yylast_ = 217;
  const int selectParser::yynnts_ = 4;
  const int selectParser::yyempty_ = -2;
  const int selectParser::yyfinal_ = 23;
  const int selectParser::yyterror_ = 1;
  const int selectParser::yyerrcode_ = 256;
  const int selectParser::yyntokens_ = 22;

  const unsigned int selectParser::yyuser_token_number_max_ = 273;
  const selectParser::token_number_type selectParser::yyundef_token_ = 2;

} // namespace ibis

#line 361 "selectParser.yy"

void ibis::selectParser::error(const ibis::selectParser::location_type& l,
			       const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::selectParser encountered " << m << " at location " << l;
}


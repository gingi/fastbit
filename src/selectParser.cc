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
#line 74 "selectParser.yy"

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
        case 23: /* "\"name\"" */
#line 71 "selectParser.yy"
	{ delete (yyvaluep->stringVal); };
#line 211 "selectParser.cc"
	break;
      case 30: /* "mathExpr" */
#line 72 "selectParser.yy"
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
#line 83 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 5:
#line 87 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 6:
#line 91 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 7:
#line 95 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 8:
#line 99 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 9:
#line 103 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 10:
#line 107 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 11:
#line 111 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 12:
#line 115 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 13:
#line 119 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 14:
#line 123 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 15:
#line 127 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 16:
#line 131 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
;}
    break;

  case 17:
#line 135 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
;}
    break;

  case 18:
#line 139 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
;}
    break;

  case 19:
#line 143 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
;}
    break;

  case 20:
#line 147 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
;}
    break;

  case 21:
#line 151 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
;}
    break;

  case 22:
#line 155 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
;}
    break;

  case 23:
#line 159 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
;}
    break;

  case 24:
#line 163 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
;}
    break;

  case 25:
#line 167 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
;}
    break;

  case 26:
#line 171 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 27:
#line 175 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 28:
#line 179 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 29:
#line 184 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
;}
    break;

  case 30:
#line 189 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 31:
#line 194 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 32:
#line 199 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 33:
#line 204 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
;}
    break;

  case 34:
#line 209 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 35:
#line 214 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
;}
    break;

  case 36:
#line 219 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 37:
#line 224 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
;}
    break;

  case 38:
#line 229 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 39:
#line 234 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
;}
    break;

  case 40:
#line 239 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 41:
#line 244 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL);
;}
    break;

  case 42:
#line 252 "selectParser.yy"
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

  case 43:
#line 264 "selectParser.yy"
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

  case 44:
#line 276 "selectParser.yy"
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

  case 45:
#line 288 "selectParser.yy"
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

  case 46:
#line 300 "selectParser.yy"
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

  case 47:
#line 312 "selectParser.yy"
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

  case 48:
#line 324 "selectParser.yy"
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

  case 49:
#line 336 "selectParser.yy"
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

  case 50:
#line 348 "selectParser.yy"
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

  case 51:
#line 360 "selectParser.yy"
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

  case 52:
#line 373 "selectParser.yy"
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

  case 53:
#line 383 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(2) - (2)].selectNode);
;}
    break;

  case 54:
#line 386 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(3) - (2)].selectNode);
;}
    break;

  case 55:
#line 389 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.selectNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
;}
    break;

  case 56:
#line 397 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.selectNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
;}
    break;


    /* Line 675 of lalr1.cc.  */
#line 944 "selectParser.cc"
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
  const signed char selectParser::yypact_ninf_ = -23;
  const short int
  selectParser::yypact_[] =
  {
        90,   -22,   -21,   -19,   -18,   -17,    -1,     1,    44,    47,
      49,   256,   256,   -23,    51,   256,    43,    90,    21,   256,
     252,   256,   256,   256,   256,   256,   256,   256,   256,    60,
      60,   256,   114,   -23,   -23,   -23,    81,   256,   256,   256,
     256,   256,   256,   256,   256,   -23,   126,    45,   138,   150,
     162,   174,   186,   198,   210,   222,   234,   101,   -23,    18,
     266,   272,   276,   276,    60,    60,    60,    60,    19,    46,
      48,    50,    52,    54,    34,    56,    58,    59,    61,   -23,
     256,   -23,   -23,   -23,    82,   -23,   -23,    85,   -23,   -23,
      86,   -23,   -23,    87,   -23,   -23,    88,   -23,   -23,   100,
     -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,
     -23,   246,    62,    63,    64,    65,    66,    67,   -23,   -23,
     -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,
     -23
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  selectParser::yydefact_[] =
  {
         0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    56,    55,     0,     0,     2,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    53,
      52,     0,     0,     1,     3,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    54,     0,
      49,    48,    42,    43,    44,    45,    46,    47,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    50,
       0,    41,    40,     5,     0,     4,     9,     0,     8,     7,
       0,     6,    13,     0,    12,    11,     0,    10,    15,     0,
      14,    17,    16,    19,    18,    21,    20,    23,    22,    25,
      24,     0,     0,     0,     0,     0,     0,     0,    51,    29,
      28,    33,    32,    31,    30,    37,    36,    35,    34,    39,
      38
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yypgoto_[] =
  {
       -23,    69,   -23,   -11
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yydefgoto_[] =
  {
        -1,    16,    17,    18
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If zero, do what YYDEFACT says.  */
  const signed char selectParser::yytable_ninf_ = -1;
  const unsigned char
  selectParser::yytable_[] =
  {
        29,    30,    19,    20,    32,    21,    22,    23,    46,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    81,    83,
      57,    35,    84,    24,    36,    25,    60,    61,    62,    63,
      64,    65,    66,    67,   101,    37,    38,    39,    40,    41,
      42,    43,    44,    33,    82,    85,    86,    45,    89,    87,
      92,    90,    95,    93,    98,    96,   103,    99,   105,   107,
     102,   109,   119,   121,   123,   125,   127,   129,    26,   111,
      69,    27,    88,    28,    91,    31,    94,     0,    97,     0,
     100,    44,   104,     0,   106,   108,    34,   110,   120,   122,
     124,   126,   128,   130,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    59,   112,    11,    12,   113,   114,
     115,   116,    13,    14,    15,    37,    38,    39,    40,    41,
      42,    43,    44,   117,     0,     0,    79,    80,    37,    38,
      39,    40,    41,    42,    43,    44,     0,     0,     0,    58,
      37,    38,    39,    40,    41,    42,    43,    44,     0,     0,
       0,    68,    37,    38,    39,    40,    41,    42,    43,    44,
       0,     0,     0,    70,    37,    38,    39,    40,    41,    42,
      43,    44,     0,     0,     0,    71,    37,    38,    39,    40,
      41,    42,    43,    44,     0,     0,     0,    72,    37,    38,
      39,    40,    41,    42,    43,    44,     0,     0,     0,    73,
      37,    38,    39,    40,    41,    42,    43,    44,     0,     0,
       0,    74,    37,    38,    39,    40,    41,    42,    43,    44,
       0,     0,     0,    75,    37,    38,    39,    40,    41,    42,
      43,    44,     0,     0,     0,    76,    37,    38,    39,    40,
      41,    42,    43,    44,     0,     0,     0,    77,    37,    38,
      39,    40,    41,    42,    43,    44,     0,     0,     0,    78,
      37,    38,    39,    40,    41,    42,    43,    44,    11,    12,
      47,   118,    11,    12,    13,    14,    15,     0,    13,    14,
      15,    38,    39,    40,    41,    42,    43,    44,    39,    40,
      41,    42,    43,    44,    41,    42,    43,    44
  };

  /* YYCHECK.  */
  const signed char
  selectParser::yycheck_[] =
  {
        11,    12,    24,    24,    15,    24,    24,    24,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,     0,     0,
      31,     0,     3,    24,     3,    24,    37,    38,    39,    40,
      41,    42,    43,    44,     0,    14,    15,    16,    17,    18,
      19,    20,    21,     0,    26,    26,     0,    26,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,    24,    80,
      25,    24,    26,    24,    26,    24,    26,    -1,    26,    -1,
      26,    21,    26,    -1,    26,    26,    17,    26,    26,    26,
      26,    26,    26,    26,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    23,    23,    16,    17,    23,    23,
      23,    23,    22,    23,    24,    14,    15,    16,    17,    18,
      19,    20,    21,    23,    -1,    -1,    25,    26,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    25,
      14,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      -1,    25,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    -1,    25,    14,    15,    16,    17,    18,    19,
      20,    21,    -1,    -1,    -1,    25,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    -1,    25,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    25,
      14,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      -1,    25,    14,    15,    16,    17,    18,    19,    20,    21,
      -1,    -1,    -1,    25,    14,    15,    16,    17,    18,    19,
      20,    21,    -1,    -1,    -1,    25,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    -1,    25,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    25,
      14,    15,    16,    17,    18,    19,    20,    21,    16,    17,
      18,    25,    16,    17,    22,    23,    24,    -1,    22,    23,
      24,    15,    16,    17,    18,    19,    20,    21,    16,    17,
      18,    19,    20,    21,    18,    19,    20,    21
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  selectParser::yystos_[] =
  {
         0,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    16,    17,    22,    23,    24,    28,    29,    30,    24,
      24,    24,    24,    24,    24,    24,    24,    24,    24,    30,
      30,    24,    30,     0,    28,     0,     3,    14,    15,    16,
      17,    18,    19,    20,    21,    26,    30,    18,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    25,    23,
      30,    30,    30,    30,    30,    30,    30,    30,    25,    25,
      25,    25,    25,    25,    25,    25,    25,    25,    25,    25,
      26,     0,    26,     0,     3,    26,     0,     3,    26,     0,
       3,    26,     0,     3,    26,     0,     3,    26,     0,     3,
      26,     0,    26,     0,    26,     0,    26,     0,    26,     0,
      26,    30,    23,    23,    23,    23,    23,    23,    25,     0,
      26,     0,    26,     0,    26,     0,    26,     0,    26,     0,
      26
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  selectParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,    40,    41,    44
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  selectParser::yyr1_[] =
  {
         0,    27,    28,    28,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
      29,    29,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  selectParser::yyr2_[] =
  {
         0,     2,     1,     2,     5,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     5,     5,     5,     5,     5,     5,
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
  "\"count\"", "\"min\"", "\"max\"", "\"sum\"", "\"varpop\"",
  "\"varsamp\"", "\"stdpop\"", "\"stdsamp\"", "\"distinct\"", "\"|\"",
  "\"&\"", "\"+\"", "\"-\"", "\"*\"", "\"/\"", "\"%\"", "\"**\"",
  "\"numerical value\"", "\"name\"", "'('", "')'", "','", "$accept",
  "slist", "sterm", "mathExpr", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const selectParser::rhs_number_type
  selectParser::yyrhs_[] =
  {
        28,     0,    -1,    29,    -1,    29,    28,    -1,     4,    24,
      30,    25,    26,    -1,     4,    24,    30,    25,     0,    -1,
       5,    24,    30,    25,    26,    -1,     5,    24,    30,    25,
       0,    -1,     5,    24,    18,    25,    26,    -1,     5,    24,
      18,    25,     0,    -1,     7,    24,    30,    25,    26,    -1,
       7,    24,    30,    25,     0,    -1,     6,    24,    30,    25,
      26,    -1,     6,    24,    30,    25,     0,    -1,     8,    24,
      30,    25,    26,    -1,     8,    24,    30,    25,     0,    -1,
       9,    24,    30,    25,    26,    -1,     9,    24,    30,    25,
       0,    -1,    10,    24,    30,    25,    26,    -1,    10,    24,
      30,    25,     0,    -1,    11,    24,    30,    25,    26,    -1,
      11,    24,    30,    25,     0,    -1,    12,    24,    30,    25,
      26,    -1,    12,    24,    30,    25,     0,    -1,    13,    24,
      30,    25,    26,    -1,    13,    24,    30,    25,     0,    -1,
      30,    26,    -1,    30,     0,    -1,     4,    24,    30,    25,
       3,    23,    26,    -1,     4,    24,    30,    25,     3,    23,
       0,    -1,     5,    24,    30,    25,     3,    23,    26,    -1,
       5,    24,    30,    25,     3,    23,     0,    -1,     5,    24,
      18,    25,     3,    23,    26,    -1,     5,    24,    18,    25,
       3,    23,     0,    -1,     7,    24,    30,    25,     3,    23,
      26,    -1,     7,    24,    30,    25,     3,    23,     0,    -1,
       6,    24,    30,    25,     3,    23,    26,    -1,     6,    24,
      30,    25,     3,    23,     0,    -1,     8,    24,    30,    25,
       3,    23,    26,    -1,     8,    24,    30,    25,     3,    23,
       0,    -1,    30,     3,    23,    26,    -1,    30,     3,    23,
       0,    -1,    30,    16,    30,    -1,    30,    17,    30,    -1,
      30,    18,    30,    -1,    30,    19,    30,    -1,    30,    20,
      30,    -1,    30,    21,    30,    -1,    30,    15,    30,    -1,
      30,    14,    30,    -1,    23,    24,    30,    25,    -1,    23,
      24,    30,    26,    30,    25,    -1,    17,    30,    -1,    16,
      30,    -1,    24,    30,    25,    -1,    23,    -1,    22,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  selectParser::yyprhs_[] =
  {
         0,     0,     3,     5,     8,    14,    20,    26,    32,    38,
      44,    50,    56,    62,    68,    74,    80,    86,    92,    98,
     104,   110,   116,   122,   128,   134,   140,   143,   146,   154,
     162,   170,   178,   186,   194,   202,   210,   218,   226,   234,
     242,   247,   252,   256,   260,   264,   268,   272,   276,   280,
     284,   289,   296,   299,   302,   306,   308
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  selectParser::yyrline_[] =
  {
         0,    82,    82,    82,    83,    87,    91,    95,    99,   103,
     107,   111,   115,   119,   123,   127,   131,   135,   139,   143,
     147,   151,   155,   159,   163,   167,   171,   175,   179,   184,
     189,   194,   199,   204,   209,   214,   219,   224,   229,   234,
     239,   244,   252,   264,   276,   288,   300,   312,   324,   336,
     348,   360,   373,   383,   386,   389,   397
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
      24,    25,     2,     2,    26,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int selectParser::yyeof_ = 0;
  const int selectParser::yylast_ = 297;
  const int selectParser::yynnts_ = 4;
  const int selectParser::yyempty_ = -2;
  const int selectParser::yyfinal_ = 33;
  const int selectParser::yyterror_ = 1;
  const int selectParser::yyerrcode_ = 256;
  const int selectParser::yyntokens_ = 27;

  const unsigned int selectParser::yyuser_token_number_max_ = 278;
  const selectParser::token_number_type selectParser::yyundef_token_ = 2;

} // namespace ibis

#line 406 "selectParser.yy"

void ibis::selectParser::error(const ibis::selectParser::location_type& l,
			       const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "ibis::selectParser encountered " << m << " at location " << l;
}


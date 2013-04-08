/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++
   
      Copyright (C) 2002-2013 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
/* "%code top" blocks.  */
/* Line 276 of lalr1.cc  */
#line 6 "selectParser.yy"

/** \file Defines the parser for the select clause accepted by FastBit
    IBIS.  The definitions are processed through bison.
*/
#include <iostream>


/* Line 276 of lalr1.cc  */
#line 44 "selectParser.cc"

// Take the name prefix into account.
#define yylex   ibislex

/* First part of user declarations.  */

/* Line 283 of lalr1.cc  */
#line 52 "selectParser.cc"


#include "selectParser.hh"

/* User implementation prologue.  */
/* Line 289 of lalr1.cc  */
#line 70 "selectParser.yy"

#include "selectLexer.h"

#undef yylex
#define yylex driver.lexer->lex

/* Line 289 of lalr1.cc  */
#line 67 "selectParser.cc"


# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* FIXME: INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (/*CONSTCOND*/ false)
# endif


/* Suppress unused-variable warnings by "using" E.  */
#define YYUSE(e) ((void) (e))

/* Enable debugging if requested.  */
#if YYDEBUG

/* A pseudo ostream that takes yydebug_ into account.  */
# define YYCDEBUG if (yydebug_) (*yycdebug_)

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

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Type, Value, Location) YYUSE(Type)
# define YY_REDUCE_PRINT(Rule)        static_cast<void>(0)
# define YY_STACK_PRINT()             static_cast<void>(0)

#endif /* !YYDEBUG */

#define yyerrok		(yyerrstatus_ = 0)
#define yyclearin	(yychar = yyempty_)

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)


namespace ibis {
/* Line 357 of lalr1.cc  */
#line 162 "selectParser.cc"

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


  /// Build a parser object.
  selectParser::selectParser (class ibis::selectClause& driver_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
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
    std::ostream& yyo = debug_stream ();
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    YYUSE (yytype);
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
#endif

  void
  selectParser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
    {
      case 13: /* "name" */
/* Line 452 of lalr1.cc  */
#line 67 "selectParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 265 "selectParser.cc"
        break;
      case 14: /* "string literal" */
/* Line 452 of lalr1.cc  */
#line 66 "selectParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 272 "selectParser.cc"
        break;
      case 23: /* mathExpr */
/* Line 452 of lalr1.cc  */
#line 68 "selectParser.yy"
        { delete ((*yyvaluep).selectNode); };
/* Line 452 of lalr1.cc  */
#line 279 "selectParser.cc"
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

#if YYDEBUG
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
#endif

  inline bool
  selectParser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  selectParser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  selectParser::parse ()
  {
    /// Lookahead and lookahead in internal form.
    int yychar = yyempty_;
    int yytoken = 0;

    // State.
    int yyn;
    int yylen = 0;
    int yystate = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// Semantic value of the lookahead.
    static semantic_type yyval_default;
    semantic_type yylval = yyval_default;
    /// Location of the lookahead.
    location_type yylloc;
    /// The locations where the error started and ended.
    location_type yyerror_range[3];

    /// $$.
    semantic_type yyval;
    /// @$.
    location_type yyloc;

    int yyresult;

    // FIXME: This shoud be completely indented.  It is not yet to
    // avoid gratuitous conflicts when merging into the master branch.
    try
      {
    YYCDEBUG << "Starting parse" << std::endl;


/* User initialization code.  */
/* Line 539 of lalr1.cc  */
#line 28 "selectParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}
/* Line 539 of lalr1.cc  */
#line 379 "selectParser.cc"

    /* Initialize the stacks.  The initial state will be pushed in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystate_stack_.clear ();
    yysemantic_stack_.clear ();
    yylocation_stack_.clear ();
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* New state.  */
  yynewstate:
    yystate_stack_.push (yystate);
    YYCDEBUG << "Entering state " << yystate << std::endl;

    /* Accept?  */
    if (yystate == yyfinal_)
      goto yyacceptlab;

    goto yybackup;

    /* Backup.  */
  yybackup:

    /* Try to take a decision without lookahead.  */
    yyn = yypact_[yystate];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    /* Read a lookahead token.  */
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
	if (yy_table_value_is_error_ (yyn))
	  goto yyerrlab;
	yyn = -yyn;
	goto yyreduce;
      }

    /* Shift the lookahead token.  */
    YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

    /* Discard the token being shifted.  */
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

    // Compute the default @$.
    {
      slice<location_type, location_stack_type> slice (yylocation_stack_, yylen);
      YYLLOC_DEFAULT (yyloc, slice, yylen);
    }

    // Perform the reduction.
    YY_REDUCE_PRINT (yyn);
    switch (yyn)
      {
          case 4:
/* Line 664 of lalr1.cc  */
#line 79 "selectParser.yy"
    {
    driver.addTerm((yysemantic_stack_[(2) - (1)].selectNode), 0);
}
    break;

  case 5:
/* Line 664 of lalr1.cc  */
#line 82 "selectParser.yy"
    {
    driver.addTerm((yysemantic_stack_[(2) - (1)].selectNode), 0);
}
    break;

  case 6:
/* Line 664 of lalr1.cc  */
#line 85 "selectParser.yy"
    {
    driver.addTerm((yysemantic_stack_[(3) - (1)].selectNode), (yysemantic_stack_[(3) - (2)].stringVal));
    delete (yysemantic_stack_[(3) - (2)].stringVal);
}
    break;

  case 7:
/* Line 664 of lalr1.cc  */
#line 89 "selectParser.yy"
    {
    driver.addTerm((yysemantic_stack_[(3) - (1)].selectNode), (yysemantic_stack_[(3) - (2)].stringVal));
    delete (yysemantic_stack_[(3) - (2)].stringVal);
}
    break;

  case 8:
/* Line 664 of lalr1.cc  */
#line 93 "selectParser.yy"
    {
    driver.addTerm((yysemantic_stack_[(4) - (1)].selectNode), (yysemantic_stack_[(4) - (3)].stringVal));
    delete (yysemantic_stack_[(4) - (3)].stringVal);
}
    break;

  case 9:
/* Line 664 of lalr1.cc  */
#line 97 "selectParser.yy"
    {
    driver.addTerm((yysemantic_stack_[(4) - (1)].selectNode), (yysemantic_stack_[(4) - (3)].stringVal));
    delete (yysemantic_stack_[(4) - (3)].stringVal);
}
    break;

  case 10:
/* Line 664 of lalr1.cc  */
#line 104 "selectParser.yy"
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
}
    break;

  case 11:
/* Line 664 of lalr1.cc  */
#line 116 "selectParser.yy"
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
}
    break;

  case 12:
/* Line 664 of lalr1.cc  */
#line 128 "selectParser.yy"
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
}
    break;

  case 13:
/* Line 664 of lalr1.cc  */
#line 140 "selectParser.yy"
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
}
    break;

  case 14:
/* Line 664 of lalr1.cc  */
#line 152 "selectParser.yy"
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
}
    break;

  case 15:
/* Line 664 of lalr1.cc  */
#line 164 "selectParser.yy"
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
}
    break;

  case 16:
/* Line 664 of lalr1.cc  */
#line 176 "selectParser.yy"
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
}
    break;

  case 17:
/* Line 664 of lalr1.cc  */
#line 188 "selectParser.yy"
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
}
    break;

  case 18:
/* Line 664 of lalr1.cc  */
#line 200 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << "(*)";
#endif
    ibis::math::term *fun = 0;
    if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "count") == 0) { // aggregation count
	ibis::math::variable *var = new ibis::math::variable("*");
	fun = driver.addAgregado(ibis::selectClause::CNT, var);
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- only operator COUNT supports * as the argument, "
	    "but received " << *(yysemantic_stack_[(4) - (1)].stringVal);
	throw "invalid use of (*)";
    }
    delete (yysemantic_stack_[(4) - (1)].stringVal);
    (yyval.selectNode) = fun;
}
    break;

  case 19:
/* Line 664 of lalr1.cc  */
#line 219 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(4) - (3)].selectNode) << ")";
#endif
    ibis::math::term *fun = 0;
    if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "count") == 0) { // aggregation count
	delete (yysemantic_stack_[(4) - (3)].selectNode); // drop the expression, replace it with "*"
	ibis::math::variable *var = new ibis::math::variable("*");
	fun = driver.addAgregado(ibis::selectClause::CNT, var);
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "max") == 0) { // aggregation max
	fun = driver.addAgregado(ibis::selectClause::MAX, (yysemantic_stack_[(4) - (3)].selectNode));
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "min") == 0) { // aggregation min
	fun = driver.addAgregado(ibis::selectClause::MIN, (yysemantic_stack_[(4) - (3)].selectNode));
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "sum") == 0) { // aggregation sum
	fun = driver.addAgregado(ibis::selectClause::SUM, (yysemantic_stack_[(4) - (3)].selectNode));
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "median") == 0) { // aggregation median
	fun = driver.addAgregado(ibis::selectClause::MEDIAN, (yysemantic_stack_[(4) - (3)].selectNode));
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "countd") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "countdistinct") == 0) {
	// count distinct values
	fun = driver.addAgregado(ibis::selectClause::DISTINCT, (yysemantic_stack_[(4) - (3)].selectNode));
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "concat") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "group_concat") == 0) {
	// concatenate all values as ASCII strings
	fun = driver.addAgregado(ibis::selectClause::CONCAT, (yysemantic_stack_[(4) - (3)].selectNode));
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "avg") == 0) { // aggregation avg
	ibis::math::term *numer =
	    driver.addAgregado(ibis::selectClause::SUM, (yysemantic_stack_[(4) - (3)].selectNode));
	ibis::math::variable *var = new ibis::math::variable("*");
	ibis::math::term *denom =
	    driver.addAgregado(ibis::selectClause::CNT, var);
	ibis::math::bediener *opr =
	    new ibis::math::bediener(ibis::math::DIVIDE);
	opr->setRight(denom);
	opr->setLeft(numer);
	fun = opr;
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "varp") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "varpop") == 0) {
	// population variance is computed as
	// fabs(sum (x^2) / count(*) - (sum (x) / count(*))^2)
	ibis::math::term *x = (yysemantic_stack_[(4) - (3)].selectNode);
	ibis::math::number *two = new ibis::math::number(2.0);
	ibis::math::variable *star = new ibis::math::variable("*");
	ibis::math::term *t11 = new ibis::math::bediener(ibis::math::POWER);
	t11->setLeft(x);
	t11->setRight(two);
	t11 = driver.addAgregado(ibis::selectClause::SUM, t11);
	ibis::math::term *t12 =
	    driver.addAgregado(ibis::selectClause::CNT, star);
	ibis::math::term *t13 = new ibis::math::bediener(ibis::math::DIVIDE);
	t13->setLeft(t11);
	t13->setRight(t12);
	ibis::math::term *t21 =
	    driver.addAgregado(ibis::selectClause::SUM, x->dup());
	ibis::math::term *t23 = new ibis::math::bediener(ibis::math::DIVIDE);
	t23->setLeft(t21);
	t23->setRight(t12->dup());
	ibis::math::term *t24 = new ibis::math::bediener(ibis::math::POWER);
	t24->setLeft(t23);
	t24->setRight(two->dup());
        ibis::math::term *t0 = new ibis::math::bediener(ibis::math::MINUS);
	t0->setLeft(t13);
	t0->setRight(t24);
        fun = new ibis::math::stdFunction1("fabs");
        fun->setLeft(t0);
	//fun = driver.addAgregado(ibis::selectClause::VARPOP, $3);
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "var") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "varsamp") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "variance") == 0) {
	// sample variance is computed as
	// fabs((sum (x^2) / count(*) - (sum (x) / count(*))^2) * (count(*) / (count(*)-1)))
	ibis::math::term *x = (yysemantic_stack_[(4) - (3)].selectNode);
	ibis::math::number *two = new ibis::math::number(2.0);
	ibis::math::variable *star = new ibis::math::variable("*");
	ibis::math::term *t11 = new ibis::math::bediener(ibis::math::POWER);
	t11->setLeft(x);
	t11->setRight(two);
	t11 = driver.addAgregado(ibis::selectClause::SUM, t11);
	ibis::math::term *t12 =
	    driver.addAgregado(ibis::selectClause::CNT, star);
	ibis::math::term *t13 = new ibis::math::bediener(ibis::math::DIVIDE);
	t13->setLeft(t11);
	t13->setRight(t12);
	ibis::math::term *t21 =
	    driver.addAgregado(ibis::selectClause::SUM, x->dup());
	ibis::math::term *t23 = new ibis::math::bediener(ibis::math::DIVIDE);
	t23->setLeft(t21);
	t23->setRight(t12->dup());
	ibis::math::term *t24 = new ibis::math::bediener(ibis::math::POWER);
	t24->setLeft(t23);
	t24->setRight(two->dup());
	ibis::math::term *t31 = new ibis::math::bediener(ibis::math::MINUS);
	t31->setLeft(t13);
	t31->setRight(t24);
	ibis::math::term *t32 = new ibis::math::bediener(ibis::math::MINUS);
	ibis::math::number *one = new ibis::math::number(1.0);
	t32->setLeft(t12->dup());
	t32->setRight(one);
	ibis::math::term *t33 = new ibis::math::bediener(ibis::math::DIVIDE);
	t33->setLeft(t12->dup());
	t33->setRight(t32);
        ibis::math::term *t0 = new ibis::math::bediener(ibis::math::MULTIPLY);
	t0->setLeft(t31);
	t0->setRight(t33);
        fun = new ibis::math::stdFunction1("fabs");
        fun->setLeft(t0);
	//fun = driver.addAgregado(ibis::selectClause::VARSAMP, $3);
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "stdevp") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "stdpop") == 0) {
	// population standard deviation is computed as
	// sqrt(fabs(sum (x^2) / count(*) - (sum (x) / count(*))^2))
	ibis::math::term *x = (yysemantic_stack_[(4) - (3)].selectNode);
	ibis::math::number *two = new ibis::math::number(2.0);
	ibis::math::variable *star = new ibis::math::variable("*");
	ibis::math::term *t11 = new ibis::math::bediener(ibis::math::POWER);
	t11->setLeft(x);
	t11->setRight(two);
	t11 = driver.addAgregado(ibis::selectClause::SUM, t11);
	ibis::math::term *t12 =
	    driver.addAgregado(ibis::selectClause::CNT, star);
	ibis::math::term *t13 = new ibis::math::bediener(ibis::math::DIVIDE);
	t13->setLeft(t11);
	t13->setRight(t12);
	ibis::math::term *t21 =
	    driver.addAgregado(ibis::selectClause::SUM, x->dup());
	ibis::math::term *t23 = new ibis::math::bediener(ibis::math::DIVIDE);
	t23->setLeft(t21);
	t23->setRight(t12->dup());
	ibis::math::term *t24 = new ibis::math::bediener(ibis::math::POWER);
	t24->setLeft(t23);
	t24->setRight(two->dup());
	ibis::math::term *t31 = new ibis::math::bediener(ibis::math::MINUS);
	t31->setLeft(t13);
	t31->setRight(t24);
        ibis::math::term *t0 = new ibis::math::stdFunction1("fabs");
        t0->setLeft(t31);
	fun = new ibis::math::stdFunction1("sqrt");
	fun->setLeft(t0);
	//fun = driver.addAgregado(ibis::selectClause::STDPOP, $3);
    }
    else if (stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "std") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "stdev") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "stddev") == 0 ||
	     stricmp((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), "stdsamp") == 0) {
	// sample standard deviation is computed as
	// sqrt(fabs(sum (x^2) / count(*) - (sum (x) / count(*))^2) * (count(*) / (count(*)-1))))
	ibis::math::term *x = (yysemantic_stack_[(4) - (3)].selectNode);
	ibis::math::number *two = new ibis::math::number(2.0);
	ibis::math::variable *star = new ibis::math::variable("*");
	ibis::math::term *t11 = new ibis::math::bediener(ibis::math::POWER);
	t11->setLeft(x);
	t11->setRight(two);
	t11 = driver.addAgregado(ibis::selectClause::SUM, t11);
	ibis::math::term *t12 =
	    driver.addAgregado(ibis::selectClause::CNT, star);
	ibis::math::term *t13 = new ibis::math::bediener(ibis::math::DIVIDE);
	t13->setLeft(t11);
	t13->setRight(t12);
	ibis::math::term *t21 =
	    driver.addAgregado(ibis::selectClause::SUM, x->dup());
	ibis::math::term *t23 = new ibis::math::bediener(ibis::math::DIVIDE);
	t23->setLeft(t21);
	t23->setRight(t12->dup());
	ibis::math::term *t24 = new ibis::math::bediener(ibis::math::POWER);
	t24->setLeft(t23);
	t24->setRight(two->dup());
	ibis::math::term *t31 = new ibis::math::bediener(ibis::math::MINUS);
	t31->setLeft(t13);
	t31->setRight(t24);
	ibis::math::term *t32 = new ibis::math::bediener(ibis::math::MINUS);
	ibis::math::number *one = new ibis::math::number(1.0);
	t32->setLeft(t12->dup());
	t32->setRight(one);
	ibis::math::term *t33 = new ibis::math::bediener(ibis::math::DIVIDE);
	t33->setLeft(t12->dup());
	t33->setRight(t32);
	ibis::math::term *t34 = new ibis::math::bediener(ibis::math::MULTIPLY);
	t34->setLeft(t31);
	t34->setRight(t33);
        ibis::math::term *t0 = new ibis::math::stdFunction1("fabs");
        t0->setLeft(t34);
	fun = new ibis::math::stdFunction1("sqrt");
	fun->setLeft(t0);
	// fun = driver.addAgregado(ibis::selectClause::STDSAMP, $3);
    }
    else { // assume it is a standard math function
	fun = new ibis::math::stdFunction1((yysemantic_stack_[(4) - (1)].stringVal)->c_str());
	fun->setLeft((yysemantic_stack_[(4) - (3)].selectNode));
    }
    delete (yysemantic_stack_[(4) - (1)].stringVal);
    (yyval.selectNode) = fun;
}
    break;

  case 20:
/* Line 670 of lalr1.cc  */
#line 419 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- FORMAT_UNIXTIME_GMT("
	<< *(yysemantic_stack_[(6) - (3)].selectNode) << ", " << *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif
    ibis::math::formatUnixTime fut((yysemantic_stack_[(6) - (5)].stringVal)->c_str(), "GMT");
    ibis::math::stringFunction1 *fun = new ibis::math::stringFunction1(fut);
    fun->setLeft((yysemantic_stack_[(6) - (3)].selectNode));
    (yyval.selectNode) = fun;
    delete (yysemantic_stack_[(6) - (5)].stringVal);
}
    break;

  case 21:
/* Line 664 of lalr1.cc  */
#line 435 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- FORMAT_UNIXTIME_GMT("
	<< *(yysemantic_stack_[(6) - (3)].selectNode) << ", " << *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif
    ibis::math::formatUnixTime fut((yysemantic_stack_[(6) - (5)].stringVal)->c_str(), "GMT");
    ibis::math::stringFunction1 *fun = new ibis::math::stringFunction1(fut);
    fun->setLeft((yysemantic_stack_[(6) - (3)].selectNode));
    (yyval.selectNode) = fun;
    delete (yysemantic_stack_[(6) - (5)].stringVal);
}
    break;

  case 22:
/* Line 664 of lalr1.cc  */
#line 447 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- FORMAT_UNIXTIME_LOCAL("
	<< *(yysemantic_stack_[(6) - (3)].selectNode) << ", " << *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif
    ibis::math::formatUnixTime fut((yysemantic_stack_[(6) - (5)].stringVal)->c_str());
    ibis::math::stringFunction1 *fun = new ibis::math::stringFunction1(fut);
    fun->setLeft((yysemantic_stack_[(6) - (3)].selectNode));
    (yyval.selectNode) = fun;
    delete (yysemantic_stack_[(6) - (5)].stringVal);
}
    break;

  case 23:
/* Line 664 of lalr1.cc  */
#line 459 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- FORMAT_UNIXTIME_LOCAL("
	<< *(yysemantic_stack_[(6) - (3)].selectNode) << ", " << *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif
    ibis::math::formatUnixTime fut((yysemantic_stack_[(6) - (5)].stringVal)->c_str());
    ibis::math::stringFunction1 *fun = new ibis::math::stringFunction1(fut);
    fun->setLeft((yysemantic_stack_[(6) - (3)].selectNode));
    (yyval.selectNode) = fun;
    delete (yysemantic_stack_[(6) - (5)].stringVal);
}
    break;

  case 24:
/* Line 664 of lalr1.cc  */
#line 471 "selectParser.yy"
    {
    /* two-arugment math functions */
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
}
    break;

  case 21:
/* Line 670 of lalr1.cc  */
#line 432 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *(yysemantic_stack_[(2) - (2)].selectNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight((yysemantic_stack_[(2) - (2)].selectNode));
    (yyval.selectNode) = opr;
}
    break;

  case 22:
/* Line 670 of lalr1.cc  */
#line 442 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(2) - (2)].selectNode);
}
    break;

  case 23:
/* Line 670 of lalr1.cc  */
#line 445 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(3) - (2)].selectNode);
}
    break;

  case 24:
/* Line 670 of lalr1.cc  */
#line 448 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.selectNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
}
    break;

  case 25:
/* Line 670 of lalr1.cc  */
#line 456 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.selectNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
}
    break;


/* Line 670 of lalr1.cc  */
#line 991 "selectParser.cc"
      default:
        break;
      }

    /* User semantic actions sometimes alter yychar, and that requires
       that yytoken be updated with the new translation.  We take the
       approach of translating immediately before every use of yytoken.
       One alternative is translating here after every semantic action,
       but that translation would be missed if the semantic action
       invokes YYABORT, YYACCEPT, or YYERROR immediately after altering
       yychar.  In the case of YYABORT or YYACCEPT, an incorrect
       destructor might then be invoked immediately.  In the case of
       YYERROR, subsequent parser actions might lead to an incorrect
       destructor call or verbose syntax error message before the
       lookahead is translated.  */
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
    /* Make sure we have latest lookahead translation.  See comments at
       user semantic actions for why this is necessary.  */
    yytoken = yytranslate_ (yychar);

    /* If not already recovering from an error, report this error.  */
    if (!yyerrstatus_)
      {
	++yynerrs_;
	if (yychar == yyempty_)
	  yytoken = yyempty_;
	error (yylloc, yysyntax_error_ (yystate, yytoken));
      }

    yyerror_range[1] = yylloc;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
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

    /* Else will try to reuse lookahead token after shifting the error
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

    yyerror_range[1] = yylocation_stack_[yylen - 1];
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
	if (!yy_pact_value_is_default_ (yyn))
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

	yyerror_range[1] = yylocation_stack_[0];
	yydestruct_ ("Error: popping",
		     yystos_[yystate],
		     &yysemantic_stack_[0], &yylocation_stack_[0]);
	yypop_ ();
	yystate = yystate_stack_[0];
	YY_STACK_PRINT ();
      }

    yyerror_range[2] = yylloc;
    // Using YYLLOC is tempting, but would change the location of
    // the lookahead.  YYLOC is available though.
    YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
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
    if (yychar != yyempty_)
      {
        /* Make sure we have latest lookahead translation.  See comments
           at user semantic actions for why this is necessary.  */
        yytoken = yytranslate_ (yychar);
        yydestruct_ ("Cleanup: discarding lookahead", yytoken, &yylval,
                     &yylloc);
      }

    /* Do not reclaim the symbols of the rule which action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystate_stack_.height ())
      {
        yydestruct_ ("Cleanup: popping",
                     yystos_[yystate_stack_[0]],
                     &yysemantic_stack_[0],
                     &yylocation_stack_[0]);
        yypop_ ();
      }

    return yyresult;
    }
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack"
                 << std::endl;
        // Do not try to display the values of the reclaimed symbols,
        // as their printer might throw an exception.
        if (yychar != yyempty_)
          {
            /* Make sure we have latest lookahead translation.  See
               comments at user semantic actions for why this is
               necessary.  */
            yytoken = yytranslate_ (yychar);
            yydestruct_ (YY_NULL, yytoken, &yylval, &yylloc);
          }

        while (1 < yystate_stack_.height ())
          {
            yydestruct_ (YY_NULL,
                         yystos_[yystate_stack_[0]],
                         &yysemantic_stack_[0],
                         &yylocation_stack_[0]);
            yypop_ ();
          }
        throw;
      }
  }

  // Generate an error message.
  std::string
  selectParser::yysyntax_error_ (int yystate, int yytoken)
  {
    std::string yyres;
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    size_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yytoken) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yychar.
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state
         merging (from LALR or IELR) and default reductions corrupt the
         expected token list.  However, the list is correct for
         canonical LR with one exception: it will still contain any
         token that will not be accepted due to an error action in a
         later state.
    */
    if (yytoken != yyempty_)
      {
        yyarg[yycount++] = yytname_[yytoken];
        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            /* Stay within bounds of both yycheck and yytname.  */
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yyterror_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULL;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
        YYCASE_(0, YY_("syntax error"));
        YYCASE_(1, YY_("syntax error, unexpected %s"));
        YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    // Argument number.
    size_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
  const signed char selectParser::yypact_ninf_ = -13;
  const signed char
  selectParser::yypact_[] =
  {
       126,   126,   126,   -13,   -12,   -13,    -6,    -2,   126,    30,
     126,    29,    20,    20,   113,   126,   126,    61,   -13,   -13,
     -13,    28,   126,   126,   126,   126,   126,   126,   126,   126,
       2,   -13,    24,    45,    93,   107,   -13,     3,    68,    83,
       0,     0,    20,    20,    20,    20,   -13,   -13,   -13,   126,
     -13,    -9,     4,   -13,   -13,    77,    25,    26,    38,    39,
     -13,   -13,   -13,   -13,   -13
  };

  /* YYDEFACT[S] -- default reduction number in state S.  Performed when
     YYTABLE doesn't specify something else to do.  Zero means the
     default is an error.  */
  const unsigned char
  selectParser::yydefact_[] =
  {
         0,     0,     0,    30,    28,    29,     0,     0,     0,     0,
       2,     0,    26,    25,     0,     0,     0,     0,     1,     3,
       5,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     4,     0,     0,     0,     0,    27,     0,    17,    16,
      10,    11,    12,    13,    14,    15,     7,     6,    18,     0,
      19,     0,     0,     9,     8,     0,     0,     0,     0,     0,
      24,    20,    21,    22,    23
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yypgoto_[] =
  {
       -13,    37,   -13,    -1
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yydefgoto_[] =
  {
        -1,     9,    10,    11
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If YYTABLE_NINF_, syntax error.  */
  const signed char selectParser::yytable_ninf_ = -1;
  const unsigned char
  selectParser::yytable_[] =
  {
        12,    13,    46,    53,    56,    57,    14,    17,    26,    27,
      28,    29,    15,    33,    34,    35,    16,    58,    59,    47,
      54,    38,    39,    40,    41,    42,    43,    44,    45,    20,
      18,    29,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    37,    30,    48,    61,    62,    31,    19,    55,    22,
      23,    24,    25,    26,    27,    28,    29,    63,    64,     0,
       0,     0,    49,     0,    50,    22,    23,    24,    25,    26,
      27,    28,    29,    23,    24,    25,    26,    27,    28,    29,
      36,    22,    23,    24,    25,    26,    27,    28,    29,    24,
      25,    26,    27,    28,    29,     0,    60,    22,    23,    24,
      25,    26,    27,    28,    29,     0,     0,     0,     0,     0,
      51,    22,    23,    24,    25,    26,    27,    28,    29,     1,
       2,    32,     0,     0,    52,     3,     4,     5,     6,     7,
       0,     8,     1,     2,     0,     0,     0,     0,     3,     4,
       5,     6,     7,     0,     8
  };

  /* YYCHECK.  */
  const signed char
  selectParser::yycheck_[] =
  {
         1,     2,     0,     0,    13,    14,    18,     8,     8,     9,
      10,    11,    18,    14,    15,    16,    18,    13,    14,    17,
      17,    22,    23,    24,    25,    26,    27,    28,    29,     0,
       0,    11,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    13,    13,    19,    19,    19,    17,    10,    49,     4,
       5,     6,     7,     8,     9,    10,    11,    19,    19,    -1,
      -1,    -1,    17,    -1,    19,     4,     5,     6,     7,     8,
       9,    10,    11,     5,     6,     7,     8,     9,    10,    11,
      19,     4,     5,     6,     7,     8,     9,    10,    11,     6,
       7,     8,     9,    10,    11,    -1,    19,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,
      17,     4,     5,     6,     7,     8,     9,    10,    11,     6,
       7,     8,    -1,    -1,    17,    12,    13,    14,    15,    16,
      -1,    18,     6,     7,    -1,    -1,    -1,    -1,    12,    13,
      14,    15,    16,    -1,    18
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  selectParser::yystos_[] =
  {
         0,     6,     7,    12,    13,    14,    15,    16,    18,    21,
      22,    23,    23,    23,    18,    18,    18,    23,     0,    21,
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      13,    17,     8,    23,    23,    23,    19,    13,    23,    23,
      23,    23,    23,    23,    23,    23,     0,    17,    19,    17,
      19,    17,    17,     0,    17,    23,    13,    14,    13,    14,
      19,    19,    19,    19,    19
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  selectParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,    44,    40,    41
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  selectParser::yyr1_[] =
  {
         0,    20,    21,    21,    22,    22,    22,    22,    22,    22,
      23,    23,    23,    23,    23,    23,    23,    23,    23,    23,
      23,    23,    23,    23,    23,    23,    23,    23,    23,    23,
      23
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  selectParser::yyr2_[] =
  {
         0,     2,     1,     2,     2,     2,     3,     3,     4,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     4,
       6,     6,     6,     6,     6,     2,     2,     3,     1,     1,
       1
  };


  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const selectParser::yytname_[] =
  {
    "\"end of input\"", "error", "$undefined", "\"as\"", "\"|\"", "\"&\"",
  "\"+\"", "\"-\"", "\"*\"", "\"/\"", "\"%\"", "\"**\"",
  "\"numerical value\"", "\"name\"", "\"string literal\"",
  "\"FORMAT_UNIXTIME_GMT\"", "\"FORMAT_UNIXTIME_LOCAL\"", "','", "'('",
  "')'", "$accept", "slist", "sterm", "mathExpr", YY_NULL
  };

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const selectParser::rhs_number_type
  selectParser::yyrhs_[] =
  {
        21,     0,    -1,    22,    -1,    22,    21,    -1,    23,    17,
      -1,    23,     0,    -1,    23,    13,    17,    -1,    23,    13,
       0,    -1,    23,     3,    13,    17,    -1,    23,     3,    13,
       0,    -1,    23,     6,    23,    -1,    23,     7,    23,    -1,
      23,     8,    23,    -1,    23,     9,    23,    -1,    23,    10,
      23,    -1,    23,    11,    23,    -1,    23,     5,    23,    -1,
      23,     4,    23,    -1,    13,    18,     8,    19,    -1,    13,
      18,    23,    19,    -1,    15,    18,    23,    17,    13,    19,
      -1,    15,    18,    23,    17,    14,    19,    -1,    16,    18,
      23,    17,    13,    19,    -1,    16,    18,    23,    17,    14,
      19,    -1,    13,    18,    23,    17,    23,    19,    -1,     7,
      23,    -1,     6,    23,    -1,    18,    23,    19,    -1,    13,
      -1,    14,    -1,    12,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned char
  selectParser::yyprhs_[] =
  {
         0,     0,     3,     5,     8,    11,    14,    18,    22,    27,
      32,    36,    40,    44,    48,    52,    56,    60,    64,    69,
      74,    81,    88,    95,   102,   109,   112,   115,   119,   121,
     123
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  selectParser::yyrline_[] =
  {
         0,    74,    74,    74,    75,    78,    81,    85,    89,    93,
     100,   112,   124,   136,   148,   160,   172,   184,   196,   215,
     419,   432,   442,   445,   448,   456
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
	       << " (line " << yylno << "):" << std::endl;
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
      18,    19,     2,     2,    17,     2,     2,     2,     2,     2,
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
      15,    16
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int selectParser::yyeof_ = 0;
  const int selectParser::yylast_ = 144;
  const int selectParser::yynnts_ = 4;
  const int selectParser::yyempty_ = -2;
  const int selectParser::yyfinal_ = 18;
  const int selectParser::yyterror_ = 1;
  const int selectParser::yyerrcode_ = 256;
  const int selectParser::yyntokens_ = 20;

  const unsigned int selectParser::yyuser_token_number_max_ = 271;
  const selectParser::token_number_type selectParser::yyundef_token_ = 2;


} // ibis
/* Line 1141 of lalr1.cc  */
#line 1541 "selectParser.cc"
/* Line 1142 of lalr1.cc  */
#line 465 "selectParser.yy"

void ibis::selectParser::error(const ibis::selectParser::location_type& l,
			       const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::selectParser encountered " << m << " at location "
	<< l;
}

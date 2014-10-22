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
#line 6 "whereParser.yy"

/** \file Defines the parser for the where clause accepted by FastBit IBIS.
    The definitions are processed through bison.
*/

#include <iostream>


/* Line 276 of lalr1.cc  */
#line 45 "whereParser.cc"

// Take the name prefix into account.
#define yylex   ibislex

/* First part of user declarations.  */

/* Line 283 of lalr1.cc  */
#line 53 "whereParser.cc"


#include "whereParser.hh"

/* User implementation prologue.  */
/* Line 289 of lalr1.cc  */
#line 103 "whereParser.yy"

#include "whereLexer.h"

#undef yylex
#define yylex driver.lexer->lex

/* Line 289 of lalr1.cc  */
#line 68 "whereParser.cc"


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
#line 163 "whereParser.cc"

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


  /// Build a parser object.
  whereParser::whereParser (class ibis::whereClause& driver_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
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
    std::ostream& yyo = debug_stream ();
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    YYUSE (yytype);
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
#endif

  void
  whereParser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
    {
      case 36: /* "signed integer sequence" */
/* Line 452 of lalr1.cc  */
#line 100 "whereParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 266 "whereParser.cc"
        break;
      case 37: /* "unsigned integer sequence" */
/* Line 452 of lalr1.cc  */
#line 100 "whereParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 273 "whereParser.cc"
        break;
      case 38: /* "name string" */
/* Line 452 of lalr1.cc  */
#line 100 "whereParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 280 "whereParser.cc"
        break;
      case 39: /* "number sequence" */
/* Line 452 of lalr1.cc  */
#line 100 "whereParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 287 "whereParser.cc"
        break;
      case 40: /* "string sequence" */
/* Line 452 of lalr1.cc  */
#line 100 "whereParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 294 "whereParser.cc"
        break;
      case 41: /* "string literal" */
/* Line 452 of lalr1.cc  */
#line 100 "whereParser.yy"
        { delete ((*yyvaluep).stringVal); };
/* Line 452 of lalr1.cc  */
#line 301 "whereParser.cc"
        break;
      case 48: /* qexpr */
/* Line 452 of lalr1.cc  */
#line 101 "whereParser.yy"
        { delete ((*yyvaluep).whereNode); };
/* Line 452 of lalr1.cc  */
#line 308 "whereParser.cc"
        break;
      case 49: /* simpleRange */
/* Line 452 of lalr1.cc  */
#line 101 "whereParser.yy"
        { delete ((*yyvaluep).whereNode); };
/* Line 452 of lalr1.cc  */
#line 315 "whereParser.cc"
        break;
      case 50: /* compRange2 */
/* Line 452 of lalr1.cc  */
#line 101 "whereParser.yy"
        { delete ((*yyvaluep).whereNode); };
/* Line 452 of lalr1.cc  */
#line 322 "whereParser.cc"
        break;
      case 51: /* compRange3 */
/* Line 452 of lalr1.cc  */
#line 101 "whereParser.yy"
        { delete ((*yyvaluep).whereNode); };
/* Line 452 of lalr1.cc  */
#line 329 "whereParser.cc"
        break;
      case 52: /* mathExpr */
/* Line 452 of lalr1.cc  */
#line 101 "whereParser.yy"
        { delete ((*yyvaluep).whereNode); };
/* Line 452 of lalr1.cc  */
#line 336 "whereParser.cc"
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

#if YYDEBUG
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
#endif

  inline bool
  whereParser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  whereParser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  whereParser::parse ()
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
#line 29 "whereParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}
/* Line 539 of lalr1.cc  */
#line 436 "whereParser.cc"

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
          case 2:
/* Line 664 of lalr1.cc  */
#line 112 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " || " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_OR);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
}
    break;

  case 3:
/* Line 664 of lalr1.cc  */
#line 122 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " ^ " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_XOR);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
}
    break;

  case 4:
/* Line 664 of lalr1.cc  */
#line 132 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " && " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
}
    break;

  case 5:
/* Line 664 of lalr1.cc  */
#line 142 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].whereNode)
	<< " &~ " << *(yysemantic_stack_[(3) - (3)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_MINUS);
    (yyval.whereNode)->setRight((yysemantic_stack_[(3) - (3)].whereNode));
    (yyval.whereNode)->setLeft((yysemantic_stack_[(3) - (1)].whereNode));
}
    break;

  case 6:
/* Line 664 of lalr1.cc  */
#line 152 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ! " << *(yysemantic_stack_[(2) - (2)].whereNode);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft((yysemantic_stack_[(2) - (2)].whereNode));
}
    break;

  case 7:
/* Line 664 of lalr1.cc  */
#line 160 "whereParser.yy"
    {
    (yyval.whereNode) = (yysemantic_stack_[(3) - (2)].whereNode);
}
    break;

  case 11:
/* Line 664 of lalr1.cc  */
#line 169 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- EXISTS(" << *(yysemantic_stack_[(2) - (2)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qExists((yysemantic_stack_[(2) - (2)].stringVal)->c_str());
    delete (yysemantic_stack_[(2) - (2)].stringVal);
}
    break;

  case 12:
/* Line 664 of lalr1.cc  */
#line 177 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- EXISTS(" << *(yysemantic_stack_[(2) - (2)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qExists((yysemantic_stack_[(2) - (2)].stringVal)->c_str());
    delete (yysemantic_stack_[(2) - (2)].stringVal);
}
    break;

  case 13:
/* Line 664 of lalr1.cc  */
#line 185 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- EXISTS(" << *(yysemantic_stack_[(4) - (3)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qExists((yysemantic_stack_[(4) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(4) - (3)].stringVal);
}
    break;

  case 14:
/* Line 664 of lalr1.cc  */
#line 193 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- EXISTS(" << *(yysemantic_stack_[(4) - (3)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qExists((yysemantic_stack_[(4) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(4) - (3)].stringVal);
}
    break;

  case 15:
/* Line 664 of lalr1.cc  */
#line 201 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qDiscreteRange((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 16:
/* Line 664 of lalr1.cc  */
#line 211 "whereParser.yy"
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
}
    break;

  case 17:
/* Line 664 of lalr1.cc  */
#line 223 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(5) - (1)].stringVal) << " IN ("
	<< (yysemantic_stack_[(5) - (4)].doubleVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qContinuousRange((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), ibis::qExpr::OP_EQ, (yysemantic_stack_[(5) - (4)].doubleVal));
    delete (yysemantic_stack_[(5) - (1)].stringVal);
}
    break;

  case 18:
/* Line 664 of lalr1.cc  */
#line 232 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " NOT NULL";
#endif
    (yyval.whereNode) = new ibis::qContinuousRange((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), ibis::qExpr::OP_UNDEFINED, 0U);
}
    break;

  case 19:
/* Line 664 of lalr1.cc  */
#line 239 "whereParser.yy"
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
}
    break;

  case 20:
/* Line 664 of lalr1.cc  */
#line 250 "whereParser.yy"
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
}
    break;

  case 21:
/* Line 664 of lalr1.cc  */
#line 263 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << " NOT IN ("
	<< (yysemantic_stack_[(6) - (5)].doubleVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qContinuousRange((yysemantic_stack_[(6) - (1)].stringVal)->c_str(), ibis::qExpr::OP_EQ, (yysemantic_stack_[(6) - (5)].doubleVal)));
    delete (yysemantic_stack_[(6) - (1)].stringVal);
}
    break;

  case 22:
/* Line 664 of lalr1.cc  */
#line 273 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " IN ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 23:
/* Line 664 of lalr1.cc  */
#line 283 "whereParser.yy"
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
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 24:
/* Line 664 of lalr1.cc  */
#line 300 "whereParser.yy"
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
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 25:
/* Line 664 of lalr1.cc  */
#line 317 "whereParser.yy"
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
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 26:
/* Line 664 of lalr1.cc  */
#line 334 "whereParser.yy"
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
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 27:
/* Line 664 of lalr1.cc  */
#line 351 "whereParser.yy"
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
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(5) - (4)].stringVal);
    delete (yysemantic_stack_[(5) - (1)].stringVal);
}
    break;

  case 28:
/* Line 664 of lalr1.cc  */
#line 365 "whereParser.yy"
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
    (yyval.whereNode) = new ibis::qAnyString((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), val.c_str());
    delete (yysemantic_stack_[(5) - (4)].stringVal);
    delete (yysemantic_stack_[(5) - (1)].stringVal);
}
    break;

  case 29:
/* Line 664 of lalr1.cc  */
#line 379 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " LIKE "
	<< *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qLike((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 30:
/* Line 664 of lalr1.cc  */
#line 389 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " LIKE "
	<< *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qLike((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 31:
/* Line 664 of lalr1.cc  */
#line 399 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << " NOT IN ("
	<< *(yysemantic_stack_[(4) - (4)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), (yysemantic_stack_[(4) - (4)].stringVal)->c_str()));
    delete (yysemantic_stack_[(4) - (4)].stringVal);
    delete (yysemantic_stack_[(4) - (1)].stringVal);
}
    break;

  case 32:
/* Line 664 of lalr1.cc  */
#line 410 "whereParser.yy"
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
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
}
    break;

  case 33:
/* Line 664 of lalr1.cc  */
#line 428 "whereParser.yy"
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
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
}
    break;

  case 34:
/* Line 664 of lalr1.cc  */
#line 446 "whereParser.yy"
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
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
}
    break;

  case 35:
/* Line 664 of lalr1.cc  */
#line 464 "whereParser.yy"
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
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(8) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(8) - (7)].stringVal);
    delete (yysemantic_stack_[(8) - (5)].stringVal);
    delete (yysemantic_stack_[(8) - (1)].stringVal);
}
    break;

  case 36:
/* Line 664 of lalr1.cc  */
#line 482 "whereParser.yy"
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
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(6) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(6) - (5)].stringVal);
    delete (yysemantic_stack_[(6) - (1)].stringVal);
}
    break;

  case 37:
/* Line 664 of lalr1.cc  */
#line 497 "whereParser.yy"
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
    (yyval.whereNode)->setLeft(new ibis::qAnyString((yysemantic_stack_[(6) - (1)].stringVal)->c_str(), val.c_str()));
    delete (yysemantic_stack_[(6) - (5)].stringVal);
    delete (yysemantic_stack_[(6) - (1)].stringVal);
}
    break;

  case 38:
/* Line 664 of lalr1.cc  */
#line 512 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " in ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qIntHod((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 39:
/* Line 664 of lalr1.cc  */
#line 522 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << " not in ("
	<< *(yysemantic_stack_[(4) - (4)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qIntHod((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), (yysemantic_stack_[(4) - (4)].stringVal)->c_str()));
    delete (yysemantic_stack_[(4) - (4)].stringVal);
    delete (yysemantic_stack_[(4) - (1)].stringVal);
}
    break;

  case 40:
/* Line 664 of lalr1.cc  */
#line 533 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " in ("
	<< *(yysemantic_stack_[(3) - (3)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qUIntHod((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 41:
/* Line 664 of lalr1.cc  */
#line 543 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << " not in ("
	<< *(yysemantic_stack_[(4) - (4)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qUIntHod((yysemantic_stack_[(4) - (1)].stringVal)->c_str(), (yysemantic_stack_[(4) - (4)].stringVal)->c_str()));
    delete (yysemantic_stack_[(4) - (4)].stringVal);
    delete (yysemantic_stack_[(4) - (1)].stringVal);
}
    break;

  case 42:
/* Line 664 of lalr1.cc  */
#line 554 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal)
	<< " CONTAINS " << *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qKeyword((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (1)].stringVal);
    delete (yysemantic_stack_[(3) - (3)].stringVal);
}
    break;

  case 43:
/* Line 664 of lalr1.cc  */
#line 564 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(3) - (1)].stringVal)
	<< " CONTAINS " << *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qKeyword((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 44:
/* Line 664 of lalr1.cc  */
#line 574 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(5) - (1)].stringVal)
	<< " CONTAINS " << *(yysemantic_stack_[(5) - (4)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qKeyword((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), (yysemantic_stack_[(5) - (4)].stringVal)->c_str());
    delete (yysemantic_stack_[(5) - (1)].stringVal);
    delete (yysemantic_stack_[(5) - (4)].stringVal);
}
    break;

  case 45:
/* Line 664 of lalr1.cc  */
#line 584 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(5) - (1)].stringVal)
	<< " CONTAINS " << *(yysemantic_stack_[(5) - (4)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qKeyword((yysemantic_stack_[(5) - (1)].stringVal)->c_str(), (yysemantic_stack_[(5) - (4)].stringVal)->c_str());
    delete (yysemantic_stack_[(5) - (4)].stringVal);
    delete (yysemantic_stack_[(5) - (1)].stringVal);
}
    break;

  case 46:
/* Line 664 of lalr1.cc  */
#line 594 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(7) - (1)].stringVal)
	<< " CONTAINS (" << *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qAllWords((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), (yysemantic_stack_[(7) - (4)].stringVal)->c_str(), (yysemantic_stack_[(7) - (6)].stringVal)->c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 47:
/* Line 664 of lalr1.cc  */
#line 605 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(7) - (1)].stringVal)
	<< " CONTAINS (" << *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qAllWords((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), (yysemantic_stack_[(7) - (4)].stringVal)->c_str(), (yysemantic_stack_[(7) - (6)].stringVal)->c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 48:
/* Line 664 of lalr1.cc  */
#line 616 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(7) - (1)].stringVal)
	<< " CONTAINS (" << *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qAllWords((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), (yysemantic_stack_[(7) - (4)].stringVal)->c_str(), (yysemantic_stack_[(7) - (6)].stringVal)->c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 49:
/* Line 664 of lalr1.cc  */
#line 627 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(7) - (1)].stringVal)
	<< " CONTAINS (" << *(yysemantic_stack_[(7) - (4)].stringVal) << ", " << *(yysemantic_stack_[(7) - (6)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qAllWords((yysemantic_stack_[(7) - (1)].stringVal)->c_str(), (yysemantic_stack_[(7) - (4)].stringVal)->c_str(), (yysemantic_stack_[(7) - (6)].stringVal)->c_str());
    delete (yysemantic_stack_[(7) - (6)].stringVal);
    delete (yysemantic_stack_[(7) - (4)].stringVal);
    delete (yysemantic_stack_[(7) - (1)].stringVal);
}
    break;

  case 50:
/* Line 664 of lalr1.cc  */
#line 638 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << (yysemantic_stack_[(3) - (1)].stringVal)
	<< " CONTAINS (" << *(yysemantic_stack_[(3) - (3)].stringVal) << ')';
#endif
    (yyval.whereNode) = new ibis::qAllWords((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 51:
/* Line 664 of lalr1.cc  */
#line 648 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ANY(" << *(yysemantic_stack_[(6) - (3)].stringVal) << ") = "
	<< (yysemantic_stack_[(6) - (6)].doubleVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qAnyAny((yysemantic_stack_[(6) - (3)].stringVal)->c_str(), (yysemantic_stack_[(6) - (6)].doubleVal));
    delete (yysemantic_stack_[(6) - (3)].stringVal);
}
    break;

  case 52:
/* Line 664 of lalr1.cc  */
#line 657 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- ANY(" << *(yysemantic_stack_[(6) - (3)].stringVal) << ") = "
	<< *(yysemantic_stack_[(6) - (6)].stringVal) << ")";
#endif
    (yyval.whereNode) = new ibis::qAnyAny((yysemantic_stack_[(6) - (3)].stringVal)->c_str(), (yysemantic_stack_[(6) - (6)].stringVal)->c_str());
    delete (yysemantic_stack_[(6) - (6)].stringVal);
    delete (yysemantic_stack_[(6) - (3)].stringVal);
}
    break;

  case 53:
/* Line 664 of lalr1.cc  */
#line 667 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " = " << *(yysemantic_stack_[(3) - (3)].int64Val);
#endif
    (yyval.whereNode) = new ibis::qIntHod((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].int64Val));
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 54:
/* Line 664 of lalr1.cc  */
#line 675 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " != " << *(yysemantic_stack_[(3) - (3)].int64Val);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qIntHod((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].int64Val)));
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 55:
/* Line 664 of lalr1.cc  */
#line 684 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " = " << *(yysemantic_stack_[(3) - (3)].uint64Val);
#endif
    (yyval.whereNode) = new ibis::qUIntHod((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].uint64Val));
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 56:
/* Line 664 of lalr1.cc  */
#line 692 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " != " << *(yysemantic_stack_[(3) - (3)].uint64Val);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qUIntHod((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].uint64Val)));
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 57:
/* Line 664 of lalr1.cc  */
#line 701 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (3)].stringVal) << " = "
	<< *(yysemantic_stack_[(3) - (1)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qString((yysemantic_stack_[(3) - (3)].stringVal)->c_str(), (yysemantic_stack_[(3) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 58:
/* Line 664 of lalr1.cc  */
#line 711 "whereParser.yy"
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
}
    break;

  case 59:
/* Line 664 of lalr1.cc  */
#line 722 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " = "
	<< *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qString((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str());
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 60:
/* Line 664 of lalr1.cc  */
#line 732 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " != "
	<< *(yysemantic_stack_[(3) - (3)].stringVal);
#endif
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(new ibis::qString((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), (yysemantic_stack_[(3) - (3)].stringVal)->c_str()));
    delete (yysemantic_stack_[(3) - (3)].stringVal);
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 61:
/* Line 664 of lalr1.cc  */
#line 743 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " = "
	<< *me2;
#endif
    if (me2->termType() == ibis::math::NUMBER) {
	(yyval.whereNode) = new ibis::qContinuousRange((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), ibis::qExpr::OP_EQ, me2->eval());
	delete (yysemantic_stack_[(3) - (3)].whereNode);
    }
    else {
	ibis::math::variable *me1 = new ibis::math::variable((yysemantic_stack_[(3) - (1)].stringVal)->c_str());
	(yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2);
    }
    delete (yysemantic_stack_[(3) - (1)].stringVal);
}
    break;

  case 62:
/* Line 664 of lalr1.cc  */
#line 760 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].stringVal) << " = "
	<< *me2;
#endif
    ibis::qExpr*tmp = 0;
    if (me2->termType() == ibis::math::NUMBER) {
	tmp = new ibis::qContinuousRange((yysemantic_stack_[(3) - (1)].stringVal)->c_str(), ibis::qExpr::OP_EQ, me2->eval());
	delete (yysemantic_stack_[(3) - (3)].whereNode);
    }
    else {
	ibis::math::variable *me1 = new ibis::math::variable((yysemantic_stack_[(3) - (1)].stringVal)->c_str());
	tmp = new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2);
    }
    delete (yysemantic_stack_[(3) - (1)].stringVal);
    (yyval.whereNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.whereNode)->setLeft(tmp);
}
    break;

  case 63:
/* Line 664 of lalr1.cc  */
#line 783 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " = "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2);
}
    break;

  case 64:
/* Line 664 of lalr1.cc  */
#line 793 "whereParser.yy"
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
}
    break;

  case 65:
/* Line 664 of lalr1.cc  */
#line 804 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2);
}
    break;

  case 66:
/* Line 664 of lalr1.cc  */
#line 814 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2);
}
    break;

  case 67:
/* Line 664 of lalr1.cc  */
#line 824 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_GT, me2);
}
    break;

  case 68:
/* Line 664 of lalr1.cc  */
#line 834 "whereParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].whereNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].whereNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2;
#endif
    (yyval.whereNode) = new ibis::compRange(me1, ibis::qExpr::OP_GE, me2);
}
    break;

  case 69:
/* Line 664 of lalr1.cc  */
#line 896 "whereParser.yy"
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
}
    break;

  case 70:
/* Line 664 of lalr1.cc  */
#line 908 "whereParser.yy"
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
}
    break;

  case 71:
/* Line 664 of lalr1.cc  */
#line 920 "whereParser.yy"
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
}
    break;

  case 72:
/* Line 664 of lalr1.cc  */
#line 932 "whereParser.yy"
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
}
    break;

  case 73:
/* Line 664 of lalr1.cc  */
#line 944 "whereParser.yy"
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
}
    break;

  case 74:
/* Line 664 of lalr1.cc  */
#line 956 "whereParser.yy"
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
}
    break;

  case 75:
/* Line 664 of lalr1.cc  */
#line 968 "whereParser.yy"
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
}
    break;

  case 76:
/* Line 664 of lalr1.cc  */
#line 980 "whereParser.yy"
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
}
    break;

  case 77:
/* Line 664 of lalr1.cc  */
#line 992 "whereParser.yy"
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
}
    break;

  case 78:
/* Line 664 of lalr1.cc  */
#line 1007 "whereParser.yy"
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
}
    break;

  case 79:
/* Line 664 of lalr1.cc  */
#line 1019 "whereParser.yy"
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
}
    break;

  case 80:
/* Line 664 of lalr1.cc  */
#line 1031 "whereParser.yy"
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
}
    break;

  case 81:
/* Line 664 of lalr1.cc  */
#line 1043 "whereParser.yy"
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
}
    break;

  case 82:
/* Line 664 of lalr1.cc  */
#line 1055 "whereParser.yy"
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
}
    break;

  case 83:
/* Line 664 of lalr1.cc  */
#line 1067 "whereParser.yy"
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
}
    break;

  case 84:
/* Line 664 of lalr1.cc  */
#line 1079 "whereParser.yy"
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
}
    break;

  case 85:
/* Line 664 of lalr1.cc  */
#line 1091 "whereParser.yy"
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
}
    break;

  case 86:
/* Line 664 of lalr1.cc  */
#line 1103 "whereParser.yy"
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
}
    break;

  case 87:
/* Line 664 of lalr1.cc  */
#line 1115 "whereParser.yy"
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
}
    break;

  case 88:
/* Line 664 of lalr1.cc  */
#line 1128 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- FROM_UNIXTIME_LOCAL("
	<< *(yysemantic_stack_[(6) - (3)].whereNode) << ", " << *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif

    ibis::math::fromUnixTime fut((yysemantic_stack_[(6) - (5)].stringVal)->c_str());
    ibis::math::customFunction1 *fun =
	new ibis::math::customFunction1(fut);
    fun->setLeft((yysemantic_stack_[(6) - (3)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(fun);
    delete (yysemantic_stack_[(6) - (5)].stringVal);
}
    break;

  case 89:
/* Line 664 of lalr1.cc  */
#line 1142 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- FROM_UNIXTIME_GMT("
	<< *(yysemantic_stack_[(6) - (3)].whereNode) << ", " << *(yysemantic_stack_[(6) - (5)].stringVal) << ")";
#endif

    ibis::math::fromUnixTime fut((yysemantic_stack_[(6) - (5)].stringVal)->c_str(), "GMT0");
    ibis::math::customFunction1 *fun =
	new ibis::math::customFunction1(fut);
    fun->setLeft((yysemantic_stack_[(6) - (3)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(fun);
    delete (yysemantic_stack_[(6) - (5)].stringVal);
}
    break;

  case 90:
/* Line 664 of lalr1.cc  */
#line 1156 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- TO_UNIXTIME_LOCAL("
	<< *(yysemantic_stack_[(4) - (3)].whereNode) << ")";
#endif

    ibis::math::toUnixTime fut;
    ibis::math::customFunction1 *fun =
	new ibis::math::customFunction1(fut);
    fun->setLeft((yysemantic_stack_[(4) - (3)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(fun);
}
    break;

  case 91:
/* Line 664 of lalr1.cc  */
#line 1169 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- TO_UNIXTIME_GMT("
	<< *(yysemantic_stack_[(4) - (3)].whereNode) << ")";
#endif

    ibis::math::toUnixTime fut("GMT0");
    ibis::math::customFunction1 *fun =
	new ibis::math::customFunction1(fut);
    fun->setLeft((yysemantic_stack_[(4) - (3)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(fun);
}
    break;

  case 92:
/* Line 664 of lalr1.cc  */
#line 1182 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *(yysemantic_stack_[(2) - (2)].whereNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight((yysemantic_stack_[(2) - (2)].whereNode));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(opr);
}
    break;

  case 93:
/* Line 664 of lalr1.cc  */
#line 1192 "whereParser.yy"
    {
    (yyval.whereNode) = (yysemantic_stack_[(2) - (2)].whereNode);
}
    break;

  case 94:
/* Line 664 of lalr1.cc  */
#line 1195 "whereParser.yy"
    {
    (yyval.whereNode) = (yysemantic_stack_[(3) - (2)].whereNode);
}
    break;

  case 95:
/* Line 664 of lalr1.cc  */
#line 1198 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    ibis::math::variable *var =
	new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    (yyval.whereNode) = static_cast<ibis::qExpr*>(var);
    delete (yysemantic_stack_[(1) - (1)].stringVal);
}
    break;

  case 96:
/* Line 664 of lalr1.cc  */
#line 1208 "whereParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    ibis::math::number *num = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
    (yyval.whereNode) = static_cast<ibis::qExpr*>(num);
}
    break;

  case 97:
/* Line 664 of lalr1.cc  */
#line 1218 "whereParser.yy"
    { /* pass qexpr to the driver */
    driver.expr_ = (yysemantic_stack_[(2) - (1)].whereNode);
}
    break;

  case 98:
/* Line 664 of lalr1.cc  */
#line 1221 "whereParser.yy"
    { /* pass qexpr to the driver */
    driver.expr_ = (yysemantic_stack_[(2) - (1)].whereNode);
}
    break;


/* Line 664 of lalr1.cc  */
#line 2072 "whereParser.cc"
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
  whereParser::yysyntax_error_ (int yystate, int yytoken)
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
  const signed char whereParser::yypact_ninf_ = -34;
  const short int
  whereParser::yypact_[] =
  {
        74,    74,   187,   -33,   -19,   -15,   -13,    -2,    83,    83,
     -34,   104,    -6,    74,    21,   -34,   -34,   -34,   134,    36,
     -34,   -34,   -34,   -26,    83,    83,    83,    83,    30,    32,
      83,   -34,   -34,    11,   199,   223,   147,    33,   -18,    83,
      41,    61,    -5,    56,   -34,    74,    74,    74,    74,   -34,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,   -34,    63,   109,   258,   279,
     287,   307,   123,   315,   -34,   286,   -34,   -34,   -34,   170,
     -34,   -34,   -34,   170,   -34,   -34,   -34,    51,   -34,   -34,
     -34,   -34,    39,   -34,   -34,   250,   -34,   -34,   -34,   -34,
     -34,   -34,    26,   212,   143,   178,   151,   186,   170,   170,
     242,   270,   325,   339,   339,   157,   157,   157,   157,   -34,
     -34,   150,   152,   -34,   -34,    -7,   -34,   -34,   -34,   -34,
     116,   101,   203,   210,   215,   218,   -34,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,   185,   192,   196,
     200,   247,   276,   283,   -34,    52,   -34,    78,   -34,   206,
     -34,    86,   -34,    87,   335,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   -34,   -34,   -34,   -34,   -34,   214,
     -34,   114,   -34,   197,   208,   221,   249,   304,   305,   306,
     314,   328,   329,   -34,   330,   331,   332,   333,   334,   -34,
     -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
     -34,   -34,   -34
  };

  /* YYDEFACT[S] -- default reduction number in state S.  Performed when
     YYTABLE doesn't specify something else to do.  Zero means the
     default is an error.  */
  const unsigned char
  whereParser::yydefact_[] =
  {
         0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      96,    95,     0,     0,     0,     8,     9,    10,     0,     0,
       6,    11,    12,     0,     0,     0,     0,     0,     0,    95,
       0,    93,    92,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    97,     0,     0,     0,     0,    98,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,    18,     0,    53,    55,    59,    61,
      54,    56,    60,    62,    42,    50,    43,     0,    38,    40,
      15,    22,     0,    29,    30,     0,    57,    58,     7,    94,
       4,     5,     2,     3,    66,    68,    65,    67,    63,    64,
       0,    85,    84,    78,    79,    80,    81,    82,    83,    13,
      14,     0,     0,    91,    90,     0,    39,    41,    19,    31,
       0,     0,     0,     0,     0,     0,    86,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    44,     0,    45,     0,    17,     0,
      27,     0,    28,     0,     0,    72,    71,    76,    75,    70,
      69,    74,    73,    77,    89,    88,    51,    52,    21,     0,
      36,     0,    37,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    87,     0,     0,     0,     0,     0,    49,
      48,    47,    46,    16,    23,    25,    24,    26,    20,    32,
      34,    33,    35
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  whereParser::yypgoto_[] =
  {
       -34,    12,   -34,   -34,   -34,    -8,   -34
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  whereParser::yydefgoto_[] =
  {
        -1,    14,    15,    16,    17,    18,    19
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If YYTABLE_NINF_, syntax error.  */
  const signed char whereParser::yytable_ninf_ = -1;
  const unsigned char
  whereParser::yytable_[] =
  {
        31,    32,   149,    40,    41,    43,    45,    46,    47,    48,
      24,   150,    66,    20,    74,    67,    68,    69,    70,    71,
      93,    44,    73,    94,    25,    42,    79,    83,    26,    75,
      27,    95,    45,    46,    47,    48,    65,    45,    46,    98,
      48,    28,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   100,   101,   102,
     103,    50,    51,    52,    53,    54,    55,    49,    72,    88,
      89,    56,    90,    91,   133,    39,    92,   134,     1,    96,
     135,    57,    58,    59,    60,    61,    62,    63,    64,   131,
     184,     2,   132,   185,     3,     4,     5,     6,     7,    97,
      99,     8,     9,     3,     4,     5,     6,   119,    33,    10,
       8,     9,    11,    34,    35,    12,   186,    13,    10,   187,
      36,    29,    37,    38,   189,   191,    30,   190,   192,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,    50,
      51,    52,    53,    54,    55,   154,   155,    39,   138,    56,
     139,   151,   195,   120,   152,   196,   142,   153,   143,    57,
      58,    59,    60,    61,    62,    63,    64,   125,    57,    58,
      59,    60,    61,    62,    63,    64,    57,    58,    59,    60,
      61,    62,    63,    64,   140,    84,   141,    85,    86,    64,
      87,   147,   144,   148,   145,    57,    58,    59,    60,    61,
      62,    63,    64,    57,    58,    59,    60,    61,    62,    63,
      64,    57,    58,    59,    60,    61,    62,    63,    64,     3,
       4,     5,     6,    45,    46,    21,     8,     9,    22,   174,
      23,   176,    76,    77,    10,   197,   175,    29,   198,   177,
      78,   188,    30,     3,     4,     5,     6,   156,   157,   194,
       8,     9,   199,   146,   158,   159,    80,    81,    10,   160,
     161,    29,   162,   163,    82,   200,    30,    57,    58,    59,
      60,    61,    62,    63,    64,    57,    58,    59,    60,    61,
      62,    63,    64,    57,    58,    59,    60,    61,    62,    63,
      64,   178,   179,   201,   136,   137,    58,    59,    60,    61,
      62,    63,    64,   121,    57,    58,    59,    60,    61,    62,
      63,    64,    57,    58,    59,    60,    61,    62,    63,    64,
     180,   181,   126,   127,   122,   128,   129,   182,   183,   130,
       0,   123,    57,    58,    59,    60,    61,    62,    63,    64,
      57,    58,    59,    60,    61,    62,    63,    64,   202,   203,
     204,   124,    59,    60,    61,    62,    63,    64,   205,    99,
      57,    58,    59,    60,    61,    62,    63,    64,    61,    62,
      63,    64,   206,   207,   208,   209,   210,   211,   212,   193
  };

  /* YYCHECK.  */
  const short int
  whereParser::yycheck_[] =
  {
         8,     9,     9,     9,    10,    13,    11,    12,    13,    14,
      43,    18,    38,     1,     3,    41,    24,    25,    26,    27,
      38,     0,    30,    41,    43,    13,    34,    35,    43,    18,
      43,    39,    11,    12,    13,    14,     0,    11,    12,    44,
      14,    43,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    45,    46,    47,
      48,     5,     6,     7,     8,     9,    10,    46,    38,    36,
      37,    15,    39,    40,    35,    43,    43,    38,     4,    38,
      41,    25,    26,    27,    28,    29,    30,    31,    32,    38,
      38,    17,    41,    41,    20,    21,    22,    23,    24,    38,
      44,    27,    28,    20,    21,    22,    23,    44,     4,    35,
      27,    28,    38,     9,    10,    41,    38,    43,    35,    41,
      16,    38,    18,    19,    38,    38,    43,    41,    41,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,     5,
       6,     7,     8,     9,    10,    44,    45,    43,     5,    15,
       7,    35,    38,    44,    38,    41,     5,    41,     7,    25,
      26,    27,    28,    29,    30,    31,    32,    44,    25,    26,
      27,    28,    29,    30,    31,    32,    25,    26,    27,    28,
      29,    30,    31,    32,     6,    38,     8,    40,    41,    32,
      43,    41,     6,    41,     8,    25,    26,    27,    28,    29,
      30,    31,    32,    25,    26,    27,    28,    29,    30,    31,
      32,    25,    26,    27,    28,    29,    30,    31,    32,    20,
      21,    22,    23,    11,    12,    38,    27,    28,    41,    44,
      43,    35,    33,    34,    35,    38,    44,    38,    41,    39,
      41,    35,    43,    20,    21,    22,    23,    44,    45,    35,
      27,    28,    44,    11,    44,    45,    33,    34,    35,    44,
      45,    38,    44,    45,    41,    44,    43,    25,    26,    27,
      28,    29,    30,    31,    32,    25,    26,    27,    28,    29,
      30,    31,    32,    25,    26,    27,    28,    29,    30,    31,
      32,    44,    45,    44,    44,    45,    26,    27,    28,    29,
      30,    31,    32,    45,    25,    26,    27,    28,    29,    30,
      31,    32,    25,    26,    27,    28,    29,    30,    31,    32,
      44,    45,    36,    37,    45,    39,    40,    44,    45,    43,
      -1,    44,    25,    26,    27,    28,    29,    30,    31,    32,
      25,    26,    27,    28,    29,    30,    31,    32,    44,    44,
      44,    44,    27,    28,    29,    30,    31,    32,    44,    44,
      25,    26,    27,    28,    29,    30,    31,    32,    29,    30,
      31,    32,    44,    44,    44,    44,    44,    44,    44,    44
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  whereParser::yystos_[] =
  {
         0,     4,    17,    20,    21,    22,    23,    24,    27,    28,
      35,    38,    41,    43,    48,    49,    50,    51,    52,    53,
      48,    38,    41,    43,    43,    43,    43,    43,    43,    38,
      43,    52,    52,     4,     9,    10,    16,    18,    19,    43,
       9,    10,    48,    52,     0,    11,    12,    13,    14,    46,
       5,     6,     7,     8,     9,    10,    15,    25,    26,    27,
      28,    29,    30,    31,    32,     0,    38,    41,    52,    52,
      52,    52,    38,    52,     3,    18,    33,    34,    41,    52,
      33,    34,    41,    52,    38,    40,    41,    43,    36,    37,
      39,    40,    43,    38,    41,    52,    38,    38,    44,    44,
      48,    48,    48,    48,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    44,
      44,    45,    45,    44,    44,    44,    36,    37,    39,    40,
      43,    38,    41,    35,    38,    41,    44,    45,     5,     7,
       6,     8,     5,     7,     6,     8,    11,    41,    41,     9,
      18,    35,    38,    41,    44,    45,    44,    45,    44,    45,
      44,    45,    44,    45,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    44,    44,    35,    39,    44,    45,
      44,    45,    44,    45,    38,    41,    38,    41,    35,    38,
      41,    38,    41,    44,    35,    38,    41,    38,    41,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      44,    44,    44
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,    40,    41,    44,    59
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  whereParser::yyr1_[] =
  {
         0,    47,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    50,    50,    50,    50,    50,    50,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    52,    52,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    52,    52,    52,    53,    53
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  whereParser::yyr2_[] =
  {
         0,     2,     3,     3,     3,     3,     2,     3,     1,     1,
       1,     2,     2,     4,     4,     3,     7,     5,     3,     4,
       8,     6,     3,     7,     7,     7,     7,     5,     5,     3,
       3,     4,     8,     8,     8,     8,     6,     6,     3,     4,
       3,     4,     3,     3,     5,     5,     7,     7,     7,     7,
       3,     6,     6,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     5,
       5,     5,     5,     5,     5,     5,     5,     5,     3,     3,
       3,     3,     3,     3,     3,     3,     4,     6,     6,     6,
       4,     4,     2,     2,     3,     1,     1,     2,     2
  };


  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const whereParser::yytname_[] =
  {
    "\"end of input\"", "error", "$undefined", "\"null\"", "\"not\"",
  "\"<=\"", "\">=\"", "\"<\"", "\">\"", "\"==\"", "\"!=\"", "\"and\"",
  "\"&!\"", "\"or\"", "\"xor\"", "\"between\"", "\"contains\"",
  "\"exists\"", "\"in\"", "\"like\"", "\"FROM_UNIXTIME_GMT\"",
  "\"FROM_UNIXTIME_LOCAL\"", "\"TO_UNIXTIME_GMT\"",
  "\"TO_UNIXTIME_LOCAL\"", "\"any\"", "\"|\"", "\"&\"", "\"+\"", "\"-\"",
  "\"*\"", "\"/\"", "\"%\"", "\"**\"", "\"integer value\"",
  "\"unsigned integer value\"", "\"floating-point number\"",
  "\"signed integer sequence\"", "\"unsigned integer sequence\"",
  "\"name string\"", "\"number sequence\"", "\"string sequence\"",
  "\"string literal\"", "CONSTAINSOP", "'('", "')'", "','", "';'",
  "$accept", "qexpr", "simpleRange", "compRange2", "compRange3",
  "mathExpr", "START", YY_NULL
  };

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const whereParser::rhs_number_type
  whereParser::yyrhs_[] =
  {
        53,     0,    -1,    48,    13,    48,    -1,    48,    14,    48,
      -1,    48,    11,    48,    -1,    48,    12,    48,    -1,     4,
      48,    -1,    43,    48,    44,    -1,    49,    -1,    50,    -1,
      51,    -1,    17,    38,    -1,    17,    41,    -1,    17,    43,
      38,    44,    -1,    17,    43,    41,    44,    -1,    38,    18,
      39,    -1,    38,    18,    43,    35,    45,    35,    44,    -1,
      38,    18,    43,    35,    44,    -1,    38,     4,     3,    -1,
      38,     4,    18,    39,    -1,    38,     4,    18,    43,    35,
      45,    35,    44,    -1,    38,     4,    18,    43,    35,    44,
      -1,    38,    18,    40,    -1,    38,    18,    43,    38,    45,
      38,    44,    -1,    38,    18,    43,    41,    45,    38,    44,
      -1,    38,    18,    43,    38,    45,    41,    44,    -1,    38,
      18,    43,    41,    45,    41,    44,    -1,    38,    18,    43,
      38,    44,    -1,    38,    18,    43,    41,    44,    -1,    38,
      19,    38,    -1,    38,    19,    41,    -1,    38,     4,    18,
      40,    -1,    38,     4,    18,    43,    38,    45,    38,    44,
      -1,    38,     4,    18,    43,    41,    45,    38,    44,    -1,
      38,     4,    18,    43,    38,    45,    41,    44,    -1,    38,
       4,    18,    43,    41,    45,    41,    44,    -1,    38,     4,
      18,    43,    38,    44,    -1,    38,     4,    18,    43,    41,
      44,    -1,    38,    18,    36,    -1,    38,     4,    18,    36,
      -1,    38,    18,    37,    -1,    38,     4,    18,    37,    -1,
      38,    16,    38,    -1,    38,    16,    41,    -1,    38,    16,
      43,    38,    44,    -1,    38,    16,    43,    41,    44,    -1,
      38,    16,    43,    41,    45,    41,    44,    -1,    38,    16,
      43,    41,    45,    38,    44,    -1,    38,    16,    43,    38,
      45,    41,    44,    -1,    38,    16,    43,    38,    45,    38,
      44,    -1,    38,    16,    40,    -1,    24,    43,    38,    44,
       9,    35,    -1,    24,    43,    38,    44,    18,    39,    -1,
      38,     9,    33,    -1,    38,    10,    33,    -1,    38,     9,
      34,    -1,    38,    10,    34,    -1,    41,     9,    38,    -1,
      41,    10,    38,    -1,    38,     9,    41,    -1,    38,    10,
      41,    -1,    38,     9,    52,    -1,    38,    10,    52,    -1,
      52,     9,    52,    -1,    52,    10,    52,    -1,    52,     7,
      52,    -1,    52,     5,    52,    -1,    52,     8,    52,    -1,
      52,     6,    52,    -1,    52,     7,    52,     7,    52,    -1,
      52,     7,    52,     5,    52,    -1,    52,     5,    52,     7,
      52,    -1,    52,     5,    52,     5,    52,    -1,    52,     8,
      52,     8,    52,    -1,    52,     8,    52,     6,    52,    -1,
      52,     6,    52,     8,    52,    -1,    52,     6,    52,     6,
      52,    -1,    52,    15,    52,    11,    52,    -1,    52,    27,
      52,    -1,    52,    28,    52,    -1,    52,    29,    52,    -1,
      52,    30,    52,    -1,    52,    31,    52,    -1,    52,    32,
      52,    -1,    52,    26,    52,    -1,    52,    25,    52,    -1,
      38,    43,    52,    44,    -1,    38,    43,    52,    45,    52,
      44,    -1,    21,    43,    52,    45,    41,    44,    -1,    20,
      43,    52,    45,    41,    44,    -1,    23,    43,    52,    44,
      -1,    22,    43,    52,    44,    -1,    28,    52,    -1,    27,
      52,    -1,    43,    52,    44,    -1,    38,    -1,    35,    -1,
      48,     0,    -1,    48,    46,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  whereParser::yyprhs_[] =
  {
         0,     0,     3,     7,    11,    15,    19,    22,    26,    28,
      30,    32,    35,    38,    43,    48,    52,    60,    66,    70,
      75,    84,    91,    95,   103,   111,   119,   127,   133,   139,
     143,   147,   152,   161,   170,   179,   188,   195,   202,   206,
     211,   215,   220,   224,   228,   234,   240,   248,   256,   264,
     272,   276,   283,   290,   294,   298,   302,   306,   310,   314,
     318,   322,   326,   330,   334,   338,   342,   346,   350,   354,
     360,   366,   372,   378,   384,   390,   396,   402,   408,   412,
     416,   420,   424,   428,   432,   436,   440,   445,   452,   459,
     466,   471,   476,   479,   482,   486,   488,   490,   493
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  whereParser::yyrline_[] =
  {
         0,   112,   112,   122,   132,   142,   152,   160,   163,   164,
     165,   169,   177,   185,   193,   201,   211,   223,   232,   239,
     250,   263,   273,   283,   300,   317,   334,   351,   365,   379,
     389,   399,   410,   428,   446,   464,   482,   497,   512,   522,
     533,   543,   554,   564,   574,   584,   594,   605,   616,   627,
     638,   648,   657,   667,   675,   684,   692,   701,   711,   722,
     732,   743,   760,   783,   793,   804,   814,   824,   834,   896,
     908,   920,   932,   944,   956,   968,   980,   992,  1007,  1019,
    1031,  1043,  1055,  1067,  1079,  1091,  1103,  1115,  1128,  1142,
    1156,  1169,  1182,  1192,  1195,  1198,  1208,  1218,  1221
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
      43,    44,     2,     2,    45,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    46,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int whereParser::yyeof_ = 0;
  const int whereParser::yylast_ = 379;
  const int whereParser::yynnts_ = 7;
  const int whereParser::yyempty_ = -2;
  const int whereParser::yyfinal_ = 65;
  const int whereParser::yyterror_ = 1;
  const int whereParser::yyerrcode_ = 256;
  const int whereParser::yyntokens_ = 47;

  const unsigned int whereParser::yyuser_token_number_max_ = 297;
  const whereParser::token_number_type whereParser::yyundef_token_ = 2;


} // ibis
/* Line 1135 of lalr1.cc  */
#line 2810 "whereParser.cc"
/* Line 1136 of lalr1.cc  */
#line 1226 "whereParser.yy"

void ibis::whereParser::error(const ibis::whereParser::location_type& l,
			      const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::whereParser encountered " << m
	<< " at location " << l;
}

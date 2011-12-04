/* A Bison parser, made by GNU Bison 2.5.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++
   
      Copyright (C) 2002-2011 Free Software Foundation, Inc.
   
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

/* Line 286 of lalr1.cc  */
#line 6 "fromParser.yy"

/** \file Defines the parser for the from clause accepted by FastBit
    IBIS.  The definitions are processed through bison.
*/
#include <iostream>



/* Line 286 of lalr1.cc  */
#line 46 "fromParser.cc"

// Take the name prefix into account.
#define yylex   ibislex

/* First part of user declarations.  */


/* Line 293 of lalr1.cc  */
#line 55 "fromParser.cc"


#include "fromParser.hh"

/* User implementation prologue.  */

/* Line 299 of lalr1.cc  */
#line 80 "fromParser.yy"

#include "fromLexer.h"

#undef yylex
#define yylex driver.lexer->lex


/* Line 299 of lalr1.cc  */
#line 72 "fromParser.cc"

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

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                               \
 do                                                                    \
   if (N)                                                              \
     {                                                                 \
       (Current).begin = YYRHSLOC (Rhs, 1).begin;                      \
       (Current).end   = YYRHSLOC (Rhs, N).end;                        \
     }                                                                 \
   else                                                                \
     {                                                                 \
       (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;        \
     }                                                                 \
 while (false)
#endif

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
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_REDUCE_PRINT(Rule)
# define YY_STACK_PRINT()

#endif /* !YYDEBUG */

#define yyerrok		(yyerrstatus_ = 0)
#define yyclearin	(yychar = yyempty_)

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)


namespace ibis {

/* Line 382 of lalr1.cc  */
#line 158 "fromParser.cc"

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  fromParser::yytnamerr_ (const char *yystr)
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
  fromParser::fromParser (class ibis::fromClause& driver_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      driver (driver_yyarg)
  {
  }

  fromParser::~fromParser ()
  {
  }

#if YYDEBUG
  /*--------------------------------.
  | Print this symbol on YYOUTPUT.  |
  `--------------------------------*/

  inline void
  fromParser::yy_symbol_value_print_ (int yytype,
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
  fromParser::yy_symbol_print_ (int yytype,
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
  fromParser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
      {
        case 24: /* "\"name\"" */

/* Line 480 of lalr1.cc  */
#line 77 "fromParser.yy"
	{ delete (yyvaluep->stringVal); };

/* Line 480 of lalr1.cc  */
#line 263 "fromParser.cc"
	break;
      case 32: /* "compRange2" */

/* Line 480 of lalr1.cc  */
#line 78 "fromParser.yy"
	{ delete (yyvaluep->fromNode); };

/* Line 480 of lalr1.cc  */
#line 272 "fromParser.cc"
	break;
      case 33: /* "compRange3" */

/* Line 480 of lalr1.cc  */
#line 78 "fromParser.yy"
	{ delete (yyvaluep->fromNode); };

/* Line 480 of lalr1.cc  */
#line 281 "fromParser.cc"
	break;
      case 34: /* "mathExpr" */

/* Line 480 of lalr1.cc  */
#line 78 "fromParser.yy"
	{ delete (yyvaluep->fromNode); };

/* Line 480 of lalr1.cc  */
#line 290 "fromParser.cc"
	break;

	default:
	  break;
      }
  }

  void
  fromParser::yypop_ (unsigned int n)
  {
    yystate_stack_.pop (n);
    yysemantic_stack_.pop (n);
    yylocation_stack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  fromParser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  fromParser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  fromParser::debug_level_type
  fromParser::debug_level () const
  {
    return yydebug_;
  }

  void
  fromParser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif

  inline bool
  fromParser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  fromParser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  fromParser::parse ()
  {
    /// Lookahead and lookahead in internal form.
    int yychar = yyempty_;
    int yytoken = 0;

    /* State.  */
    int yyn;
    int yylen = 0;
    int yystate = 0;

    /* Error handling.  */
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// Semantic value of the lookahead.
    semantic_type yylval;
    /// Location of the lookahead.
    location_type yylloc;
    /// The locations where the error started and ended.
    location_type yyerror_range[3];

    /// $$.
    semantic_type yyval;
    /// @$.
    location_type yyloc;

    int yyresult;

    YYCDEBUG << "Starting parse" << std::endl;


    /* User initialization code.  */
    
/* Line 565 of lalr1.cc  */
#line 28 "fromParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}

/* Line 565 of lalr1.cc  */
#line 387 "fromParser.cc"

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

    {
      slice<location_type, location_stack_type> slice (yylocation_stack_, yylen);
      YYLLOC_DEFAULT (yyloc, slice, yylen);
    }
    YY_REDUCE_PRINT (yyn);
    switch (yyn)
      {
	  case 4:

/* Line 690 of lalr1.cc  */
#line 89 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(2) - (1)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 5:

/* Line 690 of lalr1.cc  */
#line 93 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(2) - (1)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 6:

/* Line 690 of lalr1.cc  */
#line 97 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(3) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(3) - (2)].stringVal));
}
    break;

  case 7:

/* Line 690 of lalr1.cc  */
#line 101 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(3) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(3) - (2)].stringVal));
}
    break;

  case 8:

/* Line 690 of lalr1.cc  */
#line 104 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(4) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(4) - (3)].stringVal));
}
    break;

  case 9:

/* Line 690 of lalr1.cc  */
#line 108 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(4) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(4) - (3)].stringVal));
}
    break;

  case 10:

/* Line 690 of lalr1.cc  */
#line 112 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(4) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(4) - (3)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 11:

/* Line 690 of lalr1.cc  */
#line 118 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(6) - (5)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 12:

/* Line 690 of lalr1.cc  */
#line 124 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (7)].stringVal));
}
    break;

  case 13:

/* Line 690 of lalr1.cc  */
#line 130 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(6) - (5)].stringVal));
}
    break;

  case 14:

/* Line 690 of lalr1.cc  */
#line 136 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.aliases_.push_back("");
    std::string nm1 = *(yysemantic_stack_[(6) - (1)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(6) - (3)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(6) - (5)].stringVal);
    nm2 += *(yysemantic_stack_[(6) - (5)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 15:

/* Line 690 of lalr1.cc  */
#line 151 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.aliases_.push_back("");
    std::string nm1 = *(yysemantic_stack_[(8) - (3)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(8) - (5)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(8) - (7)].stringVal);
    nm2 += *(yysemantic_stack_[(8) - (7)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 16:

/* Line 690 of lalr1.cc  */
#line 166 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(10) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(10) - (5)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (7)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(10) - (3)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(10) - (7)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(10) - (9)].stringVal);
    nm2 += *(yysemantic_stack_[(10) - (9)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 17:

/* Line 690 of lalr1.cc  */
#line 181 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(8) - (1)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(8) - (5)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(8) - (7)].stringVal);
    nm2 += *(yysemantic_stack_[(8) - (7)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 18:

/* Line 690 of lalr1.cc  */
#line 196 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(7) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(7) - (2)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(7) - (4)].stringVal));
    driver.aliases_.push_back("");
    std::string nm1 = *(yysemantic_stack_[(7) - (2)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(7) - (4)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(7) - (6)].stringVal);
    nm2 += *(yysemantic_stack_[(7) - (6)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 19:

/* Line 690 of lalr1.cc  */
#line 211 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (2)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (4)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(8) - (2)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(8) - (5)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(8) - (7)].stringVal);
    nm2 += *(yysemantic_stack_[(8) - (7)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 20:

/* Line 690 of lalr1.cc  */
#line 226 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(7) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(7) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(7) - (4)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(7) - (1)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(7) - (4)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(7) - (6)].stringVal);
    nm2 += *(yysemantic_stack_[(7) - (6)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 21:

/* Line 690 of lalr1.cc  */
#line 241 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.aliases_.push_back("");
    std::string nm1 = *(yysemantic_stack_[(8) - (1)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(8) - (3)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(8) - (6)].stringVal);
    nm2 += *(yysemantic_stack_[(8) - (6)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 22:

/* Line 690 of lalr1.cc  */
#line 256 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(10) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(10) - (5)].stringVal));
    driver.aliases_.push_back("");
    std::string nm1 = *(yysemantic_stack_[(10) - (3)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(10) - (5)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(10) - (8)].stringVal);
    nm2 += *(yysemantic_stack_[(10) - (8)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 23:

/* Line 690 of lalr1.cc  */
#line 271 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(12) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(12) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(12) - (5)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(12) - (7)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(12) - (3)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(12) - (7)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(12) - (10)].stringVal);
    nm2 += *(yysemantic_stack_[(12) - (10)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 24:

/* Line 690 of lalr1.cc  */
#line 286 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(10) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(10) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (5)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(10) - (1)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(10) - (5)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(10) - (8)].stringVal);
    nm2 += *(yysemantic_stack_[(10) - (8)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 25:

/* Line 690 of lalr1.cc  */
#line 301 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(9) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(9) - (2)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(9) - (4)].stringVal));
    driver.aliases_.push_back("");
    std::string nm1 = *(yysemantic_stack_[(9) - (2)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(9) - (4)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(9) - (7)].stringVal);
    nm2 += *(yysemantic_stack_[(9) - (7)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 26:

/* Line 690 of lalr1.cc  */
#line 316 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(10) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (2)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(10) - (4)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (5)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(10) - (2)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(10) - (5)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(10) - (8)].stringVal);
    nm2 += *(yysemantic_stack_[(10) - (8)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 27:

/* Line 690 of lalr1.cc  */
#line 331 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(9) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(9) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(9) - (4)].stringVal));
    std::string nm1 = *(yysemantic_stack_[(9) - (1)].stringVal);
    std::string nm2 = *(yysemantic_stack_[(9) - (4)].stringVal);
    nm1 += '.';
    nm2 += '.';
    nm1 += *(yysemantic_stack_[(9) - (7)].stringVal);
    nm2 += *(yysemantic_stack_[(9) - (7)].stringVal);
    ibis::math::variable* var1 = new ibis::math::variable(nm1.c_str());
    ibis::math::variable* var2 = new ibis::math::variable(nm2.c_str());
    driver.jcond_ = new ibis::compRange(var1, ibis::qExpr::OP_EQ, var2);
}
    break;

  case 28:

/* Line 690 of lalr1.cc  */
#line 346 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(6) - (5)].fromNode));
}
    break;

  case 29:

/* Line 690 of lalr1.cc  */
#line 353 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(8) - (7)].fromNode));
}
    break;

  case 30:

/* Line 690 of lalr1.cc  */
#line 360 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(10) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(10) - (5)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (7)].stringVal));
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(10) - (9)].fromNode));
}
    break;

  case 31:

/* Line 690 of lalr1.cc  */
#line 367 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(8) - (7)].fromNode));
}
    break;

  case 32:

/* Line 690 of lalr1.cc  */
#line 374 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(7) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(7) - (2)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(7) - (4)].stringVal));
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(7) - (6)].fromNode));
}
    break;

  case 33:

/* Line 690 of lalr1.cc  */
#line 381 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (2)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (4)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(8) - (7)].fromNode));
}
    break;

  case 34:

/* Line 690 of lalr1.cc  */
#line 388 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(7) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(7) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(7) - (4)].stringVal));
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(7) - (6)].fromNode));
}
    break;

  case 37:

/* Line 690 of lalr1.cc  */
#line 399 "fromParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " = "
	<< *me2;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2);
}
    break;

  case 38:

/* Line 690 of lalr1.cc  */
#line 409 "fromParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " != "
	<< *me2;
#endif
    (yyval.fromNode) = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
    (yyval.fromNode)->setLeft(new ibis::compRange(me1, ibis::qExpr::OP_EQ, me2));
}
    break;

  case 39:

/* Line 690 of lalr1.cc  */
#line 420 "fromParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2);
}
    break;

  case 40:

/* Line 690 of lalr1.cc  */
#line 430 "fromParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2);
}
    break;

  case 41:

/* Line 690 of lalr1.cc  */
#line 440 "fromParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_GT, me2);
}
    break;

  case 42:

/* Line 690 of lalr1.cc  */
#line 450 "fromParser.yy"
    {
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(3) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_GE, me2);
}
    break;

  case 43:

/* Line 690 of lalr1.cc  */
#line 463 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2 << " < " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LT, me3);
}
    break;

  case 44:

/* Line 690 of lalr1.cc  */
#line 475 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " < "
	<< *me2 << " <= " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LE, me3);
}
    break;

  case 45:

/* Line 690 of lalr1.cc  */
#line 487 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2 << " < " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LT, me3);
}
    break;

  case 46:

/* Line 690 of lalr1.cc  */
#line 499 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " <= "
	<< *me2 << " <= " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me1, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LE, me3);
}
    break;

  case 47:

/* Line 690 of lalr1.cc  */
#line 511 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2 << " > " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me3, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LT, me1);
}
    break;

  case 48:

/* Line 690 of lalr1.cc  */
#line 523 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " > "
	<< *me2 << " >= " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me3, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LT, me1);
}
    break;

  case 49:

/* Line 690 of lalr1.cc  */
#line 535 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2 << " > " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me3, ibis::qExpr::OP_LT, me2,
			     ibis::qExpr::OP_LE, me1);
}
    break;

  case 50:

/* Line 690 of lalr1.cc  */
#line 547 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " >= "
	<< *me2 << " >= " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me3, ibis::qExpr::OP_LE, me2,
			     ibis::qExpr::OP_LE, me1);
}
    break;

  case 51:

/* Line 690 of lalr1.cc  */
#line 559 "fromParser.yy"
    {
    ibis::math::term *me3 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (5)].fromNode));
    ibis::math::term *me2 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (3)].fromNode));
    ibis::math::term *me1 = static_cast<ibis::math::term*>((yysemantic_stack_[(5) - (1)].fromNode));
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *me1 << " BETWEEN "
	<< *me2 << " AND " << *me3;
#endif
    (yyval.fromNode) = new ibis::compRange(me2, ibis::qExpr::OP_LE, me1,
			     ibis::qExpr::OP_LE, me3);
}
    break;

  case 52:

/* Line 690 of lalr1.cc  */
#line 574 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " + " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::PLUS);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 53:

/* Line 690 of lalr1.cc  */
#line 586 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " - " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MINUS);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 54:

/* Line 690 of lalr1.cc  */
#line 598 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " * " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::MULTIPLY);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 55:

/* Line 690 of lalr1.cc  */
#line 610 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " / " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::DIVIDE);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 56:

/* Line 690 of lalr1.cc  */
#line 622 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " % " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::REMAINDER);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 57:

/* Line 690 of lalr1.cc  */
#line 634 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " ^ " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::POWER);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 58:

/* Line 690 of lalr1.cc  */
#line 646 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " & " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITAND);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 59:

/* Line 690 of lalr1.cc  */
#line 658 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(3) - (1)].fromNode)
	<< " | " << *(yysemantic_stack_[(3) - (3)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::BITOR);
    opr->setRight((yysemantic_stack_[(3) - (3)].fromNode));
    opr->setLeft((yysemantic_stack_[(3) - (1)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 60:

/* Line 690 of lalr1.cc  */
#line 670 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(4) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(4) - (3)].fromNode) << ")";
#endif
    ibis::math::stdFunction1 *fun =
	new ibis::math::stdFunction1((yysemantic_stack_[(4) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(4) - (1)].stringVal);
    fun->setLeft((yysemantic_stack_[(4) - (3)].fromNode));
    (yyval.fromNode) = fun;
}
    break;

  case 61:

/* Line 690 of lalr1.cc  */
#line 682 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- " << *(yysemantic_stack_[(6) - (1)].stringVal) << "("
	<< *(yysemantic_stack_[(6) - (3)].fromNode) << ", " << *(yysemantic_stack_[(6) - (5)].fromNode) << ")";
#endif
    ibis::math::stdFunction2 *fun =
	new ibis::math::stdFunction2((yysemantic_stack_[(6) - (1)].stringVal)->c_str());
    fun->setRight((yysemantic_stack_[(6) - (5)].fromNode));
    fun->setLeft((yysemantic_stack_[(6) - (3)].fromNode));
    (yyval.fromNode) = fun;
    delete (yysemantic_stack_[(6) - (1)].stringVal);
}
    break;

  case 62:

/* Line 690 of lalr1.cc  */
#line 695 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " parsing -- - " << *(yysemantic_stack_[(2) - (2)].fromNode);
#endif
    ibis::math::bediener *opr =
	new ibis::math::bediener(ibis::math::NEGATE);
    opr->setRight((yysemantic_stack_[(2) - (2)].fromNode));
    (yyval.fromNode) = opr;
}
    break;

  case 63:

/* Line 690 of lalr1.cc  */
#line 705 "fromParser.yy"
    {
    (yyval.fromNode) = (yysemantic_stack_[(2) - (2)].fromNode);
}
    break;

  case 64:

/* Line 690 of lalr1.cc  */
#line 708 "fromParser.yy"
    {
    (yyval.fromNode) = (yysemantic_stack_[(3) - (2)].fromNode);
}
    break;

  case 65:

/* Line 690 of lalr1.cc  */
#line 711 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.fromNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
}
    break;

  case 66:

/* Line 690 of lalr1.cc  */
#line 719 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.fromNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
}
    break;



/* Line 690 of lalr1.cc  */
#line 1500 "fromParser.cc"
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
  fromParser::yysyntax_error_ (int yystate, int yytoken)
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

    char const* yyformat = 0;
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
  const signed char fromParser::yypact_ninf_ = -36;
  const short int
  fromParser::yypact_[] =
  {
       -18,    21,    11,   -18,   -36,   -12,    -6,     2,   -36,   -36,
     -36,    44,    47,   -36,    -1,   -36,   -36,    42,   -36,   -36,
      43,    -9,    39,    91,    48,    16,    64,    -9,    -9,   -36,
      85,    -9,   122,   -36,   -36,    72,   125,    88,    -9,    84,
      -9,   100,   154,   -36,   119,    -9,   167,   -36,    -9,   190,
     126,   126,    -9,   155,   -36,    -9,    -9,    -9,    -9,    -9,
      -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,
     -36,   165,   161,   212,   170,   213,   222,   199,    -9,   194,
     211,   224,   225,   202,   227,   228,   205,   120,   -36,    40,
      98,   134,   112,   147,   181,   181,   188,    56,   159,   159,
     126,   126,   126,   126,   230,   -36,   -36,   204,   -36,   -36,
     206,   232,   234,   214,   -36,    -9,   195,   -36,   -36,   208,
     -36,   -36,   209,    -9,   -36,    -9,    -9,    -9,    -9,    -9,
      -9,    -9,    -9,    -9,   -36,   237,   239,   -36,   -36,   215,
     240,   241,   219,   244,   245,   168,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   -36,   -36,   246,   -36,   -36,
     220,   -36,   -36,   -36,   -36,   248,   -36
  };

  /* YYDEFACT[S] -- default reduction number in state S.  Performed when
     YYTABLE doesn't specify something else to do.  Zero means the
     default is an error.  */
  const unsigned char
  fromParser::yydefact_[] =
  {
         0,     0,     0,     2,     5,     0,     0,     0,     4,     1,
       3,     0,     0,     7,     0,     6,     9,     0,     8,    10,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    66,
      65,     0,     0,    35,    36,     0,     0,     0,     0,     0,
       0,     0,     0,    11,     0,     0,     0,    13,     0,     0,
      63,    62,     0,     0,    28,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
      40,    42,    39,    41,    37,    38,    59,    58,    52,    53,
      54,    55,    56,    57,     0,    34,    20,     0,    32,    18,
       0,     0,     0,     0,    12,     0,     0,    29,    15,     0,
      31,    17,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    21,     0,     0,    33,    19,     0,
       0,     0,     0,     0,     0,     0,    51,    46,    45,    50,
      49,    44,    43,    48,    47,    27,    25,     0,    30,    16,
       0,    22,    24,    61,    26,     0,    23
  };

  /* YYPGOTO[NTERM-NUM].  */
  const short int
  fromParser::yypgoto_[] =
  {
       -36,   247,   -36,   -35,   -36,   -36,   -27
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  fromParser::yydefgoto_[] =
  {
        -1,     2,     3,    32,    33,    34,    35
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If YYTABLE_NINF_, syntax error.  */
  const signed char fromParser::yytable_ninf_ = -1;
  const unsigned char
  fromParser::yytable_[] =
  {
        50,    51,    13,    72,    53,    75,     1,    14,    27,    28,
      81,     9,    11,    84,    29,    30,    43,    31,    12,    44,
      45,     4,    46,    24,     5,    87,     6,    15,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   111,    16,     7,     8,    19,   125,    17,
      20,    21,    40,    22,    41,    62,    63,    64,    65,    66,
      67,    68,    69,    36,    47,    37,    25,    26,    48,    18,
      49,    23,    42,    64,    65,    66,    67,    68,    69,    55,
     140,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    38,   145,    39,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   126,    73,   127,
      74,    52,    71,    62,    63,    64,    65,    66,    67,    68,
      69,   130,    54,   131,    76,    70,    77,    62,    63,    64,
      65,    66,    67,    68,    69,    62,    63,    64,    65,    66,
      67,    68,    69,    80,   128,   123,   129,   124,    69,    62,
      63,    64,    65,    66,    67,    68,    69,   132,    78,   133,
      79,   105,    62,    63,    64,    65,    66,    67,    68,    69,
      62,    63,    64,    65,    66,    67,    68,    69,    66,    67,
      68,    69,    88,    62,    63,    64,    65,    66,    67,    68,
      69,    82,   104,    83,   107,   163,    62,    63,    64,    65,
      66,    67,    68,    69,    63,    64,    65,    66,    67,    68,
      69,   114,   106,   108,    85,   115,    86,   116,   112,   141,
     113,   142,   109,   110,   117,   118,   119,   120,   121,   122,
     134,   135,   137,   136,   138,   143,   144,   155,   139,   156,
     158,   159,   157,   160,   161,   162,   164,   165,   166,     0,
      10
  };

  /* YYCHECK.  */
  const short int
  fromParser::yycheck_[] =
  {
        27,    28,     0,    38,    31,    40,    24,     5,    17,    18,
      45,     0,    24,    48,    23,    24,     0,    26,    24,     3,
       4,     0,     6,    24,     3,    52,     5,    25,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    78,     0,    24,    25,     0,     8,     5,
       3,     4,     4,     6,     6,    15,    16,    17,    18,    19,
      20,    21,    22,    24,     0,    26,    24,    24,     4,    25,
       6,    24,    24,    17,    18,    19,    20,    21,    22,     7,
     115,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,     4,   123,     6,   125,   126,
     127,   128,   129,   130,   131,   132,   133,     9,    24,    11,
      26,    26,    24,    15,    16,    17,    18,    19,    20,    21,
      22,     9,     0,    11,    24,     0,    26,    15,    16,    17,
      18,    19,    20,    21,    22,    15,    16,    17,    18,    19,
      20,    21,    22,    24,    10,    25,    12,    27,    22,    15,
      16,    17,    18,    19,    20,    21,    22,    10,     4,    12,
       6,     0,    15,    16,    17,    18,    19,    20,    21,    22,
      15,    16,    17,    18,    19,    20,    21,    22,    19,    20,
      21,    22,    27,    15,    16,    17,    18,    19,    20,    21,
      22,    24,    27,    26,    24,    27,    15,    16,    17,    18,
      19,    20,    21,    22,    16,    17,    18,    19,    20,    21,
      22,     0,     0,     0,    24,     4,    26,     6,    24,    24,
      26,    26,     0,    24,     0,     0,    24,     0,     0,    24,
       0,    27,     0,    27,     0,    27,    27,     0,    24,     0,
       0,     0,    27,    24,     0,     0,     0,    27,     0,    -1,
       3
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  fromParser::yystos_[] =
  {
         0,    24,    29,    30,     0,     3,     5,    24,    25,     0,
      29,    24,    24,     0,     5,    25,     0,     5,    25,     0,
       3,     4,     6,    24,    24,    24,    24,    17,    18,    23,
      24,    26,    31,    32,    33,    34,    24,    26,     4,     6,
       4,     6,    24,     0,     3,     4,     6,     0,     4,     6,
      34,    34,    26,    34,     0,     7,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
       0,    24,    31,    24,    26,    31,    24,    26,     4,     6,
      24,    31,    24,    26,    31,    24,    26,    34,    27,    34,
      34,    34,    34,    34,    34,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    27,     0,     0,    24,     0,     0,
      24,    31,    24,    26,     0,     4,     6,     0,     0,    24,
       0,     0,    24,    25,    27,     8,     9,    11,    10,    12,
       9,    11,    10,    12,     0,    27,    27,     0,     0,    24,
      31,    24,    26,    27,    27,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,     0,     0,    27,     0,     0,
      24,     0,     0,    27,     0,    27,     0
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  fromParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,    44,    40,    41
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  fromParser::yyr1_[] =
  {
         0,    28,    29,    29,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    31,    31,    32,    32,    32,
      32,    32,    32,    33,    33,    33,    33,    33,    33,    33,
      33,    33,    34,    34,    34,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,    34,    34
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  fromParser::yyr2_[] =
  {
         0,     2,     1,     2,     2,     2,     3,     3,     4,     4,
       4,     6,     8,     6,     6,     8,    10,     8,     7,     8,
       7,     8,    10,    12,    10,     9,    10,     9,     6,     8,
      10,     8,     7,     8,     7,     1,     1,     3,     3,     3,
       3,     3,     3,     5,     5,     5,     5,     5,     5,     5,
       5,     5,     3,     3,     3,     3,     3,     3,     3,     3,
       4,     6,     2,     2,     3,     1,     1
  };

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const fromParser::yytname_[] =
  {
    "\"end of input\"", "error", "$undefined", "\"as\"", "\"on\"",
  "\"join\"", "\"using\"", "\"between\"", "\"and\"", "\"<=\"", "\">=\"",
  "\"<\"", "\">\"", "\"==\"", "\"!=\"", "\"|\"", "\"&\"", "\"+\"", "\"-\"",
  "\"*\"", "\"/\"", "\"%\"", "\"**\"", "\"numerical value\"", "\"name\"",
  "','", "'('", "')'", "$accept", "flist", "fterm", "compRange",
  "compRange2", "compRange3", "mathExpr", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const fromParser::rhs_number_type
  fromParser::yyrhs_[] =
  {
        29,     0,    -1,    30,    -1,    30,    29,    -1,    24,    25,
      -1,    24,     0,    -1,    24,    24,    25,    -1,    24,    24,
       0,    -1,    24,     3,    24,    25,    -1,    24,     3,    24,
       0,    -1,    24,     5,    24,     0,    -1,    24,     3,    24,
       5,    24,     0,    -1,    24,     3,    24,     5,    24,     3,
      24,     0,    -1,    24,     5,    24,     3,    24,     0,    -1,
      24,     5,    24,     6,    24,     0,    -1,    24,     3,    24,
       5,    24,     6,    24,     0,    -1,    24,     3,    24,     5,
      24,     3,    24,     6,    24,     0,    -1,    24,     5,    24,
       3,    24,     6,    24,     0,    -1,    24,    24,     5,    24,
       6,    24,     0,    -1,    24,    24,     5,    24,    24,     6,
      24,     0,    -1,    24,     5,    24,    24,     6,    24,     0,
      -1,    24,     5,    24,     6,    26,    24,    27,     0,    -1,
      24,     3,    24,     5,    24,     6,    26,    24,    27,     0,
      -1,    24,     3,    24,     5,    24,     3,    24,     6,    26,
      24,    27,     0,    -1,    24,     5,    24,     3,    24,     6,
      26,    24,    27,     0,    -1,    24,    24,     5,    24,     6,
      26,    24,    27,     0,    -1,    24,    24,     5,    24,    24,
       6,    26,    24,    27,     0,    -1,    24,     5,    24,    24,
       6,    26,    24,    27,     0,    -1,    24,     5,    24,     4,
      31,     0,    -1,    24,     3,    24,     5,    24,     4,    31,
       0,    -1,    24,     3,    24,     5,    24,     3,    24,     4,
      31,     0,    -1,    24,     5,    24,     3,    24,     4,    31,
       0,    -1,    24,    24,     5,    24,     4,    31,     0,    -1,
      24,    24,     5,    24,    24,     4,    31,     0,    -1,    24,
       5,    24,    24,     4,    31,     0,    -1,    32,    -1,    33,
      -1,    34,    13,    34,    -1,    34,    14,    34,    -1,    34,
      11,    34,    -1,    34,     9,    34,    -1,    34,    12,    34,
      -1,    34,    10,    34,    -1,    34,    11,    34,    11,    34,
      -1,    34,    11,    34,     9,    34,    -1,    34,     9,    34,
      11,    34,    -1,    34,     9,    34,     9,    34,    -1,    34,
      12,    34,    12,    34,    -1,    34,    12,    34,    10,    34,
      -1,    34,    10,    34,    12,    34,    -1,    34,    10,    34,
      10,    34,    -1,    34,     7,    34,     8,    34,    -1,    34,
      17,    34,    -1,    34,    18,    34,    -1,    34,    19,    34,
      -1,    34,    20,    34,    -1,    34,    21,    34,    -1,    34,
      22,    34,    -1,    34,    16,    34,    -1,    34,    15,    34,
      -1,    24,    26,    34,    27,    -1,    24,    26,    34,    25,
      34,    27,    -1,    18,    34,    -1,    17,    34,    -1,    26,
      34,    27,    -1,    24,    -1,    23,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  fromParser::yyprhs_[] =
  {
         0,     0,     3,     5,     8,    11,    14,    18,    22,    27,
      32,    37,    44,    53,    60,    67,    76,    87,    96,   104,
     113,   121,   130,   141,   154,   165,   175,   186,   196,   203,
     212,   223,   232,   240,   249,   257,   259,   261,   265,   269,
     273,   277,   281,   285,   291,   297,   303,   309,   315,   321,
     327,   333,   339,   343,   347,   351,   355,   359,   363,   367,
     371,   376,   383,   386,   389,   393,   395
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  fromParser::yyrline_[] =
  {
         0,    88,    88,    88,    89,    93,    97,   101,   104,   108,
     112,   118,   124,   130,   136,   151,   166,   181,   196,   211,
     226,   241,   256,   271,   286,   301,   316,   331,   346,   353,
     360,   367,   374,   381,   388,   397,   397,   399,   409,   420,
     430,   440,   450,   463,   475,   487,   499,   511,   523,   535,
     547,   559,   574,   586,   598,   610,   622,   634,   646,   658,
     670,   682,   695,   705,   708,   711,   719
  };

  // Print the state stack on the debug stream.
  void
  fromParser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (state_stack_type::const_iterator i = yystate_stack_.begin ();
	 i != yystate_stack_.end (); ++i)
      *yycdebug_ << ' ' << *i;
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  fromParser::yy_reduce_print_ (int yyrule)
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
  fromParser::token_number_type
  fromParser::yytranslate_ (int t)
  {
    static
    const token_number_type
    translate_table[] =
    {
           0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      26,    27,     2,     2,    25,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int fromParser::yyeof_ = 0;
  const int fromParser::yylast_ = 250;
  const int fromParser::yynnts_ = 7;
  const int fromParser::yyempty_ = -2;
  const int fromParser::yyfinal_ = 9;
  const int fromParser::yyterror_ = 1;
  const int fromParser::yyerrcode_ = 256;
  const int fromParser::yyntokens_ = 28;

  const unsigned int fromParser::yyuser_token_number_max_ = 279;
  const fromParser::token_number_type fromParser::yyundef_token_ = 2;


} // ibis

/* Line 1136 of lalr1.cc  */
#line 2142 "fromParser.cc"


/* Line 1138 of lalr1.cc  */
#line 728 "fromParser.yy"

void ibis::fromParser::error(const ibis::fromParser::location_type& l,
			     const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::fromParser encountered " << m << " at location "
	<< l;
}


/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++
   
      Copyright (C) 2002-2010 Free Software Foundation, Inc.
   
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

/* Line 303 of lalr1.cc  */
#line 6 "fromParser.yy"

/** \file Defines the parser for the from clause accepted by FastBit
    IBIS.  The definitions are processed through bison.
*/
#include <iostream>



/* Line 303 of lalr1.cc  */
#line 46 "fromParser.cc"

// Take the name prefix into account.
#define yylex   ibislex

/* First part of user declarations.  */


/* Line 310 of lalr1.cc  */
#line 55 "fromParser.cc"


#include "fromParser.hh"

/* User implementation prologue.  */

/* Line 316 of lalr1.cc  */
#line 80 "fromParser.yy"

#include "fromLexer.h"

#undef yylex
#define yylex driver.lexer->lex


/* Line 316 of lalr1.cc  */
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

/* Line 379 of lalr1.cc  */
#line 138 "fromParser.cc"
#if YYERROR_VERBOSE

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

#endif

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

/* Line 479 of lalr1.cc  */
#line 77 "fromParser.yy"
	{ delete (yyvaluep->stringVal); };

/* Line 479 of lalr1.cc  */
#line 245 "fromParser.cc"
	break;
      case 32: /* "compRange2" */

/* Line 479 of lalr1.cc  */
#line 78 "fromParser.yy"
	{ delete (yyvaluep->fromNode); };

/* Line 479 of lalr1.cc  */
#line 254 "fromParser.cc"
	break;
      case 33: /* "compRange3" */

/* Line 479 of lalr1.cc  */
#line 78 "fromParser.yy"
	{ delete (yyvaluep->fromNode); };

/* Line 479 of lalr1.cc  */
#line 263 "fromParser.cc"
	break;
      case 34: /* "mathExpr" */

/* Line 479 of lalr1.cc  */
#line 78 "fromParser.yy"
	{ delete (yyvaluep->fromNode); };

/* Line 479 of lalr1.cc  */
#line 272 "fromParser.cc"
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
    location_type yyerror_range[2];

    /// $$.
    semantic_type yyval;
    /// @$.
    location_type yyloc;

    int yyresult;

    YYCDEBUG << "Starting parse" << std::endl;


    /* User initialization code.  */
    
/* Line 552 of lalr1.cc  */
#line 28 "fromParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}

/* Line 552 of lalr1.cc  */
#line 357 "fromParser.cc"

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
    if (yyn == yypact_ninf_)
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
	if (yyn == 0 || yyn == yytable_ninf_)
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

/* Line 677 of lalr1.cc  */
#line 89 "fromParser.yy"
    {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*(yysemantic_stack_[(2) - (1)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 5:

/* Line 677 of lalr1.cc  */
#line 94 "fromParser.yy"
    {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*(yysemantic_stack_[(2) - (1)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 6:

/* Line 677 of lalr1.cc  */
#line 99 "fromParser.yy"
    {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*(yysemantic_stack_[(3) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(3) - (2)].stringVal));
}
    break;

  case 7:

/* Line 677 of lalr1.cc  */
#line 104 "fromParser.yy"
    {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*(yysemantic_stack_[(3) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(3) - (2)].stringVal));
}
    break;

  case 8:

/* Line 677 of lalr1.cc  */
#line 108 "fromParser.yy"
    {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*(yysemantic_stack_[(4) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(4) - (3)].stringVal));
}
    break;

  case 9:

/* Line 677 of lalr1.cc  */
#line 113 "fromParser.yy"
    {
    const size_t pos = driver.names_.size();
    driver.names_.push_back(*(yysemantic_stack_[(4) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(4) - (3)].stringVal));
}
    break;

  case 10:

/* Line 677 of lalr1.cc  */
#line 118 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(4) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(4) - (3)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 11:

/* Line 677 of lalr1.cc  */
#line 124 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(6) - (5)].stringVal));
    driver.aliases_.push_back("");
}
    break;

  case 12:

/* Line 677 of lalr1.cc  */
#line 130 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (7)].stringVal));
}
    break;

  case 13:

/* Line 677 of lalr1.cc  */
#line 136 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(6) - (5)].stringVal));
}
    break;

  case 14:

/* Line 677 of lalr1.cc  */
#line 142 "fromParser.yy"
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

/* Line 677 of lalr1.cc  */
#line 157 "fromParser.yy"
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

/* Line 677 of lalr1.cc  */
#line 172 "fromParser.yy"
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

/* Line 677 of lalr1.cc  */
#line 187 "fromParser.yy"
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

/* Line 677 of lalr1.cc  */
#line 202 "fromParser.yy"
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

  case 19:

/* Line 677 of lalr1.cc  */
#line 217 "fromParser.yy"
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

  case 20:

/* Line 677 of lalr1.cc  */
#line 232 "fromParser.yy"
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

  case 21:

/* Line 677 of lalr1.cc  */
#line 247 "fromParser.yy"
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

  case 22:

/* Line 677 of lalr1.cc  */
#line 262 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(6) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(6) - (3)].stringVal));
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(6) - (5)].fromNode));
}
    break;

  case 23:

/* Line 677 of lalr1.cc  */
#line 269 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.aliases_.push_back("");
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(8) - (7)].fromNode));
}
    break;

  case 24:

/* Line 677 of lalr1.cc  */
#line 276 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(10) - (1)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (3)].stringVal));
    driver.names_.push_back(*(yysemantic_stack_[(10) - (5)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(10) - (7)].stringVal));
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(10) - (9)].fromNode));
}
    break;

  case 25:

/* Line 677 of lalr1.cc  */
#line 283 "fromParser.yy"
    {
    driver.names_.push_back(*(yysemantic_stack_[(8) - (1)].stringVal));
    driver.aliases_.push_back("");
    driver.names_.push_back(*(yysemantic_stack_[(8) - (3)].stringVal));
    driver.aliases_.push_back(*(yysemantic_stack_[(8) - (5)].stringVal));
    driver.jcond_ = dynamic_cast<ibis::compRange*>((yysemantic_stack_[(8) - (7)].fromNode));
}
    break;

  case 28:

/* Line 677 of lalr1.cc  */
#line 294 "fromParser.yy"
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

  case 29:

/* Line 677 of lalr1.cc  */
#line 304 "fromParser.yy"
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

  case 30:

/* Line 677 of lalr1.cc  */
#line 315 "fromParser.yy"
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

  case 31:

/* Line 677 of lalr1.cc  */
#line 325 "fromParser.yy"
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

  case 32:

/* Line 677 of lalr1.cc  */
#line 335 "fromParser.yy"
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

  case 33:

/* Line 677 of lalr1.cc  */
#line 345 "fromParser.yy"
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

  case 34:

/* Line 677 of lalr1.cc  */
#line 358 "fromParser.yy"
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

  case 35:

/* Line 677 of lalr1.cc  */
#line 370 "fromParser.yy"
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

  case 36:

/* Line 677 of lalr1.cc  */
#line 382 "fromParser.yy"
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

  case 37:

/* Line 677 of lalr1.cc  */
#line 394 "fromParser.yy"
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

  case 38:

/* Line 677 of lalr1.cc  */
#line 406 "fromParser.yy"
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

  case 39:

/* Line 677 of lalr1.cc  */
#line 418 "fromParser.yy"
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

  case 40:

/* Line 677 of lalr1.cc  */
#line 430 "fromParser.yy"
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

  case 41:

/* Line 677 of lalr1.cc  */
#line 442 "fromParser.yy"
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

  case 42:

/* Line 677 of lalr1.cc  */
#line 454 "fromParser.yy"
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

  case 43:

/* Line 677 of lalr1.cc  */
#line 469 "fromParser.yy"
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

  case 44:

/* Line 677 of lalr1.cc  */
#line 481 "fromParser.yy"
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

  case 45:

/* Line 677 of lalr1.cc  */
#line 493 "fromParser.yy"
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

  case 46:

/* Line 677 of lalr1.cc  */
#line 505 "fromParser.yy"
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

  case 47:

/* Line 677 of lalr1.cc  */
#line 517 "fromParser.yy"
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

  case 48:

/* Line 677 of lalr1.cc  */
#line 529 "fromParser.yy"
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

  case 49:

/* Line 677 of lalr1.cc  */
#line 541 "fromParser.yy"
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

  case 50:

/* Line 677 of lalr1.cc  */
#line 553 "fromParser.yy"
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

  case 51:

/* Line 677 of lalr1.cc  */
#line 565 "fromParser.yy"
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

  case 52:

/* Line 677 of lalr1.cc  */
#line 577 "fromParser.yy"
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

  case 53:

/* Line 677 of lalr1.cc  */
#line 590 "fromParser.yy"
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

  case 54:

/* Line 677 of lalr1.cc  */
#line 600 "fromParser.yy"
    {
    (yyval.fromNode) = (yysemantic_stack_[(2) - (2)].fromNode);
}
    break;

  case 55:

/* Line 677 of lalr1.cc  */
#line 603 "fromParser.yy"
    {
    (yyval.fromNode) = (yysemantic_stack_[(3) - (2)].fromNode);
}
    break;

  case 56:

/* Line 677 of lalr1.cc  */
#line 606 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.fromNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
}
    break;

  case 57:

/* Line 677 of lalr1.cc  */
#line 614 "fromParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.fromNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
}
    break;



/* Line 677 of lalr1.cc  */
#line 1311 "fromParser.cc"
	default:
          break;
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

    yyerror_range[1] = yylloc;
    // Using YYLLOC is tempting, but would change the location of
    // the lookahead.  YYLOC is available though.
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
    if (yychar != yyempty_)
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
  fromParser::yysyntax_error_ (int yystate, int tok)
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
  const signed char fromParser::yypact_ninf_ = -36;
  const short int
  fromParser::yypact_[] =
  {
       -21,    14,    22,   -21,   -36,    20,    29,    15,   -36,   -36,
     -36,    16,   191,   -36,   -36,   -36,    80,   -36,   -36,    94,
      25,    21,   196,    46,    25,    25,   -36,    49,    25,   101,
     -36,   -36,    47,   115,   112,   -36,   117,    25,    79,   -36,
      25,    93,   129,   129,    25,    -9,   -36,    25,    25,    25,
      25,    25,    25,    25,    25,    25,    25,    25,    25,    25,
      25,    25,   -36,   126,   201,   154,   172,   150,   192,   193,
     174,   113,   -36,    77,    91,   127,   105,   140,   161,   161,
     168,    69,    51,    51,   129,   129,   129,   129,   203,   -36,
      25,   147,   -36,   -36,   177,   -36,   -36,   179,    25,   -36,
      25,    25,    25,    25,    25,    25,    25,    25,    25,   -36,
     208,   209,   186,   211,   212,   148,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   -36,   -36,   187,   -36,   -36,
     -36,   213,   -36
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  fromParser::yydefact_[] =
  {
         0,     0,     0,     2,     5,     0,     0,     0,     4,     1,
       3,     0,     0,     7,     6,     9,     0,     8,    10,     0,
       0,     0,     0,     0,     0,     0,    57,    56,     0,     0,
      26,    27,     0,     0,     0,    11,     0,     0,     0,    13,
       0,     0,    54,    53,     0,     0,    22,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,     0,    31,    33,    30,    32,    28,    29,
      50,    49,    43,    44,    45,    46,    47,    48,     0,    12,
       0,     0,    23,    15,     0,    25,    17,     0,     0,    51,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     0,    42,    37,    36,    41,
      40,    35,    34,    39,    38,    24,    16,     0,    19,    21,
      52,     0,    20
  };

  /* YYPGOTO[NTERM-NUM].  */
  const short int
  fromParser::yypgoto_[] =
  {
       -36,   214,   -36,   -35,   -36,   -36,   -24
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  fromParser::yydefgoto_[] =
  {
        -1,     2,     3,    29,    30,    31,    32
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If zero, do what YYDEFACT says.  */
  const signed char fromParser::yytable_ninf_ = -1;
  const unsigned char
  fromParser::yytable_[] =
  {
        42,    43,    65,     1,    45,    68,    54,    55,    56,    57,
      58,    59,    60,    61,     4,    13,    15,     5,    72,     6,
      71,    16,     9,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     7,     8,
      14,    17,    24,    25,    11,    33,    39,    34,    26,    27,
      40,    28,    41,    12,    47,   110,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      58,    59,    60,    61,   115,    44,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   100,    56,    57,    58,    59,
      60,    61,    54,    55,    56,    57,    58,    59,    60,    61,
     101,    46,   102,    66,    22,    67,    54,    55,    56,    57,
      58,    59,    60,    61,   105,    62,   106,    69,    23,    70,
      54,    55,    56,    57,    58,    59,    60,    61,    54,    55,
      56,    57,    58,    59,    60,    61,    63,   103,    98,   104,
      99,    64,    54,    55,    56,    57,    58,    59,    60,    61,
     107,    61,   108,    88,    92,    54,    55,    56,    57,    58,
      59,    60,    61,    54,    55,    56,    57,    58,    59,    60,
      61,   111,    93,   112,    94,   130,    54,    55,    56,    57,
      58,    59,    60,    61,    55,    56,    57,    58,    59,    60,
      61,    18,    95,    96,    19,    20,    35,    21,    97,    36,
      37,    89,    38,   109,   113,    90,   114,    91,   125,   126,
     127,   128,   129,   132,   131,     0,     0,    10
  };

  /* YYCHECK.  */
  const signed char
  fromParser::yycheck_[] =
  {
        24,    25,    37,    24,    28,    40,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     3,    27,     5,
      44,     5,     0,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    24,    25,
      25,    25,    17,    18,    24,    24,     0,    26,    23,    24,
       4,    26,     6,    24,     7,    90,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      19,    20,    21,    22,    98,    26,   100,   101,   102,   103,
     104,   105,   106,   107,   108,     8,    17,    18,    19,    20,
      21,    22,    15,    16,    17,    18,    19,    20,    21,    22,
       9,     0,    11,    24,    24,    26,    15,    16,    17,    18,
      19,    20,    21,    22,     9,     0,    11,    24,    24,    26,
      15,    16,    17,    18,    19,    20,    21,    22,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    10,    25,    12,
      27,    24,    15,    16,    17,    18,    19,    20,    21,    22,
      10,    22,    12,    27,     0,    15,    16,    17,    18,    19,
      20,    21,    22,    15,    16,    17,    18,    19,    20,    21,
      22,    24,     0,    26,    24,    27,    15,    16,    17,    18,
      19,    20,    21,    22,    16,    17,    18,    19,    20,    21,
      22,     0,     0,     0,     3,     4,     0,     6,    24,     3,
       4,     0,     6,     0,    27,     4,    27,     6,     0,     0,
      24,     0,     0,     0,    27,    -1,    -1,     3
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  fromParser::yystos_[] =
  {
         0,    24,    29,    30,     0,     3,     5,    24,    25,     0,
      29,    24,    24,     0,    25,     0,     5,    25,     0,     3,
       4,     6,    24,    24,    17,    18,    23,    24,    26,    31,
      32,    33,    34,    24,    26,     0,     3,     4,     6,     0,
       4,     6,    34,    34,    26,    34,     0,     7,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,     0,    24,    24,    31,    24,    26,    31,    24,
      26,    34,    27,    34,    34,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,    34,    34,    34,    27,     0,
       4,     6,     0,     0,    24,     0,     0,    24,    25,    27,
       8,     9,    11,    10,    12,     9,    11,    10,    12,     0,
      31,    24,    26,    27,    27,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,     0,     0,    24,     0,     0,
      27,    27,     0
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
      30,    30,    30,    30,    30,    30,    31,    31,    32,    32,
      32,    32,    32,    32,    33,    33,    33,    33,    33,    33,
      33,    33,    33,    34,    34,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,    34,    34,    34
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  fromParser::yyr2_[] =
  {
         0,     2,     1,     2,     2,     2,     3,     3,     4,     4,
       4,     6,     8,     6,     6,     8,    10,     8,     8,    10,
      12,    10,     6,     8,    10,     8,     1,     1,     3,     3,
       3,     3,     3,     3,     5,     5,     5,     5,     5,     5,
       5,     5,     5,     3,     3,     3,     3,     3,     3,     3,
       3,     4,     6,     2,     2,     3,     1,     1
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
       3,    24,     6,    24,     0,    -1,    24,     5,    24,     6,
      26,    24,    27,     0,    -1,    24,     3,    24,     5,    24,
       6,    26,    24,    27,     0,    -1,    24,     3,    24,     5,
      24,     3,    24,     6,    26,    24,    27,     0,    -1,    24,
       5,    24,     3,    24,     6,    26,    24,    27,     0,    -1,
      24,     5,    24,     4,    31,     0,    -1,    24,     3,    24,
       5,    24,     4,    31,     0,    -1,    24,     3,    24,     5,
      24,     3,    24,     4,    31,     0,    -1,    24,     5,    24,
       3,    24,     4,    31,     0,    -1,    32,    -1,    33,    -1,
      34,    13,    34,    -1,    34,    14,    34,    -1,    34,    11,
      34,    -1,    34,     9,    34,    -1,    34,    12,    34,    -1,
      34,    10,    34,    -1,    34,    11,    34,    11,    34,    -1,
      34,    11,    34,     9,    34,    -1,    34,     9,    34,    11,
      34,    -1,    34,     9,    34,     9,    34,    -1,    34,    12,
      34,    12,    34,    -1,    34,    12,    34,    10,    34,    -1,
      34,    10,    34,    12,    34,    -1,    34,    10,    34,    10,
      34,    -1,    34,     7,    34,     8,    34,    -1,    34,    17,
      34,    -1,    34,    18,    34,    -1,    34,    19,    34,    -1,
      34,    20,    34,    -1,    34,    21,    34,    -1,    34,    22,
      34,    -1,    34,    16,    34,    -1,    34,    15,    34,    -1,
      24,    26,    34,    27,    -1,    24,    26,    34,    25,    34,
      27,    -1,    18,    34,    -1,    17,    34,    -1,    26,    34,
      27,    -1,    24,    -1,    23,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  fromParser::yyprhs_[] =
  {
         0,     0,     3,     5,     8,    11,    14,    18,    22,    27,
      32,    37,    44,    53,    60,    67,    76,    87,    96,   105,
     116,   129,   140,   147,   156,   167,   176,   178,   180,   184,
     188,   192,   196,   200,   204,   210,   216,   222,   228,   234,
     240,   246,   252,   258,   262,   266,   270,   274,   278,   282,
     286,   290,   295,   302,   305,   308,   312,   314
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  fromParser::yyrline_[] =
  {
         0,    88,    88,    88,    89,    94,    99,   104,   108,   113,
     118,   124,   130,   136,   142,   157,   172,   187,   202,   217,
     232,   247,   262,   269,   276,   283,   292,   292,   294,   304,
     315,   325,   335,   345,   358,   370,   382,   394,   406,   418,
     430,   442,   454,   469,   481,   493,   505,   517,   529,   541,
     553,   565,   577,   590,   600,   603,   606,   614
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
  const int fromParser::yylast_ = 217;
  const int fromParser::yynnts_ = 7;
  const int fromParser::yyempty_ = -2;
  const int fromParser::yyfinal_ = 9;
  const int fromParser::yyterror_ = 1;
  const int fromParser::yyerrcode_ = 256;
  const int fromParser::yyntokens_ = 28;

  const unsigned int fromParser::yyuser_token_number_max_ = 279;
  const fromParser::token_number_type fromParser::yyundef_token_ = 2;


} // ibis

/* Line 1053 of lalr1.cc  */
#line 1855 "fromParser.cc"


/* Line 1055 of lalr1.cc  */
#line 623 "fromParser.yy"

void ibis::fromParser::error(const ibis::fromParser::location_type& l,
			     const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::fromParser encountered " << m << " at location " << l;
}


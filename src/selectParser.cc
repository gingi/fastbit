/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++
   
      Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Free
   Software Foundation, Inc.
   
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

/* Line 304 of lalr1.cc  */
#line 6 "selectParser.yy"

/** \file Defines the parser for the select clause accepted by FastBit
    IBIS.  The definitions are processed through bison.
*/
#include <iostream>



/* Line 304 of lalr1.cc  */
#line 47 "selectParser.cc"

// Take the name prefix into account.
#define yylex   ibislex

/* First part of user declarations.  */


/* Line 311 of lalr1.cc  */
#line 56 "selectParser.cc"


#include "selectParser.hh"

/* User implementation prologue.  */

/* Line 317 of lalr1.cc  */
#line 77 "selectParser.yy"

#include "selectLexer.h"

#undef yylex
#define yylex driver.lexer->lex


/* Line 317 of lalr1.cc  */
#line 73 "selectParser.cc"

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

/* Line 380 of lalr1.cc  */
#line 139 "selectParser.cc"
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
#endif

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
        case 24: /* "\"name\"" */

/* Line 480 of lalr1.cc  */
#line 74 "selectParser.yy"
	{ delete (yyvaluep->stringVal); };

/* Line 480 of lalr1.cc  */
#line 246 "selectParser.cc"
	break;
      case 31: /* "mathExpr" */

/* Line 480 of lalr1.cc  */
#line 75 "selectParser.yy"
	{ delete (yyvaluep->selectNode); };

/* Line 480 of lalr1.cc  */
#line 255 "selectParser.cc"
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

  int
  selectParser::parse ()
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
    
/* Line 559 of lalr1.cc  */
#line 28 "selectParser.yy"
{ // initialize location object
    yylloc.begin.filename = yylloc.end.filename = &(driver.clause_);
}

/* Line 559 of lalr1.cc  */
#line 340 "selectParser.cc"

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

/* Line 678 of lalr1.cc  */
#line 86 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 5:

/* Line 678 of lalr1.cc  */
#line 90 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 6:

/* Line 678 of lalr1.cc  */
#line 94 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 7:

/* Line 678 of lalr1.cc  */
#line 98 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 8:

/* Line 678 of lalr1.cc  */
#line 102 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 9:

/* Line 678 of lalr1.cc  */
#line 106 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 10:

/* Line 678 of lalr1.cc  */
#line 110 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 11:

/* Line 678 of lalr1.cc  */
#line 114 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 12:

/* Line 678 of lalr1.cc  */
#line 118 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 13:

/* Line 678 of lalr1.cc  */
#line 122 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 14:

/* Line 678 of lalr1.cc  */
#line 126 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 15:

/* Line 678 of lalr1.cc  */
#line 130 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 16:

/* Line 678 of lalr1.cc  */
#line 134 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 17:

/* Line 678 of lalr1.cc  */
#line 138 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 18:

/* Line 678 of lalr1.cc  */
#line 142 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 19:

/* Line 678 of lalr1.cc  */
#line 146 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 20:

/* Line 678 of lalr1.cc  */
#line 150 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 21:

/* Line 678 of lalr1.cc  */
#line 154 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 22:

/* Line 678 of lalr1.cc  */
#line 158 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 23:

/* Line 678 of lalr1.cc  */
#line 162 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 24:

/* Line 678 of lalr1.cc  */
#line 166 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 25:

/* Line 678 of lalr1.cc  */
#line 170 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 26:

/* Line 678 of lalr1.cc  */
#line 174 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 27:

/* Line 678 of lalr1.cc  */
#line 178 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 28:

/* Line 678 of lalr1.cc  */
#line 182 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 29:

/* Line 678 of lalr1.cc  */
#line 186 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 30:

/* Line 678 of lalr1.cc  */
#line 190 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 31:

/* Line 678 of lalr1.cc  */
#line 195 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 32:

/* Line 678 of lalr1.cc  */
#line 200 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 33:

/* Line 678 of lalr1.cc  */
#line 205 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 34:

/* Line 678 of lalr1.cc  */
#line 210 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 35:

/* Line 678 of lalr1.cc  */
#line 215 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 36:

/* Line 678 of lalr1.cc  */
#line 220 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 37:

/* Line 678 of lalr1.cc  */
#line 225 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 38:

/* Line 678 of lalr1.cc  */
#line 230 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 39:

/* Line 678 of lalr1.cc  */
#line 235 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 40:

/* Line 678 of lalr1.cc  */
#line 240 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 41:

/* Line 678 of lalr1.cc  */
#line 245 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 42:

/* Line 678 of lalr1.cc  */
#line 250 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 43:

/* Line 678 of lalr1.cc  */
#line 255 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 44:

/* Line 678 of lalr1.cc  */
#line 260 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 45:

/* Line 678 of lalr1.cc  */
#line 265 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 46:

/* Line 678 of lalr1.cc  */
#line 270 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 47:

/* Line 678 of lalr1.cc  */
#line 275 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 48:

/* Line 678 of lalr1.cc  */
#line 283 "selectParser.yy"
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

  case 49:

/* Line 678 of lalr1.cc  */
#line 295 "selectParser.yy"
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

  case 50:

/* Line 678 of lalr1.cc  */
#line 307 "selectParser.yy"
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

  case 51:

/* Line 678 of lalr1.cc  */
#line 319 "selectParser.yy"
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

  case 52:

/* Line 678 of lalr1.cc  */
#line 331 "selectParser.yy"
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

  case 53:

/* Line 678 of lalr1.cc  */
#line 343 "selectParser.yy"
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

  case 54:

/* Line 678 of lalr1.cc  */
#line 355 "selectParser.yy"
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

  case 55:

/* Line 678 of lalr1.cc  */
#line 367 "selectParser.yy"
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

  case 56:

/* Line 678 of lalr1.cc  */
#line 379 "selectParser.yy"
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
}
    break;

  case 57:

/* Line 678 of lalr1.cc  */
#line 391 "selectParser.yy"
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
}
    break;

  case 58:

/* Line 678 of lalr1.cc  */
#line 404 "selectParser.yy"
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

  case 59:

/* Line 678 of lalr1.cc  */
#line 414 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(2) - (2)].selectNode);
}
    break;

  case 60:

/* Line 678 of lalr1.cc  */
#line 417 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(3) - (2)].selectNode);
}
    break;

  case 61:

/* Line 678 of lalr1.cc  */
#line 420 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.selectNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
}
    break;

  case 62:

/* Line 678 of lalr1.cc  */
#line 428 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.selectNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
}
    break;



/* Line 678 of lalr1.cc  */
#line 1159 "selectParser.cc"
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
  const signed char selectParser::yypact_ninf_ = -24;
  const short int
  selectParser::yypact_[] =
  {
       102,   -23,   -22,   -20,   -19,   -18,    -1,     0,     1,    59,
      72,    77,   280,   280,   -24,    79,   280,    45,   102,    20,
     280,   276,   280,   280,   280,   280,   280,   280,   280,   280,
     280,    57,    57,   280,   126,   -24,   -24,   -24,    75,   280,
     280,   280,   280,   280,   280,   280,   280,   -24,   138,    91,
     150,   162,   174,   186,   198,   210,   222,   234,   246,   258,
     113,   -24,    43,   290,   296,   300,   300,    57,    57,    57,
      57,    19,    48,    49,    50,    54,    55,    44,    61,    62,
      64,    56,    60,   -24,   280,   -24,   -24,   -24,    94,   -24,
     -24,    97,   -24,   -24,    98,   -24,   -24,    99,   -24,   -24,
     100,   -24,   -24,   112,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   114,   -24,   -24,   125,   -24,   270,
      65,    66,    67,    68,    69,    73,    74,    78,   -24,   -24,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   -24
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  selectParser::yydefact_[] =
  {
         0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,    61,     0,     0,     2,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    58,     0,     0,     1,     3,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    28,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    60,     0,    55,    54,    48,    49,    50,    51,    52,
      53,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    56,     0,    43,    42,     5,     0,     4,
       9,     0,     8,     7,     0,     6,    13,     0,    12,    11,
       0,    10,    15,     0,    14,    17,    16,    19,    18,    21,
      20,    23,    22,    25,     0,    24,    27,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,    31,
      30,    35,    34,    33,    32,    39,    38,    37,    36,    41,
      40,    45,    44,    47,    46
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yypgoto_[] =
  {
       -24,    80,   -24,   -12
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  selectParser::yydefgoto_[] =
  {
        -1,    17,    18,    19
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If zero, do what YYDEFACT says.  */
  const signed char selectParser::yytable_ninf_ = -1;
  const unsigned char
  selectParser::yytable_[] =
  {
        31,    32,    20,    21,    34,    22,    23,    24,    48,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    87,
      37,    60,    88,    38,    25,    26,    27,    63,    64,    65,
      66,    67,    68,    69,    70,    39,    40,    41,    42,    43,
      44,    45,    46,    85,   105,    35,    89,    47,    90,    93,
      96,    91,    94,    97,    99,   102,   113,   100,   103,   114,
     116,   107,   109,   117,   111,   129,   131,   133,   135,   137,
      86,   106,   119,   139,   141,    92,    95,    98,   143,    46,
       0,   101,   104,   115,    28,     0,     0,   118,   108,   110,
       0,   112,   130,   132,   134,   136,   138,    29,    36,    62,
     140,   142,    30,     0,    33,   144,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    72,   120,    12,
      13,   121,   122,   123,   124,    14,    15,    16,    39,    40,
      41,    42,    43,    44,    45,    46,   125,     0,   126,    83,
      84,    39,    40,    41,    42,    43,    44,    45,    46,   127,
       0,     0,    61,    39,    40,    41,    42,    43,    44,    45,
      46,     0,     0,     0,    71,    39,    40,    41,    42,    43,
      44,    45,    46,     0,     0,     0,    73,    39,    40,    41,
      42,    43,    44,    45,    46,     0,     0,     0,    74,    39,
      40,    41,    42,    43,    44,    45,    46,     0,     0,     0,
      75,    39,    40,    41,    42,    43,    44,    45,    46,     0,
       0,     0,    76,    39,    40,    41,    42,    43,    44,    45,
      46,     0,     0,     0,    77,    39,    40,    41,    42,    43,
      44,    45,    46,     0,     0,     0,    78,    39,    40,    41,
      42,    43,    44,    45,    46,     0,     0,     0,    79,    39,
      40,    41,    42,    43,    44,    45,    46,     0,     0,     0,
      80,    39,    40,    41,    42,    43,    44,    45,    46,     0,
       0,     0,    81,    39,    40,    41,    42,    43,    44,    45,
      46,     0,     0,     0,    82,    39,    40,    41,    42,    43,
      44,    45,    46,    12,    13,    49,   128,    12,    13,    14,
      15,    16,     0,    14,    15,    16,    40,    41,    42,    43,
      44,    45,    46,    41,    42,    43,    44,    45,    46,    43,
      44,    45,    46
  };

  /* YYCHECK.  */
  const signed char
  selectParser::yycheck_[] =
  {
        12,    13,    25,    25,    16,    25,    25,    25,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,     0,
       0,    33,     3,     3,    25,    25,    25,    39,    40,    41,
      42,    43,    44,    45,    46,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,     0,    27,    27,     0,     0,
       0,     3,     3,     3,     0,     0,     0,     3,     3,     3,
       0,     0,     0,     3,     0,     0,     0,     0,     0,     0,
      27,    27,    84,     0,     0,    27,    27,    27,     0,    22,
      -1,    27,    27,    27,    25,    -1,    -1,    27,    27,    27,
      -1,    27,    27,    27,    27,    27,    27,    25,    18,    24,
      27,    27,    25,    -1,    25,    27,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    26,    24,    17,
      18,    24,    24,    24,    24,    23,    24,    25,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    -1,    24,    26,
      27,    15,    16,    17,    18,    19,    20,    21,    22,    24,
      -1,    -1,    26,    15,    16,    17,    18,    19,    20,    21,
      22,    -1,    -1,    -1,    26,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,    -1,    -1,    26,    15,    16,    17,
      18,    19,    20,    21,    22,    -1,    -1,    -1,    26,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    -1,    -1,
      26,    15,    16,    17,    18,    19,    20,    21,    22,    -1,
      -1,    -1,    26,    15,    16,    17,    18,    19,    20,    21,
      22,    -1,    -1,    -1,    26,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,    -1,    -1,    26,    15,    16,    17,
      18,    19,    20,    21,    22,    -1,    -1,    -1,    26,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    -1,    -1,
      26,    15,    16,    17,    18,    19,    20,    21,    22,    -1,
      -1,    -1,    26,    15,    16,    17,    18,    19,    20,    21,
      22,    -1,    -1,    -1,    26,    15,    16,    17,    18,    19,
      20,    21,    22,    17,    18,    19,    26,    17,    18,    23,
      24,    25,    -1,    23,    24,    25,    16,    17,    18,    19,
      20,    21,    22,    17,    18,    19,    20,    21,    22,    19,
      20,    21,    22
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  selectParser::yystos_[] =
  {
         0,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    17,    18,    23,    24,    25,    29,    30,    31,
      25,    25,    25,    25,    25,    25,    25,    25,    25,    25,
      25,    31,    31,    25,    31,     0,    29,     0,     3,    15,
      16,    17,    18,    19,    20,    21,    22,    27,    31,    19,
      31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
      31,    26,    24,    31,    31,    31,    31,    31,    31,    31,
      31,    26,    26,    26,    26,    26,    26,    26,    26,    26,
      26,    26,    26,    26,    27,     0,    27,     0,     3,    27,
       0,     3,    27,     0,     3,    27,     0,     3,    27,     0,
       3,    27,     0,     3,    27,     0,    27,     0,    27,     0,
      27,     0,    27,     0,     3,    27,     0,     3,    27,    31,
      24,    24,    24,    24,    24,    24,    24,    24,    26,     0,
      27,     0,    27,     0,    27,     0,    27,     0,    27,     0,
      27,     0,    27,     0,    27
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  selectParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,    40,    41,    44
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  selectParser::yyr1_[] =
  {
         0,    28,    29,    29,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    31,    31,
      31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
      31,    31,    31
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  selectParser::yyr2_[] =
  {
         0,     2,     1,     2,     5,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     5,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     5,     5,     5,     5,     2,     2,
       7,     7,     7,     7,     7,     7,     7,     7,     7,     7,
       7,     7,     4,     4,     7,     7,     7,     7,     3,     3,
       3,     3,     3,     3,     3,     3,     4,     6,     2,     2,
       3,     1,     1
  };

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const selectParser::yytname_[] =
  {
    "\"end of input\"", "error", "$undefined", "\"as\"", "\"avg\"",
  "\"count\"", "\"min\"", "\"max\"", "\"sum\"", "\"varpop\"",
  "\"varsamp\"", "\"stdpop\"", "\"stdsamp\"", "\"distinct\"", "\"median\"",
  "\"|\"", "\"&\"", "\"+\"", "\"-\"", "\"*\"", "\"/\"", "\"%\"", "\"**\"",
  "\"numerical value\"", "\"name\"", "'('", "')'", "','", "$accept",
  "slist", "sterm", "mathExpr", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const selectParser::rhs_number_type
  selectParser::yyrhs_[] =
  {
        29,     0,    -1,    30,    -1,    30,    29,    -1,     4,    25,
      31,    26,    27,    -1,     4,    25,    31,    26,     0,    -1,
       5,    25,    31,    26,    27,    -1,     5,    25,    31,    26,
       0,    -1,     5,    25,    19,    26,    27,    -1,     5,    25,
      19,    26,     0,    -1,     7,    25,    31,    26,    27,    -1,
       7,    25,    31,    26,     0,    -1,     6,    25,    31,    26,
      27,    -1,     6,    25,    31,    26,     0,    -1,     8,    25,
      31,    26,    27,    -1,     8,    25,    31,    26,     0,    -1,
       9,    25,    31,    26,    27,    -1,     9,    25,    31,    26,
       0,    -1,    10,    25,    31,    26,    27,    -1,    10,    25,
      31,    26,     0,    -1,    11,    25,    31,    26,    27,    -1,
      11,    25,    31,    26,     0,    -1,    12,    25,    31,    26,
      27,    -1,    12,    25,    31,    26,     0,    -1,    13,    25,
      31,    26,    27,    -1,    13,    25,    31,    26,     0,    -1,
      14,    25,    31,    26,    27,    -1,    14,    25,    31,    26,
       0,    -1,    31,    27,    -1,    31,     0,    -1,     4,    25,
      31,    26,     3,    24,    27,    -1,     4,    25,    31,    26,
       3,    24,     0,    -1,     5,    25,    31,    26,     3,    24,
      27,    -1,     5,    25,    31,    26,     3,    24,     0,    -1,
       5,    25,    19,    26,     3,    24,    27,    -1,     5,    25,
      19,    26,     3,    24,     0,    -1,     7,    25,    31,    26,
       3,    24,    27,    -1,     7,    25,    31,    26,     3,    24,
       0,    -1,     6,    25,    31,    26,     3,    24,    27,    -1,
       6,    25,    31,    26,     3,    24,     0,    -1,     8,    25,
      31,    26,     3,    24,    27,    -1,     8,    25,    31,    26,
       3,    24,     0,    -1,    31,     3,    24,    27,    -1,    31,
       3,    24,     0,    -1,    13,    25,    31,    26,     3,    24,
      27,    -1,    13,    25,    31,    26,     3,    24,     0,    -1,
      14,    25,    31,    26,     3,    24,    27,    -1,    14,    25,
      31,    26,     3,    24,     0,    -1,    31,    17,    31,    -1,
      31,    18,    31,    -1,    31,    19,    31,    -1,    31,    20,
      31,    -1,    31,    21,    31,    -1,    31,    22,    31,    -1,
      31,    16,    31,    -1,    31,    15,    31,    -1,    24,    25,
      31,    26,    -1,    24,    25,    31,    27,    31,    26,    -1,
      18,    31,    -1,    17,    31,    -1,    25,    31,    26,    -1,
      24,    -1,    23,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  selectParser::yyprhs_[] =
  {
         0,     0,     3,     5,     8,    14,    20,    26,    32,    38,
      44,    50,    56,    62,    68,    74,    80,    86,    92,    98,
     104,   110,   116,   122,   128,   134,   140,   146,   152,   155,
     158,   166,   174,   182,   190,   198,   206,   214,   222,   230,
     238,   246,   254,   259,   264,   272,   280,   288,   296,   300,
     304,   308,   312,   316,   320,   324,   328,   333,   340,   343,
     346,   350,   352
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  selectParser::yyrline_[] =
  {
         0,    85,    85,    85,    86,    90,    94,    98,   102,   106,
     110,   114,   118,   122,   126,   130,   134,   138,   142,   146,
     150,   154,   158,   162,   166,   170,   174,   178,   182,   186,
     190,   195,   200,   205,   210,   215,   220,   225,   230,   235,
     240,   245,   250,   255,   260,   265,   270,   275,   283,   295,
     307,   319,   331,   343,   355,   367,   379,   391,   404,   414,
     417,   420,   428
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
      25,    26,     2,     2,    27,     2,     2,     2,     2,     2,
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

  const int selectParser::yyeof_ = 0;
  const int selectParser::yylast_ = 322;
  const int selectParser::yynnts_ = 4;
  const int selectParser::yyempty_ = -2;
  const int selectParser::yyfinal_ = 35;
  const int selectParser::yyterror_ = 1;
  const int selectParser::yyerrcode_ = 256;
  const int selectParser::yyntokens_ = 28;

  const unsigned int selectParser::yyuser_token_number_max_ = 279;
  const selectParser::token_number_type selectParser::yyundef_token_ = 2;


} // ibis

/* Line 1054 of lalr1.cc  */
#line 1736 "selectParser.cc"


/* Line 1056 of lalr1.cc  */
#line 437 "selectParser.yy"

void ibis::selectParser::error(const ibis::selectParser::location_type& l,
			       const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::selectParser encountered " << m << " at location " << l;
}


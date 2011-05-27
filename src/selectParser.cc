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
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 7:

/* Line 678 of lalr1.cc  */
#line 99 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 8:

/* Line 678 of lalr1.cc  */
#line 104 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 9:

/* Line 678 of lalr1.cc  */
#line 109 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::AVG);
}
    break;

  case 10:

/* Line 678 of lalr1.cc  */
#line 114 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 11:

/* Line 678 of lalr1.cc  */
#line 118 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 12:

/* Line 678 of lalr1.cc  */
#line 122 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 13:

/* Line 678 of lalr1.cc  */
#line 127 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 14:

/* Line 678 of lalr1.cc  */
#line 132 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 15:

/* Line 678 of lalr1.cc  */
#line 137 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 16:

/* Line 678 of lalr1.cc  */
#line 142 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 17:

/* Line 678 of lalr1.cc  */
#line 146 "selectParser.yy"
    {
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 18:

/* Line 678 of lalr1.cc  */
#line 150 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 19:

/* Line 678 of lalr1.cc  */
#line 155 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 20:

/* Line 678 of lalr1.cc  */
#line 160 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 21:

/* Line 678 of lalr1.cc  */
#line 165 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back(new ibis::math::variable("*"));
    driver.aggr_.push_back(ibis::selectClause::CNT);
}
    break;

  case 22:

/* Line 678 of lalr1.cc  */
#line 170 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 23:

/* Line 678 of lalr1.cc  */
#line 174 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 24:

/* Line 678 of lalr1.cc  */
#line 178 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 25:

/* Line 678 of lalr1.cc  */
#line 183 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 26:

/* Line 678 of lalr1.cc  */
#line 188 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 27:

/* Line 678 of lalr1.cc  */
#line 193 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MAX);
}
    break;

  case 28:

/* Line 678 of lalr1.cc  */
#line 198 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 29:

/* Line 678 of lalr1.cc  */
#line 202 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 30:

/* Line 678 of lalr1.cc  */
#line 206 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 31:

/* Line 678 of lalr1.cc  */
#line 211 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 32:

/* Line 678 of lalr1.cc  */
#line 216 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 33:

/* Line 678 of lalr1.cc  */
#line 221 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MIN);
}
    break;

  case 34:

/* Line 678 of lalr1.cc  */
#line 226 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 35:

/* Line 678 of lalr1.cc  */
#line 230 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 36:

/* Line 678 of lalr1.cc  */
#line 234 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 37:

/* Line 678 of lalr1.cc  */
#line 239 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 38:

/* Line 678 of lalr1.cc  */
#line 244 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 39:

/* Line 678 of lalr1.cc  */
#line 249 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::SUM);
}
    break;

  case 40:

/* Line 678 of lalr1.cc  */
#line 254 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 41:

/* Line 678 of lalr1.cc  */
#line 258 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 42:

/* Line 678 of lalr1.cc  */
#line 262 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 43:

/* Line 678 of lalr1.cc  */
#line 267 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 44:

/* Line 678 of lalr1.cc  */
#line 272 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 45:

/* Line 678 of lalr1.cc  */
#line 277 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARPOP);
}
    break;

  case 46:

/* Line 678 of lalr1.cc  */
#line 282 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 47:

/* Line 678 of lalr1.cc  */
#line 286 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 48:

/* Line 678 of lalr1.cc  */
#line 290 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 49:

/* Line 678 of lalr1.cc  */
#line 295 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 50:

/* Line 678 of lalr1.cc  */
#line 300 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 51:

/* Line 678 of lalr1.cc  */
#line 305 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::VARSAMP);
}
    break;

  case 52:

/* Line 678 of lalr1.cc  */
#line 310 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 53:

/* Line 678 of lalr1.cc  */
#line 314 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 54:

/* Line 678 of lalr1.cc  */
#line 318 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 55:

/* Line 678 of lalr1.cc  */
#line 323 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 56:

/* Line 678 of lalr1.cc  */
#line 328 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 57:

/* Line 678 of lalr1.cc  */
#line 333 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDPOP);
}
    break;

  case 58:

/* Line 678 of lalr1.cc  */
#line 338 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 59:

/* Line 678 of lalr1.cc  */
#line 342 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 60:

/* Line 678 of lalr1.cc  */
#line 346 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 61:

/* Line 678 of lalr1.cc  */
#line 351 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 62:

/* Line 678 of lalr1.cc  */
#line 356 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 63:

/* Line 678 of lalr1.cc  */
#line 361 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::STDSAMP);
}
    break;

  case 64:

/* Line 678 of lalr1.cc  */
#line 366 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 65:

/* Line 678 of lalr1.cc  */
#line 370 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 66:

/* Line 678 of lalr1.cc  */
#line 374 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 67:

/* Line 678 of lalr1.cc  */
#line 379 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 68:

/* Line 678 of lalr1.cc  */
#line 384 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 69:

/* Line 678 of lalr1.cc  */
#line 389 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::DISTINCT);
}
    break;

  case 70:

/* Line 678 of lalr1.cc  */
#line 394 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 71:

/* Line 678 of lalr1.cc  */
#line 398 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(5) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 72:

/* Line 678 of lalr1.cc  */
#line 402 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 73:

/* Line 678 of lalr1.cc  */
#line 407 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(6) - (5)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(6) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 74:

/* Line 678 of lalr1.cc  */
#line 412 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 75:

/* Line 678 of lalr1.cc  */
#line 417 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(7) - (6)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(7) - (3)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::MEDIAN);
}
    break;

  case 76:

/* Line 678 of lalr1.cc  */
#line 422 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 77:

/* Line 678 of lalr1.cc  */
#line 426 "selectParser.yy"
    {
    driver.terms_.push_back((yysemantic_stack_[(2) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 78:

/* Line 678 of lalr1.cc  */
#line 430 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(3) - (2)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(3) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 79:

/* Line 678 of lalr1.cc  */
#line 435 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(3) - (2)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(3) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 80:

/* Line 678 of lalr1.cc  */
#line 440 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 81:

/* Line 678 of lalr1.cc  */
#line 445 "selectParser.yy"
    {
    driver.alias_[*(yysemantic_stack_[(4) - (3)].stringVal)] = driver.terms_.size();
    driver.terms_.push_back((yysemantic_stack_[(4) - (1)].selectNode));
    driver.aggr_.push_back(ibis::selectClause::NIL_AGGR);
}
    break;

  case 82:

/* Line 678 of lalr1.cc  */
#line 453 "selectParser.yy"
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

  case 83:

/* Line 678 of lalr1.cc  */
#line 465 "selectParser.yy"
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

  case 84:

/* Line 678 of lalr1.cc  */
#line 477 "selectParser.yy"
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

  case 85:

/* Line 678 of lalr1.cc  */
#line 489 "selectParser.yy"
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

  case 86:

/* Line 678 of lalr1.cc  */
#line 501 "selectParser.yy"
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

  case 87:

/* Line 678 of lalr1.cc  */
#line 513 "selectParser.yy"
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

  case 88:

/* Line 678 of lalr1.cc  */
#line 525 "selectParser.yy"
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

  case 89:

/* Line 678 of lalr1.cc  */
#line 537 "selectParser.yy"
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

  case 90:

/* Line 678 of lalr1.cc  */
#line 549 "selectParser.yy"
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

  case 91:

/* Line 678 of lalr1.cc  */
#line 561 "selectParser.yy"
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

  case 92:

/* Line 678 of lalr1.cc  */
#line 574 "selectParser.yy"
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

  case 93:

/* Line 678 of lalr1.cc  */
#line 584 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(2) - (2)].selectNode);
}
    break;

  case 94:

/* Line 678 of lalr1.cc  */
#line 587 "selectParser.yy"
    {
    (yyval.selectNode) = (yysemantic_stack_[(3) - (2)].selectNode);
}
    break;

  case 95:

/* Line 678 of lalr1.cc  */
#line 590 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a variable name " << *(yysemantic_stack_[(1) - (1)].stringVal);
#endif
    (yyval.selectNode) = new ibis::math::variable((yysemantic_stack_[(1) - (1)].stringVal)->c_str());
    delete (yysemantic_stack_[(1) - (1)].stringVal);
}
    break;

  case 96:

/* Line 678 of lalr1.cc  */
#line 598 "selectParser.yy"
    {
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(ibis::gVerbose >= 0)
	<< __FILE__ << ":" << __LINE__ << " got a number " << (yysemantic_stack_[(1) - (1)].doubleVal);
#endif
    (yyval.selectNode) = new ibis::math::number((yysemantic_stack_[(1) - (1)].doubleVal));
}
    break;



/* Line 678 of lalr1.cc  */
#line 1533 "selectParser.cc"
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
       143,   -23,   -22,   -20,   -19,   -18,     0,     1,    95,   133,
     134,   137,   321,   321,   -24,   138,   321,   121,   143,    20,
     321,   317,   321,   321,   321,   321,   321,   321,   321,   321,
     321,   122,   122,   321,   167,   -24,   -24,   -24,   140,   321,
     321,   321,   321,   321,   321,   321,   321,    24,   -24,   179,
     139,   191,   203,   215,   227,   239,   251,   263,   275,   287,
     299,   154,   -24,    66,   331,   337,   341,   341,   122,   122,
     122,   122,   -24,   -24,    19,    45,    49,    50,    54,    55,
      56,    60,    61,    62,    67,    68,   -24,   321,   -24,   -24,
     -24,   153,    90,   -24,   -24,   155,    96,   -24,   -24,   166,
      97,   -24,   -24,   168,    98,   -24,   -24,   178,    99,   -24,
     -24,   180,   100,   -24,   -24,   190,   101,   -24,   -24,   192,
     102,   -24,   -24,   202,   103,   -24,   -24,   204,   104,   -24,
     -24,   214,   105,   -24,   -24,   216,   106,   -24,   311,   107,
     -24,   -24,   108,   -24,   -24,   109,   -24,   -24,   110,   -24,
     -24,   111,   -24,   -24,   112,   -24,   -24,   113,   -24,   -24,
     114,   -24,   -24,   115,   -24,   -24,   116,   -24,   -24,   118,
     -24,   -24,   119,   -24,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,
     -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24,   -24
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  selectParser::yydefact_[] =
  {
         0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    96,    95,     0,     0,     2,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    93,    92,     0,     0,     1,     3,    77,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    76,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    94,     0,    89,    88,    82,    83,    84,    85,
      86,    87,    79,    78,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    90,     0,    81,    80,
       5,     0,     0,     4,    17,     0,     0,    16,    11,     0,
       0,    10,    29,     0,     0,    28,    23,     0,     0,    22,
      35,     0,     0,    34,    41,     0,     0,    40,    47,     0,
       0,    46,    53,     0,     0,    52,    59,     0,     0,    58,
      65,     0,     0,    64,    71,     0,     0,    70,     0,     0,
       7,     6,     0,    19,    20,     0,    13,    12,     0,    31,
      30,     0,    25,    24,     0,    37,    36,     0,    43,    42,
       0,    49,    48,     0,    55,    54,     0,    61,    60,     0,
      67,    66,     0,    73,    72,    91,     9,     8,    21,    18,
      15,    14,    33,    32,    27,    26,    39,    38,    45,    44,
      51,    50,    57,    56,    63,    62,    69,    68,    75,    74
  };

  /* YYPGOTO[NTERM-NUM].  */
  const short int
  selectParser::yypgoto_[] =
  {
       -24,   160,   -24,   -12
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
        31,    32,    20,    21,    34,    22,    23,    24,    49,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    90,
      37,    61,    91,    38,    72,    25,    26,    64,    65,    66,
      67,    68,    69,    70,    71,    39,    40,    41,    42,    43,
      44,    45,    46,    92,    47,    94,    93,    48,    95,    98,
     102,    73,    99,   103,   106,   110,   114,   107,   111,   115,
     118,   122,   126,   119,   123,   127,    88,   130,   134,    96,
     131,   135,    97,   100,   104,   138,   101,   105,   108,   112,
     116,   109,   113,   117,   120,   124,   128,   121,   125,   129,
     140,   132,   136,    89,   133,   137,   143,   146,   149,   152,
     155,   158,   161,   164,   167,   170,   173,   176,   178,   180,
     182,   184,   186,   188,   190,   192,   194,   141,   196,   198,
      27,    35,     0,   144,   147,   150,   153,   156,   159,   162,
     165,   168,   171,   174,   177,   179,   181,   183,   185,   187,
     189,   191,   193,   195,    46,   197,   199,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    28,    29,
      12,    13,    30,    33,    63,    75,    14,    15,    16,    39,
      40,    41,    42,    43,    44,    45,    46,   139,    36,   142,
      86,    87,    39,    40,    41,    42,    43,    44,    45,    46,
     145,     0,   148,    62,    39,    40,    41,    42,    43,    44,
      45,    46,   151,     0,   154,    74,    39,    40,    41,    42,
      43,    44,    45,    46,   157,     0,   160,    76,    39,    40,
      41,    42,    43,    44,    45,    46,   163,     0,   166,    77,
      39,    40,    41,    42,    43,    44,    45,    46,   169,     0,
     172,    78,    39,    40,    41,    42,    43,    44,    45,    46,
       0,     0,     0,    79,    39,    40,    41,    42,    43,    44,
      45,    46,     0,     0,     0,    80,    39,    40,    41,    42,
      43,    44,    45,    46,     0,     0,     0,    81,    39,    40,
      41,    42,    43,    44,    45,    46,     0,     0,     0,    82,
      39,    40,    41,    42,    43,    44,    45,    46,     0,     0,
       0,    83,    39,    40,    41,    42,    43,    44,    45,    46,
       0,     0,     0,    84,    39,    40,    41,    42,    43,    44,
      45,    46,     0,     0,     0,    85,    39,    40,    41,    42,
      43,    44,    45,    46,    12,    13,    50,   175,    12,    13,
      14,    15,    16,     0,    14,    15,    16,    40,    41,    42,
      43,    44,    45,    46,    41,    42,    43,    44,    45,    46,
      43,    44,    45,    46
  };

  /* YYCHECK.  */
  const signed char
  selectParser::yycheck_[] =
  {
        12,    13,    25,    25,    16,    25,    25,    25,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,     0,
       0,    33,     3,     3,     0,    25,    25,    39,    40,    41,
      42,    43,    44,    45,    46,    15,    16,    17,    18,    19,
      20,    21,    22,    24,    24,     0,    27,    27,     3,     0,
       0,    27,     3,     3,     0,     0,     0,     3,     3,     3,
       0,     0,     0,     3,     3,     3,     0,     0,     0,    24,
       3,     3,    27,    24,    24,    87,    27,    27,    24,    24,
      24,    27,    27,    27,    24,    24,    24,    27,    27,    27,
       0,    24,    24,    27,    27,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    27,     0,     0,
      25,     0,    -1,    27,    27,    27,    27,    27,    27,    27,
      27,    27,    27,    27,    27,    27,    27,    27,    27,    27,
      27,    27,    27,    27,    22,    27,    27,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    25,    25,
      17,    18,    25,    25,    24,    26,    23,    24,    25,    15,
      16,    17,    18,    19,    20,    21,    22,    24,    18,    24,
      26,    27,    15,    16,    17,    18,    19,    20,    21,    22,
      24,    -1,    24,    26,    15,    16,    17,    18,    19,    20,
      21,    22,    24,    -1,    24,    26,    15,    16,    17,    18,
      19,    20,    21,    22,    24,    -1,    24,    26,    15,    16,
      17,    18,    19,    20,    21,    22,    24,    -1,    24,    26,
      15,    16,    17,    18,    19,    20,    21,    22,    24,    -1,
      24,    26,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    -1,    -1,    26,    15,    16,    17,    18,    19,    20,
      21,    22,    -1,    -1,    -1,    26,    15,    16,    17,    18,
      19,    20,    21,    22,    -1,    -1,    -1,    26,    15,    16,
      17,    18,    19,    20,    21,    22,    -1,    -1,    -1,    26,
      15,    16,    17,    18,    19,    20,    21,    22,    -1,    -1,
      -1,    26,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    -1,    -1,    26,    15,    16,    17,    18,    19,    20,
      21,    22,    -1,    -1,    -1,    26,    15,    16,    17,    18,
      19,    20,    21,    22,    17,    18,    19,    26,    17,    18,
      23,    24,    25,    -1,    23,    24,    25,    16,    17,    18,
      19,    20,    21,    22,    17,    18,    19,    20,    21,    22,
      19,    20,    21,    22
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
      16,    17,    18,    19,    20,    21,    22,    24,    27,    31,
      19,    31,    31,    31,    31,    31,    31,    31,    31,    31,
      31,    31,    26,    24,    31,    31,    31,    31,    31,    31,
      31,    31,     0,    27,    26,    26,    26,    26,    26,    26,
      26,    26,    26,    26,    26,    26,    26,    27,     0,    27,
       0,     3,    24,    27,     0,     3,    24,    27,     0,     3,
      24,    27,     0,     3,    24,    27,     0,     3,    24,    27,
       0,     3,    24,    27,     0,     3,    24,    27,     0,     3,
      24,    27,     0,     3,    24,    27,     0,     3,    24,    27,
       0,     3,    24,    27,     0,     3,    24,    27,    31,    24,
       0,    27,    24,     0,    27,    24,     0,    27,    24,     0,
      27,    24,     0,    27,    24,     0,    27,    24,     0,    27,
      24,     0,    27,    24,     0,    27,    24,     0,    27,    24,
       0,    27,    24,     0,    27,    26,     0,    27,     0,    27,
       0,    27,     0,    27,     0,    27,     0,    27,     0,    27,
       0,    27,     0,    27,     0,    27,     0,    27,     0,    27
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
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    31,    31,    31,    31,    31,    31,    31,    31,
      31,    31,    31,    31,    31,    31,    31
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  selectParser::yyr2_[] =
  {
         0,     2,     1,     2,     5,     5,     6,     6,     7,     7,
       5,     5,     6,     6,     7,     7,     5,     5,     7,     6,
       6,     7,     5,     5,     6,     6,     7,     7,     5,     5,
       6,     6,     7,     7,     5,     5,     6,     6,     7,     7,
       5,     5,     6,     6,     7,     7,     5,     5,     6,     6,
       7,     7,     5,     5,     6,     6,     7,     7,     5,     5,
       6,     6,     7,     7,     5,     5,     6,     6,     7,     7,
       5,     5,     6,     6,     7,     7,     2,     2,     3,     3,
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
       4,    25,    31,    26,    24,    27,    -1,     4,    25,    31,
      26,    24,     0,    -1,     4,    25,    31,    26,     3,    24,
      27,    -1,     4,    25,    31,    26,     3,    24,     0,    -1,
       5,    25,    31,    26,    27,    -1,     5,    25,    31,    26,
       0,    -1,     5,    25,    31,    26,    24,    27,    -1,     5,
      25,    31,    26,    24,     0,    -1,     5,    25,    31,    26,
       3,    24,    27,    -1,     5,    25,    31,    26,     3,    24,
       0,    -1,     5,    25,    19,    26,    27,    -1,     5,    25,
      19,    26,     0,    -1,     5,    25,    19,    26,     3,    24,
      27,    -1,     5,    25,    19,    26,    24,     0,    -1,     5,
      25,    19,    26,    24,    27,    -1,     5,    25,    19,    26,
       3,    24,     0,    -1,     7,    25,    31,    26,    27,    -1,
       7,    25,    31,    26,     0,    -1,     7,    25,    31,    26,
      24,    27,    -1,     7,    25,    31,    26,    24,     0,    -1,
       7,    25,    31,    26,     3,    24,    27,    -1,     7,    25,
      31,    26,     3,    24,     0,    -1,     6,    25,    31,    26,
      27,    -1,     6,    25,    31,    26,     0,    -1,     6,    25,
      31,    26,    24,    27,    -1,     6,    25,    31,    26,    24,
       0,    -1,     6,    25,    31,    26,     3,    24,    27,    -1,
       6,    25,    31,    26,     3,    24,     0,    -1,     8,    25,
      31,    26,    27,    -1,     8,    25,    31,    26,     0,    -1,
       8,    25,    31,    26,    24,    27,    -1,     8,    25,    31,
      26,    24,     0,    -1,     8,    25,    31,    26,     3,    24,
      27,    -1,     8,    25,    31,    26,     3,    24,     0,    -1,
       9,    25,    31,    26,    27,    -1,     9,    25,    31,    26,
       0,    -1,     9,    25,    31,    26,    24,    27,    -1,     9,
      25,    31,    26,    24,     0,    -1,     9,    25,    31,    26,
       3,    24,    27,    -1,     9,    25,    31,    26,     3,    24,
       0,    -1,    10,    25,    31,    26,    27,    -1,    10,    25,
      31,    26,     0,    -1,    10,    25,    31,    26,    24,    27,
      -1,    10,    25,    31,    26,    24,     0,    -1,    10,    25,
      31,    26,     3,    24,    27,    -1,    10,    25,    31,    26,
       3,    24,     0,    -1,    11,    25,    31,    26,    27,    -1,
      11,    25,    31,    26,     0,    -1,    11,    25,    31,    26,
      24,    27,    -1,    11,    25,    31,    26,    24,     0,    -1,
      11,    25,    31,    26,     3,    24,    27,    -1,    11,    25,
      31,    26,     3,    24,     0,    -1,    12,    25,    31,    26,
      27,    -1,    12,    25,    31,    26,     0,    -1,    12,    25,
      31,    26,    24,    27,    -1,    12,    25,    31,    26,    24,
       0,    -1,    12,    25,    31,    26,     3,    24,    27,    -1,
      12,    25,    31,    26,     3,    24,     0,    -1,    13,    25,
      31,    26,    27,    -1,    13,    25,    31,    26,     0,    -1,
      13,    25,    31,    26,    24,    27,    -1,    13,    25,    31,
      26,    24,     0,    -1,    13,    25,    31,    26,     3,    24,
      27,    -1,    13,    25,    31,    26,     3,    24,     0,    -1,
      14,    25,    31,    26,    27,    -1,    14,    25,    31,    26,
       0,    -1,    14,    25,    31,    26,    24,    27,    -1,    14,
      25,    31,    26,    24,     0,    -1,    14,    25,    31,    26,
       3,    24,    27,    -1,    14,    25,    31,    26,     3,    24,
       0,    -1,    31,    27,    -1,    31,     0,    -1,    31,    24,
      27,    -1,    31,    24,     0,    -1,    31,     3,    24,    27,
      -1,    31,     3,    24,     0,    -1,    31,    17,    31,    -1,
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
         0,     0,     3,     5,     8,    14,    20,    27,    34,    42,
      50,    56,    62,    69,    76,    84,    92,    98,   104,   112,
     119,   126,   134,   140,   146,   153,   160,   168,   176,   182,
     188,   195,   202,   210,   218,   224,   230,   237,   244,   252,
     260,   266,   272,   279,   286,   294,   302,   308,   314,   321,
     328,   336,   344,   350,   356,   363,   370,   378,   386,   392,
     398,   405,   412,   420,   428,   434,   440,   447,   454,   462,
     470,   476,   482,   489,   496,   504,   512,   515,   518,   522,
     526,   531,   536,   540,   544,   548,   552,   556,   560,   564,
     568,   573,   580,   583,   586,   590,   592
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  selectParser::yyrline_[] =
  {
         0,    85,    85,    85,    86,    90,    94,    99,   104,   109,
     114,   118,   122,   127,   132,   137,   142,   146,   150,   155,
     160,   165,   170,   174,   178,   183,   188,   193,   198,   202,
     206,   211,   216,   221,   226,   230,   234,   239,   244,   249,
     254,   258,   262,   267,   272,   277,   282,   286,   290,   295,
     300,   305,   310,   314,   318,   323,   328,   333,   338,   342,
     346,   351,   356,   361,   366,   370,   374,   379,   384,   389,
     394,   398,   402,   407,   412,   417,   422,   426,   430,   435,
     440,   445,   453,   465,   477,   489,   501,   513,   525,   537,
     549,   561,   574,   584,   587,   590,   598
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
  const int selectParser::yylast_ = 363;
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
#line 2169 "selectParser.cc"


/* Line 1056 of lalr1.cc  */
#line 607 "selectParser.yy"

void ibis::selectParser::error(const ibis::selectParser::location_type& l,
			       const std::string& m) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- ibis::selectParser encountered " << m << " at location "
	<< l;
}


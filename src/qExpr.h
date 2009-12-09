// File: $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 1998-2009 the Regents of the University of California
//
// Primary contact: John Wu <John.Wu at acm.org>
#ifndef IBIS_EXPR_H
#define IBIS_EXPR_H
///@file
/// Define the query expression.
///
#include "util.h"
#include "array_t.h"

namespace ibis { // additional names related to qExpr
    class qRange;	///< A simple range defined on a single attribute.
    class qContinuousRange;///< A range defined with one or two boundaries.
    class qDiscreteRange;	///< A range defined with discrete values.
    class qString;	///< An equality expression with a string literal.
    class qMultiString;	///< A range condition involving multiple strings.
    class compRange;	///< A comparisons involving arithmetic expression.
    class rangeJoin;	///< A special expression for range join operations.
    class qAnyAny;	///< A special form of any-match-any query.
    class qLike;	///< A representation of the operator LIKE.
}

/// @ingroup FastBitIBIS
/// The top level query expression object.  It encodes the logical
/// operations between two child expressions, serving as the interior nodes
/// of an expression tree.  Leaf nodes are going to be derived later.
class ibis::qExpr {
public:
    /// Definition of node types.  Logical operators are listed in the
    /// front and leaf node types are listed at the end.
    enum TYPE {
	LOGICAL_UNDEFINED, LOGICAL_NOT, LOGICAL_AND, LOGICAL_OR, LOGICAL_XOR,
	LOGICAL_MINUS, RANGE, DRANGE, STRING, MSTRING, COMPRANGE, MATHTERM,
	JOIN, TOPK, ANYANY, LIKE
    };
    /// Comparison operator supported in RANGE.
    enum COMPARE {
	OP_UNDEFINED, OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ
    };

    /// Default constructor.  It generates a node of undefined type.
    qExpr() : type(LOGICAL_UNDEFINED), left(0), right(0) {}

    /// Construct a node of specified type.  Not for implicit type conversion.
    explicit qExpr(TYPE op) : type(op), left(0), right(0) {}
    /// Construct a full specified node.  All three arguments are present.
    qExpr(TYPE op, qExpr* qe1, qExpr* qe2) : type(op), left(qe1),
					     right(qe2) {}
    /// Deep copy.
    qExpr(const qExpr& qe) : type(qe.type),
			     left(qe.left ? qe.left->dup() : 0),
			     right(qe.right ? qe.right->dup() : 0) {}
    /// Destruct the node recursively.
    virtual ~qExpr() {delete right; delete left;}

    /// Change the left child.
    void setLeft(qExpr *expr) {delete left; left=expr;}
    /// Change the right child.
    void setRight(qExpr *expr) {delete right; right=expr;}
    /// Return a pointer to the left child.
    qExpr*& getLeft() {return left;}
    /// Return a pointer to the right child.
    qExpr*& getRight() {return right;}

    /// Return the node type.
    TYPE getType() const {return type;}
    /// Return a const pointer to the left child.
    const qExpr* getLeft() const {return left;}
    /// Return a const pointer to the right child.
    const qExpr* getRight() const {return right;}

    /// Return true is there is a term with join operation, false otherwise.
    bool hasJoin() const;

    /// Count the number of items in the query expression.
    virtual uint32_t nItems() const {
	return 1 + (left != 0 ? left->nItems() : 0) +
	    (right != 0 ? right->nItems() : 0);}

    /// Print out the node in the string form.
    virtual void print(std::ostream&) const;
    /// Print out the full expression.
    virtual void printFull(std::ostream& out) const;

    /// A functor to be used by the function reorder.
    struct weight {
	virtual double operator()(const qExpr* ex) const = 0;
	virtual ~weight() {};
    };
    double reorder(const weight&); ///< Reorder the expressions tree.
    // Duplicate this query expression.  Return the pointer to the new object.
    virtual qExpr* dup() const {
	qExpr* res = new qExpr(type);
	if (left)
	    res->left = left->dup();
	if (right)
	    res->right = right->dup();
	return res;
    }

    bool isTerminal() const {return (left==0 && right==0);}
    bool directEval() const
    {return (type==RANGE || type==STRING || type==COMPRANGE ||
	     type==DRANGE || type==MSTRING || type==ANYANY ||
	     (type==LOGICAL_NOT && left && left->directEval()));}

    /// Is the expression simple, i.e., containing only simple range
    /// conditions joined with logical operators ?
    virtual bool isSimple() const {
	if (left) {
	    if (right) return left->isSimple() && right->isSimple();
	    else return left->isSimple();
	}
	else if (right) {
	    return right->isSimple();
	}
	else { // the derived classes essentially overrides this case.
	    return true;
	}
    }

    /// Separate an expression tree into two joined with an AND operator.
    /// The first one of the two new expressions contains only simple range
    /// expressions, and the second contains what is left.
    int separateSimple(ibis::qExpr *&simple, ibis::qExpr *&tail) const;

    /// Extract conjunctive terms of the specified type.
    void extractJoins(std::vector<const rangeJoin*>& terms) const;
    /// Find the first range condition involving the named variable.
    qRange* findRange(const char* vname);

    /// Attempt to convert simplify the query expressions.
    static void simplify(ibis::qExpr*&);

    qExpr& operator=(const qExpr& rhs) {
	delete left; delete right;
	type = rhs.type;
	left = (rhs.left != 0 ? rhs.left->dup() : 0);
	right = (rhs.right != 0 ? rhs.right->dup() : 0);
	return *this;
    }

private:
    TYPE   type;	// the type of node, logical operator, type of leaf
    qExpr* left;	// the left child
    qExpr* right;	// the right child

    // adjust the tree to favor the sequential evaluation order
    void adjust();
}; // ibis::qExpr

/// A class to represent simple range conditions.  This is an abstract base
/// class for qContinuousRange and qDiscreteRange.  The main virtual
/// functions, colName and inRange are used by procedures that evaluate the
/// conditions.
class ibis::qRange : public ibis::qExpr {
public:
    /// Returns the name of the attribute involved.
    virtual const char* colName() const = 0;
    /// Given a value, determine whether it is in the range defined.
    /// Return true if it is, return false, otherwise.
    virtual bool inRange(double val) const = 0;

    /// Reduce the range to be no more than [left, right].
    virtual void restrictRange(double left, double right) = 0;
    /// The lower bound of the range.
    virtual const double& leftBound() const = 0;
    /// The upper bound of the range.
    virtual const double& rightBound() const = 0;
    /// Is the current range empty?
    virtual bool empty() const = 0;

    virtual ~qRange() {}; // nothing to do

protected:
    // reduce the scope of the constructor
    qRange() : qExpr() {};
    qRange(TYPE t) : qExpr(t) {};

private:
    qRange(const qRange&) {}; // no copy constructor allowed, must use dup
    qRange& operator=(const qRange&);
}; // ibis::qRange

/// Simple range condition.  It is implemented as a derived class of qExpr.
/// Possible range operator are defined in ibis::qExpr::COMPARE.  It is
/// designed to expression equality conditions, one-sided range conditions
/// and two-sided range conditions.
/// @verbatim
///
/// /* an equality expression */
/// ibis::qExpr *expr = new ibis::qContinuousRange("a", ibis::qExpr::OP_EQ, 5.0);
/// /* a one-sided range expression */
/// ibis::qExpr *expr = new ibis::qContinuousRange("a", ibis::qExpr::OP_GE, 1.3);
/// /* a two-sided range expression */
/// ibis::qExpr *expr = new ibis::qContinuousRange(3.6, ibis::qExpr::OP_LE,
/// "a", ibis::qExpr::OP_LT, 4.7);
///
/// @endverbatim
class ibis::qContinuousRange : public ibis::qRange {
public:
    /// Construct an empty range expression.
    qContinuousRange()
	: qRange(ibis::qExpr::RANGE), name(0), lower(0), upper(0),
	  left_op(OP_UNDEFINED), right_op(OP_UNDEFINED) {};
    /// Construct a range expression from strings.
    qContinuousRange(const char* lstr, COMPARE lop, const char* prop,
		     COMPARE rop, const char* rstr);
    /// Construct a range expression with an integer boundary.
    qContinuousRange(const char* col, COMPARE op, uint32_t val) :
	qRange(ibis::qExpr::RANGE), name(ibis::util::strnewdup(col)),
	lower(val), upper(DBL_MAX), left_op(op), right_op(OP_UNDEFINED) {};
    /// Copy constructor.
    qContinuousRange(const qContinuousRange& rhs) :
	qRange(ibis::qExpr::RANGE), name(ibis::util::strnewdup(rhs.name)),
	lower(rhs.lower), upper(rhs.upper), left_op(rhs.left_op),
	right_op(rhs.right_op) {};
    /// Construct a range expression from double-precision boundaries.
    qContinuousRange(double lv, COMPARE lop, const char* prop,
		     COMPARE rop, double rv)
	: qRange(ibis::qExpr::RANGE), name(ibis::util::strnewdup(prop)),
	  lower(lv), upper(rv), left_op(lop), right_op(rop) {};
    /// Construct a one-side range expression.
    qContinuousRange(const char* prop, COMPARE op, double val)
	: qRange(ibis::qExpr::RANGE), name(ibis::util::strnewdup(prop)),
	  lower(-DBL_MAX), upper(val), left_op(OP_UNDEFINED), right_op(op) {
	// prefer to use the operator < and <= rather than > and >=
	if (right_op == ibis::qExpr::OP_GT) {
	    right_op = ibis::qExpr::OP_UNDEFINED;
	    left_op = ibis::qExpr::OP_LT;
	    lower = upper;
	    upper = DBL_MAX;
	}
	else if (right_op == ibis::qExpr::OP_GE) {
	    right_op = ibis::qExpr::OP_UNDEFINED;
	    left_op = ibis::qExpr::OP_LE;
	    lower = upper;
	    upper = DBL_MAX;
	}
    };

    virtual ~qContinuousRange() {delete [] name;}

    // provide read access to all private variables
    virtual const char *colName() const {return name;}
    COMPARE leftOperator() const {return left_op;}
    COMPARE rightOperator() const {return right_op;}
    virtual const double& leftBound() const {
	return (lower);}
    virtual const double& rightBound() const {
	return (upper);}
    // allow one to possibly change the left and right bounds, the left and
    // right operator
    double& leftBound() {return lower;}
    double& rightBound() {return upper;}
    COMPARE& leftOperator() {return left_op;}
    COMPARE& rightOperator() {return right_op;}

    // Fold the boundaries to integers.
    void foldBoundaries();
    // Fold the boundaries to unsigned integers.
    void foldUnsignedBoundaries();

    // duplicate *this
    virtual qContinuousRange* dup() const {return new qContinuousRange(*this);}
    virtual bool inRange(double val) const;
    virtual void restrictRange(double left, double right);
    virtual bool empty() const;

    virtual void print(std::ostream&) const;
    virtual void printFull(std::ostream& out) const {print(out);}
    /// An operator for comparing two query expressions.
    inline bool operator<(const qContinuousRange& y) const;

private:
    char* name;
    double lower, upper;
    COMPARE left_op, right_op;

    qContinuousRange& operator=(const qContinuousRange&);
    friend void ibis::qExpr::simplify(ibis::qExpr*&);
}; // ibis::qContinuousRange

/// A discrete range expression.  It is used to capture expression of the
/// form "A in (aaa, bbb, ccc, ...)."
class ibis::qDiscreteRange : public ibis::qRange {
public:
    /// Construct an empty dicrete range expression.
    qDiscreteRange() : qRange(DRANGE) {};
    qDiscreteRange(const char *col, const char *nums);
    qDiscreteRange(const char *col, const std::vector<uint32_t>& val);
    qDiscreteRange(const char *col, const std::vector<double>& val);
    qDiscreteRange(const char *col, ibis::array_t<uint32_t>& val);
    qDiscreteRange(const char *col, ibis::array_t<double>& val);

    /// Copy constructor.
    qDiscreteRange(const qDiscreteRange& dr)
	: qRange(DRANGE), name(dr.name), values(dr.values) {}
    virtual ~qDiscreteRange() {}; // private variables automatically destructs

    // main access functions
    virtual const char* colName() const {return name.c_str();}
    const ibis::array_t<double>& getValues() const {return values;}
    ibis::array_t<double>& getValues() {return values;}

    /// Duplicate thy self.
    virtual qDiscreteRange* dup() const {return new qDiscreteRange(*this);}
    virtual bool inRange(double val) const;
    virtual void restrictRange(double left, double right);
    virtual bool empty() const {return values.empty();}
    virtual const double& leftBound() const {return values.front();}
    virtual const double& rightBound() const {return values.back();}
    virtual uint32_t nItems() const {return values.size();}

    /// Convert to a sequence of qContinuousRange.
    ibis::qExpr* convert() const;

    virtual void print(std::ostream&) const;
    virtual void printFull(std::ostream& out) const {print(out);}

private:
    std::string name;
    ibis::array_t<double> values; ///< values are sorted.

    qDiscreteRange& operator=(const qDiscreteRange&);
}; // ibis::qDiscreteRange

/// The class qString encapsulates information for comparing string values.
/// Only equality comparison is supported at this point.  It does not
/// ensure the names are valid in any way.  When the check does happen,
/// the left side will be checked first.  If it matches the name of a
/// ibis::column, the right side will be assumed to be the value one is
/// trying to match.  If the left side does not match any know column name,
/// but the right side does, the right side will be assumed to name of
/// column to be searched and the left side will be the value to search
/// against.  If neither matches the name of any column, the expression
/// will evaluate to NULL (i.e., no hit).
class ibis::qString : public ibis::qExpr {
public:
    // construct the qString from two strings
    qString() : qExpr(STRING), lstr(0), rstr(0) {};
    qString(const char* ls, const char* rs);
    virtual ~qString() {delete [] rstr; delete [] lstr;}

    const char* leftString() const {return lstr;}
    const char* rightString() const {return rstr;}

    virtual qString* dup() const {return new qString(*this);}
    virtual void print(std::ostream&) const;
    virtual void printFull(std::ostream& out) const {print(out);}

private:
    char* lstr;
    char* rstr;

    qString(const qString& rhs) : qExpr(STRING),
				  lstr(ibis::util::strnewdup(rhs.lstr)),
				  rstr(ibis::util::strnewdup(rhs.rstr)) {}
    qString& operator=(const qString&);
}; // ibis::qString

/// Representing the operator 'LIKE'.
class ibis::qLike : public ibis::qExpr {
public:
    /// Default constructor.
    qLike() : qExpr(LIKE), lstr(0), rpat(0) {};
    qLike(const char* ls, const char* rs);
    /// Destructor.
    virtual ~qLike() {delete [] rpat; delete [] lstr;}

    /// Name of the column to be searched.
    const char* colName() const {return lstr;}
    /// The string form of the pattern.
    const char* pattern() const {return rpat;}

    virtual qLike* dup() const {return new qLike(*this);}
    virtual void print(std::ostream&) const;
    virtual void printFull(std::ostream& out) const {print(out);}

private:
    /// Column name.
    char* lstr;
    /// Pattern
    char* rpat;

    /// Copy constructor.  Deep copy.
    qLike(const qLike& rhs) : qExpr(LIKE),
			      lstr(ibis::util::strnewdup(rhs.lstr)),
			      rpat(ibis::util::strnewdup(rhs.rpat)) {}
    qLike& operator=(const qLike&);
}; // ibis::qLike

/// The column contains one of the values in a list.  A data structure to
/// hold the string-valued version of the IN expression, name IN ('aaa',
/// 'bbb', ...).
class ibis::qMultiString : public ibis::qExpr {
public:
    qMultiString() : qExpr(MSTRING) {};
    qMultiString(const char *col, const char *sval);
    virtual ~qMultiString() {}; // name and values automatically destroyed

    /// Duplicate the object with the compiler generated copy constructor.
    virtual qMultiString* dup() const {return new qMultiString(*this);}
    virtual void print(std::ostream& out) const;
    virtual void printFull(std::ostream& out) const {print(out);}

    /// Return the column name, the left hand side of the IN operator.
    const char* colName() const {return name.c_str();}
    /// Return the string values in the parentheses as a vector.
    const std::vector<std::string>& valueList() const {return values;}
    /// Convert into a sequence of qString objects.
    ibis::qExpr* convert() const;

private:
    std::string name;
    std::vector<std::string> values;
}; // ibis::qMultiString

namespace ibis {
    /// A namespace for arithmetic expressions.
    namespace math {
	/// Types of terms allowed in the mathematical expressions.
	enum TERM_TYPE {UNDEFINED, VARIABLE, NUMBER, STRING, OPERATOR,
			STDFUNCTION1, STDFUNCTION2,
			CUSTOMFUNCTION1, CUSTOMFUNCTION2};
	/// All supported arithmetic operators.  The word operador is
	/// Spainish for operator.
	enum OPERADOR {UNKNOWN=0, BITOR, BITAND,
		       PLUS, MINUS, MULTIPLY, DIVIDE, REMAINDER, NEGATE, POWER};
	/// List standard 1-argument and 2-argument functions.
	enum STDFUN1 {ACOS=0, ASIN, ATAN, CEIL, COS, COSH, EXP, FABS, FLOOR,
		      FREXP, LOG10, LOG, MODF, SIN, SINH, SQRT, TAN, TANH};
	enum STDFUN2 {ATAN2=0, FMOD, LDEXP, POW};

	/// String form of the operators.
	extern const char* operator_name[];
	/// String form of the one-argument standard functions.
	extern const char* stdfun1_name[];
	/// String form of the two-argument standard functions.
	extern const char* stdfun2_name[];
	/// Whether to keep arithmetic expression as user inputed them.
	/// - If it is true, FastBit will not consolidate constant
	/// expressions nor perform other simply optimizations.
	/// - If it is false, the software will attempt to minimize the
	/// number of actual operations needed to apply them on data
	/// records.
	///
	///  Keep the arithmetic expressions unaltered will preserve its
	/// round-off properties and produce exactly the same numeric
	/// results as one might expect.  However, this is normally not the
	/// most important consideration as the differences are typically
	/// quite small.  Therefore, the default value of this variable is
	/// false.
	extern bool preserveInputExpressions;

	/// All types of terms allowed in a compRange.
	class term : public ibis::qExpr { // abstract term class
	public:
	    virtual ~term() {};

	    virtual TERM_TYPE termType() const = 0;

	    /// Evaluate the term.
	    virtual double eval() const = 0;
	    /// Make a duplicate copy of the term.
	    virtual term* dup() const = 0;
	    /// Print a human readable version of the expression.
	    virtual void print(std::ostream& out) const = 0;
	    /// Shorten the expression by evaluating the constants.  Return
	    /// a new pointer if the expression is changed, otherwise
	    /// return the pointer this.
	    virtual term* reduce() {return this;};

	protected:
	    term() : qExpr(MATHTERM) {}; // used by concrete derived classes
	}; // abstract term

	/// A barrel to hold a list of variables.
	class barrel {
	public:
	    // public member functions
	    barrel() {};
	    barrel(const term* const t) {recordVariable(t);}
	    virtual ~barrel() {}; // member variables clean themselves

	    // access functions to the names and values
	    uint32_t size() const {return varmap.size();}
	    const char* name(uint32_t i) const {return namelist[i];}
	    const double& value(uint32_t i) const {return varvalues[i];}
	    double& value(uint32_t i) {return varvalues[i];}

	    /// Record the variable names appear in the @c term.
	    void recordVariable(const term* const t);
	    /// Record the specified name.  Return the variable number that is
	    /// to be used later in functions @c name and @c value for
	    /// retrieving the variable name and its value.
	    inline uint32_t recordVariable(const char* name);
	    /// Is the given @c barrel of variables equivalent to this one?
	    bool equivalent(const barrel& rhs) const;

	protected:
	    // the data structure to store the variable names in a mathematical
	    // expression
	    typedef std::map< const char*, uint32_t, ibis::lessi > termMap;

	    // functions used by the class variable for accessing values of the
	    // variables
	    friend class variable;
	    double getValue(uint32_t i) const {return varvalues[i];}
	    /// Return the value of the named variable.
	    double getValue(const char* nm) const {
		termMap::const_iterator it = varmap.find(nm);
		if (it != varmap.end()) {
		    uint32_t i = (*it).second;
		    return varvalues[i];
		}
		else {
		    return DBL_MAX;
		}
	    }

	    /// Associate a variable name with a position in @c varvalues and
	    /// @c namelist.
	    termMap varmap;
	    std::vector< double > varvalues; ///< Cast values to double.
	    std::vector< const char* > namelist; ///< List of variable names.
	}; // class barrel

	/// A variable.
	class variable : public term {
	public:
	    // The constructor inserts the variable name to a list in expr and
	    // record the position in private member variable (that is used
	    // later to retrieve value from expr class).
	    variable(const char* var)
		: name(ibis::util::strnewdup(var)), myBar(0), varind(0) {}
	    variable(const variable& v)
		: name(ibis::util::strnewdup(v.name)), myBar(v.myBar),
		  varind(v.varind) {}
	    virtual ~variable() {delete [] name;}

	    virtual TERM_TYPE termType() const {return VARIABLE;}
	    virtual variable* dup() const {return new variable(*this);}
	    virtual double eval() const {return myBar->getValue(varind);}

	    virtual uint32_t nItems() const {return 1U;}
	    virtual void print(std::ostream& out) const {out << name;}
	    const char* variableName() const {return name;}

	    void recordVariable(barrel& bar) const {
		varind = bar.recordVariable(name);
		myBar = &bar;
	    }

	private:
	    char* name;	// the variable name
	    mutable barrel* myBar;// the barrel containing it
	    mutable uint32_t varind;// the token to retrieve value from myBar

	    variable& operator=(const variable&);
	}; // the variable term

	/// A number.
	class number : public term {
	public:
	    number(const char* num) : val(atof(num)) {};
	    number(double v) : val(v) {};
	    virtual ~number() {};

	    virtual TERM_TYPE termType() const {return NUMBER;}
	    virtual number* dup() const {return new number(val);}
	    virtual double eval() const {return val;}

	    virtual uint32_t nItems() const {return 1U;}
	    virtual void print(std::ostream& out) const {out << val;}

	    // to negate the value
	    void negate() {val = -val;}
	    // to invert the value
	    void invert() {val = 1.0/val;}

	private:
	    double val;
	    friend class bediener;
	    friend void ibis::qExpr::simplify(ibis::qExpr*&);
	}; // number

	/// A string literal.
	class literal : public term {
	public:
	    literal(const char* s) : str(ibis::util::strnewdup(s)) {};
	    virtual ~literal() {delete [] str;}

	    virtual TERM_TYPE termType() const {return ibis::math::STRING;}
	    virtual literal* dup() const {return new literal(str);}
	    virtual double eval() const {return 0.0;}

	    virtual uint32_t nItems() const {return 1U;}
	    virtual void print(std::ostream& out) const {out << str;}
	    operator char* () const {return str;}

	private:
	    char* str;

	    literal(const literal&);
	    literal& operator=(const literal&);
	}; // literal

	/// An operator.  Bediener is German for operator.
	class bediener : public term {
	public:
	    bediener(ibis::math::OPERADOR op) : operador(op) {};
	    virtual ~bediener() {};

	    virtual TERM_TYPE termType() const {return OPERATOR;}
	    virtual bediener* dup() const {
		bediener *tmp = new bediener(operador);
		tmp->setRight(getRight()->dup());
		tmp->setLeft(getLeft()->dup());
		return tmp;
	    }
	    virtual double eval() const;
	    virtual void print(std::ostream& out) const;
	    virtual term* reduce();

	private:
	    ibis::math::OPERADOR operador; // Spanish for operator

	    void reorder(); // reorder the tree of operators
	    // place the operands into the list of terms if the operator
	    // matches the specified one.
	    void linearize(const ibis::math::OPERADOR op,
			   std::vector<ibis::math::term*>& terms);
	    // If the right operand is a constant, change operator from - to +
	    // or from / to *.
	    void convertConstants();
	    friend void ibis::qExpr::simplify(ibis::qExpr*&);
	}; // bediener

	/// One-argument standard functions.
	class stdFunction1 : public term {
	public:
	    stdFunction1(const char* name);
	    stdFunction1(const STDFUN1 ft) : ftype(ft) {}
	    virtual ~stdFunction1() {}

	    virtual stdFunction1* dup() const {
		stdFunction1 *tmp = new stdFunction1(ftype);
		tmp->setLeft(getLeft()->dup());
		return tmp;
	    }
	    virtual TERM_TYPE termType() const {return STDFUNCTION1;}
	    virtual double eval() const;
	    virtual void print(std::ostream& out) const;
	    virtual term* reduce();

	private:
	    STDFUN1 ftype;
	}; // stdFunction1

	/// Two-argument standard functions.
	class stdFunction2 : public term {
	public:
	    stdFunction2(const char* name);
	    stdFunction2(const STDFUN2 ft) : ftype(ft) {}
	    virtual ~stdFunction2() {}

	    virtual stdFunction2* dup() const {
		stdFunction2 *tmp = new stdFunction2(ftype);
		tmp->setRight(getRight()->dup());
		tmp->setLeft(getLeft()->dup());
		return tmp;
	    }
	    virtual TERM_TYPE termType() const {return STDFUNCTION2;}
	    virtual double eval() const;
	    virtual void print(std::ostream& out) const;
	    virtual term* reduce();

	private:
	    STDFUN2 ftype;
	}; // stdFunction2
    } // namespace ibis::math
} // namespace ibis

/// The class compRange stores computed ranges.  It is for those
/// comparisons involving nontrivial arithmetic expression.
class ibis::compRange : public ibis::qExpr {
public:

    // construct the range from strings
    compRange() : qExpr(ibis::qExpr::COMPRANGE), expr3(0),
		  op12(ibis::qExpr::OP_UNDEFINED),
		  op23(ibis::qExpr::OP_UNDEFINED) {;}
    compRange(ibis::math::term* me1, COMPARE lop,
	      ibis::math::term* me2)
	: qExpr(ibis::qExpr::COMPRANGE, me1, me2), expr3(0),
	  op12(lop), op23(ibis::qExpr::OP_UNDEFINED) {;}
    compRange(ibis::math::term* me1, ibis::qExpr::COMPARE lop,
	      ibis::math::term* me2, ibis::qExpr::COMPARE rop,
	      ibis::math::term* me3)
	: qExpr(ibis::qExpr::COMPRANGE, me1, me2), expr3(me3),
	  op12(lop), op23(rop) {;}
    // copy constructor -- actually copy the math expressions
    compRange(const compRange& rhs) :
	ibis::qExpr(rhs), expr3(rhs.expr3 ? rhs.expr3->dup() : 0),
	op12(rhs.op12), op23(rhs.op23) {};
    virtual ~compRange() {delete expr3;}

    // provide read access to the operators
    ibis::qExpr::COMPARE leftOperator() const {return op12;}
    ibis::qExpr::COMPARE rightOperator() const {return op23;}
    ibis::math::term* getTerm3() {return expr3;}
    const ibis::math::term* getTerm3() const {return expr3;}
    void setTerm3(ibis::math::term* t) {delete expr3; expr3 = t;}

    /// Duplicate this object and return a pointer to the new copy.
    virtual qExpr* dup() const {return new compRange(*this);}
    /// Evaluate the logical expression.
    inline bool inRange() const;

    virtual uint32_t nItems() const {
	return ibis::qExpr::nItems() +
	    (expr3 != 0 ? expr3->nItems() : 0);}
    /// Print the query expression.
    virtual void print(std::ostream&) const;
    virtual void printFull(std::ostream& out) const {print(out);}

    virtual bool isSimple() const {return isSimpleRange();}
    /// Is this a simple range expression that can be stored as ibis::qRange?
    inline bool isSimpleRange() const;

    /// Is the string a possible simple string comparison.
    bool maybeStringCompare() const;

    // convert a simple expression to qContinuousRange
    ibis::qContinuousRange* simpleRange() const;

private:
    ibis::math::term *expr3;	// the right most expression
    ibis::qExpr::COMPARE op12;	// between qExpr::left and qExpr::right
    ibis::qExpr::COMPARE op23;	// between qExpr::right and expr3
}; // ibis::compRange

/// A join is defined by two names and a numerical expression.  If the
/// numerical expression is not specified, it is a standard equal-join,
/// 'name1 = name2'.  If the numerical expression is specified, it is a
/// range-join, 'name1 between name2 - expr and name2 + expr'.
class ibis::rangeJoin : public ibis::qExpr {
public:
    rangeJoin(const char* n1, const char *n2)
	: ibis::qExpr(ibis::qExpr::JOIN), name1(n1), name2(n2), expr(0) {};
    rangeJoin(const char* n1, const char *n2, ibis::math::term *x) : 
	ibis::qExpr(ibis::qExpr::JOIN), name1(n1), name2(n2), expr(x) {};
    virtual ~rangeJoin() {delete expr;};

    virtual void print(std::ostream& out) const;
    virtual void printFull(std::ostream& out) const {print(out);}
    virtual rangeJoin* dup() const
    {return new rangeJoin(name1.c_str(), name2.c_str(), expr->dup());};

    const char* getName1() const {return name1.c_str();}
    const char* getName2() const {return name2.c_str();}
    ibis::math::term* getRange() {return expr;}
    const ibis::math::term* getRange() const {return expr;}
    void setRange(ibis::math::term *t) {delete expr; expr = t;}

    virtual uint32_t nItems() const {
	return ibis::qExpr::nItems() +
	    (expr != 0 ? expr->nItems() : 0);}

private:
    std::string name1;
    std::string name2;
    ibis::math::term *expr;

    rangeJoin(const rangeJoin&);
    rangeJoin& operator=(const rangeJoin&);
}; // class ibis::rangeJoin

/// A user specifies this type of query expression with the following
/// syntax,
/// @arg any(prefix) = value
/// @arg any(prefix) in (list of values)
/// If any column with the given prefix contains the specified values,
/// the row is considered as a hit.  This is intended to be used in the
/// cases where the @c prefix is actually the name of a set-valued
/// attribute, such as triggerID in STAR datasets.  In this case, the
/// set-valued attribute is translated into a number of columns with the
/// same prefix.  A common query is "does the set contain a particular
/// value?" or "does the set contain a particular set of values?"
class ibis::qAnyAny : public ibis::qExpr {
public:
    qAnyAny() : qExpr(ANYANY) {};
    qAnyAny(const char *pre, const double dbl);
    qAnyAny(const char *pre, const char *val);
    ~qAnyAny() {}; // all data members can delete themselves.

    const char* getPrefix() const {return prefix.c_str();}
    const ibis::array_t<double>& getValues() const {return values;}

    // Use the compiler generated copy constructor to perform duplication.
    virtual qExpr* dup() const {return new qAnyAny(*this);}

    virtual void print(std::ostream& out) const;
    virtual void printFull(std::ostream& out) const {print(out);}

private:
    std::string prefix; ///< The prefix of the column names to search.
    ibis::array_t<double> values; ///< The list of values to match.
}; // class ibis::qAnyAny

inline void ibis::qContinuousRange::foldBoundaries() {
    switch (left_op) {
    case ibis::qExpr::OP_LT:
	lower = floor(lower);
	break;
    case ibis::qExpr::OP_LE:
	lower = ceil(lower);
	break;
    case ibis::qExpr::OP_GT:
	lower = ceil(lower);
	break;
    case ibis::qExpr::OP_GE:
	lower = floor(lower);
	break;
    case ibis::qExpr::OP_EQ:
	if (lower != floor(lower))
	    left_op = ibis::qExpr::OP_UNDEFINED;
	break;
    default:
	break;
    }
    switch (right_op) {
    case ibis::qExpr::OP_LT:
	upper = ceil(upper);
	break;
    case ibis::qExpr::OP_LE:
	upper = floor(upper);
	break;
    case ibis::qExpr::OP_GT:
	upper = floor(upper);
	break;
    case ibis::qExpr::OP_GE:
	upper = ceil(upper);
	break;
    case ibis::qExpr::OP_EQ:
	if (upper != floor(upper))
	    right_op = ibis::qExpr::OP_UNDEFINED;
	break;
    default:
	break;
    }
} //ibis::qContinuousRange::foldBoundaries

inline void ibis::qContinuousRange::foldUnsignedBoundaries() {
    switch (left_op) {
    case ibis::qExpr::OP_LT:
	if (lower >= 0.0) {
	    lower = floor(lower);
	}
	else {
	    left_op = ibis::qExpr::OP_LE;
	    lower = 0.0;
	}
	break;
    case ibis::qExpr::OP_LE:
	if (lower >= 0.0)
	    lower = ceil(lower);
	else
	    lower = 0.0;
	break;
    case ibis::qExpr::OP_GT:
	lower = ceil(lower);
	break;
    case ibis::qExpr::OP_GE:
	lower = floor(lower);
	break;
    case ibis::qExpr::OP_EQ:
	if (lower != floor(lower) || lower < 0.0)
	    left_op = ibis::qExpr::OP_UNDEFINED;
	break;
    default:
	break;
    }
    switch (right_op) {
    case ibis::qExpr::OP_LT:
	upper = ceil(upper);
	break;
    case ibis::qExpr::OP_LE:
	upper = floor(upper);
	break;
    case ibis::qExpr::OP_GT:
	if (upper > 0.0) {
	    upper = floor(upper);
	}
	else {
	    right_op = ibis::qExpr::OP_GE;
	    upper = 0.0;
	}
	break;
    case ibis::qExpr::OP_GE:
	if (upper >= 0.0)
	    upper = ceil(upper);
	else
	    upper = 0.0;
	break;
    case ibis::qExpr::OP_EQ:
	if (upper != floor(upper) || upper < 0.0)
	    right_op = ibis::qExpr::OP_UNDEFINED;
	break;
    default:
	break;
    }
} //ibis::qContinuousRange::foldUnsignedBoundaries

/// The operator< for ibis::qContinuousRange.
inline bool ibis::qContinuousRange::operator<
    (const ibis::qContinuousRange& y) const {
    int cmp = strcmp(colName(), y.colName());
    if (cmp < 0)
	return true;
    else if (cmp > 0)
	return false;
    else if (left_op < y.left_op)
	return true;
    else if (left_op > y.left_op)
	return false;
    else if (right_op < y.right_op)
	return true;
    else if (right_op > y.right_op)
	return false;
    else if (lower < y.lower)
	return true;
    else if (lower > y.lower)
	return false;
    else if (upper < y.upper)
	return true;
    else
	return false;
} // ibis::qContinuousRange::operator<

inline bool ibis::compRange::isSimpleRange() const {
    bool res = false;
    if (expr3 == 0 && getLeft() != 0)
	res = ((static_cast<const ibis::math::term*>(getLeft())->
		termType()==ibis::math::VARIABLE &&
		static_cast<const ibis::math::term*>(getRight())->
		termType()==ibis::math::NUMBER) ||
	       (static_cast<const ibis::math::term*>(getLeft())->
		termType()==ibis::math::NUMBER &&
		static_cast<const ibis::math::term*>(getRight())->
		termType()==ibis::math::VARIABLE));
    else if (expr3 != 0 && expr3->termType()==ibis::math::NUMBER)
	res = (getLeft() == 0 &&
	       static_cast<const ibis::math::term*>(getRight())->termType()
	       == ibis::math::VARIABLE) || 
	    (static_cast<const ibis::math::term*>(getLeft())->
	     termType()==ibis::math::NUMBER &&
	     static_cast<const ibis::math::term*>(getRight())->
	     termType()==ibis::math::VARIABLE);
    return res;
} // ibis::compRange::isSimpleRange

inline bool ibis::compRange::maybeStringCompare() const {
    return (expr3 == 0 && op12==OP_EQ && getLeft() != 0 && getRight() != 0 &&
	    (static_cast<const ibis::math::term*>(getLeft())->termType()
	     ==ibis::math::VARIABLE ||
	     static_cast<const ibis::math::term*>(getLeft())->termType()
	     ==ibis::math::STRING) &&
	    (static_cast<const ibis::math::term*>(getRight())->termType()
	     ==ibis::math::VARIABLE ||
	     static_cast<const ibis::math::term*>(getRight())->termType()
	     ==ibis::math::STRING));
} // ibis::compRange::maybeStringCompare

/// Does the input value match any of the values on record ?
/// It returns false in case of error.
inline bool ibis::compRange::inRange() const {
    if (getRight() == 0) return false;

    const double tm2 =
	static_cast<const ibis::math::term*>(getRight())->eval();
    if (op12 == OP_UNDEFINED && op23 == OP_UNDEFINED)
	return (tm2 != 0.0);

    bool res = true;
    if (getLeft() != 0 && op12 != OP_UNDEFINED) {
	const double tm1 =
	    static_cast<const ibis::math::term*>(getLeft())->eval();
	switch (op12) {
	case OP_LT: res = (tm1 < tm2);  break;
	case OP_LE: res = (tm1 <= tm2); break;
	case OP_GT: res = (tm1 > tm2);  break;
	case OP_GE: res = (tm1 >= tm2); break;
	case OP_EQ: res = (tm1 == tm2); break;
	default:    break;
	}
    }
    if (expr3 != 0 && op23 != OP_UNDEFINED && res == true) {
	const double tm3 = expr3->eval();
	switch (op23) {
	case OP_LT: res = (tm2 < tm3);  break;
	case OP_LE: res = (tm2 <= tm3); break;
	case OP_GT: res = (tm2 > tm3);  break;
	case OP_GE: res = (tm2 >= tm3); break;
	case OP_EQ: res = (tm2 == tm3); break;
	default:    break;
	}
    }
    return res;
} // ibis::compRange::inRange

/// Is the argument @a val one of the values stored ?  Return true or
/// false.
/// It uses a binary search if there are more than 32 elements and uses
/// linear search otherwise.
inline bool ibis::qDiscreteRange::inRange(double val) const {
    if (values.empty()) return false;
    if (val < values[0] || val > values.back()) return false;

    uint32_t i = 0, j = values.size();
    if (j < 32) { // sequential search
	// -- because the heavy branch prediction cost, linear search is
	// more efficient for fairly large range.
	for (i = 0; i < j; ++ i)
	    if (values[i] == val) return true;
	return false;
    }
    else { // binary search
	uint32_t m = (i + j) / 2;
	while (i < m) {
	    if (values[m] == val) return true;
	    if (values[m] < val)
		i = m;
	    else
		j = m;
	    m = (i + j) / 2;
	}
	return (values[m] == val);
    }
} // ibis::qDiscreteRange::inRange

/// Record a variable name and return its position in the list of variables
/// in the @c barrel.
inline uint32_t ibis::math::barrel::recordVariable(const char* name) {
    uint32_t ind = varmap.size();
    termMap::const_iterator it = varmap.find(name);
    if (it == varmap.end()) {
	varmap[name] = ind;
	namelist.push_back(name);
	varvalues.push_back(0.0);
    }
    else {
	ind = (*it).second;
    }
    return ind;
} // ibis::math::barrel::recordVariable

namespace std {
    inline ostream& operator<<(ostream&, const ibis::qExpr&);
    inline ostream& operator<<(ostream&, const ibis::qExpr::COMPARE&);
}

/// Wrap the function print as operator<<.
inline std::ostream& std::operator<<(std::ostream& out, const ibis::qExpr& pn) {
    pn.print(out);
    return out;
} // std::operator<<

/// Print a comparison operator.
inline std::ostream& std::operator<<(std::ostream& out,
				     const ibis::qExpr::COMPARE& op) {
    switch (op) {
    default:
    case ibis::qExpr::OP_UNDEFINED:
	out << "??"; break;
    case ibis::qExpr::OP_LT:
	out << "<"; break;
    case ibis::qExpr::OP_LE:
	out << "<="; break;
    case ibis::qExpr::OP_GT:
	out << ">"; break;
    case ibis::qExpr::OP_GE:
	out << ">="; break;
    case ibis::qExpr::OP_EQ:
	out << "=="; break;
    }
    return out;
} // std::operator<<
#endif // IBIS_EXPR_H

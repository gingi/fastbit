//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2008 the Regents of the University of California
#ifndef IBIS_QUERY_H
#define IBIS_QUERY_H
///@file
/// The header file defining the individual query objects.
///
#include "part.h"	// class part

/// A data structure for representing user queries.  This is the primary
/// entry for user to take advantage of bitmap indexing facilities.  A
/// query is a very limited version of the SQL SELECT statement.  It is
/// only defined on one table and it takes a where clause and a select
/// clause.  The where clause is mandatory.  It contains a list of range
/// conditions joined together with logical operators, such as "temperature
/// > 700 and 100 <= presessure < 350".  Records whose attribute values
/// satisfy the conditions defined in the where clause is considered hits.
/// A query may retrieve values of variables/columns specified in the
/// select clause.  A select clause is optional.  If specified, it contains
/// a list of column names.  These attributes must not be NULL in order for
/// a record to be a hit.  The select clause may also contain column names
/// appearing as the argument to one of the four functions: @c avg, @c max,
/// @c min and @c sum.  For example, "temperature, pressure,
/// average(ho2_concentration)" may be a select statement for a Chemistry
/// application.
///
/// The hits can be computed in two ways by using functions @c estimate or
/// @c evaluate.  The function @c estimate can take advantage of the
/// indices to give two approximate solutions, one as an upper bound and
/// the other as a lower bound.  The bitmap indices will be automatically
/// built according to the specification if they are not present.  The
/// accuracy of the bounds depend on the nature of the indices available.
/// If no index can be constructed, the lower bound would be empty and the
/// upper bound would include every record.  When the function @c evaluate
/// is called, the exact solution is computed no matter whether the function
/// @c estimate has been called or not.  The solution produced is recorded
/// as a bit vector.  The user may use ibis::bitvector::indexSet to
/// extract the record numbers of the hits or use one of the functions @c
/// getQualifiedInts, @c getQualifiedFloats, and @c getQualifiedDoubles to
/// retrieve the values of the selected attributes.
/// Additionally, one may call either @c printSelected or @c
/// printSelectedWithRID to print the selected values to the specified I/O
/// stream.
class FASTBIT_CXX_DLLSPEC ibis::query {
public:
    enum QUERY_STATE {
	UNINITIALIZED,	//< The query object is currently empty.
	SET_COMPONENTS,	//< The query object has a select clause.
	SET_RIDS,	//< The query object contains a list of RIDs.
	SET_PREDICATE,	//< The query object has a where clause.
	SPECIFIED,	//< SET_COMPONENTS & (SET_RIDS | SET_PREDICATE).
	QUICK_ESTIMATE, //< A upper and a lower bound are computed.
	FULL_EVALUATE,	//< The exact hits are computed.
	BUNDLES_TRUNCATED,	//< Only top-K results are stored.
	HITS_TRUNCATED	//< The hit vector has been updated to match bundles.
    };

    virtual ~query();
    /// Constructor.  Reconstructs query from stored information in the
    /// named directory @c dir.  This is only used for recovering from
    /// program crashes.
    query(const char* dir, const ibis::partList& tl);
    /// Constructor.  Generates a new query on the given table et.
    query(const char* uid=0, const part* et=0, const char* pref=0);

    /// Functions about the identity of the query
    const char* id() const {return myID;};	///< The query token.
    const char* dir() const {return myDir;}	///< For persistent data
    const char* userName() const {return user;} ///< User started the query
    /// The time stamp on the data used to process the query.
    time_t timestamp() const {return dstime;}
    /// Return the pointer to the data table used to process the query.
    const part* partition() const {return table0;}
    /// Return a list of names specified in the select clause.
    const selected& components() const {return comps;};

    /// Specifies a list of Row IDs for the query object to retrieve the
    /// records. 
    int setRIDs(const RIDSet& set);
    /// Specify the where clause in string form.
    int setWhereClause(const char *str);
    /// Specify the where clause as a set of conjunctive ranges.
    int setWhereClause(const std::vector<const char*>& names,
		       const std::vector<double>& lbounds,
		       const std::vector<double>& rbounds);
    /// Specify the where clause through a qExpr object.
    int setWhereClause(const ibis::qExpr* qexp);
    /// Specifies the select clause for the query.  The select clause is a
    /// string of attribute names (plus the four predefined functions, @c
    /// avg, @c max, @c min and @c sum) separated by spaces, commas (,) or
    /// semicolons(;).  Repeated calls to this function simply overwrite
    /// the previous definition of the select clause.  If no select clause
    /// is specified, the where clause alone determines whether record is a
    /// hit or not.  The select clause will be reordered to make the plain
    /// column names without functions appear before with functions.
    virtual int setSelectClause(const char *str);
    /// Resets the table used to evaluate the query conditions to the table
    /// specified in the argument.
    int setTable(const ibis::part* tbl);
    /// Return the where clause string.
    virtual const char* getWhereClause() const {return condition;}
    /// Return the select clause string.
    virtual const char* getSelectClause() const {return *comps;}

    /// Expands where clause to preferred bounds.  This is to make sure the
    /// function estimate will give exact answer.  It does nothing if there
    /// is no preferred bounds in the indices.
    void expandQuery();
    /// Contracts where clause to preferred bounds. Similar to function
    /// exandQuery, but makes the bounds of the range conditions narrower
    /// rather than wider.
    void contractQuery();

    /// Return a const pointer to the copy of the user supplied RID set.
    const RIDSet* getUserRIDs() const {return rids_in;}

    /// Separate out the sub-expressions that are not simple.  This is
    /// intended to allow the overall where clause to be evaluated in
    /// separated steps, where the simple conditions are left for this
    /// software to handle and the more complex ones are to be handled by
    /// another software.  The set of conditions remain with this query
    /// object and the conditions returned by this function are assumed to
    /// be connected with the operator AND.  If the top-most operator in
    /// the WHERE clause is not an AND operator, the whole clause will be
    /// returned if it contains any conditions that is not simple,
    /// otherwise, an empty string will be returned.
    std::string removeComplexConditions();

    // Functions to perform estimation.

    /// Functions to perform estimation and retrieve range of hits Computes
    /// a lower and an upper bound of hits.  This is done by using the
    /// indices.  If possible it will build new indices.  The lower bound
    /// contains only records that are hits and the upper bound contains all
    /// hits but may also contain some records that are not hits.
    /// Returns 0 for success, a negative value for error.
    int estimate();
    /// Return the number of records in the lower bound.
    long getMinNumHits() const;
    /// Return the number of records in the upper bound.
    long getMaxNumHits() const;

    // Functions related to full evaluation.

    /// Computes the exact hits.  The same answer shall be computed whether
    /// there is any index or not.  The argument evalSelect indicates
    /// whether the select clause should be evaluated at the same time.  If
    /// its value is true, the columns specified in the select clause
    /// will be retrieved from disk and stored in the temporary location for
    /// this query.  If not, the qualified values will be retrieved from disk
    /// when one of getRIDs, getQualifiedInts, getQualifiedFloats, and
    /// getQualifiedDoubles is issued.  In the later case, only the
    /// specified column is retrieved.  In addition, the values of
    /// column at the time of the function are read, which can be
    /// potentially different different from the time when the function
    /// evaluate was called.
    ///
    /// Returns 0 for success, a negative value for error.
    ///
    /// @see getQualifiedInts
    int evaluate(const bool evalSelect=false);
    /// Return the pointer to the internal hit vector.  The user should NOT
    /// attempt to free the returned pointer.
    const ibis::bitvector* getHitVector() const {return hits;}
    /// Return the number of records in the exact solution.
    long getNumHits() const;
    /// Count the number of hits.  Don't generate the hit vector if not
    /// already there.
    long countHits() const;
    /// Return the list of row IDs of the hits.  The user is responsible
    /// for freeing the pointer.
    /// @see getQualifiedInts
    RIDSet* getRIDs() const;
    /// Return a list of row ids that match the mask.  The user is
    /// responsible for freeing the pointer.
    /// @see getQualifiedInts
    RIDSet* getRIDs(const ibis::bitvector& mask) const;
    /// Return the list of row IDs of the hits within the specified bundle.
    const RIDSet* getRIDsInBundle(const uint32_t bid) const;

    /// Re-order the bundles according the the new "ORDER BY"
    /// specification.  It returns 0 if it completes successfully.  It
    /// returns a negative number to indicate error.
    /// If @c direction >= 0, sort the values in ascending order,
    /// otherwise, sort them in descending order.
    int orderby(const char *names, int direction) const;

    /// Truncate the bundles to provide the top-K rows of the bundles.  It
    /// returns the number of results kept, which is the smaller of the
    /// current number of bundles and the input argument @c keep.  A
    /// negative value is returned in case of error, e.g., query has not
    /// been fully specified.  If the second argument is true, the internal
    /// hit vector is updated to match the truncated solution.  Otherwise,
    /// the internal hit vector is left unchanged.  Since the functions
    /// getNumHits and getQualifiedXXX uses this internal hit vector, it is
    /// generally a good idea to update the hit vector.  On the other hand,
    /// one may wish to avoid this update if the hit vector is not used in
    /// any way.
    long int limit(const char *names, int direction, uint32_t keep,
		   bool updateHits = true);

    /// The functions @c getQualifiedXXX return the values of selected
    /// columns in the records that satisfies the specified conditions.
    /// The caller must call the operator @c delete to free the pointers
    /// returned.
    ///
    /// @note
    /// Any column of the table may be specified, not just those given in
    /// the select clause.  The content returned is read from disk when
    /// these functions are called, which may be different from their
    /// values when the function @c evaluate was called.  In other word,
    /// they may be inconsistent with the conditions specified in the where
    /// clause.  For append-only data, this is NOT an issue.
    ///
    /// The above caveat also applies to the two versions of getRIDs.
    array_t<int32_t>* getQualifiedInts(const char* column_name);
    array_t<uint32_t>* getQualifiedUInts(const char* column_name);
    array_t<float>* getQualifiedFloats(const char* column_name);
    array_t<double>* getQualifiedDoubles(const char* column_name);

    /// Print the values of the selected columns to the specified output
    /// stream.  The printed values are grouped by the columns without
    /// functions.  For each group, the functions are evaluated on the
    /// columns named in the function.  This is equivalent to have implicit
    /// "GROUP BY" and "ORDER BY" keywords on all columns appears without a
    /// function in the select clause.
    void printSelected(std::ostream& out) const;
    /// Print the values of the columns in the select clause without
    /// functions.  One the groups of unique values are printed.  For each
    /// group, the row ID (RID) of the rows are also printed.
    void printSelectedWithRID(std::ostream& out) const;

    /// Return a (new) bitvector that contains the result of directly scan
    /// the raw data to determine what records satisfy the user specified
    /// conditions.  It is mostly used for testing purposes.  It can be
    /// called any time after the where clause is set, and does not change
    /// the state of the current query.
    long sequentialScan(ibis::bitvector& bv) const;

    /// Get a bitvector containing all rows satisfying the query
    /// condition. The resulting bitvector inculdes both active rows and
    /// inactive rows.
    long getExpandedHits(ibis::bitvector&) const;

    // used by ibis::bundle
    RIDSet* readRIDs() const;
    void writeRIDs(const RIDSet* rids) const;

    /// Used to print information about the progress or state of query
    /// processing.  It prefixes each message with a query token.
    void logMessage(const char* event, const char* fmt, ...) const;

    // Functions for cleaning up, retrieving query states
    // and error messages.

    /// Releases the resources held by the query object and re-initialize
    /// the select clause and the where clause to blank.
    void clear();
    /// Return the current state of query.
    QUERY_STATE getState() const;
    /// Return the last error message recorded internally.
    const char* getLastError() const {return lastError;}
    /// Reset the last error message to blank.
    void clearErrorMessage() const {*lastError=0;}

    /// Is the given string a valid query token.  Return true if it has the
    /// expected token format, otherwise false.
    static bool isValidToken(const char* tok);
    /// Length of the query token.
    // *** the value 16 is hard coded in functions newToken and ***
    // *** isValidToken ***
    static unsigned tokenLength() {return 16;}

    /// Tell the destructor to remove all stored information about queries.
    static void removeQueryRecords()
    {ibis::gParameters().add("query.purgeTempFiles", "true");}
    /// Tell the destructor to leave stored information on disk.
    static void keepQueryRecords()
    {ibis::gParameters().add("query.purgeTempFiles", "false");}

    class result; // Forward declaration only

protected:
    char* user; 	///< Name of the user who specified the query
    char* condition;	///< Query condition (string)
    selected comps;	///< Names of selected components
    QUERY_STATE state;	///< Status of the query
    ibis::bitvector* hits;///< Solution in bitvector form (or lower bound)
    ibis::bitvector* sup;///< Estimated upper bound
    mutable ibis::part::readLock* dslock;	///< A read lock on the table0
    mutable char lastError[MAX_LINE+PATH_MAX];	///< The warning/error message

    void logError(const char* event, const char* fmt, ...) const;
    void logWarning(const char* event, const char* fmt, ...) const;

    void reorderExpr(); // reorder query expression

    bool hasBundles() const;
    int  verifyPredicate(qExpr*& qexpr);
    int  computeHits();	  // generate the hit vector for range queries
    void getBounds();	  // get the upper and lower bounds for range queries
    // use index only to come up with a upper bound and a lower bound
    void doEstimate(const qExpr* term, ibis::bitvector& low,
		    ibis::bitvector& high) const;
    // assume the mask contains information from the indexes already, read the
    // data table to resolve the query
    int doScan(const qExpr* term, const ibis::bitvector& mask,
	       ibis::bitvector& hits) const;
    // only read the data table to resolve the query
    int doScan(const qExpr* term, ibis::bitvector& hits) const;
    // attempt evaluate one predicate condition at a time by using both
    // index and raw data table
    int doEvaluate(const qExpr* term, ibis::bitvector& hits) const;
    int doEvaluate(const qExpr* term, const ibis::bitvector& mask,
		   ibis::bitvector& hits) const;

    /// Process the join operation and return the number of pairs.
    int64_t processJoin();
    /// Add constraints derived from domains of the two join columns.
    void addJoinConstraints(ibis::qExpr*& exp0) const;

    // read/write the basic information about the query and its state
    virtual void writeQuery();
    void readQuery(const ibis::partList& tl);
    void removeFiles(); // remove the files written by this object

    // read/write the results of the query processing (hits, fids, rids,
    // file bundles)
    void readHits();
    void writeHits() const;
    void printRIDs(const RIDSet& ridset) const;
    // count the number of pages might be accessed to retrieve every value
    // in the hit vector
    uint32_t countPages(unsigned wordsize) const;

    // expand/contract query expression
    int doExpand(ibis::qExpr* exp0) const;
    int doContract(ibis::qExpr* exp0) const;

    // A group of functions to count the number of pairs
    // satisfying the join conditions.
    int64_t sortJoin(const std::vector<const ibis::rangeJoin*>& terms,
		     const ibis::bitvector& mask) const;
    int64_t sortJoin(const ibis::rangeJoin& cmp,
		     const ibis::bitvector& mask) const;
    int64_t sortEquiJoin(const ibis::rangeJoin& cmp,
			 const ibis::bitvector& mask) const;
    int64_t sortRangeJoin(const ibis::rangeJoin& cmp,
			  const ibis::bitvector& mask) const;
    int64_t sortEquiJoin(const ibis::rangeJoin& cmp,
			 const ibis::bitvector& mask,
			 const char* pairfile) const;
    int64_t sortRangeJoin(const ibis::rangeJoin& cmp,
			  const ibis::bitvector& mask,
			  const char* pairfile) const;
    void orderPairs(const char* pairfile) const;
    int64_t mergePairs(const char* pairfile) const;

    template <typename T1, typename T2>
    int64_t countEqualPairs(const array_t<T1>& val1,
			    const array_t<T2>& val2) const;
    template <typename T1, typename T2>
    int64_t countDeltaPairs(const array_t<T1>& val1,
			    const array_t<T2>& val2, const T1& delta) const;
    template <typename T1, typename T2>
    int64_t recordEqualPairs(const array_t<T1>& val1,
			     const array_t<T2>& val2,
			     const array_t<uint32_t>& ind1,
			     const array_t<uint32_t>& ind2,
			     const char* pairfile) const;
    template <typename T1, typename T2>
    int64_t recordDeltaPairs(const array_t<T1>& val1,
			     const array_t<T2>& val2,
			     const array_t<uint32_t>& ind1,
			     const array_t<uint32_t>& ind2,
			     const T1& delta, const char* pairfile) const;

    // functions for access control
    void gainReadAccess(const char* mesg) const {
	if (ibis::gVerbose > 10)
	    logMessage("gainReadAccess", "acquiring a read lock for %s",
		       mesg);
	if (0 != pthread_rwlock_rdlock(&lock))
	    logMessage("gainReadAccess",
		       "unable to gain read access to rwlock for %s", mesg);
    }
    void gainWriteAccess(const char* mesg) const {
	if (ibis::gVerbose > 10)
	    logMessage("gainWriteAccess", "acquiring a write lock for %s",
		       mesg);
	if (0 != pthread_rwlock_wrlock(&lock))
	    logMessage("gainWriteAccess",
		       "unable to gain write access to rwlock for %s", mesg);
    }
    void releaseAccess(const char* mesg) const {
	if (ibis::gVerbose > 10)
	    logMessage("releaseAccess", "releasing rwlock for %s", mesg);
	if (0 != pthread_rwlock_unlock(&lock))
	    logMessage("releaseAccess", "unable to unlock the rwlock for %s",
		       mesg);
    }

    // these two simple class are needed in order to ensure locks are
    // properly released even if the functions using them do not properly
    // terminate in particular, if the caller encounters an exception, the
    // destructor will be called automatically to ensure the locks are
    // released.
    class readLock {
    public:
	readLock(const query* q, const char* m) : theQuery(q), mesg(m) {
	    theQuery->gainReadAccess(m);
	};
	~readLock() {theQuery->releaseAccess(mesg);}
    private:
	const query* theQuery;
	const char* mesg;

	readLock() {}; // no default constructor
	readLock(const readLock&) {}; // can not copy
    };

    class writeLock {
    public:
	writeLock(const query* q, const char* m) : theQuery(q), mesg(m) {
	    theQuery->gainWriteAccess(m);
	};
	~writeLock() {theQuery->releaseAccess(mesg);}
    private:
	const query* theQuery;
	const char* mesg;

	writeLock() {}; // no default constructor
	writeLock(const writeLock&) {}; // can not copy
    };
    friend class readLock;
    friend class writeLock;

    // a class to be used for reordering the terms in the where clauses
    class weight : public ibis::qExpr::weight {
    public:
	virtual double operator()(const ibis::qExpr* ex) const;
	weight(const ibis::part* ds) : dataset(ds) {}

    private:
	const ibis::part* dataset;
    };

private:
    char* myID; 	// The unique ID of this query object
    char* myDir;	// Name of the directory containing the query record
    RIDSet* rids_in;	// Rid list specified in an RID query
    qExpr* expr;	// Query expression (pointer to the root of the tree)
    const part* table0;	// Default table used to process the query
    time_t dstime;		// When query evaluation started
    mutable pthread_rwlock_t lock; // Rwlock for access control

    // private functions
    static char* newToken(const char*); ///< Generate a new unique token.
    /// Determine a directory for storing information about the query.
    void setMyDir(const char *pref);

    query(const query&);
    const query& operator=(const query&);
}; // class ibis::query

namespace ibis {
    ///@{
    /// This is an explicit specialization of a protected member of
    /// ibis::query class.
    /// @note The C++ language rules require explicit specialization of
    /// template member function be declared in the namespace containing
    /// the function, not inside the class!  This apparently causes them to
    /// be listed as public functions in Doxygen document.
    template <>
    int64_t query::countEqualPairs(const array_t<int32_t>& val1,
				   const array_t<uint32_t>& val2) const;
    template <>
    int64_t query::countEqualPairs(const array_t<uint32_t>& val1,
				   const array_t<int32_t>& val2) const;
    template <>
    int64_t query::countDeltaPairs(const array_t<int32_t>& val1,
				   const array_t<uint32_t>& val2,
				   const int32_t& delta) const;
    template <>
    int64_t query::countDeltaPairs(const array_t<uint32_t>& val1,
				   const array_t<int32_t>& val2,
				   const uint32_t& delta) const;
    template <>
    int64_t query::recordEqualPairs(const array_t<int32_t>& val1,
				    const array_t<uint32_t>& val2,
				    const array_t<uint32_t>& ind1,
				    const array_t<uint32_t>& ind2,
				    const char *pairfile) const;
    template <>
    int64_t query::recordEqualPairs(const array_t<uint32_t>& val1,
				    const array_t<int32_t>& val2,
				    const array_t<uint32_t>& ind1,
				    const array_t<uint32_t>& ind2,
				    const char *pairfile) const;
    template <>
    int64_t query::recordDeltaPairs(const array_t<int32_t>& val1,
				    const array_t<uint32_t>& val2,
				    const array_t<uint32_t>& ind1,
				    const array_t<uint32_t>& ind2,
				    const int32_t& delta,
				    const char *pairfile) const;
    template <>
    int64_t query::recordDeltaPairs(const array_t<uint32_t>& val1,
				    const array_t<int32_t>& val2,
				    const array_t<uint32_t>& ind1,
				    const array_t<uint32_t>& ind2,
				    const uint32_t& delta,
				    const char *pairfile) const;
    ///@}
}
#endif // IBIS_QUERY_H

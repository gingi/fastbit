//File: $Id$
// Author: John Wu <John.Wu@ACM.org>
// Copyright 2000-2008 the Regents of the University of California
#ifndef IBIS_COLUMN_H
#define IBIS_COLUMN_H
///@file
/// Define the class column.
///
/// A column of a relational table is also known as an attribute of a
/// relation.  In IBIS, columns are stored separate from each other.  This
/// storage strategy is commonly known as vertical partitioning.
///
#include "table.h"	// ibis::TYPE_T
#include "qExpr.h"	// ibis::qContinuousRange
#include "bitvector.h"
#include <string>

namespace ibis { // additional names to the namespace ibis
    // derived classes of ibis::column, implemented in category.cpp
    class category; // for categorical values (low-cardinality text fields)
    class text;     // arbitrary cardinality text fields
    class dictionary; // maps string to integer, use by text and category

    // the following are used for storing selected values of different types
    // of columns (implemented in colValues.cpp)
    class colInts;
    class colUInts;
    class colLongs;
    class colULongs;
    class colFloats;
    class colDoubles;
} // namespace

/// The class to represent a column of a data partition.  FastBit
/// represents user data as tables (each table may be divided into multiple
/// partitions) where each table consists of a number of columns.
/// Internally, the data values for each column is stored separated from
/// others.  In relational algebra terms, this is equivalent to projecting
/// out each attribute of a relation separately.  It increases the
/// efficiency of searching on relatively small number of attributes
/// compared to the horizontal data organization used in typical relational
/// database systems.
class FASTBIT_CXX_DLLSPEC ibis::column {
public:

    virtual ~column();
    /// Reconstitute a column from the content of a file.
    column(const part* tbl, FILE* file);
    /// Construct a new column of specified type.
    column(const part* tbl, ibis::TYPE_T t, const char* name,
	   const char* desc="", double low=DBL_MAX, double high=-DBL_MAX);
    column(const column& rhs); ///< copy constructor

    // simple access functions
    ///@note Name and type can not be changed.
    ibis::TYPE_T type() const {return m_type;}
    const char* name() const {return m_name.c_str();}
    const char* description() const {return m_desc.c_str();}
    const double& lowerBound() const {return lower;}
    const double& upperBound() const {return upper;}
    inline int elementSize() const; //< Size of a data element in bytes.
    inline bool isFloat() const; //< Is it floating-point values?
    inline bool isInteger() const; //< Is it an integer values?
    inline bool isNumeric() const; //< Is it a numberical value?
    void description(const char* d) {m_desc = d;}
    void lowerBound(double d) {lower = d;}
    void upperBound(double d) {upper = d;}
    const part* partition() const {return thePart;}

    // function related to index/bin
    const char* indexSpec() const; ///< Retrieve the index specification.
    uint32_t numBins() const; ///< Retrieve the number of bins used.
    /// Set the index specification.
    void indexSpec(const char* spec) {m_bins=spec;}
    /// Retrive the bin boundaries if the index currently in use.
    void preferredBounds(std::vector<double>&) const;
    /// Retrive the number of rows in each bin.
    void binWeights(std::vector<uint32_t>&) const;

    /// Compute the actual min/max values by actually going through all the
    /// values.  This function reads the data in the active data directory
    /// and modifies the member variables to record the actual min/max.
    virtual void computeMinMax();
    virtual void computeMinMax(const char *dir);
    /// Compute the actual min/max of the data in directory @c dir.  Report
    /// the actual min/max found back through output arguments @c min and
    /// @c max.
    virtual void computeMinMax(const char *dir,
			       double& min, double &max) const;

    /// Load the index associated with the column.
    virtual void loadIndex(const char* opt=0) const throw ();
    /// Unload the index associated with the column.
    void unloadIndex() const;
    /// Compute the index size (in bytes).
    virtual long indexSize() const;
    /// Perform a set of built-in tests to determine the speed of common
    /// operations.
    void indexSpeedTest() const;
    /// Purge the index files assocated with the current column.
    void purgeIndexFile(const char *dir=0) const;

    /// Name of the data file in the given data directory.  If the
    /// directory name is not given, the directory is assumed to be the
    /// current data directory of the data partition.
    const char* dataFileName(std::string& fname, const char *dir=0) const;
    /// Name of the NULL mask file.
    const char* nullMaskName(std::string& fname) const;
    void  getNullMask(bitvector& mask) const;

    /// Return the string value for the <code>i</code>th row.  Only valid
    /// for ibis::text and ibis::category.
    ///@ref ibis::text
    virtual void getString(uint32_t i, std::string &val) const {};
    /// Determine if the input string is one of the records.  If yes,
    /// return the pointer to the incoming string, otherwise return nil.
    virtual const char* findString(const char* str) const
    {return static_cast<const char*>(0);}

    /// Return all rows of the column as an array_t object. Caller is
    /// responsible for deleting the returned object.
    array_t<int32_t>* getIntArray() const;
    /// @sa getIntArray
    array_t<float>*   getFloatArray() const;
    /// @sa getIntArray
    array_t<double>*  getDoubleArray() const;
    ibis::fileManager::storage* getRawData() const;
    template <typename T> int getRawData(array_t<T>& vals) const;

    /// Return selected rows of the column as an array_t object.  Caller is
    /// responsible for deleting the returned object.
    virtual array_t<char>*     selectBytes(const bitvector& mask) const;
    virtual array_t<unsigned char>* selectUBytes(const bitvector& mask) const;
    virtual array_t<int16_t>*  selectShorts(const bitvector& mask) const;
    virtual array_t<uint16_t>* selectUShorts(const bitvector& mask) const;
    virtual array_t<int32_t>*  selectInts(const bitvector& mask) const;
    virtual array_t<int64_t>*  selectLongs(const bitvector& mask) const;
    virtual array_t<uint64_t>* selectULongs(const bitvector& mask) const;
    virtual array_t<float>*    selectFloats(const bitvector& mask) const;
    virtual array_t<double>*   selectDoubles(const bitvector& mask) const;
    virtual std::vector<std::string>*
	selectStrings(const bitvector& mask) const {return 0;}
    virtual array_t<uint32_t>* selectUInts(const bitvector& mask) const;
    template <typename T>
    long selectValues(const bitvector& mask, array_t<T>& vals) const;
    template <typename T>
    long selectValues(const bitvector& mask,
		      array_t<T>& vals, array_t<uint32_t>& inds) const;

    virtual void write(FILE* file) const; // write the TDC entry
    virtual void print(std::ostream& out) const; // print header info
    void logMessage(const char* event, const char* fmt, ...) const;
    void logWarning(const char* event, const char* fmt, ...) const;

    /// expand/contract range condition so that the new ranges fall exactly
    /// on the bin boundaries
    int expandRange(ibis::qContinuousRange& rng) const;
    int contractRange(ibis::qContinuousRange& rng) const;

    /// Compute a lower bound and an upper bound on the number of hits
    /// using the bitmap index.  If no index is available a new one will be
    /// built.  If no index can be built, the lower bound will contain
    /// nothing and the the upper bound will contain everything.  The two
    /// bounds are returned as bitmaps which marked the qualified rows as
    /// one, where the lower bound is stored in 'low' and the upper bound
    /// is stored in 'high'.  If the bitvector 'high' has less bits than
    /// 'low', the bitvector 'low' is assumed to have an exact solution.
    /// This function always returns zero (0).
    virtual long estimateRange(const ibis::qContinuousRange& cmp,
			       ibis::bitvector& low,
			       ibis::bitvector& high) const;
    virtual long estimateRange(const ibis::qDiscreteRange& cmp,
			       ibis::bitvector& low,
			       ibis::bitvector& high) const;

    /// Attempt to compute the exact answer.  If successful, return the
    /// number of hits, otherwise return a negative value.
    virtual long evaluateRange(const ibis::qContinuousRange& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;
    virtual long evaluateRange(const ibis::qDiscreteRange& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;

    /// Use an index to compute an upper bound on the number of hits.  If
    /// no index can be computed, it will return the number of rows as the
    /// upper bound.
    virtual long estimateRange(const ibis::qContinuousRange& cmp) const;
    virtual long estimateRange(const ibis::qDiscreteRange& cmp) const;

    /// Estimate the cost of evaluate the query expression.
    virtual double estimateCost(const ibis::qContinuousRange& cmp) const;
    virtual double estimateCost(const ibis::qDiscreteRange& cmp) const;
    virtual double estimateCost(const ibis::qString& cmp) const {
	return 0;}
    virtual double estimateCost(const ibis::qMultiString& cmp) const {
	return 0;}

    /// Compute the locations of the rows can not be decided by the index.
    /// Returns the fraction of rows might satisfy the specified range
    /// condition.
    virtual float getUndecidable(const ibis::qContinuousRange& cmp,
				 ibis::bitvector& iffy) const;
    virtual float getUndecidable(const ibis::qDiscreteRange& cmp,
				 ibis::bitvector& iffy) const;

    /// Append new data in directory df to the end of existing data in dt.
    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, const uint32_t nbuf, char* buf);

    /// Record the content in array va1 to directory dir.  Extend the mask.
    virtual long writeData(const char* dir, uint32_t nold, uint32_t nnew,
			   ibis::bitvector& mask, const void *va1,
			   const void *va2=0);

    virtual long saveSelected(const ibis::bitvector& sel, const char *dest,
			      char *buf, uint32_t nbuf);

    /// truncate the number of data entries in the named dir to @c nent.
    /// Adjust the null mask accordingly.
    long truncateData(const char* dir, uint32_t nent,
		      ibis::bitvector& mask) const;

    // A group of functions to compute some basic statistics for the
    // attribute values.

    /// Compute the actual minimum value by reading the data or examining
    /// the index.  It returns DBL_MAX in case of error.
    double getActualMin() const;
    /// Compute the actual maximum value by reading the data or examining
    /// the index.  It returns -DBL_MAX in case of error.
    double getActualMax() const;
    /// Compute the sum of all values by reading the data.
    double getSum() const;
    /// Compute the actual data distribution.  It will generate an index
    /// for the column if one is not already available.  The value in @c
    /// cts[i] is the number of values less than @c bds[i].  If there is no
    /// NULL values in the column, the array @c cts will start with 0 and
    /// and end the number of rows in the data.  The array @c bds will end
    /// with a value that is greater than the actual maximum value.
    long getCumulativeDistribution(std::vector<double>& bounds,
				   std::vector<uint32_t>& counts) const;
    /// Count the number of records in each bin.  The array @c bins
    /// contains bin boundaries that defines the following bins:
    /// @code
    ///    (..., bins[0]) [bins[0], bins[1]) ... [bins.back(), ...).
    /// @endcode
    /// Because of the two open bins at the end, N bin boundaries defines
    /// N+1 bins.  The array @c counts has one more element than @c bins.
    /// This function returns the number of bins.  If this function was
    /// executed successfully, the return value should be the same as the
    /// size of array @c counts, and one larger than the size of array @c
    /// bbs.
    long getDistribution(std::vector<double>& bbs,
			 std::vector<uint32_t>& counts) const;

    class info;
    class indexLock;
    class mutexLock;

protected:
    // protected member variables
    const part* thePart;
    ibis::bitvector mask_; // the entries marked 1 are valid
    ibis::TYPE_T m_type;
    std::string m_name;
    std::string m_desc;
    std::string m_bins;
    double lower;
    double upper;
    /// The index for this column.  It is not consider as a must-have member.
    mutable ibis::index* idx;

    /// Print messages started with "Error" and throw a string exception.
    void logError(const char* event, const char* fmt, ...) const;
    /// Convert strings in the opened file to a list of integers with the
    /// aid of a dictionary.
    long string2int(int fptr, dictionary& dic, uint32_t nbuf, char* buf,
		    array_t<uint32_t>& out) const;
    /// Read the data values and compute the minimum value.
    double computeMin() const;
    /// Read the base data to compute the maximum value.
    double computeMax() const;
    /// Read the base data to compute the total sum.
    double computeSum() const;
    /// Given the name of the data file, compute the actual minimum and the
    /// maximum value.
    void actualMinMax(const char *fname, const ibis::bitvector& mask,
		      double &min, double &max) const;
    template <typename T>
    void actualMinMax(const array_t<T>& vals, const ibis::bitvector& mask,
		      double& min, double& max) const;
    template <typename T>
    T computeMin(const array_t<T>& vals,
		 const ibis::bitvector& mask) const;
    template <typename T>
    T computeMax(const array_t<T>& vals,
		 const ibis::bitvector& mask) const;
    template <typename T>
    double computeSum(const array_t<T>& vals,
		      const ibis::bitvector& mask) const;

    class readLock;
    class writeLock;
    class softWriteLock;
    friend class readLock;
    friend class writeLock;
    friend class indexLock;
    friend class mutexLock;
    friend class softWriteLock;

private:
    // these are for tracking idx only and are accessed through indexLock
    // and writeLock
    mutable pthread_rwlock_t rwlock;
    mutable pthread_mutex_t mutex;
    mutable uint32_t idxcnt; // number of functions who want to use index

    const column& operator=(const column&);
}; // ibis::column

/// Some basic information about a column.  Can only be used if the
/// original column used to generate the info object exists in memory.
class FASTBIT_CXX_DLLSPEC ibis::column::info {
public:
    const char* name;		///< Column name.
    const char* description;	///< A description about the column.
    const double expectedMin;	///< The expected lower bound.
    const double expectedMax;	///< The expected upper bound.
    const ibis::TYPE_T type;	///< The type of the values.
    info(const ibis::column& col)
	: name(col.name()), description(col.description()),
	  expectedMin(col.lowerBound()),
	  expectedMax(col.upperBound()), type(col.type()) {};
}; // ibis::column::info

/// A class for controlling access of the index object of a column.  It
/// directly accesses two member variables of ibis::column class, @c idx
/// and @c idxcnt.
class ibis::column::indexLock {
public:
    indexLock(const ibis::column* col, const char* m)
	: theColumn(col), mesg(m) {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::indexLock",
			       "locking column %s for %s", col->name(),
			       (m ? m : "?"));
#endif
	// only attempt to build the index if idxcnt is zero and idx is zero
	if (theColumn->idxcnt == 0 && theColumn->idx == 0)
	    theColumn->loadIndex();
	if (theColumn->idx != 0) {
	    ++ theColumn->idxcnt; // increment counter

	    int ierr = pthread_rwlock_rdlock(&(col->rwlock));
	    if (ierr)
		col->logWarning("gainReadAccess", "pthread_rwlock_rdlock() "
				"for %s returned %d", m, ierr);
	    else if (ibis::gVerbose > 9)
		col->logMessage("gainReadAccess",
				"pthread_rwlock_rdlock for %s", m);
	}
    }
    ~indexLock() {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::indexLock",
			       "unlocking column %s (%s)", theColumn->name(),
			       (mesg ? mesg : "?"));
#endif
	if (theColumn->idx != 0) {
	    int ierr = pthread_rwlock_unlock(&(theColumn->rwlock));
	    if (ierr)
		theColumn->logWarning("releaseReadAccess",
				      "pthread_rwlock_unlock() for %s "
				      "returned %d", mesg, ierr);
	    else if (ibis::gVerbose > 9)
		theColumn->logMessage("releaseReadAccess",
				      "pthread_rwlock_unlock for %s", mesg);

	    -- (theColumn->idxcnt); // decrement counter
	}
    }

    const ibis::index* getIndex() const {return theColumn->idx;};

private:
    const ibis::column* theColumn;
    const char* mesg;

    indexLock();
    indexLock(const indexLock&);
    const indexLock& operator=(const indexLock&);
}; // ibis::column::indexLock

/// Provide a mutual exclusion lock on an ibis::column.
class ibis::column::mutexLock {
public:
    mutexLock(const ibis::column* col, const char* m)
	: theColumn(col), mesg(m) {
	if (ibis::gVerbose > 9)
	    col->logMessage("gainExclusiveAccess",
			    "pthread_mutex_lock for %s", m);
	int ierr = pthread_mutex_lock(&(col->mutex));
	if (ierr)
	    col->logWarning("gainExclusiveAccess", "pthread_mutex_lock() "
			    "for %s returned %d", m, ierr);
    }
    ~mutexLock() {
	if (ibis::gVerbose > 9)
	    theColumn->logMessage("releaseExclusiveAccess",
				 "pthread_mutex_unlock for %s", mesg);
	int ierr = pthread_mutex_unlock(&(theColumn->mutex));
	if (ierr)
	    theColumn->logWarning("releaseExclusiveAccess",
				 "pthread_mutex_unlock() for %s "
				 "returned %d", mesg, ierr);
    }

private:
    const ibis::column* theColumn;
    const char* mesg;

    mutexLock() {}; // no default constructor
    mutexLock(const mutexLock&) {}; // can not copy
    const mutexLock& operator=(const mutexLock&);
}; // ibis::column::mutexLock

/// Provide a write lock on a ibis::column object.
class ibis::column::writeLock {
public:
    writeLock(const ibis::column* col, const char* m)
	: theColumn(col), mesg(m) {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::writeLock",
			       "locking column %s for %s", col->name(),
			       (m ? m : "?"));
#endif
	int ierr = pthread_rwlock_wrlock(&(col->rwlock));
	if (ierr)
	    col->logWarning("gainWriteAccess", "pthread_rwlock_wrlock() "
			    "for %s returned %d", m, ierr);
	else if (ibis::gVerbose > 9)
	    col->logMessage("gainWriteAccess",
			    "pthread_rwlock_wrlock for %s", m);
    }
    ~writeLock() {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::writeLock",
			       "unlocking column %s (%s)", theColumn->name(),
			       (mesg ? mesg : "?"));
#endif
	int ierr = pthread_rwlock_unlock(&(theColumn->rwlock));
	if (ierr)
	    theColumn->logWarning("releaseWriteAccess",
				  "pthread_rwlock_unlock() for %s "
				  "returned %d", mesg, ierr);
	else if (ibis::gVerbose > 9)
	    theColumn->logMessage("releaseWriteAccess",
				  "pthread_rwlock_unlock for %s", mesg);
    }

private:
    const ibis::column* theColumn;
    const char* mesg;

    writeLock();
    writeLock(const writeLock&);
    const writeLock& operator=(const writeLock&);
}; // ibis::column::writeLock

/// Provide a write lock on a ibis::column object.
class ibis::column::softWriteLock {
public:
    softWriteLock(const ibis::column* col, const char* m)
	: theColumn(col), mesg(m),
	  locked(0 == pthread_rwlock_trywrlock(&(col->rwlock))) {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::softWriteLock",
			       "locking column %s for %s", col->name(),
			       (m ? m : "?"));
#endif
	if (ibis::gVerbose > 9 && locked)
	    col->logMessage("gainWriteAccess",
			    "pthread_rwlock_wrlock for %s", m);
    }
    ~softWriteLock() {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::softWriteLock",
			       "unlocking column %s (%s)", theColumn->name(),
			       (mesg ? mesg : "?"));
#endif
	if (locked) {
	    int ierr = pthread_rwlock_unlock(&(theColumn->rwlock));
	    if (ierr)
		theColumn->logWarning("releaseWriteAccess",
				      "pthread_rwlock_unlock() for %s "
				      "returned %d", mesg, ierr);
	    else if (ibis::gVerbose > 9)
		theColumn->logMessage("releaseWriteAccess",
				      "pthread_rwlock_unlock for %s", mesg);
	}
    }
    bool isLocked() const {return locked;}

private:
    const ibis::column* theColumn;
    const char* mesg;
    const bool locked;

    softWriteLock();
    softWriteLock(const softWriteLock&);
    const softWriteLock& operator=(const softWriteLock&);
}; // ibis::column::softWriteLock

/// Provide a write lock on a ibis::column object.
class ibis::column::readLock {
public:
    readLock(const ibis::column* col, const char* m)
	: theColumn(col), mesg(m) {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::readLock",
			       "locking column %s for %s", col->name(),
			       (m ? m : "?"));
#endif
	int ierr = pthread_rwlock_rdlock(&(col->rwlock));
	if (ierr)
	    col->logWarning("gainReadAccess", "pthread_rwlock_rdlock() "
			    "for %s returned %d", m, ierr);
	else if (ibis::gVerbose > 9)
	    col->logMessage("gainReadAccess",
			    "pthread_rwlock_rdlock for %s", m);
    }
    ~readLock() {
#if defined(DEBUG) && DEBUG > 0
	ibis::util::logMessage("ibis::column::readLock",
			       "unlocking column %s (%s)", theColumn->name(),
			       (mesg ? mesg : "?"));
#endif
	int ierr = pthread_rwlock_unlock(&(theColumn->rwlock));
	if (ierr)
	    theColumn->logWarning("releaseReadAccess",
				  "pthread_rwlock_unlock() for %s "
				  "returned %d", mesg, ierr);
	else if (ibis::gVerbose > 9)
	    theColumn->logMessage("releaseReadAccess",
				  "pthread_rwlock_unlock for %s", mesg);
    }

private:
    const ibis::column* theColumn;
    const char* mesg;

    readLock();
    readLock(const readLock&);
    const readLock& operator=(const readLock&);
}; // ibis::column::readLock

inline int ibis::column::elementSize() const {
    int sz = -1;
    switch (m_type) {
    case ibis::OID: sz = sizeof(rid_t); break;
    case ibis::INT: sz = sizeof(int32_t); break;
    case ibis::UINT: sz = sizeof(uint32_t); break;
    case ibis::LONG: sz = sizeof(int64_t); break;
    case ibis::ULONG: sz = sizeof(uint64_t); break;
    case ibis::FLOAT: sz = sizeof(float); break;
    case ibis::DOUBLE: sz = sizeof(double); break;
    case ibis::BYTE: sz = sizeof(char); break;
    case ibis::UBYTE: sz = sizeof(unsigned char); break;
    case ibis::SHORT: sz = sizeof(int16_t); break;
    case ibis::USHORT: sz = sizeof(uint16_t); break;
    case ibis::CATEGORY: sz = 0; break; // no fixed size per element
    case ibis::TEXT: sz = 0; break; // no fixed size per element
    default: sz = -1; break;
    }
    return sz;
} // ibis::column::elementSize

inline bool ibis::column::isFloat() const {
    return(m_type == ibis::FLOAT || m_type == ibis::DOUBLE);
} // ibis::column::isFloat

inline bool ibis::column::isInteger() const {
    return(m_type == ibis::BYTE || m_type == ibis::UBYTE ||
	   m_type == ibis::SHORT || m_type == ibis::USHORT ||
	   m_type == ibis::INT || m_type == ibis::UINT ||
	   m_type == ibis::LONG || m_type == ibis::ULONG);
} // ibis::column::isInteger

inline bool ibis::column::isNumeric() const {
    return(m_type == ibis::BYTE || m_type == ibis::UBYTE ||
	   m_type == ibis::SHORT || m_type == ibis::USHORT ||
	   m_type == ibis::INT || m_type == ibis::UINT ||
	   m_type == ibis::LONG || m_type == ibis::ULONG ||
	   m_type == ibis::FLOAT || m_type == ibis::DOUBLE);
} // ibis::column::isNumeric

// the operator to print a column to an output stream
inline std::ostream& operator<<(std::ostream& out, const ibis::column& prop) {
    prop.print(out);
    return out;
}
#endif // IBIS_COLUMN_H

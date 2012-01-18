//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
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
    class category;   // for categorical values (low-cardinality text fields)
    class text;       // arbitrary cardinality text fields
    class blob;       // text may contain null characters

    // the following are used for storing selected values of different types
    // of columns (implemented in colValues.cpp)
    class colBytes;
    class colUBytes;
    class colShorts;
    class colUShorts;
    class colInts;
    class colUInts;
    class colLongs;
    class colULongs;
    class colFloats;
    class colDoubles;
    class colStrings;
} // namespace

/// @ingroup FastBitIBIS
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

    /// Type of the data.
    ///@note The type shall not be changed.
    ibis::TYPE_T type() const {return m_type;}
    /// Name of the column.
    const char* name() const {return m_name.c_str();}
    /// Rename the column.
    void name(const char* nm) {m_name = nm;}
    /// Description of the column.  Can be an arbitrary string.
    const char* description() const {return m_desc.c_str();}
    /// The lower bound of the values.
    const double& lowerBound() const {return lower;}
    /// The upper bound of the values.
    const double& upperBound() const {return upper;}

    int elementSize() const;
    bool isFloat() const;
    bool isInteger() const;
    bool isSignedInteger() const;
    bool isUnsignedInteger() const;
    bool isNumeric() const;
    bool isSorted() const {return m_sorted;} ///< Are the values sorted?
    void description(const char* d) {m_desc = d;}
    void lowerBound(double d) {lower = d;}
    void upperBound(double d) {upper = d;}
    const part* partition() const {return thePart;}
    void isSorted(bool);

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

    virtual void loadIndex(const char* iopt=0, int ropt=0) const throw ();
    virtual void unloadIndex() const;
    virtual long indexSize() const;

    uint32_t indexedRows() const;
    void indexSpeedTest() const;
    void purgeIndexFile(const char *dir=0) const;

    const char* dataFileName(std::string& fname, const char *dir=0) const;
    const char* nullMaskName(std::string& fname) const;
    void getNullMask(bitvector& mask) const;
    int  setNullMask(const bitvector&);

    /// Return the string value for the <code>i</code>th row.  Only
    /// implemented for ibis::text and ibis::category.  @sa ibis::text
    virtual void getString(uint32_t, std::string &) const {};
    /// Determine if the input string has appeared in this data partition.
    /// If yes, return the pointer to the incoming string, otherwise return
    /// nil.
    virtual const char* findString(const char*) const
    {return static_cast<const char*>(0);}

    array_t<int32_t>* getIntArray() const;
    array_t<float>*   getFloatArray() const;
    array_t<double>*  getDoubleArray() const;
    virtual int getValuesArray(void* vals) const;
    virtual ibis::fileManager::storage* getRawData() const;

    virtual array_t<signed char>*   selectBytes(const bitvector& mask) const;
    virtual array_t<unsigned char>* selectUBytes(const bitvector& mask) const;
    virtual array_t<int16_t>*  selectShorts(const bitvector& mask) const;
    virtual array_t<uint16_t>* selectUShorts(const bitvector& mask) const;
    virtual array_t<int32_t>*  selectInts(const bitvector& mask) const;
    virtual array_t<uint32_t>* selectUInts(const bitvector& mask) const;
    virtual array_t<int64_t>*  selectLongs(const bitvector& mask) const;
    virtual array_t<uint64_t>* selectULongs(const bitvector& mask) const;
    virtual array_t<float>*    selectFloats(const bitvector& mask) const;
    virtual array_t<double>*   selectDoubles(const bitvector& mask) const;
    virtual std::vector<std::string>*
	selectStrings(const bitvector& mask) const;

    long selectValues(const bitvector&, void*) const;
    long selectValues(const bitvector&, void*, array_t<uint32_t>&) const;
    long selectValues(const ibis::qContinuousRange&, void*) const;

    /// Write the metadata entry.
    virtual void write(FILE* file) const;
    /// Print some basic infomation about this column.
    virtual void print(std::ostream& out) const;
    /// Log messages using printf syntax.
    void logMessage(const char* event, const char* fmt, ...) const;
    /// Log warming message using printf syntax.
    void logWarning(const char* event, const char* fmt, ...) const;

    /// Expand the range expression so that the new range falls exactly on
    /// the bin boundaries.
    int expandRange(ibis::qContinuousRange& rng) const;
    /// Contract the range expression so that the new range falls exactly
    /// on the bin boundaries.
    int contractRange(ibis::qContinuousRange& rng) const;

    /// Compute the exact answer.  If successful, return the number of
    /// hits, otherwise return a negative value.
    virtual long evaluateRange(const ibis::qContinuousRange& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;
    /// Compute the exact answer to a discrete range expression.
    virtual long evaluateRange(const ibis::qDiscreteRange& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;
    /// Compute the exact answer to a discrete range expression.
    virtual long evaluateRange(const ibis::qIntHod& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;
    /// Compute the exact answer to a discrete range expression.
    virtual long evaluateRange(const ibis::qUIntHod& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;

    virtual long stringSearch(const char*, ibis::bitvector&) const;
    virtual long stringSearch(const std::vector<std::string>&,
			      ibis::bitvector&) const;
    virtual long stringSearch(const char*) const;
    virtual long stringSearch(const std::vector<std::string>&) const;
    virtual long keywordSearch(const char*, ibis::bitvector&) const;
    virtual long keywordSearch(const char*) const;
    virtual long patternSearch(const char*) const;
    virtual long patternSearch(const char*, ibis::bitvector &) const;

    virtual long evaluateAndSelect(const ibis::qContinuousRange&,
				   const ibis::bitvector&, void*,
				   ibis::bitvector&) const;

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
    /// Compute a lower bound and an upper bound for hits.
    virtual long estimateRange(const ibis::qDiscreteRange& cmp,
			       ibis::bitvector& low,
			       ibis::bitvector& high) const;
    /// Compute a lower bound and an upper bound for hits.
    virtual long estimateRange(const ibis::qIntHod& cmp,
			       ibis::bitvector& low,
			       ibis::bitvector& high) const;
    /// Compute a lower bound and an upper bound for hits.
    virtual long estimateRange(const ibis::qUIntHod& cmp,
			       ibis::bitvector& low,
			       ibis::bitvector& high) const;

    virtual long estimateRange(const ibis::qContinuousRange& cmp) const;
    virtual long estimateRange(const ibis::qDiscreteRange& cmp) const;
    /// Compute an upper bound on the number of hits.
    virtual long estimateRange(const ibis::qIntHod& cmp) const;
    /// Compute an upper bound on the number of hits.
    virtual long estimateRange(const ibis::qUIntHod& cmp) const;

    /// Estimate the cost of evaluating the query expression.
    virtual double estimateCost(const ibis::qContinuousRange& cmp) const;
    /// Estimate the cost of evaluating a dicreate range expression.
    virtual double estimateCost(const ibis::qDiscreteRange& cmp) const;
    /// Estimate the cost of evaluating a dicreate range expression.
    virtual double estimateCost(const ibis::qIntHod& cmp) const;
    /// Estimate the cost of evaluating a dicreate range expression.
    virtual double estimateCost(const ibis::qUIntHod& cmp) const;
    /// Estimate the cost of evaluating a string lookup.
    virtual double estimateCost(const ibis::qString&) const {
	return 0;}
    /// Estimate the cost of looking up a group of strings.
    virtual double estimateCost(const ibis::qMultiString&) const {
	return 0;}

    virtual float getUndecidable(const ibis::qContinuousRange& cmp,
				 ibis::bitvector& iffy) const;
    /// Find rows that can not be decided with the existing index.
    virtual float getUndecidable(const ibis::qDiscreteRange& cmp,
				 ibis::bitvector& iffy) const;
    /// Find rows that can not be decided with the existing index.
    virtual float getUndecidable(const ibis::qIntHod& cmp,
				 ibis::bitvector& iffy) const;
    /// Find rows that can not be decided with the existing index.
    virtual float getUndecidable(const ibis::qUIntHod& cmp,
				 ibis::bitvector& iffy) const;

    /// Append new data in directory df to the end of existing data in dt.
    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf);

    virtual long append(const void* vals, const ibis::bitvector& msk);
    virtual long writeData(const char* dir, uint32_t nold, uint32_t nnew,
			   ibis::bitvector& mask, const void *va1,
			   void *va2=0);
    template <typename T>
    long castAndWrite(const array_t<double>& vals, ibis::bitvector& mask,
		      const T special);
    virtual long saveSelected(const ibis::bitvector& sel, const char *dest,
			      char *buf, uint32_t nbuf);
    virtual long truncateData(const char* dir, uint32_t nent,
			      ibis::bitvector& mask) const;

    /// A group of functions to compute some basic statistics for the
    /// attribute values.
    ///@{
    /// Compute the actual minimum value by reading the data or examining
    /// the index.  It returns DBL_MAX in case of error.
    virtual double getActualMin() const;
    /// Compute the actual maximum value by reading the data or examining
    /// the index.  It returns -DBL_MAX in case of error.
    virtual double getActualMax() const;
    /// Compute the sum of all values by reading the data.
    virtual double getSum() const;
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
    /// @}
    class info;
    class indexLock;
    class mutexLock;

protected:
    // protected member variables
    const part* thePart; ///< Data partition containing this column.
    ibis::bitvector mask_;/// The entries marked 1 are valid.
    ibis::TYPE_T m_type;///< Data type.
    std::string m_name;	///< Name of the column.
    std::string m_desc;	///< Free-form description of the column.
    std::string m_bins;	///< Index/binning specification.
    bool m_sorted;	///< Are the column values in ascending order?
    double lower;	///< The minimum value.
    double upper;	///< The maximum value.
    /// The index for this column.  It is not consider as a must-have member.
    mutable ibis::index* idx;
    /// The number of functions using the index.
    mutable ibis::util::sharedInt32 idxcnt;

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
    /// Compute the minimum and maximum of the values in the array.
    template <typename T>
    void actualMinMax(const array_t<T>& vals, const ibis::bitvector& mask,
		      double& min, double& max) const;
    /// Compute the minimum value in the array.
    template <typename T>
    T computeMin(const array_t<T>& vals,
		 const ibis::bitvector& mask) const;
    /// Compute the maximum value in the array.
    template <typename T>
    T computeMax(const array_t<T>& vals,
		 const ibis::bitvector& mask) const;
    /// Compute the sum of values in the array.
    template <typename T>
    double computeSum(const array_t<T>& vals,
		      const ibis::bitvector& mask) const;

    /// Resolve a continuous range condition on a sorted column.
    virtual int searchSorted(const ibis::qContinuousRange&,
			     ibis::bitvector&) const;
    /// Resolve a discrete range condition on a sorted column.
    virtual int searchSorted(const ibis::qDiscreteRange&,
			     ibis::bitvector&) const;
    /// Resolve a discrete range condition on a sorted column.
    virtual int searchSorted(const ibis::qIntHod&,
			     ibis::bitvector&) const;
    /// Resolve a discrete range condition on a sorted column.
    virtual int searchSorted(const ibis::qUIntHod&,
			     ibis::bitvector&) const;
    /// Resolve a continuous range condition on an array of values.
    template <typename T> int
	searchSortedICC(const array_t<T>& vals,
			const ibis::qContinuousRange& rng,
			ibis::bitvector& hits) const;
    /// Resolve a discrete range condition on an array of values.
    template <typename T> int
	searchSortedICD(const array_t<T>& vals,
			const ibis::qDiscreteRange& rng,
			ibis::bitvector& hits) const;
    /// Resolve a discrete range condition on an array of values.
    template <typename T> int
	searchSortedICD(const array_t<T>& vals,
			const ibis::qIntHod& rng,
			ibis::bitvector& hits) const;
    /// Resolve a discrete range condition on an array of values.
    template <typename T> int
	searchSortedICD(const array_t<T>& vals,
			const ibis::qUIntHod& rng,
			ibis::bitvector& hits) const;
    /// Resolve a continuous range condition using file operations.
    template <typename T> int
	searchSortedOOCC(const char* fname,
			 const ibis::qContinuousRange& rng,
			 ibis::bitvector& hits) const;
    /// Resolve a discrete range condition using file operations.
    template <typename T> int
	searchSortedOOCD(const char* fname,
			 const ibis::qDiscreteRange& rng,
			 ibis::bitvector& hits) const;
    /// Resolve a discrete range condition using file operations.
    template <typename T> int
	searchSortedOOCD(const char* fname,
			 const ibis::qIntHod& rng,
			 ibis::bitvector& hits) const;
    /// Resolve a discrete range condition using file operations.
    template <typename T> int
	searchSortedOOCD(const char* fname,
			 const ibis::qUIntHod& rng,
			 ibis::bitvector& hits) const;

    /// Find the smallest value >= tgt.
    template <typename T> uint32_t
	findLower(int fdes, const uint32_t nr, const T tgt) const;
    /// Find the smallest value > tgt.
    template <typename T> uint32_t
	findUpper(int fdes, const uint32_t nr, const T tgt) const;

    template <typename T>
	long selectValuesT(const char*, const bitvector&, array_t<T>&) const;
    template <typename T>
	long selectValuesT(const char*, const bitvector& mask,
			   array_t<T>& vals, array_t<uint32_t>& inds) const;
    template <typename T>
	long selectToStrings(const char*, const bitvector&,
			     std::vector<std::string>&) const;

    /// Append the content of incoming array to the current data.
    template <typename T>
	long appendValues(const array_t<T>&, const ibis::bitvector&);
    /// Append the strings to the current data.
    long appendStrings(const std::vector<std::string>&, const ibis::bitvector&);

    class readLock;
    class writeLock;
    class softWriteLock;
    friend class readLock;
    friend class writeLock;
    friend class indexLock;
    friend class mutexLock;
    friend class softWriteLock;

private:
    /// The actual read-write lock used by readLock, writeLock and
    /// softWriteLock.
    mutable pthread_rwlock_t rwlock;
    /// The mutual exclusion lock used by indexLock and others.
    mutable pthread_mutex_t mutex;

    column& operator=(const column&); // no assignment
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
    info(const ibis::column& col);
    info(const info& rhs)
	: name(rhs.name), description(rhs.description),
	expectedMin(rhs.expectedMin),
	expectedMax(rhs.expectedMax),
	type(rhs.type) {};

 private:
    info();
    info& operator=(const info&);
}; // ibis::column::info

/// A class for controlling access of the index object of a column.  It
/// directly accesses two member variables of ibis::column class, @c idx
/// and @c idxcnt.
class ibis::column::indexLock {
public:
    ~indexLock();
    indexLock(const ibis::column* col, const char* m);
    const ibis::index* getIndex() const {return theColumn->idx;};

private:
    const ibis::column* theColumn;
    const char* mesg;

    indexLock();
    indexLock(const indexLock&);
    indexLock& operator=(const indexLock&);
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
	if (0 != ierr)
	    col->logWarning("gainExclusiveAccess", "pthread_mutex_lock for %s "
			    "returned %d (%s)", m, ierr, strerror(ierr));
    }
    ~mutexLock() {
	if (ibis::gVerbose > 9)
	    theColumn->logMessage("releaseExclusiveAccess",
				  "pthread_mutex_unlock for %s", mesg);
	int ierr = pthread_mutex_unlock(&(theColumn->mutex));
	if (0 != ierr)
	    theColumn->logWarning("releaseExclusiveAccess",
				  "pthread_mutex_unlock for %s returned %d "
				  "(%s)", mesg, ierr, strerror(ierr));
    }

private:
    const ibis::column* theColumn;
    const char* mesg;

    mutexLock() {}; // no default constructor
    mutexLock(const mutexLock&) {}; // can not copy
    mutexLock& operator=(const mutexLock&);
}; // ibis::column::mutexLock

/// Provide a write lock on a ibis::column object.
class ibis::column::writeLock {
public:
    writeLock(const ibis::column* col, const char* m);
    ~writeLock();

private:
    const ibis::column* theColumn;
    const char* mesg;

    writeLock();
    writeLock(const writeLock&);
    writeLock& operator=(const writeLock&);
}; // ibis::column::writeLock

/// Provide a write lock on a ibis::column object.
class ibis::column::softWriteLock {
public:
    softWriteLock(const ibis::column* col, const char* m);
    ~softWriteLock();
    bool isLocked() const {return(locked==0);}

private:
    const ibis::column* theColumn;
    const char* mesg;
    const int locked;

    softWriteLock();
    softWriteLock(const softWriteLock&);
    softWriteLock& operator=(const softWriteLock&);
}; // ibis::column::softWriteLock

/// Provide a write lock on a ibis::column object.
class ibis::column::readLock {
public:
    readLock(const ibis::column* col, const char* m);
    ~readLock();

private:
    const ibis::column* theColumn;
    const char* mesg;

    readLock();
    readLock(const readLock&);
    readLock& operator=(const readLock&);
}; // ibis::column::readLock

/// Size of a data element in bytes.
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

/// Are they floating-point values?
inline bool ibis::column::isFloat() const {
    return(m_type == ibis::FLOAT || m_type == ibis::DOUBLE);
} // ibis::column::isFloat

///< Are they integer values?
inline bool ibis::column::isInteger() const {
    return(m_type == ibis::BYTE || m_type == ibis::UBYTE ||
	   m_type == ibis::SHORT || m_type == ibis::USHORT ||
	   m_type == ibis::INT || m_type == ibis::UINT ||
	   m_type == ibis::LONG || m_type == ibis::ULONG);
} // ibis::column::isInteger

///< Are they signed integer values?
inline bool ibis::column::isSignedInteger() const {
    return(m_type == ibis::BYTE || m_type == ibis::SHORT ||
	   m_type == ibis::INT || m_type == ibis::LONG);
} // ibis::column::isSignedInteger

///< Are they unsigned integer values?
inline bool ibis::column::isUnsignedInteger() const {
    return(m_type == ibis::UBYTE || m_type == ibis::USHORT ||
	   m_type == ibis::UINT || m_type == ibis::ULONG);
} // ibis::column::isUnsignedInteger

///< Are they numberical values?
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

namespace ibis { // for template specialization
    template <> long column::selectToStrings<signed char>
    (const char*, const bitvector&, std::vector<std::string>&) const;
    template <> long column::selectToStrings<unsigned char>
    (const char*, const bitvector&, std::vector<std::string>&) const;
}
#endif // IBIS_COLUMN_H

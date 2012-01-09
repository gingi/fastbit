//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2012 the Regents of the University of California
#ifndef IBIS_INDEX_H
#define IBIS_INDEX_H
///@file
/// Definition of the common functions of an index.
///
/// The index class is a pure virtual base class with a static create
/// function and a few virtual functions that provide common functionality.
///
/// An index is built for each individual column (ibis::column) of a data
/// table.  The primary function of the index is to compute the solution or
/// an estimation (as a pair of upper and lower bounds) for a range query.
/// It needs to be generated and updated as necessary.  The simplest way of
/// generating an index is to build one from a file containing the binary
/// values of a column.  An index can only be updated for new records
/// appended to the data table.  Any other form of update, such as removal
/// of some records, change some existing records can only be updated by
/// removing the existing index then recreate the index.
///
/// 
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "qExpr.h"
#include "bitvector.h"

#include <string>
#include <limits>	// std::numeric_limits

namespace ibis { // the concrete classes of index hierarchy
    class bin;	 // equal size bins (equality encoded bitmap index)
    class range; // one-sided range encoded (cumulative) index
    class mesa;  // interval encoded index (two-sided ranges)
    class ambit; // two-level, cumulative index (cumulate on all levels)
    class pale;  // two-level, cumulative on the fine level only
    class pack;  // two-level, cumulative on the coarse level only
    class zone;  // two-level, do not cumulate on either levels
    class relic; // the basic bitmap index (one bitmap per distinct value)
    class slice; // the bit slice index (binary encoding of ibis::relic)
    class fade;  // a more controlled slice (multicomponent range code)
    class sbiad; // Italian translation of fade (multicomponent interval code)
    class sapid; // closest word to sbiad in English (multicomponent equality
		 // code)
    class egale; // French word for "equal" (multicomponent equality code on
		 // bins)
    class moins; // French word for "less" (multicomponent range code on bins)
    class entre; // French word for "in between" (multicomponent interval code
		 // on bins)
    class bak;   // Dutch word for "bin" (simple equality encoding for
		 // values mapped to reduced precision floating-point
		 // values)
    class bak2;  // a variation of bak that splits each bak bin in two
//     class waaier;// Dutch word for "range" (simple range encoding for values
// 		 // mapped to reduced precision floating-point values)
//     class tussen;// Dutch word for "in between" (simple interval encoding
// 		 // for values mapped to reduce precision floating-point
// 		 // values)
//     class uguale;// Italian word for "equal" (multicomponent version of bak)
//     class meno;  // Italian word for "less" (multicomponent version of waaier)
//     class fra;   // Italian word for "in between" (multicomponent version of
// 		 // tussen)
    class keywords;// A boolean version of term-document matrix.
    class mesh;  // Composite index on 2-D regular mesh
    class band;  // Composite index on 2-D bands.
    class direkte;// Directly use the integer values as bin numbers.
    class bylt;  // Unbinned version of pack.
    class zona;  // Unbinned version of zone.
    class fuzz;  // Unbinned version of interval-equality encoding.
    class fuge;  // Binned version of interval-equality encoding.
} // namespace ibis

/// @ingroup FastBitIBIS
/// The base index class.  Class ibis::index contains the common definitions
/// and virtual functions of the class hierarchy.  It is assumed that an
/// ibis::index is for only one column.  The user is to create an new index
/// through the function ibis::index::create and only use the functions
/// defined in this class.
class ibis::index {
public:
    /// The integer values of this enum type are used in the index files to
    /// differentiate the indexes.
    /// **** Reordering the list will make some index files invalid ****
    enum INDEX_TYPE {
	BINNING=0,	///< ibis::bin.
	RANGE,	///< ibis::range.
	MESA,	///< ibis::interval.
	AMBIT,	///< ibis::ambit, range-range two level encoding on bins.
	PALE,	///< ibis::pale, equality-range encoding on bins.
	PACK,	///< ibis::pack, range-equality encoding on bins.
	ZONE,	///< ibis::zone, equality-equality encoding on bins.
	RELIC,	///< ibis::relic, the basic bitmap index.
	ROSTER,	///< ibis::roster, RID list.
	SLICE,	///< ibis::slice, bit-sliced index.
	FADE,	///< ibis::fade, multicomponent range encoding (unbinned).
	SBIAD,	///< ibis::sbiad, multicomponent interval encoding (unbinned).
	SAPID,	///< ibis::sapid, multicomponent equality encoding (unbinned).
	EGALE,	///< ibis::egale, multicomponent equality encoding on bins.
	MOINS,	///< ibis::moins, multicomponent range encoding on bins.
	ENTRE,	///< ibis::entre, multicomponent interval encoding on bins.
	BAK,	///< ibis::bak, reduced precision mapping, equality code.
	BAK2,	///< ibis::bak2, splits each BAK bin in two, one less than
		/// the mapped value, one greater and equal
		/// to the mapped value.
	KEYWORDS,	///< ibis::keywords, boolean term-document matrix.
	MESH,	///< not used.
	BAND,	///< not used.
	DIREKTE,///< ibis::direkte, hash value to bitmaps.
	GENERIC,///< not used.
	BYLT,	///< ibis::bylt, unbinned range-equality encoding.
	FUZZ,	///< ibis::fuzz, unbinned interval-equality encoding.
	ZONA,	///< ibis::zona, unbinned equality-equality encoding.
	FUGE,	///< ibis::fuge, binned interval-equality encoding.
	EXTERN	///< externally defined index
    };

    static index* create(const column* c, const char* name=0,
			 const char* spec=0, int inEntirety=0);
    static bool isIndex(const char* f, INDEX_TYPE t);
    /// The destructor.
    virtual ~index () {clear();};

    /// Returns an index type identifier.
    virtual INDEX_TYPE type() const = 0;
    /// Returns the name of the index, similar to the function @c type, but
    /// returns a string instead.
    virtual const char* name() const = 0;

    /// To evaluate the exact hits.  On success, return the number of hits,
    /// otherwise a negative value is returned.
    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const = 0;
    /// Evaluate the range condition and select values.
    virtual long select(const ibis::qContinuousRange&, void*) const =0;
    /// Evaluate the range condition, select values, and record the positions.
    virtual long select(const ibis::qContinuousRange&, void*,
			ibis::bitvector&) const =0;
    /// To evaluate the exact hits.  On success, return the number of hits,
    /// otherwise a negative value is returned.
    virtual long evaluate(const ibis::qDiscreteRange&,
    			  ibis::bitvector&) const {
	return -1;}

    /// Computes an approximation of hits as a pair of lower and upper
    /// bounds.
    /// @param expr the query expression to be evaluated.
    /// @param lower a bitvector marking a subset of the hits.  All
    /// rows marked with one (1) are definitely hits.
    /// @param upper a bitvector marking a superset of the hits.  All
    /// hits are marked with one, but some of the rows marked one may not
    /// be hits.
    /// If the variable upper is empty, the variable lower is assumed to
    /// contain the exact answer.
    virtual void estimate(const ibis::qContinuousRange&,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const {
	lower.set(0, nrows); upper.set(1, nrows);}
    /// Returns an upper bound on the number of hits.
    virtual uint32_t estimate(const ibis::qContinuousRange&) const {
	return nrows;}
    /// Mark the position of the rows that can not be decided with this
    /// index.
    /// @param expr the range conditions to be evaluated.
    /// @param iffy the bitvector marking the positions of rows that can not
    /// be decided using the index.
    /// Return value is the expected fraction of undecided rows that might
    /// satisfy the range conditions.
    virtual float undecidable(const ibis::qContinuousRange&,
			      ibis::bitvector&) const {return 0.5;}

    /// Estimate the hits for discrete ranges, i.e., those translated from
    /// 'a IN (x, y, ..)'.
    virtual void estimate(const ibis::qDiscreteRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qDiscreteRange& expr) const;
    virtual float undecidable(const ibis::qDiscreteRange& expr,
			      ibis::bitvector& iffy) const;

    /// Estimate the pairs for the range join operator.
    virtual void estimate(const ibis::index& idx2,
			  const ibis::deprecatedJoin& expr,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    /// Estimate the pairs for the range join operator.  Only records that
    /// are masked are evaluated.
    virtual void estimate(const ibis::index& idx2,
			  const ibis::deprecatedJoin& expr,
			  const ibis::bitvector& mask,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual void estimate(const ibis::index& idx2,
			  const ibis::deprecatedJoin& expr,
			  const ibis::bitvector& mask,
			  const ibis::qRange* const range1,
			  const ibis::qRange* const range2,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    /// Estimate an upper bound for the number of pairs.
    virtual int64_t estimate(const ibis::index& idx2,
			     const ibis::deprecatedJoin& expr) const;
    /// Estimate an upper bound for the number of pairs produced from
    /// marked records.
    virtual int64_t estimate(const ibis::index& idx2,
			     const ibis::deprecatedJoin& expr,
			     const ibis::bitvector& mask) const;
    virtual int64_t estimate(const ibis::index& idx2,
			     const ibis::deprecatedJoin& expr,
			     const ibis::bitvector& mask,
			     const ibis::qRange* const range1,
			     const ibis::qRange* const range2) const;

    /// Evaluating a join condition with one (likely composite) index.
    virtual void estimate(const ibis::deprecatedJoin& expr,
			  const ibis::bitvector& mask,
			  const ibis::qRange* const range1,
			  const ibis::qRange* const range2,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual int64_t estimate(const ibis::deprecatedJoin& expr,
			     const ibis::bitvector& mask,
			     const ibis::qRange* const range1,
			     const ibis::qRange* const range2) const;

    /// Estimate the cost of evaluating a range condition.
    virtual double estimateCost(const ibis::qContinuousRange&) const {
	return (offset32.empty() ? (nrows<<3) : offset32.back());}
    /// Estimate the cost of evaluating a range condition.
    virtual double estimateCost(const ibis::qDiscreteRange&) const {
	return (offset32.empty() ? (nrows<<3) : offset32.back());}

    /// Prints human readable information.  Outputs information about the
    /// index as text to the specified output stream.
    virtual void print(std::ostream& out) const = 0;
    /// Save index to a file.  Outputs the index in a compact binary format
    /// to the named file or directory.  The index file contains a header
    /// that can be identified by the function isIndex.
    virtual int write(const char* name) const = 0;
    /// Reconstructs an index from the named file.  The name can be the
    /// directory containing an index file.  In this case, the name of the
    /// index file must be the name of the column followed by ".idx" suffix.
    virtual int read(const char* name) = 0;
    /// Reconstructs an index from an array of bytes.  Intended for internal
    /// use only!
    virtual int read(ibis::fileManager::storage* st) = 0;

    /// Extend the index.
    virtual long append(const char*, const char*, uint32_t) {return -1;}

    /// Time some logical operations and print out their speed.
    virtual void speedTest(std::ostream&) const {};
    /// Returns the number of bit vectors used by the index.
    virtual uint32_t numBitvectors() const {return bits.size();}
    /// Return a pointer to the ith bitvector used in the index (may be 0).
    virtual const ibis::bitvector* getBitvector(uint32_t i) const {
	if (i < bits.size()) {
	    if (bits[i] == 0)
		activate(i);
	    return bits[i];
	}
	else {
	    return 0;
	}
    }

    /// The function binBoundaries and binWeights return bin boundaries and
    /// counts of each bin respectively.
    virtual void binBoundaries(std::vector<double>&) const {return;}
    virtual void binWeights(std::vector<uint32_t>&) const {return;}

    /// Cumulative distribution of the data.
    virtual long getCumulativeDistribution
    (std::vector<double>& bds, std::vector<uint32_t>& cts) const;
    /// Binned distribution of the data.
    virtual long getDistribution
    (std::vector<double>& bbs, std::vector<uint32_t>& cts) const;
    /// The minimum value recorded in the index.
    virtual double getMin() const {
	return std::numeric_limits<double>::quiet_NaN();}
    /// The maximum value recorded in the index.
    virtual double getMax() const {
	return std::numeric_limits<double>::quiet_NaN();}
    /// Compute the approximate sum of all the values indexed.  If it
    /// decides that computing the sum directly from the vertical partition
    /// is more efficient, it will return NaN immediately.
    virtual double getSum() const {
	return std::numeric_limits<double>::quiet_NaN();}
    /// Return the number of rows represented by this object.
    uint32_t getNRows() const {return nrows;}

    /// The functions expandRange and contractRange expands or contracts the
    /// boundaries of a range condition so that the new range will have
    /// exact answers using the function estimate.  The default
    /// implementation provided does nothing since this is only meaningful
    /// for indices based on bins.
    virtual int expandRange(ibis::qContinuousRange&) const {return 0;}
    virtual int contractRange(ibis::qContinuousRange&) const {return 0;}

    typedef std::map< double, ibis::bitvector* > VMap;
    typedef std::map< double, uint32_t > histogram;
    template <typename E>
    static void mapValues(const array_t<E>& val, VMap& bmap);
    template <typename E>
    static void mapValues(const array_t<E>& val, histogram& hist,
			  uint32_t count=0);
    template <typename E>
    static void mapValues(const array_t<E>& val, array_t<E>& bounds,
			  std::vector<uint32_t>& cnts);
    template <typename E1, typename E2>
    static void mapValues(const array_t<E1>& val1, const array_t<E2>& val2,
			  array_t<E1>& bnd1, array_t<E2>& bnd2,
			  std::vector<uint32_t>& cnts);
    /// Determine how to split the array @c cnt, so that each group has
    /// roughly the same total value.
    static void divideCounts(array_t<uint32_t>& bounds,
			     const array_t<uint32_t>& cnt);

    // three static functions to perform the task of summing up bit sequences
    static void addBits(const array_t<bitvector*>& bits,
			uint32_t ib, uint32_t ie, ibis::bitvector& res);
    static void sumBits(const array_t<bitvector*>& bits,
			uint32_t ib, uint32_t ie, ibis::bitvector& res);
    static void sumBits(const array_t<bitvector*>& bits,
			const ibis::bitvector& tot, uint32_t ib, uint32_t ie,
			ibis::bitvector& res);
    // a static function to assign bases for multicomponent schemes
    static void setBases(array_t<uint32_t>& bases, uint32_t card,
			 uint32_t nbase = 2);

protected:
    // shared members for all indexes
    /// Pointer to the column this index is for.
    const ibis::column* col;
    /// The underlying storage.  It may be nil if bitvectors are not from a
    /// storage object managed by the file manager.
    mutable ibis::fileManager::storage* str;
    /// The name of the file containing the index.
    mutable const char* fname;
    /// Starting positions of the bitvectors.
    mutable array_t<int32_t> offset32;
    /// Starting positions of the bitvectors.  This is the 64-bit version
    /// of offset32 to deal with large indexes.  All functions that
    /// requires these offsets will attempt to use the 64-bit first.
    mutable array_t<int64_t> offset64;
    /// A list of bitvectors.
    mutable array_t<ibis::bitvector*> bits;
    /// The number of rows represented by the index.  Can not take more
    /// than 2^32 rows because the bitvector class can not hold more than
    /// 2^32 bits.
    uint32_t nrows;

    /// Default constructor.  Protect the constructor so that ibis::index
    /// can not be instantiated directly.  Protecting it also reduces the
    /// size of public interface.
    index(const ibis::column* c=0) : col(c), str(0), fname(0), nrows(0) {}
    index(const ibis::column* c, ibis::fileManager::storage* s);

    /// Generate data file name from "f"
    void dataFileName(const char* f, std::string& name) const;
    /// Generate index file name from "f"
    void indexFileName(const char* f, std::string& name) const;
    static void indexFileName(std::string& name, const ibis::column* col1,
			      const ibis::column* col2, const char* f=0);

    /// Regenerate all bitvectors from the underlying storage.
    virtual void activate() const;
    /// Regenerate the ith bitvector from the underlying storage.
    virtual void activate(uint32_t i) const;
    /// Regenerate bitvectors i (inclusive) through j (exclusive) from the
    /// underlying storage. 
    virtual void activate(uint32_t i, uint32_t j) const;
    /// Clear the existing content
    virtual void clear();

    ////////////////////////////////////////////////////////////////////////
    // both VMap and histogram assume that all supported data types can be
    // safely stored in double
    ////////////////////////////////////////////////////////////////////////
    /// Map the positions of each individual value.
    void mapValues(const char* f, VMap& bmap) const;
    /// Generate a histogram.
    void mapValues(const char* f, histogram& hist, uint32_t count=0) const;
    void computeMinMax(const char* f, double& min, double& max) const;
    /// A function to decide whether to uncompress the bitvectors.
    void optionalUnpack(array_t<ibis::bitvector*>& bits,
			const char* opt);
    /// Add the sum of @c bits[ib] through @c bits[ie-1] to @c res.  Always
    /// explicitly use @c bits[ib] through @c bits[ie-1].
    void addBins(uint32_t ib, uint32_t ie, ibis::bitvector& res) const;
    /// Compute the sum of bit vectors [@c ib, @c ie).  If computing a
    /// complement is faster, assume all bit vectors add up to @c tot.
    void addBins(uint32_t ib, uint32_t ie, ibis::bitvector& res,
		 const ibis::bitvector& tot) const;
    /// Compute the bitwise OR of all bitvectors (in bits) from ib to ie.
    /// As usual, bits[ib] is included but bits[ie] is excluded.
    void sumBins(uint32_t ib, uint32_t ie, ibis::bitvector& res) const;
    /// Compute a new sum for bit vectors [ib, ie) by taking advantage of the
    /// old sum for bitvectors [ib0, ie0).
    void sumBins(uint32_t ib, uint32_t ie, ibis::bitvector& res,
		 uint32_t ib0, uint32_t ie0) const;

    int initOffsets(int fdes, const char offsize, size_t start,
		    uint32_t nobs);
    int initOffsets(ibis::fileManager::storage* st, size_t start,
		    uint32_t nobs);
    void initBitmaps(int fdes);
    void initBitmaps(ibis::fileManager::storage* st);

    class barrel;

private:
    index(const index&); // no copy constructor
    index& operator=(const index&); // no assignment operator
}; // ibis::index


/// A specialization that adds function @c setValue.  This function allows
/// the client to directly set the value for an individual variable.
class ibis::index::barrel : public ibis::math::barrel {
public:
    barrel(const ibis::math::term* t) : ibis::math::barrel(t) {}

    void setValue(uint32_t i, double v) {varvalues[i] = v;}
}; // ibis::index::barrel
#endif // IBIS_INDEX_H

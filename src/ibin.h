//File: $Id$
// Author: John Wu <John.Wu@ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2008 the Regents of the University of California
#ifndef IBIS_IBIN_H
#define IBIS_IBIN_H
///@file
/// Define ibis::bin and derived classes.
///@verbatim
/// bin -> range, mesa, ambit, pale, pack, zone, egale, bak, bak2
/// egale -> moins, entre
///@endverbatim
#include "index.h"
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#undef min
#undef max
#endif

/// The equality encoded bitmap index with binning.  The exact bin boundary
/// assignment is controlled by indexing options '<binning ... />'.
///
/// The 0th bit vector represents x < bounds[0];
/// The (nobs-1)st bit vector represents x >= bounds[nobs-2];
/// The ith bit vector represents bounds[i-1] <= x < bounds[i], (0 < i <
/// nbos-1).
class ibis::bin : public ibis::index {
public:

    virtual ~bin() {clear();};
    bin(const ibis::bin& rhs);
    bin(const ibis::column* c=0, const char* f=0);
    bin(const ibis::column* c, ibis::fileManager::storage* st,
	uint32_t offset = 8);
    bin(const ibis::column* c, const char* f, const array_t<double>& bd);
    bin(const ibis::column* c, const char* f, const std::vector<double>& bd);

    virtual void print(std::ostream& out) const;
    virtual void write(const char* dt) const; // write to the named file
    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;
//     virtual long evaluate(const ibis::qDiscreteRange& expr,
// 			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    /// Estimate the hits for symmetric joins.
    virtual void estimate(const ibis::rangeJoin& expr,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual void estimate(const ibis::rangeJoin& expr,
			  const ibis::bitvector& mask,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual void estimate(const ibis::rangeJoin& expr,
			  const ibis::bitvector& mask,
			  const ibis::qRange* const range1,
			  const ibis::qRange* const range2,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual int64_t estimate(const ibis::rangeJoin& expr,
			     const ibis::bitvector& mask,
			     const ibis::qRange* const range1,
			     const ibis::qRange* const range2) const;

    /// Estimate the number of hits for nonsymmetric joins.
    virtual void estimate(const ibis::bin& idx2,
			  const ibis::rangeJoin& expr,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual void estimate(const ibis::bin& idx2,
			  const ibis::rangeJoin& expr,
			  const ibis::bitvector& mask,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual void estimate(const ibis::bin& idx2,
			  const ibis::rangeJoin& expr,
			  const ibis::bitvector& mask,
			  const ibis::qRange* const range1,
			  const ibis::qRange* const range2,
			  ibis::bitvector64& lower,
			  ibis::bitvector64& upper) const;
    virtual int64_t estimate(const ibis::bin& idx2,
			     const ibis::rangeJoin& expr) const;
    virtual int64_t estimate(const ibis::bin& idx2,
			     const ibis::rangeJoin& expr,
			     const ibis::bitvector& mask) const;
    virtual int64_t estimate(const ibis::bin& idx2,
			     const ibis::rangeJoin& expr,
			     const ibis::bitvector& mask,
			     const ibis::qRange* const range1,
			     const ibis::qRange* const range2) const;

    virtual INDEX_TYPE type() const {return BINNING;}
    virtual const char* name() const {return "bin";}
    virtual uint32_t numBins() const {return (nobs>2?nobs-2:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    // expand/contract the boundaries of a range condition
    virtual int  expandRange(ibis::qContinuousRange& rng) const;
    virtual int  contractRange(ibis::qContinuousRange& rng) const;
    virtual void speedTest(std::ostream& out) const;
    virtual double estimateCost(const ibis::qContinuousRange& expr) const;
    virtual double estimateCost(const ibis::qDiscreteRange& expr) const;

    virtual long getCumulativeDistribution(std::vector<double>& bds,
					   std::vector<uint32_t>& cts) const;
    virtual long getDistribution(std::vector<double>& bbs,
				 std::vector<uint32_t>& cts) const;
    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;

    /// Read an ibis::bin embedded inside a file.
    void read(int fdes, uint32_t offset, const char *fname);
    /// Append the @c tail to this index.
    long append(const ibis::bin& tail);
    /// Append a list of integers representing bin numbers.
    long append(const array_t<uint32_t>& ind);
    array_t<uint32_t>* indices(const ibis::bitvector& mask) const;
    /// Candidate check using the binned values.  Returns the number of
    /// hits if successful, otherwise it returns a negative value.
    long checkBin(const ibis::qRange& cmp, uint32_t jbin,
		  ibis::bitvector& res) const;
    /// Candidate check using the binned values.  The bitvector @c mask
    /// marks the actual values in the bin (because the bitmaps stored in
    /// @c bits do not directly corresponds to the bin).
    long checkBin(const ibis::qRange& cmp, uint32_t jbin,
		  const ibis::bitvector& mask, ibis::bitvector& res) const;

    /// A data structure to assist the mapping of values to lower
    /// precisions.  Any integral or floating-point value may be mapped to
    /// lower precision floating-point value.  This would produce a more
    /// granular representation of the values.  The low precision
    /// floating-point value is called a target in this description.  To
    /// facilitate this type of dynamic binning, we device this simple data
    /// structure to record the position of all records mapped to a
    /// particular target value.  It separates out the values that are
    /// larger than or equal to the target from those that are smaller than
    /// the target.  The variables min0 and max0 store the actual minimum
    /// and maximum value among those that are smaller than the target.
    /// The variables min1 and max1 store the actual minimum and maximum
    /// value among those that are larger than or equal to the target
    /// value.
    struct granule {
	double min0, max0, min1, max1;
	ibis::bitvector* loc0;
	ibis::bitvector* loc1;

	// the default construct, user to explicitly allocated the bitvector
	granule() : min0(DBL_MAX), max0(-DBL_MAX), min1(DBL_MAX),
		    max1(-DBL_MAX), loc0(0), loc1(0) {};
	~granule() {delete loc0; delete loc1;};
    };
    // key = target value
    typedef std::map< double, granule* > granuleMap;

protected:
    // member variables shared by all derived classes -- the derived classes
    // are allowed to interpret the actual content differently.
    uint32_t nobs;		///< Number of bitvectors.
    array_t<double> bounds;	///< The nominal boundaries.
    array_t<double> maxval;	///< The maximal values in each bin.
    array_t<double> minval;	///< The minimal values in each bin.

    // a constructor to accommodate multicomponent encodings
    bin(const ibis::column* c, const uint32_t nbits,
	ibis::fileManager::storage* st, uint32_t offset = 8);

    /// Generate bins according to the specified boundaries.
    void binning(const char* f, const std::vector<double>& bd);
    void binning(const char* f, const array_t<double>& bd);
    /// Read the data file and partition the values into bins according to
    /// the specified bin boundary.
    void binning(const char* f);
    /// Read the data file, partition the values, and write out the bin
    /// ordered data with .bin suffix.
    template <typename E>
    void binningT(const char* fname);
    template <typename E>
    long checkBin0(const ibis::qRange& cmp, uint32_t jbin,
		   ibis::bitvector& res) const;
    template <typename E>
    long checkBin1(const ibis::qRange& cmp, uint32_t jbin,
		   const ibis::bitvector& mask, ibis::bitvector& res) const;
    /// Write bin-ordered values.
    template <typename E>
    long binOrderT(const char* fname) const;
    long binOrder(const char* fname) const;

    /// Set bin boundaries
    void setBoundaries(const char* f);
    void setBoundaries(array_t<double>& bnds,
		       const ibis::bin& bin0) const;
    void setBoundaries(array_t<double>& bnds,
		       const ibis::bin& idx1,
		       const array_t<uint32_t> cnt1,
		       const array_t<uint32_t> cnt0) const;
    // functions to deal with in-memory arrays
    template <typename E>
    void construct(const array_t<E>& varr);
    template <typename E>
    void binning(const array_t<E>& varr);
    template <typename E>
    void binning(const array_t<E>& varr, const array_t<double>& bd);
    template <typename E>
    void setBoundaries(const array_t<E>& varr);
    template <typename E>
    void scanAndPartition(const array_t<E>&, unsigned);
    template <typename E>
    void mapGranules(const array_t<E>&, granuleMap& gmap) const;
    void printGranules(std::ostream& out, const granuleMap& gmap) const;
    // Convert the granule map into binned index.  Destroy the content of
    // the granuleMap.
    void convertGranules(granuleMap& gmap);

    // read a file containing a list of floating-point numbers
    void readBinBoundaries(const char* name, uint32_t nb);
    // partition the range based on the (approximate) histogram of the data
    void scanAndPartition(const char*, unsigned, uint32_t nbins=0);
    // the function used by setBoudaries() to actually generate the bounds
    void addBounds(double lbd, double rbd, uint32_t nbins, uint32_t eqw);
    // parse the index specs to determine eqw and nbins
    uint32_t parseNbins() const;
    unsigned parseScale() const;
    // parse the index spec to extract precision
    unsigned parsePrec() const;

    // partition the bitmaps into groups of takes about the same amount of
    // storage.
    void divideBitmaps(const std::vector<ibis::bitvector*>& bms,
		       std::vector<unsigned>& parts) const;

    // compute the sum of values from the information in the index
    virtual double computeSum() const;
    // some common functions that work on the bitvectors
    virtual void adjustLength(uint32_t nrows);    // fill with zeros
    virtual uint32_t locate(const double& val) const; // bin containing val
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1) const;
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1,
			uint32_t& hit0, uint32_t& hit1) const;
    void swap(bin& rhs) { // swap the content of the index
	const ibis::column* c = col;
	col = rhs.col;
	rhs.col = c;
	uint32_t tmp = nobs;
	nobs = rhs.nobs;
	rhs.nobs = tmp;
	tmp = nrows;
	nrows = rhs.nrows;
	rhs.nrows = tmp;
	bounds.swap(rhs.bounds);
	maxval.swap(rhs.maxval);
	minval.swap(rhs.minval);
	bits.swap(rhs.bits);
    } // swap

    // free current resources, re-initialize all member variables
    virtual void clear();
    void write(int fptr) const; // write to an open file

private:
    // private member functions
    const bin& operator=(const bin&);

    unsigned parseScale(const char*) const;

    void print(std::ostream& out, const uint32_t tot,
	       const double& lbound, const double& rbound) const;

    /// Evaluate the equi-joins.
    void equiJoin(ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void equiJoin(const ibis::bin& idx2,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void rangeJoin(const double& delta,
		   ibis::bitvector64& lower,
		   ibis::bitvector64& iffy) const;
    void rangeJoin(const ibis::bin& idx2,
		   const double& delta,
		   ibis::bitvector64& lower,
		   ibis::bitvector64& iffy) const;
    void compJoin(const ibis::compRange::term *expr,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void compJoin(const ibis::bin& idx2,
		  const ibis::compRange::term *expr,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void equiJoin(const ibis::bitvector& mask,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void equiJoin(const ibis::bin& idx2,
		  const ibis::bitvector& mask,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void rangeJoin(const double& delta,
		   const ibis::bitvector& mask,
		   ibis::bitvector64& lower,
		   ibis::bitvector64& iffy) const;
    void rangeJoin(const ibis::bin& idx2,
		   const double& delta,
		   const ibis::bitvector& mask,
		   ibis::bitvector64& lower,
		   ibis::bitvector64& iffy) const;
    void compJoin(const ibis::compRange::term *expr,
		  const ibis::bitvector& mask,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;
    void compJoin(const ibis::bin& idx2,
		  const ibis::compRange::term *expr,
		  const ibis::bitvector& mask,
		  ibis::bitvector64& lower,
		  ibis::bitvector64& iffy) const;

    void equiJoin(const ibis::bitvector& mask,
		  const ibis::qRange* const range1,
		  const ibis::qRange* const range2,
		  ibis::bitvector64& sure,
		  ibis::bitvector64& iffy) const;
    void rangeJoin(const double& delta,
		   const ibis::bitvector& mask,
		   const ibis::qRange* const range1,
		   const ibis::qRange* const range2,
		   ibis::bitvector64& sure,
		   ibis::bitvector64& iffy) const;
    void compJoin(const ibis::compRange::term *delta,
		  const ibis::bitvector& mask,
		  const ibis::qRange* const range1,
		  const ibis::qRange* const range2,
		  ibis::bitvector64& sure,
		  ibis::bitvector64& iffy) const;

    int64_t equiJoin(const ibis::bitvector& mask,
		     const ibis::qRange* const range1,
		     const ibis::qRange* const range2) const;
    int64_t rangeJoin(const double& delta,
		      const ibis::bitvector& mask,
		      const ibis::qRange* const range1,
		      const ibis::qRange* const range2) const;
    int64_t compJoin(const ibis::compRange::term *delta,
		     const ibis::bitvector& mask,
		     const ibis::qRange* const range1,
		     const ibis::qRange* const range2) const;

    void equiJoin(const ibis::bin& idx2,
		  const ibis::bitvector& mask,
		  const ibis::qRange* const range1,
		  const ibis::qRange* const range2,
		  ibis::bitvector64& sure,
		  ibis::bitvector64& iffy) const;
    void rangeJoin(const ibis::bin& idx2,
		   const double& delta,
		   const ibis::bitvector& mask,
		   const ibis::qRange* const range1,
		   const ibis::qRange* const range2,
		   ibis::bitvector64& sure,
		   ibis::bitvector64& iffy) const;
    void compJoin(const ibis::bin& idx2,
		  const ibis::compRange::term *delta,
		  const ibis::bitvector& mask,
		  const ibis::qRange* const range1,
		  const ibis::qRange* const range2,
		  ibis::bitvector64& sure,
		  ibis::bitvector64& iffy) const;

    int64_t equiJoin(const ibis::bin& idx2,
		     const ibis::bitvector& mask,
		     const ibis::qRange* const range1,
		     const ibis::qRange* const range2) const;
    int64_t rangeJoin(const ibis::bin& idx2,
		      const double& delta,
		      const ibis::bitvector& mask,
		      const ibis::qRange* const range1,
		      const ibis::qRange* const range2) const;
    int64_t compJoin(const ibis::bin& idx2,
		     const ibis::compRange::term *delta,
		     const ibis::bitvector& mask,
		     const ibis::qRange* const range1,
		     const ibis::qRange* const range2) const;

    // need these friendships to access the protected member variables
    friend class ibis::mesa;
    friend class ibis::range;
    friend class ibis::ambit;
    friend class ibis::pack;
    friend class ibis::pale;
    friend class ibis::zone;
    friend class ibis::mesh;
    friend class ibis::band;
}; // ibis::bin

/// The range encoded bitmap index based.  It can be thought of as a
/// cumulative version of ibis::bin, where the ith bit vector marks the
/// possibles of all entries where <code>x < bounds[i]</code>.
class ibis::range : public ibis::bin {
public:

    virtual ~range() {};
    range(const ibis::column* c=0, const char* f=0);
    range(const ibis::column* c, ibis::fileManager::storage* st,
	  uint32_t offset = 8);
    explicit range(const ibis::bin& rhs); // convert a bin to a range

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void write(const char* dt) const; // write to the named file
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return RANGE;}
    virtual const char* name() const {return "range";}
    virtual uint32_t numBins() const {return (nobs>1?nobs-1:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    // expand/contract the boundaries of a range condition
    virtual int  expandRange(ibis::qContinuousRange& range) const;
    virtual int  contractRange(ibis::qContinuousRange& range) const;
    virtual double getMax() const;
    virtual double getSum() const;

    /// Read an ibis::ragne embedded with multiple data structures.
    void read(int fdes, uint32_t offset, const char *fname);
    long append(const ibis::range& tail);
    virtual void speedTest(std::ostream& out) const;

protected:
    // protected member variables
    double max1, min1; // the min and max of the bin not explicitly tracked

    // have to have its own locate functions because a bin is not explicitly
    // stored
    virtual uint32_t locate(const double& val) const {
	return ibis::bin::locate(val);
    }
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1) const;
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1,
			uint32_t& hit0, uint32_t& hit1) const;
    virtual double computeSum() const;

private:
    // private member functions
    void construct(const char* f, const array_t<double>& bd);
    void write(int fptr) const; // write to the given stream
    void print(std::ostream& out, const uint32_t tot, const double& lbound,
	       const double& rbound) const;

    friend class ibis::pale; // pale uses ibis::range
}; // ibis::range

/// This class implements the two-side range encoding from Chan and
/// Ioannidis.
class ibis::mesa : public ibis::bin {
public:
    virtual ~mesa() {};
    mesa(const ibis::column* c=0, const char* f=0);
    mesa(const ibis::column* c, ibis::fileManager::storage* st,
	 uint32_t offset = 8);
    explicit mesa(const ibis::bin& rhs); // convert a bin to a mesa

    virtual void print(std::ostream& out) const;
    virtual void write(const char* dt) const; // write to the named file
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return MESA;}
    virtual const char* name() const {return "interval";}
    virtual uint32_t numBins() const {return (nobs>2?nobs-2:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    virtual double getSum() const;

    virtual void speedTest(std::ostream& out) const;
    long append(const ibis::mesa& tail);

protected:
    virtual double computeSum() const;

private:
    // private member functions
    void write(int fptr) const; // write to the given stream
    //void print(std::ostream& out, const uint32_t tot, const double& lbound,
    //       const double& rbound) const;

    mesa(const mesa&);
    const mesa& operator=(const mesa&);
}; // ibis::mesa

/// The multi-level range based (cumulative) index.
/// Each level/each bin consists of a range index.
class ibis::ambit : public ibis::bin {
public:
    virtual ~ambit() {clear();};
    ambit(const ibis::column* c=0, const char* f=0);
    ambit(const ibis::column* c, ibis::fileManager::storage* st,
	  uint32_t offset = 8);
    explicit ambit(const ibis::bin& rhs); // convert from a ibis::bin

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return AMBIT;}
    virtual const char* name() const {return "range-range";}
    virtual uint32_t numBins() const {return (nobs>1?nobs-1:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    virtual void adjustLength(uint32_t nrows);
    virtual double getSum() const;

    virtual void speedTest(std::ostream& out) const;
    long append(const ibis::ambit& tail);

protected:
    virtual double computeSum() const;
    virtual void clear();

private:
    // min and max of range nobs (the one that is not explicitly recorded)
    double max1, min1;
    std::vector<ibis::ambit*> sub;

    // private member functions
    void write(int fptr) const;
    void read(int fdes, uint32_t offset, const char *fn);
    void print(std::ostream& out, const uint32_t tot, const double& lbound,
	       const double& rbound) const;
    void construct(const char* f, const array_t<double>& bd);

    ambit(const ambit&);
    const ambit& operator=(const ambit&);
}; // ibis::ambit

/// A two-level index.  Coarse level not cumulative, fine level is
/// cumulative.
class ibis::pale : public ibis::bin {
public:
    virtual ~pale() {clear();};
    pale(const ibis::column* c, ibis::fileManager::storage* st,
	 uint32_t offset = 8);
    explicit pale(const ibis::bin& rhs); // convert from a ibis::bin

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return PALE;}
    virtual const char* name() const {return "equality-range";}
    virtual uint32_t numBins() const {return (nobs>1?nobs-1:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    virtual void adjustLength(uint32_t nrows);

    virtual void speedTest(std::ostream& out) const;
    long append(const ibis::pale& tail);

protected:
    virtual void clear();

private:
    // private member variables
    std::vector<ibis::range*> sub;

    // private member functions
    void write(int fptr) const;

    pale(const pale&);
    const pale& operator=(const pale&);
}; // ibis::pale

/// A two-level index.  Coarse level is cumulative, but not the bottom
/// level.
class ibis::pack : public ibis::bin {
public:
    virtual ~pack() {clear();};
    pack(const ibis::column* c, ibis::fileManager::storage* st,
	  uint32_t offset = 8);
    explicit pack(const ibis::bin& rhs); // convert from a ibis::bin

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return PACK;}
    virtual const char* name() const {return "range-equality";}
    virtual uint32_t numBins() const {return (nobs>1?nobs-1:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    virtual void adjustLength(uint32_t nrows);
    virtual double getSum() const;

    virtual void speedTest(std::ostream& out) const;
    long append(const ibis::pack& tail);

protected:
    virtual double computeSum() const;
    virtual void clear();

private:
    // private member variables
    // min and max of range nobs (the one that is not explicitly recorded)
    double max1, min1;
    std::vector<ibis::bin*> sub;

    // private member functions
    void write(int fptr) const;

    pack(const pack&);
    const pack& operator=(const pack&);
}; // ibis::pack

/// A two-level index.  Both levels are not cumulative, i.e., both levels
/// are equality encoded.
class ibis::zone : public ibis::bin {
public:
    virtual ~zone() {clear();};
    zone(const ibis::column* c, ibis::fileManager::storage* st,
	 uint32_t offset = 8);
    explicit zone(const ibis::bin& rhs); // convert from a ibis::bin

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return ZONE;}
    virtual const char* name() const {return "equality-equality";}
    virtual uint32_t numBins() const {return (nobs>1?nobs-1:0);}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    virtual void adjustLength(uint32_t nrows);

    virtual void speedTest(std::ostream& out) const;
    long append(const ibis::zone& tail);

protected:
    virtual void clear();

private:
    // private member variable
    std::vector<ibis::bin*> sub;

    // private member functions
    void write(int fptr) const;

    zone(const zone&);
    const zone& operator=(const zone&);
}; // ibis::zone

/// A two-level index.  The top (coarse) level uses the interval encoding
/// and the bottom (fine) level uses the equality encoding.  Similar to
/// class ibis::fuzz, the fine level bitmaps are kept together as in
/// ibis::bin and the coarse level bitmaps are placed at the end of the
/// index file.
class ibis::fuge : public ibis::bin {
public:
    virtual ~fuge() {clear();};
    fuge(const ibis::column* c, ibis::fileManager::storage* st,
	 uint32_t offset = 8);
    fuge(const ibis::column*, const char*);
    explicit fuge(const ibis::bin& rhs); // convert from a ibis::bin

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void print(std::ostream& out) const;
    virtual void write(const char* dt) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;

    virtual INDEX_TYPE type() const {return FUGE;}
    virtual const char* name() const {return "interval-equality";}
    virtual void adjustLength(uint32_t nrows);

    long append(const ibis::fuge& tail);

protected:
    virtual void clear() {clearCoarse(); ibis::bin::clear();}

private:
    // private member variable
    mutable std::vector<ibis::bitvector*> cbits;
    array_t<uint32_t> cbounds;
    array_t<int32_t> coffsets;

    void coarsen(); // given fine level, add coarse level
    void activateCoarse() const; // activate all coarse level bitmaps
    void activateCoarse(uint32_t i) const; // activate one bitmap
    void activateCoarse(uint32_t i, uint32_t j) const;

    void writeCoarse(int fdes) const;
    void readCoarse(const char *fn);
    void clearCoarse();

    /// Estimate the cost of answer a range query [lo, hi).
    long coarseEstimate(uint32_t lo, uint32_t hi) const;
    /// Evaluate the range condition [lo, hi) and place the result in @c res.
    long coarseEvaluate(uint32_t lo, uint32_t hi, ibis::bitvector& res) const;

    fuge(const fuge&);
    const fuge& operator=(const fuge&);
}; // ibis::fuge

/// The multicomponent equality code on bins.
///
/// The word egale is a French word for 'equal'.
class ibis::egale : public ibis::bin {
public:
    virtual ~egale() {clear();};
    egale(const ibis::column* c = 0, const char* f = 0,
	  const uint32_t nbase = 2);
    egale(const ibis::column* c, ibis::fileManager::storage* st,
	  uint32_t offset = 8);
    egale(const ibis::bin& rhs, const uint32_t nbase = 2);

    virtual void read(const char* idxfile);
    virtual void read(ibis::fileManager::storage* st);
    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const;

    virtual INDEX_TYPE type() const {return EGALE;}
    virtual const char* name() const {return "MCBin";}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>& b) const;
    virtual void binWeights(std::vector<uint32_t>& b) const;
    virtual double getSum() const;

    virtual void speedTest(std::ostream& out) const;
    long append(const ibis::egale& tail);
    long append(const array_t<uint32_t>& ind);

protected:
    // protected member variables
    uint32_t nbits;		// number of bitvectors, (size of bits)
    uint32_t nbases;		// size of array bases
    array_t<uint32_t> cnts;	// number of records in each bin
    array_t<uint32_t> bases;	// the size of the bases used

    // protected member functions
    egale(const ibis::column* c, const char* f, const array_t<double>& bd,
	  const array_t<uint32_t> bs);
    void addBins_(uint32_t ib, uint32_t ie, ibis::bitvector& res) const;
    virtual double computeSum() const;
    virtual void clear() {
	cnts.clear(); bases.clear();
	ibis::bin::clear();
    }

    void write(int fdes) const;
    void construct(const char* f);

private:
    // private member functions
    void setBit(const uint32_t i, const double val);
    void convert();

    void evalEQ(ibis::bitvector& res, uint32_t b) const;
    void evalLE(ibis::bitvector& res, uint32_t b) const;
    void evalLL(ibis::bitvector& res, uint32_t b0, uint32_t b1) const;

    egale(const egale&);
    const egale& operator=(const egale&);
}; // ibis::egale

/// The multicomponent range code on bins.
///
/// Moins is a French word for 'less'.
class ibis::moins : public ibis::egale {
public:
    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual INDEX_TYPE type() const {return MOINS;}
    virtual const char* name() const {return "MCBRange";}

    virtual ~moins() {clear();};
    moins(const ibis::column* c = 0, const char* f = 0,
	  const uint32_t nbase = 2);
    moins(const ibis::column* c, ibis::fileManager::storage* st,
	  uint32_t offset = 8);
    moins(const ibis::bin& rhs, const uint32_t nbase = 2);

    virtual void speedTest(std::ostream& out) const;
    virtual double getSum() const;

    long append(const ibis::moins& tail);
    long append(const array_t<uint32_t>& ind);

protected:
    virtual double computeSum() const;

private:
    // private member functions
    moins(const ibis::column* c, const char* f, const array_t<double>& bd,
	  const array_t<uint32_t> bs);
    void convert();

    void evalEQ(ibis::bitvector& res, uint32_t b) const;
    void evalLE(ibis::bitvector& res, uint32_t b) const;
    void evalLL(ibis::bitvector& res, uint32_t b0, uint32_t b1) const;

    moins(const moins&);
    const moins& operator=(const moins&);
}; // ibis::moins

/// The multicomponent interval code on bins.
///
///  Entre is a French word for 'in between'.
class ibis::entre : public ibis::egale {
public:
    virtual ~entre() {clear();};
    entre(const ibis::column* c = 0, const char* f = 0,
	  const uint32_t nbase = 2);
    entre(const ibis::column* c, ibis::fileManager::storage* st,
	  uint32_t offset = 8);
    entre(const ibis::bin& rhs, const uint32_t nbase = 2);

    virtual void write(const char* dt) const;
    virtual void print(std::ostream& out) const;
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;

    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual INDEX_TYPE type() const {return ENTRE;}
    virtual const char* name() const {return "MCBInterval";}

    virtual void speedTest(std::ostream& out) const;
    virtual double getSum() const;

    long append(const ibis::entre& tail);
    long append(const array_t<uint32_t>& ind);

protected:
    virtual double computeSum() const;

private:
    // private member functions
    entre(const ibis::column* c, const char* f, const array_t<double>& bd,
	  const array_t<uint32_t> bs);
    void convert();

    void evalEQ(ibis::bitvector& res, uint32_t b) const;
    void evalLE(ibis::bitvector& res, uint32_t b) const;
    void evalLL(ibis::bitvector& res, uint32_t b0, uint32_t b1) const;

    entre(const entre&);
    const entre& operator=(const entre&);
}; // ibis::entre

/// Maps each value to a lower prevision (decimal) values and use the the
/// low precision value as center of the bin.
/// It reuses the same variables of ibis::bin, but have to interpret them
/// differently.
///
/// Bak is a Dutch word for 'bin'.
class ibis::bak : public ibis::bin {
public:
    virtual ~bak() {clear();};
    bak(const ibis::column* c=0, const char* f=0);
    bak(const ibis::column* c, ibis::fileManager::storage* st,
	uint32_t offset = 8) : ibis::bin(c, st, offset) {};

    virtual void print(std::ostream& out) const;
    virtual void write(const char* dt) const; // write to the named file
    virtual void read(const char* idxfile);
    virtual long append(const char* dt, const char* df, uint32_t nnew);
    virtual INDEX_TYPE type() const {return BAK;}
    virtual const char* name() const
    {return "equality code on mapped values";}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    // expand/contract the boundaries of a range condition
    virtual int  expandRange(ibis::qContinuousRange& rng) const;
    virtual int  contractRange(ibis::qContinuousRange& rng) const;

    long append(const ibis::bin& tail);

    // a simple structure to record the position of the values mapped to the
    // same value.  The bitvector marked the locations of the values and the
    // min and max record the actual minimum and maximum value encountered.
    struct grain {
	double min, max;
	ibis::bitvector* loc;

	// the default construct, user to explicitly allocated the bitvector
	grain() : min(DBL_MAX), max(-DBL_MAX), loc(0) {}
    };

    typedef std::map< double, grain > bakMap;


protected:

    // reads all values and records positions in bmap
    void mapValues(const char* f, bakMap& bmap) const;
    void printMap(std::ostream& out, const bakMap& bmap) const;

    virtual uint32_t locate(const double& val) const;
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1) const {
	ibis::bin::locate(expr, cand0, cand1);
    }
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1,
			uint32_t& hit0, uint32_t& hit1) const {
	ibis::bin::locate(expr, cand0, cand1, hit0, hit1);
    }

private:
    // coverts the std::map structure into the structure defined in ibis::bin
    void construct(const bakMap& bmap);

    bak(const bak&);
    const bak& operator&=(const bak&);
}; // ibis::bak

/// A variation on ibis::bak, it splits each bin of ibis::bak in two,
/// one for entries less than the mapped value and one for the entries that
/// greater and equal to the mapped value.  This way, the index can be used
/// to answer question involving ranges exactly on the mapped values.  All
/// internal variables are processed same as a regular ibis::bin index.
class ibis::bak2 : public ibis::bin {
public:
    virtual ~bak2() {clear();};
    bak2(const ibis::column* c=0, const char* f=0);
    bak2(const ibis::column* c, ibis::fileManager::storage* st,
	 uint32_t offset = 8) : ibis::bin(c, st, offset) {};

    virtual void print(std::ostream& out) const;
    virtual void write(const char* dt) const; // write to the named file
    virtual void read(const char* idxfile);
    virtual long append(const char* dt, const char* df, uint32_t nnew);
    virtual INDEX_TYPE type() const {return BAK;}
    virtual const char* name() const
    {return "equality code on mapped values";}
    // bin boundaries and counts of each bin
    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;
    // expand/contract the boundaries of a range condition
    virtual int  expandRange(ibis::qContinuousRange& rng) const;
    virtual int  contractRange(ibis::qContinuousRange& rng) const;

    long append(const ibis::bin& tail);

    /// A simple structure to record the position of the values mapped to
    /// the same value.  The ibis::bitvector marked the locations of the
    /// values and the min and max record the actual minimum and maximum
    /// value encountered.
    struct grain {
	double min0, max0, min1, max1;
	ibis::bitvector* loc0;
	ibis::bitvector* loc1;

	// the default construct, user to explicitly allocated the bitvector
	grain() : min0(DBL_MAX), max0(-DBL_MAX), min1(DBL_MAX), max1(-DBL_MAX),
		  loc0(0), loc1(0) {}
    };

    typedef std::map< double, grain > bakMap;


protected:

    /// Reads all values and records positions in @c bmap.
    void mapValues(const char* f, bakMap& bmap) const;
    void printMap(std::ostream& out, const bakMap& bmap) const;

    virtual uint32_t locate(const double& val) const;
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1) const {
	ibis::bin::locate(expr, cand0, cand1);
    }
    virtual void locate(const ibis::qContinuousRange& expr,
			uint32_t& cand0, uint32_t& cand1,
			uint32_t& hit0, uint32_t& hit1) const {
	ibis::bin::locate(expr, cand0, cand1, hit0, hit1);
    }

private:
    /// Coverts the @c std::map structure into the structure defined in @c
    /// ibis::bin.
    void construct(const bakMap& bmap);

    bak2(const bak2&);
    const bak2& operator=(const bak2&);
}; // ibis::bak2

#endif // IBIS_IBIN_H

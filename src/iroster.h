//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
#ifndef IBIS_ROSTER_H
#define IBIS_ROSTER_H
#include "array_t.h"
#include "util.h"
///@file
/// Defines a pseudo-index.  Used in some performance comparisons.

/// @ingroup FastBitIBIS
/// A roster list is a list of indices for ordering the values in the
/// ascending order.  It can use external sort if the data and indices can
/// not fit into memory.  The indices will be written to a file with .ind
/// extension and if the external sorting procedure is used a file with
/// .srt extension is also generated to store a sorted version of the
/// values.  If the indices can not be loaded into memory as a whole, the
/// .ind file will be opened for future operations.
class ibis::roster {
public:
    ~roster() {clear();};
    /// Sort all data elements in the named file (default to the one in the
    /// current working directory).
    roster(const ibis::column* c, const char* f = 0);
    /// For reconstructing the data structure from raw bytes stored in @c
    /// st.
    roster(const ibis::column* c, ibis::fileManager::storage* st,
	   uint32_t offset = 8);
    /// Select the values that are marked 1 in @c mask and sort them.
    roster(const ibis::column* c, const ibis::bitvector& mask,
	   const char* f=0);

    const char* name() const {return "roster list";}
    const ibis::column* getColumn() const {return col;}
    uint32_t size() const;

    void read(const char* idxfile);
    void read(ibis::fileManager::storage* st);

    /// Output minimal information about the roster list.
    void print(std::ostream& out) const;
    /// Write two files, .ind for indices and .srt to the sorted values.
    void write(const char* dt) const;
    /// Write the sorted version of the attribute values to a .srt file.
    void writeSorted(const char* dt) const;

    const array_t<uint32_t>& array() const {return ind;}
    inline uint32_t operator[](uint32_t i) const;

    /// Locate the the values and set their positions in the bitvector.
    /// Return a negative value for error, zero or a positive value for in
    /// case of success.
    /// The input values are assumed to be sorted in ascending order.
    int locate(const std::vector<int32_t>& vals,
	       ibis::bitvector& positions) const;
    int locate(const std::vector<uint32_t>& vals,
	       ibis::bitvector& positions) const;
    int locate(const std::vector<float>& vals,
	       ibis::bitvector& positions) const;
    int locate(const std::vector<double>& vals,
	       ibis::bitvector& positions) const;

    template <typename T> void
    icSearch(const std::vector<T>& vals, std::vector<uint32_t>& pos) const;
    template <typename T> void
    oocSearch(const std::vector<T>& vals, std::vector<uint32_t>& pos) const;
//     void estimate(const ibis::qContinuousRange& expr,
//     		  ibis::bitvector& lower,
//     		  ibis::bitvector& upper) const;
//     uint32_t estimate(const ibis::qContinuousRange& expr) const;

    /// A two-way merge algorithm.  Uses std::less<T> for comparisons.
    /// Assumes the sorted segment size is @c segment elements of type T.
    template <class T>
    static long mergeBlock2(const char *dsrc, const char *dout,
			    const uint32_t segment, array_t<T>& buf1,
			    array_t<T>& buf2, array_t<T>& buf3);

//     /// A templated multi-way merge sort algorithm for a data file with
//     /// values of the same type.  Return the number of passes if
//     /// successful, other return a negative number to indicate error.
//     template <class Type, uint32_t mway=64, uint32_t block=8192>
//     static int diskSort(const char *datafile, const char *scratchfile);

protected:
//     /// This function performs the initial sort of blocks.  Entries in each
//     /// (@c mway * @c block)-segment is sorted and written back to the same
//     /// data file.
//     template <class Type, uint32_t mway, uint32_t block>
//     static int diskSortInit(const char *datafile);
//     /// Merge blocks.  The variable @c segment contains the number of
//     /// consecutive entries (a segement) that are already sorted.  To
//     /// consecutive such segments will be merged.
//     template <class Type, uint32_t mway, uint32_t block>
//     static int diskSortMerge(const char *from, const char *to,
// 			     uint32_t segment);

private:
    // private member variables
    const ibis::column* col;    ///< Each roster is for one column.
    array_t<uint32_t> ind;	///< @c [ind[i]] is the ith smallest value.
    mutable int inddes;		///< The descriptor for the @c .ind file.

    // private member functions
    void clear() {ind.clear(); if (inddes>=0) UnixClose(inddes);};
    void write(FILE* fptr) const;

    /// The in-core sorting function to build the roster list.
    void icSort(const char* f = 0);
    /// The out-of-core sorting function to build the roster list.
    void oocSort(const char* f = 0);
    template <class T>
    long oocSortBlocks(const char *src, const char *dest,
		       const char *ind, const uint32_t mblock,
		       array_t<T>& dbuf1, array_t<T>& dbuf2,
		       array_t<uint32_t>& ibuf) const;
    template <class T>
    long oocMergeBlocks(const char *dsrc, const char *dout,
			const char *isrc, const char *iout,
			const uint32_t mblock, const uint32_t stride,
			array_t<T>& dbuf1, array_t<T>& dbuf2,
			array_t<uint32_t>& ibuf1,
			array_t<uint32_t>& ibuf2) const;

    uint32_t locate(const double& val) const;
//     void locate(const ibis::qContinuousRange& expr,
// 		uint32_t& hit0, uint32_t& hit1) const;

    roster();
    roster(const roster&);
    const roster& operator=(const roster&);
}; // ibis::roster

inline uint32_t ibis::roster::operator[](uint32_t i) const {
    uint32_t tmp = UINT_MAX;
    if (i < ind.size()) {
	tmp = ind[i];
    }
    else if (inddes >= 0) {
	UnixSeek(inddes, i*sizeof(uint32_t), SEEK_SET);
	UnixRead(inddes, &tmp, sizeof(uint32_t));
    }
    else {
	ibis::util::logMessage("Error", "roster object (ind[%u], inddes=%d) "
			       "index i (%u) is out of range", ind.size(),
			       inddes, i);
    }
    return tmp;
} // ibis::roster::operator[]
#endif // IBIS_ROSTER_H


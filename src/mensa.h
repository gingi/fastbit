// File: $Id$
// Author: John Wu <John.Wu at nersc.gov>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2008 the Regents of the University of California
#ifndef IBIS_MENSA_H
#define IBIS_MENSA_H
#include "table.h"	// ibis::table
#include "util.h"	// ibis::partList
#include "fileManager.h"// ibis::fileManager::storage

/**@file

A table with multiple data partitions on disk.
This class defines the data structure to encapsulate multiple on-disk data
partitions into a logical table.  The class translates the function defined
on ibis::part to the ibis::table interface.
 */
namespace ibis {
    class mensa;
} // namespace ibis

/// Class ibis::mensa contains multiple (horizontal) data partitions (@c
/// ibis::part) to form a logical data table.  The base data contained in
/// this table is logically immutable as reordering rows (through function
/// @c reorder) does not change the overall content of the table.  The
/// functions @c reverseRows and @c groupby are not implmented.
///
/// @note Mensa is a Latin word for "table."
class ibis::mensa : public ibis::table {
public:
    explicit mensa(const char* dir);
    mensa(const char* dir1, const char* dir2);
    virtual ~mensa() {clear();}

    virtual uint64_t nRows() const {return nrows;}
    virtual size_t nColumns() const {return naty.size();}

    virtual stringList columnNames() const;
    virtual typeList columnTypes() const;
    virtual int addPartition(const char* dir);

    virtual void describe(std::ostream&) const;
    virtual int dump(std::ostream&, const char*) const;

    virtual int64_t getColumnAsBytes(const char*, char*) const;
    virtual int64_t getColumnAsUBytes(const char*, unsigned char*) const;
    virtual int64_t getColumnAsShorts(const char*, int16_t*) const;
    virtual int64_t getColumnAsUShorts(const char*, uint16_t*) const;
    virtual int64_t getColumnAsInts(const char*, int32_t*) const;
    virtual int64_t getColumnAsUInts(const char*, uint32_t*) const;
    virtual int64_t getColumnAsLongs(const char*, int64_t*) const;
    virtual int64_t getColumnAsULongs(const char*, uint64_t*) const;
    virtual int64_t getColumnAsFloats(const char*, float*) const;
    virtual int64_t getColumnAsDoubles(const char*, double*) const;
    virtual int64_t getColumnAsStrings(const char*,
				       std::vector<std::string>&) const;

    virtual long getHistogram(const char*, const char*,
			      double, double, double,
			      std::vector<size_t>&) const;
    virtual long getHistogram2D(const char*, const char*,
				double, double, double,
				const char*,
				double, double, double,
				std::vector<size_t>&) const;
    virtual long getHistogram3D(const char*, const char*,
				double, double, double,
				const char*,
				double, double, double,
				const char*,
				double, double, double,
				std::vector<size_t>&) const;

    virtual void estimate(const char* cond,
			  uint64_t& nmin, uint64_t& nmax) const;
    virtual table* select(const char* sel, const char* cond) const;
    /// A variation of the function select defined in ibis::table.  It
    /// accepts an extra argument for caller to specify a list of names of
    /// data partitions that will participate in the select operation.  The
    /// argument pts may contain wild characters accepted by SQL function
    /// 'LIKE', '_' and '%'.
    virtual table* select2(const char* sel, const char* cond,
			   const char* pts) const;

    virtual void orderby(const stringList&);
    virtual void orderby(const char *str) {ibis::table::orderby(str);}
    /// Reversing the ordering of the rows on disk requires too much work
    /// but has no obvious benefit.
    virtual void reverseRows() {};
    /// Directly performing group-by on the base data (without selection)
    /// is not currently supported.
    virtual table* groupby(const stringList&) const {return 0;}
    virtual table* groupby(const char *) const {return 0;}

    virtual int buildIndex(const char*, const char*);
    virtual int buildIndexes(const char*);
    virtual const char* indexSpec(const char*) const;
    virtual void indexSpec(const char*, const char*);

    // Cursor class for row-wise data accesses.
    class cursor;
    /// Create a @c cursor object to perform row-wise data access.
    virtual ibis::table::cursor* createCursor() const;

protected:
    /// List of data partitions.
    ibis::partList parts;
    /// A combined list of columns names.
    ibis::table::namesTypes naty;
    uint64_t nrows;

    /// Clear the existing content.
    void clear();
    /// Compute the number of hits.
    int64_t computeHits(const char* cond) const {
	return computeHits(cond, parts);}
    /// Compute he number of hits from a list of data partitions
    static int64_t computeHits(const char* cond, const ibis::partList& pts);
    /// Append new data (in @c from) to a larger array (pointed to by @c to).
    template <typename T>
    void addIncoreData(void*& to, const array_t<T>& from,
		       size_t nold, const T special) const;
    void addStrings(void*&, const std::vector<std::string>&, size_t) const;

private:
    // disallow copying.
    mensa(const mensa&);
    const mensa& operator=(const mensa&);

    friend class cursor;
}; // ibis::mensa

class ibis::mensa::cursor : public ibis::table::cursor {
public:
    cursor(const ibis::mensa& t);
    virtual ~cursor() {clearBuffers();};

    virtual uint64_t nRows() const {return tab.nRows();}
    virtual size_t nColumns() const {return tab.nColumns();}
    virtual ibis::table::stringList columnNames() const {
	return tab.columnNames();}
    virtual ibis::table::typeList columnTypes() const {
	return tab.columnTypes();}
    virtual int fetch();
    virtual int fetch(uint64_t);
    virtual int fetch(ibis::table::row&);
    virtual int fetch(uint64_t, ibis::table::row&);
    virtual uint64_t getCurrentRowNumber() const {return curRow;}
    virtual int dump(std::ostream& out, const char* del) const;

    int dumpBlock(std::ostream& out, const char* del);

    virtual int getColumnAsByte(const char*, char*) const;
    virtual int getColumnAsUByte(const char*, unsigned char*) const;
    virtual int getColumnAsShort(const char*, int16_t*) const;
    virtual int getColumnAsUShort(const char*, uint16_t*) const;
    virtual int getColumnAsInt(const char*, int32_t*) const;
    virtual int getColumnAsUInt(const char*, uint32_t*) const;
    virtual int getColumnAsLong(const char*, int64_t*) const;
    virtual int getColumnAsULong(const char*, uint64_t*) const;
    virtual int getColumnAsFloat(const char*, float*) const;
    virtual int getColumnAsDouble(const char*, double*) const;
    virtual int getColumnAsString(const char*, std::string&) const;

    virtual int getColumnAsByte(size_t, char*) const;
    virtual int getColumnAsUByte(size_t, unsigned char*) const;
    virtual int getColumnAsShort(size_t, int16_t*) const;
    virtual int getColumnAsUShort(size_t, uint16_t*) const;
    virtual int getColumnAsInt(size_t, int32_t*) const;
    virtual int getColumnAsUInt(size_t, uint32_t*) const;
    virtual int getColumnAsLong(size_t, int64_t*) const;
    virtual int getColumnAsULong(size_t, uint64_t*) const;
    virtual int getColumnAsFloat(size_t, float*) const;
    virtual int getColumnAsDouble(size_t, double*) const;
    virtual int getColumnAsString(size_t, std::string&) const;

protected:
    struct bufferElement {
	const char* cname;
	ibis::TYPE_T ctype;
	mutable ibis::fileManager::storage* cval;

	bufferElement() : cname(0), ctype(ibis::UNKNOWN_TYPE), cval(0) {}
    }; // bufferElement
    typedef std::map<const char*, size_t, ibis::lessi> bufferMap;
    std::vector<bufferElement> buffer;
    bufferMap bufmap;
    const ibis::mensa& tab;
    int64_t curRow; // the current row number
    uint64_t curBlock; // the first row number of the current block
    ibis::partList::const_iterator curPart;

    void clearBuffers();
    void fillBuffers() const;
    int fillBuffer(size_t) const;
    void fillRow(ibis::table::row& res) const;
    int dumpIJ(std::ostream&, size_t, size_t) const;

private:
    cursor();
    cursor(const cursor&);
    cursor& operator=(const cursor&);
}; // ibis::mensa::cursor

inline int
ibis::mensa::cursor::getColumnAsByte(const char* cn, char* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsByte((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsByte

inline int
ibis::mensa::cursor::getColumnAsUByte(const char* cn,
				      unsigned char* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsUByte((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsUByte

inline int
ibis::mensa::cursor::getColumnAsShort(const char* cn, int16_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsShort((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsShort

inline int
ibis::mensa::cursor::getColumnAsUShort(const char* cn, uint16_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsUShort((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsUShort

inline int
ibis::mensa::cursor::getColumnAsInt(const char* cn, int32_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsInt((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsInt

inline int
ibis::mensa::cursor::getColumnAsUInt(const char* cn, uint32_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsUInt((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsUInt

inline int
ibis::mensa::cursor::getColumnAsLong(const char* cn, int64_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsLong((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsLong

inline int
ibis::mensa::cursor::getColumnAsULong(const char* cn, uint64_t* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsULong((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsULong

inline int
ibis::mensa::cursor::getColumnAsFloat(const char* cn, float* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsFloat((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsFloat

inline int
ibis::mensa::cursor::getColumnAsDouble(const char* cn, double* val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsDouble((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsDouble

inline int
ibis::mensa::cursor::getColumnAsString(const char* cn,
				       std::string& val) const {
    if (curRow < 0 || curPart == tab.parts.end() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsString((*it).second, val);
    else
	return -2;
} // ibis::mensa::cursor::getColumnAsString
#endif // IBIS_MENSA_H

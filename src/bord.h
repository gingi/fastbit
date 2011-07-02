// File: $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2011 the Regents of the University of California
#ifndef IBIS_BORD_H
#define IBIS_BORD_H
#include "table.h"	// ibis::table
#include "util.h"	// ibis::partList
#include "part.h"	// ibis::part

///@file
/// Defines ibis::bord.  This is an in-memory data table, with a single
/// data partition completely residing in memory.
namespace ibis {
    class bord;
} // namespace ibis

/// Class ibis::bord stores all its data in memory.  The function @c
/// ibis::table::select produces an ibis::bord object to store nontrivial
/// results.  Since all data records are stored in memory, the number of
/// rows that can be stored are limited.  This also implies that the
/// function ibis::table::select will not be able to produce a proper table
/// object if the selection is very large.  An additional limit is that the
/// number of rows is internally stored in a 32-bit unsigned integer, which
/// can store no more than 4 billion rows.
///
/// @note Bord is a Danish word for "table."
class FASTBIT_CXX_DLLSPEC ibis::bord : public ibis::table, public ibis::part {
public:
    bord(const char *tn, const char *td, uint64_t nr,
	 ibis::table::bufferList &buf,
	 const ibis::table::typeList &ct,
	 const ibis::table::stringList &cn,
	 const ibis::table::stringList *cdesc=0);
    bord(const char *tn, const char *td,
	 const ibis::selectClause &sc, const ibis::part &ref);
    virtual ~bord() {clear();}

    virtual uint64_t nRows() const {return nEvents;}
    virtual uint32_t nColumns() const {return ibis::part::nColumns();}

    virtual ibis::table::stringList columnNames() const;
    virtual ibis::table::typeList columnTypes() const;

    virtual void describe(std::ostream&) const;
    virtual void dumpNames(std::ostream&, const char*) const;
    virtual int dump(std::ostream&, const char*) const;
    virtual int dump(std::ostream&, uint64_t, const char*) const;
    virtual int dump(std::ostream&, uint64_t, uint64_t, const char*) const;
    virtual int backup(const char* dir, const char* tname=0,
		       const char* tdesc=0) const;

    virtual int64_t
	getColumnAsBytes(const char*, char*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsUBytes(const char*, unsigned char*, uint64_t =0,
			  uint64_t =0) const;
    virtual int64_t
	getColumnAsShorts(const char*, int16_t*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsUShorts(const char*, uint16_t*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsInts(const char*, int32_t*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsUInts(const char*, uint32_t*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsLongs(const char*, int64_t*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsULongs(const char*, uint64_t*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsFloats(const char*, float*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsDoubles(const char*, double*, uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsDoubles(const char*, std::vector<double>&,
			   uint64_t =0, uint64_t =0) const;
    virtual int64_t
	getColumnAsStrings(const char*, std::vector<std::string>&,
			   uint64_t =0, uint64_t =0) const;
    virtual double getColumnMin(const char*) const;
    virtual double getColumnMax(const char*) const;

    virtual long getHistogram(const char*, const char*,
			      double, double, double,
			      std::vector<uint32_t>&) const;
    virtual long getHistogram2D(const char*, const char*,
				double, double, double,
				const char*,
				double, double, double,
				std::vector<uint32_t>&) const;
    virtual long getHistogram3D(const char*, const char*,
				double, double, double,
				const char*,
				double, double, double,
				const char*,
				double, double, double,
				std::vector<uint32_t>&) const;

    virtual void estimate(const char* cond,
			  uint64_t& nmin, uint64_t& nmax) const;
    virtual void estimate(const ibis::qExpr* cond,
			  uint64_t& nmin, uint64_t& nmax) const;
    using table::select;
    virtual table* select(const char* sel, const char* cond) const;
    virtual table* groupby(const ibis::table::stringList&) const;
    virtual table* groupby(const char* str) const;
    virtual void orderby(const ibis::table::stringList&);
    virtual void reverseRows();

    virtual int buildIndex(const char*, const char*) {return -1;}
    virtual int buildIndexes(const char*) {return -1;}
    virtual const char* indexSpec(const char*) const {return 0;}
    virtual void indexSpec(const char*, const char*) {return;}
    virtual int getPartitions(std::vector<const ibis::part*> &) const;

    int restoreCategoriesAsStrings(const char*);
    ibis::table* groupby(const ibis::selectClause&) const;
    ibis::table* evaluateTerms(const ibis::selectClause&,
			       const char*) const;

    virtual long reorder(const ibis::table::stringList&);

    int append(const ibis::selectClause&, const ibis::part&,
	       const ibis::bitvector&);
    int renameColumns(const ibis::selectClause&);
    int limit(uint32_t);

    template <typename T>
	long sortValues(array_t<T>& vals,
			const array_t<uint32_t>& indin,
			array_t<uint32_t>& indout,
			array_t<uint32_t>& starts) const;
    template <typename T>
	long reorderValues(array_t<T>& vals,
			   const array_t<uint32_t>& ind) const;
    long sortStrings(std::vector<std::string>& vals,
		     const array_t<uint32_t>& idxin,
		     array_t<uint32_t>& idxout,
		     array_t<uint32_t>& starts) const;
    long reorderStrings(std::vector<std::string>& vals,
			const array_t<uint32_t>& ind) const;


    void copyColumn(const char*, ibis::TYPE_T&, void*&) const;
    static void copyValue(ibis::TYPE_T type,
			  void* outbuf, size_t outpos,
			  const void* inbuf, size_t inpos);

    /// Append new data (in @c from) to a larger array (pointed to by
    /// @c to).
    template <typename T> void 
	addIncoreData(void*& to, const array_t<T>& from,
		      uint32_t nold, const T special);
    void addStrings(void*&, const std::vector<std::string>&, uint32_t);

    // Cursor class for row-wise data accesses.
    class cursor;
    /// Create a @c cursor object to perform row-wise data access.
    virtual ibis::table::cursor* createCursor() const;

    // forward declarations
    class column;

protected:
    /// Clear the existing content.
    void clear();
    /// Compute the number of hits.
    int64_t computeHits(const char* cond) const;

private:
    // disallow copying.
    bord(const bord&);
    bord& operator=(const bord&);

    friend class cursor;
}; // ibis::bord

/// An in-memory version of ibis::column.  For integers and floating-point
/// values, the buffer (with type void*) points to an ibis::array_t<T>
/// where the type T is designated by the column type.  For a string-valued
/// column, the buffer (with type void*) is std::vector<std::string>*.
///
/// @note Since the in-memory data tables are typically created at run-time
/// through select operations, the data types associated with a column is
/// only known at run-time.  Casting to void* is a ugly option; the
/// developers welcome suggestions for a replacement.
class ibis::bord::column : public ibis::column {
public:
    column(const ibis::bord* tbl, ibis::TYPE_T t, const char* name, void *buf=0,
	   const char* desc="", double low=DBL_MAX, double high=-DBL_MAX);
    column(const ibis::bord*, const ibis::column&, void *buf);
    column(const column& rhs);
    virtual ~column();

    virtual ibis::fileManager::storage* getRawData() const;

    using ibis::column::evaluateRange;
    virtual long evaluateRange(const ibis::qContinuousRange& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;
    virtual long evaluateRange(const ibis::qDiscreteRange& cmp,
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

    virtual array_t<signed char>*   selectBytes(const ibis::bitvector&) const;
    virtual array_t<unsigned char>* selectUBytes(const ibis::bitvector&) const;
    virtual array_t<int16_t>*       selectShorts(const ibis::bitvector&) const;
    virtual array_t<uint16_t>*      selectUShorts(const ibis::bitvector&) const;
    virtual array_t<int32_t>*       selectInts(const ibis::bitvector&) const;
    virtual array_t<uint32_t>*      selectUInts(const ibis::bitvector&) const;
    virtual array_t<int64_t>*       selectLongs(const ibis::bitvector&) const;
    virtual array_t<uint64_t>*      selectULongs(const ibis::bitvector&) const;
    virtual array_t<float>*         selectFloats(const ibis::bitvector&) const;
    virtual array_t<double>*        selectDoubles(const ibis::bitvector&) const;
    virtual std::vector<std::string>*
	selectStrings(const bitvector& mask) const;

    virtual void computeMinMax() {
	computeMinMax(thePart->currentDataDir(), lower, upper);}
    virtual void computeMinMax(const char *dir) {
	computeMinMax(dir, lower, upper);}
    virtual void computeMinMax(const char *, double &min, double &max) const;
    virtual void getString(uint32_t i, std::string &val) const;
    virtual int  getValuesArray(void* vals) const;

    void reverseRows();
    int  limit(uint32_t nr);

    void*& getArray() {return buffer;}
    void*  getArray() const {return buffer;}
    int dump(std::ostream& out, uint32_t i) const;

    int restoreCategoriesAsStrings(const ibis::part&);

protected:
    /// The in-memory storage.  A pointer to an array<T> or
    /// std::vector<std::string> depending on data type.
    void *buffer;

    column& operator=(const column&); // no assignment
}; // ibis::bord::column

class FASTBIT_CXX_DLLSPEC ibis::bord::cursor : public ibis::table::cursor {
public:
    cursor(const ibis::bord& t);
    virtual ~cursor() {};

    virtual uint64_t nRows() const {return tab.nRows();}
    virtual uint32_t nColumns() const {return tab.nColumns();}
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

    virtual int getColumnAsByte(const char*, char&) const;
    virtual int getColumnAsUByte(const char*, unsigned char&) const;
    virtual int getColumnAsShort(const char*, int16_t&) const;
    virtual int getColumnAsUShort(const char*, uint16_t&) const;
    virtual int getColumnAsInt(const char*, int32_t&) const;
    virtual int getColumnAsUInt(const char*, uint32_t&) const;
    virtual int getColumnAsLong(const char*, int64_t&) const;
    virtual int getColumnAsULong(const char*, uint64_t&) const;
    virtual int getColumnAsFloat(const char*, float&) const;
    virtual int getColumnAsDouble(const char*, double&) const;
    virtual int getColumnAsString(const char*, std::string&) const;

    virtual int getColumnAsByte(uint32_t, char&) const;
    virtual int getColumnAsUByte(uint32_t, unsigned char&) const;
    virtual int getColumnAsShort(uint32_t, int16_t&) const;
    virtual int getColumnAsUShort(uint32_t, uint16_t&) const;
    virtual int getColumnAsInt(uint32_t, int32_t&) const;
    virtual int getColumnAsUInt(uint32_t, uint32_t&) const;
    virtual int getColumnAsLong(uint32_t, int64_t&) const;
    virtual int getColumnAsULong(uint32_t, uint64_t&) const;
    virtual int getColumnAsFloat(uint32_t, float&) const;
    virtual int getColumnAsDouble(uint32_t, double&) const;
    virtual int getColumnAsString(uint32_t, std::string&) const;

protected:
    struct bufferElement {
	const char* cname;
	ibis::TYPE_T ctype;
	void* cval;

	bufferElement() : cname(0), ctype(ibis::UNKNOWN_TYPE), cval(0) {}
    }; // bufferElement
    typedef std::map<const char*, uint32_t, ibis::lessi> bufferMap;
    std::vector<bufferElement> buffer;
    bufferMap bufmap;
    const ibis::bord& tab;
    int64_t curRow; // the current row number

    void fillRow(ibis::table::row& res) const;
    int dumpIJ(std::ostream&, uint32_t, uint32_t) const;

private:
    cursor();
    cursor(const cursor&);
    cursor& operator=(const cursor&);
}; // ibis::bord::cursor

/// Copy a single value from inbuf to outbuf.  The output buffer must have
/// the correct size on entry; this function does not attempt to resize the
/// output buffer.
inline void ibis::bord::copyValue(ibis::TYPE_T type,
				  void* outbuf, size_t outpos,
				  const void* inbuf, size_t inpos) {
    switch (type) {
    default:
	break;
    case ibis::BYTE: {
	(*static_cast<array_t<signed char>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<signed char>*>(inbuf))[inpos];
	break;}
    case ibis::UBYTE: {
	(*static_cast<array_t<unsigned char>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<unsigned char>*>(inbuf))[inpos];
	break;}
    case ibis::SHORT: {
	(*static_cast<array_t<int16_t>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<int16_t>*>(inbuf))[inpos];
	break;}
    case ibis::USHORT: {
	(*static_cast<array_t<uint16_t>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<uint16_t>*>(inbuf))[inpos];
	LOGGER(ibis::gVerbose > 5)
	    << "DEBUG -- copied inbuf[" << inpos << "] (="
	    << (*static_cast<const array_t<uint16_t>*>(inbuf))[inpos]
	    << ") to outbuf[" << outpos << "] (="
	    << (*static_cast<array_t<uint16_t>*>(outbuf))[outpos] << ')';
	break;}
    case ibis::INT: {
	(*static_cast<array_t<int32_t>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<int32_t>*>(inbuf))[inpos];
	break;}
    case ibis::UINT: {
	(*static_cast<array_t<uint32_t>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<uint32_t>*>(inbuf))[inpos];
	break;}
    case ibis::LONG: {
	(*static_cast<array_t<int64_t>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<int64_t>*>(inbuf))[inpos];
	break;}
    case ibis::ULONG: {
	(*static_cast<array_t<uint64_t>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<uint64_t>*>(inbuf))[inpos];
	break;}
    case ibis::FLOAT: {
	(*static_cast<array_t<float>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<float>*>(inbuf))[inpos];
	break;}
    case ibis::DOUBLE: {
	(*static_cast<array_t<double>*>(outbuf))[outpos]
	    = (*static_cast<const array_t<double>*>(inbuf))[inpos];
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	(*static_cast<std::vector<std::string>*>(outbuf))[outpos]
	= (*static_cast<const std::vector<std::string>*>(inbuf))[inpos];
	break;}
    }
} //ibis::bord::copyValue

inline int ibis::bord::column::dump(std::ostream& out, uint32_t i) const {
    int ierr = -1;
    if (buffer == 0) {
	out << "(no data in memory)";
	return ierr;
    }

    switch (m_type) {
    case ibis::BYTE: {
	const array_t<signed char>* vals =
	    static_cast<const array_t<signed char>*>(buffer);
	out << (int)((*vals)[i]);
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* vals =
	    static_cast<const array_t<unsigned char>*>(buffer);
	out << (unsigned)((*vals)[i]);
	ierr = 0;
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* vals =
	    static_cast<const array_t<int16_t>*>(buffer);
	out << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* vals =
	    static_cast<const array_t<uint16_t>*>(buffer);
	out << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::INT: {
	const array_t<int32_t>* vals =
	    static_cast<const array_t<int32_t>*>(buffer);
	out << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::UINT: {
	const array_t<uint32_t>* vals =
	    static_cast<const array_t<uint32_t>*>(buffer);
	out << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* vals =
	    static_cast<const array_t<int64_t>*>(buffer);
	out << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* vals =
	    static_cast<const array_t<uint64_t>*>(buffer);
	out << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	const array_t<float>* vals =
	    static_cast<const array_t<float>*>(buffer);
	out << std::setprecision(7) << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* vals =
	    static_cast<const array_t<double>*>(buffer);
	out << std::setprecision(15) << (*vals)[i];
	ierr = 0;
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	std::string tmp;
	getString(i, tmp);
	out << '"' << tmp << '"';
	ierr = 0;
	break;}
    default: {
	ierr = -2;
	break;}
    }
    return ierr;
} // ibis::bord::column::dump

inline int ibis::bord::cursor::fetch() {
    ++ curRow;
    return (0 - (curRow >= (int64_t) tab.nRows()));
} // ibis::bord::cursor::fetch

inline int ibis::bord::cursor::fetch(uint64_t irow) {
    if (irow < tab.nRows()) {
	curRow = static_cast<int64_t>(irow);
	return 0;
    }
    else {
	return -1;
    }
} // ibis::bord::cursor::fetch

inline int ibis::bord::cursor::fetch(ibis::table::row& res) {
    ++ curRow;
    if ((uint64_t) curRow < tab.nRows()) {
	fillRow(res);
	return 0;
    }
    else {
	return -1;
    }
} // ibis::bord::cursor::fetch

inline int ibis::bord::cursor::fetch(uint64_t irow, ibis::table::row& res) {
    if (irow < tab.nRows()) {
	curRow = static_cast<int64_t>(irow);
	fillRow(res);
	return 0;
    }
    else {
	return -1;
    }
} // ibis::bord::cursor::fetch

inline int
ibis::bord::cursor::dumpIJ(std::ostream& out, uint32_t i,
			   uint32_t j) const {
    if (buffer[j].cval == 0) return -1;

    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	const array_t<const signed char>* vals =
	    static_cast<const array_t<const signed char>*>(buffer[j].cval);
	out << (int) ((*vals)[i]);
	break;}
    case ibis::UBYTE: {
	const array_t<const unsigned char>* vals =
	    static_cast<const array_t<const unsigned char>*>(buffer[j].cval);
	out << (unsigned int) ((*vals)[i]);
	break;}
    case ibis::SHORT: {
	const array_t<const int16_t>* vals =
	    static_cast<const array_t<const int16_t>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::USHORT: {
	const array_t<const uint16_t>* vals =
	    static_cast<const array_t<const uint16_t>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::INT: {
	const array_t<const int32_t>* vals =
	    static_cast<const array_t<const int32_t>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::UINT: {
	const array_t<const uint32_t>* vals =
	    static_cast<const array_t<const uint32_t>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::LONG: {
	const array_t<const int64_t>* vals =
	    static_cast<const array_t<const int64_t>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::ULONG: {
	const array_t<const uint64_t>* vals =
	    static_cast<const array_t<const uint64_t>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::FLOAT: {
	const array_t<const float>* vals =
	    static_cast<const array_t<const float>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::DOUBLE: {
	const array_t<const double>* vals =
	    static_cast<const array_t<const double>*>(buffer[j].cval);
	out << (*vals)[i];
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	const std::vector<std::string>* vals =
	    static_cast<const std::vector<std::string>*>(buffer[j].cval);
	out << '"' << (*vals)[i] << '"';
	break;}
    default: {
	return -2;}
    }
    return 0;
} // ibis::bord::cursor::dumpIJ

inline int
ibis::bord::cursor::getColumnAsByte(const char* cn, char& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsByte((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsByte

inline int
ibis::bord::cursor::getColumnAsUByte(const char* cn,
				     unsigned char& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsUByte((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsUByte

inline int
ibis::bord::cursor::getColumnAsShort(const char* cn, int16_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsShort((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsShort

inline int
ibis::bord::cursor::getColumnAsUShort(const char* cn, uint16_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsUShort((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsUShort

inline int
ibis::bord::cursor::getColumnAsInt(const char* cn, int32_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsInt((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsInt

inline int
ibis::bord::cursor::getColumnAsUInt(const char* cn, uint32_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsUInt((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsUInt

inline int
ibis::bord::cursor::getColumnAsLong(const char* cn, int64_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsLong((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsLong

inline int
ibis::bord::cursor::getColumnAsULong(const char* cn, uint64_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsULong((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsULong

inline int
ibis::bord::cursor::getColumnAsFloat(const char* cn, float& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsFloat((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsFloat

inline int
ibis::bord::cursor::getColumnAsDouble(const char* cn, double& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsDouble((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsDouble

inline int
ibis::bord::cursor::getColumnAsString(const char* cn,
				      std::string& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows() || cn == 0 || *cn == 0)
	return -1;
    bufferMap::const_iterator it = bufmap.find(cn);
    if (it != bufmap.end())
	return getColumnAsString((*it).second, val);
    else
	return -2;
} // ibis::bord::cursor::getColumnAsString
#endif // IBIS_BORD_H

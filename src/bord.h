// File: $Id$
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
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
/// ibis::table::select produces an ibis::bord object if the query
/// produce nontrivial results.
///
/// @note Bord is a Danish word for "table."
class ibis::bord : public ibis::table {
public:
    typedef std::vector<void *> bufferList;
    bord(const char *tn, const char *td, uint64_t nr,
	 const ibis::table::stringList &cn,
	 const ibis::table::typeList &ct,
	 const bufferList &buf, const ibis::table::stringList *cdesc=0);
    virtual ~bord() {clear();}

    virtual uint64_t nRows() const {return mypart.nRows();}
    virtual size_t nColumns() const {return mypart.nColumns();}

    virtual ibis::table::stringList columnNames() const;
    virtual ibis::table::typeList columnTypes() const;

    virtual void describe(std::ostream&) const;
    virtual void dumpNames(std::ostream&, const char*) const;
    virtual int dump(std::ostream&, const char*) const;
    virtual int dump(std::ostream&, uint64_t, const char*) const;

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
    virtual int64_t getColumnAsDoubles(const char*,
				       std::vector<double>&) const;
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
    virtual table* groupby(const ibis::table::stringList&) const;
    virtual table* groupby(const char* str) const {
	return ibis::table::groupby(str);
    }
    virtual void orderby(const ibis::table::stringList&);
    virtual void orderby(const char* str) {
	ibis::table::orderby(str);
    }
    virtual void reverseRows();

    virtual int buildIndex(const char*, const char*) {return -1;}
    virtual int buildIndexes(const char*) {return -1;}
    virtual const char* indexSpec(const char*) const {return 0;}
    virtual void indexSpec(const char*, const char*) {return;}

    // Cursor class for row-wise data accesses.
    class cursor;
    /// Create a @c cursor object to perform row-wise data access.
    virtual ibis::table::cursor* createCursor() const;

    int restoreCategoriesAsStrings(const ibis::part&, const char*);

protected:
    class column;
    /// An in-memory data partition.
    class part : virtual public ibis::part {
    public:
	part(const char *tn, const char *td, uint64_t nr,
	     const ibis::table::stringList &cn,
	     const ibis::table::typeList   &ct,
	     const ibis::bord::bufferList  &buf,
	     const ibis::table::stringList *cdesc=0);

	template <typename E>
	long doScan(const array_t<E>& varr,
		    const ibis::qContinuousRange& cmp,
		    const ibis::bitvector& mask,
		    ibis::bitvector& hits) const {
	    return ibis::part::doScan(varr, cmp, mask, hits);}

	ibis::table* groupby(const ibis::table::stringList&) const;
	virtual long reorder(const ibis::table::stringList&);
	virtual long reorder() {return ibis::part::reorder();}

	virtual int dump(std::ostream&, size_t, const char*) const;

	void describe(std::ostream&) const;
	void dumpNames(std::ostream&, const char*) const;
	void reverseRows();
	int limit(size_t);

	template <typename T>
	long reorderValues(array_t<T>& vals,
			   const array_t<uint32_t>& indin,
			   array_t<uint32_t>& indout,
			   array_t<uint32_t>& starts) const;
	template <typename T>
	long reorderValues(array_t<T>& vals,
			   const array_t<uint32_t>& ind) const;
	long reorderStrings(std::vector<std::string>& vals,
			    const array_t<uint32_t>& ind) const;

	int restoreCategoriesAsStrings(const ibis::part&, const char*);

    private:
	part();
	part(const part&);
	part& operator=(const part&);
    }; // ibis::bord::part

    part mypart; ///< The data partition for an in-memory table.

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
    column(const ibis::bord::part* tbl, ibis::TYPE_T t,
	   const char* name, void *buf,
	   const char* desc="", double low=DBL_MAX, double high=-DBL_MAX);
    column(const ibis::bord::part*, const ibis::column&, void *buf);
    column(const column& rhs);
    virtual ~column();

    virtual ibis::fileManager::storage* getRawData() const;

    virtual long evaluateRange(const ibis::qContinuousRange& cmp,
			       const ibis::bitvector& mask,
			       ibis::bitvector& res) const;
    virtual array_t<int32_t>* selectInts(const ibis::bitvector& mask) const;
    virtual array_t<uint32_t>* selectUInts(const ibis::bitvector& mask) const;
    virtual array_t<int64_t>* selectLongs(const ibis::bitvector& mask) const;
    virtual array_t<float>* selectFloats(const ibis::bitvector& mask) const;
    virtual array_t<double>* selectDoubles(const ibis::bitvector& mask) const;

    virtual void computeMinMax() {
	computeMinMax(thePart->currentDataDir(), lower, upper);}
    virtual void computeMinMax(const char *dir) {
	computeMinMax(dir, lower, upper);}
    virtual void computeMinMax(const char *, double &min, double &max) const;
    virtual void getString(uint32_t i, std::string &val) const;

    void reverseRows();
    int  limit(size_t nr);

    void* getArray() const {return buffer;}
    template <typename T> int getRawData(array_t<T> &vals) const;
    int dump(std::ostream& out, size_t i) const;

    int restoreCategoriesAsStrings(const ibis::part&);

protected:
    /// The in-memory storage.  A pointer to an array<T> or
    /// std::vector<std::string> depending on data type.
    void *buffer;

    column& operator=(const column&);
}; // ibis::bord::column

class ibis::bord::cursor : public ibis::table::cursor {
public:
    cursor(const ibis::bord& t);
    virtual ~cursor() {};

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

    virtual int getColumnAsByte(size_t, char&) const;
    virtual int getColumnAsUByte(size_t, unsigned char&) const;
    virtual int getColumnAsShort(size_t, int16_t&) const;
    virtual int getColumnAsUShort(size_t, uint16_t&) const;
    virtual int getColumnAsInt(size_t, int32_t&) const;
    virtual int getColumnAsUInt(size_t, uint32_t&) const;
    virtual int getColumnAsLong(size_t, int64_t&) const;
    virtual int getColumnAsULong(size_t, uint64_t&) const;
    virtual int getColumnAsFloat(size_t, float&) const;
    virtual int getColumnAsDouble(size_t, double&) const;
    virtual int getColumnAsString(size_t, std::string&) const;

protected:
    struct bufferElement {
	const char* cname;
	ibis::TYPE_T ctype;
	void* cval;

	bufferElement() : cname(0), ctype(ibis::UNKNOWN_TYPE), cval(0) {}
    }; // bufferElement
    typedef std::map<const char*, size_t, ibis::lessi> bufferMap;
    std::vector<bufferElement> buffer;
    bufferMap bufmap;
    const ibis::bord& tab;
    int64_t curRow; // the current row number

    void fillRow(ibis::table::row& res) const;
    int dumpIJ(std::ostream&, size_t, size_t) const;

private:
    cursor();
    cursor(const cursor&);
    cursor& operator=(const cursor&);
}; // ibis::bord::cursor

inline void ibis::bord::describe(std::ostream &out) const {
    mypart.describe(out);
} // ibis::bord::describe

inline void ibis::bord::dumpNames(std::ostream &out, const char* del) const {
    mypart.dumpNames(out, del);
} // ibis::bord::dumpNames

inline int ibis::bord::dump(std::ostream &out, const char* del) const {
    return mypart.dump(out, mypart.nRows(), del);
} // ibis::bord::dump

inline int ibis::bord::dump(std::ostream &out, uint64_t nr,
			    const char* del) const {
    return mypart.dump(out, static_cast<size_t>(nr), del);
} // ibis::bord::dump

inline ibis::table* 
ibis::bord::groupby(const ibis::table::stringList& keys) const {
    return mypart.groupby(keys);
} // ibis::bord::groupby

inline void ibis::bord::orderby(const ibis::table::stringList& keys) {
    mypart.reorder(keys);
} // ibis::bord::orderby

inline void ibis::bord::reverseRows() {
    mypart.reverseRows();
} // ibis::bord::reverseRows

inline ibis::table::cursor* ibis::bord::createCursor() const {
    return new ibis::bord::cursor(*this);
} // ibis::bord::createCursor

/// Convert the integer representation to string representation.
inline int 
ibis::bord::restoreCategoriesAsStrings(const ibis::part& prt, const char* nm) {
    return mypart.restoreCategoriesAsStrings(prt, nm);
} // ibis::bord::restoreCategoriesAsStrings

/// Convert the integer representation to string representation.
inline int
ibis::bord::part::restoreCategoriesAsStrings(const ibis::part& prt,
					     const char* nm) {
    if (nm == 0 || *nm == 0)
	return -1;
    ibis::bord::column *col = static_cast<ibis::bord::column*>(getColumn(nm));
    if (col != 0)
	return col->restoreCategoriesAsStrings(prt);
    else
	return -2;
} // ibis::bord::part::restoreCategoriesAsStrings

/// Retrieve the raw data buffer as an ibis::array_t object.
///@note NO type check, caller need to make sure the currect type is
///specified.  In addition, string valued columns are stored as
///std::vector<std::string> not ibis::array_t!
template <typename T>
inline int ibis::bord::column::getRawData(array_t<T> &vals) const {
    array_t<T> tmp(*static_cast<const array_t<T>*>(buffer));
    vals.swap(tmp);
    return 0;
} // ibis::bord::column::getRawData

inline ibis::fileManager::storage*
ibis::bord::column::getRawData() const {
    return 0;
} // ibis::bord::column::getRawData

inline int ibis::bord::column::dump(std::ostream& out, size_t i) const {
    int ierr = -1;
    if (buffer == 0) return ierr;

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
	out << tmp;
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
ibis::bord::cursor::dumpIJ(std::ostream& out, size_t i, size_t j) const {
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

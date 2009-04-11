// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2007-2009 the Regents of the University of California
#ifndef IBIS_TAFEL_H
#define IBIS_TAFEL_H
#include "table.h"	// ibis::table
#include "bitvector.h"	// ibis::bitvector

///@file
/// An expandable table.  This file defines ibis::tafel.

namespace ibis {
    class tafel;
}

/// An expandable table.  It inherents from ibis::tablex only therefore
/// does not support any querying functions.  It stores all its content in
/// memory before the function write is called, therefore, it can only
/// handle relatively small number of records.
///
///@note The word tafel is a German word for table.
class ibis::tafel : public ibis::tablex {
public:
    tafel() : nrows(0U) {}
    virtual ~tafel() {clear();}

    virtual int addColumn(const char* cname, ibis::TYPE_T ctype);
    virtual int addColumn(const char* cname, ibis::TYPE_T ctype,
			  const char* cdesc);
    virtual int append(const char* cname, uint64_t begin, uint64_t end,
		       void* values);
    virtual int appendRow(const ibis::table::row&);
    virtual int appendRow(const char*, const char*);
    virtual int appendRows(const std::vector<ibis::table::row>&);
    virtual int readCSV(const char* filename, const int maxrows=0,
			const char* delimiters=0);
    virtual int write(const char* dir, const char* tname,
		      const char* tdesc) const;
    virtual void clearData();
    virtual void reserveSpace(unsigned);
    virtual unsigned capacity() const;

protected:
    /// In-memory version of a column.
    struct column {
	std::string name;
	std::string desc;
	ibis::TYPE_T type;
	/// For fix-sized elements, this is a pointer to an array_t
	/// object.  For string-valued elements, this is a pointer to
	/// std::vector<std::string>.
	void* values;
	/// Valid values correspond to 1, null values correspond to 0.
	ibis::bitvector mask;

	column() : type(ibis::UNKNOWN_TYPE), values(0) {}
	~column();
    }; // column
    typedef std::map<const char*, column*, ibis::lessi> columnList;
    /// List of columns in alphabetical order.
    columnList cols;
    /// Order of columns as they were specified through @c addColumn.
    std::vector<column*> colorder;
    /// Number of rows of this table.
    ibis::bitvector::word_t nrows;

    /// Clear all content.  Removes both data and metadata.
    void clear();

    /// Make all short columns catch up with the longest one.
    void normalize();

    template <typename T>
    void append(const T* in, ibis::bitvector::word_t be,
		ibis::bitvector::word_t en, array_t<T>& out,
		const T& fill, ibis::bitvector& mask) const;
    void appendString(const std::vector<std::string>* in,
		      ibis::bitvector::word_t be,
		      ibis::bitvector::word_t en,
		      std::vector<std::string>& out,
		      ibis::bitvector& mask) const;

    template <typename T>
    void locate(ibis::TYPE_T, std::vector<array_t<T>*>& buf) const;
    void locateString(ibis::TYPE_T t,
		      std::vector<std::vector<std::string>*>& buf) const;
    template <typename T>
    void append(const std::vector<std::string>& nm, const std::vector<T>& va,
		std::vector<array_t<T>*>& buf);
    void appendString(const std::vector<std::string>& nm,
		      const std::vector<std::string>& va,
		      std::vector<std::vector<std::string>*>& buf);
    int parseLine(const char* str, const char* del, const char* id);

    template <typename T>
    int writeColumn(int fdes, ibis::bitvector::word_t nold,
		    ibis::bitvector::word_t nnew,
		    const array_t<T>& vals, const T& fill,
		    ibis::bitvector& totmask,
		    const ibis::bitvector& newmask) const;
    int writeString(int fdes, ibis::bitvector::word_t nold,
		    ibis::bitvector::word_t nnew,
		    const std::vector<std::string>& vals,
		    ibis::bitvector& totmask,
		    const ibis::bitvector& newmask) const;

private:
    tafel(const tafel&);
    const tafel& operator=(const tafel&);
}; // class ibis::tafel
#endif // IBIS_TAFEL_H

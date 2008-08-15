// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2007-2008 the Regents of the University of California
#ifndef IBIS_TABLE_H
#define IBIS_TABLE_H
/**@file

   @brief FastBit Table Interface

   This is a facade to provide a high-level view of operations are on
   relational tables.  Two main classes are defined here, @c table and @c
   tablex.  The class @c table is for read-only data and it provides mostly
   querying functions.  The class @c tablex is for users to add new records
   to a table and it does not support any querying operations.
 */
#include <ostream>	// std::ostream
#include <vector>	// std::vector
#include <map>		// std::map
#include <string>	// std::string
#include <cstdlib>	// size_t
#include "const.h"	// intxx_t, uintxx_t, ... from stdint.h

namespace ibis {

    /// Supported data types.
    enum TYPE_T {UNKNOWN_TYPE=0,///< Unknown type, can't really do
				///  anything with it.
		 OID,	///< A special eight-byte ID type for
			///  internal use.
		 BYTE,	///< One-byte signed integers.
		 UBYTE,	///< One-byte unsigned integers.
		 SHORT,	///< Two-byte signed integers.
		 USHORT,///< Two-byte unsigned integers.
		 INT,	///< Four-byte signed integers.
		 UINT,	///< Four-byte unsigned integers.
		 LONG,	///< Eight-byte signed integers.
		 ULONG,	///< Eight-byte unsigned integers.
		 FLOAT,	///< Four-byte IEEE floating-point numbers.
		 DOUBLE,///< Eight-byte IEEE floating-point numbers.
		 CATEGORY,	///< Low cardinality null-terminated strings.
		 TEXT	///< Arbitrary null-terminated strings.
    };
    /// Human readable version of the enumeration types.
    FASTBIT_CXX_DLLSPEC extern const char** TYPESTRING;
    /// One-character code for the enumeration types.
    FASTBIT_CXX_DLLSPEC extern const char* TYPECODE;

    class table;
    class tablex;
    class tableList;
} // namespace ibis

/// @ingroup FastBitMain
/// The abstract table class.
/// This is an abstract base class that defines the common operations on a
/// data table.  Conceptually, data records in a table is organized into
/// rows and columns.  A query on a table produces a filtered version of
/// the table.  In many database systems this is known as a view on a
/// table.  All data tables and views are logically treated as
/// specialization of this ibis::table class.
class FASTBIT_CXX_DLLSPEC ibis::table {
public:
    /// Create a table object from the specified data directory.
    static ibis::table* create(const char* dir);
    /// Create a table object from a pair of data directories.  The
    /// intention of maintaining two sets of data files is to continue
    /// processing queries using one set while accepting new data records
    /// with the other.  However, such functionality is not currently
    /// implemented!
    static ibis::table* create(const char* dir1, const char* dir2);

    /// Destructor.
    virtual ~table() {};

    /// Name of the table object.
    virtual const char* name() const {return name_.c_str();}
    /// Free text description.
    virtual const char* description() const {return desc_.c_str();}
    virtual uint64_t nRows() const=0;
    virtual size_t nColumns() const=0;

    /// A list of strings.
    typedef std::vector<const char*> stringList;
    /// A list of data types.
    typedef std::vector<ibis::TYPE_T> typeList;
    /// An associate array of names and types.
    typedef std::map<const char*, ibis::TYPE_T, ibis::lessi> namesTypes;

    virtual stringList columnNames() const=0; ///< Return column names.
    virtual typeList columnTypes() const=0; ///< Return data types.

    /// Print a description of the table to the specified output stream.
    virtual void describe(std::ostream&) const=0;
    /// Print the values in ASCII form to the specified output stream.  The
    /// default delimiter is coma (","), which produces
    /// Comma-Separated-Values (CSV).
    virtual int dump(std::ostream& out, const char* del=", ") const=0;
    /// Print the first nr rows.
    virtual int dump(std::ostream& out, uint64_t nr) const=0;
    /// Estimate the number of rows satisfying the selection conditions.
    /// The number of rows is between [@c nmin, @c nmax].
    virtual void estimate(const char* cond,
			  uint64_t& nmin, uint64_t& nmax) const=0;
    /// Given a set of column names and a set of selection conditions,
    /// compute another table that represents the selected values.
    virtual table* select(const char* sel, const char* cond) const=0;

    /// Perform aggregate functions on the current table.  It produces a
    /// new table.  The list of strings passed to this function are
    /// interpreted as a set of names followed by a set of functions.
    /// Currently, only functions COUNT, AVG, MIN, MAX, and SUM are
    /// supported, and the functions can only accept a column name as
    /// arguments.
    virtual table* groupby(const stringList&) const=0;
    /// Perform group-by operation.  The column names and operations are
    /// separated by comma.
    virtual table* groupby(const char*) const;
    /// Reorder the rows.  Sort the rows in ascending order of the columns
    /// specified in the list of column names.  This function is not
    /// designated @c const because though it does not change the content
    /// in SQL logic, but it may change internal representations.
    /// @note If an empty list is passed to this function, it will reorder
    /// rows using all columns with the column having the smallest number
    /// of distinct values first.
    virtual void orderby(const stringList&)=0;
    /// Reorder the rows.  The column names are separated by comma.
    virtual void orderby(const char*);
    /// Reverse the order of the rows.
    virtual void reverseRows()=0;

    /// Add data partition defined in the named directory.  It returns 0 to
    /// indicate success, a negative number to indicate failure, and a
    /// positive number to indicate some adversary conditions.
    /// @note On systems that supports readdir and friend (all
    /// unix-type of systems do), it also recursively traverses all
    /// subdirectories.
    virtual int addPartition(const char* dir) {return -1;}

    /// @{

    /// Create the index for the named column.  The existing index will be
    /// replaced.  If an indexing option is not specified, it will use the
    /// internally recorded option for the named column or the table
    /// containing the column.
    ///
    /// @note Unless any there is a specific instruction to not index a
    /// column, the querying functions will automatically build indices as
    /// necessary.  However, as building an index is relatively expensive
    /// process, building an index on a column is on average about four or
    /// five times as expensive as reading the column from disk, this
    /// function is provided so that it is possible to build indexes
    /// beforehand.
    virtual int buildIndex(const char* colname, const char* option=0) =0;
    /// Create indexes for every column of the table.  Existing indexes
    /// will be replaced.  If an indexing option is not specified, the
    /// internally recorded options will be used.
    /// @sa buildIndex
    virtual int buildIndexes(const char* options=0) =0;
    /// Retrieve the current indexing option.  If no column name is
    /// specified, it retrieve the indexing option for the table.
    virtual const char* indexSpec(const char* colname=0) const=0;
    /// Replace the current indexing option.  If no column name is
    /// specified, it resets the indexing option for the table.
    virtual void indexSpec(const char* opt, const char* colname=0) =0;
    /// @}

    /// Retrieve all values of the named column.  The member functions of
    /// this class only support access to whole column at a time.  Use @c
    /// table::cursor class for row-wise accesses.  For fixed-width data
    /// types, the raw pointers are used to point to the values to be
    /// returned.  In these cases, the caller is responsible for allocating
    /// enough storage for the values to be returned.
    /// @{
    virtual int64_t getColumnAsBytes(const char* cname, char* vals) const=0;
    virtual int64_t getColumnAsUBytes(const char* cname,
				      unsigned char* vals) const=0;
    virtual int64_t getColumnAsShorts(const char* cname,
				      int16_t* vals) const=0;
    virtual int64_t getColumnAsUShorts(const char* cname,
				       uint16_t* vals) const=0;
    virtual int64_t getColumnAsInts(const char* cname,
				    int32_t* vals) const=0;
    virtual int64_t getColumnAsUInts(const char* cname,
				     uint32_t* vals) const=0;
    virtual int64_t getColumnAsLongs(const char* cname,
				     int64_t* vals) const=0;
    virtual int64_t getColumnAsULongs(const char* cname,
				      uint64_t* vals) const=0;
    virtual int64_t getColumnAsFloats(const char* cname,
				      float* vals) const=0;
    virtual int64_t getColumnAsDoubles(const char* cname,
				       double* vals) const=0;
    /// Retrieve the null-terminated strings as a vector of std::string
    /// objects.  Both ibis::CATEGORY and ibis::TEXT types can be retrieved
    /// using this function.
    virtual int64_t getColumnAsStrings(const char* cname,
				       std::vector<std::string>& vals) const=0;
    /// @}

    /// @{
    /// Compute the histogram of the named column.  This version uses the
    /// user specified bins:
    /// @code [begin, begin+stride) [begin+stride, begin+2*stride) ....@endcode
    /// A record is placed in bin
    /// @code (x - begin) / stride, @endcode where the first bin is bin 0.
    /// The total number of bins is
    /// @code (end - begin) / stride. @endcode
    /// @note Records (rows) outside of the range [begin, end] are not
    /// counted.
    /// @note Non-positive @c stride is considered as an error.
    /// @note If @c end is less than @c begin, an empty array @c counts is
    /// returned along with return value 0.
    virtual long getHistogram(const char* constraints,
			      const char* cname,
			      double begin, double end, double stride,
			      std::vector<size_t>& counts) const=0;
    /// Compute a two-dimension histogram on columns @c cname1 and @c
    /// cname2.  The bins along each dimension are defined the same way as
    /// in function @c getHistogram.  The array @c counts stores the
    /// two-dimensional bins with the first dimension as the slow varying
    /// dimension following C convention for ordering multi-dimensional
    /// arrays.
    virtual long getHistogram2D(const char* constraints,
				const char* cname1,
				double begin1, double end1, double stride1,
				const char* cname2,
				double begin2, double end2, double stride2,
				std::vector<size_t>& counts) const=0;
    /// Compute a three-dimensional histogram on the named columns.  The
    /// triplets <begin, end, stride> are used the same ways in @c
    /// getHistogram and @c getHistogram2D.  The three dimensional bins
    /// are linearized in @c counts with the first being the slowest
    /// varying dimension and the third being the fastest varying dimension
    /// following the C convention for ordering multi-dimensional arrays.
    virtual long getHistogram3D(const char* constraints,
				const char* cname1,
				double begin1, double end1, double stride1,
				const char* cname2,
				double begin2, double end2, double stride2,
				const char* cname3,
				double begin3, double end3, double stride3,
				std::vector<size_t>& counts) const=0;
    /// @}

    /// A simple struct for storing a row of a table.
    struct row {
	std::vector<std::string>   bytesnames; ///< For ibis::BYTE.
	std::vector<signed char>   bytesvalues;
	std::vector<std::string>   ubytesnames; ///< For ibis::UBYTE.
	std::vector<unsigned char> ubytesvalues;
	std::vector<std::string>   shortsnames; ///< For ibis::SHORT.
	std::vector<int16_t>       shortsvalues;
	std::vector<std::string>   ushortsnames; ///< For ibis::USHORT.
	std::vector<uint16_t>      ushortsvalues;
	std::vector<std::string>   intsnames; ///< For ibis::INT.
	std::vector<int32_t>       intsvalues;
	std::vector<std::string>   uintsnames; ///< For ibis::UINT.
	std::vector<uint32_t>      uintsvalues;
	std::vector<std::string>   longsnames; ///< For ibis::LONG.
	std::vector<int64_t>       longsvalues;
	std::vector<std::string>   ulongsnames; ///< For ibis::ULONG.
	std::vector<uint64_t>      ulongsvalues;
	std::vector<std::string>   floatsnames; ///< For ibis::FLOAT.
	std::vector<float>         floatsvalues;
	std::vector<std::string>   doublesnames; ///< For ibis::DOUBLE.
	std::vector<double>        doublesvalues;
	std::vector<std::string>   catsnames; ///< For ibis::CATEGORY.
	std::vector<std::string>   catsvalues;
	std::vector<std::string>   textsnames; ///< For ibis::TEXT.
	std::vector<std::string>   textsvalues;

	/// Clear all names and values.
	void clear();
	/// Clear the content of arrays of values.  Leave the names alone.
	void clearValues();
    }; // struct row

    // Cursor class for row-wise data accesses.
    class cursor;
    /// Create a @c cursor object to perform row-wise data access.
    virtual cursor* createCursor() const=0;

protected:

    std::string name_;	///< Name of the table.
    std::string desc_;	///< Description of the table.

    /// The default constructor.
    table() {};
    /// Copy constructor.
    table(const char* na, const char* de) : name_(na), desc_(de) {};
    /// Parse a string into a set of names.  Some bytes may be turned into
    /// 0 to mark the end of names or functions.
    void parseNames(char* in, stringList& out) const;

private:
    // re-enforce the prohibitions on copying and assignment.
    table(const table&);
    const table& operator=(const table&);
}; // class ibis::table

/// @ingroup FastBitMain
/// The class for expandable tables.
/// @note Each function that returns an integer returns 0 in case of
/// success, a negative value in case error and a positive number as
/// advisory information.
class FASTBIT_CXX_DLLSPEC ibis::tablex {
public:
    /// Create a minimalistic table exclusively for entering new records.
    static ibis::tablex* create();
//     /// Make the incoming table expandable.  Not yet implemented
//     static ibis::tablex* makeExtensible(ibis::table* t);

    virtual ~tablex() {}; // nothing to do.

    /// Add a column.
    virtual int addColumn(const char* cname, ibis::TYPE_T ctype) =0;
    virtual int addColumn(const char* cname, ibis::TYPE_T ctype,
			  const char* cdesc) =0;
    /// Add values to the named column.  The column name must be in the
    /// table already.  The first value is to be placed at row @c begin (the
    /// row numbers start with 0) and the last value before row @c end.
    /// The array @c values must contain values of the correct type
    /// corresponding to the type specified before.
    ///
    /// The expected types of values are "const std::vector<std::string>*"
    /// for string valued columns, and "const T*" for a fix-sized column of
    /// type T.  More specifically, if the column type is float, the type
    /// of values is "const float*"; if the column type is category, the
    /// type of values is "const std::vector<std::string>*".
    ///
    /// @note Since each column may have different number of rows filled,
    /// the number of rows in the table is considered to be the maximum
    /// number of rows filled of all columns.
    ///
    /// @note This function can not be used to introduce new columns in a
    /// table.  A new column must be added with @c addColumn.
    ///
    /// @sa appendRow
    virtual int append(const char* cname, uint64_t begin, uint64_t end,
		       void* values) =0;

    /// Add one row.  If an array of names has the same number of elements
    /// as the array of values, the names are used as column names.  If the
    /// names are not specified explicitly, the values are assigned to the
    /// columns of the same data type in the order as they are specified
    /// through @c addColumn or if the same order as they are recreated
    /// from an existing dataset (which is typically alphabetical).
    ///
    /// @note The column names are not case-sensitive.
    ///
    /// @note Like @c append, this function can not be used to introduce
    /// new columns in a table.  A new column must be added with @c
    /// addColumn.
    ///
    /// @note Since the various columns may have different numbers of rows
    /// filled, the number of rows in the table is assumed to the largest
    /// number of rows filled so far.  The new row appended here increases
    /// the number of rows in the table by 1.  The unfilled rows are
    /// assumed to be null.
    ///
    /// @note A null value of an integer column is recorded as the maximum
    /// possible of the type of integer.  A null value of a floating-point
    /// valued column is recorded as a quiet NaN (Not-a-Number).  A null
    /// value of a string-valued column is recorded as an empty string.  In
    /// all cases, a null mask is used to indicate that they are null
    /// values.
    virtual int appendRow(const ibis::table::row&) =0;
    /// Append a row stored in ASCII form.  The ASCII form of the values
    /// are assumed to be separated by comma (,) or space, but additional
    /// delimiters may be added through the second argument.
    virtual int appendRow(const char* line, const char* delimiters=0) = 0;
    /// Add multiple rows.  Rows in the incoming vector are processed on
    /// after another.  The ordering of the values in earlier rows are
    /// automatically carried over to the later rows until another set of
    /// names is specified.
    /// @sa appendRow
    virtual int appendRows(const std::vector<ibis::table::row>&) =0;

    /// Read the content of the specified as comma-separated values.
    /// Append the records to this table.  By default the records are
    /// delimited by comma (,) and blank space.  One may specify additional
    /// delimiters using the second argument.
    virtual int readCSV(const char* filename, const char* delimiters=0) =0;

    /// Write the in-memory data records to the specified directory on
    /// disk.  If the table name (@c tname) is a null string or an empty,
    /// the last component of the directory name is used.  If the
    /// description (@c tdesc) is a null string or an empty string, a time
    /// stamp will be printed in its place.  If the specified directory
    /// already contains data, the new records will be appended to the
    /// existing data.  In this case, the table name specified here will
    /// overwrite the existing name, but the existing name and description
    /// will be retained if the current arguments are null strings or empty
    /// strings.  The data type associated with this table will overwrite
    /// the existing data type information.
    virtual int write(const char* dir, const char* tname,
		      const char* tdesc) const =0;

    /// Remove all data recorded.  Keeps the metadata.  It is intended to
    /// be used after a call to function write to store new rows.
    virtual void clearData() =0;

protected:
    tablex() {}; // Derived classes need this.

private:
    tablex(const tablex&); // no copying
    const tablex& operator=(const tablex&); // no assignment
}; // class ibis::tablex

/// A list of tables.  It supports simple lookup through operator[] and
/// manages the table objects passed to it.  Most functions are simply
/// wrappers on std::map.
class FASTBIT_CXX_DLLSPEC ibis::tableList {
public:
    typedef std::map< const char*, ibis::table*, ibis::lessi > tableSet;
    typedef tableSet::const_iterator iterator;

    /// Is the list empty? Returns true of the list is empty, otherwise
    /// returns false.
    bool empty() const {return tables.empty();}
    /// Return the number of tables in the list.
    size_t size() const {return tables.size();}
    /// Return the iterator to the first table.
    iterator begin() const {return tables.begin();}
    /// Return the iterator to the end of the list.  Following STL
    /// convention, the @c end is always one past the last element in the
    /// list.
    iterator end() const {return tables.end();}

    /// Find the named table.  Returns null pointer if no table with the
    /// given name is found.
    const ibis::table* operator[](const char* tname) const {
	tableSet::const_iterator it = tables.find(tname);
	if (it != tables.end())
	    return (*it).second;
	else
	    return 0;
    }

    /// Add a new table object to the list.  Transfers the control of the
    /// object to the tableList.  If the name of the table already exists,
    /// the existing table will be passed back out, otherwise, the argument
    /// @c tb is set to null.  In either case, the caller can call delete
    /// on the variable and should do so to avoid memory leak.
    void add(ibis::table*& tb) {
	tableSet::iterator it = tables.find(tb->name());
	if (it == tables.end()) {
	    tables[tb->name()] = tb;
	    tb=0;
	}
	else {
	    ibis::table* tmp = (*it).second;
	    tables[tb->name()] = tb;
	    tb = tmp;
	}
    }

    /// Remove the named data table from the list.  The destructor of this
    /// function automatically clean up all table objects, there is no need
    /// to explicit remove them.
    void remove(const char* tname) {
	tableSet::iterator it = tables.find(tname);
	if (it != tables.end()) {
	    ibis::table* tmp = (*it).second;
	    tables.erase(it);
	    delete tmp;
	}
    }

    /// Default constructor.
    tableList() {};

    /// Destructor.  Delete all table objects.
    ~tableList() {
	while (! tables.empty()) {
	    tableSet::iterator it = tables.begin();
	    ibis::table* tmp = (*it).second;
	    tables.erase(it);
	    delete tmp;
	}
    }

private:
    /// Actual storage of the sets of tables.
    tableSet tables;

    // Can not copy or assign.
    tableList(const tableList&);
    const tableList& operator=(const tableList&);
}; // ibis::tableList

/// Cursor class for row-wise data accesses.
/// @note Note that this cursor is associated with a table object and can
/// only iterate overall rows of a table.  To iterate an arbitrary
/// selection of rows, use the select function to create a new table and then
/// iterate over the new table.
class FASTBIT_CXX_DLLSPEC ibis::table::cursor {
public:
    virtual ~cursor() {};
    virtual uint64_t nRows() const=0;
    virtual size_t nColumns() const=0;
    virtual ibis::table::stringList columnNames() const=0;
    virtual ibis::table::typeList columnTypes() const=0;
    /// Make the next row of the data set available for retrieval.  Returns
    /// 0 if successful, returns a negative number to indicate error.
    virtual int fetch() =0;
    /// Make the specified row in the data set available for retrieval.
    /// Returns 0 if the specified row is found, returns a negative number
    /// to indicate error, such as @c rownum out of range (-1).
    virtual int fetch(uint64_t rownum) =0;
    /// Return the current row number.  Rows in a data set are numbered [0
    /// - @c nRows()-1].  If the cursor is not ready, such as before the
    /// first call to @c fetch or function @c fetch returned an error, this
    /// function return the same value as function @c nRows.
    virtual uint64_t getCurrentRowNumber() const=0;

    /// Fetch the content of the next row and make the next row as the
    /// current row as well.
    virtual int fetch(ibis::table::row&) =0;
    /// Fetch the content of the specified row and make that row the
    /// current row as well.
    virtual int fetch(uint64_t rownum, ibis::table::row&) =0;

    /// Print out the values of the current row.
    virtual int dump(std::ostream& out, const char* del=", ") const=0;

    /// Retrieve the value of the named column.
    /// @note Note the cost of name lookup is likely to dominate the total
    /// cost of such a function.
    virtual int getColumnAsByte(const char* cname, char*) const=0;
    virtual int getColumnAsUByte(const char* cname, unsigned char*) const=0;
    virtual int getColumnAsShort(const char* cname, int16_t*) const=0;
    virtual int getColumnAsUShort(const char* cname, uint16_t*) const=0;
    virtual int getColumnAsInt(const char* cname, int32_t*) const=0;
    virtual int getColumnAsUInt(const char* cname, uint32_t*) const=0;
    virtual int getColumnAsLong(const char* cname, int64_t*) const=0;
    virtual int getColumnAsULong(const char* cname, uint64_t*) const=0;
    virtual int getColumnAsFloat(const char* cname, float*) const=0;
    virtual int getColumnAsDouble(const char* cname, double*) const=0;
    virtual int getColumnAsString(const char* cname, std::string&) const=0;

    /// This version of getColumnAsTTT directly use the column number, i.e.,
    /// the position of a column in the list returned by function @c
    /// columnNames or @c columnTypes.  This version of the data access
    /// function may be able to avoid the name lookup and reduce the
    /// execution time.
    virtual int getColumnAsByte(size_t cnum, char* val) const=0;
    virtual int getColumnAsUByte(size_t cnum, unsigned char* val) const=0;
    virtual int getColumnAsShort(size_t cnum, int16_t* val) const=0;
    virtual int getColumnAsUShort(size_t cnum, uint16_t* val) const=0;
    virtual int getColumnAsInt(size_t cnum, int32_t* val) const=0;
    virtual int getColumnAsUInt(size_t cnum, uint32_t* val) const=0;
    virtual int getColumnAsLong(size_t cnum, int64_t* val) const=0;
    virtual int getColumnAsULong(size_t cnum, uint64_t* val) const=0;
    virtual int getColumnAsFloat(size_t cnum, float* val) const=0;
    virtual int getColumnAsDouble(size_t cnum, double* val) const=0;
    virtual int getColumnAsString(size_t cnum, std::string& val) const=0;

protected:
    cursor() {};
    cursor(const cursor&); // not implemented
    cursor& operator=(const cursor&) ; // not implemented
}; // ibis::table::cursor

inline void ibis::table::row::clear() {
    bytesnames.clear();
    bytesvalues.clear();
    ubytesnames.clear();
    ubytesvalues.clear();
    shortsnames.clear();
    shortsvalues.clear();
    ushortsnames.clear();
    ushortsvalues.clear();
    intsnames.clear();
    intsvalues.clear();
    uintsnames.clear();
    uintsvalues.clear();
    longsnames.clear();
    longsvalues.clear();
    ulongsnames.clear();
    ulongsvalues.clear();
    floatsnames.clear();
    floatsvalues.clear();
    doublesnames.clear();
    doublesvalues.clear();
    catsnames.clear();
    catsvalues.clear();
    textsnames.clear();
    textsvalues.clear();
} // ibis::table::row::clear

inline void ibis::table::row::clearValues() {
    bytesvalues.clear();
    ubytesvalues.clear();
    shortsvalues.clear();
    ushortsvalues.clear();
    intsvalues.clear();
    uintsvalues.clear();
    longsvalues.clear();
    ulongsvalues.clear();
    floatsvalues.clear();
    doublesvalues.clear();
    catsvalues.clear();
    textsvalues.clear();
} // ibis::table::row::clearValues

inline ibis::table* ibis::table::groupby(const char* str) const {
    stringList lst;
    char* buf = 0;
    if (str != 0 && *str != 0) {
	buf = new char[strlen(str)+1];
	strcpy(buf, str);
	parseNames(buf, lst);
    }
    ibis::table* res = groupby(lst);
    delete [] buf;
    return res;
} // ibis::table::groupby

inline void ibis::table::orderby(const char* str) {
    stringList lst;
    char* buf = 0;
    if (str != 0 && *str != 0) {
	buf = new char[strlen(str)+1];
	strcpy(buf, str);
	parseNames(buf, lst);
    }
    orderby(lst);
    delete [] buf;
} // ibis::table::orderby
#endif // IBIS_TABLE_H

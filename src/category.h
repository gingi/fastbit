//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2010 the Regents of the University of California
#ifndef IBIS_CATEGORY_H
#define IBIS_CATEGORY_H
///@file
/// Define two specialization of the column class.
///
/// IBIS represents incoming data table with vertical partitioning.  Each
/// column object represents one column of the relational table.  The terms
/// used to describe each column of the table are strongly influenced by
/// the first project using this software, a high-energy physics experiment
/// named STAR.
///
#include "irelic.h"
#include "column.h"

/// A data structure for storing null-terminated text.  The only type of
/// search supported on this type of data is keyword search.
/// The keyword search operation is implemented through a boolean
/// term-document matrix (ibis::keywords) that has to be generated
/// externally.
class ibis::text : public ibis::column {
public:
    virtual ~text() {unloadIndex();};
    text(const part* tbl, FILE* file);
    text(const part* tbl, const char* name, ibis::TYPE_T t=ibis::TEXT);
    text(const ibis::column& col); // copy from column

    virtual long keywordSearch(const char* str, ibis::bitvector& hits) const;
    virtual long keywordSearch(const char* str) const;
    //     long keywordSearch(const std::vector<std::string>& strs) const;
    //     long keywordSearch(const std::vector<std::string>& strs,
    // 		       ibis::bitvector& hits) const;

    virtual long stringSearch(const char* str, ibis::bitvector& hits) const;
    virtual long stringSearch(const std::vector<std::string>& strs,
			      ibis::bitvector& hits) const;
    virtual long stringSearch(const char* str) const {
	ibis::bitvector tmp;
	long ierr = stringSearch(str, tmp);
	return (ierr < 0 ? ierr : tmp.cnt());
    }
    virtual long stringSearch(const std::vector<std::string>& strs) const {
	ibis::bitvector tmp;
	long ierr = stringSearch(strs, tmp);
	return (ierr < 0 ? ierr : tmp.cnt());
    }

    virtual double estimateCost(const ibis::qString& cmp) const;
    virtual double estimateCost(const ibis::qMultiString& cmp) const;
    virtual double estimateCost(const ibis::qContinuousRange& cmp) const {
	return ibis::column::estimateCost(cmp);}
    virtual double estimateCost(const ibis::qDiscreteRange& cmp) const {
	return ibis::column::estimateCost(cmp);}

    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf);
    virtual long saveSelected(const ibis::bitvector& sel, const char *dest,
			      char *buf, uint32_t nbuf);
    /// Return the positions of records marked 1 in the mask.
    virtual array_t<uint32_t>* selectUInts(const bitvector& mask) const;
    /// Return the starting positions of strings marked 1 in the mask.
    virtual array_t<int64_t>* selectLongs(const bitvector& mask) const;
    virtual
    std::vector<std::string>* selectStrings(const bitvector& mask) const;
    virtual const char* findString(const char* str) const;
    virtual void getString(uint32_t i, std::string &val) const {
	readString(i, val);}

    virtual void write(FILE* file) const; ///< Write the metadata entry.
    virtual void print(std::ostream& out) const; ///< Print header info.

    const column* IDColumnForKeywordIndex() const;

protected:
    /// Locate the starting position of each string.
    void startPositions(const char *dir, char *buf, uint32_t nbuf) const;
    /// Read the string value of <code>i</code>th row.
    void readString(uint32_t i, std::string &val) const;
    /// Read one string from an open file.
    int  readString(std::string&, int, long, long, char*, uint32_t,
		    uint32_t&, off_t&) const;
    int  writeStrings(const char *to, const char *from,
		      const char *spto, const char *spfrom,
		      ibis::bitvector &msk, const ibis::bitvector &sel,
		      char *buf, uint32_t nbuf) const;

private:
    text& operator=(const text&);
}; // ibis::text

/// Provide a dual-directional mapping between strings and integers.  A
/// utility class used by ibis::category.  The NULL string is always the
/// 0th string.
class ibis::dictionary {
public:
    typedef std::map< const char*, uint32_t, ibis::lessi > wordList;

    ~dictionary() {clear();}
    dictionary(const dictionary& dic);
    /// Default constructor.  Generates one (NULL) entry.
    dictionary() : svec(), s2i(), ncontig(1) {
	svec.push_back(const_cast<char*>(nullstring.c_str()));
    }

    /// Return the number of valid (not null) strings in the dictionary.
    uint32_t size() const {return s2i.size();}
    inline const char* operator[](uint32_t i) const;
    inline uint32_t operator[](const char* str) const;
    inline const char* find(const char* str) const;
    inline uint32_t insert(const char* str);
    inline uint32_t insertRaw(char* str);

    void clear();
    void read(const char* name);
    void write(const char* name) const;

protected:

    /// Member variable svec contains pointers to the location of the
    /// strings and is considered the owner of the memory allocated.
    ///
    /// @note The string svec[0] is always saved for the null string.
    /// Strings svec[1:ncontig-1] are allocated in a single contiguous
    /// piece of memory.  Strings svec[ncontig:..] are allocated one at a
    /// time.
    std::vector<char*> svec;
    /// Member variable s2i maps each string to its index in svec.
    wordList s2i;
    /// The number of contiguous entries.  These entries are contiguous in
    /// memory and therefore should be freed by a single call to delete [].
    uint32_t ncontig;

    /// The common null string use in the dictionaries.  A char* of 0 and a
    /// blank string are interpreted as null strings.
    ///
    /// @note The null string is printed out as "<NULL>".  However, it is
    /// possible for the user to have a string with the same value
    /// "<NULL>".  If the user insert "<NULL>" to a dictionary, it will
    /// receive an assignment of number back that is guaranteed to be not
    /// 0.  Therefore, the user string "<NULL>" is NOT treated as a null
    /// string.
    static std::string nullstring;
    void copy(const dictionary& rhs);

private:
    dictionary& operator=(const dictionary&);
}; // ibis::dictionary

/// A specialized low-cardinality text field.  It is also known as control
/// values or categorical values.  This implementation directly converts
/// string values into bitvectors (as ibis::relic), and does not store
/// integer version of the string.
///
/// @note The integer zero (0) is reserved for NULL values.
class ibis::category : public ibis::text {
public:
    virtual ~category();
    category(const part* tbl, FILE* file);
    category(const part* tbl, const char* name);
    category(const ibis::column& col); // copy from column
    // a special construct for meta-tag attributes
    category(const part* tbl, const char* name, const char* value,
	     const char* dir=0, uint32_t nevt=0);

    /// Match a particular string.
    virtual long stringSearch(const char* str, ibis::bitvector& hits) const;
    /// Match a list of strings
    virtual long stringSearch(const std::vector<std::string>& vals,
			      ibis::bitvector& hits) const;
    /// Estimate the number of matches.
    virtual long stringSearch(const char* str) const;
    /// Estimate the total number of matches for a list of strings.
    virtual long stringSearch(const std::vector<std::string>& vals) const;

    virtual long patternSearch(const char* pat) const;
    virtual long patternSearch(const char* pat, ibis::bitvector &hits) const;
    virtual double estimateCost(const ibis::qLike& cmp) const;
    virtual double estimateCost(const ibis::qString& cmp) const;
    virtual double estimateCost(const ibis::qMultiString& cmp) const;
    virtual double estimateCost(const ibis::qContinuousRange& cmp) const {
	return ibis::column::estimateCost(cmp);}
    virtual double estimateCost(const ibis::qDiscreteRange& cmp) const {
	return ibis::column::estimateCost(cmp);}

    /// Append the content in @a df to the directory @a dt.
    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf);
    /// Return the integers corresponding to the select strings.
    virtual array_t<uint32_t>* selectUInts(const bitvector& mask) const;
    virtual std::vector<std::string>*
    selectStrings(const bitvector& mask) const {
	return ibis::text::selectStrings(mask);}
    virtual void getString(uint32_t i, std::string &val) const;

    /// Return the number of key values.
    virtual uint32_t getNumKeys() const {return dic.size();}
    /// Return the ith value in the dictionary.
    virtual const char* getKey(uint32_t i) const {return dic[i];}
    /// Is the given string one of the keys in the dictionary?  Return a
    /// null pointer if not.
    virtual const char* isKey(const char* str) const {
	return dic.find(str);}

    virtual void write(FILE* file) const; ///< Write the metadata entry.
    virtual void print(std::ostream& out) const; ///< Print header info.

    ibis::relic* fillIndex(const char *dir=0) const;

private:
    // private member variables

    // dictionary is mutable in order to delay the reading of dictionary
    // from disk as late as possible
    mutable ibis::dictionary dic;

    // private member functions
    void prepareMembers() const;
    void readDictionary(const char *dir=0) const;

    category& operator=(const category&);
}; // ibis::category

/// A class to provide minimal support for byte arrays.  Since a byte array
/// may contain any arbitrary byte values, we can not rely on the null
/// terminator any more, nor use std::string as the container for each
/// array.  It is intended to store opaque data that can not be searched.
class ibis::blob : public ibis::column {
public:
    virtual ~blob() {};
    blob(const part*, FILE*);
    blob(const part*, const char*);
    blob(const ibis::column&);

    virtual long stringSearch(const char*, ibis::bitvector&) const {return -1;}

    virtual void computeMinMax() {}
    virtual void computeMinMax(const char*) {}
    virtual void computeMinMax(const char*, double&, double&) const {}
    virtual void loadIndex(const char*, int) const throw () {}
    virtual long indexSize() const {return -1;}
    virtual int  getValuesArray(void*) const {return -1;}

    virtual array_t<char>* selectBytes(const bitvector&) const {return 0;}
    virtual array_t<unsigned char>* selectUBytes(const bitvector&) const {return 0;}
    virtual array_t<int16_t>* selectShorts(const bitvector&) const {return 0;}
    virtual array_t<uint16_t>* selectUShorts(const bitvector&) const {return 0;}
    virtual array_t<int32_t>* selectInts(const bitvector&) const {return 0;}
    virtual array_t<uint32_t>* selectUInts(const bitvector&) const {return 0;}
    virtual array_t<int64_t>* selectLongs(const bitvector&) const {return 0;}
    virtual array_t<uint64_t>* selectULongs(const bitvector&) const {return 0;}
    virtual array_t<float>* selectFloats(const bitvector&) const {return 0;}
    virtual array_t<double>* selectDoubles(const bitvector&) const {return 0;}
    virtual std::vector<std::string>* selectStrings(const bitvector&) const {return 0;}

    virtual long estimateRange(const ibis::qContinuousRange&,
			       ibis::bitvector&,
			       ibis::bitvector&) const {return -1;}
    virtual long estimateRange(const ibis::qDiscreteRange&,
			       ibis::bitvector&,
			       ibis::bitvector&) const {return -1;}
    virtual long evaluateRange(const ibis::qContinuousRange&,
			       const ibis::bitvector&,
			       ibis::bitvector&) const {return -1;}
    virtual long evaluateRange(const ibis::qDiscreteRange&,
			       const ibis::bitvector&,
			       ibis::bitvector&) const {return -1;}
    virtual long estimateRange(const ibis::qContinuousRange&) const {return -1;}
    virtual long estimateRange(const ibis::qDiscreteRange&) const {return -1;}
    virtual double estimateCost(const ibis::qContinuousRange&) const {return 0;}
    virtual double estimateCost(const ibis::qDiscreteRange& cmp) const {return 0;}
    virtual double estimateCost(const ibis::qString&) const {return 0;}
    virtual double estimateCost(const ibis::qMultiString&) const {return 0;}

    virtual float getUndecidable(const ibis::qContinuousRange&,
				 ibis::bitvector&) const {return 1;}
    virtual float getUndecidable(const ibis::qDiscreteRange&,
				 ibis::bitvector&) const {return 1;}

    virtual double getActualMin() const {return DBL_MAX;}
    virtual double getActualMax() const {return -DBL_MAX;}
    virtual double getSum() const {return 0;}
    virtual long append(const void*, const ibis::bitvector&) {return -1;}

    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf);
    virtual long writeData(const char* dir, uint32_t nold, uint32_t nnew,
			   ibis::bitvector& mask, const void *va1,
			   void *va2);

    virtual void write(FILE*) const;
    virtual void print(std::ostream&) const;

    long countRawBytes(const bitvector&) const;
    int selectRawBytes(const bitvector&,
		       array_t<unsigned char>&, array_t<uint32_t>&) const;
    int getBlob(uint32_t ind, unsigned char *&buf, uint32_t &size) const;

protected:
    int extractAll(const bitvector&,
		   array_t<unsigned char>&, array_t<uint32_t>&,
		   const array_t<unsigned char>&,
		   const array_t<int64_t>&) const;
    int extractSome(const bitvector&,
		    array_t<unsigned char>&, array_t<uint32_t>&,
		    const array_t<unsigned char>&, const array_t<int64_t>&,
		    const uint32_t) const;
    int extractAll(const bitvector&,
		   array_t<unsigned char>&, array_t<uint32_t>&,
		   const char*, const array_t<int64_t>&) const;
    int extractSome(const bitvector&,
		    array_t<unsigned char>&, array_t<uint32_t>&,
		    const char*, const array_t<int64_t>&, const uint32_t) const;
    int extractSome(const bitvector&,
		    array_t<unsigned char>&, array_t<uint32_t>&,
		    const char*, const char*, const uint32_t) const;
    int readBlob(uint32_t ind, unsigned char *&buf, uint32_t &size,
		 const array_t<int64_t> &starts, const char *datafile) const;
    int readBlob(uint32_t ind, unsigned char *&buf, uint32_t &size,
		 const char *spfile, const char *datafile) const;
}; // ibis::blob

/// Return a string corresponding to the integer.  If the index is beyond
/// the valid range, i.e., i > size(), then a null pointer will be
/// returned.
inline const char* ibis::dictionary::operator[](uint32_t i) const {
    if (i < svec.size()) {
	return svec[i];
    }
    else {
	return static_cast<const char*>(0);
    }
} // int to string

/// Convert a string to integer.  Returns 0 for empty (null) strings,
/// 1:size() for strings in the dictionary, and dictionary::size()+1 for
/// unknown values.
inline uint32_t ibis::dictionary::operator[](const char* str) const {
    if (str == 0) return 0;
    if (*str == 0) return 0;
    wordList::const_iterator it = s2i.find(str);
    if (it != s2i.end()) return (*it).second;
    else return svec.size();
} // string to int

/// If the input string is found in the dictionary, it returns the string.
/// Otherwise it returns null pointer.  This function makes a little easier
/// to determine whether a string is in a dictionary.
inline const char* ibis::dictionary::find(const char* str) const {
    const char* ret = 0;
    if (s2i.find(str) != s2i.end())
	ret = str;
    return ret;
} // ibis::dictionary::find

/// Insert a string to the dictionary.  Returns the integer value assigned
/// to the string.  A copy of the string is stored internally.
inline uint32_t ibis::dictionary::insert(const char* str) {
    if (str == 0) return 0;
    if (*str == 0) return 0;
    wordList::const_iterator it = s2i.find(str);
    if (it != s2i.end()) {
	return (*it).second;
    }
    else {
	uint32_t ret = svec.size();
	char* tmp = ibis::util::strnewdup(str); // make a copy
	svec.push_back(tmp);
	s2i[tmp] = ret;
	return ret;
    }
} // ibis::dictionary::insert

/// Non-copying insert.  Do not make a copy of the input string.  Transfers
/// the ownership of @c str to the dictionary.  Caller needs to check
/// whether it is a new word in the dictionary.  If it is not a new word in
/// the dictionary, the dictionary does not take ownership of the string
/// argument.
inline uint32_t ibis::dictionary::insertRaw(char* str) {
    if (str == 0) return 0;
    if (*str == 0) return 0;
    wordList::const_iterator it = s2i.find(str);
    if (it != s2i.end()) {
	return (*it).second;
    }
    else {
	uint32_t ret = svec.size();
	svec.push_back(str);
	s2i[str] = ret;
	return ret;
    }
} // ibis::dictionary::insertRaw
#endif // IBIS_CATEGORY_H

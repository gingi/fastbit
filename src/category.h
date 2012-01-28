//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
#ifndef IBIS_CATEGORY_H
#define IBIS_CATEGORY_H
///@file
/// Define three specialization of the column class.
///
/// IBIS represents incoming data table with vertical partitioning.  Each
/// column object represents one column of the relational table.  The terms
/// used to describe each column of the table are strongly influenced by
/// the first project using this software, a high-energy physics experiment
/// named STAR.
///
#include "irelic.h"
#include "column.h"
#include "dictionary.h"

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
    virtual long stringSearch(const char* str) const;
    virtual long stringSearch(const std::vector<std::string>& strs) const;
    virtual long patternSearch(const char*, ibis::bitvector&) const;
    virtual long patternSearch(const char*) const;

    using ibis::column::estimateCost;
    virtual double estimateCost(const ibis::qString& cmp) const;
    virtual double estimateCost(const ibis::qMultiString& cmp) const;

    virtual void loadIndex(const char* iopt=0, int ropt=0) const throw ();
    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf);
    virtual long append(const void*, const ibis::bitvector&) {return -1;}
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
    void TDListForKeywordIndex(std::string&) const;
    void delimitersForKeywordIndex(std::string&) const;

    /// A tokenizer class to turn a string buffer into tokens.  Used by
    /// ibis::keywords to build a term-document index.
    struct tokenizer {
	/// A tokenizer must implement a two-argument operator().  It takes
	/// an input string in buf to produce a list of tokens in tkns.  The
	/// input buffer may be modified in this function.  The return
	/// value shall be zero (0) to indicate success, and a positive
	/// value to carray a warning message and a negative value to
	/// indicate fatal error.
	virtual int operator()(std::vector<const char*>& tkns, char *buf) = 0;
	/// Destructor.
	virtual ~tokenizer() {}
    }; // struct tokenizer

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
    using ibis::text::estimateCost;
    virtual double estimateCost(const ibis::qLike& cmp) const;
    virtual double estimateCost(const ibis::qString& cmp) const;
    virtual double estimateCost(const ibis::qMultiString& cmp) const;

    virtual void loadIndex(const char*, int) const throw ();
    /// Append the content in @a df to the directory @a dt.
    virtual long append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf);
    virtual long append(const void*, const ibis::bitvector&) {return -1;}
    /// Return the integers corresponding to the select strings.
    virtual array_t<uint32_t>* selectUInts(const bitvector& mask) const;
    virtual std::vector<std::string>*
    selectStrings(const bitvector& mask) const;
    virtual void getString(uint32_t i, std::string &val) const;

    virtual uint32_t getNumKeys() const;
    virtual const char* getKey(uint32_t i) const;
    virtual const char* isKey(const char* str) const;

    virtual void write(FILE* file) const;
    virtual void print(std::ostream& out) const;

    ibis::relic* fillIndex(const char *dir=0) const;
    /// Return a pointer to the dictionary used for the categorical values.
    const ibis::dictionary* getDictionary() const {return &dic;}

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
    virtual long stringSearch(const std::vector<std::string>&,
			      ibis::bitvector&) const {return -1;}
    virtual long stringSearch(const char*) const {return -1;}
    virtual long stringSearch(const std::vector<std::string>&) const {
	return -1;}

    virtual void computeMinMax() {}
    virtual void computeMinMax(const char*) {}
    virtual void computeMinMax(const char*, double&, double&) const {}
    virtual void loadIndex(const char*, int) const throw () {}
    virtual long indexSize() const {return -1;}
    virtual int  getValuesArray(void*) const {return -1;}

    virtual array_t<signed char>* selectBytes(const bitvector&) const {return 0;}
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

    // virtual long estimateRange(const ibis::qContinuousRange&,
    // 			       ibis::bitvector&,
    // 			       ibis::bitvector&) const {return -1;}
    // virtual long estimateRange(const ibis::qDiscreteRange&,
    // 			       ibis::bitvector&,
    // 			       ibis::bitvector&) const {return -1;}
    // virtual long evaluateRange(const ibis::qContinuousRange&,
    // 			       const ibis::bitvector&,
    // 			       ibis::bitvector&) const {return -1;}
    // virtual long evaluateRange(const ibis::qDiscreteRange&,
    // 			       const ibis::bitvector&,
    // 			       ibis::bitvector&) const {return -1;}
    // virtual long estimateRange(const ibis::qContinuousRange&) const {return -1;}
    // virtual long estimateRange(const ibis::qDiscreteRange&) const {return -1;}
    // virtual double estimateCost(const ibis::qContinuousRange&) const {return 0;}
    // virtual double estimateCost(const ibis::qDiscreteRange& cmp) const {return 0;}
    // virtual double estimateCost(const ibis::qString&) const {return 0;}
    // virtual double estimateCost(const ibis::qMultiString&) const {return 0;}

    // virtual float getUndecidable(const ibis::qContinuousRange&,
    // 				 ibis::bitvector&) const {return 1;}
    // virtual float getUndecidable(const ibis::qDiscreteRange&,
    // 				 ibis::bitvector&) const {return 1;}

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
#endif // IBIS_CATEGORY_H

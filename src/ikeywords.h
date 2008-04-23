//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2006-2008 the Regents of the University of California
#ifndef IBIS_KEYWORDS_H
#define IBIS_KEYWORDS_H
///@file
/// This index is a keyword index for a string-valued column.  It
/// contains a boolean version of the term-document matrix and supports
/// exact matches of keywords/terms.

#include "index.h"	// base index class
#include "category.h"	// definitions of string-valued columns

/// Class ibis::keywords defines a boolean term-document matrix.  The
/// terms are stored in an ibis::dictionary and the bits are stored in a
/// series of bitvectors.
///
/// The current implementation requires a @c .tdlist file alongside the
/// binary string values.  In addition, it uses an entry in the ibis.rc
/// file of the following form to specify the ids used in the @c .tdlist
/// file.
///
/// <table-name>.<column-name>.docIDName=<id-column-name>
///
/// For example,
///
/// enrondata.subject.docIDName=mid
/// enrondata.body.docIDName=mid
///
/// If an ID column is not specified, the integer IDs in the @c .tdlist
/// file is assumed to the row number.
class ibis::keywords : public ibis::index {
public:
    virtual ~keywords() {clear();}
    keywords(const ibis::column* c,
	     const ibis::column* idcol=0, const char* f=0);
    keywords(const ibis::column* c, ibis::fileManager::storage* st,
	     uint32_t offset = 8);

    virtual INDEX_TYPE type() const {return KEYWORDS;}
    virtual const char* name() const {return "keywords";}
    virtual void binBoundaries(std::vector<double>& b) const {b.clear();}
    virtual void binWeights(std::vector<uint32_t>& b) const;
    virtual double getMin() const {return DBL_MAX;}
    virtual double getMax() const {return -DBL_MAX;}
    virtual double getSum() const {return -DBL_MAX;}
    /// Match a particular keyword.
    long search(const char* kw, ibis::bitvector& hits) const;
    /// Estimate the number of matches.
    long search(const char* kw) const;

    virtual void print(std::ostream& out) const;
    virtual int write(const char* dt) const;
    virtual int read(const char* idxfile);
    virtual int read(ibis::fileManager::storage* st);
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;
    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    /// This class and its derived classes should produce exact answers,
    /// therefore no undecidable rows.
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const {
	iffy.clear();
	return 0.0;
    }
    virtual double estimateCost(const ibis::qContinuousRange& expr) const;
    virtual double estimateCost(const ibis::qDiscreteRange& expr) const;

protected:
    /// Reads a term-document list from a external file.  Returns the
    /// number of terms found if successful, otherwise returns a negative
    /// number to indicate error.
    int readTermDocFile(const ibis::column* idcol, const char* f);
    /// Extract the keyword from a line of input term-document file.
    inline char readKeyword(const char*& buf, std::string &key) const;
    /// Extract the next integer in an inputline.
    inline uint32_t readUInt(const char*& buf) const;
    /// Read and process one line from the term-docuement file.
    int readLine(std::istream& in, std::string& key,
		 std::vector<uint32_t>& idlist,
		 char* buf, uint32_t nbuf) const;
    void setBits(std::vector<uint32_t>& pos, ibis::bitvector& bvec) const;

    /// Clear the current content.
    void clear();

private:
    ibis::dictionary terms;	//< A dictionary for the terms.
}; // class ibis::keywords

/// A keyword is any number of printable characters.  Returns the first
/// non-space character after the keyword.
inline char ibis::keywords::readKeyword(const char*& buf,
					std::string &keyword) const {
    while (isspace(*buf)) // skip leading space
	++ buf;
    while (isprint(*buf) && !(isspace(*buf) || *buf == ':')) {
	keyword += *buf;
	++ buf;
    }
    while (isspace(*buf)) // skip trailing space
	++ buf;
    return *buf;
} // ibis::keywords::readKeyword

inline uint32_t ibis::keywords::readUInt(const char*& buf) const {
    uint32_t res = 0;
    while (*buf && ! isdigit(*buf)) // skip leading non-digit
	++ buf;

    while (isdigit(*buf)) {
	res = res * 10 + (*buf - '0');
	++ buf;
    }
    return res;
} // ibis::keywords::readUInt
#endif

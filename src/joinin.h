// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008-2009 the Regents of the University of California
#ifndef IBIS_JOININ_H
#define IBIS_JOININ_H
/**@file
   @brief FastBit In-memory Natual Join.

   This is a concrete implementation of the join operation involving two
   data partitions that can fit in memory.
 */
#include "join.h"	// ibis::join

namespace ibis {
    class joinIN; // forward definition
} // namespace ibis

/// FastBit In-memory Natual Join.
class ibis::joinIN : public ibis::join {
public:
    joinIN(const ibis::part& partr, const ibis::part& parts,
	   const char* colname, const char* condr, const char* conds);
    virtual ~joinIN();

    virtual void estimate(uint64_t& nmin, uint64_t& nmax);
    virtual int64_t evaluate();

    virtual ibis::join::result*
    select(const std::vector<const char*>& colnames);

    class result;

protected:
    const ibis::part& R_;
    const ibis::part& S_;
    const ibis::column *colR_;
    const ibis::column *colS_;
    ibis::bitvector maskR_;
    ibis::bitvector maskS_;
    array_t<uint32_t> *orderR_;
    array_t<uint32_t> *orderS_;
    void *valR_;
    void *valS_;
    std::string desc_;
    int64_t nrows;

    friend class ibis::joinIN::result;
    static void freeBuffer(void* buffer, ibis::TYPE_T type);

private:
    joinIN(const joinIN&); // no copying
    joinIN& operator=(const joinIN&); // no assignment
}; // class ibis::joinIN

class ibis::joinIN::result : public ibis::join::result {
public:
    virtual ~result();
    result(const ibis::joinIN& jin, const std::vector<const char*>& colnames);

    virtual uint64_t nRows() const {return jin_.nrows;}
    virtual size_t nColumns() const {return 0;}

    virtual std::vector<std::string> columnNames() const;
    virtual ibis::table::typeList columnTypes() const;
    virtual void describe(std::ostream& out) const;

    virtual int fetch();
    virtual int dump(std::ostream& out, const char* del=", ") const;

    virtual int getColumnAsByte(const char* cname, char*) const;
    virtual int getColumnAsUByte(const char* cname, unsigned char*) const;
    virtual int getColumnAsShort(const char* cname, int16_t*) const;
    virtual int getColumnAsUShort(const char* cname, uint16_t*) const;
    virtual int getColumnAsInt(const char* cname, int32_t*) const;
    virtual int getColumnAsUInt(const char* cname, uint32_t*) const;
    virtual int getColumnAsLong(const char* cname, int64_t*) const;
    virtual int getColumnAsULong(const char* cname, uint64_t*) const;
    virtual int getColumnAsFloat(const char* cname, float*) const;
    virtual int getColumnAsDouble(const char* cname, double*) const;
    virtual int getColumnAsString(const char* cname, std::string&) const;

    virtual int getColumnAsByte(size_t cnum, char* val) const;
    virtual int getColumnAsUByte(size_t cnum, unsigned char* val) const;
    virtual int getColumnAsShort(size_t cnum, int16_t* val) const;
    virtual int getColumnAsUShort(size_t cnum, uint16_t* val) const;
    virtual int getColumnAsInt(size_t cnum, int32_t* val) const;
    virtual int getColumnAsUInt(size_t cnum, uint32_t* val) const;
    virtual int getColumnAsLong(size_t cnum, int64_t* val) const;
    virtual int getColumnAsULong(size_t cnum, uint64_t* val) const;
    virtual int getColumnAsFloat(size_t cnum, float* val) const;
    virtual int getColumnAsDouble(size_t cnum, double* val) const;
    virtual int getColumnAsString(size_t cnum, std::string& val) const;

protected:
    const ibis::joinIN& jin_;
    const uint32_t endR_, endS_;
    uint32_t currR_, currS_, blockR_, blockS_, startS_;
    std::vector<const ibis::column*> colR_, colS_;
    std::vector<ibis::TYPE_T> typeR_, typeS_;
    std::vector<void*> valR_, valS_;
    std::map<const char*, uint32_t, ibis::lessi> namesToPos;
    std::vector<uint32_t> ipToPos;

    template <typename T>
    int nextMatch(const array_t<T>& col1, const array_t<T>& col2);
    int stringMatch(const std::vector<std::string>& col1,
		    const std::vector<std::string>& col2);
    void dumpR(std::ostream& out, size_t ind) const;
    void dumpS(std::ostream& out, size_t ind) const;

private:
    result(const result&); // no copying
    result& operator=(const result&); // no assignment
}; // class ibis::joinIN::result

inline int
ibis::joinIN::result::getColumnAsByte(size_t cnum, char* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (typeR_[pos] == ibis::BYTE && colR_[pos] != 0) {
	    *val = (*(static_cast<array_t<char>*>(valR_[pos])))
		[(*jin_.orderR_)[currR_]];
	    ierr = 0;
	}
    }
    else {
	pos -= colR_.size();
	if (typeS_[pos] == ibis::BYTE && colS_[pos] != 0) {
	    *val = (*(static_cast<array_t<char>*>(valS_[pos])))
		[(*jin_.orderS_)[currS_]];
	    ierr = 0;
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsByte

inline int
ibis::joinIN::result::getColumnAsUByte(size_t cnum, unsigned char* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (typeR_[pos] == ibis::UBYTE && colR_[pos] != 0) {
	    *val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		[(*jin_.orderR_)[currR_]];
	    ierr = 0;
	}
    }
    else {
	pos -= colR_.size();
	if (typeS_[pos] == ibis::UBYTE && colS_[pos] != 0) {
	    *val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		[(*jin_.orderS_)[currS_]];
	    ierr = 0;
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsUByte

inline int
ibis::joinIN::result::getColumnAsShort(size_t cnum, int16_t* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsShort

inline int
ibis::joinIN::result::getColumnAsUShort(size_t cnum, uint16_t* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsUShort

inline int
ibis::joinIN::result::getColumnAsInt(size_t cnum, int32_t* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsInt

inline int
ibis::joinIN::result::getColumnAsUInt(size_t cnum, uint32_t* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsUInt

inline int
ibis::joinIN::result::getColumnAsLong(size_t cnum, int64_t* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::LONG:
		*val = (*(static_cast<array_t<int64_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::LONG:
		*val = (*(static_cast<array_t<int64_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsLong

inline int
ibis::joinIN::result::getColumnAsULong(size_t cnum, uint64_t* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::ULONG:
		*val = (*(static_cast<array_t<uint64_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::ULONG:
		*val = (*(static_cast<array_t<uint64_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsULong

inline int
ibis::joinIN::result::getColumnAsFloat(size_t cnum, float* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::FLOAT:
		*val = (*(static_cast<array_t<float>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::FLOAT:
		*val = (*(static_cast<array_t<float>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsFloat

inline int
ibis::joinIN::result::getColumnAsDouble(size_t cnum, double* val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::FLOAT:
		*val = (*(static_cast<array_t<float>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::DOUBLE:
		*val = (*(static_cast<array_t<double>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE:
		*val = (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		*val = (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::SHORT:
		*val = (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::USHORT:
		*val = (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::INT:
		*val = (*(static_cast<array_t<int32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::UINT:
		*val = (*(static_cast<array_t<uint32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::FLOAT:
		*val = (*(static_cast<array_t<float>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    case ibis::DOUBLE:
		*val = (*(static_cast<array_t<double>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		ierr = 0;
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsDouble

inline void
ibis::joinIN::result::dumpR(std::ostream& out, size_t ind) const {
    if (colR_[ind] == 0) return;
    switch (typeR_[ind]) {
    default: break;
    case ibis::BYTE:
	out << (int)(*(static_cast<const array_t<char>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::UBYTE:
	out << (int)(*(static_cast<const array_t<unsigned char>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::SHORT:
	out << (*(static_cast<const array_t<int16_t>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::USHORT:
	out << (*(static_cast<const array_t<uint16_t>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::INT:
	out << (*(static_cast<const array_t<int32_t>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::UINT:
	out << (*(static_cast<const array_t<uint32_t>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::LONG:
	out << (*(static_cast<const array_t<int64_t>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::ULONG:
	out << (*(static_cast<const array_t<uint64_t>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::FLOAT:
	out << (*(static_cast<const array_t<float>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::DOUBLE:
	out << (*(static_cast<const array_t<double>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	out << (*(static_cast<const std::vector<std::string>*>(valR_[ind])))
	    [(*jin_.orderR_)[currR_]];
	break;
    }
} // ibis::joinIN::result::dumpR

inline void
ibis::joinIN::result::dumpS(std::ostream& out, size_t ind) const {
    if (colS_[ind] == 0) return;
    switch (typeS_[ind]) {
    default: break;
    case ibis::BYTE:
	out << (int)(*(static_cast<const array_t<char>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::UBYTE:
	out << (int)(*(static_cast<const array_t<unsigned char>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::SHORT:
	out << (*(static_cast<const array_t<int16_t>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::USHORT:
	out << (*(static_cast<const array_t<uint16_t>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::INT:
	out << (*(static_cast<const array_t<int32_t>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::UINT:
	out << (*(static_cast<const array_t<uint32_t>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::LONG:
	out << (*(static_cast<const array_t<int64_t>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::ULONG:
	out << (*(static_cast<const array_t<uint64_t>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::FLOAT:
	out << (*(static_cast<const array_t<float>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::DOUBLE:
	out << (*(static_cast<const array_t<double>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	out << (*(static_cast<const std::vector<std::string>*>(valS_[ind])))
	    [(*jin_.orderS_)[currS_]];
	break;
    }
} // ibis::joinIN::result::dumpS
#endif


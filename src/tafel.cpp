// File $Id$
// Author: John Wu <John.Wu@ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2008 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tafel.h"	// ibis::tafel
#include "part.h"	// ibis::part

#include <fstream>	// std::ofstream
#include <limits>	// std::numeric_limits
#include <typeinfo>	// typeid

/// Return value 0 == success, -1 == invalid name or type, 1 == specified
/// name already in the list of columns.
int ibis::tafel::addColumn(const char* cn, ibis::TYPE_T ct) {
    if (cn == 0 || *cn == 0 || ct == ibis::UNKNOWN_TYPE) return -1;
    columnList::const_iterator it = cols.find(cn);
    if (it != cols.end()) return 1;

    column* col = new column();
    col->name = cn;
    col->type = ct;
    switch (ct) {
    case ibis::BYTE:
	col->values = new array_t<signed char>();
	break;
    case ibis::UBYTE:
	col->values = new array_t<unsigned char>();
	break;
    case ibis::SHORT:
	col->values = new array_t<int16_t>();
	break;
    case ibis::USHORT:
	col->values = new array_t<uint16_t>();
	break;
    case ibis::INT:
	col->values = new array_t<int32_t>();
	break;
    case ibis::UINT:
	col->values = new array_t<uint32_t>();
	break;
    case ibis::LONG:
	col->values = new array_t<int64_t>();
	break;
    case ibis::ULONG:
	col->values = new array_t<uint64_t>();
	break;
    case ibis::FLOAT:
	col->values = new array_t<float>();
	break;
    case ibis::DOUBLE:
	col->values = new array_t<double>();
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	col->values = new std::vector<std::string>();
	break;
    default:
	break;
    }
    cols[col->name.c_str()] = col;
    colorder.push_back(col);
    return 0;
} // ibis::tafel::addColumn

int ibis::tafel::addColumn(const char* cn, ibis::TYPE_T ct,
			   const char* cd) {
    if (cn == 0 || *cn == 0 || ct == ibis::UNKNOWN_TYPE ||
	cd == 0 || *cd == 0) return -1;
    columnList::const_iterator it = cols.find(cn);
    if (it != cols.end()) return 1;

    column* col = new column();
    col->name = cn;
    col->type = ct;
    col->desc = cd;
    switch (ct) {
    case ibis::BYTE:
	col->values = new array_t<signed char>();
	break;
    case ibis::UBYTE:
	col->values = new array_t<unsigned char>();
	break;
    case ibis::SHORT:
	col->values = new array_t<int16_t>();
	break;
    case ibis::USHORT:
	col->values = new array_t<uint16_t>();
	break;
    case ibis::INT:
	col->values = new array_t<int32_t>();
	break;
    case ibis::UINT:
	col->values = new array_t<uint32_t>();
	break;
    case ibis::LONG:
	col->values = new array_t<int64_t>();
	break;
    case ibis::ULONG:
	col->values = new array_t<uint64_t>();
	break;
    case ibis::FLOAT:
	col->values = new array_t<float>();
	break;
    case ibis::DOUBLE:
	col->values = new array_t<double>();
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	col->values = new std::vector<std::string>();
	break;
    default:
	break;
    }
    cols[col->name.c_str()] = col;
    colorder.push_back(col);
    return 0;
} // ibis::tafel::addColumn

template <typename T>
void ibis::tafel::append(const T* in, ibis::bitvector::word_t be,
			 ibis::bitvector::word_t en, array_t<T>& out,
			 const T& fill, ibis::bitvector& mask) const {
    ibis::bitvector inmsk;
    inmsk.appendFill(0, be);
    inmsk.appendFill(1, en-be);
    if (out.size() > en)
	inmsk.appendFill(0, out.size()-en);
    if (out.size() < be)
	out.insert(out.end(), be-out.size(), fill);
    if (out.size() < en) {
	out.resize(en);
	mask.adjustSize(0, en);
    }
    std::copy(in, in+(en-be), out.begin()+be);
    mask |= inmsk;
#if defined(_DEBUG)
    LOGGER(0) << "ibis::tafel::append(" << typeid(T).name()
			   << ", " << be << ", " << en << ")\ninmask: "
			   << inmsk << "totmask: " << mask;
#endif
} // ibis::tafel::append

void ibis::tafel::appendString(const std::vector<std::string>* in,
			       ibis::bitvector::word_t be,
			       ibis::bitvector::word_t en,
			       std::vector<std::string>& out,
			       ibis::bitvector& mask) const {
    ibis::bitvector inmsk;
    inmsk.appendFill(0, be);
    inmsk.appendFill(1, en-be);
    if (out.size() < be) {
	const std::string tmp;
	out.insert(out.end(), be-out.size(), tmp);
    }
    if (out.size() > en)
	inmsk.appendFill(0, out.size()-en);
    if (out.size() < en) {
	out.resize(en);
	mask.adjustSize(0, en);
    }
    std::copy(in->begin(), in->begin()+(en-be), out.begin()+be);
    mask |= inmsk;
#if defined(_DEBUG)
    LOGGER(0) << "ibis::tafel::appendString(" << be
			   << ", " << en << ")\ninmask: "
			   << inmsk << "totmask: " << mask;
#endif
} // ibis::tafel::appendString

int ibis::tafel::append(const char* cn, uint64_t begin, uint64_t end,
			void* values) {
    ibis::bitvector::word_t be = static_cast<ibis::bitvector::word_t>(begin);
    ibis::bitvector::word_t en = static_cast<ibis::bitvector::word_t>(end);
    if (be != begin || en != end || be >= en || cn == 0 || *cn == 0 ||
	values == 0) return -1;

    columnList::iterator it = cols.find(cn);
    if (it == cols.end()) return -2;

    if (en > nrows) nrows = en;
    column* col = (*it).second;
    switch (col->type) {
    case ibis::BYTE:
	append(static_cast<const signed char*>(values), be, en,
	       *static_cast<array_t<signed char>*>(col->values),
	       (signed char)0x7F, col->mask);
	break;
    case ibis::UBYTE:
	append(static_cast<const unsigned char*>(values), be, en,
	       *static_cast<array_t<unsigned char>*>(col->values),
	       (unsigned char)0xFFU, col->mask);
	break;
    case ibis::SHORT:
	append(static_cast<const int16_t*>(values), be, en,
	       *static_cast<array_t<int16_t>*>(col->values),
	       (int16_t)0x7FFF, col->mask);
	break;
    case ibis::USHORT:
	append(static_cast<const uint16_t*>(values), be, en,
	       *static_cast<array_t<uint16_t>*>(col->values),
	       (uint16_t)0xFFFF, col->mask);
	break;
    case ibis::INT:
	append(static_cast<const int32_t*>(values), be, en,
	       *static_cast<array_t<int32_t>*>(col->values),
	       (int32_t)0x7FFFFFFF, col->mask);
	break;
    case ibis::UINT:
	append(static_cast<const uint32_t*>(values), be, en,
	       *static_cast<array_t<uint32_t>*>(col->values),
	       (uint32_t)0xFFFFFFFFU, col->mask);
	break;
    case ibis::LONG:
	append<int64_t>(static_cast<const int64_t*>(values), be, en,
			*static_cast<array_t<int64_t>*>(col->values),
			0x7FFFFFFFFFFFFFFFLL, col->mask);
	break;
    case ibis::ULONG:
	append<uint64_t>(static_cast<const uint64_t*>(values), be, en,
			 *static_cast<array_t<uint64_t>*>(col->values),
			 0xFFFFFFFFFFFFFFFFULL, col->mask);
	break;
    case ibis::FLOAT:
	append(static_cast<const float*>(values), be, en,
	       *static_cast<array_t<float>*>(col->values),
	       std::numeric_limits<float>::quiet_NaN(), col->mask);
	break;
    case ibis::DOUBLE:
	append(static_cast<const double*>(values), be, en,
	       *static_cast<array_t<double>*>(col->values),
	       std::numeric_limits<double>::quiet_NaN(), col->mask);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	appendString(static_cast<const std::vector<std::string>*>(values),
		     be, en,
		     *static_cast<std::vector<std::string>*>(col->values),
		     col->mask);
	break;
    default:
	break;
    }
    return 0;
} // ibis::tafel::append

void ibis::tafel::normalize() {
    if (cols.empty() || nrows == 0) return;
    for (columnList::iterator it = cols.begin(); it != cols.end(); ++ it) {
	column& col = *((*it).second);
	switch (col.type) {
	case ibis::BYTE: {
	    array_t<signed char>& vals =
		* static_cast<array_t<signed char>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), 0x7F);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::UBYTE: {
	    array_t<unsigned char>& vals =
		* static_cast<array_t<unsigned char>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), 0xFFU);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::SHORT: {
	    array_t<int16_t>& vals =
		* static_cast<array_t<int16_t>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), 0x7FFF);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::USHORT: {
	    array_t<uint16_t>& vals =
		* static_cast<array_t<uint16_t>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), 0xFFFFU);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::INT: {
	    array_t<int32_t>& vals =
		* static_cast<array_t<int32_t>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), 0x7FFFFFFF);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::UINT: {
	    array_t<uint32_t>& vals =
		* static_cast<array_t<uint32_t>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), 0xFFFFFFFFU);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::LONG: {
	    array_t<int64_t>& vals =
		* static_cast<array_t<int64_t>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(),
			    0x7FFFFFFFFFFFFFFFLL);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t>& vals =
		* static_cast<array_t<uint64_t>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(),
			    0xFFFFFFFFFFFFFFFFULL);
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::FLOAT: {
	    array_t<float>& vals =
		* static_cast<array_t<float>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(),
			    std::numeric_limits<float>::quiet_NaN());
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>& vals =
		* static_cast<array_t<double>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(),
			    std::numeric_limits<double>::quiet_NaN());
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
	    std::vector<std::string>& vals =
		* static_cast<std::vector<std::string>*>(col.values);
	    if (vals.size() < nrows) {
		col.mask.adjustSize(vals.size(), nrows);
		vals.insert(vals.end(), nrows-vals.size(), "");
	    }
	    else if (vals.size() > nrows) {
		col.mask.adjustSize(nrows, nrows);
		vals.resize(nrows);
	    }
	    break;}
	default: {
	    break;}
	}
    }
} // ibis::tafel::normalize

template <typename T>
void ibis::tafel::locate(ibis::TYPE_T t, std::vector<array_t<T>*>& buf) const {
    buf.clear();
    for (size_t i = 0; i < colorder.size(); ++ i) {
	if (colorder[i]->type == t)
	    buf.push_back(static_cast<array_t<T>*>(colorder[i]->values));
    }
} // ibis::tafel::locate

void ibis::tafel::locateString(ibis::TYPE_T t,
			       std::vector<std::vector<std::string>*>& buf)
    const {
    buf.clear();
    for (size_t i = 0; i < colorder.size(); ++ i) {
	if (colorder[i]->type == t)
	    buf.push_back(static_cast<std::vector<std::string>*>
			  (colorder[i]->values));
    }
} // ibis::tafel::locateString

template <typename T>
void ibis::tafel::append(const std::vector<std::string>& nm,
			 const std::vector<T>& va,
			 std::vector<array_t<T>*>& buf) {
    const size_t n1 = (nm.size() <= va.size() ? nm.size() : va.size());
    for (size_t i = 0; i < n1; ++ i) {
	if (nm[i].empty()) {
	    if (buf.size() > i && buf[i] != 0)
		buf[i]->push_back(va[i]);
	}
	else {
	    columnList::iterator it = cols.find(nm[i].c_str());
	    if (it != cols.end()) {
		if (buf.size() < i) buf.resize(i+1);
		buf[i] = static_cast<array_t<T>*>((*it).second->values);
		buf[i]->push_back(va[i]);
	    }
	}
    }

    const size_t n2 = (va.size() <= buf.size() ? va.size() : buf.size());    
    for (size_t i = n1; i < n2; ++ i) {
	if (buf[i] != 0)
	    buf[i]->push_back(va[i]);
    }
} // ibis::tafel::append

void ibis::tafel::appendString(const std::vector<std::string>& nm,
			       const std::vector<std::string>& va,
			       std::vector<std::vector<std::string>*>& buf) {
    const size_t n1 = (nm.size() <= va.size() ? nm.size() : va.size());
    for (size_t i = 0; i < n1; ++ i) {
	if (nm[i].empty()) {
	    if (buf.size() > i && buf[i] != 0)
		buf[i]->push_back(va[i]);
	}
	else {
	    columnList::iterator it = cols.find(nm[i].c_str());
	    if (it != cols.end()) {
		if (buf.size() < i) buf.resize(i+1);
		buf[i] = static_cast<std::vector<std::string>*>
		    ((*it).second->values);
		buf[i]->push_back(va[i]);
	    }
	}
    }

    const size_t n2 = (va.size() <= buf.size() ? va.size() : buf.size());    
    for (size_t i = n1; i < n2; ++ i) {
	if (buf[i] != 0)
	    buf[i]->push_back(va[i]);
    }
} // ibis::tafel::appendString

int ibis::tafel::appendRow(const ibis::table::row& r) {
    normalize(); // make sure every column has the same number of rows
    size_t cnt = 0;
    if (r.bytesvalues.size() > 0) {
	std::vector<array_t<signed char>*> bytesptr;
	locate(ibis::BYTE, bytesptr);
	cnt += r.bytesvalues.size();
	append(r.bytesnames, r.bytesvalues, bytesptr);
    }
    if (r.ubytesvalues.size() > 0) {
	std::vector<array_t<unsigned char>*> ubytesptr;
	locate(ibis::UBYTE, ubytesptr);
	cnt += r.ubytesvalues.size();
	append(r.ubytesnames, r.ubytesvalues, ubytesptr);
    }
    if (r.shortsvalues.size() > 0) {
	std::vector<array_t<int16_t>*> shortsptr;
	locate(ibis::SHORT, shortsptr);
	cnt += r.shortsvalues.size();
	append(r.shortsnames, r.shortsvalues, shortsptr);
    }
    if (r.ushortsvalues.size() > 0) {
	std::vector<array_t<uint16_t>*> ushortsptr;
	locate(ibis::USHORT, ushortsptr);
	cnt += r.ushortsvalues.size();
	append(r.ushortsnames, r.ushortsvalues, ushortsptr);
    }
    if (r.intsvalues.size() > 0) {
	std::vector<array_t<int32_t>*> intsptr;
	locate(ibis::INT, intsptr);
	cnt += r.intsvalues.size();
	append(r.intsnames, r.intsvalues, intsptr);
    }
    if (r.uintsvalues.size() > 0) {
	std::vector<array_t<uint32_t>*> uintsptr;
	locate(ibis::UINT, uintsptr);
	cnt += r.uintsvalues.size();
	append(r.uintsnames, r.uintsvalues, uintsptr);
    }
    if (r.longsvalues.size() > 0) {
	std::vector<array_t<int64_t>*> longsptr;
	locate(ibis::LONG, longsptr);
	cnt += r.longsvalues.size();
	append(r.longsnames, r.longsvalues, longsptr);
    }
    if (r.ulongsvalues.size() > 0) {
	std::vector<array_t<uint64_t>*> ulongsptr;
	locate(ibis::ULONG, ulongsptr);
	cnt += r.ulongsvalues.size();
	append(r.ulongsnames, r.ulongsvalues, ulongsptr);
    }
    if (r.floatsvalues.size() > 0) {
	std::vector<array_t<float>*> floatsptr;
	locate(ibis::FLOAT, floatsptr);
	cnt += r.floatsvalues.size();
	append(r.floatsnames, r.floatsvalues, floatsptr);
    }
    if (r.doublesvalues.size() > 0) {
	std::vector<array_t<double>*> doublesptr;
	locate(ibis::DOUBLE, doublesptr);
	cnt += r.doublesvalues.size();
	append(r.doublesnames, r.doublesvalues, doublesptr);
    }
    if (r.catsvalues.size() > 0) {
	std::vector<std::vector<std::string>*> catsptr;
	locateString(ibis::CATEGORY, catsptr);
	cnt += r.catsvalues.size();
	appendString(r.catsnames, r.catsvalues, catsptr);
    }
    if (r.textsvalues.size() > 0) {
	std::vector<std::vector<std::string>*> textsptr;
	locateString(ibis::TEXT, textsptr);
	cnt += r.textsvalues.size();
	appendString(r.textsnames, r.textsvalues, textsptr);
    }
    nrows += (cnt > 0);
    return 0;
} // ibis::tafel::appendRow

int ibis::tafel::appendRows(const std::vector<ibis::table::row>& rs) {
    if (rs.empty()) return 0;
    std::vector<array_t<signed char>*> bytesptr;
    locate(ibis::BYTE, bytesptr);
    std::vector<array_t<unsigned char>*> ubytesptr;
    locate(ibis::UBYTE, ubytesptr);
    std::vector<array_t<int16_t>*> shortsptr;
    locate(ibis::SHORT, shortsptr);
    std::vector<array_t<uint16_t>*> ushortsptr;
    locate(ibis::USHORT, ushortsptr);
    std::vector<array_t<int32_t>*> intsptr;
    locate(ibis::INT, intsptr);
    std::vector<array_t<uint32_t>*> uintsptr;
    locate(ibis::UINT, uintsptr);
    std::vector<array_t<int64_t>*> longsptr;
    locate(ibis::LONG, longsptr);
    std::vector<array_t<uint64_t>*> ulongsptr;
    locate(ibis::ULONG, ulongsptr);
    std::vector<array_t<float>*> floatsptr;
    locate(ibis::FLOAT, floatsptr);
    std::vector<array_t<double>*> doublesptr;
    locate(ibis::DOUBLE, doublesptr);
    std::vector<std::vector<std::string>*> catsptr;
    locateString(ibis::CATEGORY, catsptr);
    std::vector<std::vector<std::string>*> textsptr;
    locateString(ibis::TEXT, textsptr);

    const size_t ncols = cols.size();
    size_t cnt = 0;
    for (size_t i = 0; i < rs.size(); ++ i) {
	if (cnt < ncols)
	    normalize();

	cnt = 0;
	if (rs[i].bytesvalues.size() > 0) {
	    cnt += rs[i].bytesvalues.size();
	    append(rs[i].bytesnames, rs[i].bytesvalues, bytesptr);
	}
	if (rs[i].ubytesvalues.size() > 0) {
	    cnt += rs[i].ubytesvalues.size();
	    append(rs[i].ubytesnames, rs[i].ubytesvalues, ubytesptr);
	}
	if (rs[i].shortsvalues.size() > 0) {
	    cnt += rs[i].shortsvalues.size();
	    append(rs[i].shortsnames, rs[i].shortsvalues, shortsptr);
	}
	if (rs[i].ushortsvalues.size() > 0) {
	    cnt += rs[i].ushortsvalues.size();
	    append(rs[i].ushortsnames, rs[i].ushortsvalues, ushortsptr);
	}
	if (rs[i].intsvalues.size() > 0) {
	    cnt += rs[i].intsvalues.size();
	    append(rs[i].intsnames, rs[i].intsvalues, intsptr);
	}
	if (rs[i].uintsvalues.size() > 0) {
	    cnt += rs[i].uintsvalues.size();
	    append(rs[i].uintsnames, rs[i].uintsvalues, uintsptr);
	}
	if (rs[i].longsvalues.size() > 0) {
	    cnt += rs[i].longsvalues.size();
	    append(rs[i].longsnames, rs[i].longsvalues, longsptr);
	}
	if (rs[i].ulongsvalues.size() > 0) {
	    cnt += rs[i].ulongsvalues.size();
	    append(rs[i].ulongsnames, rs[i].ulongsvalues, ulongsptr);
	}
	if (rs[i].floatsvalues.size() > 0) {
	    cnt += rs[i].floatsvalues.size();
	    append(rs[i].floatsnames, rs[i].floatsvalues, floatsptr);
	}
	if (rs[i].doublesvalues.size() > 0) {
	    cnt += rs[i].doublesvalues.size();
	    append(rs[i].doublesnames, rs[i].doublesvalues, doublesptr);
	}
	if (rs[i].catsvalues.size() > 0) {
	    cnt += rs[i].catsvalues.size();
	    appendString(rs[i].catsnames, rs[i].catsvalues, catsptr);
	}
	if (rs[i].textsvalues.size() > 0) {
	    cnt += rs[i].textsvalues.size();
	    appendString(rs[i].textsnames, rs[i].textsvalues, textsptr);
	}
	nrows += (cnt > 0);
    }
    return 0;
} // ibis::tafel::appendRows

/// Return error code:
/// -  0: successful completion.
/// - -1: no directory specified.
/// - -2: column type conflicts.
/// - -3: unable to open the metadata file.
/// - -4: unable to open a data file.
/// - -5: failed to write the expected number of records.
int ibis::tafel::write(const char* dir, const char* tname,
		       const char* tdesc) const {
    if (cols.empty() || nrows == 0) return 0; // nothing new to write
    if (dir == 0 || *dir == 0) return -1; // dir must be specified
    ibis::horometer timer;
    if (ibis::gVerbose > 0)
	timer.start();

    std::string oldnm, olddesc;
    ibis::bitvector::word_t nold = 0;
    { // read the existing meta data file in the directory dir
	ibis::part tmp(dir, static_cast<const char*>(0));
	nold = static_cast<ibis::bitvector::word_t>(tmp.nRows());
	if (nold > 0 && tmp.nColumns() > 0) {
	    if (tname == 0 || *tname == 0) {
		oldnm = tmp.name();
		tname = oldnm.c_str();
	    }
	    if (tdesc == 0 || *tdesc == 0) {
		olddesc = tmp.description();
		tdesc = olddesc.c_str();
	    }

	    unsigned nconflicts = 0;
	    for (columnList::const_iterator it = cols.begin();
		 it != cols.end(); ++ it) {
		const column& col = *((*it).second);
		const ibis::column* old = tmp.getColumn((*it).first);
		bool conflict = false;
		if (old != 0) { // possibility of conflict exists
		    switch (col.type) {
		    default:
			conflict = (old->type() != col.type); break;
		    case ibis::BYTE:
		    case ibis::UBYTE:
			conflict = (old->type() != ibis::BYTE &&
				    old->type() != ibis::UBYTE);
			break;
		    case ibis::SHORT:
		    case ibis::USHORT:
			conflict = (old->type() != ibis::SHORT &&
				    old->type() != ibis::USHORT);
			break;
		    case ibis::INT:
		    case ibis::UINT:
			conflict = (old->type() != ibis::INT &&
				    old->type() != ibis::UINT);
			break;
		    case ibis::LONG:
		    case ibis::ULONG:
			conflict = (old->type() != ibis::LONG &&
				    old->type() != ibis::ULONG);
			break;
		    }
		}
		if (conflict) {
		    ++ nconflicts;
		    LOGGER(0)
			<< "ibis::tafel::write(" << dir
			<< ") column " << (*it).first
			<< " has conflicting types specified, "
			"previously " << ibis::TYPESTRING[(int)old->type()]
			<< ", currently "
			<< ibis::TYPESTRING[(int)col.type];
		}
	    }
	    if (nconflicts > 0) {
		LOGGER(0)
		    << "ibis::tafel::write(" << dir
		    << ") can not proceed because " << nconflicts
		    << " column" << (nconflicts>1 ? "s" : "")
		    << " contains conflicting type specifications";
		return -2;
	    }
	    else LOGGER(3) 
		<< "ibis::tafel::write(" << dir
		<< ") found existing data partition named "
		<< tmp.name() << " with " << tmp.nRows()
		<< " row" << (tmp.nRows()>1 ? "s" : "")
		<< " and " << tmp.nColumns() << " column"
		<< (tmp.nColumns()>1?"s":"")
		<< ", will append " << nrows << " new row"
		<< (nrows>1 ? "s" : "");
	}
    }
    time_t currtime = time(0); // current time
    char stamp[28];
    ibis::util::secondsToString(currtime, stamp);
    if (tdesc == 0 || *tdesc == 0) { // generate a description
	std::ostringstream oss;
	oss << "Table constructed with ibis::tablex interface on "
	    << stamp << " with " << cols.size() << " column "
	    << (cols.size() > 1 ? "s" : "") << " and " << nold + nrows
	    << "row" << (nold+nrows>1 ? "s" : "");
	olddesc = oss.str();
	tdesc = olddesc.c_str();
    }
    if (tname == 0 || *tname == 0) { // use the directory name as table name
	tname = strrchr(dir, DIRSEP);
	if (tname == 0)
	    tname = strrchr(dir, '/');
	if (tname != 0) {
	    if (tname[1] != 0) {
		++ tname;
	    }
	    else { // dir ends with DIRSEP
		oldnm = dir;
		oldnm.erase(oldnm.size()-1); // remove the last DIRSEP
		size_t j = 1 + oldnm.rfind(DIRSEP);
		if (j > oldnm.size())
		    j = 1 + oldnm.rfind('/');
		if (j < oldnm.size())
		    oldnm.erase(0, j);
		if (! oldnm.empty())
		    tname = oldnm.c_str();
		else
		    tname = 0;
	    }
	}
	else if (tname == 0 && *dir != '.') { // no directory separator
	    tname = dir;
	}
	if (tname == 0) {
	    uint32_t sum = ibis::util::checksum(tdesc, strlen(tdesc));
	    ibis::util::int2string(oldnm, sum);
	    if (! isalpha(oldnm[0]))
		oldnm[0] = 'A' + (oldnm[0] % 26);
	}
    }

    std::string mdfile = dir;
    mdfile += DIRSEP;
    mdfile += "-part.txt";
    std::ofstream md(mdfile.c_str());
    if (! md) return -3; // metadata file not ready

    md << "# meta data for data partition " << tname
       << " written by ibis::tafel::write on " << stamp << "\n\n"
       << "BEGIN HEADER\nName = " << tname << "\nDescription = "
       << tdesc << "\nNumber_of_rows = " << nold+nrows
       << "\nNumber_of_columns = " << cols.size()
       << "\nTimestamp = " << currtime;
    { // try to find the default index specification
	olddesc = "ibis.";
	olddesc += tname;
	olddesc += ".index";
	const char* str = ibis::gParameters()[olddesc.c_str()];
	if (str != 0)
	    md << "\nindex = " << str;
    }
    md << "\nEND HEADER\n";

    int ierr = 0;
    for (columnList::const_iterator it = cols.begin();
	 it != cols.end(); ++ it) {
	const column& col = *((*it).second);
	std::string cnm = dir;
	cnm += DIRSEP;
	cnm += (*it).first;
	int fdes = UnixOpen(cnm.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
	if (fdes < 0) {
	    if (ibis::gVerbose > -1)
		ibis::util::logMessage("ibis::tafel::write",
				       "failed to open file %s for writing",
				       cnm.c_str()); 
	    return -4;
	}
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdes, _O_BINARY);
#endif

	olddesc = cnm; // mask file name
	olddesc += ".msk";
	ibis::bitvector msk(olddesc.c_str());

	switch (col.type) {
	case ibis::BYTE:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<signed char>*>
			       (col.values), (signed char)0x7F, msk, col.mask);
	    break;
	case ibis::UBYTE:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<unsigned char>*>
			       (col.values), (unsigned char)0xFF, msk,
			       col.mask);
	    break;
	case ibis::SHORT:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<int16_t>*>
			       (col.values), (int16_t)0x7FFF, msk, col.mask);
	    break;
	case ibis::USHORT:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<uint16_t>*>
			       (col.values), (uint16_t)0xFFFF, msk, col.mask);
	    break;
	case ibis::INT:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<int32_t>*>
			       (col.values), (int32_t)0x7FFFFFFF, msk,
			       col.mask);
	    break;
	case ibis::UINT:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<uint32_t>*>
			       (col.values), (uint32_t)0xFFFFFFFF, msk,
			       col.mask);
	    break;
	case ibis::LONG:
	    ierr = writeColumn<int64_t>
		(fdes, nold, nrows, * static_cast<const array_t<int64_t>*>
		 (col.values), 0x7FFFFFFFFFFFFFFFLL, msk, col.mask);
	    break;
	case ibis::ULONG:
	    ierr = writeColumn<uint64_t>
		(fdes, nold, nrows, * static_cast<const array_t<uint64_t>*>
		 (col.values), 0xFFFFFFFFFFFFFFFFULL, msk, col.mask);
	    break;
	case ibis::FLOAT:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<float>*>
			       (col.values),
			       std::numeric_limits<float>::quiet_NaN(),
			       msk, col.mask);
	    break;
	case ibis::DOUBLE:
	    ierr = writeColumn(fdes, nold, nrows,
			       * static_cast<const array_t<double>*>
			       (col.values), 
			       std::numeric_limits<double>::quiet_NaN(),
			       msk, col.mask);
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    ierr = writeString(fdes, nold, nrows,
			       * static_cast<const std::vector<std::string>*>
			       (col.values), msk, col.mask);
	    break;
	default:
	    break;
	}
#if _POSIX_FSYNC+0 > 0
	(void) fsync(fdes); // write to disk
#endif
	UnixClose(fdes); // close the data file
	if (ierr < 0) {
	    if (ibis::gVerbose > 0)
		ibis::util::logMessage("ibis::tafel::write",
				       "failed to write column %s (type %s) "
				       "to %s", (*it).first,
				       ibis::TYPESTRING[(int)col.type],
				       cnm.c_str());
	    return ierr;
	}

	if (msk.cnt() != msk.size()) {
	    msk.write(olddesc.c_str());
	}
	else { // remove the mask file
	    remove(olddesc.c_str());
	}
	ibis::fileManager::instance().flushFile(olddesc.c_str());

	md << "\nBegin Column\nname = " << (*it).first << "\ndata_type = "
	   << ibis::TYPESTRING[(int) col.type];
	if (col.type == ibis::TEXT)
	    md << "\nindex=none";
	md << "\nEnd Column\n";
    } 
   if (ibis::gVerbose > 0) {
	timer.stop();
	LOGGER(1) << "ibis::tafel::write completed writing partition " 
		  << tname << " (" << tdesc << ") with "
		  << cols.size() << " column" << (cols.size()>1 ? "s" : "")
		  << " and " << nrows << " row" << (nrows>1 ? "s" : "")
		  << " (total " << nold+nrows << ") to " << dir
		  << " using " << timer.CPUTime() << " sec(CPU) and "
		  << timer.realTime() << " sec(elapsed)";
    }
    md.close(); // close the file
    ibis::fileManager::instance().flushDir(dir);

    return 0;
} // ibis::tafel::write

template <typename T>
int ibis::tafel::writeColumn(int fdes, ibis::bitvector::word_t nold,
			     ibis::bitvector::word_t nnew,
			     const array_t<T>& vals, const T& fill,
			     ibis::bitvector& totmask,
			     const ibis::bitvector& newmask) const {
    const size_t elem = sizeof(T);
    off_t pos = UnixSeek(fdes, 0, SEEK_END);
    if (pos < 0) return -3; // failed to find the EOF position
    if ((size_t) pos < nold*elem) {
	const size_t n1 = (size_t)pos / elem;
	totmask.adjustSize(n1, nold);
	for (size_t j = n1; j < nold; ++ j)
	    UnixWrite(fdes, &fill, elem);
    }
    else if ((size_t) pos > nold*elem) {
	pos = UnixSeek(fdes, nold*elem, SEEK_SET);
	totmask.adjustSize(nold, nold);
    }
    else {
	totmask.adjustSize(nold, nold);
    }

    if (vals.size() >= nnew) {
	pos = UnixWrite(fdes, vals.begin(), nnew*elem);
	totmask += newmask;
    }
    else {
	pos = UnixWrite(fdes, vals.begin(), vals.size()*elem);
	for (size_t j = vals.size(); j < nnew; ++ j)
	    pos += UnixWrite(fdes, &fill, elem);
	totmask += newmask;
    }
    totmask.adjustSize(totmask.size(), nnew+nold);
#if DEBUG+0>0
    LOGGER(0)
	<< "ibis::tafel::writeColumn wrote " << pos << " bytes of "
	<< typeid(T).name() << " for " << nnew << " elements\n"
	<< "Overall bit mask: "<< totmask;
#endif
    return (-5 * ((size_t) pos != nnew*elem));
} // ibis::tafel::writeColumn

int ibis::tafel::writeString(int fdes, ibis::bitvector::word_t nold,
			     ibis::bitvector::word_t nnew,
			     const std::vector<std::string>& vals,
			     ibis::bitvector& totmask,
			     const ibis::bitvector& newmask) const {
    off_t pos = UnixSeek(fdes, 0, SEEK_END);
    if (pos < 0) return -3; // failed to find the EOF position

    pos = 0;
    totmask.adjustSize(nold, nold);
    if (vals.size() >= nnew) {
	for (size_t j = 0; j < nnew; ++ j)
	    pos += (0 < UnixWrite(fdes, vals[j].c_str(), vals[j].size()+1));
    }
    else {
	for (size_t j = 0; j < vals.size(); ++ j)
	    pos += (0 < UnixWrite(fdes, vals[j].c_str(), vals[j].size()+1));
	char buf[MAX_LINE];
	memset(buf, 0, MAX_LINE);
	for (size_t j = vals.size(); j < nnew; j += MAX_LINE)
	    pos += UnixWrite(fdes, buf, (j+MAX_LINE<=nnew?MAX_LINE:nnew-j));
    }

    totmask += newmask;
    totmask.adjustSize(totmask.size(), nnew+nold);
#if DEBUG+0>0
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "ibis::tafel::writeString wrote " << pos
		<< " strings (" << nnew << " expected)\nvals.size()="
		<< vals.size() << ", content\n";
    for (size_t j = 0; j < (nnew <= vals.size() ? nnew : vals.size()); ++ j)
	lg.buffer() << j << "\t" << vals[j] << "\n";
    lg.buffer() << "Overall bit mask: " << totmask;
#endif
    return (-5 * ((size_t) pos != nnew));
} // ibis::tafel::writeString

void ibis::tafel::clear() {
    while (! cols.empty()) {
	columnList::iterator it = cols.begin();
	column* col = (*it).second;
	cols.erase(it);
	delete col;
    }
} // ibis::tafel::clear

int ibis::tafel::parseLine(const char* str, const char* del, const char* id) {
    int cnt = 0;
    int ierr;
    int64_t itmp;
    double dtmp;
    std::string stmp;
    const size_t ncol = colorder.size();
    for (size_t i = 0; i < ncol; ++ i) {
	column& col = *(colorder[i]);
	switch (col.type) {
	case ibis::BYTE: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		signed char tmp = static_cast<signed char>(itmp);
		if (tmp != itmp) {
		    LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			      << "column " << i+1 << " in "
			      << id << " (" << itmp << ") "
			      << "can not fit into a byte";
		    continue; // skip the line
		}
		static_cast<array_t<signed char>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in "
			  << id << " can not be parsed "
		    "correctly as an integer";
		continue;
	    }
	    break;}
	case ibis::UBYTE: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		unsigned char tmp = static_cast<unsigned char>(itmp);
		if ((int64_t)tmp != itmp) {
		    LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			      << "column " << i+1 << " in "
			      << id << " (" << itmp << ") "
			      << "can not fit into a byte";
		    continue; // skip the line
		}
		static_cast<array_t<unsigned char>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";
		continue;
	    }
	    break;}
	case ibis::SHORT: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		int16_t tmp = static_cast<int16_t>(itmp);
		if (tmp != itmp) {
		    LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			      << "column " << i+1 << " in "
			      << id << " (" << itmp << ") "
			      << "can not fit into a two-byte integer";

		    continue; // skip the line
		}
		static_cast<array_t<int16_t>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";

		continue;
	    }
	    break;}
	case ibis::USHORT: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		uint16_t tmp = static_cast<uint16_t>(itmp);
		if ((int64_t)tmp != itmp) {
		    LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			      << "column " << i+1 << " in "
			      << id << " (" << itmp << ") "
			      << "can not fit into a two-byte integer";

		    continue; // skip the line
		}
		static_cast<array_t<uint16_t>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";

		continue;
	    }
	    break;}
	case ibis::INT: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		int32_t tmp = static_cast<int32_t>(itmp);
		if (tmp != itmp) {
		    LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			      << "column " << i+1 << " in "
			      << id << " (" << itmp << ") "
			      << "can not fit into a four-byte integer";

		    continue; // skip the line
		}
		static_cast<array_t<int32_t>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";

		continue;
	    }
	    break;}
	case ibis::UINT: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		uint32_t tmp = static_cast<uint32_t>(itmp);
		if ((int64_t)tmp != itmp) {
		    LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			      << "column " << i+1 << " in "
			      << id << " (" << itmp << ") "
			      << "can not fit into a four-byte integer";

		    continue; // skip the line
		}
		static_cast<array_t<uint32_t>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";

		continue;
	    }
	    break;}
	case ibis::LONG: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		static_cast<array_t<int64_t>*>(col.values)
		    ->push_back(itmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";

		continue;
	    }
	    break;}
	case ibis::ULONG: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		static_cast<array_t<uint64_t>*>(col.values)
		    ->push_back(static_cast<uint64_t>(itmp));
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as an integer";

		continue;
	    }
	    break;}
	case ibis::FLOAT: {
	    ierr = ibis::util::readDouble(dtmp, str, del);
	    if (ierr == 0) {
		static_cast<array_t<float>*>(col.values)
		    ->push_back((float)dtmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as a "
		    "floating-point number";

		continue;
	    }
	    break;}
	case ibis::DOUBLE: {
	    ierr = ibis::util::readDouble(dtmp, str, del);
	    if (ierr == 0) {
		static_cast<array_t<double>*>(col.values)
		    ->push_back(dtmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(3) << "Warning -- ibis::tafel::parseLine "
			  << "column " << i+1 << " in " << id
			  << " can not be parsed correctly as a "
		    "floating-point number";

		continue;
	    }
	    break;}
	case ibis::CATEGORY:
	case ibis::TEXT: {
	    ibis::util::getString(stmp, str, del);
	    if (! stmp.empty()) {
		static_cast<std::vector<std::string>*>(col.values)
		    ->push_back(stmp);
		col.mask += 1;
		++ cnt;
	    }
	    break;}
	default:
	    break;
	}

	if (*str != 0) { // skip trailing sapace and one delimeter
	    while (*str != 0 && isspace(*str)) ++ str; // trailing space
	    if (*str != 0 && strchr(del, *str) != 0) ++ str;
	}
	else {
	    break;
	}
    }
    return cnt;
} // ibis::tafel::parseLine

int ibis::tafel::appendRow(const char* line, const char* del) {
    if (line == 0) return -1;
    while (*line != 0 && isspace(*line)) ++ line;
    if (*line == 0) return -1;
    if (*line == '#' || (*line == '-' && line[1] == '-')) return 0;

    std::string id = "string ";
    id.append(line, 10);
    id += " ...";
    std::string delimiters = (del != 0 && *del != 0 ? del : ", ");

    normalize();
    int ierr = parseLine(line, delimiters.c_str(), id.c_str());
    nrows += (ierr > 0);
    ierr = -(ierr <= 0);
    return ierr;
} // ibis::tafel::appendRow

int ibis::tafel::readCSV(const char* filename, const char* del) {
    if (filename == 0 || *filename == 0) return -1;
    if (colorder.empty()) return -2;
    std::string delimiters = (del != 0 && *del != 0 ? del : ", ");

    char linebuf[MAX_LINE];
    std::ifstream csv(filename);
    if (! csv) return -2; // failed to open the specified data file

    int ierr;
    int64_t itmp;
    double dtmp;
    std::string stmp;
    size_t cnt = 0;
    size_t iline = 0;
    char* str; // pointer to next character to be processed
    const size_t ncol = colorder.size();
    while (csv.getline(linebuf, MAX_LINE)) {
	str = linebuf;
	while (*str != 0 && isspace(*str)) ++ str; // skip leading space
	if (*str == '#' || (*str == '-' && str[1] == '-')) {
	    // skip comment line (shell style comment and SQL style comments)
	    ++ iline;
	    continue;
	}

	if (cnt != ncol)
	    normalize();

	for (size_t i = 0; i < ncol; ++ i) {
	    column& col = *(colorder[i]);
	    switch (col.type) {
	    case ibis::BYTE: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    signed char tmp = static_cast<signed char>(itmp);
		    if (tmp != itmp) {
			LOGGER(3) << "Warning -- ibis::tafel::readCSV "
				  << "column " << i+1 << " in row "
				  << iline+1 << " (" << itmp << ") "
				  << "can not fit into a byte";

			continue; // skip the line
		    }
		    static_cast<array_t<signed char>*>(col.values)
			->push_back(tmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1 
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::UBYTE: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    unsigned char tmp = static_cast<unsigned char>(itmp);
		    if ((int64_t)tmp != itmp) {
			LOGGER(3)  << "Warning -- ibis::tafel::readCSV "
				   << "column " << i+1 << " in row "
				   << iline+1 << " (" << itmp << ") "
				   << "can not fit into a byte";
			continue; // skip the line
		    }
		    static_cast<array_t<unsigned char>*>(col.values)
			->push_back(tmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::SHORT: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    int16_t tmp = static_cast<int16_t>(itmp);
		    if (tmp != itmp) {
			LOGGER(3) << "Warning -- ibis::tafel::readCSV "
				  << "column " << i+1 << " in row "
				  << iline+1 << " (" << itmp << ") "
				  << "can not fit into a two-byte integer";
			continue; // skip the line
		    }
		    static_cast<array_t<int16_t>*>(col.values)
			->push_back(tmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::USHORT: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    uint16_t tmp = static_cast<uint16_t>(itmp);
		    if ((int64_t)tmp != itmp) {
			LOGGER(3) << "Warning -- ibis::tafel::readCSV "
				  << "column " << i+1 << " in row "
				  << iline+1 << " (" << itmp << ") "
				  << "can not fit into a two-byte integer";
			continue; // skip the line
		    }
		    static_cast<array_t<uint16_t>*>(col.values)
			->push_back(tmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::INT: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    int32_t tmp = static_cast<int32_t>(itmp);
		    if (tmp != itmp) {
			LOGGER(3) << "Warning -- ibis::tafel::readCSV "
				  << "column " << i+1 << " in row "
				  << iline+1 << " (" << itmp << ") "
				  << "can not fit into a four-byte integer";
			continue; // skip the line
		    }
		    static_cast<array_t<int32_t>*>(col.values)
			->push_back(tmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::UINT: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    uint32_t tmp = static_cast<uint32_t>(itmp);
		    if ((int64_t)tmp != itmp) {
			LOGGER(3) << "Warning -- ibis::tafel::readCSV "
				  << "column " << i+1 << " in row "
				  << iline+1 << " (" << itmp << ") "
				  << "can not fit into a four-byte integer";
			continue; // skip the line
		    }
		    static_cast<array_t<uint32_t>*>(col.values)
			->push_back(tmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::LONG: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    static_cast<array_t<int64_t>*>(col.values)
			->push_back(itmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::ULONG: {
		ierr = ibis::util::readInt(itmp, const_cast<const char*&>(str),
					   delimiters.c_str());
		if (ierr == 0) {
		    static_cast<array_t<uint64_t>*>(col.values)
			->push_back(static_cast<uint64_t>(itmp));
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as an integer";
		    continue;
		}
		break;}
	    case ibis::FLOAT: {
		ierr = ibis::util::readDouble(dtmp,
					      const_cast<const char*&>(str),
					      delimiters.c_str());
		if (ierr == 0) {
		    static_cast<array_t<float>*>(col.values)
			->push_back((float)dtmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as a "
			"floating-point number";
		    continue;
		}
		break;}
	    case ibis::DOUBLE: {
		ierr = ibis::util::readDouble(dtmp,
					      const_cast<const char*&>(str),
					      delimiters.c_str());
		if (ierr == 0) {
		    static_cast<array_t<double>*>(col.values)
			->push_back(dtmp);
		    col.mask += 1;
		    ++ cnt;
		}
		else {
		    LOGGER(3) << "Warning -- ibis::tafel::readCSV "
			      << "column " << i+1 << " in row " << iline+1
			      << " can not be parsed correctly as a "
			"floating-point number";
		    continue;
		}
		break;}
	    case ibis::CATEGORY:
	    case ibis::TEXT: {
		ibis::util::getString(stmp, const_cast<const char*&>(str),
				      delimiters.c_str());
		if (! stmp.empty()) {
		    static_cast<std::vector<std::string>*>(col.values)
			->push_back(stmp);
		    col.mask += 1;
		    ++ cnt;
		}
		break;}
	    default:
		break;
	    }
	
	    if (*str != 0) { // skip trailing sapace and one delimeter
		while (*str != 0 && isspace(*str)) ++ str; // trailing space
		if (*str != 0 && strchr(delimiters.c_str(), *str) != 0) ++ str;
	    }
	    else {
		break;
	    }
	}
	nrows += (cnt > 0);
	++ iline;
    }

    return 0;
} // ibis::tafel::readCSV

ibis::tafel::column::~column() {
    switch (type) {
    case ibis::BYTE:
	delete static_cast<array_t<signed char>*>(values);
	break;
    case ibis::UBYTE:
	delete static_cast<array_t<unsigned char>*>(values);
	break;
    case ibis::SHORT:
	delete static_cast<array_t<int16_t>*>(values);
	break;
    case ibis::USHORT:
	delete static_cast<array_t<uint16_t>*>(values);
	break;
    case ibis::INT:
	delete static_cast<array_t<int32_t>*>(values);
	break;
    case ibis::UINT:
	delete static_cast<array_t<uint32_t>*>(values);
	break;
    case ibis::LONG:
	delete static_cast<array_t<int64_t>*>(values);
	break;
    case ibis::ULONG:
	delete static_cast<array_t<uint64_t>*>(values);
	break;
    case ibis::FLOAT:
	delete static_cast<array_t<float>*>(values);
	break;
    case ibis::DOUBLE:
	delete static_cast<array_t<double>*>(values);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	delete static_cast<std::vector<std::string>*>(values);
	break;
    default:
	break;
    }
} // ibis::tafel::column::~column

/// Create a tablex for entering new data.
ibis::tablex* ibis::tablex::create() {
    return new ibis::tafel;
} // ibis::tablex::create

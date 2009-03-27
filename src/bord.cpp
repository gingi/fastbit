// File $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tab.h"	// ibis::tabula and ibis::tabele
#include "bord.h"	// ibis::bord
#include "query.h"	// ibis::query
#include "bundle.h"	// ibis::bundle
#include <algorithm>	// std::reverse, std::copy
#include <sstream>	// std::ostringstream
#include <limits>	// std::numeric_limits

ibis::bord::bord(const char *tn, const char *td, uint64_t nr,
		 const ibis::table::stringList &cn,
		 const ibis::table::typeList   &ct,
		 const ibis::bord::bufferList  &buf,
		 const ibis::table::stringList *cdesc)
    : ibis::table(tn, td), mypart(tn, td, nr, cn, ct, buf, cdesc) {
} // ibis::bord::bord

void ibis::bord::clear() {
} // ibis::bord::clear

/// @note The pointers returned are pointing to names stored internally.
/// The caller should not attempt to free these pointers.
ibis::table::stringList ibis::bord::columnNames() const {
    ibis::table::stringList res(mypart.nColumns());
    for (size_t i = 0; i < mypart.nColumns(); ++ i) {
	ibis::column* col = mypart.getColumn(i);
	res[i] = (col != 0 ? col->name() : static_cast<const char*>(0));
    }
    return res;
} // ibis::bord::columnNames

ibis::table::typeList ibis::bord::columnTypes() const {
    ibis::table::typeList res(mypart.nColumns());
    for (size_t i = 0; i < mypart.nColumns(); ++ i) {
	ibis::column* col = mypart.getColumn(i);
	res[i] = (col != 0 ? col->type() : ibis::UNKNOWN_TYPE);
    }
    return res;
} // ibis::bord::columnTypes

int64_t ibis::bord::getColumnAsBytes(const char *cn, char *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsBytes

int64_t
ibis::bord::getColumnAsUBytes(const char *cn, unsigned char *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsUBytes

int64_t ibis::bord::getColumnAsShorts(const char *cn, int16_t *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsShorts

int64_t ibis::bord::getColumnAsUShorts(const char *cn, uint16_t *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsUShorts

int64_t ibis::bord::getColumnAsInts(const char *cn, int32_t *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::INT || col->type() == ibis::UINT) {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT) {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsInts

int64_t ibis::bord::getColumnAsUInts(const char *cn, uint32_t *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::INT || col->type() == ibis::UINT) {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsUInts

int64_t ibis::bord::getColumnAsLongs(const char *cn, int64_t *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::LONG || col->type() == ibis::ULONG) {
	const array_t<int64_t>* arr =
	    static_cast<const array_t<int64_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::INT) {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UINT) {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT) {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsLongs

int64_t ibis::bord::getColumnAsULongs(const char *cn, uint64_t *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::LONG || col->type() == ibis::ULONG) {
	const array_t<uint64_t>* arr =
	    static_cast<const array_t<uint64_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::INT || col->type() == ibis::UINT) {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsULongs

int64_t ibis::bord::getColumnAsFloats(const char *cn, float *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::FLOAT: {
	const array_t<float>* arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsFloats

int64_t ibis::bord::getColumnAsDoubles(const char *cn, double *vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::DOUBLE: {
	const array_t<double>* arr =
	    static_cast<const array_t<double>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::FLOAT: {
	const array_t<float>* arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::INT: {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::UINT: {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	std::copy(arr->begin(), arr->begin()+sz, vals);
	return sz;}
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsDoubles

int64_t ibis::bord::getColumnAsDoubles(const char* cn,
				       std::vector<double>& vals) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::DOUBLE: {
	const array_t<double>* arr =
	    static_cast<const array_t<double>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::FLOAT: {
	const array_t<float>* arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::INT: {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::UINT: {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;
	const size_t sz = (mypart.nRows() <= arr->size() ? 
			   mypart.nRows() : arr->size());
	vals.resize(sz);
	std::copy(arr->begin(), arr->begin()+sz, vals.begin());
	return sz;}
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsDoubles

int64_t ibis::bord::getColumnAsStrings(const char *cn,
				       std::vector<std::string> &vals) const {
    const ibis::bord::column *col =
	dynamic_cast<ibis::bord::column*>(mypart.getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::TEXT:
    case ibis::CATEGORY: {
	const std::vector<std::string>* arr =
	    static_cast<const std::vector<std::string>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	std::copy(arr->begin(), arr->end(), vals.begin());
	return sz;}
    case ibis::DOUBLE: {
	const array_t<double> *arr =
	    static_cast<const array_t<double>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::FLOAT: {
	const array_t<float> *arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::LONG: {
	const array_t<int64_t> *arr =
	    static_cast<const array_t<int64_t>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::ULONG: {
	const array_t<uint64_t> *arr =
	    static_cast<const array_t<uint64_t>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::INT: {
	const array_t<int32_t> *arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::UINT: {
	const array_t<uint32_t> *arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::SHORT: {
	const array_t<int16_t> *arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::USHORT: {
	const array_t<uint16_t> *arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::BYTE: {
	const array_t<signed char> *arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << static_cast<int>((*arr)[i]);
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::UBYTE: {
	const array_t<unsigned char> *arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	const size_t sz = arr->size();
	vals.resize(sz);
	for (size_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << static_cast<int>((*arr)[i]);
	    vals[i] = oss.str();
	}
	return sz;}
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsDoubles

long ibis::bord::getHistogram(const char *constraints,
			     const char *cname,
			     double begin, double end, double stride,
			     std::vector<size_t>& counts) const {
    if (sizeof(size_t) == sizeof(uint32_t)) {
	return mypart.get1DDistribution
	    (constraints, cname, begin, end, stride,
	     reinterpret_cast<std::vector<uint32_t>&>(counts));
    }
    else {
	std::vector<uint32_t> tmp;
	long ierr = mypart.get1DDistribution(constraints, cname, begin, end,
					     stride, tmp);
	if (ierr >= 0) {
	    counts.resize(tmp.size());
	    std::copy(tmp.begin(), tmp.end(), counts.begin());
	}
	return ierr;
    }
} // ibis::bord::getHistogram

long ibis::bord::getHistogram2D(const char *constraints,
				const char *cname1,
				double begin1, double end1, double stride1,
				const char* cname2,
				double begin2, double end2, double stride2,
				std::vector<size_t>& counts) const {
    if (sizeof(size_t) == sizeof(uint32_t)) {
	return mypart.get2DDistribution
	    (constraints, cname1, begin1, end1, stride1,
	     cname2, begin2, end2, stride2,
	     reinterpret_cast<std::vector<uint32_t>&>(counts));
    }
    else {
	std::vector<uint32_t> tmp;
	long ierr = mypart.get2DDistribution
	    (constraints, cname1, begin1, end1, stride1,
	     cname2, begin2, end2, stride2, tmp);
	if (ierr >= 0) {
	    counts.resize(tmp.size());
	    std::copy(tmp.begin(), tmp.end(), counts.begin());
	}
	return ierr;
    }
} // ibis::bord::getHistogram2D

long ibis::bord::getHistogram3D(const char *constraints,
				const char *cname1,
				double begin1, double end1, double stride1,
				const char *cname2,
				double begin2, double end2, double stride2,
				const char *cname3,
				double begin3, double end3, double stride3,
				std::vector<size_t>& counts) const {
    if (sizeof(size_t) == sizeof(uint32_t)) {
	return mypart.get3DDistribution
	    (constraints, cname1, begin1, end1, stride1,
	     cname2, begin2, end2, stride2,
	     cname3, begin3, end3, stride3,
	     reinterpret_cast<std::vector<uint32_t>&>(counts));
    }
    else {
	std::vector<uint32_t> tmp;
	long ierr = mypart.get3DDistribution
	    (constraints, cname1, begin1, end1, stride1,
	     cname2, begin2, end2, stride2,
	     cname3, begin3, end3, stride3, tmp);
	if (ierr >= 0) {
	    counts.resize(tmp.size());
	    std::copy(tmp.begin(), tmp.end(), counts.begin());
	}
	return ierr;
    }
} // ibis::bord::getHistogram3D

void ibis::bord::estimate(const char *cond,
			  uint64_t &nmin, uint64_t &nmax) const {
    ibis::query q(ibis::util::userName(), &mypart);
    q.setWhereClause(cond);
    int ierr = q.estimate();
    if (ierr >= 0) {
	nmin = q.getMinNumHits();
	nmax = q.getMaxNumHits();
    }
    else {
	nmin = 0;
	nmax = mypart.nRows();
    }
} // ibis::bord::estimate

ibis::table* ibis::bord::select(const char *sel, const char *cond) const {
    if (cond == 0 || *cond == 0) return 0;
    if (sel != 0) // skip leading space
	while (isspace(*sel)) ++ sel;

    ibis::query q(ibis::util::userName(), &mypart);
    if (sel != 0 && *sel != 0 && strnicmp(sel, "count(", 6) != 0) {
	q.setSelectClause(sel);
	sel = q.getSelectClause();
    }
    q.setWhereClause(cond);
    int ierr = q.evaluate();
    if (ierr < 0) {
	return 0; // something went badly wrong
    }
    const ibis::bitvector *hits = q.getHitVector();
    if (hits == 0 || hits->cnt() == 0)
	return 0;

    std::string tn;
    std::string de = "SELECT ";
    de += sel;
    de += " FROM ";
    de += mypart.name();
    de += " WHERE ";
    de += cond;
    {
	uint32_t tmp;
	tmp = ibis::util::checksum(de.c_str(), de.size());
	ibis::util::int2string(tn, tmp);
	std::swap(tn[0], tn[5]);
	if (! isalpha(tn[0]))
	    tn[0] = 'A' + (tn[0] % 26);
    }
    if (sel == 0 || *sel == 0) {
	uint64_t nhits = hits->cnt();
	return new ibis::tabula(tn.c_str(), de.c_str(), nhits);
    }
    else if (strnicmp(sel, "count(", 6) == 0) { // count(*)
	uint64_t nhits = hits->cnt();
	return new ibis::tabele(tn.c_str(), de.c_str(), nhits);
    }
    else {
	ibis::table::stringList nm;
	ibis::table::typeList tp;
	const ibis::nameList nms(sel);
	ibis::bord::bufferList buf;
	for (size_t i = 0; i < nms.size(); ++ i) {
	    const ibis::column *col = mypart.getColumn(nms[i]);
	    if (col != 0) {
		switch (col->type()) {
		case ibis::BYTE:
		case ibis::SHORT:
		case ibis::INT: {
		    array_t<int32_t> *tmp = col->selectInts(*hits);
		    if (tmp != 0) {
			buf.push_back(tmp);
			nm.push_back(nms[i]);
			tp.push_back(ibis::INT);
		    }
		    break;}
		case ibis::UBYTE:
		case ibis::USHORT:
		case ibis::UINT: {
		    array_t<uint32_t> *tmp = col->selectUInts(*hits);
		    if (tmp != 0) {
			buf.push_back(tmp);
			nm.push_back(nms[i]);
			tp.push_back(ibis::UINT);
		    }
		    break;}
		case ibis::LONG:
		case ibis::ULONG: {
		    array_t<int64_t> *tmp = col->selectLongs(*hits);
		    if (tmp != 0) {
			buf.push_back(tmp);
			nm.push_back(nms[i]);
			tp.push_back(ibis::LONG);
		    }
		    break;}
		case ibis::FLOAT: {
		    array_t<float> *tmp = col->selectFloats(*hits);
		    if (tmp != 0) {
			buf.push_back(tmp);
			nm.push_back(nms[i]);
			tp.push_back(ibis::FLOAT);
		    }
		    break;}
		case ibis::DOUBLE: {
		    array_t<double> *tmp = col->selectDoubles(*hits);
		    if (tmp != 0) {
			buf.push_back(tmp);
			nm.push_back(nms[i]);
			tp.push_back(ibis::DOUBLE);
		    }
		    break;}
		default: {
		    ibis::util::logMessage
			("ibis::bord::select",
			 "unable to handle column (%s) with type %s",
			 col->name(), ibis::TYPESTRING[(int)col->type()]);
		    break;}
		}
	    }
	}
	if (nm.size() > 0) {
	    return new ibis::bord(tn.c_str(), de.c_str(), q.getNumHits(),
				  nm, tp, buf);
	}
	else {
	    return new ibis::tabula(tn.c_str(), de.c_str(), q.getNumHits());
	}
    }
} // ibis::bord::select

int64_t ibis::bord::computeHits(const char *cond) const {
    int64_t res = -1;
    ibis::query q(ibis::util::userName(), &mypart);
    q.setWhereClause(cond);
    res = q.evaluate();
    if (res >= 0)
	res = q.getNumHits();
    return res;
} // ibis::bord::computeHits

ibis::bord::part::part(const char *tn, const char *td, uint64_t nr,
		       const ibis::table::stringList &cn,
		       const ibis::table::typeList   &ct,
		       const ibis::bord::bufferList  &buf,
		       const ibis::table::stringList *cdesc)
    : ibis::part("incore") {
    m_name = ibis::util::strnewdup(tn);
    m_desc = td;
    nEvents = static_cast<size_t>(nr);
    if (nEvents != nr) {
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::bord::part::part can not handle " << nr
	    << " rows in an in-memory table";
	throw "Too many rows for an in-memory table";
    }

    const size_t nc = (cn.size()<=ct.size() ? cn.size() : ct.size());
    for (size_t i = 0; i < nc; ++ i) {
	if (columns.find(cn[i]) == columns.end()) {
	    ibis::column *tmp;
	    if (cdesc != 0 && cdesc->size() > i)
		tmp = new ibis::bord::column(this, ct[i], cn[i], buf[i],
					     (*cdesc)[i]);
	    else
		tmp = new ibis::bord::column(this, ct[i], cn[i], buf[i]);
	    columns[tmp->name()] = tmp;
	    colorder.push_back(tmp);
	}
    }

    LOGGER(ibis::gVerbose > 0)
	<< "ibis::bord::part::part(" << (m_name != 0 ? m_name : "<unnamed>")
	<< ", " << m_desc << ") completed allocating memory for "
	<< columns.size() << " column" << (columns.size() > 1U ? "s" : "")
	<< " with " << nr << " row" << (nr > 1U ? "s" : "");
} // ibis::bord::part::part

void ibis::bord::part::describe(std::ostream& out) const {
    out << "Table (in memory) " << m_name << " (" << m_desc
	<< ") contsists of " << columns.size() << " column"
	<< (columns.size()>1 ? "s" : "") << " and " << nEvents
	<< " row" << (nEvents>1 ? "s" : "");
    if (colorder.empty()) {
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it)
	    out << "\n" << (*it).first << "\t"
		<< ibis::TYPESTRING[(int)(*it).second->type()];
    }
    else if (colorder.size() == columns.size()) {
	for (size_t i = 0; i < columns.size(); ++ i)
	    out << "\n" << colorder[i]->name() << "\t"
		<< ibis::TYPESTRING[(int)colorder[i]->type()];
    }
    else {
	std::set<const char*, ibis::lessi> names;
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it)
	    names.insert((*it).first);
	for (size_t i = 0; i < colorder.size(); ++ i) {
	    out << "\n" << colorder[i]->name() << "\t"
		<< ibis::TYPESTRING[(int)colorder[i]->type()];
	    names.erase(colorder[i]->name());
	}
	for (std::set<const char*, ibis::lessi>::const_iterator it =
		 names.begin(); it != names.end(); ++ it) {
	    ibis::part::columnList::const_iterator cit = columns.find(*it);
	    out << "\n" << (*cit).first << "\t"
		<< ibis::TYPESTRING[(int)(*cit).second->type()];
	}
    }
    out << std::endl;
} // ibis::bord::part::describe

void ibis::bord::part::dumpNames(std::ostream& out, const char* del) const {
    if (columns.empty()) return;

    if (colorder.empty()) {
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it) {
	    if (it != columns.begin())
		out << del;
	    out << (*it).first;
	}
    }
    else if (colorder.size() == columns.size()) {
	out << colorder[0]->name();
	for (size_t i = 1; i < columns.size(); ++ i)
	    out << del << colorder[i]->name();
    }
    else {
	std::set<const char*, ibis::lessi> names;
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it)
	    names.insert((*it).first);
	for (size_t i = 0; i < colorder.size(); ++ i) {
	    if (i > 0)
		out << del;
	    out << colorder[i]->name();
	    names.erase(colorder[i]->name());
	}
	for (std::set<const char*, ibis::lessi>::const_iterator it =
		 names.begin(); it != names.end(); ++ it) {
	    ibis::part::columnList::const_iterator cit = columns.find(*it);
	    out << del << (*cit).first;
	}
    }
    out << std::endl;
} // ibis::bord::part::dumpNames

/**
   return values:
   0  -- normal (successful) completion
  -1  -- no data in-memory
  -2  -- unknown data type
  -3  -- some columns not ibis::bord::column (not in-memory)
  -4  -- error in the output stream
 */
int ibis::bord::part::dump(std::ostream& out, size_t nr,
			   const char* del) const {
    const size_t ncol = columns.size();
    if (ncol == 0 || nr == 0) return 0;
    if (del == 0) del = ",";

    std::vector<const ibis::bord::column*> clist;
    if (colorder.empty()) { // alphabetic ordering
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it) {
	    const ibis::bord::column* col =
		dynamic_cast<const ibis::bord::column*>((*it).second);
	    if (col != 0)
		clist.push_back(col);
	}
    }
    else if (colorder.size() == ncol) { // use external order
	for (size_t i = 0; i < ncol; ++ i) {
	    const ibis::bord::column* col =
		dynamic_cast<const ibis::bord::column*>(colorder[i]);
	    if (col != 0)
		clist.push_back(col);
	}
    }
    else { // externally specified ones are ordered first
	std::set<const char*, ibis::lessi> names;
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it)
	    names.insert((*it).first);
	for (size_t i = 0; i < colorder.size(); ++ i) {
	    const ibis::bord::column* col =
		dynamic_cast<const ibis::bord::column*>(colorder[i]);
	    if (col != 0) {
		clist.push_back(col);
		names.erase(col->name());
	    }
	}
	for (std::set<const char*, ibis::lessi>::const_iterator it =
		 names.begin();
	     it != names.end(); ++ it) {
	    ibis::part::columnList::const_iterator cit = columns.find(*it);
	    const ibis::bord::column* col =
		dynamic_cast<const ibis::bord::column*>((*cit).second);
	    if (col != 0)
		clist.push_back(col);
	}
    }
    if (clist.size() < ncol) return -3;

    int ierr = 0;
    // print the first row with error checking
    ierr = clist[0]->dump(out, 0U);
    if (ierr < 0) return ierr;
    for (size_t j = 1; j < ncol; ++ j) {
	out << del;
	ierr = clist[j]->dump(out, 0U);
	if (ierr < 0) return ierr;
    }
    out << "\n";
    if (! out) return -4;
    // print the remaining rows without checking the return values from
    // functions called
    if (nr > nEvents) nr = nEvents;
    for (size_t i = 1; i < nr; ++ i) {
	(void) clist[0]->dump(out, i);
	for (size_t j = 1; j < ncol; ++ j) {
	    out << del;
	    (void) clist[j]->dump(out, i);
	}
	out << "\n";
    }
    if (! out)
	ierr = -4;
    return ierr;
} // ibis::bord::part::dump

ibis::table*
ibis::bord::part::groupby(const ibis::table::stringList& keys) const {
    if (keys.empty()) return 0;

    ibis::selected sel;
    sel.select(keys); // parse the incoming arguments
    if (sel.size() == 0) return 0;

    std::vector<size_t> bad;
    ibis::bord::bufferList buf;
    buf.reserve(sel.size());
    for (size_t i = 0; i < sel.size(); ++ i) {
	columnList::const_iterator it = columns.find(sel.getName(i));
	if (it != columns.end()) {
	    const ibis::bord::column* col =
		dynamic_cast<const ibis::bord::column*>((*it).second);
	    if (col != 0) {
		buf.push_back(col->getArray());
	    }
	    else {
		bad.push_back(i);
	    }
	}
	else {
	    bad.push_back(i);
	}
    }
    if (sel.size() == 0) {
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "ibis::bord::part::groupby -- none of the names "
		"given in \"" << keys[0];
	    for (size_t i = 1; i < keys.size(); ++ i)
		lg.buffer() << ", " << keys[i];
	    lg.buffer() << "\" are valid columns names with in-memory data";
	}
	return 0;
    }
    if (! bad.empty())
	sel.remove(bad);

    // create bundle
    ibis::bundle *bdl = ibis::bundle::create(*this, sel, buf);
    if (bdl == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::bord::part::groupby -- failed to create "
	    "bundle for \"" << *sel << "\" from in-memory data";

	return 0;
    }
    const size_t nc = bdl->width();
    const size_t nr = bdl->size();
    if (nr == 0) {
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "Warning -- ibis::bord::part::groupby("
			<< keys[0];
	    for (size_t i = 1; i < keys.size(); ++ i)
		lg.buffer() << ", " << keys[i];
	    lg.buffer() << ") produced no answer on a table with nRows = "
			<< nEvents;
	}
	delete bdl;
	return 0;
    }

    // convert bundle back into a table, first generate the name and
    // description for the new table
    std::string td = "ORDER BY ";
    td += *sel;
    td += " on table ";
    td += m_name;
    td += " (";
    td += m_desc;
    td += ')';
    std::string tn = ibis::util::shortName(td);
    if (nc == 0)
	return new ibis::tabula(tn.c_str(), td.c_str(), nr);

    // prepare the types and values for the new table
    std::vector<std::string> nms(nc);
    std::vector<const char*> nmc(nc);
    ibis::table::typeList tps(nc);
    buf.resize(nc);
    for (size_t i = 0; i < nc; ++ i) {
	nms[i] = sel.getTerm(i);
	nmc[i] = nms[i].c_str();
	tps[i] = bdl->columnType(i);
	if (bdl->columnArray(i) == 0) {
	    buf[i] = 0;
	    continue;
	}

	switch (tps[i]) {
	case ibis::INT:
	    buf[i] = new array_t<int32_t>
		(* static_cast<const array_t<int32_t>*>(bdl->columnArray(i)));
	    break;
	case ibis::UINT:
	    buf[i] = new array_t<uint32_t>
		(* static_cast<const array_t<uint32_t>*>(bdl->columnArray(i)));
	    break;
	case ibis::LONG:
	    buf[i] = new array_t<int64_t>
		(* static_cast<const array_t<int64_t>*>(bdl->columnArray(i)));
	    break;
	case ibis::ULONG:
	    buf[i] = new array_t<uint64_t>
		(* static_cast<const array_t<uint64_t>*>(bdl->columnArray(i)));
	    break;
	case ibis::FLOAT:
	    buf[i] = new array_t<float>
		(* static_cast<const array_t<float>*>(bdl->columnArray(i)));
	    break;
	case ibis::DOUBLE:
	    buf[i] = new array_t<double>
		(* static_cast<const array_t<double>*>(bdl->columnArray(i)));
	    break;
	default:
	    buf[i] = 0;
	    break;
	}
    }
    // count(*) is always appended to the end
    array_t<uint32_t>* cnts = new array_t<uint32_t>;
    bdl->rowCounts(*cnts);
    nmc.push_back("COUNT(*)");
    tps.push_back(ibis::UINT);
    buf.push_back(cnts);
    delete bdl;
    return new ibis::bord(tn.c_str(), td.c_str(), nr, nmc, tps, buf);
} // ibis::bord::part::groupby

long ibis::bord::part::reorder(const ibis::table::stringList& cols) {
    long ierr = 0;
    if (nRows() == 0 || nColumns() == 0) return ierr;

    std::string evt = "part[";
    evt += m_name;
    evt += "]::reorder";
    writeLock lock(this, evt.c_str()); // can't process other operations
    for (columnList::const_iterator it = columns.begin();
	 it != columns.end();
	 ++ it) { // purge all index files
	(*it).second->unloadIndex();
	(*it).second->purgeIndexFile();
    }

    // first gather all numerical valued columns
    typedef std::vector<ibis::column*> colVector;
    std::set<const char*, ibis::lessi> used;
    colVector keys, load; // sort according to the keys
    for (ibis::table::stringList::const_iterator nit = cols.begin();
	 nit != cols.end(); ++ nit) {
	ibis::part::columnList::iterator it = columns.find(*nit);
	if (it != columns.end()) {
	    used.insert((*it).first);
	    if (! (*it).second->isNumeric()) {
		load.push_back((*it).second);
	    }
	    else if ((*it).second->upperBound() > (*it).second->lowerBound()) {
		keys.push_back((*it).second);
	    }
	    else {
		(*it).second->computeMinMax();
		if ((*it).second->upperBound() > (*it).second->lowerBound())
		    keys.push_back((*it).second);
		else
		    load.push_back((*it).second);
	    }
	}
    }

    if (keys.empty()) { // use all integral values
	if (ibis::gVerbose > 2) {
	    if (cols.empty()) {
		logMessage("reorder", "user did not specify ordering keys, "
			   "will attempt to use all integer columns as "
			   "ordering keys");
	    }
	    else {
		std::ostringstream oss;
		oss << cols[0];
		for (unsigned i = 1; i < cols.size(); ++ i)
		    oss << ", " << cols[i];
		logMessage("reorder", "user specified ordering keys \"%s\" "
			   "does not match any numerical columns with more "
			   "than one distinct value, will attempt to use "
			   "all integer columns as ordering keys",
			   oss.str().c_str());
	    }
	}

	load.clear();
	keys.clear();
	array_t<double> width;
	for (ibis::part::columnList::iterator it = columns.begin();
	     it != columns.end(); ++ it) {
	    if (! (*it).second->isInteger()) {
		load.push_back((*it).second);
	    }
	    else if ((*it).second->upperBound() > (*it).second->lowerBound()) {
		keys.push_back((*it).second);
		width.push_back((*it).second->upperBound() -
				(*it).second->lowerBound());
	    }
	    else {
		double cmin, cmax;
		(*it).second->computeMinMax(0, cmin, cmax);
		if (cmax > cmin) {
		    keys.push_back((*it).second);
		    width.push_back(cmax - cmin);
		}
		else {
		    load.push_back((*it).second);
		}
	    }
	}
	if (keys.empty()) return 0; //no integral values to use as key
	if (keys.size() > 1) {
	    colVector tmp(keys.size());
	    array_t<uint32_t> idx;
	    width.sort(idx);
	    for (size_t i = 0; i < keys.size(); ++ i)
		tmp[i] = keys[idx[i]];
	    tmp.swap(keys);
	}
    }
    else {
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it) {
	    std::set<const char*, ibis::lessi>::const_iterator uit =
		used.find((*it).first);
	    if (uit == used.end())
		load.push_back((*it).second);
	}
    }
    if (keys.empty()) {
	if (ibis::gVerbose > 1)
	    logMessage("reorder", "no keys found for sorting, do nothing");
	return 0;
    }
    if (ibis::gVerbose > 1) {
	std::ostringstream oss;
	oss << evt << '(' << keys[0]->name();
	for (unsigned i = 1; i < keys.size(); ++ i)
	    oss << ", " << keys[i]->name();
	oss << ')';
	evt = oss.str();
    }
    ibis::util::timer mytimer(evt.c_str(), 1);

    ierr = nEvents;
    array_t<uint32_t> ind1;
    { // the sorting loop -- use a block to limit the scope of starts and ind0
	array_t<uint32_t>  starts, ind0;
	for (uint32_t i = 0; i < keys.size(); ++ i) {
	    ibis::bord::column* col =
		dynamic_cast<ibis::bord::column*>(keys[i]);
	    if (col == 0) {
		logError("reorder", "all columns must be in-memory");
		return -2;
	    }

	    switch (keys[i]->type()) {
	    case ibis::CATEGORY:
		ierr = reorderValues(* static_cast<array_t<uint32_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::DOUBLE:
		ierr = reorderValues(* static_cast<array_t<double>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::FLOAT:
		ierr = reorderValues(* static_cast<array_t<float>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::ULONG:
		ierr = reorderValues(* static_cast<array_t<uint64_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::LONG:
		ierr = reorderValues(* static_cast<array_t<int64_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::UINT:
		ierr = reorderValues(* static_cast<array_t<uint32_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::INT:
		ierr = reorderValues(* static_cast<array_t<int32_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::USHORT:
		ierr = reorderValues(* static_cast<array_t<uint16_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::SHORT:
		ierr = reorderValues(* static_cast<array_t<int16_t>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::UBYTE:
		ierr = reorderValues(* static_cast<array_t<unsigned char>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    case ibis::BYTE:
		ierr = reorderValues(* static_cast<array_t<char>*>
				     (col->getArray()), ind1, ind0, starts);
		break;
	    default:
		logWarning("reorder", "column %s type %d is not supported",
			   keys[i]->name(), static_cast<int>(keys[i]->type()));
		break;
	    }

	    if (ierr == static_cast<long>(nRows())) {
		ind1.swap(ind0);
	    }
	    else {
		logError("reorder", "failed to reorder column %s, ierr=%ld.  "
			 "data files are no longer consistent!",
			 keys[i]->name(), ierr);
	    }
	}
    }
#if defined(DEBUG)
    {
	ibis::util::logger lg(4);
	std::vector<bool> marks(ind1.size(), false);
	for (uint32_t i = 0; i < ind1.size(); ++ i) {
	    if (i != ind1[i])
		lg.buffer() << "ind[" << i << "]=" << ind1[i] << "\n";
	    if (ind1[i] < marks.size())
		marks[ind1[i]] = true;
	}
	bool isperm = true;
	for (uint32_t i = 0; isperm && i < marks.size(); ++ i)
	    isperm = marks[i];
	if (isperm)
	    lg.buffer() << "array ind IS a permutation\n";
	else
	    lg.buffer() << "array ind is NOT a permutation\n";
    }
#endif
    for (ibis::part::columnList::const_iterator it = columns.begin();
	 it != columns.end();
	 ++ it) { // update the m_sorted flag of each column
	(*it).second->isSorted((*it).second == keys[0]);
    }

    for (uint32_t i = 0; i < load.size(); ++ i) {
	ibis::bord::column* col = dynamic_cast<ibis::bord::column*>(load[i]);
	if (col == 0) {
	    logError("reorder", "all columns must be in-memory");
	    return -2;
	}

	switch (load[i]->type()) {
	case ibis::CATEGORY:
	case ibis::TEXT:
	    ierr = reorderStrings(* static_cast<std::vector<std::string>*>
				  (col->getArray()), ind1);
	    break;
	case ibis::DOUBLE:
	    ierr = reorderValues(* static_cast<array_t<double>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::FLOAT:
	    ierr = reorderValues(* static_cast<array_t<float>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::ULONG:
	    ierr = reorderValues(* static_cast<array_t<uint64_t>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::LONG:
	    ierr = reorderValues(* static_cast<array_t<int64_t>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::UINT:
	    ierr = reorderValues(* static_cast<array_t<uint32_t>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::INT:
	    ierr = reorderValues(* static_cast<array_t<int32_t>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::USHORT:
	    ierr = reorderValues(* static_cast<array_t<uint16_t>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::SHORT:
	    ierr = reorderValues(* static_cast<array_t<int16_t>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::UBYTE:
	    ierr = reorderValues(* static_cast<array_t<unsigned char>*>
				 (col->getArray()), ind1);
	    break;
	case ibis::BYTE:
	    ierr = reorderValues(* static_cast<array_t<char>*>
				 (col->getArray()), ind1);
	    break;
	default:
	    logError("reorder", "column %s type %s is not supported",
		     keys[i]->name(),
		     ibis::TYPESTRING[static_cast<int>(keys[i]->type())]);
	    break;
	}
    }
    return ierr;
} // ibis::bord::part::reorder

template <typename T>
long ibis::bord::part::reorderValues(array_t<T>& vals,
				     const array_t<uint32_t>& indin,
				     array_t<uint32_t>& indout,
				     array_t<uint32_t>& starts) const {
    ibis::horometer timer;
    if (ibis::gVerbose > 4)
	timer.start();

    if (vals.size() != nEvents ||
	(indin.size() != vals.size() && ! indin.empty())) {
	if (ibis::gVerbose > 1)
	    logMessage("reorderValues", "array sizes do not match, both "
		       "vals.size(%ld) and indin.size(%ld) are expected to "
		       "be %ld", static_cast<long>(vals.size()),
		       static_cast<long>(indin.size()),
		       static_cast<long>(nEvents));
	return -3;
    }
    if (indin.empty() || starts.size() < 2 || starts[0] != 0
	|| starts.back() != vals.size()) {
	starts.resize(2);
	starts[0] = 0;
	starts[1] = vals.size();
	if (ibis::gVerbose > 1)
	    logMessage("reorderValues", "(re)set array starts to contain "
		       "[0, %lu]", static_cast<long unsigned>(nEvents));
    }

    uint32_t nseg = starts.size() - 1;
    if (nseg > nEvents) { // no sorting necessary
	indout.resize(nEvents);
	for (uint32_t i = 0; i < nEvents; ++i)
	    indout[i] = indin[i];
    }
    else if (nseg > 1) { // sort multiple blocks
	indout.resize(nEvents);
	array_t<uint32_t> starts2;

	for (uint32_t iseg = 0; iseg < nseg; ++ iseg) {
	    const uint32_t segstart = starts[iseg];
	    const uint32_t segsize = starts[iseg+1]-starts[iseg];
	    if (segsize > 1) {
		// copy the segment into a temporary array, then sort it
		array_t<T> tmp(segsize);
		array_t<uint32_t> ind0;
		for (unsigned i = 0; i < segsize; ++ i)
		    tmp[i] = vals[indin[i+segstart]];
		tmp.sort(ind0);

		starts2.push_back(segstart);
		T last = tmp[ind0[0]];
		indout[segstart] = indin[ind0[0] + segstart];
		for (unsigned i = 1; i < segsize; ++ i) {
		    indout[i+segstart] = indin[ind0[i] + segstart];
		    if (tmp[ind0[i]] > last) {
			starts2.push_back(i + segstart);
			last = tmp[ind0[i]];
		    }
		}
	    }
	    else { // segment contains only one element
		starts2.push_back(segstart);
		indout[segstart] = indin[segstart];
	    }
	}
	starts2.push_back(nEvents);
	starts.swap(starts2);
    }
    else { // all in one block
	vals.sort(indout);

	starts.clear();
	starts.push_back(0U);
	T last = vals[indout[0]];
	for (uint32_t i = 1; i < nEvents; ++ i) {
	    if (vals[indout[i]] > last) {
		starts.push_back(i);
		last = vals[indout[i]];
	    }
	}
	starts.push_back(nEvents);
    }

    { // place values in the new order
	array_t<T> tmp(nEvents);
	for (uint32_t i = 0; i < nEvents; ++ i)
	    tmp[i] = vals[indout[i]];
	vals.swap(tmp);
    }
    if (ibis::gVerbose > 4) {
	timer.stop();
	nseg = starts.size() - 1;
	logMessage("reorderValues", "reordered %lu value%s (into %lu "
		   "segment%s) in %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(nEvents), (nEvents>1 ? "s" : ""),
		   static_cast<long unsigned>(nseg), (nseg>1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return nEvents;
} // ibis::part::reorderValues

template <typename T>
long ibis::bord::part::reorderValues(array_t<T>& vals,
				     const array_t<uint32_t>& ind) const {
    if (vals.size() != nEvents || ind.size() != vals.size()) {
	if (ibis::gVerbose > 1)
	    logMessage("reorderValues", "array sizes do not match, both "
		       "vals.size(%ld) and ind.size(%ld) are expected to "
		       "be %ld", static_cast<long>(vals.size()),
		       static_cast<long>(ind.size()),
		       static_cast<long>(nEvents));
	return -3;
    }
    array_t<T> tmp(vals.size());
    for (uint32_t i = 0; i < vals.size(); ++ i)
	tmp[i] = vals[ind[i]];
    tmp.swap(vals);
    return nEvents;
} // ibis::bord::part::reorderValues

long ibis::bord::part::reorderStrings(std::vector<std::string>& vals,
				      const array_t<uint32_t>& ind) const {
    if (vals.size() != nEvents || ind.size() != vals.size()) {
	if (ibis::gVerbose > 1)
	    logMessage("reorderValues", "array sizes do not match, both "
		       "vals.size(%ld) and ind.size(%ld) are expected to "
		       "be %ld", static_cast<long>(vals.size()),
		       static_cast<long>(ind.size()),
		       static_cast<long>(nEvents));
	return -3;
    }
    std::vector<std::string> tmp(vals.size());
    for (uint32_t i = 0; i < vals.size(); ++ i)
	tmp[i] = vals[ind[i]];
    tmp.swap(vals);
    return nEvents;
} // ibis::bord::part::reorderValues

void ibis::bord::part::reverseRows() {
    for (ibis::part::columnList::iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	reinterpret_cast<ibis::bord::column*>((*it).second)->reverseRows();
    }
} // ibis::bord::reverseRows

int ibis::bord::part::limit(size_t nr) {
    int ierr = 0;
    if (nEvents <= nr) return ierr;

    for (ibis::part::columnList::iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	ierr = reinterpret_cast<ibis::bord::column*>((*it).second)->limit(nr);
	if (ierr < 0) return ierr;
    }
    nEvents = nr;
    return ierr;
} // ibis::bord::part::limit

ibis::bord::column::column(const ibis::bord::part *tbl, ibis::TYPE_T t,
			   const char *cn, void *st,
			   const char *de, double lo, double hi)
    : ibis::column(tbl, t, cn, de, lo, hi), buffer(st) {
} // ibis::bord::column::column

///@note Transfer the ownership of @c st to the new @c column object.
ibis::bord::column::column(const ibis::bord::part *tbl,
			   const ibis::column& old, void *st)
    : ibis::column(tbl, old.type(), old.name(), old.description(),
		   old.lowerBound(), old.upperBound()),
      buffer(st) {
} // ibis::bord::column::column

ibis::bord::column::column(const ibis::bord::column &c)
    : ibis::column(c.thePart, c.m_type, c.m_name.c_str(), c.m_desc.c_str(),
		   c.lower, c.upper) {
    switch (c.m_type) {
    case ibis::BYTE: {
	buffer = new array_t<char>(* static_cast<array_t<char>*>(c.buffer));
	break;}
    case ibis::UBYTE: {
	buffer = new array_t<unsigned char>
	    (* static_cast<array_t<unsigned char>*>(c.buffer));
	break;}
    case ibis::SHORT: {
	buffer = new array_t<int16_t>
	    (* static_cast<array_t<int16_t>*>(c.buffer));
	break;}
    case ibis::USHORT: {
	buffer = new array_t<uint16_t>
	    (* static_cast<array_t<uint16_t>*>(c.buffer));
	break;}
    case ibis::INT: {
	buffer = new array_t<int32_t>
	    (* static_cast<array_t<int32_t>*>(c.buffer));
	break;}
    case ibis::UINT: {
	buffer = new array_t<uint32_t>
	    (* static_cast<array_t<uint32_t>*>(c.buffer));
	break;}
    case ibis::LONG: {
	buffer = new array_t<int64_t>
	    (* static_cast<array_t<int64_t>*>(c.buffer));
	break;}
    case ibis::ULONG: {
	buffer = new array_t<uint64_t>
	    (* static_cast<array_t<uint64_t>*>(c.buffer));
	break;}
    case ibis::FLOAT: {
	buffer = new array_t<float>
	    (* static_cast<array_t<float>*>(c.buffer));
	break;}
    case ibis::DOUBLE: {
	buffer = new array_t<double>
	    (* static_cast<array_t<double>*>(c.buffer));
	break;}
    default: {
	buffer = 0;
	ibis::util::logMessage
	    ("ibis::bord::column::column",
	     "copy constructor can not handle column (%s) with type %s",
	     c.name(), ibis::TYPESTRING[(int)c.type()]);
	break;}
    }
    m_sorted = c.m_sorted;
} // ibis::bord::column::column

ibis::bord::column::~column() {
    switch (m_type) {
    case ibis::BYTE: {
	delete static_cast<array_t<char>*>(buffer);
	break;}
    case ibis::UBYTE: {
	delete static_cast<array_t<unsigned char>*>(buffer);
	break;}
    case ibis::SHORT: {
	delete static_cast<array_t<int16_t>*>(buffer);
	break;}
    case ibis::USHORT: {
	delete static_cast<array_t<uint16_t>*>(buffer);
	break;}
    case ibis::INT: {
	delete static_cast<array_t<int32_t>*>(buffer);
	break;}
    case ibis::UINT: {
	delete static_cast<array_t<uint32_t>*>(buffer);
	break;}
    case ibis::LONG: {
	delete static_cast<array_t<int64_t>*>(buffer);
	break;}
    case ibis::ULONG: {
	delete static_cast<array_t<uint64_t>*>(buffer);
	break;}
    case ibis::FLOAT: {
	delete static_cast<array_t<float>*>(buffer);
	break;}
    case ibis::DOUBLE: {
	delete static_cast<array_t<double>*>(buffer);
	break;}
    default: {
	break;}
    }
} // ibis::bord::column::~column

void ibis::bord::column::computeMinMax(const char *,
				      double &min, double &max) const {
    if (buffer == 0) return;

    switch (m_type) {
    case ibis::UBYTE: {
	const array_t<unsigned char> &val =
	    * static_cast<const array_t<unsigned char>*>(buffer);

	unsigned char imin = val[0];
	unsigned char imax = val[0];
	long unsigned nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %u, max = %u",
		       nelm, static_cast<unsigned>(imin),
		       static_cast<unsigned>(imax));
	min = imin;
	max = imax;
	break;}
    case ibis::BYTE: {
	const array_t<signed char> &val =
	    * static_cast<const array_t<signed char>*>(buffer);

	signed char imin = val[0];
	signed char imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %d, max = %d",
		       nelm, static_cast<int>(imin), static_cast<int>(imax));
	min = imin;
	max = imax;
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t> &val = 
	    * static_cast<const array_t<uint16_t>*>(buffer);

	uint16_t imin = val[0];
	uint16_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %u, max = %u",
		       nelm, static_cast<unsigned>(imin),
		       static_cast<unsigned>(imax));
	min = imin;
	max = imax;
	break;}
    case ibis::SHORT: {
	const array_t<int16_t> &val =
	    * static_cast<const array_t<int16_t>*>(buffer);

	int16_t imin = val[0];
	int16_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %d, max = %d",
		       nelm, static_cast<int>(imin), static_cast<int>(imax));
	min = imin;
	max = imax;
	break;}
    case ibis::UINT: {
	const array_t<uint32_t> &val =
	    * static_cast<const array_t<uint32_t>*>(buffer);

	uint32_t imin = val[0];
	uint32_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %lu, max = %lu",
		       nelm, static_cast<long unsigned>(imin),
		       static_cast<long unsigned>(imax));
	min = imin;
	max = imax;
	break;}
    case ibis::INT: {
	const array_t<int32_t> &val =
	    *static_cast<const array_t<int32_t>*>(buffer);

	int32_t imin = val[0];
	int32_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %ld, max = %ld",
		       nelm, static_cast<long>(imin), static_cast<long>(imax));
	min = imin;
	max = imax;
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t> &val =
	    * static_cast<const array_t<uint64_t>*>(buffer);

	uint64_t imin = val[0];
	uint64_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %llu, max = %llu",
		       nelm, static_cast<long long unsigned>(imin),
		       static_cast<long long unsigned>(imax));
	min = static_cast<double>(imin);
	max = static_cast<double>(imax);
	break;}
    case ibis::LONG: {
	const array_t<int64_t> &val =
	    *static_cast<const array_t<int64_t>*>(buffer);

	int64_t imin = val[0];
	int64_t imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %lld, max = %lld",
		       nelm, static_cast<long long>(imin),
		       static_cast<long long>(imax));
	min = static_cast<double>(imin);
	max = static_cast<double>(imax);
	break;}
    case ibis::FLOAT: {
	const array_t<float> &val =
	    * static_cast<const array_t<float>*>(buffer);

	float imin = val[0];
	float imax = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (imin > val[i])
		imin = val[i];
	    else if (imax < val[i])
		imax = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %g, max = %g",
		       nelm, imin, imax);
	min = imin;
	max = imax;
	break;}
    case ibis::DOUBLE: {
	const array_t<double> &val =
	    * static_cast<const array_t<double>*>(buffer);

	min = val[0];
	max = val[0];
	uint32_t nelm = val.size();
	for (uint32_t i = 1; i < nelm; ++i) {
	    if (min > val[i])
		min = val[i];
	    else if (max < val[i])
		max = val[i];
	}
	if (ibis::gVerbose > 5)
	    logMessage("computeMinMax", "nelm = %lu, min = %lg, max = %lg",
		       nelm, min, max);
	break;}
    default:
	if (ibis::gVerbose > 2)
	    logMessage("computeMinMax", "column type %s is not one of the "
		       "supported types (int, uint, float, double)",
		       TYPESTRING[static_cast<int>(m_type)]);
	min = 0;
	max = (thePart != 0) ? thePart->nRows() : -DBL_MAX;
    } // switch(m_type)
} // ibis::bord::column::computeMinMax

long ibis::bord::column::evaluateRange(const ibis::qContinuousRange& cmp,
				       const ibis::bitvector& mask,
				       ibis::bitvector& res) const {
    long ierr = -1;
    const ibis::bord::part* mytable =
	dynamic_cast<const ibis::bord::part*>(thePart);
    if (mytable == 0) return ierr;

    switch (m_type) {
    case ibis::UBYTE: {
	const array_t<unsigned char> &val =
	    * static_cast<const array_t<unsigned char>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::BYTE: {
	const array_t<char> &val =
	    * static_cast<const array_t<char>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t> &val =
	    * static_cast<const array_t<uint16_t>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::SHORT: {
	const array_t<int16_t> &val =
	    * static_cast<const array_t<int16_t>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::UINT: {
	const array_t<uint32_t> &val =
	    * static_cast<const array_t<uint32_t>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::INT: {
	const array_t<int32_t> &val =
	    * static_cast<const array_t<int32_t>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t> &val =
	    * static_cast<const array_t<uint64_t>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::LONG: {
	const array_t<int64_t> &val =
	    * static_cast<const array_t<int64_t>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::FLOAT: {
	const array_t<float> &val =
	    * static_cast<const array_t<float>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    case ibis::DOUBLE: {
	const array_t<double> &val =
	    * static_cast<const array_t<double>*>(buffer);
	ierr = mytable->doScan(val, cmp, mask, res);
	break;}
    default:
	if (ibis::gVerbose > 2)
	    logMessage("evaluateRange", "column type %s is not one of the "
		       "supported types (int, uint, float, double)",
		       TYPESTRING[static_cast<int>(m_type)]);
	ierr = -2;
    } // switch(m_type)
    return ierr;
} // ibis::bord::column::evaluateRange

array_t<int32_t>*
ibis::bord::column::selectInts(const ibis::bitvector &mask) const {
    array_t<int32_t>* array = new array_t<int32_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::INT || m_type == ibis::UINT ||
	m_type == ibis::CATEGORY || m_type == ibis::TEXT) {
	const array_t<int32_t> &prop =
	    * static_cast<const array_t<int32_t>*>(buffer);
	uint32_t i = 0;
	array->resize(tot);
	const long unsigned nprop = prop.size();
#if defined(DEBUG)
	logMessage("DEBUG", "selectInts mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if defined(DEBUG)
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>(idx0[1]),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
#if defined(DEBUG)
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>
			       (idx0[1]<=nprop ? idx0[1] : nprop),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::SHORT) {
	const array_t<int16_t> &prop =
	    * static_cast<const array_t<int16_t>*>(buffer);
	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::USHORT) {
	const array_t<uint16_t> &prop =
	    * static_cast<const array_t<uint16_t>*>(buffer);
	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::BYTE) {
	const array_t<char> &prop =
	    * static_cast<const array_t<char>*>(buffer);
	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::UBYTE) {
	const array_t<unsigned char> &prop =
	    * static_cast<const array_t<unsigned char>*>(buffer);
	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectInts", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectInts", "retrieving %lu integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectInts

/// Can be called on columns of unsigned integral types, UINT, CATEGORY,
/// USHORT, and UBYTE.
array_t<uint32_t>*
ibis::bord::column::selectUInts(const ibis::bitvector& mask) const {
    array_t<uint32_t>* array = new array_t<uint32_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    if (m_type == ibis::UINT || m_type == ibis::CATEGORY ||
	m_type == ibis::TEXT) {
	const array_t<uint32_t> &prop =
	    * static_cast<const array_t<uint32_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectUInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == USHORT) {
	const array_t<uint16_t> &prop =
	    * static_cast<const array_t<uint16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectUInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == UBYTE) {
	const array_t<unsigned char> &prop =
	    * static_cast<const array_t<unsigned char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectUInts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectUInts", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectUInts", "retrieving %lu unsigned integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectUInts

/// Can be called on all integral types.  Note that 64-byte unsigned
/// integers are simply treated as signed integer.  This may cause the
/// values to be interperted incorrectly.  Shorter version of unsigned
/// integers are treated correctly as positive values.
array_t<int64_t>*
ibis::bord::column::selectLongs(const ibis::bitvector& mask) const {
    array_t<int64_t>* array = new array_t<int64_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::LONG || m_type == ibis::ULONG) {
	const array_t<int64_t> &prop =
	    * static_cast<const array_t<int64_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const long unsigned nprop = prop.size();
#if defined(DEBUG)
	logMessage("DEBUG", "selectLongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if defined(DEBUG)
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>(idx0[1]),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
#if defined(DEBUG)
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>
			       (idx0[1]<=nprop ? idx0[1] : nprop),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::UINT || m_type == ibis::CATEGORY ||
	     m_type == ibis::TEXT) {
	const array_t<uint32_t> &prop =
	    * static_cast<const array_t<uint32_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const long unsigned nprop = prop.size();
#if defined(DEBUG)
	logMessage("DEBUG", "selectLongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if defined(DEBUG)
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>(idx0[1]),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
#if defined(DEBUG)
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>
			       (idx0[1]<=nprop ? idx0[1] : nprop),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::INT) {
	const array_t<int32_t> &prop =
	    * static_cast<const array_t<int32_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const long unsigned nprop = prop.size();
#if defined(DEBUG)
	logMessage("DEBUG", "selectLongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if defined(DEBUG)
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>(idx0[1]),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
#if defined(DEBUG)
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if defined(DEBUG)
		    logMessage("DEBUG", "copying range [%lu, %lu), i=%lu",
			       static_cast<long unsigned>(*idx0),
			       static_cast<long unsigned>
			       (idx0[1]<=nprop ? idx0[1] : nprop),
			       static_cast<long unsigned>(i));
#endif
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
#if defined(DEBUG)
			logMessage("DEBUG", "copying value %lu to i=%lu",
				   static_cast<long unsigned>(idx0[j]),
				   static_cast<long unsigned>(i));
#endif
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::USHORT) {
	const array_t<uint16_t> &prop =
	    * static_cast<const array_t<uint16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == SHORT) {
	const array_t<int16_t> &prop =
	    * static_cast<const array_t<int16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == UBYTE) {
	const array_t<unsigned char> &prop =
	    * static_cast<const array_t<unsigned char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == BYTE) {
	const array_t<char> &prop =
	    * static_cast<const array_t<char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectLongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectLongs", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectLongs", "retrieving %lu integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectLongs

/// Put selected values of a float column into an array.
array_t<float>*
ibis::bord::column::selectFloats(const ibis::bitvector& mask) const {
    array_t<float>* array = new array_t<float>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    if (m_type == FLOAT) {
	const array_t<float> &prop =
	    * static_cast<const array_t<float>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectFloats", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::USHORT) {
	const array_t<uint16_t> &prop =
	    * static_cast<const array_t<uint16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectFloats", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == SHORT) {
	const array_t<int16_t> &prop =
	    * static_cast<const array_t<int16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectFloats", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == UBYTE) {
	const array_t<unsigned char> &prop =
	    * static_cast<const array_t<unsigned char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectFloats", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == BYTE) {
	const array_t<char> &prop =
	    * static_cast<const array_t<char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j < idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else { // need to check loop bounds against nprop
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j < (idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}
	if (i != tot) {
	    array->resize(i);
	    logWarning("selectFloats", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectFloats", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectFloats", "retrieving %lu float value%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectFloats

/// Put the selected values into an array as doubles.
/// @note Any column type could be selected as doubles.  Other selectXXXs
/// function only work on the same data type.  This is the only function
/// that allows one to convert to a different type.  This is mainly to 
array_t<double>*
ibis::bord::column::selectDoubles(const ibis::bitvector& mask) const {
    array_t<double>* array = new array_t<double>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    switch(m_type) {
    case ibis::CATEGORY:
    case ibis::UINT: {
	const array_t<uint32_t> &prop =
	    * static_cast<const array_t<uint32_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu unsigned integer%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::INT: {
	const array_t<int32_t> &prop =
	    * static_cast<const array_t<int32_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu integer%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t> &prop =
	    * static_cast<const array_t<uint16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu unsigned short "
		       "integer%s took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::SHORT: {
	const array_t<int16_t> &prop =
	    * static_cast<const array_t<int16_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu short integer%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char> &prop =
	    * static_cast<const array_t<unsigned char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu unsigned 1-byte "
		       "integer%s took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::BYTE: {
	const array_t<char> &prop =
	    * static_cast<const array_t<char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu 1-byte integer%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::FLOAT: {
	const array_t<float> &prop =
	    * static_cast<const array_t<float>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu float value%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::DOUBLE: {
	const array_t<double> &prop =
	    * static_cast<const array_t<double>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (prop[idx0[j]]);
		    }
		}
		++ index;
	    }
	}
	else {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
		    for (uint32_t j = *idx0;
			 j<(idx0[1]<=nprop ? idx0[1] : nprop);
			 ++j, ++i) {
			(*array)[i] = (prop[j]);
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop)
			    (*array)[i] = (prop[idx0[j]]);
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectDoubles", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectDoubles", "retrieving %lu double value%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    default: {
	logWarning("selectDoubles", "incompatible data type");
	break;}
    }
    return array;
} // ibis::bord::column::selectDoubles

void ibis::bord::column::getString(uint32_t i, std::string &val) const {
    val.erase();
    if (m_type == ibis::TEXT || m_type == ibis::CATEGORY) {
	std::vector<std::string> *str_column = 
	    static_cast<std::vector<std::string> *>(buffer);
	if ( i < str_column->size())
	    val = str_column->at(i);
    }
} // ibis::bord::column::getString

void ibis::bord::column::reverseRows() {
    switch(m_type) {
    case ibis::ULONG: {
	array_t<uint64_t> &prop =
	    * static_cast<array_t<uint64_t>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::LONG: {
	array_t<int64_t> &prop =
	    * static_cast<array_t<int64_t>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	array_t<uint32_t> &prop =
	    * static_cast<array_t<uint32_t>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::INT: {
	array_t<int32_t> &prop =
	    * static_cast<array_t<int32_t>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> &prop =
	    * static_cast<array_t<uint16_t>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::SHORT: {
	array_t<int16_t> &prop =
	    * static_cast<array_t<int16_t>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> &prop =
	    * static_cast<array_t<unsigned char>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::BYTE: {
	array_t<char> &prop =
	    * static_cast<array_t<char>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::FLOAT: {
	array_t<float> &prop =
	    * static_cast<array_t<float>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    case ibis::DOUBLE: {
	array_t<double> &prop =
	    * static_cast<array_t<double>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    default: {
	logWarning("reverseRows", "incompatible data type");
	break;}
    }
} // ibis::bord::column::reverseRows

int ibis::bord::column::limit(size_t nr) {
    switch(m_type) {
    case ibis::ULONG: {
	array_t<uint64_t> &prop =
	    * static_cast<array_t<uint64_t>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::LONG: {
	array_t<int64_t> &prop =
	    * static_cast<array_t<int64_t>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	array_t<uint32_t> &prop =
	    * static_cast<array_t<uint32_t>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::INT: {
	array_t<int32_t> &prop =
	    * static_cast<array_t<int32_t>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::USHORT: {
	array_t<uint16_t> &prop =
	    * static_cast<array_t<uint16_t>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::SHORT: {
	array_t<int16_t> &prop =
	    * static_cast<array_t<int16_t>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::UBYTE: {
	array_t<unsigned char> &prop =
	    * static_cast<array_t<unsigned char>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::BYTE: {
	array_t<char> &prop =
	    * static_cast<array_t<char>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::FLOAT: {
	array_t<float> &prop =
	    * static_cast<array_t<float>*>(buffer);
	prop.resize(nr);
	return 0;}
    case ibis::DOUBLE: {
	array_t<double> &prop =
	    * static_cast<array_t<double>*>(buffer);
	prop.resize(nr);
	return 0;}
    default: {
	logWarning("reverseRows", "incompatible data type");
	return -1;}
    }
} // ibis::bord::column::limit

ibis::bord::cursor::cursor(const ibis::bord &t)
    : buffer(t.nColumns()), tab(t), curRow(-1) {
    if (buffer.empty()) return;
    for (size_t j = 0; j < t.nColumns(); ++ j) {
	const ibis::bord::column *col =
	    dynamic_cast<const ibis::bord::column*>(t.mypart.getColumn(j));
	buffer[j].cname = col->name();
	buffer[j].ctype = col->type();
	buffer[j].cval = col->getArray();
	bufmap[col->name()] = j;
    }
} // ibis::bord::cursor::cursor

int ibis::bord::cursor::dump(std::ostream& out, const char* del) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (! out) return -4;

    const size_t cr = static_cast<size_t>(curRow);
    int ierr = dumpIJ(out, cr, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (size_t j = 1; j < buffer.size(); ++ j) {
	out << del;
	ierr = dumpIJ(out, cr, j);
	if (ierr < 0) return ierr;
    }
    out << "\n";
    return (out ? ierr : -4);
} // ibis::bord::cursor::dump

void ibis::bord::cursor::fillRow(ibis::table::row& res) const {
    res.clear();
    for (size_t j = 0; j < buffer.size(); ++ j) {
	switch (buffer[j].ctype) {
	case ibis::BYTE: {
	    res.bytesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.bytesvalues.push_back
		    ((* static_cast<const array_t<const char>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.bytesvalues.push_back(0x7F);
	    }
	    break;}
	case ibis::UBYTE: {
	    res.bytesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.ubytesvalues.push_back
		    ((* static_cast<const array_t<const unsigned char>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.ubytesvalues.push_back(0xFF);
	    }
	    break;}
	case ibis::SHORT: {
	    res.shortsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.shortsvalues.push_back
		    ((* static_cast<const array_t<int16_t>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.shortsvalues.push_back(0x7FFF);
	    }
	    break;}
	case ibis::USHORT: {
	    res.shortsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.ushortsvalues.push_back
		    ((* static_cast<const array_t<const uint16_t>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.ushortsvalues.push_back(0xFFFF);
	    }
	    break;}
	case ibis::INT: {
	    res.intsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.intsvalues.push_back
		    ((* static_cast<const array_t<int32_t>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.intsvalues.push_back(0x7FFFFFFF);
	    }
	    break;}
	case ibis::UINT: {
	    res.intsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.uintsvalues.push_back
		    ((* static_cast<const array_t<const uint32_t>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.uintsvalues.push_back(0xFFFFFFFF);
	    }
	    break;}
	case ibis::LONG: {
	    res.longsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.longsvalues.push_back
		    ((* static_cast<const array_t<int64_t>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.longsvalues.push_back(0x7FFFFFFFFFFFFFFFLL);
	    }
	    break;}
	case ibis::ULONG: {
	    res.longsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.ulongsvalues.push_back
		    ((* static_cast<const array_t<const uint64_t>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.ulongsvalues.push_back(0xFFFFFFFFFFFFFFFFULL);
	    }
	    break;}
	case ibis::FLOAT: {
	    res.floatsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.floatsvalues.push_back
		    ((* static_cast<const array_t<float>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.floatsvalues.push_back
		    (std::numeric_limits<float>::quiet_NaN());
	    }
	    break;}
	case ibis::DOUBLE: {
	    res.doublesnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.doublesvalues.push_back
		    ((* static_cast<const array_t<const double>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.doublesvalues.push_back
		    (std::numeric_limits<double>::quiet_NaN());
	    }
	    break;}
	case ibis::CATEGORY: {
	    res.catsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.catsvalues.push_back
		    ((* static_cast<const std::vector<std::string>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.catsvalues.push_back("");
	    }
	    break;}
	case ibis::TEXT: {
	    res.textsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		res.textsvalues.push_back
		    ((* static_cast<const std::vector<std::string>*>
		      (buffer[j].cval))[curRow]);
	    }
	    else {
		res.textsvalues.push_back("");
	    }
	    break;}
	default: { // unexpected
	    if (ibis::gVerbose > 1)
		ibis::util::logMessage
		    ("Warning", "ibis::bord::cursor::fillRow is not expected "
		     "to encounter data type %s (column name %s)",
		     ibis::TYPESTRING[(int)buffer[j].ctype], buffer[j].cname);
	    break;}
	}
    }
} // ibis::bord::cursor::fillRow

int ibis::bord::cursor::getColumnAsByte(size_t j, char* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsByte

int ibis::bord::cursor::getColumnAsUByte(size_t j, unsigned char* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsUByte

int ibis::bord::cursor::getColumnAsShort(size_t j, int16_t* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = (* static_cast<const array_t<char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<int16_t>*>(buffer[j].cval))
		 [curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsShort

int ibis::bord::cursor::getColumnAsUShort(size_t j, uint16_t* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>(buffer[j].cval))
		 [curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsUShort

int ibis::bord::cursor::getColumnAsInt(size_t j, int32_t* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = (* static_cast<const array_t<char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = (* static_cast<const array_t<int16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = (* static_cast<const array_t<int32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsInt

int ibis::bord::cursor::getColumnAsUInt(size_t j, uint32_t* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = (* static_cast<const array_t<uint32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsUInt

int ibis::bord::cursor::getColumnAsLong(size_t j, int64_t* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = (* static_cast<const array_t<char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = (* static_cast<const array_t<int16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT: {
	*val = (* static_cast<const array_t<int32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UINT: {
	*val = (* static_cast<const array_t<uint32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	*val = (* static_cast<const array_t<int64_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsLong

int ibis::bord::cursor::getColumnAsULong(size_t j, uint64_t* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	*val = (* static_cast<const array_t<uint32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	*val = (* static_cast<const array_t<uint64_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsULong

int ibis::bord::cursor::getColumnAsFloat(size_t j, float* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = (* static_cast<const array_t<char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = (* static_cast<const array_t<int16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	*val = (* static_cast<const array_t<float>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsFloat

int ibis::bord::cursor::getColumnAsDouble(size_t j, double* val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	*val = (* static_cast<const array_t<char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	*val = (* static_cast<const array_t<unsigned char>*>
		 (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	*val = (* static_cast<const array_t<int16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	*val = (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT: {
	*val = (* static_cast<const array_t<int32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UINT: {
	*val = (* static_cast<const array_t<uint32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	*val = (* static_cast<const array_t<double>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsDouble

int ibis::bord::cursor::getColumnAsString(size_t j, std::string& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    std::ostringstream oss;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	oss << static_cast<int>((* static_cast<const array_t<char>*>
				 (buffer[j].cval))[curRow]);
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	oss << static_cast<unsigned>
	    ((* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow]);
	ierr = 0;
	break;}
    case ibis::SHORT: {
	oss << (* static_cast<const array_t<int16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	oss << (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT: {
	oss << (* static_cast<const array_t<int32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UINT: {
	oss << (* static_cast<const array_t<uint32_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::LONG: {
	oss << (* static_cast<const array_t<int64_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::ULONG: {
	oss << (* static_cast<const array_t<uint64_t>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	oss << (* static_cast<const array_t<float>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	oss << (* static_cast<const array_t<double>*>
		(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::CATEGORY:
    case ibis::TEXT: {
	const ibis::column* col = tab.mypart.getColumn(buffer[j].cname);
	if (col != 0) {
	    col->getString(static_cast<size_t>(curRow), val);
	    ierr = 0;
	}
	else {
	    ierr = -1;
	}
	break;}
    default: {
	ierr = -1;
	break;}
    }
    val = oss.str();
    return ierr;
} // ibis::bord::cursor::getColumnAsLong

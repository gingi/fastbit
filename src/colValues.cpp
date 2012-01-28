//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
///
/// Implementation of the colValues class hierarchy.
///
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include <math.h>	// sqrt
#include <vector>
#include <algorithm>
#include "bundle.h"
#include "column.h"
#include "bord.h"

//////////////////////////////////////////////////////////////////////
// functions of ibis::colValues and derived classes
ibis::colValues* ibis::colValues::create(const ibis::column* c,
					 const ibis::bitvector& hits) {
    if (c == 0) return 0;
    switch (c->type()) {
    case ibis::UBYTE:
	return new colUBytes(c, hits);
    case ibis::BYTE:
	return new colBytes(c, hits);
    case ibis::USHORT:
	return new colUShorts(c, hits);
    case ibis::SHORT:
	return new colShorts(c, hits);
    case ibis::UINT:
    case ibis::CATEGORY:
	return new colUInts(c, hits);
    case ibis::INT:
	return new colInts(c, hits);
    case ibis::ULONG:
	return new colULongs(c, hits);
    case ibis::LONG:
	return new colLongs(c, hits);
    case ibis::FLOAT:
	return new colFloats(c, hits);
    case ibis::DOUBLE:
	return new colDoubles(c, hits);
    case ibis::TEXT:
	return new colStrings(c, hits);
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colValues does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
	return 0;
    }
} // ibis::colValues::create

/// Use values stored in the storage object.
ibis::colValues* ibis::colValues::create(const ibis::column* c,
					 ibis::fileManager::storage* store,
					 const uint32_t start,
					 const uint32_t end) {
    if (c == 0) return 0;
    switch (c->type()) {
    case ibis::UBYTE:
	return new colUBytes(c, store, start, end);
    case ibis::BYTE:
	return new colBytes(c, store, start, end);
    case ibis::USHORT:
	return new colUShorts(c, store, start, end);
    case ibis::SHORT:
	return new colShorts(c, store, start, end);
    case ibis::UINT:
    case ibis::CATEGORY:
	return new colUInts(c, store, start, end);
    case ibis::INT:
	return new colInts(c, store, start, end);
    case ibis::ULONG:
	return new colULongs(c, store, start, end);
    case ibis::LONG:
	return new colLongs(c, store, start, end);
    case ibis::FLOAT:
	return new colFloats(c, store, start, end);
    case ibis::DOUBLE:
	return new colDoubles(c, store, start, end);
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colValues does not yet support type "
	    << ibis::TYPESTRING[(int)(c->type())];
	return 0;
    }
} // ibis::colValues::create

/// Use all rows of the column.
ibis::colValues* ibis::colValues::create(const ibis::column* c) {
    if (c == 0) return 0;
    switch (c->type()) {
    case ibis::UBYTE:
	return new colUBytes(c);
    case ibis::BYTE:
	return new colBytes(c);
    case ibis::USHORT:
	return new colUShorts(c);
    case ibis::SHORT:
	return new colShorts(c);
    case ibis::UINT:
    case ibis::CATEGORY:
	return new colUInts(c);
    case ibis::INT:
	return new colInts(c);
    case ibis::ULONG:
	return new colULongs(c);
    case ibis::LONG:
	return new colLongs(c);
    case ibis::FLOAT:
	return new colFloats(c);
    case ibis::DOUBLE:
	return new colDoubles(c);
    case ibis::TEXT:
	return new colStrings(c);
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colValues does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
	return 0;
    }
} // ibis::colValues::create

ibis::colBytes::colBytes(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<signed char>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UBYTE:
    case ibis::BYTE: {
	c->getValuesArray(array);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colBytes does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colBytes::colBytes

ibis::colUBytes::colUBytes(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<unsigned char>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UBYTE:
    case ibis::BYTE: {
	c->getValuesArray(array);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colUBytes does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colUBytes::colUBytes

ibis::colShorts::colShorts(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<int16_t>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT:
    case ibis::SHORT: {
	c->getValuesArray(array);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colShorts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colShorts::colShorts

ibis::colUShorts::colUShorts(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<uint16_t>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT:
    case ibis::SHORT: {
	c->getValuesArray(array);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colUShorts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colUShorts::colUShorts

ibis::colInts::colInts(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<int32_t>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	array_t<uint32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::INT: {
	c->getValuesArray(array);
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::SHORT: {
	array_t<int16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<int32_t>(arr[i]);
	break;}
    case ibis::LONG: {
	array_t<int64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<int32_t>(arr[i]);
	break;}
    case ibis::FLOAT: {
	array_t<float> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<int32_t>(arr[i]);
	break;}
    case ibis::DOUBLE: {
	array_t<double> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<int32_t>(arr[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colInts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colInts::colInts

ibis::colUInts::colUInts(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<uint32_t>), dic(0) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::CATEGORY: { // store the integers instead of strings
	delete array;
	ibis::bitvector hits;
	hits.set(1, c->partition()->nRows());
	array = c->selectUInts(hits);
	dic = static_cast<const ibis::category*>(c)->getDictionary();
    }
    case ibis::UINT: {
	c->getValuesArray(array);
	// check to see if the column actually carries a dictionary already
	const ibis::bord::column* bc =
	    dynamic_cast<const ibis::bord::column*>(c);
	if (bc != 0)
	    dic = bc->getDictionary();
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::INT: {
	array_t<int32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::SHORT: {
	array_t<int16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>(arr[i]);
	break;}
    case ibis::LONG: {
	array_t<int64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>(arr[i]);
	break;}
    case ibis::FLOAT: {
	array_t<float> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>(arr[i]);
	break;}
    case ibis::DOUBLE: {
	array_t<double> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>(arr[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colUInts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colUInts::colUInts

ibis::colLongs::colLongs(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<int64_t>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	array_t<uint32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::INT: {
	array_t<int32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::SHORT: {
	array_t<int16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::LONG: {
	c->getValuesArray(array);
	break;}
    case ibis::FLOAT: {
	array_t<float> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<int64_t>(arr[i]);
	break;}
    case ibis::DOUBLE: {
	array_t<double> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<int64_t>(arr[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colLongs does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colLongs::colLongs

ibis::colULongs::colULongs(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<uint64_t>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	array_t<uint32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::INT: {
	array_t<int32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::SHORT: {
	array_t<int16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::ULONG: {
	c->getValuesArray(array);
	break;}
    case ibis::LONG: {
	array_t<int64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::FLOAT: {
	array_t<float> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<uint64_t>(arr[i]);
	break;}
    case ibis::DOUBLE: {
	array_t<double> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<uint64_t>(arr[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colULongs does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colULongs::colULongs

ibis::colFloats::colFloats(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<float>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	array_t<uint32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<float>(arr[i]);
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::INT: {
	array_t<int32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<float>(arr[i]);
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::SHORT: {
	array_t<int16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<float>(arr[i]);
	break;}
    case ibis::LONG: {
	array_t<int64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<float>(arr[i]);
	break;}
    case ibis::FLOAT: {
	c->getValuesArray(array);
	break;}
    case ibis::DOUBLE: {
	array_t<double> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colFloats does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colFloats::colFloats

ibis::colDoubles::colDoubles(const ibis::column* c)
    : colValues(c), array(new ibis::array_t<double>) {
    if (c == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	array_t<uint32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::INT: {
	array_t<int32_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::BYTE: {
	array_t<signed char> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::SHORT: {
	array_t<int16_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<double>(arr[i]);
	break;}
    case ibis::LONG: {
	array_t<int64_t> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = static_cast<double>(arr[i]);
	break;}
    case ibis::FLOAT: {
	array_t<float> arr;
	try {
	    c->getValuesArray(&arr);
	    array->resize(arr.size());
	}
	catch (...) {
	    delete array;
	    throw;
	}
	for (uint32_t i = 0; i < arr.size(); ++ i)
	    (*array)[i] = arr[i];
	break;}
    case ibis::DOUBLE: {
	c->getValuesArray(array);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colDoubles does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colDoubles::colDoubles

/// Construct ibis::colStrings from an existing list of strings.
ibis::colStrings::colStrings(const ibis::column* c)
    : colValues(c), array(0) {
    if (c == 0) return;
    if (c->type() == ibis::CATEGORY) {
	ibis::bitvector hits;
	hits.set(1, c->partition()->nRows());
	array = c->selectStrings(hits);
    }
    if (c->type() == ibis::TEXT) {
	array =new std::vector<std::string>;
	c->getValuesArray(array);
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- colStrings does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colStrings::colStrings

void ibis::colInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		int32_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    int32_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    int32_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	int32_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		int32_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    int32_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colInts::sort

void ibis::colInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			 ibis::colList::iterator head,
			 ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		int32_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    int32_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    int32_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	int32_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		int32_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    int32_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colInts::sort

void ibis::colInts::sort(uint32_t i, uint32_t j,
			 array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colInts::sort

void ibis::colUInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		uint32_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    uint32_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    uint32_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	uint32_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		uint32_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    uint32_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colUInts::sort

void ibis::colUInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			  ibis::colList::iterator head,
			  ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		uint32_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    uint32_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    uint32_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	uint32_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		uint32_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    uint32_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colUInts::sort

void ibis::colUInts::sort(uint32_t i, uint32_t j,
			  array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colUInts::sort

void ibis::colLongs::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		int64_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    int64_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    int64_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	int64_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		int64_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    int64_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colLongs::sort

void ibis::colLongs::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			  ibis::colList::iterator head,
			  ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		int64_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    int64_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    int64_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	int64_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		int64_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    int64_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colLongs::sort

void ibis::colLongs::sort(uint32_t i, uint32_t j,
			  array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colLongs::sort

void ibis::colULongs::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		uint64_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    uint64_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    uint64_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	uint64_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		uint64_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    uint64_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colULongs::sort

void ibis::colULongs::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			   ibis::colList::iterator head,
			   ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		uint64_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    uint64_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    uint64_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	uint64_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		uint64_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    uint64_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colULongs::sort

void ibis::colULongs::sort(uint32_t i, uint32_t j,
			   array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colULongs::sort

void ibis::colShorts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		int16_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    int16_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    int16_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	int16_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		int16_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    int16_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colShorts::sort

void ibis::colShorts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			   ibis::colList::iterator head,
			   ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		int16_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    int16_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    int16_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	int16_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		int16_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    int16_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colShorts::sort

void ibis::colShorts::sort(uint32_t i, uint32_t j,
			   array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colShorts::sort

void ibis::colUShorts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		uint16_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    uint16_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    uint16_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	uint16_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		uint16_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    uint16_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colUShorts::sort

void ibis::colUShorts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			    ibis::colList::iterator head,
			    ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		uint16_t tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    uint16_t tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    uint16_t tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	uint16_t sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		uint16_t tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    uint16_t tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colUShorts::sort

void ibis::colUShorts::sort(uint32_t i, uint32_t j,
			    array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colUShorts::sort

void ibis::colBytes::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		signed char tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    signed char tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    signed char tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	signed char sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		signed char tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    signed char tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colBytes::sort

void ibis::colBytes::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			  ibis::colList::iterator head,
			  ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		signed char tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    signed char tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    signed char tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	signed char sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		signed char tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    signed char tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colBytes::sort

void ibis::colBytes::sort(uint32_t i, uint32_t j,
			  array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colBytes::sort

void ibis::colUBytes::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		unsigned char tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    unsigned char tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    unsigned char tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	unsigned char sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		unsigned char tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    unsigned char tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colUBytes::sort

void ibis::colUBytes::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			   ibis::colList::iterator head,
			   ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		unsigned char tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    unsigned char tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    unsigned char tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	unsigned char sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		unsigned char tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    unsigned char tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colUBytes::sort

void ibis::colUBytes::sort(uint32_t i, uint32_t j,
			   array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colUBytes::sort

void ibis::colFloats::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		float tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    float tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    float tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	float sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		float tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    float tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colFloats::sort

void ibis::colFloats::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			   ibis::colList::iterator head,
			   ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		float tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    float tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    float tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	float sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		float tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    float tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colFloats::sort

void ibis::colFloats::sort(uint32_t i, uint32_t j,
			   array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colFloats::sort

void ibis::colDoubles::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		double tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    double tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    double tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	double sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		double tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    double tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
} // colDoubles::sort

void ibis::colDoubles::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			    ibis::colList::iterator head,
			    ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		double tmp = (*array)[i1];
		(*array)[i1] = (*array)[imin];
		(*array)[imin] = tmp;
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i] > (*array)[i1]) {
	    double tmp = (*array)[i];
	    (*array)[i] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1] > (*array)[i2]) {
	    double tmp = (*array)[i2];
	    (*array)[i2] = (*array)[i1];
	    (*array)[i1] = tmp;
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i] > (*array)[i1]) {
		tmp = (*array)[i];
		(*array)[i] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	double sep = (*array)[i1]; // sep the median of the three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    if ((*array)[i1] < sep && (*array)[i2] >= sep) {
		// both i1 and i2 are in the right places
		++i1; --i2;
	    }
	    else if ((*array)[i1] < sep) {
		// i1 is in the right place
		++i1;
	    }
	    else if ((*array)[i2] >= sep) {
		// i2 is in the right place
		--i2;
	    }
	    else { // both are in the wrong places, swap them
		double tmp = (*array)[i2];
		(*array)[i2] = (*array)[i1];
		(*array)[i1] = tmp;
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++i1; --i2;
	    }
	}
	i1 += ((*array)[i1] < sep);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    double tmp = (*array)[i1];
	    (*array)[i1] = (*array)[i2];
	    (*array)[i2] = tmp;
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1] == (*array)[i2])
		++i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
} // colDoubles::sort

void ibis::colDoubles::sort(uint32_t i, uint32_t j,
			    array_t<uint32_t>& ind) const {
    if (i < j) {
	ind.clear();
	ind.reserve(j-i);
	for (uint32_t k = i; k < j; ++ k)
	    ind.push_back(k);
	array->sort(ind);
    }
} // ibis::colDoubles::sort

void ibis::colStrings::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    const uint32_t istart = i;
    const uint32_t jend = j;
#endif
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2].compare((*array)[imin]) < 0)
		    imin = i2;
	    }
	    if (imin > i1) {
		(*array)[i1].swap((*array)[imin]);
		if (bdl) bdl->swapRIDs(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i].compare((*array)[i1]) > 0) {
	    (*array)[i].swap((*array)[i1]);
	    if (bdl) bdl->swapRIDs(i, i1);
	}
	if ((*array)[i1].compare((*array)[i2]) > 0) {
	    (*array)[i2].swap((*array)[i1]);
	    if (bdl) bdl->swapRIDs(i2, i1);
	    if ((*array)[i].compare((*array)[i1]) > 0) {
		(*array)[i].swap((*array)[i1]);
		if (bdl) bdl->swapRIDs(i, i1);
	    }
	}
	const std::string sep = (*array)[i1]; // sep is the median of three
	i1 = i;
	i2 = j - 1;
	bool stayleft  = ((*array)[i1].compare(sep) < 0);
	bool stayright = ((*array)[i2].compare(sep) >= 0);
	while (i1 < i2) {
	    if (stayleft || stayright) {
		if (stayleft) {
		    ++ i1;
		    stayleft  = ((*array)[i1].compare(sep) < 0);
		}
		if (stayright) {
		    -- i2;
		    stayright = ((*array)[i2].compare(sep) >= 0);
		}
	    }
	    else { // both are in the wrong places, swap them
		(*array)[i2].swap((*array)[i1]);
		if (bdl) bdl->swapRIDs(i2, i1);
		++ i1; -- i2;
		stayleft  = ((*array)[i1].compare(sep) < 0);
		stayright = ((*array)[i2].compare(sep) >= 0);
	    }
	}
	i1 += (int)stayleft;
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl);
	    sort(i1, j, bdl);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    (*array)[i1].swap((*array)[i2]);
	    if (bdl) bdl->swapRIDs(i1, i2);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1].compare((*array)[i2]) == 0)
		++ i2;
	    if (i2 < j)
		sort(i2, j, bdl);
	}
    } // end quick sort
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg() << "DEBUG -- colStrings[" << col->partition()->name() << '.'
	     << col->name() << "]::sort exiting with the following:";
	for (uint32_t ii = istart; ii < jend; ++ ii)
	    lg() << "\narray[" << ii << "] = " << (*array)[ii];
    }
#endif
} // colStrings::sort

void ibis::colStrings::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			    ibis::colList::iterator head,
			    ibis::colList::iterator tail) {
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    const uint32_t istart = i;
    const uint32_t jend = j;
#endif
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
	    }
	    if (imin > i1) {
		(*array)[i1].swap((*array)[imin]);
		if (bdl) bdl->swapRIDs(i1, imin);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i1, imin);
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[i].compare((*array)[i1]) > 0) {
	    (*array)[i].swap((*array)[i1]);
	    if (bdl) bdl->swapRIDs(i, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i, i1);
	}
	if ((*array)[i1].compare((*array)[i2]) > 0){
	    (*array)[i2].swap((*array)[i1]);
	    if (bdl) bdl->swapRIDs(i2, i1);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);
	    if ((*array)[i].compare((*array)[i1]) > 0) {
		(*array)[i].swap((*array)[i1]);
		if (bdl) bdl->swapRIDs(i, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i, i1);
	    }
	}
	const std::string sep = (*array)[i1]; // sep is the median of three
	i1 = i;
	i2 = j - 1;
	bool stayleft  = (sep.compare((*array)[i1]) > 0);
	bool stayright = (sep.compare((*array)[i2]) <= 0);
	while (i1 < i2) {
	    if (stayleft || stayright) { // at least one is in the right place
		if (stayleft) {
		    ++ i1;
		    stayleft = (sep.compare((*array)[i1]) > 0);
		}
		if (stayright) {
		    -- i2;
		    stayright = (sep.compare((*array)[i2]) <= 0);
		}
	    }
	    else { // both are in the wrong places, swap them
		(*array)[i2].swap((*array)[i1]);
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++ i1; -- i2;
		stayleft  = (sep.compare((*array)[i1]) > 0);
		stayright = (sep.compare((*array)[i2]) <= 0);
	    }
	}
	i1 += (int)stayleft;
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sort(i, i1, bdl, head, tail);
	    sort(i1, j, bdl, head, tail);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    (*array)[i1].swap((*array)[i2]);
	    if (bdl) bdl->swapRIDs(i1, i2);
	    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		(*ii)->swap(i2, i1);

	    // collect all elements equal to (*array)[i1]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[i1].compare((*array)[i2]) == 0)
		++ i2;
	    if (i2 < j)
		sort(i2, j, bdl, head, tail);
	}
    } // end quick sort
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    if (ibis::gVerbose > 5) {
	ibis::util::logger lg;
	lg() << "DEBUG -- colStrings[" << col->partition()->name() << '.'
	     << col->name() << "]::sort exiting with the following:";
	for (uint32_t ii = istart; ii < jend; ++ ii)
	    lg() << "\narray[" << ii << "] = " << (*array)[ii];
    }
#endif
} // colStrings::sort

void ibis::colStrings::sort(uint32_t i, uint32_t j,
			    array_t<uint32_t>& ind) const {
    if (i >= j)
	return;
    ind.clear();
    ind.reserve(j-i);
    for (uint32_t k = i; k < j; ++ k)
	ind.push_back(k);
    sortsub(0, j-i, ind);
} // ibis::colStrings::sort

/// Sort a subset of values specified by the index array ind.  Upon
/// completion of this operation, (*array)[ind[i:j)] will be in
/// non-descending order.
void ibis::colStrings::sortsub(uint32_t i, uint32_t j,
			       array_t<uint32_t>& ind) const {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2) {
		if ((*array)[ind[i2]].compare((*array)[ind[imin]]) < 0)
		    imin = i2;
	    }
	    if (imin > i1) {
		uint32_t tmp = ind[i1];
		ind[i1] = ind[imin];
		ind[imin] = tmp;
	    }
	}
    } // end selection sort
    else { // use quick sort
	// sort three rows to find the median
	uint32_t i1=(i+j)/2, i2=j-1;
	if ((*array)[ind[i]].compare((*array)[ind[i1]]) > 0) {
	    uint32_t tmp = ind[i];
	    ind[i] = ind[i1];
	    ind[i1] = tmp;
	}
	if ((*array)[ind[i1]].compare((*array)[ind[i2]]) > 0) {
	    uint32_t tmp = ind[i1];
	    ind[i1] = ind[i2];
	    ind[i2] = tmp;
	    if ((*array)[ind[i]].compare((*array)[ind[i1]]) > 0) {
		tmp = ind[i];
		ind[i] = ind[i1];
		ind[i1] = tmp;
	    }
	}
	const std::string& sep = (*array)[ind[i1]]; // the median of three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    const bool stayleft  = (sep.compare((*array)[ind[i1]]) > 0);
	    const bool stayright = (sep.compare((*array)[ind[i2]]) <= 0);
	    if (stayleft || stayright) {
		// either i1 or i2 is in the right places
		i1 += (int)stayleft;
		i2 -= (int)stayright;
	    }
	    else { // both are in the wrong places, swap them
		uint32_t tmp = ind[i2];
		ind[i2] = ind[i1];
		ind[i1] = tmp;
		++ i1; -- i2;
	    }
	}
	i1 += (int)(sep.compare((*array)[ind[i1]]) > 0);
	if (i1 > i+1) { // elements in range [i, i1) are smaller than sep
	    sortsub(i, i1, ind);
	    sortsub(i1, j, ind);
	}
	else { // elements i and (i+j)/2 must be the smallest ones
	    i1 = i + 1;
	    i2 = (i+j) / 2;
	    const uint32_t tmp = ind[i1];
	    ind[i1] = ind[i2];
	    ind[i2] = tmp;

	    // collect all elements equal to (*array)[ind[i1]]
	    i2 = i1 + 1;
	    while (i2 < j && (*array)[ind[i1]].compare((*array)[ind[i2]]) == 0)
		++ i2;
	    if (i2 < j)
		sortsub(i2, j, ind);
	}
    } // end quick sort
} // ibis::colStrings::sortsub

/// The median-of-three parition function.  Upon returning from this
/// function, the return value partitions the strings into groups where
/// strings in the first group are lexicographically less than those in the
/// second group.  If the return value places all strings into one group,
/// the string values are already sorted.
uint32_t ibis::colStrings::partitionsub(uint32_t i, uint32_t j,
					array_t<uint32_t>& ind) const {
    // sort three strings, i, (i+j)/2 and j-1, to find the median
    uint32_t i1=(i+j)/2, i2=j-1;
    if ((*array)[ind[i]].compare((*array)[ind[i1]]) > 0) {
	uint32_t tmp = ind[i];
	ind[i] = ind[i1];
	ind[i1] = tmp;
    }
    if ((*array)[ind[i1]].compare((*array)[ind[i2]]) > 0) {
	uint32_t tmp = ind[i1];
	ind[i1] = ind[i2];
	ind[i2] = tmp;
	if ((*array)[ind[i]].compare((*array)[ind[i1]]) > 0) {
	    tmp = ind[i];
	    ind[i] = ind[i1];
	    ind[i1] = tmp;
	}
    }
    const std::string& sep = (*array)[ind[i1]]; // the median of three
    i1 = i;
    i2 = j - 1;
    while (i1 < i2) {
	const bool stayleft  = (sep.compare((*array)[ind[i1]]) > 0);
	const bool stayright = (sep.compare((*array)[ind[i2]]) <= 0);
	if (stayleft || stayright) {
	    // either i1 or i2 is in the right places
	    i1 += (int)stayleft;
	    i2 -= (int)stayright;
	}
	else { // both are in the wrong places, swap them
	    uint32_t tmp = ind[i2];
	    ind[i2] = ind[i1];
	    ind[i1] = tmp;
	    ++ i1; -- i2;
	}
    }
    i1 += (int)(sep.compare((*array)[ind[i1]]) > 0);
    if (i1 <= i+1) { // elements i and (i+j)/2 must be the smallest ones
	i1 = i + 1;
	i2 = (i+j) / 2;
	const uint32_t tmp = ind[i1];
	ind[i1] = ind[i2];
	ind[i2] = tmp;

	// collect all elements equal to (*array)[ind[i]]
	for (++ i1; i1 < j && (*array)[ind[i1]].compare((*array)[ind[i]]) == 0;
	     ++ i1);
    }
    return i1;
} // ibis::colStrings::partitionsub

// mark the start positions of the segments with identical values
ibis::array_t<uint32_t>*
ibis::colInts::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    int32_t target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colInts::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colInts::segment

ibis::array_t<uint32_t>*
ibis::colUInts::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    uint32_t target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colUInts::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colUInts::segment

// mark the start positions of the segments with identical values
ibis::array_t<uint32_t>*
ibis::colLongs::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    int64_t target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colLongs::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colLongs::segment

ibis::array_t<uint32_t>*
ibis::colULongs::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    uint64_t target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colULongs::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colULongs::segment

// mark the start positions of the segments with identical values
ibis::array_t<uint32_t>*
ibis::colShorts::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    int16_t target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colShorts::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colShorts::segment

ibis::array_t<uint32_t>*
ibis::colUShorts::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    uint16_t target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colUShorts::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colUShorts::segment

// mark the start positions of the segments with identical values
ibis::array_t<uint32_t>*
ibis::colBytes::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    signed char target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colBytes::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colBytes::segment

ibis::array_t<uint32_t>*
ibis::colUBytes::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    unsigned char target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colUBytes::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colUBytes::segment

/// Mark the start positions of the segments with identical values.
ibis::array_t<uint32_t>*
ibis::colFloats::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    float target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colFloats::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colFloats::segment

// mark the start positions of the segments with identical values
ibis::array_t<uint32_t>*
ibis::colDoubles::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    double target;
    const uint32_t nelm = array->size();
    const uint32_t nold = (old != 0 ? old->size() : 0);

    if (nold > 2) {
	// find segments within the previously defined segments
	for (uint32_t i = 0; i < nold-1; ++ i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm)
		    target = (*array)[j];
	    }
	}
    }
    else { // start with all elements in one segment
	j = 1;
	res->push_back(0); // the first number is always 0
	target = *(array->begin());
	while (j < nelm) {
	    while (j < nelm && (*array)[j] == target)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = (*array)[j];
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colDoubles::segment: old groups "
	 << (old != 0 ? nold-1 : 0) << ", new groups "
	 << res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colDoubles::segment

// mark the start positions of the segments with identical values
ibis::array_t<uint32_t>*
ibis::colStrings::segment(const array_t<uint32_t>* old) const {
    ibis::array_t<uint32_t>* res = new ibis::array_t<uint32_t>;
    uint32_t j;
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments within the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    j = (*old)[i];
	    if (i == 0 || res->back() < j)
		res->push_back(j);
	    uint32_t target = j;
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] &&
		       (*array)[target].compare((*array)[j]) == 0)
		    ++ j;
		res->push_back(j);
		if (j < (*old)[i+1])
		    target = j;
	    }
	}
    }
    else { // start with all elements in one segment
	uint32_t target = 0;
	res->push_back(0); // the first number is always 0
	j = 1;
	while (j < nelm) {
	    while (j < nelm &&
		   (*array)[target].compare((*array)[j]) == 0)
		++ j;
	    res->push_back(j);
	    if (j < nelm) {
		target = j;
		++ j;
	    }
	}
    }
    if (res->back() < nelm)
	res->push_back(nelm);
#if _DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg() << "DEBUG -- colStrings::segment: old groups "
	 << (old != 0 ? old->size()-1 : 0) << ", new groups "
	 << res->size()-1;
    if (old->size() > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colStrings::segment

/// Remove the duplicate elements according to the array starts
void ibis::colInts::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<int32_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colInts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colUInts::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<uint32_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colUInts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colLongs::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<int64_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colLongs::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colULongs::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<uint64_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colULongs::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colShorts::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<int16_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colShorts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colUShorts::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<uint16_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colUShorts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colBytes::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<signed char> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colBytes::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colUBytes::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<unsigned char> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colUBytes::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colFloats::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<float> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colFloats::reduce

/// remove the duplicate elements according to the array starts
void ibis::colDoubles::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<double> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colDoubles::reduce

/// remove the duplicate elements according to the array starts
void ibis::colStrings::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	if (starts[i] > i)
	    (*array)[i].swap((*array)[starts[i]]);
    array->resize(nseg);
} // ibis::colStrings::reduce

/// remove the duplicate elements according to the array starts
void ibis::colInts::reduce(const array_t<uint32_t>& starts,
			   ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<int>(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		int32_t lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<int32_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colInts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colUInts::reduce(const array_t<uint32_t>& starts,
			    ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<unsigned>
		    (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		uint32_t lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<uint32_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colUInts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colLongs::reduce(const array_t<uint32_t>& starts,
			    ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = static_cast<double>((*array)[starts[i]]);
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<int>(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		int64_t lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<int64_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colLongs::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colULongs::reduce(const array_t<uint32_t>& starts,
			     ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = static_cast<double>((*array)[starts[i]]);
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<unsigned>
		    (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		uint64_t lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<uint64_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colULongs::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colShorts::reduce(const array_t<uint32_t>& starts,
			     ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = static_cast<double>((*array)[starts[i]]);
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<int>(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		int16_t lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<int16_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colShorts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colUShorts::reduce(const array_t<uint32_t>& starts,
			      ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = static_cast<double>((*array)[starts[i]]);
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<unsigned>
		    (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		uint16_t lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<uint16_t> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colUShorts::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colBytes::reduce(const array_t<uint32_t>& starts,
			    ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = static_cast<double>((*array)[starts[i]]);
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<int>(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<int>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		signed char lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<signed char> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colBytes::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colUBytes::reduce(const array_t<uint32_t>& starts,
			     ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = static_cast<double>((*array)[starts[i]]);
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<unsigned>
		    (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		unsigned char lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<unsigned char> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colUBytes::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colFloats::reduce(const array_t<uint32_t>& starts,
			     ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first few value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++ i) {
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    sum += (*array)[j];
		(*array)[i] = static_cast<float>
		    (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = static_cast<float>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<float>
			(sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = ((*array)[starts[i]]-avg)
			*((*array)[starts[i]]-avg)/count;
		}
		else {
		    (*array)[i] = static_cast<float>
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		float lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<float> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colFloats::reduce

/// Remove the duplicate elements according to the array starts
void ibis::colDoubles::reduce(const array_t<uint32_t>& starts,
			      ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selectClause::NIL_AGGR:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    (*array)[i] = starts[i+1] - starts[i];
	}
	break;
    case ibis::selectClause::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
	    if (starts[i+1] > starts[i]+1) {
		(*array)[i] = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    (*array)[i] += (*array)[j];
		(*array)[i] /= (starts[i+1]-starts[i]);
	    }
	    else {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    case ibis::selectClause::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::VARPOP:
    case ibis::selectClause::VARSAMP:
    case ibis::selectClause::STDPOP:
    case ibis::selectClause::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stdev can be derived from
    	// variance 
	// - population standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows.
	// - sample standard variance =  sum of squared differences from
    	//   mean/avg, divided by number of rows-1.
	// - population standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows.
	// - sample standard deviation = square root of sum of squared
    	//   differences from mean/avg, divided by number of rows-1.
	double avg;
        uint32_t count;
	for (uint32_t i = 0; i < nseg; ++i) {
            count = 1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++ count;
                }
		avg = (sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg = (*array)[starts[i]];
	    }


            if (((func == ibis::selectClause::VARSAMP) ||
		 (func == ibis::selectClause::STDSAMP)) && count > 1) {
		-- count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = variance/count;
		}
		else {
		    (*array)[i] = (sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selectClause::VARPOP ||
		    func == ibis::selectClause::VARSAMP) {
		    (*array)[i] = ((*array)[starts[i]]-avg)
			*((*array)[starts[i]]-avg)/count;
		}
		else {
		    (*array)[i] =
			(sqrt(((*array)[starts[i]]-avg)
			      *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		double lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}
		(*array)[i] = static_cast<int> (distinct);
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = 1;
		}
		else {
		    (*array)[i] = 2;
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = 1;
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		if (nv % 2 == 1) {
		    (*array)[i] = (*array)[starts[i] + nv/2];
		}
		else {
		    (*array)[i] = ((*array)[starts[i] + nv/2 - 1]
				   + (*array)[starts[i] + nv/2]) / 2;
		}
	    }
	    else if (nv == 2) {
		(*array)[i] = ((*array)[starts[i]]
			       + (*array)[starts[i] + 1]) / 2;
	    }
	    else if (nv == 1 && starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	ibis::array_t<double> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colDoubles::reduce

void ibis::colStrings::reduce(const array_t<uint32_t>& starts,
			      ibis::selectClause::AGREGADO func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default:
	LOGGER(ibis::gVerbose >= 0 && col != 0)
	    << "Warning -- colStrings::reduce can NOT apply aggregate "
	    << (int)func << " on column " << col->name() << " (type "
	    << ibis::TYPESTRING[(int)(col->type())] << ")";
	break;
    case ibis::selectClause::NIL_AGGR: // only save the first value
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selectClause::CNT: // count
	for (uint32_t i = 0; i < nseg; ++ i) {
	    std::ostringstream oss;
	    oss << starts[i+1] - starts[i];
	    (*array)[i] = oss.str();
	}
	break;
    case ibis::selectClause::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selectClause::DISTINCT: // count distinct values
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) {
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);

		std::string lastVal = (*array)[starts[i]];
		uint32_t distinct = 1;

		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    if ((*array)[j] != lastVal) {
			lastVal = (*array)[j];
			++ distinct;
		    }
		}

		std::ostringstream oss;
		oss << distinct;
		(*array)[i] = oss.str();
	    }
	    else if (nv == 2) {
		if ((*array)[starts[i]] == (*array)[starts[i]+1]) {
		    (*array)[i] = "1";
		}
		else {
		    (*array)[i] = "2";
		}
	    }
	    else if (nv == 1) {
		(*array)[i] = "1";
	    }
	}
	break;
    case ibis::selectClause::MEDIAN: // compute median
	for (uint32_t i = 0; i < nseg; ++i) {
	    const uint32_t nv = starts[i+1] - starts[i];
	    if (nv > 2) { // general case, require sorting
		std::sort(array->begin()+starts[i], array->begin()+starts[i+1]);
		(*array)[i] = (*array)[starts[i] + nv/2];
	    }
	    else if (starts[i] > i) {
		(*array)[i] = (*array)[starts[i]];
	    }
	}
	break;
    }
    array->resize(nseg);
    if (array->capacity() > 1000 && array->capacity() > nseg+nseg) {
	// replace the storage object with a smaller one
	std::vector<std::string> tmp(nseg);
	std::copy(array->begin(), array->end(), tmp.begin());
	array->swap(tmp);
    }
} // ibis::colStrings::reduce

double ibis::colInts::getMin() const {
    const uint32_t nelm = array->size();
    int32_t ret = 0x7FFFFFFF;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colInts::getMin

double ibis::colInts::getMax() const {
    const uint32_t nelm = array->size();
    int32_t ret = 0x80000000;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colInts::getMax

double ibis::colInts::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colInts::getSum

double ibis::colUInts::getMin() const {
    const uint32_t nelm = array->size();
    uint32_t ret = 0xFFFFFFFFU;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colUInts::getMin

double ibis::colUInts::getMax() const {
    const uint32_t nelm = array->size();
    uint32_t ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colUInts::getMax

double ibis::colUInts::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colUInts::getSum

double ibis::colLongs::getMin() const {
    const uint32_t nelm = array->size();
    int64_t ret = 0x7FFFFFFFFFFFFFFFLL;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colLongs::getMin

double ibis::colLongs::getMax() const {
    const uint32_t nelm = array->size();
    int64_t ret = 0x8000000000000000LL;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colLongs::getMax

double ibis::colLongs::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colLongs::getSum

double ibis::colULongs::getMin() const {
    const uint32_t nelm = array->size();
    uint64_t ret = 0xFFFFFFFFFFFFFFFFULL;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colULongs::getMin

double ibis::colULongs::getMax() const {
    const uint32_t nelm = array->size();
    uint64_t ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colULongs::getMax

double ibis::colULongs::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colULongs::getSum

double ibis::colShorts::getMin() const {
    const uint32_t nelm = array->size();
    int16_t ret = 0x7FFF;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colShorts::getMin

double ibis::colShorts::getMax() const {
    const uint32_t nelm = array->size();
    int16_t ret = 0x8000;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colShorts::getMax

double ibis::colShorts::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colShorts::getSum

double ibis::colUShorts::getMin() const {
    const uint32_t nelm = array->size();
    uint16_t ret = 0xFFFFU;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colUShorts::getMin

double ibis::colUShorts::getMax() const {
    const uint32_t nelm = array->size();
    uint16_t ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colUShorts::getMax

double ibis::colUShorts::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colUShorts::getSum

double ibis::colBytes::getMin() const {
    const uint32_t nelm = array->size();
    signed char ret = 0x7F;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colBytes::getMin

double ibis::colBytes::getMax() const {
    const uint32_t nelm = array->size();
    signed char ret = 0x80;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colBytes::getMax

double ibis::colBytes::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colBytes::getSum

double ibis::colUBytes::getMin() const {
    const uint32_t nelm = array->size();
    unsigned char ret = 0xFFU;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colUBytes::getMin

double ibis::colUBytes::getMax() const {
    const uint32_t nelm = array->size();
    unsigned char ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return static_cast<double>(ret);
} // ibis::colUBytes::getMax

double ibis::colUBytes::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colUBytes::getSum

double ibis::colFloats::getMin() const {
    const uint32_t nelm = array->size();
    float ret = FLT_MAX;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colFloats::getMin

double ibis::colFloats::getMax() const {
    const uint32_t nelm = array->size();
    float ret = -FLT_MAX;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colFloats::getMax

double ibis::colFloats::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colFloats::getSum

double ibis::colDoubles::getMin() const {
    const uint32_t nelm = array->size();
    double ret = DBL_MAX;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret > (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colDoubles::getMin

double ibis::colDoubles::getMax() const {
    const uint32_t nelm = array->size();
    double ret = -DBL_MAX;
    for (uint32_t i = 0; i < nelm; ++ i)
	if (ret < (*array)[i])
	    ret = (*array)[i];
    return ret;
} // ibis::colDoubles::getMax

double ibis::colDoubles::getSum() const {
    const uint32_t nelm = array->size();
    double ret = 0;
    for (uint32_t i = 0; i < nelm; ++ i)
	ret += (*array)[i];
    return ret;
} // ibis::colDoubles::getSum

uint32_t ibis::colStrings::write(FILE* fptr) const {
    if (array == 0 || col == 0)
	return 0;

    uint32_t cnt = 0;
    const uint32_t nelm = array->size();
    for (uint32_t i = 0; i < nelm; ++ i) {
	int ierr = fwrite((*array)[i].c_str(), sizeof(char),
			  (*array)[i].size()+1, fptr);
	cnt += (int) (ierr > long((*array)[i].size()));
	LOGGER(ierr <= 0 && ibis::gVerbose >= 0)
	    << "Warning -- colStrings[" << col->partition()->name() << '.'
	    << col->name() << "]::write failed to string " << (*array)[i]
	    << "(# " << i << " out of " << array->size()
	    << ") to file, ierr = " << ierr;
    }
    return cnt;
} // ibis::colStrings::write

void ibis::colStrings::reorder(const array_t<uint32_t> &ind) {
    if (array == 0 || col == 0 || ind.size() > array->size())
	return;

    std::vector<std::string> tmp(array->size());
    for (uint32_t i = 0; i < ind.size(); ++ i)
	tmp[i].swap((*array)[ind[i]]);
    tmp.swap(*array);
} // ibis::colStrings::reorder

/// Fill the array ind with positions of the k largest elements.  The array
/// may contain more than k elements, if the kth largest element is not
/// unique.  The array may contain less than k elements if this object
/// contains less than k elements.  The array ind contains the largest
/// element in ascending order with the index to the largest string at the
/// end.
void ibis::colStrings::topk(uint32_t k, array_t<uint32_t> &ind) const {
    ind.clear();
    if (col == 0 || array == 0)
	return;
    if (k >= array->size()) {
	k = array->size();
	sort(0, k, ind);
	return;
    }

    uint32_t front = 0;
    uint32_t back = array->size();
    ind.resize(back);
    for (uint32_t i = 0; i < back; ++ i)
	ind[i] = i;

    const uint32_t mark = back - k;
    while (back > front + 32 && back > mark) {
	uint32_t p = partitionsub(front, back, ind);
	if (p >= mark) {
	    sortsub(p, back, ind);
	    back = p;
	}
	else {
	    front = p;
	}
    }
    if (back > mark)
	sortsub(front, back, ind);
    // find the first value before [mark] that quals to it
    for (back = mark;
	 back > 0 && (*array)[mark].compare((*array)[back-1]) == 0;
	 -- back);
    if (back > 0) { // move [back:..] to the front of ind
	for (front = 0; back < array->size(); ++ front, ++ back)
	    ind[front] = ind[back];
	ind.resize(front);
    }
#if _DEBUG+0>2 || DEBUG+0>1
    ibis::util::logger lg(4);
    lg() << "colStrings::topk(" << k << ")\n";
    for (uint32_t i = 0; i < back; ++i)
	lg() << ind[i] << "\t" << (*array)[ind[i]] << "\n";
    std::flush(lg());
#endif
} // ibis::colStrings::topk

/// Find positions of the k smallest strings.
void ibis::colStrings::bottomk(uint32_t k, array_t<uint32_t> &ind) const {
    ind.clear();
    if (col == 0 || array == 0)
	return;
    if (k >= array->size()) {
	k = array->size();
	sort(0, k, ind);
	return;
    }

    uint32_t front = 0;
    uint32_t back = array->size();
    ind.resize(back);
    for (uint32_t i = 0; i < back; ++ i)
	ind[i] = i;

    while (back > front + 32 && back > k) {
	uint32_t p = partitionsub(front, back, ind);
	if (p <= k) {
	    sortsub(front, p, ind);
	    front = p;
	}
	else {
	    back = p;
	}
    }
    if (front < k)
	sortsub(front, back, ind);
    // find the last value after [k-1] that quals to it
    for (back = k;
	 back < array->size() && (*array)[k-1].compare((*array)[back]) == 0;
	 ++ back);
    ind.resize(back);
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    ibis::util::logger lg(4);
    lg() << "colStrings::bottomk(" << k << ")\n";
    for (uint32_t i = 0; i < back; ++i)
	lg() << ind[i] << "\t" << (*array)[ind[i]] << "\n";
    std::flush(lg());
#endif
} // ibis::colStrings::bottomk

long ibis::colInts::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colInts::truncate

long ibis::colUInts::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colUInts::truncate

long ibis::colLongs::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colLongs::truncate

long ibis::colULongs::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colULongs::truncate

long ibis::colShorts::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colShorts::truncate

long ibis::colUShorts::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colUShorts::truncate

long ibis::colBytes::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colBytes::truncate

long ibis::colUBytes::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colUBytes::truncate

long ibis::colFloats::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colFloats::truncate

long ibis::colDoubles::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->nosharing();
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colDoubles::truncate

long ibis::colStrings::truncate(uint32_t keep) {
    if (array == 0) return -1;
    if (array->size() > keep) {
	array->resize(keep);
	return keep;
    }
    else {
	return array->size();
    }
} // ibis::colStrings::truncate

long ibis::colInts::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colInts::truncate

long ibis::colUInts::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colUInts::truncate

long ibis::colLongs::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colLongs::truncate

long ibis::colULongs::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colULongs::truncate

long ibis::colShorts::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colShorts::truncate

long ibis::colUShorts::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colUShorts::truncate

long ibis::colBytes::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colBytes::truncate

long ibis::colUBytes::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colUBytes::truncate

long ibis::colFloats::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colFloats::truncate

long ibis::colDoubles::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    array->truncate(keep, start);
    return array->size();
} // ibis::colDoubles::truncate

long ibis::colStrings::truncate(uint32_t keep, uint32_t start) {
    if (array == 0) return -1;
    if (start == 0) {
	if (array->size() > keep) {
	    array->resize(keep);
	}
    }
    else if (start < array->size()) {
	if (keep+start > array->size())
	    keep = array->size() - start;
	for (uint32_t j = 0; j < keep; ++ j)
	    (*array)[j].swap((*array)[j+start]);
	array->resize(keep);
    }
    else {
	array->clear();
    }
    return array->size();
} // ibis::colStrings::truncate

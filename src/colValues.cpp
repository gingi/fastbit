//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
///
/// Implementatioin of the colValues class hierarchy.
///
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include <cmath>	// std::ceil, std::log, ...
#include <vector>
#include <algorithm>
#include "bundle.h"
#include "column.h"


//////////////////////////////////////////////////////////////////////
// functions of ibis::colValues and derived classes
ibis::colValues* ibis::colValues::create(const ibis::column* c,
					 const ibis::bitvector& hits) {
    if (c == 0) return 0;
    switch (c->type()) {
    case ibis::UINT:
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::CATEGORY:
	return new colUInts(c, hits);
    case ibis::INT:
    case ibis::BYTE:
    case ibis::SHORT:
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
	    << "Warning -- ibis::colValues does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
	return 0;
    }
} // ibis::colValues::create

ibis::colValues* ibis::colValues::create(const ibis::column* c,
					 ibis::fileManager::storage* store,
					 const uint32_t start,
					 const uint32_t nelm) {
    if (c == 0) return 0;
    switch (c->type()) {
    case ibis::UINT:
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::CATEGORY:
	return new colUInts(c, store, start, nelm);
    case ibis::BYTE:
    case ibis::SHORT:
    case ibis::INT:
	return new colInts(c, store, start, nelm);
    case ibis::ULONG:
	return new colULongs(c, store, start, nelm);
    case ibis::LONG:
	return new colLongs(c, store, start, nelm);
    case ibis::FLOAT:
	return new colFloats(c, store, start, nelm);
    case ibis::DOUBLE:
	return new colDoubles(c, store, start, nelm);
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colValues does not yet support type "
	    << ibis::TYPESTRING[(int)(c->type())];
	return 0;
    }
} // ibis::colValues::create

ibis::colValues* ibis::colValues::create(const ibis::column* c,
					 void* vals) {
    if (c == 0 || vals == 0) return 0;
    switch (c->type()) {
    case ibis::UINT:
    case ibis::UBYTE:
    case ibis::USHORT:
	return new colUInts(c, vals);
    case ibis::INT:
    case ibis::BYTE:
    case ibis::SHORT:
	return new colInts(c, vals);
    case ibis::ULONG:
	return new colULongs(c, vals);
    case ibis::LONG:
	return new colLongs(c, vals);
    case ibis::FLOAT:
	return new colFloats(c, vals);
    case ibis::DOUBLE:
	return new colDoubles(c, vals);
    case ibis::TEXT:
    case ibis::CATEGORY:
	return new colStrings(c, vals);
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colValues does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
	return 0;
    }
} // ibis::colValues::create

ibis::colInts::colInts(const ibis::column* c, void* vals)
    : colValues(c), array(new ibis::array_t<int32_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colInts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colInts::colInts

ibis::colUInts::colUInts(const ibis::column* c, void* vals)
    : colValues(c), array(new ibis::array_t<uint32_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colUInts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colUInts::colUInts

ibis::colLongs::colLongs(const ibis::column* c, void* vals)
    : colValues(c), array(new ibis::array_t<int64_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int64_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int64_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colLongs does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colLongs::colLongs

ibis::colULongs::colULongs(const ibis::column* c, void* vals)
    : colValues(c), array(new ibis::array_t<uint64_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint64_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint64_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colULongs does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colULongs::colULongs

ibis::colFloats::colFloats(const ibis::column* c, void* vals)
    : colValues(c), array(new ibis::array_t<float>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colFloats does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colFloats::colFloats

ibis::colDoubles::colDoubles(const ibis::column* c, void* vals)
    : colValues(c), array(new ibis::array_t<double>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<double>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<double>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colDoubles does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colDoubles::colDoubles

/// Construct ibis::colStrings from an existing list of strings.  This
/// function will take over the actual content of the strings and leave the
/// incoming vals with empty strings.  Do NOT use vals after invoking this
/// function.
ibis::colStrings::colStrings(const ibis::column* c, void* vals)
    : colValues(c), array(0) {
    if (c == 0 || vals == 0) return;
    if (c->type() == ibis::TEXT || c->type() == ibis::CATEGORY) {
	std::vector<std::string>* arr =
	    static_cast<std::vector<std::string>*>(vals);
	array = new std::vector<std::string>(arr->size());
	for (uint32_t i = 0; i < arr->size(); ++ i)
	    (*array)[i].swap((*arr)[i]);
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::colStrings does not support type "
	    << ibis::TYPESTRING[(int)(c->type())];
    }
} // ibis::colStrings::colStrings

void ibis::colInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	array_t<int32_t> tmp(*array, i, j-i);
	tmp.sort(ind);
    }
} // ibis::colInts::sort

void ibis::colUInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	array_t<uint32_t> tmp(*array, i, j-i);
	tmp.sort(ind);
    }
} // ibis::colUInts::sort

void ibis::colLongs::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	array_t<int64_t> tmp(*array, i, j-i);
	tmp.sort(ind);
    }
} // ibis::colLongs::sort

void ibis::colULongs::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	array_t<uint64_t> tmp(*array, i, j-i);
	tmp.sort(ind);
    }
} // ibis::colULongs::sort

void ibis::colFloats::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	array_t<float> tmp(*array, i, j-i);
	tmp.sort(ind);
    }
} // ibis::colFloats::sort

void ibis::colDoubles::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	array_t<double> tmp(*array, i, j-i);
	tmp.sort(ind);
    }
} // ibis::colDoubles::sort

void ibis::colStrings::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2].compare((*array)[imin]) < 0)
		    imin = i2;
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
	const std::string& sep = (*array)[i1]; // sep is the median of three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    const bool stayleft  = ((*array)[i1].compare(sep) < 0);
	    const bool stayright = ((*array)[i2].compare(sep) >= 0);
	    if (stayleft || stayright) {
		i1 += (const int)(stayleft);
		i2 -= (const int)(stayright);
	    }
	    else { // both are in the wrong places, swap them
		(*array)[i2].swap((*array)[i1]);
		if (bdl) bdl->swapRIDs(i2, i1);
		++ i1; -- i2;
	    }
	}
	i1 += (int)(sep.compare((*array)[i1]) > 0);
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
} // colStrings::sort

void ibis::colStrings::sort(uint32_t i, uint32_t j, ibis::bundle* bdl,
			    ibis::colList::iterator head,
			    ibis::colList::iterator tail) {
    if (i+32 > j) { // use selection sort
	for (uint32_t i1=i; i1+1<j; ++i1) {
	    uint32_t imin = i1;
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[i2] < (*array)[imin])
		    imin = i2;
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
	const std::string& sep = (*array)[i1]; // sep is the median of three
	i1 = i;
	i2 = j - 1;
	while (i1 < i2) {
	    const bool stayleft  = (sep.compare((*array)[i1]) > 0);
	    const bool stayright = (sep.compare((*array)[i2]) <= 0);
	    if (stayleft || stayright) { // at least one is in the right place
		i1 += (const int) stayleft;
		i2 -= (const int) stayright;
	    }
	    else { // both are in the wrong places, swap them
		(*array)[i2].swap((*array)[i1]);
		if (bdl) bdl->swapRIDs(i2, i1);
		for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
		    (*ii)->swap(i2, i1);
		++ i1; -- i2;
	    }
	}
	i1 += (int)(sep.compare((*array)[i1]) > 0);
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
	    for (uint32_t i2=i1+1; i2<j; ++i2)
		if ((*array)[ind[i2]].compare((*array)[ind[imin]]) < 0)
		    imin = i2;
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
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
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
    lg.buffer() << "DEBUG -- colInts::segment: old groups "
		<< (old != 0 ? nold-1 : 0) << ", new groups "
		<< res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg.buffer() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
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
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
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
    lg.buffer() << "DEBUG -- colUInts::segment: old groups "
		<< (old != 0 ? nold-1 : 0) << ", new groups "
		<< res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg.buffer() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
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
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
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
    lg.buffer() << "DEBUG -- colLongs::segment: old groups "
		<< (old != 0 ? nold-1 : 0) << ", new groups "
		<< res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg.buffer() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
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
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
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
    lg.buffer() << "DEBUG -- colULongs::segment: old groups "
		<< (old != 0 ? nold-1 : 0) << ", new groups "
		<< res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg.buffer() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
#endif
    return res;
} // ibis::colULongs::segment

// mark the start positions of the segments with identical values
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
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
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
    lg.buffer() << "DEBUG -- colFloats::segment: old groups "
		<< (old != 0 ? nold-1 : 0) << ", new groups "
		<< res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg.buffer() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
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
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
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
#if defined(_DEBUG) //_DEBUG+0>1 || DEBUG+0>1
    unsigned jold = 0, jnew = 0;
    ibis::util::logger lg(4);
    lg.buffer() << "DEBUG -- colDoubles::segment: old groups "
		<< (old != 0 ? nold-1 : 0) << ", new groups "
		<< res->size()-1;
    if (nold > 2) {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i] << "\t";
	    if (i == (*old)[jold]) {
		lg.buffer() << "++";
		++ jold;
	    }
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
		++ jnew;
	    }
	}
    }
    else {
	for (unsigned i = 0; i < nelm; ++ i) {
	    lg.buffer() << "\n" << i << "\t" << (*array)[i];
	    if (i == (*res)[jnew]) {
		lg.buffer() << "\t--";
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
	    std::vector<std::string>::reference target = (*array)[j];
	    for (++ j; j < (*old)[i+1]; ++ j) {
		while (j < (*old)[i+1] && target.compare((*array)[j]) == 0)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    }
	}
    }
    else { // start with all elements in one segment
	std::vector<std::string>::reference target = array->front();
	res->push_back(0); // the first number is always 0
	j = 1;
	while (j < nelm) {
	    while (j < nelm && target.compare((*array)[j]) == 0)
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
    return res;
} // ibis::colStrings::segment

// remove the duplicate elements accouting to the array starts
void ibis::colInts::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
} // ibis::colInts::reduce

// remove the duplicate elements accouting to the array starts
void ibis::colUInts::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
} // ibis::colUInts::reduce

// remove the duplicate elements accouting to the array starts
void ibis::colLongs::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
} // ibis::colLongs::reduce

// remove the duplicate elements accouting to the array starts
void ibis::colULongs::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
} // ibis::colULongs::reduce

// remove the duplicate elements accouting to the array starts
void ibis::colFloats::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
} // ibis::colFloats::reduce

// remove the duplicate elements accouting to the array starts
void ibis::colDoubles::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i] = (*array)[starts[i]];
    array->resize(nseg);
} // ibis::colDoubles::reduce

// remove the duplicate elements accouting to the array starts
void ibis::colStrings::reduce(const array_t<uint32_t>& starts) {
    const uint32_t nseg = starts.size() - 1;
    for (uint32_t i = 0; i < nseg; ++i) 
	(*array)[i].swap((*array)[starts[i]]);
    array->resize(nseg);
} // ibis::colStrings::reduce

// remove the duplicate elements according to the array starts
void ibis::colInts::reduce(const array_t<uint32_t>& starts,
			   ibis::selected::FUNCTION func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selected::NIL:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selected::AVG: // average
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
    case ibis::selected::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selected::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::VARPOP:
    case ibis::selected::VARSAMP:
    case ibis::selected::STDPOP:
    case ibis::selected::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stddev can be derived from
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
            count=1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++count;
                }
		avg=(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg=(*array)[starts[i]];
	    }


            if ((func == ibis::selected::VARSAMP) ||
		(func == ibis::selected::STDSAMP)) {
		--count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<int>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(std::sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<int>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(std::sqrt(((*array)[starts[i]]-avg)
				   *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selected::DISTINCT: // count distinct
	for (uint32_t i = 0; i < nseg; ++i) {
            std::vector<int32_t> values;
            values.resize(starts[i+1]-starts[i]);
            uint32_t c = 0;

	    for (uint32_t j = starts[i]; j < starts[i+1]; ++ j) {
                values[c++]=(*array)[j];
            }
            std::sort(values.begin(), values.end());

            int lastVal = *(values.begin());
 	    uint32_t distinct = 1;

            std::vector<int32_t>::iterator v;
            for (v = values.begin() +1 ; v < values.end(); ++v) {
                if (*v != lastVal) {
		    lastVal=*v;
		    ++distinct;
		}
            }
	    (*array)[i] = static_cast<int> (distinct);
	}
	break;
    }
    array->resize(nseg);
} // ibis::colInts::reduce

// remove the duplicate elements according to the array starts
void ibis::colUInts::reduce(const array_t<uint32_t>& starts,
			    ibis::selected::FUNCTION func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selected::NIL:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selected::AVG: // average
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
    case ibis::selected::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selected::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::VARPOP:
    case ibis::selected::VARSAMP:
    case ibis::selected::STDPOP:
    case ibis::selected::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stddev can be derived from
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
            count=1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++count;
                }
		avg=(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg=(*array)[starts[i]];
	    }


            if ((func == ibis::selected::VARSAMP) ||
		(func == ibis::selected::STDSAMP)) {
		--count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(std::sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(std::sqrt(((*array)[starts[i]]-avg)
				   *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selected::DISTINCT: // count distinct
	for (uint32_t i = 0; i < nseg; ++i) {
            std::vector<uint32_t> values;
            values.resize(starts[i+1]-starts[i]);
            uint32_t c = 0;

	    for (uint32_t j = starts[i]; j < starts[i+1]; ++ j) {
                values[c++]=(*array)[j];
            }
            std::sort(values.begin(), values.end());

            unsigned lastVal = *(values.begin());
 	    uint32_t distinct = 1;

            std::vector<uint32_t>::iterator v;
            for (v = values.begin() +1 ; v < values.end(); ++v) {
                if (*v != lastVal) {
		    lastVal=*v;
		    ++distinct;
		}
            }
	    (*array)[i] = static_cast<unsigned> (distinct);
	}
	break;
    }
    array->resize(nseg);
} // ibis::colUInts::reduce

// remove the duplicate elements according to the array starts
void ibis::colLongs::reduce(const array_t<uint32_t>& starts,
			    ibis::selected::FUNCTION func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selected::NIL:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selected::AVG: // average
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
    case ibis::selected::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selected::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::VARPOP:
    case ibis::selected::VARSAMP:
    case ibis::selected::STDPOP:
    case ibis::selected::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stddev can be derived from
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
            count=1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++count;
                }
		avg=(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg=(*array)[starts[i]];
	    }


            if ((func == ibis::selected::VARSAMP) ||
		(func == ibis::selected::STDSAMP)) {
		--count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<int>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(std::sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<int>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<int>
			(std::sqrt(((*array)[starts[i]]-avg)
				   *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selected::DISTINCT: // count distinct
	for (uint32_t i = 0; i < nseg; ++i) {
            std::vector<int64_t> values;
            values.resize(starts[i+1]-starts[i]);
            uint32_t c = 0;

	    for (uint32_t j = starts[i]; j < starts[i+1]; ++ j) {
                values[c++]=(*array)[j];
            }
            std::sort(values.begin(), values.end());

            int lastVal = *(values.begin());
 	    uint32_t distinct = 1;

            std::vector<int64_t>::iterator v;
            for (v = values.begin() +1 ; v < values.end(); ++v) {
                if (*v != lastVal) {
		    lastVal=*v;
		    ++distinct;
		}
            }
	    (*array)[i] = static_cast<int> (distinct);
	}
	break;
    }
    array->resize(nseg);
} // ibis::colLongs::reduce

// remove the duplicate elements according to the array starts
void ibis::colULongs::reduce(const array_t<uint32_t>& starts,
			     ibis::selected::FUNCTION func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selected::NIL:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selected::AVG: // average
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
    case ibis::selected::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selected::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::VARPOP:
    case ibis::selected::VARSAMP:
    case ibis::selected::STDPOP:
    case ibis::selected::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stddev can be derived from
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
            count=1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++count;
                }
		avg=(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg=(*array)[starts[i]];
	    }


            if ((func == ibis::selected::VARSAMP) ||
		(func == ibis::selected::STDSAMP)) {
		--count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(std::sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<unsigned>
			(((*array)[starts[i]]-avg)
			 *((*array)[starts[i]]-avg)/count);
		}
		else {
		    (*array)[i] = static_cast<unsigned>
			(std::sqrt(((*array)[starts[i]]-avg)
				   *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selected::DISTINCT: // count distinct
	for (uint32_t i = 0; i < nseg; ++i) {
            std::vector<uint64_t> values;
            values.resize(starts[i+1]-starts[i]);
            uint32_t c = 0;

	    for (uint32_t j = starts[i]; j < starts[i+1]; ++ j) {
                values[c++]=(*array)[j];
            }
            std::sort(values.begin(), values.end());

            unsigned lastVal = *(values.begin());
 	    uint32_t distinct = 1;

            std::vector<uint64_t>::iterator v;
            for (v = values.begin() +1 ; v < values.end(); ++v) {
                if (*v != lastVal) {
		    lastVal=*v;
		    ++distinct;
		}
            }
	    (*array)[i] = static_cast<unsigned> (distinct);
	}
	break;
    }
    array->resize(nseg);
} // ibis::colULongs::reduce

// remove the duplicate elements according to the array starts
void ibis::colFloats::reduce(const array_t<uint32_t>& starts,
			     ibis::selected::FUNCTION func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selected::NIL:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selected::AVG: // average
	for (uint32_t i = 0; i < nseg; ++i) {
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
    case ibis::selected::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selected::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::VARPOP:
    case ibis::selected::VARSAMP:
    case ibis::selected::STDPOP:
    case ibis::selected::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stddev can be derived from
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
            count=1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++count;
                }
		avg=(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg=(*array)[starts[i]];
	    }


            if ((func == ibis::selected::VARSAMP) ||
		(func == ibis::selected::STDSAMP)) {
		--count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = static_cast<float>(variance/count);
		}
		else {
		    (*array)[i] = static_cast<float>
			(std::sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = ((*array)[starts[i]]-avg)
			*((*array)[starts[i]]-avg)/count;
		}
		else {
		    (*array)[i] = static_cast<float>
			(std::sqrt(((*array)[starts[i]]-avg)
				   *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selected::DISTINCT: // count distinct
	for (uint32_t i = 0; i < nseg; ++i) {
            std::vector<float> values;
            values.resize(starts[i+1]-starts[i]);
            uint32_t c = 0;

	    for (uint32_t j = starts[i]; j < starts[i+1]; ++ j) {
                values[c++]=(*array)[j];
            }
            std::sort(values.begin(), values.end());

            float lastVal = *(values.begin());
 	    uint32_t distinct = 1;

            std::vector<float>::iterator v;
            for (v = values.begin() +1 ; v < values.end(); ++v) {
                if (*v != lastVal) {
		    lastVal=*v;
		    ++distinct;
		}
            }
	    (*array)[i] = static_cast<float> (distinct);
	}
	break;
    }
    array->resize(nseg);
} // ibis::colFloats::reduce

// remove the duplicate elements according to the array starts
void ibis::colDoubles::reduce(const array_t<uint32_t>& starts,
			      ibis::selected::FUNCTION func) {
    const uint32_t nseg = starts.size() - 1;
    switch (func) {
    default: // only save the first value
    case ibis::selected::NIL:
	for (uint32_t i = 0; i < nseg; ++i) 
	    (*array)[i] = (*array)[starts[i]];
	break;
    case ibis::selected::AVG: // average
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
    case ibis::selected::SUM: // sum
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		(*array)[i] += (*array)[j];
	}
	break;
    case ibis::selected::MIN: // min
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] > (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::MAX: // max
	for (uint32_t i = 0; i < nseg; ++i) {
	    (*array)[i] = (*array)[starts[i]];
	    for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		if ((*array)[i] < (*array)[j])
		    (*array)[i] = (*array)[j];
	}
	break;
    case ibis::selected::VARPOP:
    case ibis::selected::VARSAMP:
    case ibis::selected::STDPOP:
    case ibis::selected::STDSAMP:
    	// we can use the same functionality for all functions as sample &
    	// population functions are similar, and stddev can be derived from
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
            count=1; // calculate avg first because needed in the next step
	    if (starts[i+1] > starts[i]+1) {
		double sum = (*array)[starts[i]];
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j) {
		    sum += (*array)[j];
                    ++count;
                }
		avg=(sum / (starts[i+1]-starts[i]));
	    }
	    else {
		avg=(*array)[starts[i]];
	    }


            if ((func == ibis::selected::VARSAMP) ||
		(func == ibis::selected::STDSAMP)) {
		--count; // sample version denominator is number of rows -1
	    }

	    if (starts[i+1] > starts[i]+1) {
		double variance = (((*array)[starts[i]])-avg)
		    *(((*array)[starts[i]]-avg));
		for (uint32_t j = starts[i]+1; j < starts[i+1]; ++ j)
		    variance += ((*array)[j]-avg)*((*array)[j]-avg);

		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = variance/count;
		}
		else {
		    (*array)[i] = (std::sqrt(variance/count));
		}
	    }
	    else {
		if (func == ibis::selected::VARPOP ||
		    func == ibis::selected::VARSAMP) {
		    (*array)[i] = ((*array)[starts[i]]-avg)
			*((*array)[starts[i]]-avg)/count;
		}
		else {
		    (*array)[i] =
			(std::sqrt(((*array)[starts[i]]-avg)
				   *((*array)[starts[i]]-avg)/count));
		}
	    }
	}
	break;
    case ibis::selected::DISTINCT: // count distinct
	for (uint32_t i = 0; i < nseg; ++i) {
            std::vector<double> values;
            values.resize(starts[i+1]-starts[i]);
            uint32_t c = 0;

	    for (uint32_t j = starts[i]; j < starts[i+1]; ++ j) {
                values[c++]=(*array)[j];
            }
            std::sort(values.begin(), values.end());

            double lastVal = *(values.begin());
 	    uint32_t distinct = 1;

            std::vector<double>::iterator v;
            for (v = values.begin() +1 ; v < values.end(); ++v) {
                if (*v != lastVal) {
		    lastVal=*v;
		    ++distinct;
		}
            }
	    (*array)[i] = distinct;
	}
	break;
    }
    array->resize(nseg);
} // ibis::colDoubles::reduce

void ibis::colStrings::reduce(const array_t<uint32_t>&,
			      ibis::selected::FUNCTION) {
    LOGGER(ibis::gVerbose >= 0 && col != 0)
	<< "Warning -- colStrings::reduce can NOT apply any aggregate "
	"function on column " << col->name() << " (type "
	<< ibis::TYPESTRING[(int)(col->type())] << ")";
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
    int32_t ret = 0x10000000;
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
    int64_t ret = 0x1000000000000000LL;
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
	    << "(# " << i << " out of " << array->size() << ") to file, ierr = "
	    << ierr;
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
#if defined(DEBUG) //|| defined(_DEBUG) //&& DEBUG > 2
    ibis::util::logger lg(4);
    lg.buffer() << "colStrings::topk(" << k << ")\n";
    for (uint32_t i = 0; i < back; ++i)
	lg.buffer() << ind[i] << "\t" << (*array)[ind[i]] << "\n";
    std::flush(lg.buffer());
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
#if defined(DEBUG) //|| defined(_DEBUG) //&& DEBUG > 2
    ibis::util::logger lg(4);
    lg.buffer() << "colStrings::bottomk(" << k << ")\n";
    for (uint32_t i = 0; i < back; ++i)
	lg.buffer() << ind[i] << "\t" << (*array)[ind[i]] << "\n";
    std::flush(lg.buffer());
#endif
} // ibis::colStrings::bottomk


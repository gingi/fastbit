//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2008 the Regents of the University of California
///
/// Implementatioin of the colValues class hierarchy.
///
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

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
    case ibis::TEXT:
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
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colValues does not support type "
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
	    << "ibis::colValues does not yet support type "
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
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colValues does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
	return 0;
    }
} // ibis::colValues::create

ibis::colInts::colInts(const ibis::column* c, void* vals)
    : colValues(c), array(new array_t<int32_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int32_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colInts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
    }
} // ibis::colInts::colInts

ibis::colUInts::colUInts(const ibis::column* c, void* vals)
    : colValues(c), array(new array_t<uint32_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint32_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colUInts does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
    }
} // ibis::colUInts::colUInts

ibis::colLongs::colLongs(const ibis::column* c, void* vals)
    : colValues(c), array(new array_t<int64_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int64_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<int64_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colLongs does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
    }
} // ibis::colLongs::colLongs

ibis::colULongs::colULongs(const ibis::column* c, void* vals)
    : colValues(c), array(new array_t<uint64_t>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint64_t>((*arr)[i]);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<uint64_t>((*arr)[i]);
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colULongs does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
    }
} // ibis::colULongs::colULongs

ibis::colFloats::colFloats(const ibis::column* c, void* vals)
    : colValues(c), array(new array_t<float>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<float>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colFloats does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
    }
} // ibis::colFloats::colFloats

ibis::colDoubles::colDoubles(const ibis::column* c, void* vals)
    : colValues(c), array(new array_t<double>) {
    if (c == 0 || vals == 0) return;
    switch (c->type()) {
    case ibis::UINT: {
	const array_t<uint32_t>* arr = static_cast<array_t<uint32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<array_t<unsigned char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* arr = static_cast<array_t<uint16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::INT: {
	const array_t<int32_t>* arr = static_cast<array_t<int32_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<array_t<signed char>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* arr = static_cast<array_t<int16_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* arr = static_cast<array_t<uint64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<double>((*arr)[i]);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* arr = static_cast<array_t<int64_t>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = static_cast<double>((*arr)[i]);
	break;}
    case ibis::FLOAT: {
	const array_t<float>* arr = static_cast<array_t<float>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* arr = static_cast<array_t<double>*>(vals);
	array->resize(arr->size());
	for (size_t i = 0; i < arr->size(); ++ i)
	    (*array)[i] = (*arr)[i];
	break;}
    default:
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::colDoubles does not support type "
	    << ibis::TYPESTRING[(int)(c->type())] << " yet";
    }
} // ibis::colDoubles::colDoubles

void ibis::colInts::sort(uint32_t i, uint32_t j, ibis::bundle* bdl) {
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    int32_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    int32_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
			(*ii)->swap(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    uint32_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    uint32_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
			(*ii)->swap(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    int64_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    int64_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
			(*ii)->swap(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    unsigned tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    uint64_t tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
			(*ii)->swap(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    float tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    float tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
			(*ii)->swap(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    double tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		}
    } // end buble sort
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
    if (i+32 > j) { // use buble sort
	for (uint32_t i1=j-1; i1>i; --i1)
	    for (uint32_t i2=i; i2<i1; ++i2)
		if ((*array)[i2] > (*array)[i2+1]) {
		    double tmp = (*array)[i2];
		    (*array)[i2] = (*array)[i2+1];
		    (*array)[i2+1] = tmp;
		    if (bdl) bdl->swapRIDs(i2, i2+1);
		    for (ibis::colList::iterator ii=head; ii!=tail; ++ii)
			(*ii)->swap(i2, i2+1);
		}
    } // end buble sort
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

// mark the start positions of the segments with identical values
array_t<uint32_t>*
ibis::colInts::segment(const array_t<uint32_t>* old) const {
    array_t<uint32_t>* res = new array_t<uint32_t>;
    res->push_back(0); // the first number is always 0
    uint32_t j = 1;
    int32_t target = *(array->begin());
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments with in the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    do {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    } while (j < (*old)[i+1]);
	}
    }
    else { // start with all elements in one segment
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
    return res;
} // ibis::colInts::segment

array_t<uint32_t>*
ibis::colUInts::segment(const array_t<uint32_t>* old) const {
    array_t<uint32_t>* res = new array_t<uint32_t>;
    res->push_back(0); // the first number is always 0
    uint32_t j = 1;
    uint32_t target = *(array->begin());
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments with in the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    do {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    } while (j < (*old)[i+1]);
	}
    }
    else { // start with all elements in one segment
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
    return res;
} // ibis::colUInts::segment

// mark the start positions of the segments with identical values
array_t<uint32_t>*
ibis::colLongs::segment(const array_t<uint32_t>* old) const {
    array_t<uint32_t>* res = new array_t<uint32_t>;
    res->push_back(0); // the first number is always 0
    uint32_t j = 1;
    int64_t target = *(array->begin());
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments with in the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    do {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    } while (j < (*old)[i+1]);
	}
    }
    else { // start with all elements in one segment
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
    return res;
} // ibis::colLongs::segment

array_t<uint32_t>*
ibis::colULongs::segment(const array_t<uint32_t>* old) const {
    array_t<uint32_t>* res = new array_t<uint32_t>;
    res->push_back(0); // the first number is always 0
    uint32_t j = 1;
    uint64_t target = *(array->begin());
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments with in the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    do {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    } while (j < (*old)[i+1]);
	}
    }
    else { // start with all elements in one segment
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
    return res;
} // ibis::colULongs::segment

// mark the start positions of the segments with identical values
array_t<uint32_t>*
ibis::colFloats::segment(const array_t<uint32_t>* old) const {
    array_t<uint32_t>* res = new array_t<uint32_t>;
    res->push_back(0); // the first number is always 0
    uint32_t j = 1;
    float target = *(array->begin());
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments with in the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    do {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    } while (j < (*old)[i+1]);
	}
    }
    else { // start with all elements in one segment
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
    return res;
} // ibis::colFloats::segment

// mark the start positions of the segments with identical values
array_t<uint32_t>*
ibis::colDoubles::segment(const array_t<uint32_t>* old) const {
    array_t<uint32_t>* res = new array_t<uint32_t>;
    res->push_back(0); // the first number is always 0
    uint32_t j = 1;
    double target = *(array->begin());
    const uint32_t nelm = array->size();

    if (old != 0 && old->size()>2) {
	// find segments with in the previously defined segments
	for (uint32_t i=0; i<old->size()-1; ++i) {
	    do {
		while (j < (*old)[i+1] && (*array)[j] == target)
		    ++ j;
		res->push_back(j);
		if (j < nelm) {
		    target = (*array)[j];
		    ++ j;
		}
	    } while (j < (*old)[i+1]);
	}
    }
    else { // start with all elements in one segment
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
    return res;
} // ibis::colDoubles::segment

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
    }
    array->resize(nseg);
} // ibis::colDoubles::reduce

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

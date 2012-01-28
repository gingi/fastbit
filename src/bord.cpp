// File: $id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2012 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tab.h"	// ibis::tabula and ibis::tabele
#include "bord.h"	// ibis::bord
#include "query.h"	// ibis::query
#include "countQuery.h"	// ibis::countQuery
#include "bundle.h"	// ibis::bundle
#include "ikeywords.h"	// ibis::keyword::tokenizer

#include <iomanip>	// std::setprecision
#include <limits>	// std::numeric_limits
#include <sstream>	// std::ostringstream
#include <typeinfo>	// std::typeid
#include <memory>	// std::auto_ptr
#include <algorithm>	// std::reverse, std::copy

/// Constructor.  The responsibility of freeing the memory pointed by the
/// elements of buf is transferred to this object.
ibis::bord::bord(const char *tn, const char *td, uint64_t nr,
		 ibis::table::bufferList       &buf,
		 const ibis::table::typeList   &ct,
		 const ibis::table::stringList &cn,
		 const ibis::table::stringList *cdesc,
		 const std::vector<const ibis::dictionary*> *dct)
    : ibis::table(), ibis::part("in-core") {
    nEvents = static_cast<uint32_t>(nr);
    if (nEvents != nr) {
	LOGGER(ibis::gVerbose >= 0)
	    << "bord::ctor can not handle " << nr
	    << " rows in an in-memory table";
	throw "Too many rows for an in-memory table";
    }

    switchTime = time(0);
    if (td != 0 && *td != 0) {
	m_desc = td;
    }
    else if (tn != 0 && *tn != 0) {
	m_desc = tn;
    }
    else {
	char abuf[32];
	ibis::util::secondsToString(switchTime, abuf);
	m_desc = "unnamed in-memory data partition constructed at ";
	m_desc += abuf;
    }
    m_name = ibis::util::strnewdup(tn?tn:ibis::util::shortName(m_desc).c_str());
    name_ = m_name; // make sure the name of part and table are the same
    desc_ = m_desc;

    const uint32_t nc = (cn.size()<=ct.size() ? cn.size() : ct.size());
    for (uint32_t i = 0; i < nc; ++ i) {
	if (columns.find(cn[i]) == columns.end()) { // a new column
	    ibis::bord::column *tmp;
	    if (cdesc != 0 && cdesc->size() > i)
		tmp = new ibis::bord::column
		    (this, ct[i], cn[i], buf[i], (*cdesc)[i]);
	    else
		tmp = new ibis::bord::column(this, ct[i], cn[i], buf[i]);
	    if (dct != 0 && i < dct->size())
		tmp->setDictionary((*dct)[i]);
	    columns[tmp->name()] = tmp;
	    colorder.push_back(tmp);
	}
	else { // duplicate name
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- bord::ctor found column " << i << " ("
		<< cn[i] << ") to be a duplicate, discarding it...";
	    // free the buffer because it will not be freed anywhere else
	    freeBuffer(buf[i], ct[i]);
	}
	buf[i] = 0;
    }

    amask.set(1, nEvents);
    state = ibis::part::STABLE_STATE;
    if (ibis::gVerbose > 1) {
	ibis::util::logger lg;
	lg() << "Constructed in-memory data partition "
	     << (m_name != 0 ? m_name : "<unnamed>") << " -- " << m_desc
	     << " -- with " << nr << " row" << (nr > 1U ? "s" : "") << " and "
	     << columns.size() << " column" << (columns.size() > 1U ? "s" : "");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    dumpNames(lg(), ",\t");
	    if (ibis::gVerbose > 6) {
		uint64_t npr = (1ULL << (ibis::gVerbose - 4));
		lg() << "\n";
		dump(lg(), npr, ",\t");
	    }
	}
    }
} // ibis::bord::bord

/// Constructor.  It produces an empty data partition for storing values to
/// be selected by the select clause.  The reference data partition ref is
/// used to determine the data types.  Use the append function to add data
/// for the actual selected values.
ibis::bord::bord(const char *tn, const char *td,
		 const ibis::selectClause &sc, const ibis::part &ref)
    : ibis::part("in-core") {
    if (td != 0 && *td != 0) {
	m_desc = td;
    }
    else {
	std::ostringstream oss;
	oss << "in-memory data partition for select clause " << sc;
	m_desc = oss.str();
    }
    if (tn != 0 && *tn != 0) {
	m_name = ibis::util::strnewdup(tn);
    }
    else {
	m_name = ibis::util::strnewdup(ibis::util::shortName(m_desc).c_str());
    }
    desc_ = m_desc;
    name_ = m_name;
    const size_t nagg = sc.aggSize();
    for (size_t j = 0; j < nagg; ++ j) {
	const char* cname = sc.aggName(j);
	const ibis::math::term* ctrm = sc.aggExpr(j);
	if (cname == 0 || *cname == 0 || ctrm == 0) continue;
	cname = ibis::part::skipPrefix(cname);

	switch (ctrm->termType()) {
	case ibis::math::UNDEF_TERM:
	case ibis::math::NUMBER:
	case ibis::math::STRING:
	    break;
	case ibis::math::VARIABLE: {
	    const ibis::math::variable &var =
		*static_cast<const ibis::math::variable*>(ctrm);
	    const ibis::column* refcol = ref.getColumn(var.variableName());
	    if (*(var.variableName()) == '*') { // special name
		ibis::bord::column *col = new ibis::bord::column
		    (this, ibis::UINT, "*", 0, "count(*)");
		if (col != 0) {
		    columns[col->name()] = col;
		    colorder.push_back(col);
		}
		else {
		    LOGGER(ibis::gVerbose > 1)
			<< "Warning -- bord::ctor failed to allocate column "
			<< cname << " for in-memory partition " << tn;
		    throw "bord::ctor failed to allocate a column";
		}
	    }
	    else if (refcol != 0) {
		ibis::TYPE_T t = refcol->type();
		if (refcol->type() == ibis::CATEGORY)
		    t = ibis::UINT;
		ibis::bord::column *col = new ibis::bord::column
		    (this, t, cname, 0, sc.aggName(j));
		if (col != 0) {
		    if (refcol->type() == ibis::CATEGORY) {
			col->loadIndex();
			col->setDictionary(static_cast<const ibis::category*>
					   (refcol)->getDictionary());
		    }
		    else if (refcol->type() == ibis::UINT) {
			const ibis::bord::column *bc =
			    dynamic_cast<const ibis::bord::column*>(refcol);
			if (bc != 0)
			    col->setDictionary(bc->getDictionary());
		    }
		    columns[col->name()] = col;
		    colorder.push_back(col);
		}
		else {
		    LOGGER(ibis::gVerbose > 1)
			<< "Warning -- bord::ctor failed to allocate column "
			<< cname << " for in-memory partition " << tn;
		    throw "bord::ctor failed to allocate a column";
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- bord::ctor failed to locate column "
		    << var.variableName() << " in data partition "
		    << ref.name();
		throw "bord::ctor failed to locate a needed column";
	    }
	    break;}
	default: {
	    ibis::bord::column *col = new ibis::bord::column
		(this, ibis::DOUBLE, cname, 0, sc.aggName(j));
	    if (col != 0) {
		columns[col->name()] = col;
		colorder.push_back(col);
	    }
	    else {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- bord::ctor failed to allocate column "
		    << cname << " for in-memory partition " << tn;
		throw "bord::ctor failed to allocate a column";
	    }
	    break;}
	}
    }

    state = ibis::part::STABLE_STATE;
    LOGGER(ibis::gVerbose > 1)
	<< "Constructed in-memory data partition "
	<< (m_name != 0 ? m_name : "<unnamed>") << " -- " << m_desc
	<< " -- with " << columns.size() << " column"
	<< (columns.size() > 1U ? "s" : "");
} // ctor

/// Clear the existing content.
void ibis::bord::clear() {
} // ibis::bord::clear

/// @note The pointers returned are pointing to names stored internally.
/// The caller should not attempt to free these pointers.
ibis::table::stringList ibis::bord::columnNames() const {
    return ibis::part::columnNames();
} // ibis::bord::columnNames

ibis::table::typeList ibis::bord::columnTypes() const {
    return ibis::part::columnTypes();
} // ibis::bord::columnTypes

int64_t ibis::bord::getColumnAsBytes(const char *cn, char *vals,
				     uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() != ibis::BYTE && col->type() != ibis::UBYTE)
	return -2;

    const array_t<signed char>* arr =
	static_cast<const array_t<signed char>*>(col->getArray());
    if (arr == 0) return -3;

    uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
    if (begin >= sz)
	return 0;

    if (end > begin)
	sz = end - begin;
    else
	sz = sz - begin;
    std::copy(arr->begin()+begin, arr->begin()+sz, vals);
    return sz;
} // ibis::bord::getColumnAsBytes

int64_t
ibis::bord::getColumnAsUBytes(const char *cn, unsigned char *vals,
			      uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() != ibis::BYTE && col->type() != ibis::UBYTE)
	return -2;

    const array_t<unsigned char>* arr =
	static_cast<const array_t<unsigned char>*>(col->getArray());
    if (arr == 0) return -3;

    uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
    if (begin >= sz)
	return 0;

    if (end > begin)
	sz = end - begin;
    else
	sz = sz - begin;
    std::copy(arr->begin()+begin, arr->begin()+sz, vals);
    return sz;
} // ibis::bord::getColumnAsUBytes

int64_t ibis::bord::getColumnAsShorts(const char *cn, int16_t *vals,
				      uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsShorts

int64_t ibis::bord::getColumnAsUShorts(const char *cn, uint16_t *vals,
				       uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsUShorts

int64_t ibis::bord::getColumnAsInts(const char *cn, int32_t *vals,
				    uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::INT || col->type() == ibis::UINT) {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT) {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsInts

int64_t ibis::bord::getColumnAsUInts(const char *cn, uint32_t *vals,
				     uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::INT || col->type() == ibis::UINT) {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsUInts

int64_t ibis::bord::getColumnAsLongs(const char *cn, int64_t *vals,
				     uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::LONG || col->type() == ibis::ULONG) {
	const array_t<int64_t>* arr =
	    static_cast<const array_t<int64_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::INT) {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UINT) {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT) {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE) {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsLongs

int64_t ibis::bord::getColumnAsULongs(const char *cn, uint64_t *vals,
				      uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    if (col->type() == ibis::LONG || col->type() == ibis::ULONG) {
	const array_t<uint64_t>* arr =
	    static_cast<const array_t<uint64_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::INT || col->type() == ibis::UINT) {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::SHORT || col->type() == ibis::USHORT) {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else if (col->type() == ibis::BYTE || col->type() == ibis::UBYTE) {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    else {
	return -2;
    }
} // ibis::bord::getColumnAsULongs

int64_t ibis::bord::getColumnAsFloats(const char *cn, float *vals,
				      uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::FLOAT: {
	const array_t<float>* arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::SHORT: {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::USHORT: {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsFloats

int64_t ibis::bord::getColumnAsDoubles(const char *cn, double *vals,
				       uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::DOUBLE: {
	const array_t<double>* arr =
	    static_cast<const array_t<double>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::FLOAT: {
	const array_t<float>* arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::INT: {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::UINT: {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::SHORT: {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::USHORT: {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	std::copy(arr->begin()+begin, arr->begin()+sz, vals);
	return sz;
    }
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsDoubles

int64_t ibis::bord::getColumnAsDoubles(const char* cn,
				       std::vector<double>& vals,
				       uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<const ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::DOUBLE: {
	const array_t<double>* arr =
	    static_cast<const array_t<double>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::FLOAT: {
	const array_t<float>* arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::INT: {
	const array_t<int32_t>* arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::UINT: {
	const array_t<uint32_t>* arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::SHORT: {
	const array_t<int16_t>* arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::USHORT: {
	const array_t<uint16_t>* arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::BYTE: {
	const array_t<signed char>* arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::UBYTE: {
	const array_t<unsigned char>* arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsDoubles

int64_t ibis::bord::getColumnAsStrings(const char *cn,
				       std::vector<std::string> &vals,
				       uint64_t begin, uint64_t end) const {
    const ibis::bord::column *col =
	dynamic_cast<ibis::bord::column*>(getColumn(cn));
    if (col == 0)
	return -1;

    switch (col->type()) {
    case ibis::TEXT: {
	const std::vector<std::string>* arr =
	    static_cast<const std::vector<std::string>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	if (end > begin)
	    sz = end - begin;
	else
	    sz = sz - begin;
	vals.resize(sz);
	std::copy(arr->begin()+begin, arr->begin()+sz, vals.begin());
	return sz;
    }
    case ibis::DOUBLE: {
	const array_t<double> *arr =
	    static_cast<const array_t<double>*>(col->getArray());
	if (arr == 0) return -3;

	const uint32_t sz = arr->size();
	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::FLOAT: {
	const array_t<float> *arr =
	    static_cast<const array_t<float>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::LONG: {
	const array_t<int64_t> *arr =
	    static_cast<const array_t<int64_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::ULONG: {
	const array_t<uint64_t> *arr =
	    static_cast<const array_t<uint64_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::INT: {
	const array_t<int32_t> *arr =
	    static_cast<const array_t<int32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}

    case ibis::CATEGORY:{
	const array_t<uint32_t> *arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	const ibis::dictionary* aDic = col->getDictionary();
	for (uint32_t i = 0; i < sz; ++ i) {
	    vals[i] = (*aDic)[(*arr)[i+begin]];
	}
	return sz;}
    case ibis::UINT: {
	const array_t<uint32_t> *arr =
	    static_cast<const array_t<uint32_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::SHORT: {
	const array_t<int16_t> *arr =
	    static_cast<const array_t<int16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::USHORT: {
	const array_t<uint16_t> *arr =
	    static_cast<const array_t<uint16_t>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << (*arr)[i+begin];
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::BYTE: {
	const array_t<signed char> *arr =
	    static_cast<const array_t<signed char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << static_cast<int>((*arr)[i+begin]);
	    vals[i] = oss.str();
	}
	return sz;}
    case ibis::UBYTE: {
	const array_t<unsigned char> *arr =
	    static_cast<const array_t<unsigned char>*>(col->getArray());
	if (arr == 0) return -3;

	uint32_t sz = (nEvents <= arr->size() ? nEvents : arr->size());
	if (begin >= sz)
	    return 0;

	vals.resize(sz);
	for (uint32_t i = 0; i < sz; ++ i) {
	    std::ostringstream oss;
	    oss << static_cast<int>((*arr)[i+begin]);
	    vals[i] = oss.str();
	}
	return sz;}
    default:
	break;
    }
    return -2;
} // ibis::bord::getColumnAsStrings

double ibis::bord::getColumnMin(const char* cn) const {
    return getActualMin(cn);
} // ibis::bord::getColumnMin

double ibis::bord::getColumnMax(const char* cn) const {
    return getActualMax(cn);
} // ibis::bord::getColumnMax

long ibis::bord::getHistogram(const char *constraints,
			      const char *cname,
			      double begin, double end, double stride,
			      std::vector<uint32_t>& counts) const {
    if (sizeof(uint32_t) == sizeof(uint32_t)) {
	return get1DDistribution
	    (constraints, cname, begin, end, stride,
	     reinterpret_cast<std::vector<uint32_t>&>(counts));
    }
    else {
	std::vector<uint32_t> tmp;
	long ierr = get1DDistribution(constraints, cname, begin, end,
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
				std::vector<uint32_t>& counts) const {
    if (sizeof(uint32_t) == sizeof(uint32_t)) {
	return get2DDistribution
	    (constraints, cname1, begin1, end1, stride1,
	     cname2, begin2, end2, stride2,
	     reinterpret_cast<std::vector<uint32_t>&>(counts));
    }
    else {
	std::vector<uint32_t> tmp;
	long ierr = get2DDistribution
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
				std::vector<uint32_t>& counts) const {
    if (sizeof(uint32_t) == sizeof(uint32_t)) {
	return get3DDistribution
	    (constraints, cname1, begin1, end1, stride1,
	     cname2, begin2, end2, stride2,
	     cname3, begin3, end3, stride3,
	     reinterpret_cast<std::vector<uint32_t>&>(counts));
    }
    else {
	std::vector<uint32_t> tmp;
	long ierr = get3DDistribution
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
    nmin = 0;
    nmax = nEvents;
    ibis::countQuery q;
    int ierr = q.setWhereClause(cond);
    if (ierr < 0) return;

    ierr = q.setPartition(this);
    if (ierr < 0) return;

    ierr = q.estimate();
    if (ierr >= 0) {
	nmin = q.getMinNumHits();
	nmax = q.getMaxNumHits();
    }
} // ibis::bord::estimate

void ibis::bord::estimate(const ibis::qExpr *cond,
			  uint64_t &nmin, uint64_t &nmax) const {
    nmin = 0;
    nmax = nEvents;
    ibis::countQuery q;
    int ierr = q.setWhereClause(cond);
    if (ierr < 0) return;

    ierr = q.setPartition(this);
    if (ierr < 0) return;

    ierr = q.estimate();
    if (ierr >= 0) {
	nmin = q.getMinNumHits();
	nmax = q.getMaxNumHits();
    }
} // ibis::bord::estimate

ibis::table* ibis::bord::select(const char *sel, const char *cond) const {
    if (cond == 0 || *cond == 0) return 0;
    if (sel != 0) // skip leading space
	while (isspace(*sel)) ++ sel;

    ibis::constPartList prts(1);
    prts[0] = this;
    return ibis::table::select(prts, sel, cond);
} // ibis::bord::select

/// Compute the number of hits.
int64_t ibis::bord::computeHits(const char *cond) const {
    int64_t res = -1;
    ibis::query q(ibis::util::userName(), this);
    q.setWhereClause(cond);
    res = q.evaluate();
    if (res >= 0)
	res = q.getNumHits();
    return res;
} // ibis::bord::computeHits

int ibis::bord::getPartitions(ibis::constPartList &lst) const {
    lst.resize(1);
    lst[0] = this;
    return 1;
} // ibis::bord::getPartitions

void ibis::bord::describe(std::ostream& out) const {
    out << "Table (in memory) " << name_ << " (" << m_desc
	<< ") contsists of " << columns.size() << " column"
	<< (columns.size()>1 ? "s" : "") << " and " << nEvents
	<< " row" << (nEvents>1 ? "s" : "");
    if (colorder.empty()) {
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it) {
	    out << "\n" << (*it).first << "\t"
		<< ibis::TYPESTRING[(int)(*it).second->type()];
	    if (static_cast<const ibis::bord::column*>(it->second)
		->getDictionary() != 0)
		out << " (dictionary size: "
		    << static_cast<const ibis::bord::column*>(it->second)
		    ->getDictionary()->size() << ')';
	    if (it->second->description() != 0 && ibis::gVerbose > 1)
		out << "\t" << it->second->description();
	}
    }
    else if (colorder.size() == columns.size()) {
	for (uint32_t i = 0; i < columns.size(); ++ i) {
	    out << "\n" << colorder[i]->name() << "\t"
		<< ibis::TYPESTRING[(int)colorder[i]->type()];
	    if (static_cast<const ibis::bord::column*>(colorder[i])
		->getDictionary() != 0)
		out << " (dictionary size: "
		    << static_cast<const ibis::bord::column*>(colorder[i])
		    ->getDictionary()->size() << ')';
	    if (colorder[i]->description() != 0 && ibis::gVerbose > 1)
		out << "\t" << colorder[i]->description();
	}
    }
    else {
	std::set<const char*, ibis::lessi> names;
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it)
	    names.insert((*it).first);
	for (uint32_t i = 0; i < colorder.size(); ++ i) {
	    out << "\n" << colorder[i]->name() << "\t"
		<< ibis::TYPESTRING[(int)colorder[i]->type()];
	    if (static_cast<const ibis::bord::column*>(colorder[i])
		->getDictionary() != 0)
		out << " (dictionary size: "
		    << static_cast<const ibis::bord::column*>(colorder[i])
		    ->getDictionary()->size() << ')';
	    if (colorder[i]->description() != 0 && ibis::gVerbose > 1)
		out << "\t" << colorder[i]->description();
	    names.erase(colorder[i]->name());
	}
	for (std::set<const char*, ibis::lessi>::const_iterator it =
		 names.begin(); it != names.end(); ++ it) {
	    ibis::part::columnList::const_iterator cit = columns.find(*it);
	    out << "\n" << (*cit).first << "\t"
		<< ibis::TYPESTRING[(int)(*cit).second->type()];
	    if (static_cast<const ibis::bord::column*>(cit->second)
		->getDictionary() != 0)
		out << " (dictionary size: "
		    << static_cast<const ibis::bord::column*>(cit->second)
		    ->getDictionary()->size() << ')';
	    if (cit->second->description() != 0 && ibis::gVerbose > 1)
		out << "\t" << (*cit).second->description();
	}
    }
    out << std::endl;
} // ibis::bord::describe

void ibis::bord::dumpNames(std::ostream& out, const char* del) const {
    if (columns.empty()) return;
    if (del == 0) del = ", ";

    if (colorder.empty()) {
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it) {
	    if (it != columns.begin())
		out << del;
	    out << (*it).first;
	    if (static_cast<const ibis::bord::column*>(it->second)
		->getDictionary() != 0 && ibis::gVerbose > 2)
		out << " (k)";
	}
    }
    else if (colorder.size() == columns.size()) {
	out << colorder[0]->name();
	if (static_cast<const ibis::bord::column*>(colorder[0])
	    ->getDictionary() != 0 && ibis::gVerbose > 2)
	    out << " (k)";
	for (uint32_t i = 1; i < columns.size(); ++ i) {
	    out << del << colorder[i]->name();
	    if (static_cast<const ibis::bord::column*>(colorder[i])
		->getDictionary() != 0 && ibis::gVerbose > 2)
		out << " (k)";
	}
    }
    else {
	std::set<const char*, ibis::lessi> names;
	for (ibis::part::columnList::const_iterator it = columns.begin();
	     it != columns.end(); ++ it)
	    names.insert((*it).first);
	for (uint32_t i = 0; i < colorder.size(); ++ i) {
	    if (i > 0)
		out << del;
	    out << colorder[i]->name();
	    if (static_cast<const ibis::bord::column*>(colorder[i])
		->getDictionary() != 0 && ibis::gVerbose > 2)
		out << " (k)";
	    names.erase(colorder[i]->name());
	}
	for (std::set<const char*, ibis::lessi>::const_iterator it =
		 names.begin(); it != names.end(); ++ it) {
	    out << del << *it;
	    ibis::part::columnList::const_iterator cit = columns.find(*it);
	    if (static_cast<const ibis::bord::column*>(cit->second)
		->getDictionary() != 0 && ibis::gVerbose > 2)
		out << " (k)";
	}
    }
    out << std::endl;
} // ibis::bord::dumpNames

int ibis::bord::dump(std::ostream& out, const char* del) const {
    return dump(out, nEvents, del);
} // ibis::bord::dump

/**
   Print the first nr rows of the data to the given output stream.

   The return values:
   @code
   0  -- normal (successful) completion
   -1  -- no data in-memory
   -2  -- unknown data type
   -3  -- some columns not ibis::bord::column (not in-memory)
   -4  -- error in the output stream
   @endcode
*/
int ibis::bord::dump(std::ostream& out, uint64_t nr,
		     const char* del) const {
    const uint32_t ncol = columns.size();
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
	for (uint32_t i = 0; i < ncol; ++ i) {
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
	for (uint32_t i = 0; i < colorder.size(); ++ i) {
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
    for (uint32_t j = 1; j < ncol; ++ j) {
	out << del;
	ierr = clist[j]->dump(out, 0U);
	if (ierr < 0) return ierr;
    }
    out << "\n";
    if (! out) return -4;
    // print the remaining rows without checking the return values from
    // functions called
    if (nr > nEvents) nr = nEvents;
    for (uint32_t i = 1; i < nr; ++ i) {
	(void) clist[0]->dump(out, i);
	for (uint32_t j = 1; j < ncol; ++ j) {
	    out << del;
	    (void) clist[j]->dump(out, i);
	}
	out << "\n";
    }
    if (! out)
	ierr = -4;
    return ierr;
} // ibis::bord::dump

/// Print nr rows starting with row offset.  Note that the row number
/// starts with 0, i.e., the first row is row 0.
int ibis::bord::dump(std::ostream& out, uint64_t offset, uint64_t nr,
		     const char* del) const {
    const uint32_t ncol = columns.size();
    if (ncol == 0 || nr == 0 || offset >= nEvents) return 0;
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
	for (uint32_t i = 0; i < ncol; ++ i) {
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
	for (uint32_t i = 0; i < colorder.size(); ++ i) {
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
    // print row offset with error checking
    ierr = clist[0]->dump(out, offset);
    if (ierr < 0) return ierr;
    for (uint32_t j = 1; j < ncol; ++ j) {
	out << del;
	ierr = clist[j]->dump(out, offset);
	if (ierr < 0) return ierr;
    }
    out << "\n";
    if (! out) return -4;
    // print the remaining rows without checking the return values from
    // functions called
    nr += offset;
    if (nr > nEvents) nr = nEvents;
    for (uint32_t i = offset+1; i < nr; ++ i) {
	(void) clist[0]->dump(out, i);
	for (uint32_t j = 1; j < ncol; ++ j) {
	    out << del;
	    (void) clist[j]->dump(out, i);
	}
	out << "\n";
    }
    if (! out)
	ierr = -4;
    return ierr;
} // ibis::bord::dump

int ibis::bord::column::dump(std::ostream& out, uint32_t i) const {
    int ierr = -1;
    if (buffer == 0) {
	out << "(no data in memory)";
	return ierr;
    }

    switch (m_type) {
    case ibis::BYTE: {
	const array_t<signed char>* vals =
	    static_cast<const array_t<signed char>*>(buffer);
	if (i < vals->size()) {
	    out << (int)((*vals)[i]);
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>* vals =
	    static_cast<const array_t<unsigned char>*>(buffer);
	if (i < vals->size()) {
	    out << (unsigned)((*vals)[i]);
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>* vals =
	    static_cast<const array_t<int16_t>*>(buffer);
	if (i < vals->size()) {
	    out << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>* vals =
	    static_cast<const array_t<uint16_t>*>(buffer);
	if (i < vals->size()) {
	    out << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::INT: {
	const array_t<int32_t>* vals =
	    static_cast<const array_t<int32_t>*>(buffer);
	if (i < vals->size()) {
	    out << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::UINT: {
	const array_t<uint32_t>* vals =
	    static_cast<const array_t<uint32_t>*>(buffer);
	if (i < vals->size()) {
	    if (dic == 0) {
		out << (*vals)[i];
	    }
	    else if ((*vals)[i] > dic->size()) {
		out << (*vals)[i];
	    }
	    else if ((*vals)[i] > 0) {
		out << (*dic)[(*vals)[i]];
	    }
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::LONG: {
	const array_t<int64_t>* vals =
	    static_cast<const array_t<int64_t>*>(buffer);
	if (i < vals->size()) {
	    out << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>* vals =
	    static_cast<const array_t<uint64_t>*>(buffer);
	if (i < vals->size()) {
	    out << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::FLOAT: {
	const array_t<float>* vals =
	    static_cast<const array_t<float>*>(buffer);
	if (i < vals->size()) {
	    out << std::setprecision(7) << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::DOUBLE: {
	const array_t<double>* vals =
	    static_cast<const array_t<double>*>(buffer);
	if (i < vals->size()) {
	    out << std::setprecision(15) << (*vals)[i];
	    ierr = 0;
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::CATEGORY:
    case ibis::TEXT: {
	std::string tmp;
	getString(i, tmp);
	out << '"' << tmp << '"';
	ierr = 0;
	break;}
    default: {
	ierr = -2;
	break;}
    }
    return ierr;
} // ibis::bord::column::dump

/// Write the content of partition into the specified directory dir.  The
/// directory dir must be writable.  If the second and third arguments are
/// valid, the output data will use them as the name and the description of
/// the data partition.
int ibis::bord::backup(const char* dir, const char* tname,
		       const char* tdesc) const {
    if (dir == 0 || *dir == 0) return -1;
    int ierr = ibis::util::makeDir(dir);
    if (ierr < 0) return ierr;

    if (tname == 0 || *tname == 0)
	tname = m_name;
    if (tdesc == 0 || *tdesc == 0)
	tdesc = m_desc.c_str();
    LOGGER(ibis::gVerbose > 1)
	<< "bord::backup starting to write " << nEvents << " row"
	<< (nEvents>1?"s":"") << " and " << columns.size() << " column"
	<< (columns.size()>1?"s":"") << " to " << dir << " as data partition "
	<< tname << " to " << dir;
    ibis::horometer timer;
    if (ibis::gVerbose > 0)
	timer.start();

    time_t currtime = time(0); // current time
    char stamp[28];
    ibis::util::secondsToString(currtime, stamp);
    std::string mdfile = dir;
    mdfile += FASTBIT_DIRSEP;
    mdfile += "-part.txt";
    std::ofstream md(mdfile.c_str());
    if (! md) {
	LOGGER(ibis::gVerbose > 0)
	    << "bord::backup(" << dir << ") failed to open metadata file "
	    "\"-part.txt\"";
	return -3; // metadata file not ready
    }

    ibis::bitvector msk0, msk1;
    msk0.set(1, nEvents);
    md << "# meta data for data partition " << tname
       << " written by bord::backup on " << stamp << "\n\n"
       << "BEGIN HEADER\nName = " << tname << "\nDescription = "
       << tdesc << "\nNumber_of_rows = " << nEvents
       << "\nNumber_of_columns = " << columns.size()
       << "\nTimestamp = " << currtime;
    if (idxstr != 0 && *idxstr != 0) {
	md << "\nindex = " << idxstr;
    }
    else { // try to find the default index specification
	std::string idxkey = "ibis.";
	idxkey += tname;
	idxkey += ".index";
	const char* str = ibis::gParameters()[idxkey.c_str()];
	if (str != 0 && *str != 0)
	    md << "\nindex = " << str;
    }
    md << "\nEND HEADER\n";

    for (columnList::const_iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	const column& col = *(static_cast<column*>((*it).second));
	std::string cnm = dir;
	cnm += FASTBIT_DIRSEP;
	cnm += (*it).first;
	int fdes = UnixOpen(cnm.c_str(), OPEN_WRITEADD, OPEN_FILEMODE);
	if (fdes < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "bord::backup(" << dir << ") failed to open file "
		<< cnm << " for writing";
	    return -4;
	}
	IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdes, _O_BINARY);
#endif
	LOGGER(ibis::gVerbose > 2)
	    << "bord::backup opened file " << cnm
	    << " to write data for column " << (*it).first;
	std::string mskfile = cnm; // mask file name
	mskfile += ".msk";
	msk1.clear();

	switch (col.type()) {
	case ibis::BYTE: {
	    array_t<signed char> *values = col.selectBytes(msk0);
	    if (values != 0) {
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values,
		     (signed char)0x7F, msk1, msk0);
	    }
	    else {
		ierr = -4;
	    }
	    break;}
	case ibis::UBYTE: {
	    array_t<unsigned char> *values = col.selectUBytes(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values,
		     (unsigned char)0xFF, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::SHORT: {
	    array_t<int16_t> *values = col.selectShorts(msk0);
	    if (values != 0) 
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values,
		     (int16_t)0x7FFF, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::USHORT: {
	    array_t<uint16_t> *values = col.selectUShorts(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values,
		     (uint16_t)0xFFFF, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::INT: {
	    array_t<int32_t> *values = col.selectInts(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values,
		     (int32_t)0x7FFFFFFF, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::UINT: {
	    array_t<uint32_t> *values = col.selectUInts(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values,
		     (uint32_t)0xFFFFFFFF, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *values = col.selectLongs(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn<int64_t>
		    (fdes, 0, nEvents, *values,
		     0x7FFFFFFFFFFFFFFFLL, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *values = col.selectULongs(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn<uint64_t>
		    (fdes, 0, nEvents, *values, 0xFFFFFFFFFFFFFFFFULL,
		     msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *values = col.selectFloats(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values, FASTBIT_FLOAT_NULL, msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *values = col.selectDoubles(msk0);
	    if (values != 0)
		ierr = ibis::part::writeColumn
		    (fdes, 0, nEvents, *values, FASTBIT_DOUBLE_NULL,
		     msk1, msk0);
	    else
		ierr = -4;
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
	    std::vector<std::string> *values = col.selectStrings(msk0);
	    if (values != 0)
		ierr = ibis::part::writeString
		    (fdes, 0, nEvents, *values, msk1, msk0);
	    else
		ierr =-4;
	    break;}
	    // 	case ibis::BLOB: {
	    // 	    std::string spname = cnm;
	    // 	    spname += ".sp";
	    // 	    int sdes = UnixOpen(spname.c_str(), OPEN_READWRITE, OPEN_FILEMODE);
	    // 	    if (sdes < 0) {
	    // 		LOGGER(ibis::gVerbose >= 0)
	    // 		    << "bord::backup(" << dir << ") failed to open file "
	    // 		    << spname << " for writing the starting positions";
	    // 		return -4;
	    // 	    }
	    // 	    ibis::util::guard gsdes = ibis::util::makeGuard(UnixClose, sdes);
	    // #if defined(_WIN32) && defined(_MSC_VER)
	    // 	    (void)_setmode(sdes, _O_BINARY);
	    // #endif

	    // 	    ierr = ibis::part::writeRaw
	    // 		(fdes, sdes, 0, nEvents,
	    // 		 * static_cast<const array_t<unsigned char>*>(values),
	    // 		 col.starts, msk1, msk0);
	    // 	    break;}
	default:
	    break;
	}
#if defined(FASTBIT_SYNC_WRITE)
#if _POSIX_FSYNC+0 > 0
	(void) UnixFlush(fdes); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
	(void) _commit(fdes);
#endif
#endif
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "bord::backup(" << dir << ") failed to write column "
		<< (*it).first << " (type " << ibis::TYPESTRING[(int)col.type()]
		<< ") to " << cnm;
	    return ierr;
	}

	LOGGER(msk1.cnt() != msk1.size() && ibis::gVerbose > 1)
	    << "Warning -- bord::backup(" << dir
	    << ") expected msk1 to contain only 1s for column " << col.name()
	    << ", but it has only " << msk1.cnt() << " out of " << msk1.size();
	remove(mskfile.c_str());

	md << "\nBegin Column\nname = " << (*it).first << "\ndata_type = "
	   << ibis::TYPESTRING[(int) col.type()];
	if (col.indexSpec() != 0 && *(col.indexSpec()) != 0) {
	    md << "\nindex = " << col.indexSpec();
	}
	else if (col.type() == ibis::TEXT) {
	    md << "\nindex=none";
	}
	else {
	    std::string idxkey = "ibis.";
	    idxkey += tname;
	    idxkey += ".";
	    idxkey += (*it).first;
	    idxkey += ".index";
	    const char* str = ibis::gParameters()[idxkey.c_str()];
	    if (str != 0)
		md << "\nindex = " << str;
	}
	md << "\nEnd Column\n";
    }
    md.close(); // close the file
    ibis::fileManager::instance().flushDir(dir);
    if (ibis::gVerbose > 0) {
	timer.stop();
	ibis::util::logger()()
	    << "bord::backup completed writing partition " 
	    << tname << " (" << tdesc << ") with " << columns.size()
	    << " column" << (columns.size()>1 ? "s" : "") << " and "
	    << nEvents << " row" << (nEvents>1 ? "s" : "") << ") to " << dir
	    << " using " << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed)";
    }

    return 0;
} // ibis::bord::backup

ibis::table* ibis::bord::groupby(const char* keys) const {
    ibis::selectClause sel(keys);
    return groupby(sel);
} // ibis::bord::groupby

ibis::table* 
ibis::bord::groupby(const ibis::table::stringList& keys) const {
    ibis::selectClause sel(keys);
    return groupby(sel);
} // ibis::bord::groupby

/// The actual function to perform the group by operation.
///
/// @note The input argument can only contain column names and supported
/// aggregation functions with column names arguments.  No futher
/// arithmetic operations are allowed!
ibis::table*
ibis::bord::xgroupby(const ibis::selectClause& sel) const {
    if (sel.empty())
	return 0;

    std::string td = "GROUP BY ";
    td += *sel;
    td += " on table ";
    td += m_name;
    td += " (";
    td += m_desc;
    td += ')';
    LOGGER(ibis::gVerbose > 3) << "bord::groupby -- " << td;
    readLock lock(this, td.c_str());
    std::string tn = ibis::util::shortName(td);
    if (nEvents == 0)
	return new ibis::tabula(tn.c_str(), td.c_str(), nEvents);

    // create bundle
    std::auto_ptr<ibis::bundle> bdl(ibis::bundle::create(*this, sel));
    if (bdl.get() == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- bord::groupby failed to create "
	    "bundle for \"" << *sel << "\" from in-memory data";

	return 0;
    }
    const uint32_t nr = bdl->size();
    if (nr == 0) {
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- bord::groupby(";
	    sel.print(lg());
	    lg() << ") produced no answer on a table with nRows = "
		 << nEvents;
	}
	return 0;
    }

    const uint32_t nc1 = sel.aggSize();
    if (nc1 == 0)
	return new ibis::tabula(tn.c_str(), td.c_str(), nr);

    // can we finish this in one run?
    const ibis::selectClause::mathTerms& xtms(sel.getTerms());
    bool onerun = (xtms.size() == sel.aggSize());
    for (unsigned j = 0; j < xtms.size() && onerun; ++ j)
	onerun = (xtms[j]->termType() == ibis::math::VARIABLE);

    // prepare the types and values for the new table
    std::vector<const ibis::dictionary*> dct(nc1, 0);
    std::vector<std::string> nms(nc1), des(nc1);
    ibis::table::stringList  nmc(nc1), dec(nc1);
    ibis::table::bufferList  buf(nc1, 0);
    ibis::table::typeList    tps(nc1, ibis::UNKNOWN_TYPE);
    ibis::util::guard gbuf
	= ibis::util::makeGuard(ibis::table::freeBuffers,
				ibis::util::ref(buf),
				ibis::util::ref(tps));
    uint32_t jbdl = 0;
#ifdef FASTBIT_ALWAYS_OUTPUT_COUNTS
    bool countstar = false;
#endif
    for (uint32_t i = 0; i < nc1; ++ i) {
	void *bptr = 0;
	nms[i] = (onerun ? sel.termName(i) : sel.aggName(i));
	nmc[i] = nms[i].c_str();
	des[i] = sel.aggDescription(i);
	dec[i] = des[i].c_str();
	bool iscstar = (sel.aggExpr(i)->termType() == ibis::math::VARIABLE &&
			sel.getAggregator(i) == ibis::selectClause::CNT);
	if (iscstar)
	    iscstar = (*(static_cast<const ibis::math::variable*>
			 (sel.aggExpr(i))->variableName()));
	if (iscstar) {
#ifdef FASTBIT_ALWAYS_OUTPUT_COUNTS
	    countstar = true;
#endif
	    array_t<uint32_t>* cnts = new array_t<uint32_t>;
	    bdl->rowCounts(*cnts);
	    tps[i] = ibis::UINT;
	    buf[i] = cnts;
	    if (! onerun) {
		std::ostringstream oss;
		oss << "__" << std::hex << i;
		nms[i] = oss.str();
		nmc[i] = nms[i].c_str();
	    }
	    continue;
	}
	else if (jbdl < bdl->width()) {
	    tps[i] = bdl->columnType(jbdl);
	    bptr = bdl->columnArray(jbdl);
	    ++ jbdl;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::groupby exhausted all columns in bundle, "
		"not enough information to construct the result table";
	    return 0;
	}

	if (bptr == 0) {
	    buf[i] = 0;
	    continue;
	}

	switch (tps[i]) {
	case ibis::BYTE:
	    buf[i] = new array_t<signed char>
		(* static_cast<const array_t<signed char>*>(bptr));
	    break;
	case ibis::UBYTE:
	    buf[i] = new array_t<unsigned char>
		(* static_cast<const array_t<unsigned char>*>(bptr));
	    break;
	case ibis::SHORT:
	    buf[i] = new array_t<int16_t>
		(* static_cast<const array_t<int16_t>*>(bptr));
	    break;
	case ibis::USHORT: {
	    buf[i] = new array_t<uint16_t>
		(* static_cast<const array_t<uint16_t>*>(bptr));
	    break;}
	case ibis::INT:
	    buf[i] = new array_t<int32_t>
		(* static_cast<const array_t<int32_t>*>(bptr));
	    break;
	case ibis::UINT: {
	    buf[i] = new array_t<uint32_t>
		(* static_cast<const array_t<uint32_t>*>(bptr));
	    const ibis::bord::column *bc =
		dynamic_cast<const ibis::bord::column*>
		(bdl->columnPointer(jbdl));
	    if (bc != 0)
		dct[i] = bc->getDictionary();
	    break;}
	case ibis::LONG:
	    buf[i] = new array_t<int64_t>
		(* static_cast<const array_t<int64_t>*>(bptr));
	    break;
	case ibis::ULONG:
	    buf[i] = new array_t<uint64_t>
		(* static_cast<const array_t<uint64_t>*>(bptr));
	    break;
	case ibis::FLOAT:
	    buf[i] = new array_t<float>
		(* static_cast<const array_t<float>*>(bptr));
	    break;
	case ibis::DOUBLE:
	    buf[i] = new array_t<double>
		(* static_cast<const array_t<double>*>(bptr));
	    break;
	case ibis::TEXT: {
	    std::vector<std::string> &bstr =
		* static_cast<std::vector<std::string>*>(bptr);
	    std::vector<std::string> *tmp =
		new std::vector<std::string>(bstr.size());
	    for (uint32_t j = 0; j < bstr.size(); ++ j)
		(*tmp)[j] = bstr[j];
	    buf[i] = tmp;
	    break;}
	case ibis::CATEGORY: {
	    buf[i] = new array_t<uint32_t>
		(* static_cast<const array_t<uint32_t>*>(bptr));
	    dct[i] = static_cast<const ibis::category*>
		(bdl->columnPointer(jbdl))->getDictionary();
	    break;}
	default:
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << td << " can not process column "
		<< nms[i] << " (" << des[i] << ") of type "
		<< ibis::TYPESTRING[static_cast<int>(tps[i])];
	    buf[i] = 0;
	    break;
	}
    }
#ifdef FASTBIT_ALWAYS_OUTPUT_COUNTS
    if (! countstar) {// if count(*) is not already there, add it
	array_t<uint32_t>* cnts = new array_t<uint32_t>;
	bdl->rowCounts(*cnts);
	nmc.push_back("count0");
	dec.push_back("COUNT(*)");
	tps.push_back(ibis::UINT);
	buf.push_back(cnts);
    }
#endif
    std::auto_ptr<ibis::bord>
	brd1(new ibis::bord(tn.c_str(), td.c_str(), nr, buf, tps, nmc, &dec,
			    &dct));
    if (brd1.get() == 0)
	return 0;
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg() << "bord::groupby -- creates an in-memory data partition with "
	     << brd1->nRows() << " row" << (brd1->nRows()>1?"s":"")
	     << " and " << brd1->nColumns() << " column"
	     << (brd1->nColumns()>1?"s":"");
	if (ibis::gVerbose > 4) {
	    lg() << "\n";
	    brd1->describe(lg());
	}
    }

    delete bdl.release(); // free the bundle
    if (onerun) {
	gbuf.dismiss();
	return brd1.release();
    }

    // not quite done yet, evaluate the top-level arithmetic expressions
    ibis::bitvector msk;
    const unsigned nc2 = xtms.size();
    msk.set(1, brd1->nRows());
    nms.resize(nc2);
    des.resize(nc2);
    nmc.resize(nc2);
    dec.resize(nc2);
    buf.resize(nc2);
    tps.resize(nc2);
    dct.resize(nc2);
    for (unsigned j = 0; j < nc2; ++ j) {
	tps[j] = ibis::UNKNOWN_TYPE;
	buf[j] = 0;
	dct[j] = 0;
    }

    for (unsigned j = 0; j < nc2; ++ j) {
	nms[j] = sel.termName(j);
	nmc[j] = nms[j].c_str();
	const ibis::math::term* tm = xtms[j];
	if (tm == 0 || tm->termType() == ibis::math::UNDEF_TERM) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- bord[" << name_ << "]::groupby(" << sel
		<< ") failed to process term " << j << " named \"" << nms[j]
		<< '"';
	    return 0;
	}

	std::ostringstream oss;
	oss << *tm;
	des[j] = oss.str();
	dec[j] = des[j].c_str();

	switch (tm->termType()) {
	default: {
	    tps[j] = ibis::DOUBLE;
	    buf[j] = new ibis::array_t<double>;
	    brd1->calculate(*tm, msk, *static_cast<array_t<double>*>(buf[j]));
	    break;}
	case ibis::math::NUMBER: {
	    tps[j] = ibis::DOUBLE;
	    buf[j] = new ibis::array_t<double>(nr, tm->eval());
	    break;}
	case ibis::math::STRING: {
	    tps[j] = ibis::CATEGORY;
	    const std::string val = (const char*)*
		(static_cast<const ibis::math::literal*>(tm));
	    buf[j] = new std::vector<std::string>(nr, val);
	    break;}
	case ibis::math::VARIABLE: {
	    const char* var =
		static_cast<const ibis::math::variable*>(tm)->variableName();
	    brd1->copyColumn(var, tps[j], buf[j], dct[j]);
	    break;}
	}
    }

    std::auto_ptr<ibis::table>
	brd2(new ibis::bord(tn.c_str(), td.c_str(), nr, buf, tps, nmc, &dec,
			    &dct));
    if (brd2.get() != 0)
	// buf has been successfully transferred to the new table object
	gbuf.dismiss();
    return brd2.release();
} // ibis::bord::xgroupby

ibis::table*
ibis::bord::groupby(const ibis::selectClause& sel) const {
    std::auto_ptr<ibis::bord> brd1(ibis::bord::groupbya(*this, sel));
    if (brd1.get() == 0)
	return 0;

    // can we finish this in one run?
    const ibis::selectClause::mathTerms& xtms(sel.getTerms());
    bool onerun = (xtms.size() == sel.aggSize());
    for (unsigned j = 0; j < xtms.size() && onerun; ++ j)
	onerun = (xtms[j]->termType() == ibis::math::VARIABLE);
    if (onerun) {
	brd1->renameColumns(sel);
	if (ibis::gVerbose > 2) {
	    ibis::util::logger lg;
	    lg() << "bord::groupby -- completed ";
	    brd1->describe(lg());
	}

	return brd1.release();
    }

    std::auto_ptr<ibis::bord> brd2(ibis::bord::groupbyc(*brd1, sel));
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg() << "bord::groupby -- completed ";
	brd2->describe(lg());
    }

    return brd2.release();
} // ibis::bord::groupby

/// Perform the aggregation operations specified in the select clause.  If
/// there is any further computation on the aggregated values, the user
/// need to call groupbyc to complete those operations.  This separation
/// allows one to possibly conduct group by operations on multiple data
/// partitions on partition at a time, which should reduce the memory
/// requirement.
ibis::bord*
ibis::bord::groupbya(const ibis::bord& src, const ibis::selectClause& sel) {
    if (sel.empty() || sel.aggSize() == 0 || src.nRows() == 0)
	return 0;

    std::string td = "GROUP BY ";
    td += sel.aggDescription(0);
    for (unsigned j = 1; j < sel.aggSize(); ++ j) {
	td += ", ";
	td += sel.aggDescription(j);
    }
    td += " on table ";
    td += src.part::name();
    LOGGER(ibis::gVerbose > 3)
	<< "bord::groupbya -- processing aggregations for " << td;
    std::string tn = ibis::util::randName(td);

    readLock lock(&src, td.c_str());
    // create bundle
    std::auto_ptr<ibis::bundle> bdl(ibis::bundle::create(src, sel));
    if (bdl.get() == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- bord::groupbya failed to create bundle for \""
	    << td << "\"";
	return 0;
    }
    const uint32_t nr = bdl->size();
    if (nr == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- bord::groupbya produced no answer for " << td;
	return 0;
    }

    // prepare the types and values for the new table
    const uint32_t nca = sel.aggSize();
    std::vector<const ibis::dictionary*> dct(nca, 0);
    std::vector<std::string> nms(nca), des(nca);
    ibis::table::stringList  nmc(nca), dec(nca);
    ibis::table::bufferList  buf(nca, 0);
    ibis::table::typeList    tps(nca, ibis::UNKNOWN_TYPE);
    IBIS_BLOCK_GUARD(ibis::table::freeBuffers, ibis::util::ref(buf),
		     ibis::util::ref(tps));
    uint32_t jbdl = 0;
#ifdef FASTBIT_ALWAYS_OUTPUT_COUNTS
    bool countstar = false;
#endif
    for (uint32_t i = 0; i < nca; ++ i) {
	void *bptr = 0;
	nms[i] = sel.aggName(i);
	nmc[i] = nms[i].c_str();
	des[i] = sel.aggDescription(i);
	dec[i] = des[i].c_str();
	const ibis::column *refcol = (jbdl < bdl->width() ?
				      bdl->columnPointer(jbdl) : 0);
	bool iscstar = (sel.aggExpr(i)->termType() == ibis::math::VARIABLE &&
			sel.getAggregator(i) == ibis::selectClause::CNT);
	if (iscstar)
	    iscstar = (*(static_cast<const ibis::math::variable*>
			 (sel.aggExpr(i))->variableName()) == '*');
	if (iscstar) {
#ifdef FASTBIT_ALWAYS_OUTPUT_COUNTS
	    countstar = true;
#endif
	    array_t<uint32_t>* cnts = new array_t<uint32_t>;
	    bdl->rowCounts(*cnts);
	    tps[i] = ibis::UINT;
	    buf[i] = cnts;
	    continue;
	}
	else if (jbdl < bdl->width()) {
	    tps[i] = bdl->columnType(jbdl);
	    bptr = bdl->columnArray(jbdl);
	    ++ jbdl;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::groupbya exhausted columns in bundle, "
		"not enough information to construct the result table";
	    return 0;
	}

	if (bptr == 0) {
	    buf[i] = 0;
	    continue;
	}

	switch (tps[i]) {
	case ibis::BYTE:
	    buf[i] = new array_t<signed char>
		(* static_cast<const array_t<signed char>*>(bptr));
	    break;
	case ibis::UBYTE:
	    buf[i] = new array_t<unsigned char>
		(* static_cast<const array_t<unsigned char>*>(bptr));
	    break;
	case ibis::SHORT:
	    buf[i] = new array_t<int16_t>
		(* static_cast<const array_t<int16_t>*>(bptr));
	    break;
	case ibis::USHORT:
	    buf[i] = new array_t<uint16_t>
		(* static_cast<const array_t<uint16_t>*>(bptr));
	    break;
	case ibis::INT:
	    buf[i] = new array_t<int32_t>
		(* static_cast<const array_t<int32_t>*>(bptr));
	    break;
	case ibis::UINT: {
	    buf[i] = new array_t<uint32_t>
		(* static_cast<const array_t<uint32_t>*>(bptr));
	    if (refcol->type() == ibis::CATEGORY) {
		dct[i] = static_cast<const ibis::category*>(refcol)
		    ->getDictionary();
	    }
	    else {
		const ibis::bord::column *bc =
		    dynamic_cast<const ibis::bord::column*>(refcol);
		if (bc != 0)
		    dct[i] = bc->getDictionary();
	    }
	    break;}
	case ibis::LONG:
	    buf[i] = new array_t<int64_t>
		(* static_cast<const array_t<int64_t>*>(bptr));
	    break;
	case ibis::ULONG:
	    buf[i] = new array_t<uint64_t>
		(* static_cast<const array_t<uint64_t>*>(bptr));
	    break;
	case ibis::FLOAT:
	    buf[i] = new array_t<float>
		(* static_cast<const array_t<float>*>(bptr));
	    break;
	case ibis::DOUBLE:
	    buf[i] = new array_t<double>
		(* static_cast<const array_t<double>*>(bptr));
	    break;
	case ibis::TEXT: {
	    std::vector<std::string> &bstr =
		* static_cast<std::vector<std::string>*>(bptr);
	    std::vector<std::string> *tmp =
		new std::vector<std::string>(bstr.size());
	    for (uint32_t j = 0; j < bstr.size(); ++ j)
		(*tmp)[j] = bstr[j];
	    buf[i] = tmp;
	    break;}
	case ibis::CATEGORY: { // stored as UINT, copy pointer to dictionary
	    buf[i] = new array_t<uint32_t>
		(* static_cast<const array_t<uint32_t>*>(bptr));
	    dct[i] = static_cast<const ibis::category*>
		(refcol)->getDictionary();
	    break;}
	default:
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << td << " can not process column "
		<< nms[i] << " (" << des[i] << ") of type "
		<< ibis::TYPESTRING[static_cast<int>(tps[i])];
	    buf[i] = 0;
	    break;
	}
    }
#ifdef FASTBIT_ALWAYS_OUTPUT_COUNTS
    if (! countstar) {// if count(*) is not already there, add it
	array_t<uint32_t>* cnts = new array_t<uint32_t>;
	bdl->rowCounts(*cnts);
	nmc.push_back("count0");
	dec.push_back("COUNT(*)");
	tps.push_back(ibis::UINT);
	buf.push_back(cnts);
    }
#endif
    return (new ibis::bord(tn.c_str(), td.c_str(), nr, buf, tps, nmc, &dec,
			   &dct));
} // ibis::bord::groupbya

/// The function to perform the final computations specified by the select
/// clause.  This is to be called after all the aggregation operations have
/// been performed.  The objective to separate the aggregation operations
/// and the final arithmetic operations is to allow the aggregation
/// operations to be performed on different data partitions separately.
///
/// @note The incoming ibis::bord object must be the output from
/// ibis::bord::groupbya.
ibis::bord*
ibis::bord::groupbyc(const ibis::bord& src, const ibis::selectClause& sel) {
    if (sel.empty())
	return 0;

    const unsigned nr = src.nRows();
    const unsigned ncx = sel.numTerms();
    if (nr == 0 || ncx == 0)
	return 0;

    std::string td = "GROUP BY ";
    td += *sel;
    td += " on table ";
    td += src.part::name();
    td += " (";
    td += src.part::description();
    td += ')';
    LOGGER(ibis::gVerbose > 3)
	<< "bord::groupbyc -- starting the final computations for "
	<< td;
    readLock lock(&src, td.c_str());
    std::string tn = ibis::util::shortName(td);

    // prepare the types and values for the new table
    const ibis::selectClause::mathTerms& xtms(sel.getTerms());
    std::vector<const ibis::dictionary*> dct(ncx, 0);
    std::vector<std::string> nms(ncx), des(ncx);
    ibis::table::stringList  nmc(ncx), dec(ncx);
    ibis::table::bufferList  buf(ncx, 0);
    ibis::table::typeList    tps(ncx, ibis::UNKNOWN_TYPE);
    IBIS_BLOCK_GUARD(ibis::table::freeBuffers, ibis::util::ref(buf),
		     ibis::util::ref(tps));
    ibis::bitvector msk;
    msk.set(1, src.nRows());

    for (unsigned j = 0; j < ncx; ++ j) {
	nms[j] = sel.termName(j);
	nmc[j] = nms[j].c_str();
	const ibis::math::term* tm = xtms[j];
	if (tm == 0 || tm->termType() == ibis::math::UNDEF_TERM) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- bord::groupbyc(" << td
		<< ") failed to process term " << j << " named \"" << nms[j]
		<< '"';
	    return 0;
	}

	std::ostringstream oss;
	oss << *tm;
	des[j] = oss.str();
	dec[j] = des[j].c_str();

	switch (tm->termType()) {
	default: {
	    tps[j] = ibis::DOUBLE;
	    buf[j] = new ibis::array_t<double>;
	    src.calculate(*tm, msk, *static_cast<array_t<double>*>(buf[j]));
	    break;}
	case ibis::math::NUMBER: {
	    tps[j] = ibis::DOUBLE;
	    buf[j] = new ibis::array_t<double>(nr, tm->eval());
	    break;}
	case ibis::math::STRING: {
	    tps[j] = ibis::CATEGORY;
	    const std::string val = (const char*)*
		(static_cast<const ibis::math::literal*>(tm));
	    buf[j] = new std::vector<std::string>(nr, val);
	    break;}
	case ibis::math::VARIABLE: {
	    const char* var =
		static_cast<const ibis::math::variable*>(tm)->variableName();
	    src.copyColumn(var, tps[j], buf[j], dct[j]);
	    break;}
	}
    }

    return(new ibis::bord(tn.c_str(), td.c_str(), nr, buf, tps, nmc, &dec,
			  &dct));
} // ibis::bord::groupbyc

/// Merge the incoming data partition with this one.  This function is
/// intended to combine partial results produced by ibis::bord::groupbya;
/// both this and rhs must be produced with the same select clause sel.  It
/// only work with separable aggregation operators.
///
/// It returns the number of rows in the combined result upon a successful
/// completion, otherwise, it returns a negative number.
int ibis::bord::merge(const ibis::bord &rhs, const ibis::selectClause& sel) {
    int ierr = -1;
    if (columns.size() != rhs.columns.size() ||
	columns.size() != sel.aggSize()) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- bord::merge expects the same number of columns in "
	    << this->part::name() << " (" << nColumns() << "), "
	    << rhs.part::name() << " (" << rhs.nColumns()
	    << ") and the select clauses (" << sel.aggSize() << ")";
	return -1;
    }

    // divide the columns into keys and vals
    std::vector<ibis::bord::column*> keys, keyr, vals, valr;
    std::vector<ibis::selectClause::AGREGADO> agg;
    for (unsigned i = 0; i < sel.aggSize(); ++ i) {
	const char* nm = sel.aggName(i);
	ibis::bord::column *cs =
	    dynamic_cast<ibis::bord::column*>(getColumn(nm));
	ibis::bord::column *cr =
	    dynamic_cast<ibis::bord::column*>(rhs.getColumn(nm));
	if (cs == 0 || cr == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::merge expects a column named " << nm
		<< " from data partition " << this->part::name() << " and "
		<< rhs.part::name();
	    return -2;
	}
	if (cs->type() != cr->type()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::merge expects the columns named " << nm
		<< " from data partition " << this->part::name() << " and "
		<< rhs.part::name() << " to have the same type";
	    return -3;
	}
	if (cs->getArray() == 0 || cr->getArray() == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::merge column " << nm
		<< " from data partition " << this->part::name() << " and "
		<< rhs.part::name() << " must have data in memory";
	    return -4;
	}

	ibis::selectClause::AGREGADO a0 = sel.getAggregator(i);
	if (a0 == ibis::selectClause::NIL_AGGR) { // a group-by key
	    keys.push_back(cs);
	    keyr.push_back(cr);
	}
	else if (a0 == ibis::selectClause::CNT ||
		 a0 == ibis::selectClause::SUM ||
		 a0 == ibis::selectClause::MAX ||
		 a0 == ibis::selectClause::MIN) { // a separable operator
	    agg.push_back(a0);
	    vals.push_back(cs);
	    valr.push_back(cr);
	}
	else { // can not deal with this operator in this function
	    return -5;
	}
    }
    if (keys.size() != keyr.size() || vals.size() != valr.size())
	return -2;
    if (ibis::gVerbose > 4) {
	ibis::util::logger lg;
	lg() << "bord::merge -- merging " << this->part::name() << " ("
	     << nRows() << ") and " << rhs.part::name() << " ("
	     << rhs.nRows() << ") into " << this->part::name() << " using ";
	if (keys.size() == 0) {
	    lg() << "no keys";
	}
	else {
	    lg() << '(' << keys[0]->name();
	    for (unsigned j = 1; j < keys.size(); ++ j)
		lg() << ", " << keys[j]->name();
	    lg() << ") as keys";
	}
    }

    bool match = (this->part::nRows() == rhs.part::nRows());
    for (uint32_t jc = 0; match && jc < keys.size(); ++ jc) {
	match = keys[jc]->equal_to(*keyr[jc]);
    }

    if (match) { // all the keys match, work on the columns one at a time
	ierr = merge0(vals, valr, agg);
    }
    else {
	if (keys.size() == 1) {
	    if (vals.size() == 1)
		ierr = merge11(*keys[0], *vals[0], *keyr[0], *valr[0], agg[0]);
	    else if (vals.size() == 2)
		ierr = merge12(*keys[0], *vals[0], *vals[1],
			       *keyr[0], *valr[0], *valr[1],
			       agg[0], agg[1]);
	    else
		ierr = merge10(*keys[0], vals, *keyr[0], valr, agg);
	}
	else if (keys.size() == 2) {
	    if (vals.size() == 1) {
		ierr = merge21(*keys[0], *keys[1], *vals[0],
			       *keyr[0], *keyr[1], *valr[0], agg[0]);
	    }
	    else {
		ierr = merge20(*keys[0], *keys[1], vals,
			       *keyr[0], *keyr[1], valr, agg);
	    }
	}
	else { // a generic version
	    ierr = merger(keys, vals, keyr, valr, agg);
	}

	// update the number of rows
	if (ierr > 0)
	    nEvents = ierr;
	else
	    nEvents = 0;
    }
    return ierr;
} // ibis::bord::merge

/// Merge values from two partial results and place the final resules in
/// the first argument.  This is the most generic version that expects the
/// keys to not match and therefore needs to produce a new set of values.
/// It also uses the generic algorithm for comparisons, where each
/// comparison of a pair of values requires a function call.
int ibis::bord::merger(std::vector<ibis::bord::column*> &keys,
		       std::vector<ibis::bord::column*> &vals,
		       const std::vector<ibis::bord::column*> &keyr,
		       const std::vector<ibis::bord::column*> &valr,
		       const std::vector<selectClause::AGREGADO> &agg) {
    // number of columns must match, their types must match
    if (keys.size() != keyr.size() || vals.size() != valr.size() ||
	vals.size() != agg.size())
	return -1;
    for (unsigned j = 0; j < keyr.size(); ++ j) {
	if (keys[j]->type() != keyr[j]->type() ||
	    keys[j]->getArray() == 0 || keyr[j]->getArray() == 0)
	    return -2;
    }
    for (unsigned j = 0; j < agg.size(); ++ j) {
	if (vals[j]->type() != valr[j]->type() ||
	    vals[j]->getArray() == 0 || valr[j]->getArray() == 0)
	    return -3;
	if (agg[j] != ibis::selectClause::CNT &&
	    agg[j] != ibis::selectClause::SUM &&
	    agg[j] != ibis::selectClause::MIN &&
	    agg[j] != ibis::selectClause::MAX)
	    return -4;
    }

    // make a copy of keys and vals as keyt and valt
    std::vector<ibis::bord::column*> keyt, valt;
    IBIS_BLOCK_GUARD(ibis::util::clearVec<ibis::bord::column>,
		     ibis::util::ref(keyt));
    IBIS_BLOCK_GUARD(ibis::util::clearVec<ibis::bord::column>,
		     ibis::util::ref(valt));
    for (unsigned j = 0; j < keys.size(); ++ j) {
	keyt.push_back(new ibis::bord::column(*keys[j]));
	keys[j]->limit(0);
    }
    for (unsigned j = 0; j < vals.size(); ++ j) {
	valt.push_back(new ibis::bord::column(*vals[j]));
	vals[j]->limit(0);
    }

    int ierr = 0;
    uint32_t ir = 0, it = 0;
    const uint32_t nk = keyr.size();
    const uint32_t nv = valr.size();
    const uint32_t nr = keyr[0]->partition()->nRows();
    const uint32_t nt = keyt[0]->partition()->nRows();
    while (ir < nr && it < nt) {
	bool match = true;
	uint32_t j0 = 0;
	while (match && j0 < nk) {
	    if (keyt[j0]->equal_to(*keyr[j0], it, ir))
		j0 += 1;
	    else
		match = false;
	}
	if (match) {
	    for (unsigned j1 = 0; j1 < nk; ++ j1)
		keys[j1]->append(keyt[j1]->getArray(), it);
	    for (unsigned j1 = 0; j1 < nv; ++ j1)
		vals[j1]->append(valt[j1]->getArray(), it,
				 valr[j1]->getArray(), ir, agg[j1]);
	    ++ it;
	    ++ ir;
	}
	else if (keyt[j0]->less_than(*keyr[j0], it, ir)) {
	    for (unsigned j1 = 0; j1 < nk; ++ j1)
		keys[j1]->append(keyt[j1]->getArray(), it);
	    for (unsigned j1 = 0; j1 < nv; ++ j1)
		vals[j1]->append(valt[j1]->getArray(), it);
	    ++ it;
	}
	else {
	    for (unsigned j1 = 0; j1 < nk; ++ j1)
		keys[j1]->append(keyr[j1]->getArray(), ir);
	    for (unsigned j1 = 0; j1 < nv; ++ j1)
		vals[j1]->append(valr[j1]->getArray(), ir);
	    ++ ir;
	}
	++ ierr;
    }

    while (ir < nr) {
	for (unsigned j1 = 0; j1 < nk; ++ j1)
	    keys[j1]->append(keyr[j1]->getArray(), ir);
	for (unsigned j1 = 0; j1 < nv; ++ j1)
	    vals[j1]->append(valr[j1]->getArray(), ir);
	++ ierr;
	++ ir;
    }
    while (it < nt) {
	for (unsigned j1 = 0; j1 < nk; ++ j1)
	    keys[j1]->append(keyt[j1]->getArray(), it);
	for (unsigned j1 = 0; j1 < nv; ++ j1)
	    vals[j1]->append(valt[j1]->getArray(), it);
	++ ierr;
	++ it;
    }
    return ierr;
} // ibis::bord::merger


/// Merge values according to the given operators.  The corresponding
/// group-by keys match, only the values needs to be updated.
int ibis::bord::merge0(std::vector<ibis::bord::column*> &vals,
		       const std::vector<ibis::bord::column*> &valr,
		       const std::vector<selectClause::AGREGADO>& agg) {
    if (vals.size() != valr.size() || vals.size() != agg.size())
	return -6;

    int ierr = 0;
    for (uint32_t jc = 0; jc < agg.size(); ++ jc) {
	if (vals[jc] == 0 || valr[jc] == 0)
	    return -1;
	if (vals[jc]->getArray() == 0 || valr[jc]->getArray() == 0)
	    return -2;
	if (vals[jc]->type() != valr[jc]->type())
	    return -3;

	switch (vals[jc]->type()) {
	case ibis::BYTE:
	    ierr = merge0T(*static_cast<array_t<signed char>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<signed char>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::UBYTE:
	    ierr = merge0T(*static_cast<array_t<unsigned char>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<unsigned char>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::SHORT:
	    ierr = merge0T(*static_cast<array_t<int16_t>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<int16_t>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::USHORT:
	    ierr = merge0T(*static_cast<array_t<uint16_t>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<uint16_t>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::INT:
	    ierr = merge0T(*static_cast<array_t<int32_t>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<int32_t>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::UINT:
	    ierr = merge0T(*static_cast<array_t<uint32_t>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<uint32_t>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::LONG:
	    ierr = merge0T(*static_cast<array_t<int64_t>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<int64_t>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::ULONG:
	    ierr = merge0T(*static_cast<array_t<uint64_t>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<uint64_t>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::FLOAT:
	    ierr = merge0T(*static_cast<array_t<float>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<float>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	case ibis::DOUBLE:
	    ierr = merge0T(*static_cast<array_t<double>*>
			   (vals[jc]->getArray()),
			   *static_cast<array_t<double>*>
			   (valr[jc]->getArray()),
			   agg[jc]);
	    break;
	default:
	    ierr = -5;
	    break;
	}
    }
    return ierr;
} // ibis::bord::merge0

/// Template function to perform the merger operations on arrays with
/// matching keys.
template <typename T> int
ibis::bord::merge0T(ibis::array_t<T>& vs, const ibis::array_t<T>& vr,
		    ibis::selectClause::AGREGADO ag) {
    if (vs.size() != vr.size()) return -11;
    switch (ag) {
    default:
	return -12;
    case ibis::selectClause::CNT:
    case ibis::selectClause::SUM:
	for (size_t j = 0; j < vr.size(); ++ j)
	    vs[j] += vr[j];
	break;
    case ibis::selectClause::MAX:
	for (size_t j = 0; j < vr.size(); ++ j)
	    if (vs[j] < vr[j])
		vs[j] = vr[j];
	break;
    case ibis::selectClause::MIN:
	for (size_t j = 0; j < vr.size(); ++ j)
	    if (vs[j] > vr[j])
		vs[j] = vr[j];
	break;
    }
    return vs.size();
} // ibis::bord::merge0T

/// Merge with one key column and an arbitrary number of value columns.
int ibis::bord::merge10(ibis::bord::column &k1,
			std::vector<ibis::bord::column*> &v1,
			const ibis::bord::column &k2,
			const std::vector<ibis::bord::column*> &v2,
			const std::vector<ibis::selectClause::AGREGADO> &agg) {
    int ierr = -1;
    if (k1.type() != k2.type())
	return ierr;
    if (v1.size() != v2.size() || v1.size() != agg.size())
	return ierr;

    std::vector<ibis::bord::column*> av1(v1.size());
    IBIS_BLOCK_GUARD(ibis::util::clearVec<ibis::bord::column>,
		     ibis::util::ref(av1));
    for (unsigned j = 0; j < v1.size(); ++ j)
	av1[j] = new ibis::bord::column(*v1[j]);

    switch (k1.type()) {
    default:
	return -6;
    case ibis::BYTE: {
	ibis::array_t<signed char> &ak0 =
	    * static_cast<ibis::array_t<signed char>*>(k1.getArray());
	const ibis::array_t<signed char> &ak2 =
	    * static_cast<const ibis::array_t<signed char>*>(k2.getArray());
	const ibis::array_t<signed char> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::UBYTE: {
	ibis::array_t<unsigned char> &ak0 =
	    * static_cast<ibis::array_t<unsigned char>*>(k1.getArray());
	const ibis::array_t<unsigned char> &ak2 =
	    * static_cast<const ibis::array_t<unsigned char>*>(k2.getArray());
	const ibis::array_t<unsigned char> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::SHORT: {
	ibis::array_t<int16_t> &ak0 =
	    * static_cast<ibis::array_t<int16_t>*>(k1.getArray());
	const ibis::array_t<int16_t> &ak2 =
	    * static_cast<const ibis::array_t<int16_t>*>(k2.getArray());
	const ibis::array_t<int16_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::USHORT: {
	ibis::array_t<uint16_t> &ak0 =
	    * static_cast<ibis::array_t<uint16_t>*>(k1.getArray());
	const ibis::array_t<uint16_t> &ak2 =
	    * static_cast<const ibis::array_t<uint16_t>*>(k2.getArray());
	const ibis::array_t<uint16_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::INT: {
	ibis::array_t<int32_t> &ak0 =
	    * static_cast<ibis::array_t<int32_t>*>(k1.getArray());
	const ibis::array_t<int32_t> &ak2 =
	    * static_cast<const ibis::array_t<int32_t>*>(k2.getArray());
	const ibis::array_t<int32_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::UINT: {
	ibis::array_t<uint32_t> &ak0 =
	    * static_cast<ibis::array_t<uint32_t>*>(k1.getArray());
	const ibis::array_t<uint32_t> &ak2 =
	    * static_cast<const ibis::array_t<uint32_t>*>(k2.getArray());
	const ibis::array_t<uint32_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::LONG: {
	ibis::array_t<int64_t> &ak0 =
	    * static_cast<ibis::array_t<int64_t>*>(k1.getArray());
	const ibis::array_t<int64_t> &ak2 =
	    * static_cast<const ibis::array_t<int64_t>*>(k2.getArray());
	const ibis::array_t<int64_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::ULONG: {
	ibis::array_t<uint64_t> &ak0 =
	    * static_cast<ibis::array_t<uint64_t>*>(k1.getArray());
	const ibis::array_t<uint64_t> &ak2 =
	    * static_cast<const ibis::array_t<uint64_t>*>(k2.getArray());
	const ibis::array_t<uint64_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::FLOAT: {
	ibis::array_t<float> &ak0 =
	    * static_cast<ibis::array_t<float>*>(k1.getArray());
	const ibis::array_t<float> &ak2 =
	    * static_cast<const ibis::array_t<float>*>(k2.getArray());
	const ibis::array_t<float> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    case ibis::DOUBLE: {
	ibis::array_t<double> &ak0 =
	    * static_cast<ibis::array_t<double>*>(k1.getArray());
	const ibis::array_t<double> &ak2 =
	    * static_cast<const ibis::array_t<double>*>(k2.getArray());
	const ibis::array_t<double> ak1(ak0);
	ak0.nosharing();
	ierr = merge10T(ak0, v1, ak1, av1, ak2, v2, agg);
	break;}
    }
    return ierr;
} // ibis::bord::merge10

/// Perform merge operation with one key column and an arbitrary number of
/// value columns.
template <typename Tk> int
ibis::bord::merge10T(ibis::array_t<Tk> &kout,
		     std::vector<ibis::bord::column*> &vout,
		     const ibis::array_t<Tk> &kin1,
		     const std::vector<ibis::bord::column*> &vin1,
		     const ibis::array_t<Tk> &kin2,
		     const std::vector<ibis::bord::column*> &vin2,
		     const std::vector<ibis::selectClause::AGREGADO> &agg) {
    int ierr = -1;
    kout.clear();
    for (size_t j = 0; j < vout.size(); ++ j)
	vout[j]->limit(0);
    if (vout.size() != vin1.size() || vout.size() != vin2.size() ||
	vout.size() != agg.size())
	return ierr;

    uint32_t i1 = 0;
    uint32_t i2 = 0;
    while (i1 < kin1.size() && i2 < kin2.size()) {
	if (kin1[i1] == kin2[i2]) {
	    kout.push_back(kin1[i1]);
	    for (unsigned j = 0; j < vin1.size(); ++ j)
		vout[j]->append(vin1[j]->getArray(), i1,
				vin2[j]->getArray(), i2, agg[j]);
	    ++ i1;
	    ++ i2;
	}
	else if (kin1[i1] < kin2[i2]) {
	    kout.push_back(kin1[i1]);
	    for (unsigned j = 0; j < vin1.size(); ++ j)
		vout[j]->append(vin1[j]->getArray(), i1);
	    ++ i1;
	}
	else {
	    kout.push_back(kin2[i2]);
	    for (unsigned j = 0; j < vin2.size(); ++ j)
		vout[j]->append(vin2[j]->getArray(), i2);
	    ++ i2;
	}
    }

    while (i1 < kin1.size()) {
	kout.push_back(kin1[i1]);
	for (unsigned j = 0; j < vin1.size(); ++ j)
	    vout[j]->append(vin1[j]->getArray(), i1);
	++ i1;
    }

    while (i2 < kin2.size()) {
	kout.push_back(kin2[i2]);
	for (unsigned j = 0; j < vin2.size(); ++ j)
	    vout[j]->append(vin2[j]->getArray(), i2);
	++ i2;
    }
    ierr = kout.size();
    return ierr;
} // ibis::bord::merge10T

/// Function to merge one column as key and one column as value.
int ibis::bord::merge11(ibis::bord::column &k1,
			ibis::bord::column &v1,
			const ibis::bord::column &k2,
			const ibis::bord::column &v2,
			ibis::selectClause::AGREGADO agg) {
    if (k1.type() != k2.type() || v1.type() != v2.type()) {
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- bord::merge11 expects the same types and sizes "
	    "for the keys and values";
	return -1;
    }

    int ierr = -1;
    switch (k1.type()) {
    default:
	return -2;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &ak2 =
	    *static_cast<const ibis::array_t<signed char>*>(k2.getArray());
	ibis::array_t<signed char> &ak0 =
	    *static_cast<ibis::array_t<signed char>*>(k1.getArray());
	const ibis::array_t<signed char> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &ak2 =
	    *static_cast<const ibis::array_t<unsigned char>*>(k2.getArray());
	ibis::array_t<unsigned char> &ak0 =
	    *static_cast<ibis::array_t<unsigned char>*>(k1.getArray());
	const ibis::array_t<unsigned char> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &ak2 =
	    *static_cast<const ibis::array_t<int16_t>*>(k2.getArray());
	ibis::array_t<int16_t> &ak0 =
	    *static_cast<ibis::array_t<int16_t>*>(k1.getArray());
	const ibis::array_t<int16_t> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &ak2 =
	    *static_cast<const ibis::array_t<uint16_t>*>(k2.getArray());
	ibis::array_t<uint16_t> &ak0 =
	    *static_cast<ibis::array_t<uint16_t>*>(k1.getArray());
	const ibis::array_t<uint16_t> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &ak2 =
	    *static_cast<const ibis::array_t<int32_t>*>(k2.getArray());
	ibis::array_t<int32_t> &ak0 =
	    *static_cast<ibis::array_t<int32_t>*>(k1.getArray());
	const ibis::array_t<int32_t> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &ak2 =
	    *static_cast<const ibis::array_t<uint32_t>*>(k2.getArray());
	ibis::array_t<uint32_t> &ak0 =
	    *static_cast<ibis::array_t<uint32_t>*>(k1.getArray());
	const ibis::array_t<uint32_t> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &ak2 =
	    *static_cast<const ibis::array_t<int64_t>*>(k2.getArray());
	ibis::array_t<int64_t> &ak0 =
	    *static_cast<ibis::array_t<int64_t>*>(k1.getArray());
	const ibis::array_t<int64_t> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &ak2 =
	    *static_cast<const ibis::array_t<uint64_t>*>(k2.getArray());
	ibis::array_t<uint64_t> &ak0 =
	    *static_cast<ibis::array_t<uint64_t>*>(k1.getArray());
	const ibis::array_t<uint64_t> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &ak2 =
	    *static_cast<const ibis::array_t<float>*>(k2.getArray());
	ibis::array_t<float> &ak0 =
	    *static_cast<ibis::array_t<float>*>(k1.getArray());
	const ibis::array_t<float> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &ak2 =
	    *static_cast<const ibis::array_t<double>*>(k2.getArray());
	ibis::array_t<double> &ak0 =
	    *static_cast<ibis::array_t<double>*>(k1.getArray());
	const ibis::array_t<double> ak1(ak0);
	ak0.nosharing();
	switch (v1.type()) {
	default:
	    return -3;
	case ibis::BYTE: {
	    const ibis::array_t<signed char> &av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char> &av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char> &av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char> &av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t> &av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t> &av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t> &av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t> &av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t> &av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t> &av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t> &av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t> &av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t> &av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t> &av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t> &av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t> &av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float> &av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float> &av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double> &av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double> &av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge11T(ak0, av0, ak1, av1, ak2, av2, agg);
	    break;}
	} // v1.type()
	break;}
    } // k1.type()
    return ierr;
} // ibis::bord::merge11

/// Template to perform merge operation with one column as key and one
/// column as value.
template <typename Tk, typename Tv> int
ibis::bord::merge11T(ibis::array_t<Tk> &kout,
		     ibis::array_t<Tv> &vout,
		     const ibis::array_t<Tk> &kin1,
		     const ibis::array_t<Tv> &vin1,
		     const ibis::array_t<Tk> &kin2,
		     const ibis::array_t<Tv> &vin2,
		     ibis::selectClause::AGREGADO agg) {
    kout.clear();
    vout.clear();
    if (kin1.size() != vin1.size() ||
	kin2.size() != vin2.size())
	return -10;
    if (kin1.empty() || vin1.empty()) {
	kout.copy(kin2);
	vout.copy(vin2);
	return kin2.size();
    }
    else if (kin2.empty() || vin2.empty()) {
	kout.copy(kin1);
	vout.copy(vin1);
	return kin1.size();
    }

    size_t i1 = 0;
    size_t i2 = 0;
    while (i1 < kin1.size() && i2 < kin2.size()) {
	if (kin1[i1] == kin2[i2]) {
	    switch (agg) {
	    default:
		kout.clear();
		vout.clear();
		return -6;
	    case ibis::selectClause::CNT:
	    case ibis::selectClause::SUM:
		vout.push_back(vin1[i1] + vin2[i2]);
		break;
	    case ibis::selectClause::MIN:
		vout.push_back(vin1[i1] <= vin2[i2] ? vin1[i1] : vin2[i2]);
		break;
	    case ibis::selectClause::MAX:
		vout.push_back(vin1[i1] >= vin2[i2] ? vin1[i1] : vin2[i2]);
		break;
	    }
	    kout.push_back(kin1[i1]);
	    ++ i1;
	    ++ i2;
	}
	else if (kin1[i1] < kin2[i2]) {
	    kout.push_back(kin1[i1]);
	    vout.push_back(vin1[i1]);
	    ++ i1;
	}
	else {
	    kout.push_back(kin2[i2]);
	    vout.push_back(vin2[i2]);
	    ++ i2;
	}
    }

    while (i1 < kin1.size()) {
	kout.push_back(kin1[i1]);
	vout.push_back(vin1[i1]);
	++ i1;
    }
    while (i2 < kin2.size()) {
	kout.push_back(kin2[i2]);
	vout.push_back(vin2[i2]);
	++ i2;
    }
    return kout.size();
} // ibis::bord::merge11T

/// Merge two aggregations sharing the same key.
int ibis::bord::merge12(ibis::bord::column &k1,
			ibis::bord::column &u1,
			ibis::bord::column &v1,
			const ibis::bord::column &k2,
			const ibis::bord::column &u2,
			const ibis::bord::column &v2,
			ibis::selectClause::AGREGADO au,
			ibis::selectClause::AGREGADO av) {
    int ierr = -1;
    if (k1.type() != k2.type() || u1.type() != u2.type() ||
	v1.type() != v2.type()) {
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- bord::merge12 expects the same types and sizes "
	    "for the keys and values";
	return ierr;
    }

    switch (k1.type()) {
    default:
	return -2;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &ak2 =
	    *static_cast<const ibis::array_t<signed char>*>(k2.getArray());
	ibis::array_t<signed char> &ak0 =
	    *static_cast<ibis::array_t<signed char>*>(k1.getArray());
	const ibis::array_t<signed char> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &ak2 =
	    *static_cast<const ibis::array_t<unsigned char>*>(k2.getArray());
	ibis::array_t<unsigned char> &ak0 =
	    *static_cast<ibis::array_t<unsigned char>*>(k1.getArray());
	const ibis::array_t<unsigned char> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &ak2 =
	    *static_cast<const ibis::array_t<int16_t>*>(k2.getArray());
	ibis::array_t<int16_t> &ak0 =
	    *static_cast<ibis::array_t<int16_t>*>(k1.getArray());
	const ibis::array_t<int16_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &ak2 =
	    *static_cast<const ibis::array_t<uint16_t>*>(k2.getArray());
	ibis::array_t<uint16_t> &ak0 =
	    *static_cast<ibis::array_t<uint16_t>*>(k1.getArray());
	const ibis::array_t<uint16_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &ak2 =
	    *static_cast<const ibis::array_t<int32_t>*>(k2.getArray());
	ibis::array_t<int32_t> &ak0 =
	    *static_cast<ibis::array_t<int32_t>*>(k1.getArray());
	const ibis::array_t<int32_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &ak2 =
	    *static_cast<const ibis::array_t<uint32_t>*>(k2.getArray());
	ibis::array_t<uint32_t> &ak0 =
	    *static_cast<ibis::array_t<uint32_t>*>(k1.getArray());
	const ibis::array_t<uint32_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &ak2 =
	    *static_cast<const ibis::array_t<int64_t>*>(k2.getArray());
	ibis::array_t<int64_t> &ak0 =
	    *static_cast<ibis::array_t<int64_t>*>(k1.getArray());
	const ibis::array_t<int64_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &ak2 =
	    *static_cast<const ibis::array_t<uint64_t>*>(k2.getArray());
	ibis::array_t<uint64_t> &ak0 =
	    *static_cast<ibis::array_t<uint64_t>*>(k1.getArray());
	const ibis::array_t<uint64_t> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &ak2 =
	    *static_cast<const ibis::array_t<float>*>(k2.getArray());
	ibis::array_t<float> &ak0 =
	    *static_cast<ibis::array_t<float>*>(k1.getArray());
	const ibis::array_t<float> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &ak2 =
	    *static_cast<const ibis::array_t<double>*>(k2.getArray());
	ibis::array_t<double> &ak0 =
	    *static_cast<ibis::array_t<double>*>(k1.getArray());
	const ibis::array_t<double> ak1(ak0);
	ak0.nosharing();
	ierr = merge12S(ak0, ak1, ak2, u1, v1, u2, v2, au, av);
	break;}
    }
    return ierr;
} // ibis::bord::merge12

template <typename Tk> int
ibis::bord::merge12S(ibis::array_t<Tk> &kout,
		     const ibis::array_t<Tk> &kin1,
		     const ibis::array_t<Tk> &kin2,
		     ibis::bord::column &u1,
		     ibis::bord::column &v1,
		     const ibis::bord::column &u2,
		     const ibis::bord::column &v2,
		     ibis::selectClause::AGREGADO au,
		     ibis::selectClause::AGREGADO av) {
    int ierr = -1;
    if (u1.type() != u2.type() || v1.type() != v2.type())
	return ierr;

    switch (u1.type()) {
    default:
	return ierr;
    case ibis::BYTE: {
	const ibis::array_t<signed char>& au2 =
	    *static_cast<const ibis::array_t<signed char>*>(u2.getArray());
	ibis::array_t<signed char>& au0 =
	    *static_cast<ibis::array_t<signed char>*>(u1.getArray());
	const ibis::array_t<signed char> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char>& au2 =
	    *static_cast<const ibis::array_t<unsigned char>*>(u2.getArray());
	ibis::array_t<unsigned char>& au0 =
	    *static_cast<ibis::array_t<unsigned char>*>(u1.getArray());
	const ibis::array_t<unsigned char> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t>& au2 =
	    *static_cast<const ibis::array_t<int16_t>*>(u2.getArray());
	ibis::array_t<int16_t>& au0 =
	    *static_cast<ibis::array_t<int16_t>*>(u1.getArray());
	const ibis::array_t<int16_t> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t>& au2 =
	    *static_cast<const ibis::array_t<uint16_t>*>(u2.getArray());
	ibis::array_t<uint16_t>& au0 =
	    *static_cast<ibis::array_t<uint16_t>*>(u1.getArray());
	const ibis::array_t<uint16_t> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t>& au2 =
	    *static_cast<const ibis::array_t<int32_t>*>(u2.getArray());
	ibis::array_t<int32_t>& au0 =
	    *static_cast<ibis::array_t<int32_t>*>(u1.getArray());
	const ibis::array_t<int32_t> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t>& au2 =
	    *static_cast<const ibis::array_t<uint32_t>*>(u2.getArray());
	ibis::array_t<uint32_t>& au0 =
	    *static_cast<ibis::array_t<uint32_t>*>(u1.getArray());
	const ibis::array_t<uint32_t> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t>& au2 =
	    *static_cast<const ibis::array_t<int64_t>*>(u2.getArray());
	ibis::array_t<int64_t>& au0 =
	    *static_cast<ibis::array_t<int64_t>*>(u1.getArray());
	const ibis::array_t<int64_t> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t>& au2 =
	    *static_cast<const ibis::array_t<uint64_t>*>(u2.getArray());
	ibis::array_t<uint64_t>& au0 =
	    *static_cast<ibis::array_t<uint64_t>*>(u1.getArray());
	const ibis::array_t<uint64_t> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float>& au2 =
	    *static_cast<const ibis::array_t<float>*>(u2.getArray());
	ibis::array_t<float>& au0 =
	    *static_cast<ibis::array_t<float>*>(u1.getArray());
	const ibis::array_t<float> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double>& au2 =
	    *static_cast<const ibis::array_t<double>*>(u2.getArray());
	ibis::array_t<double>& au0 =
	    *static_cast<ibis::array_t<double>*>(u1.getArray());
	const ibis::array_t<double> au1(au0);
	au0.nosharing();
	switch(v1.type()) {
	default:
	    return ierr;
	case ibis::BYTE: {
	    const ibis::array_t<signed char>& av2 =
		*static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	    ibis::array_t<signed char>& av0 =
		*static_cast<ibis::array_t<signed char>*>(v1.getArray());
	    const ibis::array_t<signed char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UBYTE: {
	    const ibis::array_t<unsigned char>& av2 =
		*static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	    ibis::array_t<unsigned char>& av0 =
		*static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	    const ibis::array_t<unsigned char> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::SHORT: {
	    const ibis::array_t<int16_t>& av2 =
		*static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	    ibis::array_t<int16_t>& av0 =
		*static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	    const ibis::array_t<int16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::USHORT: {
	    const ibis::array_t<uint16_t>& av2 =
		*static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	    ibis::array_t<uint16_t>& av0 =
		*static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	    const ibis::array_t<uint16_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::INT: {
	    const ibis::array_t<int32_t>& av2 =
		*static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	    ibis::array_t<int32_t>& av0 =
		*static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	    const ibis::array_t<int32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::UINT: {
	    const ibis::array_t<uint32_t>& av2 =
		*static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	    ibis::array_t<uint32_t>& av0 =
		*static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	    const ibis::array_t<uint32_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::LONG: {
	    const ibis::array_t<int64_t>& av2 =
		*static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	    ibis::array_t<int64_t>& av0 =
		*static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	    const ibis::array_t<int64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::ULONG: {
	    const ibis::array_t<uint64_t>& av2 =
		*static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	    ibis::array_t<uint64_t>& av0 =
		*static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	    const ibis::array_t<uint64_t> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::FLOAT: {
	    const ibis::array_t<float>& av2 =
		*static_cast<const ibis::array_t<float>*>(v2.getArray());
	    ibis::array_t<float>& av0 =
		*static_cast<ibis::array_t<float>*>(v1.getArray());
	    const ibis::array_t<float> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	case ibis::DOUBLE: {
	    const ibis::array_t<double>& av2 =
		*static_cast<const ibis::array_t<double>*>(v2.getArray());
	    ibis::array_t<double>& av0 =
		*static_cast<ibis::array_t<double>*>(v1.getArray());
	    const ibis::array_t<double> av1(av0);
	    av0.nosharing();
	    ierr = merge12T(kout, au0, av0, kin1, au1, av1, kin2, au2, av2,
			    au, av);
	    break;}
	}
	break;}
    }
    return ierr;
} // ibis::bord::merge12S

template <typename Tk, typename Tu, typename Tv> int
ibis::bord::merge12T(ibis::array_t<Tk> &kout,
		     ibis::array_t<Tu> &uout,
		     ibis::array_t<Tv> &vout,
		     const ibis::array_t<Tk> &kin1,
		     const ibis::array_t<Tu> &uin1,
		     const ibis::array_t<Tv> &vin1,
		     const ibis::array_t<Tk> &kin2,
		     const ibis::array_t<Tu> &uin2,
		     const ibis::array_t<Tv> &vin2,
		     ibis::selectClause::AGREGADO au,
		     ibis::selectClause::AGREGADO av) {
    kout.clear();
    uout.clear();
    vout.clear();
    int ierr = -1;
    if (kin1.size() != uin1.size() || kin1.size() != vin1.size() ||
	kin2.size() != uin2.size() || kin2.size() != vin2.size())
	return ierr;

    size_t j1 = 0;
    size_t j2 = 0;
    while (j1 < kin1.size() && j2 < kin2.size()) {
	if (kin1[j1] == kin2[j2]) { // same key value
	    switch (au) {
	    default:
		return ierr;
	    case ibis::selectClause::CNT:
	    case ibis::selectClause::SUM:
		uout.push_back(uin1[j1] + uin2[j2]);
		break;
	    case ibis::selectClause::MAX:
		uout.push_back(uin1[j1] >= uin2[j2] ? uin1[j1] : uin2[j2]);
		break;
	    case ibis::selectClause::MIN:
		uout.push_back(uin1[j1] <= uin2[j2] ? uin1[j1] : uin2[j2]);
		break;
	    }
	    switch (av) {
	    default:
		return ierr;
	    case ibis::selectClause::CNT:
	    case ibis::selectClause::SUM:
		vout.push_back(vin1[j1] + vin2[j2]);
		break;
	    case ibis::selectClause::MAX:
		vout.push_back(vin1[j1] >= vin2[j2] ? vin1[j1] : vin2[j2]);
		break;
	    case ibis::selectClause::MIN:
		vout.push_back(vin1[j1] <= vin2[j2] ? vin1[j1] : vin2[j2]);
		break;
	    }
	    kout.push_back(kin1[j1]);
	    ++ j1;
	    ++ j2;
	}
	else if (kin1[j1] < kin2[j2]) {
	    uout.push_back(uin1[j1]);
	    vout.push_back(vin1[j1]);
	    kout.push_back(kin1[j1]);
	    ++ j1;
	}
	else {
	    uout.push_back(uin2[j2]);
	    vout.push_back(vin2[j2]);
	    kout.push_back(kin2[j2]);
	    ++ j2;
	}
    }

    while (j1 < kin1.size()) {
	kout.push_back(kin1[j1]);
	uout.push_back(uin1[j1]);
	vout.push_back(vin1[j1]);
	++ j1;
    }
    while (j2 < kin2.size()) {
	kout.push_back(kin2[j2]);
	uout.push_back(uin2[j2]);
	vout.push_back(vin2[j2]);
	++ j2;
    }
    return kout.size();
} // ibis::bord::merge12T

/// Merge with two key columns and arbitrary number of value columns.
int ibis::bord::merge20(ibis::bord::column &k11,
			ibis::bord::column &k21,
			std::vector<ibis::bord::column*> &v1,
			const ibis::bord::column &k12,
			const ibis::bord::column &k22,
			const std::vector<ibis::bord::column*> &v2,
			const std::vector<ibis::selectClause::AGREGADO> &agg) {
    int ierr = -1;
    if (k11.type() != k21.type()) return ierr;

    switch (k11.type()) {
    default:
	return ierr;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &ak12 =
	    *static_cast<const ibis::array_t<signed char>*>(k12.getArray());
	ibis::array_t<signed char> &ak10 =
	    *static_cast<ibis::array_t<signed char>*>(k11.getArray());
	const ibis::array_t<signed char> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &ak12 =
	    *static_cast<const ibis::array_t<unsigned char>*>(k12.getArray());
	ibis::array_t<unsigned char> &ak10 =
	    *static_cast<ibis::array_t<unsigned char>*>(k11.getArray());
	const ibis::array_t<unsigned char> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &ak12 =
	    *static_cast<const ibis::array_t<int16_t>*>(k12.getArray());
	ibis::array_t<int16_t> &ak10 =
	    *static_cast<ibis::array_t<int16_t>*>(k11.getArray());
	const ibis::array_t<int16_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &ak12 =
	    *static_cast<const ibis::array_t<uint16_t>*>(k12.getArray());
	ibis::array_t<uint16_t> &ak10 =
	    *static_cast<ibis::array_t<uint16_t>*>(k11.getArray());
	const ibis::array_t<uint16_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &ak12 =
	    *static_cast<const ibis::array_t<int32_t>*>(k12.getArray());
	ibis::array_t<int32_t> &ak10 =
	    *static_cast<ibis::array_t<int32_t>*>(k11.getArray());
	const ibis::array_t<int32_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &ak12 =
	    *static_cast<const ibis::array_t<uint32_t>*>(k12.getArray());
	ibis::array_t<uint32_t> &ak10 =
	    *static_cast<ibis::array_t<uint32_t>*>(k11.getArray());
	const ibis::array_t<uint32_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &ak12 =
	    *static_cast<const ibis::array_t<int64_t>*>(k12.getArray());
	ibis::array_t<int64_t> &ak10 =
	    *static_cast<ibis::array_t<int64_t>*>(k11.getArray());
	const ibis::array_t<int64_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &ak12 =
	    *static_cast<const ibis::array_t<uint64_t>*>(k12.getArray());
	ibis::array_t<uint64_t> &ak10 =
	    *static_cast<ibis::array_t<uint64_t>*>(k11.getArray());
	const ibis::array_t<uint64_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &ak12 =
	    *static_cast<const ibis::array_t<float>*>(k12.getArray());
	ibis::array_t<float> &ak10 =
	    *static_cast<ibis::array_t<float>*>(k11.getArray());
	const ibis::array_t<float> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &ak12 =
	    *static_cast<const ibis::array_t<double>*>(k12.getArray());
	ibis::array_t<double> &ak10 =
	    *static_cast<ibis::array_t<double>*>(k11.getArray());
	const ibis::array_t<double> ak11(ak10);
	ak10.nosharing();
	ierr = merge20T1(ak10, ak11, ak12, k21, v1, k22, v2, agg);
	break;}
    }
    return ierr;
} // ibis::bord::merge20

/// Merge with two key columns and arbitrary number of value columns.
/// The first key column is templated.
template <typename Tk1> int
ibis::bord::merge20T1(ibis::array_t<Tk1> &k1out,
		      const ibis::array_t<Tk1> &k1in1,
		      const ibis::array_t<Tk1> &k1in2,
		      ibis::bord::column &k21,
		      std::vector<ibis::bord::column*> &vin1,
		      const ibis::bord::column &k22,
		      const std::vector<ibis::bord::column*> &vin2,
		      const std::vector<ibis::selectClause::AGREGADO> &agg) {
    int ierr = -1;
    if (k21.type() != k22.type()) return ierr;

    std::vector<ibis::bord::column*> av1(vin1.size());
    IBIS_BLOCK_GUARD(ibis::util::clearVec<ibis::bord::column>,
		     ibis::util::ref(av1));
    for (unsigned j = 0; j < vin1.size(); ++ j)
	av1[j] = new ibis::bord::column(*vin1[j]);

    switch (k21.type()) {
    default:
	return ierr;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &ak22 =
	    *static_cast<const ibis::array_t<signed char>*>(k22.getArray());
	ibis::array_t<signed char> &ak20 =
	    *static_cast<ibis::array_t<signed char>*>(k21.getArray());
	const ibis::array_t<signed char> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &ak22 =
	    *static_cast<const ibis::array_t<unsigned char>*>(k22.getArray());
	ibis::array_t<unsigned char> &ak20 =
	    *static_cast<ibis::array_t<unsigned char>*>(k21.getArray());
	const ibis::array_t<unsigned char> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &ak22 =
	    *static_cast<const ibis::array_t<int16_t>*>(k22.getArray());
	ibis::array_t<int16_t> &ak20 =
	    *static_cast<ibis::array_t<int16_t>*>(k21.getArray());
	const ibis::array_t<int16_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &ak22 =
	    *static_cast<const ibis::array_t<uint16_t>*>(k22.getArray());
	ibis::array_t<uint16_t> &ak20 =
	    *static_cast<ibis::array_t<uint16_t>*>(k21.getArray());
	const ibis::array_t<uint16_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &ak22 =
	    *static_cast<const ibis::array_t<int32_t>*>(k22.getArray());
	ibis::array_t<int32_t> &ak20 =
	    *static_cast<ibis::array_t<int32_t>*>(k21.getArray());
	const ibis::array_t<int32_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &ak22 =
	    *static_cast<const ibis::array_t<uint32_t>*>(k22.getArray());
	ibis::array_t<uint32_t> &ak20 =
	    *static_cast<ibis::array_t<uint32_t>*>(k21.getArray());
	const ibis::array_t<uint32_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &ak22 =
	    *static_cast<const ibis::array_t<int64_t>*>(k22.getArray());
	ibis::array_t<int64_t> &ak20 =
	    *static_cast<ibis::array_t<int64_t>*>(k21.getArray());
	const ibis::array_t<int64_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &ak22 =
	    *static_cast<const ibis::array_t<uint64_t>*>(k22.getArray());
	ibis::array_t<uint64_t> &ak20 =
	    *static_cast<ibis::array_t<uint64_t>*>(k21.getArray());
	const ibis::array_t<uint64_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &ak22 =
	    *static_cast<const ibis::array_t<float>*>(k22.getArray());
	ibis::array_t<float> &ak20 =
	    *static_cast<ibis::array_t<float>*>(k21.getArray());
	const ibis::array_t<float> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &ak22 =
	    *static_cast<const ibis::array_t<double>*>(k22.getArray());
	ibis::array_t<double> &ak20 =
	    *static_cast<ibis::array_t<double>*>(k21.getArray());
	const ibis::array_t<double> ak21(ak20);
	ak20.nosharing();
	ierr = merge20T2(k1out, ak20, vin1, k1in1, ak21, av1,
			 k1in2, ak22, vin2, agg);
	break;}
    }
    return ierr;
} // ibis::bord::merge20T1

/// Merge in-memory table with two keys and more than one value columns.
/// Both key columns are templated.
template <typename Tk1, typename Tk2> int
ibis::bord::merge20T2(ibis::array_t<Tk1> &k1out,
		      ibis::array_t<Tk2> &k2out,
		      std::vector<ibis::bord::column*> &vout,
		      const ibis::array_t<Tk1> &k1in1,
		      const ibis::array_t<Tk2> &k2in1,
		      const std::vector<ibis::bord::column*> &vin1,
		      const ibis::array_t<Tk1> &k1in2,
		      const ibis::array_t<Tk2> &k2in2,
		      const std::vector<ibis::bord::column*> &vin2,
		      const std::vector<ibis::selectClause::AGREGADO> &agg) {
    int ierr = -1;
    k1out.clear();
    k2out.clear();
    for (size_t j = 0; j < vout.size(); ++ j)
	vout[j]->limit(0);
    if (vout.size() != vin1.size() || vout.size() != vin2.size() ||
	vout.size() != agg.size())
	return ierr;

    size_t j1 = 0;
    size_t j2 = 0;
    while (j1 < k1in1.size() && j2 < k1in2.size()) {
	if (k1in1[j1] == k1in2[j2]) {
	    if (k2in1[j1] == k2in2[j2]) {
		k1out.push_back(k1in1[j1]);
		k2out.push_back(k2in1[j1]);
		for (unsigned j = 0; j < vin1.size(); ++ j)
		    vout[j]->append(vin1[j]->getArray(), j1,
				    vin2[j]->getArray(), j2, agg[j]);
		++ j1;
		++ j2;
	    }
	    else if (k2in1[j1] < k2in2[j2]) {
		k1out.push_back(k1in1[j1]);
		k2out.push_back(k2in1[j1]);
		for (unsigned j = 0; j < vin1.size(); ++ j)
		    vout[j]->append(vin1[j]->getArray(), j1);
		++ j1;
	    }
	    else {
		k1out.push_back(k1in2[j2]);
		k2out.push_back(k2in2[j2]);
		for (unsigned j = 0; j < vin2.size(); ++ j)
		    vout[j]->append(vin2[j]->getArray(), j2);
		++ j2;
	    }
	}
	else if (k1in1[j1] < k1in2[j2]) {
	    k1out.push_back(k1in1[j1]);
	    k2out.push_back(k2in1[j1]);
	    for (unsigned j = 0; j < vin1.size(); ++ j)
		vout[j]->append(vin1[j]->getArray(), j1);
	    ++ j1;
	}
	else {
	    k1out.push_back(k1in2[j2]);
	    k2out.push_back(k2in2[j2]);
	    for (unsigned j = 0; j < vin2.size(); ++ j)
		vout[j]->append(vin2[j]->getArray(), j2);
	    ++ j2;
	}
    }

    while (j1 < k1in1.size()) {
	k1out.push_back(k1in1[j1]);
	k2out.push_back(k2in1[j1]);
	for (unsigned j = 0; j < vin1.size(); ++ j)
	    vout[j]->append(vin1[j]->getArray(), j1);
	++ j1;
    }

    while (j2 < k1in2.size()) {
	k1out.push_back(k1in2[j2]);
	k2out.push_back(k2in2[j2]);
	for (unsigned j = 0; j < vin2.size(); ++ j)
	    vout[j]->append(vin2[j]->getArray(), j2);
	++ j2;
    }

    ierr = k1out.size();
    return ierr;
} // ibis::bord::merge20T2

/// Merge two key columns with one value column.
int ibis::bord::merge21(ibis::bord::column &k11,
			ibis::bord::column &k21,
			ibis::bord::column &v1,
			const ibis::bord::column &k12,
			const ibis::bord::column &k22,
			const ibis::bord::column &v2,
			ibis::selectClause::AGREGADO ag) {
    int ierr = -1;
    if (k11.type() != k12.type()) return ierr;

    switch (k11.type()) {
    default:
	return ierr;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &ak12 =
	    *static_cast<const ibis::array_t<signed char>*>(k12.getArray());
	ibis::array_t<signed char> &ak10 =
	    *static_cast<ibis::array_t<signed char>*>(k11.getArray());
	const ibis::array_t<signed char> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &ak12 =
	    *static_cast<const ibis::array_t<unsigned char>*>(k12.getArray());
	ibis::array_t<unsigned char> &ak10 =
	    *static_cast<ibis::array_t<unsigned char>*>(k11.getArray());
	const ibis::array_t<unsigned char> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &ak12 =
	    *static_cast<const ibis::array_t<int16_t>*>(k12.getArray());
	ibis::array_t<int16_t> &ak10 =
	    *static_cast<ibis::array_t<int16_t>*>(k11.getArray());
	const ibis::array_t<int16_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &ak12 =
	    *static_cast<const ibis::array_t<uint16_t>*>(k12.getArray());
	ibis::array_t<uint16_t> &ak10 =
	    *static_cast<ibis::array_t<uint16_t>*>(k11.getArray());
	const ibis::array_t<uint16_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &ak12 =
	    *static_cast<const ibis::array_t<int32_t>*>(k12.getArray());
	ibis::array_t<int32_t> &ak10 =
	    *static_cast<ibis::array_t<int32_t>*>(k11.getArray());
	const ibis::array_t<int32_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &ak12 =
	    *static_cast<const ibis::array_t<uint32_t>*>(k12.getArray());
	ibis::array_t<uint32_t> &ak10 =
	    *static_cast<ibis::array_t<uint32_t>*>(k11.getArray());
	const ibis::array_t<uint32_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &ak12 =
	    *static_cast<const ibis::array_t<int64_t>*>(k12.getArray());
	ibis::array_t<int64_t> &ak10 =
	    *static_cast<ibis::array_t<int64_t>*>(k11.getArray());
	const ibis::array_t<int64_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &ak12 =
	    *static_cast<const ibis::array_t<uint64_t>*>(k12.getArray());
	ibis::array_t<uint64_t> &ak10 =
	    *static_cast<ibis::array_t<uint64_t>*>(k11.getArray());
	const ibis::array_t<uint64_t> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &ak12 =
	    *static_cast<const ibis::array_t<float>*>(k12.getArray());
	ibis::array_t<float> &ak10 =
	    *static_cast<ibis::array_t<float>*>(k11.getArray());
	const ibis::array_t<float> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &ak12 =
	    *static_cast<const ibis::array_t<double>*>(k12.getArray());
	ibis::array_t<double> &ak10 =
	    *static_cast<ibis::array_t<double>*>(k11.getArray());
	const ibis::array_t<double> ak11(ak10);
	ak10.nosharing();
	ierr = merge21T1(ak10, ak11, ak12, k21, v1, k22, v2, ag);
	break;}
    }

    return ierr;
} // ibis::bord::merge21

/// Merge two key columns with one value column.
/// The first key column is templated.
template <typename Tk1>
int ibis::bord::merge21T1(ibis::array_t<Tk1> &k1out,
			  const ibis::array_t<Tk1> &k1in1,
			  const ibis::array_t<Tk1> &k1in2,
			  ibis::bord::column &k21,
			  ibis::bord::column &v1,
			  const ibis::bord::column &k22,
			  const ibis::bord::column &v2,
			  ibis::selectClause::AGREGADO ag) {
    int ierr = -1;
    if (k21.type() != k22.type()) return ierr;

    switch (k21.type()) {
    default:
	return ierr;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &ak22 =
	    *static_cast<const ibis::array_t<signed char>*>(k22.getArray());
	ibis::array_t<signed char> &ak20 =
	    *static_cast<ibis::array_t<signed char>*>(k21.getArray());
	const ibis::array_t<signed char> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &ak22 =
	    *static_cast<const ibis::array_t<unsigned char>*>(k22.getArray());
	ibis::array_t<unsigned char> &ak20 =
	    *static_cast<ibis::array_t<unsigned char>*>(k21.getArray());
	const ibis::array_t<unsigned char> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &ak22 =
	    *static_cast<const ibis::array_t<int16_t>*>(k22.getArray());
	ibis::array_t<int16_t> &ak20 =
	    *static_cast<ibis::array_t<int16_t>*>(k21.getArray());
	const ibis::array_t<int16_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &ak22 =
	    *static_cast<const ibis::array_t<uint16_t>*>(k22.getArray());
	ibis::array_t<uint16_t> &ak20 =
	    *static_cast<ibis::array_t<uint16_t>*>(k21.getArray());
	const ibis::array_t<uint16_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &ak22 =
	    *static_cast<const ibis::array_t<int32_t>*>(k22.getArray());
	ibis::array_t<int32_t> &ak20 =
	    *static_cast<ibis::array_t<int32_t>*>(k21.getArray());
	const ibis::array_t<int32_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &ak22 =
	    *static_cast<const ibis::array_t<uint32_t>*>(k22.getArray());
	ibis::array_t<uint32_t> &ak20 =
	    *static_cast<ibis::array_t<uint32_t>*>(k21.getArray());
	const ibis::array_t<uint32_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &ak22 =
	    *static_cast<const ibis::array_t<int64_t>*>(k22.getArray());
	ibis::array_t<int64_t> &ak20 =
	    *static_cast<ibis::array_t<int64_t>*>(k21.getArray());
	const ibis::array_t<int64_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &ak22 =
	    *static_cast<const ibis::array_t<uint64_t>*>(k22.getArray());
	ibis::array_t<uint64_t> &ak20 =
	    *static_cast<ibis::array_t<uint64_t>*>(k21.getArray());
	const ibis::array_t<uint64_t> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &ak22 =
	    *static_cast<const ibis::array_t<float>*>(k22.getArray());
	ibis::array_t<float> &ak20 =
	    *static_cast<ibis::array_t<float>*>(k21.getArray());
	const ibis::array_t<float> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &ak22 =
	    *static_cast<const ibis::array_t<double>*>(k22.getArray());
	ibis::array_t<double> &ak20 =
	    *static_cast<ibis::array_t<double>*>(k21.getArray());
	const ibis::array_t<double> ak21(ak20);
	ak20.nosharing();
	ierr = merge21T2(k1out, ak20, k1in1, ak21, k1in2, ak22, v1, v2, ag);
	break;}
    }

    return ierr;
} // ibis::bord::merge21T1

/// Merge two key columns with one value column.
/// The two key columns are templated.
template <typename Tk1, typename Tk2>
int ibis::bord::merge21T2(ibis::array_t<Tk1> &k1out,
			  ibis::array_t<Tk2> &k2out,
			  const ibis::array_t<Tk1> &k1in1,
			  const ibis::array_t<Tk2> &k2in1,
			  const ibis::array_t<Tk1> &k1in2,
			  const ibis::array_t<Tk2> &k2in2,
			  ibis::bord::column &v1,
			  const ibis::bord::column &v2,
			  ibis::selectClause::AGREGADO ag) {
    int ierr = -1;
    if (v1.type() != v2.type()) return ierr;

    switch (v1.type()) {
    default:
	return ierr;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &av2 =
	    *static_cast<const ibis::array_t<signed char>*>(v2.getArray());
	ibis::array_t<signed char> &av0 =
	    *static_cast<ibis::array_t<signed char>*>(v1.getArray());
	const ibis::array_t<signed char> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &av2 =
	    *static_cast<const ibis::array_t<unsigned char>*>(v2.getArray());
	ibis::array_t<unsigned char> &av0 =
	    *static_cast<ibis::array_t<unsigned char>*>(v1.getArray());
	const ibis::array_t<unsigned char> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &av2 =
	    *static_cast<const ibis::array_t<int16_t>*>(v2.getArray());
	ibis::array_t<int16_t> &av0 =
	    *static_cast<ibis::array_t<int16_t>*>(v1.getArray());
	const ibis::array_t<int16_t> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &av2 =
	    *static_cast<const ibis::array_t<uint16_t>*>(v2.getArray());
	ibis::array_t<uint16_t> &av0 =
	    *static_cast<ibis::array_t<uint16_t>*>(v1.getArray());
	const ibis::array_t<uint16_t> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::INT: {
	const ibis::array_t<int32_t> &av2 =
	    *static_cast<const ibis::array_t<int32_t>*>(v2.getArray());
	ibis::array_t<int32_t> &av0 =
	    *static_cast<ibis::array_t<int32_t>*>(v1.getArray());
	const ibis::array_t<int32_t> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &av2 =
	    *static_cast<const ibis::array_t<uint32_t>*>(v2.getArray());
	ibis::array_t<uint32_t> &av0 =
	    *static_cast<ibis::array_t<uint32_t>*>(v1.getArray());
	const ibis::array_t<uint32_t> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &av2 =
	    *static_cast<const ibis::array_t<int64_t>*>(v2.getArray());
	ibis::array_t<int64_t> &av0 =
	    *static_cast<ibis::array_t<int64_t>*>(v1.getArray());
	const ibis::array_t<int64_t> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &av2 =
	    *static_cast<const ibis::array_t<uint64_t>*>(v2.getArray());
	ibis::array_t<uint64_t> &av0 =
	    *static_cast<ibis::array_t<uint64_t>*>(v1.getArray());
	const ibis::array_t<uint64_t> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::FLOAT: {
	const ibis::array_t<float> &av2 =
	    *static_cast<const ibis::array_t<float>*>(v2.getArray());
	ibis::array_t<float> &av0 =
	    *static_cast<ibis::array_t<float>*>(v1.getArray());
	const ibis::array_t<float> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &av2 =
	    *static_cast<const ibis::array_t<double>*>(v2.getArray());
	ibis::array_t<double> &av0 =
	    *static_cast<ibis::array_t<double>*>(v1.getArray());
	const ibis::array_t<double> av1(av0);
	av0.nosharing();
	ierr = merge21T3(k1out, k2out, av0,
			 k1in1, k2in1, av1,
			 k1in2, k2in2, av2, ag);
	break;}
    }

    return ierr;
} // ibis::bord::merge21T2

/// Merge two key columns with one value column.
/// The two key columns and the value column are all templated.
template <typename Tk1, typename Tk2, typename Tv>
int ibis::bord::merge21T3(ibis::array_t<Tk1> &k1out,
			  ibis::array_t<Tk2> &k2out,
			  ibis::array_t<Tv>  &vout,
			  const ibis::array_t<Tk1> &k1in1,
			  const ibis::array_t<Tk2> &k2in1,
			  const ibis::array_t<Tv>  &vin1,
			  const ibis::array_t<Tk1> &k1in2,
			  const ibis::array_t<Tk2> &k2in2,
			  const ibis::array_t<Tv>  &vin2,
			  ibis::selectClause::AGREGADO av) {
    k1out.clear();
    k2out.clear();
    vout.clear();
    int ierr = -1;
    if (k1in1.size() != k2in1.size() || k1in1.size() != vin1.size() ||
	k1in2.size() != k2in2.size() || k1in2.size() != vin2.size())
	return ierr;

    size_t j1 = 0; // for k1in1, k2in1, vin1
    size_t j2 = 0; // for k1in2, k2in2, vin2
    while (j1 < k1in1.size() && j2 < k1in2.size()) {
	if (k1in1[j1] == k1in2[j2]) { // same k1
	    if (k2in1[j1] == k2in2[j2]) { // same k2
		switch (av) {
		default:
		    return ierr;
		case ibis::selectClause::CNT:
		case ibis::selectClause::SUM:
		    vout.push_back(vin1[j1] + vin2[j2]);
		    break;
		case ibis::selectClause::MAX:
		    vout.push_back(vin1[j1] >= vin2[j2] ? vin1[j1] : vin2[j2]);
		    break;
		case ibis::selectClause::MIN:
		    vout.push_back(vin1[j1] <= vin2[j2] ? vin1[j1] : vin2[j2]);
		    break;
		}
		k1out.push_back(k1in1[j1]);
		k2out.push_back(k2in1[j1]);
		++ j1;
		++ j2;
	    }
	    else if (k2in1[j1] < k2in2[j2]) { // copy from *1
		k1out.push_back(k1in1[j1]);
		k2out.push_back(k2in1[j1]);
		vout.push_back(vin1[j1]);
		++ j1;
	    }
	    else { // copy from *2
		k1out.push_back(k1in2[j2]);
		k2out.push_back(k2in2[j2]);
		vout.push_back(vin2[j2]);
		++ j2;
	    }
	}
	else if (k1in1[j1] < k1in2[j2]) { // copy from *1
	    k1out.push_back(k1in1[j1]);
	    k2out.push_back(k2in1[j1]);
	    vout.push_back(vin1[j1]);
	    ++ j1;
	}
	else { // copy from *2
	    k1out.push_back(k1in2[j2]);
	    k2out.push_back(k2in2[j2]);
	    vout.push_back(vin2[j2]);
	    ++ j2;
	}
    }

    while (j1 < k1in1.size()) {
	k1out.push_back(k1in1[j1]);
	k2out.push_back(k2in1[j1]);
	vout.push_back(vin1[j1]);
	++ j1;
    }

    while (j2 < k1in2.size()) {
	k1out.push_back(k1in2[j2]);
	k2out.push_back(k2in2[j2]);
	vout.push_back(vin2[j2]);
	++ j2;
    }

    return k1out.size();
} // ibis::bord::merge21T3

void ibis::bord::orderby(const ibis::table::stringList& keys) {
    std::vector<bool> directions;
    (void) reorder(keys, directions);
} // ibis::bord::orderby

void ibis::bord::orderby(const ibis::table::stringList& keys,
			 const std::vector<bool>& directions) {
    (void) reorder(keys, directions);
} // ibis::bord::orderby

long ibis::bord::reorder() {
    return ibis::part::reorder();
} // ibis::bord::reorder

long ibis::bord::reorder(const ibis::table::stringList& keys) {
    std::vector<bool> directions;
    return reorder(keys, directions);
} // ibis::bord::reorder

long ibis::bord::reorder(const ibis::table::stringList& cols,
			 const std::vector<bool>& directions) {
    long ierr = 0;
    if (nRows() == 0 || nColumns() == 0) return ierr;

    std::string evt = "bord[";
    evt += m_name;
    evt += "]::reorder";
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg;
	lg() << evt << " -- reordering with " << cols[0];
	for (unsigned j = 1; j < cols.size(); ++ j)
	    lg() << ", " << cols[j];
    }

    writeLock lock(this, evt.c_str()); // can't process other operations
    for (columnList::const_iterator it = columns.begin();
	 it != columns.end();
	 ++ it) { // purge all index files
	(*it).second->unloadIndex();
	(*it).second->purgeIndexFile();
    }

    // look through all columns to match the incoming column names
    typedef std::vector<ibis::column*> colVector;
    std::set<const char*, ibis::lessi> used;
    colVector keys, load; // sort according to the keys
    for (ibis::table::stringList::const_iterator nit = cols.begin();
	 nit != cols.end(); ++ nit) {
	ibis::part::columnList::iterator it = columns.find(*nit);
	if (it != columns.end()) {
	    used.insert((*it).first);
	    if ((*it).second->upperBound() > (*it).second->lowerBound()) {
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
	else {
	    LOGGER(ibis::gVerbose > 2)
		<< "Warning -- " << evt << " can not find a column named "
		<< *nit;
	}
    }

    if (keys.empty()) { // use all integral values
	if (ibis::gVerbose > 0) {
	    if (cols.empty()) {
		LOGGER(true)
		    << evt << " -- user did not specify ordering keys, will "
		    "attempt to use all integer columns as ordering keys";
	    }
	    else {
		std::ostringstream oss;
		oss << cols[0];
		for (unsigned i = 1; i < cols.size(); ++ i)
		    oss << ", " << cols[i];
		LOGGER(true)
		    << evt << " -- user specified ordering keys \"" << oss.str()
		    << "\" does not match any numerical columns with more "
		    "than one distinct value, will attempt to use "
		    "all integer columns as ordering keys";
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
	if (keys.empty()) return -1; //no integral values to use as key
	if (keys.size() > 1) {
	    colVector tmp(keys.size());
	    array_t<uint32_t> idx;
	    width.sort(idx);
	    for (uint32_t i = 0; i < keys.size(); ++ i)
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
	LOGGER(ibis::gVerbose > 1)
	    << evt << " no keys found for sorting, do nothing";
	return -2;
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
		return -3;
	    }

	    const bool asc = (directions.size()>i?directions[i]:true);
	    switch (keys[i]->type()) {
	    case ibis::TEXT:
		ierr = sortStrings(* static_cast<std::vector<std::string>*>
				   (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::DOUBLE:
		ierr = sortValues(* static_cast<array_t<double>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::FLOAT:
		ierr = sortValues(* static_cast<array_t<float>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::ULONG:
		ierr = sortValues(* static_cast<array_t<uint64_t>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::LONG:
		ierr = sortValues(* static_cast<array_t<int64_t>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::CATEGORY:
	    case ibis::UINT:
		ierr = sortValues(* static_cast<array_t<uint32_t>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::INT:
		ierr = sortValues(* static_cast<array_t<int32_t>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::USHORT:
		ierr = sortValues(* static_cast<array_t<uint16_t>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::SHORT:
		ierr = sortValues(* static_cast<array_t<int16_t>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::UBYTE:
		ierr = sortValues(* static_cast<array_t<unsigned char>*>
				  (col->getArray()), starts, ind0, ind1, asc);
		break;
	    case ibis::BYTE:
		ierr = sortValues(* static_cast<array_t<signed char>*>
				  (col->getArray()), starts, ind0, ind1, asc);
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
    {
	ibis::util::logger lg(4);
	lg() << "DEBUG -- bord[" << ibis::part::name() << "]::reorder --\n";
	std::vector<bool> marks(ind1.size(), false);
	for (uint32_t i = 0; i < ind1.size(); ++ i) {
	    if (ibis::gVerbose > 6)
		lg() << "ind[" << i << "]=" << ind1[i] << "\n";
	    if (ind1[i] < marks.size())
		marks[ind1[i]] = true;
	}
	bool isperm = true;
	for (uint32_t i = 0; isperm && i < marks.size(); ++ i)
	    isperm = marks[i];
	if (isperm)
	    lg() << "array ind IS a permutation\n";
	else
	    lg() << "array ind is NOT a permutation\n";
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
	    return -4;
	}

	switch (load[i]->type()) {
	
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
	case ibis::CATEGORY:
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
	    ierr = reorderValues(* static_cast<array_t<signed char>*>
				 (col->getArray()), ind1);
	    break;
	default:
	    logWarning("reorder", "column %s type %s is not supported",
		       keys[i]->name(),
		       ibis::TYPESTRING[static_cast<int>(keys[i]->type())]);
	    break;
	}
    }
    return ierr;
} // ibis::bord::reorder

/// A simple sorting procedure.  The incoming values in vals are divided
/// into segements with starts.  Within each segement, this function orders
/// the values in ascending order by default unless ascending[i] is present
/// and is false.
///
/// @note This function uses a simple algorithm and requires space for a
/// copy of vals plus a copy of starts.
template <typename T>
long ibis::bord::sortValues(array_t<T>& vals,
			    array_t<uint32_t>& starts,
			    array_t<uint32_t>& idxout,
			    const array_t<uint32_t>& idxin,
			    bool ascending) const {
    ibis::horometer timer;
    if (ibis::gVerbose > 4)
	timer.start();

    if (vals.size() != nEvents ||
	(idxin.size() != vals.size() && ! idxin.empty())) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- bord[" << ibis::part::name() << "]::sortValues<"
	    << typeid(T).name() << "> can not proceed because array sizes do "
	    "not match, both vals.size(" << vals.size() << ") and idxin.size("
	    << idxin.size() << ") are expected to be " << nEvents;
	return -3;
    }
    if (idxin.empty() || starts.size() < 2 || starts[0] != 0
	|| starts.back() != vals.size()) {
	starts.resize(2);
	starts[0] = 0;
	starts[1] = vals.size();
	LOGGER(ibis::gVerbose > 1)
	    << "bord[" << ibis::part::name() << "]::sortValues<"
	    << typeid(T).name() << "> (re)set array starts to contain [0, "
	    << nEvents << "]";
    }

    uint32_t nseg = starts.size() - 1;
    if (nseg > nEvents) { // no sorting necessary
	idxout.copy(idxin);
    }
    else if (nseg > 1) { // sort multiple blocks
	idxout.resize(nEvents);
	array_t<uint32_t> starts2;
	array_t<T> tmp(nEvents);

	for (uint32_t iseg = 0; iseg < nseg; ++ iseg) {
	    const uint32_t segstart = starts[iseg];
	    const uint32_t segsize = starts[iseg+1]-starts[iseg];
	    if (segsize > 2) {
		// copy the segment into a temporary array, then sort it
		array_t<uint32_t> ind0;
		tmp.resize(segsize);
		for (unsigned i = 0; i < segsize; ++ i)
		    tmp[i] = vals[idxin[i+segstart]];
		tmp.sort(ind0);
		if (! ascending)
		    std::reverse(ind0.begin(), ind0.end());

		starts2.push_back(segstart);
		T last = tmp[ind0[0]];
		idxout[segstart] = idxin[ind0[0] + segstart];
		for (unsigned i = 1; i < segsize; ++ i) {
		    idxout[i+segstart] = idxin[ind0[i] + segstart];
		    if (tmp[ind0[i]] > last) {
			starts2.push_back(i + segstart);
			last = tmp[ind0[i]];
		    }
		}
	    }
	    else if (segsize == 2) { // two-element segment
		if (vals[idxin[segstart]] < vals[idxin[segstart+1]]) {
		    if (ascending) {// in the right order
			idxout[segstart] = idxin[segstart];
			idxout[segstart+1] = idxin[segstart+1];
		    }
		    else {
			idxout[segstart] = idxin[segstart+1];
			idxout[segstart+1] = idxin[segstart];
		    }
		    starts2.push_back(segstart);
		    starts2.push_back(segstart+1);
		}
		else if (vals[idxin[segstart]] == vals[idxin[segstart+1]]) {
		    idxout[segstart] = idxin[segstart];
		    idxout[segstart+1] = idxin[segstart+1];
		    starts2.push_back(segstart);
		}
		else { // assum the 1st value is larger (could be
		       // incomparable though)
		    if (ascending) {
			idxout[segstart] = idxin[segstart+1];
			idxout[segstart+1] = idxin[segstart];
		    }
		    else {
			idxout[segstart] = idxin[segstart];
			idxout[segstart+1] = idxin[segstart+1];
		    }
		    starts2.push_back(segstart);
		    starts2.push_back(segstart+1);
		}
	    }
	    else { // segment contains only one element
		starts2.push_back(segstart);
		idxout[segstart] = idxin[segstart];
	    }
	}
	starts2.push_back(nEvents);
	starts.swap(starts2);

	// place values in the new order
	tmp.resize(nEvents);
	for (uint32_t i = 0; i < nEvents; ++ i)
	    tmp[i] = vals[idxout[i]];
	vals.swap(tmp);
    }
    else { // all in one block
	idxout.resize(nEvents);
	for (uint32_t j = 0; j < nEvents; ++ j)
	    idxout[j] = j;
	ibis::util::sortKeys(vals, idxout);
	if (! ascending) {
	    std::reverse(vals.begin(), vals.end());
	    std::reverse(idxout.begin(), idxout.end());
	}

	starts.clear();
	starts.push_back(0U);
	T last = vals[0];
	for (uint32_t i = 1; i < nEvents; ++ i) {
	    if (vals[i] > last) {
		starts.push_back(i);
		last = vals[i];
	    }
	}
	starts.push_back(nEvents);
    }

    if (ibis::gVerbose > 4) {
	timer.stop();
	nseg = starts.size() - 1;
	LOGGER(1)
	    << "bord::sortValues -- reordered " << nEvents << " value"
	    << (nEvents>1 ? "s" : "") << " (into " << nseg
	    << " segment" << (nseg>1 ? "s" : "") << ") in "
	    << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed)";
    }
    return nEvents;
} // ibis::bord::sortValues

/// Sort the string values.  It preserves the previous order determined
/// represented by idxin and starts.
long ibis::bord::sortStrings(std::vector<std::string>& vals,
			     array_t<uint32_t>& starts,
			     array_t<uint32_t>& idxout,
			     const array_t<uint32_t>& idxin,
			     bool ascending) const {
    ibis::horometer timer;
    if (ibis::gVerbose > 4)
	timer.start();

    if (vals.size() != nEvents ||
	(idxin.size() != vals.size() && ! idxin.empty())) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- bord[" << ibis::part::name() << "]::sortStrings "
	    " can not proceed because array sizes do not match, both "
	    "vals.size(" << vals.size() << ") and idxin.size(" << idxin.size()
	    << ") are expected to be " << nEvents;
	return -3;
    }
    if (idxin.empty() || starts.size() < 2 || starts[0] != 0
	|| starts.back() != vals.size()) {
	starts.resize(2);
	starts[0] = 0;
	starts[1] = vals.size();
	LOGGER(ibis::gVerbose > 1)
	    << "bord[" << ibis::part::name() << "]::sortStrings -- (re)set "
	    "array starts to contain [0, " << nEvents << "]";
    }

    uint32_t nseg = starts.size() - 1;
    if (nseg > nEvents) { // no sorting necessary
	idxout.copy(idxin);
    }
    else if (nseg > 1) { // sort multiple blocks
	idxout.resize(nEvents);
	array_t<uint32_t> starts2;
	std::vector<std::string> tmp(nEvents);

	for (uint32_t iseg = 0; iseg < nseg; ++ iseg) {
	    const uint32_t segstart = starts[iseg];
	    const uint32_t segsize = starts[iseg+1]-starts[iseg];
	    if (segsize > 2) {
		// copy the segment into a temporary array, then sort it
		tmp.resize(segsize);
		array_t<uint32_t> ind0(segsize);
		for (unsigned i = segstart; i < starts[iseg+1]; ++ i) {
		    tmp[i-segstart] = vals[idxin[i]];
		    ind0[i-segstart] = idxin[i];
		}
		// sort tmp and move ind0
		ibis::util::sortStrings(tmp, ind0);
		if (! ascending)
		    std::reverse(ind0.begin(), ind0.end());

		starts2.push_back(segstart);
		uint32_t last = 0;
		idxout[segstart] = ind0[0];
		for (unsigned i = 1; i < segsize; ++ i) {
		    idxout[i+segstart] = ind0[i];
		    if (tmp[i].compare(tmp[last]) != 0) {
			starts2.push_back(i + segstart);
			last = i;
		    }
		}
	    }
	    else if (segsize == 2) {
		int cmp = vals[idxin[segstart]].compare
		    (vals[idxin[segstart+1]]);
		if (cmp < 0) { // in the right order, different strings
		    if (ascending) {
			idxout[segstart] = idxin[segstart];
			idxout[segstart+1] = idxin[segstart+1];
		    }
		    else {
			idxout[segstart] = idxin[segstart+1];
			idxout[segstart+1] = idxin[segstart];
		    }
		    starts2.push_back(segstart);
		    starts2.push_back(segstart+1);
		}
		else if (cmp == 0) { // two strings are the same
		    idxout[segstart] = idxin[segstart];
		    idxout[segstart+1] = idxin[segstart+1];
		    starts2.push_back(segstart);
		}
		else { // in the wrong order, different strings
		    if (ascending) {
			idxout[segstart] = idxin[segstart+1];
			idxout[segstart+1] = idxin[segstart];
		    }
		    else {
			idxout[segstart] = idxin[segstart];
			idxout[segstart+1] = idxin[segstart+1];
		    }
		    starts2.push_back(segstart);
		    starts2.push_back(segstart+1);
		}
	    }
	    else { // segment contains only one element
		starts2.push_back(segstart);
		idxout[segstart] = idxin[segstart];
	    }
	}
	starts2.push_back(nEvents);
	starts.swap(starts2);
	// place values in the new order
	tmp.resize(nEvents);
	for (uint32_t i = 0; i < nEvents; ++ i)
	    tmp[i].swap(vals[idxout[i]]);
	vals.swap(tmp);
    }
    else { // all in one block
	idxout.resize(nEvents);
	for (uint32_t j = 0; j < nEvents; ++ j)
	    idxout[j] = j;
	ibis::util::sortStrings(vals, idxout);
	if (! ascending) {
	    std::reverse(vals.begin(), vals.end());
	    std::reverse(idxout.begin(), idxout.end());
	}

	starts.clear();
	starts.push_back(0U);
	uint32_t last = 0;
	for (uint32_t i = 1; i < nEvents; ++ i) {
	    if (vals[i].compare(vals[last]) != 0) {
		starts.push_back(i);
		last = i;
	    }
	}
	starts.push_back(nEvents);
    }

    if (ibis::gVerbose > 4) {
	timer.stop();
	nseg = starts.size() - 1;
	logMessage("sortStrings", "reordered %lu string%s (into %lu "
		   "segment%s) in %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(nEvents), (nEvents>1 ? "s" : ""),
		   static_cast<long unsigned>(nseg), (nseg>1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return nEvents;
} // ibis::bord::sortStrings

template <typename T>
long ibis::bord::reorderValues(array_t<T>& vals,
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
} // ibis::bord::reorderValues

/// Reorder the vector of strings.  To avoid recreating the content of the
/// string values, this function uses swap operations to move the existing
/// strings into their new locations.  It only works if ind is a proper
/// permutation of integers between 0 and vals.size() (include 0 but
/// exclude vals.size()), however, it does not check whether the input
/// array is a proper permutation.
long ibis::bord::reorderStrings(std::vector<std::string>& vals,
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
	tmp[i].swap(vals[ind[i]]);
    tmp.swap(vals);
    return nEvents;
} // ibis::bord::reorderValues

void ibis::bord::reverseRows() {
    for (ibis::part::columnList::iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	reinterpret_cast<ibis::bord::column*>((*it).second)->reverseRows();
    }
} // ibis::bord::reverseRows

int ibis::bord::limit(uint32_t nr) {
    int ierr = 0;
    if (nEvents <= nr) return ierr;

    for (ibis::part::columnList::iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	ierr = reinterpret_cast<ibis::bord::column*>((*it).second)->limit(nr);
	if (ierr < 0) return ierr;
    }
    nEvents = nr;
    return ierr;
} // ibis::bord::limit

/// Evaluate the arithmetic expressions in the select clause to derive an
/// in-memory data table.
ibis::table*
ibis::bord::evaluateTerms(const ibis::selectClause& sel,
			  const char* desc) const {
    std::string mydesc;
    if (desc == 0 || *desc == 0) {
	mydesc = sel.getString();
	mydesc += " on ";
	mydesc += m_desc;
	desc = mydesc.c_str();
    }
    std::string tn = ibis::util::shortName(desc);
    if (nEvents == 0 || columns.empty() || sel.empty()) {
	return new ibis::tabula(tn.c_str(), desc, nEvents);
    }

    long ierr;
    ibis::bitvector msk;
    msk.set(1, nEvents);
    ibis::table::bufferList  buf;
    ibis::table::typeList    ct;
    ibis::table::stringList  cn;
    ibis::table::stringList  cd;
    std::vector<std::string> cdesc;
    ibis::util::guard gbuf =
	ibis::util::makeGuard(ibis::table::freeBuffers, ibis::util::ref(buf),
			      ibis::util::ref(ct));
    for (uint32_t j = 0; j < sel.aggSize(); ++ j) {
	const ibis::math::term* t = sel.aggExpr(j);
	std::string desc = sel.aggDescription(j);
	LOGGER(ibis::gVerbose > 4)
	    << "bord[" << ibis::part::name() << "] -- evaluating \""
	    << desc << '"';

	switch (t->termType()) {
	default: {
	    cdesc.push_back(desc);
	    cn.push_back(sel.aggName(j));
	    ct.push_back(ibis::DOUBLE);
	    buf.push_back(new ibis::array_t<double>(nEvents));
	    ierr = calculate
		(*t, msk, *static_cast< ibis::array_t<double>* >(buf.back()));
	    if (ierr != (long)nEvents) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- bord::evaluateTerms(" << desc
		    << ") failed to evaluate term " << j << " ("
		    << cdesc.back() << "), ierr = " << ierr;
		return 0;
	    }
	    break;}
	case ibis::math::NUMBER: { // a fixed number
	    cn.push_back(sel.aggName(j));
	    ct.push_back(ibis::DOUBLE);
	    cdesc.push_back(desc);
	    buf.push_back(new ibis::array_t<double>(nEvents, t->eval()));
	    break;}
	case ibis::math::STRING: { // a string literal
	    cn.push_back(sel.aggName(j));
	    ct.push_back(ibis::CATEGORY);
	    cdesc.push_back(desc);
	    buf[j] = new std::vector<std::string>
		(nEvents, (const char*)*
		 static_cast<const ibis::math::literal*>(t));
	    break;}
	case ibis::math::VARIABLE: {
	    const char* cn1 = static_cast<const ibis::math::variable*>
		(t)->variableName();
	    if (*cn1 == '*') { // must be count(*)
		cn.push_back(cn1);
		cdesc.push_back(desc);
		ct.push_back(ibis::UINT);
		cd.push_back(cdesc.back().c_str());
		ibis::part::columnList::const_iterator it = columns.find("*");
		if (it == columns.end()) { // new values
		    buf.push_back(new ibis::array_t<uint32_t>(nEvents, 1U));
		}
		else { // copy existing values
		    buf.push_back(new ibis::array_t<uint32_t>
				  (it->second->getRawData()));
		}
		continue;
	    }

	    ibis::part::columnList::const_iterator it = columns.find(cn1);
	    if (it == columns.end()) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- bord::evaluateTerms(" << desc
		    << ") failed to find a column " << j << " named " << cn1;
		continue;
	    }

	    cn.push_back(cn1);
	    cdesc.push_back(desc);
	    ct.push_back(it->second->type());
	    switch (ct.back()) {
	    default: {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- bord::evaluateTerms(" << desc
		    << ") can not handle column " << j << " type "
		    << ibis::TYPESTRING[(int)ct.back()];
		return 0;}
	    case ibis::BYTE:
		buf.push_back(new ibis::array_t<signed char>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values for column " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::UBYTE:
		buf.push_back(new ibis::array_t<unsigned char>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::SHORT:
		buf.push_back(new ibis::array_t<int16_t>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values for column " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::USHORT:
		buf.push_back(new ibis::array_t<uint16_t>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::INT:
		buf.push_back(new ibis::array_t<int32_t>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values for column " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::UINT:
		buf.push_back(new ibis::array_t<uint32_t>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::LONG:
		buf.push_back(new ibis::array_t<int64_t>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values for column " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::ULONG:
		buf.push_back(new ibis::array_t<uint64_t>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::FLOAT:
		buf.push_back(new ibis::array_t<float>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values for column " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::DOUBLE:
		buf.push_back(new ibis::array_t<double>);
		ierr = it->second->selectValues(msk, buf.back());
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got " << ierr;
		    return 0;
		}
		break;
	    case ibis::TEXT:
	    case ibis::CATEGORY:
		buf.push_back(it->second->selectStrings(msk));
		if (buf[j] == 0 || static_cast<std::vector<std::string>*>
		    (buf[j])->size() != nEvents) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- bord::evaluateTerms(" << desc
			<< ") expected to retrieve " << nEvents
			<< " values " << j << " (" << cn1
			<< ", " << ibis::TYPESTRING[(int)ct.back()]
			<< "), but got "
			<< (buf.back() != 0 ?
			    static_cast<std::vector<std::string>*>
			    (buf.back())->size() : 0U);
		    return 0;
		}
		break;
	    }
	    break;}
	} // switch (t->termType())

	cn.back() = skipPrefix(cn.back());
	cd.push_back(cdesc[j].c_str());
    } // for (uint32_t j...

    std::auto_ptr<ibis::bord> brd1
	(new ibis::bord(tn.c_str(), desc, (uint64_t)nEvents, buf, ct, cn, &cd));
    if (brd1.get() != 0)
	gbuf.dismiss();
    return brd1.release();
} // ibis::bord::evaluateTerms

/// Convert the integer representation of categorical columns back to the
/// string representation.  The argument is used to determine if the
/// original column was categorical values.
///
/// Upon successful completion of this function, it returns the number of
/// rows in the column.  It returns a negative number to indicate errors.
int ibis::bord::restoreCategoriesAsStrings(const ibis::part& ref) {
    int ierr = nEvents;
    for (columnList::iterator it = columns.begin();
	 it != columns.end(); ++ it) {
	if (it->second->type() == ibis::UINT) {
	    const ibis::category* cat =
		dynamic_cast<const ibis::category*>(ref.getColumn(it->first));
	    if (cat != 0) {
		ierr = static_cast<ibis::bord::column*>(it->second)
		    ->restoreCategoriesAsStrings(*cat);
		if (ierr < 0)
		    return ierr;
	    }
	}
    }
    return ierr;
} // ibis::bord::restoreCategoriesAsStrings

/// Copy the type and values of the named column.  It uses a shallow copy
/// for integers and floating-point numbers.
void ibis::bord::copyColumn(const char* nm, ibis::TYPE_T& t, void*& buf,
			    const ibis::dictionary*& dic) const {
    columnList::const_iterator it = columns.find(nm);
    if (it == columns.end()) {
	nm = skipPrefix(nm);
	it = columns.find(nm);
    }
    if (it == columns.end()) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- bord[" << name_ << "]::copyColumn failed to find "
	    "a column named " << nm;
	t = ibis::UNKNOWN_TYPE;
	buf = 0;
	return;
    }

    const ibis::column& col = *(it->second);
    t = col.type();
    switch (col.type()) {
    case ibis::BYTE:
	buf = new ibis::array_t<signed char>;
	col.getValuesArray(buf);
	break;
    case ibis::UBYTE:
	buf = new ibis::array_t<unsigned char>;
	col.getValuesArray(buf);
	break;
    case ibis::SHORT:
	buf = new ibis::array_t<int16_t>;
	col.getValuesArray(buf);
	break;
    case ibis::USHORT:
	buf = new ibis::array_t<uint16_t>;
	col.getValuesArray(buf);
	break;
    case ibis::INT:
	buf = new ibis::array_t<int32_t>;
	col.getValuesArray(buf);
	break;
    case ibis::UINT: {
	buf = new ibis::array_t<uint32_t>;
	col.getValuesArray(buf);
	const ibis::bord::column *bc =
	    dynamic_cast<const ibis::bord::column*>(&col);
	if (bc != 0)
	    dic = bc->getDictionary();
	break;}
    case ibis::LONG:
	buf = new ibis::array_t<int64_t>;
	col.getValuesArray(buf);
	break;
    case ibis::ULONG:
	buf = new ibis::array_t<uint64_t>;
	col.getValuesArray(buf);
	break;
    case ibis::FLOAT:
	buf = new ibis::array_t<float>;
	col.getValuesArray(buf);
	break;
    case ibis::DOUBLE:
	buf = new ibis::array_t<double>;
	col.getValuesArray(buf);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	dic = static_cast<const ibis::category&>(col).getDictionary();
	buf = new std::vector<std::string>;
	col.getValuesArray(buf);
	break;
    default:
	t = ibis::UNKNOWN_TYPE;
	buf = 0;
	break;
    }
} // ibis::bord::copyColumn

int ibis::bord::renameColumns(const ibis::selectClause& sel) {
    ibis::selectClause::nameMap nmap;
    int ierr = sel.getAliases(nmap);
    if (ierr <= 0) return ierr;

    for (ibis::selectClause::nameMap::const_iterator it = nmap.begin();
	 it != nmap.end(); ++ it) {
	ibis::part::columnList::iterator cit = columns.find(it->first);
	if (cit != columns.end()) {
	    ibis::column *col = cit->second;
	    columns.erase(cit);
	    col->name(it->second);
	    columns[col->name()] = col;
	    LOGGER(ibis::gVerbose > 5)
		<< "bord::renameColumns -- " << it->first << " --> "
		<< col->name();
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::renameColumns can not find a column named "
		<< it->first << " to change it to " << it->second;
	}
    }

    ierr = 0;
    // re-establish the order of columns according to xtrms_
    colorder.clear();
    const unsigned ntrms = sel.getTerms().size();
    for (unsigned j = 0; j < ntrms; ++ j) {
	const char *tn = sel.termName(j);
	const ibis::column *col = getColumn(tn);
	if (col != 0) {
	    colorder.push_back(col);
	}
	else {
	    -- ierr;
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::renameColumns can not find a column named "
		<< tn << ", but the select clause contains the name as term "
		<< j;
	}
    }
    return ierr;
} // ibis::bord::renameColumns

/// Append the values marked 1 to this data partition.  This is an
/// in-memory operation and therefore can only accomodate relatively small
/// number of rows.
///
/// It returns the number of rows added upon successful completion.
/// Otherwise it returns a negative number to indicate error.
int ibis::bord::append(const ibis::selectClause& sc, const ibis::part& prt,
		       const ibis::bitvector &mask) {
    int ierr = 0;
    if (mask.cnt() == 0) return ierr;

    const ibis::selectClause::StringToInt& colmap = sc.getOrdered();
    const uint32_t nagg = sc.aggSize();
    const uint32_t nh   = nEvents;
    const uint32_t nqq  = mask.cnt();
    std::string mesg    = "bord[";
    mesg += m_name;
    mesg += "]::append";
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg;
	lg() << mesg << " -- to process " << nqq << " row" << (nqq>1?"s":"")
	     << " from " << prt.name() << ", # of existing rows = " << nh;
	if (ibis::gVerbose > 5) {
	    lg() << "\n    colmap[" << colmap.size() << "]";
	    for (ibis::selectClause::StringToInt::const_iterator it
                     = colmap.begin(); it != colmap.end(); ++ it) {
                lg() << "\n\t" << it->first << " --> " << it->second;
                if (it->second < nagg)
                    lg() << " (" << *(sc.aggExpr(it->second)) << ")";
            }
	}
    }

    amask.adjustSize(0, nh);
    ibis::bitvector newseg; // mask for the new segment of data
    newseg.set(1, nqq);
    for (columnList::iterator cit = columns.begin();
	 cit != columns.end() && ierr >= 0; ++ cit) {
	ibis::bord::column& col =
	    *(static_cast<ibis::bord::column*>(cit->second));
	ibis::selectClause::StringToInt::const_iterator mit =
	    colmap.find(cit->first);
	if (mit == colmap.end()) {
	    mit = colmap.find(col.description());
	    if (mit == colmap.end()) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " failed to locate "
		    << cit->first << " in the list of names in " << sc;
		return -13;
	    }
	}
	const uint32_t itm = mit->second;
	if (itm >= nagg) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- " << mesg << " mapped " << col.name()
		<< " into term " << itm << " which is outside of " << sc;
	    return -14;
	}
	const ibis::math::term *aterm = sc.aggExpr(itm);

	if (aterm->termType() != ibis::math::VARIABLE) {
	    if (aterm->termType() == ibis::math::UNDEF_TERM) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " -- can not handle a "
		    "math::term of undefined type";
		ierr = -15;
	    }
	    else {
		array_t<double> tmp;
		ierr = prt.calculate(*aterm, mask, tmp);
		if (ierr > 0) {
		    LOGGER(ibis::gVerbose > 2)
			<< mesg << " -- adding " << tmp.size() << " element"
			<< (tmp.size()>1?"s":"") << " to column " << cit->first
			<< " from " << *aterm;
		    ierr = col.append(&tmp, newseg);
		}
	    }
	}
	else {
	    const ibis::math::variable &var =
		*static_cast<const ibis::math::variable*>(aterm);

	    const ibis::column* scol = prt.getColumn(var.variableName());
	    if (scol == 0) {
		if (*(var.variableName()) == '*') { // counts
		    col.addCounts(nEvents+nqq);
		}
		else {
		    LOGGER(ibis::gVerbose > 1)
			<< "Warning -- " << mesg << " -- \""
			<< var.variableName()
			<< "\" is not a column of partition " << prt.name();
		    ierr = -16;
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 4)
		    << mesg << " -- adding " << nqq << " element"
		    << (nqq>1?"s":"") << " to column \"" << cit->first
		    << "\" from column \"" << scol->name()
		    << "\" of partition " << prt.name();
		ierr = col.append(*scol, mask);
	    }
	}
    }
    if (ierr >= 0) {
	ierr = nqq;
	nEvents += nqq;
	amask.adjustSize(nEvents, nEvents);
	LOGGER(ibis::gVerbose > 3)
	    << mesg << " -- added " << nqq << " row" << (nqq>1?"s":"")
	    << " to make a total of " << nEvents;
    }
    return ierr;
} // ibis::bord::append

/// Append the rows satisfying the specified range expression.  This
/// function assumes the select clause only needs the column involved in
/// the range condition to complete its operations.
///
/// It returns the number rows satisfying the range expression on success,
/// otherwise it returns a negative value.
int ibis::bord::append(const ibis::selectClause &sc, const ibis::part& prt,
		       const ibis::qContinuousRange &cnd) {
    const ibis::column *scol = prt.getColumn(cnd.colName());
    if (scol == 0) return -18;
    std::string mesg = "bord[";
    mesg += m_name;
    mesg += "]::append";

    // use a temporary bord to hold the new data
    ibis::bord btmp;
    ibis::bord::column *ctmp = new ibis::bord::column
	(&btmp, scol->type(), scol->name());
    btmp.columns[ctmp->name()] = ctmp;
    int ierr = ctmp->append(*scol, cnd);
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- " << mesg << " failed to retrieve values "
	    << "satisfying \"" << cnd << " from partition " << prt.name()
	    << ", ierr = " << ierr;
	return -17;
    }
    if (ierr == 0)
	return ierr;

    btmp.nEvents = ierr;
    const uint32_t nh = nEvents;
    const uint32_t nqq = ierr;
    amask.adjustSize(0, nh);
    ibis::bitvector newseg; // mask for the new segment of data
    newseg.set(1, nqq);
    const ibis::selectClause::StringToInt& colmap = sc.getOrdered();
    for (columnList::iterator cit = columns.begin();
	 cit != columns.end() && ierr >= 0; ++ cit) {
	ibis::bord::column& col =
	    *(static_cast<ibis::bord::column*>(cit->second));
	ibis::selectClause::StringToInt::const_iterator mit =
	    colmap.find(cit->first);
	if (mit == colmap.end()) {
	    mit = colmap.find(col.description());
	    if (mit == colmap.end()) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " failed to locate "
		    << cit->first << " in the list of names in " << sc;
		return -13;
	    }
	}
	const uint32_t itm = mit->second;
	if (itm >= sc.aggSize()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- " << mesg << " mapped " << col.name()
		<< " into term " << itm << " which is outside of " << sc;
	    return -14;
	}
	const ibis::math::term *aterm = sc.aggExpr(itm);

	if (aterm->termType() != ibis::math::VARIABLE) {
	    if (aterm->termType() == ibis::math::UNDEF_TERM) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " -- can not handle a "
		    "math::term of undefined type";
		ierr = -15;
	    }
	    else {
		array_t<double> tmp;
		ierr = btmp.calculate(*aterm, newseg, tmp);
		if (ierr > 0) {
		    LOGGER(ibis::gVerbose > 2)
			<< mesg << " -- adding " << tmp.size() << " element"
			<< (tmp.size()>1?"s":"") << " to column " << cit->first
			<< " from " << *aterm;
		    ierr = col.append(&tmp, newseg);
		}
	    }
	}
	else {
	    const ibis::math::variable &var =
		*static_cast<const ibis::math::variable*>(aterm);
	    scol = btmp.getColumn(var.variableName());
	    if (scol == 0) {
		if (*(var.variableName()) == '*') { // counts
		    col.addCounts(nEvents+nqq);
		}
		else {
		    LOGGER(ibis::gVerbose > 1)
			<< "Warning -- " << mesg << " -- \""
			<< var.variableName()
			<< "\" is unexpected";
		    ierr = -16;
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 4)
		    << mesg << " -- adding " << nqq << " element"
		    << (nqq>1?"s":"") << " to column " << cit->first
		    << " from column " << scol->name()
		    << " of partition " << prt.name();
		ierr = col.append(*scol, newseg);
	    }
	}
    }
    if (ierr >= 0) {
	ierr = nqq;
	nEvents += nqq;
	amask.adjustSize(nEvents, nEvents);
	LOGGER(ibis::gVerbose > 3)
	    << mesg << " -- added " << nqq << " row" << (nqq>1?"s":"")
	    << " to make a total of " << nEvents;
    }
    return ierr;
} // ibis::bord::append

ibis::table::cursor* ibis::bord::createCursor() const {
    return new ibis::bord::cursor(*this);
} // ibis::bord::createCursor

/// Allocate a buffer of the specified type and size.
void* ibis::table::allocateBuffer(ibis::TYPE_T type, size_t sz) {
    void* ret = 0;
    switch (type) {
    default:
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- table::allocateBuffer("
	    << ibis::TYPESTRING[(int)type] << ", "
	    << sz << ") unable to handle the data type";
	break;
    case ibis::OID:
	ret = new array_t<rid_t>(sz);
	break;
    case ibis::BYTE:
	ret = new array_t<signed char>(sz);
	break;
    case ibis::UBYTE:
	ret = new array_t<unsigned char>(sz);
	break;
    case ibis::SHORT:
	ret = new array_t<int16_t>(sz);
	break;
    case ibis::USHORT:
	ret = new array_t<uint16_t>(sz);
	break;
    case ibis::INT:
	ret = new array_t<int32_t>(sz);
	break;
    case ibis::UINT:
	ret = new array_t<uint32_t>(sz);
	break;
    case ibis::LONG:
	ret = new array_t<int64_t>(sz);
	break;
    case ibis::ULONG:
	ret = new array_t<uint64_t>(sz);
	break;
    case ibis::FLOAT:
	ret = new array_t<float>(sz);
	break;
    case ibis::DOUBLE:
	ret = new array_t<double>(sz);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	ret = new std::vector<std::string>(sz);
        break;
    }
    return ret;
} // ibis::table::allocateBuffer

/// Freeing a buffer for storing in-memory values.
void ibis::table::freeBuffer(void *buffer, ibis::TYPE_T type) {
    if (buffer != 0) {
	switch (type) {
	default:
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- table::freeBuffer(" << buffer << ", "
		<< (int) type << ") unable to handle data type "
		<< ibis::TYPESTRING[(int)type];
	    break;
	case ibis::OID:
	    delete static_cast<array_t<rid_t>*>(buffer);
	    break;
	case ibis::BYTE:
	    delete static_cast<array_t<signed char>*>(buffer);
	    break;
	case ibis::UBYTE:
	    delete static_cast<array_t<unsigned char>*>(buffer);
	    break;
	case ibis::SHORT:
	    delete static_cast<array_t<int16_t>*>(buffer);
	    break;
	case ibis::USHORT:
	    delete static_cast<array_t<uint16_t>*>(buffer);
	    break;
	case ibis::INT:
	    delete static_cast<array_t<int32_t>*>(buffer);
	    break;
	case ibis::UINT:
	    delete static_cast<array_t<uint32_t>*>(buffer);
	    break;
	case ibis::LONG:
	    delete static_cast<array_t<int64_t>*>(buffer);
	    break;
	case ibis::ULONG:
	    delete static_cast<array_t<uint64_t>*>(buffer);
	    break;
	case ibis::FLOAT:
	    delete static_cast<array_t<float>*>(buffer);
	    break;
	case ibis::DOUBLE:
	    delete static_cast<array_t<double>*>(buffer);
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    delete static_cast<std::vector<std::string>*>(buffer);
	    break;
	}
    }
} // ibis::table::freeBuffer

/// Freeing a list of buffers.
void ibis::table::freeBuffers(ibis::table::bufferList& buf,
			      ibis::table::typeList& typ) {
    LOGGER(ibis::gVerbose > 3)
	<< "table::freeBuffers to free buf[" << buf.size() << "] and typ["
	<< typ.size() << "]";
    const size_t nbt = (buf.size() <= typ.size() ? buf.size() : typ.size());
    LOGGER((nbt < buf.size() || nbt < typ.size()) && ibis::gVerbose > 1)
	<< "Warning -- freeBuffers expects buf[" << buf.size()
	<< "] and typ["	<< typ.size()
	<< "] to be the same size, but they are not";

    for (size_t j = 0; j < buf.size(); ++ j) {
	if (buf[j] != 0) {
	    switch (typ[j]) {
	    default:
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- table::freeBuffers cann't free "
		    << buf[j] << " of type " << ibis::TYPESTRING[(int)typ[j]];
		break;
	    case ibis::OID:
		delete static_cast<array_t<ibis::rid_t>*>(buf[j]);
		break;
	    case ibis::BYTE:
		delete static_cast<array_t<signed char>*>(buf[j]);
		break;
	    case ibis::UBYTE:
		delete static_cast<array_t<unsigned char>*>(buf[j]);
		break;
	    case ibis::SHORT:
		delete static_cast<array_t<int16_t>*>(buf[j]);
		break;
	    case ibis::USHORT:
		delete static_cast<array_t<uint16_t>*>(buf[j]);
		break;
	    case ibis::INT:
		delete static_cast<array_t<int32_t>*>(buf[j]);
		break;
	    case ibis::UINT:
		delete static_cast<array_t<uint32_t>*>(buf[j]);
		break;
	    case ibis::LONG:
		delete static_cast<array_t<int64_t>*>(buf[j]);
		break;
	    case ibis::ULONG:
		delete static_cast<array_t<uint64_t>*>(buf[j]);
		break;
	    case ibis::FLOAT:
		delete static_cast<array_t<float>*>(buf[j]);
		break;
	    case ibis::DOUBLE:
		delete static_cast<array_t<double>*>(buf[j]);
		break;
	    case ibis::TEXT:
	    case ibis::CATEGORY:
		delete static_cast<std::vector<std::string>*>(buf[j]);
		break;
	    }
	}
    }
    buf.clear();
    typ.clear();
} // ibis::table::freeBuffers

/// Constructor.
ibis::bord::column::column(const ibis::bord *tbl, ibis::TYPE_T t,
			   const char *cn, void *st, const char *de,
			   double lo, double hi)
    : ibis::column(tbl, t, cn, de, lo, hi), buffer(st), dic(0) {
    if (buffer != 0) { // check the size of buffer
	uint32_t nr = 0;
	switch (m_type) {
	case ibis::BYTE: {
	    nr = static_cast<array_t<signed char>*>(st)->size();
	    break;}
	case ibis::UBYTE: {
	    nr = static_cast<array_t<unsigned char>*>(st)->size();
	    break;}
	case ibis::SHORT: {
	    nr = static_cast<array_t<int16_t>*>(st)->size();
	    break;}
	case ibis::USHORT: {
	    nr = static_cast<array_t<uint16_t>*>(st)->size();
	    break;}
	case ibis::INT: {
	    nr = static_cast<array_t<int32_t>*>(st)->size();
	    break;}
	case ibis::UINT: {
	    nr = static_cast<array_t<uint32_t>*>(st)->size();
	    break;}
	case ibis::LONG: {
	    nr = static_cast<array_t<int64_t>*>(st)->size();
	    break;}
	case ibis::ULONG: {
	    nr = static_cast<array_t<uint64_t>*>(st)->size();
	    break;}
	case ibis::FLOAT: {
	    nr = static_cast<array_t<float>*>(st)->size();
	    break;}
	case ibis::DOUBLE: {
	    nr = static_cast<array_t<double>*>(st)->size();
	    break;}
	case ibis::TEXT: {
	    nr = static_cast<std::vector<std::string>*>(st)->size();
	    break;}
	case ibis::CATEGORY: {
	    nr = static_cast<std::vector<std::string>*>(st)->size();
	    // std::vector<std::string> *stv =
	    // 	static_cast<std::vector<std::string>*>(st);
	    // std::vector<std::string> * tmpSortedDic =
	    // 	new std::vector<std::string>(nr);
	    // for (size_t i = 0 ; i < nr ; i++)
	    // 	(*tmpSortedDic)[i] = (((*stv)[i]));
	    // sort(tmpSortedDic->begin(), tmpSortedDic->end());
	    // dic = new ibis::dictionary();
	    // dic->insert("");
	    // for (size_t i = 0 ; i < tmpSortedDic->size() ; i++)
	    // 	dic->insert((*tmpSortedDic)[i].c_str());
	    // delete tmpSortedDic;
	    // array_t<uint32_t> *tmp = new array_t<uint32_t>();
	    // tmp->resize(nr);
	    // for (size_t i = 0 ; i < nr ; i++)
	    // 	(*tmp)[i] = dic->insert(((*stv)[i]).c_str());
	    // buffer = tmp;
	    // delete stv;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::column::ctor can not handle column ("
		<< cn << ") with type " << ibis::TYPESTRING[(int)t];
	    throw "bord::column unexpected type";}
	}
	mask_.adjustSize(nr, tbl->nRows());
	LOGGER(nr != tbl->nRows() && ibis::gVerbose > 4)
	    << "bord::column " << tbl->m_name << '.' << cn << " has "
	    << nr << " row" << (nr>1?"s":"") << ", but expected "
	    << tbl->nRows();
    }
    else { // allocate buffer
	switch (m_type) {
	case ibis::BYTE: {
	    buffer = new array_t<signed char>;
	    break;}
	case ibis::UBYTE: {
	    buffer = new array_t<unsigned char>;
	    break;}
	case ibis::SHORT: {
	    buffer = new array_t<int16_t>;
	    break;}
	case ibis::USHORT: {
	    buffer = new array_t<uint16_t>;
	    break;}
	case ibis::INT: {
	    buffer = new array_t<int32_t>;
	    break;}
	case ibis::UINT: {
	    buffer = new array_t<uint32_t>;
	    break;}
	case ibis::LONG: {
	    buffer = new array_t<int64_t>;
	    break;}
	case ibis::ULONG: {
	    buffer = new array_t<uint64_t>;
	    break;}
	case ibis::FLOAT: {
	    buffer = new array_t<float>;
	    break;}
	case ibis::DOUBLE: {
	    buffer = new array_t<double>;
	    break;}
	case ibis::TEXT:{
	    buffer = new std::vector<std::string>;
	    break;}
	case ibis::CATEGORY: {
	    buffer = new std::vector<std::string>;
	    //dic = new ibis::dictionary();
	    break;}
	default: {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- bord::column::ctor can not handle column ("
		<< cn << ") with type " << ibis::TYPESTRING[(int)t];
	    throw "bord::column unexpected type";}
	}
    }
} // ibis::bord::column::column

/// Constructor.
///@note Transfer the ownership of @c st to the new @c column object.
ibis::bord::column::column(const ibis::bord *tbl,
			   const ibis::column& old, void *st)
    : ibis::column(tbl, old.type(), old.name(), old.description(),
		   old.lowerBound(), old.upperBound()),
      buffer(st), dic(0) {
} // ibis::bord::column::column

/// Copy constructor.  Performs a shallow copy of the storage buffer.
ibis::bord::column::column(const ibis::bord::column &c)
    : ibis::column(c), buffer(0), dic(c.dic) {
    switch (c.m_type) {
    case ibis::BYTE: {
	buffer = new array_t<signed char>
	    (* static_cast<array_t<signed char>*>(c.buffer));
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
    case ibis::CATEGORY:
    case ibis::TEXT: {
	buffer = new std::vector<std::string>
	    (* static_cast<std::vector<std::string>*>(c.buffer));
	break;}
    default: {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- bord::column::ctor can not copy column ("
	    << c.name() << ") with type " << ibis::TYPESTRING[(int)c.type()];
	break;}
    }
    m_sorted = c.m_sorted;
} // ibis::bord::column::column

ibis::bord::column::~column() {
    switch (m_type) {
    case ibis::BYTE: {
	delete static_cast<array_t<signed char>*>(buffer);
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
    case ibis::CATEGORY:
    case ibis::TEXT: {
	delete static_cast<std::vector<std::string>*>(buffer);
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- bord::column["
	    << (thePart ? thePart->name() : "") << '.' << m_name
	    << "] has an unexpected type "
	    << ibis::TYPESTRING[(int)m_type] << " (" << (int)m_type
	    << ')';
	break;}
    }
#if defined(DEBUG) || defined(_DEBUG)
    LOGGER(ibis::gVerbose > 5)
	<< "DEBUG -- bord::column[" << (thePart ? thePart->name() : "")
	<< '.' << m_name << "] freed buffer at " << buffer;
#endif
} // ibis::bord::column::~column

/// Retrieve the raw data buffer as an ibis::fileManager::storage.  Since
/// this function exposes the internal storage representation, it should
/// not be relied upon for general uses.  This is mostly a convenience
/// thing for FastBit internal development!
///
/// @note Only fix-sized columns are stored using
/// ibis::fileManager::storage objects.  It will return a nil pointer for
/// string-valued columns.
ibis::fileManager::storage*
ibis::bord::column::getRawData() const {
    ibis::fileManager::storage *str = 0;
    switch (m_type) {
    case ibis::OID: {
	str = static_cast<array_t<ibis::rid_t>*>(buffer)->getStorage();
	break;}
    case ibis::BYTE: {
	str = static_cast<array_t<signed char>*>(buffer)->getStorage();
	break;}
    case ibis::UBYTE: {
	str = static_cast<array_t<unsigned char>*>(buffer)->getStorage();
	break;}
    case ibis::SHORT: {
	str = static_cast<array_t<int16_t>*>(buffer)->getStorage();
	break;}
    case ibis::USHORT: {
	str = static_cast<array_t<uint16_t>*>(buffer)->getStorage();
	break;}
    case ibis::INT: {
	str = static_cast<array_t<int32_t>*>(buffer)->getStorage();
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	str = static_cast<array_t<uint32_t>*>(buffer)->getStorage();
	break;}
    case ibis::LONG: {
	str = static_cast<array_t<int64_t>*>(buffer)->getStorage();
	break;}
    case ibis::ULONG: {
	str = static_cast<array_t<uint64_t>*>(buffer)->getStorage();
	break;}
    case ibis::FLOAT: {
	str = static_cast<array_t<float>*>(buffer)->getStorage();
	break;}
    case ibis::DOUBLE: {
	str = static_cast<array_t<double>*>(buffer)->getStorage();
	break;}
    default: {
	break;}
    }

    return str;
} // ibis::bord::column::getRawData

void ibis::bord::column::computeMinMax(const char *,
				       double &min, double &max) const {
    if (buffer == 0) return;

    switch (m_type) {
    case ibis::UBYTE: {
	const array_t<unsigned char> &val =
	    * static_cast<const array_t<unsigned char>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::BYTE: {
	const array_t<signed char> &val =
	    * static_cast<const array_t<signed char>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t> &val = 
	    * static_cast<const array_t<uint16_t>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::SHORT: {
	const array_t<int16_t> &val =
	    * static_cast<const array_t<int16_t>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::UINT: {
	const array_t<uint32_t> &val =
	    * static_cast<const array_t<uint32_t>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::INT: {
	const array_t<int32_t> &val =
	    *static_cast<const array_t<int32_t>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t> &val =
	    * static_cast<const array_t<uint64_t>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::LONG: {
	const array_t<int64_t> &val =
	    *static_cast<const array_t<int64_t>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::FLOAT: {
	const array_t<float> &val =
	    * static_cast<const array_t<float>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    case ibis::DOUBLE: {
	const array_t<double> &val =
	    * static_cast<const array_t<double>*>(buffer);

	ibis::column::actualMinMax(val, mask_, min, max);
	break;}
    default:
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- column[" << (thePart ? thePart->name() : "")
	    << '.' << m_name << "]::computeMinMax -- column type "
	    << TYPESTRING[static_cast<int>(m_type)] << " is not one of the "
	    "supported types (int, uint, float, double)";
	min = 0;
	max = (thePart != 0) ? thePart->nRows() : -DBL_MAX;
    } // switch(m_type)
} // ibis::bord::column::computeMinMax

long ibis::bord::column::evaluateRange(const ibis::qContinuousRange& cmp,
				       const ibis::bitvector& mask,
				       ibis::bitvector& res) const {
    long ierr = -1;
    ibis::bitvector mymask(mask);
    if (mask_.size() > 0)
	mymask &= mask_;
    if (thePart != 0)
	mymask.adjustSize(0, thePart->nRows());
	
    switch (m_type) {
    case ibis::UBYTE: {
	const array_t<unsigned char> &val =
	    * static_cast<const array_t<unsigned char>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::BYTE: {
	const array_t<signed char> &val =
	    * static_cast<const array_t<signed char>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t> &val =
	    * static_cast<const array_t<uint16_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::SHORT: {
	const array_t<int16_t> &val =
	    * static_cast<const array_t<int16_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::UINT: {
	const array_t<uint32_t> &val =
	    * static_cast<const array_t<uint32_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::INT: {
	const array_t<int32_t> &val =
	    * static_cast<const array_t<int32_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t> &val =
	    * static_cast<const array_t<uint64_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::LONG: {
	const array_t<int64_t> &val =
	    * static_cast<const array_t<int64_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::FLOAT: {
	const array_t<float> &val =
	    * static_cast<const array_t<float>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::DOUBLE: {
	const array_t<double> &val =
	    * static_cast<const array_t<double>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    default:
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- column[" << (thePart ? thePart->name() : "?")
	    << '.' << m_name << "]::evaluateRange deos not support column type "
	    << TYPESTRING[static_cast<int>(m_type)]
	    << ", only supports integers and floats";
	ierr = -2;
    } // switch(m_type)
    return ierr;
} // ibis::bord::column::evaluateRange

long ibis::bord::column::evaluateRange(const ibis::qDiscreteRange& cmp,
				       const ibis::bitvector& mask,
				       ibis::bitvector& res) const {
    long ierr = -1;
    ibis::bitvector mymask(mask);
    if (mask_.size() > 0)
	mymask &= mask_;
    if (thePart != 0)
	mymask.adjustSize(0, thePart->nRows());

    switch (m_type) {
    case ibis::UBYTE: {
	const array_t<unsigned char> &val =
	    * static_cast<const array_t<unsigned char>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::BYTE: {
	const array_t<signed char> &val =
	    * static_cast<const array_t<signed char>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t> &val =
	    * static_cast<const array_t<uint16_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::SHORT: {
	const array_t<int16_t> &val =
	    * static_cast<const array_t<int16_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::UINT: {
	const array_t<uint32_t> &val =
	    * static_cast<const array_t<uint32_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::INT: {
	const array_t<int32_t> &val =
	    * static_cast<const array_t<int32_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t> &val =
	    * static_cast<const array_t<uint64_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::LONG: {
	const array_t<int64_t> &val =
	    * static_cast<const array_t<int64_t>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::FLOAT: {
	const array_t<float> &val =
	    * static_cast<const array_t<float>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    case ibis::DOUBLE: {
	const array_t<double> &val =
	    * static_cast<const array_t<double>*>(buffer);
	ierr = ibis::part::doScan(val, cmp, mymask, res);
	break;}
    default:
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- column[" << (thePart ? thePart->name() : "?")
	    << '.' << m_name << "]::evaluateRange deos not support column type "
	    << TYPESTRING[static_cast<int>(m_type)]
	    << ", only supports integers and floats";
	ierr = -2;
    } // switch(m_type)
    return ierr;
} // ibis::bord::column::evaluateRange

/// Locate the strings that match the given string.  The comaprison is case
/// sensitive.  If the incoming strign is a nil pointer, it matches nothing.
long ibis::bord::column::stringSearch(const char* str,
				      ibis::bitvector& hits) const {
    std::string evt = "column[";
    evt += (thePart ? thePart->name() : "");
    evt += '.';
    evt += m_name;
    evt += "]::stringSearch(";
    evt += (str ? str : "");
    evt += ')';
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " can not proceed with a nil buffer";
	return -2;
    }

    const array_t<uint32_t>& vals(*static_cast<const array_t<uint32_t>*>(buffer));
    
    if (str == 0) { // null string can not match any thing
	hits.set(0, thePart ? thePart->nRows() : vals.size());
	return 0;
    }

    uint32_t stri = (*dic)[str];

    ibis::util::timer mytimer(evt.c_str(), 3);
    hits.clear();
    for (size_t j = 0; j < vals.size(); ++ j) {
	if (vals[j] == stri)
	    hits.setBit(j, 1);
    }
    hits.adjustSize(0, thePart ? thePart->nRows() : vals.size());
    return hits.cnt();
} // ibis::bord::column::stringSearch

long ibis::bord::column::stringSearch(const std::vector<std::string>& str,
				      ibis::bitvector& hits) const {
    std::string evt = "column[";
    evt += (thePart ? thePart->name() : "");
    evt += '.';
    evt += m_name;
    evt += "]::stringSearch(<...>)";
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " can not proceed with a nil buffer";
	return -2;
    }
    const array_t<uint32_t>& vals(*static_cast<const array_t<uint32_t>*>(buffer));
    if (str.empty()) { // null string can not match any thing
	hits.set(0, thePart ? thePart->nRows() : vals.size());
	return 0;
    }


    ibis::util::timer mytimer(evt.c_str(), 3);

    array_t<uint32_t> stri;
    stri.resize(str.size());
    for (size_t j = 0; j < str.size(); ++ j) {
	stri[j] = (*dic)[str[j].c_str()];
    }
		
		 


    hits.clear();
    for (size_t j = 0; j < vals.size(); ++ j) {
	bool hit = false;
	for (size_t i = 0; i < str.size() && hit == false; ++ i) {
	    hit = (stri[i] == vals[j]);
	}
	if (hit) {
	    hits.setBit(j, 1);
	}
    }
    hits.adjustSize(0, thePart ? thePart->nRows() : vals.size());
    return hits.cnt();
} // ibis::bord::column::stringSearch

/// Compute an estimate of the maximum number of possible matches.  This is
/// a trivial implementation that does not actually perform any meaningful
/// checks.  It simply returns the number of strings in memory as the
/// estimate.
long ibis::bord::column::stringSearch(const char* str) const {
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::stringSearch is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::stringSearch can not proceed with a nil buffer";
	return -2;
    }
    if (str == 0) return 0;

    const array_t<uint32_t>&vals(*static_cast<const array_t<uint32_t>*>(buffer));
    return vals.size();
} // ibis::bord::column::stringSearch

/// Compute an estimate of the maximum number of possible matches.  This is
/// a trivial implementation that does not actually perform any meaningful
/// checks.  It simply returns the number of strings in memory as the
/// estimate.
long
ibis::bord::column::stringSearch(const std::vector<std::string>& str) const {
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::stringSearch is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::stringSearch can not proceed with a nil buffer";
	return -2;
    }
    if (str.empty()) return 0;

    const array_t<uint32_t>&vals(*static_cast<const array_t<uint32_t>*>(buffer));
    return vals.size();
} // ibis::bord::column::stringSearch

/// Find the given keyword and return the rows.
long ibis::bord::column::keywordSearch(const char* key,
				       ibis::bitvector& hits) const {
    std::string evt = "bord::column[";
    evt += (thePart ? thePart->name() : "");
    evt += '.';
    evt += m_name;
    evt += "]::keywordSearch";
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt
	    << "]::keywordSearch can not proceed with a nil buffer";
	return -2;
    }
    hits.clear();
    if (key == 0 || *key == 0) return 0;

    const std::vector<std::string>&
	vals(*static_cast<const std::vector<std::string>*>(buffer));
    ibis::fileManager::buffer<char> buf(1024);
    ibis::keywords::tokenizer tknz;
    std::vector<const char*> ks;
    ibis::util::timer mytimer(evt.c_str(), 3);
    for (unsigned j = 0; j < vals.size(); ++ j) {
	if (vals[j].empty()) continue;

	if (buf.size() < vals[j].size()) {
	    if (buf.resize(vals[j].size()+buf.size()) < vals[j].size()) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " failed to allocate space "
		    "for storing string value in row " << j;
		hits.clear();
		return -3;
	    }
	}
	(void) memcpy(buf.address(), vals[j].c_str(), vals[j].size());
	(void) tknz(ks, buf.address());
	LOGGER(ks.empty() && ibis::gVerbose > 2)
	    << evt << " could not extract any token from string \""
	    << vals[j] << "\"";

	bool hit = false;
	for (unsigned i = 0; i < ks.size() && !hit; ++ i)
	    hit = (0 == strcmp(key, ks[i]));
	if (hit)
	    hits.setBit(j, 1);
    }
    hits.adjustSize(0, thePart ? thePart->nRows() : vals.size());
    return hits.cnt();
} // ibis::bord::column::keywordSearch

/// Return an upper bound on the number of matches.
long ibis::bord::column::keywordSearch(const char* str) const {
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::keywordSearch is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::keywordSearch can not proceed with a nil buffer";
	return -2;
    }
    if (str == 0) return 0;

    const std::vector<std::string>&
	vals(*static_cast<const std::vector<std::string>*>(buffer));
    return vals.size();
} // ibis::bord::column::keywordSearch

/// Compute an estimate of the maximum number of possible matches.  This is
/// a trivial implementation that does not actually perform any meaningful
/// checks.  It simply returns the number of strings in memory as the
/// estimate.
long ibis::bord::column::patternSearch(const char* pat) const {
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::patternSearch is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- column[" << (thePart ? thePart->name() : "") << '.'
	    << m_name << "]::patternSearch can not proceed with a nil buffer";
	return -2;
    }
    if (pat == 0) return 0;

    const array_t<uint32_t>&vals(*static_cast<const array_t<uint32_t>*>(buffer));
    return vals.size();
} // ibis::bord::column::patternSearch

long ibis::bord::column::patternSearch(const char* pat,
				       ibis::bitvector &hits) const {
    std::string evt = "column[";
    evt += (thePart ? thePart->name() : "");
    evt += '.';
    evt += m_name;
    evt += "]::patternSearch(";
    evt += (pat ? pat : "");
    evt += ')';
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " is not supported on column type "
	    << ibis::TYPESTRING[(int)m_type];
	return -1;
    }
    if (buffer == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " can not proceed with a nil buffer";
	return -2;
    }

    const array_t<uint32_t>&vals(*static_cast<const array_t<uint32_t>*>(buffer));

    if (pat == 0) { // null string can not match any thing
	hits.set(0, thePart ? thePart->nRows() : vals.size());
	return 0;
    }

    ibis::util::timer mytimer(evt.c_str(), 3);
    hits.clear();

    array_t<uint32_t> stri;
    for (size_t j = 0; j < (*dic).size(); ++ j) {
	if (ibis::util::strMatch((*dic)[j], pat)) stri.push_back(j);
    }
	
    hits.clear();
    for (size_t j = 0; j < vals.size(); ++ j) {
	bool hit = false;
	for (size_t i = 0; i < stri.size() && hit == false; ++ i) {
	    hit = (stri[i] == vals[j]);
	}
	if (hit) {
	    hits.setBit(j, 1);
	}
    }

    hits.adjustSize(0, thePart ? thePart->nRows() : vals.size());
    return hits.cnt();
} // ibis::bord::column::patternSearch

ibis::array_t<signed char>*
ibis::bord::column::selectBytes(const ibis::bitvector &mask) const {
    ibis::array_t<signed char>* array = new array_t<signed char>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::BYTE) {
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);
	const uint32_t nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<signed char> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) { // no need to check loop bounds
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
	    logWarning("selectBytes", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectBytes", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectBytes", "retrieving %lu integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectBytes

ibis::array_t<unsigned char>*
ibis::bord::column::selectUBytes(const ibis::bitvector& mask) const {
    array_t<unsigned char>* array = new array_t<unsigned char>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    if (m_type == UBYTE) {
	const array_t<unsigned char> &prop =
	    * static_cast<const array_t<unsigned char>*>(buffer);
	const uint32_t nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<unsigned char> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) {
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
	    logWarning("selectUBytes", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectUBytes", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectUBytes", "retrieving %lu unsigned integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectUBytes

ibis::array_t<int16_t>*
ibis::bord::column::selectShorts(const ibis::bitvector &mask) const {
    ibis::array_t<int16_t>* array = new array_t<int16_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::SHORT) {
	const array_t<int16_t> &prop =
	    * static_cast<const array_t<int16_t>*>(buffer);
	const uint32_t nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<int16_t> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
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
	    logWarning("selectShorts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == ibis::BYTE) {
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);
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
	    logWarning("selectShorts", "expects to retrieve %lu elements "
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
	    logWarning("selectShorts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectShorts", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectShorts", "retrieving %lu integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectShorts

ibis::array_t<uint16_t>*
ibis::bord::column::selectUShorts(const ibis::bitvector& mask) const {
    array_t<uint16_t>* array = new array_t<uint16_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    if (m_type == USHORT) {
	const array_t<uint16_t> &prop =
	    * static_cast<const array_t<uint16_t>*>(buffer);
	const uint32_t nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<uint16_t> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) {
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
	    logWarning("selectUShorts", "expects to retrieve %lu elements "
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
	    logWarning("selectUShorts", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectUShorts", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectUShorts", "retrieving %lu unsigned integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectUShorts

ibis::array_t<int32_t>*
ibis::bord::column::selectInts(const ibis::bitvector &mask) const {
    ibis::array_t<int32_t>* array = new array_t<int32_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::INT) {
	const array_t<int32_t> &prop =
	    * static_cast<const array_t<int32_t>*>(buffer);
	const long unsigned nprop = prop.size();
	uint32_t i = 0;
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectInts mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<int32_t> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);
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
ibis::array_t<uint32_t>*
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
	const uint32_t nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<uint32_t> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) {
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
ibis::array_t<int64_t>*
ibis::bord::column::selectLongs(const ibis::bitvector& mask) const {
    ibis::array_t<int64_t>* array = new array_t<int64_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::LONG) {
	const array_t<int64_t> &prop =
	    * static_cast<const array_t<int64_t>*>(buffer);
	const long unsigned nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectLongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= mask.size()) { // use shallow copy
	    ibis::array_t<int64_t> tmp(prop);
	    tmp.swap(*array);
	    i = nprop;
	}
	else if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectLongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectLongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);

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

ibis::array_t<uint64_t>*
ibis::bord::column::selectULongs(const ibis::bitvector& mask) const {
    ibis::array_t<uint64_t>* array = new array_t<uint64_t>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;

    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();
    if (m_type == ibis::ULONG) {
	const array_t<uint64_t> &prop =
	    * static_cast<const array_t<uint64_t>*>(buffer);
	const long unsigned nprop = prop.size();
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectULongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<uint64_t> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectULongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	logMessage("DEBUG", "selectULongs mask.size(%lu) and nprop=%lu",
		   static_cast<long unsigned>(mask.size()), nprop);
#endif
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) { // no need to check loop bounds
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering unchecked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    logMessage("DEBUG", "entering checked loops");
#endif
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (*idx0 >= nprop) break;
		if (index.isRange()) {
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else if (m_type == BYTE) {
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);

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
	    logWarning("selectULongs", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
    }
    else {
	logWarning("selectULongs", "incompatible data type");
    }
    if (ibis::gVerbose > 5) {
	timer.stop();
	long unsigned cnt = mask.cnt();
	logMessage("selectULongs", "retrieving %lu integer%s "
		   "took %g sec(CPU), %g sec(elapsed)",
		   static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return array;
} // ibis::bord::column::selectULongs

/// Put selected values of a float column into an array.
ibis::array_t<float>*
ibis::bord::column::selectFloats(const ibis::bitvector& mask) const {
    ibis::array_t<float>* array = new array_t<float>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    if (m_type == FLOAT) {
	const array_t<float> &prop =
	    * static_cast<const array_t<float>*>(buffer);
	const uint32_t nprop = prop.size();

	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<float> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) { // no need to check loop bounds
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
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);

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
/// @note Any column type could be selected as doubles.  Other selectXXs
/// function only work on the same data type.  This is the only function
/// that allows one to convert to a different type.  This is mainly to 
ibis::array_t<double>*
ibis::bord::column::selectDoubles(const ibis::bitvector& mask) const {
    ibis::array_t<double>* array = new array_t<double>;
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
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);

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
	const uint32_t nprop = prop.size();
	uint32_t i = 0;
	if (tot < nprop)
	    array->resize(tot);
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (tot >= nprop) {
	    ibis::array_t<double> tmp(prop);
	    array->swap(tmp);
	    i = nprop;
	}
	else if (nprop >= mask.size()) {
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

/// Output the selected values as strings.  Most data types can be
/// converted and shown as strings.
std::vector<std::string>*
ibis::bord::column::selectStrings(const ibis::bitvector& mask) const {
    std::vector<std::string>* array = new std::vector<std::string>;
    const uint32_t tot = mask.cnt();
    if (tot == 0)
	return array;
    ibis::horometer timer;
    if (ibis::gVerbose > 5)
	timer.start();

    switch(m_type) {
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
			std::ostringstream oss;
			oss << prop[j];
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu unsigned integer%s "
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu integer%s "
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu unsigned short "
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu short integer%s "
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
			std::ostringstream oss;
			oss << (unsigned) (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (unsigned)(prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (unsigned) (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (unsigned)(prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu unsigned 1-byte "
		       "integer%s took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::BYTE: {
	const array_t<signed char> &prop =
	    * static_cast<const array_t<signed char>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			std::ostringstream oss;
			oss << (int) (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (int) (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (int) (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (int) (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu 1-byte integer%s "
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu float value%s "
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu double value%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::OID: {
	const array_t<ibis::rid_t> &prop =
	    * static_cast<const array_t<ibis::rid_t>*>(buffer);

	uint32_t i = 0;
	array->resize(tot);
	const uint32_t nprop = prop.size();
	ibis::bitvector::indexSet index = mask.firstIndexSet();
	if (nprop >= mask.size()) {
	    while (index.nIndices() > 0) {
		const ibis::bitvector::word_t *idx0 = index.indices();
		if (index.isRange()) {
		    for (uint32_t j = *idx0; j<idx0[1]; ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			std::ostringstream oss;
			oss << (prop[idx0[j]]);
			(*array)[i] = oss.str();
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
			std::ostringstream oss;
			oss << (prop[j]);
			(*array)[i] = oss.str();
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    std::ostringstream oss;
			    oss << (prop[idx0[j]]);
			    (*array)[i] = oss.str();
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu ibis::rid_t value%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::CATEGORY: {
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
			(*array)[i] = (*dic)[(prop[j])];
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			(*array)[i] = (*dic)[ (prop[idx0[j]])];
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
			(*array)[i] = (*dic)[(prop[j])];
		    }
		}
		else {
		    for (uint32_t j = 0; j<index.nIndices(); ++j, ++i) {
			if (idx0[j] < nprop) {
			    (*array)[i] = (*dic)[(prop[idx0[j]])];
			}
			else
			    break;
		    }
		}
		++ index;
	    }
	}

	if (i != tot) {
	    array->resize(i);
	    logWarning("selectStrings", "expects to retrieve %lu elements "
		       "but only got %lu", static_cast<long unsigned>(tot),
		       static_cast<long unsigned>(i));
	}
	else if (ibis::gVerbose > 5) {
	    timer.stop();
	    long unsigned cnt = mask.cnt();
	    logMessage("selectStrings", "retrieving %lu string value%s "
		       "took %g sec(CPU), %g sec(elapsed)",
		       static_cast<long unsigned>(cnt), (cnt > 1 ? "s" : ""),
		       timer.CPUTime(), timer.realTime());
	}
	break;}
    case ibis::TEXT:
	{
	    const std::vector<std::string> &prop =
		* static_cast<const std::vector<std::string>*>(buffer);

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
			    if (idx0[j] < nprop) {
				(*array)[i] = (prop[idx0[j]]);
			    }
			    else
				break;
			}
		    }
		    ++ index;
		}
	    }

	    if (i != tot) {
		array->resize(i);
		logWarning("selectStrings", "expects to retrieve %lu elements "
			   "but only got %lu", static_cast<long unsigned>(tot),
			   static_cast<long unsigned>(i));
	    }
	    else if (ibis::gVerbose > 5) {
		timer.stop();
		long unsigned cnt = mask.cnt();
		logMessage("selectStrings", "retrieving %lu string value%s "
			   "took %g sec(CPU), %g sec(elapsed)",
			   static_cast<long unsigned>(cnt),
			   (cnt > 1 ? "s" : ""),
			   timer.CPUTime(), timer.realTime());
	    }
	    break;}
    default: {
	logWarning("selectStrings", "incompatible data type");
	break;}
    }
    return array;
} // ibis::bord::column::selectStrings

/// Convert the integer value i into the corresponding string value.
void ibis::bord::column::getString(uint32_t i, std::string &val) const {
    val.erase();
    if (m_type == ibis::TEXT ) {
	std::vector<std::string> *str_column = 
	    static_cast<std::vector<std::string> *>(buffer);
	if ( i < str_column->size())
	    val = str_column->at(i);
    }
    else if (m_type == ibis::CATEGORY) {
	if (i > 0 && i <= (*dic).size())
	    val = (*dic)[i];
    }
    else if (m_type == ibis::UINT && dic != 0) {
	if (i > 0 && i <= dic->size())
	    val = (*dic)[i];
    }
} // ibis::bord::column::getString

/// Makes a copy of the in-memory data.  Uses a shallow copy for
/// ibis::array_t objects, but a deap copy for the string values.
int ibis::bord::column::getValuesArray(void* vals) const {
    if (vals == 0 || buffer == 0) return -1;
    switch (m_type) {
    default: {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- bord[" << (thePart?thePart->name():"")
	    << "]::column[" << m_name << "]::getValuesArray does not yet "
	    "support column type " << ibis::TYPESTRING[(int)m_type];
	break;}
    case ibis::BYTE: {
	static_cast<array_t<signed char>*>(vals)->
	    copy(* static_cast<const array_t<signed char>*>(buffer));
	break;}
    case ibis::UBYTE: {
	static_cast<array_t<unsigned char>*>(vals)->
	    copy(* static_cast<const array_t<unsigned char>*>(buffer));
	break;}
    case ibis::SHORT: {
	static_cast<array_t<int16_t>*>(vals)->
	    copy(* static_cast<const array_t<int16_t>*>(buffer));
	break;}
    case ibis::USHORT: {
	static_cast<array_t<uint16_t>*>(vals)->
	    copy(* static_cast<const array_t<uint16_t>*>(buffer));
	break;}
    case ibis::INT: {
	static_cast<array_t<int32_t>*>(vals)->
	    copy(* static_cast<const array_t<int32_t>*>(buffer));
	break;}
    case ibis::UINT:
    case ibis::CATEGORY:{
	static_cast<array_t<uint32_t>*>(vals)->
	    copy(* static_cast<const array_t<uint32_t>*>(buffer));
	break;}
    case ibis::LONG: {
	static_cast<array_t<int64_t>*>(vals)->
	    copy(* static_cast<const array_t<int64_t>*>(buffer));
	break;}
    case ibis::ULONG: {
	static_cast<array_t<uint64_t>*>(vals)->
	    copy(* static_cast<const array_t<uint64_t>*>(buffer));
	break;}
    case ibis::FLOAT: {
	static_cast<array_t<float>*>(vals)->
	    copy(* static_cast<const array_t<float>*>(buffer));
	break;}
    case ibis::DOUBLE: {
	static_cast<array_t<double>*>(vals)->
	    copy(* static_cast<const array_t<double>*>(buffer));
	break;}
    case ibis::TEXT:
	{
	    std::vector<std::string>
		tmp(* static_cast<const std::vector<std::string>*>(buffer));
	    static_cast<std::vector<std::string>*>(vals)->swap(tmp);
	    break;}
    }
    return 0;
} // ibis::bord::column::getValuesArray

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
	array_t<signed char> &prop =
	    * static_cast<array_t<signed char>*>(buffer);
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
    case ibis::TEXT: {
	std::vector<std::string> &prop =
	    * static_cast<std::vector<std::string>*>(buffer);
	std::reverse(prop.begin(), prop.end());
	break;}
    default: {
	logWarning("reverseRows", "incompatible data type");
	break;}
    }
} // ibis::bord::column::reverseRows

int ibis::bord::column::limit(uint32_t nr) {
    int ierr = 0;
    switch(m_type) {
    case ibis::ULONG: {
	array_t<uint64_t> &prop =
	    * static_cast<array_t<uint64_t>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::LONG: {
	array_t<int64_t> &prop =
	    * static_cast<array_t<int64_t>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	array_t<uint32_t> &prop =
	    * static_cast<array_t<uint32_t>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::INT: {
	array_t<int32_t> &prop =
	    * static_cast<array_t<int32_t>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> &prop =
	    * static_cast<array_t<uint16_t>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::SHORT: {
	array_t<int16_t> &prop =
	    * static_cast<array_t<int16_t>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> &prop =
	    * static_cast<array_t<unsigned char>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::BYTE: {
	array_t<signed char> &prop =
	    * static_cast<array_t<signed char>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::FLOAT: {
	array_t<float> &prop =
	    * static_cast<array_t<float>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::DOUBLE: {
	array_t<double> &prop =
	    * static_cast<array_t<double>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    case ibis::TEXT: {
	std::vector<std::string> &prop =
	    * static_cast<std::vector<std::string>*>(buffer);
	if (nr < prop.size())
	    prop.resize(nr);
	break;}
    default: {
	logWarning("reverseRows", "incompatible data type");
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::column::limit

/// Convert the integer representation back to the string representation.
/// The existing data type must be ibis::UINT and the column with the same
/// in in the given ibis::part prt must be of type ibis::CATEGORY.
int ibis::bord::column::restoreCategoriesAsStrings(const ibis::category& cat) {
    if (m_type != ibis::UINT) // must be uint32_t
	return -2;

    ibis::array_t<uint32_t> *arrint =
	static_cast<ibis::array_t<uint32_t>*>(buffer);
    const int nr = (thePart->nRows() <= arrint->size() ?
		    thePart->nRows() : arrint->size());
    std::vector<std::string> *arrstr = new std::vector<std::string>(nr);
    if (dic != 0) {
	for (int j = 0; j < nr; ++ j)
	    (*arrstr)[j] = (*dic)[(*arrint)[j]];
    }
    else {
	for (int j = 0; j < nr; ++ j)
	    cat.getString((*arrint)[j], (*arrstr)[j]);
    }
    delete arrint; // free the storage for the integers.
    m_type = ibis::CATEGORY;
    buffer = arrstr;
    return nr;
} // ibis::bord::column::restoreCategoriesAsStrings

long ibis::bord::column::append(const char* dt, const char* df,
				const uint32_t nold, const uint32_t nnew,
				uint32_t nbuf, char* buf) {
    return ibis::column::append(dt, df, nold, nnew, nbuf, buf);
} // ibis::bord::column::append

/// Append user supplied data to the current column.  The incoming values
/// is carried by a void* which is cast to the same type as the buffer used
/// by the column.  The mask is used to indicate which values in the
/// incoming array are valid.
long ibis::bord::column::append(const void* vals, const ibis::bitvector& msk) {
    if (vals == 0 || msk.size() == 0 || msk.cnt() == 0) return 0;
    int ierr = 0;
    switch (m_type) {
    case ibis::BYTE: {
	ibis::bord::column::addIncoreData<signed char>
	    (reinterpret_cast<array_t<signed char>*&>(buffer),
	     thePart->nRows(),
	     *static_cast<const array_t<signed char>*>(vals),
	     static_cast<signed char>(0x7F));
	break;}
    case ibis::UBYTE: {
	ibis::bord::column::addIncoreData<unsigned char>
	    (reinterpret_cast<array_t<unsigned char>*&>(buffer),
	     thePart->nRows(),
	     *static_cast<const array_t<unsigned char>*>(vals),
	     static_cast<unsigned char>(0xFF));
	break;}
    case ibis::SHORT: {
	addIncoreData(reinterpret_cast<array_t<int16_t>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<int16_t>*>(vals),
		      static_cast<int16_t>(0x7FFF));
	break;}
    case ibis::USHORT: {
	addIncoreData(reinterpret_cast<array_t<uint16_t>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<uint16_t>*>(vals),
		      static_cast<uint16_t>(0xFFFF));
	break;}
    case ibis::INT: {
	addIncoreData(reinterpret_cast<array_t<int32_t>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<int32_t>*>(vals),
		      static_cast<int32_t>(0x7FFFFFFF));
	break;}
    case ibis::UINT: {
	addIncoreData(reinterpret_cast<array_t<uint32_t>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<uint32_t>*>(vals),
		      static_cast<uint32_t>(0xFFFFFFFF));
	break;}
    case ibis::LONG: {
	addIncoreData(reinterpret_cast<array_t<int64_t>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<int64_t>*>(vals),
		      static_cast<int64_t>(0x7FFFFFFFFFFFFFFFLL));
	break;}
    case ibis::ULONG: {
	addIncoreData(reinterpret_cast<array_t<uint64_t>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<uint64_t>*>(vals),
		      static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFLL));
	break;}
    case ibis::FLOAT: {
	addIncoreData(reinterpret_cast<array_t<float>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<float>*>(vals),
		      FASTBIT_FLOAT_NULL);
	break;}
    case ibis::DOUBLE: {
	addIncoreData(reinterpret_cast<array_t<double>*&>(buffer),
		      thePart->nRows(),
		      *static_cast<const array_t<double>*>(vals),
		      FASTBIT_DOUBLE_NULL);
	break;}
    // case ibis::CATEGORY: {
    // 	const std::vector<std::string>*stv = static_cast<const std::vector<std::string>*>(vals);
    // 	array_t<uint32_t> * ibuffer = new array_t<uint32_t>();
		 
    // 	ibuffer->resize(stv->size());
    // 	for (size_t i = 0 ; i < stv->size() ; i++) (*ibuffer)[i]=dic->insert(((*stv)[i]).c_str());
    // 	addIncoreData(reinterpret_cast<array_t<uint32_t>*&>(buffer),
    // 		      thePart->nRows(),
    // 		      *static_cast<const array_t<uint32_t>*>(ibuffer),
    // 		      static_cast<uint32_t>(0));
    // 	delete ibuffer;
    // 	break;
    // }
    case ibis::CATEGORY:
    case ibis::TEXT: {
	addStrings(reinterpret_cast<std::vector<std::string>*&>(buffer),
		   thePart->nRows(),
		   *static_cast<const std::vector<std::string>*>(vals));
	break;}
    default: {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- column[" << thePart->name() << '.' << m_name
	    << "]::append -- unable to process column " << m_name
	    << " (type " << ibis::TYPESTRING[(int)m_type] << ")";
	ierr = -17;
	break;}
    }

    if (ierr == 0) {
	mask_.adjustSize(0, thePart->nRows());
	mask_ += msk;
    }
    return ierr;
} // ibis::bord::column::append

/// Append selected values from the given column to the current column.
/// This function extracts the values using the given mask from scol, and
/// then append the values to the current column.  The type of scol must be
/// ligitimately converted to the type of this column.  It returns the
/// number of values added to the column on success, or a negative number
/// to indicate errors.
long ibis::bord::column::append(const ibis::column& scol,
				const ibis::bitvector& msk) {
    if (msk.size() == 0 || msk.cnt() == 0) return 0;
    int ierr = 0;
    switch (m_type) {
    case ibis::BYTE: {
	std::auto_ptr< array_t<signed char> > vals(scol.selectBytes(msk));
	if (vals.get() != 0)
	    ierr = ibis::bord::column::addIncoreData<signed char>
		(reinterpret_cast<array_t<signed char>*&>(buffer),
		 thePart->nRows(), *vals,
		 static_cast<signed char>(0x7F));
	else
	    ierr = -18;
	break;}
    case ibis::UBYTE: {
	std::auto_ptr< array_t<unsigned char> > vals(scol.selectUBytes(msk));
	if (vals.get() != 0)
	    ierr = ibis::bord::column::addIncoreData<unsigned char>
		(reinterpret_cast<array_t<unsigned char>*&>(buffer),
		 thePart->nRows(), *vals,
		 static_cast<unsigned char>(0xFF));
        else
	    ierr = -18;
	break;}
    case ibis::SHORT: {
	std::auto_ptr< array_t<int16_t> > vals(scol.selectShorts(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<int16_t>*&>(buffer),
				 thePart->nRows(), *vals,
				 static_cast<int16_t>(0x7FFF));
	else
	    ierr = -18;
	break;}
    case ibis::USHORT: {
	std::auto_ptr< array_t<uint16_t> > vals(scol.selectUShorts(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<uint16_t>*&>(buffer),
				 thePart->nRows(), *vals,
				 static_cast<uint16_t>(0xFFFF));
        else
	    ierr = -18;
	break;}
    case ibis::INT: {
	std::auto_ptr< array_t<int32_t> > vals(scol.selectInts(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<int32_t>*&>(buffer),
				 thePart->nRows(), *vals,
				 static_cast<int32_t>(0x7FFFFFFF));
	else
	    ierr = -18;
	break;}
    case ibis::UINT: {
	std::auto_ptr< array_t<uint32_t> > vals(scol.selectUInts(msk));
	if (dic == 0) {
	    const ibis::bord::column *bc =
		dynamic_cast<const ibis::bord::column*>(&scol);
	    if (bc != 0)
		dic = bc->dic;
	}
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<uint32_t>*&>(buffer),
				 thePart->nRows(), *vals,
				 static_cast<uint32_t>(0xFFFFFFFF));
        else
	    ierr = -18;
	break;}
    case ibis::LONG: {
	std::auto_ptr< array_t<int64_t> > vals(scol.selectLongs(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<int64_t>*&>(buffer),
				 thePart->nRows(), *vals,
				 static_cast<int64_t>(0x7FFFFFFFFFFFFFFFLL));
        else
	    ierr = -18;
	break;}
    case ibis::ULONG: {
	std::auto_ptr< array_t<uint64_t> > vals(scol.selectULongs(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<uint64_t>*&>(buffer),
				 thePart->nRows(), *vals,
				 static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFLL));
	else
	    ierr = -18;
	break;}
    case ibis::FLOAT: {
	std::auto_ptr< array_t<float> > vals(scol.selectFloats(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<float>*&>(buffer),
				 thePart->nRows(), *vals,
				 FASTBIT_FLOAT_NULL);
	else
	    ierr = -18;
	break;}
    case ibis::DOUBLE: {
	std::auto_ptr< array_t<double> > vals(scol.selectDoubles(msk));
	if (vals.get() != 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<double>*&>(buffer),
				 thePart->nRows(), *vals,
				 FASTBIT_DOUBLE_NULL);
	else
	    ierr = -18;
	break;}
    // case ibis::CATEGORY: {
    // 	std::vector<std::string>*vals = scol.selectStrings(msk);
    // 	if (vals != 0){
    // 	    array_t<uint32_t> * ibuffer = new array_t<uint32_t>();
		 
    // 	    ibuffer->resize(vals->size());
    // 	    for (size_t i = 0 ; i < vals->size() ; i++) (*ibuffer)[i]=dic->insert(((*vals)[i]).c_str());
    // 	    ierr = addIncoreData(reinterpret_cast<array_t<uint32_t>*&>(buffer),
    // 				 thePart->nRows(),
    // 				 *static_cast<const array_t<uint32_t>*>(ibuffer),
    // 				 static_cast<uint32_t>(0));
    // 	    delete ibuffer;
    // 	    delete vals;
    // 	} else {
    // 	    ierr = -18;
    // 	}
    // 	break;}
    case ibis::CATEGORY:
    case ibis::TEXT: {
	std::auto_ptr< std::vector<std::string> > vals(scol.selectStrings(msk));
	if (vals.get() != 0)
	    ierr = addStrings
		(reinterpret_cast<std::vector<std::string>*&>(buffer),
		 thePart->nRows(), *vals);
	else
	    ierr = -18;
	break;}
    default: {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- column[" << thePart->name() << '.' << m_name
	    << "]::append -- unable to process column " << m_name
	    << " (type " << ibis::TYPESTRING[(int)m_type] << ")";
	ierr = -17;
	break;}
    }

    if (ierr > 0) {
	const ibis::bitvector::word_t sz = thePart->nRows() + ierr;
	mask_.adjustSize(sz, sz);
    }
    return ierr;
} // ibis::bord::column::append

/// Append selected values from the given column to the current column.
/// This function extracts the values using the given range condition on
/// scol, and then append the values to the current column.  The type of
/// scol must be ligitimately converted to the type of this column.  It
/// returns 0 to indicate success, a negative number to indicate error.
long ibis::bord::column::append(const ibis::column& scol,
				const ibis::qContinuousRange& cnd) {
    int ierr = 0;
    switch (m_type) {
    case ibis::BYTE: {
	array_t<signed char> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = ibis::bord::column::addIncoreData<signed char>
		(reinterpret_cast<array_t<signed char>*&>(buffer),
		 thePart->nRows(), vals,
		 static_cast<signed char>(0x7F));
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = ibis::bord::column::addIncoreData<unsigned char>
		(reinterpret_cast<array_t<unsigned char>*&>(buffer),
		 thePart->nRows(), vals,
		 static_cast<unsigned char>(0xFF));
	break;}
    case ibis::SHORT: {
	array_t<int16_t> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<int16_t>*&>(buffer),
				 thePart->nRows(), vals,
				 static_cast<int16_t>(0x7FFF));
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<uint16_t>*&>(buffer),
				 thePart->nRows(), vals,
				 static_cast<uint16_t>(0xFFFF));
	break;}
    case ibis::INT: {
	array_t<int32_t> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<int32_t>*&>(buffer),
				 thePart->nRows(), vals,
				 static_cast<int32_t>(0x7FFFFFFF));
	break;}
    case ibis::UINT: {
	array_t<uint32_t> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<uint32_t>*&>(buffer),
				 thePart->nRows(), vals,
				 static_cast<uint32_t>(0xFFFFFFFF));
	break;}
    case ibis::LONG: {
	array_t<int64_t> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<int64_t>*&>(buffer),
				 thePart->nRows(), vals,
				 static_cast<int64_t>(0x7FFFFFFFFFFFFFFFLL));
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<uint64_t>*&>(buffer),
				 thePart->nRows(), vals,
				 static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFLL));
	break;}
    case ibis::FLOAT: {
	array_t<float> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<float>*&>(buffer),
				 thePart->nRows(), vals, FASTBIT_FLOAT_NULL);
	break;}
    case ibis::DOUBLE: {
	array_t<double> vals;
	ierr = scol.selectValues(cnd, &vals);
	if (ierr > 0)
	    ierr = addIncoreData(reinterpret_cast<array_t<double>*&>(buffer),
				 thePart->nRows(), vals, FASTBIT_DOUBLE_NULL);
	else
	    ierr = -18;
	break;}
    default: {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- column[" << thePart->name() << '.' << m_name
	    << "]::append -- unable to process column " << m_name
	    << " (type " << ibis::TYPESTRING[(int)m_type] << ")";
	ierr = -17;
	break;}
    }
    if (ierr > 0) {
	const ibis::bitvector::word_t sz = thePart->nRows() + ierr;
	mask_.adjustSize(sz, sz);
    }
    return ierr;
} // ibis::bord::column::append

/// Extend the buffer to have nr elements.  All new elements have the value
/// 1U.
void ibis::bord::column::addCounts(uint32_t nr) {
    if (*m_name.c_str() != '*' || m_type != ibis::UINT) return;
    if (buffer == 0) {
	buffer = new ibis::array_t<uint32_t>(nr, 1U);
    }
    else {
	ibis::array_t<uint32_t> *ubuf =
	    static_cast<ibis::array_t<uint32_t>*>(buffer);
	if (nr > ubuf->size())
	    ubuf->insert(ubuf->end(), nr - ubuf->size(), 1U);
    }
} // ibis::bord::column::addCounts

template <typename T> int
ibis::bord::column::addIncoreData(array_t<T>*& to, uint32_t nold,
				  const array_t<T>& from, const T special) {
    const int nqq = from.size();

    if (to == 0)
	to = new array_t<T>();
    if (nqq > 0) {
	if (nold > 0) {
	    to->reserve(nold+nqq);
	    if ((size_t)nold > to->size())
		to->insert(to->end(), nold - to->size(), special);
	    to->insert(to->end(), from.begin(), from.end());
	}
	else {
	    to->copy(from);
	}
    }
    return nqq;
} // ibis::bord::column::addIncoreData

int ibis::bord::column::addStrings(std::vector<std::string>*& to,
				   uint32_t nold,
				   const std::vector<std::string>& from) {
    const int nqq = from.size();
    if (to == 0)
	to = new std::vector<std::string>();
    std::vector<std::string>& target = *to;
    target.reserve(nold+nqq);
    if (nold > (long)target.size()) {
	const std::string dummy;
	target.insert(target.end(), nold-target.size(), dummy);
    }
    if (nqq > 0)
	target.insert(target.end(), from.begin(), from.end());
    return nqq;
} // ibis::bord::column::addStrings

/// Does this column have the same values as the other.
bool ibis::bord::column::equal_to(const ibis::bord::column &other) const {
    if (m_type != other.m_type) return false;
    if (buffer == 0 || other.buffer == 0) return false;
    if (buffer == other.buffer) return true;

    switch (m_type) {
    default:
	return false;
    case ibis::BYTE: {
	const ibis::array_t<signed char> &v0 =
	    *static_cast<const ibis::array_t<signed char>*>(buffer);
	const ibis::array_t<signed char> &v1 =
	    *static_cast<const ibis::array_t<signed char>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::UBYTE: {
	const ibis::array_t<unsigned char> &v0 =
	    *static_cast<const ibis::array_t<unsigned char>*>(buffer);
	const ibis::array_t<unsigned char> &v1 =
	    *static_cast<const ibis::array_t<unsigned char>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::SHORT: {
	const ibis::array_t<int16_t> &v0 =
	    *static_cast<const ibis::array_t<int16_t>*>(buffer);
	const ibis::array_t<int16_t> &v1 =
	    *static_cast<const ibis::array_t<int16_t>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::USHORT: {
	const ibis::array_t<uint16_t> &v0 =
	    *static_cast<const ibis::array_t<uint16_t>*>(buffer);
	const ibis::array_t<uint16_t> &v1 =
	    *static_cast<const ibis::array_t<uint16_t>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::INT: {
	const ibis::array_t<int32_t> &v0 =
	    *static_cast<const ibis::array_t<int32_t>*>(buffer);
	const ibis::array_t<int32_t> &v1 =
	    *static_cast<const ibis::array_t<int32_t>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::UINT: {
	const ibis::array_t<uint32_t> &v0 =
	    *static_cast<const ibis::array_t<uint32_t>*>(buffer);
	const ibis::array_t<uint32_t> &v1 =
	    *static_cast<const ibis::array_t<uint32_t>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::LONG: {
	const ibis::array_t<int64_t> &v0 =
	    *static_cast<const ibis::array_t<int64_t>*>(buffer);
	const ibis::array_t<int64_t> &v1 =
	    *static_cast<const ibis::array_t<int64_t>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::ULONG: {
	const ibis::array_t<uint64_t> &v0 =
	    *static_cast<const ibis::array_t<uint64_t>*>(buffer);
	const ibis::array_t<uint64_t> &v1 =
	    *static_cast<const ibis::array_t<uint64_t>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::FLOAT: {
	const ibis::array_t<float> &v0 =
	    *static_cast<const ibis::array_t<float>*>(buffer);
	const ibis::array_t<float> &v1 =
	    *static_cast<const ibis::array_t<float>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::DOUBLE: {
	const ibis::array_t<double> &v0 =
	    *static_cast<const ibis::array_t<double>*>(buffer);
	const ibis::array_t<double> &v1 =
	    *static_cast<const ibis::array_t<double>*>(other.buffer);
	return v0.equal_to(v1);}
    case ibis::CATEGORY:{
	const ibis::dictionary * aDic1 = other.getDictionary();
	if (dic->equal_to(*aDic1)) {
	    const ibis::array_t<uint32_t> &v0 = *static_cast<const ibis::array_t<uint32_t>*>(buffer);
	    const ibis::array_t<uint32_t> &v1 = *static_cast<const ibis::array_t<uint32_t>*>(other.buffer);
	    return v0.equal_to(v1);
	} else return false;

    }
    case ibis::BLOB:
    case ibis::TEXT: {
	const std::vector<std::string> &v0 =
	    *static_cast<const std::vector<std::string>*>(buffer);
	const std::vector<std::string> &v1 =
	    *static_cast<const std::vector<std::string>*>(other.buffer);
	bool match = (v0.size() == v1.size());
	for (size_t j = 0; match && j < v0.size(); ++ j)
	    match = (0 == v0[j].compare(v1[j]));
	return match;}
    }
} // ibis::bord::column::equal_to

// // explicit template function instantiations
// template int
// ibis::bord::addIncoreData<signed char>(void*&, const array_t<signed char>&,
// 				       uint32_t, const signed char);
// template int
// ibis::bord::addIncoreData<unsigned char>(void*&, const array_t<unsigned char>&,
// 					 uint32_t, const unsigned char);
// template int
// ibis::bord::addIncoreData<int16_t>(void*&, const array_t<int16_t>&, uint32_t,
// 				   const int16_t);
// template int
// ibis::bord::addIncoreData<uint16_t>(void*&, const array_t<uint16_t>&, uint32_t,
// 				    const uint16_t);
// template int
// ibis::bord::addIncoreData<int32_t>(void*&, const array_t<int32_t>&, uint32_t,
// 				   const int32_t);
// template int
// ibis::bord::addIncoreData<uint32_t>(void*&, const array_t<uint32_t>&, uint32_t,
// 				    const uint32_t);
// template int
// ibis::bord::addIncoreData<int64_t>(void*&, const array_t<int64_t>&, uint32_t,
// 				   const int64_t);
// template int
// ibis::bord::addIncoreData<uint64_t>(void*&, const array_t<uint64_t>&, uint32_t,
// 				    const uint64_t);
// template int
// ibis::bord::addIncoreData<float>(void*&, const array_t<float>&, uint32_t,
// 				 const float);
// template int
// ibis::bord::addIncoreData<double>(void*&, const array_t<double>&, uint32_t,
// 				  const double);

/// Constructor.  It retrieves the columns from the table object using that
/// function ibis::part::getColumn(uint32_t), which preserves the order
/// specified in the original table construction, but may leave the columns
/// in an arbitrary order.
ibis::bord::cursor::cursor(const ibis::bord &t)
    : buffer(t.nColumns()), tab(t), curRow(-1) {
    if (buffer.empty()) return;
    for (uint32_t j = 0; j < t.nColumns(); ++ j) {
	const ibis::bord::column *col =
	    dynamic_cast<const ibis::bord::column*>(t.getColumn(j));
	if (col != 0) {
	    buffer[j].cname = col->name();
	    buffer[j].ctype = col->type();
	    buffer[j].cval = col->getArray();
	    buffer[j].dic = col->getDictionary();
	    bufmap[col->name()] = j;
	}
    }
} // ibis::bord::cursor::cursor

/// Print the content of the current row.
int ibis::bord::cursor::dump(std::ostream& out, const char* del) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (! out) return -4;

    const uint32_t cr = static_cast<uint32_t>(curRow);
    int ierr = dumpIJ(out, cr, 0U);
    if (ierr < 0) return ierr;
    if (del == 0) del = ", ";
    for (uint32_t j = 1; j < buffer.size(); ++ j) {
	out << del;
	ierr = dumpIJ(out, cr, j);
	if (ierr < 0) return ierr;
    }
    out << "\n";
    return (out ? ierr : -4);
} // ibis::bord::cursor::dump

void ibis::bord::cursor::fillRow(ibis::table::row& res) const {
    res.clear();
    for (uint32_t j = 0; j < buffer.size(); ++ j) {
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
		res.floatsvalues.push_back(FASTBIT_FLOAT_NULL);
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
		res.doublesvalues.push_back(FASTBIT_DOUBLE_NULL);
	    }
	    break;}
	case ibis::CATEGORY: {
	    res.catsnames.push_back(buffer[j].cname);
	    if (buffer[j].cval) {
		uint32_t v = (* static_cast<const array_t<uint32_t>*>(buffer[j].cval))[curRow];
		const ibis::dictionary * aDic = buffer[j].dic;
		res.catsvalues.push_back((*aDic)[v]);
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
		    ("Warning", "bord::cursor::fillRow is not expected "
		     "to encounter data type %s (column name %s)",
		     ibis::TYPESTRING[(int)buffer[j].ctype], buffer[j].cname);
	    break;}
	}
    }
} // ibis::bord::cursor::fillRow

int ibis::bord::cursor::getColumnAsByte(uint32_t j, char& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<signed char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsByte

int ibis::bord::cursor::getColumnAsUByte(uint32_t j, unsigned char& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsUByte

int ibis::bord::cursor::getColumnAsShort(uint32_t j, int16_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (* static_cast<const array_t<signed char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (* static_cast<const array_t<int16_t>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsShort

int ibis::bord::cursor::getColumnAsUShort(uint32_t j, uint16_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsUShort

int ibis::bord::cursor::getColumnAsInt(uint32_t j, int32_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (* static_cast<const array_t<signed char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (* static_cast<const array_t<int16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	val = (* static_cast<const array_t<int32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsInt

int ibis::bord::cursor::getColumnAsUInt(uint32_t j, uint32_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	val = (* static_cast<const array_t<uint32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsUInt

int ibis::bord::cursor::getColumnAsLong(uint32_t j, int64_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (* static_cast<const array_t<signed char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (* static_cast<const array_t<int16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT: {
	val = (* static_cast<const array_t<int32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UINT: {
	val = (* static_cast<const array_t<uint32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	val = (* static_cast<const array_t<int64_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsLong

int ibis::bord::cursor::getColumnAsULong(uint32_t j, uint64_t& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE:
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT:
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT:
    case ibis::UINT: {
	val = (* static_cast<const array_t<uint32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::LONG:
    case ibis::ULONG: {
	val = (* static_cast<const array_t<uint64_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsULong

int ibis::bord::cursor::getColumnAsFloat(uint32_t j, float& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (* static_cast<const array_t<signed char>*>(buffer[j].cval))
	    [curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (* static_cast<const array_t<int16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	val = (* static_cast<const array_t<float>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsFloat

int ibis::bord::cursor::getColumnAsDouble(uint32_t j, double& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	val = (* static_cast<const array_t<signed char>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	val = (* static_cast<const array_t<unsigned char>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::SHORT: {
	val = (* static_cast<const array_t<int16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::USHORT: {
	val = (* static_cast<const array_t<uint16_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::INT: {
	val = (* static_cast<const array_t<int32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::UINT: {
	val = (* static_cast<const array_t<uint32_t>*>
	       (buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	val = (* static_cast<const array_t<double>*>(buffer[j].cval))[curRow];
	ierr = 0;
	break;}
    default: {
	ierr = -1;
	break;}
    }
    return ierr;
} // ibis::bord::cursor::getColumnAsDouble

int ibis::bord::cursor::getColumnAsString(uint32_t j, std::string& val) const {
    if (curRow < 0 || curRow >= (int64_t) tab.nRows())
	return -1;
    if (buffer[j].cval == 0)
	return -2;

    int ierr;
    std::ostringstream oss;
    switch (buffer[j].ctype) {
    case ibis::BYTE: {
	oss << static_cast<int>((* static_cast<const array_t<signed char>*>
				 (buffer[j].cval))[curRow]);
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::UBYTE: {
	oss << static_cast<unsigned>
	    ((* static_cast<const array_t<unsigned char>*>
	      (buffer[j].cval))[curRow]);
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::SHORT: {
	oss << (* static_cast<const array_t<int16_t>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::USHORT: {
	oss << (* static_cast<const array_t<uint16_t>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::INT: {
	oss << (* static_cast<const array_t<int32_t>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::UINT: {
	oss << (* static_cast<const array_t<uint32_t>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::LONG: {
	oss << (* static_cast<const array_t<int64_t>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::ULONG: {
	oss << (* static_cast<const array_t<uint64_t>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::FLOAT: {
	oss << (* static_cast<const array_t<float>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::DOUBLE: {
	oss << (* static_cast<const array_t<double>*>
		(buffer[j].cval))[curRow];
	val = oss.str();
	ierr = 0;
	break;}
    case ibis::CATEGORY:{
	uint32_t v = (* static_cast<const array_t<uint32_t>*>
		      (buffer[j].cval))[curRow];
	const ibis::dictionary * aDic = buffer[j].dic;
	val = (*aDic)[v];
	ierr = 0;
	break;
    }
    case ibis::TEXT: {
	const ibis::column* col = tab.getColumn(buffer[j].cname);
	if (col != 0) {
	    col->getString(static_cast<uint32_t>(curRow), val);
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
    return ierr;
} // ibis::bord::cursor::getColumnAsLong

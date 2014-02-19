/* File: $Id$
   Author: John Wu <John.Wu at acm.org>
      Lawrence Berkeley National Laboratory
   Copyright 20014-2014 the Regents of the University of California
*/
#include "iapi.h"
#include "bord.h"
#include "countQuery.h"
#include <memory>	// std::unique_ptr
#include <unordered_map>

/// A global variable in the file scope to hold all the active arrays known
/// to this interface.
static std::vector<ibis::bord::column*> __fastbit_iapi_all_arrays;

/// Allow for a quick look up of column objects from the address of the base
/// data.
typedef std::unordered_map<void*, uint64_t> FastBitIAPIAddressMap;
static FastBitIAPIAddressMap __fastbit_iapi_address_map;
/// Allow for a quick look up of column objects from the name of the
/// columns.
typedef std::unordered_map<const char*, uint64_t, std::hash<const char*>,
                           std::equal_to<const char*> > FastBitIAPINameMap;
static FastBitIAPINameMap __fastbit_iapi_name_map;
/// Store the query results to avoid recomputing them.
typedef std::unordered_map<FastBitSelectionHandle, ibis::bitvector*>
FastBitIAPISelectionList;
static FastBitIAPISelectionList __fastbit_iapi_selection_list;

/// A global lock with the file scope.
static pthread_mutex_t __fastbit_iapi_lock = PTHREAD_MUTEX_INITIALIZER;

// A local function for converting the types.
inline ibis::TYPE_T __fastbit_iapi_convert_data_type(FastBitDataType t) {
    switch (t) {
    default:
    case FastBitDataTypeUnknown:
        return ibis::UNKNOWN_TYPE;
    case FastBitDataTypeByte:
        return ibis::BYTE;
    case FastBitDataTypeUByte:
        return ibis::UBYTE;
    case FastBitDataTypeShort:
        return ibis::SHORT;
    case FastBitDataTypeUShort:
        return ibis::USHORT;
    case FastBitDataTypeInt:
        return ibis::INT;
    case FastBitDataTypeUInt:
        return ibis::UINT;
    case FastBitDataTypeLong:
        return ibis::LONG;
    case FastBitDataTypeULong:
        return ibis::ULONG;
    case FastBitDataTypeFloat:
        return ibis::FLOAT;
    case FastBitDataTypeDouble:
        return ibis::DOUBLE;
    }
} // __fastbit_iapi_convert_data_type

// A local function for converting a single value to double.  Returns
// FASTBIT_DOUBLE_NULL in case of error.
inline double __fastbit_iapi_convert_data_to_double
(FastBitDataType t, void *v0) {
    double ret = FASTBIT_DOUBLE_NULL;
    switch (t) {
    default:
    case FastBitDataTypeUnknown:
        break;
    case FastBitDataTypeByte:
        ret = *static_cast<signed char*>(v0);
        break;
    case FastBitDataTypeUByte:
        ret = *static_cast<unsigned char*>(v0);
        break;
    case FastBitDataTypeShort:
        ret = *static_cast<int16_t*>(v0);
        break;
    case FastBitDataTypeUShort:
        ret = *static_cast<uint16_t*>(v0);
        break;
    case FastBitDataTypeInt:
        ret = *static_cast<int32_t*>(v0);
        break;
    case FastBitDataTypeUInt:
        ret = *static_cast<uint32_t*>(v0);
        break;
    case FastBitDataTypeLong: {
        int64_t itmp = *static_cast<int64_t*>(v0);
        ret = static_cast<double>(itmp);
        LOGGER(ibis::gVerbose > 0 && itmp != static_cast<int64_t>(ret))
            << "Warning -- __fastbit_iapi_convert_data_to_double converting "
            << itmp << " to " << ret << ", the value has changed";
        break;}
    case FastBitDataTypeULong: {
        uint64_t itmp = *static_cast<uint64_t*>(v0);
        ret = static_cast<double>(itmp);
        LOGGER(ibis::gVerbose > 0 && itmp != static_cast<uint64_t>(ret))
            << "Warning -- __fastbit_iapi_convert_data_to_double converting "
            << itmp << " to " << ret << ", the value has changed";
        break;}
    case FastBitDataTypeFloat:
        ret = *static_cast<float*>(v0);
        break;
    case FastBitDataTypeDouble:
        ret = *static_cast<double*>(v0);
        break;
    }
    return ret;
} // __fastbit_iapi_convert_data_to_double

// Convert comparison operators to FastBit IBIS type.
// FastBit IBIS does not have NOT-EQUAL.  This function translates it to
// OP_UNDEFINED.
inline ibis::qExpr::COMPARE
__fastbit_iapi_convert_compare_type(FastBitCompareType t) {
    switch (t) {
    default:
        return ibis::qExpr::OP_UNDEFINED;
    case FastBitCompareLess:
        return ibis::qExpr::OP_LT;
    case FastBitCompareEqual:
        return ibis::qExpr::OP_EQ;
    case FastBitCompareGreater:
        return ibis::qExpr::OP_GT;
    case FastBitCompareLessEqual:
        return ibis::qExpr::OP_LE;
    case FastBitCompareGreaterEqual:
        return ibis::qExpr::OP_GE;
    }
} // __fastbit_iapi_convert_compare_type

ibis::bord::column* fastbit_register_array
(FastBitDataType t, void* addr, uint64_t n) {
    if (addr == 0 || t == FastBitDataTypeUnknown || n == 0)
        return 0;

    ibis::util::mutexLock(&__fastbit_iapi_lock, "fastbit_register_array");
    FastBitIAPIAddressMap::iterator it = __fastbit_iapi_address_map.find(addr);
    if (it != __fastbit_iapi_address_map.end())
        return __fastbit_iapi_all_arrays[it->second];

    uint64_t pos = __fastbit_iapi_all_arrays.size();
    std::ostringstream oss;
    oss << 'A' << pos;
    switch (t) {
    default:
    case FastBitDataTypeUnknown:
        return 0;
    case FastBitDataTypeByte: {
        ibis::array_t<signed char> *buf =
            new ibis::array_t<signed char>((signed char*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::BYTE, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeUByte: {
        ibis::array_t<unsigned char> *buf
            = new ibis::array_t<unsigned char>((unsigned char*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::UBYTE, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeShort: {
        ibis::array_t<int16_t> *buf =
            new ibis::array_t<int16_t>((int16_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::SHORT, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeUShort: {
        ibis::array_t<uint16_t> *buf =
            new ibis::array_t<uint16_t>((uint16_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::USHORT, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeInt: {
        ibis::array_t<int32_t> *buf =
            new ibis::array_t<int32_t>((int32_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::INT, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeUInt: {
        ibis::array_t<uint32_t> *buf =
            new ibis::array_t<uint32_t>((uint32_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::UINT, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeLong: {
        ibis::array_t<int64_t> *buf =
            new ibis::array_t<int64_t>((int64_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::LONG, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeULong: {
        ibis::array_t<uint64_t> *buf =
            new ibis::array_t<uint64_t>((uint64_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::ULONG, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeFloat: {
        ibis::array_t<float> *buf =
            new ibis::array_t<float>((float*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::FLOAT, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeDouble: {
        ibis::array_t<double> *buf =
            new ibis::array_t<double>((double*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(0, ibis::DOUBLE, oss.str().c_str(), buf);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    }
} // fastbit_register_array

ibis::bord::column* fastbit_register_array_nd
(FastBitDataType t, void* addr, uint64_t *dims, uint64_t nd) {
    if (addr == 0 || t == FastBitDataTypeUnknown || dims == 0 || nd == 0)
        return 0;

    uint64_t n = *dims;
    for (unsigned j = 1; j < nd; ++ j)
        n *= dims[j];

    ibis::util::mutexLock(&__fastbit_iapi_lock, "fastbit_register_array_nd");
    FastBitIAPIAddressMap::iterator it = __fastbit_iapi_address_map.find(addr);
    if (it != __fastbit_iapi_address_map.end())
        return __fastbit_iapi_all_arrays[it->second];

    uint64_t pos = __fastbit_iapi_all_arrays.size();
    std::ostringstream oss;
    oss << 'A' << pos;
    const char *cnm = oss.str().c_str();

    switch (t) {
    default:
    case FastBitDataTypeUnknown:
        return 0;
    case FastBitDataTypeByte: {
        ibis::array_t<signed char> *buf =
            new ibis::array_t<signed char>((signed char*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::BYTE, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeUByte: {
        ibis::array_t<unsigned char> *buf
            = new ibis::array_t<unsigned char>((unsigned char*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::UBYTE, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeShort: {
        ibis::array_t<int16_t> *buf =
            new ibis::array_t<int16_t>((int16_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::SHORT, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeUShort: {
        ibis::array_t<uint16_t> *buf =
            new ibis::array_t<uint16_t>((uint16_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::USHORT, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeInt: {
        ibis::array_t<int32_t> *buf =
            new ibis::array_t<int32_t>((int32_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::INT, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeUInt: {
        ibis::array_t<uint32_t> *buf =
            new ibis::array_t<uint32_t>((uint32_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::UINT, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeLong: {
        ibis::array_t<int64_t> *buf =
            new ibis::array_t<int64_t>((int64_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::LONG, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeULong: {
        ibis::array_t<uint64_t> *buf =
            new ibis::array_t<uint64_t>((uint64_t*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::ULONG, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeFloat: {
        ibis::array_t<float> *buf =
            new ibis::array_t<float>((float*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::FLOAT, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    case FastBitDataTypeDouble: {
        ibis::array_t<double> *buf =
            new ibis::array_t<double>((double*)addr, n);
        ibis::bord::column *tmp =
            new ibis::bord::column(ibis::DOUBLE, cnm, buf, dims, nd);
        __fastbit_iapi_all_arrays.push_back(tmp);
        __fastbit_iapi_address_map[addr] = pos;
        __fastbit_iapi_name_map[tmp->name()] = pos;
        return tmp;}
    }
} // fastbit_register_array_nd

ibis::bord::column* fastbit_iapi_array_by_name(const char *name) {
    if (name == 0 || *name == 0) return 0;
    FastBitIAPINameMap::iterator it = __fastbit_iapi_name_map.find(name);
    if (it != __fastbit_iapi_name_map.end()) {
        if (it->second < __fastbit_iapi_all_arrays.size()) {
            return __fastbit_iapi_all_arrays[it->second];
        }
    }
    return 0;
} // fastbit_iapi_array_by_name

void fastbit_free_array(void *addr) {
    FastBitIAPIAddressMap::const_iterator it =
        __fastbit_iapi_address_map.find(addr);
    if (it == __fastbit_iapi_address_map.end()) return;

    ibis::util::mutexLock lock(&__fastbit_iapi_lock, "fastbit_free_array");
    if (it->second < __fastbit_iapi_all_arrays.size()) {
        ibis::bord::column *col = __fastbit_iapi_all_arrays[it->second];
        __fastbit_iapi_name_map.erase(col->name());
        delete col;
        __fastbit_iapi_all_arrays[it->second] = 0;
    }
    __fastbit_iapi_address_map.erase(it);
} // fastbit_free_array

void fastbit_free_all_arrays() {
    ibis::util::mutexLock lock(&__fastbit_iapi_lock, "fastbit_free_all_arrays");
    for (unsigned j = 0; j < __fastbit_iapi_all_arrays.size(); ++j)
        delete __fastbit_iapi_all_arrays[j];
    __fastbit_iapi_name_map.clear();
    __fastbit_iapi_all_arrays.clear();
    __fastbit_iapi_address_map.clear();
} // fastbit_free_all_arrays

void fastbit_iapi_reregister_array(uint64_t i) {
    ibis::bord::column *col = __fastbit_iapi_all_arrays[i];
    __fastbit_iapi_name_map[col->name()] = i;
    switch (col->type()) {
    default:
        break;
    case FastBitDataTypeByte: {
        ibis::array_t<signed char> *buf =
            static_cast<ibis::array_t<signed char>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeUByte: {
        ibis::array_t<unsigned char> *buf =
            static_cast<ibis::array_t<unsigned char>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeShort: {
        ibis::array_t<int16_t> *buf =
            static_cast<ibis::array_t<int16_t>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeUShort: {
        ibis::array_t<uint16_t> *buf =
            static_cast<ibis::array_t<uint16_t>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeInt: {
        ibis::array_t<int32_t> *buf =
            static_cast<ibis::array_t<int32_t>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeUInt: {
        ibis::array_t<uint32_t> *buf =
            static_cast<ibis::array_t<uint32_t>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeLong: {
        ibis::array_t<int64_t> *buf =
            static_cast<ibis::array_t<int64_t>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeULong: {
        ibis::array_t<uint64_t> *buf =
            static_cast<ibis::array_t<uint64_t>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeFloat: {
        ibis::array_t<float> *buf =
            static_cast<ibis::array_t<float>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    case FastBitDataTypeDouble: {
        ibis::array_t<double> *buf =
            static_cast<ibis::array_t<double>*>(col->getArray());
        __fastbit_iapi_address_map[buf->begin()] = i;
        break;}
    }
} // fastbut_iapi_reregister_array

void fastbit_iapi_rename_array(uint64_t i) {
    std::ostringstream oss;
    oss << 'A' << i;
    __fastbit_iapi_all_arrays[i]->name(oss.str().c_str());
    fastbit_iapi_reregister_array(i);
} // fastbit_iapi_rename_array

void fastbit_iapi_rename_arrays() {
    ibis::util::mutexLock lock(&__fastbit_iapi_lock, "fastbit_free_all_arrays");
    const uint64_t ncols = __fastbit_iapi_all_arrays.size();
    __fastbit_iapi_address_map.clear();
    __fastbit_iapi_name_map.clear();
    uint64_t i = 0;
    uint64_t j = 0;
    do {
        while (i < ncols && __fastbit_iapi_all_arrays[i] != 0) {
            const char *current = __fastbit_iapi_all_arrays[i]->name();
            bool neednewname =
                (*current == 'A' || std::isdigit(current[1]) != 0);
            if (neednewname) {
                uint64_t itmp;
                ++ current;
                if (ibis::util::readUInt(itmp, current) == 0 &&
                    itmp != i)
                    neednewname = true;
            }
            if (neednewname) {
                fastbit_iapi_rename_array(i);
            }
            else {
                fastbit_iapi_reregister_array(i);
            }
            ++ i;
        }
        if (i < ncols) {
            for (j = i+1;
                 j < ncols && __fastbit_iapi_all_arrays[j] == 0;
                 ++ j);
            if (j < ncols) {
                __fastbit_iapi_all_arrays[i] = __fastbit_iapi_all_arrays[j];
                fastbit_iapi_rename_array(i);
                __fastbit_iapi_all_arrays[j] = 0;
                ++ i;
                ++ j;
            }
        }
        else {
            j = i;
        }
    } while (j < ncols);
    // settle the new size
    __fastbit_iapi_all_arrays.resize(i);
} // fastbit_free_all_arrays

const ibis::array_t<uint64_t>& fastbit_iapi_get_mesh_shape
(FastBitSelectionHandle h) {
    const static ibis::array_t<uint64_t> empty;
    while (h->getType() != ibis::qExpr::RANGE &&
           h->getType() != ibis::qExpr::DRANGE)
        h = h->getLeft();
    ibis::qRange *qr = static_cast<ibis::qRange*>(h);
    ibis::bord::column *col = fastbit_iapi_array_by_name(qr->colName());
    if (col != 0)
        return col->getMeshShape();
    else
        return empty;
} // fastbit_iapi_get_mesh_shape

void fastbit_iapi_gather_columns
(FastBitSelectionHandle h, std::vector<ibis::bord::column*> &all) {
    switch (h->getType()) {
    default: {
        if (h->getLeft())
            fastbit_iapi_gather_columns(h->getLeft(), all);
        if (h->getRight())
            fastbit_iapi_gather_columns(h->getRight(), all);
        break;}
    case ibis::qExpr::COMPRANGE: {
        if (h->getLeft())
            fastbit_iapi_gather_columns(h->getLeft(), all);
        if (h->getRight())
            fastbit_iapi_gather_columns(h->getRight(), all);
        ibis::compRange *cr = static_cast<ibis::compRange*>(h);
        if (cr->getTerm3() != 0)
            fastbit_iapi_gather_columns(cr->getTerm3(), all);
        break;}
    case ibis::qExpr::RANGE:
    case ibis::qExpr::DRANGE: {
        ibis::qRange *qr = static_cast<ibis::qRange*>(h);
        ibis::bord::column *tmp = fastbit_iapi_array_by_name(qr->colName());
        if (tmp != 0) {
            ibis::util::mutexLock
                lck1(&__fastbit_iapi_lock, "fastbit_iapi_gather_columns");
            all.push_back(tmp);
        }
        break;}
    case ibis::qExpr::STRING: {
        ibis::qString *qr = static_cast<ibis::qString*>(h);
        ibis::bord::column *tmp = fastbit_iapi_array_by_name(qr->leftString());
        if (tmp != 0) {
            ibis::util::mutexLock
                lck1(&__fastbit_iapi_lock, "fastbit_iapi_gather_columns");
            all.push_back(tmp);
        }
        break;}
    case ibis::qExpr::INTHOD: {
        ibis::qIntHod *qr = static_cast<ibis::qIntHod*>(h);
        ibis::bord::column *tmp = fastbit_iapi_array_by_name(qr->colName());
        if (tmp != 0) {
            ibis::util::mutexLock
                lck1(&__fastbit_iapi_lock, "fastbit_iapi_gather_columns");
            all.push_back(tmp);
        }
        break;}
    case ibis::qExpr::UINTHOD: {
        ibis::qUIntHod *qr = static_cast<ibis::qUIntHod*>(h);
        ibis::bord::column *tmp = fastbit_iapi_array_by_name(qr->colName());
        if (tmp != 0) {
            ibis::util::mutexLock
                lck1(&__fastbit_iapi_lock, "fastbit_iapi_gather_columns");
            all.push_back(tmp);
        }
        break;}
    }
} // fastbit_iapi_gather_columns

/// Gather all columns into an in-memory data table.
ibis::bord* fastbit_iapi_gather_columns(FastBitSelectionHandle h) {
    std::vector<ibis::bord::column*> cols;
    fastbit_iapi_gather_columns(h, cols);
    return new ibis::bord(cols);
} // fastbit_iapi_gather_columns

const ibis::bitvector* fastbit_iapi_lookup_solution(FastBitSelectionHandle h) {
    FastBitIAPISelectionList::const_iterator it =
        __fastbit_iapi_selection_list.find(h);
    if (it == __fastbit_iapi_selection_list.end())
        return 0;

    return it->second;
} // fastbit_iapi_lookup_solution

/// Copy the values from base to buf.  Only the values marked 1 in the mask
/// are copied.  Additionally, it skips over the first skip elements.
///
/// Return the number of elemented copied.
template <typename T> int64_t
fastbit_iapi_copy_values(const T *base, uint64_t nbase,
                         const ibis::bitvector &mask,
                         T *buf, uint64_t nbuf, uint64_t skip) {
    uint64_t j1 = 0;
    const ibis::bitvector::word_t *ii = 0;
    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
         is.nIndices() > 0 && j1 < nbuf; ++ is) {
        ii = is.indices();
        if (skip > 0) {
            if (skip >= is.nIndices()) {
                skip -= is.nIndices();
                continue;
            }
            if (is.isRange()) {
                for (unsigned j0 = ii[0]+skip; j0 < ii[1] && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = base[j0];
                }
            }
            else {
                for (unsigned j0 = skip; j0 < is.nIndices() && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = base[ii[j0]];
                }
            }
            skip = 0;
        }
        else {
            if (is.isRange()) {
                for (unsigned j0 = ii[0]; j0 < ii[1] && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = base[j0];
                }
            }
            else {
                for (unsigned j0 = 0; j0 < is.nIndices() && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = base[ii[j0]];
                }
            }
        }
    }
    return j1;
} // fastbit_iapi_copy_values

/// Extract the position of the rows marked 1 in the mask.  Skipping the
/// first few selected rows.
/// Return the number of positions copied.
int64_t fastbit_iapi_get_coordinates_1d
(const ibis::bitvector &mask, uint64_t *buf, uint64_t nbuf, uint64_t skip) {
    uint64_t j1 = 0;
    const ibis::bitvector::word_t *ii = 0;
    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
         is.nIndices() > 0 && j1 < nbuf; ++ is) {
        ii = is.indices();
        if (skip > 0) {
            if (skip >= is.nIndices()) {
                skip -= is.nIndices();
                continue;
            }
            if (is.isRange()) {
                for (unsigned j0 = ii[0]+skip; j0 < ii[1] && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = j0;
                }
            }
            else {
                for (unsigned j0 = skip; j0 < is.nIndices() && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = ii[j0];
                }
            }
            skip = 0;
        }
        else {
            if (is.isRange()) {
                for (unsigned j0 = ii[0]; j0 < ii[1] && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = j0;
                }
            }
            else {
                for (unsigned j0 = 0; j0 < is.nIndices() && j1 < nbuf;
                     ++ j0, ++j1) {
                    buf[j1] = ii[j0];
                }
            }
        }
    }
    return j1;
} // fastbit_iapi_get_coordinates_1d

/// Convert the selected positions to 2-dimension coordinates.  The
/// argument @c dim1 is the faster varying dimension.
int64_t fastbit_iapi_get_coordinates_2d
(const ibis::bitvector &mask, uint64_t *buf, uint64_t nbuf, uint64_t skip,
 uint64_t dim1) {
    if (dim1 == 0 || nbuf < 2) return -1;

    uint64_t j1 = 0;
    const ibis::bitvector::word_t *ii = 0;
    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
         is.nIndices() > 0 && j1 < nbuf; ++ is) {
        ii = is.indices();
        if (skip > 0) {
            if (skip >= is.nIndices()) {
                skip -= is.nIndices();
                continue;
            }
            if (is.isRange()) {
                for (unsigned j0 = ii[0]+skip; j0 < ii[1] && j1+1 < nbuf;
                     ++ j0, j1+=2) {
                    buf[j1]   = j0 / dim1;
                    buf[j1+1] = j0 % dim1;
                }
            }
            else {
                for (unsigned j0 = skip; j0 < is.nIndices() && j1+1 < nbuf;
                     ++ j0, j1+=2) {
                    buf[j1]   = ii[j0] / dim1;
                    buf[j1+1] = ii[j0] % dim1;
                }
            }
            skip = 0;
        }
        else {
            if (is.isRange()) {
                for (unsigned j0 = ii[0]; j0 < ii[1] && j1+1 < nbuf;
                     ++ j0, j1 += 2) {
                    buf[j1]   = j0 / dim1;
                    buf[j1+1] = j0 % dim1;
                }
            }
            else {
                for (unsigned j0 = 0; j0 < is.nIndices() && j1+1 < nbuf;
                     ++ j0, j1+=2) {
                    buf[j1]   = ii[j0] / dim1;
                    buf[j1+1] = ii[j0] % dim1;
                }
            }
        }
    }
    return (j1>>1);
} // fastbit_iapi_get_coordinates_2d

/// Convert the selected positions to 3-dimension coordinates.  The
/// argument @c dim2 is the fastest varying dimension.
int64_t fastbit_iapi_get_coordinates_3d
(const ibis::bitvector &mask, uint64_t *buf, uint64_t nbuf, uint64_t skip,
 uint64_t dim1, uint64_t dim2) {
    if (dim1 == 0 || dim2 == 0 || nbuf < 3) return -1;

    uint64_t j1 = 0;
    const uint64_t dim12 = dim1 * dim2;
    const ibis::bitvector::word_t *ii = 0;
    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
         is.nIndices() > 0 && j1 < nbuf; ++ is) {
        ii = is.indices();
        if (skip > 0) {
            if (skip >= is.nIndices()) {
                skip -= is.nIndices();
                continue;
            }
            if (is.isRange()) {
                for (unsigned j0 = ii[0]+skip; j0 < ii[1] && j1+2 < nbuf;
                     ++ j0, j1+=3) {
                    buf[j1]   = j0 / dim12;
                    buf[j1+1] = (j0 % dim12) / dim2;
                    buf[j1+2] %= dim2;
                }
            }
            else {
                for (unsigned j0 = skip; j0 < is.nIndices() && j1+2 < nbuf;
                     ++ j0, j1+=3) {
                    buf[j1]   = ii[j0] / dim12;
                    buf[j1+1] = (ii[j0] % dim12) / dim2;
                    buf[j1+2] = ii[j0] % dim2;
                }
            }
            skip = 0;
        }
        else {
            if (is.isRange()) {
                for (unsigned j0 = ii[0]; j0 < ii[1] && j1+2 < nbuf;
                     ++ j0, j1 += 2) {
                    buf[j1]   = j0 / dim12;
                    buf[j1+1] = (j0 % dim12) / dim2;
                    buf[j1+2] = j0 % dim2;
                }
            }
            else {
                for (unsigned j0 = 0; j0 < is.nIndices() && j1+2 < nbuf;
                     ++ j0, j1+=3) {
                    buf[j1]   = ii[j0] / dim12;
                    buf[j1+1] = (ii[j0] % dim12) / dim2;
                    buf[j1+2] = ii[j0] % dim2;
                }
            }
        }
    }
    return (j1/3);
} // fastbit_iapi_get_coordinates_3d

/// Convert a global position to N-D coordinates.  It assume nd > 1 without
/// checking.
inline void fastbit_iapi_global_to_nd
(uint64_t nd, uint64_t *coords, uint64_t global, const uint64_t *cumu) {
    for (unsigned jd = 0; jd+1 < nd; ++jd) {
        coords[jd] = global / cumu[jd+1];
        global -= (global / cumu[jd+1]) * cumu[jd+1];
    }
    coords[nd-1] = global;
} // fasbtit_iapi_global_to_nd

/// Convert the selected positions to N-dimension coordinates.  This
/// function can not be used for cases with 1 dimension.
int64_t fastbit_iapi_get_coordinates_nd
(const ibis::bitvector &mask, uint64_t *buf, uint64_t nbuf, uint64_t skip,
 const uint64_t *dims, uint64_t nd) {
    if (dims == 0 || nd == 0 || nbuf < nd || nd < 2) return -1;

    uint64_t j1 = 0;
    uint64_t *cumu = new uint64_t[nd];
    cumu[nd-1] = dims[nd-1];
    for (unsigned j = nd-1; j > 0; -- j) {
        cumu[j-1] = cumu[j] * dims[j-1];
    }

    const ibis::bitvector::word_t *ii = 0;
    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
         is.nIndices() > 0 && j1 < nbuf; ++ is) {
        ii = is.indices();
        if (skip > 0) {
            if (skip >= is.nIndices()) {
                skip -= is.nIndices();
                continue;
            }
            if (is.isRange()) {
                for (unsigned j0 = ii[0]+skip; j0 < ii[1] && j1+nd <= nbuf;
                     ++ j0, j1+=nd) {
                    fastbit_iapi_global_to_nd(nd, buf+j1, j0, cumu);
                }
            }
            else {
                for (unsigned j0 = skip; j0 < is.nIndices() && j1+nd <= nbuf;
                     ++ j0, j1+=nd) {
                    fastbit_iapi_global_to_nd(nd, buf+j1, ii[j0], cumu);
                }
            }
            skip = 0;
        }
        else {
            if (is.isRange()) {
                for (unsigned j0 = ii[0]; j0 < ii[1] && j1+nd <= nbuf;
                     ++ j0, j1 += nd) {
                    fastbit_iapi_global_to_nd(nd, buf+j1, j0, cumu);
                }
            }
            else {
                for (unsigned j0 = 0; j0 < is.nIndices() && j1+2 < nbuf;
                     ++ j0, j1+=3) {
                    fastbit_iapi_global_to_nd(nd, buf+j1, ii[j0], cumu);
                }
            }
        }
    }
    return (j1/nd);
} // fastbit_iapi_get_coordinates_nd




// *** public functions start here ***

/// The incoming type must of an elementary data type, both buf and bound
/// must be valid pointers.  This function registers the incoming array as
/// ibis::bord::column object.
///
/// It returns a nil value in case of error.
extern "C" FastBitSelectionHandle fastbit_create_selection
(FastBitDataType dtype, void *buf, uint64_t nelm,
 FastBitCompareType ctype, void *bound) {
    if (dtype == FastBitDataTypeUnknown || buf == 0 || nelm == 0 || bound == 0)
        return 0;

    ibis::bord::column *col = fastbit_register_array(dtype, buf, nelm);
    ibis::qExpr::COMPARE cmp = __fastbit_iapi_convert_compare_type(ctype);
    ibis::qExpr *ret = 0;
    bool negate = false;
    if (cmp == ibis::qExpr::OP_UNDEFINED) {
        cmp = ibis::qExpr::OP_EQ;
        negate = true;
    }

    double dval = __fastbit_iapi_convert_data_to_double(dtype, bound);
    if (dval == FASTBIT_DOUBLE_NULL)
        return 0;

    ret = new ibis::qContinuousRange(col->name(), cmp, dval);
    if (negate) {
        ibis::qExpr *tmp = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
        tmp->setLeft(ret);
        ret = tmp;
    }
    return ret;
} // fastbit_create_selection

/// The incoming type must of an elementary data type, both buf and bound
/// must be valid pointers.  This function registers the incoming array as
/// ibis::bord::column object.
///
/// It returns a nil value in case of error.
extern "C" FastBitSelectionHandle fastbit_create_selection_nd
(FastBitDataType dtype, void *buf, uint64_t *dims, uint64_t nd,
 FastBitCompareType ctype, void *bound) {
    if (dtype == FastBitDataTypeUnknown || buf == 0 || dims == 0 || nd == 0
        || bound == 0)
        return 0;

    ibis::bord::column *col = fastbit_register_array_nd(dtype, buf, dims, nd);
    ibis::qExpr::COMPARE cmp = __fastbit_iapi_convert_compare_type(ctype);
    ibis::qExpr *ret = 0;
    bool negate = false;
    if (cmp == ibis::qExpr::OP_UNDEFINED) {
        cmp = ibis::qExpr::OP_EQ;
        negate = true;
    }

    double dval = __fastbit_iapi_convert_data_to_double(dtype, bound);
    if (dval == FASTBIT_DOUBLE_NULL)
        return 0;

    ret = new ibis::qContinuousRange(col->name(), cmp, dval);
    if (negate) {
        ibis::qExpr *tmp = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
        tmp->setLeft(ret);
        ret = tmp;
    }
    return ret;
} // fastbit_create_selection_nd

/// Free the objects representing the selection.  Only the top most level
/// of the object hierarchy, i.e., the last selection handle return by the
/// combine operations, needs to be freed.
extern "C" void fastbit_free_selection(FastBitSelectionHandle h) {
    delete h;
} // fastbit_free_selection


/// Combine two sets of selection conditions into one.  The new object take
/// ownership of the two incoming expressions.  This arrangement allows the
/// user to delete the last object produced to free all objects going into
/// building the last combined object.
extern "C" FastBitSelectionHandle fastbit_combine_selections
(FastBitSelectionHandle h1, FastBitCombineType cmb, FastBitSelectionHandle h2) {
    ibis::qExpr *ret = 0;
    if (h1 == 0 || h2 == 0) {
        LOGGER(ibis::gVerbose > 2)
            << "Warning -- fastbit_combine_selections can not proceed with "
            "a nil FastBit select handle";
        return ret;
    }

    switch (cmb) {
    default:
        break;
    case FastBitCombineAnd: {
        ret = new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
        ret->setLeft(h1);
        ret->setRight(h2);
        break;}
    case FastBitCombineOr: {
        ret = new ibis::qExpr(ibis::qExpr::LOGICAL_OR);
        ret->setLeft(h1);
        ret->setRight(h2);
        break;}
    case FastBitCombineXor: {
        ret = new ibis::qExpr(ibis::qExpr::LOGICAL_XOR);
        ret->setLeft(h1);
        ret->setRight(h2);
        break;}
    case FastBitCombineNand: {
        ibis::qExpr *tmp = new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
        tmp->setLeft(h1);
        tmp->setRight(h2);
        ret = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
        ret->setLeft(tmp);
        break;}
    case FastBitCombineNor: {
        ibis::qExpr *tmp = new ibis::qExpr(ibis::qExpr::LOGICAL_OR);
        tmp->setLeft(h1);
        tmp->setRight(h2);
        ret = new ibis::qExpr(ibis::qExpr::LOGICAL_NOT);
        ret->setLeft(tmp);
        break;}
    }
    return ret;
} // fastbit_combine_selections

/// This function is only meant to provide a rough eastimate of the upper
/// bound of the number of hits.  There is no guarantee on how accurate is
/// the estimation.  This estimation may be sufficient for the purpose of
/// allocating workspace required for reading the selection.
extern "C" uint64_t fastbit_estimate_num_hits(FastBitSelectionHandle h) {
    const ibis::bitvector *res = fastbit_iapi_lookup_solution(h);
    if (res != 0)
        return res->cnt();

    std::unique_ptr<ibis::bord> brd(fastbit_iapi_gather_columns(h));
    ibis::countQuery que(brd.get());
    int ierr = que.setWhereClause(h);
    if (ierr < 0)
        return brd->nRows();

    ierr = que.estimate();
    if (ierr < 0)
        return brd->nRows();

    return que.getMaxNumHits();
} // fastbit_estimate_num_hits

/// Compute the numebr of hits.  This function performs the exact
/// evaluation and store the results in a global data structure.
///
/// @note The precise evaluation needs to be performed before reading the
/// data values.  If it is not performed, the read selection function will
/// perform the precise evaluation.
extern "C" int64_t fastbit_get_num_hits(FastBitSelectionHandle h) {
    if (h == 0) {
        LOGGER(ibis::gVerbose > 2)
            << "Warning -- fastbit_read_selection can not proceed with "
            "a nil FastBit select handle";
        return -1;
    }

    const ibis::bitvector *res = fastbit_iapi_lookup_solution(h);
    if (res != 0)
        return res->cnt();

    std::unique_ptr<ibis::bord> brd(fastbit_iapi_gather_columns(h));
    if (brd.get() == 0)
        return -2;
    ibis::countQuery que(brd.get());
    int ierr = que.setWhereClause(h);
    if (ierr < 0)
        return -3;

    ierr = que.evaluate();
    if (ierr < 0)
        return -4;

    ibis::util::mutexLock lock(&__fastbit_iapi_lock, "fastbit_get_num_hits");
    __fastbit_iapi_selection_list[h] = new ibis::bitvector(*que.getHitVector());
    return que.getNumHits();
} // fastbit_get_num_hits

/// Fill the buffer (buf) with the next set of values satisfying the
/// selection criteria.
///
/// Both nbase and nbuf are measured in number of elements of the specified
/// type, NOT in bytes.
///
/// The start position is measuremeted as positions in the list of selected
/// values, not positions in the base data.
///
/// The return value is the number of elements successfully read.  In case
/// of error, a negative value is returned.
extern "C" int64_t fastbit_read_selection
(FastBitDataType dtype, void *base, uint64_t nbase,
 FastBitSelectionHandle h, void *buf, uint64_t nbuf, uint64_t start) {
    if (dtype == FastBitDataTypeUnknown || base == 0 || nbase == 0 ||
        h == 0 || buf == 0 || nbuf == 0) {
        LOGGER(ibis::gVerbose > 2)
            << "Warning -- fastbit_read_selection can not proceed with "
            "a nil FastBit select handle or nil buffer";
        return -1;
    }
    if (start >= nbase)
        return 0;

    int64_t ierr = fastbit_get_num_hits(h);
    if (ierr <= 0) return ierr;

    const ibis::bitvector &mask = *__fastbit_iapi_selection_list[h];
    switch(dtype) {
    default:
        return -5;
    case FastBitDataTypeByte:
        ierr = fastbit_iapi_copy_values<signed char>
            (static_cast<signed char*>(base), nbase, mask,
             static_cast<signed char*>(buf), nbuf, start);
        break;
    case FastBitDataTypeUByte:
        ierr = fastbit_iapi_copy_values<unsigned char>
            (static_cast<unsigned char*>(base), nbase, mask,
             static_cast<unsigned char*>(buf), nbuf, start);
        break;
    case FastBitDataTypeShort:
        ierr = fastbit_iapi_copy_values<int16_t>
            (static_cast<int16_t*>(base), nbase, mask,
             static_cast<int16_t*>(buf), nbuf, start);
        break;
    case FastBitDataTypeUShort:
        ierr = fastbit_iapi_copy_values<uint16_t>
            (static_cast<uint16_t*>(base), nbase, mask,
             static_cast<uint16_t*>(buf), nbuf, start);
        break;
    case FastBitDataTypeInt:
        ierr = fastbit_iapi_copy_values<int32_t>
            (static_cast<int32_t*>(base), nbase, mask,
             static_cast<int32_t*>(buf), nbuf, start);
        break;
    case FastBitDataTypeUInt:
        ierr = fastbit_iapi_copy_values<uint32_t>
            (static_cast<uint32_t*>(base), nbase, mask,
             static_cast<uint32_t*>(buf), nbuf, start);
        break;
    case FastBitDataTypeLong:
        ierr = fastbit_iapi_copy_values<int64_t>
            (static_cast<int64_t*>(base), nbase, mask,
             static_cast<int64_t*>(buf), nbuf, start);
        break;
    case FastBitDataTypeULong:
        ierr = fastbit_iapi_copy_values<uint64_t>
            (static_cast<uint64_t*>(base), nbase, mask,
             static_cast<uint64_t*>(buf), nbuf, start);
        break;
    case FastBitDataTypeFloat:
        ierr = fastbit_iapi_copy_values<float>
            (static_cast<float*>(base), nbase, mask,
             static_cast<float*>(buf), nbuf, start);
        break;
    case FastBitDataTypeDouble:
        ierr = fastbit_iapi_copy_values<double>
            (static_cast<double*>(base), nbase, mask,
             static_cast<double*>(buf), nbuf, start);
        break;
    }
    return ierr;
} // fastbit_read_selection

/// 
/// The shape of the array is determined by shape of the array in the first
/// (left-most) selection condition tree.  The implicit assumption is that
/// all arrays/variables involved in the selection conditions have the same
/// shape.
extern "C" int64_t fastbit_get_coordinates
(FastBitSelectionHandle h, uint64_t *buf, uint64_t nbuf, uint64_t skip) {
    if (h == 0 || buf == 0 || nbuf == 0) {
        LOGGER(ibis::gVerbose > 2)
            << "Warning -- fastbit_get_coordinates can not proceed with "
            "a nil FastBit select handle or nil buffer";
        return -1;
    }

    int64_t ierr = fastbit_get_num_hits(h);
    if (ierr <= 0) return ierr;
    if (skip >= (uint64_t)ierr)
        return 0;

    const ibis::bitvector &mask = *__fastbit_iapi_selection_list[h];
    const ibis::array_t<uint64_t> &dims = fastbit_iapi_get_mesh_shape(h);
    if (dims.size() > nbuf) {
        LOGGER(ibis::gVerbose > 2)
            << "Warning -- fastbit_get_coordinates can not write one set "
            "of coordinates into the given buffer, dims.size() = "
            << dims.size() << ", nbuf = " << nbuf;
        return -1;
    }

    switch (dims.size()) {
    case 0:
    case 1:
        ierr = fastbit_iapi_get_coordinates_1d
            (mask, buf, nbuf, skip);
        break;
    case 2:
        ierr = fastbit_iapi_get_coordinates_2d
            (mask, buf, nbuf, skip, dims[1]);
        break;
    case 3:
        ierr = fastbit_iapi_get_coordinates_3d
            (mask, buf, nbuf, skip, dims[1], dims[2]);
        break;
    default:
        ierr = fastbit_iapi_get_coordinates_nd
            (mask, buf, nbuf, skip, dims.begin(), dims.size());
        break;
    }
    return ierr;
} // fastbit_get_coordinates


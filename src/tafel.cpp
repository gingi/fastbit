// File $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2007-2009 the Regents of the University of California
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "tafel.h"	// ibis::tafel
#include "part.h"	// ibis::part

#include <fstream>	// std::ofstream
#include <limits>	// std::numeric_limits
#include <typeinfo>	// typeid
// This file definte does not use the min and max macro.  Their presence
// could cause the calls to numeric_limits::min and numeric_limits::max to
// be misunderstood!
#undef max
#undef min

/// Add metadata about a new column.
/// Return value
/// -  0 == success,
/// - -2 == invalid name or type,
/// -  1 == name already in the list of columns, same type,
/// - -1 == existing column with different type.
int ibis::tafel::addColumn(const char* cn, ibis::TYPE_T ct,
			   const char* cd, const char* idx) {
    if (cn == 0 || *cn == 0 || ct == ibis::UNKNOWN_TYPE) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- tafel::addColumn(" << (void*)cn << ", "
	    << (void*)ct << ", " << (void*)cd << ", " << (void*)idx
	    << ") expects a valid name (1st arguement) and type (2nd argument)";
	return -2;
    }
    columnList::iterator it = cols.find(cn);
    if (it != cols.end()) {
	LOGGER(ibis::gVerbose > 1)
	    << "tafel::addColumn(" << cn << ", " << ct
	    << ") -- name already in the data partition";
	if (cd != 0 && *cd != 0)
	    it->second->desc = cd;
	if (idx != 0 && *idx != 0)
	    it->second->indexSpec = idx;
	return (ct == it->second->type ? 1 : -1);
    }

    column* col = new column();
    col->name = cn;
    col->type = ct;
    col->desc = (cd && *cd ? cd : cn);
    if (idx != 0 && *idx != 0)
	col->indexSpec = idx;
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
    case ibis::BLOB:
	col->values = new array_t<unsigned char>();
    default:
	break;
    }
    cols[col->name.c_str()] = col;
    colorder.push_back(col);
    return 0;
} // ibis::tafel::addColumn

/// Ingest a complete SQL CREATE TABLE statement.  Creates all metadata
/// specified.  It extracts the table name (into tname) to be used later by
/// functions such as write and writeMetaData.
///
/// The statement is expected to be in the form of "create table tname
/// (column1, column2, ...)".  It can not contain embedded comments.
///
/// Because the SQL standard supports many more data types than FastBit
/// does, many SQL column types are mapped in a crude manner.  Here is the
/// current list.
/// - enum = ibis::CATEGORY; the values specified are not recorded.
/// - set = ibis::CATEGORY; this treatment does not full reflect the
///         flexibility with which a set value can be handled in SQL.
/// - blob = ibis::BLOB; however, since SQL dump file contains only
///          printable characters, this type is somewhat useless.
int ibis::tafel::SQLCreateTable(const char *stmt, std::string &tname) {
    if (stmt == 0 && *stmt == 0) return -1;
    if (strnicmp(stmt, "create table ", 13) != 0)
	return -1;

    const char *buf = stmt + 13;
    ibis::util::getString(tname, buf, 0); // extract table name

    while (*buf != 0 && *buf != '(') ++ buf;
    buf += (*buf == '('); // skip opening (
    if (buf == 0 || *buf == 0) { // incomplete SQL statement
	tname.erase();
	return -1;
    }

    clear(); // clear all existing content
    std::string colname, tmp;
    ibis::TYPE_T coltype;
    ibis::tafel::column *col;
    const char *delim = " ,\t\n\v";
    while (*buf != 0 && *buf != ')') { // loop till closing )
	ibis::util::getString(colname, buf, 0);
	if (colname.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "tafel::SQLCreateTable failed to extract a column";
	    return -2;
	}
	else if (stricmp(colname.c_str(), "key") == 0) { // reserved word
	    // KEY name (name, name)
	    ibis::util::getString(colname, buf, 0);
	    while (*buf != 0 && *buf != '(' && *buf != ',') ++ buf;
	    if (*buf == '(') { // skip till ')'
		while (*buf != 0 && *buf != ')') ++ buf;
		buf += (*buf == ')');
	    }
	    while (*buf != 0 && *buf != ',' && *buf != ')') ++ buf;
	    buf += (*buf == ',');
	    continue;
	}

	while (*buf !=0 && isspace(*buf)) ++ buf;
	switch (*buf) { // for data types
	defaul: { // unknown type
		col = 0;
		ibis::util::getString(tmp, buf, delim);
		LOGGER(ibis::gVerbose > 0)
		    << "tafel::SQLCreateTable column " << colname
		    << " has a unexpected type (" << tmp
		    << "), skip column specification";
		while (*buf != 0 && *buf != ',') ++ buf;
		break;}
	case 'b':
	case 'B': { // blob/bigint
	    if (strnicmp(buf, "bigint", 6) == 0) {
		col = new ibis::tafel::column;
		col->name.swap(colname);
		buf += 6;
		if (*buf == '(') {
		    for (++ buf; *buf != ')'; ++ buf);
		    buf += (*buf == ')');
		}
		while (*buf != 0 && isspace(*buf)) ++ buf;
		if (*buf != 0 && strnicmp(buf, "unsigned", 9) == 0) {
		    buf += 8;
		    col->type = ibis::ULONG;
		    col->values = new ibis::array_t<uint64_t>();
		}
		else {
		    col->type = ibis::LONG;
		    col->values = new ibis::array_t<int64_t>();
		}
	    }
	    else { // assume blob
		buf += 4;
		col = new ibis::tafel::column;
		col->name.swap(colname);
		col->type = ibis::BLOB;
		col->values = new array_t<unsigned char>();
	    }
	    break;}
	case 'e':
	case 'E': { // enum
	    buf += 4;
	    while (*buf != ',' && isspace(*buf)) ++ buf;
	    if (*buf == '(') {
		for (++ buf; *buf != 0 && *buf != ')'; ++ buf);
		buf += (*buf == ')');
	    }
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    col->type = ibis::CATEGORY;
	    col->values = new std::vector<std::string>();
	    break;}
	case 'd':
	case 'D': { // double
	    buf += 6;
	    if (*buf == '(') {
		for (++ buf; *buf != ')'; ++ buf);
		buf += (*buf == ')');
	    }
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    col->type = ibis::DOUBLE;
	    col->values = new ibis::array_t<double>();
	    break;}
	case 'f':
	case 'F': { // float
	    buf += 5;
	    if (*buf == '(') {
		for (++ buf; *buf != ')'; ++ buf);
		buf += (*buf == ')');
	    }
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    col->type = ibis::FLOAT;
	    col->values = new ibis::array_t<float>();
	    break;}
	case 'i':
	case 'I': { // int/integer	    
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    buf += (strnicmp(buf, "integer", 7) == 0 ? 7 : 3);
	    if (*buf == '(') {
		for (++ buf; *buf != ')'; ++ buf);
		buf += (*buf == ')');
	    }
	    while (*buf != 0 && isspace(*buf)) ++ buf;
	    if (*buf != 0 && strnicmp(buf, "unsigned", 8) == 0) {
		buf += 8;
		col->type = ibis::UINT;
		col->values = new ibis::array_t<uint32_t>();
	    }
	    else {
		col->type = ibis::INT;
		col->values = new ibis::array_t<int32_t>();
	    }
	    break;}
	case 's':
	case 'S': { // smallint/short/set
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    if (strnicmp(buf, "set", 3) == 0) {
		buf += 3;
		while (*buf != ',' && isspace(*buf)) ++ buf;
		if (*buf == '(') {
		    for (++ buf; *buf != 0 && *buf != ')'; ++ buf);
		    buf += (*buf == ')');
		}
		col->type = ibis::CATEGORY;
		col->values = new std::vector<std::string>();
	    }
	    else { // assume smallint
		buf += (strnicmp(buf, "short", 5) == 0 ? 5 : 8);
		if (*buf == '(') {
		    for (++ buf; *buf != ')'; ++ buf);
		    buf += (*buf == ')');
		}
		while (*buf != 0 && isspace(*buf)) ++ buf;
		if (*buf != 0 && strnicmp(buf, "unsigned", 8) == 0) {
		    buf += 8;
		    col->type = ibis::USHORT;
		    col->values = new ibis::array_t<uint16_t>();
		}
		else {
		    coltype = ibis::SHORT;
		    col->values = new ibis::array_t<int16_t>();
		}
	    }
	    break;}
	case 't':
	case 'T': { // tinyint
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    buf += 7;
	    if (*buf == '(') {
		for (++ buf; *buf != ')'; ++ buf);
		buf += (*buf == ')');
	    }
	    while (*buf != 0 && isspace(*buf)) ++ buf;
	    if (*buf != 0 && strnicmp(buf, "unsigned", 8) == 0) {
		buf += 8;
		col->type = ibis::UBYTE;
		col->values = new ibis::array_t<unsigned char>();
	    }
	    else {
		col->type = ibis::BYTE;
		col->values = new ibis::array_t<signed char>();
	    }
	    break;}
	case 'v':
	case 'V': { // varchar
	    col = new ibis::tafel::column;
	    col->name.swap(colname);
	    buf += 7;
	    int precision = 0;
	    if (*buf == '(') {
		for (++ buf; isdigit(*buf); ++ buf) {
		    precision = 10 * precision + (*buf - '0');
		}
		while (*buf != 0 && *buf != ')') ++ buf;
		buf += (*buf == ')');
	    }
	    if (precision < 6) {
		col->type = ibis::CATEGORY;
	    }
	    else {
		col->type = ibis::TEXT;
	    }
	    col->values = new std::vector<std::string>();
	    break;}
	}

	if (col != 0) { // add col to cols
	    cols[col->name.c_str()] = col;
	    colorder.push_back(col);

	    while (*buf != 0 && *buf != ',') {
		ibis::util::getString(tmp, buf, delim);
		if ((!tmp.empty()) && stricmp(tmp.c_str(), "default") == 0) {
		    int ierr = assignDefaultValue(*col, buf);
		    LOGGER(ierr < 0 && ibis::gVerbose > 1)
			<< "tafel::SQLCreateTable failed to assign a default "
			"value to column " << col->name;
		    break;
		}
	    }

	    if (ibis::gVerbose > 4) {
		ibis::util::logger lg;
		lg.buffer() << "tafel::SQLCreateTable created column "
			    << col->name << " with type "
			    << ibis::TYPESTRING[(int)col->type];
		if (col->defval != 0) {
		    lg.buffer() << " and defaul value ";
		    switch (col->type) {
		    case ibis::BYTE:
			lg.buffer() << static_cast<short>
			    (*static_cast<signed char*>(col->defval));
			break;
		    case ibis::UBYTE:
			lg.buffer() << static_cast<short>
			    (*static_cast<unsigned char*>(col->defval));
			break;
		    case ibis::SHORT:
			lg.buffer() << *static_cast<int16_t*>(col->defval);
			break;
		    case ibis::USHORT:
			lg.buffer() << *static_cast<uint16_t*>(col->defval);
			break;
		    case ibis::INT:
			lg.buffer() << *static_cast<int32_t*>(col->defval);
			break;
		    case ibis::UINT:
			lg.buffer() << *static_cast<uint32_t*>(col->defval);
			break;
		    case ibis::LONG:
			lg.buffer() << *static_cast<int64_t*>(col->defval);
			break;
		    case ibis::ULONG:
			lg.buffer() << *static_cast<uint64_t*>(col->defval);
			break;
		    case ibis::FLOAT:
			lg.buffer() << *static_cast<float*>(col->defval);
			break;
		    case ibis::DOUBLE:
			lg.buffer() << *static_cast<double*>(col->defval);
			break;
		    case ibis::BLOB:
		    case ibis::TEXT:
		    case ibis::CATEGORY:
			lg.buffer() << *static_cast<std::string*>(col->defval);
			break;
		    default:
			break;
		    }
		}
	    }
	}

	// skip the remaining part of this column specification
	while (*buf != 0 && *buf != ',') ++ buf;
	buf += (*buf == ',');
    }

    LOGGER(ibis::gVerbose > 2)
	<< "tafel::SQLCreateTable extract meta data for " << cols.size()
	<< " column" << (cols.size()>1 ? "s" : "") << " from " << stmt;
    return cols.size();
} // ibis::tafel::SQLCreateTable

/// Assign the default value for the given column.  Returns 0 on success
/// and a negative number for error.
int ibis::tafel::assignDefaultValue(ibis::tafel::column& col,
				    const char *val) const {
    char *ptr;
    switch (col.type) {
    case ibis::BYTE: {
	long tmp = strtol(val, &ptr, 0);
	if (tmp >= -128 && tmp <= 127) {
	    char *actual = new char;
	    *actual = static_cast<char>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is out of range for column type BYTE";
	    return -14;
	}
	break;}
    case ibis::UBYTE: {
	long tmp = strtol(val, &ptr, 0);
	if (tmp >= 0 && tmp <= 255) {
	    unsigned char *actual = new unsigned char;
	    *actual = static_cast<unsigned char>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is out of range for column type UBYTE";
	    return -13;
	}
	break;}
    case ibis::SHORT: {
	long tmp = strtol(val, &ptr, 0);
	if (tmp >= std::numeric_limits<int16_t>::min() &&
	    tmp <= std::numeric_limits<int16_t>::max()) {
	    int16_t *actual = new int16_t;
	    *actual = static_cast<int16_t>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is out of range for column type SHORT";
	    return -12;
	}
	break;}
    case ibis::USHORT: {
	long tmp = strtol(val, &ptr, 0);
	if (tmp >= 0 && tmp <= std::numeric_limits<uint16_t>::max()) {
	    uint16_t *actual = new uint16_t;
	    *actual = static_cast<uint16_t>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is out of range for column type USHORT";
	    return -11;
	}
	break;}
    case ibis::INT: {
#if defined(_WIN32) && defined(_MSC_VER)
	long tmp = strtol(val, &ptr, 0);
#else
	long long tmp = strtoll(val, &ptr, 0);
#endif
	if (tmp >= std::numeric_limits<int32_t>::min() &&
	    tmp <= std::numeric_limits<int32_t>::max()) {
	    int32_t *actual = new int32_t;
	    *actual = static_cast<int32_t>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is out of range for column type INT";
	    return -10;
	}
	break;}
    case ibis::UINT: {
#if defined(_WIN32) && defined(_MSC_VER)
	long tmp = strtol(val, &ptr, 0);
#else
	long long tmp = strtoll(val, &ptr, 0);
#endif
	if (tmp >= 0 && tmp <= std::numeric_limits<uint32_t>::max()) {
	    uint32_t *actual = new uint32_t;
	    *actual = static_cast<uint32_t>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is out of range for column type UINT";
	    return -9;
	}
	break;}
    case ibis::LONG: {
	errno = 0;
#if defined(_WIN32) && defined(_MSC_VER)
	long tmp = strtol(val, &ptr, 0);
#else
	long long tmp = strtoll(val, &ptr, 0);
#endif
	if (errno == 0) {
	    int64_t *actual = new int64_t;
	    *actual = tmp;
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is invalid or out of range for column type LONG";
	    return -8;
	}
	break;}
    case ibis::ULONG: {
	errno = 0;
#if defined(_WIN32) && defined(_MSC_VER)
	long tmp = strtol(val, &ptr, 0);
#else
	long long tmp = strtoll(val, &ptr, 0);
#endif
	if (tmp >= 0 && errno == 0) {
	    uint64_t *actual = new uint64_t;
	    *actual = static_cast<uint64_t>(tmp);
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is invalid or out of range for column type ULONG";
	    return -7;
	}
	break;}
    case ibis::FLOAT: {
	errno = 0;
#if defined(_WIN32) && defined(_MSC_VER)
	float tmp = strtod(val, &ptr);
#else
	float tmp = strtof(val, &ptr);
#endif
	if (errno == 0) { // no conversion error
	    float *actual = new float;
	    *actual = tmp;
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is invalid or out of range for column type FLOAT";
	    return -6;
	}
	break;}
    case ibis::DOUBLE: {
	errno = 0;
	double tmp = strtod(val, &ptr);
	if (errno == 0) { // no conversion error
	    double *actual = new double;
	    *actual = tmp;
	    col.defval = actual;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "tafel::assignDefaultValue(" << col.name << ", " << val
		<< ") can not continue because the value (" << tmp
		<< ") is invalid or out of range for column type DOUBLE";
	    return -5;
	}
	break;}
    case ibis::BLOB:
    case ibis::TEXT:
    case ibis::CATEGORY: {
	if (col.defval == 0)
	    col.defval = new std::string;
	std::string &str = *(static_cast<std::string*>(col.defval));
	str.clear();
	if (val != 0 && *val != 0)
	    ibis::util::getString(str, val, 0);
	break;}
    default: {
	LOGGER(ibis::gVerbose > 1)
	    << "tafel::assignDefaultValue(" << col.name << ", " << val
	    << ") can not handle column type "
	    << ibis::TYPESTRING[(int)col.type];
	return -3;
	break;}
    } // switch (col.type)
    return 0;
} // ibis::tafel::assignDefault

/// Add values to an array of type T.  The input values (in) are copied to
/// out[be:en-1].  If the array out has less then be elements to start
/// with, it will be filled with value fill.  The output mask indicates
/// whether the values in array out are valid.  This version works with one
/// column as at a time.
/// @note It is a const function because it only makes changes to its
/// arguments.
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

    LOGGER(ibis::gVerbose > 7)
	<< "tafel::append(" << typeid(T).name()
	<< ", " << be << ", " << en << ")\ninmask: "
	<< inmsk << "totmask: " << mask;
} // ibis::tafel::append

/// Copy the incoming strings to out[be:en-1].  Work with one column at a
/// time.
/// @note It is a const function because it only makes changes to its
/// arguments.
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

    LOGGER(ibis::gVerbose > 7)
	<< "tafel::appendString(" << be << ", " << en << ")\ninmask: "
	<< inmsk << "totmask: " << mask;
} // ibis::tafel::appendString

/// Copy the incoming values to rows [begin:end) of column cn.
int ibis::tafel::append(const char* cn, uint64_t begin, uint64_t end,
			void* values) {
    ibis::bitvector::word_t be = static_cast<ibis::bitvector::word_t>(begin);
    ibis::bitvector::word_t en = static_cast<ibis::bitvector::word_t>(end);
    if (be != begin || en != end || be >= en || cn == 0 || *cn == 0 ||
	values == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::append(" << cn << ", " << begin << ", " << end
	    << ", " << values << ") can not proceed because of invalid "
	    "parameters";
	return -1;
    }

    columnList::iterator it = cols.find(cn);
    if (it == cols.end()) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::append(" << cn << ", " << begin << ", " << end
	    << ", " << values << ") can not proceed because " << cn
	    << " is not a column of this data partition";
	return -2;
    }

    if (en > mrows) mrows = en;
    column& col = *((*it).second);
    switch (col.type) {
    case ibis::BYTE:
	append(static_cast<const signed char*>(values), be, en,
	       *static_cast<array_t<signed char>*>(col.values),
	       (signed char)0x7F, col.mask);
	break;
    case ibis::UBYTE:
	append(static_cast<const unsigned char*>(values), be, en,
	       *static_cast<array_t<unsigned char>*>(col.values),
	       (unsigned char)0xFFU, col.mask);
	break;
    case ibis::SHORT:
	append(static_cast<const int16_t*>(values), be, en,
	       *static_cast<array_t<int16_t>*>(col.values),
	       (int16_t)0x7FFF, col.mask);
	break;
    case ibis::USHORT:
	append(static_cast<const uint16_t*>(values), be, en,
	       *static_cast<array_t<uint16_t>*>(col.values),
	       (uint16_t)0xFFFFU, col.mask);
	break;
    case ibis::INT:
	append(static_cast<const int32_t*>(values), be, en,
	       *static_cast<array_t<int32_t>*>(col.values),
	       (int32_t)0x7FFFFFFF, col.mask);
	break;
    case ibis::UINT:
	append(static_cast<const uint32_t*>(values), be, en,
	       *static_cast<array_t<uint32_t>*>(col.values),
	       (uint32_t)0xFFFFFFFFU, col.mask);
	break;
    case ibis::LONG:
	append<int64_t>(static_cast<const int64_t*>(values), be, en,
			*static_cast<array_t<int64_t>*>(col.values),
			0x7FFFFFFFFFFFFFFFLL, col.mask);
	break;
    case ibis::ULONG:
	append<uint64_t>(static_cast<const uint64_t*>(values), be, en,
			 *static_cast<array_t<uint64_t>*>(col.values),
			 0xFFFFFFFFFFFFFFFFULL, col.mask);
	break;
    case ibis::FLOAT:
	append(static_cast<const float*>(values), be, en,
	       *static_cast<array_t<float>*>(col.values),
	       std::numeric_limits<float>::quiet_NaN(), col.mask);
	break;
    case ibis::DOUBLE:
	append(static_cast<const double*>(values), be, en,
	       *static_cast<array_t<double>*>(col.values),
	       std::numeric_limits<double>::quiet_NaN(), col.mask);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	appendString(static_cast<const std::vector<std::string>*>(values),
		     be, en,
		     *static_cast<std::vector<std::string>*>(col.values),
		     col.mask);
	break;
    default:
	break;
    }
#if defined(_DEBUG) || defined(DEBUG)
    LOGGER(ibis::gVerbose > 6)
	<< "tafel::append(" << cn  << ", " << begin << ", " << end
	<< ", " << values << ") worked with column "
	<< static_cast<void*>((*it).second) << " with mask("
	<< it->second->mask.cnt() << " out of " << it->second->mask.size()
	<< ")";
#endif
    return 0;
} // ibis::tafel::append

void ibis::tafel::normalize() {
    if (cols.empty()) return;
    // loop one - determine the maximum values is all the columns
    bool need2nd = false;
    for (columnList::iterator it = cols.begin(); it != cols.end(); ++ it) {
	column& col = *((*it).second);
	switch (col.type) {
	case ibis::BYTE: {
	    array_t<signed char>& vals =
		* static_cast<array_t<signed char>*>(col.values);
	    if (mrows < vals.size()) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::UBYTE: {
	    array_t<unsigned char>& vals =
		* static_cast<array_t<unsigned char>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::SHORT: {
	    array_t<int16_t>& vals =
		* static_cast<array_t<int16_t>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::USHORT: {
	    array_t<uint16_t>& vals =
		* static_cast<array_t<uint16_t>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::INT: {
	    array_t<int32_t>& vals =
		* static_cast<array_t<int32_t>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::UINT: {
	    array_t<uint32_t>& vals =
		* static_cast<array_t<uint32_t>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::LONG: {
	    array_t<int64_t>& vals =
		* static_cast<array_t<int64_t>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t>& vals =
		* static_cast<array_t<uint64_t>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::FLOAT: {
	    array_t<float>& vals =
		* static_cast<array_t<float>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>& vals =
		* static_cast<array_t<double>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
	    std::vector<std::string>& vals =
		* static_cast<std::vector<std::string>*>(col.values);
	    if (vals.size() > mrows) {
		mrows = vals.size();
		need2nd = true;
	    }
	    else if (mrows > vals.size()) {
		need2nd = true;
	    }
	    break;}
	case ibis::BLOB: {
	    if (col.starts.size() > mrows+1) {
		mrows = col.starts.size()-1;
		need2nd = true;
	    }
	    else if (mrows+1 > col.starts.size()) {
		need2nd = true;
	    }
	    break;}
	default: {
	    break;}
	}
	if (col.mask.size() > mrows) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "tafel::normalize - col[" << col.name << "].mask("
		<< col.mask.cnt() << ", " << col.mask.size() << ") -- mrows = "
		<< mrows;
	    mrows = col.mask.size();
	    need2nd = true;
	}
    }
    if (! need2nd) return;

    // second loop - adjust the array sizes
    for (columnList::iterator it = cols.begin(); it != cols.end(); ++ it) {
	column& col = *((*it).second);
	switch (col.type) {
	case ibis::BYTE: {
	    array_t<signed char>& vals =
		* static_cast<array_t<signed char>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<signed char*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), 0x7F);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::UBYTE: {
	    array_t<unsigned char>& vals =
		* static_cast<array_t<unsigned char>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<unsigned char*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), 0xFFU);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::SHORT: {
	    array_t<int16_t>& vals =
		* static_cast<array_t<int16_t>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<int16_t*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), 0x7FFF);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::USHORT: {
	    array_t<uint16_t>& vals =
		* static_cast<array_t<uint16_t>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<uint16_t*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), 0xFFFFU);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::INT: {
	    array_t<int32_t>& vals =
		* static_cast<array_t<int32_t>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<int32_t*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), 0x7FFFFFFF);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::UINT: {
	    array_t<uint32_t>& vals =
		* static_cast<array_t<uint32_t>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<uint32_t*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), 0xFFFFFFFFU);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::LONG: {
	    array_t<int64_t>& vals =
		* static_cast<array_t<int64_t>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(0, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<int64_t*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				0x7FFFFFFFFFFFFFFFLL);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t>& vals =
		* static_cast<array_t<uint64_t>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<uint64_t*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				0xFFFFFFFFFFFFFFFFULL);
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::FLOAT: {
	    array_t<float>& vals =
		* static_cast<array_t<float>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<float*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				std::numeric_limits<float>::quiet_NaN());
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>& vals =
		* static_cast<array_t<double>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<double*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				std::numeric_limits<double>::quiet_NaN());
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
	    std::vector<std::string>& vals =
		* static_cast<std::vector<std::string>*>(col.values);
	    if (vals.size() < mrows) {
		if (col.defval != 0) {
		    col.mask.set(1, mrows);
		    vals.insert(vals.end(), mrows-vals.size(),
				* static_cast<std::string*>(col.defval));
		}
		else {
		    col.mask.adjustSize(vals.size(), mrows);
		    vals.insert(vals.end(), mrows-vals.size(), "");
		}
	    }
	    else if (vals.size() > mrows) {
		col.mask.adjustSize(mrows, mrows);
		vals.resize(mrows);
	    }
	    break;}
	case ibis::BLOB: {
	    if (col.starts.size() < mrows+1) {
		if (col.defval != 0) {
		    ibis::array_t<char>& bytes =
			* static_cast<array_t<char>*>(col.values);
		    const std::string &def =
			* static_cast<std::string*>(col.defval);
		    col.starts.reserve(mrows+1);
		    if (col.starts.size() <= 1) {
			col.starts.resize(1);
			col.starts[0] = 0;
		    }
		    for (size_t j = col.starts.size(); j <= mrows; ++ j) {
			col.starts.push_back(col.starts.back()+def.size());
			bytes.insert(bytes.end(), def.data(),
				     def.data()+def.size());
		    }
		}
		else if (col.starts.size() < mrows+1) {
		    col.starts.insert(col.starts.end(),
				      mrows+1-col.starts.size(),
				      col.starts.back());
		}
	    }
	    else if (col.starts.size() > mrows+1) {
		col.starts.resize(mrows+1);
		ibis::array_t<unsigned char>& bytes =
		    * static_cast<array_t<unsigned char>*>(col.values);
		if (bytes.size() > col.starts[mrows]) {
		    bytes.resize(col.starts.back());
		}
		else if (bytes.size() < col.starts[mrows]) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- tafel::normalize expects column "
			<< col.name
			<< " (type: blob) to have more than "
			<< col.starts[mrows]
			<< " bytes in col.values, but it has only "
			<< bytes.size();
		}
	    }
	    break;}
	default: {
	    break;}
	}
    }
} // ibis::tafel::normalize

/// Locate the buffers and masks associated with a data type.
template <typename T>
void ibis::tafel::locate(ibis::TYPE_T t, std::vector<array_t<T>*>& buf,
			 std::vector<ibis::bitvector*>& msk) const {
    buf.clear();
    msk.clear();
    for (uint32_t i = 0; i < colorder.size(); ++ i) {
	if (colorder[i]->type == t) {
	    buf.push_back(static_cast<array_t<T>*>(colorder[i]->values));
	    msk.push_back(&(colorder[i]->mask));
	}
    }
} // ibis::tafel::locate

/// Locate the buffers and masks associated with a string-valued data type.
void ibis::tafel::locateString(ibis::TYPE_T t,
			       std::vector<std::vector<std::string>*>& buf,
			       std::vector<ibis::bitvector*>& msk) const {
    buf.clear();
    msk.clear();
    for (uint32_t i = 0; i < colorder.size(); ++ i) {
	if (colorder[i]->type == t) {
	    buf.push_back(static_cast<std::vector<std::string>*>
			  (colorder[i]->values));
	    msk.push_back(&(colorder[i]->mask));
	}
    }
} // ibis::tafel::locateString

/// Append one row to columns of a particular type.  This version with
/// multiple columns but only one row.
///
/// @note It assumes that the existing data have been normalized, i.e., all
/// columns have the same number of rows.
template <typename T>
void ibis::tafel::append(const std::vector<std::string>& nm,
			 const std::vector<T>& va,
			 std::vector<array_t<T>*>& buf,
			 std::vector<ibis::bitvector*>& msk) {
    const uint32_t n1 = (nm.size() <= va.size() ? nm.size() : va.size());
    for (uint32_t i = 0; i < n1; ++ i) {
	if (nm[i].empty()) {
	    if (buf.size() > i && buf[i] != 0)
		buf[i]->push_back(va[i]);
	    if (msk.size() > i && msk[i] != 0)
		msk[i]->operator+=(1);
	}
	else {
	    columnList::iterator it = cols.find(nm[i].c_str());
	    if (it != cols.end()) {
		if (buf.size() < i) buf.resize(i+1);
		buf[i] = static_cast<array_t<T>*>((*it).second->values);
		buf[i]->push_back(va[i]);
		if (msk.size() < i) msk.resize(i+1);
		msk[i] = &(it->second->mask);
		*(msk[i]) += 1;
	    }
	}
    }

    const uint32_t n2 = (va.size() <= buf.size() ? va.size() : buf.size());    
    for (uint32_t i = n1; i < n2; ++ i) {
	if (buf[i] != 0)
	    buf[i]->push_back(va[i]);
	if (msk.size() > i && msk[i] != 0)
	    *(msk[i]) += 1;
    }
} // ibis::tafel::append

/// Append one row to string-valued columns.  This version with multiple
/// columns but only one row.
///
/// @note It assumes that the existing data have been normalized, i.e., all
/// columns have the same number of rows.
void ibis::tafel::appendString(const std::vector<std::string>& nm,
			       const std::vector<std::string>& va,
			       std::vector<std::vector<std::string>*>& buf,
			       std::vector<ibis::bitvector*>& msk) {
    const uint32_t n1 = (nm.size() <= va.size() ? nm.size() : va.size());
    for (uint32_t i = 0; i < n1; ++ i) {
	if (nm[i].empty()) {
	    if (buf.size() > i && buf[i] != 0)
		buf[i]->push_back(va[i]);
	    if (msk.size() > i && msk[i] != 0)
		*(msk[i]) += 1;
	}
	else {
	    columnList::iterator it = cols.find(nm[i].c_str());
	    if (it != cols.end()) {
		if (buf.size() < i) buf.resize(i+1);
		buf[i] = static_cast<std::vector<std::string>*>
		    ((*it).second->values);
		buf[i]->push_back(va[i]);
		msk[i] = &(it->second->mask);
		*(msk[i]) += 1;
	    }
	}
    }

    const uint32_t n2 = (va.size() <= buf.size() ? va.size() : buf.size());    
    for (uint32_t i = n1; i < n2; ++ i) {
	if (buf[i] != 0)
	    buf[i]->push_back(va[i]);
	if (msk.size() > i && msk[i] != 0)
	    *(msk[i]) += 1;
    }
} // ibis::tafel::appendString

int ibis::tafel::appendRow(const ibis::table::row& r) {
    int cnt = 0;
    if (r.nColumns() >= cols.size())
	normalize();

    std::vector<ibis::bitvector*> msk;
    if (r.bytesvalues.size() > 0) {
	std::vector<array_t<signed char>*> bytesptr;
	locate(ibis::BYTE, bytesptr, msk);
	cnt += r.bytesvalues.size();
	append(r.bytesnames, r.bytesvalues, bytesptr, msk);
    }
    if (r.ubytesvalues.size() > 0) {
	std::vector<array_t<unsigned char>*> ubytesptr;
	locate(ibis::UBYTE, ubytesptr, msk);
	cnt += r.ubytesvalues.size();
	append(r.ubytesnames, r.ubytesvalues, ubytesptr, msk);
    }
    if (r.shortsvalues.size() > 0) {
	std::vector<array_t<int16_t>*> shortsptr;
	locate(ibis::SHORT, shortsptr, msk);
	cnt += r.shortsvalues.size();
	append(r.shortsnames, r.shortsvalues, shortsptr, msk);
    }
    if (r.ushortsvalues.size() > 0) {
	std::vector<array_t<uint16_t>*> ushortsptr;
	locate(ibis::USHORT, ushortsptr, msk);
	cnt += r.ushortsvalues.size();
	append(r.ushortsnames, r.ushortsvalues, ushortsptr, msk);
    }
    if (r.intsvalues.size() > 0) {
	std::vector<array_t<int32_t>*> intsptr;
	locate(ibis::INT, intsptr, msk);
	cnt += r.intsvalues.size();
	append(r.intsnames, r.intsvalues, intsptr, msk);
    }
    if (r.uintsvalues.size() > 0) {
	std::vector<array_t<uint32_t>*> uintsptr;
	locate(ibis::UINT, uintsptr, msk);
	cnt += r.uintsvalues.size();
	append(r.uintsnames, r.uintsvalues, uintsptr, msk);
    }
    if (r.longsvalues.size() > 0) {
	std::vector<array_t<int64_t>*> longsptr;
	locate(ibis::LONG, longsptr, msk);
	cnt += r.longsvalues.size();
	append(r.longsnames, r.longsvalues, longsptr, msk);
    }
    if (r.ulongsvalues.size() > 0) {
	std::vector<array_t<uint64_t>*> ulongsptr;
	locate(ibis::ULONG, ulongsptr, msk);
	cnt += r.ulongsvalues.size();
	append(r.ulongsnames, r.ulongsvalues, ulongsptr, msk);
    }
    if (r.floatsvalues.size() > 0) {
	std::vector<array_t<float>*> floatsptr;
	locate(ibis::FLOAT, floatsptr, msk);
	cnt += r.floatsvalues.size();
	append(r.floatsnames, r.floatsvalues, floatsptr, msk);
    }
    if (r.doublesvalues.size() > 0) {
	std::vector<array_t<double>*> doublesptr;
	locate(ibis::DOUBLE, doublesptr, msk);
	cnt += r.doublesvalues.size();
	append(r.doublesnames, r.doublesvalues, doublesptr, msk);
    }
    if (r.catsvalues.size() > 0) {
	std::vector<std::vector<std::string>*> catsptr;
	locateString(ibis::CATEGORY, catsptr, msk);
	cnt += r.catsvalues.size();
	appendString(r.catsnames, r.catsvalues, catsptr, msk);
    }
    if (r.textsvalues.size() > 0) {
	std::vector<std::vector<std::string>*> textsptr;
	locateString(ibis::TEXT, textsptr, msk);
	cnt += r.textsvalues.size();
	appendString(r.textsnames, r.textsvalues, textsptr, msk);
    }
    mrows += ((size_t)cnt >= cols.size());
    return cnt;
} // ibis::tafel::appendRow

int ibis::tafel::appendRows(const std::vector<ibis::table::row>& rs) {
    if (rs.empty()) return 0;
    std::vector<ibis::bitvector*> bytesmsk;
    std::vector<array_t<signed char>*> bytesptr;
    locate(ibis::BYTE, bytesptr, bytesmsk);
    std::vector<ibis::bitvector*> ubytesmsk;
    std::vector<array_t<unsigned char>*> ubytesptr;
    locate(ibis::UBYTE, ubytesptr, ubytesmsk);
    std::vector<ibis::bitvector*> shortsmsk;
    std::vector<array_t<int16_t>*> shortsptr;
    locate(ibis::SHORT, shortsptr, shortsmsk);
    std::vector<ibis::bitvector*> ushortsmsk;
    std::vector<array_t<uint16_t>*> ushortsptr;
    locate(ibis::USHORT, ushortsptr, ushortsmsk);
    std::vector<ibis::bitvector*> intsmsk;
    std::vector<array_t<int32_t>*> intsptr;
    locate(ibis::INT, intsptr, intsmsk);
    std::vector<ibis::bitvector*> uintsmsk;
    std::vector<array_t<uint32_t>*> uintsptr;
    locate(ibis::UINT, uintsptr, uintsmsk);
    std::vector<ibis::bitvector*> longsmsk;
    std::vector<array_t<int64_t>*> longsptr;
    locate(ibis::LONG, longsptr, longsmsk);
    std::vector<ibis::bitvector*> ulongsmsk;
    std::vector<array_t<uint64_t>*> ulongsptr;
    locate(ibis::ULONG, ulongsptr, ulongsmsk);
    std::vector<ibis::bitvector*> floatsmsk;
    std::vector<array_t<float>*> floatsptr;
    locate(ibis::FLOAT, floatsptr, floatsmsk);
    std::vector<ibis::bitvector*> doublesmsk;
    std::vector<array_t<double>*> doublesptr;
    locate(ibis::DOUBLE, doublesptr, doublesmsk);
    std::vector<ibis::bitvector*> catsmsk;
    std::vector<std::vector<std::string>*> catsptr;
    locateString(ibis::CATEGORY, catsptr, catsmsk);
    std::vector<ibis::bitvector*> textsmsk;
    std::vector<std::vector<std::string>*> textsptr;
    locateString(ibis::TEXT, textsptr, textsmsk);

    const uint32_t ncols = cols.size();
    uint32_t cnt = 0;
    int jnew = 0;
    for (uint32_t i = 0; i < rs.size(); ++ i) {
	if (cnt < ncols)
	    normalize();

	cnt = 0;
	if (rs[i].bytesvalues.size() > 0) {
	    cnt += rs[i].bytesvalues.size();
	    append(rs[i].bytesnames, rs[i].bytesvalues, bytesptr, bytesmsk);
	}
	if (rs[i].ubytesvalues.size() > 0) {
	    cnt += rs[i].ubytesvalues.size();
	    append(rs[i].ubytesnames, rs[i].ubytesvalues, ubytesptr, ubytesmsk);
	}
	if (rs[i].shortsvalues.size() > 0) {
	    cnt += rs[i].shortsvalues.size();
	    append(rs[i].shortsnames, rs[i].shortsvalues, shortsptr, shortsmsk);
	}
	if (rs[i].ushortsvalues.size() > 0) {
	    cnt += rs[i].ushortsvalues.size();
	    append(rs[i].ushortsnames, rs[i].ushortsvalues, ushortsptr,
		   ushortsmsk);
	}
	if (rs[i].intsvalues.size() > 0) {
	    cnt += rs[i].intsvalues.size();
	    append(rs[i].intsnames, rs[i].intsvalues, intsptr, intsmsk);
	}
	if (rs[i].uintsvalues.size() > 0) {
	    cnt += rs[i].uintsvalues.size();
	    append(rs[i].uintsnames, rs[i].uintsvalues, uintsptr, uintsmsk);
	}
	if (rs[i].longsvalues.size() > 0) {
	    cnt += rs[i].longsvalues.size();
	    append(rs[i].longsnames, rs[i].longsvalues, longsptr, longsmsk);
	}
	if (rs[i].ulongsvalues.size() > 0) {
	    cnt += rs[i].ulongsvalues.size();
	    append(rs[i].ulongsnames, rs[i].ulongsvalues, ulongsptr, ulongsmsk);
	}
	if (rs[i].floatsvalues.size() > 0) {
	    cnt += rs[i].floatsvalues.size();
	    append(rs[i].floatsnames, rs[i].floatsvalues, floatsptr, floatsmsk);
	}
	if (rs[i].doublesvalues.size() > 0) {
	    cnt += rs[i].doublesvalues.size();
	    append(rs[i].doublesnames, rs[i].doublesvalues, doublesptr,
		   doublesmsk);
	}
	if (rs[i].catsvalues.size() > 0) {
	    cnt += rs[i].catsvalues.size();
	    appendString(rs[i].catsnames, rs[i].catsvalues, catsptr, catsmsk);
	}
	if (rs[i].textsvalues.size() > 0) {
	    cnt += rs[i].textsvalues.size();
	    appendString(rs[i].textsnames, rs[i].textsvalues, textsptr,
			 textsmsk);
	}
	if (cnt > 0) {
	    ++ mrows;
	    ++ jnew;
	}
	
    }
    return jnew;
} // ibis::tafel::appendRows

/// Write the metadata file if no metadata file already exists in the given
/// directory.
/// Return error code:
/// - number of columns: successful completion.  The return value of this
///   function should match the return of mColumns.
/// -  0: a metadata file already exists.  The content of the existing
///    metadata file is not checked.
/// - -1: no directory specified.
/// - -3: unable to open the metadata file.
int ibis::tafel::writeMetaData(const char* dir, const char* tname,
			       const char* tdesc, const char* idx) const {
    if (cols.empty()) return 0; // nothing new to write
    if (dir == 0 || *dir == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- tafel::writeMetaData needs a valid output directory";
	return -1; // dir must be specified
    }
    std::string mdfile = dir;
    mdfile += FASTBIT_DIRSEP;
    mdfile += "-part.txt";
    if (ibis::util::getFileSize(mdfile.c_str()) > 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "tafel::writeMetaData detects an existing -part.txt in " << dir
	    << ", retun now";
	return 0;
    }
    ibis::horometer timer;
    if (ibis::gVerbose > 0)
	timer.start();

    std::string oldnm, olddesc;
    time_t currtime = time(0); // current time
    char stamp[28];
    ibis::util::secondsToString(currtime, stamp);
    if (tdesc == 0 || *tdesc == 0) { // generate a description
	std::ostringstream oss;
	oss << "Metadata written with ibis::tablex::writeMetaData on "
	    << stamp << " with " << cols.size() << " column"
	    << (cols.size() > 1 ? "s" : "");
	olddesc = oss.str();
	tdesc = olddesc.c_str();
    }
    if (tname == 0 || *tname == 0) { // use the directory name as table name
	tname = strrchr(dir, FASTBIT_DIRSEP);
	if (tname == 0)
	    tname = strrchr(dir, '/');
	if (tname != 0) {
	    if (tname[1] != 0) {
		++ tname;
	    }
	    else { // dir ends with FASTBIT_DIRSEP
		oldnm = dir;
		oldnm.erase(oldnm.size()-1); // remove the last FASTBIT_DIRSEP
		uint32_t j = 1 + oldnm.rfind(FASTBIT_DIRSEP);
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
    LOGGER(ibis::gVerbose > 1)
	<< "tafel::writeMetaData starting to write " << cols.size()
	<< " column" << (cols.size()>1?"s":"") << " to " << dir << " as "
	<< " data partition " << tname;

    std::ofstream md(mdfile.c_str());
    if (! md) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeMetaData(" << dir
	    << ") failed to open metadata file \"-part.txt\"";
	return -3; // metadata file not ready
    }

    md << "# meta data for data partition " << tname
       << " written by ibis::tafel::writeMetaData on " << stamp << "\n\n"
       << "BEGIN HEADER\nName = " << tname << "\nDescription = "
       << tdesc << "\nNumber_of_rows = 0"
       << "\nNumber_of_columns = " << cols.size()
       << "\nTimestamp = " << currtime;
    if (idx != 0 && *idx != 0) {
	md << "\nindex = " << idx;
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

    for (columnList::const_iterator it = cols.begin();
	 it != cols.end(); ++ it) {
	const column& col = *((*it).second);
	md << "\nBegin Column\nname = " << (*it).first << "\ndata_type = "
	   << ibis::TYPESTRING[(int) col.type];
	if (!col.desc.empty())
	    md << "\ndescription = " << col.desc;
	if (! col.indexSpec.empty()) {
	    md << "\nindex = " << col.indexSpec;
	}
	else if (col.type == ibis::TEXT) {
	    md << "\nindex = none";
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
	ibis::util::logger().buffer()
	    << "tafel::writeMetaData completed writing partition " 
	    << tname << " (" << tdesc << ") with " << cols.size()
	    << " column" << (cols.size()>1 ? "s" : "") << " to " << dir
	    << " using " << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed)";
    }

    return cols.size();
} // ibis::tafel::writeMetaData

/// Write the data values and update the metadata file.
/// Return error code:
/// -  0: successful completion.
/// - -1: no directory specified.
/// - -2: column type conflicts.
/// - -3: unable to open the metadata file.
/// - -4: unable to open a data file.
/// - -5: failed to write the expected number of records.
int ibis::tafel::write(const char* dir, const char* tname,
		       const char* tdesc, const char* idx) const {
    if (cols.empty() || mrows == 0) return 0; // nothing new to write
    if (dir == 0 || *dir == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- tafel::write needs a valid output directory name";
	return -1; // dir must be specified
    }
    ibis::horometer timer;
    if (ibis::gVerbose > 0)
	timer.start();

    std::string oldnm, olddesc, oldidx;
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

	    if (tmp.indexSpec() != 0 && *(tmp.indexSpec()) != 0)
		oldidx = tmp.indexSpec();
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
		    LOGGER(ibis::gVerbose >= 0)
			<< "tafel::write(" << dir
			<< ") column " << (*it).first
			<< " has conflicting types specified, "
			"previously " << ibis::TYPESTRING[(int)old->type()]
			<< ", currently "
			<< ibis::TYPESTRING[(int)col.type];
		}
	    }
	    if (nconflicts > 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "tafel::write(" << dir
		    << ") can not proceed because " << nconflicts
		    << " column" << (nconflicts>1 ? "s" : "")
		    << " contains conflicting type specifications";
		return -2;
	    }
	    else {
		LOGGER(ibis::gVerbose > 2) 
		    << "tafel::write(" << dir
		    << ") found existing data partition named "
		    << tmp.name() << " with " << tmp.nRows()
		    << " row" << (tmp.nRows()>1 ? "s" : "")
		    << " and " << tmp.nColumns() << " column"
		    << (tmp.nColumns()>1?"s":"")
		    << ", will append " << mrows << " new row"
		    << (mrows>1 ? "s" : "");
	    }
	}
    }

    time_t currtime = time(0); // current time
    char stamp[28];
    ibis::util::secondsToString(currtime, stamp);
    if (tdesc == 0 || *tdesc == 0) { // generate a description
	std::ostringstream oss;
	oss << "Data initialy wrote with ibis::tablex interface on "
	    << stamp << " with " << cols.size() << " column"
	    << (cols.size() > 1 ? "s" : "") << " and " << nold + mrows
	    << " row" << (nold+mrows>1 ? "s" : "");
	olddesc = oss.str();
	tdesc = olddesc.c_str();
    }
    if (tname == 0 || *tname == 0) { // use the directory name as table name
	tname = strrchr(dir, FASTBIT_DIRSEP);
	if (tname == 0)
	    tname = strrchr(dir, '/');
	if (tname != 0) {
	    if (tname[1] != 0) {
		++ tname;
	    }
	    else { // dir ends with FASTBIT_DIRSEP
		oldnm = dir;
		oldnm.erase(oldnm.size()-1); // remove the last FASTBIT_DIRSEP
		uint32_t j = 1 + oldnm.rfind(FASTBIT_DIRSEP);
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
    LOGGER(ibis::gVerbose > 1)
	<< "tafel::write starting to write " << mrows << " row"
	<< (mrows>1?"s":"") << " and " << cols.size() << " column"
	<< (cols.size()>1?"s":"") << " to " << dir << " as data partition "
	<< tname;

    std::string mdfile = dir;
    mdfile += FASTBIT_DIRSEP;
    mdfile += "-part.txt";
    std::ofstream md(mdfile.c_str());
    if (! md) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::write(" << dir << ") failed to open metadata file "
	    "\"-part.txt\"";
	return -3; // metadata file not ready
    }

    md << "# meta data for data partition " << tname
       << " written by ibis::tafel::write on " << stamp << "\n\n"
       << "BEGIN HEADER\nName = " << tname << "\nDescription = "
       << tdesc << "\nNumber_of_rows = " << nold+mrows
       << "\nNumber_of_columns = " << cols.size()
       << "\nTimestamp = " << currtime;
    if (idx != 0 && *idx != 0) {
	md << "\nindex = " << idx;
    }
    else if (! oldidx.empty()) {
	md << "\nindex = " << oldidx;
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

    int ierr = 0;
    for (columnList::const_iterator it = cols.begin();
	 it != cols.end(); ++ it) {
	const column& col = *((*it).second);
	std::string cnm = dir;
	cnm += FASTBIT_DIRSEP;
	cnm += (*it).first;
	int fdes = UnixOpen(cnm.c_str(), OPEN_WRITEADD, OPEN_FILEMODE);
	if (fdes < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "tafel::write(" << dir << ") failed to open file "
		<< cnm << " for writing";
	    return -4;
	}
	ibis::util::guard gfdes = ibis::util::makeGuard(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdes, _O_BINARY);
#endif
	LOGGER(ibis::gVerbose > 2)
	    << "tafel::write opened file " << cnm
	    << " to write data for column " << (*it).first;
	std::string mskfile = cnm; // mask file name
	mskfile += ".msk";
	ibis::bitvector msk(mskfile.c_str());

	switch (col.type) {
	case ibis::BYTE:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<signed char>*>(col.values),
		     * static_cast<const signed char*>(col.defval),
		     msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<signed char>*>(col.values),
		     (signed char)0x7F, msk, col.mask);
	    }
	    break;
	case ibis::UBYTE:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<unsigned char>*>(col.values),
		     *static_cast<const unsigned char*>(col.defval),
		     msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<unsigned char>*>(col.values),
		     (unsigned char)0xFF, msk, col.mask);
	    }
	    break;
	case ibis::SHORT:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<int16_t>*>(col.values),
		     * static_cast<const int16_t*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<int16_t>*>(col.values),
		     (int16_t)0x7FFF, msk, col.mask);
	    }
	    break;
	case ibis::USHORT:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<uint16_t>*>(col.values),
		     * static_cast<const uint16_t*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<uint16_t>*>(col.values),
		     (uint16_t)0xFFFF, msk, col.mask);
	    }
	    break;
	case ibis::INT:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<int32_t>*>(col.values),
		     * static_cast<const int32_t*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<int32_t>*>(col.values),
		     (int32_t)0x7FFFFFFF, msk, col.mask);
	    }
	    break;
	case ibis::UINT:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<uint32_t>*>(col.values),
		     * static_cast<const uint32_t*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<uint32_t>*>(col.values),
		     (uint32_t)0xFFFFFFFF, msk, col.mask);
	    }
	    break;
	case ibis::LONG:
	    if (col.defval != 0) {
		ibis::bitvector tmp;
		tmp.set(1, mrows);
		ierr = writeColumn<int64_t>
		    (fdes, nold, mrows,
		     * static_cast<const array_t<int64_t>*>(col.values),
		     * static_cast<const int64_t*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn<int64_t>
		    (fdes, nold, mrows,
		     * static_cast<const array_t<int64_t>*>(col.values),
		     0x7FFFFFFFFFFFFFFFLL, msk, col.mask);
	    }
	    break;
	case ibis::ULONG:
	    if (col.defval != 0) {
		ierr = writeColumn<uint64_t>
		    (fdes, nold, mrows,
		     * static_cast<const array_t<uint64_t>*>(col.values),
		     * static_cast<const uint64_t*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn<uint64_t>
		    (fdes, nold, mrows, * static_cast<const array_t<uint64_t>*>
		     (col.values), 0xFFFFFFFFFFFFFFFFULL, msk, col.mask);
	    }
	    break;
	case ibis::FLOAT:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<float>*>(col.values),
		     * static_cast<const float*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<float>*>(col.values),
		     std::numeric_limits<float>::quiet_NaN(), msk, col.mask);
	    }
	    break;
	case ibis::DOUBLE:
	    if (col.defval != 0) {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<double>*>(col.values), 
		     * static_cast<const double*>(col.defval), msk, col.mask);
	    }
	    else {
		ierr = writeColumn
		    (fdes, nold, mrows,
		     * static_cast<const array_t<double>*>(col.values), 
		     std::numeric_limits<double>::quiet_NaN(), msk, col.mask);
	    }
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    if (col.defval != 0) {
		ierr = writeString
		    (fdes, nold, mrows,
		     * static_cast<const std::vector<std::string>*>
		     (col.values), msk, col.mask);
	    }
	    else {
		ierr = writeString
		    (fdes, nold, mrows,
		     * static_cast<const std::vector<std::string>*>
		     (col.values), msk, col.mask);
	    }
	    break;
	case ibis::BLOB: {
	    std::string spname = cnm;
	    spname += ".sp";
	    int sdes = UnixOpen(spname.c_str(), OPEN_READWRITE, OPEN_FILEMODE);
	    if (sdes < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "tafel::write(" << dir << ") failed to open file "
		    << spname << " for writing the starting positions";
		return -4;
	    }
	    ibis::util::guard gsdes = ibis::util::makeGuard(UnixClose, sdes);
#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(sdes, _O_BINARY);
#endif

	    ierr = writeRaw
		(fdes, sdes, nold, mrows,
		 * static_cast<const array_t<unsigned char>*>(col.values),
		 col.starts, msk, col.mask);
	    break;}
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
		<< "tafel::write(" << dir << ") failed to write column "
		<< (*it).first << " (type " << ibis::TYPESTRING[(int)col.type]
		<< ") to " << cnm;
	    return ierr;
	}

	if (col.defval != 0) { // 
	    if (msk.size() == mrows) {
		msk.set(1, mrows);
	    }
	    else {
		msk.adjustSize(0, nold);
		msk.adjustSize(nold+mrows, nold+mrows);
	    }
	}
	if (msk.cnt() != msk.size()) {
	    msk.write(mskfile.c_str());
	}
	else { // remove the mask file
	    remove(mskfile.c_str());
	}
	ibis::fileManager::instance().flushFile(mskfile.c_str());

	md << "\nBegin Column\nname = " << (*it).first << "\ndata_type = "
	   << ibis::TYPESTRING[(int) col.type];
	if (! col.indexSpec.empty()) {
	    md << "\nindex = " << col.indexSpec;
	}
	else if (col.type == ibis::TEXT) {
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
	ibis::util::logger().buffer()
	    << "tafel::write completed writing partition " 
	    << tname << " (" << tdesc << ") with "
	    << cols.size() << " column" << (cols.size()>1 ? "s" : "")
	    << " and " << mrows << " row" << (mrows>1 ? "s" : "")
	    << " (total " << nold+mrows << ") to " << dir
	    << " using " << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed)";
    }

    return 0;
} // ibis::tafel::write

/// Write the content of vals to an open file.  This template function
/// works with fixed size elements stored in array_t.
template <typename T>
int ibis::tafel::writeColumn(int fdes, ibis::bitvector::word_t nold,
			     ibis::bitvector::word_t nnew,
			     const array_t<T>& vals, const T& fill,
			     ibis::bitvector& totmask,
			     const ibis::bitvector& newmask) const {
    const uint32_t elem = sizeof(T);
    off_t pos = UnixSeek(fdes, 0, SEEK_END);
    if (pos < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeColumn<" << typeid(T).name() << ">(" << fdes
	    << ", " << nold << ", " << nnew << " ...) failed to seek to "
	    "the end of the file";
	return -3; // failed to find the EOF position
    }
    if ((uint32_t) pos < nold*elem) {
	const uint32_t n1 = (uint32_t)pos / elem;
	totmask.adjustSize(n1, nold);
	for (uint32_t j = n1; j < nold; ++ j)
	    UnixWrite(fdes, &fill, elem);
    }
    else if ((uint32_t) pos > nold*elem) {
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
	for (uint32_t j = vals.size(); j < nnew; ++ j)
	    pos += UnixWrite(fdes, &fill, elem);
	totmask += newmask;
    }
    totmask.adjustSize(totmask.size(), nnew+nold);
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg;
	lg.buffer() << "tafel::writeColumn wrote " << pos << " bytes of "
		    << typeid(T).name() << " for " << nnew << " elements\n";
	if (ibis::gVerbose > 6) {
	    if (ibis::gVerbose > 7)
		lg.buffer() << "mask for new records: " << newmask << "\n";
	    lg.buffer() << "Overall bit mask: "<< totmask;
	}
    }
    return (-5 * ((uint32_t) pos != nnew*elem));
} // ibis::tafel::writeColumn

/// Write strings to an open file.  The strings are stored in a
/// std::vector<std::string>.  The strings are null-terminated and
/// therefore can not contain null characters in them.
int ibis::tafel::writeString(int fdes, ibis::bitvector::word_t nold,
			     ibis::bitvector::word_t nnew,
			     const std::vector<std::string>& vals,
			     ibis::bitvector& totmask,
			     const ibis::bitvector& newmask) const {
    off_t pos = UnixSeek(fdes, 0, SEEK_END);
    if (pos < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeString(" << fdes << ", " << nold << ", " << nnew
	    << " ...) failed to seek to the end of the file";
	return -3; // failed to find the EOF position
    }

    pos = 0;
    totmask.adjustSize(nold, nold);
    if (vals.size() >= nnew) {
	for (uint32_t j = 0; j < nnew; ++ j)
	    pos += (0 < UnixWrite(fdes, vals[j].c_str(), vals[j].size()+1));
    }
    else {
	for (uint32_t j = 0; j < vals.size(); ++ j)
	    pos += (0 < UnixWrite(fdes, vals[j].c_str(), vals[j].size()+1));
	char buf[MAX_LINE];
	memset(buf, 0, MAX_LINE);
	for (uint32_t j = vals.size(); j < nnew; j += MAX_LINE)
	    pos += UnixWrite(fdes, buf, (j+MAX_LINE<=nnew?MAX_LINE:nnew-j));
    }

    totmask += newmask;
    totmask.adjustSize(totmask.size(), nnew+nold);
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg;
	lg.buffer() << "tafel::writeString wrote " << pos
		    << " strings (" << nnew << " expected)\n";
#if DEBUG+0>0
	lg.buffer() << "vals[" << vals.size() << "]:\n"
	for (uint32_t j = 0; j < (nnew <= vals.size() ? nnew : vals.size());
	     ++ j)
	    lg.buffer() << "  " << j << "\t" << vals[j] << "\n";
#endif
	if (ibis::gVerbose > 6) {
	    if (ibis::gVerbose > 7)
		lg.buffer() << "mask for new records: " << newmask << "\n";
	    lg.buffer() << "Overall bit mask: " << totmask;
	}
    }
    return (-5 * ((uint32_t) pos != nnew));
} // ibis::tafel::writeString

/// Write raw bytes to an open file.  It also requires a second file to
/// store starting positions of the raw binary objects.
int ibis::tafel::writeRaw(int bdes, int sdes,
			  ibis::bitvector::word_t nold,
			  ibis::bitvector::word_t nnew,
			  const ibis::array_t<unsigned char>& bytes,
			  const ibis::array_t<int64_t>& starts,
			  ibis::bitvector& totmask,
			  const ibis::bitvector& newmask) const {
    off_t ierr;
    const uint32_t selem = sizeof(int64_t);
    int64_t bpos = UnixSeek(bdes, 0, SEEK_END);
    if (bpos < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeRaw(" << bdes << ", " << sdes << ", " << nold
	    << ", " << nnew << " ...) failed to seek to the end of file "
	    << bdes << ", seek returned " << bpos;
	return -3; // failed to find the EOF position
    }
    off_t spos = UnixSeek(sdes, 0, SEEK_END);
    if (spos < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeRaw(" << bdes << ", " << sdes << ", " << nold
	    << ", " << nnew << "...) failed to the end of file " << sdes
	    << ", seek returned " << spos;
	return -4;
    }
    if (spos % selem != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeRaw expects the file for starting posistion to "
	    "have a multiple of " << selem << " bytes, but it is " << spos;
	return -5;
    }
    if (spos == (int64_t)selem) {
	spos = 0;
	ierr = UnixSeek(sdes, 0, SEEK_SET);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to seek to the beginning of file "
		<< sdes << " for starting positions, seek returned " << ierr;
	    return -6;
	}
    }

    int64_t stmp;
    if (spos > 0) {
	ierr = UnixSeek(sdes, spos-selem, SEEK_SET);
	if (ierr != spos-selem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to seek to " << spos-selem
		<< " in file " << sdes << " for starting positions, "
		"seek returned" << ierr;
	    return -7;
	}
	ierr = UnixRead(sdes, &stmp, selem);
	if (ierr < (off_t)selem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to read the last " << selem
		<< " bytes from file " << sdes << " for starting positions, "
		"read returned " << ierr;
	    return -8;
	}
	if (stmp != bpos) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw expects the last value in file " << sdes
		<< "(which is " << stmp << ") to match the size of file "
		<< bdes << " (which is " << bpos << "), but they do NOT";
	    return -9;
	}
    }

    const ibis::bitvector::word_t nold1 =
	(spos > selem ? (spos / selem - 1) : 0);
    if (nold1 == 0) { // need to write the 1st number which is always 0
	bpos = 0;
	ierr = UnixWrite(sdes, &bpos, selem);
	if (ierr < (off_t)selem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to write " << bpos
		<< " to file " << sdes << ", write returned " << ierr;
	    return -10;
	}
    }
    if (nold1 < nold) {
	// existing data file does not have enough elements, add empty ones
	// to fill them
	for (size_t j = spos/selem; j <= nold; ++ j) {
	    ierr = UnixWrite(sdes, &bpos, selem);
	    if (ierr < (off_t)selem) {
		LOGGER(ibis::gVerbose > 0)
		    << "tafel::writeRaw failed to write " << bpos
		    << " to the end of file " << sdes << ", write returned "
		    << ierr;
		return -11;
	    }
	}
    }
    else if (nold1 > nold) {
	// existing files have too many elements
	spos = nold*selem;
	ierr = UnixSeek(sdes, spos, SEEK_SET);
	if (ierr != spos) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to seek to " << spos << " in file "
		<< sdes << " for starting positions, seek returned " << ierr;
	    return -12;
	}
	ierr = UnixRead(sdes, &bpos, selem);
	if (ierr < (off_t)selem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to read " << selem << " bytes from "
		<< spos << " of file " << sdes << " for starting positions, "
		" read returned " << ierr;
	    return -13;
	}
	ierr = UnixSeek(bdes, bpos, SEEK_SET);
	if (ierr != bpos) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to seek to " << bpos << " in file "
		<< bpos << " for binary objects, seek returned " << ierr;
	    return -14;
	}
    }

    ibis::bitvector::word_t nnew1 = (starts.size() <= nnew+1 ?
				     (starts.size()>1 ? starts.size()-1 : 0)
				     : nnew);
    for (bitvector::word_t j = 0; j < nnew1; ++ j) {
	bpos += starts[j+1] - starts[j];
	ierr = UnixWrite(sdes, &bpos, selem);
	if (ierr < (int64_t)selem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::writeRaw failed to write " << bpos << " to file "
		<< sdes << " for starting positioins, write returned " << ierr;
	    return -15;
	}
    }
    stmp = starts[nnew1] - starts[0];
    ierr = UnixWrite(bdes, bytes.begin(), stmp);
    if (ierr != stmp) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::writeRaw expects to write " << stmp << " byte"
	    << (stmp>1 ? "s" : "") << ", but wrote " << ierr << " instead";
	return -16;
    }

    totmask.adjustSize(nold1, nold);
    totmask += newmask;
    totmask.adjustSize(totmask.size(), nnew1+nold);
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg;
	lg.buffer() << "tafel::writeRaw wrote " << nnew1 << " binary object"
		    << (nnew1>1?"s":"") << " (" << nnew << " expected)\n";
	if (ibis::gVerbose > 6) {
	    if (ibis::gVerbose > 7)
		lg.buffer() << "mask for new records: " << newmask << "\n";
	    lg.buffer() << "Overall bit mask: " << totmask;
	}
    }
    return (-17 * (nnew1 != nnew));
} // ibis::tafel::writeRaw

void ibis::tafel::clearData() {
    mrows = 0;
    for (columnList::iterator it = cols.begin(); it != cols.end(); ++ it) {
	column& col = *((*it).second);
	col.mask.clear();
	switch (col.type) {
	case ibis::BLOB:
	    static_cast<array_t<unsigned char>*>(col.values)->clear();
	    col.starts.clear();
	    break;
	case ibis::BYTE:
	    static_cast<array_t<signed char>*>(col.values)->clear();
	    break;
	case ibis::UBYTE:
	    static_cast<array_t<unsigned char>*>(col.values)->clear();
	    break;
	case ibis::SHORT:
	    static_cast<array_t<int16_t>*>(col.values)->clear();
	    break;
	case ibis::USHORT:
	    static_cast<array_t<uint16_t>*>(col.values)->clear();
	    break;
	case ibis::INT:
	    static_cast<array_t<int32_t>*>(col.values)->clear();
	    break;
	case ibis::UINT:
	    static_cast<array_t<uint32_t>*>(col.values)->clear();
	    break;
	case ibis::LONG:
	    static_cast<array_t<int64_t>*>(col.values)->clear();
	    break;
	case ibis::ULONG:
	    static_cast<array_t<uint64_t>*>(col.values)->clear();
	    break;
	case ibis::FLOAT:
	    static_cast<array_t<float>*>(col.values)->clear();
	    break;
	case ibis::DOUBLE:
	    static_cast<array_t<double>*>(col.values)->clear();
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    static_cast<std::vector<std::string>*>(col.values)->clear();
	    break;
	default:
	    break;
	} // switch
    } // for
} // ibis::tafel::clearData

/// Attempt to reserve enough memory for maxr rows to be stored in memory.
/// This function will not reserve space for more than 1 billion rows.  If
/// maxr is less than mrows, it will simply return mrows.  It calls
/// doReserve to performs the actual reservations.  If doReserve throws an
/// exception, it will reduce the value of maxr and try again.  It will
/// give up after 5 tries and return -1, otherwise, it returns the actual
/// capacity allocated.
///
/// @note
/// If the caller does not store more rows than can be held in memory, the
/// underlying data structure should automatically expand to accomodate the
/// new rows.  However, it is definitely advantages in reserving space
/// ahead of time.  It will reduce the need to expand the underlying
/// storage objects, which can reduce the execution time.  In addition,
/// reserving a good fraction of the physical memory, say 10 - 40%, for
/// storing rows in memory can reduce the number of times the write
/// operation is invoked when loading a large number of rows from external
/// sources.  Since the string values are stored as std::vector objects,
/// additional memory is allocated for each new string added to memory,
/// therefore, after importing many long strings, it is still possible to
/// run out of memory even after one successfully reserved space with this
/// function.
///
/// @note It is possible for the existing content to be lost if doReserve
/// throws an exception, therefore, one should call this function when
/// this object does not hold any user data in memory.
int32_t ibis::tafel::reserveSpace(uint32_t maxr) {
    if (cols.empty()) return maxr;
    if (mrows >= maxr) return mrows;
    if (maxr > 0x40000000) maxr = 0x40000000;

    int32_t ret = 0;
    try {
	ret = doReserve(maxr);
    }
    catch (...) {
	if (mrows > 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "tafel::reserveSpace(" << maxr << ") failed while mrows="
		<< mrows << ", existing content has been lost";
	    mrows = 0;
	    return -2;
	}

	maxr >>= 1;
	try {
	    ret = doReserve(maxr);
	}
	catch (...) {
	    maxr >>= 2;
	    try {
		ret = doReserve(maxr);
	    }
	    catch (...) {
		maxr >>= 2;
		try {
		    ret = doReserve(maxr);
		}
		catch (...) {
		    maxr >>= 2;
		    try {
			ret = doReserve(maxr);
		    }
		    catch (...) {
			LOGGER(ibis::gVerbose >= 0)
			    << "tafel::reserveSpace(" << maxr
			    << ") failed after 5 tries";
			ret = -1;
		    }
		}
	    }
	}
    }
    return ret;
} // ibis::tafel::reserveSpace

/// Reserve space for maxr records in memory.  This function does not
/// perform error checking.  The public version of it reserveSpace does.
int32_t ibis::tafel::doReserve(uint32_t maxr) {
    if (mrows >= maxr)
	return mrows;

    int32_t ret = 0x7FFFFFFF;
    for (columnList::iterator it = cols.begin(); it != cols.end(); ++ it) {
	column& col = *((*it).second);
	col.mask.clear();
	switch (col.type) {
	case ibis::BYTE: {
	    array_t<signed char>* tmp = 
		static_cast<array_t<signed char>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<signed char>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t)curr) {
		ret = curr;
	    }
	    break;}
        case ibis::UBYTE: {
	    array_t<unsigned char>* tmp = 
		static_cast<array_t<unsigned char>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<unsigned char>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::SHORT: {
	    array_t<int16_t>* tmp = 
		static_cast<array_t<int16_t>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<int16_t>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::USHORT: {
	    array_t<uint16_t>* tmp = 
		static_cast<array_t<uint16_t>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<uint16_t>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::INT: {
	    array_t<int32_t>* tmp = 
		static_cast<array_t<int32_t>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<int32_t>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::UINT: {
	    array_t<uint32_t>* tmp = 
		static_cast<array_t<uint32_t>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<uint32_t>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::LONG: {
	    array_t<int64_t>* tmp = 
		static_cast<array_t<int64_t>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<int64_t>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t>* tmp = 
		static_cast<array_t<uint64_t>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<uint64_t>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* tmp = 
		static_cast<array_t<float>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<float>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* tmp = 
		static_cast<array_t<double>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new array_t<double>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::TEXT:
	case ibis::CATEGORY: {
	    std::vector<std::string>* tmp = 
		static_cast<std::vector<std::string>*>(col.values);
	    const uint32_t curr = tmp->capacity();
	    if (mrows == 0 && curr > (maxr >> 1)*3) {
		delete tmp;
		tmp = new std::vector<std::string>(maxr);
		col.values = tmp;
		ret = maxr;
	    }
	    else if (curr < maxr) {
		tmp->reserve(maxr);
		ret = maxr;
	    }
	    else if (ret > (int32_t) curr) {
		ret = curr;
	    }
	    break;}
	case ibis::BLOB: {
	    ibis::array_t<unsigned char>* tmp =
		static_cast<ibis::array_t<unsigned char>*>(col.values);
	    col.starts.reserve(maxr);
	    tmp->reserve(maxr);
	    ret = maxr;
	    break;}
	default:
	    break;
	} // switch
    } // for
    LOGGER(ibis::gVerbose > 1)
        << "tafel::doReserve(" << maxr << ") completed with actual capacity "
        << ret;
    return ret;
} // ibis::tafel::doReserve

uint32_t ibis::tafel::capacity() const {
    if (cols.empty()) return 0U;
    uint32_t cap = 0xFFFFFFFF;
    for (columnList::const_iterator it = cols.begin();
	 it != cols.end(); ++ it) {
	column& col = *((*it).second);
	if (col.values == 0) {
	    col.mask.clear();
	    return 0U;
	}

	uint32_t tmp;
	switch (col.type) {
	case ibis::BYTE:
	    tmp = static_cast<array_t<signed char>*>(col.values)->capacity();
	    break;
	case ibis::UBYTE:
	    tmp = static_cast<array_t<unsigned char>*>(col.values)->capacity();
	    break;
	case ibis::SHORT:
	    tmp = static_cast<array_t<int16_t>*>(col.values)->capacity();
	    break;
	case ibis::USHORT:
	    tmp = static_cast<array_t<uint16_t>*>(col.values)->capacity();
	    break;
	case ibis::INT:
	    tmp = static_cast<array_t<int32_t>*>(col.values)->capacity();
	    break;
	case ibis::UINT:
	    tmp = static_cast<array_t<uint32_t>*>(col.values)->capacity();
	    break;
	case ibis::LONG:
	    tmp = static_cast<array_t<int64_t>*>(col.values)->capacity();
	    break;
	case ibis::ULONG:
	    tmp = static_cast<array_t<uint64_t>*>(col.values)->capacity();
	    break;
	case ibis::FLOAT:
	    tmp = static_cast<array_t<float>*>(col.values)->capacity();
	    break;
	case ibis::DOUBLE:
	    tmp = static_cast<array_t<double>*>(col.values)->capacity();
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    tmp =
		static_cast<std::vector<std::string>*>(col.values)->capacity();
	    break;
	default:
	    break;
	} // switch

	if (tmp < cap)
	    cap = tmp;
	if (tmp == 0U)
	    return tmp;
    } // for
    return cap;
} // ibis::tafel::capacity

/// Compute the number of rows that are likely to fit in available memory.
/// It only count string valued column to cost 16 bytes for each row.  This
/// can be a significant underestimate of the actual cost.  Memory
/// fragmentation may also significantly reduce the available space.
uint32_t ibis::tafel::preferredSize() const {
    long unsigned width = 0;
    for (columnList::const_iterator it = cols.begin(); it != cols.end();
	 ++ it) {
	const column& col = *((*it).second);
	switch (col.type) {
	case ibis::BYTE:
	case ibis::UBYTE:
	    ++ width;
	    break;
	case ibis::SHORT:
	case ibis::USHORT:
	    width += 2;
	    break;
	case ibis::INT:
	case ibis::UINT:
	case ibis::FLOAT:
	    width += 4;
	    break;
	case ibis::LONG:
	case ibis::ULONG:
	case ibis::DOUBLE:
	    width += 8;
	    break;
	default:
	    width += 16;
	    break;
	} // switch
    } // for
    if (width == 0) width = 1024;
    width = ibis::fileManager::bytesFree() / width;
    width = static_cast<long unsigned>(ibis::util::coarsen(0.45*width, 1));
    if (width > 100000000)
	width = 100000000;
    return width;
} // ibis::tafel::preferredSize

void ibis::tafel::clear() {
    const uint32_t ncol = colorder.size();
    LOGGER(ibis::gVerbose > 2)
	<< "clearing content of ibis::tafel " << (void*)this;
    for (uint32_t i = 0; i < ncol; ++ i)
	delete colorder[i];
    colorder.clear();
    cols.clear();
    mrows = 0;
} // ibis::tafel::clear

/// Digest a line of text and place the values identified into the
/// corresponding columns.
int ibis::tafel::parseLine(const char* str, const char* del, const char* id) {
    int cnt = 0;
    int ierr;
    int64_t itmp;
    double dtmp;
    std::string stmp;
    const uint32_t ncol = colorder.size();
    for (uint32_t i = 0; i < ncol; ++ i) {
	column& col = *(colorder[i]);
	switch (col.type) {
	case ibis::BYTE: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		signed char tmp = static_cast<signed char>(itmp);
		if (tmp != itmp) {
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine column " << i+1
		    << " in " << id << " can not be parsed "
		    "correctly as an integer";
		continue;
	    }
	    break;}
	case ibis::UBYTE: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		unsigned char tmp = static_cast<unsigned char>(itmp);
		if ((int64_t)tmp != itmp) {
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- tafel::parseLine column " << i+1
			<< " in " << id << " (" << itmp << ") "
			<< "can not fit into a byte";
		    continue; // skip the line
		}
		static_cast<array_t<unsigned char>*>(col.values)
		    ->push_back(tmp);
		col.mask += 1;
		++ cnt;
	    }
	    else {
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine column " << i+1
		    << " in " << id
		    << " can not be parsed correctly as an integer";
		continue;
	    }
	    break;}
	case ibis::SHORT: {
	    ierr = ibis::util::readInt(itmp, str, del);
	    if (ierr == 0) {
		int16_t tmp = static_cast<int16_t>(itmp);
		if (tmp != itmp) {
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		    LOGGER(ibis::gVerbose > 2)
			<< "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- tafel::parseLine "
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
	case ibis::BLOB: {
	    ibis::util::getString(stmp, str, del);
	    ibis::array_t<char> *raw =
		static_cast<ibis::array_t<char>*>(col.values);
	    if (raw->empty()) {
		col.starts.resize(1);
		col.starts[0] = 0;
	    }
	    raw->insert(raw->end(), stmp.data(), stmp.data()+stmp.size());
	    col.starts.push_back(col.starts.back()+stmp.size());
	    col.mask += 1;
	    ++ cnt;
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
    if (line == 0 || *line == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::appendRow can not proceed because the incoming line "
	    "is nil or empty";
	return -1;
    }
    while (*line != 0 && isspace(*line)) ++ line;
    if (*line == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::appendRow can not proceed because the incoming line "
	    "is a blank string";
	return -1;
    }
    if (*line == '#' || (*line == '-' && line[1] == '-')) return 0;

    std::string id = "string ";
    id.append(line, 10);
    id += " ...";
    std::string delimiters = (del != 0 && *del != 0 ? del : ",");

    normalize();
    int ierr = parseLine(line, delimiters.c_str(), id.c_str());
    LOGGER(ierr < static_cast<int>(cols.size()) && ibis::gVerbose > 1)
	<< "tafel::appendRow expects to extract " << cols.size() << " value"
	<< (cols.size()>1?"s":"") << ", but got " << ierr;
    mrows += (ierr > 0);
    return ierr;
} // ibis::tafel::appendRow

int ibis::tafel::readCSV(const char* filename, int maxrows,
			 const char* outdir, const char* del) {
    if (filename == 0 || *filename == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::readCSV needs a filename to proceed";
	return -1;
    }
    if (colorder.empty()) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::readCSV(" << filename << ") can not proceed because of "
	    "improper initialization (colorder is empty)";
	return -2;
    }
    std::string delimiters = (del != 0 && *del != 0 ? del : ",");
    ibis::horometer timer;
    timer.start();

    ibis::fileManager::buffer<char> linebuf(MAX_LINE);
    std::ifstream csv(filename);
    if (! csv) {
	LOGGER(ibis::gVerbose >= 0)
	    << "tafel::readCSV(" << filename << ") failed to open the named "
	    "file for reading";
	return -3; // failed to open the specified data file
    }
    if (maxrows <= 0)
	maxrows = preferredSize();
    if (maxrows > 1) {
	try { // try to reserve request amount of space
	    reserveSpace(maxrows);
	}
	catch (...) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::readCSV(" << filename << ", " << maxrows << ", "
		<< delimiters << ") -- failed to reserve space for "
		<< maxrows << " rows for reading, continue anyway";
	}
    }

    int ierr;
    int ret = 0;
    uint32_t cnt = 0;
    uint32_t iline = 0;
    bool more = true;
    const uint32_t pline = (ibis::gVerbose < 3 ? 1000000 :
			    ibis::gVerbose < 5 ? 100000 :
			    ibis::gVerbose < 7 ? 10000 : 1000);
    char* str; // pointer to next character to be processed
    const uint32_t ncol = colorder.size();
    while (more) {
	++ iline;
	std::streampos linestart = csv.tellg();
	while (! csv.getline(linebuf.address(), linebuf.size())) {
	    if (csv.eof()) {
		*(linebuf.address()) = 0;
		more = false;
		-- iline;
		break;
	    }

	    // failed to read the line
	    const uint32_t nold =
		(linebuf.size() > 0 ? linebuf.size() : MAX_LINE);
	    // double the size of linebuf
	    if (nold+nold != linebuf.resize(nold+nold)) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- tafel::readCSV(" << filename
		    << ") failed to allocate linebuf of " << nold+nold
		    << " bytes";
		more = false;
		break;
	    }
	    csv.clear(); // clear the error bit
	    // go back to the beginning of the line so we can try to read again
	    if (! csv.seekg(linestart, std::ios::beg)) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- tafel::readCSV(" << filename
		    << ") failed to seek to the start of line # " << iline
		    << ", no way to continue";
		*(linebuf.address()) = 0;
		more = false;
		break;
	    }
	}

	str = linebuf.address();
	if (str == 0) break;
	while (*str != 0 && isspace(*str)) ++ str; // skip leading space
	if (*str == 0 || *str == '#' || (*str == '-' && str[1] == '-')) {
	    // skip comment line (shell style comment and SQL style comments)
	    continue;
	}

	if (0 < cnt && cnt < ncol)
	    normalize();
	try {
	    cnt = parseLine(str, delimiters.c_str(), filename);
	}
	catch (...) {
	    if (outdir != 0 && *outdir != 0 && mrows > 0) {
		LOGGER(ibis::gVerbose > 3)
		    << "tafel::readCSV(" << filename << ") encountered an "
		    "exception while processing line " << iline
		    << ", writing in-memory data and then continue";
		ierr = write(outdir, 0, 0, 0);
		if (ierr < 0)
		    return ierr - 10;
		ret += mrows;
		// update maxrows to avoid out of memory problem
		if (mrows > 1024) {
		    maxrows = (int) ibis::util::coarsen((double)mrows, 1U);
		    if ((unsigned)maxrows >= mrows)
			maxrows >>= 1;
		}
		else {
		    maxrows = mrows;
		}
	    }
	    else {
		return -4;
	    }

	    cnt = 0;
	    -- iline;
	    clearData();
	    csv.seekg(linestart, std::ios::beg);
	}

	mrows += (cnt > 0);
	LOGGER(ibis::gVerbose > 0 && (iline % pline) == 0)
	    << "tafel::readCSV(" << filename << ") processed line "
	    << iline << " ...";
	if (mrows >= maxrows && maxrows > 1 && outdir != 0 && *outdir != 0) {
	    ierr = write(outdir, 0, 0, 0);
	    ret += mrows;
	    if (ierr < 0)
		return ierr - 20;
	    else
		clearData();
	}
    }

    ret += mrows;
    timer.stop();
    LOGGER(ibis::gVerbose > 0)
	<< "tafel::readCSV(" << filename << ") processed " << iline
	<< (iline>1 ? " lines":" line") << " of text and extracted " << ret
	<< (ret>1?" records":" record") << " using " << timer.CPUTime()
	<< " sec(CPU), " << timer.realTime() << " sec(elapsed)";
    return ret;
} // ibis::tafel::readCSV

int ibis::tafel::readSQLDump(const char* filename, std::string& tname,
			     int maxrows, const char* outdir) {
    if (filename == 0 || *filename == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tafel::readSQLDump needs a filename to proceed";
	return -1;
    }
    const char* delimiters = ",";
    ibis::horometer timer;
    timer.start();

    const unsigned defaultBufferSize = 1048576; // 1 MB
    ibis::fileManager::buffer<char> linebuf(defaultBufferSize);
    ibis::fileManager::buffer<char> stmtbuf(defaultBufferSize);
    std::ifstream sqlfile(filename);
    if (! sqlfile) {
	LOGGER(ibis::gVerbose >= 0)
	    << "tafel::readSQLDump(" << filename << ") failed to open the "
	    "named file for reading";
	return -3; // failed to open the specified data file
    }
    if (maxrows <= 0)
	maxrows = preferredSize();
    if (maxrows > 1) {
	try { // try to reserve request amount of space
	    reserveSpace(maxrows);
	}
	catch (...) {
	    LOGGER(ibis::gVerbose > 0)
		<< "tafel::readSQLDump(" << filename << ", " << maxrows << ", "
		<< delimiters << ") -- failed to reserve space for "
		<< maxrows << " rows for reading, continue anyway";
	}
    }
    LOGGER(ibis::gVerbose > 2)
	<< "tafel::readSQLDump(" << filename
	<< ") successfully opened the named file for reading";
    int ierr;
    char *str;
    char *ptr;
    std::string tmp;
    int ret = 0;
    uint32_t iline = 0;
    const uint32_t pline = (ibis::gVerbose < 3 ? 1000000 :
			    ibis::gVerbose < 5 ? 100000 :
			    ibis::gVerbose < 7 ? 10000 : 1000);
    while ((ierr = readSQLStatement(sqlfile, stmtbuf, linebuf)) > 0) {
	++ iline;
	if (strnicmp(stmtbuf.address(), "create table ", 13) == 0) {
	    ierr = SQLCreateTable(stmtbuf.address(), tname);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- tafel::readSQLDump(" << filename
		    << ") failed to digest the creat table statement:\n\t"
		    << stmtbuf.address();
		return ierr - 10;
	    }
	    else {
		LOGGER(ibis::gVerbose > 2)
		    << "tafel::readSQLDump(" << filename
		    << ") ingest the create table statement, starting "
		    "a brand new in-memory data table with " << cols.size()
		    << " column" << (cols.size()>1?"s":"");
	    }
	}
	else if (strnicmp(stmtbuf.address(), "insert into ", 12) == 0) {
	    str = stmtbuf.address() + 12;
	    ibis::util::getString(tmp, const_cast<const char*&>(str),
				  static_cast<const char*>(0));
	    if (!tname.empty() && tmp.compare(tname) != 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "tafel::readSQLDump(" << filename << ") SQL statment # "
		    << iline << " refers to table " << tmp
		    << ", but the current active table is " << tname
		    << ", skipping this statement";
		continue;
	    }

	    do { // loop through the values
		// skip to the open '('
		while (*str != 0 && *str != '(') ++ str;
		if (*str == '(') {
		    ++ str;
		    if (*str != 0) {
			// string values can contain paired parentheses
			int nesting = 0;
			for (ptr = str;
			     *ptr != 0 && (nesting > 0 || *ptr != ')');
			     ++ ptr) {
			    nesting += (int)(*ptr == '(') - (int)(*ptr == ')');
			}
		    }
		    if (ptr > str) {
			if (*ptr == ')') *ptr = 0;
			try {
			    ierr = parseLine(str, delimiters, filename);
			}
			catch (...) {
			    if (outdir != 0 && *outdir != 0 && mrows > 0) {
				LOGGER(ibis::gVerbose > 3)
				    << "tafel::readSQLDump(" << filename
				    << ") encountered an exception while "
				    "processing statement " << iline
				    << ", writing out in-memory data";
				ierr = write(outdir, tname.c_str(), 0, 0);
				if (ierr < 0)
				    return ierr - 20;

				// to avoid future out-of-memory problem
				if (mrows > 1024) {
				    maxrows = (int)
					ibis::util::coarsen((double)mrows, 1);
				    if ((unsigned)maxrows >= mrows)
					maxrows >>= 1;
				}
				else {
				    maxrows = mrows;
				}
				ret += mrows;
				clearData();

				// try to parse the same line
				ierr = parseLine(str, delimiters, filename);
			    }
			    else {
				return -4;
			    }
			}
			mrows += (ierr > 0);

			LOGGER(ibis::gVerbose > 1 && ierr < (colorder.size()))
			    << "tafel::readSQLDump(" << filename
			    << ") expects to extract " << colorder.size()
			    << " value" << (colorder.size()>1?"s":"")
			    << ", but actually got " << ierr
			    << " while processing SQL statement # " << iline
			    << " and row " << mrows;
			LOGGER(ibis::gVerbose > 0 && (mrows % pline) == 0)
			    << "tafel::readSQLDump(" << filename
			    << ") processed row " << mrows << " ...";

			if (mrows >= maxrows && maxrows > 1 &&
			    outdir != 0 && *outdir != 0) {
			    ierr = write(outdir, tname.c_str(), 0, 0);
			    ret += mrows;
			    if (ierr < 0)
				return ierr - 20;

			    clearData();
			}
		    }
		    str = ptr + 1;
		}
	    } while (*str != 0);
	}
	else { // do nothing with this statement
	    LOGGER(ibis::gVerbose > 4)
		<< "tafel::readSQLDump(" << filename << ") skipping: "
		<< stmtbuf.address();
	}
    }

    ret += mrows;
    timer.stop();
    LOGGER(ibis::gVerbose > 0)
	<< "tafel::readSQLDump(" << filename << ") processed " << iline
	<< (iline>1 ? " lines":" line") << " of text and extracted " << ret
	<< (ret>1?" records":" record") << " using " << timer.CPUTime()
	<< " sec(CPU), " << timer.realTime() << " sec(elapsed)";
    return ret;
} // ibis::tafel::readSQLDump

/// Read one complete SQL statment from an SQL dump file.  It will read one
/// line at a time until a semicolon ';' is found.  It will expand the
/// buffers as needed.  The return value is either the number of bytes in
/// the SQL statement or an eror code (less than 0).
int ibis::tafel::readSQLStatement(std::istream& sqlfile,
				  ibis::fileManager::buffer<char>& stmt,
				  ibis::fileManager::buffer<char>& line) const {
    if (! sqlfile) return -1;

    bool more = true, retry = false;
    int nstmt = 0; // # of bytes used in stmt
    char* ptr;
    char* qtr;
    while (more) {
	std::streampos linestart = sqlfile.tellg();

	do {
	    // attempt to read a line
	    sqlfile.getline(line.address(), line.size());
	    int bytes = sqlfile.gcount();
	    retry = ! sqlfile.good();

	    // failed to read the line
	    if (sqlfile.eof() && bytes <= 0) {
		return 0;
	    }

	    if (bytes+1 == line.size()) { // line too small
		const uint32_t nold = (line.size() > 0 ? line.size() : 1048576);
		// double the size of line
		if (nold+nold != line.resize(nold+nold)) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- tafel::readSQLStatement failed to "
			"allocate a line buffer with " << nold+nold << " bytes";
		    return -2;
		}

		sqlfile.clear(); // clear the error bit
		// go back to the beginning of the line so we can try to
		// read again
		if (! sqlfile.seekg(linestart, std::ios::beg)) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- tafel::readSQLStatement failed to seek "
			"to the start of line, no way to continue";
		    *(stmt.address()) = 0;
		    return -3;
		}
		else {
		    retry = true;
		}
	    }
	} while (retry);

	// got a line of input text
	ptr = line.address();
	while (*ptr != 0 && isspace(*ptr)) ++ ptr; // skip leading space
	if (*ptr == 0) continue;

	qtr = strstr(ptr, "--");
	if (qtr != 0) // change 1st - to nil character to terminate the string
	    *qtr = 0;

	while (ptr != 0 && *ptr != 0) {
	    // copy till / *
	    qtr = strstr(ptr, "/*");
	    if (qtr == 0)
		for (qtr = ptr+1; *qtr != 0; ++ qtr);

	    int newchars = (qtr - ptr);
	    uint32_t newsize = nstmt + newchars;
	    if (newsize > stmt.size()) { // allocate new space
		newsize = (stmt.size()+stmt.size() >= newsize ?
			   stmt.size()+stmt.size() : newsize);
		ibis::fileManager::buffer<char> newbuf(line.size() << 1);
		if (newbuf.size() < line.size()) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- tafel::readSQLStatement failed to "
			"allocate  a new buffer of " << newsize << " bytes";
		    return -4;
		}
		if (nstmt > 0) {
		    char *dest = newbuf.address();
		    const char *src = stmt.address();
		    for (int j = 0; j < nstmt; ++ j)
			dest[j] = src[j];
		}
		newbuf.swap(stmt);
	    }

	    if (nstmt > 0 && isspace(stmt[nstmt-1]) == 0) {
		stmt[nstmt] = ' '; // add space between lines
		++ nstmt;
	    }
	    else if (nstmt == 0) { // skip ; at the beginning of a statement
		while (ptr < qtr && (isspace(*ptr) || *ptr == ';')) ++ ptr;
	    }
	    for (char *curr = stmt.address()+nstmt; ptr < qtr;
		 ++ ptr, ++ curr, ++ nstmt)
		*curr = *ptr;

	    // skip past * /, if not found, assume everything is comment
	    qtr = strstr(ptr, "*/");
	    if (qtr == 0) {
		ptr = 0; // skip the remaining of the line
	    }
	    else {
		ptr = qtr + 2; // skip over * /
	    }
	}

	// remove trailing space
	while (nstmt > 0 && isspace(stmt[nstmt-1])) -- nstmt;
	if (nstmt == 1 && stmt[0] == ';') {
	    nstmt = 0;
	}
	else if (nstmt > 1 && stmt[nstmt-1] == ';') {
	    more = false;
	}
    } // while (more)

    if (nstmt > 1 && stmt[nstmt-1] == ';') {
	// turn semicolon into nil character
	-- nstmt;
	stmt[nstmt] = 0;
    }
    return nstmt;
} // ibis::tafel::readSQLStatement

void ibis::tafel::describe(std::ostream &out) const {
    out << "An extensible (in-memory) table with " << mrows << " row"
	<< (mrows>1 ? "s" : "") << " and " << cols.size() << " column"
	<< (cols.size()>1 ? "s" : "");
    for (columnList::const_iterator it = cols.begin();
	 it != cols.end(); ++ it) {
	const ibis::tafel::column& col = *((*it).second);
	out << "\n  " << (*it).first
#if defined(_DEBUG) || defined(DEBUG)
	    << "(" << static_cast<void*>((*it).second) << ")"
#endif
	    << ", " << ibis::TYPESTRING[col.type]
	    << ", mask(" << col.mask.cnt() << " out of " << col.mask.size()
	    << ")";
    }
    out << std::endl;
} // ibis::tafel::describe

/// Default constructor.  The name and type are assigned later.
ibis::tafel::column::column() : type(ibis::UNKNOWN_TYPE), values(0), defval(0) {
}

/// Destructor.
ibis::tafel::column::~column() {
    LOGGER(ibis::gVerbose > 5 && !name.empty())
	<< "clearing tafel::column " << name;

    switch (type) {
    case ibis::BYTE:
	delete static_cast<array_t<signed char>*>(values);
	delete static_cast<signed char*>(defval);
	break;
    case ibis::UBYTE:
	delete static_cast<array_t<unsigned char>*>(values);
	delete static_cast<unsigned char*>(defval);
	break;
    case ibis::SHORT:
	delete static_cast<array_t<int16_t>*>(values);
	delete static_cast<int16_t*>(defval);
	break;
    case ibis::USHORT:
	delete static_cast<array_t<uint16_t>*>(values);
	delete static_cast<uint16_t*>(defval);
	break;
    case ibis::INT:
	delete static_cast<array_t<int32_t>*>(values);
	delete static_cast<int32_t*>(defval);
	break;
    case ibis::UINT:
	delete static_cast<array_t<uint32_t>*>(values);
	delete static_cast<uint32_t*>(defval);
	break;
    case ibis::LONG:
	delete static_cast<array_t<int64_t>*>(values);
	delete static_cast<int64_t*>(defval);
	break;
    case ibis::ULONG:
	delete static_cast<array_t<uint64_t>*>(values);
	delete static_cast<uint64_t*>(defval);
	break;
    case ibis::FLOAT:
	delete static_cast<array_t<float>*>(values);
	delete static_cast<float*>(defval);
	break;
    case ibis::DOUBLE:
	delete static_cast<array_t<double>*>(values);
	delete static_cast<double*>(defval);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	delete static_cast<std::vector<std::string>*>(values);
	delete static_cast<std::string*>(defval);
	break;
    case ibis::BLOB:
	delete static_cast<ibis::array_t<unsigned char>*>(values);
	delete static_cast<std::string*>(defval);
	starts.clear();
	break;
    default:
	break;
    }
} // ibis::tafel::column::~column

/// Create a tablex for entering new data.
ibis::tablex* ibis::tablex::create() {
    return new ibis::tafel;
} // ibis::tablex::create

int ibis::tablex::readNamesAndTypes(const char* filename) {
    if (filename == 0 || *filename == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tablex::readNamesAndTypes needs a filename to proceed";
	return -1;
    }

    ibis::fileManager::buffer<char> linebuf(MAX_LINE);
    std::ifstream ntfile(filename);
    if (! ntfile) {
	LOGGER(ibis::gVerbose >= 0)
	    << "tablex::readNamesAndTypes(" << filename
	    << ") failed to open the named file for reading";
	return -3;
    }

    int ret = 0;
    bool more = true;
    while (more) {
	std::streampos linestart = ntfile.tellg();
	while (! ntfile.getline(linebuf.address(), linebuf.size())) {
	    if (ntfile.eof()) {
		*(linebuf.address()) = 0;
		more = false;
		break;
	    }

	    const uint32_t nold =
		(linebuf.size() > 0  ? linebuf.size() : MAX_LINE);
	    if (nold+nold != linebuf.resize(nold+nold)) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- tablex::readNamesAndTypes(" << filename
		    << ") failed to allocate linebuf of " << nold+nold
		    << " bytes";
		more = false;
		break;
	    }
	    ntfile.clear(); // clear the error bit
	    if (! ntfile.seekg(linestart, std::ios::beg)) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- tablex::readNamesAndTypes(" << filename
		    << ") failed to seek to the beginning of a line";
		*(linebuf.address()) = 0;
		more = false;
		break;
	    }
	}
	if (linebuf.address() != 0 && *(linebuf.address()) != 0) {
	    int ierr = parseNamesAndTypes(linebuf.address());
	    if (ierr >  0)
		ret += ierr;
	}
    } // while (more)

    LOGGER(ibis::gVerbose > 2)
	<< "tablex::readNamesAndTypes(" << filename << ") successfully parsed "
	<< ret << " name-type pair" << (ret > 1 ? "s" : "");
    return ret;
} // ibis::tablex::readNamesAndTypes

/// A column name must start with an alphabet or a underscore (_); it can be
/// followed by any number of alphanumeric characters (including
/// underscores).  For each built-in data types, the type names recognized
/// are as follows:
/// - ibis::BYTE: byte,
/// - ibis::UBYTE: ubyte, unsigned byte,
/// - ibis::SHORT: short, halfword
/// - ibis::USHORT: ushort, unsigned short,
/// - ibis::INT: int,
/// - ibis::UINT: uint, unsigned int,
/// - ibis::LONG: long,
/// - ibis::ULONG: ulong, unsigned long,
/// - ibis::FLOAT: float, real,
/// - ibis::DOUBLE: double,
/// - ibis::CATEGORY: category, key
/// - ibis::TEXT: text, string
///
/// If it can not find a type, but a valid name is found, then the type is
/// assumed to be int.
///
/// @note Column names are not case-sensitive and all types should be
/// specified in lower case letters.
///
/// Characters following '#' or '--' on a line will be treated as comments
/// and discarded.
int ibis::tablex::parseNamesAndTypes(const char* txt) {
    if (txt == 0 || *txt == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "tablex::parseNamesAndTypes received an empty string";
	return -1;
    }

    int ret = 0;
    const char *str = txt;
    std::string nm, type;
    while (*str != 0) {
	// skip leading space
	while (*str != 0 && isspace(*str)) ++ str;
	// find first alphabet or _
	while (*str != 0) {
	    if (*str == '#' || (*str == '-' && str[1] == '-')) return ret;
	    else if(*str != '_' && isalpha(*str) == 0) ++ str;
	    else break;
	}
	nm.clear();
	while (*str != 0 && (isalnum(*str) != 0 || *str == '_')) {
	    nm += *str;
	    ++ str;
	}

	if (nm.empty()) return ret;

	// skip ti
	while (*str != 0) {
	    if (*str == '#' || (*str == '-' && str[1] == '-')) {
		for (++ str; *str != 0; ++ str);
	    }
	    else if (isalpha(*str) == 0) ++ str;
	    else break;
	}
	type.clear();
	while (*str != 0 && isalpha(*str) != 0) {
	    type += *str;
	    ++ str;
	}
	if (type.compare("unsigned") == 0 || type.compare("signed") == 0) {
	    // read the second word, drop signed
	    if (type.compare("signed") == 0)
		type.clear();
	    while (*str != 0 && isspace(*str) != 0) ++ str;
	    while (*str != 0 && isalpha(*str) != 0) {
		type += *str;
		++ str;
	    }
	}
	if (type.empty())
	    type = 'i';

	LOGGER(ibis::gVerbose > 2)
	    << "tablex::parseNamesAndTypes processing name:type pair \"" << nm
	    << ':' << type << "\"";

	if (type.compare("unsigned") == 0) {
	    switch (*(type.c_str()+9)) {
	    case 'b':
	    case 'B':
		addColumn(nm.c_str(), ibis::UBYTE); break;
	    case 's':
	    case 'S':
		addColumn(nm.c_str(), ibis::USHORT); break;
	    case 'i':
	    case 'I':
		addColumn(nm.c_str(), ibis::UINT); break;
	    case 'l':
	    case 'L':
		addColumn(nm.c_str(), ibis::ULONG); break;
	    default:
		LOGGER(ibis::gVerbose > 2)
		    << "tablex::parseNamesAndTypes assumes type \"" << type
		    << "\" to be uint32_t";
		addColumn(nm.c_str(), ibis::UINT); break;
	    }
	}
	else if (type[0] == 'u' || type[0] == 'U') {
	    switch (*(type.c_str()+1)) {
	    case 'b':
	    case 'B':
		addColumn(nm.c_str(), ibis::UBYTE); break;
	    case 's':
	    case 'S':
		addColumn(nm.c_str(), ibis::USHORT); break;
	    case 'i':
	    case 'I':
		addColumn(nm.c_str(), ibis::UINT); break;
	    case 'l':
	    case 'L':
		addColumn(nm.c_str(), ibis::ULONG); break;
	    default:
		LOGGER(ibis::gVerbose > 2)
		    << "tablex::parseNamesAndTypes assumes type \"" << type
		    << "\" to be uint32_t";
		addColumn(nm.c_str(), ibis::UINT); break;
	    }
	}
	else {
	    switch (type[0]) {
	    case 'a':
	    case 'A':
		addColumn(nm.c_str(), ibis::UBYTE); break;
	    case 'b':
	    case 'B':
		if (type[1] == 'l' || type[1] == 'L')
		    addColumn(nm.c_str(), ibis::BLOB);
		else
		    addColumn(nm.c_str(), ibis::BYTE);
		break;
	    case 'h':
	    case 'H':
		addColumn(nm.c_str(), ibis::SHORT); break;
	    case 'g':
	    case 'G':
		addColumn(nm.c_str(), ibis::USHORT); break;
	    case 'i':
	    case 'I':
		addColumn(nm.c_str(), ibis::INT); break;
	    case 'l':
	    case 'L':
		addColumn(nm.c_str(), ibis::LONG); break;
	    case 'v':
	    case 'V':
		addColumn(nm.c_str(), ibis::ULONG); break;
	    case 'r':
	    case 'R':
	    case 'f':
	    case 'F':
		addColumn(nm.c_str(), ibis::FLOAT); break;
	    case 'd':
	    case 'D':
		addColumn(nm.c_str(), ibis::DOUBLE); break;
	    case 'c':
	    case 'C':
	    case 'k':
	    case 'K':
		addColumn(nm.c_str(), ibis::CATEGORY); break;
	    case 't':
	    case 'T':
		addColumn(nm.c_str(), ibis::TEXT); break;
	    case 'q':
	    case 'Q':
		addColumn(nm.c_str(), ibis::BLOB); break;
	    case 's':
	    case 'S':
		if (type[1] == 't' && type[1] == 'T')
		    addColumn(nm.c_str(), ibis::TEXT);
		else
		    addColumn(nm.c_str(), ibis::SHORT);
		break;
	    default:
		LOGGER(ibis::gVerbose > 2)
		    << "tablex::parseNamesAndTypes assumes type \"" << type
		    << "\" to be int32_t";
		addColumn(nm.c_str(), ibis::INT); break;
	    }
	}
	++ ret;
    } // while (*str != 0)

    LOGGER(ibis::gVerbose > 4)
	<< "tablex::parseNamesAndType extracted " << ret
	<< " name-type pair" << (ret > 1 ? "s" : "");
    return ret;
} // ibis::tablex::parseNamesAndTypes

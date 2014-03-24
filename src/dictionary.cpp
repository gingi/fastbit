//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2014 the Regents of the University of California
#include "dictionary.h"
#include "utilidor.h"

/// The header of dictionary files.  It has 20 bytes exactly.
static char _fastbit_dictionary_header[20] =
    {'#', 'I', 'B', 'I', 'S', ' ', 'D', 'i', 'c', 't',
     'i', 'o', 'n', 'a', 'r', 'y', 1, 0, 0, 0};

/// Copy constructor.  Places all the string in one contiguous buffer.
ibis::dictionary::dictionary(const ibis::dictionary& old)
    : raw_(old.raw_.size()), buffer_(1) {
    raw_[0] = 0;
    if (old.key_.empty()) {
	buffer_[0] = 0;
	return;
    }

    const uint32_t nelm = old.raw_.size();
    // find out the size of the buffer to allocate
    size_t sz = nelm;
    for (size_t i = 1; i < nelm; ++ i)
	sz += std::strlen(old.raw_[i+1U]);
    char *str = new char[sz];
    buffer_[0] = str;

    // copy the string values and populate the hash_map
    for (size_t i = 0; i < nelm; ++ i) {
	raw_[i+1U] = str;
	for (const char *t = old.raw_[i+1U]; *t != 0; ++ t, ++ str)
	    *str = *t;
	*str = 0;
	++ str;
        key_[raw_[i+1U]] = i+1U;
    }
} // copy constructor

/// Compare whether this dicrionary and the other are equal in content.
/// The two dictionaries are considered same only if they have the same
/// keys in the same order.
bool ibis::dictionary::equal_to(const ibis::dictionary& other) const {
    if (key_.size() != other.key_.size() || raw_.size() != other.raw_.size())
	return false;

    for (size_t j = 1; j < raw_.size(); ++ j)
	if (std::strcmp(raw_[j], other.raw_[j]) != 0)
	    return false;
    return true;
} // ibis::dictionary::equal_to

/// Copy function.  Use copy constructor and swap the content.
void ibis::dictionary::copy(const ibis::dictionary& old) {
    ibis::dictionary tmp(old);
    swap(tmp);
} // ibis::dictionary::copy

/**
   Write the content of the dictionary to the named file.  The existing
   content in the named file is overwritten.  The content of the dictionary
   file is laid out as follows.

   \li Signature "#IBIS Dictionary " and version number (currently
   0x010000). (20 bytes)

   \li N = Number of strings in the file. (4 bytes)

   \li uint64_t[N+1]: the starting positions of the strings in this file.

   \li the string values packed one after the other with their nil
   terminators.
*/
int ibis::dictionary::write(const char* name) const {
    if (name == 0 || *name == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write can not proceed with a "
	    "null string as the file name";
	return -1;
    }
    if (key_.size()+1U != raw_.size()) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") can not write an inconsistent dictionary, key_.size("
	    << key_.size() << "), raw_.size(" << raw_.size() << ")";
	return -2;
    }

    std::string evt = "dictionary::write";
    if (ibis::gVerbose > 1) {
        evt += '(';
        evt += name;
        evt += ')';
    }
    ibis::util::timer mytimer(evt.c_str(), 4);
    FILE* fptr = fopen(name, "wb");
    if (fptr == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to open the file ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -3;
    }

    IBIS_BLOCK_GUARD(fclose, fptr);
    int ierr = fwrite(_fastbit_dictionary_header, 1, 20, fptr);
    if (ierr != 20) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt
	    << " failed to write the header, fwrite returned " << ierr;
	return -4;
    }

    const uint32_t nkeys = key_.size();
    ierr = fwrite(&nkeys, sizeof(nkeys), 1, fptr);
    if (ierr != 1) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to write the size(" << nkeys
	    << "), fwrite returned " << ierr;
	return -5;
    }
    if (nkeys == 0) // nothing else to write
	return 0;

    mergeBuffers();
    array_t<uint64_t> pos(nkeys+1);
    ierr = (buffer_.size() > 1);
    for (unsigned j = 1; ierr == 0 && j < raw_.size(); ++ j) {
        if (raw_[j] > raw_[j-1]) {
            pos[j] = (raw_[j] - buffer_[0]);
        }
        else {
            ierr = 1;
        }
    }
    if (ierr == 0) {
        ierr = writeBuffer(fptr, nkeys, pos);
    }
    else {
        ierr = writeKeys(fptr, nkeys, pos);
    }
    return ierr;
} // ibis::dictionary::write

/// Write the dictionary one keyword at a time.  This version requires on
/// write call on each keyword, which can be time consuming when there are
/// many keywords.
int ibis::dictionary::writeKeys(FILE *fptr, uint32_t nkeys,
                                array_t<uint64_t> &pos) const {
    int ierr = fseek(fptr, sizeof(uint64_t)*(nkeys+1), SEEK_CUR);
    long int tmp = ftell(fptr);
    pos[0] = tmp;
    for (unsigned i = 0; i < nkeys; ++ i) {
	const int len = 1 + std::strlen(raw_[i+1U]);
	ierr = fwrite(raw_[i+1U], 1, len, fptr);
	LOGGER(ierr != len && ibis::gVerbose > 1)
	    << "Warning -- dictionary::writeKeys failed to write key[" << i
            << "]; expected fwrite to return " << len << ", but got " << ierr;

	tmp = ftell(fptr);
	pos[i+1] = tmp;
	LOGGER(24 > tmp && ibis::gVerbose > 1)
	    << "Warning -- dictionary::writeKeys failed to store position "
	    << tmp << " into a 64-bit integer; file will be UNUSABLE!";
    }

    // go back to write the positions
    ierr = fseek(fptr, 24, SEEK_SET);
    LOGGER(ierr != 0 && ibis::gVerbose > 1)
	<< "Warning -- dictionary::writeKeys failed to seek to offset 24 "
        "to write the offsets";

    ierr = fwrite(pos.begin(), sizeof(uint64_t), nkeys+1, fptr);
    LOGGER(ierr != (int)(nkeys+1) && ibis::gVerbose > 1)
	<< "Warning -- dictionary::writeKeys failed to write the offsets, "
        "expected fwrite to return " << nkeys+1 << ", but got " << ierr;
    return -7 * (ierr != (int)(nkeys+1));
} // ibis::dictionary::writeKeys

/// Write the buffer out directly.
///
/// @ntoe This function is intended to be used by dictionary::write and
/// must satisfy the following conditions.  There must be only one buffer,
/// and the raw_ must be ordered in that buffer.  Under these conditions,
/// we can write the buffer using a single sequential write operations,
/// which should reduce the I/O time.
int ibis::dictionary::writeBuffer(FILE *fptr, uint32_t nkeys,
                                  array_t<uint64_t> &pos) const {
    unsigned long offset = 24 + 8 * (nkeys+1);
    for (unsigned j = 0; j < nkeys; ++ j)
        pos[j] = offset + pos[j+1];
    size_t ierr = (raw_[nkeys] != 0);
    if (ierr > 0U) {
        for (const char *tmp = raw_[nkeys]; *tmp != 0; ++ tmp)
            ++ ierr;
    }
    pos[nkeys] = pos[nkeys-1] + (unsigned)ierr;

    ierr = fwrite(pos.begin(), sizeof(uint64_t), nkeys+1, fptr);
    LOGGER(ierr != (int)(nkeys+1) && ibis::gVerbose > 1)
	<< "Warning -- dictionary::writeBuffer failed to write the offsets, "
        "expected fwrite to return " << nkeys+1 << ", but got " << ierr;

    const char *buff = buffer_[0];
    size_t sz = pos[nkeys] - pos[0];
    while (sz > 0) {  // a large buffer may need multuple fwrite calls
        ierr = fwrite(buff, 1, sz, fptr);
        if (ierr > 0U && ierr <= sz) {
            buff += ierr;
            sz -= ierr;
        }
        else {
            LOGGER(ibis::gVerbose > 1)
                << "Warning -- dictionary::writeBuffer failed to write the "
                "buffer, fwrite retruned 0";
            return -6;
        }
    }
    return 0;
} // ibis::dictionary::writeBuffer

/// Read the content of the named file.  The file content is read into the
/// buffer in one-shot and then digested.
///
/// This function determines the version of the dictionary and invokes the
/// necessary reading function to perform the actual reading operations.
/// Currently there are three possible version of dictioanries
/// 0x01000000 - the version produced by the current write function,
/// 0x00000000 - the version produced by the previous version of the write
///              function that uses 32-bit offsets and stores strings in
///              sorted order.
/// unmarked   - the version without a header, only has the bare strings in
///              the code order.
int ibis::dictionary::read(const char* name) {
    if (name == 0 || *name == 0) return -1;
    std::string evt = "dictionary::read";
    if (ibis::gVerbose > 1) {
        evt += '(';
        evt += name;
        evt += ')';
    }
    // open the file to read
    int ierr = 0;
    FILE* fptr = fopen(name, "rb");
    if (fptr == 0) {
	LOGGER(ibis::gVerbose > 3)
	    << "Warning -- " << evt << " failed to open the file ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -2;
    }

    ibis::util::timer mytimer(evt.c_str(), 4);
    IBIS_BLOCK_GUARD(fclose, fptr);
    ierr = fseek(fptr, 0, SEEK_END); // to the end
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to seek to the end of the file";
	return -3;
    }

    uint32_t version = 0xFFFFFFFFU;
    long int sz = ftell(fptr); // file size
    if (sz > 24) {
	char header[20];
	ierr = fseek(fptr, 0, SEEK_SET);
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- " << evt << " failed to seek to the beginning "
		"of the file";
	    return -4;
	}

	ierr = fread(header, 1, 20, fptr);
	if (ierr != 20) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- " << evt << " failed to read the 20-byte header";
	    return -5;
	}
	if (header[0] == _fastbit_dictionary_header[0] &&
	    header[1] == _fastbit_dictionary_header[1] &&
	    header[2] == _fastbit_dictionary_header[2] &&
	    header[3] == _fastbit_dictionary_header[3] &&
	    header[4] == _fastbit_dictionary_header[4] &&
	    header[5] == _fastbit_dictionary_header[5] &&
	    header[6] == _fastbit_dictionary_header[6] &&
	    header[7] == _fastbit_dictionary_header[7] &&
	    header[8] == _fastbit_dictionary_header[8] &&
	    header[9] == _fastbit_dictionary_header[9] &&
	    header[10] == _fastbit_dictionary_header[10] &&
	    header[11] == _fastbit_dictionary_header[11] &&
	    header[12] == _fastbit_dictionary_header[12] &&
	    header[13] == _fastbit_dictionary_header[13] &&
	    header[14] == _fastbit_dictionary_header[14] &&
	    header[15] == _fastbit_dictionary_header[15]) {
            version = (header[16] << 24 | header[17] << 16 |
                       header[18] << 8 | header[19]);
	}
	else {
	    LOGGER(ibis::gVerbose > 2)
		<< evt << " did not find the expected header, assume "
		"to have no header (oldest version of dictioinary)";
	}
    }

    // invoke the actual reader based on version number
    switch (version) {
    case 0x01000000:
	    ierr = readKeys1(evt.c_str(), fptr);
            break;
    case 0x00000000:
	    ierr = readKeys0(evt.c_str(), fptr);
            break;
    default:
	    ierr = readRaw(evt.c_str(), fptr);
            break;
    }
    if (ibis::gVerbose > 4) {
        ibis::util::logger lg;
        lg() << evt << " completed with ";
        print(lg());
    }
    return ierr;
} // ibis::dictionary::read

/// Read the raw strings.  This is for the oldest style dictionary that
/// contains the raw strings.  There is no header in the dictionary file,
/// therefore this function has rewind back to the beginning of the file.
/// On successful completion, this function returns 0.
int ibis::dictionary::readRaw(const char *evt, FILE *fptr) {
    int ierr = fseek(fptr, 0, SEEK_END);
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to seek to the end of the file";
	return -11;
    }
    clear();
    long int sz = ftell(fptr); // file size
    ierr = sz;
    if (ierr != sz) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " can not proceed because the "
	    "dictionary file size (" << sz
	    << ") does not fit into a 32-bit integer";
	return -12;
    }

    buffer_.resize(1);
    buffer_[0] = new char[sz];
    ierr = fseek(fptr, 0, SEEK_SET);
    if (ierr != 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to seek to the beginning of "
	    "the file";
	return -13;
    }
    ierr = fread(buffer_[0], 1, sz, fptr);
    if (ierr != sz) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read " << sz << " byte"
	    << (sz>1?"s":"") << ", fread returned " << ierr;
	delete [] buffer_[0];
	buffer_.clear();
	return -14;
    }

    const char *str = buffer_[0];
    const char *end = buffer_[0] + ierr;
    do {
        key_[str] = raw_.size();
        raw_.push_back(str);
	while (*str != 0 && str < end) ++ str;
	if (*str == 0) {
	    ++ str;
	}
    } while (str < end);

    return 0;
} // ibis::dictionary::readRaw

/// Read the string values.  This function processes the data produced by
/// version 0x00000000 of the write function.  On successful completion, it
/// returns 0.
///
/// Note that this function assume the 20-byte header has been read
/// already.
int ibis::dictionary::readKeys0(const char *evt, FILE *fptr) {
    uint32_t nkeys;
    int ierr = fread(&nkeys, 4, 1, fptr);
    if (ierr != 1) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt
	    << " failed to read the number of keys, fread returned " << ierr;
	return -6;
    }

    clear();
    array_t<uint32_t> codes(nkeys);
    ierr = fread(codes.begin(), 4, nkeys, fptr);
    if (ierr != (long)nkeys) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read " << nkeys
            << ", fread returned " << ierr;
	return -7;
    }

    array_t<uint32_t> offsets(nkeys+1);
    ierr = fread(offsets.begin(), 4, nkeys+1, fptr);
    if (ierr != (int)(1+nkeys)) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read the string positions, "
	    "expected fread to return " << nkeys+1 << ", but got " << ierr;
	return -8;
    }

    buffer_.resize(1);
    buffer_[0] = new char[offsets.back()-offsets.front()];
    ierr = fread(buffer_[0], 1, offsets.back()-offsets.front(), fptr);
    if (ierr != (int)(offsets.back()-offsets.front())) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read the strings, "
	    "expected fread to return " << (offsets.back()-offsets.front())
	    << ", but got " << ierr;
	return -9;
    }
    raw_.resize(nkeys+1);
    key_.reserve(nkeys+nkeys);
    for (unsigned j = 0; j < nkeys; ++ j) {
        const uint32_t ik = codes[j];
        if (ik > 0 && ik <= nkeys) {
            raw_[ik] = buffer_[0] + (offsets[j] - offsets[0]);
            key_[raw_[ik]] = ik;
        }
        else {
            LOGGER(ibis::gVerbose > 1)
                << "Warning -- " << evt << " encounter code " << ik
                << " which is out of the expected range [0, " << nkeys << ']';
        }
#if DEBUG+0 > 2 || _DEBUG+0 > 2
        LOGGER(ibis::gVerbose > 0)
            << "DEBUG -- " << evt << " raw_[" << ik << "] = \"" << raw_[ik]
            << '"';
#endif
    }

#if DEBUG+0 > 2 || _DEBUG+0 > 2
    ibis::util::logger lg;
    lg() << "DEBUG -- " << evt << " got the following keys\n\t";
    for (MYMAP::const_iterator it = key_.begin(); it != key_.end(); ++ it)
        lg() << '"' << it->first << '"' << "(" << it->second << ") ";
#endif
    return 0;
} // ibis::dictionary::readKeys0

/// Read the string values.  This function processes the data produced by
/// version 0x01000000 of the write function.  On successful completion, it
/// returns 0.
int ibis::dictionary::readKeys1(const char *evt, FILE *fptr) {
    uint32_t nkeys;
    int ierr = fread(&nkeys, 4, 1, fptr);
    if (ierr != 1) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt
	    << " failed to read the number of keys, fread returned " << ierr;
	return -6;
    }

    clear();

    array_t<uint64_t> offsets(nkeys+1);
    ierr = fread(offsets.begin(), 8, nkeys+1, fptr);
    if (ierr != (int)(1+nkeys)) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read the string positions, "
	    "expected fread to return " << nkeys+1 << ", but got " << ierr;
	return -8;
    }

    buffer_.resize(1);
    buffer_[0] = new char[offsets.back()-offsets.front()];
    ierr = fread(buffer_[0], 1, offsets.back()-offsets.front(), fptr);
    if (ierr != (int)(offsets.back()-offsets.front())) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read the strings, "
	    "expected fread to return " << (offsets.back()-offsets.front())
	    << ", but got " << ierr;
	return -9;
    }
    raw_.resize(nkeys+1);
    key_.reserve(nkeys+nkeys);
    for (unsigned j = 0; j < nkeys; ++ j) {
	raw_[j+1] = buffer_[0] + (offsets[j] - offsets[0]);
        key_[raw_[j+1]] = j+1;
#if DEBUG+0 > 2 || _DEBUG+0 > 2
        LOGGER(ibis::gVerbose > 0)
            << "DEBUG -- " << evt << " raw_[" << j+1 << "] = \"" << raw_[j+1]
            << '"';
#endif
    }

#if DEBUG+0 > 2 || _DEBUG+0 > 2
    ibis::util::logger lg;
    lg() << "DEBUG -- " << evt << " got the following keys\n\t";
    for (MYMAP::const_iterator it = key_.begin(); it != key_.end(); ++ it)
        lg() << '"' << it->first << '"' << "(" << it->second << ") ";
#endif
    return 0;
} // ibis::dictionary::readKeys1

void ibis::dictionary::print(std::ostream &out) const {
    out << "dictionary @" << static_cast<const void*>(this) << " with "
        << raw_.size() - 1 << " entr" << (raw_.size()>2?"ies":"y");
    for (unsigned j = 1; j < raw_.size(); ++ j)
        out << "\n" << j << ": \"" << (raw_[j]?raw_[j]:"") << '"';
} // ibis::dictionary::print

/// Clear the allocated memory.  Leave only the NULL entry.
void ibis::dictionary::clear() {
    for (size_t i = 0; i < buffer_.size(); ++ i)
	delete [] buffer_[i];
    buffer_.clear();
    key_.clear();
    raw_.resize(1);
    raw_[0] = 0;
} // ibis::dictionary::clear

/// Find all codes that matches the SQL LIKE pattern.
/// If the pattern is null or empty, matches is not changed.
void ibis::dictionary::patternSearch(const char* pat,
				     array_t<uint32_t>& matches) const {
    if (pat == 0) return;
    if (*pat == 0) return;
    if (key_.size() == 0) return;
    if (! (key_.size()+1 == raw_.size())) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::patternSearch(" << pat
	    << ") can not proceed because the member variables have "
	    "inconsistent sizes: raw_.size(" << raw_.size() << ", key_.size("
	    << key_.size() << ')';
	return;
    }

#if FASTBIT_CASE_SENSITIVE_COMPARE+0 == 0
    for (char *ptr = const_cast<char*>(pat); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif
    // extract longest constant prefix to restrict range
    size_t pos;
    bool esc = false;
    bool meta = false;
    std::string prefix;
    const size_t len = std::strlen(pat);
    for (pos = 0; pos < len && !meta; ++pos) {
	const char c = *(pat + pos);
	if (esc) {
	    prefix.append(1, c);
	    esc = false;
	}
	else {
	    switch (c) {
	    case STRMATCH_META_ESCAPE:
		esc = true;
		break;
	    case STRMATCH_META_CSH_ANY:
	    case STRMATCH_META_CSH_ONE:
	    case STRMATCH_META_SQL_ANY:
	    case STRMATCH_META_SQL_ONE:
		meta = true;
		break;
	    default:
		prefix.append(1, c);
		break;
	    }
	}
    }

    // if there is no meta char, find the string directly
    if (!meta) {
	uint32_t code = operator[](prefix.c_str());
	if (code < raw_.size()) {
	    matches.push_back(code);
	}
	return;
    }

    // match values in the range
    for (MYMAP::const_iterator j = key_.begin();
         j != key_.end(); ++ j) {
	if (ibis::util::strMatch(j->first, pat)) {
	    matches.push_back(j->second);
	}
    }

#if DEBUG+0 > 2 || _DEBUG+0 > 2
    ibis::util::logger lg;
    lg() << "DEBUG -- dictionary::patternSearch(" << pat << ") found "
         << matches.size() << " matching value" << (matches.size()>1?"s":"")
         << ":\t";
    for (unsigned j = 0; j < matches.size(); ++ j)
        lg() << matches[j] << ' ';
#endif
}

/// Convert a string to its integer code.  Returns 0 for empty (null)
/// strings, 1:size() for strings in the dictionary, and
/// dictionary::size()+1 for unknown values.
uint32_t ibis::dictionary::operator[](const char* str) const {
    if (str == 0) return 0;
#ifdef FASTBIT_EMPTY_STRING_AS_NULL
    if (*str == 0) return 0;
#endif
#if FASTBIT_CASE_SENSITIVE_COMPARE+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif

#if DEBUG+0 > 2 || _DEBUG+0 > 2
    {
        ibis::util::logger lg;
        lg() << "DEBUG -- dictionary has the following keys\n\t";
        for (MYMAP::const_iterator it = key_.begin(); it != key_.end(); ++ it)
            lg() << '"' << it->first << '"' << ' ';
    }
#endif
    MYMAP::const_iterator it = key_.find(str);
    if (it != key_.end()) {
#if DEBUG+0 > 2 || _DEBUG+0 > 2
        LOGGER(ibis::gVerbose > 0)
            << "DEBUG -- dictionary::operator[] found code " << it->second
            << " for string \"" << str << '"';
#endif
        return it->second;
    }
    else {
#if DEBUG+0 > 2 || _DEBUG+0 > 2
        LOGGER(ibis::gVerbose > 0)
            << "DEBUG -- dictionary::operator[] could NOT find a code for "
            "string \"" << str << '"';
#endif
        return raw_.size();
    }
} // string to int

/// Insert a string to the dictionary.  Returns the integer value assigned
/// to the string.  A copy of the string is stored internally.
uint32_t ibis::dictionary::insert(const char* str) {
    if (str == 0) return 0;
#ifdef FASTBIT_EMPTY_STRING_AS_NULL
    if (*str == 0) return 0;
#endif
#if FASTBIT_CASE_SENSITIVE_COMPARE+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif
    MYMAP::const_iterator it = key_.find(str);
    if (it != key_.end()) {
#if DEBUG+0 > 2 || _DEBUG+0 > 2
        LOGGER(ibis::gVerbose > 0)
            << "DEBUG -- dictionary::insert(" << str << ") return an existing code "
            << it->second;
#endif
        return it->second;
    }
    else {
        // incoming string is a new entry
        const uint32_t nk = raw_.size();
        char *copy = ibis::util::strnewdup(str);
        buffer_.push_back(copy);
        raw_.push_back(copy);
        key_[copy] = nk;
#if DEBUG+0 > 2 || _DEBUG+0 > 2
        LOGGER(ibis::gVerbose > 0)
            << "DEBUG -- dictionary::insert(" << raw_.back() << ") return a new code " << nk;
#endif
        return nk;
    }
} // ibis::dictionary::insert

/// Append a string to the dictionary.  Returns the integer value assigned
/// to the string.  A copy of the string is stored internally.
///
/// This function assumes the incoming string is ordered after all known
/// strings to this dictionary object.  In other word, this function
/// expects the strings to be passed in in the sorted (ascending) order.
/// It does not attempt to check that the incoming is indeed ordered after
/// all known strings.  However, if this assumption is violated, the
/// resulting dictionary will not be able to work properly.
///
/// @note The incoming string is copied to this object.
uint32_t ibis::dictionary::appendOrdered(const char* str) {
    if (str == 0) return 0;
#ifdef FASTBIT_EMPTY_STRING_AS_NULL
    if (*str == 0) return 0;
#endif
#if FASTBIT_CASE_SENSITIVE_COMPARE+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif

    const uint32_t ind = key_.size();
    char *copy = ibis::util::strnewdup(str);
    buffer_.push_back(copy);
    raw_.push_back(copy);
    key_[copy] = ind+1U;
    return ind+1U;
} // ibis::dictionary::appendOrdered

/// Non-copying insert.  Do not make a copy of the input string.  Transfers
/// the ownership of @c str to the dictionary.  Caller needs to check
/// whether it is a new word in the dictionary.  If it is not a new word in
/// the dictionary, the dictionary does not take ownership of the string
/// argument.
uint32_t ibis::dictionary::insertRaw(char* str) {
    if (str == 0) return 0;
#ifdef FASTBIT_EMPTY_STRING_AS_NULL
    if (*str == 0) return 0;
#endif
#if FASTBIT_CASE_SENSITIVE_COMPARE+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif
    MYMAP::const_iterator it = key_.find(str);
    if (it != key_.end()) {
        return it->second;
    }
    else {
        // incoming string is a new entry
        const uint32_t nk = raw_.size();
        buffer_.push_back(str);
        raw_.push_back(str);
        key_[str] = nk;
        return nk;
    }
} // ibis::dictionary::insertRaw

/// Reassign the integer values to the strings.  Upon successful completion
/// of this function, the integer values assigned to the strings will be in
/// ascending order.  In other word, string values that are lexigraphically
/// smaller will have smaller integer representations.
///
/// The argument to this function carrys the permutation information needed
/// to turn the previous integer assignments into the new ones.  If the
/// previous assignment was k, the new assignement will be o2n[k].  Note
/// that the name o2n is shorthand for old-to-new.
void ibis::dictionary::sort(ibis::array_t<uint32_t> &o2n) {
    const size_t nelm = raw_.size();
    ibis::array_t<uint32_t> n2o(nelm);
    for (size_t j = 0; j < nelm; ++ j)
        n2o[j] = j;
#if DEBUG+0 > 2 || _DEBUG+0 > 2
    {
        ibis::util::logger lg;
        lg() << "DEBUG -- dictionary::sort starts with\n\t";
        for (MYMAP::const_iterator it = key_.begin(); it != key_.end(); ++ it)
            lg() << '"' << it->first << '"' << "(" << it->second << ") ";
    }
#endif
    ibis::util::sortStrings(raw_, n2o, 1, nelm);
    o2n.resize(nelm);
    for (size_t j = 0; j < nelm; ++ j)
        o2n[n2o[j]] = j;
    for (MYMAP::iterator it = key_.begin(); it != key_.end(); ++ it)
        it->second = o2n[it->second];
#if DEBUG+0 > 2 || _DEBUG+0 > 2
    {
        ibis::util::logger lg;
        lg() << "DEBUG -- dictionary::sort ends with";
        for (MYMAP::const_iterator it = key_.begin(); it != key_.end(); ++ it)
            lg() << "\n\t\"" << it->first << '"' << "(" << it->second << ": " << raw_[it->second] << ") ";
        lg() << "\n\to2n(" << o2n.size() << "):";
        for (size_t j = 1; j < nelm; ++ j)
            lg() << "\n\to2n[" << j << "] = " << o2n[j] << ": " << raw_[o2n[j]];
    }
#endif
} // ibis::dictionary::sort

/// Merge the incoming dictionary with this one.  It produces a dictionary
/// that combines the words in both dictionaries and keep the words in
/// ascending order.
///
/// Upon successful completion of this function, the return value will be
/// the new size of the dictionary, i.e., the number of non-empty words.
/// It returns a negative value to indicate error.
int ibis::dictionary::merge(const ibis::dictionary& rhs) {
    const uint32_t nr = rhs.key_.size();
    if (nr == 0) {
	return key_.size();
    }

    for (size_t j = 1; j < rhs.raw_.size(); ++ j)
        (void) insert(rhs.raw_[j]);
    return key_.size();
} // ibis::dictionary::merge

/// Produce an array that maps the integers in old dictionary to the new
/// one.  The incoming dictionary represents the old dictionary, this
/// dictionary represents the new one.
///
/// Upon successful completion of this fuction, the array o2n will have
/// (old.size()+1) number of elements, where the new value for the old code
/// i is stored as o2n[i].
int ibis::dictionary::morph(const ibis::dictionary &old,
			    ibis::array_t<uint32_t> &o2n) const {
    const uint32_t nold = old.key_.size();
    const uint32_t nnew = key_.size();
    if (nold > nnew) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::morph can not proceed because the "
	    "new dictioanry is smaller than the old one";
	return -1;
    }

    o2n.resize(nold+1);
    o2n[0] = 0;
    if (nold == 0) return 0;

    for (uint32_t j0 = 1; j0 < old.raw_.size(); ++ j0)
        o2n[j0] = operator[](raw_[j0]);
    return nold;
} // ibis::dictioniary::morph

/// Merge all buffers into a single one.  It may not do anything if it is
/// not able to allocate sufficient buffer space.
///
/// @note Logically, this function does not change the content of the
/// dictionary, but it actually changes a number of pointers.  The
/// implementation of the function contains a number of const_cast to allow
/// the pointers to be modified.  However, the function is declared as
/// const member function to allow it to be used in other const member
/// functions.
void ibis::dictionary::mergeBuffers() const {
    if (buffer_.size() <= 1) return; // nothing to do

    // determine the new buffer size
    size_t sz = 0;
    for (size_t j = 1; j < raw_.size(); ++ j) {
        const char *tmp = raw_[j];
        if (tmp != 0) {
            for (++ sz; *tmp != 0; ++ tmp) ++ sz;
        }
    }

    // allocate the new buffer
    char *newbuff = 0;
    try {
        newbuff = new char[sz];
    }
    catch (...) {
        LOGGER(ibis::gVerbose > 2)
            << "Warning -- dictionary::mergeBuffers failed to allocate "
            << sz << " bytes of contiguous memory, buffers not modified";
        return;
    }

    // copy content to the new buffer
    size_t jnew = 0;
    const_cast<dictionary*>(this)->key_.clear();
    for (size_t j = 1; j < raw_.size(); ++ j) {
        const char *tmp = raw_[j];
        if (tmp != 0) {
            char *copy = newbuff + jnew;
            while (*tmp != 0) {
                newbuff[jnew] = *tmp;
                ++ jnew;
                ++ tmp;
            }
            newbuff[jnew] = 0;
            const_cast<dictionary*>(this)->raw_[j] = copy;
            const_cast<dictionary*>(this)->key_[copy] = j;
            ++ jnew;
        }
    }
    // reclaim the old buffers
    for (size_t j = 0; j < buffer_.size(); ++ j)
        delete [] const_cast<dictionary*>(this)->buffer_[j];
    const_cast<dictionary*>(this)->buffer_.resize(1);
    const_cast<dictionary*>(this)->buffer_[0] = newbuff;
} // ibis::dictionary::mergeBuffers

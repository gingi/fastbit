//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
#include "dictionary.h"
#include "utilidor.h"

/// The header of dictionary files.  It has 20 bytes exactly.
static char _fastbit_dictionary_header[20] =
    {'#', 'I', 'B', 'I', 'S', ' ', 'D', 'i', 'c', 't',
     'i', 'o', 'n', 'a', 'r', 'y', 0, 0, 0, 0};

/// Copy constructor.  Places all the string in one contiguous buffer.
ibis::dictionary::dictionary(const ibis::dictionary& old)
    : raw_(old.raw_.size()), key_(old.key_.size()),
      code_(old.code_.size()), buffer_(1) {
    raw_[0] = 0;
    if (old.key_.empty() ||
	! (old.code_.size() == old.key_.size() &&
	   old.code_.size()+1 == old.raw_.size())) {
	buffer_[0] = 0;
	return;
    }

    const uint32_t nelm = old.key_.size();
    // find out the size of the buffer to allocate
    uint32_t sz = nelm;
    for (size_t i = 0; i < nelm; ++ i)
	sz += strlen(old.key_[i]);
    char *str = new char[sz];
    buffer_[0] = str;

    // copy the string values
    for (size_t i = 0; i < nelm; ++ i) {
	raw_[i+1] = str;
	key_[i] = str;
	code_[i] = i+1;
	for (const char *t = old.raw_[i+1]; *t != 0; ++ t, ++ str)
	    *str = *t;
	*str = 0;
	++ str;
    }
    ibis::util::sortStrings(key_, code_);
} // copy constructor

/// Compare whether this dicrionary and the other are equal in content.
/// The two dictionaries are considered same only if they have the same
/// keys and the the same integer representations.
bool ibis::dictionary::equal_to(const ibis::dictionary& other) const {
    if (key_.size() != other.key_.size() || key_.size() != code_.size() ||
	code_.size() != other.code_.size())
	return false;

    for (size_t j = 0; j < key_.size(); ++ j)
	if (strcmp(key_[j], other.key_[j]) != 0 || code_[j] != other.code_[j])
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
   file is as follows.

   \li Signature "#IBIS Dictionary " and version number (currently 0). (20
   bytes)

   \li N = Number of strings in the file. (4 bytes)

   \li uint32_t[N]: the integer values assigned to the strings.

   \li uint32_t[N+1]: the starting positions of the strings in this file.

   \li the string values one after the other with nil terminators.
*/
int ibis::dictionary::write(const char* name) const {
    if (name == 0 || *name == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write can not proceed with a "
	    "null string as the file name";
	return -1;
    }
    if (! (code_.size() == key_.size() &&
	   code_.size()+1 == raw_.size())) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") can not write an inconsistent dictionary, key_.size("
	    << key_.size() << "), code_.size(" << code_.size()
	    << "), raw_.size(" << raw_.size() << ")";
	return -2;
    }

    FILE* fptr = fopen(name, "wb");
    if (fptr == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") failed to open the file ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -3;
    }

    IBIS_BLOCK_GUARD(fclose, fptr);
    int ierr = fwrite(_fastbit_dictionary_header, 1, 20, fptr);
    if (ierr != 20) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") failed to write the header, fwrite returned " << ierr;
	return -4;
    }

    const uint32_t nkeys = key_.size();
    ierr = fwrite(&nkeys, sizeof(nkeys), 1, fptr);
    if (ierr != 1) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") failed to write the size(" << nkeys
	    << "), fwrite returned " << ierr;
	return -5;
    }
    if (nkeys == 0) // nothing else to write
	return 0;
    ierr = fwrite(code_.begin(), sizeof(uint32_t), nkeys, fptr);
    if (ierr != (int)nkeys) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") failed to write " << nkeys << " code value"
	    << (nkeys>1?"s":"") << ", fwrite returned " << ierr;
	return -6;
    }

    array_t<uint32_t> pos(nkeys+1);
    ierr = fseek(fptr, sizeof(uint32_t)*(nkeys+1), SEEK_CUR);
    long int tmp = ftell(fptr);
    pos[0] = tmp;
    for (unsigned i = 0; i < nkeys; ++ i) {
	const int len = 1 + strlen(key_[i]);
	ierr = fwrite(key_[i], 1, len, fptr);
	LOGGER(ierr != len && ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") failed to write key[" << i << "]; expected fwrite to return "
	    << len << ", but got " << ierr;

	tmp = ftell(fptr);
	pos[i+1] = tmp;
	LOGGER((long int)(pos[i+1]) != tmp && ibis::gVerbose > 1)
	    << "Warning -- dictionary::write(" << name
	    << ") failed to store position " << tmp
	    << " into a 32-bit integer; dictionary file will be unusable!";
    }

    // go back to write the positions
    ierr = fseek(fptr, 24+4*nkeys, SEEK_SET);
    LOGGER(ierr != 0 && ibis::gVerbose > 1)
	<< "Warning -- dictionary::write(" << name
	<< ") failed to seek to " << 24+4*nkeys
	<< " to write the offsets";

    ierr = fwrite(pos.begin(), 4, nkeys+1, fptr);
    LOGGER(ierr != (int)(nkeys+1) && ibis::gVerbose > 1)
	<< "Warning -- dictionary::write(" << name
	<< ") failed to write the offsets, expected fwrite to return "
	<< nkeys+1 << ", but got " << ierr;
    return -7 * (ierr != (int)(nkeys+1));
} // ibis::dictionary::write

/// Read the content of the named file.  The file content is read into the
/// buffer in one-shot and then digested.
int ibis::dictionary::read(const char* name) {
    if (name == 0 || *name == 0) return -1;
    std::string evt = "dictionary::read(";
    evt += name;
    evt += ')';
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

    long int sz = ftell(fptr); // file size
    if (sz < 24) { // must be the old style dictionary file
	return readRaw(evt.c_str(), fptr);
    }
    else {
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
	    header[15] == _fastbit_dictionary_header[15] &&
	    header[16] == _fastbit_dictionary_header[16] &&
	    header[17] == _fastbit_dictionary_header[17] &&
	    header[18] == _fastbit_dictionary_header[18] &&
	    header[19] == _fastbit_dictionary_header[19]) {
	    // got the expected header
	    return readKeys(evt.c_str(), fptr);
	}
	else {
	    LOGGER(ibis::gVerbose > 2)
		<< evt << " did not find the expected header, assume "
		"to be an old-style dictionary";
	    return readRaw(evt.c_str(), fptr);
	}
    }
} // ibis::dictionary::read

/// Read the raw strings.  This is the older style dictionary that contains
/// the raw strings.  On successful completion, this function returns 1.
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

    uint32_t cd = 1;
    const char *str = buffer_[0];
    const char *end = buffer_[0] + ierr;
    raw_.push_back(str);
    key_.push_back(str);
    code_.push_back(cd);
    do {
	while (*str != 0 && str < end) ++ str;
	if (*str == 0) {
	    ++ cd;
	    ++ str;
	    if (str < end) {
		raw_.push_back(str);
		key_.push_back(str);
		code_.push_back(cd);
		++ str;
	    }
	}
    } while (str < end);

    // sort the keys
    ibis::util::sortStrings(key_, code_);
    return 1;
} // ibis::dictionary::readRaw

/// Read the ordered strings.  This function process the data produced by
/// the write function.  On successful completion, it returns 0.
int ibis::dictionary::readKeys(const char *evt, FILE *fptr) {
    uint32_t nkeys;
    int ierr = fread(&nkeys, 4, 1, fptr);
    if (ierr != 1) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt
	    << " failed to read the number of keys, fread returned " << ierr;
	return -6;
    }

    clear();
    code_.resize(nkeys);
    ierr = fread(code_.begin(), 4, nkeys, fptr);
    if (ierr != (int)nkeys) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- " << evt << " failed to read the code values, "
	    "expected fread to return " << nkeys << ", but got " << ierr;
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
	    << "Warning -- " << evt << " failed to read the strings, expected "
	    "fread to return " << (offsets.back()-offsets.front())
	    << ", but got " << ierr;
	return -9;
    }
    raw_.resize(nkeys+1);
    key_.resize(nkeys);
    for (unsigned j = 0; j < nkeys; ++ j)
	key_[j] = buffer_[0] + (offsets[j] - offsets[0]);
    for (unsigned j = 0; j < nkeys; ++ j)
	raw_[code_[j]] = key_[j];
    return 0;
} // ibis::dictionary::readKeys

/// Clear the allocated memory.  Leave only the NULL entry.
void ibis::dictionary::clear() {
    for (size_t i = 0; i < buffer_.size(); ++ i)
	delete [] buffer_[i];
    buffer_.clear();
    code_.clear();
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
    if (! (code_.size() == key_.size() &&
	   key_.size()+1 == raw_.size())) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::patternSearch(" << pat
	    << ") can not proceed because the member variables have "
	    "inconsistent sizes: raw_.size(" << raw_.size() << ", key_.size("
	    << key_.size() << "), and code_.size(" << code_.size() << ')';
	return;
    }

#if FASTBIT_CS_PATTERN_MATCH+0 == 0
    for (char *ptr = const_cast<char*>(pat); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif
    // extract longest constant prefix to restrict range
    size_t pos;
    bool esc = false;
    bool meta = false;
    std::string prefix;
    const size_t len = strlen(pat);
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
	if (code != size() + 1) {
	    matches.push_back(code);
	}
	return;
    }

    // locate prefix to restrict matching range
    int32_t min = -1, max = -1;
    if (prefix.size() == 0) {
	min = 0;
	max = key_.size();
    }
    else if (key_.size() < 16) {
	// use linear search
	for (uint32_t m = 0; m < key_.size(); ++ m) {
	    if (min < 0) {
		int comp = strncmp(key_[m], prefix.c_str(), prefix.length());
		if (comp == 0) {
		    min = m;
		}
		else if (comp > 0) {
		    break;
		}
	    }
	    else if (max < 0) {
		if (strncmp(key_[m], prefix.c_str(), prefix.length()) != 0) {
		    max = m;
		    break;
		}
	    }
	}
	if (min < 0) return;
	if (max < 0) max = key_.size();
    }
    else {
	// find lower bound using binary search
	int32_t b = 0;
	int32_t e = key_.size() - 1;
	while (b <= e) {
	    int32_t m = (b + e) / 2;
	    if (strncmp(key_[m], prefix.c_str(), prefix.size()) >= 0) {
		e = m - 1;
	    }
	    else {
		b = m + 1;
	    }
	}

	if (b < (long)key_.size() &&
	    strncmp(key_[b], prefix.c_str(), prefix.size()) == 0) {
	    min = b;
	}
	else {
	    return;
	}

	// find upper bound using binary search
	e = key_.size() - 1;
	while (b <= e) {
	    int32_t m = (b + e) / 2;
	    if (strncmp(key_[m], prefix.c_str(), prefix.size()) > 0) {
		e = m - 1;
	    }
	    else {
		b = m + 1;
	    }
	}
	max = b;
    }

    // match values in the range
    for (int32_t j = min; j < max; ++ j) {
	if (ibis::util::strMatch(key_[j] + prefix.size(), pat + pos - 1)) {
	    matches.push_back(code_[j]);
	}
    }
    // // case insensitive, test all values
    // const uint32_t nd = key_.size();
    // for (uint32_t j = 0; j < nd; ++ j) {
    // 	if (ibis::util::strMatch(key_[j], pat)) {
    // 	    matches.push_back(code_[j]);
    // 	}
    // }
}

/// Convert a string to its integer code.  Returns 0 for empty (null)
/// strings, 1:size() for strings in the dictionary, and
/// dictionary::size()+1 for unknown values.
uint32_t ibis::dictionary::operator[](const char* str) const {
    if (str == 0) return 0;
    if (*str == 0) return 0;
    if (! (code_.size() == key_.size() &&
	   key_.size()+1 == raw_.size())) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::operator[" << str
	    << "] can not proceed because the member variables have "
	    "inconsistent sizes: raw_.size(" << raw_.size() << ", key_.size("
	    << key_.size() << "), and code_.size(" << code_.size() << ')';
	return 0;
    }
#if FASTBIT_CS_PATTERN_MATCH+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif

    if (key_.size() < 16) { // use linear search
	for (uint32_t m = 0; m < key_.size(); ++ m) {
	    const int cmp = strcmp(str, key_[m]);
	    if (cmp == 0) {
		return code_[m];
	    }
	    else if (cmp < 0) {
		return raw_.size()+1;
	    }
	}
    }
    else { // use binary search
	uint32_t b = 0;
	uint32_t e = key_.size();
	uint32_t m = (b+e)/2;
	while (b < m) {
	    const int cmp = strcmp(str, key_[m]);
	    if (cmp < 0) {
		e = m;
	    }
	    else if (cmp > 0) {
		b = m+1;
	    }
	    else {
		return code_[m];
	    }

	    m = (b + e) / 2;
	}

	if (m < key_.size() && strcmp(str, key_[m]) == 0) {
	    return code_[m];
	}
	else {
	    return raw_.size();
	}
    }
    return raw_.size();
} // string to int

/// Insert a string to the dictionary.  Returns the integer value assigned
/// to the string.  A copy of the string is stored internally.
uint32_t ibis::dictionary::insert(const char* str) {
    if (str == 0) return 0;
#ifdef FASTBIT_EMPTY_STRING_AS_NULL
    if (*str == 0) return 0;
#endif
    if (! (code_.size() == key_.size() &&
	   key_.size()+1 == raw_.size())) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::inert(" << str
	    << ") can not proceed because the member variables have "
	    "inconsistent sizes: raw_.size(" << raw_.size() << ", key_.size("
	    << key_.size() << "), and code_.size(" << code_.size() << ')';
	return 0;
    }

#if FASTBIT_CS_PATTERN_MATCH+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif
    uint32_t ind = 0;
    if (key_.size() < 16) { // use linear search
	for (ind = 0; ind < key_.size(); ++ ind) {
	    const int cmp = strcmp(str, key_[ind]);
	    if (cmp == 0) {
		return code_[ind];
	    }
	    else if (cmp < 0) {
		break;
	    }
	}
    }
    else { // use binary search
	uint32_t b = 0;
	uint32_t e = key_.size();
	ind = (b+e)/2;
	while (b < ind) {
	    const int cmp = strcmp(str, key_[ind]);
	    if (cmp < 0) {
		e = ind;
	    }
	    else if (cmp > 0) {
		b = ind+1;
	    }
	    else {
		return code_[ind];
	    }

	    ind = (b + e) / 2;
	}

	if (ind < key_.size()) {
	    const int cmp = strcmp(str, key_[ind]);
	    if (cmp == 0) {
		return code_[ind];
	    }
	    else if (cmp > 0) {
		++ ind;
	    }
	}
    }

    // incoming string is a new entry
    const uint32_t nk = raw_.size();
    char *copy = ibis::util::strnewdup(str);
    buffer_.push_back(copy);
    raw_.push_back(copy);
    if (key_.capacity() <= nk+1)
	key_.reserve(nk+nk);
    if (code_.capacity() <= nk+1)
	code_.reserve(nk+nk);
    key_.resize(nk);
    code_.resize(nk);
    for (uint32_t j = nk-1; j > ind; -- j) {
	key_[j] = key_[j-1];
	code_[j] = code_[j-1];
    }
    key_[ind] = copy;
    code_[ind] = nk;
#if defined(_DEBUG) || defined(DEBUG)
    bool ordered = true;
    for (unsigned j = 1; ordered && j < nk; ++ j)
	ordered = (strcmp(key_[j-1], key_[j]) <= 0);
    if (ordered == false && ibis::gVerbose >= 0) {
	ibis::util::logger lg;
	lg() << "Warning -- dictionary::insert(" << str
	     << ") incorrectly produced an unsorted list of keys";
	for (unsigned j = 0; j < nk; ++ j) {
	    lg() << "\nkey[" << j << "] = " << key_[j];
	    if (j > 0 && strcmp(key_[j-1], key_[j]) > 0)
		lg() << "\t<-- out of order";
	}
    }
#endif
    return nk;
} // ibis::dictionary::insert

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
    if (! (code_.size() == key_.size() &&
	   key_.size()+1 == raw_.size())) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::inertRaw(" << str
	    << ") can not proceed because the member variables have "
	    "inconsistent sizes: raw_.size(" << raw_.size() << ", key_.size("
	    << key_.size() << "), and code_.size(" << code_.size() << ')';
	return 0;
    }
#if FASTBIT_CS_PATTERN_MATCH+0 == 0
    for (char *ptr = const_cast<char*>(str); *ptr != 0; ++ ptr) {
	*ptr = toupper(*ptr);
    }
#endif

    uint32_t ind = 0;
    if (key_.size() < 16) { // use linear search
	for (ind = 0; ind < key_.size(); ++ ind) {
	    const int cmp = strcmp(str, key_[ind]);
	    if (cmp == 0) {
		return code_[ind];
	    }
	    else if (cmp < 0) {
		break;
	    }
	}
    }
    else { // use binary search
	uint32_t b = 0;
	uint32_t e = key_.size();
	ind = (b+e)/2;
	while (b < ind) {
	    const int cmp = strcmp(str, key_[ind]);
	    if (cmp < 0) {
		e = ind;
	    }
	    else if (cmp > 0) {
		b = ind+1;
	    }
	    else {
		return code_[ind];
	    }

	    ind = (b + e) / 2;
	}

	if (ind < key_.size()) {
	    const int cmp = strcmp(str, key_[ind]);
	    if (cmp == 0) {
		return code_[ind];
	    }
	    else if (cmp > 0) {
		++ ind;
	    }
	}
    }

    // incoming string is a new entry
    const uint32_t nk = raw_.size();
    buffer_.push_back(str);
    raw_.push_back(str);
    key_.resize(nk);
    code_.resize(nk);
    for (uint32_t j = nk-1; j > ind; -- j) {
	key_[j] = key_[j-1];
	code_[j] = code_[j-1];
    }
    key_[ind] = str;
    code_[ind] = nk;
    return nk;
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
    const size_t nelm = code_.size();
    o2n.resize(nelm+1);
    o2n[0] = 0;  // 0 is always mapped to 0
    for (uint32_t j = 0; j < nelm; ++ j) {
	raw_[j+1] = key_[j];
	o2n[code_[j]] = j+1;
	code_[j] = j+1;
    }
} // ibis::dictionary::sort

/// Merge the incoming dictionary with this one.  It produces a dictionary
/// that combines the words in both dictionaries and keep the words in
/// ascending order.
///
/// Upon successful completion of this function, the return value will be
/// the new size of the dictionary, i.e., the number of non-empty words.
/// It returns a negative value to indicate error.
int ibis::dictionary::merge(const ibis::dictionary& rhs) {
    const uint32_t nt = key_.size();
    const uint32_t nr = rhs.key_.size();
    if (nr == 0) {
	return key_.size();
    }
    if (nt == 0) {
	ibis::dictionary tmp(rhs);
	swap(tmp);
	return key_.size();
    } // ibis::dictionary::merge

    
    uint32_t jt = 0;
    uint32_t jr = 0;
    array_t<uint32_t> code2;
    array_t<const char*> raw2, key2;
    try {
	code2.reserve(nr+nt);
	raw2.reserve(nr+nt+1);
	key2.reserve(nr+nt);
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::merge failed to allocate temporary "
	    "arrays";
	return -1;
    }

    // the linear merge loop
    while (jt < nt && jr < nr) {
	int cmp = strcmp(key_[jt], rhs.key_[jr]);
	if (cmp <= 0) {
	    key2.push_back(key_[jt]);
	    ++ jt;
	    jr += (cmp == 0);
	}
	else {
	    char *cp = ibis::util::strnewdup(rhs.key_[jr]);
	    if (cp == 0) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- dictionary::merge failed to allocate "
		    "memory for a new string value";
		return -2;
	    }
	    buffer_.push_back(cp);
	    key2.push_back(cp);
	    ++ jr;
	}
    } // (jt < nt && jr < nr)

    while (jt < nt) {
	key2.push_back(key_[jt]);
	++ jt;
    }
    while (jr < nr) {
	char *cp = ibis::util::strnewdup(rhs.key_[jr]);
	if (cp == 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- dictionary::merge failed to allocate "
		"memory for a new string value";
	    return -3;
	}
	buffer_.push_back(cp);
	key2.push_back(cp);
	++ jr;
    }

    raw2.push_back(0);
    for (jt = 0; jt < key2.size(); ++ jt) {
	raw2.push_back(key2[jt]);
	code2.push_back(jt+1);
    }
    raw2.swap(raw_);
    key2.swap(key_);
    code2.swap(code_);
    return key_.size();
} // ibis::dictionary::merge

/// Produce an array that mapps the integers in old dictionary to the new
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

    uint32_t j1 = 0, j0 = 0;
    while (j0 < nold && j1 < nnew) {
	int cmp = strcmp(key_[j1], old.key_[j0]);
	if (cmp < 0) {
	    ++ j1;
	}
	else if (cmp == 0) {
	    o2n[old.code_[j0]] = code_[j1];
	    ++ j0;
	    ++ j1;
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- dictionary::morph can not find \""
		<< old.key_[j0] << "\" in the new dictionary";
	    return -2;
	}
    }
    if (j0 < nold) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- dictionary::morph exhausted the new dictionary "
	    "entries but only found " << j0 << " out of " << nold
	    << " entr" << (nold>1?"ies":"y") << " from the old one";
	return -3;
    }
    return j0;
} // ibis::dictioniary::morph
    

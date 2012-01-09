// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
//
// This file contains the implementation of the class called ibis::slice.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "irelic.h"
#include "part.h"
#include "column.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////
// functions from ibis::islice
//
// construct a bitmap index from current data
ibis::slice::slice(const ibis::column* c, const char* f) : ibis::relic(0) {
    if (c == 0) return;  // nothing can be done
    col = c;
    try {
	if (c->partition()->nRows() < 1000000) {
	    construct1(f);
	}
	else {
	    construct2(f);
	}
	if (ibis::gVerbose > 2) {
	    ibis::util::logger lg;
	    const uint32_t card = vals.size();
	    const uint32_t nbits = bits.size();
	    lg() << "slice[" << col->partition()->name() << '.' << col->name()
		 << "]::ctor -- constructed a bit-sliced index with " << nbits
		 << " bitmap" << (nbits>1?"s":"") << " on " << card
		 << " distinct value" << (card>1?"s":"") << " and " << nrows
		 << " row" << (nrows>1?"s":"");
	    if (ibis::gVerbose > 6) {
		lg() << "\n";
		print(lg());
	    }
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- slice[" << col->partition()->name() << '.'
	    << col->name() << "]::ctor receiveed an exception, cleaning up ...";
	clear();
	throw;
    }
} // constructor

/// Reconstruct from content of a storage object.
/// The content of the file (following the 8-byte header) is
///@code
/// nrows(uint32_t)       -- the number of bits in each bit sequence
/// nobs (uint32_t)       -- the number of bit sequences
/// card (uint32_t)       -- the number of distinct values, i.e., cardinality
/// (padding to ensure the next data element is on 8-byte boundary)
/// values (double[card]) -- the distinct values as doubles
/// offset ([nobs+1])     -- the starting positions of the bit sequences (as
///				bit vectors)
/// cnts (uint32_t[card]) -- the counts for each distinct value
/// bitvectors            -- the bitvectors one after another
///@endcode
ibis::slice::slice(const ibis::column* c, ibis::fileManager::storage* st,
		   size_t start)
    : ibis::relic(c, st, start),
      cnts(st, 8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(int32_t)*
	   (ibis::relic::bits.size()+1)+
	   sizeof(double)*ibis::relic::vals.size(),
	   8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(int32_t)*
	   (ibis::relic::bits.size()+1)+
	   sizeof(double)*ibis::relic::vals.size()+
	   sizeof(uint32_t)*(*(reinterpret_cast<uint32_t*>
			       (st->begin()+start+sizeof(uint32_t))))) {
    try {
	activate(); // always activate all bitvectors
	if (ibis::gVerbose > 2) {
	    ibis::util::logger lg;
	    const uint32_t card = vals.size();
	    const uint32_t nbits = bits.size();
	    lg() << "slice[" << col->partition()->name() << '.' << col->name()
		 << "]::ctor -- intialized a bit-sliced index with " << nbits
		 << " bitmap" << (nbits>1?"s":"") << " on " << card
		 << " distinct value" << (card>1?"s":"") << " and " << nrows
		 << " row" << (nrows>1?"s":"") << " from storage object @ "
		 << st << " offset " << start;
	    if (ibis::gVerbose > 6) {
		lg() << "\n";
		print(lg());
	    }
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- slice[" << col->partition()->name() << '.'
	    << col->name() << "]::ctor received an exception, cleaning up ...";
	clear();
	throw;
    }
}

// the argument is the name of the directory or the file name
int ibis::slice::write(const char* dt) const {
    if (vals.empty()) return -1;

    std::string fnm;
    indexFileName(dt, fnm);

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITENEW, OPEN_FILEMODE);
    if (fdes < 0) {
	ibis::fileManager::instance().flushFile(fnm.c_str());
	fdes = UnixOpen(fnm.c_str(), OPEN_WRITENEW, OPEN_FILEMODE);
	if (fdes < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- slice[" << col->partition()->name() << '.'
		<< col->name() << "]::write failed to open \"" << fnm
		<< "\" for writing";
	    return -2;
	}
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

#ifdef FASTBIT_USE_LONG_OFFSETS
    const bool useoffset64 = true;
#else
    const bool useoffset64 = (getSerialSize()+8 > 0x80000000UL);
#endif
    char header[] = "#IBIS\11\0\0";
    header[5] = (char)ibis::index::SLICE;
    header[6] = (char)(useoffset64 ? 8 : 4);
    int ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- slice[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    if (useoffset64)
	ierr = write64(fdes);
    else
	ierr = write32(fdes);
    if (ierr >= 0) {
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
	(void) UnixFlush(fdes); // write to disk
#endif

	LOGGER(ibis::gVerbose > 3)
	    << "slice[" << col->partition()->name() << "." << col->name()
	    << "]::write wrote " << bits.size() << " bitmap"
	    << (bits.size()>1?"s":"") << " to file " << fnm;
    }
    return ierr;
} // ibis::slice::write

/// Write the content to a file opened by the caller.  This function uses
/// 32-bit bitmap offsets.
int ibis::slice::write32(int fdes) const {
    if (vals.empty()) return -4;
    std::string evt = "slice";
    if (ibis::gVerbose > 0) {
	evt += '[';
	evt += col->partition()->name();
	evt += '.';
	evt += col->name();
	evt += ']';
    }
    evt += "::write32";

    const off_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " seek(" << fdes 
	    << ", 0, SEEK_CUR) returned " << start
	    << ", but a value >= 8 is expected";
	return -5;
    }

    const uint32_t card = vals.size();
    const uint32_t nbits = bits.size();
    off_t ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr += UnixWrite(fdes, &nbits, sizeof(uint32_t));
    ierr += UnixWrite(fdes, &card, sizeof(uint32_t));
    if (ierr < 12) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects to write 3 4-byte words to "
	    << fdes << ", but the number of byte wrote is " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -6;
    }

    offset64.clear();
    offset32.resize(nbits+1);
    offset32[0] = 8*((start+sizeof(uint32_t)*3+7)/8);
    ierr = UnixSeek(fdes, offset32[0], SEEK_SET);
    if (ierr != offset32[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " seek(" << fdes << ", " << offset32[0]
	    << ", SEEK_SET) returned " << ierr;
	UnixSeek(fdes, start, SEEK_SET);
	return -7;
    }

    ierr = UnixWrite(fdes, vals.begin(), sizeof(double)*card);
    if (ierr < static_cast<off_t>(sizeof(double)*card)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expected to write "
	    << sizeof(double)*card << " bytes to file descriptor "
	    << fdes << ", but actually wrote " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -8;
    }

    offset32[0] += sizeof(double) * card +sizeof(int32_t) * nbits
	+ sizeof(int32_t);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nbits+1), SEEK_CUR);
    if (ierr != offset32[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " attempting to seek to " << offset32[0]
	    << " file descriptor " << fdes << " returned " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -9;
    }
    ierr = UnixWrite(fdes, cnts.begin(), sizeof(uint32_t)*card);
    if (ierr < static_cast<off_t>(sizeof(uint32_t)*card)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expected to write "
	    << sizeof(int32_t)*card << " bytes to file descriptor "
	    << fdes << ", but actually wrote " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -10;
    }
    offset32[0] += sizeof(uint32_t)*card;
    for (uint32_t i = 0; i < nbits; ++i) {
	bits[i]->write(fdes);
	offset32[i+1] = UnixSeek(fdes, 0, SEEK_CUR);
    }

    const off_t offpos = 8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(double)*card;
    ierr = UnixSeek(fdes, offpos, SEEK_SET);
    if (ierr != offpos) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " seek(" << fdes << ", " << offpos
	    << ", SEEK_SET) returned " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -11;
    }
    ierr = UnixWrite(fdes, offset32.begin(), sizeof(int32_t)*(nbits+1));
    if (ierr < static_cast<off_t>(sizeof(int32_t)*(nbits+1))) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expected to write "
	    << sizeof(int32_t)*(nbits+1) << " bytes to file descriptor "
	    << fdes << ", but actually wrote " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -12;
    }
    ierr = UnixSeek(fdes, offset32.back(), SEEK_SET);
    return (ierr == offset32[nbits] ? 0 : -13);
} // ibis::slice::write32

/// Write the content to a file opened by the caller.  This function uses
/// 64-bit bitmap offsets.
int ibis::slice::write64(int fdes) const {
    if (vals.empty()) return -4;
    std::string evt = "slice";
    if (ibis::gVerbose > 0) {
	evt += '[';
	evt += col->partition()->name();
	evt += '.';
	evt += col->name();
	evt += ']';
    }
    evt += "::write64";

    const off_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " seek(" << fdes 
	    << ", 0, SEEK_CUR) returned " << start
	    << ", but a value >= 8 is expected";
	return -5;
    }

    const uint32_t card = vals.size();
    const uint32_t nbits = bits.size();
    off_t ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr += UnixWrite(fdes, &nbits, sizeof(uint32_t));
    ierr += UnixWrite(fdes, &card, sizeof(uint32_t));
    if (ierr < 12) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects to write 3 4-byte words to "
	    << fdes << ", but the number of byte wrote is " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -6;
    }

    offset32.clear();
    offset64.resize(nbits+1);
    offset64[0] = 8*((start+sizeof(uint32_t)*3+7)/8);
    ierr = UnixSeek(fdes, offset64[0], SEEK_SET);
    if (ierr != offset64[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " seek(" << fdes << ", " << offset64[0]
	    << ", SEEK_SET) returned " << ierr;
	UnixSeek(fdes, start, SEEK_SET);
	return -7;
    }

    ierr = UnixWrite(fdes, vals.begin(), sizeof(double)*card);
    if (ierr < static_cast<off_t>(sizeof(double)*card)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expected to write "
	    << sizeof(double)*card << " bytes to file descriptor "
	    << fdes << ", but actually wrote " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -8;
    }

    offset64[0] += sizeof(double) * card + sizeof(int64_t) * nbits
	+ sizeof(int64_t);
    ierr = UnixSeek(fdes, sizeof(int64_t)*(nbits+1), SEEK_CUR);
    if (ierr != offset64[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " attempting to seek to " << offset64[0]
	    << " file descriptor " << fdes << " returned " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -9;
    }
    ierr = UnixWrite(fdes, cnts.begin(), sizeof(uint32_t)*card);
    if (ierr < static_cast<off_t>(sizeof(uint32_t)*card)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expected to write "
	    << sizeof(int32_t)*card << " bytes to file descriptor "
	    << fdes << ", but actually wrote " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -10;
    }
    offset64[0] += sizeof(uint32_t)*card;
    for (uint32_t i = 0; i < nbits; ++i) {
	bits[i]->write(fdes);
	offset64[i+1] = UnixSeek(fdes, 0, SEEK_CUR);
    }

    const off_t offpos = 8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(double)*card;
    ierr = UnixSeek(fdes, offpos, SEEK_SET);
    if (ierr != offpos) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " seek(" << fdes << ", " << offpos
	    << ", SEEK_SET) returned " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -11;
    }
    ierr = UnixWrite(fdes, offset64.begin(), sizeof(int64_t)*(nbits+1));
    if (ierr < static_cast<off_t>(sizeof(int64_t)*(nbits+1))) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expected to write "
	    << sizeof(int64_t)*(nbits+1) << " bytes to file descriptor "
	    << fdes << ", but actually wrote " << ierr;
	(void) UnixSeek(fdes, start, SEEK_SET);
	return -12;
    }
    ierr = UnixSeek(fdes, offset64.back(), SEEK_SET);
    return (ierr == offset64[nbits] ? 0 : -13);
} // ibis::slice::write64

/// Read the index contained in the file f.  This function always reads all
/// bitvectors.
int ibis::slice::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);

    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- slice[" << col->partition()->name() << '.'
	    << col->name() << "]::read failed to open " << fnm;
	return -1; // can not do anything else
    }

    char header[8];
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    int ierr = UnixRead(fdes, static_cast<void*>(header), 8);
    if (ierr != 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- slice[" << col->partition()->name() << '.'
	    << col->name() << "]::read failed to read 8 bytes from "
	    << fnm;
	return -2;
    }
    if (!(header[0] == '#' && header[1] == 'I' &&
	  header[2] == 'B' && header[3] == 'I' &&
	  header[4] == 'S' &&
	  header[5] == static_cast<char>(ibis::index::SLICE) &&
	  (header[6] == 8 || header[6] == 4) &&
	  header[7] == static_cast<char>(0))) {
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg() << "Warning -- slice[" << col->partition()->name() << '.'
		 << col->name() << "]::read the header from " << fnm << " (";
	    if (isprint(header[0]) != 0)
		lg() << header[0];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[0]
			    << std::dec;
	    if (isprint(header[1]) != 0)
		lg() << header[1];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[1]
			    << std::dec;
	    if (isprint(header[2]) != 0)
		lg() << header[2];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[2] << std::dec;
	    if (isprint(header[3]) != 0)
		lg() << header[3];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[3] << std::dec;
	    if (isprint(header[4]) != 0)
		lg() << header[4];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[4] << std::dec;
	    if (isprint(header[5]) != 0)
		lg() << header[5];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[5] << std::dec;
	    if (isprint(header[6]) != 0)
		lg() << header[6];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[6] << std::dec;
	    if (isprint(header[7]) != 0)
		lg() << header[7];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[7] << std::dec;
	    lg() << ") does not contain the expected values";
	}
	return -3;
    }

    uint32_t dim[3]; // nrows, nobs, card
    size_t begin, end;
    clear(); // clear the current content

    ierr = UnixRead(fdes, static_cast<void*>(dim), 3*sizeof(uint32_t));
    nrows = dim[0];
    // read vals
    begin = 8*((3*sizeof(uint32_t) + 15) / 8);
    end = begin + dim[2] * sizeof(double);
    {
	array_t<double> dbl(fnm.c_str(), fdes, begin, end);
	vals.swap(dbl);
    }
    // read the offsets
    begin = end;
    end += header[6] * (dim[1] + 1);
    ierr = initOffsets(fdes, header[6], begin, dim[1]);
    if (ierr < 0)
	return ierr;

    // cnts
    begin = end;
    end += sizeof(uint32_t) * dim[2];
    {
	array_t<uint32_t> szt(fnm.c_str(), fdes, begin, end);
	cnts.swap(szt);
    }
    ibis::fileManager::instance().recordPages(0, end);

    initBitmaps(fdes);
    activate();
    return 0;
} // ibis::slice::read

/// Reconstruct an index from a piece of consecutive memory.  Unlike the
/// implementations for other type indices, this function always reads all
/// bit vectors.
int ibis::slice::read(ibis::fileManager::storage* st) {
    if (st == 0) return -1;
    clear(); // clear the current conent

    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    uint32_t pos = 8+sizeof(uint32_t);
    const uint32_t nobs = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t);
    const uint32_t card = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t) + 7;
    pos = 8 * (pos / 8);
    int ierr = initOffsets(st, pos + sizeof(double)*card, nobs);
    if (ierr < 0)
	return ierr;

    { // limit the scope of dbl
	array_t<double> dbl(st, pos, card);
	vals.swap(dbl);
    }
    {
	pos += sizeof(double)*card + sizeof(int32_t)*(nobs+1);
	array_t<uint32_t> szt(st, pos, card);
	cnts.swap(szt);
    }

    initBitmaps(st);
    activate();
    return 0;
} // ibis::slice::read

/// Free the memory hold by this object.
void ibis::slice::clear() {
    cnts.clear();
    ibis::relic::clear();
} // ibis::slice::clear

// assume that the array vals is initialized properly, this function
// converts the value val into a set of bits to be stored in the bitvectors
// contained in bits
// **** CAN ONLY be used by construct2() to build a new bit-sliced index ****
void ibis::slice::setBit(const uint32_t i, const double val) {
    if (val > vals.back()) return;
    if (val < vals[0]) return;

    // perform a binary search to locate position of val in vals
    uint32_t ii = 0, jj = vals.size() - 1;
    uint32_t kk = (ii + jj) / 2;
    while (kk > ii) {
	if (vals[kk] < val)
	    ii = kk;
	else
	    jj = kk;
	kk = (ii + jj) / 2;
    }

    if (vals[jj] == val) { // vals[jj] is the same as val
	ii = 0;
	while (jj > 0) {
	    if (jj % 2)
		bits[ii]->setBit(i, 1);
	    jj >>= 1;
	    ++ ii;
	}
    }
    else if (vals[ii] == val) { // vals[ii] is the same as val
	jj = 0;
	while (ii > 0) {
	    if (ii % 2)
		bits[ii]->setBit(i, 1);
	    ii >>= 1;
	    ++ jj;
	}
    }
} // ibis::slice::setBit

/// Construct an index.  It takes one pass through the data to produce a
/// list of values and their corresponding locations (as bitvectors), then
/// transforms the bitvectors into those in binary encoding.
void ibis::slice::construct1(const char* f) {
    VMap bmap; // a map between values and their position
    try {
	mapValues(f, bmap);
    }
    catch (...) { // need to clean up bmap
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- slice[" << col->partition()->name() << '.'
	    << col->name() << "]::construct reclaiming storage "
	    "allocated to bitvectors (" << bmap.size() << ")";

	for (VMap::iterator it = bmap.begin(); it != bmap.end(); ++ it)
	    delete (*it).second;
	bmap.clear();
	ibis::fileManager::instance().signalMemoryAvailable();
	throw;
    }
    if (bmap.empty()) return;

    // fill the arrays vals and cnts
    uint32_t tmp = bmap.size();
    vals.resize(tmp);
    cnts.resize(tmp);
    VMap::const_iterator it = bmap.begin();
    for (uint32_t i = 0; i < tmp; ++i, ++it) {
	vals[i] = (*it).first;
	cnts[i] = (*it).second->cnt();
    }

    // determine the number of bits needed for the bit-sliced index
    -- tmp;
    uint32_t nobs = 0;
    while (tmp > 0) {
	++ nobs;
	tmp >>= 1;
    }
    if (nobs == 0)
	nobs = 1;
    bits.resize(nobs);
    // initialize all bitvectors in bits to contain only zero bits
    it = bmap.begin();
    nrows = (*it).second->size();
    for (uint32_t i = 0; i < nobs; ++i) {
	bits[i] = new ibis::bitvector();
	bits[i]->set(0, nrows);
	if (nobs > 10) // expected to be operated on more than 100 times
	    bits[i]->decompress();
    }
    delete (*it).second;
    LOGGER(ibis::gVerbose > 5)
	<< "slice[" << col->partition()->name() << '.' << col->name()
	<< "]::construct initialized the array of bitvectors, "
	<< "start converting " << vals.size() << " bitmaps into "
	<< nobs << " bit slices";

    // fill the bitvectors for the bit-sliced index
    for (tmp = 1, ++it; it != bmap.end(); ++it, ++tmp) {
	uint32_t b = tmp;
	for (uint32_t i = 0; i < nobs && b > 0; ++i, b >>= 1) {
	    if (b % 2 > 0) {
		*(bits[i]) |= *((*it).second);
	    }
	}
	delete (*it).second; // no longer need this one
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	if (ibis::gVerbose > 11 && (tmp & 1023) == 1023) {
	    LOGGER(ibis::gVerbose >= 0) << tmp << " ... ";
	}
#endif
    }
#if DEBUG+0 > 0 || _DEBUG+0 > 0
    if (ibis::gVerbose > 11) {
	LOGGER(ibis::gVerbose >= 0) << "DONE";
    }
#endif
    // attempt to compress all the bitmaps
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i]->compress();
    optionalUnpack(bits, col->indexSpec());

    // write out the current content to standard output
    if (ibis::gVerbose > 4) {
 	ibis::util::logger lg;
 	print(lg());
    }
} // ibis::slice::construct1

/// generate a new bit-sliced index.  This version performs its task in two
/// steps.
/// - scan the data to generate a list of distinct values and their counts.
/// - scan the data a second time to produce the bit vectors.
void ibis::slice::construct2(const char* f) {
    uint32_t tmp;
    {
	histogram hst;
	mapValues(f, hst); // scan the data to produce the histogram
	if (hst.empty()) // no data, of course no index
	    return;

	// convert histogram into two arrays
	tmp = hst.size();
	vals.resize(tmp);
	cnts.resize(tmp);
	histogram::const_iterator it = hst.begin();
	for (uint32_t i = 0; i < tmp; ++i) {
	    vals[i] = (*it).first;
	    cnts[i] = (*it).second;
	    ++ it;
	}
    }
    
    // allocate the correct number of bitvectors
    -- tmp;
    uint32_t nobs = 0;
    while (tmp > 0) {
	tmp >>= 1;
	++ nobs;
    }
    if (nobs == 0)
	nobs = 1;
    bits.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i] = new ibis::bitvector;

    std::string fnm; // name of the data file
    dataFileName(f, fnm);

    nrows = col->partition()->nRows();
    ibis::bitvector mask;
    {   // name of mask file associated with the data file
	array_t<ibis::bitvector::word_t> arr;
	std::string mname(fnm);
	mname += ".msk";
	if (ibis::fileManager::instance().getFile(mname.c_str(), arr) == 0)
	    mask.copy(ibis::bitvector(arr)); // convert arr to a bitvector
	else
	    mask.set(1, nrows); // default mask
    }

    // need to do different things for different columns
    switch (col->type()) {
    case ibis::TEXT:
    case ibis::UINT: {// unsigned int
	array_t<uint32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::ULONG: {// unsigned long int
	array_t<uint64_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::LONG: {// signed long int
	array_t<int64_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::USHORT: {// unsigned short int
	array_t<uint16_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::SHORT: {// signed short int
	array_t<int16_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::UBYTE: {// unsigned char
	array_t<uint32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::BYTE: {// signed char
	array_t<signed char> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("slice::construct", "the data file \"%s\" "
				"contains more elements (%lu) then expected "
				"(%lu)", fnm.c_str(),
				static_cast<long unsigned>(val.size()),
				static_cast<long unsigned>(mask.size()));
		mask.adjustSize(nrows, nrows);
	    }
	    ibis::bitvector::indexSet iset = mask.firstIndexSet();
	    uint32_t nind = iset.nIndices();
	    const ibis::bitvector::word_t *iix = iset.indices();
	    while (nind) {
		if (iset.isRange()) { // a range
		    uint32_t k = (iix[1] < nrows ? iix[1] : nrows);
		    for (uint32_t i = *iix; i < k; ++i)
			setBit(i, val[i]);
		}
		else if (*iix+ibis::bitvector::bitsPerLiteral() < nrows) {
		    // a list of indices
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			setBit(k, val[k]);
		    }
		}
		else {
		    for (uint32_t i = 0; i < nind; ++i) {
			uint32_t k = iix[i];
			if (k < nrows)
			    setBit(k, val[k]);
		    }
		}
		++iset;
		nind = iset.nIndices();
		if (*iix >= nrows)
		    nind = 0;
	    } // while (nind)
	}
	else {
	    col->logWarning("slice::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::CATEGORY: // no need for a separate index
	col->logWarning("slice::ctor", "no need for another index");
	return;
    default:
	col->logWarning("slice::ctor", "unable to create bit slice index "
			"for this type of column");
	return;
    }

    // make sure all bit vectors are the same size
    for (uint32_t i = 0; i < nobs; ++i) {
	bits[i]->adjustSize(0, nrows);
	bits[i]->compress();
    }

    optionalUnpack(bits, col->indexSpec()); // uncompress the bitmaps
    // write out the current content to standard output
    if (ibis::gVerbose > 4) {
 	ibis::util::logger lg;
 	print(lg());
    }
} // ibis::slice::construct2

// a simple function to test the speed of the bitvector operations
void ibis::slice::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    uint32_t i, nloops = 1000000000 / nrows;
    activate(); // activate all bitmaps
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;
    col->logMessage("slice::speedTest", "testing the speed of operator &");

    for (i = 0; i < bits.size()-1; ++i) {
	ibis::bitvector* tmp;
	tmp = *(bits[i+1]) & *(bits[i]);
	delete tmp;

	timer.start();
	for (uint32_t j=0; j<nloops; ++j) {
	    tmp = *(bits[i+1]) | *(bits[i]);
	    delete tmp;
	}
	timer.stop();
	{
	    ibis::util::ioLock lock;
	    out << bits[i]->size() << " "
		<< static_cast<double>(bits[i]->bytes() + bits[i+1]->bytes())
		* 4.0 / static_cast<double>(bits[i]->size()) << " "
		<< bits[i]->cnt() << " " << bits[i+1]->cnt() << " "
		<< timer.realTime() / nloops << "\n";
	}
    }
} // ibis::slice::speedTest

// the printing function
void ibis::slice::print(std::ostream& out) const {
    out << "index(slice) for " << col->partition()->name() << '.'
	<< col->name() << " contains " << bits.size() << " bitvectors for "
	<< nrows << " objects \n";
    const uint32_t nobs = bits.size();
    if (nobs > 0) { // the short form
	out << "bitvector information (number of set bits, number "
	    << "of bytes)\n";
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i])
		out << i << '\t' << bits[i]->cnt() << '\t'
		    << bits[i]->bytes() << "\n";
	}
    }
    if (ibis::gVerbose > 6) { // also print the list of distinct values
	out << "distinct values, number of apparences\n";
	for (uint32_t i=0; i<vals.size(); ++i) {
	    out.precision(12);
	    out << vals[i] << '\t' << cnts[i] << "\n";
	}
    }
    out << "\n";
} // ibis::slice::print

// create index based data in dt -- have to start from data directly
long ibis::slice::append(const char* dt, const char* df, uint32_t nnew) {
    clear();		// clear the current content
    construct2(dt);	// generate the new version of the index
    //write(dt);		// write out the new content
    return nnew;
} // ibis::slice::append

// compute the bitvector that is the answer for the query x >= b
void ibis::slice::evalGE(ibis::bitvector& res, uint32_t b) const {
    if (b >= vals.size()) {
	res.set(0, nrows);
    }
    else if (b > 0) {
	uint32_t i = 0;
	while (b % 2 == 0) {
	    b >>= 1;
	    ++ i;
	}
	if (bits[i])
	    res.copy(*(bits[i]));
	else
	    res.set(0, nrows);
	b >>= 1;
	++ i;
	while (b > 0) {
	    if (b % 2 > 0) {
		if (bits[i])
		    res &= *(bits[i]);
		else
		    res.set(0, nrows);
	    }
	    else if (bits[i]) {
		res |= *(bits[i]);
	    }
	    b >>= 1;
	    ++ i;
	}
	while (i < bits.size()) {
	    if (bits[i])
		res |= *(bits[i]);
	    ++ i;
	}
    }
    else {
	res.set(1, nrows);
    }
} // evalGE

// compute the bitvector that is the answer for the query x = b
void ibis::slice::evalEQ(ibis::bitvector& res, uint32_t b) const {
    if (b >= vals.size()) {
	res.set(0, nrows);
    }
    else {
	res.set(1, nrows);
	for (uint32_t i=0; i < bits.size(); ++i) {
	    if (b % 2 > 0) {
		if (bits[i])
		    res &= *(bits[i]);
		else
		    res.set(0, nrows);
	    }
	    else if (bits[i]) {
		res -= *(bits[i]);
	    }
	    b >>= 1;
	}
    }
} // evalEQ

// Evaluate a continuous range expression
long ibis::slice::evaluate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower) const {
    if (bits.empty()) {
	lower.set(0, nrows);
	return 0L;
    }

    // values in the range [hit0, hit1) satisfy the query expression
    uint32_t hit0, hit1;
    locate(expr, hit0, hit1);

    // actually accumulate the bits in the range [hit0, hit1)
    if (hit0 >= hit1) {
	lower.set(0, nrows);
    }
    else if (hit0+1 == hit1) { // equal to one single value
	evalEQ(lower, hit0);
    }
    else if (hit1 == vals.size()) { // >= hit0
	evalGE(lower, hit0);
    }
    else if (hit0 == 0) { // < hit1 (translates to NOT (>= hit1))
	evalGE(lower, hit1);
	lower.flip();
    }
    else { // need to go through most bitvectors twice
	ibis::bitvector upper;
	evalGE(lower, hit0); // lower := (>= hit0)
	evalGE(upper, hit1); // upper := (>= hit1)
	lower -= upper;      // lower := (>= hit0) AND NOT (>= hit1)
    }
    return lower.cnt();
} // ibis::slice::evaluate

// Evaluate a set of discrete range conditions.
long ibis::slice::evaluate(const ibis::qDiscreteRange& expr,
			   ibis::bitvector& lower) const {
    const ibis::array_t<double>& varr = expr.getValues();
    lower.set(0, nrows);
    for (unsigned i = 0; i < varr.size(); ++ i) {
	unsigned int itmp = locate(varr[i]);
	if (itmp > 0 && vals[itmp-1] == varr[i]) {
	    -- itmp;
	    ibis::bitvector tmp;
	    evalEQ(tmp, itmp);
	    if (tmp.size() == lower.size())
		lower |= tmp;
	}
    }
    return lower.cnt();
} // ibis::slice::evaluate

void ibis::slice::estimate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower,
			   ibis::bitvector& upper) const {
    if (bits.empty()) {
	lower.set(0, nrows);
	upper.clear();
	return;
    }

    // values in the range [hit0, hit1) satisfy the query expression
    uint32_t hit0, hit1;
    locate(expr, hit0, hit1);

    // actually accumulate the bits in the range [hit0, hit1)
    if (hit0 >= hit1) {
	lower.set(0, nrows);
    }
    else if (hit0+1 == hit1) { // equal to one single value
	evalEQ(lower, hit0);
    }
    else if (hit1 == vals.size()) { // >= hit0
	evalGE(lower, hit0);
    }
    else if (hit0 == 0) { // < hit1 (translates to NOT (>= hit1))
	evalGE(lower, hit1);
	lower.flip();
    }
    else { // need to go through most bitvectors twice
	evalGE(lower, hit0); // lower := (>= hit0)
	evalGE(upper, hit1); // upper := (>= hit1)
	lower -= upper;      // lower := (>= hit0) AND NOT (>= hit1)
    }
    upper.clear();
} // ibis::slice::estimate

void ibis::slice::binWeights(std::vector<uint32_t>& c) const {
    c.resize(cnts.size());
    for (uint32_t i = 0; i < cnts.size(); ++ i) {
	c[i] = cnts[i];
    }
} // ibis::slice::binWeights

// return the number of hits
uint32_t ibis::slice::estimate(const ibis::qContinuousRange& expr) const {
    if (bits.empty()) return 0;

    uint32_t h0, h1;
    locate(expr, h0, h1);

    uint32_t nhits = 0;
    for (uint32_t i=h0; i<h1; ++i)
	nhits += cnts[i];
    return nhits;
} // ibis::slice::estimate()

double ibis::slice::getSum() const {
    double ret = 0;
    if (vals.size() == cnts.size()) {
	for (uint32_t i = 0; i < vals.size(); ++ i)
	    ret += vals[i] * cnts[i];
    }
    else {
	col->logWarning("slice::getSum", "internal error - arrays "
			"vals[%lu] and cnts[%lu] are expected to have "
			"the same size but are not",
			static_cast<long unsigned>(vals.size()),
			static_cast<long unsigned>(cnts.size()));
	ibis::util::setNaN(ret);
    }
    return ret;
} // ibis::slice::getSum

/// Estimate the size of the index in a file.
size_t ibis::slice::getSerialSize() const throw() {
    size_t res = 24 + 8 * vals.size() + 12 * bits.size();
    for (unsigned j = 0; j < bits.size(); ++ j)
	if (bits[j] != 0)
	    res += bits[j]->getSerialSize();
    return res;
} // ibis::slice::getSerialSize

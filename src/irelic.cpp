// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2008 the Regents of the University of California
//
// This file contains the implementation of the classes defined in index.h
// The primary function from the database point of view is a functioin
// called estimate().  It evaluates a given range condition and produces
// two bit vectors representing the range where the actual solution lies.
// The bulk of the code is devoted to maintain and update the index called
// ibis::relic.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifiers longer than 256 characters
#endif
#include "bitvector64.h"
#include "irelic.h"
#include "part.h"

#include <cmath>	// std::fabs
#include <sstream> // std::ostringstream

////////////////////////////////////////////////////////////////////////
// functions from ibis::irelic
//
// construct a bitmap index from current data
ibis::relic::relic(const ibis::column* c, const char* f)
    : ibis::index(c) {
    if (c == 0) return;  // nothing can be done

    try {
	if (f) {
	    read(f);
	}
	if (vals.empty() && c->partition()->nRows() > 0 &&
	    c->type() != ibis::CATEGORY &&
	    c->type() != ibis::TEXT) {
	    construct(f);
	    if (ibis::gVerbose > 5) {
		ibis::util::logger lg(5);
		print(lg.buffer());
	    }
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning -- ibis::column[" << col->name()
	    << "]::relic::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // constructor

/// Construct a dummy index.  All entries have the same value @c popu.
/// This is used to generate index for meta tags from STAR data.
ibis::relic::relic(const ibis::column* c, uint32_t popu, uint32_t ntpl)
    : ibis::index(c) {
    if (c == 0) return; // must has a valid column
    try {
	if (ntpl == 0)
	    ntpl = c->partition()->nRows();
	nrows = ntpl;
	vals.resize(1);
	bits.resize(1);
	vals[0] = popu;
	bits[0] = new ibis::bitvector();
	bits[0]->set(1, ntpl);
	if (ibis::gVerbose > 5) {
	    ibis::util::logger lg(5);
	    print(lg.buffer());
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning -- ibis::column[" << col->name()
	    << "]::relic::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // constructor for dummy attributes

/// Construct an index from an integer array.
ibis::relic::relic(const ibis::column* c, uint32_t card,
		   array_t<uint32_t>& ind) : ibis::index(c) {
    if (c == 0) return;
    if (ind.empty()) return;

    try {
	vals.resize(card);
	bits.resize(card);
	for (uint32_t i = 0; i < card; ++i) {
	    vals[i] = i;
	    bits[i] = new ibis::bitvector();
	}
	nrows = ind.size();
	for (uint32_t i = 0; i < nrows; ++i) {
	    if (ind[i] < card) {
		bits[ind[i]]->setBit(i, 1);
	    }
#if defined(DEBUG) && DEBUG + 0 > 1
	    else {
		LOGGER(ibis::gVerbose >= 0)
		    << "ind[" << i << "]=" << ind[i] << " >=" << card;
	    }
#endif
	}
	for (uint32_t i = 0; i < card; ++i) {
	    bits[i]->adjustSize(0, nrows);
	}
	if (ibis::gVerbose > 5) {
	    ibis::util::logger lg(5);
	    print(lg.buffer());
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning -- ibis::column[" << col->name()
	    << "]::relic::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // construct an index from an integer array

/// Reconstruct from content of fileManager::storage.
/**
   The content of the file (following the 8-byte header) is
   nrows(uint32_t) -- number of bits in each bit sequences
   nobs (uint32_t) -- number of bit sequences
   card (uint32_t) -- the number of distinct values, i.e., cardinality
   (padding to ensure the next data element is on 8-byte boundary)
   values (double[card]) -- the values as doubles
   offset (uint32_t[nobs+1]) -- the starting positions of the bit sequences (as
   bit vectors)
   bitvectors -- the bitvectors one after another
*/
ibis::relic::relic(const ibis::column* c, ibis::fileManager::storage* st,
		   uint32_t start)
    : ibis::index(c, st),
      vals(st, 8*((3 * sizeof(uint32_t) + start + 7)/8),
	   *(reinterpret_cast<uint32_t*>(st->begin() + start +
				       2*sizeof(uint32_t)))) {
    try {
	nrows = *(reinterpret_cast<uint32_t*>(st->begin()+start));
	start += sizeof(uint32_t);
	const uint32_t nobs =
	    *(reinterpret_cast<uint32_t*>(st->begin()+start));
	start += sizeof(uint32_t);
	const uint32_t card =
	    *(reinterpret_cast<uint32_t*>(st->begin()+start));
	start += sizeof(uint32_t);
	array_t<int32_t> offs(st, 8*((start+7)/8)
			    + sizeof(double)*card, nobs+1);
	offsets.copy(offs);
	bits.resize(nobs);
	for (uint32_t i = 0; i < nobs; ++i)
	    bits[i] = 0;
#if defined(DEBUG)
    if (ibis::gVerbose > 5) {
	unsigned nprt = (ibis::gVerbose < 30 ? (1 << ibis::gVerbose) : nobs);
	if (nprt > nobs)
	    nprt = nobs;
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- ibis::relic::relic("
		    << (st->filename()?st->filename():"unnamed")
		    << ") got nobs = " << nobs << ", card = " << card
		    << ", the offsets of the bit vectors (starting at "
		    <<  8*((start+7)/8)+sizeof(double)*card
		    << ") are\n";
	for (unsigned i = 0; i < nprt; ++ i)
	    lg.buffer() << offsets[i] << " ";
	if (nprt < nobs)
	    lg.buffer() << "... (skipping " << nobs-nprt << ") ... ";
	lg.buffer() << offsets[nobs];
    }
#endif
	if (st->isFileMap()) { // only map the first bitvector
#if defined(ALWAY_READ_BITVECTOR0)
	    array_t<ibis::bitvector::word_t>
		a0(st, offs[0],
		   (offs[1]-offs[0])/sizeof(ibis::bitvector::word_t));
	    bits[0] = new ibis::bitvector(a0);
	    bits[0]->setSize(nrows);
#endif
	    str = st;
	}
	else { // all bytes in memory, generate all bitvectors
	    for (uint32_t i = 0; i < nobs; ++i) {
		if (offs[i+1] > offs[i]) {
		    array_t<ibis::bitvector::word_t>
			a(st, offs[i], (offs[i+1]-offs[i])/
			  sizeof(ibis::bitvector::word_t));
		    ibis::bitvector* btmp = new ibis::bitvector(a);
		    btmp->setSize(nrows);
		    bits[i] = btmp;
		}
		else if (ibis::gVerbose > 0) {
		    c->logWarning("readIndex", "bitvector %lu is invalid "
				  "(offsets[%lu]=%lu, offsets[%lu]=%lu)",
				  static_cast<long unsigned>(i),
				  static_cast<long unsigned>(i),
				  static_cast<long unsigned>(offs[i]),
				  static_cast<long unsigned>(i+1),
				  static_cast<long unsigned>(offs[i+1]));
		}
	    }

	    str = 0;

	    if (ibis::gVerbose > 8 &&
		static_cast<INDEX_TYPE>(*(st->begin()+5)) == RELIC) {
		ibis::util::logger lg(8);
		print(lg.buffer());
	    }
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning -- ibis::column[" << col->name()
	    << "]::relic::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // constructor

// the argument is the name of the directory, the file name is
// column::name() + ".idx"
int ibis::relic::write(const char* dt) const {
    if (vals.empty() || bits.empty() || nrows == 0) return -1;
    if (vals.size() != bits.size()) {
	if (ibis::gVerbose > 0)
	    col->logMessage("relic::write", "vals.size(%lu) and bits.size"
			    "(%lu) are expected to be the same, but are not",
			    static_cast<long unsigned>(vals.size()),
			    static_cast<long unsigned>(bits.size()));
	return -1;
    }

    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;
    if (fname != 0 || str != 0)
	activate(); // activate all bitvectors

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	col->logWarning("relic::write", "unable to open \"%s\" for write",
			fnm.c_str());
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    int32_t ierr = 0;
    const uint32_t nobs = vals.size();

    array_t<int32_t> offs(nobs+1);
    char header[] = "#IBIS\7\0\0";
    header[5] = (char)ibis::index::RELIC;
    header[6] = (char)sizeof(int32_t);
    ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::column[" << col->partition()->name() << "."
	    << col->name() << "]::relic::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr = write(fdes); // write the bulk of the index file
#if _POSIX_FSYNC+0 > 0
    (void) fsync(fdes); // write to disk
#endif
    (void) UnixClose(fdes);

    if (ibis::gVerbose > 5)
	col->logMessage("relic::write", "wrote %lu bitmap%s to %s",
			static_cast<long unsigned>(nobs),
			(nobs>1?"s":""), fnm.c_str());
    return ierr;
} // ibis::relic::write

// write the content to a file already opened
int ibis::relic::write(int fdes) const {
    if (vals.empty() || bits.empty() || nrows == 0)
	return -4;

    const uint32_t nobs = (vals.size()<=bits.size()?vals.size():bits.size());
    const int32_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	ibis::util::logMessage("Warning", "ibis::relic::write call to UnixSeek"
			       "(%d, 0, SEEK_CUR) failed ... ", fdes,
			       strerror(errno));
	return -5;
    }

    int32_t ierr;
    array_t<int32_t> offs(nobs+1);
    ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    offs[0] = 8*((7+start+3*sizeof(uint32_t))/8);
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) {
	UnixSeek(fdes, start, SEEK_SET);
	LOGGER(ibis::gVerbose >= 1)
	    << "ibis::relic::write(" << fdes << ") failed to seek to "
	    << offs[0];
	return -6;
    }

    ierr = UnixWrite(fdes, vals.begin(), sizeof(double)*nobs);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nobs+1), SEEK_CUR);
    for (uint32_t i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	if (bits[i]) {
	    bits[i]->write(fdes);
	    if (bits[i]->size() != nrows)
		col->logWarning("relic::write", "bits[%lu] has %lu bits, "
				"expected %lu", static_cast<long unsigned>(i),
				static_cast<long unsigned>(bits[i]->size()),
				static_cast<long unsigned>(nrows));
	}
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes,
		    8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(double)*nobs,
		    SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
    ierr = UnixSeek(fdes, offs[nobs], SEEK_SET); // move to the end
#if defined(DEBUG)
    ibis::util::logger lg(4);
    lg.buffer() << "DEBUG -- ibis::relic::write wrote " << nobs
		<< " bitvectors, with the following file offsets (written to "
		<< 8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(double)*nobs
		<< ")\n";
    for (unsigned i = 0; i <= nobs; ++ i)
	lg.buffer() << offs[i] << " ";
#endif
    return 0;
} // ibis::relic::write

// read the index contained in the file f
int ibis::relic::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);

    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0) return -1;

    char header[8];
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    if (8 != UnixRead(fdes, static_cast<void*>(header), 8)) {
	UnixClose(fdes);
	return -2;
    }

    if (false == (header[0] == '#' && header[1] == 'I' &&
		  header[2] == 'B' && header[3] == 'I' &&
		  header[4] == 'S' &&
		  header[6] == static_cast<char>(sizeof(int32_t)) &&
		  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return -3;
    }

    uint32_t dim[3];
    uint32_t begin, end;
    clear(); // clear the current content
    fname = ibis::util::strnewdup(fnm.c_str());

    int ierr = UnixRead(fdes, static_cast<void*>(dim), 3*sizeof(uint32_t));
    if (ierr < static_cast<int>(3*sizeof(uint32_t))) {
	if (ibis::gVerbose > 1)
	    col->logWarning("relic::read", "file %s has a correct header, "
			    "but does not hold an index", fnm.c_str());
	UnixClose(fdes);
	return -4;
    }

    nrows = dim[0];
    bool trymmap = false;
#if defined(HAS_FILE_MAP)
    trymmap = (dim[2] > ibis::fileManager::pageSize());
#endif
    // read vals
    begin = 8*((3*sizeof(uint32_t) + 15) / 8);
    end = begin + dim[2] * sizeof(double);
    if (trymmap) {
	// try to use memory map to enable reading only the needed values
	array_t<double> dbl(fname, begin, end);
	vals.swap(dbl);
    }
    else { // read all values into memory
	array_t<double> dbl(fdes, begin, end);
	vals.swap(dbl);
    }
    // read the offsets
    begin = end;
    end += sizeof(int32_t) * (dim[1] + 1);
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }
    ibis::fileManager::instance().recordPages(0, end);
#if defined(DEBUG)
    if (ibis::gVerbose > 5) {
	unsigned nprt = (ibis::gVerbose < 30 ? (1 << ibis::gVerbose) : dim[1]);
	if (nprt > dim[1])
	    nprt = dim[1];
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- ibis::relic::read(" << f
		    << ") got nobs = " << dim[1] << ", card = " << dim[2]
		  << ", the offsets of the bit vectors are\n";
	for (unsigned i = 0; i < nprt; ++ i)
	    lg.buffer() << offsets[i] << " ";
	if (nprt < dim[1])
	    lg.buffer() << "... (skipping " << dim[1]-nprt << ") ... ";
	lg.buffer() << offsets[dim[1]];
    }
#endif

    bits.resize(dim[1]);
    for (uint32_t i = 0; i < dim[1]; ++i)
	bits[i] = 0;
#if defined(ALWAY_READ_BITVECTOR0)
    // read the first bitvector
    if (offsets[1] > offsets[0]) {
	array_t<ibis::bitvector::word_t> a0(fdes, offsets[0], offsets[1]);
	bits[0] = new ibis::bitvector(a0);
	bits[0]->setSize(nrows);
    }
    else {
	bits[0] = new ibis::bitvector;
	bits[0]->set(0, nrows);
    }
#endif
    (void) UnixClose(fdes);
    str = 0;
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fnm.c_str());
    return 0;
} // ibis::relic::read

// attempt to reconstruct an index from a piece of consecutive memory
int ibis::relic::read(ibis::fileManager::storage* st) {
    if (st == 0) return -1;
    ibis::index::clear();

    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    uint32_t pos = 8 + sizeof(uint32_t);
    const uint32_t nobs = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t);
    const uint32_t card = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t) + 7;
    {
	array_t<int32_t> offs(st, 8*(pos/8) + sizeof(double)*card, nobs+1);
	offsets.swap(offs);
	array_t<double> dbl(st, 8*(pos/8), card);
	vals.swap(dbl);
    }

    for (uint32_t i = 0; i < bits.size(); ++ i)
	delete bits[i];
    bits.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i] = 0;
    if (st->isFileMap()) { // only restore the first bitvector
#if defined(ALWAY_READ_BITVECTOR0)
	if (offsets[1] > offsets[0]) {
	    array_t<ibis::bitvector::word_t>
		a0(st, offsets[0], (offsets[1]-offsets[0])/
		   sizeof(ibis::bitvector::word_t));
	    bits[0] = new ibis::bitvector(a0);
	    bits[0]->setSize(nrows);
	}
	else {
	    bits[0] = new ibis::bitvector;
	    bits[0]->adjustSize(0, nrows);
	}
#endif
	str = st;
    }
    else { // regenerate all the bitvectors
	for (uint32_t i = 0; i < nobs; ++i) {
	    if (offsets[i+1] > offsets[i]) {
		array_t<ibis::bitvector::word_t>
		    a(st, offsets[i], (offsets[i+1]-offsets[i])/
		      sizeof(ibis::bitvector::word_t));
		ibis::bitvector* btmp = new ibis::bitvector(a);
		btmp->setSize(nrows);
		bits[i] = btmp;
	    }
	    else if (ibis::gVerbose > 0) {
		col->logWarning("relic::read", "bitvector %lu is invalid "
				"(offsets[%lu]=%lu, offsets[%lu]=%lu)",
				static_cast<long unsigned>(i),
				static_cast<long unsigned>(i),
				static_cast<long unsigned>(offsets[i]),
				static_cast<long unsigned>(i+1),
				static_cast<long unsigned>(offsets[i+1]));
	    }
	}
	str = 0;
    }
    return 0;
} // ibis::relic::read

void ibis::relic::clear() {
    vals.clear();
    ibis::index::clear();
} // ibis::relic::clear

// generate a new index -- the basic bitmap index, one bitmap per
// distinct value.
// the string f can be the name of the index file (the corresponding data file
// is assumed to be without the '.idx' suffix), the name of the data file, or
// the directory contain the data file
void ibis::relic::construct(const char* f) {
    VMap bmap;
    try {
	mapValues(f, bmap);
    }
    catch (...) { // need to clean up bmap
	LOGGER(ibis::gVerbose >= 1)
	    << "ibis::relic::construct reclaiming storage "
	    "allocated to bitvectors (" << bmap.size() << ")";

	for (VMap::iterator it = bmap.begin(); it != bmap.end(); ++ it)
	    delete (*it).second;
	bmap.clear();
	ibis::fileManager::instance().signalMemoryAvailable();
	throw; // rethrow the exception
    }
    if (bmap.empty()) return; // bmap is empty

    uint32_t i;
    // copy the pointer in VMap into two linear structures
    const uint32_t nobs = bmap.size();
    bits.resize(nobs);
    vals.resize(nobs);
    VMap::const_iterator it;
    for (it = bmap.begin(); nrows == 0 && it != bmap.end(); ++ it)
	if ((*it).second)
	    nrows = (*it).second->size();
    for (i = 0, it = bmap.begin(); i < nobs; ++ it, ++ i) {
	vals[i] = (*it).first;
	bits[i] = (*it).second;
    }
    optionalUnpack(bits, col->indexSpec());

    // write out the current content
    if (ibis::gVerbose > 6) {
 	ibis::util::logger lg(6);
 	print(lg.buffer());
    }
} // ibis::relic::construct

template <typename E>
void ibis::relic::construct(const array_t<E>& arr) {
    VMap bmap;
    nrows = arr.size();
    try {
	mapValues(arr, bmap);
    }
    catch (...) { // need to clean up bmap
	LOGGER(ibis::gVerbose >= 1)
	    << "ibis::relic::construct reclaiming storage "
	    "allocated to bitvectors (" << bmap.size() << ")";

	for (VMap::iterator it = bmap.begin(); it != bmap.end(); ++ it)
	    delete (*it).second;
	bmap.clear();
	ibis::fileManager::instance().signalMemoryAvailable();
	throw;
    }
    if (bmap.empty()) return; // bmap is empty

    // copy the pointer in VMap into two linear structures
    const uint32_t nobs = bmap.size();
    bits.resize(nobs);
    vals.resize(nobs);
    uint32_t i;
    VMap::const_iterator it;
    for (i = 0, it = bmap.begin(); i < nobs; ++it, ++i) {
	vals[i] = (*it).first;
	bits[i] = (*it).second;
    }
    optionalUnpack(bits, col->indexSpec());

    // write out the current content
    if (ibis::gVerbose > 6) {
 	ibis::util::logger lg(6);
 	print(lg.buffer());
    }
} // ibis::relic::construct

// explicit instantiation of construct function
template void ibis::relic::construct(const array_t<char>& arr);
template void ibis::relic::construct(const array_t<unsigned char>& arr);
template void ibis::relic::construct(const array_t<int16_t>& arr);
template void ibis::relic::construct(const array_t<uint16_t>& arr);
template void ibis::relic::construct(const array_t<int32_t>& arr);
template void ibis::relic::construct(const array_t<uint32_t>& arr);
template void ibis::relic::construct(const array_t<int64_t>& arr);
template void ibis::relic::construct(const array_t<uint64_t>& arr);
template void ibis::relic::construct(const array_t<float>& arr);
template void ibis::relic::construct(const array_t<double>& arr);

// a simple function to test the speed of the bitvector operations
void ibis::relic::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    uint32_t nloops = 1000000000 / nrows;
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;

    try {
	activate(); // activate all bitvectors
    }
    catch (const std::exception& e) {
	col->logWarning("relic::speedTest", "received a standard "
			"exception - %s", e.what());
	return;
    }
    catch (const char* s) {
	col->logWarning("relic::speedTest", "received a string exception - %s",
			s);
	return;
    }
    catch (...) {
	col->logWarning("relic::speedTest", "received an unexpected "
			"exception");
	return;
    }
    bool crossproduct = false;
    {
	std::string which = col->partition()->name();
	which += ".";
	which += col->name();
	which += ".measureCrossProduct";
	crossproduct = ibis::gParameters().isTrue(which.c_str());
    }
    if (crossproduct) {
	col->logMessage("relic::speedTest", "testing the speed of cross "
			"product operation\n# bits, # 1s, # 1s, # bytes, "
			"# bytes, clustering factor, result 1s, result "
			"bytes, wall time");
	nloops = 2;
    }
    else {
	col->logMessage("relic::speedTest", "testing the speed of operator "
			"|\n# bits, # 1s, # 1s, # bytes, # bytes, "
			"clustering factor, result 1s, result bytes, "
			"wall time");
    }

    for (unsigned i = 1; i < bits.size(); ++i) {
	if (bits[i-1] == 0 || bits[i] == 0)
	    continue;
	int64_t ocnt, osize; // the size of the output of operation
	ibis::bitvector* tmp;
	try {
	    tmp = *(bits[i-1]) | *(bits[i]);
	    osize = tmp->bytes();
	    ocnt = tmp->cnt();
	    delete tmp;
	}
	catch (const std::exception& e) {
	    col->logWarning("relic::speedTest", "received a standard "
			    "exception while calling operator | (i=%u) - %s",
			    i, e.what());
	    continue;
	}
	catch (const char* s) {
	    col->logWarning("relic::speedTest", "received a string exception "
			    "while calling operator | (i=%u) - %s", i, s);
	    continue;
	}
	catch (...) {
	    col->logWarning("relic::speedTest", "received an unexpected "
			    "exception while calling operator | (i=%u)", i);
	    continue;
	}

	try {
	    const double cf = ibis::bitvector::clusteringFactor
		(bits[i]->size(), bits[i]->cnt(), bits[i]->bytes());
	    timer.start();
	    if (crossproduct) {
		for (uint32_t j=0; j<nloops; ++j) {
		    ibis::bitvector64 t64;
		    ibis::outerProduct(*(bits[i-1]), *(bits[i]), t64);
		    osize = t64.bytes();
		    ocnt = t64.cnt();
		}
	    }
	    else {
		for (uint32_t j=0; j<nloops; ++j) {
		    tmp = *(bits[i-1]) | *(bits[i]);
		    delete tmp;
		}
	    }
	    timer.stop();
	    ibis::util::ioLock lock;
	    out << bits[i]->size() << ", "
		<< bits[i-1]->cnt() << ", " << bits[i]->cnt() << ", "
		<< bits[i-1]->bytes() << ", " << bits[i]->bytes() << ", "
		<< cf << ", " << ocnt << ", " << osize << ", "
		<< timer.realTime() / nloops << "\n";
	}
	catch (...) {
	}
    }
} // ibis::relic::speedTest()

// the printing function
void ibis::relic::print(std::ostream& out) const {
    if (vals.size() != bits.size() || bits.empty())
	return;

    out << "the basic bitmap index for "
	<< col->partition()->name() << '.'
	<< col->name() << " contains " << bits.size()
	<< " bitvectors for " << nrows << " objects";
    const uint32_t nobs = bits.size();
    uint32_t skip = 0;
    if (ibis::gVerbose <= 0) {
	skip = nobs;
    }
    else if ((nobs >> (2*ibis::gVerbose)) > 2) {
	skip = static_cast<uint32_t>
	    (ibis::util::compactValue
	     (static_cast<double>(nobs >> (1+2*ibis::gVerbose)),
	      static_cast<double>(nobs >> (2*ibis::gVerbose))));
	if (skip < 1)
	    skip = 1;
    }
    if (skip < 1)
	skip = 1;
    if (skip > 1) {
	out << " (printing 1 out of every " << skip << ")";
    }
    out << "\n";

    for (uint32_t i=0; i<nobs; i += skip) {
	if (bits[i]) {
	    out << i << ":\t";
	    out.precision(12);
	    out << vals[i] << "\t" << bits[i]->cnt()
		<< "\t" << bits[i]->bytes() << "\n";
	}
	else if (ibis::gVerbose > 7) {
	    out << i << ":\t";
	    out.precision(12);
	    out << vals[i] << " ... \n";
	}
    }
    if ((nobs-1) % skip) {
	if (bits[nobs-1]) {
	    out << nobs-1 << ":\t";
	    out << vals[nobs-1] << "\t" << bits[nobs-1]->cnt()
		<< "\t" << bits[nobs-1]->bytes() << "\n";
	}
	else if (ibis::gVerbose > 7) {
	    out << nobs-1 << ":\t";
	    out << vals[nobs-1] << " ... \n";
	}
    }
    out << "\n";
} // ibis::relic::print

// convert the bitvector mask into bin indices -- used by
// ibis::category::selectUInt
array_t<uint32_t>* ibis::relic::keys(const ibis::bitvector& mask) const {
    if (mask.cnt() == 0) // nothing to do
	return 0;

    ibis::bitvector* tmp = 0;
    uint32_t nobs = bits.size();
    std::map<uint32_t, uint32_t> ii;

    activate(); // need all bitvectors to be in memory
    for (uint32_t i = 0; i < nobs; ++i) { // loop to generate ii
	if (bits[i]) {
	    if (bits[i]->size() == mask.size()) {
		uint32_t nind = 0;
		tmp = mask & *(bits[i]);
		ibis::bitvector::indexSet is = tmp->firstIndexSet();
		const ibis::bitvector::word_t *iix = is.indices();
		nind = is.nIndices();
		while (nind > 0) {
		    if (is.isRange()) {
			for (uint32_t j = *iix; j < iix[1]; ++j)
			    ii[j] = static_cast<uint32_t>(vals[i]);
		    }
		    else if (nind > 0) {
			for  (uint32_t j = 0; j < nind; ++j)
			    ii[iix[j]] = static_cast<uint32_t>(vals[i]);
		    }

		    ++ is;
		    nind =is.nIndices();
		}
		delete tmp;
	    }
	    else if (ibis::gVerbose > 2) {
		col->logMessage("relic::keys", "bits[%lu]->size()=%lu, "
				"mask.size()=%lu",
				static_cast<long unsigned>(i),
				static_cast<long unsigned>(bits[i]->size()),
				static_cast<long unsigned>(mask.size()));
	    }
	}
	else if (ibis::gVerbose > 4) {
	    col->logMessage("relic::keys", "bits[%lu] can not be activated",
			    static_cast<long unsigned>(i));
	}
    }

    array_t<uint32_t>* ret = new array_t<uint32_t>(ii.size());
    std::map<uint32_t, uint32_t>::const_iterator it = ii.begin();
    for (uint32_t i = 0; i < ii.size(); ++i) {
	(*ret)[i] = (*it).second;
	++ it;
    }
    if (ret->empty()) {
	col->logWarning("relic::keys", "not able to compute the keys most "
			"likely because the index has changed, please "
			"re-evaluate the query.");
    }
    return ret;
} // ibis::relic::keys

/// Append a list of integers.  The integers are treated as bin numbers.
/// This function is primarily used by ibis::category::append().
long ibis::relic::append(const array_t<uint32_t>& ind) {
    if (ind.empty()) return 0; // nothing to do
    uint32_t i, nobs = bits.size();
    activate(); // need all bitvectors to be in memory
    for (i = 0; i < ind.size(); ++ i, ++ nrows) {
	const uint32_t j = ind[i];
	if (j >= nobs) { // extend the number of bitvectors
	    for (uint32_t k = nobs; k <= j; ++ k) {
		bits.push_back(new ibis::bitvector);
		vals.push_back(k);
	    }
	    nobs = bits.size();
	}
	bits[j]->setBit(nrows, 1);
    }

    uint32_t nset = 0;
    for (i = 0; i < nobs; ++i) {
	bits[i]->adjustSize(0, nrows);
	nset += bits[i]->cnt();
    }
    if (nset != nrows)
	col->logMessage("relic::append", "new index contains %lu objects "
			"(!= bitmap length %lu)",
			static_cast<long unsigned>(nset),
			static_cast<long unsigned>(nrows));
    return ind.size();
} // ibis::relic::append

// create index based data in df and append the result to the index in dt
long ibis::relic::append(const char* dt, const char* df, uint32_t nnew) {
    std::string fnm;
    indexFileName(df, fnm);

    ibis::relic* bin0=0;
    ibis::fileManager::storage* st0=0;
    int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), &st0);
    if (ierr == 0 && st0 != 0) {
	const char* header = st0->begin();
	if (header[0] == '#' && header[1] == 'I' && header[2] == 'B' &&
	    header[3] == 'I' && header[4] == 'S' &&
	    header[5] == ibis::index::RELIC &&
	    header[6] == static_cast<char>(sizeof(int32_t)) &&
	    header[7] == static_cast<char>(0)) {
	    bin0 = new ibis::relic(col, st0);
	}
	else {
	    if (ibis::gVerbose > 5)
		col->logMessage("relic::append", "file \"%s\" has "
				"unexecpted header -- it will be removed",
				fnm.c_str());
	    ibis::fileManager::instance().flushFile(fnm.c_str());
	    remove(fnm.c_str());
	}
    }
    if (bin0 == 0) {
	if (col->type() == ibis::TEXT) {
	    fnm.erase(fnm.size()-3);
	    fnm += "int";
	    if (ibis::util::getFileSize(fnm.c_str()) > 0)
		bin0 = new ibis::relic(col, fnm.c_str());
	    else {
		col->logWarning("relic::append", "file \"%s\" must exist "
				"before calling this function",
				fnm.c_str());
		ierr = -2;
		return ierr;
	    }
	}
	else {
	    bin0 = new ibis::relic(col, df);
	}
    }

    if (bin0) {
	ierr = append(*bin0);
	delete bin0;
	if (ierr == 0) {
	    write(dt); // write out the new content
	    return nnew;
	}
	else {
	    return ierr;
	}
    }
    else {
	col->logWarning("relic::append", "failed to generate index with "
			"data from %s", df);
	return -6;
    }
} // ibis::relic::append

// this function first convert *this into a map then back into the linear
// data structure
long ibis::relic::append(const ibis::relic& tail) {
    if (tail.col != col) return -1;
    if (tail.bits.empty()) return -3;

    activate(); // need all bitvectors;
    tail.activate();

    unsigned i;
    uint32_t nobs = bits.size();
    const uint32_t n0 = nrows;
    std::map<double, ibis::bitvector*> bmap;
    std::map<double, ibis::bitvector*>::iterator it;
    // copy *this into bmap, make another copy just in case of the current
    // bitmap index is built on top of file maps
    for (i = 0; i < nobs; ++i) {
	ibis::bitvector* tmp = new ibis::bitvector();
	if (bits[i] != 0) {
	    tmp->copy(*(bits[i]));
	    bmap[vals[i]] = tmp;
	}
	else {
	    col->logWarning("relic::append", "bits[%u] (<==> %g) is nil, "
			    "assume it is no longer needed", i, vals[i]);
	    delete tmp;
	}
    }
    clear(); // clear the current content

    // combine the two sets of bitmaps
    for (i = 0; i < tail.vals.size(); ++i) {
	if (tail.bits[i] != 0 && tail.bits[i]->size() > 0) {
	    it = bmap.find(tail.vals[i]);
	    if (it != bmap.end()) {
		(*it).second->operator+=(*(tail.bits[i]));
	    }
	    else if (n0 > 0) {
		ibis::bitvector* tmp = new ibis::bitvector();
		tmp->set(0, n0);
		tmp->operator+=(*(tail.bits[i]));
		bmap[tail.vals[i]] = tmp;
	    }
	    else {
		ibis::bitvector* tmp = new ibis::bitvector();
		tmp->copy(*(tail.bits[i]));
		bmap[tail.vals[i]] = tmp;
	    }
	}
    }

    // nobs --> count only those values that actually appeared
    nobs = 0;
    uint32_t nset = 0;
    const uint32_t totbits = n0 + tail.nrows;
    for (it = bmap.begin(); it != bmap.end(); ++it) {
	(*it).second->adjustSize(0, totbits);
	nobs += ((*it).second->cnt() > 0);
	nset += (*it).second->cnt();
	if (ibis::gVerbose > 18)
	    col->logMessage("relic::append", "value %g appeared %lu times "
			    "out of %lu", (*it).first,
			    static_cast<long unsigned>((*it).second->cnt()),
			    static_cast<long unsigned>(totbits));
    }
    if (nset != totbits && ibis::gVerbose > 0)
	col->logMessage("relic::append", "new index contains %lu objects "
			"(!= bitmap length %lu)",
			static_cast<long unsigned>(nset),
			static_cast<long unsigned>(totbits));
    nrows = totbits;
    // convert from bmap to the linear structure
    bits.resize(nobs);
    vals.resize(nobs);
    for (i = 0, it = bmap.begin(); it != bmap.end(); ++it) {
	if ((*it).second->cnt() > 0) {
	    vals[i] = (*it).first;
	    bits[i] = (*it).second;
	    ++ i;
	}
	else {
	    delete (*it).second;
	}
    }

    if (ibis::gVerbose > 10) {
	ibis::util::logger lg(10);
	lg.buffer() << "\nNew combined index (append an index for "
		    << tail.nrows
		    << " objects to an index for " << n0 << " events\n" ;
	print(lg.buffer());
    }
    return 0;
} // ibis::relic::append

/// Find the smallest i such that vals[i] > val.
uint32_t ibis::relic::locate(const double& val) const {
    // check the extreme cases -- use negative tests to capture abnormal
    // numbers
    const uint32_t nval = vals.size();
    if (nval <= 0) return 0;
    if (! (val >= vals[0])) {
	return 0;
    }
    else if (! (val < vals[nval-1])) {
	if (vals[nval-1] < DBL_MAX)
	    return nval;
	else
	    return nval - 1;
    }

    // the normal cases -- two different search strategies
    if (nval >= 8) { // binary search
	uint32_t i0 = 0, i1 = nval, it = nval/2;
	while (i0 < it) { // vals[i1] >= val
	    if (val < vals[it])
		i1 = it;
	    else
		i0 = it;
	    it = (i0 + i1) / 2;
	}
#if defined(DEBUG) && DEBUG + 0 > 1
	ibis::util::logMessage("locate", "%g in [%g, %g) ==> %lu", val,
			       vals[i0], vals[i1],
			       static_cast<long unsigned>(i1));
#endif
	return i1;
    }
    else { // do linear search
	for (uint32_t i = 0; i < nval; ++i) {
	    if (val < vals[i]) {
#if defined(DEBUG) && DEBUG + 0 > 1
		if (i > 0)
		    ibis::util::logMessage("locate", "%g in [%g, %g) ==> %lu",
					   val, vals[i-1], vals[i],
					   static_cast<long unsigned>(i));
		else
		    ibis::util::logMessage("locate", "%g in (..., %g) ==> 0",
					   val, vals[i]);
#endif
		return i;
	    }
	}
    }
    return vals.size();
} // ibis::relic::locate

/// Locate the bitmaps covered by the range expression.  Bitmaps hit0
/// (inclusive) through hit1 (execlusive) correspond to values satisfy the
/// range expression expr.
void ibis::relic::locate(const ibis::qContinuousRange& expr, uint32_t& hit0,
			 uint32_t& hit1) const {
    const uint32_t nval = vals.size();
    uint32_t bin0 = (expr.leftOperator()!=ibis::qExpr::OP_UNDEFINED) ?
	locate(expr.leftBound()) : 0;
    uint32_t bin1 = (expr.rightOperator()!=ibis::qExpr::OP_UNDEFINED) ?
	locate(expr.rightBound()) : 0;
    switch (expr.leftOperator()) {
    case ibis::qExpr::OP_LT:
	hit0 = bin0;
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit1 = nval;
	    break;
	case ibis::qExpr::OP_LT:
	    if (bin1 > 0)
		hit1 = bin1 - (expr.rightBound() == vals[bin1-1]);
	    else
		hit1 = 0;
	    break;
	case ibis::qExpr::OP_LE:
	    hit1 = bin1;
	    break;
	case ibis::qExpr::OP_GT:
	    hit1 = nval;
	    if (expr.rightBound() > expr.leftBound())
		hit0 = bin1;
	    break;
	case ibis::qExpr::OP_GE:
	    hit1 = nval;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 > 0)
		    hit0 = bin1 - (expr.rightBound() == vals[bin1-1]);
		else
		    hit0 = 0;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 > nval || bin1 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.rightBound() == vals[bin1-1]) {
		    hit0 = bin1 - 1;
		    hit1 = bin1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LT
    case ibis::qExpr::OP_LE:
	if (bin0 > 0)
	    hit0 = bin0 - (expr.leftBound() == vals[bin0-1]);
	else
	    hit0 = 0;
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit1 = nval;
	    break;
	case ibis::qExpr::OP_LT:
	    if (bin1 > 0)
		hit1 = bin1 - (expr.rightBound() == vals[bin1-1]);
	    else
		hit1 = 0;
	    break;
	case ibis::qExpr::OP_LE:
	    hit1 = bin1;
	    break;
	case ibis::qExpr::OP_GT:
	    hit1 = nval;
	    if (expr.rightBound() > expr.leftBound())
		hit0 = bin1;
	    break;
	case ibis::qExpr::OP_GE:
	    hit1 = nval;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 > 0)
		    hit0 = bin1 - (expr.rightBound() == vals[bin1-1]);
		else
		    hit0 = 0;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 > nval || bin1 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.rightBound() == vals[bin1-1]) {
		    hit0 = bin1 - 1;
		    hit1 = bin1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LE
    case ibis::qExpr::OP_GT:
	if (bin0 > 0)
	    hit1 = bin0 - (expr.leftBound() == vals[bin0-1]);
	else
	    hit1 = 0;
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    hit0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 > 0)
		    hit1 = bin1 - (expr.rightBound() == vals[bin1-1]);
		else
		    hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    hit0 = 0;
	    if (expr.rightBound() < expr.leftBound())
		hit1 = bin1 ;
	    break;
	case ibis::qExpr::OP_GT:
	    hit0 = bin1;
	    break;
	case ibis::qExpr::OP_GE:
	    if (bin1 > 0)
		hit0 = bin1 - (expr.rightBound() == vals[bin1-1]);
	    else
		hit0 = 0;
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 > nval || bin1 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.rightBound() == vals[bin1-1]) {
		    hit0 = bin1 - 1;
		    hit1 = bin1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GT
    case ibis::qExpr::OP_GE:
	hit1 = bin0;
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    hit0 = 0;
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 > 0)
		    hit1 = bin1 - (expr.rightBound() == vals[bin1-1]);
		else
		    hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    hit0 = 0;
	    if (expr.rightBound() < expr.leftBound())
		hit1 = bin1;
	    break;
	case ibis::qExpr::OP_GT:
	    hit0 = bin1;
	    break;
	case ibis::qExpr::OP_GE:
	    if (bin1 > 0)
		hit0 = bin1 - (expr.rightBound() == vals[bin1-1]);
	    else
		hit0 = 0;
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 > nval || bin1 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.rightBound() == vals[bin1-1]) {
		    hit0 = bin1 - 1;
		    hit1 = bin1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GE
    case ibis::qExpr::OP_EQ:
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    if (bin0 > nval || bin0 == 0) {
		hit0 = 0;
		hit1 = 0;
	    }
	    else if (expr.leftBound() == vals[bin0-1]) {
		hit0 = bin0 - 1;
		hit1 = bin0;
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LT:
	    if (expr.leftBound() < expr.rightBound()) {
		if (bin0 > nval || bin0 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.leftBound() == vals[bin0-1]) {
		    hit0 = bin0- 1;
		    hit1 = bin0;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (expr.leftBound() <= expr.rightBound()) {
		if (bin0 > nval || bin0 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.leftBound() == vals[bin0-1]) {
		    hit0 = bin0 - 1;
		    hit1 = bin0;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (expr.leftBound() > expr.rightBound()) {
		if (bin0 > nval || bin0 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.leftBound() == vals[bin0-1]) {
		    hit0 = bin0 - 1;
		    hit1 = bin0;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (expr.leftBound() >= expr.rightBound()) {
		if (bin0 > nval || bin0 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.leftBound() == vals[bin0-1]) {
		    hit0 = bin0 - 1;
		    hit1 = bin0;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.leftBound() == expr.rightBound()) {
		if (bin0 > nval || bin0 == 0) {
		    hit0 = 0;
		    hit1 = 0;
		}
		else if (expr.rightBound() <= vals[bin0-1]) {
		    hit0 = bin1 - 1;
		    hit1 = bin1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_EQ
    default:
    case ibis::qExpr::OP_UNDEFINED:
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    col->logWarning("relic::locate", "operators for the range not "
			    "specified");
	    return;
	case ibis::qExpr::OP_LT:
	    hit0 = 0;
	    if (bin1 > 0)
		hit1 = bin1 - (expr.rightBound() == vals[bin1-1]);
	    else
		hit1 = 0;
	    break;
	case ibis::qExpr::OP_LE:
	    hit0 = 0;
	    hit1 = bin1;
	    break;
	case ibis::qExpr::OP_GT:
	    hit1 = nval;
	    hit0 = bin1;
	    break;
	case ibis::qExpr::OP_GE:
	    hit1 = nval;
	    if (bin1 > 0)
		hit0 = bin1 - (expr.rightBound() == vals[bin1-1]);
	    else
		hit0 = 0;
	    break;
	case ibis::qExpr::OP_EQ:
	    if (bin1 > nval || bin1 == 0) {
		hit0 = 0;
		hit1 = 0;
	    }
	    else if (expr.rightBound() == vals[bin1-1]) {
		hit0 = bin1 - 1;
		hit1 = bin1;
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_UNDEFINED
    } // switch (expr.leftOperator())
    if (ibis::gVerbose > 5) {
	std::ostringstream ostr;
	ostr << expr;
	col->logMessage("relic::locate", "expr(%s) -> [%lu, %lu)",
			ostr.str().c_str(), static_cast<long unsigned>(hit0),
			static_cast<long unsigned>(hit1));
    }
} // ibis::relic::locate

// Compute the hits as a @c bitvector.
long ibis::relic::evaluate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower) const {
    // values in the range [hit0, hit1) satisfy the query
    uint32_t hit0, hit1;
    if (bits.empty()) {
	lower.set(0, nrows);
	return 0L;
    }

    locate(expr, hit0, hit1);
    if (hit1 < hit0)
	hit1 = hit0;
    sumBits(hit0, hit1, lower);
    return lower.cnt();
} // ibis::relic::evaluate

uint32_t ibis::relic::estimate(const ibis::qContinuousRange& expr) const {
    if (bits.empty()) return 0;

    uint32_t h0, h1;
    uint32_t nhits = 0;
    locate(expr, h0, h1);
    activate(h0, h1);
    for (uint32_t i=h0; i<h1; ++i) {
	nhits += bits[i]->cnt();
    }
    return nhits;
} // ibis::relic::estimate

double ibis::relic::estimateCost(const ibis::qContinuousRange& expr) const {
    double ret = 0.0;
    if (offsets.size() > bits.size()) {
	uint32_t h0, h1;
	locate(expr, h0, h1);
	if (h1 > h0 && bits.size() > h1 )
	    ret = offsets[h1] - offsets[h0];
    }
    return ret;
} // ibis::relic::estimateCost

double ibis::relic::estimateCost(const ibis::qDiscreteRange& expr) const {
    double ret = 0.0;
    if (offsets.size() > bits.size()) {
	const std::vector<double>& varr = expr.getValues();
	for (unsigned j = 0; j < varr.size(); ++ j) {
	    uint32_t itmp = locate(varr[j]);
	    if (itmp < bits.size())
		ret += offsets[itmp+1] - offsets[itmp];
	}
    }
    return ret;
} // ibis::relic::estimateCost

long ibis::relic::evaluate(const ibis::qDiscreteRange& expr,
			   ibis::bitvector& lower) const {
    const std::vector<double>& varr = expr.getValues();
    lower.set(0, nrows);
    for (unsigned i = 0; i < varr.size(); ++ i) {
	unsigned int itmp = locate(varr[i]);
	if (itmp > 0 && vals[itmp-1] == varr[i]) {
	    -- itmp;
	    if (bits[itmp] == 0)
		activate(itmp);
	    if (bits[itmp])
		lower |= *(bits[itmp]);
	}
    }
    return lower.cnt();
} // ibis::relic::evaluate

uint32_t ibis::relic::estimate(const ibis::qDiscreteRange& expr) const {
    const std::vector<double>& varr = expr.getValues();
    uint32_t cnt = 0;
    for (unsigned i = 0; i < varr.size(); ++ i) {
	unsigned int itmp = locate(varr[i]);
	if (itmp > 0 && vals[itmp-1] == varr[i]) {
	    -- itmp;
	    if (bits[itmp] == 0)
		activate(itmp);
	    if (bits[itmp])
		cnt += bits[itmp]->cnt();
	}
    }
    return cnt;
} // ibis::relic::evaluate

void ibis::relic::binBoundaries(std::vector<double>& b) const {
    b.resize(vals.size());
    for (uint32_t i = 0; i < vals.size(); ++ i)
	b[i] = vals[i];
} // ibis::relic::binBoundaries

void ibis::relic::binWeights(std::vector<uint32_t>& c) const {
    activate(); // need to make sure all bitmaps are available
    c.resize(vals.size());
    for (uint32_t i = 0; i < vals.size(); ++ i)
	c[i] = bits[i]->cnt();
} // ibis::relic::binWeights

double ibis::relic::getSum() const {
    double ret;
    bool here = false;
    { // a small test block to evaluate variable here
	const uint32_t nbv = col->elementSize() * col->partition()->nRows();
	if (str != 0)
	    here = (str->bytes() < nbv);
	else if (offsets.size() > bits.size())
	    here = (static_cast<uint32_t>(offsets[bits.size()]) < nbv);
    }
    if (here) {
	ret = computeSum();
    }
    else { // indicate sum is not computed
	ibis::util::setNaN(ret);
    }
    return ret;
} // ibis::relic::getSum

double ibis::relic::computeSum() const {
    double sum = 0;
    activate(); // need to activate all bitvectors
    for (uint32_t i = 0; i < bits.size(); ++ i)
	if (bits[i] != 0)
	    sum += vals[i] * bits[i]->cnt();
    return sum;
} // ibis::relic::computeSum

long ibis::relic::getCumulativeDistribution(std::vector<double>& bds,
					    std::vector<uint32_t>& cts) const {
    bds.clear();
    cts.clear();
    long ierr = 0;
    binBoundaries(bds);
    if (bds.size() > 0) {
	binWeights(cts);
	if (bds.size() == cts.size()) {
	    // convert to cumulative distribution
	    // cts[i] = number of values less than bds[i]
	    uint32_t cnt = cts[0];
	    cts[0] = 0;
	    for (uint32_t i = 1; i < bds.size(); ++ i) {
		uint32_t tmp = cts[i] + cnt;
		cts[i] = cnt;
		cnt = tmp;
	    }
	    bds.push_back(ibis::util::compactValue
			  (bds.back(), bds.back()+bds.back()));
	    cts.push_back(cnt);
	    ierr = bds.size();
	}
	else {
	    // don't match, delete the content
	    col->logMessage("getCumulativeDistribution",
			    "bds[%lu] and cts[%lu] sizes do not match",
			    static_cast<long unsigned>(bds.size()),
			    static_cast<long unsigned>(cts.size()));
#if defined(DEBUG) && DEBUG + 0 > 1
	    {
		ibis::util::logger lg(4);
		lg.buffer() << "DEBUG -- bds array:\n";
		for (uint32_t i = 0; i < bds.size(); ++ i)
		    lg.buffer() << bds[i] << " ";
		lg.buffer() << "\nDEBUG -- cts array:\n";
		for (uint32_t i = 0; i < cts.size(); ++ i)
		    lg.buffer() << cts[i] << " ";
		lg.buffer() << "\n";
	    }
#endif
	    bds.clear();
	    cts.clear();
	    ierr = -2;
	}
    }
    else {
	col->logMessage("relic::getCumulativeDistribution",
			"can not find bin boundaries, probably not data");
	bds.clear();
	cts.clear();
	ierr = -1;
    }
    return ierr;
} // ibis::relic::getCumulativeDistribution

long ibis::relic::getDistribution(std::vector<double>& bds,
				  std::vector<uint32_t>& cts) const {
    bds.clear();
    cts.clear();
    long ierr = 0;
    binBoundaries(bds);
    if (bds.size() > 0) {
	binWeights(cts);
	if (bds.size() == cts.size()) {
	    for (uint32_t i = 0; i+1 < bds.size(); ++ i)
		bds[i] = bds[i+1];
	    bds.resize(bds.size()-1);
	    ierr = cts.size();
	}
	else {
	    // don't match, delete the content
	    col->logMessage("getDistribution",
			    "bds[%lu] and cts[%lu] sizes do not match",
			    static_cast<long unsigned>(bds.size()),
			    static_cast<long unsigned>(cts.size()));
#if defined(DEBUG) && DEBUG + 0 > 1
	    {
		ibis::util::logger lg(4);
		lg.buffer() << "DEBUG -- bds array:\n";
		for (uint32_t i = 0; i < bds.size(); ++ i)
		    lg.buffer() << bds[i] << " ";
		lg.buffer() << "\nDEBUG -- cts array:\n";
		for (uint32_t i = 0; i < cts.size(); ++ i)
		    lg.buffer() << cts[i] << " ";
	    }
#endif
	    bds.clear();
	    cts.clear();
	    ierr = -2;
	}
    }
    else {
	col->logMessage("relic::getDistribution",
			"can not find bin boundaries, probably not data");
	bds.clear();
	cts.clear();
	ierr = -1;
    }
    return ierr;
} // ibis::relic::getDistribution

/// @note It is assumed that @c range1 is for column 1 in the join
/// expression and @c range2 is for column 2 in the join expression.  No
/// name matching is performed.
void ibis::relic::estimate(const ibis::relic& idx2,
			   const ibis::rangeJoin& expr,
			   const ibis::bitvector& mask,
			   const ibis::qRange* const range1,
			   const ibis::qRange* const range2,
			   ibis::bitvector64& lower,
			   ibis::bitvector64& upper) const {
    lower.clear();
    upper.clear();
    if (col == 0 || idx2.col == 0) // can not do anything useful
	return;
    if (mask.cnt() == 0)
	return;
    if (range1 == 0 && range2 == 0) {
	estimate(idx2, expr, mask, lower, upper);
	return;
    }

    int64_t cnt = 0;
    horometer timer;
    if (ibis::gVerbose > 1)
	timer.start();

    if (expr.getRange() == 0) {
	cnt = equiJoin(idx2, mask, range1, range2, lower);
    }
    else if (expr.getRange()->termType() == ibis::compRange::NUMBER) {
	const double delta = fabs(expr.getRange()->eval());
	if (delta == 0.0)
	    cnt = equiJoin(idx2, mask, range1, range2, lower);
	else
	    cnt = rangeJoin(idx2, mask, range1, range2, delta, lower);
    }
    else {
	cnt = compJoin(idx2, mask, range1, range2, *(expr.getRange()), lower);
    }
    if (ibis::gVerbose > 1) {
	timer.stop();
	std::ostringstream ostr;
	ostr << expr;
	ostr << " with a mask (" << mask.cnt() << ")";
	if (range1) {
	    if (range2)
		ostr << ", " << *range1 << ", and " << *range2;
	    else
		ostr << " and " << *range1;
	}
	else if (range2) {
	    ostr << " and " << *range2;
	}
	if (cnt >= 0) {
	    ostr << " produced " << cnt << " hit" << (cnt>1 ? "s" : "")
		 << "(result bitvector size " << lower.bytes() << " bytes)";
	    ibis::util::logMessage
		("relic::estimate", "processing %s took %g sec(CPU) and "
		 "%g sec(elapsed)",
		 ostr.str().c_str(), timer.CPUTime(), timer.realTime());
	}
	else {
	    ibis::util::logMessage("Warning", "relic::estimate could not "
				   "effectively evaluate %s, reverting to "
				   "simple scans",
				   ostr.str().c_str());
	    cnt = col->partition()->evaluateJoin(expr, mask, lower);
	    upper.clear();
	}
    }
} // ibis::relic::estimate

int64_t ibis::relic::estimate(const ibis::relic& idx2,
			      const ibis::rangeJoin& expr,
			      const ibis::bitvector& mask,
			      const ibis::qRange* const range1,
			      const ibis::qRange* const range2) const {
    if (col == 0 || idx2.col == 0) // can not do anything useful
	return -1;
    if (mask.cnt() == 0)
	return 0;
    if (range1 == 0 && range2 == 0)
	return estimate(idx2, expr, mask);

    int64_t cnt = 0;
    horometer timer;
    if (ibis::gVerbose > 1)
	timer.start();

    if (expr.getRange() == 0) {
	cnt = equiJoin(idx2, mask, range1, range2);
    }
    else if (expr.getRange()->termType() == ibis::compRange::NUMBER) {
	const double delta = fabs(expr.getRange()->eval());
	if (delta == 0.0)
	    cnt = equiJoin(idx2, mask, range1, range2);
	else
	    cnt = rangeJoin(idx2, mask, range1, range2, delta);
    }
    else {
	cnt = compJoin(idx2, mask, range1, range2, *(expr.getRange()));
    }
    if (ibis::gVerbose > 1) {
	timer.stop();
	std::ostringstream ostr;
	ostr << expr;
	ostr << " with a mask (" << mask.cnt() << ")";
	if (range1) {
	    if (range2)
		ostr << ", " << *range1 << ", and " << *range2;
	    else
		ostr << " and " << *range1;
	}
	else if (range2) {
	    ostr << " and " << *range2;
	}
	if (cnt >= 0) {
	    ostr << " produced " << cnt << " hit" << (cnt>1 ? "s" : "");
	    ibis::util::logMessage
		("relic::estimate", "processing %s took %g sec(CPU) and "
		 "%g sec(elapsed)",
		 ostr.str().c_str(), timer.CPUTime(), timer.realTime());
	}
	else {
	    ibis::util::logMessage("Warning", "relic::estimate could not "
				   "effectively process %s, revert to simple "
				   "scan",
				   ostr.str().c_str());
	    cnt = col->partition()->evaluateJoin(expr, mask);
	}
    }
    return cnt;
} // ibis::relic::estimate

void ibis::relic::estimate(const ibis::relic& idx2,
			   const ibis::rangeJoin& expr,
			   const ibis::bitvector& mask,
			   ibis::bitvector64& lower,
			   ibis::bitvector64& upper) const {
    lower.clear();
    upper.clear();
    if (col == 0 || idx2.col == 0) // can not do anything useful
	return;
    if (mask.cnt() == 0)
	return;

    int64_t cnt = 0;
    horometer timer;
    if (ibis::gVerbose > 1)
	timer.start();

    if (expr.getRange() == 0) {
	cnt = equiJoin(idx2, mask, lower);
    }
    else if (expr.getRange()->termType() == ibis::compRange::NUMBER) {
	const double delta = fabs(expr.getRange()->eval());
	if (delta == 0.0)
	    cnt = equiJoin(idx2, mask, lower);
	else
	    cnt = rangeJoin(idx2, mask, delta, lower);
    }
    else {
	cnt = compJoin(idx2, mask, *(expr.getRange()), lower);
    }
    if (ibis::gVerbose > 1) {
	timer.stop();
	std::ostringstream ostr;
	ostr << expr << " with a mask (" << mask.cnt() << ")";
	if (cnt >= 0) {
	    ostr << " produced " << cnt << " hit" << (cnt>1 ? "s" : "")
		 << "(result bitvector size " << lower.bytes() << " bytes)";
	    ibis::util::logMessage
		("relic::estimate", "processing %s took %g sec(CPU) and "
		 "%g sec(elapsed)",
		 ostr.str().c_str(), timer.CPUTime(), timer.realTime());
	}
	else {
	    ibis::util::logMessage("Warning", "relic::estimate could not "
				   "effectively evaluate %s, revert to "
				   "simple scan",
				   ostr.str().c_str());
	    cnt = col->partition()->evaluateJoin(expr, mask, lower);
	}
    }
} // ibis::relic::estimate

int64_t ibis::relic::estimate(const ibis::relic& idx2,
			      const ibis::rangeJoin& expr,
			      const ibis::bitvector& mask) const {
    if (col == 0 || idx2.col == 0) // can not do anything useful
	return -1;
    if (mask.cnt() == 0)
	return 0;

    int64_t cnt = 0;
    horometer timer;
    if (ibis::gVerbose > 1)
	timer.start();

    if (expr.getRange() == 0) {
	cnt = equiJoin(idx2, mask);
    }
    else if (expr.getRange()->termType() == ibis::compRange::NUMBER) {
	const double delta = fabs(expr.getRange()->eval());
	if (delta == 0.0)
	    cnt = equiJoin(idx2, mask);
	else
	    cnt = rangeJoin(idx2, mask, delta);
    }
    else {
	cnt = compJoin(idx2, mask, *(expr.getRange()));
    }
    if (ibis::gVerbose > 1) {
	timer.stop();
	std::ostringstream ostr;
	ostr << expr << " with a mask (" << mask.cnt() << ")";
	if (cnt >= 0) {
	    ostr  << " produced " << cnt << " hit" << (cnt>1 ? "s" : "");
	    ibis::util::logMessage("relic::estimate", "processing %s took %g "
				   "sec(CPU) and %g sec(elapsed)",
				   ostr.str().c_str(), timer.CPUTime(),
				   timer.realTime());
	}
	else {
	    ibis::util::logMessage("Warning", "relic::estimate could not "
				   "effectively evaluate %s, revert to "
				   "simply scan",
				   ostr.str().c_str());
	    cnt = col->partition()->evaluateJoin(expr, mask);
	}
    }
    return cnt;
} // ibis::relic::estimate

/// Evaluate an equi-join using two ibis::relic indices.  The restriction
/// is presented as a bit mask.

// Implementation note:
// 2006/02/25 -- use a very simple implementation right now, the outer
// products are computed one at a time
int64_t ibis::relic::equiJoin(const ibis::relic& idx2,
			      const ibis::bitvector& mask,
			      ibis::bitvector64& hits) const {
    hits.clear();
    if (mask.cnt() == 0) return 0;

    uint32_t ib1 = 0; // index over vals in idx1
    uint32_t ib2 = 0; // index over vals in idx2
    const uint32_t nb1 = vals.size();
    const uint32_t nb2 = idx2.vals.size();

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name() << ") using " << name()
	    << " indices";
    }

    activate(); // need all bitvectors in memory
    idx2.activate(); // need all bitvectors in memory
    while (ib1 < nb1 && ib2 < nb2) {
	while (ib1 < nb1 && vals[ib1] < idx2.vals[ib2])
	    ++ ib1;
	if (ib1 >= nb1) break;
	while (ib2 < nb2 && vals[ib1] > idx2.vals[ib2])
	    ++ ib2;
	if (ib2 >= nb2) break;
	if (vals[ib1] == idx2.vals[ib2]) { // found a match
	    ibis::bitvector tmp1;
	    if (bits[ib1]) {
		tmp1.copy(mask);
		tmp1 &= *(bits[ib1]);
	    }
	    if (tmp1.cnt() > 0) {
		ibis::bitvector tmp2;
		if (idx2.bits[ib2]) {
		    tmp2.copy(mask);
		    tmp2 &= *(idx2.bits[ib2]);
		    if (tmp2.cnt() > 0) { // add the outer product
			ibis::outerProduct(tmp1, tmp2, hits);
		    }
		}
	    }

	    // move on to the next value
	    ++ ib1;
	    ++ ib2;
	}
    } // while (ib1 < nb1 && ib2 < nb2)

    if (ibis::gVerbose > 3) {
	uint64_t cnt = hits.cnt();
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name() << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return hits.cnt();
} // ibis::relic::equiJoin

int64_t ibis::relic::equiJoin(const ibis::relic& idx2,
			      const ibis::bitvector& mask) const {
    if (mask.cnt() == 0) return 0;

    uint32_t ib1 = 0; // index over vals in idx1
    uint32_t ib2 = 0; // index over vals in idx2
    const uint32_t nb1 = vals.size();
    const uint32_t nb2 = idx2.vals.size();
    int64_t cnt = 0;

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name() << ") using " << name()
	    << " indices";
    }

    activate(); // need all bitvectors in memory
    idx2.activate(); // need all bitvectors in memory
    while (ib1 < nb1 && ib2 < nb2) {
	while (ib1 < nb1 && vals[ib1] < idx2.vals[ib2])
	    ++ ib1;
	if (ib1 >= nb1) break;
	while (ib2 < nb2 && vals[ib1] > idx2.vals[ib2])
	    ++ ib2;
	if (ib2 >= nb2) break;
	if (vals[ib1] == idx2.vals[ib2]) { // found a match
	    ibis::bitvector tmp1;
	    if (bits[ib1]) {
		tmp1.copy(mask);
		tmp1 &= *(bits[ib1]);
	    }
	    if (tmp1.cnt() > 0) {
		ibis::bitvector tmp2;
		if (idx2.bits[ib2]) {
		    tmp2.copy(mask);
		    tmp2 &= *(idx2.bits[ib2]);
		    cnt += tmp1.cnt() * tmp2.cnt();
		}
	    }

	    // move on to the next value
	    ++ ib1;
	    ++ ib2;
	}
    } // while (ib1 < nb1 && ib2 < nb2)

    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name() << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return cnt;
} // ibis::relic::equiJoin

/// Evaluate an equi-join with explicit restrictions on the join columns.
int64_t ibis::relic::equiJoin(const ibis::relic& idx2,
			      const ibis::bitvector& mask,
			      const ibis::qRange* const range1,
			      const ibis::qRange* const range2,
			      ibis::bitvector64& hits) const {
    hits.clear();
    if (mask.cnt() == 0) return 0;

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name() << ") using " << name()
	    << " indices";
    }

    // use the range restrictions to figure out what bitvectors to activate
    uint32_t ib1=0; // the first (begin) bitvector to be used from *this
    uint32_t ib1e=0; // the last+1 (end) bitvector to be used from *this
    uint32_t ib2=0;
    uint32_t ib2e=0;
    if (range1 == 0) {
	ib1 = 0;
	ib1e = bits.size();
    }
    else if (range1->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range1),
	       ib1, ib1e);
    }
    else {
	ibis::qContinuousRange tmp(range1->leftBound(), ibis::qExpr::OP_LE,
				   col->name(), ibis::qExpr::OP_LE,
				   range1->rightBound());
	locate(tmp, ib1, ib1e);
    }
    if (range2 == 0) {
	ib2 = 0;
	ib2e = idx2.bits.size();
    }
    else if (range2->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range2),
	       ib2, ib2e);
    }
    else {
	ibis::qContinuousRange tmp(range2->leftBound(), ibis::qExpr::OP_LE,
				   idx2.col->name(), ibis::qExpr::OP_LE,
				   range2->rightBound());
	locate(tmp, ib2, ib2e);
    }

    activate(ib1, ib1e); // need bitvectors in memory
    idx2.activate(ib2, ib2e); // need bitvectors in memory
    while (ib1 < ib1e && ib2 < ib2e) {
	while (ib1 < ib1e && vals[ib1] < idx2.vals[ib2])
	    ++ ib1;
	if (ib1 >= ib1e) break;
	while (ib2 < ib2e && vals[ib1] > idx2.vals[ib2])
	    ++ ib2;
	if (ib2 >= ib2e) break;
	if (vals[ib1] == idx2.vals[ib2]) { // found a match
	    if ((range1 == 0 || range1->inRange(vals[ib1])) &&
		(range2 == 0 || range2->inRange(vals[ib1]))) {
		ibis::bitvector tmp1;
		if (bits[ib1]) {
		    tmp1.copy(mask);
		    tmp1 &= *(bits[ib1]);
		}
		if (tmp1.cnt() > 0) {
		    ibis::bitvector tmp2;
		    if (idx2.bits[ib2]) {
			tmp2.copy(mask);
			tmp2 &= *(idx2.bits[ib2]);
			if (tmp2.cnt() > 0) { // add the outer product
			    ibis::outerProduct(tmp1, tmp2, hits);
			}
		    }
		}
	    }
	    // move on to the next value
	    ++ ib1;
	    ++ ib2;
	}
    } // while (ib1 < ib1e && ib2 < ib2e)

    if (ibis::gVerbose > 3) {
	uint64_t cnt = hits.cnt();
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name() << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return hits.cnt();
} // ibis::relic::equiJoin

/// Evaluate an equi-join with explicit restrictions on the join columns.
int64_t ibis::relic::equiJoin(const ibis::relic& idx2,
			      const ibis::bitvector& mask,
			      const ibis::qRange* const range1,
			      const ibis::qRange* const range2) const {
    int64_t cnt = 0;
    if (mask.cnt() == 0) return cnt;

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name() << ") using " << name()
	    << " indices";
    }

    // use the range restrictions to figure out what bitvectors to activate
    uint32_t ib1=0; // the first (begin) bitvector to be used from *this
    uint32_t ib1e=0; // the last+1 (end) bitvector to be used from *this
    uint32_t ib2=0;
    uint32_t ib2e=0;
    if (range1 == 0) {
	ib1 = 0;
	ib1e = bits.size();
    }
    else if (range1->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range1),
	       ib1, ib1e);
    }
    else {
	ibis::qContinuousRange tmp(range1->leftBound(), ibis::qExpr::OP_LE,
				   col->name(), ibis::qExpr::OP_LE,
				   range1->rightBound());
	locate(tmp, ib1, ib1e);
    }
    if (range2 == 0) {
	ib2 = 0;
	ib2e = idx2.bits.size();
    }
    else if (range2->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range2),
	       ib2, ib2e);
    }
    else {
	ibis::qContinuousRange tmp(range2->leftBound(), ibis::qExpr::OP_LE,
				   idx2.col->name(), ibis::qExpr::OP_LE,
				   range2->rightBound());
	locate(tmp, ib2, ib2e);
    }

    activate(ib1, ib1e); // need bitvectors in memory
    idx2.activate(ib2, ib2e); // need bitvectors in memory
    while (ib1 < ib1e && ib2 < ib2e) {
	while (ib1 < ib1e && vals[ib1] < idx2.vals[ib2])
	    ++ ib1;
	if (ib1 >= ib1e) break;
	while (ib2 < ib2e && vals[ib1] > idx2.vals[ib2])
	    ++ ib2;
	if (ib2 >= ib2e) break;
	if (vals[ib1] == idx2.vals[ib2]) { // found a match
	    if ((range1 == 0 || range1->inRange(vals[ib1])) &&
		(range2 == 0 || range2->inRange(vals[ib1]))) {
		ibis::bitvector tmp1;
		if (bits[ib1]) {
		    tmp1.copy(mask);
		    tmp1 &= *(bits[ib1]);
		}
		if (tmp1.cnt() > 0) {
		    ibis::bitvector tmp2;
		    if (idx2.bits[ib2]) {
			tmp2.copy(mask);
			tmp2 &= *(idx2.bits[ib2]);
			cnt += tmp1.cnt() * tmp2.cnt();
		    }
		}
	    }
	    // move on to the next value
	    ++ ib1;
	    ++ ib2;
	}
    } // while (ib1 < ib1e && ib2 < ib2e)

    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::equiJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name() << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return cnt;
} // ibis::relic::equiJoin

/// @note If the input value @c delta is less than zero it is treated as
/// equal to zero (0).
int64_t ibis::relic::rangeJoin(const ibis::relic& idx2,
			       const ibis::bitvector& mask,
			       const double& delta,
			       ibis::bitvector64& hits) const {
    hits.clear();
    if (mask.cnt() == 0) return 0;

    if (delta <= 0)
	return equiJoin(idx2, mask, hits);

    uint32_t ib2s = 0; // idx2.vals[ib2s] + delta >= vals[ib1]
    uint32_t ib2e = 0; // idx2.vals[ib2e] - delta > vals[ib1]
    const uint32_t nb1 = vals.size();
    const uint32_t nb2 = idx2.vals.size();
    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") using " << name() << " indices";
    }

    activate(); // need all bitvectors in memory
    idx2.activate(); // need all bitvectors in memory
    for (uint32_t ib1 = 0; ib1 < nb1; ++ ib1) {
	if (bits[ib1] == 0) continue;
	ibis::bitvector tmp1 = mask;
	tmp1 &= *(bits[ib1]);
	if (tmp1.cnt() == 0) continue;

	const double lo = vals[ib1] - delta;
	const double hi = vals[ib1] + delta;
	// ib2s catch up with vals[ib1]-delta
	while (ib2s < nb2 && idx2.vals[ib2s] < lo)
	    ++ ib2s;
	// ib2s catch up with vals[ib1]+delta
	if (ib2e <= ib2s)
	    ib2e = ib2s;
	while (ib2e < nb2 && idx2.vals[ib2e] <= hi)
	    ++ ib2e;

	if (ib2e > ib2s) {
	    ibis::bitvector tmp2;
	    idx2.sumBits(ib2s, ib2e, tmp2);
	    tmp2 &= mask;
	    if (tmp2.cnt() > 0) {
		ibis::outerProduct(tmp1, tmp2, hits);
	    }
	}
    } // for (uint32_t ib1...

    if (ibis::gVerbose > 3) {
	uint64_t cnt = hits.cnt();
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return hits.cnt();
} // ibis::relic::rangeJoin

int64_t ibis::relic::rangeJoin(const ibis::relic& idx2,
			       const ibis::bitvector& mask,
			       const double& delta) const {
    int64_t cnt = 0;
    if (mask.cnt() == 0) return cnt;

    if (delta <= 0)
	return equiJoin(idx2, mask);

    uint32_t ib2s = 0; // idx2.vals[ib2s] + delta >= vals[ib1]
    uint32_t ib2e = 0; // idx2.vals[ib2e] - delta > vals[ib1]
    const uint32_t nb1 = vals.size();
    const uint32_t nb2 = idx2.vals.size();

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") using " << name() << " indices";
    }

    activate(); // need all bitvectors in memory
    idx2.activate(); // need all bitvectors in memory
    for (uint32_t ib1 = 0; ib1 < nb1; ++ ib1) {
	if (bits[ib1] == 0) continue;
	ibis::bitvector tmp1 = mask;
	tmp1 &= *(bits[ib1]);
	if (tmp1.cnt() == 0) continue;

	const double lo = vals[ib1] - delta;
	const double hi = vals[ib1] + delta;
	// ib2s catch up with vals[ib1]-delta
	while (ib2s < nb2 && idx2.vals[ib2s] < lo)
	    ++ ib2s;
	// ib2s catch up with vals[ib1]+delta
	if (ib2e <= ib2s)
	    ib2e = ib2s;
	while (ib2e < nb2 && idx2.vals[ib2e] <= hi)
	    ++ ib2e;

	if (ib2e > ib2s) {
	    ibis::bitvector tmp2;
	    idx2.sumBits(ib2s, ib2e, tmp2);
	    tmp2 &= mask;
	    cnt += tmp1.cnt() * tmp2.cnt();
	}
    } // for (uint32_t ib1...

    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return cnt;
} // ibis::relic::rangeJoin

/// @note If the input value @c delta is less than zero it is treated as
/// equal to zero (0).
int64_t ibis::relic::rangeJoin(const ibis::relic& idx2,
			       const ibis::bitvector& mask,
			       const ibis::qRange* const range1,
			       const ibis::qRange* const range2,
			       const double& delta,
			       ibis::bitvector64& hits) const {
    hits.clear();
    if (mask.cnt() == 0) return 0;

    if (delta <= 0)
	return equiJoin(idx2, mask, range1, range2, hits);
    if (range2 != 0 && range2->getType() != ibis::qExpr::RANGE) {
	col->logMessage("relic::rangeJoin", "current implementation does "
			"more work than necessary because if can not "
			"handle discrete range restrictions on %s!",
			idx2.col->name());
    }

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") using " << name() << " indices";
    }

    uint32_t nb1s = 0;
    uint32_t nb1e = 0;
    uint32_t nb2s = 0;
    uint32_t nb2e = 0;
    const uint32_t nb2 = idx2.vals.size();
    if (range1 == 0) {
	nb1s = 0;
	nb1e = bits.size();
    }
    else if (range1->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range1),
	       nb1s, nb1e);
    }
    else {
	ibis::qContinuousRange tmp(range1->leftBound(), ibis::qExpr::OP_LE,
				   col->name(), ibis::qExpr::OP_LE,
				   range1->rightBound());
	locate(tmp, nb1s, nb1e);
    }
    if (range2 == 0) {
	nb2s = 0;
	nb2e = idx2.bits.size();
    }
    else if (range2->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range2),
	       nb2s, nb2e);
    }
    else {
	ibis::qContinuousRange tmp(range2->leftBound(), ibis::qExpr::OP_LE,
				   idx2.col->name(), ibis::qExpr::OP_LE,
				   range2->rightBound());
	locate(tmp, nb2s, nb2e);
    }
    uint32_t ib2s = nb2s; // idx2.vals[ib2s] + delta >= vals[ib1]
    uint32_t ib2e = nb2s; // idx2.vals[ib2e] - delta > vals[ib1]
    activate(nb1s, nb1e); // need bitvectors in memory
    idx2.activate(nb2s, nb2e); // need bitvectors in memory
    for (uint32_t ib1 = nb1s; ib1 < nb1e; ++ ib1) {
	if (bits[ib1] == 0 || !(range1 == 0 || range1->inRange(vals[ib1])))
	    continue;
	ibis::bitvector tmp1 = mask;
	tmp1 &= *(bits[ib1]);
	if (tmp1.cnt() == 0) continue;

	const double lo = vals[ib1] - delta;
	const double hi = vals[ib1] + delta;
	// ib2s catch up with vals[ib1]-delta
	while (ib2s < nb2 && idx2.vals[ib2s] < lo)
	    ++ ib2s;
	// ib2s catch up with vals[ib1]+delta
	if (ib2e <= ib2s)
	    ib2e = ib2s;
	while (ib2e < nb2 && idx2.vals[ib2e] <= hi)
	    ++ ib2e;

	// this only work if range2 is a qContinuousRange
	if (ib2e > ib2s) {
	    ibis::bitvector tmp2;
	    idx2.sumBits(ib2s, ib2e, tmp2);
	    tmp2 &= mask;
	    if (tmp2.cnt() > 0) {
		ibis::outerProduct(tmp1, tmp2, hits);
	    }
	}
    } // for (uint32_t ib1...

    if (ibis::gVerbose > 3) {
	uint64_t cnt = hits.cnt();
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return hits.cnt();
} // ibis::relic::rangeJoin

int64_t ibis::relic::rangeJoin(const ibis::relic& idx2,
			       const ibis::bitvector& mask,
			       const ibis::qRange* const range1,
			       const ibis::qRange* const range2,
			       const double& delta) const {
    int64_t cnt = 0;
    if (mask.cnt() == 0) return cnt;

    if (delta <= 0)
	return equiJoin(idx2, mask, range1, range2);
    if (range2 != 0 && range2->getType() != ibis::qExpr::RANGE) {
	col->logMessage("relic::rangeJoin", "current implementation does "
			"more work than necessary because if can not "
			"handle discrete range restrictions on %s!",
			idx2.col->name());
    }

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") using " << name() << " indices";
    }

    uint32_t nb1s = 0;
    uint32_t nb1e = 0;
    uint32_t nb2s = 0;
    uint32_t nb2e = 0;
    const uint32_t nb2 = idx2.vals.size();
    if (range1 == 0) {
	nb1s = 0;
	nb1e = bits.size();
    }
    else if (range1->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range1),
	       nb1s, nb1e);
    }
    else {
	ibis::qContinuousRange tmp(range1->leftBound(), ibis::qExpr::OP_LE,
				   col->name(), ibis::qExpr::OP_LE,
				   range1->rightBound());
	locate(tmp, nb1s, nb1e);
    }
    if (range2 == 0) {
	nb2s = 0;
	nb2e = idx2.bits.size();
    }
    else if (range2->getType() == ibis::qExpr::RANGE) {
	locate(*reinterpret_cast<const ibis::qContinuousRange*const>(range2),
	       nb2s, nb2e);
    }
    else {
	ibis::qContinuousRange tmp(range2->leftBound(), ibis::qExpr::OP_LE,
				   idx2.col->name(), ibis::qExpr::OP_LE,
				   range2->rightBound());
	locate(tmp, nb2s, nb2e);
    }
    uint32_t ib2s = nb2s; // idx2.vals[ib2s] + delta >= vals[ib1]
    uint32_t ib2e = nb2s; // idx2.vals[ib2e] - delta > vals[ib1]
    activate(nb1s, nb1e); // need bitvectors in memory
    idx2.activate(nb2s, nb2e); // need bitvectors in memory
    for (uint32_t ib1 = nb1s; ib1 < nb1e; ++ ib1) {
	if (bits[ib1] == 0 || ! (range1 == 0 || range1->inRange(vals[ib1])))
	    continue;
	ibis::bitvector tmp1 = mask;
	tmp1 &= *(bits[ib1]);
	if (tmp1.cnt() == 0) continue;

	const double lo = vals[ib1] - delta;
	const double hi = vals[ib1] + delta;
	// ib2s catch up with vals[ib1]-delta
	while (ib2s < nb2 && idx2.vals[ib2s] < lo)
	    ++ ib2s;
	// ib2s catch up with vals[ib1]+delta
	if (ib2e <= ib2s)
	    ib2e = ib2s;
	while (ib2e < nb2 && idx2.vals[ib2e] <= hi)
	    ++ ib2e;

	// this only work if range2 is a qContinuousRange
	if (ib2e > ib2s) {
	    ibis::bitvector tmp2;
	    idx2.sumBits(ib2s, ib2e, tmp2);
	    tmp2 &= mask;
	    cnt += tmp2.cnt() * tmp1.cnt();
	}
    } // for (uint32_t ib1...

    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::rangeJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return cnt;
} // ibis::relic::rangeJoin

/// @note If the input value @c delta is less than zero it is treated as
/// equal to zero (0).
int64_t ibis::relic::compJoin(const ibis::relic& idx2,
			      const ibis::bitvector& mask,
			      const ibis::compRange::term& delta,
			      ibis::bitvector64& hits) const {
    hits.clear();
    if (mask.cnt() == 0) return 0;

    ibis::compRange::barrel bar(&delta);
    if (bar.size() == 1 &&
	stricmp(bar.name(0), col->name()) == 0) {
	// continue
    }
    else if (bar.size() < 1) { // no variable involved
	return rangeJoin(idx2, mask, delta.eval(), hits);
    }
    else { // can not evaluate the join effectively here
	return -1;
    }

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::compJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") using " << name() << " indices";
    }
    const uint32_t nb1 = vals.size();
    activate(); // need all bitvectors in memory
    idx2.activate(); // need all bitvectors in memory
    for (uint32_t ib1 = 0; ib1 < nb1; ++ ib1) {
	if (bits[ib1] == 0) continue;
	ibis::bitvector tmp1 = mask;
	tmp1 &= *(bits[ib1]);
	if (tmp1.cnt() == 0) continue;

	bar.value(0) = vals[ib1];
	const double dt = std::fabs(delta.eval());
	const double lo = vals[ib1] - dt;
	const double hi = ibis::util::incrDouble(vals[ib1] + dt);
	uint32_t ib2s = idx2.vals.find(lo);
	uint32_t ib2e = idx2.vals.find(hi);
	if (ib2e > ib2s) {
	    ibis::bitvector tmp2;
	    idx2.sumBits(ib2s, ib2e, tmp2);
	    tmp2 &= mask;
	    if (tmp2.cnt() > 0) {
		ibis::outerProduct(tmp1, tmp2, hits);
	    }
	}
    } // for (uint32_t ib1...

    if (ibis::gVerbose > 3) {
	uint64_t cnt = hits.cnt();
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::compJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return hits.cnt();
} // ibis::relic::compJoin

int64_t ibis::relic::compJoin(const ibis::relic& idx2,
			      const ibis::bitvector& mask,
			      const ibis::compRange::term& delta) const {
    int64_t cnt = 0;
    if (mask.cnt() == 0) return cnt;

    ibis::compRange::barrel bar(&delta);
    if (bar.size() == 1 &&
	stricmp(bar.name(0), col->name()) == 0) {
	// continue
    }
    else if (bar.size() < 1) { // no variable involved
	return rangeJoin(idx2, mask, delta.eval());
    }
    else { // can not evaluate the join effectively here
	return -1;
    }

    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	timer.start();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::compJoin starting to evaluate join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") using " << name() << " indices";
    }
    const uint32_t nb1 = vals.size();
    activate(); // need all bitvectors in memory
    idx2.activate(); // need all bitvectors in memory
    for (uint32_t ib1 = 0; ib1 < nb1; ++ ib1) {
	if (bits[ib1] == 0) continue;
	ibis::bitvector tmp1 = mask;
	tmp1 &= *(bits[ib1]);
	if (tmp1.cnt() == 0) continue;

	bar.value(0) = vals[ib1];
	const double dt = std::fabs(delta.eval());
	const double lo = vals[ib1] - dt;
	const double hi = ibis::util::incrDouble(vals[ib1] + dt);
	uint32_t ib2s = idx2.vals.find(lo);
	uint32_t ib2e = idx2.vals.find(hi);
	if (ib2e > ib2s) {
	    ibis::bitvector tmp2;
	    idx2.sumBits(ib2s, ib2e, tmp2);
	    tmp2 &= mask;
	    cnt += tmp1.cnt() * tmp2.cnt();
	}
    } // for (uint32_t ib1...

    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::relic::compJoin completed evaluating join("
	    << col->name() << ", " << idx2.col->name()
	    << ", " << delta << ") produced " << cnt
	    << (cnt>1 ? " hits" : " hit") << " in "
	    << timer.realTime() << " sec elapsed time";
    }
    return cnt;
} // ibis::relic::compJoin

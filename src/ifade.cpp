// $Id$
// Author: John Wu <John.Wu@ACM.org>
// Copyright 2000-2008 the Regents of the University of California
//
// This file contains the implementation of the class called ibis::fade.
//
// fade  -- multicomponent range-encoded bitmap index
// sbiad -- multicomponent interval-encoded bitmap index
// sapid -- multicomponent equality-encoded bitmap index
//
// Definition of the word fade as a noun accoring to The American Heritage
// 2000
// 1.A gradual diminution or increase in the brightness or visibility of
//   an image in cinema or television. 
// 2.A periodic reduction in the received strength of a radio transmission. 
// 3.Sports. A moderate, usually controlled slice, as in golf. 
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "irelic.h"
#include "part.h"
#include "column.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////
// functions of ibis::fade
//
// construct a bitmap index from current data
ibis::fade::fade(const ibis::column* c, const char* f, const uint32_t nbase)
    : ibis::relic(0) {
    if (c == 0) return;  // nothing can be done
    col = c;
    try {
	if (c->partition()->nRows() < 1000000) {
	    construct1(f, nbase); // uses more temporary space
	}
	else {
	    construct2(f, nbase); // scan data twice
	}

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg(4);
	    print(lg.buffer());
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning -- ibis::column[" << col->name()
	    << "]::fade::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // constructor

// reconstruct from content of fileManager::storage
// the content of the file (following the 8-byte header) is
// nrows(uint32_t) -- the number of bits in each bit sequence
// nobs (uint32_t) -- the number of bit sequences
// card (uint32_t) -- the number of distinct values, i.e., cardinality
// (padding to ensure the next data element is on 8-byte boundary)
// values (double[card])  -- the distinct values as doubles
// offset(int32_t[nobs+1]) -- the starting positions of the bit sequences (as
//				bit vectors)
// nbases(uint32_t) -- the number of components (bases) used
// cnts (uint32_t[card])    -- the counts for each distinct value
// bases(uint32_t[nbases])  -- the bases sizes
// bitvectors -- the bitvectors one after another
ibis::fade::fade(const ibis::column* c, ibis::fileManager::storage* st,
		 uint32_t offset)
    : ibis::relic(c, st, offset),
      cnts(st, 8*((offset+sizeof(uint32_t)*3+7)/8)+sizeof(uint32_t)
	   +sizeof(int32_t)*(ibis::relic::bits.size()+1)
	   +sizeof(double)*ibis::relic::vals.size(),
	   *(reinterpret_cast<uint32_t*>(st->begin()+offset+
					 sizeof(uint32_t)*2))),
      bases(st, 8*((offset+sizeof(uint32_t)*3+7)/8)+sizeof(uint32_t)
	    +sizeof(int32_t)*(ibis::relic::bits.size()+1)
	    +(sizeof(uint32_t)+sizeof(double))*ibis::relic::vals.size(),
	    *(reinterpret_cast<uint32_t*>
	      (st->begin()+8*((offset+sizeof(uint32_t)*3+7)/8)+
	       sizeof(int32_t)*(ibis::relic::bits.size()+1)+
	       sizeof(double)*ibis::relic::vals.size()))) {
    if (ibis::gVerbose > 8 &&
	FADE == static_cast<ibis::index::INDEX_TYPE>(*(st->begin()+5))) {
	ibis::util::logger lg;
	print(lg.buffer());
    }
} // reconstruct data from content of a file

// the argument is the name of the directory or the index file name
int ibis::fade::write(const char* dt) const {
    if (vals.empty()) return -1;

    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;
    if (fname != 0 || str != 0)
	activate(); // retrieve all bitvectors

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	col->logWarning("fade::write", "unable to open \"%s\" for write",
			fnm.c_str());
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    const uint32_t nb = bases.size();
    const uint32_t card = vals.size();
    const uint32_t nobs = bits.size();
    array_t<int32_t> offs(nobs+1);
    char header[] = "#IBIS\12\0\0";
    header[5] = (char)ibis::index::FADE;
    header[6] = (char)sizeof(int32_t);
    int32_t ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::column[" << col->partition()->name() << "."
	    << col->name() << "]::fade::write(" << fnm << ") failed to write "
	    << "the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr = UnixWrite(fdes, &nrows, sizeof(nobs));
    ierr = UnixWrite(fdes, &nobs, sizeof(nobs));
    ierr = UnixWrite(fdes, &card, sizeof(card));
    offs[0] = 8*((3*sizeof(uint32_t)+15)/8);
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) { // unable to write to file
	UnixClose(fdes);
	remove(fnm.c_str());
	return -4;
    }

    ierr = UnixWrite(fdes, vals.begin(), sizeof(double)*card);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nobs+1), SEEK_CUR);
    ierr = UnixWrite(fdes, &nb, sizeof(nb));
    ierr = UnixWrite(fdes, cnts.begin(), sizeof(uint32_t)*card);
    ierr = UnixWrite(fdes, bases.begin(), sizeof(uint32_t)*nb);
    for (uint32_t i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes, 8*((15+3*sizeof(uint32_t))/8)+sizeof(double)*card,
		    SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
#if defined(linux)
    fsync(fdes);
#endif
    (void) UnixClose(fdes);
    return 0;
} // ibis::fade::write

// write the content to a file already opened
int ibis::fade::write(int fdes) const {
    if (vals.empty()) return -1;
    if (fname != 0 || str != 0)
	activate(); // retrieve all bitvectors
    const int32_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	ibis::util::logMessage("Warning", "ibis::fade::write call to UnixSeek"
			       "(%d, 0, SEEK_CUR) failed ... %s", fdes,
			       strerror(errno));
	return -2;
    }

    const uint32_t nb = bases.size();
    const uint32_t card = vals.size();
    const uint32_t nobs = bits.size();
    array_t<int32_t> offs(nobs+1);
    int ierr = UnixWrite(fdes, &nrows, sizeof(nobs));
    ierr = UnixWrite(fdes, &nobs, sizeof(nobs));
    ierr = UnixWrite(fdes, &card, sizeof(card));
    offs[0] = 8*((start+sizeof(uint32_t)*3+7)/8);
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) {
	LOGGER(ibis::gVerbose >= 1)
	    << "ibis::fade::write(" << fdes << ") failed to seek to" << offs[0];
	UnixSeek(fdes, start, SEEK_SET);
	return -3;
    }

    ierr = UnixWrite(fdes, vals.begin(), sizeof(double)*card);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nobs+1), SEEK_CUR);
    ierr = UnixWrite(fdes, &nb, sizeof(nb));
    ierr = UnixWrite(fdes, cnts.begin(), sizeof(uint32_t)*card);
    ierr = UnixWrite(fdes, bases.begin(), sizeof(uint32_t)*nb);
    for (uint32_t i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes,
		    8*((start+sizeof(uint32_t)*3+7)/8)+sizeof(double)*card,
		    SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
    ierr = UnixSeek(fdes, offs.back(), SEEK_SET);
    return 0;
} // ibis::fade::write

/// Read the index contained in the file named @c f.
int ibis::fade::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;
    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0)
	return -1;

    char header[8];
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    if (8 != UnixRead(fdes, static_cast<void*>(header), 8)) {
	UnixClose(fdes);
	return -2;
    }

    if (!(header[0] == '#' && header[1] == 'I' &&
	  header[2] == 'B' && header[3] == 'I' &&
	  header[4] == 'S' &&
	  header[6] == static_cast<char>(sizeof(int32_t)) &&
	  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return -3;
    }

    uint32_t dim[3]; // nobs, card
    uint32_t begin, end;
    clear(); // clear the current content
    if (fname) delete [] fname;
    fname = ibis::util::strnewdup(fnm.c_str());

    int ierr = UnixRead(fdes, static_cast<void*>(dim), 3*sizeof(uint32_t));
    if (ierr < static_cast<int>(3*sizeof(uint32_t))) {
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
    array_t<double> dbl(fdes, begin, end);
    vals.swap(dbl);
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
    // nbases, cnts, and bases
    uint32_t nb;
    ierr = UnixSeek(fdes, end, SEEK_SET);
    if (ierr != end) {
	LOGGER(ibis::gVerbose >= 1)
	    << "ibis::fade::read(" << fnm << ") failed to seek to " << end;
	UnixClose(fdes);
	clear();
	return -5;
    }

    ierr = UnixRead(fdes, static_cast<void*>(&nb), sizeof(uint32_t));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	clear();
	return -6;
    }
    begin = end + sizeof(uint32_t);
    end += sizeof(uint32_t)*(dim[2]+1);
    if (trymmap) {
	array_t<uint32_t> szt(fname, begin, end);
	cnts.swap(szt);
    }
    else {
	array_t<uint32_t> szt(fdes, begin, end);
	cnts.swap(szt);
    }
    begin = end;
    end += sizeof(uint32_t) * nb;
    {
	array_t<uint32_t> szb(fdes, begin, end);
	bases.swap(szb);
    }
    ibis::fileManager::instance().recordPages(0, end);

    // read the first bitvector
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(dim[1]);
    for (uint32_t i = 0; i < dim[1]; ++i)
	bits[i] = 0;
#if defined(ALWAY_READ_BITVECTOR0)
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
    str = 0;
    (void) UnixClose(fdes);
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fnm.c_str());
    return 0;
} // ibis::fade::read

/// Reconstruct an index from a piece of consecutive memory.
int ibis::fade::read(ibis::fileManager::storage* st) {
    if (st == 0) return -1;
    clear(); // wipe out the existing content
    str = st;

    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    uint32_t pos = 8 + sizeof(uint32_t);
    const uint32_t nobs = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t);
    const uint32_t card = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t) + 7;
    const uint32_t nbases= *(reinterpret_cast<uint32_t*>
			   (st->begin()+8*(pos/8) +
			    sizeof(double)*card+sizeof(uint32_t)*(nobs+1)));
    array_t<int32_t> offs(st, 8*(pos/8) + sizeof(double)*card, nobs+1);
    {
	array_t<double> dbl(st, 8*(pos/8), card);
	vals.swap(dbl);
    }
    {
	array_t<uint32_t> szt(st, 8*(pos/8) + sizeof(double)*card
			    + sizeof(uint32_t)*(nobs+2), card);
	cnts.swap(szt);
    }
    {
	array_t<uint32_t> szb(st, 8*(pos/8) + sizeof(double)*card
			    + sizeof(uint32_t)*(card+nobs+2), nbases);
	bases.swap(szb);
    }

    for (uint32_t i = 0; i < bits.size(); ++ i)
	delete bits[i];
    bits.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i] = 0;
    if (st->isFileMap()) { // only restore the first bitvector
#if defined(ALWAY_READ_BITVECTOR0)
	array_t<ibis::bitvector::word_t>
	    a0(st, offs[0], (offs[1]-offs[0])/
	       sizeof(ibis::bitvector::word_t));
	bits[0] = new ibis::bitvector(a0);
	bits[0]->setSize(nrows);
#endif
	offsets.swap(offs);
	str = st;
    }
    else {
	for (uint32_t i = 0; i < nobs; ++i) {
	    if (offs[i+1] > offs[i]) {
		array_t<ibis::bitvector::word_t>
		    a(st, offs[i], (offs[i+1]-offs[i])/
		      sizeof(ibis::bitvector::word_t));
		ibis::bitvector* btmp = new ibis::bitvector(a);
		btmp->setSize(nrows);
		bits[i] = btmp;
	    }
	}
	str = 0;
    }
    return 0;
} // ibis::fade::read

void ibis::fade::clear() {
    cnts.clear();
    bases.clear();
    ibis::relic::clear();
} // ibis::fade::clear

// assume that the array vals is initialized properly, this function
// converts the value val into a set of bits to be stored in the bitvectors
// contained in bits
// **** CAN ONLY be used by construct2() to build a new ibis::fade index ****
void ibis::fade::setBit(const uint32_t i, const double val) {
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
	kk = jj;
    }
    else if (vals[ii] == val) { // vals[ii] is the same as val
	kk = ii;
    }
    else {
	return;
    }

    // now we know what bitvectors to modify
    const uint32_t nb = bases.size();
    uint32_t offset = 0; // offset into bits
    for (ii = 0; ii < nb; ++ ii) {
	jj = kk % bases[ii];
	if (jj+1 < bases[ii] || bases[ii] == 1)
	    bits[offset+jj]->setBit(i, 1);
	kk /= bases[ii];
	offset += (bases[ii] > 1 ? bases[ii]-1 : bases[ii]);
    }
} // setBit

// this version of the constructor take one pass throught the data by
// constructing a ibis::index::VMap first than construct the fade from the
// VMap -- uses more computer memory than the two-pass version
void ibis::fade::construct1(const char* f, const uint32_t nbase) {
    VMap bmap; // a map between values and their position
    try {
	mapValues(f, bmap);
    }
    catch (...) { // need to clean up bmap
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::fade::construct1 reclaiming storage "
	    "allocated to bitvectors (" << bmap.size() << ")";

	for (VMap::iterator it = bmap.begin(); it != bmap.end(); ++ it)
	    delete (*it).second;
	bmap.clear();
	ibis::fileManager::instance().signalMemoryAvailable();
	throw;
    }
    if (bmap.empty()) return;
    nrows = (*(bmap.begin())).second->size();
    if (nrows != col->partition()->nRows()) {
	for (VMap::iterator it = bmap.begin(); it != bmap.end(); ++ it)
	    delete (*it).second;
	bmap.clear();
	ibis::fileManager::instance().signalMemoryAvailable();

	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::fade::construct1 the bitvectors "
	    "do not have the expected size(" << col->partition()->nRows()
	    << "). stopping..";

	throw ibis::bad_alloc("incorrect bitvector sizes");
    }

    // convert bmap into the current data structure
    // fill the arrays vals and cnts
    const uint32_t card = bmap.size();
    vals.reserve(card);
    cnts.reserve(card);
    for (VMap::const_iterator it = bmap.begin(); it != bmap.end(); ++it) {
	vals.push_back((*it).first);
	cnts.push_back((*it).second->cnt());
    }
    // fill the array bases
    setBases(bases, card, nbase);
    // count the number of bitvectors to genreate
    const uint32_t nb = bases.size();
    uint32_t nobs = 0;
    uint32_t i;
    for (i = 0; i < nb; ++i)
	nobs += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
    if (nobs == 0)
	nobs = 1;
    // allocate enough bitvectors in bits
    bits.resize(nobs);
    for (i = 0; i < nobs; ++i)
	bits[i] = 0;
    if (ibis::gVerbose > 5) {
	col->logMessage("fade::construct", "initialized the array of "
			"bitvectors, start converting %lu bitmaps into %lu-"
			"component range code (with %lu bitvectors)",
			static_cast<long unsigned>(vals.size()),
			static_cast<long unsigned>(nb),
			static_cast<long unsigned>(nobs));
    }

    // generate the correct bitmaps
    i = 0;
    for (VMap::const_iterator it = bmap.begin(); it != bmap.end();
	 ++it, ++i) {
	uint32_t offset = 0;
	uint32_t ii = i;
	for (uint32_t j = 0; j < nb; ++j) {
	    uint32_t k = ii % bases[j];
	    if (k+1 < bases[j] || bases[j] == 1) {
		if (bits[offset+k]) {
		    *(bits[offset+k]) |= *((*it).second);
		}
		else {
		    bits[offset+k] = new ibis::bitvector();
		    bits[offset+k]->copy(*((*it).second));
		    // expected to be operated on more than 64 times
		    if (vals.size()/bases[j] > 64)
			bits[offset+k]->decompress();
		}
	    }
	    ii /= bases[j];
	    offset += (bases[j] > 1 ? bases[j] - 1 : bases[j]);
	}

	delete (*it).second; // no longer need the bitmap
#if defined(DEBUG) || defined(_DEBUG)
	if (ibis::gVerbose > 5 && (i & 255) == 255) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "DEBUG: fade::constructor " << i << " ... ";
	}
#endif
    }
    for (i = 0; i < nobs; ++i) {
	if (bits[i] == 0) {
	    bits[i] = new ibis::bitvector();
	    bits[i]->set(0, nrows);
	}
	else {
	    bits[i]->compress();
	}
    }
#if defined(DEBUG) || defined(_DEBUG)
    LOGGER(ibis::gVerbose >= 6)
	<< "DEBUG: fade::constructor " << vals.size()
	<< "... convert to range encoding ...";
#endif
    // sum up the bitvectors according range-encoding
    nobs = 0;
    for (i = 0; i < nb; ++i) {
	for (uint32_t j = 1; j < bases[i]-1; ++j) {
	    *(bits[nobs+j]) |= *(bits[nobs+j-1]);
	    bits[nobs+j]->compress();
	}
	nobs += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
    }
#if defined(DEBUG) || defined(_DEBUG)
    LOGGER(ibis::gVerbose >= 6) << "DEBUG: fade::constructor DONE";
#endif

    optionalUnpack(bits, col->indexSpec());
    // write out the current content
    if (ibis::gVerbose > 4) {
 	ibis::util::logger lg(4);
 	print(lg.buffer());
    }
} // construct1

// generate a new fade index by passing through the data twice
// (1) scan the data to generate a list of distinct values and their count
// (2) scan the data a second time to produce the bit vectors
void ibis::fade::construct2(const char* f, const uint32_t nbase) {
    histogram* hst = new histogram;
    mapValues(f, *hst); // scan the data to produce the histogram
    if (hst->empty())   // no data, of course no index
	return;

    // convert histogram into two arrays
    uint32_t tmp = hst->size();
    vals.resize(tmp);
    cnts.resize(tmp);
    histogram::const_iterator it = hst->begin();
    for (uint32_t i = 0; i < tmp; ++i) {
	vals[i] = (*it).first;
	cnts[i] = (*it).second;
	++ it;
    }
    delete hst; // no more use for the histogram

    // determie the bases
    setBases(bases, vals.size(), nbase);
    const uint32_t nb = bases.size();

    // allocate the correct number of bitvectors
    uint32_t nobs = 0;
    for (tmp = 0; tmp < nb; ++tmp)
	nobs += (bases[tmp] > 1 ? bases[tmp] - 1 : bases[tmp]);
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
	    mask.copy(arr); // convert arr to a bitvector
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
		col->logWarning("fade::construct", "the data file \"%s\" "
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
	    col->logWarning("fade::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("fade::construct", "the data file \"%s\" "
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
	    col->logWarning("fade::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("fade::construct", "the data file \"%s\" "
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
	    col->logWarning("fade::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("fade::construct", "the data file \"%s\" "
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
	    col->logWarning("fade::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::CATEGORY: // no need for a separate index
	col->logWarning("fade::ctor", "no need for another index");
	return;
    default:
	col->logWarning("fade::ctor", "unable to create bit fade index for "
			"this type of column");
	return;
    }

    // make sure all bit vectors are the same size
    for (uint32_t i = 0; i < nobs; ++i) {
	bits[i]->adjustSize(0, nrows);
    }
    // sum up the bitvectors according to range-encoding
    nobs = 0; // used as a counter now
    for (uint32_t i = 0; i < nb; ++i) {
	for (uint32_t j = 1; j < bases[i]-1; ++j) {
	    *(bits[nobs+j]) |= *(bits[nobs+j-1]);
	    bits[nobs+j-1]->compress();
	}
	if (bases[i] > 1) {
	    bits[nobs+bases[i]-2]->compress();
	    nobs += bases[i] - 1;
	}
	else { // shouldn't have a basis size of 1, but just in case...
	    ++ nobs;
	}
    }

    optionalUnpack(bits, col->indexSpec());
    // write out the current content
    if (ibis::gVerbose > 4) {
 	ibis::util::logger lg(4);
 	print(lg.buffer());
    }
} // ibis::fade::construct2

// a simple function to test the speed of the bitvector operations
void ibis::fade::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    uint32_t i, nloops = 1000000000 / nrows;
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;
    col->logMessage("fade::speedTest", "testing the speed of operator -");

    activate(); // need all bitvectors
    for (i = 0; i < bits.size()-1; ++i) {
	ibis::bitvector* tmp;
	tmp = *(bits[i+1]) & *(bits[i]);
	delete tmp;

	timer.start();
	for (uint32_t j=0; j<nloops; ++j) {
	    tmp = *(bits[i+1]) - *(bits[i]);
	    delete tmp;
	}
	timer.stop();
	{
	    ibis::util::ioLock lock;
	    out << bits[i]->size() << " "
		<< static_cast<double>(bits[i]->bytes() + bits[i+1]->bytes())
		* 4.0 / static_cast<double>(bits[i]->size()) << " "
		<< bits[i]->cnt() << " " << bits[i+1]->cnt() << " "
		<< timer.realTime() / nloops << std::endl;
	}
    }
} // ibis::fade::speedTest

// the printing function
void ibis::fade::print(std::ostream& out) const {
    out << "index(multicomponent range ncomp=" << bases.size() << ") for "
	<< col->partition()->name() << '.' << col->name() << " contains "
	<< bits.size() << " bitvectors for " << nrows
	<< " objects with " << vals.size()
	<< " distinct values\nThe base sizes: ";
    for (uint32_t i=0; i<bases.size(); ++i)
	out << bases[i] << ' ';
    const uint32_t nobs = bits.size();
    out << "\nbitvector information (number of set bits, number of "
	"bytes)\n";
    for (uint32_t i=0; i<nobs; ++i) {
	if (bits[i]) {
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
    out << std::endl;
} // ibis::fade::print

// create index based data in dt -- have to start from data directly
long ibis::fade::append(const char* dt, const char* df, uint32_t nnew) {
    const uint32_t nb = bases.size();
    clear();		// clear the current content
    construct2(dt, nb); // build the new index by scanning data twice
    write(dt);		// write out the new content
    return nnew;
} // ibis::fade::append

// compute the bitvector that is the answer for the query x = b
void ibis::fade::evalEQ(ibis::bitvector& res, uint32_t b) const {
    if (b >= vals.size()) {
	res.set(0, nrows);
    }
    else {
	uint32_t offset = 0;
	res.set(1, nrows);
	for (uint32_t i=0; i < bases.size(); ++i) {
	    uint32_t k = b % bases[i];
	    if (k+1 < bases[i] || bases[i] == 1) {
		if (bits[offset+k] == 0)
		    activate(offset+k);
		if (bits[offset+k] != 0)
		    res &= *(bits[offset+k]);
		else
		    res.set(0, res.size());
	    }
	    if (k > 0) {
		if (bits[offset+k-1] == 0)
		    activate(offset+k-1);
		if (bits[offset+k-1] != 0)
		    res -= *(bits[offset+k-1]);
	    }
	    offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
	    b /= bases[i];
	}
    }
} // ibis::fade::evalEQ

// compute the bitvector that is the answer for the query x <= b
void ibis::fade::evalLE(ibis::bitvector& res, uint32_t b) const {
    if (b+1 >= vals.size()) {
	res.set(1, nrows);
    }
    else {
	uint32_t i = 0; // index into components
	uint32_t offset = 0;
	// skip till the first component that isn't the maximum value
	while (i < bases.size() && b % bases[i] == bases[i]-1) {
	    offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
	    b /= bases[i];
	    ++ i;
	}
	// copy the first non-maximum component
	if (i < bases.size()) {
	    if (bits[offset+(b%bases[i])] == 0)
		activate(offset+(b%bases[i]));
	    if (bits[offset+(b%bases[i])] != 0)
		res.copy(*(bits[offset+(b%bases[i])]));
	    else
		res.set(0, nrows);
	    offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
	    b /= bases[i];
	}
	else {
	    res.set(1, nrows);
	}
	++ i;
	// deal with the remaining components
	while (i < bases.size()) {
	    uint32_t k = b % bases[i];
	    if (k+1 < bases[i] || bases[i] == 1) {
		if (bits[offset+k] == 0)
		    activate(offset+k);
		if (bits[offset+k] != 0)
		    res &= *(bits[offset+k]);
		else
		    res.set(0, res.size());
	    }
	    if (k > 0) {
		if (bits[offset+k-1] == 0)
		    activate(offset+k-1);
		if (bits[offset+k-1] != 0)
		    res |= *(bits[offset+k-1]);
	    }
	    offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
	    b /= bases[i];
	    ++ i;
	}
    }
} // ibis::fade::evalLE

// compute the bitvector that answers the query b0 < x <= b1
void ibis::fade::evalLL(ibis::bitvector& res, uint32_t b0, uint32_t b1) const {
    if (b0 >= b1) { // no hit
	res.set(0, nrows);
    }
    else if (b1+1 >= vals.size()) { // x > b0
	evalLE(res, b0);
	res.flip();
    }
    else { // the intended general case
	// res temporarily stores the result of x <= b1
	ibis::bitvector low; // x <= b0
	uint32_t k0, k1;
	uint32_t i = 0;
	uint32_t offset = 0;
	// skip till the first component that isn't the maximum value
	while (i < bases.size()) {
	    k0 = b0 % bases[i];
	    k1 = b1 % bases[i];
	    if (k0 == bases[i]-1 && k1 == bases[i]-1) {
		offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
		b0 /= bases[i];
		b1 /= bases[i];
		++ i;
	    }
	    else {
		break;
	    }
	}
	// the first non-maximum component
	if (i < bases.size()) {
	    k0 = b0 % bases[i];
	    k1 = b1 % bases[i];
	    if (k0+1 < bases[i]) {
		if (bits[offset+k0] == 0)
		    activate(offset+k0);
		if (bits[offset+k0] != 0)
		    low = *(bits[offset+k0]);
		else
		    low.set(0, nrows);
	    }
	    else {
		low.set(1, nrows);
	    }
	    if (k1+1 < bases[i]) {
		if (bits[offset+k1] == 0)
		    activate(offset+k1);
		if (bits[offset+k1] != 0)
		    res = *(bits[offset+k1]);
		else
		    res.set(0, nrows);
	    }
	    else {
		res.set(1, nrows);
	    }
	    offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
	    b0 /= bases[i];
	    b1 /= bases[i];
	}
	else {
	    res.set(0, nrows);
	}
	++ i;
	// deal with the remaining components
	while (i < bases.size()) {
	    if (b1 > b0) { // low and res has to be separated
		k0 = b0 % bases[i];
		k1 = b1 % bases[i];
		b0 /= bases[i];
		b1 /= bases[i];
		if (k0+1 < bases[i] || bases[i] == 1) {
		    if (bits[offset+k0] == 0)
			activate(offset+k0);
		    if (bits[offset+k0] != 0)
			low &= *(bits[offset+k0]);
		    else
			low.set(0, low.size());
		}
		if (k1+1 < bases[i] || bases[i] == 1) {
		    if (bits[offset+k1] == 0)
			activate(offset+k1);
		    if (bits[offset+k1] != 0)
			res &= *(bits[offset+k1]);
		    else
			res.set(0, res.size());
		}
		if (k0 > 0) {
		    if (bits[offset+k0-1] == 0)
			activate(offset+k0-1);
		    if (bits[offset+k0-1] != 0)
			low |= *(bits[offset+k0-1]);
		}
		if (k1 > 0) {
		    if (bits[offset+k1-1] == 0)
			activate(offset+k1-1);
		    if (bits[offset+k1-1] != 0)
			res |= *(bits[offset+k1-1]);
		}
		offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
	    }
	    else { // the more significant components are the same
		res -= low;
		low.clear(); // no longer need low
		while (i < bases.size()) {
		    k1 = b1 % bases[i];
		    if (k1+1 < bases[i] || bases[i] == 1) {
			if (bits[offset+k1] == 0)
			    activate(offset+k1);
			if (bits[offset+k1] != 0)
			    res &= *(bits[offset+k1]);
			else
			    res.set(0, res.size());
		    }
		    if (k1 > 0) {
			if (bits[offset+k1-1] == 0)
			    activate(offset+k1-1);
			if (bits[offset+k1-1] != 0)
			    res -= *(bits[offset+k1-1]);
		    }
		    offset += (bases[i] > 1 ? bases[i] - 1 : bases[i]);
		    b1 /= bases[i];
		    ++ i;
		}
	    }
	    ++ i;
	}
	if (low.size() == res.size()) { // subtract low from res
	    res -= low;
	    low.clear();
	}
    }
} // ibis::fade::evalLL

// Evaluate the query expression
long ibis::fade::evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower) const {
    // values in the range [hit0, hit1) satisfy the query expression
    uint32_t hit0, hit1;
    if (bits.empty()) {
	lower.set(0, nrows);
	return 0;
    }
    locate(expr, hit0, hit1);

    // actually accumulate the bits in the range [hit0, hit1)
    if (hit1 <= hit0) {
	lower.set(0, nrows);
    }
    else if (hit0+1 == hit1) { // equal to one single value
	evalEQ(lower, hit0);
    }
    else if (hit0 == 0) { // < hit1
	evalLE(lower, hit1-1);
    }
    else if (hit1 == vals.size()) { // >= hit0 (as NOT (<= hit0-1))
	evalLE(lower, hit0-1);
	lower.flip();
    }
    else { // need to go through most bitvectors four times
	evalLL(lower, hit0-1, hit1-1);	// (hit0-1 < x <= hit1-1]
    }
    return lower.cnt();
} // ibis::fade::evaluate

// Evaluate a set of discrete range conditions.
long ibis::fade::evaluate(const ibis::qDiscreteRange& expr,
			   ibis::bitvector& lower) const {
    const std::vector<double>& varr = expr.getValues();
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
} // ibis::fade::evaluate

void ibis::fade::binWeights(std::vector<uint32_t>& c) const {
    c.resize(cnts.size());
    for (uint32_t i = 0; i < cnts.size(); ++ i) {
	c[i] = cnts[i];
    }
} // ibis::fade::binWeights

// return the number of hits
uint32_t ibis::fade::estimate(const ibis::qContinuousRange& expr) const {
    if (bits.empty()) return 0;

    uint32_t h0, h1;
    locate(expr, h0, h1);

    uint32_t nhits = 0;
    for (uint32_t i=h0; i<h1; ++i)
	nhits += cnts[i];
    return nhits;
} // ibis::fade::estimate()

double ibis::fade::getSum() const {
    double ret = 0;
    if (vals.size() == cnts.size()) {
	for (uint32_t i = 0; i < vals.size(); ++ i)
	    ret += vals[i] * cnts[i];
    }
    else {
	col->logWarning("fade::getSum", "internal error - arrays "
			"vals[%lu] and cnts[%lu] are expected to have "
			"the same size but are not",
			static_cast<long unsigned>(vals.size()),
			static_cast<long unsigned>(cnts.size()));
	ibis::util::setNaN(ret);
    }
    return ret;
} // ibis::fade::getSum

// NOT a proper implementation
double ibis::fade::estimateCost(const ibis::qContinuousRange& expr) const {
    double ret = estimate(expr);
    return ret;
} // ibis::fade::estimateCost

// double ibis::fade::estimateCost(const ibis::qDiscreteRange& expr) const {
// } // ibis::fade::estimateCost

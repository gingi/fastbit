// $Id$
// Author: John Wu <John.Wu@ACM.org>
// Copyright 2000-2008 the Regents of the University of California
//
// This file contains the implementation of the class called ibis::egale
// -- the multicomponent equality code on bins
//
// egale is French word for "equal"
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "ibin.h"
#include "part.h"
#include "column.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////
// functions of ibis::egale
//
// construct a bitmap index from current data
ibis::egale::egale(const ibis::column* c, const char* f,
		   const uint32_t nb) : ibis::bin(c, f), nbits(0), nbases(nb) {
    if (c == 0) return;  // nothing can be done
    if (nbases < 2)
	nbases = 2;
    try {
	if (bits.empty()) { // did not generate a binned index
	    setBoundaries(f);	// fill the array bounds and nobs
	    setBases(bases, nobs, nbases);	// fill the array bases
	    nbases = bases.size();
	    const uint32_t nev = col->partition()->nRows();
	    if (1e8 < static_cast<double>(nev)*static_cast<double>(nobs)) {
		binning(f); // generate the simple bins first
		convert();  // convert from simple bins
	    }
	    else { // directly generate multicomponent bins
		construct(f);
	    }
	}
	else {
	    setBases(bases, nobs, nbases);	// fill the array bases
	    convert();  // convert from 1 level to multilevel equality code
	}

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg(4);
	    print(lg.buffer());
	}
    }
    catch (...) {
	clear();
	throw;
    }
} // constructor

// a constructor that takes known bounds and bases
ibis::egale::egale(const ibis::column* c, const char* f,
		   const array_t<double>& bd, const array_t<uint32_t> bs)
    : ibis::bin(c, f, bd), nbits(bs[0]), nbases(bs.size()), bases(bs) {
    // nbits temporarily used for error checking
    for (uint32_t i = 1; i < nbases; ++i) {
	nbits *= bases[i];
    }
    if (nbits > nobs) {
	col->logWarning("egale::ctr", "The product of all %lu bases (=%lu) "
			"is expected to be larger than the number of bins "
			"(=%lu)", static_cast<long unsigned>(nbases),
			static_cast<long unsigned>(nbits),
			static_cast<long unsigned>(nobs));
	throw "ibis::egale::egale bases too small";
    }
    try { // convert from simple equality code to multicomponent code
	convert();

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg(4);
	    print(lg.buffer());
	}
    }
    catch (...) {
	clear();
	throw;
    }
} // constructor

// Copy from an ibis::bin object.
ibis::egale::egale(const ibis::bin& rhs, uint32_t nb)
    : ibis::bin(rhs), nbits(0), nbases(nb) {
    if (nbases < 2)
	nbases = 2;
    try {
	setBases(bases, nobs, nbases);
	nbases = bases.size();
	convert();

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg(4);
	    print(lg.buffer());
	}
    }
    catch (...) {
	clear();
	throw;
    }
} // copy from ibis::bin

// reconstruct from content of fileManager::storage
// the content of the file (following the 8-byte header) is
// nrows  (uint32_t)          -- number of bits in a bitvector
// nobs   (uint32_t)          -- number of bins
// nbits  (uint32_t)          -- number of bitvectors
//        padding to ensure bounds starts on multiple of 8.
// bounds (double[nobs])    -- bind boundaries
// maxval (double[nobs])    -- the maximum value in each bin
// minval (double[nobs])    -- the minimum value in each bin
// offset (int32_t[nbits+1])  -- starting position of the bitvectors
// cnts   (uint32_t[nobs])    -- number of records in each bin
// nbases (uint32_t)          -- number of components (size of array bases)
// bases  (uint32_t[nbases])  -- the bases sizes
// bitvectors               -- the bitvectors one after another
ibis::egale::egale(const ibis::column* c, ibis::fileManager::storage* st,
		   uint32_t offset) :
    ibis::bin(c, *(reinterpret_cast<uint32_t*>
		   (st->begin()+offset+2*sizeof(uint32_t))), st, offset),
    nbits(*(reinterpret_cast<uint32_t*>(st->begin()+offset+
					2*sizeof(uint32_t)))),
    nbases(*(reinterpret_cast<uint32_t*>(st->begin()+
				       8*((7+offset+3*sizeof(uint32_t))/8)+
				       nobs*sizeof(uint32_t)+
				       (nbits+1)*sizeof(int32_t)+
				       3*nobs*sizeof(double)))),
    cnts(st, 8*((7+offset+3*sizeof(uint32_t))/8)+(nbits+1)*sizeof(int32_t)
	 +3*nobs*sizeof(double), nobs),
    bases(st, 8*((7+offset+3*sizeof(uint32_t))/8)+(nobs+1)*sizeof(uint32_t)+
	  (nbits+1)*sizeof(int32_t)+3*nobs*sizeof(double), nbases) {
    if (ibis::gVerbose > 8 &&
	static_cast<ibis::index::INDEX_TYPE>(*(st->begin()+5)) == EGALE) {
	ibis::util::logger lg(8);
	print(lg.buffer());
    }
} // reconstruct data from content of a file

// the argument can be the name of the directory or the name of the file
void ibis::egale::write(const char* dt) const {
    if (nobs == 0) return;

    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return;

    if (str != 0 || fname != 0)
	activate();

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	col->logWarning("egale::write", "unable to open \"%s\" for write",
			fnm.c_str());
	return;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    array_t<int32_t> offs(nbits+1);
    char header[] = "#IBIS\15\0\0";
    header[5] = (char)ibis::index::EGALE;
    header[6] = (char)sizeof(int32_t);
    int32_t ierr = UnixWrite(fdes, header, 8);
    ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nbits, sizeof(uint32_t));
    offs[0] = 8*((15+3*sizeof(uint32_t))/8);
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) {
	UnixClose(fdes);
	remove(fnm.c_str());
	return;
    }

    ierr = UnixWrite(fdes, bounds.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, maxval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, minval.begin(), sizeof(double)*nobs);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nbits+1), SEEK_CUR);
    ierr = UnixWrite(fdes, cnts.begin(), sizeof(uint32_t)*nobs);
    ierr = UnixWrite(fdes, &nbases, sizeof(uint32_t));
    ierr = UnixWrite(fdes, bases.begin(), sizeof(uint32_t)*nbases);
    for (uint32_t i = 0; i < nbits; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	if (bits[i])
	    bits[i]->write(fdes);
    }
    offs[nbits] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes,
		    8*((15+3*sizeof(uint32_t))/8)+3*sizeof(double)*nobs,
		    SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nbits+1));
#if _POSIX_FSYNC+0 > 0
    (void) fsync(fdes); // write to disk
#endif
    ierr = UnixClose(fdes);
} // ibis::egale::write

// write directly to a file that is already opened by the caller
void ibis::egale::write(int fdes) const {
    int32_t ierr;
    array_t<int32_t> offs(nbits+1);
    const int32_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 0) {
	LOGGER(ibis::gVerbose >= 1) << "ibis::egale::write(" << fdes << ") failed to compute "
	    "the current file pointer position, must be an invalid file";
	return;
    }
    ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nbits, sizeof(uint32_t));
    offs[0] = 8*((7+start+3*sizeof(uint32_t))/8);
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) {
	LOGGER(ibis::gVerbose >= 1) << "ibis::egale::write(" << fdes << ") failed to seek to "
		  << offs[0];
	UnixSeek(fdes, start, SEEK_SET);
	return;
    }
    ierr = UnixWrite(fdes, bounds.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, maxval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, minval.begin(), sizeof(double)*nobs);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nbits+1), SEEK_CUR);
    ierr = UnixWrite(fdes, cnts.begin(), sizeof(uint32_t)*nobs);
    ierr = UnixWrite(fdes, &nbases, sizeof(uint32_t));
    ierr = UnixWrite(fdes, bases.begin(), sizeof(uint32_t)*nbases);
    for (uint32_t i = 0; i < nbits; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nbits] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes,
		    8*((7+start+3*sizeof(uint32_t))/8)+3*sizeof(double)*nobs,
		    SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nbits+1));
    ierr = UnixSeek(fdes, offs[nbits], SEEK_SET);
} // ibis::egale::write

// read from a file
void ibis::egale::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);
    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0)
	return;

    char header[8];
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    if (8 != UnixRead(fdes, static_cast<void*>(header), 8)) {
	UnixClose(fdes);
	return;
    }

    if (!(header[0] == '#' && header[1] == 'I' &&
	  header[2] == 'B' && header[3] == 'I' &&
	  header[4] == 'S' &&
	  header[6] == static_cast<char>(sizeof(int32_t)) &&
	  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return;
    }

    uint32_t begin, end;
    clear(); // clear the existing content
    fname = ibis::util::strnewdup(fnm.c_str());

    int ierr = UnixRead(fdes, static_cast<void*>(&nrows), sizeof(nrows));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	clear();
	return;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&nobs), sizeof(nobs));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	clear();
	return;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&nbits), sizeof(nbits));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	clear();
	return;
    }

    bool trymmap = false;
#if defined(HAS_FILE_MAP)
    trymmap = (nobs > ibis::fileManager::pageSize());
#endif
    // read bounds
    begin = 8*((15 + 3 * sizeof(uint32_t))/8);
    end = begin + sizeof(double) * nobs;
    if (trymmap) {
	array_t<double> dbl(fname, begin, end);
	bounds.swap(dbl);
    }
    else {
	array_t<double> dbl(fdes, begin, end);
	bounds.swap(dbl);
    }

    // read maxval
    begin = end;
    end += sizeof(double) * nobs;
    if (trymmap) {
	array_t<double> dbl(fname, begin, end);
	maxval.swap(dbl);
    }
    else {
	array_t<double> dbl(fdes, begin, end);
	maxval.swap(dbl);
    }

    // read minval
    begin = end;
    end += sizeof(double) * nobs;
    if (trymmap) {
	array_t<double> dbl(fname, begin, end);
	minval.swap(dbl);
    }
    else {
	array_t<double> dbl(fdes, begin, end);
	minval.swap(dbl);
    }

    begin = end;
    end += sizeof(int32_t) * (nbits + 1);
    if (trymmap && nbits > ibis::fileManager::pageSize()) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }
    // cnts
    begin = end;
    end += sizeof(uint32_t) * (nobs);
    if (trymmap) {
	array_t<uint32_t> szt(fname, begin, end);
	cnts.swap(szt);
    }
    else {
	array_t<uint32_t> szt(fdes, begin, end);
	cnts.swap(szt);
    }
    // nbases and bases
    ierr = UnixSeek(fdes, end, SEEK_SET);
    if (ierr != end) {
	clear();
	UnixClose(fdes);
	LOGGER(ibis::gVerbose >= 1) << "ibis::egale::read(" << fnm << ") failed to seek to "
		  << end;
	return;
    }
    ierr = UnixRead(fdes, &nbases, sizeof(uint32_t));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	clear();
	return;
    }
    begin = end + sizeof(uint32_t);
    end += sizeof(uint32_t) * (nbases + 1);
    {
	array_t<uint32_t> szt(fdes, begin, end);
	bases.swap(szt);
    }
    ibis::fileManager::instance().recordPages(0, end);

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nbits);
    for (uint32_t i = 0; i < nbits; ++i)
	bits[i] = 0;

#if defined(ALWAY_READ_BITVECTOR0)
    if (offsets[1] > offsets[0]) {// read the first bitvector
	array_t<ibis::bitvector::word_t> a0(fdes, offsets[0], offsets[1]);
	bits[0] = new ibis::bitvector(a0);
#if defined(WAH_CHECK_SIZE)
	if (bits[0]->size() != nrows)
	    col->logWarning("readIndex", "the size (%lu) of 1st "
			    "bitvector (from \"%s\") differs "
			    "from nRows (%lu)",
			    static_cast<long unsigned>(bits[0]->size()),
			    fnm.c_str(),
			    static_cast<long unsigned>(nrows));
#else
	bits[0]->setSize(nrows);
#endif
    }
    else {
	bits[0] = new ibis::bitvector;
	bits[0]->set(0, nrows);
    }
#endif
    UnixClose(fdes);
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fnm.c_str());
} // ibis::egale::read

// read from a ibis::fileManager::storage
void ibis::egale::read(ibis::fileManager::storage* st) {
    if (st == 0) return;
    clear(); // wipe out the existing content
    str = st;

    uint32_t begin;
    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    begin = 8 + sizeof(uint32_t);
    nobs = *(reinterpret_cast<uint32_t*>(st->begin()+begin));
    begin += sizeof(uint32_t);
    nbits = *(reinterpret_cast<uint32_t*>(st->begin()+begin));
    begin = 8*((15 + 3 * sizeof(uint32_t))/8);
    {
	array_t<double> dbl(st, begin, nobs);
	bounds.swap(dbl);
    }
    begin += nobs * sizeof(double);
    {
	array_t<double> dbl(st, begin, nobs);
	maxval.swap(dbl);
    }
    begin += nobs * sizeof(double);
    {
	array_t<double> dbl(st, begin, nobs);
	minval.swap(dbl);
    }

    begin += nobs * sizeof(double);
    array_t<int32_t> offs(st, begin, nbits+1);
    begin += sizeof(int32_t) * (nbits + 1);
    {
	array_t<uint32_t> szt(st, begin, nobs);
	cnts.swap(szt);
    }
    begin += sizeof(uint32_t) * nobs;
    nbases = *(reinterpret_cast<uint32_t*>(st->begin() + begin));
    begin += sizeof(uint32_t);
    {
	array_t<uint32_t> szt(st, begin, nbases);
	bases.swap(szt);
    }

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nbits);
    for (uint32_t i = 0; i < nbits; ++i)
	bits[i] = 0;

    if (st->isFileMap()) { // map only the first bitvector
#if defined(ALWAY_READ_BITVECTOR0)
	if (offs[1] > offs[0]) {
	    array_t<ibis::bitvector::word_t>
		a0(st, offs[0],
		   (offs[1] - offs[0]) / sizeof(ibis::bitvector::word_t));
	    bits[0] = new ibis::bitvector(a0);
	    bits[0]->setSize(nrows);
	}
	else {
	    bits[0] = new ibis::bitvector;
	    bits[0]->set(0, nrows);
	}
#endif
	offsets.swap(offs);
	str = st;
    }
    else { // regenerate every bitvector because all are in memory
	for (uint32_t i = 0; i < nbits; ++i) {
	    array_t<ibis::bitvector::word_t>
		a(st, offs[i], (offs[i+1] - offs[i]) /
		  sizeof(ibis::bitvector::word_t));
	    bits[i] = new ibis::bitvector(a);
	    bits[i]->setSize(nrows);
	}
	str = 0;
    }
} // ibis::egale::read

// convert from the one component equality code to a multicomponent
// equality code
void ibis::egale::convert() {
    //activate(); // make sure all bitvectors are here
    // store the current bitvectors in simple
    std::vector<ibis::bitvector*> simple(bits);

    // count the number of bitvectors to genreate
    uint32_t i;
    nbits = bases[0];
    nbases = bases.size();
    for (i = 0; nrows == 0 && i < nobs; ++ i)
	if (bits[i])
	    nrows = bits[i]->size();
    for (i = 1; i < nbases; ++i)
	nbits += bases[i];
    // allocate enough bitvectors in bits
    bits.resize(nbits);
    for (i = 0; i < nbits; ++i)
	bits[i] = 0;
    cnts.resize(nobs);
    if (ibis::gVerbose > 5) {
	col->logMessage("egale::convert", "initialized the array of "
			"bitvectors, start converting %lu bitmaps into %lu-"
			"component equality code (with %lu bitvectors)",
			static_cast<long unsigned>(nobs),
			static_cast<long unsigned>(nbases),
			static_cast<long unsigned>(nbits));
    }

    // generate the correct bitmaps
    if (nbases > 1) {
	for (i = 0; i < nobs; ++i) {
	    if (simple[i] != 0) {
		uint32_t offset = 0;
		uint32_t ii = i;
		for (uint32_t j = 0; j < nbases; ++j) {
		    uint32_t k = ii % bases[j];
		    if (bits[offset+k]) {
			*(bits[offset+k]) |= *(simple[i]);
		    }
		    else {
			bits[offset+k] = new ibis::bitvector();
			bits[offset+k]->copy(*(simple[i]));
			// expected to be operated on more than 64 times
			if (nobs > 64*bases[j])
			    bits[offset+k]->decompress();
		    }
		    ii /= bases[j];
		    offset += bases[j];
		}

		cnts[i] = simple[i]->cnt();
		delete simple[i]; // no longer need the bitmap
	    }
	    else {
		cnts[i] = 0;
	    }
#if defined(DEBUG)
	    if (ibis::gVerbose > 11 && (i & 255) == 255) {
		LOGGER(ibis::gVerbose >= 0)
		    << "DEBUG: ibis::egale::convert " << i << " ...";
	    }
#endif
	}

	for (i = 0; i < nbits; ++i) {
	    if (bits[i] == 0) {
		bits[i] = new ibis::bitvector();
		bits[i]->set(0, nrows);
	    }
	    else {
		bits[i]->compress();
	    }
	}
    }
    else { // one component -- only need to copy the pointers
	for (i = 0; i < nobs; ++i) {
	    bits[i] = simple[i];
	    cnts[i] = simple[i]->cnt();
	}
    }
    simple.clear();
#if defined(DEBUG)
    if (ibis::gVerbose > 11) {
	LOGGER(ibis::gVerbose >= 0)
	    << "DEBUG: ibis::egale::convert " << nobs << " DONE";
    }
#endif
    optionalUnpack(bits, col->indexSpec());
} // convert

// assume that the array bounds is initialized properly, this function
// converts the value val into a set of bits to be stored in the bitvectors
// contained in bits
// **** CAN ONLY be used by construct() to build a new ibis::egale index ****
void ibis::egale::setBit(const uint32_t i, const double val) {
    // perform a binary search to locate position of val in bounds
    uint32_t kk = locate(val);

    // now we know what bitvectors to modify
    ++ cnts[kk];
    if (val > maxval[kk])
	maxval[kk] = val;
    if (val < minval[kk])
	minval[kk] = val;
    uint32_t offset = 0; // offset into bits
    for (uint32_t ii = 0; ii < nbases; ++ ii) {
	uint32_t jj = kk % bases[ii];
	bits[offset+jj]->setBit(i, 1);
	offset += bases[ii];
	kk /= bases[ii];
    }
} // setBit

// generate a new egale index by directly setting the bits in the
// multicomponent bitvectors -- the alternative was to build a simple equality
// index first than convert.  Directly build the multicomponent scheme might
// use less space, at least we done have to generate the simple encoding.
void ibis::egale::construct(const char* f) {
    // determine the number of bitvectors to use
    nbits = bases[0];
    for (uint32_t i = 1; i < nbases; ++i)
	nbits += bases[i];

    // clear the current content of the bits and allocate space for new ones
    for (std::vector<ibis::bitvector*>::iterator it = bits.begin();
	 it != bits.end(); ++it)
	delete *it;
    bits.resize(nbits);
    for (uint32_t i = 0; i < nbits; ++i)
	bits[i] = new ibis::bitvector;

    // initialize cnts, maxval and minval
    cnts.resize(nobs);
    maxval.resize(nobs);
    minval.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i) {
	maxval[i] = - DBL_MAX;
	minval[i] = DBL_MAX;
	cnts[i] = 0;
    }

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
		col->logWarning("egale::construct", "the data file \"%s\" "
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
	    col->logWarning("egale::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("egale::construct", "the data file \"%s\" "
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
	    col->logWarning("egale::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("egale::construct", "the data file \"%s\" "
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
	    col->logWarning("egale::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    if (val.size() > mask.size()) {
		col->logWarning("egale::construct", "the data file \"%s\" "
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
	    col->logWarning("egale::construct", "unable to read %s",
			    fnm.c_str());
	}
	break;}
    case ibis::CATEGORY: // no need for a separate index
	col->logWarning("egale::ctor", "no need for another index");
	return;
    default:
	col->logWarning("egale::ctor", "unable to create bit egale index for "
			"this type of column");
	return;
    }

    // make sure all bit vectors are the same size
    for (uint32_t i = 0; i < nbits; ++i) {
	bits[i]->adjustSize(0, nrows);
    }
    optionalUnpack(bits, col->indexSpec());

    // write out the current content
    if (ibis::gVerbose > 4) {
 	ibis::util::logger lg(4);
 	print(lg.buffer());
    }
} // ibis::egale::construct

// a simple function to test the speed of the bitvector operations
void ibis::egale::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    uint32_t i, nloops = 1000000000 / nrows;
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;
    col->logMessage("egale::speedTest", "testing the speed of operator |");

    activate();
    for (i = 0; i < bits.size()-1; ++i) {
	ibis::bitvector* tmp;
	tmp = *(bits[i+1]) | *(bits[i]);
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
		<< timer.realTime() / nloops << std::endl;
	}
    }
} // ibis::egale::speedTest()

// the printing function
void ibis::egale::print(std::ostream& out) const {
    out << col->partition()->name() << '.' << col->name()
	<< ".index(MCBin equality code ncomp=" << bases.size()
	<< " nbins=" << nobs << ") contains " << bits.size()
	<< " bitmaps for " << nrows << " objects\nThe base sizes: ";
    for (uint32_t i = 0; i < nbases; ++ i)
	out << bases[i] << ' ';
    out << "\nbitvector information (number of set bits, number of "
	"bytes)\n";
    for (uint32_t i = 0; i < nbits; ++ i) {
	if (bits[i]) {
	    out << i << '\t' << bits[i]->cnt() << '\t'
		<< bits[i]->bytes() << "\n";
	}
    }
    if (ibis::gVerbose > 6) { // also print the list of distinct values
	out << "bin boundary [minval, maxval in bin] number of records\n";
	for (uint32_t i = 0; i < nobs; ++ i) {
	    out.precision(12);
	    out << bounds[i] << "\t[" << minval[i] << ", " << maxval[i]
		<< "]\t" << cnts[i] << "\n";
	}
    }
    out << std::endl;
} // ibis::egale::print()

// create index based data in dt -- have to start from data directly
long ibis::egale::append(const char* dt, const char* df, uint32_t nnew) {
    std::string ff, ft;
    dataFileName(df, ff);
    dataFileName(dt, ft);
    uint32_t sf = ibis::util::getFileSize(ff.c_str());
    uint32_t st = ibis::util::getFileSize(ft.c_str());
    if (sf >= (st >> 1)) {
	clear();
	construct(dt);	// rebuild the new index using the combined data
    }
    else { // attempt to make use of the existing index
	// first bin the new data using the same bin boundaries
	ibis::egale idxf(col, df, bounds, bases);
	uint32_t tot = 0;
	for (uint32_t i=0; i < nobs; ++i) {
	    tot += cnts[i] + idxf.cnts[i];
	}
	uint32_t outside = cnts[0] + idxf.cnts[0] + cnts.back() +
	    idxf.cnts.back();
	if (outside > tot / nobs) { // need to rescan the data
	    array_t<double> bnds;
	    setBoundaries(bnds, idxf, idxf.cnts, idxf.cnts);
	    clear();		// clear the current content
	    binning(dt, bnds);	// generate the new bin boundaries
	}
	else { // don't rescan the data
	    ibis::bin::append(idxf); // simply concatenate the bit vectors
	    // update min, max and cnts
	    for (uint32_t i = 0; i < nobs; ++i) {
		cnts[i] += idxf.cnts[i];
		if (minval[i] > idxf.minval[i])
		    minval[i] = idxf.minval[i];
		if (maxval[i] < idxf.maxval[i])
		    maxval[i] = idxf.maxval[i];
	    }
	}
    }
    write(dt);		// write out the new content
    return nnew;
} // ibis::egale::append()

// add up bits[ib:ie-1] and to res -- must execute \sum_{i=ib}^{ie}, can not
// use compelement
void ibis::egale::addBins_(uint32_t ib, uint32_t ie,
			   ibis::bitvector& res) const {
    const uint32_t nbs = bits.size();
    if (res.size() == 0) {
	res.set(0, nrows);
    }
    if (ie > nbs)
	ie = nbs;
    if (ib >= ie || ib >= nbs) {
	return;
    }
    else if (ib == 0 && ie == nbs) {
	res.set(1, nrows);
	return;
    }

    horometer timer;
    bool decmp = false;
    if (ibis::gVerbose > 4)
	timer.start();

    activate(ib, ie);
    // first determine whether or not to decompres the result
    if (ie-ib>64) {
	decmp = true;
    }
    else if (ie - ib > 3) {
	uint32_t tot = 0;
	for (uint32_t i = ib; i < ie; ++i)
	    tot += bits[i]->bytes();
	if (tot > (nrows >> 2))
	    decmp = true;
	else if (tot > (nrows >> 3) && ie-ib > 4)
	    decmp = true;
    }
    if (decmp) { // use decompressed res
	if (ibis::gVerbose > 5)
	    ibis::util::logMessage("ibis::egale", "addBins(%lu, %lu) using "
				   "uncompressed bitvector",
				   static_cast<long unsigned>(ib),
				   static_cast<long unsigned>(ie));
	res |= *(bits[ib]);
	res.decompress();
	for (uint32_t i = ib+1; i < ie; ++i)
	    res |= *(bits[i]);
    }
    else if (ie > ib+2) { // use compressed res
	if (ibis::gVerbose > 5) 
	    ibis::util::logMessage("ibis::egale", "addBins(%lu, %lu) using "
				   "compressed bitvector",
				   static_cast<long unsigned>(ib),
				   static_cast<long unsigned>(ie));
	// first determine an good evaluation order (ind)
	std::vector<uint32_t> ind;
	uint32_t i, j, k;
	ind.reserve(ie-ib);
	for (i = ib; i < ie; ++i)
	    ind.push_back(i);
	// sort ind according the size of the bitvectors (insertion sort)
	for (i = 0; i < ie-ib-1; ++i) {
	    k = i + 1;
	    for (j = k+1; j < ie-ib; ++j)
		if (bits[ind[j]]->bytes() < bits[ind[k]]->bytes())
		    k = j;
	    if (bits[ind[i]]->bytes() > bits[ind[k]]->bytes()) {
		j = ind[i];
		ind[i] = ind[k];
		ind[k] = j;
	    }
	    else {
		++ i;
		if (bits[ind[i]]->bytes() > bits[ind[k]]->bytes()) {
		    j = ind[i];
		    ind[i] = ind[k];
		    ind[k] = j;
		}
	    }
	}
	// evaluate according the order ind
	for (i = 0; i < ie-ib; ++i)
	    res |= *(bits[ind[i]]);
    }
    else if (ie > ib+1) {
	res |= *(bits[ib]);
	res |= *(bits[ib+1]);
    }
    else {
	res |= *(bits[ib]);
    }

    if (ibis::gVerbose > 4) {
	timer.stop();
	ibis::util::logMessage("ibis::egale", "addBins(%lu, %lu) took %g "
			       "sec(CPU) and %g sec(elapsed).",
			       static_cast<long unsigned>(ib),
			       static_cast<long unsigned>(ie),
			       timer.CPUTime(), timer.realTime());
    }
} // ibis::egale::addBins_

// compute the bitvector that is the answer for the query x = b
void ibis::egale::evalEQ(ibis::bitvector& res, uint32_t b) const {
#ifdef DEBUG
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG -- ibis::egale::evalEQ(" << b << ")...";
#endif
    if (b >= nobs) {
	res.set(0, nrows);
    }
    else {
	uint32_t offset = 0;
	res.set(1, nrows);
	for (uint32_t i=0; i < bases.size(); ++i) {
	    uint32_t k = b % bases[i];
	    const uint32_t j = offset + k;
#ifdef DEBUG
	    LOGGER(ibis::gVerbose >= 0)
		<< "DEBUG -- ibis::egale::evalEQ(" << b << ")... component "
		<< i << " = " << k << ", bits[" << j << "]";
#endif
	    if (bits[j] == 0)
		activate(j);
	    if (bits[j])
		res &= *(bits[j]);
	    offset += bases[i];
	    b /= bases[i];
	}
    }
} // ibis::egale::evalEQ

// compute the bitvector that is the answer for the query x <= b
void ibis::egale::evalLE(ibis::bitvector& res, uint32_t b) const {
#ifdef DEBUG
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG -- ibis::egale::evalLE(" << b << ")...";
#endif
    if (b+1 >= nobs) {
	res.set(1, nrows);
    }
    else {
	uint32_t i = 0; // index into components
	uint32_t offset = 0;
	// skip till the first component that isn't the maximum value
	while (i < bases.size() && b % bases[i] == bases[i]-1) {
	    offset += bases[i];
	    b /= bases[i];
	    ++ i;
	}
	// the first non-maximum component
	if (i < bases.size()) {
	    const uint32_t k = b % bases[i];
	    res.set(0, nrows);
	    if (k+k <= bases[i]) {
		addBits(offset, offset+k+1, res);
	    }
	    else {
		addBits(offset+k+1, offset+bases[i], res);
		res.flip();
	    }
	    offset += bases[i];
	    b /= bases[i];
	}
	else {
	    res.set(1, nrows);
	}
	++ i;
	// deal with the remaining components
	while (i < bases.size()) {
	    uint32_t k = b % bases[i];
	    const uint32_t j = offset + k;
	    if (bits[j] == 0)
		activate(j);
	    if (bits[j])
		res &= *(bits[j]);
	    else if (res.cnt())
		res.set(0, res.size());

	    if (k > 0) {
		if (k+k <= bases[i]) {
		    addBits(offset, j, res);
		}
		else {
		    ibis::bitvector tmp;
		    addBits(j, offset+bases[i], tmp);
		    tmp.flip();
		    res |= tmp;
		}
	    }
	    offset += bases[i];
	    b /= bases[i];
	    ++ i;
	}
    }
} // evalLE

// compute the bitvector that answers the query b0 < x <= b1
void ibis::egale::evalLL(ibis::bitvector& res,
			 uint32_t b0, uint32_t b1) const {
#ifdef DEBUG
    LOGGER(ibis::gVerbose >= 0)
	<< "DEBUG -- ibis::egale::evalLL(" << b0 << ", " << b1 << ")...";
#endif
    if (b0 >= b1) { // no hit
	res.set(0, nrows);
    }
    else if (b1 >= nobs-1) { // x > b0
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
		offset += bases[i];
		b0 /= bases[i];
		b1 /= bases[i];
		++ i;
	    }
	    else {
		break;
	    }
	}
	res.clear();
	// the first non-maximum component
	if (i < bases.size()) {
	    k0 = b0 % bases[i];
	    k1 = b1 % bases[i];
	    if (k0 <= k1) {
		if (k0+k0 <= bases[i]) {
		    addBits(offset, offset+k0+1, low);
		}
		else if (k0 < bases[i]-1) {
		    addBits(offset+k0+1, offset+bases[i], low);
		    low.flip();
		}
		else {
		    low.set(1, nrows);
		}
		if (k1 >= bases[i]-1) {
		    res.set(1, nrows);
		}
		else if (k0 < k1) {
		    if (k1+k1 <= k0+bases[i]) {
			res = low;
			addBits(offset+k0+1, offset+k1+1, res);
		    }
		    else {
			addBits(offset+k1+1, offset+bases[i], res);
			res.flip();
		    }
		}
		else {
		    res = low;
		}
	    }
	    else {
		if (k1+k1 <= bases[i]) {
		    addBits(offset, offset+k1+1, res);
		}
		else if (k1 < bases[i]-1) {
		    addBits(offset+k1+1, offset+bases[i], res);
		    res.flip();
		}
		else {
		    res.set(1, nrows);
		}
		if (k0 >= bases[i]-1) {
		    low.set(1, nrows);
		}
		else if (k0+k0 <= k1+bases[i]) {
		    low = res;
		    addBits(offset+k1+1, offset+k0+1, low);
		}
		else {
		    addBits(offset+k0+1, offset+bases[i], low);
		    low.flip();
		}
	    }
	    offset += bases[i];
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
		const uint32_t j0 = offset + k0;
		if (bits[j0] == 0)
		    activate(j0);
		if (bits[j0])
		    low &= *(bits[j0]);
		else if (low.cnt())
		    low.set(0, low.size());

		const uint32_t j1 = offset + k1;
		if (bits[j1] == 0)
		    activate(j1);
		if (bits[j1])
		    res &= *(bits[j1]);
		else if (res.cnt())
		    res.set(0, res.size());

		ibis::bitvector tmp;
		if (k0 <= k1) {
		    if (k0 > 0) {
			if (k0+k0 <= bases[i]) {
			    addBits(offset, j0, tmp);
			}
			else {
			    addBits(j0, offset+bases[i], tmp);
			    tmp.flip();
			}
			if (tmp.size() == low.size())
			    low |= tmp;
		    }
		    if (k0 < k1) {
			if (k1+k1 <= k0+bases[i]) {
			    if (k0 > 0)
				res |= tmp;
			    addBits(j0, j1, res);
			}
			else {
			    tmp.set(0, nrows);
			    addBits(j1, offset+bases[i], tmp);
			    tmp.flip();
			    res |= tmp;
			}
		    }
		    else if (tmp.size() == res.size()) {
			res |= tmp;
		    }
		}
		else {
		    if (k1 > 0) {
			if (k1+k1 <= bases[i]) {
			    addBits(offset, j1, tmp);
			}
			else {
			    tmp.set(0, nrows);
			    addBits(j1, offset+bases[i], tmp);
			    tmp.flip();
			}
			if (tmp.size() == res.size())
			    res |= tmp;
		    }
		    if (k0+k0 <= k1+bases[i]) {
			if (k1 > 0)
			    low |= tmp;
			addBits(j1, j0, low);
		    }
		    else {
			tmp.set(0, nrows);
			addBits(j0, offset+bases[i], tmp);
			tmp.flip();
			low |= tmp;
		    }
		}
		offset += bases[i];
	    }
	    else { // the more significant components are the same
		res -= low;
		low.clear(); // no longer need low
		while (i < bases.size()) {
		    k1 = b1 % bases[i];
		    const uint32_t j = offset + k1;
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j]) {
			res &= *(bits[j]);
		    }
		    else if (res.cnt()) {
			res.set(0, res.size());
			i = bases.size();
		    }
		    offset += bases[i];
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
} // evalLL

long ibis::egale::evaluate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower) const {
    if (col == 0 || col->partition() == 0) return -1;
    ibis::bitvector tmp;
    estimate(expr, lower, tmp);
    if (tmp.size() == lower.size() && tmp.cnt() > lower.cnt()) {
	tmp -= lower;
	ibis::bitvector delta;
	col->partition()->doScan(expr, tmp, delta);
	if (delta.size() == lower.size() && delta.cnt() > 0)
	    lower |= delta;
    }
    return lower.cnt();
} // ibis::egale::evaluate

// provide an estimation based on the current index
// set bits in lower are hits for certain, set bits in upper are candidates
// set bits in (upper - lower) should be checked to verifies which are
// actually hits
// if the bitvector upper contain less bits than bitvector lower
// (upper.size() < lower.size()), the content of upper is assumed to be the
// same as lower.
void ibis::egale::estimate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower,
			   ibis::bitvector& upper) const {
    // values in the range [hit0, hit1) satisfy the query expression
    uint32_t hit0, hit1, cand0, cand1;
    if (bits.empty()) {
	lower.set(0, nrows);
	upper.clear();
	return;
    }

    locate(expr, cand0, cand1, hit0, hit1);
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
    else if (hit1 == nobs) { // >= hit0 (translates to NOT (<= hit0-1))
	evalLE(lower, hit0-1);
	lower.flip();
    }
    else { // (hit0-1, hit1-1]
	evalLL(lower, hit0-1, hit1-1);
    }
    // accumulate the bits in range [cand0, cand1)
    if (cand0 == hit0 && cand1 == hit1) {
	upper.clear(); // to indicate an exact answer
    }
    else {
	if (cand0 < hit0) {
	    evalEQ(upper, cand0);
	    upper |= lower;
	}
	else {
	    upper.copy(lower);
	}
	if (cand1 > hit1) {
	    ibis::bitvector tmp;
	    evalEQ(tmp, hit1);
	    upper |= tmp;
	}
    }
} // ibis::egale::estimate()

// compute an upper bound on the number of hits
uint32_t ibis::egale::estimate(const ibis::qContinuousRange& expr) const {
    uint32_t cand0, cand1;
    ibis::bitvector upper;
    if (bits.empty()) return 0;

    locate(expr, cand0, cand1);
    // accumulate the bits in range [cand0, cand1)
    if (cand1 <= cand0) {
	upper.set(0, nrows);
    }
    else if (cand0+1 == cand1) { // equal to one single value
	evalEQ(upper, cand0);
    }
    else if (cand0 == 0) { // < cand1
	evalLE(upper, cand1-1);
    }
    else if (cand1 == nobs) {
	// >= cand0 (translates to NOT (<= cand0-1))
	evalLE(upper, cand0-1);
	upper.flip();
    }
    else { // (cand0-1, cand1-1]
	evalLL(upper, cand0-1, cand1-1);
    }
    return upper.cnt();
} // ibis::egale::estimate

// ***should implement a more efficient version***
float ibis::egale::undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const {
    float ret = 0;
    ibis::bitvector tmp;
    estimate(expr, tmp, iffy);
    if (iffy.size() == tmp.size())
	iffy -= tmp;
    else
	iffy.set(0, tmp.size());

    if (iffy.cnt() > 0) {
	uint32_t cand0=0, hit0=0, hit1=0, cand1=0;
	locate(expr, cand0, cand1, hit0, hit1);
	if (cand0+1 == hit0 && maxval[cand0] > minval[cand0]) {
	    ret = (maxval[cand0] - expr.leftBound()) /
		(maxval[cand0] - minval[cand0]);
	    if (ret < FLT_EPSILON)
		ret = FLT_EPSILON;
	}
	if (hit1+1 == cand1 && maxval[hit1] > minval[hit1]) {
	    if (ret > 0)
		ret = 0.5 * (ret + (expr.rightBound() - minval[hit1]) /
			     (maxval[hit1] - minval[hit1]));
	    else
		ret = (expr.rightBound() - minval[hit1]) /
		    (maxval[hit1] - minval[hit1]);
	    if (ret < FLT_EPSILON)
		ret = FLT_EPSILON;
	}
    }
    return ret;
} // ibis::egale::undecidable

void ibis::egale::binBoundaries(std::vector<double>& bds) const {
    bds.resize(bounds.size());
    for (uint32_t i = 0; i < bounds.size(); ++ i)
	bds[i] = bounds[i];
} // ibis::egale::binBoundaries

void ibis::egale::binWeights(std::vector<uint32_t>& wts) const {
    wts.resize(cnts.size());
    for (uint32_t i = 0; i < cnts.size(); ++ i)
	wts[i] = cnts[i];
} // ibis::egale::binWeights

double ibis::egale::getSum() const {
    double ret;
    bool here = true;
    { // a small test block to evaluate variable here
	const uint32_t nbv = col->elementSize()*col->partition()->nRows();
	if (str != 0)
	    here = (str->bytes() * (nbases+1) < nbv);
	else if (offsets.size() > nbits)
	    here = (static_cast<uint32_t>(offsets[nbits] * (nbases+1)) < nbv);
    }
    if (here) {
	ret = computeSum();
    }
    else { // indicate sum is not computed
	ibis::util::setNaN(ret);
    }
    return ret;
} // ibis::egale::getSum

double ibis::egale::computeSum() const {
    double sum = 0;
    for (uint32_t i = 0; i < nobs; ++ i) {
	ibis::bitvector tmp;
	evalEQ(tmp, i);
	uint32_t cnt = tmp.cnt();
	if (cnt > 0)
	    sum += 0.5 * (minval[i] + maxval[i]) * cnt;
    }
    return sum;
} // ibis::egale::computeSum

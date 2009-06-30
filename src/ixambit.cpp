// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
//
// This file contains the implementation of the classes defined in index.h
// The primary function from the database point of view is a functioin
// called estimate().  It evaluates a given range condition and produces
// two bit vectors representing the range where the actual solution lies.
// The bulk of the code is devoted to maintain and update the indices.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "ibin.h"
#include "part.h"
#include "column.h"
#include "resource.h"

#include <string>

// generate a new index from attribute values (stored in a file)
ibis::ambit::ambit(const ibis::column* c, const char* f) : ibis::bin(c, f) {
    if (c == 0) return;
    if (nobs <= 2) {
	clear();
	ibis::bin::clear();
	throw "ibis::ambix -- binning produced two or less bins, need more";
    }

    try {
	// decide how many corase and fine bins to use
	uint32_t nbins = nobs - 2, i, j, k;
	// the default number of coarse bins is determined based on a set
	// of simplified assumptions about expected sizes of range encoded
	// bitmaps and word size being 32 bits.
	const uint32_t defaultJ = static_cast<uint32_t>
	    (nbins < 100 ? sqrt((double)nbins) :
	     0.5*(31.0 + sqrt(31.0*(31 + 4.0*nbins))));
	const char* spec = col->indexSpec();
	if (strstr(spec, "nrefine=") != 0) {
	    // number of fine bins per coarse bin
	    const char* tmp = 8+strstr(spec, "nrefine=");
	    i = atoi(tmp);
	    if (i > 1)
		j = (nbins > i ? (nbins+i-1)/i : nbins);
	    else
		j = (nbins >= 100 ? defaultJ :
		     (nbins >= 10 ? static_cast<uint32_t>(sqrt((double)nbins))
		      : nbins));
	}
	else if (strstr(spec, "ncoarse=") != 0) { // number of coarse bins
	    const char* tmp = 8+strstr(spec, "ncoarse=");
	    j = atoi(tmp);
	    if (j <= 2)
		j = (nbins >= 100 ? defaultJ :
		     (nbins >= 10 ? static_cast<uint32_t>(sqrt((double)nbins))
		      : nbins));
	}
	else { // default
	    j = (nbins >= 100 ? defaultJ :
		 (nbins >= 10 ? static_cast<uint32_t>(sqrt((double)nbins))
		  : nbins));
	}

	bool needDecompress = false;
	if (! ibis::gParameters().isTrue("uncompressedIndex")) {
	    if (strstr(spec, "uncompressed")) {
		needDecompress = true;
	    }
	}

	std::vector<unsigned> parts(j+1);
	divideBitmaps(bits, parts);

	// swap the current content to a different name, rhs
	ibis::bin rhs;
	swap(rhs);

	// prepare the arrays
	nobs = j + 1;
	nrows = rhs.nrows;
	sub.resize(nobs);
	bits.resize(nobs);
	bounds.resize(nobs);
	maxval.resize(nobs);
	minval.resize(nobs);
	max1 = rhs.maxval.back();
	min1 = rhs.minval.back();
	if (nobs+1 < rhs.nobs) {
	    sub.resize(nobs);
	    for (i=0; i<nobs; ++i) sub[i] = 0;
	}
	else {
	    sub.clear();
	}

	// copy the first bin, it never has subranges.
	bounds[0] = rhs.bounds[0];
	maxval[0] = rhs.maxval[0];
	minval[0] = rhs.minval[0];
	bits[0] = new ibis::bitvector(*(rhs.bits[0]));
	if (needDecompress)
	    bits[0]->decompress();

	// copy the majority of the bins
	if (nobs+1 < rhs.nobs) { // two levels
	    k = 1;
	    for (i = 1; i < nobs; ++i) {
		uint32_t nbi = parts[i] - parts[i-1];
		maxval[i] = rhs.maxval[k];
		minval[i] = rhs.minval[k];
		if (nbi > 1) {
		    sub[i] = new ibis::ambit;
		    sub[i]->col = col;
		    sub[i]->nobs = nbi - 1;
		    sub[i]->nrows = nrows;
		    sub[i]->bits.resize(nbi - 1);
		    for (unsigned ii = 0; ii < nbi-1; ++ ii)
			sub[i]->bits[ii] = 0;
		    sub[i]->bounds.resize(nbi - 1);
		    sub[i]->maxval.resize(nbi - 1);
		    sub[i]->minval.resize(nbi - 1);

		    // copy the first bin
		    sub[i]->bounds[0] = rhs.bounds[k];
		    sub[i]->maxval[0] = rhs.maxval[k];
		    sub[i]->minval[0] = rhs.minval[k];
		    sub[i]->bits[0] = new ibis::bitvector(*(rhs.bits[k]));
		    if (needDecompress)
			sub[i]->bits[0]->decompress();
		    ++k;

		    // copy nbi-2 bins to the subrange
		    for (j = 1; j < nbi - 1; ++j, ++k) {
			sub[i]->bounds[j] = rhs.bounds[k];
			sub[i]->maxval[j] = rhs.maxval[k];
			sub[i]->minval[j] = rhs.minval[k];
			sub[i]->bits[j] = *(sub[i]->bits[j-1]) |
			    *(rhs.bits[k]);
			if (needDecompress)
			    sub[i]->bits[j]->decompress();
			else
			    sub[i]->bits[j]->decompress();
			if (minval[i] > rhs.minval[k])
			    minval[i] = rhs.minval[k];
			if (maxval[i] < rhs.maxval[k])
			    maxval[i] = rhs.maxval[k];
		    }
		    sub[i]->max1 = rhs.maxval[k];
		    sub[i]->min1 = rhs.minval[k];
		    if (minval[i] > rhs.minval[k])
			minval[i] = rhs.minval[k];
		    if (maxval[i] < rhs.maxval[k])
			maxval[i] = rhs.maxval[k];

		    ibis::bitvector* tmp = *(bits[i-1]) |
			*(sub[i]->bits.back());
		    bits[i] = *tmp | *(rhs.bits[k]);
		    delete tmp;
		    if (needDecompress)
			bits[i]->decompress();
		    else
			bits[i]->compress();
		}
		else {
		    sub[i] = 0;
		    bits[i] = *(bits[i-1]) | *(rhs.bits[k]);
		    if (needDecompress)
			bits[i]->decompress();
		    else
			bits[i]->compress();
		}

		bounds[i] = rhs.bounds[k];
		++ k;
	    }
	}
	else { // one level repeat the code used for ibis::range
	    for (i = 1; i < nobs; ++i) {
		bounds[i] = rhs.bounds[i];
		maxval[i] = rhs.maxval[i];
		minval[i] = rhs.minval[i];
		bits[i] = *(bits[i-1]) | *(rhs.bits[i]);
		if (needDecompress)
		    bits[i]->decompress();
		else
		    bits[i]->compress();
	    }
	}

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg;
	    print(lg.buffer());
	}
    }
    catch (...) {
	clear();
	throw;
    }
} // constructor

// generate a ibis::ambit from ibis::bin
ibis::ambit::ambit(const ibis::bin& rhs) : max1(-DBL_MAX), min1(DBL_MAX) {
    if (rhs.col == 0) return;
    if (rhs.nobs <= 1) return; // rhs does not contain an valid index
    col = rhs.col;

    try {
	// decide how many corase and fine bins to use
	uint32_t nbins = rhs.nobs - 2, i, j, k;
	const char* spec = col->indexSpec();
	const uint32_t defaultJ = static_cast<uint32_t>
	    (nbins < 100 ? sqrt((double)nbins) :
	     0.5*(31.0 + sqrt(31.0*(31 + 4.0*nbins))));
	if (strstr(spec, "nrefine=") != 0) {
	    // number of fine bins per coarse bin
	    const char* tmp = 8+strstr(spec, "nrefine=");
	    i = atoi(tmp);
	    if (i > 1)
		j = (nbins > i ? (nbins+i-1)/i : nbins);
	    else
		j = (nbins >= 100 ? defaultJ :
		     (nbins >= 10 ? static_cast<uint32_t>(sqrt((double)nbins))
		      : nbins));
	}
	else if (strstr(spec, "ncoarse=") != 0) { // number of coarse bins
	    const char* tmp = 8+strstr(spec, "ncoarse=");
	    j = atoi(tmp);
	    if (j <= 1)
		j = (nbins >= 100 ? defaultJ :
		     (nbins >= 10 ? static_cast<uint32_t>(sqrt((double)nbins))
		      : nbins));
	}
	else { // default -- sqrt(nbins) on the coarse level
	    j = (nbins >= 100 ? defaultJ :
		 (nbins >= 10 ? static_cast<uint32_t>(sqrt((double)nbins))
		  : nbins));
	}

	bool needDecompress = false;
	if (! ibis::gParameters().isTrue("uncompressedIndex")) {
	    if (strstr(spec, "uncompressed")) {
		needDecompress = true;
	    }
	}

	std::vector<unsigned> parts(j+1);
	divideBitmaps(rhs.bits, parts);

	// prepare the arrays
	nobs = j + 1;
	nrows = rhs.nrows;
	sub.resize(nobs);
	bits.resize(nobs);
	bounds.resize(nobs);
	maxval.resize(nobs);
	minval.resize(nobs);
	max1 = rhs.maxval.back();
	min1 = rhs.minval.back();
	if (nobs < rhs.nobs) {
	    sub.resize(nobs);
	    for (i=0; i<nobs; ++i)
		sub[i] = 0;
	}
	else {
	    sub.clear();
	}
	LOGGER(ibis::gVerbose > 2)
	    << "ibis::ambit::ctor starting to convert " << rhs.nobs
	    << " bitvectors into " << nobs << " coarse bins";

	// copy the first bin, it never has subranges.
	bounds[0] = rhs.bounds[0];
	maxval[0] = rhs.maxval[0];
	minval[0] = rhs.minval[0];
	bits[0] = new ibis::bitvector(*(rhs.bits[0]));
	if (needDecompress)
	    bits[0]->decompress();

	// copy the majority of the bins
	if (nobs+1 < rhs.nobs) { // two levels
	    k = 1;
	    for (i = 1; i < nobs; ++i) {
		uint32_t nbi = parts[i] - parts[i-1];
		maxval[i] = rhs.maxval[k];
		minval[i] = rhs.minval[k];
		if (nbi > 1) {
		    sub[i] = new ibis::ambit;
		    sub[i]->col = col;
		    sub[i]->nrows = nrows;
		    sub[i]->nobs = nbi - 1;
		    sub[i]->bits.resize(nbi - 1);
		    for (unsigned ii = 0; ii < nbi-1; ++ ii)
			sub[i]->bits[ii] = 0;
		    sub[i]->bounds.resize(nbi - 1);
		    sub[i]->maxval.resize(nbi - 1);
		    sub[i]->minval.resize(nbi - 1);
		    // copy the first bin
		    sub[i]->bounds[0] = rhs.bounds[k];
		    sub[i]->maxval[0] = rhs.maxval[k];
		    sub[i]->minval[0] = rhs.minval[k];
		    sub[i]->bits[0] = new ibis::bitvector(*(rhs.bits[k]));
		    if (needDecompress)
			sub[i]->bits[0]->decompress();
		    ++k;

		    // copy nbi-2 bins to the subrange
		    for (j = 1; j < nbi - 1; ++j, ++k) {
			sub[i]->bounds[j] = rhs.bounds[k];
			sub[i]->maxval[j] = rhs.maxval[k];
			sub[i]->minval[j] = rhs.minval[k];
			sub[i]->bits[j] = *(sub[i]->bits[j-1]) |
			    *(rhs.bits[k]);
			if (needDecompress)
			    sub[i]->bits[j]->decompress();
			else
			    sub[i]->bits[j]->compress();
			if (minval[i] > rhs.minval[k])
			    minval[i] = rhs.minval[k];
			if (maxval[i] < rhs.maxval[k])
			    maxval[i] = rhs.maxval[k];
		    }
		    sub[i]->max1 = rhs.maxval[k];
		    sub[i]->min1 = rhs.minval[k];
		    if (minval[i] > rhs.minval[k])
			minval[i] = rhs.minval[k];
		    if (maxval[i] < rhs.maxval[k])
			maxval[i] = rhs.maxval[k];

		    ibis::bitvector* tmp = *(bits[i-1]) |
			*(sub[i]->bits.back());
		    bits[i] = *tmp | *(rhs.bits[k]);
		    delete tmp;
		    if (needDecompress)
			bits[i]->decompress();
		    else
			bits[i]->compress();
		}
		else {
		    sub[i] = 0;
		    bits[i] = *(bits[i-1]) | *(rhs.bits[k]);
		    if (needDecompress)
			bits[i]->decompress();
		    else
			bits[i]->compress();
		}

		bounds[i] = rhs.bounds[k];
		++ k;
	    }
	}
	else { // one level repeat the code used for ibis::range
	    for (i = 1; i < nobs; ++i) {
		bounds[i] = rhs.bounds[i];
		maxval[i] = rhs.maxval[i];
		minval[i] = rhs.minval[i];
		bits[i] = *(bits[i-1]) | *(rhs.bits[i]);
		if (needDecompress)
		    bits[i]->decompress();
		else
		    bits[i]->compress();
	    }
	}

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg;
	    print(lg.buffer());
	}
    }
    catch (...) {
	clear();
	throw;
    }
} // copy from ibis::bin

// (de-serialization) reconstruct ambit from content of a file
// In addition to the common content for index::bin, the following are
// inserted after minval array: (this constructor relies the fact that max1
// and min1 follow minval immediately without any separation or padding)
// max1 (double) -- the maximum value of all data entry
// min1 (double) -- the minimum value of those larger than or equal to the
// largest bounds value (bounds[nobs-1])
// offsets_for_next_level (uint32_t[nobs]) -- as the name suggests, these are
// the offsets (in this file) for the next level ibis::ambit.
// after the bit vectors of this level are written, the next level ibis::ambit
// are written without header.
ibis::ambit::ambit(const ibis::column* c, ibis::fileManager::storage* st,
		   uint32_t offset)
    : ibis::bin(c, st, offset), max1(*(minval.end())),
      min1(*(1+minval.end())) {
    try {
	array_t<int32_t>
	    offs(st,
		 8*((offset+sizeof(int32_t)*(nobs+1)+sizeof(uint32_t)*2+7)/8)+
		 sizeof(double)*(nobs*3+2), nobs+1);
#ifdef DEBUG
	if (ibis::gVerbose > 5) {
	    ibis::util::logger lg(4);
	    lg.buffer() << "DEBUG: from ibis::ambit::ambit("
			<< col->partition()->name() << '.' << col->name()
			<< ", " << offset << ")" << "\n";
	    for (uint32_t i=0; i<=nobs; ++i)
		lg.buffer() << "offset[" << i << "] = " << offs[i] << "\n";
	}
#endif
	if (offs[nobs] > offs[0]) {
	    sub.resize(nobs);
	    for (uint32_t i=0; i<nobs; ++i) {
		if (offs[i+1] > offs[i])
		    sub[i] = new ambit(c, st, offs[i]);
		else
		    sub[i] = 0;
	    }
	}
	if (ibis::gVerbose > 6) {
	    ibis::util::logger lg;
	    print(lg.buffer());
	}
    }
    catch (...) {
	clear();
	throw;
    }
}

// read the content of a file, only read the first bitvector, rely on the
// function activate to read the rest when needed.
int ibis::ambit::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);

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
	  header[5] == static_cast<char>(ibis::index::AMBIT) &&
	  header[6] == static_cast<char>(sizeof(int32_t)) &&
	  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return -3;
    }

    uint32_t begin, end;
    clear(); // clear the existing content
    fname = ibis::util::strnewdup(fnm.c_str());
    str = 0;

    // read nrows and nobs
    int ierr = UnixRead(fdes, static_cast<void*>(&nrows), sizeof(uint32_t));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	nrows = 0;
	return -4;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&nobs), sizeof(uint32_t));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	nrows = 0;
	nobs = 0;
	return -5;
    }
    bool trymmap = false;
#if defined(HAVE_FILE_MAP)
    trymmap = (nobs > ibis::fileManager::pageSize());
#endif
    begin = 8 + 2 * sizeof(uint32_t);
    end = 8 + 2 * sizeof(uint32_t) + (nobs+1)*sizeof(int32_t);
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }
    // read bounds
    begin = 8 * ((8 + sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t)+7)/8);
    end = begin + sizeof(double)*nobs;
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
    ierr = UnixSeek(fdes, end, SEEK_SET);
    if (ierr != static_cast<int>(end)) {
	UnixClose(fdes);
	clear();
	return -5;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&max1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -6;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&min1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -7;
    }

    begin = end + 2*sizeof(double);
    end += 2*sizeof(double) + (nobs+1)*sizeof(int32_t);
    array_t<int32_t> nextlevel;
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	nextlevel.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	nextlevel.swap(tmp);
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- ibis::ambit::read(";
	if (fname)
	    lg.buffer() << fname;
	else
	    lg.buffer() << fdes;
	lg.buffer() << ") got the starting positions of the fine levels\n";
	for (uint32_t i = 0; i <= nobs; ++ i)
	    lg.buffer() << "offset[" << i << "] = " << nextlevel[i] << "\n";
    }
#endif
    ibis::fileManager::instance().recordPages(0, end);

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nobs);
    for (uint32_t i = 1; i < nobs; ++i)
	bits[i] = 0;
#if defined(FASTBIT_READ_BITVECTOR0)
    if (offsets[1] > offsets[0]) {
	array_t<ibis::bitvector::word_t>
	    a0(fdes, offsets[0], offsets[1]);
	ibis::bitvector* tmp = new ibis::bitvector(a0);
	bits[0] = tmp;
#if defined(WAH_CHECK_SIZE)
	if (tmp->size() != nrows)
	    col->logWarning("readIndex", "the length (%lu) of "
			    "bitvector 0 (from \"%s\") "
			    "differs from nRows (%lu)",
			    static_cast<long unsigned>(tmp->size()),
			    fnm.c_str(),
			    static_cast<long unsigned>(nrows));
#else
	tmp->setSize(nrows);
#endif
    }
    else {
	bits[0] = new ibis::bitvector;
	bits[0]->set(0, nrows);
    }
#endif
    // dealing with next levels
    for (uint32_t i = 0; i < sub.size(); ++i)
	delete sub[i];
    if (nextlevel[0] > nextlevel[nobs]) {
	clear();
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg.buffer() << " Error *** ibis::ambit::read(";
	    if (fname)
		lg.buffer() << fname;
	    else
		lg.buffer() << fdes;
	    lg.buffer() << ") internal error, offset[0](" << nextlevel[0]
		      << ") is expected to be no less than offset["
		      << nobs << "](" << nextlevel[nobs]
		      << "), but it is not";
	}
    }
    else if (nextlevel[0] == nextlevel[nobs]) {
	sub.clear();
    }
    else {
	sub.resize(nobs);
    }
    for (uint32_t i = 0; i < sub.size(); ++i) {
	if (nextlevel[i] < nextlevel[i+1]) {
	    sub[i] = new ibis::ambit(0);
	    sub[i]->col = col;
	    sub[i]->read(fdes, nextlevel[i], fname);
	}
	else if (nextlevel[i] == nextlevel[i+1]) {
	    sub[i] = 0;
	}
	else {
	    if (ibis::gVerbose > -1) {
		ibis::util::logger lg;
		lg.buffer() << " Error *** ibis::ambit::read(";
		if (fname)
		    lg.buffer() << fname;
		else
		    lg.buffer() << fdes;
		lg.buffer() << ") offset[" << i << "] (" << nextlevel[i]
			    << ") is expected to less or equal to offset["
			    << i+1 << "] (" << nextlevel[i+1]
			    << "), but it is not! Can not use the index file.";
	    }
	    UnixClose(fdes);
	    throw "ibis::zone::read bad offsets(nextlevel)";
	}
    }
    (void) UnixClose(fdes);
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fname);
    return 0;
} // ibis::ambit::read

// read the content of a file starting from an arbitrary position.  read
// only the first bitvector.  rely on the function activate to read the
// rest as needed.
int ibis::ambit::read(int fdes, uint32_t start, const char *fn) {
    if (fdes < 0) return -1;
    if (start != static_cast<uint32_t>(UnixSeek(fdes, start, SEEK_SET)))
	return -2;

    uint32_t begin, end;
    clear(); // clear the existing content
    if (fn)
	fname = ibis::util::strnewdup(fn);
    else
	fname = 0;

    // read nrows and nobs
    int ierr = UnixRead(fdes, static_cast<void*>(&nrows), sizeof(uint32_t));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	nrows = 0;
	return -3;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&nobs), sizeof(uint32_t));
    if (ierr < static_cast<int>(sizeof(uint32_t))) {
	UnixClose(fdes);
	nrows = 0;
	nobs = 0;
	return -4;
    }
    bool trymmap = false;
#if defined(HAVE_FILE_MAP)
    trymmap = (nobs > ibis::fileManager::pageSize()) && (fname != 0);
#endif
    begin = start + 2 * sizeof(uint32_t);
    end = start + 2 * sizeof(uint32_t) + (nobs+1) * sizeof(int32_t);
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }
    // read bounds
    begin = 8 * ((start + sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t)+7)/8);
    end = begin + sizeof(double)*nobs;
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
    ierr = UnixSeek(fdes, end, SEEK_SET);
    if (ierr != static_cast<int>(end)) {
	UnixClose(fdes);
	clear();
	return -5;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&max1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -6;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&min1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -7;
    }

    begin = end + 2*sizeof(double);
    end += 2*sizeof(double) + (nobs+1)*sizeof(int32_t);
    array_t<int32_t> nextlevel;
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	nextlevel.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	nextlevel.swap(tmp);
    }
#if defined(DEBUG)
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- ibis::ambit::read(";
	if (fname)
	    lg.buffer() << fname;
	else
	    lg.buffer() << fdes;
	lg.buffer() << ", " << start
		  << ") got the starting positions of the fine levels\n";
	for (uint32_t i = 0; i <= nobs; ++ i)
	    lg.buffer() << "offset[" << i << "] = " << nextlevel[i] << "\n";
    }
#endif
    ibis::fileManager::instance().recordPages(start, end);

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nobs);
    if (fname == 0 || *fname == 0) { // read all bitvectors
	for (uint32_t i = 0; i < nobs; ++i) {
	    if (offsets[i+1] > offsets[i]) {
		array_t<ibis::bitvector::word_t>
		    a0(fdes, offsets[i], offsets[i+1]);
		ibis::bitvector* tmp = new ibis::bitvector(a0);
		bits[i] = tmp;
#if defined(WAH_CHECK_SIZE)
		if (tmp->size() != nrows)
		    col->logWarning
			("readIndex", "the length (%lu) of "
			 "bitvector %lu differs from nRows (%lu)",
			 static_cast<long unsigned>(tmp->size()),
			 static_cast<long unsigned>(i),
			 static_cast<long unsigned>(nrows));
#else
		tmp->setSize(nrows);
#endif
	    }
	    else if (i == 0) {
		bits[0] = new ibis::bitvector;
		bits[0]->set(0, nrows);
	    }
	    else {
		bits[i] = 0;
	    }
	}
    }
    else {
	for (uint32_t i = 1; i < nobs; ++ i)
	    bits[i] = 0;
#if defined(FASTBIT_READ_BITVECTOR0)
	// read only the first bitvector
	if (offsets[1] > offsets[0]) {
	    array_t<ibis::bitvector::word_t>
		a0(fdes, offsets[0], offsets[1]);
	    ibis::bitvector* tmp = new ibis::bitvector(a0);
	    bits[0] = tmp;
#if defined(WAH_CHECK_SIZE)
	    if (tmp->size() != nrows)
		col->logWarning("readIndex", "the length (%lu) of "
				"bitvector 0 differs from nRows (%lu)",
				static_cast<long unsigned>(tmp->size()),
				static_cast<long unsigned>(nrows));
#else
	    tmp->setSize(nrows);
#endif
	}
	else {
	    bits[0] = new ibis::bitvector;
	    bits[0]->set(0, nrows);
	}
#endif
    }

    // dealing with next levels
    for (uint32_t i = 0; i < sub.size(); ++i)
	delete sub[i];
    if (nextlevel[0] > nextlevel[nobs]) {
	clear();
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg.buffer() << " Error *** ibis::ambit::read(";
	    if (fname)
		lg.buffer() << fname;
	    else
		lg.buffer() << fdes;
	    lg.buffer() << ", " << start << ") internal error, offset[0]("
			<< nextlevel[0]
			<< ") is expected to be no less than offset["
			<< nobs << "](" << nextlevel[nobs]
			<< "), but it is not";
	}
    }
    else if (nextlevel[0] == nextlevel[nobs]) {
	sub.clear();
    }
    else {
	sub.resize(nobs);
    }
    for (uint32_t i = 0; i < sub.size(); ++i) {
	if (nextlevel[i] < nextlevel[i+1]) {
	    sub[i] = new ibis::ambit(0);
	    sub[i]->col = col;
	    sub[i]->read(fdes, nextlevel[i], fn);
	}
	else if (nextlevel[i] == nextlevel[i+1]) {
	    sub[i] = 0;
	}
	else {
	    if (ibis::gVerbose > -1) {
		ibis::util::logger lg;
		lg.buffer() << " Error *** ibis::ambit::read(";
		if (fname)
		    lg.buffer() << fname;
		else
		    lg.buffer() << fdes;
		lg.buffer() << ") offset[" << i << "] (" << nextlevel[i]
			    << ") is expected to less or equal to offset["
			    << i+1 << "] (" << nextlevel[i+1]
			    << "), but it is not! Can not use the index file.";
	    }
	    UnixClose(fdes);
	    return -9;
	}
    }
    return 0;
} // ibis::ambit::read

int ibis::ambit::read(ibis::fileManager::storage* st) {
    int ierr = ibis::bin::read(st);
    if (ierr < 0) return ierr;
    max1 = *(minval.end());
    min1 = *(1+minval.end());

    array_t<uint32_t> offs
	(st, 8*((sizeof(int32_t)*(nobs+1)+sizeof(uint32_t)*2+15)/8)+
	 sizeof(double)*(nobs*3+2), nobs+1);
    for (uint32_t i = 0; i < sub.size(); ++i)
	delete sub[i];
    sub.clear();
    if (offs[0] > offs[nobs]) {
	clear();
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg.buffer() << " Error *** ibis::ambit::read(";
	    if (st->unnamed())
		lg.buffer() << static_cast<const void*>(st->begin());
	    else
		lg.buffer() << st->filename();
	    lg.buffer() << ") internal error, offset[0](" << offs[0]
			<< ") is expected to be no less than offset["
			<< nobs << "](" << offs[nobs] << "), but it is not";
	}
    }
    else if (offs[0] == offs[nobs]) {
	sub.clear();
    }
    else {
	sub.resize(nobs);
    }
    for (uint32_t i=0; i < sub.size(); ++i) {
	if (offs[i+1] > offs[i]) {
	    sub[i] = new ambit(col, st, offs[i]);
	}
	else if (offs[i] == offs[i+1]) {
	    sub[i] = 0;
	}
	else {
	    if (ibis::gVerbose > -1) {
		ibis::util::logger lg;
		lg.buffer() << " Error *** ibis::ambit::read(";
		if (st->filename())
		    lg.buffer() << st->filename();
		else
		    lg.buffer() << static_cast<const void*>(st->begin());
		lg.buffer() << ") offset[" << i << "] (" << offs[i]
			    << ") is expected to less or equal to offset["
			    << i+1 << "] (" << offs[i+1]
			    << "), but it is not! Can not use the index file.";
	    }
	    return -9;
	}
    }
    return 0;
} // ibis::ambit::read

int ibis::ambit::write(const char* dt) const {
    if (nobs <= 0) return -1;

    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	ibis::fileManager::instance().flushFile(fnm.c_str());
	fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
	if (fdes < 0) {
	    col->logWarning("ambit::write", "unable to open \"%s\" for write",
			    fnm.c_str());
	    return -2;
	}
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    char header[] = "#IBIS\2\0\0";
    header[5] = (char)ibis::index::AMBIT;
    header[6] = (char)sizeof(int32_t);
    int32_t ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::column[" << col->partition()->name() << "."
	    << col->name() << "]::ambit::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr = write(fdes); // wrtie recursively
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    (void) UnixFlush(fdes); // write to disk
#endif
    (void) UnixClose(fdes);
    return ierr;
} // ibis::ambit::write

int ibis::ambit::write(int fdes) const {
    const int32_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	ibis::util::logMessage("Warning", "ibis::ambit::write call to UnixSeek"
			       "(%d, 0, SEEK_CUR) failed ... %s", fdes,
			       strerror(errno));
	return -4;
    }

    uint32_t i;
    array_t<int32_t> offs(nobs+1);
    // write out bit sequences of this level of the index
    int ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    offs[0] = ((start+sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t)+7)/8)*8;
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::ambit::write(" << fdes << ") failed to seek to "
	    << offs[0];
	UnixSeek(fdes, start, SEEK_SET);
	return -5;
    }

    ierr = UnixWrite(fdes, bounds.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, maxval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, minval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, &max1, sizeof(double));
    ierr = UnixWrite(fdes, &min1, sizeof(double));
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nobs+1), SEEK_CUR);
    for (i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes, start+sizeof(uint32_t)*2, SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
    ierr = UnixSeek(fdes, offs[nobs], SEEK_SET); // move to the end

    // write the sub-ranges
    if (sub.size() == nobs) { // subrange defined
	for (i = 0; i < nobs; ++i) {
	    offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	    if (sub[i])
		sub[i]->write(fdes);
	}
	offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    }
    else { // subrange not defined
	offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
	for (i = 0; i < nobs; ++i)
	    offs[i] = offs[nobs];
    }

    // write the offsets for the subranges
    ierr = UnixSeek(fdes,
		    8*((start+sizeof(int32_t)*(nobs+1)+sizeof(uint32_t)*2+7)/8)+
		    sizeof(double)*(nobs*3+2), SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
    ierr = UnixSeek(fdes, offs[nobs], SEEK_SET); // move to the end
#if defined(DEBUG)
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG  from ibis::ambit::write(" << col->name() << ", "
		    << start << ") -- offsets for subranges";
	for (i=0; i<=nobs; ++i)
	    lg.buffer() << "\noffset[" << i << "] = " << offs[i];
    }
#endif
    return 0;
} // ibis::ambit::write

void ibis::ambit::clear() {
    for (std::vector<ibis::ambit*>::iterator it1 = sub.begin();
	 it1 != sub.end(); ++it1) {
	delete *it1;
    }
    sub.clear();
    ibis::bin::clear();
} // ibis::ambit::clear

// fill with zero bits or truncate
void ibis::ambit::adjustLength(uint32_t nr) {
    bin::adjustLength(nr); // the top level
    if (sub.size() == nobs) {
	for (std::vector<ibis::ambit*>::iterator it = sub.begin();
	     it != sub.end(); ++it) {
	    if (*it)
		(*it)->adjustLength(nr);
	}
    }
    else {
	for (std::vector<ibis::ambit*>::iterator it = sub.begin();
	     it != sub.end(); ++it) {
	    delete *it;
	}
	sub.clear();
    }
} // ibis::ambit::adjustLength()

// construct the bitmap index of type ambit (2 level cummulative ranges)
// by default, it will use 100 bins
void ibis::ambit::construct(const char* f, const array_t<double>& bd) {
    uint32_t i, j, k;
    nrows = col->partition()->nRows();
    uint32_t nbins = 100;  // total number of bins in two levels
    const char* spec = col->indexSpec();
    if (bd.size() < 2) {
	// determine the number of bins to use based on col->indexSpec()
	const char* str = 0;
	if (spec != 0) {
	    str = strstr(spec, "no=");
	    if (str == 0) {
		str = strstr(spec, "NO=");
		if (str == 0)
		    str = strstr(spec, "No=");
	    }
	}
	if (str == 0 && col->partition()->indexSpec() != 0) {
	    spec = col->partition()->indexSpec();
	    str = strstr(spec, "no=");
	    if (str == 0) {
		str = strstr(spec, "NO=");
		if (str == 0)
		    str = strstr(spec, "No=");
	    }
	}
	if (str != 0) {
	    spec = 3+str;
	    nbins = atoi(spec);
	    if (nbins <= 0)
		nbins = 10;
	}
	if (col->type() == ibis::TEXT ||
	    col->type() == ibis::UINT ||
	    col->type() == ibis::INT) {
	    // for integral values, each bin contains at least one value
	    j = (uint32_t)(col->upperBound() - col->lowerBound()) + 1;
	    if (j < nbins)
		nbins = j;
	}
        if (nbins == 0) // no index
	    return;
    }
    else {
	nbins = bd.size() - 1;
    }

    // j   == number of bins on the first (coarse) level
    // nb2 == number of bins in each coarse bin
    // rem == number of coarse bins that needs to have nb2+1 fine bins
    uint32_t nb2, rem;
    if (strstr(spec, "nrefine=") != 0) { // number of fine bins per coarse bin
	const char* tmp = 8+strstr(spec, "nrefine=");
	nb2 = atoi(tmp);
	if (nb2 <= 1) nb2 = 2;
	j = nbins / nb2;
	if (j <= 1) {
	    if (nbins > 3) {
		j = nbins / 2;
		nb2 = 2;
	    }
	    else {
		j = nbins;
		nb2 = 1;
	    }
	}
    }
    else if (strstr(spec, "ncoarse=") != 0) { // number of coarse bins
	const char* tmp = 8+strstr(spec, "ncoarse=");
	j = atoi(tmp);
	if (j <= 1) j = nbins / 2;
	if (j > 1) {
	    nb2 = nbins / j;
	}
	else {
	    j = nbins;
	    nb2 = 1;
	}
    }
    else { // default -- sqrt(nbins) on the coarse level
	if (nbins < 10) {
	    j = nbins;
	}
	else {
	    j = static_cast<uint32_t>(sqrt(static_cast<double>(nbins)));
	}
	nb2 = nbins / j;
    }
    rem = nbins % j;
    if (nb2 <= 1 && rem > 0) { // some bins are not subdivided at all
	j = nbins;
	rem = 0;
    }

    // allocate space for the index at this level
    nobs = j + 1;
    sub.resize(nobs);
    bits.resize(nobs);
    bounds.resize(nobs);
    maxval.resize(nobs);
    minval.resize(nobs);
    double lbb = col->lowerBound();
    double diff = col->upperBound() - lbb;
    max1 = -DBL_MAX;
    min1 = DBL_MAX;
    for (i = 0; i < nobs; ++i) {
	k = i * nb2 + (i<rem?i:rem);
	if (bd.size() < 2) {
	    bounds[i] = lbb + diff * k / nbins;
	    if (col->type() == ibis::TEXT ||
		col->type() == ibis::UINT ||
		col->type() == ibis::INT) {
		// make sure bin boundaries are integers
		bounds[i] = 0.5*floor(2.0*bounds[i] + 0.5);
	    }
	}
	else {
	    bounds[i] = bd[k];
	}
	bits[i] = new ibis::bitvector;
	maxval[i] = -DBL_MAX;
	minval[i] = DBL_MAX;
	sub[i] = 0;
    }

    if (nbins > nobs) {
	// allocate space for index at the finer level
	for (i = 1; i < nobs; ++i) {
	    k = nb2 + (i <= rem);
	    if (k > 1) {
		sub[i] = new ambit;
		sub[i]->col = col;
		sub[i]->nobs = k - 1;
		sub[i]->bits.resize(k - 1);
		for (unsigned ii = 0; ii < k-1; ++ ii)
		    sub[i]->bits[ii] = 0;
		sub[i]->bounds.resize(k - 1);
		sub[i]->maxval.resize(k - 1);
		sub[i]->minval.resize(k - 1);
		sub[i]->max1 = -DBL_MAX;
		sub[i]->min1 = DBL_MAX;
		for (j = 0; j < k - 1; ++j) {
		    sub[i]->bits[j] = new ibis::bitvector;
		    sub[i]->maxval[j] = -DBL_MAX;
		    sub[i]->minval[j] = DBL_MAX;
		    if (bd.size() < 2) {
			sub[i]->bounds[j] = bounds[i-1] +
			    (bounds[i] - bounds[i-1]) * (j+1) / k;
			if (col->type() == ibis::TEXT ||
			    col->type() == ibis::UINT ||
			    col->type() == ibis::INT) {
			    // make sure bin boundaries are integers
			    sub[i]->bounds[j] =
				0.5*floor(2.0*sub[i]->bounds[j] + 0.5);
			}
		    }
		    else {
			sub[i]->bounds[j] = bd[(i-1)*nb2 + (i-1<rem?i-1:rem) +
					       j + 1];
		    }
		}
	    }
	    else {
		sub[i] = 0;
	    }
	}
    }

    std::string fnm; // name of the data file / index file
    if (f == 0) {
	fnm = col->partition()->currentDataDir();
	fnm += FASTBIT_DIRSEP;
	fnm += col->name();
    }
    else {
	j = strlen(f);
	if (j > 4 && f[j-1] == 'x' && f[j-2] == 'd' && f[j-3] == 'i' &&
	    f[j-4] == '.') { // index file name
	    fnm = f;
	    fnm.erase(j-4);
	}
	else {
	    bool isFile = false;
	    i = strlen(col->name());
	    if (j >= i) {
		const char* tail = f + (j - i);
		isFile = (strcmp(tail, col->name()) == 0);
	    }
	    if (isFile) {
		fnm = f;
	    }
	    else { // check the existence of the file or direcotry
		Stat_T st0;
		if (UnixStat(f, &st0)) { // assume to be a file
		    fnm = f;
		}
		else if ((st0.st_mode & S_IFDIR) == S_IFDIR) {
		    // named directory exist
		    fnm = f;
		    fnm += FASTBIT_DIRSEP;
		    fnm += col->name();
		}
		else { // given name is the data file name
		    fnm = f;
		}
	    }
	}
    }

    ibis::bitvector mask;
    {   // name of mask file associated with the data file
	array_t<ibis::bitvector::word_t> arr;
	std::string mname(fnm);
	mname += ".msk";
	i = ibis::fileManager::instance().getFile(mname.c_str(), arr);
	if (i == 0)
	    mask.copy(arr); // convert arr to a bitvector
    }

    // need to do different things for different columns
    switch (col->type()) {
    case ibis::TEXT:
    case ibis::UINT: {// unsigned int
	array_t<uint32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0)
	    col->logWarning("ambit::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
		if (sub[j]) {
		    k = sub[j]->locate(val[i]);
		    if (k < sub[j]->nobs) {
			if (sub[j]->maxval[k] < val[i])
			    sub[j]->maxval[k] = val[i];
			if (sub[j]->minval[k] > val[i])
			    sub[j]->minval[k] = val[i];
		    }
		    else {
			if (sub[j]->max1 < val[i])
			    sub[j]->max1 = val[i];
			if (sub[j]->min1 > val[i])
			    sub[j]->min1 = val[i];
		    }
		    while (k < sub[j]->nobs)
			sub[j]->bits[k++]->setBit(i, 1);
		}
		while (j < nobs)
		    bits[j++]->setBit(i, 1);
	    }
	    else { // bin # nobs
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	}
	break;}
    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0) 
	    col->logWarning("ambit::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
		if (sub[j]) {
		    k = sub[j]->locate(val[i]);
		    if (k < sub[j]->nobs) {
			if (sub[j]->maxval[k] < val[i])
			    sub[j]->maxval[k] = val[i];
			if (sub[j]->minval[k] > val[i])
			    sub[j]->minval[k] = val[i];
		    }
		    else {
			if (sub[j]->max1 < val[i])
			    sub[j]->max1 = val[i];
			if (sub[j]->min1 > val[i])
			    sub[j]->min1 = val[i];
		    }
		    while (k < sub[j]->nobs)
			sub[j]->bits[k++]->setBit(i, 1);
		}
		while (j < nobs)
		    bits[j++]->setBit(i, 1);
	    }
	    else { // bin # nobs
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	}
	break;}
    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0) 
	    col->logWarning("ambit::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
		if (sub[j]) {
		    k = sub[j]->locate(val[i]);
		    if (k < sub[j]->nobs) {
			if (sub[j]->maxval[k] < val[i])
			    sub[j]->maxval[k] = val[i];
			if (sub[j]->minval[k] > val[i])
			    sub[j]->minval[k] = val[i];
		    }
		    else {
			if (sub[j]->max1 < val[i])
			    sub[j]->max1 = val[i];
			if (sub[j]->min1 > val[i])
			    sub[j]->min1 = val[i];
		    }
		    while (k < sub[j]->nobs)
			sub[j]->bits[k++]->setBit(i, 1);
		}
		while (j < nobs)
		    bits[j++]->setBit(i, 1);
	    }
	    else { // bin # nobs
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	}
	break;}
    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0) 
	    col->logWarning("ambit::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
		if (sub[j]) {
		    k = sub[j]->locate(val[i]);
		    if (k < sub[j]->nobs) {
			if (sub[j]->maxval[k] < val[i])
			    sub[j]->maxval[k] = val[i];
			if (sub[j]->minval[k] > val[i])
			    sub[j]->minval[k] = val[i];
		    }
		    else {
			if (sub[j]->max1 < val[i])
			    sub[j]->max1 = val[i];
			if (sub[j]->min1 > val[i])
			    sub[j]->min1 = val[i];
		    }
		    while (k < sub[j]->nobs)
			sub[j]->bits[k++]->setBit(i, 1);
		}
		while (j < nobs)
		    bits[j++]->setBit(i, 1);
	    }
	    else { // bin # nobs
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	}
	break;}
    case ibis::CATEGORY: // no need for a separate index
	col->logWarning("ambit::construct", "no need for an index");
	return;
    default:
	col->logWarning("ambit::construct", "unable to create index for "
			"this type of column");
	return;
    }

    // make sure all bit vectors are the same size
    if (mask.size() > nrows)
	nrows = mask.size();
    for (i = 0; i < nobs; ++i) {
	if (bits[i]->size() < nrows)
	    bits[i]->setBit(nrows-1, 0);
	if (sub[i]) {
	    for (j = 0; j < sub[i]->nobs; ++j)
		if (sub[i]->bits[j]->size() < nrows)
		    sub[i]->bits[j]->setBit(nrows-1, 0);
	}
    }
} // ibis::ambit::construct

void ibis::ambit::binBoundaries(std::vector<double>& ret) const {
    ret.clear();
    if (sub.size() == nobs) {
	for (uint32_t i = 0; i < nobs; ++i) {
	    if (sub[i]) {
		for (uint32_t j = 0; j < sub[i]->nobs; ++j)
		    ret.push_back(sub[i]->bounds[j]);
	    }
	    ret.push_back(bounds[i]);
	}
    }
    else { // assume no sub intervals
	ret.resize(bounds.size());
	for (uint32_t i = 0; i < bounds.size(); ++ i)
	    ret[i] = bounds[i];
    }
} // ibis::ambit::binBoundaries()

void ibis::ambit::binWeights(std::vector<uint32_t>& ret) const {
    ret.clear();
    activate();
    ret.push_back(bits[0]->cnt());
    for (uint32_t i=1; i < nobs; ++i) {
	if (sub[i]) {
	    ret.push_back(sub[i]->bits[0]->cnt());
	    for (uint32_t j = 1; j < sub[i]->nobs; ++j)
		ret.push_back(sub[i]->bits[j]->cnt() -
			      sub[i]->bits[j-1]->cnt());
	    ret.push_back(bits[i]->cnt() - bits[i-1]->cnt() -
			  sub[i]->bits[sub[i]->nobs-1]->cnt());
	}
    }
    ret.push_back(bits[nobs-1]->size() - bits[nobs-1]->cnt());
} //ibis::ambit::binWeights()

// a simple function to test the speed of the bitvector operations
void ibis::ambit::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    activate();
    uint32_t i, nloops = 1000000000 / nrows;
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;
    col->logMessage("ambit::speedTest", "testing the speed of operator -");

    for (i = 0; i < nobs-1; ++i) {
	ibis::bitvector* tmp;
	tmp = *(bits[i+1]) - *(bits[i]);
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
		<< timer.CPUTime() / nloops << "\n";
	}
    }
} // ibis::ambit::speedTest()

// the printing function
void ibis::ambit::print(std::ostream& out) const {
    out << "index (binned range-range code) for "
	<< col->partition()->name() << '.'
	<< col->name() << " contains " << nobs+1 << " bins for "
	<< nrows << " objects \n";
    if (ibis::gVerbose > 4) { // the long format
	uint32_t i, cnt = nrows;
	if (bits[0])
	    out << "0: " << bits[0]->cnt() << "\t(..., " << bounds[0]
		<< ")\t\t\t[" << minval[0] << ", " << maxval[0] << "]\n";
	for (i = 1; i < nobs; ++i) {
	    if (bits[i] == 0) continue;
	    out << i << ": " << bits[i]->cnt() << "\t(..., " << bounds[i]
		<< ");\t" << bits[i]->cnt() - bits[i-1]->cnt() << "\t["
		<< bounds[i-1] << ", " << bounds[i] << ")\t[" << minval[i]
		<< ", " << maxval[i] << "]\n";
	    if (cnt != bits[i]->size())
		out << "Warning: bits[" << i << "] contains "
		    << bits[i]->size()
		    << " bits, but " << cnt << " are expected\n";
	    if (sub.size() == nobs && sub[i])
		sub[i]->print(out, bits[i]->cnt() - bits[i-1]->cnt(),
			      bounds[i-1], bounds[i]);
	}
	if (bits[nobs-1])
	    out << nobs << ": " << cnt << "\t(..., ...);\t"
		<< cnt-bits[nobs-1]->cnt() << "\t[" << bounds[nobs-1]
		<< ", ...)\t[" << min1 << ", " << max1 << "]\n";
    }
    else if (sub.size() == nobs) { // the short format -- with subranges
	out << "right end of bin, bin weight, bit vector size (bytes)\n";
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] == 0) continue;
	    out.precision(12);
	    out << (maxval[i]!=-DBL_MAX?maxval[i]:bounds[i]) << ' '
		<< bits[i]->cnt() << ' ' << bits[i]->bytes() << "\n";
	    if (sub[i])
		sub[i]->print(out, bits[i]->cnt() - bits[i-1]->cnt(),
			      bounds[i-1], bounds[i]);
	}
    }
    else { // the short format -- without subranges
	out << "The three columns are (1) center of bin, (2) bin weight, "
	    "and (3) bit vector size (bytes)\n";
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] && bits[i]->cnt()) {
		out.precision(12);
		out << 0.5*(minval[i]+maxval[i]) << '\t'
		    << bits[i]->cnt() << '\t' << bits[i]->bytes()
		    << "\n";
	    }
	}
    }
    out << "\n";
} // ibis::ambit::print()

void ibis::ambit::print(std::ostream& out, const uint32_t tot,
			const double& lbound, const double& rbound) const {
    if (ibis::gVerbose > 4) { // long format
	out << "\trange [" << lbound << ", " << rbound
	    << ") is subdivided into " << nobs+1 << " bins\n";
	if (bits[0])
	    out << "\t" << bits[0]->cnt() << "\t[" << lbound << ", "
		<< bounds[0] << ")\t\t\t[" << minval[0] << ", "
		<< maxval[0] << "]\n";
	uint32_t i, cnt = nrows;
	for (i = 1; i < nobs; ++i) {
	    if (bits[i] == 0) continue;
	    out << "\t" << bits[i]->cnt() << "\t[" << lbound << ", "
		<< bounds[i] << ");\t" << bits[i]->cnt() - bits[i-1]->cnt()
		<< "\t[" << bounds[i-1] << ", " << bounds[i] << ")\t["
		<< minval[i] << ", "<< maxval[i] << "]\n";
	    if (cnt != bits[i]->size())
		out << "Warning: bits[" << i << "] contains "
		    << bits[i]->size()
		    << " bits, but " << cnt << " are expected\n";
	}
	if (bits[nobs-1])
	    out << "\t" << tot << "\t[" << lbound << ", " << rbound
		<< ");\t" << tot - bits[nobs-1]->cnt() << "\t["
		<< bounds[nobs-1] << ", " << rbound << ")\t[" << min1
		<< ", " << max1 << "]" << "\n";
    }
    else if (sub.size() == nobs) { // the short format -- with subranges
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] == 0) continue;
	    out.precision(12);
	    out << (maxval[i]!=-DBL_MAX?maxval[i]:bounds[i]) << ' '
		<< bits[i]->cnt() << ' ' << bits[i]->bytes() << "\n";
	    if (sub[i] && bits[i-1])
		sub[i]->print(out, bits[i]->cnt() - bits[i-1]->cnt(),
			      bounds[i-1], bounds[i]);
	}
    }
    else { // short format
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] && bits[i]->cnt()) {
		out.precision(12);
		out << 0.5*(minval[i]+maxval[i]) << '\t'
		    << bits[i]->cnt() << '\t' << bits[i]->bytes()
		    << "\n";
	    }
	}
    }
} // ibis::ambit::print

long ibis::ambit::append(const char* dt, const char* df, uint32_t nnew) {
    const uint32_t nold = (strcmp(dt, col->partition()->currentDataDir()) == 0 ?
			   col->partition()->nRows()-nnew : nrows);
    if (nrows != nold) { // recreate the new index
#ifdef APPEND_UPDATE_INDEXES
	LOGGER(ibis::gVerbose > 3)
	    << "ambit::append to build a new index for " << col->name()
	    << " using data in " << dt;
	clear(); // clear the current content
	array_t<double> tmp;
	construct(dt, tmp);
#endif
	return nnew;
    }

    std::string fnm;
    indexFileName(df, fnm);
    ibis::ambit* bin0=0;
    ibis::fileManager::storage* st0=0;
    long ierr = ibis::fileManager::instance().getFile(fnm.c_str(), &st0);
    if (ierr == 0 && st0 != 0) {
	const char* header = st0->begin();
	if (header[0] == '#' && header[1] == 'I' && header[2] == 'B' &&
	    header[3] == 'I' && header[4] == 'S' &&
	    header[5] == ibis::index::AMBIT &&
	    header[7] == static_cast<char>(0)) {
	    bin0 = new ibis::ambit(col, st0);
	}
	else {
	    if (ibis::gVerbose > 5)
		col->logMessage("ambit::append", "file \"%s\" has "
				"unexecpted header -- it will be removed",
				fnm.c_str());
	    ibis::fileManager::instance().flushFile(fnm.c_str());
	    remove(fnm.c_str());
	}
    }
    if (bin0 == 0) {
	ibis::bin bin1(col, df, bounds);
	bin0 = new ibis::ambit(bin1);
    }

    ierr = append(*bin0);
    delete bin0;
    if (ierr == 0) {
	//write(dt); // write out the new content
	return nnew;
    }
    else {
	return ierr;
    }
} // ibis::ambit::append

long ibis::ambit::append(const ibis::ambit& tail) {
    uint32_t i;
    if (tail.col != col) return -1;
    if (tail.nobs != nobs) return -2;
    if (tail.bits.empty()) return -3;
    if (tail.bits[0]->size() != tail.bits[1]->size()) return -4;
    for (i = 0; i < nobs; ++i)
	if (tail.bounds[i] != bounds[i]) return -5;

    array_t<double> max2, min2;
    std::vector<ibis::bitvector*> bin2;
    max2.resize(nobs);
    min2.resize(nobs);
    bin2.resize(nobs);
    activate();
    tail.activate();

    for (i = 0; i < nobs; ++i) {
	if (tail.maxval[i] > maxval[i])
	    max2[i] = tail.maxval[i];
	else
	    max2[i] = maxval[i];
	if (tail.minval[i] < minval[i])
	    min2[i] = tail.minval[i];
	else
	    min2[i] = minval[i];
	bin2[i] = new ibis::bitvector(*bits[i]);
	*bin2[i] += *(tail.bits[i]);
    }

    // replace the current content with the new one
    maxval.swap(max2);
    minval.swap(min2);
    bits.swap(bin2);
    nrows += tail.nrows;
    max1 = (max1<tail.max1?tail.max1:max1);
    min1 = (min1<tail.min1?tail.min1:min1);
    // clearup bin2
    for (i = 0; i < nobs; ++i)
	delete bin2[i];
    max2.clear();
    min2.clear();
    bin2.clear();

    if (sub.size() == nobs && tail.sub.size() == nobs) {
	long ierr = 0;
	for (i = 0; i < nobs; ++i) {
	    if (sub[i] != 0 && tail.sub[i] != 0) {
		ierr -= sub[i]->append(*(tail.sub[i]));
	    }
	    else if (sub[i] != 0 || tail.sub[i] != 0) {
		col->logWarning("ambit::append", "index for the two "
				"subrange %lu must of nil at the same time",
				static_cast<long unsigned>(i));
		delete sub[i];
		sub[i] = 0;
	    }
	}
	if (ierr != 0) return ierr;
    }
    return 0;
} // ibis::ambit::append

long ibis::ambit::evaluate(const ibis::qContinuousRange& expr,
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
} // ibis::ambit::evaluate

// compute the lower and upper bound of the hit vector for the range
// expression
void ibis::ambit::estimate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower,
			   ibis::bitvector& upper) const {
    if (bits.empty()) {
	lower.set(0, nrows);
	upper.set(0, nrows);
	return;
    }

    // when used to decide the which bins to use on the finer level, the
    // range to be searched to assumed to be [lbound, rbound).  The
    // following code attempts to convert equality comparisons into
    // equivalent form of inequality comparisons.  The success of this
    // conversion is highly sensitive to the definition of DBL_EPSILON.  It
    // should be defined as the smallest value x such that (1+x) is
    // different from x.  For a 64-bit IEEE floating-point number, it is
    // appriximately 2.2E-16 (2^{-52}) (May 2, 2001)
    double lbound=-DBL_MAX, rbound=DBL_MAX;
    // bins in the range of [hit0, hit1) are hits
    // bins in the range of [cand0, cand1) are candidates
    uint32_t cand0=0, hit0=0, hit1=0, cand1=0;
    uint32_t bin0 = (expr.leftOperator()!=ibis::qExpr::OP_UNDEFINED) ?
	locate(expr.leftBound()) : 0;
    uint32_t bin1 = (expr.rightOperator()!=ibis::qExpr::OP_UNDEFINED) ?
	locate(expr.rightBound()) : 0;
    switch (expr.leftOperator()) {
    default:
    case ibis::qExpr::OP_UNDEFINED:
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    col->logWarning("ambit::estimate", "operators for the range not "
			    "specified");
	    break;
	case ibis::qExpr::OP_LT:
	    rbound = expr.rightBound();
	    hit0 = 0;
	    cand0 = 0;
	    if (bin1 >= nobs) { // must be bin1 == nobs
		if (expr.rightBound() > max1) {
		    hit1 = nobs + 1;
		    cand1 = nobs + 1;
		}
		else if (expr.rightBound() > min1) {
		    hit1 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    hit1 = nobs;
		    cand1 = nobs;
		}
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		hit1 = bin1 + 1;
		cand1 = bin1 + 1;
	    }
	    else if (expr.rightBound() <= minval[bin1]) {
		hit1 = bin1;
		cand1 = bin1;
	    }
	    else {
		hit1 = bin1;
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    rbound = ibis::util::incrDouble(expr.rightBound());
	    hit0 = 0;
	    cand0 = 0;
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1) {
		    hit1 = nobs + 1;
		    cand1 = nobs + 1;
		}
		else if (expr.rightBound() >= min1) {
		    hit1 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    hit1 = nobs;
		    cand1 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		hit1 = bin1 + 1;
		cand1 = bin1 + 1;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		hit1 = bin1;
		cand1 = bin1;
	    }
	    else {
		hit1 = bin1;
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    lbound = ibis::util::incrDouble(expr.rightBound());
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1) {
		    hit0 = nobs + 1;
		    cand0 = nobs + 1;
		}
		else if (expr.rightBound() >= min1) {
		    hit0 = nobs + 1;
		    cand0 = nobs;
		}
		else {
		    hit0 = nobs;
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1 + 1;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    lbound = expr.rightBound();
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1) {
		    hit0 = nobs + 1;
		    cand0 = nobs + 1;
		}
		else if (expr.rightBound() > min1) {
		    hit0 = nobs + 1;
		    cand0 = nobs;
		}
		else {
		    hit0 = nobs;
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1 + 1;
	    }
	    else if (expr.rightBound() > minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (bin1 >= nobs) {
		if (expr.rightBound() <= max1 &&
		    expr.rightBound() >= min1) {
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
		    if (min1 == max1) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else if (expr.rightBound() <= maxval[bin1] &&
		     expr.rightBound() >= minval[bin1]) {
		hit0 = bin1; hit1 = bin1;
		cand0 = bin1; cand1 = bin1 + 1;
		if (maxval[bin1] == minval[bin1]) hit1 = cand1;
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_UNDEFINED
    case ibis::qExpr::OP_LT:
	lbound = ibis::util::incrDouble(expr.leftBound());
	if (bin0 >= nobs) {
	    if (expr.leftBound() >= max1) {
		hit0 = nobs + 1;
		cand0 = nobs + 1;
	    }
	    else if (expr.leftBound() >= min1) {
		hit0 = nobs + 1;
		cand0 = nobs;
	    }
	    else {
		hit0 = nobs;
		cand0 = nobs;
	    }
	}
	else if (expr.leftBound() >= maxval[bin0]) {
	    hit0 = bin0 + 1;
	    cand0 = bin0 + 1;
	}
	else if (expr.leftBound() < minval[bin0]) {
	    hit0 = bin0;
	    cand0 = bin0;
	}
	else {
	    hit0 = bin0 + 1;
	    cand0 = bin0;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    break;
	case ibis::qExpr::OP_LT:
	    rbound = expr.rightBound();
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1) {
		    hit1 = nobs + 1;
		    cand1 = nobs + 1;
		}
		else if (expr.rightBound() > min1) {
		    hit1 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    hit1 = nobs;
		    cand1 = nobs;
		}
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		hit1 = bin1 + 1;
		cand1 = bin1 + 1;
	    }
	    else if (expr.rightBound() <= minval[bin1]) {
		hit1 = bin1;
		cand1 = bin1;
	    }
	    else {
		hit1 = bin1;
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    rbound = ibis::util::incrDouble(expr.rightBound());
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1) {
		    hit1 = nobs + 1;
		    cand1 = nobs + 1;
		}
		else if (expr.rightBound() >= min1) {
		    hit1 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    hit1 = nobs;
		    cand1 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		hit1 = bin1 + 1;
		cand1 = bin1 + 1;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		hit1 = bin1;
		cand1 = bin1;
	    }
	    else {
		hit1 = bin1;
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (lbound <= expr.rightBound())
		lbound = ibis::util::incrDouble(expr.rightBound());
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1) {
			hit0 = nobs + 1;
			cand0 = nobs + 1;
		    }
		    else if (expr.rightBound() > min1) {
			hit0 = nobs + 1;
			cand0 = nobs;
		    }
		    else {
			hit0 = nobs;
			cand0 = nobs;
		    }
		}
		else if (expr.rightBound() >= maxval[bin1]) {
		    hit0 = bin1 + 1;
		    cand0 = bin1 + 1;
		}
		else if (expr.rightBound() < minval[bin1]) {
		    hit0 = bin1;
		    cand0 = bin1;
		}
		else {
		    hit0 = bin1 + 1;
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (expr.rightBound() >= expr.leftBound()) {
		lbound = expr.rightBound();
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1) {
			hit0 = nobs + 1;
			cand0 = nobs + 1;
		    }
		    else if (expr.rightBound() > min1) {
			hit0 = nobs + 1;
			cand0 = nobs;
		    }
		    else {
			hit0 = nobs;
			cand0 = nobs;
		    }
		}
		else if (expr.rightBound() > maxval[bin1]) {
		    hit0 = bin1 + 1;
		    cand0 = bin1 + 1;
		}
		else if (expr.rightBound() > minval[bin1]) {
		    hit0 = bin1 + 1;
		    cand0 = bin1;
		}
		else {
		    hit0 = bin1;
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1; hit1 = bin1;
		    cand0 = bin1; cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LT
    case ibis::qExpr::OP_LE:
	lbound = expr.leftBound();
	if (bin0 >= nobs) {
	    if (expr.leftBound() > max1) {
		hit0 = nobs + 1;
		cand0 = nobs + 1;
	    }
	    else if (expr.leftBound() > min1) {
		hit0 = nobs + 1;
		cand0 = nobs;
	    }
	    else {
		hit0 = nobs;
		cand0 = nobs;
	    }
	}
	else if (expr.leftBound() > maxval[bin0]) {
	    hit0 = bin0 + 1;
	    cand0 = bin0 + 1;
	}
	else if (expr.leftBound() <= minval[bin0]) {
	    hit0 = bin0;
	    cand0 = bin0;
	}
	else {
	    hit0 = bin0 + 1;
	    cand0 = bin0;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    break;
	case ibis::qExpr::OP_LT:
	    rbound = expr.rightBound();
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1) {
		    hit1 = nobs + 1;
		    cand1 = nobs + 1;
		}
		else if (expr.rightBound() > min1) {
		    hit1 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    hit1 = nobs;
		    cand1 = nobs;
		}
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		hit1 = bin1 + 1;
		cand1 = bin1 + 1;
	    }
	    else if (expr.rightBound() <= minval[bin1]) {
		hit1 = bin1;
		cand1 = bin1;
	    }
	    else {
		hit1 = bin1;
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    rbound = ibis::util::incrDouble(expr.rightBound());
	    if (bin1 > nobs) {
		if (expr.rightBound() >= max1) {
		    hit1 = nobs + 1;
		    cand1 = nobs + 1;
		}
		else if (expr.rightBound() >= min1) {
		    hit1 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    hit1 = nobs;
		    cand1 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		hit1 = bin1 + 1;
		cand1 = bin1 + 1;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		hit1 = bin1;
		cand1 = bin1;
	    }
	    else {
		hit1 = bin1;
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (expr.rightBound() >= expr.leftBound()) {
		lbound = ibis::util::incrDouble(expr.rightBound());
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= max1) {
			hit0 = nobs + 1;
			cand0 = nobs + 1;
		    }
		    else if (expr.rightBound() >= min1) {
			hit0 = nobs + 1;
			cand0 = nobs;
		    }
		    else {
			hit0 = nobs;
			cand0 = nobs;
		    }
		}
		else if (expr.rightBound() >= maxval[bin1]) {
		    hit0 = bin1 + 1;
		    cand0 = bin1 + 1;
		}
		else if (expr.rightBound() < minval[bin1]) {
		    hit0 = bin1;
		    cand0 = bin1;
		}
		else {
		    hit0 = bin1 + 1;
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (lbound < expr.rightBound())
		lbound = expr.rightBound();
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1) {
			hit0 = nobs + 1;
			cand0 = nobs + 1;
		    }
		    else if (expr.rightBound() > min1) {
			hit0 = nobs + 1;
			cand0 = nobs;
		    }
		    else {
			hit0 = nobs;
			cand0 = nobs;
		    }
		}
		else if (expr.rightBound() > maxval[bin1]) {
		    hit0 = bin1 + 1;
		    cand0 = bin1 + 1;
		}
		else if (expr.rightBound() > minval[bin1]) {
		    hit0 = bin1 + 1;
		    cand0 = bin1;
		}
		else {
		    hit0 = bin1;
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= min1 &&
			expr.rightBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1; hit1 = bin1;
		    cand0 = bin1; cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LE
    case ibis::qExpr::OP_GT:
	rbound = expr.leftBound();
	if (bin0 >= nobs) {
	    if (expr.rightBound() > max1) {
		hit1 = nobs + 1;
		cand1 = nobs + 1;
	    }
	    else if (expr.rightBound() > min1) {
		hit1 = nobs;
		cand1 = nobs + 1;
	    }
	    else {
		hit1 = nobs;
		cand1 = nobs;
	    }
	}
	else if (expr.leftBound() > maxval[bin0]) {
	    hit1 = bin0 + 1;
	    cand1 = bin0 + 1;
	}
	else if (expr.leftBound() <= minval[bin0]) {
	    hit1 = bin0;
	    cand1 = bin0;
	}
	else {
	    hit1 = bin0;
	    cand1 = bin0 + 1;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    cand0 = 0; hit0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    if (rbound > expr.rightBound()) rbound = expr.rightBound();
	    hit0 = 0;
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1) {
			hit1 = nobs + 1;
			cand1 = nobs + 1;
		    }
		    else if (expr.rightBound() > min1) {
			hit1 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			hit1 = nobs;
			cand1 = nobs;
		    }
		}
		else if (expr.rightBound() > maxval[bin1]) {
		    hit1 = bin1 + 1;
		    cand1 = bin1 + 1;
		}
		else if (expr.rightBound() <= minval[bin1]) {
		    hit1 = bin1;
		    cand1 = bin1;
		}
		else {
		    hit1 = bin1;
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    hit0 = 0;
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		rbound = ibis::util::incrDouble(expr.rightBound());
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= max1) {
			hit1 = nobs + 1;
			cand1 = nobs + 1;
		    }
		    else if (expr.rightBound() >= min1) {
			hit1 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			hit1 = nobs;
			cand1 = nobs;
		    }
		}
		else if (expr.rightBound() >= maxval[bin1]) {
		    hit1 = bin1 + 1;
		    cand1 = bin1 + 1;
		}
		else if (expr.rightBound() < minval[bin1]) {
		    hit1 = bin1;
		    cand1 = bin1;
		}
		else {
		    hit1 = bin1;
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    lbound = ibis::util::incrDouble(expr.rightBound());
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1) {
		    hit0 = nobs + 1;
		    cand0 = nobs + 1;
		}
		else if (expr.rightBound() >= min1) {
		    hit0 = nobs + 1;
		    cand0 = nobs;
		}
		else {
		    hit0 = nobs;
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1 + 1;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    lbound = expr.rightBound();
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1) {
		    hit0 = nobs + 1;
		    cand0 = nobs + 1;
		}
		else if (expr.rightBound() > min1) {
		    hit0 = nobs + 1;
		    cand0 = nobs;
		}
		else {
		    hit0 = nobs;
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1 + 1;
	    }
	    else if (expr.rightBound() > minval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= min1 &&
			expr.rightBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1; hit1 = bin1;
		    cand0 = bin1; cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GT
    case ibis::qExpr::OP_GE:
	rbound = ibis::util::incrDouble(expr.leftBound());
	if (bin0 >= nobs) {
	    if (expr.leftBound() >= max1) {
		hit1 = nobs + 1;
		cand1 = nobs + 1;
	    }
	    else if (expr.leftBound() > min1) {
		hit1 = nobs;
		cand1 = nobs + 1;
	    }
	    else {
		hit1 = nobs;
		cand1 = nobs;
	    }
	}
	else if (expr.leftBound() >= maxval[bin0]) {
	    hit1 = bin0 + 1;
	    cand1 = bin0 + 1;
	}
	else if (expr.leftBound() > minval[bin0]) {
	    hit1 = bin0;
	    cand1 = bin0 + 1;
	}
	else {
	    hit1 = bin0;
	    cand1 = bin0;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    hit0 = 0; cand0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    hit0 = 0;
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		rbound = expr.rightBound();
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1) {
			hit1 = nobs + 1;
			cand1 = nobs + 1;
		    }
		    else if (expr.rightBound() > min1) {
			hit1 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			hit1 = nobs;
			cand1 = nobs;
		    }
		}
		else if (expr.rightBound() > maxval[bin1]) {
		    hit1 = bin1 + 1;
		    cand1 = bin1 + 1;
		}
		else if (expr.rightBound() <= minval[bin1]) {
		    hit1 = bin1;
		    cand1 = bin1;
		}
		else {
		    hit1 = bin1;
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (rbound > expr.rightBound())
		rbound = ibis::util::incrDouble(expr.rightBound());
	    hit0 = 0;
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= max1) {
			hit1 = nobs + 1;
			cand1 = nobs + 1;
		    }
		    else if (expr.rightBound() >= min1) {
			hit1 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			hit1 = nobs;
			cand1 = nobs;
		    }
		}
		else if (expr.rightBound() >= maxval[bin1]) {
		    hit1 = bin1 + 1;
		    cand1 = bin1 + 1;
		}
		else if (expr.rightBound() < minval[bin1]) {
		    hit1 = bin1;
		    cand1 = bin1;
		}
		else {
		    hit1 = bin1;
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    lbound = ibis::util::incrDouble(expr.rightBound());
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1) {
		    hit0 = nobs + 1;
		    cand0 = nobs + 1;
		}
		else if (expr.rightBound() >= min1) {
		    hit0 = nobs + 1;
		    cand0 = nobs;
		}
		else {
		    hit0 = nobs;
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1 + 1;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    lbound = expr.rightBound();
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1) {
		    hit0 = nobs + 1;
		    cand0 = nobs + 1;
		}
		else if (expr.rightBound() > min1) {
		    hit0 = nobs + 1;
		    cand0 = nobs;
		}
		else {
		    hit0 = nobs;
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1 + 1;
	    }
	    else if (expr.rightBound() > minval[bin1]) {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= min1 &&
			expr.rightBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1; hit1 = bin1;
		    cand0 = bin1; cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GE
    case ibis::qExpr::OP_EQ:
	ibis::util::eq2range(expr.leftBound(),lbound, rbound);
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    if (bin0 >= nobs) {
		if (expr.leftBound() >= min1 &&
		    expr.rightBound() <= max1) {
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
		    if (max1 == min1) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else if (expr.leftBound() <= maxval[bin0] &&
		     expr.leftBound() >= minval[bin0]) {
		hit0 = bin0; hit1 = bin0;
		cand0 = bin0; cand1 = bin0 + 1;
		if (maxval[bin0] == minval[bin0]) hit1 = cand1;
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LT:
	    if (expr.leftBound() < expr.rightBound()) {
		if (bin0 >= nobs) {
		    if (expr.leftBound() >= min1 &&
			expr.leftBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0; hit1 = bin0;
		    cand0 = bin0; cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (expr.leftBound() <= expr.rightBound()) {
		if (bin0 >= nobs) {
		    if (expr.leftBound() >= min1 &&
			expr.leftBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0; hit1 = bin0;
		    cand0 = bin0; cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (expr.leftBound() > expr.rightBound()) {
		if (bin0 >= nobs) {
		    if (expr.leftBound() >= min1 &&
			expr.leftBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0; hit1 = bin0;
		    cand0 = bin0; cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (expr.leftBound() >= expr.rightBound()) {
		if (bin0 >= nobs) {
		    if (expr.leftBound() >= min1 &&
			expr.rightBound() <= max1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0; hit1 = bin0;
		    cand0 = bin0; cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.leftBound() == expr.rightBound()) {
		if (bin0 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			hit0 = nobs; hit1 = nobs;
			cand0 = nobs; cand1 = nobs + 1;
			if (max1 == min1) hit1 = cand1;
		    }
		    else {
			hit0 = hit1 = cand0 = cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1; hit1 = bin1;
		    cand0 = bin1; cand1 = bin1 + 1;
		    if (maxval[bin0] == minval[bin0]) hit1 = cand1;
		}
		else {
		    hit0 = hit1 = cand0 = cand1 = 0;
		}
	    }
	    else {
		hit0 = hit1 = cand0 = cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_EQ
    } // switch (expr.leftOperator())
    LOGGER(ibis::gVerbose > 5)
	<< "ibis::ambit::estimate(" << expr << ") bin number [" << cand0
	<< ":" << hit0 << ", " << hit1 << ":" << cand1 << ") boundaries ["
	<< (minval[cand0]<bounds[cand0] ? minval[cand0] : bounds[cand0]) << ":"
	<< (minval[hit0]<bounds[hit0] ? minval[hit0] : bounds[hit0]) << ", "
	<< (hit1>hit0 ? (maxval[hit1-1] < bounds[hit1-1] ?
			 maxval[hit1-1] : bounds[hit1-1]) :
	    (minval[hit0]<bounds[hit0] ? minval[hit0] : bounds[hit0])) << ":"
	<< (cand1>cand0 ? (maxval[cand1-1] < bounds[cand1-1] ?
			   maxval[cand1-1] : bounds[cand1-1]) :
	    (minval[cand0] < bounds[0] ? minval[cand0] : bounds[0])) << ")";

    uint32_t i, j;
    ibis::bitvector *tmp = 0;
    bool same = false; // are upper and lower the same ?
    // attempt to generate lower and upper bounds together
    if (cand0 >= cand1) {
	lower.set(0, nrows);
	upper.clear();
    }
    else if (cand0 == hit0 && cand1 == hit1) { // top level only
	if (hit0 >= hit1) {
	    lower.set(0, nrows);
	    upper.set(0, nrows);
	}
	else if (hit1 <= nobs && hit0 > 0) { // closed range
	    if (hit1 > hit0) {
		if (bits[hit1-1] == 0)
		    activate(hit1-1);
		if (bits[hit1-1] != 0) {
		    lower.copy(*(bits[hit1-1]));
		    if (bits[hit0-1] == 0)
			activate(hit0-1);
		    if (bits[hit0-1] != 0)
			lower -= *(bits[hit0-1]);
		}
		else {
		    lower.set(0, nrows);
		}
		upper.copy(lower);
	    }
	    else {
		lower.set(0, nrows);
		upper.set(0, nrows);
	    }
	}
	else if (hit0 > 0) { // open to the right (+infinity)
	    if (bits[hit0-1] == 0)
		activate(hit0-1);
	    if (bits[hit0-1] != 0) {
		lower.copy(*(bits[hit0-1]));
		lower.flip();
	    }
	    else {
		lower.set(1, nrows);
	    }
	    upper.copy(lower);
	}
	else if (hit1 <= nobs) {
	    if (bits[hit1-1] == 0)
		activate(hit1-1);
	    if (bits[hit1-1] != 0) {
		lower.copy(*(bits[hit1-1]));
		upper.copy(*(bits[hit1-1]));
	    }
	    else {
		lower.set(0, nrows);
		upper.set(0, nrows);
	    }
	}
	else {
	    lower.set(1, nrows);
	    upper.set(1, nrows);
	}
    }
    else if (cand0+1 == cand1) { // all in one coarse bin
	if (cand0 >= nobs) { // unrecorded (coarse) bin
	    if (bits[nobs-1] == 0)
		activate(nobs-1);
	    if (bits[nobs-1] != 0) {
		upper.copy(*(bits[nobs-1]));
		upper.flip();
	    }
	    else {
		upper.set(1, nrows);
	    }
	    lower.set(0, upper.size());
	}
	else if (sub.size() == nobs) {
	    if (sub[cand0]) { // subrange cand0 exists
		// deal with the right side of query range
		i = sub[cand0]->locate(rbound);
		if (i >= sub[cand0]->nobs) { // unrecorded (fine) bin
		    if (rbound > sub[cand0]->max1) {
			same = true;
			if (bits[cand0] == 0)
			    activate(cand0);
			if (cand0 > 0) {
			    if (bits[cand0] != 0) {
				lower.copy(*(bits[cand0]));
				if (bits[cand0-1] == 0)
				    activate(cand0-1);
				if (bits[cand0-1] != 0)
				    lower -= *(bits[cand0-1]);
			    }
			    else {
				lower.set(0, nrows);
			    }
			}
			else {
			    if (bits[0] == 0)
				activate(0);
			    if (bits[0])
				lower.copy(*(bits[0]));
			    else
				lower.set(0, nrows);
			}
		    }
		    else if (rbound > sub[cand0]->min1) {
			if (bits[cand0] == 0)
			    activate(cand0);
			if (cand0 > 0) {
			    if (bits[cand0] != 0) {
				upper.copy(*(bits[cand0]));
				if (bits[cand0-1] == 0)
				    activate(cand0-1);
				if (bits[cand0-1] != 0)
				    upper -= *(bits[cand0-1]);
			    }
			    else {
				upper.set(0, nrows);
			    }
			}
			else {
			    if (bits[0] == 0)
				activate(0);
			    if (bits[0])
				upper.copy(*(bits[0]));
			    else
				upper.set(0, nrows);
			}

			if (sub[cand0]->bits[sub[cand0]->nobs-1] == 0)
			    sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1] != 0)
			    lower.copy(*(sub[cand0]->bits.back()));
			else
			    lower.set(0, nrows);
		    }
		    else {
			same = true;
			if (sub[cand0]->bits[sub[cand0]->nobs-1] == 0)
			    sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1] != 0)
			    lower.copy(*(sub[cand0]->bits.back()));
			else
			    lower.set(0, nrows);
		    }
		}
		else if (rbound > sub[cand0]->maxval[i]) {
		    same = true;
		    if (sub[cand0]->bits[i] == 0)
			sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i] != 0)
			lower.copy(*(sub[cand0]->bits[i]));
		    else
			lower.set(0, nrows);
		}
		else if (rbound > sub[cand0]->minval[i]) {
		    sub[cand0]->activate((i>0?i-1:0), i+1);
		    if (sub[cand0]->bits[i] != 0)
			upper.copy(*(sub[cand0]->bits[i]));
		    if (i > 0 && sub[cand0]->bits[i-1] != 0)
			lower.copy(*(sub[cand0]->bits[i-1]));
		    else
			lower.set(0, nrows);
		}
		else {
		    same = true;
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1] != 0)
			    lower.copy(*(sub[cand0]->bits[i-1]));
			else
			    lower.set(0, nrows);
		    }
		    else {
			lower.set(0, nrows);
		    }
		}

		// left side of query range
		i = sub[cand0]->locate(lbound);
		if (i >= sub[cand0]->nobs) {
		    if (lbound > sub[cand0]->max1) {
			lower.set(0, nrows);
			upper.set(0, nrows);
		    }
		    else if (lbound > sub[cand0]->min1) {
			if (same)
			    upper.copy(lower);
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1])
			    upper -= *(sub[cand0]->bits[sub[cand0]->nobs-1]);
			lower.set(0, nrows);
		    }
		    else if (same) {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1])
			    lower -= *(sub[cand0]->bits.back());
			upper.copy(lower);
		    }
		    else {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1])
			    lower -= *(sub[cand0]->bits.back());
			if (sub[cand0]->bits[sub[cand0]->nobs-1])
			    upper -= *(sub[cand0]->bits.back());
		    }
		}
		else if (lbound > sub[cand0]->maxval[i]) {
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i]) {
			if (same) {
			    lower -= *(sub[cand0]->bits[i]);
			    upper.copy(lower);
			}
			else {
			    lower -= *(sub[cand0]->bits[i]);
			    upper -= *(sub[cand0]->bits[i]);
			}
		    }
		}
		else if (lbound > sub[cand0]->minval[i]) {
		    if (same) {
			upper.copy(lower);
		    }
		    sub[cand0]->activate((i>0?i-1:0), i+1);
		    if (sub[cand0]->bits[i])
			lower -= *(sub[cand0]->bits[i]);
		    if (i > 0 && sub[cand0]->bits[i-1] != 0)
			upper -= *(sub[cand0]->bits[i-1]);
		}
		else if (same) {
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1])
			    lower -= *(sub[cand0]->bits[i-1]);
		    }
		    upper.copy(lower);
		}
		else if (i > 0) {
		    sub[cand0]->activate(i-1);
		    if (sub[cand0]->bits[i-1]) {
			lower -= *(sub[cand0]->bits[i-1]);
			upper -= *(sub[cand0]->bits[i-1]);
		    }
		}
	    }
	    else { // no subrange cand0
		lower.set(0, nrows);
		if (cand0 > 0) {
		    activate(cand0-1, cand0+1);
		    if (bits[cand0] != 0) {
			upper.copy(*(bits[cand0]));
			if (bits[cand0-1] != 0)
			    upper -= *(bits[cand0-1]);
		    }
		    else {
			upper.set(0, nrows);
		    }
		}
		else {
		    if (bits[cand0] == 0)
			activate(cand0);
		    if (bits[cand0] != 0)
			upper.copy(*(bits[cand0]));
		    else
			upper.set(0, nrows);
		}
	    }
	}
	else { // no subrange at all
	    lower.set(0, nrows);
	    if (cand0 > 0) {
		activate(cand0-1, cand0+1);
		if (bits[cand0]) {
		    upper.copy(*(bits[cand0]));
		    if (bits[cand0-1])
			upper -= *(bits[cand0-1]);
		}
		else {
		    upper.set(0, nrows);
		}
	    }
	    else {
		if (bits[cand0] == 0)
		    activate(cand0);
		if (bits[cand0] != 0)
		    upper.copy(*(bits[cand0]));
		else
		    upper.set(0, nrows);
	    }
	}
    }
    else if (cand0 == hit0) { // the right end needs the finer level
	// implicitly: hit1+1 == cand1, hit1 <= nobs
	if (hit1 >= nobs) { // cand1 > nobs, i.e., open to the right
	    if (hit0 > 0) {
		activate(nobs-1);
		if (bits[nobs-1]) {
		    lower.copy(*(bits[nobs-1]));
		    activate(hit0-1);
		    if (bits[hit0-1])
			lower -= *(bits[hit0-1]);
		}
		else {
		    lower.set(0, nrows);
		}
		if (bits[hit0-1] == 0)
		    activate(hit0-1);
		if (bits[hit0-1] != 0) {
		    upper.copy(*bits[hit0-1]);
		    upper.flip();
		}
		else {
		    upper.set(1, nrows);
		}
	    }
	    else {
		activate(nobs-1);
		if (bits[nobs-1])
		    lower.copy(*(bits[nobs-1]));
		else
		    lower.set(0, nrows);
		upper.set(1, nrows);
	    }
	}
	else { // hit1 < nobs
	    j = hit1 - 1;
	    if (sub.size() == nobs) { // sub is defined
		if (sub[hit1]) { // this particular subrange exists
		    i = sub[hit1]->locate(rbound);
		    if (i >= sub[hit1]->nobs) { // fall in the unrecorded one
			if (rbound > sub[hit1]->max1) {
			    same = true;
			    if (bits[hit1] == 0)
				activate(hit1);
			    if (bits[hit1] != 0)
				lower.copy(*(bits[hit1]));
			    else
				lower.set(0, nrows);
			}
			else if (rbound > sub[hit1]->min1) {
			    if (j < nobs) {
				if (bits[j] == 0)
				    activate(j);
				if (bits[j] != 0) {
				    lower.copy(*(bits[j]));
				    sub[hit1]->activate(sub[hit1]->nobs-1);
				    if (sub[hit1]->bits[sub[hit1]->nobs-1])
					lower |= *(sub[hit1]->bits
						   [sub[hit1]->nobs-1]);
				}
				else {
				    sub[hit1]->activate(sub[hit1]->nobs-1);
				    if (sub[hit1]->bits[sub[hit1]->nobs-1])
					lower.copy(*(sub[hit1]->bits
						     [sub[hit1]->nobs-1]));
				    else
					lower.set(0, nrows);
				}
			    }
			    else {
				sub[hit1]->activate(sub[hit1]->nobs-1);
				if (sub[hit1]->bits[sub[hit1]->nobs-1])
				    lower.copy(*(sub[hit1]->bits.back()));
				else
				    lower.set(0, nrows);
			    }
			    if (bits[hit1] == 0)
				activate(hit1);
			    if (bits[hit1] != 0)
				upper.copy(*(bits[hit1]));
			    else
				upper.set(0, nrows);
			}
			else {
			    same = true;
			    if (j < nobs) {
				if (bits[j] == 0)
				    activate(j);
				if (bits[j] != 0) {
				    lower.copy(*(bits[j]));
				    if (sub[hit1]->bits[sub[hit1]->nobs-1]
					== 0)
					sub[hit1]->
					    activate(sub[hit1]->nobs-1);
				    if (sub[hit1]->bits[sub[hit1]->nobs-1]
					!= 0)
					lower |= *(sub[hit1]->bits
						   [sub[hit1]->nobs-1]);
				}
				else {
				    if (sub[hit1]->bits[sub[hit1]->nobs-1]
					== 0)
					sub[hit1]->
					    activate(sub[hit1]->nobs-1);
				    if (sub[hit1]->bits[sub[hit1]->nobs-1]
					!= 0)
					lower.copy(*(sub[hit1]->bits
						     [sub[hit1]->nobs-1]));
				    else
					lower.set(0, nrows);
				}
			    }
			    else {
				lower.copy(*(sub[hit1]->bits.back()));
			    }
			}
		    }
		    else if (rbound > sub[hit1]->maxval[i]) {
			same = true;
			if (j < nobs) {
			    if (bits[j] == 0)
				activate(j);
			    if (bits[j]) {
				lower.copy(*(bits[j]));
				sub[hit1]->activate(i);
				if (sub[hit1]->bits[i])
				    lower |=  *(sub[hit1]->bits[i]);
			    }
			    else {
				sub[hit1]->activate(i);
				if (sub[hit1]->bits[i])
				    lower.copy(*(sub[hit1]->bits[i]));
				else
				    lower.set(0, nrows);
			    }
			}
			else {
			    sub[hit1]->activate(i);
			    if (sub[hit1]->bits[i])
				lower.copy(*(sub[hit1]->bits[i]));
			    else
				lower.set(0, nrows);
			}
		    }
		    else if (rbound > sub[hit1]->minval[i]) {
			if (j < nobs) {
			    if (bits[j] == 0)
				activate(j);
			    if (bits[j])
				lower.copy(*(bits[j]));
			    else
				lower.set(0, nrows);
			}
			else {
			    lower.set(0, nrows);
			}
			upper.copy(lower);
			sub[hit1]->activate((i>0?i-1:0), i+1);
			if (i > 0 && sub[hit1]->bits[i-1] != 0)
			    lower |= *(sub[hit1]->bits[i-1]);
			if (sub[hit1]->bits[i] != 0)
			    upper |= *(sub[hit1]->bits[i]);
		    }
		    else {
			same = true;
			if (j < nobs) {
			    if (bits[j] == 0)
				activate(j);
			    if (bits[j] != 0)
				lower.copy(*(bits[j]));
			    else
				lower.set(0, nrows);
			}
			else {
			    lower.set(0, nrows);
			}
			if (i > 0) {
			    sub[hit1]->activate(i-1);
			    if (sub[hit1]->bits[i-1])
				lower |= *(sub[hit1]->bits[i-1]);
			}
		    }
		}
		else {
		    if (j < nobs) {
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0)
			    lower.copy(*(bits[j]));
			else
			    lower.set(0, nrows);
		    }
		    else {
			lower.set(0, nrows);
		    }
		    if (bits[hit1] == 0)
			activate(hit1);
		    if (bits[hit1] != 0)
			upper.copy(*(bits[hit1]));
		    else
			upper.set(0, nrows);
		}
	    }
	    else {
		if (j < nobs) {
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j] != 0)
			lower.copy(*(bits[j]));
		    else
			lower.set(0, nrows);
		}
		else {
		    lower.set(0, nrows);
		}
		if (bits[hit1] == 0)
		    activate(hit1);
		if (bits[hit1] != 0)
		    upper.copy(*(bits[hit1]));
		else
		    upper.set(0, nrows);
	    }

	    if (hit0 > 0) { // closed range
		if (bits[hit0-1] == 0)
		    activate(hit0-1);
		if (bits[hit0-1])
		    lower -= *(bits[hit0-1]);
		if (same) {
		    upper.copy(lower);
		}
		else if (bits[hit0-1]) {
		    upper -= *(bits[hit0-1]);
		}
	    }
	    else if (same) {
		upper.copy(lower);
	    }
	}
    }
    else if (cand1 == hit1) { // the left end needs finer level
	// implcitly: cand0=hit0-1; hit0 > 0
	if (hit1 <= nobs) {
	    if (bits[hit1-1] == 0)
		activate(hit1-1);
	    if (bits[hit1-1])
		lower.copy(*(bits[hit1-1]));
	    else
		lower.set(0, nrows);
	}
	else {
	    lower.set(1, nrows);
	}
	if (cand0 == 0) { // sub[0] is never defined
	    upper.copy(lower);
	    if (bits[1] == 0)
		activate(1);
	    if (bits[1] != 0)
		lower -= *(bits[1]);
	}
	else if (sub.size() == nobs && sub[cand0] != 0) { // sub defined
	    j = cand0 - 1;
	    i = sub[cand0]->locate(lbound);
	    if (i >= sub[cand0]->nobs) { // unrecorded sub-range
		if (lbound > sub[cand0]->max1) { // encomposes all
		    if (bits[cand0] == 0)
			activate(cand0);
		    if (bits[cand0] != 0)
			lower -= *(bits[cand0]);
		    upper.copy(lower);
		}
		else if (lbound > sub[cand0]->min1) {
		    // upper includes the unrecorded sub-range
		    upper.copy(lower);
		    if (bits[cand0] == 0)
			activate(cand0);
		    if (bits[cand0] != 0)
			lower -= *(bits[cand0]);
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j] != 0)
			tmp = new ibis::bitvector(*(bits[j]));
		    else
			tmp = 0;
		    sub[cand0]->activate(sub[cand0]->nobs-1);
		    if (sub[cand0]->bits[sub[cand0]->nobs-1]) {
			if (tmp != 0)
			    *tmp |= *(sub[cand0]->bits
				      [sub[cand0]->nobs-1]);
			else
			    tmp = new ibis::bitvector
				(*(sub[cand0]->
				   bits[sub[cand0]->nobs-1]));
		    }
		    if (tmp != 0) {
			upper -= *tmp;
			delete tmp;
		    }
		}
		else { // below the actual min (min1)
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j] != 0)
			tmp = new ibis::bitvector(*(bits[j]));
		    else
			tmp = 0;
		    sub[cand0]->activate(sub[cand0]->nobs-1);
		    if (sub[cand0]->bits[sub[cand0]->nobs-1]) {
			if (tmp != 0)
			    *tmp |= *(sub[cand0]->
				      bits[sub[cand0]->nobs-1]);
			else
			    tmp = new ibis::bitvector
				(*(sub[cand0]->
				   bits[sub[cand0]->nobs-1]));
		    }
		    if (tmp != 0) {
			lower -= *tmp;
			delete tmp;
		    }
		    upper.copy(lower);
		}
	    }
	    else if (lbound > sub[cand0]->maxval[i]) {
		if (bits[j] == 0)
		    activate(j);
		if (bits[j] != 0)
		    tmp = new ibis::bitvector(*(bits[j]));
		else
		    tmp = 0;
		sub[cand0]->activate(i);
		if (sub[cand0]->bits[i] != 0) {
		    if (tmp != 0)
			*tmp |= *(sub[cand0]->bits[i]);
		    else
			tmp = new ibis::bitvector
			    (*(sub[cand0]->bits[i]));
		}
		if (tmp != 0) {
		    lower -= *tmp;
		    delete tmp;
		}
		upper.copy(lower);
	    }
	    else if (lbound > sub[cand0]->minval[i]) {
		if (bits[j] == 0)
		    activate(j);
		if (bits[j] != 0)
		    lower -= *(bits[j]);
		upper.copy(lower);
		sub[cand0]->activate((i>0?i-1:0), i+1);
		if (i > 0 && sub[cand0]->bits[i-1] != 0)
		    upper -= *(sub[cand0]->bits[i-1]);
		if (sub[cand0]->bits[i] != 0)
		    lower -= *(sub[cand0]->bits[i]);
	    }
	    else {
		if (i > 0) {
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j] != 0)
			tmp = new ibis::bitvector(*(bits[j]));
		    else
			tmp = 0;
		    sub[cand0]->activate(i-1);
		    if (sub[cand0]->bits[i-1] != 0) {
			if (tmp != 0)
			    *tmp |= *(sub[cand0]->bits[i-1]);
			else
			    tmp = new ibis::bitvector
				(*(sub[cand0]->bits[i-1]));
		    }
		    if (tmp != 0) {
			lower -= *tmp;
			delete tmp;
		    }
		}
		else {
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j] != 0)
			lower -= *(bits[j]);
		}
		upper.copy(lower);
	    }
	}
	else {
	    upper.copy(lower);
	    activate(cand0-1, cand0+1);
	    if (bits[cand0])
		lower -= *(bits[cand0]);
	    if (bits[cand0-1])
		upper -= *(bits[cand0-1]);
	}
    }
    else { // both ends need the finer level
	// first deal with the right end of the range
	j = hit1 - 1;
	if (hit1 >= nobs) { // right end located in the unrecorded bin
	    upper.set(1, nrows);
	    if (bits[nobs-1] == 0)
		activate(nobs-1);
	    if (bits[nobs-1] != 0)
		lower.copy(*(bits[nobs-1]));
	    else
		lower.set(0, nrows);
	}
	else if (sub.size() == nobs) { // sub defined
	    if (sub[hit1]) { // the specific subrange exists
		i = sub[hit1]->locate(rbound);
		if (i >= sub[hit1]->nobs) { // fall in the unrecorded one
		    if (rbound > sub[hit1]->max1) {
			same = true;
			if (bits[hit1] == 0)
			    activate(hit1);
			if (bits[hit1] != 0)
			    lower.copy(*(bits[hit1]));
			else
			    lower.set(0, nrows);
		    }
		    else if (rbound > sub[hit1]->min1) {
			if (j < nobs) {
			    if (bits[j] == 0)
				activate(j);
			    if (bits[j] != 0) {
				lower.copy( *(bits[j]));
				const uint32_t ks = sub[hit1]->nobs-1;
				sub[hit1]->activate(ks);
				if (sub[hit1]->bits[ks] != 0)
				    lower |= *(sub[hit1]->bits[ks]);
			    }
			    else {
				const uint32_t ks = sub[hit1]->nobs-1;
				sub[hit1]->activate(ks);
				if (sub[hit1]->bits[ks] != 0)
				    lower.copy(*(sub[hit1]->bits[ks]));
				else
				    lower.set(0, nrows);
			    }
			}
			else {
			    const uint32_t ks = sub[hit1]->nobs-1;
			    sub[hit1]->activate(ks);
			    if (sub[hit1]->bits[ks] != 0)
				lower.copy(*(sub[hit1]->bits[ks]));
			    else
				lower.set(0, nrows);
			}
			if (bits[hit1] == 0)
			    activate(hit1);
			if (bits[hit1] != 0)
			    upper.copy(*(bits[hit1]));
			else
			    upper.set(0, nrows);
		    }
		    else {
			same = true;
			if (j < nobs) {
			    if (bits[j] == 0)
				activate(j);
			    if (bits[j] != 0) {
				lower.copy(*(bits[j]));
				const uint32_t ks = sub[hit1]->nobs-1;
				sub[hit1]->activate(ks);
				if (sub[hit1]->bits[ks] != 0)
				    lower |= *(sub[hit1]->bits[ks]);
			    }
			    else {
				const uint32_t ks = sub[hit1]->nobs-1;
				sub[hit1]->activate(ks);
				if (sub[hit1]->bits[ks] != 0)
				    lower.copy(*(sub[hit1]->bits[ks]));
				else
				    lower.set(0, nrows);
			    }
			}
			else {
			    const uint32_t ks = sub[hit1]->nobs-1;
			    sub[hit1]->activate(ks);
			    if (sub[hit1]->bits[ks] != 0)
				lower.copy(*(sub[hit1]->bits[ks]));
			    else
				lower.set(0, nrows);
			}
		    }
		}
		else if (rbound > sub[hit1]->maxval[i]) {
		    same = true;
		    if (j < nobs) {
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0) {
			    lower.copy(*(bits[j]));
			    sub[hit1]->activate(i);
			    if (sub[hit1]->bits[i])
				lower |= *(sub[hit1]->bits[i]);
			}
			else {
			    sub[hit1]->activate(i);
			    if (sub[hit1]->bits[i])
				lower.copy(*(sub[hit1]->bits[i]));
			    else
				lower.set(0, nrows);
			}
		    }
		    else {
			sub[hit1]->activate(i);
			if (sub[hit1]->bits[i])
			    lower.copy(*(sub[hit1]->bits[i]));
			else
			    lower.set(0, nrows);
		    }
		}
		else if (rbound > sub[hit1]->minval[i]) {
		    if (j < nobs) {
			if (bits[j] != 0)
			    activate(j);
			if (bits[j] != 0)
			    lower.copy(*(bits[j]));
			else
			    lower.set(0, nrows);
		    }
		    else {
			lower.set(0, nrows);
		    }
		    if (i > 0) {
			sub[hit1]->activate(i-1);
			if (sub[hit1]->bits[i-1])
			    lower |= *(sub[hit1]->bits[i-1]);
		    }
		    if (j < nobs) {
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0) {
			    upper.copy(*(bits[j]));
			    sub[hit1]->activate(i);
			    if (sub[hit1]->bits[i])
				upper |= *(sub[hit1]->bits[i]);
			}
			else {
			    sub[hit1]->activate(i);
			    if (sub[hit1]->bits[i])
				upper.copy(*(sub[hit1]->bits[i]));
			    else
				upper.set(0, nrows);
			}
		    }
		    else {
			sub[hit1]->activate(i);
			if (sub[hit1]->bits[i])
			    upper.copy(*(sub[hit1]->bits[i]));
			else
			    upper.set(0, nrows);
		    }
		}
		else {
		    same = true;
		    if (j < nobs) {
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0)
			    lower.copy(*(bits[j]));
			else
			    lower.set(0, nrows);
		    }
		    else {
			lower.set(0, nrows);
		    }
		    if (i > 0) {
			sub[hit1]->activate(i-1);
			if (sub[hit1]->bits[i-1])
			    lower |= *(sub[hit1]->bits[i-1]);
		    }
		}
	    }
	    else {
		if (bits[hit1] == 0)
		    activate(hit1);
		if (bits[hit1] != 0)
		    upper.copy(*(bits[hit1]));
		else
		    upper.set(0, nrows);

		if (bits[j] == 0)
		    activate(j);
		if (bits[j] != 0)
		    lower.copy(*(bits[j]));
		else
		    lower.set(0, nrows);
	    }
	}
	else {
	    if (bits[hit1] == 0)
		activate(hit1);
	    if (bits[hit1] != 0)
		upper.copy(*(bits[hit1]));
	    else
		upper.set(0, nrows);

	    if (bits[j] == 0)
		activate(j);
	    if (bits[j] != 0)
		lower.copy(*(bits[j]));
	    else
		lower.set(0, nrows);
	}

	// deal with the lower (left) boundary
	j = cand0 - 1;
	if (cand0 == 0) { // sub[0] never defined
	    if (same)
		upper.copy(lower);

	    if (bits[1] == 0)
		activate(1);
	    if (bits[1] != 0)
		lower -= *(bits[1]);
	}
	else if (sub.size() == nobs) { // sub defined
	    if (sub[cand0]) { // the particular subrange is defined
		i = sub[cand0]->locate(lbound);
		if (i >= sub[cand0]->nobs) { // unrecorded sub-range
		    if (lbound > sub[cand0]->max1) {
			if (bits[cand0] == 0)
			    activate(cand0);
			if (bits[cand0] != 0)
			    lower -= *(bits[cand0]);
			upper.copy(lower);
		    }
		    else if (lbound > sub[cand0]->min1) {
			if (same)
			    upper.copy(lower);

			if (bits[cand0] == 0)
			    activate(cand0);
			if (bits[cand0] != 0)
			    lower -= *(bits[cand0]);
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0)
			    upper -= *(bits[j]);
			const uint32_t ks = sub[cand0]->nobs-1;
			sub[cand0]->activate(ks);
			if (sub[cand0]->bits[ks] != 0)
			    upper -= *(sub[cand0]->bits[ks]);
		    }
		    else {
			if (bits[j] == 0)
			    activate(j);
			const uint32_t ks = sub[cand0]->nobs-1;
			sub[cand0]->activate(ks);
			if (same) {
			    if (bits[j] != 0)
				lower -= *(bits[j]);
			    if (sub[cand0]->bits[ks] != 0)
				lower -= *(sub[cand0]->bits[ks]);
			    upper.copy(lower);
			}
			else {
			    if (bits[j] != 0) {
				lower -= *(bits[j]);
				upper -= *(bits[j]);
			    }
			    if (sub[cand0]->bits[ks] != 0) {
				lower -= *(sub[cand0]->bits[ks]);
				upper -= *(sub[cand0]->bits[ks]);
			    }
			}
		    }
		}
		else if (lbound > sub[cand0]->maxval[i]) {
		    if (bits[j] == 0)
			activate(j);
		    sub[cand0]->activate(i);
		    if (same) {
			if (bits[j] != 0)
			    lower -= *(bits[j]);
			if (sub[cand0]->bits[i] != 0)
			    lower -= *(sub[cand0]->bits[i]);
			upper.copy(lower);
		    }
		    else {
			if (bits[j] != 0) {
			    lower -= *(bits[j]);
			    upper -= *(bits[j]);
			}
			if (sub[cand0]->bits[i] != 0) {
			    lower -= *(sub[cand0]->bits[i]);
			    upper -= *(sub[cand0]->bits[i]);
			}
		    }
		}
		else if (lbound > sub[cand0]->minval[i]) {
		    if (bits[j] == 0)
			activate(j);
		    if (bits[j] != 0)
			lower -= *(bits[j]);
		    if (same)
			upper.copy(lower);
		    else if (bits[j] != 0)
			upper -= *(bits[j]);
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1])
			    upper -= *(sub[cand0]->bits[i-1]);
		    }
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i])
			lower -= *(sub[cand0]->bits[i]);
		}
		else {
		    if (i > 0) {
			if (bits[j] == 0)
			    activate(j);
			sub[cand0]->activate(i-1);
			//tmp = *(bits[j]) | *(sub[cand0]->bits[i-1]);
			//lower -= *tmp;
			if (same) {
			    if (bits[j] != 0)
				lower -= *(bits[j]);
			    if (sub[cand0]->bits[i-1])
				lower -= *(sub[cand0]->bits[i-1]);
			    upper.copy(lower);
			}
			else {
			    if (bits[j] != 0) {
				lower -= *(bits[j]);
				upper -= *(bits[j]);
			    }
			    if (sub[cand0]->bits[i-1]) {
				lower -= *(sub[cand0]->bits[i-1]);
				upper -= *(sub[cand0]->bits[i-1]);
			    }
			}
		    }
		    else if (same) {
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0) {
			    lower -= *(bits[j]);
			    upper.copy(lower);
			}
		    }
		    else {
			if (bits[j] == 0)
			    activate(j);
			if (bits[j] != 0) {
			    lower -= *(bits[j]);
			    upper -= *(bits[j]);
			}
		    }
		}
	    }
	    else {
		if (same)
		    upper.copy(lower);
		activate(cand0-1, cand0+1);
		if (bits[cand0] != 0)
		    lower -= *(bits[cand0]);
		if (bits[cand0-1] != 0)
		    upper -= *(bits[cand0-1]);
	    }
	}
	else {
	    if (same)
		upper.copy(lower);
	    activate(cand0-1, cand0+1);
	    if (bits[cand0] != 0)
		lower -= *(bits[cand0]);
	    if (bits[cand0-1] != 0)
		upper -= *(bits[cand0-1]);
	}
    }
    // the following code works, but it may do twice amount of work to
    // recompute the same lower and upper bounds if they happend to be the
    // same
    //      // generate the bitvector lower
    //      if (cand0 >= cand1) { // no hits
    //  	lower.set(0, nrows);
    //      }
    //      else if (hit0 > 0) { // 0 < hit0, hit0-1 <= hit1
    //  	if (hit1 <= nobs) {
    //  	    j = hit1 - 1;
    //  	    if (hit1 < nobs) {
    //  		if (rbound > maxval[hit1]) {
    //  		    lower.copy(*(bits[hit1]));
    //  		}
    //  		else {
    //  		    if (j < nobs)
    //  			lower.copy(*(bits[j]));
    //  		    else
    //  			lower.set(0, nrows);
    //  		    if (sub.size() == nobs)
    //  			if (sub[hit1]) {
    //  			    i = sub[hit1]->locate(rbound);
    //  			    i -= ((i >= sub[hit1]->nobs) ||
    //  				  (rbound <= sub[hit1]->maxval[i]));
    //  			    if (i < sub[hit1]->nobs)
    //  				lower |= *(sub[hit1]->bits[i]);
    //  			}
    //  		}
    //  	    }
    //  	    else if (j < nobs)
    //  		lower.copy(*(bits[j]));
    //  	    else
    //  		lower.set(0, nrows);

    //  	    j = hit0 - 1;
    //  	    if (lbound < minval[j]) {
    //  		if (j > 0)
    //  		    lower -= *(bits[j-1]);
    //  	    }
    //  	    else if (sub.size() == nobs)
    //  		if (sub[j])
    //  		    if (lbound < sub[j]->bounds[sub[j]->nobs-1]) {
    //  			if (j > 0)
    //  			    lower -= *(bits[j-1]);
    //  			i = sub[j]->locate(lbound);
    //  			i -= (lbound < sub[j]->minval[i]);
    //  			if (i < sub[j]->nobs)
    //  			    lower -= *(sub[j]->bits[i]);
    //  		    }
    //  		    else
    //  			lower -= *(bits[j]);
    //  		else
    //  		    lower -= *(bits[j]);
    //  	    else
    //  		lower -= *(bits[j]);
    //  	}
    //  	else { // hit0 > 0, hit1 > nobs (open range to +infinity)
    //  	    j = hit0 - 1;
    //  	    if (lbound >= maxval[j]) {
    //  		lower.copy(*(bits[j]));
    //  	    }
    //  	    else if (sub.size() == nobs) {
    //  		if (sub[j]) {
    //  		    if (lbound < sub[j]->bounds[sub[j]->nobs-1]) {
    //  			if (j > 0)
    //  			    lower.copy(*(bits[j-1]));
    //  			else
    //  			    lower.set(0, nrows);
    //  			i = sub[j]->locate(lbound);
    //  			i -= (lbound <= sub[j]->minval[i]);
    //  			if (i < sub[j]->nobs)
    //  			    lower |= *(sub[j]->bits[i]);
    //  		    }
    //  		    else
    //  			lower.copy(*(bits[j]));
    //  		}
    //  	    }
    //  	    else
    //  		lower.copy(*(bits[j]));
    //  	    lower.flip();
    //  	}
    //      }
    //      else if (hit1 < nobs) {
    //  	if (hit1 > 0) { // hit0 == 0, but hit1 > 0 (open range to - infinity)
    //  	    j = hit1 - 1;
    //  	    if (rbound > maxval[hit1]) {
    //  		lower.copy(*(bits[hit1]));
    //  	    }
    //  	    else if (sub.size() == nobs) {
    //  		if (sub[hit1]) {
    //  		    if (rbound <= sub[hit1]->bounds[sub[hit1]->nobs-1]) {
    //  			lower.copy(*(bits[j]));
    //  			i = sub[hit1]->locate(rbound);
    //  			i -= ((i==sub[hit1]->nobs) ||
    //  			      (rbound <= sub[hit1]->maxval[i]));
    //  			if (i < sub[hit1]->nobs)
    //  			    lower |= *(sub[hit1]->bits[i]);
    //  		    }
    //  		    else
    //  			lower.copy(*(bits[hit1]));
    //  		}
    //  	    }
    //  	    else
    //  		lower.copy(*(bits[j]));
    //  	}
    //  	else { // hit0 == hit1 == 0
    //  	    lower.set(0, nrows);
    //  	}
    //      }
    //      else if (hit1 == nobs) {
    //  	lower.copy(*(bits[nobs-1]));
    //      }
    //      else { // hit0 == 0, hit1 > nobs
    //  	lower.set(1, nrows);
    //      }
    //      // generate bitvector upper
    //      if (cand0 >= cand1) { // no hits
    //  	lower.set(0, nrows);
    //      }
    //      else if (hit0 == cand0 && hit1 == cand1) {
    //  	upper.copy(lower);
    //      }
    //      else if (cand0+1 == cand1) {
    //  	// the effective range falls in one top level bin
    //  	if (cand1 > nobs) { // the right most bin, not explicitly tracked
    //  	    upper.copy(*(bits[nobs-1]));
    //  	    upper.flip();
    //  	}
    //  	else if (sub.size() != nobs) { // no subdivisionis
    //  	    upper.copy(*(bits[cand0]));
    //  	    if (cand0 > 0)
    //  		upper -= *(bits[cand0-1]);
    //  	}
    //  	else if (sub[cand0] == 0) { // no subdivision cand0
    //  	    upper.copy(*(bits[cand0]));
    //  	    if (cand0 > 0)
    //  		upper -= *(bits[cand0-1]);
    //  	}
    //  	else { // has valid subdivisions
    //  	    if (rbound > sub[cand0]->bounds[sub[cand0]->nobs-1]) {
    //  		upper.copy(*(bits[cand0]));
    //  		if (cand0 > 0)
    //  		    upper -= *(bits[cand0-1]);
    //  	    }
    //  	    else {
    //  		j = sub[cand0]->locate(rbound);
    //  		j -= ((j==sub[cand0]->nobs) ||
    //  		      (rbound <= sub[cand0]->minval[j]));
    //  		if (j < sub[cand0]->nobs)
    //  		    upper.copy(*(sub[cand0]->bits[j]));
    //  		else
    //  		    upper.set(0, nrows);
    //  	    }

    //  	    i = sub[cand0]->locate(lbound);
    //  	    if (i < sub[cand0]->nobs)
    //  		i += (lbound >= sub[cand0]->maxval[i]);
    //  	    if (i > 0)
    //  		upper -= *(sub[cand0]->bits[i-1]);
    //  	}
    //      }
    //      else if (cand0 > 0) { // 0 < cand0 < cand1
    //  	if (cand1 <= nobs) {
    //  	    j = cand1 - 1;
    //  	    if (rbound > maxval[j])
    //  		upper.copy(*(bits[j]));
    //  	    else if (sub.size() == nobs) {
    //  		if (sub[j]) {
    //  		    if (rbound <= sub[j]->bounds[sub[j]->nobs-1]) {
    //  			upper.copy(*(bits[j-1]));
    //  			i = sub[j]->locate(rbound);
    //  			i -= ((i == sub[j]->nobs) ||
    //  			      (rbound <= sub[j]->minval[i]));
    //  			if (i < sub[j]->nobs)
    //  			    upper |= *(sub[j]->bits[i]);
    //  		    }
    //  		    else {
    //  			upper.copy(*(bits[j]));
    //  		    }
    //  		}
    //  		else
    //  		    upper.copy(*(bits[j]));
    //  	    }
    //  	    else
    //  		upper.copy(*(bits[j]));

    //  	    j = cand0 - 1;
    //  	    if (lbound > maxval[cand0])
    //  		upper -= *(bits[cand0]);
    //  	    else if (lbound < minval[cand0])
    //  		upper -= *(bits[j]);
    //  	    else if (sub.size() == nobs)
    //  		if (sub[cand0]) {
    //  		    upper -= *(bits[j]);
    //  		    i = sub[cand0]->locate(lbound);
    //  		    i -= ((i >= sub[cand0]->nobs) ||
    //  			  (lbound <= sub[cand0]->maxval[i]));
    //  		    if (i < sub[cand0]->nobs)
    //  			upper -= *(sub[cand0]->bits[i]);
    //  		}
    //  		else
    //  		    upper -= *(bits[j]);
    //  	    else
    //  		upper -= *(bits[j]);
    //  	}
    //  	else { // 0 < cand0, right hand side is open
    //  	    j = cand0 - 1;
    //  	    if (lbound < minval[cand0])
    //  		upper.copy(*(bits[j]));
    //  	    else if (lbound >= maxval[cand0])
    //  		upper.copy(*(bits[cand0]));
    //  	    else {
    //  		upper.copy(*bits[j]);
    //  		if (sub.size() == nobs)
    //  		    if (sub[cand0]) {
    //  			i = sub[cand0]->locate(lbound);
    //  			i -= ((i >= sub[cand0]->nobs) ||
    //  			      (lbound <= sub[cand0]->maxval[i]));
    //  			if (i < sub[cand0]->nobs)
    //  			    upper |= *(sub[cand0]->bits[i]);
    //  		    }
    //  	    }
    //  	    upper.flip();
    //  	}
    //      }
    //      else if (cand1 <= nobs) { // left hand side of the range is open
    //  	if (cand1 > 0) { // non-empty range
    //  	    j = cand1 - 1;
    //  	    if (rbound > maxval[j])
    //  		upper.copy(*(bits[j]));
    //  	    else if (sub.size() == nobs) {
    //  		if (sub[j])
    //  		    if (rbound <= sub[j]->bounds[sub[j]->nobs-1]) {
    //  			ibis::bitvector* fbs = 0;
    //  			if (rbound > minval[j]) {
    //  			    i = sub[j]->locate(rbound);
    //  			    i -= (rbound <= sub[j]->maxval[i]);
    //  			    fbs = sub[j]->bits[i];
    //  			}
    //  			if (j > 0) {
    //  			    upper.copy(*(bits[j-1]));
    //  			    if (fbs != 0)
    //  				upper |= *fbs;
    //  			}
    //  			else if (fbs != 0)
    //  			    upper |= *fbs;
    //  			else
    //  			    upper.set(0, nrows);
    //  		    }
    //  		    else
    //  			upper.copy(*(bits[j]));
    //  		else
    //  		    upper.copy(*(bits[j]));
    //  	    }
    //  	    else
    //  		upper.copy(*(bits[j]));
    //  	}
    //  	else { // empty range
    //  	    upper.set(0, nrows);
    //  	}
    //      }
    //      else { // cand0=0; cand1>nobs;
    //  	upper.set(1, nrows);
    //      }
} // ibis::ambit::estimate

// ***should implement a more efficient version***
float ibis::ambit::undecidable(const ibis::qContinuousRange& expr,
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
} // ibis::ambit::undecidable

double ibis::ambit::getSum() const {
    double ret;
    bool here = true;
    { // a small test block to evaluate variable here
	const uint32_t nbv = col->elementSize()*col->partition()->nRows();
	if (str != 0)
	    here = (str->bytes()*2 < nbv);
	else if (offsets.size() > nobs)
	    here = (static_cast<uint32_t>(offsets[nobs]*2) < nbv);
    }
    if (here) {
	ret = computeSum();
    }
    else { // indicate sum is not computed
	ibis::util::setNaN(ret);
    }
    return ret;
} // ibis::ambit::getSum

double ibis::ambit::computeSum() const {
    double sum = 0;
    activate(); // need to activate all bitvectors
    if (minval[0] <= maxval[0])
	sum = 0.5*(minval[0] + maxval[0]) * (bits[0] ? bits[0]->cnt() : 0);
    for (uint32_t i = 1; i < nobs; ++ i)
	if (minval[i] <= maxval[i] && bits[i]) {
	    if (bits[i-1]) {
		ibis::bitvector *tmp = *(bits[i]) - *(bits[i-1]);
		sum += 0.5 * (minval[i] + maxval[i]) * tmp->cnt();
		delete tmp;
	    }
	    else {
		sum += 0.5 * (minval[i] + maxval[i]) * bits[i]->cnt();
	    }
	}
    // dealing with the last bins
    ibis::bitvector mask;
    col->getNullMask(mask);
    mask -= *(bits[nobs-1]);
    sum += 0.5*(max1 + min1) * mask.cnt();
    return sum;
} // ibis::ambit::computeSum

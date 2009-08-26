// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
//
// This file contains the implementation of the class ibis::pale.  This class
// defines a two-level bitmap index, where the top level is based on simple
// binning and the fine level is cumulative (segmented).  It uses ibis::range
// as the data structure for the fine level.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "ibin.h"
#include "part.h"
#include "column.h"
#include "resource.h"

#include <string>
//
// generate a ibis::pale from ibis::bin
ibis::pale::pale(const ibis::bin& rhs) {
    if (rhs.col == 0) return;
    if (rhs.nobs <= 1) return; // rhs does not contain an valid index
    col = rhs.col;

    try {
	// decide how many corase and fine bins to use
	uint32_t nbins = rhs.nobs - 2, i, j, k;
	const char* spec = col->indexSpec();
	if (strstr(spec, "nrefine=") != 0) {
	    // number of fine bins per coarse bin
	    const char* tmp = 8+strstr(spec, "nrefine=");
	    i = atoi(tmp);
	    if (i > 1)
		j = (nbins > i ? (nbins+i-1)/i : nbins);
	    else
		j = (nbins > 16 ? 16 : nbins);
	}
	else if (strstr(spec, "ncoarse=") != 0) { // number of coarse bins
	    const char* tmp = 8+strstr(spec, "ncoarse=");
	    j = atoi(tmp);
	    if (j <= 2)
		j = (nbins > 16 ? 16 : nbins);
	}
	else { // default -- 16 coarse bins
	    if (nbins > 31) {
		j = 16;
	    }
	    else {
		j = nbins;
	    }
	}

	std::vector<unsigned> parts(j+1);
	divideBitmaps(rhs.bits, parts);

	// prepare the arrays
	nobs = j + 2;
	nrows = rhs.nrows;
	sub.resize(nobs);
	bits.resize(nobs);
	bounds.resize(nobs);
	maxval.resize(nobs);
	minval.resize(nobs);
	if (nobs < rhs.nobs) {
	    sub.resize(nobs);
	    for (i=0; i<nobs; ++i) sub[i] = 0;
	}
	else {
	    sub.clear();
	}
	LOGGER(ibis::gVerbose > 2)
	    << "ibis::pale::ctor starting to convert " << rhs.nobs
	    << " bitvectors into " << nobs << " coarse bins";

	// copy the first bin, it never has subranges.
	bounds[0] = rhs.bounds[0];
	maxval[0] = rhs.maxval[0];
	minval[0] = rhs.minval[0];
	bits[0] = new ibis::bitvector;
	bits[0]->copy(*(rhs.bits[0]));

	// copy the majority of the bins
	if (nobs < rhs.nobs) { // two levels
	    k = 1;
	    for (i = 1; i < nobs-1; ++i) {
		uint32_t nbi = parts[i] - parts[i-1];
		minval[i] = rhs.minval[k];
		maxval[i] = rhs.maxval[k];
		if (nbi > 1) {
		    sub[i] = new ibis::range;
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
		    sub[i]->bits[0] = new ibis::bitvector;
		    sub[i]->bits[0]->copy(*(rhs.bits[k]));
		    ++k;

		    // copy nbi-2 bins to the subrange
		    for (j = 1; j < nbi - 1; ++j, ++k) {
			sub[i]->bounds[j] = rhs.bounds[k];
			sub[i]->maxval[j] = rhs.maxval[k];
			sub[i]->minval[j] = rhs.minval[k];
			sub[i]->bits[j] = *(sub[i]->bits[j-1]) |
			    *(rhs.bits[k]);
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

		    bits[i] = *(sub[i]->bits.back()) | *(rhs.bits[k]);
		    bits[i]->compress();
		    for (j = 0; j < nbi-1; ++j)
			sub[i]->bits[j]->compress();
		}
		else {
		    sub[i] = 0;
		    bits[i] = new ibis::bitvector;
		    bits[i]->copy(*(rhs.bits[k]));
		}

		bounds[i] = rhs.bounds[k];
		++ k;
	    }

	    // copy the last bin
	    bounds.back() = rhs.bounds.back();
	    maxval.back() = rhs.maxval.back();
	    minval.back() = rhs.minval.back();
	    bits.back() = new ibis::bitvector;
	    bits.back()->copy(*(rhs.bits.back()));
	}
	else { // one level
	    for (i = 1; i < nobs; ++i) {
		bounds[i] = rhs.bounds[i];
		maxval[i] = rhs.maxval[i];
		minval[i] = rhs.minval[i];
		bits[i] = new ibis::bitvector;
		bits[i]->copy(*(rhs.bits[i]));
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

// (de-serialization) reconstruct pale from content of a file
// In addition to the common content for index::bin, the following are
// inserted after minval array:
// offsets_for_next_level (uint32_t[nobs]) -- as the name suggests, these are
// the offsets (in this file) for the next level ibis::pale.
// after the bit vectors of this level are written, the next level ibis::pale
// are written without header.
ibis::pale::pale(const ibis::column* c, ibis::fileManager::storage* st,
		 uint32_t offset) : ibis::bin(c, st, offset) {
    try {
	array_t<int32_t>
	    offs(st,
		 8*((offset+sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t)+7)/8)+
		 sizeof(double)*nobs*3, nobs+1);
#ifdef DEBUG
	if (ibis::gVerbose > 5) {
	    ibis::util::logger lg(4);
	    lg.buffer() << "DEBUG from ibis::pale::pale("
			<< col->partition()->name() << '.' << col->name()
			<< ", " << offset << ") -- offsets of subranges\n";
	    for (uint32_t i=0; i<=nobs; ++i)
		lg.buffer() << "offset[" << i << "] = " << offs[i] << "\n";
	}
#endif

	if (offs[nobs] > offs[0]) {
	    sub.resize(nobs);
	    for (uint32_t i=0; i<nobs; ++i) {
		if (offs[i+1] > offs[i]) {
		    sub[i] = new ibis::range(c, st, offs[i]);
		    //sub[i]->activate();
		}
		else {
		    sub[i] = 0;
		}
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
} // construct ibis::pale from file content

int ibis::pale::write(const char* dt) const {
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
	    col->logWarning("pale::write", "unable to open \"%s\" for write",
			    fnm.c_str());
	    return -2;
	}
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    char header[] = "#IBIS\3\0\0";
    header[5] = (char)ibis::index::PALE;
    header[6] = (char)sizeof(int32_t);
    int ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::column[" << col->partition()->name() << "."
	    << col->name() << "]::pale::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr = write(fdes); // write recursively
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    (void) UnixFlush(fdes); // write to disk
#endif
    (void) UnixClose(fdes);
    return ierr;
} // ibis::pale::write

int ibis::pale::write(int fdes) const {
    const int32_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	ibis::util::logMessage("Warning", "ibis::pale::write call to UnixSeek"
			       "(%d, 0, SEEK_CUR) failed ... %s",
			       fdes, strerror(errno));
	return -4;
    }

    uint32_t i;
    array_t<int32_t> offs(nobs+1);
    // write out bit sequences of this level of the index
    int32_t ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    offs[0] = ((start+sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t)+7)/8)*8;
    ierr = UnixSeek(fdes, offs[0], SEEK_SET);
    if (ierr != offs[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::pale::write(" << fdes << ") failed to seek to "
	    << offs[0];
	UnixSeek(fdes, start, SEEK_SET);
	return -5;
    }

    ierr = UnixWrite(fdes, bounds.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, maxval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, minval.begin(), sizeof(double)*nobs);
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nobs+1), SEEK_CUR);
    for (i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes, start+2*sizeof(uint32_t), SEEK_SET);
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
		    8*((start+sizeof(int32_t)*(nobs+1)+
			2*sizeof(uint32_t)+7)/8)+
		    sizeof(double)*nobs*3, SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
    ierr = UnixSeek(fdes, offs[nobs], SEEK_SET); // move to the end
#if defined(DEBUG)
    if (ibis::gVerbose > 5) {
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- from ibis::pale::write(" << col->name()
		    << ", " << start << ") -- offsets of subranges\n";
	for (i=0; i<=nobs; ++i)
	    lg.buffer() << "offset[" << i << "] = " << offs[i] << "\n";
    }
#endif
    return 0;
} // ibis::pale::write

// read the content of a file
int ibis::pale::read(const char* f) {
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

    if (false == (header[0] == '#' && header[1] == 'I' &&
		  header[2] == 'B' && header[3] == 'I' &&
		  header[4] == 'S' &&
		  header[5] == static_cast<char>(ibis::index::PALE) &&
		  header[6] == static_cast<char>(sizeof(int32_t)) &&
		  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return -3;
    }

    clear();
    uint32_t begin, end;
    fname = ibis::util::strnewdup(fnm.c_str());

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
    end = begin + (nobs+1)*sizeof(int32_t);
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

    begin = end;
    end += (nobs+1)*sizeof(int32_t);
#if defined(DEBUG)
    array_t<int32_t> nextlevel(fdes, begin, end);
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- ibis::pale::read(";
	if (fname)
	    lg.buffer() << fname;
	else
	    lg.buffer() << fdes;
	lg.buffer() << ") got the starting positions of the fine levels\n";
	for (uint32_t i = 0; i <= nobs; ++ i)
	    lg.buffer() << "offset[" << i << "] = " << nextlevel[i] << "\n";
    }
#else
    array_t<int32_t> nextlevel;
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	nextlevel.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	nextlevel.swap(tmp);
    }
#endif
    ibis::fileManager::instance().recordPages(0, end);

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nobs);
#if defined(FASTBIT_READ_BITVECTOR0)
    if (offsets[1] > offsets[0]) {
	array_t<ibis::bitvector::word_t> a0(fdes, offsets[0], offsets[1]);
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
	tmp->sloppySize(nrows);
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
	    lg.buffer() << " Error *** ibis::pale::read(";
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
	    sub[i] = new ibis::range(0);
	    sub[i]->col = col;
	    sub[i]->read(fdes, nextlevel[i], fname);
	}
	else if (nextlevel[i] == nextlevel[i+1]) {
	    sub[i] = 0;
	}
	else {
	    if (ibis::gVerbose > -1) {
		ibis::util::logger lg;
		lg.buffer() << " Error *** ibis::pale::read(";
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
			name(), fnm.c_str());
    return 0;
} // ibis::pale::read

int ibis::pale::read(ibis::fileManager::storage* st) {
    int ierr = ibis::bin::read(st);
    if (ierr < 0) return ierr;

    array_t<int32_t> offs(st,
			  8*((sizeof(int32_t)*(nobs+1)+
			      2*sizeof(uint32_t)+15)/8)+
			  sizeof(double)*(nobs*3+2), nobs+1);
    for (uint32_t i = 0; i < sub.size(); ++i)
	delete sub[i];
    if (offs[0] > offs[nobs]) {
	clear();
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg.buffer() << " Error *** ibis::pale::read(";
	    if (st->unnamed())
		lg.buffer() << static_cast<void*>(st->begin());
	    else
		lg.buffer() << st->filename();
	    lg.buffer() << ") internal error, offset[0](" << offs[0]
			<< ") is expected to be no less than offset["
			<< nobs << "](" << offs[nobs]
			<< "), but it is not";
	}
    }
    else if (offs[0] == offs[nobs]) {
	sub.clear();
    }
    else {
	sub.resize(nobs);
    }
    for (uint32_t i=0; i<nobs; ++i) {
	if (offs[i+1] > offs[i]) {
	    sub[i] = new ibis::range(col, st, offs[i]);
	}
	else if (offs[i] == offs[i+1]) {
	    sub[i] = 0;
	}
	else {
	    if (ibis::gVerbose > -1) {
		ibis::util::logger lg;
		lg.buffer() << " Error *** ibis::pale::read(";
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
} // ibis::pale::read

void ibis::pale::clear() {
    for (std::vector<ibis::range*>::iterator it1 = sub.begin();
	 it1 != sub.end(); ++it1) {
	delete *it1;
    }
    sub.clear();
    ibis::bin::clear();
} // ibis::pale::clear

// fill with zero bits or truncate
void ibis::pale::adjustLength(uint32_t nrows) {
    bin::adjustLength(nrows); // the top level
    if (sub.size() == nobs) {
	for (std::vector<ibis::range*>::iterator it = sub.begin();
	     it != sub.end(); ++it) {
	    if (*it)
		(*it)->adjustLength(nrows);
	}
    }
    else {
	for (std::vector<ibis::range*>::iterator it = sub.begin();
	     it != sub.end(); ++it) {
	    delete *it;
	}
	sub.clear();
    }
} // ibis::pale::adjustLength

void ibis::pale::binBoundaries(std::vector<double>& ret) const {
    ret.clear();
    if (sub.size() == nobs) {
	for (uint32_t i = 0; i < nobs-1; ++i) {
	    if (sub[i]) {
		for (uint32_t j = 0; j < sub[i]->nobs; ++j)
		    ret.push_back(sub[i]->bounds[j]);
	    }
	    ret.push_back(bounds[i]);
	}
    }
    else { // assume no sub intervals
	for (uint32_t i = 0; i < nobs; ++i)
	    ret.push_back(bounds[i]);
    }
} // ibis::pale::binBoundaries

void ibis::pale::binWeights(std::vector<uint32_t>& ret) const {
    activate();
    ret.clear();
    ret.push_back(bits[0] ? bits[0]->cnt() : 0U);
    if (sub.size() == nobs) {
	for (uint32_t i = 1; i < nobs; ++i) {
	    if (sub[i] && bits[i]) {
		sub[i]->activate();
		ret.push_back(sub[i]->bits[i] ? sub[i]->bits[0]->cnt() : 0U);
		for (uint32_t j = 1; j < sub[i]->nobs; ++j)
		    if (sub[i]->bits[j]) {
			if (sub[i]->bits[j-1])
			    ret.push_back(sub[i]->bits[j]->cnt() -
					  sub[i]->bits[j-1]->cnt());
			else
			    ret.push_back(sub[i]->bits[j]->cnt());
		    }
		    else {
			ret.push_back(0);
		    }
		ret.push_back(bits[i]->cnt() -
			      sub[i]->bits.back()->cnt());
	    }
	}
    }
    else {
	for (uint32_t i = 1; i < nobs; ++i)
	    ret.push_back(bits[i] ? bits[i]->cnt() : 0U);
    }
} //ibis::pale::binWeights()

// a simple function to test the speed of the bitvector operations
void ibis::pale::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    uint32_t i, nloops = 1000000000 / nrows;
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;
    col->logMessage("pale::speedTest", "testing the speed of operator -");

    activate();
    for (i = 0; i < nobs-1; ++i) {
	ibis::bitvector* tmp;
	tmp = *(bits[i+1]) - *(bits[i]);
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
		<< timer.CPUTime() / nloops << "\n";
	}
    }
} // ibis::pale::speedTest

// the printing function
void ibis::pale::print(std::ostream& out) const {
    out << "index (binned eqaulity-range code) for "
	<< col->partition()->name() << '.' << col->name()
	<< " contains " << nobs << " coarse bins for "
	<< nrows << " objects \n";
    if (ibis::gVerbose > 4) { // the long format
	uint32_t i, cnt = nrows;
	if (bits[0])
	    out << "0: " << bits[0]->cnt() << "\t(..., " << bounds[0]
		<< ")\t\t\t[" << minval[0] << ", " << maxval[0] << "]\n";
	for (i = 1; i < nobs; ++i) {
	    if (bits[i] == 0) continue;
	    out << i << ": " << bits[i]->cnt() << "\t[" << bounds[i-1]
		<< ", " << bounds[i] << ");\t[" << minval[i] << ", "
		<< maxval[i] << "]\n";
	    if (cnt != bits[i]->size())
		out << "Warning: bits[" << i << "] contains "
		    << bits[i]->size()
		    << " bits, but " << cnt << " are expected\n";
	    if (sub.size() == nobs && sub[i] != 0)
		sub[i]->print(out, bits[i]->cnt(), bounds[i-1], bounds[i]);
	}
    }
    else if (sub.size() == nobs) { // the short format -- with subranges
	out << "right end of bin, bin weight, bit vector size (bytes)\n";
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] == 0) continue;
	    out.precision(12);
	    out << (maxval[i]!=-DBL_MAX?maxval[i]:bounds[i]) << ' '
		<< bits[i]->cnt() << ' ' << bits[i]->bytes() << "\n";
	    if (sub[i])
		sub[i]->print(out, bits[i]->cnt(), bounds[i-1], bounds[i]);
	}
    }
    else { // the short format -- without subranges
	out << "The three columns are (1) center of bin, (2) bin weight, "
	    "and (3) bit vector size (bytes)\n";
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i]) {
		out.precision(12);
		out << 0.5*(minval[i]+maxval[i]) << '\t'
		    << bits[i]->cnt() << '\t' << bits[i]->bytes()
		    << "\n";
	    }
	}
    }
    out << "\n";
} // ibis::pale::print

long ibis::pale::append(const char* dt, const char* df, uint32_t nnew) {
    const uint32_t nold = (strcmp(dt, col->partition()->currentDataDir()) == 0 ?
			   col->partition()->nRows()-nnew : nrows);
    if (nrows != nold) { // don't do anything
	return 0;
    }

    std::string fnm = df;
    indexFileName(df, fnm);
    ibis::pale* bin0=0;
    ibis::fileManager::storage* st0=0;
    long ierr = ibis::fileManager::instance().getFile(fnm.c_str(), &st0);
    if (ierr == 0 && st0 != 0) {
	const char* header = st0->begin();
	if (header[0] == '#' && header[1] == 'I' && header[2] == 'B' &&
	    header[3] == 'I' && header[4] == 'S' &&
	    header[5] == ibis::index::PALE &&
	    header[7] == static_cast<char>(0)) {
	    bin0 = new ibis::pale(col, st0);
	}
	else {
	    if (ibis::gVerbose > 5)
		col->logMessage("pale::append", "file \"%s\" has unexecpted "
				"header -- it will be removed", fnm.c_str());
	    ibis::fileManager::instance().flushFile(fnm.c_str());
	}
    }
    if (bin0 == 0) {
	ibis::bin bin1(col, df, bounds);
	bin0 = new ibis::pale(bin1);
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
} // ibis::pale::append()

long ibis::pale::append(const ibis::pale& tail) {
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
	bin2[i] = new ibis::bitvector;
	bin2[i]->copy(*bits[i]);
	*bin2[i] += *(tail.bits[i]);
    }

    // replace the current content with the new one
    nrows += tail.nrows;
    maxval.swap(max2);
    minval.swap(min2);
    bits.swap(bin2);
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
		col->logWarning("pale::append", "index for the two subrange "
				"%lu must of nil at the same time",
				static_cast<long unsigned>(i));
		delete sub[i];
		sub[i] = 0;
	    }
	}
	if (ierr != 0) return ierr;
    }
    else {
	if (ibis::gVerbose > 0)
	    col->logWarning("pale::append",
			    "removing nonmatching fine ranges.  "
			    "No fine level anymore.");
	for (i = 0; i < sub.size(); ++ i)
	    delete sub[i];
	sub.clear();
    }
    return 0;
} // ibis::pale::append

long ibis::pale::evaluate(const ibis::qContinuousRange& expr,
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
} // ibis::pale::evaluate

// compute the lower and upper bound of the hit vector for the range
// expression
void ibis::pale::estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const {
    if (bits.empty()) {
	lower.set(0, nrows);
	upper.set(1, nrows);
	return;
    }

    // when used to decide the which bins to use on the finer level, the range
    // to be searched to assumed to be [lbound, rbound).
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
	    col->logWarning("pale::estimate", "operators for the range not "
			    "specified");
	    break;
	case ibis::qExpr::OP_LT:
	    rbound = expr.rightBound();
	    hit0 = 0;
	    cand0 = 0;
	    if (bin1 >= nobs) {
		hit1 = nobs;
		cand1 = nobs;
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
		hit1 = nobs;
		cand1 = nobs;
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
		hit0 = nobs;
		cand0 = nobs;
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
		hit0 = nobs;
		cand0 = nobs;
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
	    if (bin1 >= nobs) {
		hit0 = nobs; hit1 = nobs;
		cand0 = nobs; cand1 = nobs + 1;
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
	    hit0 = nobs;
	    cand0 = nobs;
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
		hit1 = nobs;
		cand1 = nobs;
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
		hit1 = nobs;
		cand1 = nobs;
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
	    if (bin1 > bin0) {
		if (bin1 >= nobs) {
		    hit0 = nobs;
		    cand0 = nobs;
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
	    if (expr.rightBound() > expr.leftBound()) {
		lbound = expr.rightBound();
		if (bin1 >= nobs) {
		    hit0 = nobs;
		    cand0 = nobs;
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
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
	    hit0 = nobs;
	    cand0 = nobs;
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
		hit1 = nobs;
		cand1 = nobs;
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
		hit1 = nobs;
		cand1 = nobs;
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
		    hit0 = nobs;
		    cand0 = nobs;
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
	    if (bin1 > bin0) {
		if (bin1 >= nobs) {
		    hit0 = nobs;
		    cand0 = nobs;
		}
		else if (expr.rightBound() > maxval[bin1]) {
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
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
	    hit1 = nobs;
	    cand1 = nobs;
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
	    if (bin1 < bin0) {
		if (expr.rightBound() > maxval[bin1]) {
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
		    hit1 = nobs;
		    cand1 = nobs;
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
		hit0 = nobs;
		cand0 = nobs;
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
		hit0 = nobs;
		cand0 = nobs;
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
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
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
	    hit1 = nobs;
	    cand1 = nobs;
	}
	else if (expr.leftBound() > maxval[bin0]) {
	    hit1 = bin0 + 1;
	    cand1 = bin0 + 1;
	}
	else if (expr.leftBound() < minval[bin0]) {
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
	    hit0 = 0; cand0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    hit0 = 0;
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		rbound = expr.rightBound();
		if (bin1 >= nobs) {
		    hit1 = nobs;
		    cand1 = nobs;
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
	    if (bin1 < bin0) {
		if (expr.rightBound() >= maxval[bin1]) {
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
		hit0 = nobs;
		cand0 = nobs;
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
		hit0 = nobs;
		cand0 = nobs;
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
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
	case ibis::qExpr::OP_EQ:
	    ibis::util::eq2range(expr.rightBound(), lbound, rbound);
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
		hit0 = nobs; hit1 = nobs;
		cand0 = nobs; cand1 = nobs + 1;
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
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
		    hit0 = nobs; hit1 = nobs;
		    cand0 = nobs; cand1 = nobs + 1;
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
	<< "ibis::pale::estimate(" << expr << ") bin number ["
	<< cand0 << ":" << hit0 << ", " << hit1 << ":" << cand1
	<< ") boundaries ["
	<< (minval[cand0]<bounds[cand0] ? minval[cand0] : bounds[cand0]) << ":"
	<< (minval[hit0]<bounds[hit0] ? minval[hit0] : bounds[hit0]) << ", "
	<< (hit1>hit0 ? (maxval[hit1-1] < bounds[hit1-1] ?
			 maxval[hit1-1] : bounds[hit1-1]) :
	    (minval[hit0]<bounds[hit0] ? minval[hit0] : bounds[hit0])) << ":"
	<< (cand1>cand0 ? (maxval[cand1-1] < bounds[cand1-1] ?
			   maxval[cand1-1] : bounds[cand1-1]) :
	    (minval[cand0] < bounds[0] ? minval[cand0] : bounds[0])) << ")";

    uint32_t i;
    bool same = false; // are upper and lower the same ?
    // attempt to generate lower and upper bounds together
    if (cand0 >= cand1) {
	lower.set(0, nrows);
	upper.clear();
    }
    else if (cand0 == hit0 && cand1 == hit1) { // top level only
	sumBins(hit0, hit1, lower);
	upper.copy(lower);
    }
    else if (cand0+1 == cand1) { // all in one coarse bin
	if (cand0 >= nobs) { // unrecorded (coarse) bin
	    lower.set(0, nrows);
	    upper.set(0, nrows);
	}
	else if (sub.size() == nobs && sub[cand0] != 0) { // sub is defined
	    if (bits[cand0] == 0)
		activate(cand0);
	    if (bits[cand0] != 0) { // coarse bin not empty
		// deal with the right side of query range
		i = sub[cand0]->locate(rbound);
		if (i >= sub[cand0]->nobs) { // unrecorded (fine) bin
		    if (rbound > sub[cand0]->max1) {
			same = true;
			lower.copy(*(bits[cand0]));
		    }
		    else if (rbound > sub[cand0]->min1) {
			upper.copy(*(bits[cand0]));

			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1])
			    lower.copy
				(*(sub[cand0]->bits[sub[cand0]->nobs-1]));
			else
			    lower.set(0, nrows);
		    }
		    else {
			same = true;
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits[sub[cand0]->nobs-1])
			    lower.copy
				(*(sub[cand0]->bits[sub[cand0]->nobs-1]));
			else
			    lower.set(0, nrows);
		    }
		}
		else if (rbound > sub[cand0]->maxval[i]) {
		    same = true;
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i])
			lower.copy(*(sub[cand0]->bits[i]));
		    else
			lower.set(0, nrows);
		}
		else if (rbound > sub[cand0]->minval[i]) {
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i])
			upper.copy(*(sub[cand0]->bits[i]));
		    else
			upper.set(0, nrows);
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1])
			    lower.copy(*(sub[cand0]->bits[i-1]));
			else
			    lower.set(0, nrows);
		    }
		    else {
			lower.set(0, nrows);
		    }
		}
		else {
		    same = true;
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1])
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
			if (sub[cand0]->bits.back())
			    upper -= *(sub[cand0]->bits.back());
			lower.set(0, nrows);
		    }
		    else if (same) {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back())
			    lower -= *(sub[cand0]->bits.back());
			upper.copy(lower);
		    }
		    else {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back()) {
			    lower -= *(sub[cand0]->bits.back());
			    upper -= *(sub[cand0]->bits.back());
			}
		    }
		}
		else if (lbound > sub[cand0]->maxval[i]) {
		    if (same) {
			sub[cand0]->activate(i);
			if (sub[cand0]->bits[i])
			    lower -= *(sub[cand0]->bits[i]);
			upper.copy(lower);
		    }
		    else {
			sub[cand0]->activate(i);
			if (sub[cand0]->bits[i]) {
			    lower -= *(sub[cand0]->bits[i]);
			    upper -= *(sub[cand0]->bits[i]);
			}
		    }
		}
		else if (lbound > sub[cand0]->minval[i]) {
		    if (same) {
			upper.copy(lower);
		    }
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i])
			lower -= *(sub[cand0]->bits[i]);
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1])
			    upper -= *(sub[cand0]->bits[i-1]);
		    }
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
	    else { // bits[cand0] == 0
		lower.set(0, nrows);
	    }
	}
	else { // sub is not defined
	    lower.set(0, nrows);

	    if (bits[cand0] == 0)
		activate(cand0);
	    if (bits[cand0])
		upper.copy(*(bits[cand0]));
	    else
		upper.set(0, nrows);
	}
    }
    else if (cand0 == hit0) { // the right bound needs finer level
	// implicitly: hit1+1 == cand1, hit1 < nobs
	sumBins(hit0, hit1, lower);
	if (sub.size() == nobs && sub[hit1] != 0) { // sub is defined
	    if (bits[hit1] == 0)
		activate(hit1);
	    if (bits[hit1]) { // coarse bin not empty
		i = sub[hit1]->locate(rbound);
		if (i >= sub[hit1]->nobs) { // fall in the unrecorded one
		    if (rbound > sub[hit1]->max1) {
			same = true;
			lower |= (*(bits[hit1]));
			upper.copy(lower);
		    }
		    else if (rbound > sub[hit1]->min1) {
			upper.copy(lower);
			sub[hit1]->activate(sub[hit1]->nobs-1);
			if (sub[hit1]->bits.back())
			    lower |= *(sub[hit1]->bits.back());
			upper |= *(bits[hit1]);
		    }
		    else {
			same = true;
			sub[hit1]->activate(sub[hit1]->nobs-1);
			if (sub[hit1]->bits.back())
			    lower |= *(sub[hit1]->bits.back());
			upper.copy(lower);
		    }
		}
		else if (rbound > sub[hit1]->maxval[i]) {
		    same = true;
		    sub[hit1]->activate(i);
		    if (sub[hit1]->bits[i])
			lower |= *(sub[hit1]->bits[i]);
		    upper.copy(lower);
		}
		else if (rbound > sub[hit1]->minval[i]) {
		    upper.copy(lower);
		    if (i > 0) {
			lower |= *(sub[hit1]->bits[i-1]);
			upper |= *(sub[hit1]->bits[i]);
		    }
		    else {
			same = true;
			if (i > 0) {
			    sub[hit1]->activate(i);
			    if (sub[hit1]->bits[i])
				lower |= *(sub[hit1]->bits[i-1]);
			}
			upper.copy(lower);
		    }
		}
		else {
		    same = true;
		    if (i > 0) {
			sub[hit1]->activate(i-1);
			if (sub[hit1]->bits[i-1])
			    lower |= *(sub[hit1]->bits[i-1]);
		    }
		}
	    }
	}
	else {
	    upper.copy(lower);
	    if (bits[hit1] == 0)
		activate(hit1);
	    if (bits[hit1] != 0)
		upper |= (*(bits[hit1]));
	}
    }
    else if (cand1 == hit1) { // the left end needs finer level
	// implcitly: cand0=hit0-1; hit0 > 0
	sumBins(cand0, cand1, upper);

	if (sub.size() == nobs && sub[cand0] != 0) { // sub defined
	    if (bits[cand0] == 0)
		activate(cand0);
	    if (bits[cand0]) { // the coarse bin is not empty
		i = sub[cand0]->locate(lbound);
		if (i >= sub[cand0]->nobs) { // unrecorded sub-range
		    if (lbound > sub[cand0]->max1) {
			upper -= *(bits[cand0]);
			lower.copy(upper);
		    }
		    else if (lbound > sub[cand0]->min1) {
			lower.copy(upper);
			lower -= *(bits[cand0]);

			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back())
			    upper -= *(sub[cand0]->bits.back());
		    }
		    else {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back())
			    upper -= *(sub[cand0]->bits.back());
			lower.copy(upper);
		    }
		}
		else if (lbound > sub[cand0]->maxval[i]) {
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i])
			upper -= *(sub[cand0]->bits[i]);
		    lower.copy(upper);
		}
		else if (lbound > sub[cand0]->minval[i]) {
		    lower.copy(upper);
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1])
			    upper -= *(sub[cand0]->bits[i-1]);
		    }
		    if (sub[cand0]->bits[i] == 0)
			sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i] != 0)
			lower -= *(sub[cand0]->bits[i]);
		}
		else {
		    if (i > 0) {
			if (sub[cand0]->bits[i-1] == 0)
			    sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1] != 0)
			    upper -= *(sub[cand0]->bits[i-1]);
		    }
		    lower.copy(upper);
		}
	    }
	}
	else {
	    lower.copy(upper);
	    activate(cand0-1, cand0+1);
	    if (bits[cand0])
		lower -= *(bits[cand0]);
	    if (cand0 > 0 && bits[cand0-1] != 0)
		upper -= *(bits[cand0-1]);
	}
    }
    else { // both ends need the finer level
	// top level bins (add right, subtract left)
	sumBins(cand0, hit1, lower);

	// first deal with the right end of the range
	if (hit1 >= nobs) { // right end is open
	    same = true;
	}
	else if (sub.size() == nobs && sub[hit1] != 0) { // sub defined
	    if (bits[hit1] == 0)
		activate(hit1);
	    if (bits[hit1]) { // the coarse bin is not empty
		i = sub[hit1]->locate(rbound);
		if (i >= sub[hit1]->nobs) { // fall in the unrecorded one
		    if (rbound > sub[hit1]->max1) {
			same = true;
			lower |= (*(bits[hit1]));
		    }
		    else if (rbound > sub[hit1]->min1) {
			upper.copy(lower);
			upper |= (*(bits[hit1]));

			sub[hit1]->activate(sub[hit1]->nobs-1);
			if (sub[hit1]->bits.back())
			    lower |= *(sub[hit1]->bits.back());
		    }
		    else {
			same = true;
			sub[hit1]->activate(sub[hit1]->nobs-1);
			if (sub[hit1]->bits.back())
			    lower |= *(sub[hit1]->bits.back());
		    }
		}
		else if (rbound > sub[hit1]->maxval[i]) {
		    same = true;
		    sub[hit1]->activate(i);
		    if (sub[hit1]->bits[i])
			lower |= *(sub[hit1]->bits[i]);
		}
		else if (rbound > sub[hit1]->minval[i]) {
		    upper.copy(lower);
		    sub[hit1]->activate((i>0?i-1:0), i+1);
		    if (i > 0 && sub[hit1]->bits[i-1] != 0)
			lower |= *(sub[hit1]->bits[i-1]);
		    if (sub[hit1]->bits[i] != 0)
			upper |= *(sub[hit1]->bits[i]);
		}
		else {
		    same = true;
		    if (i > 0) {
			sub[hit1]->activate(i-1);
			if (sub[hit1]->bits[i-1])
			    lower |= *(sub[hit1]->bits[i-1]);
		    }
		}
	    }
	}
	else {
	    upper.copy(lower);
	    if (bits[hit1] == 0)
		activate(hit1);
	    if (bits[hit1] != 0)
		upper |= *(bits[hit1]);
	}

	// deal with the lower (left) boundary
	if (cand0 == 0) { // sub[0] never defined
	    if (same)
		upper.copy(lower);
	    if (bits[0])
		lower -= *(bits[0]);
	}
	else if (sub.size() == nobs && sub[cand0] != 0) { // sub defined
	    if (bits[cand0] == 0)
		activate(cand0);
	    if (bits[cand0]) { // the particular subrange is not empty
		i = sub[cand0]->locate(lbound);
		if (i >= sub[cand0]->nobs) { // unrecorded sub-range
		    if (lbound > sub[cand0]->max1) {
			if (same) {
			    lower -= *(bits[cand0]);
			    upper.copy(lower);
			}
			else {
			    lower -= *(bits[cand0]);
			    upper -= *(bits[cand0]);
			}
		    }
		    else if (lbound > sub[cand0]->min1) {
			if (same)
			    upper.copy(lower);
			lower -= *(bits[cand0]);

			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back())
			    upper -= *(sub[cand0]->bits.back());
		    }
		    else if (same) {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back())
			    lower -= *(sub[cand0]->bits.back());
			upper.copy(lower);
		    }
		    else {
			sub[cand0]->activate(sub[cand0]->nobs-1);
			if (sub[cand0]->bits.back()) {
			    lower -= *(sub[cand0]->bits.back());
			    upper -= *(sub[cand0]->bits.back());
			}
		    }
		}
		else if (lbound > sub[cand0]->maxval[i]) {
		    sub[cand0]->activate(i);
		    if (sub[cand0]->bits[i]) {
			lower -= *(sub[cand0]->bits[i]);
			if (same) {
			    upper.copy(lower);
			}
			else {
			    upper -= *(sub[cand0]->bits[i]);
			}
		    }
		}
		else if (lbound > sub[cand0]->minval[i]) {
		    if (same)
			upper.copy(lower);
		    sub[cand0]->activate((i>0?i-1:0), i+1);
		    if (i > 0 && sub[cand0]->bits[i-1] != 0)
			upper -= *(sub[cand0]->bits[i-1]);
		    if (sub[cand0]->bits[i] != 0)
			lower -= *(sub[cand0]->bits[i]);
		}
		else {
		    if (i > 0) {
			sub[cand0]->activate(i-1);
			if (sub[cand0]->bits[i-1]) {
			    lower -= *(sub[cand0]->bits[i-1]);
			    if (same) {
				upper.copy(lower);
			    }
			    else {
				upper -= *(sub[cand0]->bits[i-1]);
			    }
			}
		    }
		    else if (same) {
			upper.copy(lower);
		    }
		}
	    }
	}
	else {
	    if (same)
		upper.copy(lower);

	    if (bits[cand0] == 0)
		activate(cand0);
	    if (bits[cand0] != 0)
		lower -= *(bits[cand0]);
	}
    }
} // ibis::pale::estimate

// ***should implement a more efficient version***
float ibis::pale::undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const {
    float ret = 0;
    ibis::bitvector tmp;
    estimate(expr, tmp, iffy);
    if (tmp.size() == iffy.size())
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
} // ibis::pale::undecidable

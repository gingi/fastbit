// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2008 the Regents of the University of California
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
#include <sstream> // std::ostringstream

////////////////////////////////////////////////////////////////////////
// functions from ibis::range
//
ibis::range::range(const ibis::column* c, const char* f)
    : ibis::bin(c, f), max1(-DBL_MAX), min1(DBL_MAX) {
    if (c == 0) return; // nothing else can be done
    if (nobs <= 2) {
	clear();
	throw "ibis::range -- binning produced two or less bins, need more";
    }

    try {
	// convert from bin to range
	-- nobs;
	max1 = maxval[nobs];
	min1 = minval[nobs];
	bounds.resize(nobs);
	maxval.resize(nobs);
	minval.resize(nobs);
	for (uint32_t i = 1; i < nobs; ++i)
	    *(bits[i]) |= *(bits[i-1]);
	delete bits[nobs];
	bits.resize(nobs);

	// make sure all bit vectors are the same size
	for (uint32_t i = 0; i < nobs; ++i)
	    bits[i]->compress();
	optionalUnpack(bits, col->indexSpec());

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg(4);
	    print(lg.buffer());
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning - ibis::column[" << col->name()
	    << "]::range::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // constructor

// copy a ibis::bin into a ibis::range
ibis::range::range(const ibis::bin& rhs) : max1(-DBL_MAX), min1(DBL_MAX) {
    if (rhs.col == 0) return;
    if (rhs.nobs <= 1) return; // rhs does not contain a valid index
    if (rhs.nrows == 0) return;

    try {
	//rhs.activate(); // make all bitvectors available
	col = rhs.col;
	nobs = rhs.nobs-1;
	nrows = rhs.nrows;
	bits.resize(nobs);
	bounds.resize(nobs);
	maxval.resize(nobs);
	minval.resize(nobs);
	bounds[0] = rhs.bounds[0];
	maxval[0] = rhs.maxval[0];
	minval[0] = rhs.minval[0];
	bits[0] = new ibis::bitvector;
	if (bits[0])
	    bits[0]->copy(*(rhs.bits[0]));
	else
	    bits[0]->set(0, nrows);
	for (uint32_t i = 1; i < nobs; ++i) {
	    bounds[i] = rhs.bounds[i];
	    maxval[i] = rhs.maxval[i];
	    minval[i] = rhs.minval[i];
	    bits[i] = *(bits[i-1]) | *(rhs.bits[i]);
	}
	max1 = rhs.maxval.back();
	min1 = rhs.minval.back();

	// make sure all bit vectors are the same size
	for (uint32_t i = 0; i < nobs; ++i)
	    bits[i]->compress();
	optionalUnpack(bits, col->indexSpec());

	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg(4);
	    print(lg.buffer());
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning - ibis::column[" << col->name()
	    << "]::range::ctor encountered an exception, cleaning up ...";
	clear();
	throw;
    }
} // copy constructor (from bin)

// the file format are the same as ibis::bin but there is a difference in
// semantics.  The largest bounds value (bounds[nobs-1]) in this case is
// not DBL_MAX, those values above bounds[nobs-1] are not explicitly
// recorded in a bit vector.  Instead it is assumed that the compliment of
// bits[nobs-1] represent the bin.
ibis::range::range(const ibis::column* c, ibis::fileManager::storage* st,
		   uint32_t offset)
    : ibis::bin(c, st, offset), max1(*(minval.end())),
      min1(*(1+minval.end())) {
    if ((offset > 8 || ! st->isFileMap()) && ibis::gVerbose > 8 &&
	static_cast<ibis::index::INDEX_TYPE>(*(st->begin()+5)) == RANGE) {
	ibis::util::logger lg(8);
	print(lg.buffer());
    }
}

int ibis::range::read(const char* f) {
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
		  header[6] == static_cast<char>(sizeof(int32_t)) &&
		  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return -3;
    }

    uint32_t begin, end;
    clear(); // time to wipe out the existing content
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
#if defined(HAS_FILE_MAP)
    trymmap = (nobs > ibis::fileManager::pageSize());
#endif
    begin = 8 + 2 * sizeof(uint32_t);
    end = 8 + 2 * sizeof(uint32_t) + (nobs+1) * sizeof(int32_t);
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }

    // read bounds
    begin = 8 * ((15 + sizeof(uint32_t)*2 + sizeof(int32_t)*(nobs+1)) / 8);
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
    if (ierr != end) {
	UnixClose(fdes);
	remove(fnm.c_str());
	LOGGER(ibis::gVerbose >= 1)
	    << "ibis::range::read(" << fnm << ") failed to seek to " << end;
	return -6;
    }

    ierr = UnixRead(fdes, static_cast<void*>(&max1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -7;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&min1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -8;
    }
    end += sizeof(double) * 2;
    ibis::fileManager::instance().recordPages(0, end);

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i] = 0;

#if defined(ALWAY_READ_BITVECTOR0)
    if (offsets[1] > offsets[0]) { // read the first bitvector
	array_t<ibis::bitvector::word_t> a0(fdes, offsets[0], offsets[1]);
	ibis::bitvector* tmp = new ibis::bitvector(a0);
	bits[0] = tmp;
#if defined(WAH_CHECK_SIZE)
	if (tmp->size() != nrows)
	    col->logWarning("readIndex", "the size (%lu) of 1st "
			    "bitvector (from \"%s\") differs "
			    "from nRows (%lu)",
			    static_cast<long unsigned>(tmp->size()),
			    fnm.c_str(), static_cast<long unsigned>(nrows));
#else
	tmp->setSize(nrows);
#endif
    }
    else {
	bits[0] = new ibis::bitvector;
	bits[0]->set(0, nrows);
    }
#endif
    (void) UnixClose(fdes);
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fnm.c_str());
    return 0;
} // ibis::range::read

// read from a file starting at an arbitrary position offset
int ibis::range::read(int fdes, uint32_t start, const char *fn) {
    if (fdes < 0) return -1;
    if (start != static_cast<uint32_t>(UnixSeek(fdes, start, SEEK_SET)))
	return -2;

    uint32_t begin, end;
    clear(); // wipe out the existing content
    if (fn != 0 && *fn != 0)
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
#if defined(HAS_FILE_MAP)
    trymmap = (nobs > ibis::fileManager::pageSize()) && (fname != 0);
#endif
    begin = start + 2 * sizeof(uint32_t);
    end = start + 2 * sizeof(uint32_t) + (nobs+1)*sizeof(int32_t);
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }

    // read bounds
    begin = 8 * ((start + sizeof(int32_t)*(nobs+1) +
		  sizeof(uint32_t)*2 + 7) / 8);
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
    if (ierr != end) {
	UnixClose(fdes);
	clear();
	return -4;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&max1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -5;
    }
    ierr = UnixRead(fdes, static_cast<void*>(&min1), sizeof(double));
    if (ierr < static_cast<int>(sizeof(double))) {
	UnixClose(fdes);
	clear();
	return -6;
    }
    end += sizeof(double) * 2;
    ibis::fileManager::instance().recordPages(0, end);

    // initialize bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nobs);
    if (fname == 0) { // read all bitvectors
	for (uint32_t i = 0; i < nobs; ++i) {
	    if (offsets[i+1] > offsets[i]) {
		array_t<ibis::bitvector::word_t>
		    a0(fdes, offsets[i], offsets[i+1]);
		ibis::bitvector* tmp = new ibis::bitvector(a0);
		bits[i] = tmp;
#if defined(WAH_CHECK_SIZE)
		if (tmp->size() != nrows)
		    col->logWarning("readIndex", "the length (%lu) of "
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
#if defined(ALWAY_READ_BITVECTOR0)
    else if (offsets[1] > offsets[0]) {
	array_t<ibis::bitvector::word_t> a0(fdes, offsets[0], offsets[1]);
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
    return 0;
} // ibis::range::read

int ibis::range::read(ibis::fileManager::storage* st) {
    int ierr = ibis::bin::read(st);
    max1 = *(minval.end());
    min1 = *(1+minval.end());
    return ierr;
} // ibis::range::read

int ibis::range::write(const char* dt) const {
    if (nobs <= 0) return -1;

    array_t<int32_t> offs(nobs+1);
    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;
    if (fname != 0 && str != 0)
	activate();

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	col->logWarning("range::write", "unable to open \"%s\" for write",
			fnm.c_str());
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    char header[] = "#IBIS\1\0\0";
    header[5] = (char)ibis::index::RANGE;
    header[6] = (char)sizeof(int32_t);
    int ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::column[" << col->partition()->name() << "."
	    << col->name() << "]::range::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    ierr = UnixSeek(fdes,
		    ((sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t)+15)/8)*8,
		    SEEK_SET);
    ierr = UnixWrite(fdes, bounds.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, maxval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, minval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, &max1, sizeof(double));
    ierr = UnixWrite(fdes, &min1, sizeof(double));
    for (uint32_t i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes, 8+sizeof(uint32_t)*2, SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
#if _POSIX_FSYNC+0 > 0
    (void) fsync(fdes); // write to disk
#endif
    (void) UnixClose(fdes);

    if (ibis::gVerbose > 5)
	col->logMessage("range::write", "wrote to file %s (%lu bitmap(s) "
			"for %lu object(s), file size %lu", fnm.c_str(),
			static_cast<long unsigned>(nobs),
			static_cast<long unsigned>(nrows),
			static_cast<long unsigned>(offs.back()));
    return 0;
} // ibis::range::write

// write to the file already opened by the caller
int ibis::range::write(int fdes) const {
    if (nobs <= 0) return -1;
    if (fname != 0 || str != 0)
	activate();

    const int32_t start = UnixSeek(fdes, 0, SEEK_CUR);
    if (start < 8) {
	ibis::util::logMessage("Warning", "ibis::range::write call to UnixSeek"
			       "(%d, 0, SEEK_CUR) failed ... %s", fdes,
			       strerror(errno));
	return -2;
    }

    array_t<int32_t> offs(nobs+1);
    int32_t ierr = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr = UnixWrite(fdes, &nobs, sizeof(uint32_t));
    ierr = UnixSeek(fdes,
		    ((start+sizeof(int32_t)*(nobs+1)+
		      sizeof(uint32_t)*2+7)/8)*8,
		    SEEK_SET);
    ierr = UnixWrite(fdes, bounds.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, maxval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, minval.begin(), sizeof(double)*nobs);
    ierr = UnixWrite(fdes, &max1, sizeof(double));
    ierr = UnixWrite(fdes, &min1, sizeof(double));
    for (uint32_t i = 0; i < nobs; ++i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	bits[i]->write(fdes);
    }
    offs[nobs] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr = UnixSeek(fdes, start+sizeof(uint32_t)*2, SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nobs+1));
    // place the file pointer at the end
    ierr = UnixSeek(fdes, offs[nobs], SEEK_SET);
    return 0;
} // ibis::range::write

// generate a new range index -- the caller may specify an array of doubles
// as the boundaries of the bins
void ibis::range::construct(const char* f, const array_t<double>& bd) {
    uint32_t i, j;
    nrows = col->partition()->nRows();
    // determine the number of bins to use
    uint32_t nbins = 10;
    if (bd.size() < 2) {
	const char* spec = col->indexSpec();
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

    // allocate space for min and max values and initialize them to extreme
    // values
    double lbd = col->lowerBound();
    double diff = col->upperBound() - lbd;
    nobs = nbins + 1;
    bits.resize(nobs);
    bounds.resize(nobs);
    maxval.resize(nobs);
    minval.resize(nobs);
    for (i = 0; i < nobs; ++i) {
	if (nobs == bd.size()) {
	    bounds[i] = bd[i];
	}
	else {
	    bounds[i] = lbd + diff * i / nbins;
	    if (col->type() == ibis::TEXT ||
		col->type() == ibis::UINT ||
		col->type() == ibis::INT) {
		// make sure bin boundaries are integers
		bounds[i] = 0.5*floor(2.0*bounds[i]+0.5);
	    }
	}
	maxval[i] = -DBL_MAX;
	minval[i] = DBL_MAX;
	bits[i] = new ibis::bitvector;
    }
    max1 = -DBL_MAX;
    min1 = DBL_MAX;

    std::string fnm; // name of the data file / index file
    if (f == 0) {
	fnm = col->partition()->currentDataDir();
	fnm += DIRSEP;
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
		    fnm += DIRSEP;
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
	    col->logWarning("range::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
	    }
	    else {
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	    while (j < nobs)
		bits[j++]->setBit(i, 1);
	}
	break;}
    case ibis::INT: {// signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0)
	    col->logWarning("range::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
	    }
	    else {
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	    while (j < nobs)
		bits[j++]->setBit(i, 1);
	}
	break;}
    case ibis::FLOAT: {// (4-byte) floating-point values
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0)
	    col->logWarning("range::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
	    }
	    else {
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	    while (j < nobs)
		bits[j++]->setBit(i, 1);
	}
	break;}
    case ibis::DOUBLE: {// (8-byte) floating-point values
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() <= 0)
	    col->logWarning("range::construct", "unable to read %s",
			    fnm.c_str());

	nrows = val.size();
	for (i = 0; i < nrows; ++i) {
	    j = locate(val[i]);
	    if (j < nobs) {
		if (maxval[j] < val[i])
		    maxval[j] = val[i];
		if (minval[j] > val[i])
		    minval[j] = val[i];
	    }
	    else {
		if (max1 < val[i])
		    max1 = val[i];
		if (min1 > val[i])
		    min1 = val[i];
	    }
	    while (j < nobs)
		bits[j++]->setBit(i, 1);
	}
	break;}
    case ibis::CATEGORY: // no need for a separate index
	col->logWarning("range::construct", "no need for an index");
	return;
    default:
	col->logWarning("range::construct", "unable to create index for "
			"this type of column");
	return;
    }

    // make sure all bit vectors are the same size
    if (mask.size() > nrows)
	nrows = mask.size();
    for (i = 0; i < nobs; ++i)
	if (bits[i]->size() < nrows)
	    bits[i]->setBit(nrows-1, 0);
} // ibis::range::construct

void ibis::range::binBoundaries(std::vector<double>& ret) const {
    ret.reserve(nobs+2);
    for (uint32_t i = 0; i < nobs; ++ i)
	ret.push_back(bounds[i]);
    if (max1 >= min1)
	ret.push_back(ibis::util::compactValue(max1, DBL_MAX));
} // ibis::range::binBoundaries

void ibis::range::binWeights(std::vector<uint32_t>& ret) const {
    activate();
    ret.reserve(nobs+2);
    ret.push_back(nrows);
    for (uint32_t i=1; i < nobs; ++i)
	ret.push_back(bits[i]->cnt() - bits[i-1]->cnt());
    if (max1 >= min1) {
	ibis::bitvector tmp;
	col->getNullMask(tmp);
	tmp -= *(bits.back());
	ret.push_back(tmp.cnt());
    }
} //ibis::range::binWeights

// a simple function to test the speed of the bitvector operations
void ibis::range::speedTest(std::ostream& out) const {
    if (nrows == 0) return;
    activate();
    uint32_t i, nloops = 1000000000 / nrows;
    if (nloops < 2) nloops = 2;
    ibis::horometer timer;
    col->logMessage("range::speedTest", "testing the speed of operator -");

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
		<< timer.CPUTime() / nloops << std::endl;
	}
    }
} // ibis::range::speedTest

// the printing function
void ibis::range::print(std::ostream& out) const {
    out << "index (range encoded) for ibis::column " << col->name()
	<< " contains " << nobs << " bitvectors for "
	<< nrows << " objects \n";
    if (ibis::gVerbose > 4) { // the long format
	uint32_t i, cnt = nrows;
	if (bits[0])
	    out << "0: " << bits[0]->cnt() << "\t(..., " << bounds[0]
		<< ")\n";
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
	}
	if (bits[nobs-1])
	    out << nobs << ": " << cnt << "\t(..., ...);\t"
		<< cnt-bits[nobs-1]->cnt() << "\t[" << bounds[nobs-1]
		<< ", ...)\t[" << min1 << ", " << max1 << "]\n";
    }
    else { // the short format
	out << "The three columns are (1) center of bin, (2) bin weight, "
	    << "and (3) bit vector size (bytes)\n";
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] && bits[i]->cnt()) {
		out.precision(12);
		out << 0.5*(maxval[i]+minval[i]) << '\t'
		    << bits[i]->cnt() << '\t' << bits[i]->bytes() << "\n";
	    }
	}
    }
    out << std::endl;
} // ibis::range::print

// the given range is limited to the range of [lbound, rbound) with a maximum
// count of tot
void ibis::range::print(std::ostream& out, const uint32_t tot,
			const double& lbound, const double& rbound) const {
    if (ibis::gVerbose > 4) { // long format
	uint32_t i, cnt = nrows;
	out << "\trange [" << lbound << ", " << rbound
	    << ") is subdivided into "
	    << nobs+1 << " overlapping ranges\n";
	if (bits[0])
	    out << "\t" << bits[0]->cnt() << "\t[" << lbound << ", "
		<< bounds[0] << ")\t\t\t[" << minval[0] << ", "
		<< maxval[0] << "]\n";
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
	out << "\t" << tot << "\t[" << lbound << ", " << rbound << ");\t"
	    << tot - bits[nobs-1]->cnt() << "\t["
	    << bounds[nobs-1] << ", " << rbound << ")\t[" << min1 << ", "
	    << max1 << "]" << std::endl;
    }
    else { // short format
	for (uint32_t i=0; i<nobs; ++i) {
	    if (bits[i] && bits[i]->cnt()) {
		out.precision(12);
		out << 0.5*(maxval[i] + minval[i]) << '\t'
		    << bits[i]->cnt() << '\t' << bits[i]->bytes()
		    << std::endl;
	    }
	}
    }
} // ibis::range::print

long ibis::range::append(const char* dt, const char* df, uint32_t nnew) {
    std::string fnm;
    indexFileName(df, fnm);

    ibis::range* bin0=0;
    ibis::fileManager::storage* st0=0;
    long ierr = ibis::fileManager::instance().getFile(fnm.c_str(), &st0);
    if (ierr == 0 && st0 != 0) {
	const char* header = st0->begin();
	if (header[0] == '#' && header[1] == 'I' && header[2] == 'B' &&
	    header[3] == 'I' && header[4] == 'S' &&
	    header[5] == ibis::index::RANGE &&
	    header[7] == static_cast<char>(0)) {
	    bin0 = new ibis::range(col, st0);
	}
	else {
	    if (ibis::gVerbose > 5)
		col->logMessage("range::append", "file \"%s\" has unexecpted "
				"header -- it will be removed", fnm.c_str());
	    ibis::fileManager::instance().flushFile(fnm.c_str());
	    remove(fnm.c_str());
	}
    }
    if (bin0 == 0) {
	ibis::bin bin1(col, df, bounds);
	bin0 = new ibis::range(bin1);
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
	col->logWarning("range::append", "failed to generate index with "
			"data from %s", df);
	return -6;
    }
} // ibis::range::append

long ibis::range::append(const ibis::range& tail) {
    uint32_t i;
    if (tail.col != col) return -1;
    if (tail.nobs != nobs) return -2;
    if (tail.bits.empty()) return -3;
    if (tail.nrows != tail.bits[1]->size()) return -4;
    for (i = 0; i < nobs; ++i)
	if (tail.bounds[i] != bounds[i]) return -5;

    // generate the new maxval and bits
    array_t<double> max2, min2;
    std::vector<ibis::bitvector*> bin2;
    max2.resize(nobs);
    min2.resize(nobs);
    bin2.resize(nobs);
    activate();
    tail.activate();
    for (i = 0; i < nobs; ++i) {
	if (tail.maxval[i] >= maxval[i])
	    max2[i] = tail.maxval[i];
	else
	    max2[i] = maxval[i];
	if (tail.minval[i] <= minval[i])
	    min2[i] = tail.minval[i];
	else
	    min2[i] = minval[i];
	bin2[i] = new ibis::bitvector;
	bin2[i]->copy(*bits[i]);
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
    return 0;
} // ibis::range::append()

void ibis::range::locate(const ibis::qContinuousRange& expr, uint32_t& cand0,
			 uint32_t& cand1) const {
    // bins in the range of [cand0, cand1) are candidates
    cand0=0;
    cand1=0;
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
	    col->logWarning("range::locate", "operators for the range not "
			    "specified");
	    return;
	case ibis::qExpr::OP_LT:
	    cand0 = 0;
	    if (bin1 >= nobs) {
		if (expr.rightBound() >  min1)
		    cand1 = nobs + 1;
		else
		    cand1 = nobs;
	    }
	    else if (expr.rightBound() <= minval[bin1]) {
		cand1 = bin1;
	    }
	    else {
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    cand0 = 0;
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= min1)
		    cand1 = nobs + 1;
		else
		    cand1 = nobs;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		cand1 = bin1;
	    }
	    else {
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    cand1 = nobs + 1;
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1)
		    cand0 = nobs + 1;
		else
		    cand0 = nobs;
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		cand0 = bin1 + 1;
	    }
	    else {
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    cand1 = nobs + 1;
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1)
		    cand0 = nobs + 1;
		else
		    cand0 = nobs;
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		cand0 = bin1 + 1;
	    }
	    else {
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (bin1 >= nobs) {
		if (expr.rightBound() <= max1 &&
		    expr.rightBound() >= min1) {
		    cand0 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else if (expr.rightBound() <= maxval[bin1] &&
		     expr.rightBound() >= minval[bin1]) {
		cand0 = bin1;
		cand1 = bin1 + 1;
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_UNDEFINED
    case ibis::qExpr::OP_LT:
	if (bin0 >= nobs) {
	    if (expr.leftBound() >= max1)
		cand0 = nobs + 1;
	    else
		cand0 = nobs;
	}
	else if (expr.leftBound() >= maxval[bin0]) {
	    cand0 = bin0 + 1;
	}
	else {
	    cand0 = bin0;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    cand1 = nobs + 1;
	    break;
	case ibis::qExpr::OP_LT:
	    if (bin1 >= nobs) {
		if (expr.rightBound() > min1)
		    cand1 = nobs + 1;
		else
		    cand1 = nobs;
	    }
	    else if (expr.rightBound() <= minval[bin1]) {
		cand1 = bin1;
	    }
	    else {
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= min1)
		    cand1 = nobs + 1;
		else
		    cand1 = nobs;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		cand1 = bin1;
	    }
	    else {
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    cand1 = nobs + 1;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= max1)
			cand0 = nobs + 1;
		    else
			cand0 = nobs;
		}
		else if (expr.rightBound() >= maxval[bin1]) {
		    cand0 = bin1 + 1;
		}
		else {
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    cand1 = nobs + 1;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1)
			cand0 = nobs + 1;
		    else
			cand0 = nobs;
		}
		else if (expr.rightBound() > maxval[bin1]) {
		    cand0 = bin1 + 1;
		}
		else {
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LT
    case ibis::qExpr::OP_LE:
	if (bin0 >= nobs) {
	    if (expr.leftBound() > max1)
		cand0 = nobs + 1;
	    else
		cand0 = nobs;
	}
	else if (expr.leftBound() > maxval[bin0]) {
	    cand0 = bin0 + 1;
	}
	else {
	    cand0 = bin0;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    cand1 = nobs + 1;
	    break;
	case ibis::qExpr::OP_LT:
	    if (bin1 >= nobs) {
		if (expr.rightBound() > min1)
		    cand1 = nobs + 1;
		else
		    cand1 = nobs;
	    }
	    else if (expr.rightBound() <= minval[bin1]) {
		cand1 = bin1;
	    }
	    else {
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= min1)
		    cand1 = nobs + 1;
		else
		    cand1 = nobs;
	    }
	    else if (expr.rightBound() < minval[bin1]) {
		cand1 = bin1;
	    }
	    else {
		cand1 = bin1 + 1;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    cand1 = nobs + 1;
	    if (expr.rightBound() >= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= max1) {
			cand0 = nobs + 1;
		    }
		    else {
			cand0 = nobs;
		    }
		}
		else if (expr.rightBound() >= maxval[bin1]) {
		    cand0 = bin1 + 1;
		}
		else {
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    cand1 = nobs + 1;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > max1)
			cand0 = nobs + 1;
		    else
			cand0 = nobs;
		}
		else if (expr.rightBound() > maxval[bin1]) {
		    cand0 = bin1 + 1;
		}
		else {
		    cand0 = bin1;
		}
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else  {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LE
    case ibis::qExpr::OP_GT:
	if (bin0 >= nobs) {
	    if (expr.leftBound() > min1)
		cand1 = nobs + 1;
	    else
		cand1 = nobs;
	}
	else if (expr.leftBound() <= minval[bin0]) {
	    cand1 = bin0;
	}
	else {
	    cand1 = bin0 + 1;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    cand0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > min1)
			cand1 = nobs + 1;
		    else
			cand1 = nobs;
		}
		else if (expr.rightBound() <= minval[bin1]) {
		    cand1 = bin1;
		}
		else {
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= min1)
			cand1 = nobs + 1;
		    else
			cand1 = nobs;
		}
		else if (expr.rightBound() < minval[bin1]) {
		    cand1 = bin1;
		}
		else {
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (bin1 >= nobs) {
		if (expr.rightBound() >= max1)
		    cand0 = nobs + 1;
		else
		    cand0 = nobs;
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		cand0 = bin1 + 1;
	    }
	    else {
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1)
		    cand0 = nobs + 1;
		else
		    cand0 = nobs;
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		cand0 = bin1 + 1;
	    }
	    else {
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else  {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GT
    case ibis::qExpr::OP_GE:
	if (bin0 >= nobs) {
	    if (expr.leftBound() > min1)
		cand1 = nobs + 1;
	    else
		cand1 = nobs;
	}
	else if (expr.leftBound() < minval[bin0]) {
	    cand1 = bin0;
	}
	else {
	    cand1 = bin0 + 1;
	}
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    cand0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
	    cand0 = 0;
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() > min1)
			cand1 = nobs + 1;
		    else
			cand1 = nobs;
		}
		else if (expr.rightBound() <= minval[bin1]) {
		    cand1 = bin1;
		}
		else {
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    cand0 = 0;
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= min1)
			cand1 = nobs + 1;
		    else
			cand1 = nobs;
		}
		else if (expr.rightBound() < minval[bin1]) {
		    cand1 = bin1;
		}
		else {
		    cand1 = bin1 + 1;
		}
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1) {
		    cand0 = nobs + 1;
		}
		else {
		    cand0 = nobs;
		}
	    }
	    else if (expr.rightBound() >= maxval[bin1]) {
		cand0 = bin1 + 1;
	    }
	    else {
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (bin1 >= nobs) {
		if (expr.rightBound() > max1)
		    cand0 = nobs + 1;
		else
		    cand0 = nobs;
	    }
	    else if (expr.rightBound() > maxval[bin1]) {
		cand0 = bin1 + 1;
	    }
	    else {
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GE
    case ibis::qExpr::OP_EQ:
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    if (bin0 >= nobs) {
		if (expr.leftBound() <= max1 &&
		    expr.leftBound() >= min1) {
		    cand0 = nobs;
		    cand1 = nobs + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else if (expr.leftBound() <= maxval[bin0] &&
		     expr.leftBound() >= minval[bin0]) {
		cand0 = bin0;
		cand1 = bin0 + 1;
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LT:
	    if (expr.leftBound() < expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (expr.leftBound() <= expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.rightBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (expr.leftBound() > expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (expr.leftBound() >= expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.leftBound() == expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			cand0 = nobs;
			cand1 = nobs + 1;
		    }
		    else {
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		}
		else {
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_EQ
    } // switch (expr.leftOperator())
    if (ibis::gVerbose > 5) {
	std::ostringstream ostr;
	ostr << expr;
	col->logMessage("range::locate", "expr(%s) -> [%lu, %lu)",
			ostr.str().c_str(), static_cast<long unsigned>(cand0),
			static_cast<long unsigned>(cand1));
    }
} // ibis::range::locate()

void ibis::range::locate(const ibis::qContinuousRange& expr,
			 uint32_t& cand0, uint32_t& cand1,
			 uint32_t& hit0, uint32_t& hit1) const {
    // bins in the range of [hit0, hit1) are hits
    // bins in the range of [cand0, cand1) are candidates
    cand0=0, hit0=0, hit1=0, cand1=0;
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
	    col->logWarning("range::locate", "operators for the range not "
			    "specified");
	    return;
	case ibis::qExpr::OP_LT:
	    hit0 = 0;
	    cand0 = 0;
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
	    else if (expr.rightBound() <= minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (bin1 >= nobs) {
		if (expr.rightBound() <= max1 &&
		    expr.rightBound() >= min1) {
		    hit0 = nobs;
		    hit1 = nobs;
		    cand0 = nobs;
		    cand1 = nobs + 1;
		    if (max1 == min1)
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else if (expr.rightBound() <= maxval[bin1] &&
		     expr.rightBound() >= minval[bin1]) {
		hit0 = bin1;
		hit1 = bin1;
		cand0 = bin1;
		cand1 = bin1 + 1;
		if (maxval[bin1] == minval[bin1])
		    hit1 = cand1;
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_UNDEFINED
    case ibis::qExpr::OP_LT:
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
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (expr.rightBound() > expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() >= max1) {
			hit1 = nobs + 1;
			cand0 = nobs + 1;
		    }
		    else if (expr.rightBound() >= min1) {
			hit1 = nobs + 1;
			cand0 = nobs;
		    }
		    else {
			hit1 = nobs;
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
		else if (expr.rightBound() <= minval[bin1]) {
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
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1;
		    hit1 = bin1;
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LT
    case ibis::qExpr::OP_LE:
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
	    hit1 = nobs + 1;
	    cand1 = nobs + 1;
	    if (expr.rightBound() >= expr.leftBound()) {
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
		else if (expr.rightBound() <= minval[bin1]) {
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
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else  {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1;
		    hit1 = bin1;
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_LE
    case ibis::qExpr::OP_GT:
	if (bin0 >= nobs) {
	    if (expr.leftBound() > max1) {
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
	    cand0 = 0;
	    hit0 = 0;
	    break;
	case ibis::qExpr::OP_LT:
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
	    else if (expr.rightBound() <= minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() < expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else  {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1;
		    hit1 = bin1;
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GT
    case ibis::qExpr::OP_GE:
	if (bin0 >= nobs) {
	    if (expr.leftBound() > max1) {
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
	    if (expr.rightBound() <= expr.leftBound()) {
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
	    break;
	case ibis::qExpr::OP_GE:
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
	    else if (expr.rightBound() <= minval[bin1]) {
		hit0 = bin1;
		cand0 = bin1;
	    }
	    else {
		hit0 = bin1 + 1;
		cand0 = bin1;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.rightBound() <= expr.leftBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1;
		    hit1 = bin1;
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_GE
    case ibis::qExpr::OP_EQ:
	switch (expr.rightOperator()) {
	default:
	case ibis::qExpr::OP_UNDEFINED:
	    if (bin0 >= nobs) {
		if (expr.leftBound() <= max1 &&
		    expr.leftBound() >= min1) {
		    hit0 = nobs;
		    hit1 = nobs;
		    cand0 = nobs;
		    cand1 = nobs + 1;
		    if (max1 == min1)
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else if (expr.leftBound() <= maxval[bin0] &&
		     expr.leftBound() >= minval[bin0]) {
		hit0 = bin0;
		hit1 = bin0;
		cand0 = bin0;
		cand1 = bin0 + 1;
		if (maxval[bin0] == minval[bin0])
		    hit1 = cand1;
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LT:
	    if (expr.leftBound() < expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0;
		    hit1 = bin0;
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_LE:
	    if (expr.leftBound() <= expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0;
		    hit1 = bin0;
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GT:
	    if (expr.leftBound() > expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0;
		    hit1 = bin0;
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_GE:
	    if (expr.leftBound() >= expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.leftBound() <= max1 &&
			expr.leftBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.leftBound() <= maxval[bin0] &&
			 expr.leftBound() >= minval[bin0]) {
		    hit0 = bin0;
		    hit1 = bin0;
		    cand0 = bin0;
		    cand1 = bin0 + 1;
		    if (maxval[bin0] == minval[bin0])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	case ibis::qExpr::OP_EQ:
	    if (expr.leftBound() == expr.rightBound()) {
		if (bin1 >= nobs) {
		    if (expr.rightBound() <= max1 &&
			expr.rightBound() >= min1) {
			hit0 = nobs;
			hit1 = nobs;
			cand0 = nobs;
			cand1 = nobs + 1;
			if (max1 == min1)
			    hit1 = cand1;
		    }
		    else {
			hit0 = 0;
			hit1 = 0;
			cand0 = 0;
			cand1 = 0;
		    }
		}
		else if (expr.rightBound() <= maxval[bin1] &&
			 expr.rightBound() >= minval[bin1]) {
		    hit0 = bin1;
		    hit1 = bin1;
		    cand0 = bin1;
		    cand1 = bin1 + 1;
		    if (maxval[bin1] == minval[bin1])
			hit1 = cand1;
		}
		else {
		    hit0 = 0;
		    hit1 = 0;
		    cand0 = 0;
		    cand1 = 0;
		}
	    }
	    else {
		hit0 = 0;
		hit1 = 0;
		cand0 = 0;
		cand1 = 0;
	    }
	    break;
	} // switch (expr.rightOperator())
	break; // case ibis::qExpr::OP_EQ
    } // switch (expr.leftOperator())
    if (ibis::gVerbose > 5) {
	std::ostringstream ostr;
	ostr << expr;
	col->logMessage("range::locate", "expr(%s) -> [%lu:%lu, %lu:%lu)",
			ostr.str().c_str(), static_cast<long unsigned>(cand0),
			static_cast<long unsigned>(hit0),
			static_cast<long unsigned>(hit1),
			static_cast<long unsigned>(cand1));
    }
} // ibis::range::locate

long ibis::range::evaluate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower) const {
    long ierr = 0;
    uint32_t cand0=0, hit0=0, hit1=0, cand1=0;
    if (nobs <= 0) {
	lower.set(0, nrows);
	return ierr;
    }

    // bins in the range of [hit0, hit1) are hits
    // bins in the range of [cand0, cand1) are candidates
    locate(expr, cand0, cand1, hit0, hit1);
    if (cand0 >= cand1) {
	lower.set(0, nrows);
	return ierr;
    }

    // attempt to activate all bitmaps in one or two read operations
    if (hit0+2 >= hit1) { // activate all in one call
	activate(cand0>0?cand0-1:0, cand1);
    }
    else { // activate bitmaps in two steps
	activate(cand0>0?cand0-1:0, hit0);
	activate(hit1-1, cand1);
    }

    // lower contains bins [hit0, hit1)
    if (hit0 >= hit1) {
	lower.set(0, nrows);
    }
    else if (hit0 > 0) {
	if (hit1 <= nobs) {
	    if (bits[hit1-1] != 0)
		lower.copy(*(bits[hit1-1]));
	    else
		lower.set(0, nrows);
	    if (bits[hit0-1] != 0)
		lower -= *(bits[hit0-1]);
	}
	else {
	    if (bits[hit0-1] != 0) {
		lower.copy(*(bits[hit0-1]));
		lower.flip();
	    }
	    else {
		lower.set(1, nrows);
	    }
	}
    }
    else if (hit1 <= nobs) {
	if (bits[hit1-1] != 0)
	    lower.copy(*(bits[hit1-1]));
	else
	    lower.set(0, nrows);
    }
    else {
	lower.set(1, nrows);
    }

    if (cand0+1 == hit0) { // candidate bin cand0
	if (cand0 == nobs) {
	    ibis::bitvector tmp, res;
	    col->getNullMask(tmp);
	    if (cand0 > 0 && bits[cand0-1] != 0)
		tmp -= *(bits[cand0-1]);
	    if (tmp.cnt() > 0) {
		ierr = checkBin(expr, cand0, tmp, res);
		if (ierr > 0)
		    lower |= res;
		else if (ierr < 0)
		    return ierr;
	    }
	}
	else if (cand0 < nobs && bits[cand0] != 0) {
	    ibis::bitvector tmp, res;
	    tmp.copy(*(bits[cand0]));
	    if (cand0 > 0 && bits[cand0-1] != 0)
		tmp -= *(bits[cand0-1]);
	    if (tmp.cnt() > 0) {
		ierr = checkBin(expr, cand0, tmp, res);
		if (ierr > 0)
		    lower |= res;
		else if (ierr < 0)
		    return ierr;
	    }
	}
    }
    if (hit1+1 == cand1) { // candidate bin hit1
	if (hit1 == nobs) {
	    ibis::bitvector tmp, res;
	    col->getNullMask(tmp);
	    if (hit1 > 0 && bits[hit1-1] != 0)
		tmp -= *(bits[hit1-1]);
	    if (tmp.cnt() > 0) {
		ierr = checkBin(expr, hit1, tmp, res);
		if (ierr > 0)
		    lower |= res;
		else if (ierr < 0)
		    return ierr;
	    }
	}
	else if (hit1 < nobs && bits[hit1] != 0) {
	    ibis::bitvector tmp, res;
	    tmp.copy(*(bits[hit1]));
	    if (hit1 > 0 && bits[hit1-1] != 0)
		tmp -= *(bits[hit1-1]);
	    if (tmp.cnt() > 0) {
		ierr = checkBin(expr, hit1, tmp, res);
		if (ierr > 0)
		    lower |= res;
		else if (ierr < 0)
		    return ierr;
	    }
	}
    }
    ierr = lower.cnt();
    return ierr;
} // ibis::range::evaluate

// compute the lower and upper bound of the hit vector for the range
// expression
void ibis::range::estimate(const ibis::qContinuousRange& expr,
			   ibis::bitvector& lower,
			   ibis::bitvector& upper) const {
    // bins in the range of [hit0, hit1) are hits
    // bins in the range of [cand0, cand1) are candidates
    uint32_t cand0=0, hit0=0, hit1=0, cand1=0;
    if (nobs <= 0) {
	lower.set(0, nrows);
	upper.clear();
	return;
    }

    locate(expr, cand0, cand1, hit0, hit1);

    // generate the two bitvectors from the computed range boundaries
    // lower contains bins [hit0, hit1)
    if (hit0 >= hit1) {
	lower.set(0, nrows);
    }
    else if (hit0 > 0) {
	if (hit1 <= nobs) {
	    if (bits[hit1-1] == 0)
		activate(hit1-1);
	    if (bits[hit1-1] != 0)
		lower.copy(*(bits[hit1-1]));
	    else
		lower.set(0, nrows);
	    if (bits[hit0-1] == 0)
		activate(hit0-1);
	    if (bits[hit0-1] != 0)
		lower -= *(bits[hit0-1]);
	}
	else {
	    if (bits[hit0-1] == 0)
		activate(hit0-1);
	    if (bits[hit0-1] != 0) {
		lower.copy(*(bits[hit0-1]));
		lower.flip();
	    }
	    else {
		lower.set(1, nrows);
	    }
	}
    }
    else if (hit1 <= nobs) {
	if (bits[hit1-1] == 0)
	    activate(hit1-1);
	if (bits[hit1-1] != 0)
	    lower.copy(*(bits[hit1-1]));
	else
	    lower.set(0, nrows);
    }
    else {
	lower.set(1, nrows);
    }

    // upper contains bins [cand0, cand1)
    if (hit0 == cand0 && hit1 == cand1) {
	upper.copy(lower);
    }
    else if (cand0 > 0) {
	if (cand1 <= nobs) {
	    if (bits[cand1-1] == 0)
		activate(cand1-1);
	    if (bits[cand1-1] != 0)
		upper.copy(*(bits[cand1-1]));
	    else
		upper.set(0, nrows);
	    if (bits[cand0-1] == 0)
		activate(cand0-1);
	    if (bits[cand0-1] != 0)
		upper -= *(bits[cand0-1]);
	}
	else {
	    if (bits[cand0-1] == 0)
		activate(cand0-1);
	    if (bits[cand0-1] != 0) {
		upper.copy(*(bits[cand0-1]));
		upper.flip();
	    }
	    else {
		upper.set(1, nrows);
	    }
	}
    }
    else if (cand1 <= nobs) {
	if (bits[cand1-1] == 0)
	    activate(cand1-1);
	if (bits[cand1-1] != 0)
	    upper.copy(*(bits[cand1-1]));
	else
	    upper.set(1, nrows);
    }
    else {
	upper.set(1, nrows);
    }
} // ibis::range::estimate

// return an upper bound on the number of hits
uint32_t ibis::range::estimate(const ibis::qContinuousRange& expr) const {
    if (bits.empty()) return 0;

    uint32_t cand0=0, cand1=0, nhits=0;
    locate(expr, cand0, cand1);

    // compute the upper bound of number of hits
    if (cand1 > cand0) {
	if (cand0 > 0) {
	    if (cand1 <= nobs) {
		if (bits[cand1-1] == 0)
		    activate(cand1-1);
		if (bits[cand1-1] != 0) {
		    if (bits[cand0-1] == 0)
			activate(cand0-1);
		    if (bits[cand0-1] != 0)
			nhits = bits[cand1-1]->cnt() - bits[cand0-1]->cnt();
		    else
			nhits = bits[cand1-1]->cnt();
		}
		else {
		    nhits = 0;
		}
	    }
	    else {
		if (bits[cand0-1] == 0)
		    activate(cand0-1);
		if (bits[cand0-1] != 0)
		    nhits = bits[cand0-1]->size() - bits[cand0-1]->cnt();
		else
		    nhits = nrows;
	    }
	}
	else if (cand1 <= nobs) {
	    if (bits[cand1-1] == 0)
		activate(cand1-1);
	    if (bits[cand1-1] != 0)
		nhits = bits[cand1-1]->cnt();
	    else
		nhits = 0;
	}
	else {
	    nhits = nrows;
	}
    }
    return nhits;
} // ibis::range::estimate()


// ***should implement a more efficient version***
float ibis::range::undecidable(const ibis::qContinuousRange& expr,
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
} // ibis::range::undecidable

// expand range condition -- rely on the fact that the only operators used are
// LT, LE and EQ
int ibis::range::expandRange(ibis::qContinuousRange& range) const {
    uint32_t cand0, cand1;
    double left, right;
    int ret = 0;
    locate(range, cand0, cand1);
    if (cand0 < nobs) {
	if ((range.leftOperator() == ibis::qExpr::OP_LT &&
	     range.leftBound() >= minval[cand0]) ||
	    (range.leftOperator() == ibis::qExpr::OP_LE &&
	     range.leftBound() > minval[cand0])) {
	    // decrease the left bound
	    ++ ret;
	    right = minval[cand0];
	    if (cand0 > 0)
		left = maxval[cand0-1];
	    else
		left = -DBL_MAX;
	    range.leftBound() = ibis::util::compactValue(left, right);
	}
	else if (range.leftOperator() == ibis::qExpr::OP_EQ &&
		 range.leftBound() >= minval[cand0] &&
		 range.leftBound() <= maxval[cand0] &&
		 minval[cand0] < maxval[cand0]) {
	    // expand the equality condition into a range condition
	    ++ ret;
	    right = minval[cand0];
	    if (cand0 > 0)
		left = maxval[cand0-1];
	    else
		left = -DBL_MAX;
	    range.leftOperator() = ibis::qExpr::OP_LE;
	    range.leftBound() = ibis::util::compactValue(left, right);
	    left = maxval[cand0];
	    if (cand0+1 < minval.size())
		right = minval[cand0+1];
	    else
		right = DBL_MAX;
	    range.rightOperator() = ibis::qExpr::OP_LE;
	    range.rightBound() = ibis::util::compactValue(left, right);
	}
    }
    else if (cand0 == nobs) { // the bin not explicitly tracked
	if ((range.leftOperator() == ibis::qExpr::OP_LT &&
	     range.leftBound() >= min1) ||
	    (range.leftOperator() == ibis::qExpr::OP_LE &&
	     range.leftBound() > min1)) {
	    // decrease the left bound
	    ++ ret;
	    right = min1;
	    if (cand0 > 0)
		left = maxval[cand0-1];
	    else
		left = -DBL_MAX;
	    range.leftBound() = ibis::util::compactValue(left, right);
	}
	else if (range.leftOperator() == ibis::qExpr::OP_EQ &&
		 min1 < max1 && range.leftBound() >= min1 &&
		 range.leftBound() <= max1) {
	    // expand the equality condition into a range condition
	    ++ ret;
	    right = min1;
	    if (cand0 > 0)
		left = maxval[cand0-1];
	    else
		left = -DBL_MAX;
	    range.leftOperator() = ibis::qExpr::OP_LE;
	    range.leftBound() = ibis::util::compactValue(left, right);
	    left = max1;
	    right = DBL_MAX;
	    range.rightOperator() = ibis::qExpr::OP_LE;
	    range.rightBound() = ibis::util::compactValue(left, right);
	}
    }

    if (cand1 > 0 && cand1 <= nobs &&
	((range.rightOperator() == ibis::qExpr::OP_LT &&
	  range.rightBound() > minval[cand1-1]) ||
	 (range.rightOperator() == ibis::qExpr::OP_LE &&
	  range.rightBound() >= minval[cand1-1]))) {
	// increase the right bound
	++ ret;
	left = maxval[cand1-1];
	if (cand1 < nobs)
	    right = minval[cand1];
	else
	    right = DBL_MAX;
	range.rightBound() = ibis::util::compactValue(left, right);
    }
    else if (cand1 == nobs+1 && 
	((range.rightOperator() == ibis::qExpr::OP_LT &&
	  range.rightBound() > min1) ||
	 (range.rightOperator() == ibis::qExpr::OP_LE &&
	  range.rightBound() >= min1))) {
	++ ret;
	left = max1;
	right = DBL_MAX;
	range.rightBound() = ibis::util::compactValue(left, right);
    }
    return ret;
} // ibis::range::expandRange

// contract range condition -- rely on the fact that the only operators used
// are LT, LE and EQ
int ibis::range::contractRange(ibis::qContinuousRange& range) const {
    uint32_t cand0, cand1;
    double left, right;
    int ret = 0;
    locate(range, cand0, cand1);
    if (cand0 < nobs) {
	if ((range.leftOperator() == ibis::qExpr::OP_LT &&
	     range.leftBound() <= maxval[cand0]) ||
	    (range.leftOperator() == ibis::qExpr::OP_LE &&
	     range.leftBound() < maxval[cand0])) {
	    // increase the left bound
	    ++ ret;
	    left = maxval[cand0];
	    if (cand0+1 < nobs)
		right = minval[cand0+1];
	    else
		right = DBL_MAX;
	    range.leftBound() = ibis::util::compactValue(left, right);
	}
	else if (range.leftOperator() == ibis::qExpr::OP_EQ &&
		 minval[cand0] < maxval[cand0] &&
		 range.leftBound() >= minval[cand0] &&
		 range.leftBound() <= maxval[cand0]) {
	    // reduce the equality to no value
	    ++ ret;
	    right = minval[cand0];
	    if (cand0 > 0)
		left = maxval[cand0-1];
	    else
		left = -DBL_MAX;
	    range.leftBound() = ibis::util::compactValue(left, right);
	}
    }
    else if (cand0 == nobs) {
	if ((range.leftOperator() == ibis::qExpr::OP_LT &&
	     range.leftBound() <= max1) ||
	    (range.leftOperator() == ibis::qExpr::OP_LE &&
	     range.leftBound() < max1)) {
	    // increase the left bound
	    ++ ret;
	    left = max1;
	    right = DBL_MAX;
	    range.leftBound() = ibis::util::compactValue(left, right);
	}
	else if (range.leftOperator() == ibis::qExpr::OP_EQ &&
		 min1 < max1 && range.leftBound() >= min1 &&
		 range.leftBound() <= max1) {
	    // reduce the equality to no value
	    ++ ret;
	    left = ibis::util::incrDouble(max1);
	    right = DBL_MAX;
	    range.leftBound() = ibis::util::compactValue(left, right);
	}
    }

    if (cand1 > 0 && cand1 <= nobs &&
	((range.rightOperator() == ibis::qExpr::OP_LT &&
	  range.rightBound() > minval[cand1-1]) ||
	 (range.rightOperator() == ibis::qExpr::OP_LE &&
	  range.rightBound() >= minval[cand1-1]))) {
	// decrease the right bound
	++ ret;
	right = minval[cand1-1];
	if (cand1 > 1)
	    left = maxval[cand1-2];
	else
	    left = -DBL_MAX;
	range.rightBound() = ibis::util::compactValue(left, right);
    }
    else if (cand1 == nobs+1) {
	++ ret;
	right = min1;
	if (nobs > 0)
	    left = maxval[nobs-1];
	else
	    left = -DBL_MAX;
	range.rightBound() = ibis::util::compactValue(left, right);
    }
    return ret;
} // ibis::range::contractRange

double ibis::range::getMax() const {
    double ret = max1;
    for (uint32_t i = nobs; i > 0 && ret == -DBL_MAX; ) {
	-- i;
	if (ret < maxval[i])
	    ret = maxval[i];
    }
    return ret;
} // ibis::range::getMax

double ibis::range::getSum() const {
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
} // ibis::range::getSum

double ibis::range::computeSum() const {
    double sum = 0;
    activate(); // need to activate all bitvectors
    if (minval[0] <= maxval[0] && bits[0] != 0)
	sum = 0.5*(minval[0] + maxval[0]) * bits[0]->cnt();
    for (uint32_t i = 1; i < nobs; ++ i)
	if (minval[i] <= maxval[i] && bits[i] != 0) {
	    if (bits[i-1] != 0) {
		ibis::bitvector *tmp = *(bits[i]) - *(bits[i-1]);
		sum += 0.5 * (minval[i] + maxval[i]) * tmp->cnt();
		delete tmp;
	    }
	    else {
		sum += 0.5 * (minval[i] + maxval[i]) * bits[i-1]->cnt();
	    }
	}
    // dealing with the last bins
    ibis::bitvector mask;
    col->getNullMask(mask);
    mask -= *(bits[nobs-1]);
    sum += 0.5*(max1 + min1) * mask.cnt();
    return sum;
} // ibis::range::computeSum

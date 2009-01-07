// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2006-2009 the Regents of the University of California

// This file contains the implementation of the class ibis::fuge.  It
// defines a two-level index where the coarse level the interval encoding,
// but the lower level contains only the simple bins.
//
// The word interstice (a synonym of interval) when translated to german,
// the answers.com web site gives two words: Zwischenraum and Fuge.  Since
// the word Fuge is only four letter long, it is similar to many
// variantions of the index class names -- very tangentially related to the
// index it represents.

#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "ibin.h"
#include "part.h"
#include "column.h"
#include "resource.h"

#include <string>
//
ibis::fuge::fuge(const ibis::column *c, const char *f)
    : ibis::bin(c, f) {
    if (c == 0) return; // nothing to do
    if (cbits.empty() || cbits.size()+1 != cbounds.size()) {
	if (fname != 0)
	    readCoarse(f);
	else
	    coarsen();
    }
    if (ibis::gVerbose > 4) {
	ibis::util::logger lg(4);
	print(lg.buffer());
    }
} // ibis::fuge::fuge

// generate an ibis::fuge from ibis::bin
ibis::fuge::fuge(const ibis::bin& rhs) : ibis::bin(rhs) {
    if (col == 0) return;
    if (nobs <= 1) return; // rhs does not contain an valid index

    try {
	coarsen();
    }
    catch (...) {
	for (unsigned i = 0; i < cbits.size(); ++ i)
	    delete cbits[i];
	cbits.clear();
	cbounds.clear();
	coffsets.clear();
    }
    if (ibis::gVerbose > 4) {
	ibis::util::logger lg(4);
	print(lg.buffer());
    }
} // copy from ibis::bin

/// Reconstruct from content of a file.
/**
   The leading portion of the index file is the same as ibis::bin, which
   allows the constructor of the base class to work properly.  The content
   following the last bitvector in ibis::bin is as follows, @sa
   ibis::fuge::writeCoarse.

   nc      (uint32_t)                   -- number of coarse bins.
   cbounds (unsigned[nc+1])             -- boundaries of the coarse bins.
   coffsets(int32_t[nc-ceil(nc/2)+2])   -- starting positions.
   cbits   (bitvector[nc-ceil(nc/2)+1]) -- bitvectors.
 */
ibis::fuge::fuge(const ibis::column* c, ibis::fileManager::storage* st,
		 uint32_t start) : ibis::bin(c, st, start) {
    if (st->size() <= static_cast<uint32_t>(offsets.back()))
	return; // no coarse bin

    start = offsets.back();
    uint32_t nc = *(reinterpret_cast<uint32_t*>(st->begin()+start));
    if (nc == 0 ||
	st->size() <= start + (sizeof(int32_t)+sizeof(uint32_t))*(nc+1))
	return;

    const uint32_t ncb = nc - (nc+1)/2 + 1;
    start += sizeof(uint32_t);
    if (start+sizeof(uint32_t)*(nc+1) < st->size()) {
	array_t<uint32_t> tmp(st, start, nc+1);
	cbounds.swap(tmp);
    }
    start += sizeof(uint32_t) * (nc+1);
    if (start+sizeof(int32_t)*(ncb+1) < st->size()) {
	array_t<int32_t> tmp(st, start, ncb+1);
	coffsets.swap(tmp);
	if (coffsets.back() > static_cast<int32_t>(st->size())) {
	    coffsets.swap(tmp);
	    array_t<uint32_t> tmp2;
	    cbounds.swap(tmp2);
	    return;
	}
    }
    else {
	array_t<uint32_t> tmp2;
	cbounds.swap(tmp2);
	return;
    }

    cbits.resize(ncb);
    for (unsigned i = 0; i < ncb; ++ i)
	cbits[i] = 0;

    if (st->isFileMap()) {
#if defined(ALWAY_READ_BITVECTOR0)
	array_t<ibis::bitvector::word_t>
	    a0(st, coffsets[0], (coffsets[1] - coffsets[0])
	       / sizeof(ibis::bitvector::word_t));
	cbits[0] = new ibis::bitvector(a0);
	cbits[0]->setSize(nrows);
#endif
    }
    else { // all bytes in memory already
	for (unsigned i = 0; i < ncb; ++ i) {
	    if (coffsets[i+1] > coffsets[i]) {
		array_t<ibis::bitvector::word_t>
		    a(st, coffsets[i], (coffsets[i+1]-coffsets[i])
		      / sizeof(ibis::bitvector::word_t));
		cbits[i] = new ibis::bitvector(a);
		cbits[i]->setSize(nrows);
	    }
	}
    }

    if (ibis::gVerbose > 4) {
	ibis::util::logger lg;
	print(lg.buffer());
    }
} // ibis::fuge::fuge

int ibis::fuge::write(const char* dt) const {
    if (nobs <= 1) return -1;

    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	col->logWarning("fuge::write", "unable to open \"%s\" for write",
			fnm.c_str());
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    char header[] = "#IBIS\4\0\0";
    header[5] = (char)ibis::index::FUGE;
    header[6] = (char)sizeof(int32_t);
    int ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "ibis::column[" << col->partition()->name() << "."
	    << col->name() << "]::fuge::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr = ibis::bin::write(fdes); // write the basic binned index
    if (ierr >= 0)
	ierr = writeCoarse(fdes); // write the coarse level bins
#if _POSIX_FSYNC+0 > 0
    (void) fsync(fdes); // write to disk
#endif
    (void) UnixClose(fdes);
    return ierr;
} // ibis::fuge::write

// read the content of a file
int ibis::fuge::read(const char* f) {
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
		  header[5] == static_cast<char>(ibis::index::FUGE) &&
		  header[6] == static_cast<char>(sizeof(int32_t)) &&
		  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return -3;
    }

    clear(); // clear the existing content
    uint32_t begin, end;
    fname = ibis::util::strnewdup(fnm.c_str());
    str = 0;

    long ierr = UnixRead(fdes, static_cast<void*>(&nrows), sizeof(uint32_t));
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
    begin = 8+2*sizeof(uint32_t);
    end = 8+2*sizeof(uint32_t)+(nobs+1)*sizeof(int32_t);
    if (trymmap) {
	array_t<int32_t> tmp(fname, begin, end);
	offsets.swap(tmp);
    }
    else {
	array_t<int32_t> tmp(fdes, begin, end);
	offsets.swap(tmp);
    }

    // read bounds
    begin = 8 * ((15 + sizeof(int32_t)*(nobs+1)+2*sizeof(uint32_t))/8);
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
    ibis::fileManager::instance().recordPages(0, end);

    // initialized bits with nil pointers
    for (uint32_t i = 0; i < bits.size(); ++i)
	delete bits[i];
    bits.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i] = 0;

#if defined(ALWAY_READ_BITVECTOR0)
    if (offsets[1] > offsets[0]) {// read the first bitvector
	array_t<ibis::bitvector::word_t> a0(fdes, offsets[0], offsets[1]);
	ibis::bitvector* tmp = new ibis::bitvector(a0);
	bits[0] = tmp;
#if defined(WAH_CHECK_SIZE)
	if (tmp->size() != nrows)
	    col->logWarning("readIndex", "the size (%lu) of 1st "
			    "bitvector (from \"%s\") differs "
			    "from nRows (%lu)",
			    static_cast<long unsigned>(tmp->size()),
			    fnm.c_str(),
			    static_cast<long unsigned>(nrows));
#else
	tmp->setSize(nrows)
#endif
	    }
    else {
	bits[0] = new ibis::bitvector;
	bits[0]->set(0, nrows);
    }
#endif

    // reading the coarse bins
    ierr = UnixSeek(fdes, offsets.back(), SEEK_SET);
    if (ierr == offsets.back()) {
	uint32_t nc;
	ierr = UnixRead(fdes, &nc, sizeof(nc));
	begin = offsets.back() + sizeof(nc);
	end = begin + sizeof(uint32_t)*(nc+1);
	if (ierr > 0 && nc > 0) {
	    array_t<uint32_t> tmp(fdes, begin, end);
	    cbounds.swap(tmp);
	}
	begin = end;
	end += sizeof(int32_t) * (nc+2-(nc+1)/2);
	if (cbounds.size() == nc+1) {
	    array_t<int32_t> tmp(fdes, begin, end);
	    coffsets.swap(tmp);
	}

	for (unsigned i = 0; i < cbits.size(); ++ i)
	    delete cbits[i];
	cbits.resize(nc+1-(nc+1)/2);
	for (unsigned i = 0; i < nc+1-(nc+1)/2; ++ i)
	    cbits[i] = 0;
    }

    (void) UnixClose(fdes);
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fname);
    return 0;
} // ibis::fuge::read

int ibis::fuge::read(ibis::fileManager::storage* st) {
    int ierr = ibis::bin::read(st);
    if (ierr < 0) return ierr;

    if (str->size() > static_cast<uint32_t>(offsets.back())) {
	if (st->isFileMap()) {
	    uint32_t nc =
		*(reinterpret_cast<uint32_t*>(str->begin() + offsets.back()));
	    if (nc > 0 && str->size() > static_cast<uint32_t>(offsets.back()) +
		(sizeof(int32_t)+sizeof(uint32_t))*(nc+1)) {
		uint32_t start;
		start = offsets.back() + sizeof(uint32_t);
		array_t<uint32_t> btmp(str, start, nc+1);
		cbounds.swap(btmp);

		start += sizeof(uint32_t)*(nc+1);
		array_t<int32_t> otmp(str, start, nc+1);

		cbits.resize(nc);
		for (unsigned i = 0; i < nc; ++ i)
		    cbits[i] = 0;
	    }
	}
	else { // regenerate all the bitvectors
	    uint32_t nc =
		*(reinterpret_cast<uint32_t*>(str->begin() + offsets.back()));
	    const uint32_t ncb = nc + 1 - (nc+1) / 2;
	    if (nc > 0 && str->size() > offsets.back() +
		(sizeof(int32_t)+sizeof(uint32_t))*(nc+1)) {
		uint32_t start;
		start = offsets.back() + 4;
		array_t<uint32_t> btmp(str, start, nc+1);
		cbounds.swap(btmp);

		start += sizeof(uint32_t)*(ncb+1);
		array_t<int32_t> otmp(str, start, ncb+1);

		cbits.resize(ncb);
		for (unsigned i = 0; i < ncb; ++ i) {
		    if (coffsets[i+1] > coffsets[i]) {
			array_t<ibis::bitvector::word_t>
			    a(st, coffsets[i], (coffsets[i+1]-coffsets[i])
			      / sizeof(ibis::bitvector::word_t));
			cbits[i] = new ibis::bitvector(a);
			cbits[i]->setSize(nrows);
		    }
		    else {
			cbits[i] = 0;
		    }
		}
	    }
	}
    }
    return 0;
} // ibis::fuge::read

// fill with zero bits or truncate
void ibis::fuge::adjustLength(uint32_t nr) {
    ibis::bin::adjustLength(nr); // the top level
    for (unsigned j = 0; j < cbits.size(); ++ j)
	if (cbits[j] != 0)
	    cbits[j]->adjustSize(0, nr);
} // ibis::fuge::adjustLength

// the printing function
void ibis::fuge::print(std::ostream& out) const {
    const uint32_t nc = (cbounds.empty() ? 0U : cbounds.size()-1);
    const uint32_t ncb = nc+1 - (nc+1)/2;
    out << "index (binned interval-equality code) for "
	<< col->partition()->name() << '.' << col->name()
	<< " contains " << nc << " coarse bin" << (nc > 1 ? "s" : "")
	<< ", " << nobs << " fine bins for " << nrows << " objects \n";
    uint32_t nprt = (ibis::gVerbose < 30 ? 1 << ibis::gVerbose : bits.size());
    uint32_t omitted = 0;
    uint32_t end;
    if (nc > 0 && cbits.size() == ncb && coffsets.size() == ncb+1) {
	for (unsigned j = 0; j < nc; ++ j) {
	    out << "Coarse bin " << j << ", [" << cbounds[j] << ", "
		<< cbounds[j+1] << ")";
	    if (j < ncb && cbits[j] != 0)
		out << "\t{[" << bounds[j] << ", " << cbounds[j+(nc+1)/2]
		    << ")\t" << cbits[j]->cnt() << "\t" << cbits[j]->bytes()
		    << "}";
	    out << "\n";
	    end = (cbounds[j+1] <= cbounds[j]+nprt ?
		   cbounds[j+1] : cbounds[j]+nprt);
	    for (unsigned i = cbounds[j]; i < end; ++ i) {
		out << "\t" << i << ": ";
		if (i > 0)
		    out << "[" << bounds[i-1];
		else
		    out << "(...";
		out << ", " << bounds[i] << ")\t[" << minval[i]
		    << ", " << maxval[i] << "]";
		if (bits[i] != 0)
		    out << "\t" << bits[i]->cnt() << "\t"
			<< bits[i]->bytes();
		out << "\n";
	    }
	    if (cbounds[j+1] > end) {
		out << "\t...\n";
		omitted += (cbounds[j+1] - end);
	    }
	}
	if (omitted > 0)
	    out << "\tfine level bins omitted: " << omitted << "\n";
    }
    else {
	end = (nobs <= nprt ? nobs : nprt);
	for (unsigned i = 0; i < end; ++ i) {
	    out << "\t" << i << ": ";
	    if (i > 0)
		out << "[" << bounds[i-1];
	    else
		out << "(...";
	    out << ", " << bounds[i] << ")\t[" << minval[i]
		<< ", " << maxval[i] << "]";
	    if (bits[i] != 0)
		out << "\t" << bits[i]->cnt() << "\t"
		    << bits[i]->bytes();
	    out << "\n";
	}
	if (end < nobs)
	    out << "\tbins omitted: " << nobs - end << "\n";
    }
    out << std::endl;
} // ibis::fuge::print

long ibis::fuge::append(const char* dt, const char* df, uint32_t nnew) {
    long ret = ibis::bin::append(dt, df, nnew);
    if (ret <= 0 || static_cast<uint32_t>(ret) != nnew)
	return ret;

    clearCoarse();
    coarsen();
    return ret;
} // ibis::fuge::append

long ibis::fuge::append(const ibis::fuge& tail) {
    long ret = ibis::bin::append(tail);
    if (ret < 0) return ret;

    clearCoarse();
    coarsen();
    return ret;
} // ibis::fuge::append

long ibis::fuge::evaluate(const ibis::qContinuousRange& expr,
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
} // ibis::fuge::evaluate

// compute the lower and upper bound of the hit vector for the range
// expression
void ibis::fuge::estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const {
    if (bits.empty()) {
	lower.set(0, nrows);
	upper.set(1, nrows);
	return;
    }

    // bins in the range of [hit0, hit1) are hits
    // bins in the range of [cand0, cand1) are candidates
    uint32_t cand0, hit0, hit1, cand1;
    locate(expr, cand0, cand1, hit0, hit1);
    if (cand0 >= cand1 || cand1 == 0 || cand0 >= nobs) { // no hits at all
	lower.set(0, nrows);
	upper.clear();
	return;
    }
    else if (hit0 >= hit1) { // no sure hits, but some candidates
	lower.set(0, nrows);
	if (bits[cand0] == 0)
	    activate(cand0);
	if (bits[cand0] != 0)
	    upper.copy(*bits[cand0]);
	else
	    upper.clear();
    }

    const uint32_t ncoarse = (cbounds.empty() ? 0U : cbounds.size()-1);
    if (hit0+3 >= hit1 || ncoarse == 0 || (cbits.size()+1) != coffsets.size()
	|| cbits.size() != (ncoarse-(ncoarse+1)/2+1)
	|| offsets[cand1]-offsets[cand0] < 262144) {
	// use the fine level bitmaps only
	sumBins(hit0, hit1, lower);
	if (cand0 < hit0 || (cand1 > hit1 && hit1 < nobs)) {
	    upper.copy(lower);
	    if (cand0 < hit0) {
		if (bits[cand0] == 0)
		    activate(cand0);
		if (bits[cand0] != 0)
		    upper |= *(bits[cand0]);
	    }
	    if (cand1 > hit1 && hit1 < nobs) {
		if (bits[hit1] == 0)
		    activate(hit1);
		if (bits[hit1] != 0)
		    upper |= *(bits[hit1]);
	    }
	}
	else {
	    upper.clear();
	}
	return;
    }

    // see whether the coarse bins could help
    const uint32_t c0 = cbounds.find(hit0);
    const uint32_t c1 = cbounds.find(hit1);
    if (ibis::gVerbose > 4) {
	ibis::util::logger lg(4);
	lg.buffer() << "ibis::fuge::evaluate(" << expr << ") hit0=" << hit0
		  << ", hit1=" << hit1;
	if (c0 < cbounds.size())
	    lg.buffer() << ", cbounds[" << c0 << "]=" << cbounds[c0];
	else
	    lg.buffer() << ", cbounds[" << cbounds.size()-1 << "]="
		      << cbounds.back();
	if (c1 < cbounds.size())
	    lg.buffer() << ", cbounds[" << c1 << "]=" << cbounds[c1];
	else
	    lg.buffer() << ", c1=" << c1 << ", bits.size()=" << bits.size();
    }
    if (c0 >= c1) { // within the same coarse bin
	long tmp = coarseEstimate(c1-1, c1)
	    + (offsets[hit0] - offsets[cbounds[c1-1]])
	    + (offsets[cbounds[c1]] - offsets[hit1]);
	if (offsets[hit1]-offsets[hit0] <= static_cast<long>(0.99*tmp)) {
	    sumBins(hit0, hit1, lower);
	}
	else {
	    coarseEvaluate(c1-1, c1, lower);
	    if (hit0 > cbounds[c1-1]) {
		ibis::bitvector bv;
		sumBins(cbounds[c1-1], hit0, bv);
		lower -= bv;
	    }
	    if (cbounds[c1] > hit1) {
		ibis::bitvector bv;
		sumBins(hit1, cbounds[c1], bv);
		lower -= bv;
	    }
	}
    }
    else {// general case: need to evaluate 5 options
	unsigned option = 2; // option 2 [direct | - | direct]
	long cost = (offsets[cbounds[c0]] - offsets[hit0])
	    + coarseEstimate(c0, c1-1)
	    + (offsets[hit1] - offsets[cbounds[c1-1]]);
	long tmp;
	if (c0 > 0) {	// option 3: [complement | - | direct]
	    tmp = (offsets[hit0] - offsets[cbounds[c0-1]])
		+ coarseEstimate(c0-1, c1-1)
		+ (offsets[hit1] - offsets[cbounds[c1-1]]);
	    if (tmp < cost) {
		cost = tmp;
		option = 3;
	    }
	}
	// option 4: [direct | - | complement]
	tmp = (offsets[cbounds[c0]] - offsets[hit0])
	    + coarseEstimate((c0>0 ? c0-1 : 0), c1)
	    + (offsets[cbounds[c1]] - offsets[hit1]);
	if (tmp < cost) {
	    cost = tmp;
	    option = 4;
	}
	if (c0 > 0) { // option 5: [complement | - | complement]
	    tmp = (offsets[hit0] - offsets[cbounds[c0-1]])
		+ coarseEstimate(c0-1, c1)
		+ (offsets[cbounds[c1]] - offsets[hit1]);
	    if (tmp < cost) {
		cost = tmp;
		option = 5;
	    }
	}
	// option 0 and 1: fine level only
	tmp = (offsets[hit1] - offsets[hit0] <=
	       (offsets.back()-offsets[hit1])+(offsets[hit0]-offsets[0]) ?
	       offsets[hit1] - offsets[hit0] :
	       (offsets.back()-offsets[hit1])+(offsets[hit0]-offsets[0]));
	if (cost > static_cast<long>(0.99*tmp)) { // slightly prefer 0/1
	    cost = tmp;
	    option = 1;
	}
	switch (option) {
	default:
	case 1: // use fine level only
	    sumBins(hit0, hit1, lower);
	    break;
	case 2: // direct | - | direct
	    coarseEvaluate(c0, c1-1, lower);
	    if (hit0 < cbounds[c0])
		addBins(hit0, cbounds[c0], lower); // left edge bin
	    if (cbounds[c1-1] < hit1)
		addBins(cbounds[c1-1], hit1, lower); // right edge bin
	    break;
	case 3: // complement | - | direct
	    coarseEvaluate(c0-1, c1-1, lower);
	    if (cbounds[c0-1] < hit0) { // left edge bin, complement
		ibis::bitvector bv;
		sumBins(cbounds[c0-1], hit0, bv);
		lower -= bv;
	    }
	    if (cbounds[c1-1] < hit1)
		addBins(cbounds[c1-1], hit1, lower); // right edge bin
	    break;
	case 4: // direct | - | complement
	    coarseEvaluate(c0, c1, lower);
	    if (hit0 < cbounds[c0])
		addBins(hit0, cbounds[c0], lower); // left edge bin
	    if (cbounds[c1] > hit1) { // right edge bin
		ibis::bitvector bv;
		sumBins(hit1, cbounds[c1], bv);
		lower -= bv;
	    }
	    break;
	case 5: // complement | - | complement
	    coarseEvaluate(c0-1, c1, lower);
	    if (hit0 > cbounds[c0-1]) { // left edge bin
		ibis::bitvector bv;
		sumBins(cbounds[c0-1], hit0, bv);
		lower -= bv;
	    }
	    if (cbounds[c1] > hit1) { // right edge bin
		ibis::bitvector bv;
		sumBins(hit1, cbounds[c1], bv);
		lower -= bv;
	    }
	}
    }

    if (cand0 < hit0 || (cand1 > hit1 && hit1 < nobs)) {
	upper.copy(lower);
	if (cand0 < hit0) {
	    if (bits[cand0] == 0)
		activate(cand0);
	    if (bits[cand0] != 0)
		upper |= *(bits[cand0]);
	}
	if (cand1 > hit1 && hit1 < nobs) {
	    if (bits[hit1] == 0)
		activate(hit1);
	    if (bits[hit1] != 0)
		upper |= *(bits[hit1]);
	}
    }
    else {
	upper.clear();
    }
} // ibis::fuge::estimate

// fill the offsets array, and divide the bitmaps into groups according to
// the sizes (bytes) of the bitmaps
void ibis::fuge::coarsen() {
    const uint32_t nbits = bits.size();
    if (offsets.size() != nbits+1) {
	offsets.resize(nbits+1);
	offsets[0] = 0;
	for (unsigned i = 0; i < nbits; ++ i)
	    offsets[i+1] = offsets[i] + (bits[i] ? bits[i]->bytes() : 0U);
    }
    if (nobs < 32) return; // don't construct the coarse level
    if (cbits.size() > 0 && cbits.size()+1 == coffsets.size()) return;

    // default size based on the size of fine level index sf: sf(w-1)/N/sqrt(2)
    unsigned ncoarse = 0;
    if (col != 0) { // limit the scope of variables
	const char* spec = col->indexSpec();
	if (spec != 0 && *spec != 0 && strstr(spec, "ncoarse=") != 0) {
	    // number of coarse bins specified explicitly
	    const char* tmp = 8+strstr(spec, "ncoarse=");
	    unsigned j = atoi(tmp);
	    if (j > 4)
		ncoarse = j;
	}
    }
    if (ncoarse < 5 && offsets.back() > offsets[0]+nrows/31) {
	ncoarse = sizeof(ibis::bitvector::word_t);
	const int wm1 = ncoarse*8 - 1;
	const long sf = (offsets.back()-offsets[0]) / ncoarse;
	ncoarse = static_cast<unsigned>(wm1*sf/(sqrt(2.0)*nrows));
	const unsigned ncmax = (unsigned) sqrt(2.0 * nobs);
	if (ncoarse < ncmax) {
	    const double obj1 = (sf+(ncoarse+1-ceil(0.5*ncoarse))*nrows/wm1)
		*(sf*0.5/ncoarse+2.0*nrows/wm1);
	    const double obj2 = (sf+(ncoarse+2-ceil(0.5*ncoarse+0.5))*nrows/wm1)
		*(sf*0.5/(ncoarse+1.0)+2.0*nrows/wm1);
	    ncoarse += (obj2 < obj1);
	}
	else {
	    ncoarse = ncmax;
	}
    }
    if (ncoarse < 5 || ncoarse >= nobs) return;

    const uint32_t nc2 = (ncoarse + 1) / 2;
    const uint32_t ncb = ncoarse - nc2 + 1; // # of coarse level bitmaps
    // partition the fine level bitmaps into groups with nearly equal
    // number of bytes
    cbounds.resize(ncoarse+1);
    cbounds[0] = 0;
    for (unsigned i = 1; i < ncoarse; ++ i) {
	int32_t target = offsets[cbounds[i-1]] +
	    (offsets.back() - offsets[cbounds[i-1]]) / (ncoarse - i + 1);
	cbounds[i] = offsets.find(target);
	if (cbounds[i] > cbounds[i-1]+1 &&
	    offsets[cbounds[i]]-target > target-offsets[cbounds[i]-1])
	    -- (cbounds[i]);
	else if (cbounds[i] <= cbounds[i-1])
	    cbounds[i] = cbounds[i-1]+1;
    }
    cbounds[ncoarse] = nbits; // end with the last fine level bitmap
    for (unsigned i = ncoarse-1; i > 0 && cbounds[i+1] < cbounds[i]; -- i)
	cbounds[i] = cbounds[i+1] - 1;
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg(2);
	lg.buffer() << "ibis::fuge::coarsen will divide " << bits.size()
		  << " bitmaps into " << ncoarse << " groups\n";
	for (unsigned i = 0; i < cbounds.size(); ++ i)
	    lg.buffer() << cbounds[i] << " ";
	lg.buffer() << "\n";
    }
    // fill cbits
    for (unsigned i = 0; i < cbits.size(); ++ i) {
	delete cbits[i];
	cbits[i] = 0;
    }
    cbits.resize(ncb);
    cbits[0] = new ibis::bitvector();
    sumBins(0, cbounds[nc2], *(cbits[0]));
    for (unsigned i = 1; i < ncb; ++ i) {
	ibis::bitvector front, back;
	sumBins(cbounds[i-1], cbounds[i], front);
	sumBins(cbounds[i-1+nc2], cbounds[i+nc2], back);
	cbits[i] = new ibis::bitvector(*(cbits[i-1]));
	*(cbits[i]) -= front;
	*(cbits[i]) |= back;
    }

    // fill coffsets
    coffsets.resize(ncb+1);
    coffsets[0] = 0;
    for (unsigned i = 0; i < ncb; ++ i) {
	cbits[i]->compress();
	coffsets[i+1] = coffsets[i] + cbits[i]->bytes();
    }
} // ibis::fuge::coarsen

// This function is intended to be called after calling ibis::bin::write,
// however, it does not check for this fact!
int ibis::fuge::writeCoarse(int fdes) const {
    if (cbounds.empty() || cbits.empty() || nrows == 0)
	return -4;

    int32_t ierr;
    const uint32_t nc = cbounds.size()-1;
    const uint32_t nb = cbits.size();
    array_t<int32_t> offs(nb+1);
    ierr = UnixWrite(fdes, &nc, sizeof(nc));
    ierr = UnixWrite(fdes, cbounds.begin(), sizeof(uint32_t)*(nc+1));
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nb+1), SEEK_CUR);
    for (unsigned i = 0; i < nb; ++ i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	if (cbits[i] != 0)
	    cbits[i]->write(fdes);
    }
    offs[nb] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr -= sizeof(int32_t) * (nb+1);
    UnixSeek(fdes, ierr, SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nb+1));
    ierr = UnixSeek(fdes, offs.back(), SEEK_SET);
    return 0;
} // ibis::fuge::writeCoarse

// Reading information about the coarse bins.  To be used after calling
// ibis::relic::read, which happens in the constructor.
int ibis::fuge::readCoarse(const char* fn) {
    std::string fnm;
    indexFileName(fn, fnm);

    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0) return -1;
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    long ierr = UnixSeek(fdes, offsets.back(), SEEK_SET);
    if (ierr == offsets.back()) {
	uint32_t nc, begin, end;
	ierr = UnixRead(fdes, &nc, sizeof(nc));
	if (ierr <= 0 || static_cast<uint32_t>(ierr) != sizeof(nc)) {
	    UnixClose(fdes);
	    return -2;
	}

	begin = offsets.back() + sizeof(nc);
	end = begin + sizeof(uint32_t)*(nc+1);
	if (ierr > 0 && nc > 0) {
	    array_t<uint32_t> tmp(fdes, begin, end);
	    cbounds.swap(tmp);
	}
	const uint32_t ncb = nc+1-(nc+1)/2;
	begin = end;
	end += sizeof(int32_t) * (ncb+1);
	if (cbounds.size() == nc+1) {
	    array_t<int32_t> tmp(fdes, begin, end);
	    coffsets.swap(tmp);
	}

	for (unsigned i = 0; i < cbits.size(); ++ i)
	    delete cbits[i];
	cbits.resize(ncb);
	for (unsigned i = 0; i < ncb; ++ i)
	    cbits[i] = 0;
    }
    (void) UnixClose(fdes);

    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' coarse bin info "
			"from %s", name(), fnm.c_str());
    return 0;
} // ibis::fuge::readCoarse

void ibis::fuge::clearCoarse() {
    const unsigned nb = cbits.size();
    for (unsigned i = 0; i < nb; ++ i)
	delete cbits[i];

    cbits.clear();
    cbounds.clear();
    coffsets.clear();
} // ibis::fuge::clearCoarse

void ibis::fuge::activateCoarse() const {
    const uint32_t nobs = cbits.size();
    bool missing = false; // any bits[i] missing (is 0)?
    ibis::column::mutexLock lock(col, "fuge::activateCoarse");
    for (uint32_t i = 0; i < nobs && ! missing; ++ i)
	missing = (cbits[i] == 0);
    if (missing == false) return;

    if (coffsets.size() <= nobs || coffsets[0] <= offsets.back()) {
	col->logWarning("fuge::activateCoarse", "no records of coffsets, "
			"can not regenerate the bitvectors");
    }
    else if (str) { // using a ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose >= 9)
	    << "ibis::column[" << col->name()
	    << "]::fuge::activateCoarse(all) "
	    << "retrieving data from ibis::fileManager::storage(0x"
	    << str << ")";

	for (uint32_t i = 0; i < nobs; ++i) {
	    if (cbits[i] == 0 && coffsets[i+1] > coffsets[i]) {
#if defined(DEBUG)
		LOGGER(ibis::gVerbose >= 0)
		    << "fuge::activateCoarse -- activating bitvector "
		    << i << " from a raw storage ("
		    << static_cast<const void*>(str->begin())
		    << "), coffsets[" << i << "]= " << coffsets[i]
		    << ", coffsets[" << i+1 << "]= " << coffsets[i+1];
#endif
		array_t<ibis::bitvector::word_t>
		    a(str, coffsets[i], (coffsets[i+1]-coffsets[i]) /
		      sizeof(ibis::bitvector::word_t));
		cbits[i] = new ibis::bitvector(a);
		cbits[i]->setSize(nrows);
	    }
	}
    }
    else if (fname) { // using the named file directly
	int fdes = UnixOpen(fname, OPEN_READONLY);
	if (fdes >= 0) {
	    LOGGER(ibis::gVerbose >= 9)
		<< "ibis::column[" << col->name()
		<< "]::fuge::activateCoarse(all) "
		<< "retrieving data from file \"" << fname << "\"";

#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fdes, _O_BINARY);
#endif
	    uint32_t i = 0;
	    while (i < nobs) {
		// skip to next empty bit vector
		while (i < nobs && cbits[i] != 0)
		    ++ i;
		// the last bitvector to activate. can not be larger
		// than j
		uint32_t aj = (i<nobs ? i + 1 : nobs);
		while (aj < nobs && cbits[aj] == 0)
		    ++ aj;
		if (coffsets[aj] > coffsets[i]) {
		    const uint32_t start = coffsets[i];
		    ibis::fileManager::storage *a0 = new
			ibis::fileManager::storage(fdes, start,
						   coffsets[aj]);
		    while (i < aj) {
			if (coffsets[i+1] > coffsets[i]) {
			    array_t<ibis::bitvector::word_t>
				a1(a0, coffsets[i]-start,
				   (coffsets[i+1]-coffsets[i])/
				   sizeof(ibis::bitvector::word_t));
			    bits[i] = new ibis::bitvector(a1);
			    bits[i]->setSize(nrows);
#if defined(DEBUG)
			    LOGGER(ibis::gVerbose >= 0)
				<< "fuge::activateCoarse -- "
				"activating bitvector " << i
				<< "by reading file " << fname
				<< "coffsets[" << i << "]= " << coffsets[i]
				<< ", coffsets[" << i+1 << "]= "
				<< coffsets[i+1];
#endif
			}
			++ i;
		    }
		}
		i = aj; // always advance i
	    }
	    UnixClose(fdes);
	}
	else {
	    col->logWarning("fuge::activateCoarse", "failed to open file \"%s\""
			    " ... %s", fname, strerror(errno));
	}
    }
    else {
	col->logWarning("fuge::activateCoarse", "can not regenerate "
			"bitvectors because neither str or fname is "
			"specified");
    }
} // ibis::fuge::activateCoarse

void ibis::fuge::activateCoarse(uint32_t i) const {
    if (i >= bits.size()) return;	// index out of range
    if (cbits[i] != 0) return;	// already active
    ibis::column::mutexLock lock(col, "fuge::activateCoarse");
    if (cbits[i] != 0) return;	// already active
    if (coffsets.size() <= cbits.size() || coffsets[0] <= offsets.back()) {
	col->logWarning("fuge::activateCoarse", "no records of offsets, "
			"can not regenerate bitvector %lu",
			static_cast<long unsigned>(i));
    }
    else if (coffsets[i+1] <= coffsets[i]) {
	return;
    }
    if (str) { // using a ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose >= 9)
	    << "ibis::column[" << col->name()
	    << "]::fuge::activateCoarse(" << i
	    << ") retrieving data from ibis::fileManager::storage(0x"
	    << str << ")";

	array_t<ibis::bitvector::word_t>
	    a(str, coffsets[i], (coffsets[i+1]-coffsets[i]) /
	      sizeof(ibis::bitvector::word_t));
	cbits[i] = new ibis::bitvector(a);
	cbits[i]->setSize(nrows);
#if defined(DEBUG)
	LOGGER(ibis::gVerbose >= 0)
	    << "fuge::activateCoarse(" << i
	    << ") constructed a bitvector from range ["
	    << coffsets[i] << ", " << coffsets[i+1] << ") of a storage at "
	    << static_cast<const void*>(str->begin());
#endif
    }
    else if (fname) { // using the named file directly
	int fdes = UnixOpen(fname, OPEN_READONLY);
	if (fdes >= 0) {
	    LOGGER(ibis::gVerbose >= 9)
		<< "ibis::column[" << col->name()
		<< "]::fuge::activateCoarse(" << i
		<< ") retrieving data from file \"" << fname << "\"";

#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fdes, _O_BINARY);
#endif
	    array_t<ibis::bitvector::word_t> a0(fdes, coffsets[i],
						coffsets[i+1]);
	    cbits[i] = new ibis::bitvector(a0);
	    cbits[i]->setSize(nrows);
	    UnixClose(fdes);
#if defined(DEBUG)
	    LOGGER(ibis::gVerbose >= 0)
		<< "fuge::activateCoarse(" << i
		<< ") constructed a bitvector from range ["
		<< coffsets[i] << ", " << coffsets[i+1] << ") of file "
		<< fname;
#endif
	}
	else {
	    col->logWarning("fuge::activateCoarse",
			    "failed to open file \"%s\" ... %s",
			    fname, strerror(errno));
	}
    }
    else {
	col->logWarning("fuge::activateCoarse", "can not regenerate "
			"bitvector %lu because neither str or fname is "
			"specified", static_cast<long unsigned>(i));
    }
} // ibis::fuge::activateCoarse

void ibis::fuge::activateCoarse(uint32_t i, uint32_t j) const {
    if (j > cbits.size())
	j = cbits.size();
    if (i >= j) // empty range
	return;
    ibis::column::mutexLock lock(col, "fuge::activateCoarse");

    while (i < j && cbits[i] != 0) ++ i;
    if (i >= j) return; // all bitvectors active

    if (coffsets.size() <= cbits.size() || coffsets[0] <= offsets.back()) {
	col->logWarning("fuge::activateCoarse", "no records of offsets, "
			"can not regenerate bitvectors %lu:%lu",
			static_cast<long unsigned>(i),
			static_cast<long unsigned>(j));
    }
    else if (str) { // using an ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose >= 9)
	    << "ibis::column[" << col->name()
	    << "]::fuge::activateCoarse(" << i << ", " << j
	    << ") retrieving data from ibis::fileManager::storage(0x"
	    << str << ")";

	while (i < j) {
	    if (cbits[i] == 0 && coffsets[i+1] > coffsets[i]) {
		array_t<ibis::bitvector::word_t>
		    a(str, coffsets[i], (coffsets[i+1]-coffsets[i]) /
		      sizeof(ibis::bitvector::word_t));
		cbits[i] = new ibis::bitvector(a);
		cbits[i]->setSize(nrows);
#if defined(DEBUG)
		LOGGER(ibis::gVerbose >= 0)
		    << "fuge::activateCoarse(" << i
		    << ") constructed a bitvector from range ["
		    << coffsets[i] << ", " << coffsets[i+1]
		    << ") of a storage at "
		    << static_cast<const void*>(str->begin());
#endif
	    }
	    ++ i;
	}
    }
    else if (fname) { // using the named file directly
	if (coffsets[j] > coffsets[i]) {
	    int fdes = UnixOpen(fname, OPEN_READONLY);
	    if (fdes >= 0) {
		LOGGER(ibis::gVerbose >= 9)
		    << "ibis::column[" << col->name()
		    << "]::fuge::activateCoarse(" << i << ", " << j
		    << ") retrieving data from file \"" << fname << "\"";

#if defined(_WIN32) && defined(_MSC_VER)
		(void)_setmode(fdes, _O_BINARY);
#endif
		while (i < j) {
		    // skip to next empty bit vector
		    while (i < j && cbits[i] != 0)
			++ i;
		    // the last bitvector to activate. can not be larger
		    // than j
		    uint32_t aj = (i<j ? i + 1 : j);
		    while (aj < j && cbits[aj] == 0)
			++ aj;
		    if (coffsets[aj] > coffsets[i]) {
			const uint32_t start = coffsets[i];
			ibis::fileManager::storage *a0 = new
			    ibis::fileManager::storage(fdes, start,
						       coffsets[aj]);
			while (i < aj) {
			    if (coffsets[i+1] > coffsets[i]) {
				array_t<ibis::bitvector::word_t>
				    a1(a0, coffsets[i]-start,
				       (coffsets[i+1]-coffsets[i])/
				       sizeof(ibis::bitvector::word_t));
				cbits[i] = new ibis::bitvector(a1);
				cbits[i]->setSize(nrows);
#if defined(DEBUG)
				LOGGER(ibis::gVerbose >= 0)
				    << "fuge::activateCoarse(" << i
				    << ") constructed a bitvector "
				    "from range ["
				    << coffsets[i] << ", " << coffsets[i+1]
				    << ") of file " << fname;
#endif
			    }
			    ++ i;
			}
		    }
		    i = aj; // always advance i
		}
		UnixClose(fdes);
	    }
	    else {
		col->logWarning("fuge::activateCoarse",
				"failed to open file \"%s\" ... %s",
				fname, strerror(errno));
	    }
	}
    }
    else {
	col->logWarning("fuge::activateCoarse", "can not regenerate "
			"bitvector %lu because neither str or fname is "
			"specified", static_cast<long unsigned>(i));
    }
} // ibis::fuge::activateCoarse

long ibis::fuge::coarseEstimate(uint32_t lo, uint32_t hi) const {
    long cost;
    const unsigned mid = cbounds.size() / 2;
    if (lo >= cbounds.size() || lo >= hi) {
	cost = 0;
    }
    else if (hi > mid) {
	cost = coffsets[hi-mid+1] - coffsets[hi-mid];
	if (lo > hi-mid) {
	    if (lo >= mid)
		cost += coffsets[lo-mid+1] - coffsets[lo-mid];
	    else
		cost += coffsets[lo+1] - coffsets[lo];
	}
	else if (lo < hi-mid) {
	    cost += coffsets[lo+1] - coffsets[lo];
	}
    }
    else if (hi < mid) {
	cost = (coffsets[lo+1] - coffsets[lo])
	    + (coffsets[hi+1] - coffsets[hi]);
    }
    else { // hi == mid
	cost = coffsets[1] - coffsets[0];
	if (lo > 0) {
	    cost += coffsets[lo+1] - coffsets[lo];
	}
    }
    return cost;
} // ibis::fuge::coarseEstimate

long ibis::fuge::coarseEvaluate(uint32_t lo, uint32_t hi,
				ibis::bitvector &res) const {
    const unsigned mid = cbounds.size() / 2;
    if (lo >= cbounds.size() || lo >= hi) {
	res.set(0, nrows);
    }
    else if (lo+1 == hi) { // two consecutive bitmaps used
	if (hi < cbits.size()) {
	    activateCoarse(lo, hi+1);
	    if (cbits[lo] != 0) {
		res.copy(*(cbits[lo]));
		if (cbits[hi] != 0)
		    res -= *(cbits[hi]);
	    }
	    else {
		res.set(0, nrows);
	    }
	}
	else {
	    activateCoarse(lo-mid, lo-mid+2);
	    if (cbits[hi-mid] != 0) {
		res.copy(*(cbits[hi-mid]));
		if (cbits[lo-mid] != 0)
		    res -= *(cbits[lo-mid]);
	    }
	    else {
		res.set(0, nrows);
	    }
	}
    }
    else if (hi > mid) {
	if (cbits[hi-mid] == 0)
	    activateCoarse(hi-mid);
	if (cbits[hi-mid] != 0)
	    res.copy(*(cbits[hi-mid]));
	else
	    res.set(0, nrows);
	if (lo > hi-mid) {
	    if (lo >= mid) {
		if (cbits[lo-mid] == 0)
		    activateCoarse(lo-mid);
		if (cbits[lo-mid] != 0)
		    res -= *(cbits[lo-mid]);
	    }
	    else {
		if (cbits[lo] == 0)
		    activateCoarse(lo);
		if (cbits[lo] != 0)
		    res &= *(cbits[lo]);
		else
		    res.set(0, nrows);
	    }
	}
	else if (lo < hi-mid) {
	    if (cbits[lo] == 0)
		activateCoarse(lo);
	    if (cbits[lo] != 0)
		res |= *(cbits[lo]);
	}
    }
    else if (hi < mid) {
	if (cbits[lo] == 0)
	    activateCoarse(lo);
	if (cbits[lo] != 0) {
	    res.copy(*(cbits[lo]));
	    if (cbits[hi] == 0)
		activateCoarse(hi);
	    if (cbits[hi] != 0)
		res -= *(cbits[hi]);
	}
	else {
	    res.set(0, nrows);
	}
    }
    else { // hi == mid
	if (cbits[0] == 0)
	    activateCoarse(0);
	if (cbits[0] != 0)
	    res.copy(*(cbits[0]));
	else
	    res.set(0, nrows);
	if (lo > 0) {
	    if (cbits[lo] == 0)
		activateCoarse(lo);
	    if (cbits[lo] != 0)
		res &= *(cbits[lo]);
	}
    }
    return res.size();
} // ibis::fuge::coarseEvaluate

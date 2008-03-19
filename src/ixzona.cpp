// $Id$
// Author: John Wu <John.Wu@ACM.org>
// Copyright 2007-2008 the Regents of the University of California
//
// This file contains the implementation of the class ibis::zona, a
// unbinned version of equality-equality encoded index.
//
// The word 'zona' is a Danish translation of English word zone.  The class
// ibis::pack implements the binned version of equality-equality encoding.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifiers longer than 256 characters
#endif
#include "irelic.h"
#include "part.h"

#include <cmath>	// std::fabs
#include <sstream> // std::ostringstream

////////////////////////////////////////////////////////////////////////
ibis::zona::zona(const ibis::column *c, const char *f)
    : ibis::relic(c, f) {
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
} // ibis::zona::zona

/// Reconstruct from content of fileManager::storage.
/**
   The leading portion of the index file is the same as ibis::relic, which
   allows the constructor of the base class to work properly.  The content
   following the last bitvector in ibis::relic is as follows, @sa
   ibis::zona::writeCoarse.

   nc      (uint32_t)       -- number of coarse bins.
   cbounds (unsigned[nc+1]) -- boundaries of the coarse bins.
   coffsets(int32_t[nc+1])  -- starting position of the coarse level bitmaps.
   cbits   (bitvector[nc])  -- bitvector laid out one after another.
 */
ibis::zona::zona(const ibis::column* c, ibis::fileManager::storage* st,
		 uint32_t start) : ibis::relic(c, st, start) {
    if (st->size() <= static_cast<uint32_t>(offsets.back()))
	return; // no coarse bin

    start = offsets.back();
    uint32_t nc = *(reinterpret_cast<uint32_t*>(st->begin()+start));
    if (nc == 0 ||
	st->size() <= start + (sizeof(int32_t)+sizeof(uint32_t))*(nc+1))
	return;

    start += sizeof(uint32_t);
    if (start+sizeof(uint32_t)*(nc+1) < st->size()) {
	array_t<uint32_t> tmp(st, start, nc+1);
	cbounds.swap(tmp);
    }
    start += sizeof(uint32_t) * (nc+1);
    if (start+sizeof(int32_t)*(nc+1) < st->size()) {
	array_t<int32_t> tmp(st, start, nc+1);
	coffsets.swap(tmp);
	if (coffsets.back() > static_cast<int32_t>(st->size())) {
	    coffsets.swap(tmp);
	    array_t<uint32_t> tmp2;
	    cbounds.swap(tmp2);
	    return;
	}
    }
    else {
	array_t<uint32_t> tmp;
	cbounds.swap(tmp);
	return;
    }

    cbits.resize(nc);
    for (unsigned i = 0; i < nc; ++ i)
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
	for (unsigned i = 0; i < nc; ++ i) {
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
	ibis::util::logger lg(4);
	print(lg.buffer());
    }
} // ibis::zona::zona

long ibis::zona::append(const char* dt, const char* df, uint32_t nnew) {
    long ret = ibis::relic::append(dt, df, nnew);
    if (ret <= 0 || static_cast<uint32_t>(ret) != nnew)
	return ret;

    coarsen();
    return ret;
}

void ibis::zona::coarsen() {
    const uint32_t ncoarse = 11; // default number of coarse bins (w=64, 16)
    const uint32_t nbits = bits.size();

    if (offsets.size() != nbits+1) {
	offsets.resize(nbits+1);
	offsets[0] = 0;
	for (unsigned i = 0; i < nbits; ++ i)
	    offsets[i+1] = offsets[i] + (bits[i] ? bits[i]->bytes() : 0U);
    }
    if (vals.size() < 32) return; // don't construct the coarse level
    if (cbits.size() > 0 && cbits.size()+1 == coffsets.size()) return;

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
	    cbounds[i] = cbounds[i-1] + 1;
    }
    cbounds[ncoarse] = nbits; // end with the last fine level bitmap
    for (unsigned i = ncoarse-1; i > 0 && cbounds[i+1] < cbounds[i]; -- i)
	cbounds[i] = cbounds[i+1] - 1;

    // fill cbits
    cbits.reserve(ncoarse);
    for (unsigned i = 0; i < cbits.size(); ++ i) {
	delete cbits[i];
	cbits[i] = 0;
    }
    for (unsigned i = 0; i < ncoarse; ++ i) {
	// generate a new bitmap for each coarse bin, even if it only
	// contains one fine level bitmap
	ibis::bitvector tmp;
	sumBits(cbounds[i], cbounds[i+1], tmp);
	cbits.push_back(new ibis::bitvector(tmp));
    }

    // fill coffsets
    coffsets.resize(ncoarse+1);
    coffsets[0] = 0;
    for (unsigned i = 0; i < ncoarse; ++ i) {
	cbits[i]->compress();
	coffsets[i+1] = coffsets[i] + cbits[i]->bytes();
    }
} // ibis::zona::coarsen

void ibis::zona::activateCoarse() const {
    const uint32_t nobs = cbits.size();
    bool missing = false; // any bits[i] missing (is 0)?
    ibis::column::mutexLock lock(col, "zona::activateCoarse");
    for (uint32_t i = 0; i < nobs && ! missing; ++ i)
	missing = (cbits[i] == 0);
    if (missing == false) return;

    if (coffsets.size() <= nobs || coffsets[0] <= offsets.back()) {
	col->logWarning("zona::activateCoarse", "no records of coffsets, "
			"can not regenerate the bitvectors");
    }
    else if (str) { // using a ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose >= 9) << "ibis::column[" << col->name()
		  << "]::zona::activateCoarse(all) "
		  << "retrieving data from ibis::fileManager::storage(0x"
		  << str << ")";

	for (uint32_t i = 0; i < nobs; ++i) {
	    if (cbits[i] == 0 && coffsets[i+1] > coffsets[i]) {
#if defined(DEBUG)
		LOGGER(ibis::gVerbose >= 0)
		    << "zona::activateCoarse -- activating bitvector "
		    << i << " from a raw storage (0x"
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
	    LOGGER(ibis::gVerbose >= 9) << "ibis::column[" << col->name()
		      << "]::zona::activateCoarse(all) "
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
				<< "zona::activateCoarse -- "
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
	    col->logWarning("zona::activateCoarse", "failed to open file \"%s\""
			    " ... %s", fname, strerror(errno));
	}
    }
    else {
	col->logWarning("zona::activateCoarse", "can not regenerate "
			"bitvectors because neither str or fname is "
			"specified");
    }
} // ibis::zona::activateCoarse

void ibis::zona::activateCoarse(uint32_t i) const {
    if (i >= bits.size()) return;	// index out of range
    if (cbits[i] != 0) return;	// already active
    ibis::column::mutexLock lock(col, "zona::activateCoarse");
    if (cbits[i] != 0) return;	// already active
    if (coffsets.size() <= cbits.size() || coffsets[0] <= offsets.back()) {
	col->logWarning("zona::activateCoarse", "no records of offsets, "
			"can not regenerate bitvector %lu",
			static_cast<long unsigned>(i));
    }
    else if (coffsets[i+1] <= coffsets[i]) {
	return;
    }
    if (str) { // using a ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose >= 9) << "ibis::column[" << col->name()
		  << "]::zona::activateCoarse(" << i
		  << ") retrieving data from ibis::fileManager::storage(0x"
		  << str << ")";

	array_t<ibis::bitvector::word_t>
	    a(str, coffsets[i], (coffsets[i+1]-coffsets[i]) /
	      sizeof(ibis::bitvector::word_t));
	cbits[i] = new ibis::bitvector(a);
	cbits[i]->setSize(nrows);
#if defined(DEBUG)
	LOGGER(ibis::gVerbose >= 0)
	    << "zona::activateCoarse(" << i
	    << ") constructed a bitvector from range ["
	    << coffsets[i] << ", " << coffsets[i+1] << ") of a storage at "
	    << static_cast<const void*>(str->begin());
#endif
    }
    else if (fname) { // using the named file directly
	int fdes = UnixOpen(fname, OPEN_READONLY);
	if (fdes >= 0) {
	    LOGGER(ibis::gVerbose >= 9) << "ibis::column[" << col->name()
		      << "]::zona::activateCoarse(" << i
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
		<< "zona::activateCoarse(" << i
		<< ") constructed a bitvector from range ["
		<< coffsets[i] << ", " << coffsets[i+1]
		<< ") of file " << fname;
#endif
	}
	else {
	    col->logWarning("zona::activateCoarse",
			    "failed to open file \"%s\" ... %s",
			    fname, strerror(errno));
	}
    }
    else {
	col->logWarning("zona::activateCoarse", "can not regenerate "
			"bitvector %lu because neither str or fname is "
			"specified", static_cast<long unsigned>(i));
    }
} // ibis::zona::activateCoarse

void ibis::zona::activateCoarse(uint32_t i, uint32_t j) const {
    if (j > cbits.size())
	j = cbits.size();
    if (i >= j) // empty range
	return;
    ibis::column::mutexLock lock(col, "zona::activateCoarse");

    while (i < j && cbits[i] != 0) ++ i;
    if (i >= j) return; // all bitvectors active

    if (coffsets.size() <= cbits.size() || coffsets[0] <= offsets.back()) {
	col->logWarning("zona::activateCoarse", "no records of offsets, "
			"can not regenerate bitvectors %lu:%lu",
			static_cast<long unsigned>(i),
			static_cast<long unsigned>(j));
    }
    else if (str) { // using an ibis::fileManager::storage as back store
	LOGGER(ibis::gVerbose >= 9) << "ibis::column[" << col->name()
		  << "]::zona::activateCoarse(" << i << ", " << j
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
		    << "zona::activateCoarse(" << i << ", " << j
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
		LOGGER(ibis::gVerbose >= 9) << "ibis::column[" << col->name()
			  << "]::zona::activateCoarse(" << i << ", " << j
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
				    << "zona::activateCoarse(" << i
				    << ") constructed a bitvector from range ["
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
		col->logWarning("zona::activateCoarse",
				"failed to open file \"%s\" ... %s",
				fname, strerror(errno));
	    }
	}
    }
    else {
	col->logWarning("zona::activateCoarse", "can not regenerate "
			"bitvector %lu because neither str or fname is "
			"specified", static_cast<long unsigned>(i));
    }
} // ibis::zona::activateCoarse

uint32_t ibis::zona::estimate(const ibis::qContinuousRange& expr) const {
    ibis::bitvector bv;
    long ierr = evaluate(expr, bv);
    return static_cast<uint32_t>(ierr > 0 ? ierr : 0);
} // ibis::zona::estimate

double ibis::zona::estimateCost(const ibis::qContinuousRange& expr) const {
    double res = static_cast<double>(col->elementSize() * nrows);
    if (bits.empty()) {
	return res;
    }

    // values in the range [hit0, hit1) satisfy the query
    uint32_t hit0, hit1;
    locate(expr, hit0, hit1);

    const uint32_t ncoarse = (cbits.empty() || cbounds.empty() ? 0U :
			      cbits.size()+1 <= cbounds.size() ?
			      cbits.size() : cbounds.size()-1);
    const long fine = offsets[hit1] - offsets[hit0] <=
	(offsets.back() - offsets[hit1]) + (offsets[hit0] - offsets[0])
	? offsets[hit1] - offsets[hit0] :
	(offsets.back() - offsets[hit1]) + (offsets[hit0] - offsets[0]);
    if (hit1 <= hit0) {
	res = 0.0;
	return res;
    }
    if (hit0 == 0 && hit1 >= bits.size()) {
	res = 0.0;
	return res;
    }

    if (hit0+1 == hit1) { // equality condition
	res = fine;
	return res;
    }
    if (hit0+3 >= hit1 || offsets.size() < bits.size()) {
	res = fine;
	return res;
    }

    // see whether the coarse bins could help
    const long csize = coffsets.back() - coffsets[0];
    const uint32_t c0 = cbounds.find(hit0);
    const uint32_t c1 = cbounds.find(hit1);
    if (c0 >= c1) { // within the same coarse bin
	long tmp = coffsets[c0] - coffsets[c0-1]
	    + offsets[cbounds[c0]] - offsets[cbounds[c0-1]] - fine;
	res = fine;
	if (static_cast<long>(0.99*fine) > tmp)
	    res = tmp;
    }
    else if (c1 < ncoarse && cbounds[c1] == hit1 && cbounds[c0] == hit0) {
	// need coarse bins only
	const long tmp = coffsets[c1] - coffsets[c0];
	res = (2*tmp <= csize ? tmp : csize - tmp);
    }
    else {// general case: evaluate 10 options as 5 pairs
	// pair 2: [direct | - | direct] (coarse bin evaluation takes
	// into account of possibly using complement)
	long tmp = coffsets[c1-1] - coffsets[c0];
	long cost = (offsets[cbounds[c0]] - offsets[hit0])
	    + (tmp+tmp <= csize ? tmp : csize-tmp)
	    + (offsets[hit1] - offsets[cbounds[c1-1]]);
	// pair 3: [complement | - | direct]
	if (c0 > 0) {
	    tmp = coffsets[c1-1] - coffsets[c0-1];
	    tmp = (offsets[hit0] - offsets[cbounds[c0-1]])
		+ (tmp+tmp <= csize ? tmp : csize-tmp)
		+ (offsets[hit1] - offsets[cbounds[c1-1]]);
	    if (tmp < cost) {
		cost = tmp;
	    }
	}
	// pair 4: [direct | - | complement]
	tmp = coffsets[c1] - coffsets[c0];
	tmp = (offsets[cbounds[c0]] - offsets[hit0])
	    + (tmp+tmp <= csize ? tmp : csize-tmp)
	    + (offsets[cbounds[c1]] - offsets[hit1]);
	if (tmp < cost) {
	    cost = tmp;
	}
	// pair 5: [complement | - | complement]
	if (c0 > 0) {
	    tmp = coffsets[c1] - coffsets[c0-1];
	    tmp = (offsets[hit0] - offsets[cbounds[c0-1]])
		+ (tmp+tmp <= csize ? tmp : csize-tmp)
		+ (offsets[cbounds[c1]] - offsets[hit1]);
	    if (tmp < cost) {
		cost = tmp;
	    }
	}
	// pair 1: fine level only
	if (cost > static_cast<long>(0.99*fine)) { // slightly prefer 1
	    cost = fine;
	}
	res = cost;
    }
    return res;
} // ibis::zona::estimateCost

// Compute the hits as a @c bitvector.
long ibis::zona::evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower) const {
    if (bits.empty()) { // empty index
	lower.set(0, nrows);
	return 0L;
    }

    // values in the range [hit0, hit1) satisfy the query
    uint32_t hit0, hit1;
    locate(expr, hit0, hit1);
    if (hit1 <= hit0) {
	lower.set(0, nrows);
	return 0L;
    }
    if (hit0 == 0 && hit1 >= bits.size()) {
	col->getNullMask(lower);
	return lower.cnt();
    }

    if (hit0+1 == hit1) { // equality condition
	if (bits[hit0] == 0)
	    activate(hit0);
	if (bits[hit0] != 0)
	    lower.copy(*(bits[hit0]));
	else
	    lower.set(0, nrows);
	return lower.cnt();
    }
    const uint32_t ncoarse = (cbits.empty() || cbounds.empty() ? 0U :
			      cbits.size()+1 <= cbounds.size() ?
			      cbits.size() : cbounds.size()-1);
    if (hit0+3 >= hit1 || offsets.size() < bits.size() || ncoarse == 0) {
	// no more than three bitmaps involved, or don't know the sizes
	sumBits(hit0, hit1, lower);
	return lower.cnt();
    }

    // see whether the coarse bins could help
    const uint32_t c0 = cbounds.find(hit0);
    const uint32_t c1 = cbounds.find(hit1);
    if (ibis::gVerbose > 4) {
	ibis::util::logger lg(4);
	lg.buffer() << "ibis::zona::evaluate(" << expr << ") hit0=" << hit0
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
	long fine = offsets[hit1] - offsets[hit0];
	long tmp = coffsets[c0] - coffsets[c0-1]
	    + offsets[cbounds[c0]] - offsets[cbounds[c0-1]] - fine;
	if (static_cast<long>(0.99*fine) <= tmp) {
	    sumBits(hit0, hit1, lower);
	}
	else {
	    activateCoarse(c0-1);
	    if (cbits[c0-1])
		lower.copy(*(cbits[c0-1]));
	    else
		col->getNullMask(lower);
	    if (hit0 > cbounds[c0-1]) {
		ibis::bitvector bv;
		sumBits(cbounds[c0-1], hit0, bv);
		lower -= bv;
	    }
	    if (hit1 < cbounds[c0]) {
		ibis::bitvector bv;
		sumBits(hit1, cbounds[c0], bv);
		lower -= bv;
	    }
	}
    }
    else {// general case: evaluate 10 options as 5 pairs
	const long csize = coffsets.back() - coffsets[0];
	unsigned option = 2; // pair 2 [direct | - | direct]
	long tmp = coffsets[c1-1] - coffsets[c0];
	long cost = (offsets[cbounds[c0]] - offsets[hit0])
	    + ((tmp+tmp) <= csize ? tmp : csize - tmp)
	    + (offsets[hit1] - offsets[cbounds[c1-1]]);
	// pair 3: [complement | - | direct]
	if (c0 > 0) {
	    tmp = coffsets[c1-1] - coffsets[c0-1];
	    tmp = (offsets[hit0] - offsets[cbounds[c0-1]])
		+ ((tmp+tmp) <= csize ? tmp : csize - tmp)
		+ (offsets[hit1] - offsets[cbounds[c1-1]]);
	    if (tmp < cost) {
		cost = tmp;
		option = 3;
	    }
	}
	// pair 4: [direct | - | complement]
	tmp = coffsets[c1] - coffsets[c0];
	tmp = (offsets[cbounds[c0]] - offsets[hit0])
	    + ((tmp+tmp) <= csize ? tmp : csize - tmp)
	    + (offsets[cbounds[c1]] - offsets[hit1]);
	if (tmp < cost) {
	    cost = tmp;
	    option = 4;
	}
	// pair 5: [complement | - | complement]
	if (c0 > 0) {
	    tmp = coffsets[c1] - coffsets[c0-1];
	    tmp = (offsets[hit0] - offsets[cbounds[c0-1]])
		+ ((tmp+tmp) <= csize ? tmp : csize - tmp)
		+ (offsets[cbounds[c1]] - offsets[hit1]);
	    if (tmp < cost) {
		cost = tmp;
		option = 5;
	    }
	}
	// pair 1: fine level only
	tmp = (offsets[hit1] - offsets[hit0] <=
	       (offsets.back()-offsets[hit1+1])+(offsets[hit0]-offsets[0]) ?
	       offsets[hit1] - offsets[hit0] :
	       (offsets.back()-offsets[hit1+1])+(offsets[hit0]-offsets[0]));
	if (cost > static_cast<long>(0.99*tmp)) { // slightly prefer 1
	    cost = tmp;
	    option = 1;
	}
	switch (option) {
	default:
	case 1: // use fine level only
	    sumBits(hit0, hit1, lower);
	    break;
	case 2: // direct | - | direct
	    if (c0 < c1-1) {
		tmp = coffsets[c1-1] - coffsets[c0];
		if (tmp + tmp <= csize) {
		    lower.set(0, nrows);
		    activateCoarse(c0, c1-1);
		    addBins(cbits, c0, c1-1, lower);
		}
		else {
		    ibis::bitvector bv;
		    bv.set(0, nrows);
		    if (c0 > 0) {
			activateCoarse(0, c0);
			addBins(cbits, 0, c0, bv);
		    }
		    if (c1 <= ncoarse) {
			activateCoarse(c1-1, ncoarse);
			addBins(cbits, c1-1, ncoarse, bv);
		    }
		    col->getNullMask(lower);
		    lower -= bv;
		}
	    }
	    if (hit0 < cbounds[c0])
		addBits(hit0, cbounds[c0], lower); // left edge bin
	    if (cbounds[c1-1] < hit1)
		addBits(cbounds[c1-1], hit1, lower); // right edge bin
	    break;
	case 3: // complement | - | direct
	    tmp = coffsets[c1-1] - coffsets[c0-1];
	    if (tmp + tmp <= csize) {
		lower.set(0, nrows);
		activateCoarse(c0-1, c1-1);
		addBins(cbits, c0-1, c1-1, lower);
	    }
	    else {
		ibis::bitvector bv;
		bv.set(0, nrows);
		if (c0 > 1) {
		    activateCoarse(0, c0-1);
		    addBins(cbits, 0, c0-1, bv);
		}
		if (c1 <= ncoarse) {
		    activateCoarse(c1-1, ncoarse);
		    addBins(cbits, c1-1, ncoarse, bv);
		}
		col->getNullMask(lower);
		lower -= bv;
	    }
	    if (cbounds[c0-1] < hit0) { // left edge bin, complement
		ibis::bitvector bv;
		sumBits(cbounds[c0-1], hit0, bv);
		lower -= bv;
	    }
	    if (cbounds[c1-1] < hit1)
		addBits(cbounds[c1-1], hit1, lower); // right edge bin
	    break;
	case 4: // direct | - | complement
	    tmp = coffsets[c1] - coffsets[c0];
	    if (tmp + tmp <= csize) {
		lower.set(0, nrows);
		activateCoarse(c0, c1);
		addBins(cbits, c0, c1, lower);
	    }
	    else {
		ibis::bitvector bv;
		bv.set(0, nrows);
		if (c0 > 0) {
		    activateCoarse(0, c0);
		    addBins(cbits, 0, c0, bv);
		}
		if (c1 < ncoarse) {
		    activateCoarse(c1, ncoarse);
		    addBins(cbits, c1, ncoarse, bv);
		}
		col->getNullMask(lower);
		lower -= bv;
	    }
	    if (hit0 < cbounds[c0])
		addBits(hit0, cbounds[c0], lower); // left edge bin
	    if (cbounds[c1] > hit1) { // right edge bin
		ibis::bitvector bv;
		sumBits(hit1, cbounds[c1], bv);
		lower -= bv;
	    }
	    break;
	case 5: // complement | - | complement
	    tmp = coffsets[c1] - coffsets[c0-1];
	    if (tmp + tmp <= csize) {
		lower.set(0, nrows);
		activateCoarse(c0-1, c1);
		addBins(cbits, c0-1, c1, lower);
	    }
	    else {
		ibis::bitvector bv;
		bv.set(0, nrows);
		if (c0 > 1) {
		    activateCoarse(0, c0-1);
		    addBins(cbits, 0, c0-1, bv);
		}
		if (c1 < ncoarse) {
		    activateCoarse(c1, ncoarse);
		    addBins(cbits, c1, ncoarse, bv);
		}
		col->getNullMask(lower);
		lower -= bv;
	    }
	    if (hit0 > cbounds[c0-1]) { // left edge bin
		ibis::bitvector bv;
		sumBits(cbounds[c0-1], hit0, bv);
		lower -= bv;
	    }
	    if (cbounds[c1] > hit1) { // right edge bin
		ibis::bitvector bv;
		sumBits(hit1, cbounds[c1], bv);
		lower -= bv;
	    }
	}
    }
    return lower.cnt();
} // ibis::zona::evaluate

// the argument is the name of the directory, the file name is
// column::name() + ".idx"
void ibis::zona::write(const char* dt) const {
    if (vals.empty()) return;

    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return;
    if (fname != 0 || str != 0)
	activate(); // activate all bitvectors

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdes < 0) {
	col->logWarning("zona::write", "unable to open \"%s\" for write",
			fnm.c_str());
	return;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    int32_t ierr = 0;
    const uint32_t nobs = vals.size();

    array_t<int32_t> offs(nobs+1);
    char header[] = "#IBIS\7\0\0";
    header[5] = (char)ibis::index::ZONA;
    header[6] = (char)sizeof(int32_t);
    ierr = UnixWrite(fdes, header, 8);
    ibis::relic::write(fdes); // write the bulk of the index file
    writeCoarse(fdes); // write the coarse level bins
#if _POSIX_FSYNC+0 > 0
    (void) fsync(fdes); // write to disk
#endif
    ierr = UnixClose(fdes);

    if (ibis::gVerbose > 5)
	col->logMessage("zona::write", "wrote %lu bitmap%s to %s",
			static_cast<long unsigned>(nobs),
			(nobs>1?"s":""), fnm.c_str());
} // ibis::zona::write

// This function intended to be called after calling ibis::relic::write,
// however, it does not check for this fact!
void ibis::zona::writeCoarse(int fdes) const {
    if (cbounds.empty() || cbits.empty() || nrows == 0)
	return;

    int32_t ierr;
    const uint32_t nc = (cbounds.size()-1 <= cbits.size() ?
			 cbounds.size()-1 : cbits.size());
    array_t<int32_t> offs(nc+1);
    ierr = UnixWrite(fdes, &nc, sizeof(nc));
    if (ierr < 0) {
	LOGGER(ibis::gVerbose >= 1) << "ibis::zona::writeCoarse(" << fdes << ") failed to write "
		  << "an integer";
	return;
    }
    ierr = UnixWrite(fdes, cbounds.begin(), sizeof(uint32_t)*(nc+1));
    ierr = UnixSeek(fdes, sizeof(int32_t)*(nc+1), SEEK_CUR);
    for (unsigned i = 0; i < nc; ++ i) {
	offs[i] = UnixSeek(fdes, 0, SEEK_CUR);
	if (cbits[i] != 0)
	    cbits[i]->write(fdes);
    }
    offs[nc] = UnixSeek(fdes, 0, SEEK_CUR);
    ierr -= sizeof(int32_t) * (nc+1);
    UnixSeek(fdes, ierr, SEEK_SET);
    ierr = UnixWrite(fdes, offs.begin(), sizeof(int32_t)*(nc+1));
    ierr = UnixSeek(fdes, offs.back(), SEEK_SET);
} // ibis::zona::writeCoarse

// read the index contained in the file f
void ibis::zona::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);

    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0) return;

    char header[8];
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    if (8 != UnixRead(fdes, static_cast<void*>(header), 8)) {
	UnixClose(fdes);
	return;
    }

    if (false == (header[0] == '#' && header[1] == 'I' &&
		  header[2] == 'B' && header[3] == 'I' &&
		  header[4] == 'S' &&
		  header[5] == static_cast<char>(ZONA) &&
		  header[6] == static_cast<char>(sizeof(int32_t)) &&
		  header[7] == static_cast<char>(0))) {
	UnixClose(fdes);
	return;
    }

    uint32_t dim[3];
    uint32_t begin, end;
    clear(); // clear the current content
    fname = ibis::util::strnewdup(fnm.c_str());

    long ierr = UnixRead(fdes, static_cast<void*>(dim), 3*sizeof(uint32_t));
    if (ierr < static_cast<int>(3*sizeof(uint32_t))) {
	UnixClose(fdes);
	return;
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
#if defined(DEBUG) || defined(_DEBUG)
    if (ibis::gVerbose > 5) {
	unsigned nprt = (ibis::gVerbose < 30 ? (1 << ibis::gVerbose) : dim[1]);
	if (nprt > dim[1])
	    nprt = dim[1];
	ibis::util::logger lg(5);
	lg.buffer() << "DEBUG -- ibis::zona::read(" << f
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
	end += sizeof(int32_t) * (nc+1);
	if (cbounds.size() == nc+1) {
	    array_t<int32_t> tmp(fdes, begin, end);
	    coffsets.swap(tmp);
	}

	for (unsigned i = 0; i < cbits.size(); ++ i)
	    delete cbits[i];
	cbits.resize(nc);
	for (unsigned i = 0; i < nc; ++ i)
	    cbits[i] = 0;
    }
    UnixClose(fdes);
    str = 0;
    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' header from %s",
			name(), fnm.c_str());
} // ibis::zona::read

// Reading information about the coarse bins.  To be used after calling
// ibis::relic::read, which happens in the constructor.
void ibis::zona::readCoarse(const char* fn) {
    std::string fnm;
    indexFileName(fn, fnm);

    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0) return;
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    long ierr = UnixSeek(fdes, offsets.back(), SEEK_SET);
    if (ierr == offsets.back()) {
	uint32_t nc, begin, end;
	ierr = UnixRead(fdes, &nc, sizeof(nc));
	if (ierr <= 0 || static_cast<uint32_t>(ierr) != sizeof(nc)) {
	    UnixClose(fdes);
	    return;
	}

	begin = offsets.back() + sizeof(nc);
	end = begin + sizeof(uint32_t)*(nc+1);
	if (ierr > 0 && nc > 0) {
	    array_t<uint32_t> tmp(fdes, begin, end);
	    cbounds.swap(tmp);
	}
	begin = end;
	end += sizeof(int32_t) * (nc+1);
	if (cbounds.size() == nc+1) {
	    array_t<int32_t> tmp(fdes, begin, end);
	    coffsets.swap(tmp);
	}

	for (unsigned i = 0; i < cbits.size(); ++ i)
	    delete cbits[i];
	cbits.resize(nc);
	for (unsigned i = 0; i < nc; ++ i)
	    cbits[i] = 0;
    }
    UnixClose(fdes);

    if (ibis::gVerbose > 7)
	col->logMessage("readIndex", "finished reading '%s' coarse bin info "
			"from %s", name(), fnm.c_str());
} // ibis::zona::readCoarse

// attempt to reconstruct an index from a piece of consecutive memory
void ibis::zona::read(ibis::fileManager::storage* st) {
    if (st == 0) return;
    if (str != st && str != 0)
	delete str;
    if (fname) { // previously connected to a file, clean it up
	delete [] fname;
	offsets.clear();
	fname = 0;
    }

    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    uint32_t pos = 8 + sizeof(uint32_t);
    const uint32_t nobs = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t);
    const uint32_t card = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t) + 7;
    array_t<int32_t> offs(st, 8*(pos/8) + sizeof(double)*card, nobs+1);
    array_t<double> dbl(st, 8*(pos/8), card);
    offsets.copy(offs);
    vals.swap(dbl);

    for (uint32_t i = 0; i < bits.size(); ++ i)
	delete bits[i];
    bits.resize(nobs);
    for (uint32_t i = 0; i < nobs; ++i)
	bits[i] = 0;
    if (st->isFileMap()) { // only restore the first bitvector
#if defined(ALWAY_READ_BITVECTOR0)
	if (offs[1] > offs[0]) {
	    array_t<ibis::bitvector::word_t>
		a0(st, offs[0], (offs[1]-offs[0])/
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

	if (str->size() > static_cast<uint32_t>(offsets.back())) {
	    uint32_t nc =
		*(reinterpret_cast<uint32_t*>(str->begin() + offsets.back()));
	    if (nc > 0 && str->size() > static_cast<uint32_t>(offsets.back()) +
		(sizeof(int32_t)+sizeof(uint32_t))*(nc+1)) {
		uint32_t start;
		start = offsets.back() + 4;
		array_t<uint32_t> btmp(str, start, nc+1);
		cbounds.swap(btmp);

		start += sizeof(uint32_t)*(nc+1);
		array_t<int32_t> otmp(str, start, nc+1);

		cbits.resize(nc);
		for (unsigned i = 0; i < nc; ++ i)
		    cbits[i] = 0;
	    }
	}
    }
    else { // regenerate all the bitvectors
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
		col->logWarning("zona::read", "bitvector %lu is invalid "
				"(offsets[%lu]=%lu, offsets[%lu]=%lu)",
				static_cast<long unsigned>(i),
				static_cast<long unsigned>(i),
				static_cast<long unsigned>(offs[i]),
				static_cast<long unsigned>(i+1),
				static_cast<long unsigned>(offs[i+1]));
	    }
	}

	if (str->size() > static_cast<uint32_t>(offsets.back())) {
	    uint32_t nc =
		*(reinterpret_cast<uint32_t*>(str->begin() + offsets.back()));
	    if (nc > 0 && str->size() > offsets.back() +
		(sizeof(int32_t)+sizeof(uint32_t))*(nc+1)) {
		uint32_t start;
		start = offsets.back() + 4;
		array_t<uint32_t> btmp(str, start, nc+1);
		cbounds.swap(btmp);

		start += sizeof(uint32_t)*(nc+1);
		array_t<int32_t> otmp(str, start, nc+1);

		cbits.resize(nc);
		for (unsigned i = 0; i < nc; ++ i) {
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
	str = 0;
    }
} // ibis::zona::read

void ibis::zona::clear() {
    const uint32_t nc = cbits.size();
    for (unsigned i = 0; i < nc; ++ i)
	delete cbits[i];

    cbits.clear();
    cbounds.clear();
    coffsets.clear();
    ibis::relic::clear();
} // ibis::zona::clear

// the printing function
void ibis::zona::print(std::ostream& out) const {
    if (vals.size() != bits.size() || bits.empty())
	return;

    out << "the equality-equality encoded index (unbinned) for "
	<< col->partition()->name() << '.'
	<< col->name() << " contains " << bits.size()
	<< " bitvectors for " << nrows << " objects\n";
    const uint32_t nc = cbits.size();
    uint32_t nprt = (ibis::gVerbose < 30 ? 1 << ibis::gVerbose : bits.size());
    uint32_t omitted = 0;
    uint32_t end;
    if (cbounds.size() == nc+1 && nc > 0) { // has coarse bins
	for (unsigned j = 0; j < nc; ++ j) {
	    out << "Coarse bin " << j << ", [" << cbounds[j] << ", "
		<< cbounds[j+1] << ")";
	    if (cbits[j])
		out << "\t{" << cbits[j]->cnt()
		    << "\t" << cbits[j]->bytes() << "}\n";
	    else
		out << "\n";
	    end = (cbounds[j+1] <= cbounds[j]+nprt ?
		   cbounds[j+1] : cbounds[j]+nprt);
	    for (unsigned i = cbounds[j]; i < end; ++ i) {
		if (bits[i]) {
		    out << "\t" << i << ":\t";
		    out.precision(12);
		    out << vals[i] << "\t" << bits[i]->cnt()
			<< "\t" << bits[i]->bytes() << "\n";
		}
		else {
		    ++ omitted;
		}
	    }
	    if (cbounds[j+1] > end && nprt > 0) {
		out << "\t...\n";
		omitted += (cbounds[j+1] - end);
	    }
	}
	if (nprt > 0 && omitted > 0)
	    out << "\tfine level bitmaps omitted: " << omitted << "\n";
    }
    else { // no coarse bins
	const uint32_t nobs = bits.size();
	uint32_t skip = 0;
	if (ibis::gVerbose <= 0) {
	    skip = nobs;
	}
	else if ((nobs >> 2*ibis::gVerbose) > 2) {
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
	    out << " (printing 1 out of every " << skip << ")\n";
	}

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
    }
    out << std::endl;
} // ibis::zona::print

// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2006-2012 the Regents of the University of California
#include "idirekte.h"
#include "part.h"
#include <typeinfo>	// typeid

/// Constructing a new ibis::direkte object from base data in a file.
ibis::direkte::direkte(const ibis::column* c, const char* f)
    : ibis::index(c) {
    if (c == 0)
	return;
    if (c->type() == ibis::FLOAT ||
	c->type() == ibis::DOUBLE ||
	c->type() == ibis::TEXT) {
	ibis::util::logMessage("Error", "direkte can only be used "
			       "for columns with integer values (current "
			       "column %s, type=%s)", c->name(),
			       ibis::TYPESTRING[(int)c->type()]);
	throw ibis::bad_alloc("wrong column type for ibis::direkte");
    }
//     if (c->lowerBound() < 0.0 || c->lowerBound() > 1.0) {
// 	ibis::util::logMessage("Error", "direkte can only be used "
// 			       " on integer attributes with minimal value "
// 			       "of 0 or 1, current minimal value is %g",
// 			       c->lowerBound());
// 	throw ibis::bad_alloc("unexpected minimal value for ibis::direkte");
//     }

    std::string dfname;
    dataFileName(f, dfname);
    if (c->type() == ibis::CATEGORY)
	dfname += ".int";

    int ierr = 0;
    switch (c->type()) {
    default: {
	ibis::util::logMessage("Error", "direkte can only be used "
			       "for columns with integer values (current "
			       "column %s, type=%s)", c->name(),
			       ibis::TYPESTRING[(int)c->type()]);
	throw ibis::bad_alloc("wrong column type for ibis::direkte");}
    case ibis::BYTE: {
	ierr = construct<signed char>(dfname.c_str());
	break;}
    case ibis::UBYTE: {
	ierr = construct<unsigned char>(dfname.c_str());
	break;}
    case ibis::SHORT: {
	ierr = construct<int16_t>(dfname.c_str());
	break;}
    case ibis::USHORT: {
	ierr = construct<uint16_t>(dfname.c_str());
	break;}
    case ibis::INT: {
	ierr = construct<int32_t>(dfname.c_str());
	break;}
    case ibis::UINT:
    case ibis::CATEGORY: {
	ierr = construct<uint32_t>(dfname.c_str());
	break;}
    case ibis::LONG: {
	ierr = construct<int64_t>(dfname.c_str());
	break;}
    case ibis::ULONG: {
	ierr = construct<uint64_t>(dfname.c_str());
	break;}
    }
    if (ierr < 0) {
	ibis::util::logMessage("Error", "direkte failed with error "
			       "code %d", ierr);
	throw ibis::bad_alloc("direkte construction failure");
    }
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	lg()
	    << "direkte[" << col->partition()->name() << '.' << col->name()
	    << "]::ctor -- constructed a simple equality index with "
	    << bits.size() << " bitmap" << (bits.size()>1?"s":"");
	if (ibis::gVerbose > 6) {
	    lg() << "\n";
	    print(lg());
	}
    }
} // ibis::direkte::direkte

template <typename T>
int ibis::direkte::construct(const char* dfname) {
    int ierr = 0;
    array_t<T> vals;
    LOGGER(ibis::gVerbose > 4)
	<< "direkte[" << col->partition()->name() << '.' << col->name()
	<< "]::construct -- starting to process file " << dfname << " as "
	<< typeid(T).name();
    ibis::bitvector mask;
    col->getNullMask(mask);
    nrows = col->partition()->nRows();
    ierr = ibis::fileManager::instance().getFile(dfname, vals);
    if (ierr == 0) { // got a pointer to the base data
	if (col->upperBound() > col->lowerBound()) {
	    const uint32_t nbits = (uint32_t)col->upperBound() + 1;
#ifdef RESERVE_SPACE_BEFORE_CREATING_INDEX
	    const uint32_t nset = (uint32_t)(nrows+nbits-1)/nbits;
#endif
	    bits.resize(nbits);
	    for (uint32_t i = 0; i < nbits; ++ i) {
		bits[i] = new ibis::bitvector();
#ifdef RESERVE_SPACE_BEFORE_CREATING_INDEX
		bits[i]->reserve(nbits, nset);
#endif
	    }
	    if (ibis::gVerbose > 6)
		col->logMessage("direkte::construct", "finished allocating "
				"%lu bitvectors",
				static_cast<long unsigned>(nbits));
	}
	// if (vals.size() > nrows)
	//     vals.resize(nrows);

	for (ibis::bitvector::indexSet iset = mask.firstIndexSet();
	     iset.nIndices() > 0; ++ iset) {
	    const ibis::bitvector::word_t *iis = iset.indices();
	    if (iset.isRange()) { // a range
		for (uint32_t j = *iis; j < iis[1]; ++ j) {
		    const uint32_t nbits = bits.size();
		    if (nbits <= static_cast<uint32_t>(vals[j])) {
			const uint32_t newsize = vals[j]+1;
			bits.resize(newsize);
			for (uint32_t i = nbits; i < newsize; ++ i)
			    bits[i] = new ibis::bitvector;
		    }
		    bits[vals[j]]->setBit(j, 1);
		}
	    }
	    else {
		for (uint32_t i = 0; i < iset.nIndices(); ++ i) {
		    const ibis::bitvector::word_t j = iis[i];
		    const uint32_t nbits = bits.size();
		    if (nbits <= static_cast<uint32_t>(vals[j])) {
			const uint32_t newsize = vals[j]+1;
			bits.resize(newsize);
			for (uint32_t i = nbits; i < newsize; ++ i)
			    bits[i] = new ibis::bitvector;
		    }
		    bits[vals[j]]->setBit(j, 1);
		}
	    }
	}
    }
    else { // failed to read or memory map the data file, try to read the
	   // values one at a time
	const unsigned elemsize = sizeof(T);
	uint32_t sz = ibis::util::getFileSize(dfname);
	if (sz == 0) {
	    ierr = -1; // no data file
	    return ierr;
	}

	LOGGER(ibis::gVerbose > 5)
	    << "direkte[" << col->partition()->name() << '.' << col->name()
	    << "]::construct -- starting to read the values from "
	    << dfname << " one at a time";
	if (col->upperBound() > col->lowerBound()) {
	    const uint32_t nbits = (uint32_t)col->upperBound() + 1;
#ifdef RESERVE_SPACE_BEFORE_CREATING_INDEX
	    const uint32_t nset = (nrows + nbits - 1) / nbits;
#endif
	    bits.resize(nbits);
	    for (uint32_t i = 0; i < nbits; ++ i) {
		bits[i] = new ibis::bitvector();
#ifdef RESERVE_SPACE_BEFORE_CREATING_INDEX
		bits[i]->reserve(nbits, nset);
#endif
	    }
	}
	sz /= elemsize;
	if (sz > nrows)
	    sz = nrows;
	int fdes = UnixOpen(dfname, OPEN_READONLY);
	if (fdes < 0) {
	    ierr = -2; // failed to open file for reading
	    return ierr;
	}

	for (ibis::bitvector::indexSet iset = mask.firstIndexSet();
	     iset.nIndices() > 0; ++ iset) {
	    const ibis::bitvector::word_t *iis = iset.indices();
	    if (iset.isRange()) { // a range
		ierr = UnixSeek(fdes, *iis * elemsize, SEEK_SET);
		for (uint32_t j = *iis; j < iis[1]; ++ j) {
		    T val;
		    ierr = UnixRead(fdes, &val, elemsize);
		    if (ierr < static_cast<int>(elemsize)) {
			ierr = -3;
			break;
		    }

		    const uint32_t nbits = bits.size();
		    if (nbits <= static_cast<uint32_t>(val)) {
			const uint32_t newsize = val + 1;
			bits.resize(newsize);
			for (uint32_t i = nbits; i < newsize; ++ i)
			    bits[i] = new ibis::bitvector;
		    }
		    bits[val]->setBit(j, 1);
		}
	    }
	    else {
		for (uint32_t i = 0; i < iset.nIndices(); ++ i) {
		    const ibis::bitvector::word_t j = iis[i];
		    T val;
		    ierr = UnixSeek(fdes, j * elemsize, SEEK_SET);
		    if (ierr < 0 || static_cast<unsigned>(ierr) != j*elemsize) {
			ierr = -3;
			break;
		    }
		    ierr = UnixRead(fdes, &val, elemsize);
		    if (ierr < static_cast<int>(elemsize)) {
			ierr = -4;
			break;
		    }

		    const uint32_t nbits = bits.size();
		    if (nbits <= static_cast<uint32_t>(val)) {
			const uint32_t newsize = val + 1;
			bits.resize(newsize);
			for (uint32_t i = nbits; i < newsize; ++ i)
			    bits[i] = new ibis::bitvector;
		    }
		    bits[val]->setBit(j, 1);
		}
	    }

	    if (ierr < 0) break;
	}
	UnixClose(fdes);
    }

    // make sure all bitvectors are of the right size
    for (uint32_t i = 0; i < bits.size(); ++ i)
	bits[i]->adjustSize(0, nrows);
    return ierr;
} // ibis::direkte::construct

ibis::direkte::direkte(const ibis::column* c, ibis::fileManager::storage* st)
    : ibis::index(c, st) {
    read(st);
} // ibis::direkte::direkte

/// The printing function.
void ibis::direkte::print(std::ostream& out) const {
    if (ibis::gVerbose < 0) return;
    const uint32_t nobs = bits.size();
    if (nobs > 0) {
	out << "The direct bitmap index for " << col->name() << " contains "
	    << nobs << " bit vector" << (nobs > 1 ? "s" : "") << "\n";
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
	    out << " (printing 1 out of every " << skip << ")";
	}
	out << "\n";

	for (uint32_t i=0; i<nobs; i += skip) {
	    if (bits[i]) {
		out << i << "\t" << bits[i]->cnt() << "\t" << bits[i]->bytes()
#if DEBUG+0 > 0 || _DEBUG+0 > 0
		    << "\t" << bits[i]->size()
#endif
		    << "\n";
	    }
	}
	if ((nobs-1) % skip) {
	    if (bits[nobs-1]) {
		out << nobs-1 << "\t" << bits[nobs-1]->cnt()
		    << "\t" << bits[nobs-1]->bytes() << "\n";
	    }
	}
    }
    else {
	out << "The direct bitmap index for " << col->name()
	    << " is empty\n";
    }
    out << std::endl;
} // ibis::direkte::print

/// Write the direct bitmap index to a file.
int ibis::direkte::write(const char* dt) const {
    std::string fnm;
    indexFileName(dt, fnm);
    if (fname != 0 && fnm.compare(fname) == 0)
	return 0;
    if (fname != 0 || str != 0)
	activate();

    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITENEW, OPEN_FILEMODE);
    if (fdes < 0) {
	ibis::fileManager::instance().flushFile(fnm.c_str());
	fdes = UnixOpen(fnm.c_str(), OPEN_WRITENEW, OPEN_FILEMODE);
	if (fdes < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- direkte[" << col->partition()->name() << "."
		<< col->name() << "]::write failed to open \"" << fnm
		<< "\" for writing ... " << (errno ? strerror(errno) : 0);
	    errno = 0;
	    return -2;
	}
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    int ierr = 0;
    const uint32_t nobs = bits.size();

#ifdef FASTBIT_USE_LONG_OFFSETS
    const bool useoffset64 = true;
#else
    const bool useoffset64 = (8+getSerialSize() > 0x80000000UL);
#endif
    char header[] = "#IBIS\0\0\0";
    header[5] = (char)ibis::index::DIREKTE;
    header[6] = (char)(useoffset64 ? 8 : 4);
    ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr  = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr += UnixWrite(fdes, &nobs,  sizeof(uint32_t));
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write nrows and nobs, ierr = " << ierr;
	return -4;
    }
    offset64.resize(nobs+1);
    offset64[0] = 16 + header[6]*(nobs+1);
    ierr = UnixSeek(fdes, header[6]*(nobs+1), SEEK_CUR);
    if (ierr != offset64[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to seek to " << offset64[0] << ", ierr = " << ierr;
	return -5;
    }
    for (uint32_t i = 0; i < nobs; ++ i) {
	if (bits[i] != 0) {
	    if (bits[i]->cnt() > 0)
		bits[i]->write(fdes);
	}
	offset64[i+1] = UnixSeek(fdes, 0, SEEK_CUR);
    }
    ierr = UnixSeek(fdes, 16, SEEK_SET);
    if (ierr != 16) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to seek to offset 16, ierr = " << ierr;
	return -6;
    }
    if (useoffset64) {
	ierr = UnixWrite(fdes, offset64.begin(), 8*(nobs+1));
	offset32.clear();
    }
    else {
	offset32.resize(nobs+1);
	for (unsigned j = 0; j <= nobs; ++ j)
	    offset32[j] = offset64[j];
	ierr = UnixWrite(fdes, offset32.begin(), 4*(nobs+1));
	offset64.clear();
    }
    if (ierr < (off_t)(header[6]*(nobs+1))) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write bitmap offsets, ierr = " << ierr;
	return -7;
    }
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    (void) UnixFlush(fdes); // write to disk
#endif

    LOGGER(ibis::gVerbose > 5)
	<< "direkte[" << col->partition()->name() << "."
	<< col->name() << "]::write -- wrote " << nobs << " bitmap"
	<< (nobs>1?"s":"") << " to " << fnm;
    return 0;
} // ibis::direkte::write

/// Read index from the specified location.
int ibis::direkte::read(const char* f) {
    std::string fnm;
    indexFileName(f, fnm);
    int fdes = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (fdes < 0) return -1;

    char header[8];
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif
    if (8 != UnixRead(fdes, static_cast<void*>(header), 8)) {
	return -2;
    }

    if (false == (header[0] == '#' && header[1] == 'I' &&
		  header[2] == 'B' && header[3] == 'I' &&
		  header[4] == 'S' &&
		  header[5] == static_cast<char>(ibis::index::DIREKTE) &&
		  (header[6] == 8 || header[6] == 4) &&
		  header[7] == static_cast<char>(0))) {
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg()
		<< "Warning -- direkte[" << col->partition()->name() << '.'
		<< col->name() << "]::read the header from " << fnm
		<< " (";
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
		lg() << "0x" << std::hex << (uint16_t) header[2]
			    << std::dec;
	    if (isprint(header[3]) != 0)
		lg() << header[3];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[3]
			    << std::dec;
	    if (isprint(header[4]) != 0)
		lg() << header[4];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[4]
			    << std::dec;
	    if (isprint(header[5]) != 0)
		lg() << header[5];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[5]
			    << std::dec;
	    if (isprint(header[6]) != 0)
		lg() << header[6];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[6]
			    << std::dec;
	    if (isprint(header[7]) != 0)
		lg() << header[7];
	    else
		lg() << "0x" << std::hex << (uint16_t) header[7]
			    << std::dec;
	    lg() << ") does not contain the expected values";
	}
	return -3;
    }

    uint32_t dim[2];
    size_t begin, end;
    ibis::index::clear(); // clear the current bit vectors
    fname = ibis::util::strnewdup(fnm.c_str());

    off_t ierr = UnixRead(fdes, static_cast<void*>(dim), 2*sizeof(uint32_t));
    if (ierr < static_cast<int>(2*sizeof(uint32_t))) {
	return -4;
    }
    nrows = dim[0];
    // read offsets
    begin = 8 + 2*sizeof(uint32_t);
    end = 8 + 2*sizeof(uint32_t) + header[6] * (dim[1] + 1);
    ierr = initOffsets(fdes, header[6], begin, dim[1]);
    if (ierr < 0)
	return ierr;
    ibis::fileManager::instance().recordPages(0, end);
#if DEBUG+0 > 1 || _DEBUG+0 > 1
    if (ibis::gVerbose > 5) {
	unsigned nprt = (ibis::gVerbose < 30 ? (1 << ibis::gVerbose) : dim[1]);
	if (nprt > dim[1])
	    nprt = dim[1];
	ibis::util::logger lg;
	lg() << "DEBUG -- direkte[" << col->partition()->name() << '.'
		    << col->name() << "]::read(" << fnm
		    << ") got nobs = " << dim[1]
		    << ", the offsets of the bit vectors are\n";
	if (header[6] == 8) {
	    for (unsigned i = 0; i < nprt; ++ i)
		lg() << offset64[i] << " ";
	}
	else {
	    for (unsigned i = 0; i < nprt; ++ i)
		lg() << offset32[i] << " ";
	}
	if (nprt < dim[1])
	    lg() << "... (skipping " << dim[1]-nprt << ") ... ";
	if (header[6] == 8)
	    lg() << offset64[dim[1]];
	else
	    lg() << offset32[dim[1]];
    }
#endif

    initBitmaps(fdes);
    str = 0;
    LOGGER(ibis::gVerbose > 3)
	<< "direkte[" << col->name() << "]::read(" << fnm << ") finished "
	"reading index header with nrows=" << nrows << " and bits.size()="
	<< bits.size();
    return 0;
} // ibis::direkte::read

/// Reconstruct an index from a piece of consecutive memory.
int ibis::direkte::read(ibis::fileManager::storage* st) {
    if (st == 0) return -1;
    clear();

    const char offsetsize = st->begin()[6];
    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    uint32_t pos = 8 + sizeof(uint32_t);
    const uint32_t nobs = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t);
    if (offsetsize == 8) {
	array_t<int64_t> offs(st, pos, pos+8*nobs+8);
	offset64.copy(offs);
    }
    else if (offsetsize == 4) {
	array_t<int32_t> offs(st, pos, pos+4*nobs+4);
	offset32.copy(offs);
    }
    else {
	clear();
	return -2;
    }

    initBitmaps(st);
    LOGGER(ibis::gVerbose > 3)
	<< "direkte[" << col->name() << "]::read(" << st << ") finished "
	"reading index header with nrows=" << nrows << " and bits.size()="
	<< bits.size();
    return 0;
} // ibis::direkte::read

// Convert to a range [ib, ie) such that bits[ib:ie-1] contains the solution
void ibis::direkte::locate(const ibis::qContinuousRange& expr,
			   uint32_t& ib, uint32_t& ie) const {
    ib = static_cast<uint32_t>(expr.leftBound()>0.0 ? expr.leftBound() : 0.0);
    ie = static_cast<uint32_t>(expr.rightBound()>0.0 ? expr.rightBound() : 0.0);

    switch (expr.leftOperator()) {
    case ibis::qExpr::OP_LT: {
	ib += (expr.leftBound() >= ib);
	switch (expr.rightOperator()) {
	case ibis::qExpr::OP_LT: {
	    if (expr.rightBound()>ie)
		++ ie;
	    break;}
	case ibis::qExpr::OP_LE: {
	    ++ ie;
	    break;}
	case ibis::qExpr::OP_GT: {
	    if (ib < ie+1)
		ib = ie + 1;
	    ie = bits.size();
	    break;}
	case ibis::qExpr::OP_GE: {
	    if (expr.rightBound() > ie)
		++ ie;
	    if (ib < ie)
		ib = ie;
	    ie = bits.size();
	    break;}
	case ibis::qExpr::OP_EQ: {
	    if (expr.leftBound() < expr.rightBound() &&
		ie == expr.rightBound()) {
		ib = ie;
		++ ie;
	    }
	    else {
		ie = ib;
	    }
	    break;}
	default: {
	    ie = bits.size();
	    break;}
	}
	break;}
    case ibis::qExpr::OP_LE: {
	ib += (expr.leftBound() > ib);
	switch (expr.rightOperator()) {
	case ibis::qExpr::OP_LT: {
	    if (expr.rightBound()>ie)
		++ ie;
	    break;}
	case ibis::qExpr::OP_LE: {
	    ++ ie;
	    break;}
	case ibis::qExpr::OP_GT: {
	    if (ib < ie+1)
		ib = ie+1;
	    ie = bits.size();
	    break;}
	case ibis::qExpr::OP_GE: {
	    if (expr.rightBound() > ie)
		++ ie;
	    if (ib < ie)
		ib = ie;
	    ie = bits.size();
	    break;}
	case ibis::qExpr::OP_EQ: {
	    if (expr.rightBound() >= expr.leftBound() &&
		ie == expr.rightBound()) {
		ib = ie;
		++ ie;
	    }
	    else {
		ie = ib;
	    }
	    break;}
	default: {
	    ie = bits.size();
	    break;}
	}
	break;}
    case ibis::qExpr::OP_GT: {
	ib += (expr.leftBound() > ib);
	switch (expr.rightOperator()) {
	case ibis::qExpr::OP_LT: {
	    if (expr.rightBound() > ie)
		++ ie;
	    if (ib < ie)
		ie = ib;
	    ib = 0;
	    break;}
	case ibis::qExpr::OP_LE: {
	    ++ ie;
	    if (ib < ie)
		ie = ib;
	    ib = 0;
	    break;}
	case ibis::qExpr::OP_GT: {
	    uint32_t tmp = ie+1;
	    ie = ib;
	    ib = tmp;
	    break;}
	case ibis::qExpr::OP_GE: {
	    uint32_t tmp = (expr.rightBound()>ie ? ie+1 : ie);
	    ie = ib;
	    ib = tmp;
	    break;}
	case ibis::qExpr::OP_EQ: {
	    if (expr.rightBound() > expr.leftBound() &&
		expr.rightBound() == ie) {
		ib = ie;
		++ ie;
	    }
	    else {
		ie = ib;
	    }
	    break;}
	default: {
	    ie = ib;
	    ib = 0;
	    break;}
	}
	break;}
    case ibis::qExpr::OP_GE: {
	ib += (expr.leftBound() >= ib);
	switch (expr.rightOperator()) {
	case ibis::qExpr::OP_LT: {
	    if (expr.rightBound() > ie)
		++ ie;
	    if (ib < ie)
		ie = ib;
	    ib = 0;
	    break;}
	case ibis::qExpr::OP_LE: {
	    ++ ie;
	    if (ib < ie)
		ie = ib;
	    ib = 0;
	    break;}
	case ibis::qExpr::OP_GT: {
	    uint32_t tmp = ie+1;
	    ie = ib+1;
	    ib = tmp;
	    break;}
	case ibis::qExpr::OP_GE: {
	    uint32_t tmp = (expr.rightBound()<=ie ? ie : ie+1);
	    ie = ib+1;
	    ib = tmp;
	    break;}
	case ibis::qExpr::OP_EQ: {
	    if (expr.leftBound() >= expr.rightBound()) {
		ib = ie;
		++ ie;
	    }
	    else {
		ie = ib;
	    }  
	    break;}
	default: {
	    ie = ib;
	    ib = 0;
	    break;}
	}
	break;}
    case ibis::qExpr::OP_EQ: {
	if (expr.leftBound() == ib) {
	    switch (expr.rightOperator()) {
	    case ibis::qExpr::OP_LT: {
		if (expr.leftBound() < expr.rightBound())
		    ie = ib+1;
		else
		    ie = ib;
		break;}
	    case ibis::qExpr::OP_LE: {
		if (expr.leftBound() <= expr.rightBound())
		    ie = ib+1;
		else
		    ie = ib;
		break;}
	    case ibis::qExpr::OP_GT: {
		if (expr.leftBound() > expr.rightBound())
		    ie = ib+1;
		else
		    ie = ib;
		break;}
	    case ibis::qExpr::OP_GE: {
		if (expr.leftBound() >= expr.rightBound())
		    ie = ib+1;
		else
		    ie = ib;
		break;}
	    case ibis::qExpr::OP_EQ: {
		if (expr.leftBound() == expr.rightBound())
		    ie = ib+1;
		else
		    ie = ib;
		break;}
	    default: {
		ie = ib+1;
		break;}
	    }
	}
	else {
	    ie = ib;
	}
	break;}
    default: {
	switch (expr.rightOperator()) {
	case ibis::qExpr::OP_LT: {
	    ib = 0;
	    if (expr.rightBound()>ie)
		++ ie;
	    break;}
	case ibis::qExpr::OP_LE: {
	    ib = 0;
	    ++ ie;
	    break;}
	case ibis::qExpr::OP_GT: {
	    ib = ie + 1;
	    ie = bits.size();
	    break;}
	case ibis::qExpr::OP_GE: {
	    ib = (expr.rightBound() == ie ? ie : ie+1);
	    ie = bits.size();
	    break;}
	case ibis::qExpr::OP_EQ: {
	    if (expr.rightBound() == ie) {
		ib = ie;
		++ ie;
	    }
	    else {
		ie = ib;
	    }
	    break;}
	default: {
	    // nothing specified, match all
	    if (ibis::gVerbose > -1)
		col->logWarning("direkte::locate", "no operator specified "
				"in a qContinuousQuery object");
	    ib = 0;
	    ie = bits.size();
	    break;}
	}
	break;}
    }
} // ibis::direkte::locate

long ibis::direkte::evaluate(const ibis::qContinuousRange& expr,
			     ibis::bitvector& lower) const {
    uint32_t ib, ie;
    locate(expr, ib, ie);
    sumBins(ib, ie, lower);
    return lower.cnt();
} // ibis::direkte::evaluate

void ibis::direkte::estimate(const ibis::qContinuousRange& expr,
			     ibis::bitvector& lower,
			     ibis::bitvector& upper) const {
    upper.clear();
    uint32_t ib, ie;
    locate(expr, ib, ie);
    sumBins(ib, ie, lower);
} // ibis::direkte::estimate

uint32_t ibis::direkte::estimate(const ibis::qContinuousRange& expr) const {
    uint32_t ib, ie, cnt;
    locate(expr, ib, ie);
    activate(ib, ie);
    cnt = 0;
    for (uint32_t j = ib; j < ie; ++ j)
	if (bits[j])
	    cnt += bits[j]->cnt();
    return cnt;
} // ibis::direkte::estimate

long ibis::direkte::evaluate(const ibis::qDiscreteRange& expr,
			     ibis::bitvector& lower) const {
    const ibis::array_t<double>& varr = expr.getValues();
    lower.set(0, nrows);
    for (unsigned i = 0; i < varr.size(); ++ i) {
	unsigned int tmp = static_cast<unsigned int>(varr[i]);
	if (tmp < bits.size()) {
	    if (bits[tmp] == 0)
		activate(tmp);
	    if (bits[tmp])
		lower |= *(bits[tmp]);
	}
    }
    return lower.cnt();
} // ibis::direkte::evaluate

void ibis::direkte::estimate(const ibis::qDiscreteRange& expr,
			     ibis::bitvector& lower,
			     ibis::bitvector& upper) const {
    const ibis::array_t<double>& varr = expr.getValues();
    upper.clear();
    lower.set(0, nrows);
    for (unsigned i = 0; i < varr.size(); ++ i) {
	unsigned int tmp = static_cast<unsigned int>(varr[i]);
	if (tmp < bits.size()) {
	    if (bits[tmp] == 0)
		activate(tmp);
	    if (bits[tmp])
		lower |= *(bits[tmp]);
	}
    }
} // ibis::direkte::estimate

uint32_t ibis::direkte::estimate(const ibis::qDiscreteRange& expr) const {
    uint32_t res = 0;
    const ibis::array_t<double>& varr = expr.getValues();
    for (unsigned i = 0; i < varr.size(); ++ i) {
	unsigned int tmp = static_cast<unsigned int>(varr[i]);
	if (tmp < bits.size()) {
	    if (bits[tmp] == 0)
		activate(tmp);
	    if (bits[tmp])
		res += bits[tmp]->cnt();
	}
    }
    return res;
} // ibis::direkte::estimate

double ibis::direkte::estimateCost(const ibis::qContinuousRange& expr) const {
    double cost = 0.0;
    uint32_t ib, ie;
    locate(expr, ib, ie);
    if (ib < ie) {
	if (offset64.size() > bits.size()) {
	    const int32_t tot = offset64.back() - offset64[0];
	    if (ie < offset64.size()) {
		const int32_t mid = offset64[ie] - offset64[ib];
		if ((tot >> 1) >= mid)
		    cost = mid;
		else
		    cost = tot - mid;
	    }
	    else if (ib < offset64.size()) {
		const int32_t mid = offset64.back() - offset64[ib];
		if ((tot >> 1) >= mid)
		    cost = mid;
		else
		    cost = tot - mid;
	    }
	}
	else if (offset32.size() > bits.size()) {
	    const int32_t tot = offset32.back() - offset32[0];
	    if (ie < offset32.size()) {
		const int32_t mid = offset32[ie] - offset32[ib];
		if ((tot >> 1) >= mid)
		    cost = mid;
		else
		    cost = tot - mid;
	    }
	    else if (ib < offset32.size()) {
		const int32_t mid = offset32.back() - offset32[ib];
		if ((tot >> 1) >= mid)
		    cost = mid;
		else
		    cost = tot - mid;
	    }
	}
	else {
	    const unsigned elm = col->elementSize();
	    if (elm > 0)
		cost = (double)elm * col->partition()->nRows();
	    else
		cost = 4.0 * col->partition()->nRows();
	}
    }
    return cost;
} // ibis::direkte::estimateCost

double ibis::direkte::estimateCost(const ibis::qDiscreteRange& expr) const {
    double cost = 0;
    const ibis::array_t<double>& varr = expr.getValues();
    for (uint32_t j = 0; j < varr.size(); ++ j) {
	uint32_t ind = static_cast<uint32_t>(varr[j]);
	if (ind+1 < offset64.size() && ind < bits.size())
	    cost += offset64[ind+1] - offset64[ind];
	else if (ind+1 < offset32.size() && ind < bits.size())
	    cost += offset32[ind+1] - offset32[ind];
    }
    return cost;
} // ibis::direkte::estimateCost

/// Append the index in df to the one in dt.  If the index in df exists,
/// then it will be used, otherwise it simply creates a new index using the
/// data in dt.
long ibis::direkte::append(const char* dt, const char* df, uint32_t nnew) {
    if (dt == 0 || *dt == 0 || df == 0 || *df == 0 || nnew == 0) return -1L;    

    const uint32_t nold = (strcmp(dt, col->partition()->currentDataDir()) == 0 ?
			   col->partition()->nRows()-nnew : nrows);
    long ierr;
    if (nrows == nold) { // can make use of the existing index
	std::string dfidx;
	indexFileName(df, dfidx);
	ibis::direkte* idxf = 0;
	ibis::fileManager::storage* stdf = 0;
	ierr = ibis::fileManager::instance().getFile(dfidx.c_str(), &stdf);
	if (ierr == 0 && stdf != 0) {
	    const char* header = stdf->begin();
	    if (header[0] == '#' && header[1] == 'I' && header[2] == 'B' &&
		header[3] == 'I' && header[4] == 'S' &&
		header[5] == ibis::index::DIREKTE &&
		(header[6] == 8 || header[6] == 4) &&
		header[7] == static_cast<char>(0)) {
		idxf = new ibis::direkte(col, stdf);
	    }
	    else {
		LOGGER(ibis::gVerbose > 5)
		    << "Warning -- direkte[" << col->partition()->name() << '.'
		    << col->name() << "]::append -- file " << dfidx
		    << " has a unexpected header";
		remove(dfidx.c_str());
	    }
	}
	if (idxf != 0 && idxf->nrows == nnew) {
	    if (nold == 0) {
		nrows = idxf->nrows;
		str = idxf->str; idxf->str = 0;
		fname = 0;
		offset64.swap(idxf->offset64);
		offset32.swap(idxf->offset32);
		bits.swap(idxf->bits);
		delete idxf;
		//ierr = write(dt);
		return nnew;
	    }

	    activate(); // make sure all bitvectors are in memory
	    if (bits.size() < idxf->bits.size()) {
		bits.reserve(idxf->bits.size());
	    }
	    uint32_t j = 0;
	    while (j < idxf->bits.size()) {
		if (j >= bits.size()) {
		    bits.push_back(new ibis::bitvector);
		    bits[j]->set(0, nold);
		}
		if (idxf->bits[j] != 0) {
		    *(bits[j]) += *(idxf->bits[j]);
		}
		else {
		    bits[j]->adjustSize(nold, nold+nnew);
		}
		++ j;
	    }
	    while (j < bits.size()) {
		if (bits[j] != 0)
		    bits[j]->adjustSize(nold, nold+nnew);
		++ j;
	    }

	    delete idxf;
	    //ierr = write(dt);
	    return nnew;
	}
    }

    LOGGER(ibis::gVerbose > 4)
	<< "direkte[" << col->partition()->name() << '.' << col->name()
	<< "]::append to recreate the index with the data from " << dt;
    clear();
    std::string dfname;
    dataFileName(dt, dfname);
    if (col->type() == ibis::CATEGORY)
	dfname += ".int";

    switch (col->type()) {
    default: {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte can only be used "
	    "for columns with integer values (current column " << col->name()
	    << ", type=" <<  ibis::TYPESTRING[(int)col->type()] << ")";
	ierr = -2;
	return ierr;}
    case ibis::BYTE: {
	ierr = construct<signed char>(dfname.c_str());
	break;}
    case ibis::UBYTE: {
	ierr = construct<unsigned char>(dfname.c_str());
	break;}
    case ibis::SHORT: {
	ierr = construct<int16_t>(dfname.c_str());
	break;}
    case ibis::USHORT: {
	ierr = construct<uint16_t>(dfname.c_str());
	break;}
    case ibis::INT: {
	ierr = construct<int32_t>(dfname.c_str());
	break;}
    case ibis::UINT:
    case ibis::CATEGORY: {
	ierr = construct<uint32_t>(dfname.c_str());
	break;}
    case ibis::LONG: {
	ierr = construct<int64_t>(dfname.c_str());
	break;}
    case ibis::ULONG: {
	ierr = construct<uint64_t>(dfname.c_str());
	break;}
    }
    if (ierr < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- direkte::construct failed with error code "
	    << ierr;
    }
    else {
	if (ibis::gVerbose > 4) {
	    ibis::util::logger lg;
	    print(lg());
	}
	//ierr = write(dt);
	ierr = nnew;
    }
    return ierr;
} // ibis::direkte::append

double ibis::direkte::getSum() const {
    double ret = 0;
    activate(); // need all bitvectors
    for (unsigned j = 0; j < bits.size(); ++ j) {
	if (bits[j])
	    ret += j * bits[j]->cnt();
    }
    return ret;
} // ibis::direkte::getSum

void ibis::direkte::binBoundaries(std::vector<double>& bb) const {
    bb.resize(bits.size());
    for (uint32_t i = 0; i < bits.size(); ++ i)
	bb[i] = i;
} // ibis::direkte::binBoundaries

void ibis::direkte::binWeights(std::vector<uint32_t>& cnts) const {
    activate();
    cnts.resize(bits.size());
    for (uint32_t j = 0; j < bits.size(); ++ j) {
	if (bits[j])
	    cnts[j] = bits[j]->cnt();
	else
	    cnts[j] = 0;
    }
} // ibis::direkte::binWeights

long ibis::direkte::getCumulativeDistribution
(std::vector<double>& bds, std::vector<uint32_t>& cts) const {
    activate();
    bds.resize(bits.size());
    cts.resize(bits.size());
    uint32_t sum = 0;
    for (uint32_t j = 0; j < bits.size(); ++ j) {
	bds[j] = j;
	cts[j] = sum;
	if (bits[j])
	    sum += bits[j]->cnt();
    }
    return cts.size();
} // ibis::direkte::getCumulativeDistribution

long ibis::direkte::getDistribution
(std::vector<double>& bds, std::vector<uint32_t>& cts) const {
    activate();
    bds.reserve(bits.size());
    cts.reserve(bits.size());
    for (uint32_t j = 0; j < bits.size(); ++ j) {
	if (bits[j]) {
	    cts.push_back(bits[j]->cnt());
	    bds.push_back(j+1);
	}
    }
    bds.pop_back();
    return cts.size();
} // ibis::direkte::getDistribution

/// Estiamte the size of the index file.  The index file contains primarily
/// the bitmaps.
size_t ibis::direkte::getSerialSize() const throw () {
    size_t res = 16;
    for (unsigned j = 0; j < bits.size(); ++ j)
	if (bits[j] != 0)
	    res += bits[j]->getSerialSize();
    if (res + ((1+bits.size()) << 2) <= 0x80000000) {
	res += ((1+bits.size()) << 2);
    }
    else {
	res += ((1+bits.size()) << 3);
    }
    return res;
} // ibis::direkte::getSerialSize

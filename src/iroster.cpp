// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
//
// This file contains the implementation of ibis::roster -- an list of indices
// that orders the column values in ascending order.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "iroster.h"
#include "column.h"
#include "part.h"

#include <algorithm>	// std::sort
#include <sstream> // std::ostringstream

////////////////////////////////////////////////////////////////////////
// functions from ibis::iroster
//
/// Construct a roster index from current data.
ibis::roster::roster(const ibis::column* c, const char* f)
    : col(c), inddes(-1) {
    if (c == 0) return;  // nothing can be done
    read(f); // attempt to read the existing index

    if (ind.size() != col->partition()->nRows() &&
	inddes < 0) { // need to build a new roster list
	if (col->partition()->nRows() <
	    ibis::fileManager::bytesFree() / (8+col->elementSize()))
	    icSort(f); // in core sorting
	if (ind.size() != col->partition()->nRows())
	    oocSort(f);	// out of core sorting
    }
//     if (ind.size() == col->partition()->nRows())
// 	writeSorted(f); // make sure .srt file is there

    if (ibis::gVerbose > 4) {
	ibis::util::logger lg(4);
	print(lg.buffer());
    }
} // constructor

// construct a roster index from current data
ibis::roster::roster(const ibis::column* c, const ibis::bitvector& mask,
		     const char* f)
    : col(c), inddes(-1) {
    if (c == 0) return;  // nothing can be done
    read(f); // attempt to read the existing index

    if (ind.size() != col->partition()->nRows() &&
	inddes < 0) { // need to build a new roster list
	if (col->partition()->nRows() <
	    ibis::fileManager::bytesFree() / (8+col->elementSize()))
	    icSort(f); // in core sorting
	if (ind.size() != col->partition()->nRows())
	    oocSort(f);	// out of core sorting
    }
//     if (ind.size() == col->partition()->nRows())
// 	writeSorted(f); // make sure .srt file is there

    if (ibis::gVerbose > 4) {
	ibis::util::logger lg(4);
	print(lg.buffer());
    }
} // constructor

/// Reconstruct from content of a @c fileManager::storage.
/// The content of the file (following the 8-byte header) is
/// the index array @c ind.
ibis::roster::roster(const ibis::column* c,
		     ibis::fileManager::storage* st,
		     uint32_t offset)
    : col(c), ind(st, offset, c->partition()->nRows()), inddes(-1) {
    if (ibis::gVerbose > 8) {
	ibis::util::logger lg(8);
	print(lg.buffer());
    }
}

// the argument is the name of the directory, the file name is
// column::name() + ".ind"
void ibis::roster::write(const char* df) const {
    if (ind.empty()) return;

    std::string fnm;
    if (df == 0) {
	fnm = col->partition()->currentDataDir();
	fnm += DIRSEP;
    }
    else {
	fnm = df;
	uint32_t pos = fnm.rfind(DIRSEP);
	if (pos >= fnm.size()) pos = 0;
	else ++ pos;
	if (strcmp(fnm.c_str()+pos, col->name()) != 0)
	    fnm += DIRSEP;
    }
    uint32_t ierr = fnm.size();
    if (fnm[ierr-1] == DIRSEP)
	fnm += col->name();
    ierr = fnm.size();
    if (fnm[ierr-4] != '.' || fnm[ierr-3] != 'i' ||
	fnm[ierr-2] != 'n' || fnm[ierr-1] != 'd')
	fnm += ".ind";

    FILE* fptr = fopen(fnm.c_str(), "wb");
    if (fptr == 0) {
	col->logWarning("roster::write", "unable to open \"%s\" for write "
			"... %s", fnm.c_str(), (errno ? strerror(errno) :
						"no free stdio stream"));
	return;
    }

    ierr = fwrite(reinterpret_cast<const void*>(ind.begin()),
		  sizeof(uint32_t), ind.size(), fptr);
    if (ierr != ind.size())
	col->logWarning("roster::write", "expected to "
			"write %lu words but only wrote %lu",
			static_cast<long unsigned>(ind.size()),
			static_cast<long unsigned>(ierr));
    ierr = fclose(fptr);

    writeSorted(df);
} // ibis::roster::write

/// Write the sorted values into .srt file.  Attempt to read the whole
/// column into memory first.  If it fails to do so, it will read one value
/// at a time from the original data file.
void ibis::roster::writeSorted(const char *df) const {
    if (ind.empty()) return;

    std::string fnm;
    if (df == 0) {
	fnm = col->partition()->currentDataDir();
	fnm += DIRSEP;
    }
    else {
	fnm = df;
	uint32_t pos = fnm.rfind(DIRSEP);
	if (pos >= fnm.size()) pos = 0;
	else ++ pos;
	if (strcmp(fnm.c_str()+pos, col->name()) != 0)
	    fnm += DIRSEP;
    }
    uint32_t ierr = fnm.size();
    if (fnm[ierr-1] == DIRSEP)
	fnm += col->name();
    ierr = fnm.size();
    if (fnm[ierr-4] == '.' && fnm[ierr-3] == 'i' &&
	fnm[ierr-2] == 'n' && fnm[ierr-1] == 'd') {
	fnm[ierr-3] = 's';
	fnm[ierr-2] = 'r';
	fnm[ierr-1] = 't';
    }
    else if (fnm[ierr-4] != '.' || fnm[ierr-3] != 's' ||
	     fnm[ierr-2] != 'r' || fnm[ierr-1] != 't') {
	fnm += ".srt";
    }

    if (ibis::util::getFileSize(fnm.c_str()) ==
	(off_t)(col->elementSize()*ind.size()))
	return;

    FILE *fptr = fopen(fnm.c_str(), "wb");
    if (fptr == 0) {
	col->logWarning("roster::writeSorted", "fopen(%s) failed",
			fnm.c_str());
	return;
    }

    // data file name share most characters with .srt file
    fnm.erase(fnm.size()-4);
    switch (col->type()) {
    case ibis::UINT: {
	array_t<uint32_t> arr;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), arr);
	if (ierr == 0) {
	    for (uint32_t i = 0; i < ind.size(); ++ i) {
		fwrite(&(arr[ind[i]]), sizeof(unsigned), 1, fptr);
	    }
	}
	else {
	    unsigned tmp;
	    FILE *fpts = fopen(fnm.c_str(), "rb");
	    if (fpts != 0) {
		for (uint32_t i = 0; i < ind.size(); ++ i) {
		    ierr = fseek(fpts, sizeof(unsigned)*ind[i], SEEK_SET);
		    ierr = fread(&tmp, sizeof(unsigned), 1, fpts);
		    if (ierr > 0) {
			ierr = fwrite(&tmp, sizeof(unsigned), 1, fptr);
			if (ierr < sizeof(unsigned))
			    col->logWarning("roster::writeSorted",
					    "failed to value # %lu (%lu)",
					    static_cast<long unsigned>(i),
					    static_cast<long unsigned>(tmp));
		    }
		    else {
			col->logWarning("roster::writeSorted",
					"failed to read value # %lu "
					"(ind[%lu]=%lu) from %s",
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(ind[i]),
					fnm.c_str());
		    }
		}
	    }
	    ierr = 0;
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> arr;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), arr);
	if (ierr == 0) {
	    for (uint32_t i = 0; i < ind.size(); ++ i) {
		fwrite(&(arr[ind[i]]), sizeof(int), 1, fptr);
	    }
	}
	else {
	    int tmp;
	    FILE *fpts = fopen(fnm.c_str(), "rb");
	    if (fpts != 0) {
		for (uint32_t i = 0; i < ind.size(); ++ i) {
		    ierr = fseek(fpts, sizeof(int)*ind[i], SEEK_SET);
		    ierr = fread(&tmp, sizeof(int), 1, fpts);
		    if (ierr > 0) {
			ierr = fwrite(&tmp, sizeof(int), 1, fptr);
			if (ierr < sizeof(int)) {
			    col->logWarning("roster::write",
					    "failed to write value # %lu (%d)",
					    static_cast<long unsigned>(i),
					    tmp);
			}
		    }
		    else {
			col->logWarning("roster::writeSorted",
					"failed to read value # %lu "
					"(ind[%lu]=%lu) from %s",
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(ind[i]),
					fnm.c_str());
		    }
		}
	    }
	    ierr = 0;
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> arr;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), arr);
	if (ierr == 0) {
	    for (uint32_t i = 0; i < ind.size(); ++ i) {
		fwrite(&(arr[ind[i]]), sizeof(float), 1, fptr);
	    }
	}
	else {
	    float tmp;
	    FILE *fpts = fopen(fnm.c_str(), "rb");
	    if (fpts != 0) {
		for (uint32_t i = 0; i < ind.size(); ++ i) {
		    ierr = fseek(fpts, sizeof(float)*ind[i], SEEK_SET);
		    ierr = fread(&tmp, sizeof(float), 1, fpts);
		    if (ierr > 0) {
			ierr = fwrite(&tmp, sizeof(float), 1, fptr);
			if (ierr < sizeof(float))
			    col->logWarning("roster::writeSorted",
					    "failed to value # %lu (%g)",
					    static_cast<long unsigned>(i),
					    tmp);
		    }
		    else {
			col->logWarning("roster::writeSorted",
					"failed to read value # %lu "
					"(ind[%lu]=%lu) from %s",
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(ind[i]),
					fnm.c_str());
		    }
		}
	    }
	    ierr = 0;
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> arr;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), arr);
	if (ierr == 0) {
	    for (uint32_t i = 0; i < ind.size(); ++ i) {
		fwrite(&(arr[ind[i]]), sizeof(double), 1, fptr);
	    }
	}
	else {
	    double tmp;
	    FILE *fpts = fopen(fnm.c_str(), "rb");
	    if (fpts != 0) {
		for (uint32_t i = 0; i < ind.size(); ++ i) {
		    ierr = fseek(fpts, sizeof(double)*ind[i], SEEK_SET);
		    ierr = fread(&tmp, sizeof(double), 1, fpts);
		    if (ierr > 0) {
			ierr = fwrite(&tmp, sizeof(double), 1, fptr);
			if (ierr < sizeof(double))
			    col->logWarning("roster::writeSorted",
					    "failed to value # %lu (%lg)",
					    static_cast<long unsigned>(i),
					    tmp);
		    }
		    else {
			col->logWarning("roster::writeSorted",
					"failed to read value # %lu "
					"(ind[%lu]=%lu) from %s",
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(i),
					static_cast<long unsigned>(ind[i]),
					fnm.c_str());
		    }
		}
	    }
	    ierr = 0;
	}
	break;}
    default: {
	const int t = static_cast<int>(col->type());
	col->logWarning("roster::writeSorted",
			"unable to write column type %s(%d)",
			ibis::TYPESTRING[t], t);
	ierr = 0;
	break;}
    } // switch (col->type())
    fclose(fptr); // close the .srt file

    if (ierr != 0) {
	col->logWarning("roster::writeSorted", "failed to open data file %s",
			fnm.c_str());
    }
} // ibis::roster::writeSorted

// write the content of ind to a file already open
void ibis::roster::write(FILE* fptr) const {
    if (ind.empty()) return;
    uint32_t ierr = fwrite(reinterpret_cast<const void*>(ind.begin()),
			   sizeof(uint32_t), ind.size(), fptr);
    if (ierr != ind.size())
	ibis::util::logMessage("roster::write", "expected to "
			       "write %lu words but only wrote %lu",
			       static_cast<long unsigned>(ind.size()),
			       static_cast<long unsigned>(ierr));
} // ibis::roster::write

void ibis::roster::read(const char* idxf) {
    std::string fnm;
    if (idxf == 0) {
	fnm = col->partition()->currentDataDir();
	fnm += DIRSEP;
    }
    else {
	fnm = idxf;
	uint32_t pos = fnm.rfind(DIRSEP);
	if (pos >= fnm.size()) pos = 0;
	else ++ pos;
	if (strcmp(fnm.c_str()+pos, col->name()) != 0)
	    fnm += DIRSEP;
    }
    long ierr = fnm.size();
    if (fnm[ierr-1] == DIRSEP)
	fnm += col->name();
    ierr = fnm.size();
    if (fnm[ierr-4] != '.' || fnm[ierr-3] != 'i' ||
	fnm[ierr-2] != 'n' || fnm[ierr-1] != 'd') {
	if (fnm[ierr-4] == '.' &&
	    (fnm[ierr-3] == 'i' || fnm[ierr-3] == 's') &&
	    (fnm[ierr-2] == 'd' || fnm[ierr-2] == 'r') &&
	    (fnm[ierr-1] == 'x' || fnm[ierr-1] == 't'))
	    fnm.erase(ierr-4);
	fnm += ".ind";
    }

    size_t nbytes = sizeof(uint32_t)*col->partition()->nRows();
    if (ibis::util::getFileSize(fnm.c_str()) != (off_t)nbytes)
	return;

    if (nbytes < ibis::fileManager::bytesFree()) {
	ind.read(fnm.c_str());
	if (ibis::gVerbose > 4)
	    col->logMessage("roster", "read the content of %s into memory",
			    fnm.c_str());
    }
    else {
	inddes = UnixOpen(fnm.c_str(), OPEN_READONLY);
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(inddes, _O_BINARY);
#endif
	if (inddes < 0)
	    col->logMessage("roster", "Warning -- read(%s) failed to open "
			    "the name file", fnm.c_str());
	else if (ibis::gVerbose > 4)
	    col->logMessage("roster", "successfully openned file %s for "
			    "future read operations", fnm.c_str());
    }
} // ibis::roster::read

void ibis::roster::read(ibis::fileManager::storage* st) {
    if (st == 0) return;
    array_t<uint32_t> tmp(st, 0, col->partition()->nRows());
    ind.swap(tmp);
} // ibis::roster::read

/// The in-core sorting function.  Reads the content of the specified file
/// into memrory and sort the values through a simple stable sorting
/// procedure.
void ibis::roster::icSort(const char* fin) {
    std::string fnm;
    if (fin == 0) {
	fnm = col->partition()->currentDataDir();
	fnm += DIRSEP;
    }
    else {
	fnm = fin;
	uint32_t pos = fnm.rfind(DIRSEP);
	if (pos >= fnm.size()) pos = 0;
	else ++ pos;
	if (strcmp(fnm.c_str()+pos, col->name()) != 0)
	    fnm += DIRSEP;
    }
    long ierr = fnm.size();
    if (fnm[ierr-1] == DIRSEP)
	fnm += col->name();
    ibis::horometer timer;
    if (ibis::gVerbose > 1) {
	timer.start();
	col->logMessage("roster::icSort", "attempt to sort the content "
			"of file (%s) in memory", fnm.c_str());
    }

    switch (col->type()) {
    case ibis::UINT: { // unsigned int
	array_t<uint32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    const_cast<const array_t<uint32_t>&>(val).stableSort(ind);
#if defined(DEBUG) && DEBUG + 0 > 1
	    unsigned tmp;
	    uint32_t i = 0, j = 0;
	    ibis::util::logger lg(4);
	    const uint32_t n = ind.size();
	    lg.buffer() << "ibis::roster::icSort -- value, starting "
		"position, count\n";
	    while (i < n) {
		tmp = val[ind[i]];
		++ j;
		while (j < n && tmp == val[ind[j]])
		    ++ j;
		lg.buffer() << tmp << "\t" << i << "\t" << j - i << "\n";
		i = j;
	    }
#endif
	}
	break;
    }
    case ibis::INT: { // signed int
	array_t<int32_t> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    const_cast<const array_t<int32_t>&>(val).stableSort(ind);
#if defined(DEBUG) && DEBUG + 0 > 1
	    int tmp;
	    uint32_t i = 0, j = 0;
	    ibis::util::logger lg(4);
	    const uint32_t n = ind.size();
	    lg.buffer() << "ibis::roster::icSort -- value, starting "
		"position, count\n";
	    while (i < n) {
		tmp = val[ind[i]];
		++ j;
		while (j < n && tmp == val[ind[j]])
		    ++ j;
		lg.buffer() << tmp << "\t" << i << "\t" << j - i << "\n";
		i = j;
	    }
#endif
	}
	break;
    }
    case ibis::FLOAT: { // float
	array_t<float> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    const_cast<const array_t<float>&>(val).stableSort(ind);
#if defined(DEBUG) && DEBUG + 0 > 1
	    float tmp;
	    uint32_t i = 0, j = 0;
	    ibis::util::logger lg(4);
	    const uint32_t n = ind.size();
	    lg.buffer() << "ibis::roster::icSort -- value, starting "
		"position, count\n";
	    while (i < n) {
		tmp = val[ind[i]];
		++ j;
		while (j < n && tmp == val[ind[j]])
		    ++ j;
		lg.buffer() << tmp << "\t" << i << "\t" << j - i << "\n";
		i = j;
	    }
#endif
	}
	break;
    }
    case ibis::DOUBLE: { // double
	array_t<double> val;
	ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (val.size() > 0) {
	    const_cast<const array_t<double>&>(val).stableSort(ind);
#if defined(DEBUG) && DEBUG + 0 > 1
	    double tmp;
	    uint32_t i = 0, j = 0;
	    ibis::util::logger lg(4);
	    const uint32_t n = ind.size();
	    lg.buffer() << "ibis::roster::icSort -- value, starting "
		"position, count\n";
	    while (i < n) {
		tmp = val[ind[i]];
		++ j;
		while (j < n && tmp == val[ind[j]])
		    ++ j;
		lg.buffer() << tmp << "\t" << i << "\t" << j - i << "\n";
		i = j;
	    }
#endif
	}
	break;}
    case ibis::CATEGORY: { // no need for a separate index
	col->logWarning("roster", "no need for a separate index");
	break;}
    default: {
	ibis::util::logger lg(4);
	lg.buffer() << "roster -- unable to create an index for ";
	col->print(lg.buffer());
	break;}
    }

    // write out the current content
    write(static_cast<const char*>(0)); // write .ind file
    if (ibis::gVerbose > 2) {
	timer.stop();
	col->logMessage("roster::icSort", "in-core sorting of %lu numbers "
			"from %s took %g sec(CPU), %g sec(elapsed)",
			static_cast<long unsigned>(ind.size()), fnm.c_str(),
			timer.CPUTime(), timer.realTime());
    }
    if (ibis::gVerbose > 4 &&
	(ibis::gVerbose > 30 || ((1U<<ibis::gVerbose) > ind.size()))) {
 	ibis::util::logger lg(4);
 	print(lg.buffer());
    }
} // ibis::roster::icSort

/// The out-of-core sorting function.  Internally it uses four data files.
/// It eventully removes two of them and leaves only with two with the
/// extension of @c .srt and @c .ind.  The two files have the same content
/// as @c .ind and @c .srt produced by the functions @c write and @c
/// writeSorted.
void ibis::roster::oocSort(const char *fin) {
    if (ind.size() == col->partition()->nRows()) return;
    ind.clear(); // clear the index array.
    ibis::horometer timer;
    if (ibis::gVerbose > 1) {
	timer.start();
	col->logMessage("roster::oocSort",
			"attempt to sort the attribute %s out of core",
			col->name());
    }

    // nsrt is the name of the final sorted data file
    // nind is the name of the final index file
    // msrt is the intermediate sorted data file, will be removed later
    // mind is the intermediate index file, will be removed later
    std::string nsrt, nind, msrt, mind;
    if (fin == 0) {
	nind = col->partition()->currentDataDir();
	nind += DIRSEP;
    }
    else {
	nind = fin;
	uint32_t pos = nind.rfind(DIRSEP);
	if (pos >= nind.size()) pos = 0;
	else ++ pos;
	if (strcmp(nind.c_str()+pos, col->name()) != 0)
	    nind += DIRSEP;
    }
    long ierr = nind.size();
    if (nind[ierr-1] == DIRSEP)
	nind += col->name();
    if (nind[ierr-4] != '.' || nind[ierr-3] != 'i' ||
	nind[ierr-2] != 'n' || nind[ierr-1] != 'd') {
	if (nind[ierr-4] == '.' &&
	    (nind[ierr-3] == 'i' || nind[ierr-3] == 's') &&
	    (nind[ierr-2] == 'd' || nind[ierr-2] == 'r') &&
	    (nind[ierr-1] == 'x' || nind[ierr-1] == 't'))
	    nind.erase(ierr-4);
	nind += ".ind";
    }
    const size_t nrows = col->partition()->nRows();
    if (ibis::util::getFileSize(nind.c_str()) ==
	(off_t)(sizeof(uint32_t) * nrows)) {
	// open the ind file in read only mode for future operaions.
	inddes = UnixOpen(nind.c_str(), OPEN_READONLY);
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(inddes, _O_BINARY);
#endif
	return;
    }

    nsrt = nind;
    nsrt.erase(nsrt.size()-3);
    nsrt += "srt";
    std::string datafile = nind; // name of original data file
    datafile.erase(datafile.size()-4);

    mind = col->partition()->name();
    mind += ".cacheDirectory";
    const char *tmp = ibis::gParameters()[mind.c_str()];
    if (tmp != 0) {
	msrt = tmp;
	msrt += DIRSEP;
	msrt += col->partition()->name();
	msrt += '.';
	msrt += col->name();
	mind = msrt;
	msrt += ".srt";
	mind += ".ind";
    }
    else {
	msrt = nsrt;
	mind = nind;
	msrt += "-tmp";
	mind += "-tmp";
    }
    // read 256K elements at a time
    const uint32_t mblock = PREFERRED_BLOCK_SIZE;
    array_t<uint32_t> ibuf1(mblock), ibuf2(mblock);

    ierr = nrows / mblock;
    const uint32_t nblock = ierr + (nrows > static_cast<size_t>(ierr) * mblock);
    ierr = 1;
    for (uint32_t i = nblock; i > 1; ++ierr, i>>=1);
    const bool isodd = (ierr%2 == 1);
    uint32_t stride = mblock;

    switch (col->type()) {
    case ibis::UINT:
    case ibis::CATEGORY: {
	array_t<uint32_t> dbuf1(mblock), dbuf2(mblock);
	if (isodd) {
	    ierr = oocSortBlocks(datafile.c_str(), nsrt.c_str(), nind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	}
	else {
	    ierr = oocSortBlocks(datafile.c_str(), msrt.c_str(), mind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	    if (ierr == 0)
		ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				      mind.c_str(), nind.c_str(),
				      mblock, stride, dbuf1, dbuf2,
				      ibuf1, ibuf2);
	    stride += stride;
	}
	while (ierr == 0 && stride < nrows) {
	    ierr = oocMergeBlocks(nsrt.c_str(), msrt.c_str(),
				  nind.c_str(), mind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    if (ierr != 0) break;
	    stride += stride;
	    ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				  mind.c_str(), nind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    stride += stride;
	}
	break;}
    case ibis::INT: {
	array_t<int32_t> dbuf1(mblock), dbuf2(mblock);
	if (isodd) {
	    ierr = oocSortBlocks(datafile.c_str(), nsrt.c_str(), nind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	}
	else {
	    ierr = oocSortBlocks(datafile.c_str(), msrt.c_str(), mind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	    if (ierr == 0)
		ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				      mind.c_str(), nind.c_str(),
				      mblock, stride, dbuf1, dbuf2,
				      ibuf1, ibuf2);
	    stride += stride;
	}
	while (ierr == 0 && stride < nrows) {
	    ierr = oocMergeBlocks(nsrt.c_str(), msrt.c_str(),
				  nind.c_str(), mind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    if (ierr != 0) break;
	    stride += stride;
	    ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				  mind.c_str(), nind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    stride += stride;
	}
	break;}
    case ibis::FLOAT: {
	array_t<float> dbuf1(mblock), dbuf2(mblock);
	if (isodd) {
	    ierr = oocSortBlocks(datafile.c_str(), nsrt.c_str(), nind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	}
	else {
	    ierr = oocSortBlocks(datafile.c_str(), msrt.c_str(), mind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	    if (ierr == 0)
		ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				      mind.c_str(), nind.c_str(),
				      mblock, stride, dbuf1, dbuf2,
				      ibuf1, ibuf2);
	    stride += stride;
	}
	while (ierr == 0 && stride < nrows) {
	    ierr = oocMergeBlocks(nsrt.c_str(), msrt.c_str(),
				  nind.c_str(), mind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    if (ierr != 0) break;
	    stride += stride;
	    ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				  mind.c_str(), nind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    stride += stride;
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double> dbuf1(mblock), dbuf2(mblock);
	if (isodd) {
	    ierr = oocSortBlocks(datafile.c_str(), nsrt.c_str(), nind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	}
	else {
	    ierr = oocSortBlocks(datafile.c_str(), msrt.c_str(), mind.c_str(),
				 mblock, dbuf1, dbuf2, ibuf1);
	    if (ierr == 0)
		ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				      mind.c_str(), nind.c_str(),
				      mblock, stride, dbuf1, dbuf2,
				      ibuf1, ibuf2);
	    stride += stride;
	}
	while (ierr == 0 && stride < nrows) {
	    ierr = oocMergeBlocks(nsrt.c_str(), msrt.c_str(),
				  nind.c_str(), mind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    if (ierr != 0) break;
	    stride += stride;
	    ierr = oocMergeBlocks(msrt.c_str(), nsrt.c_str(),
				  mind.c_str(), nind.c_str(),
				  mblock, stride,
				  dbuf1, dbuf2, ibuf1, ibuf2);
	    stride += stride;
	}
	break;}
    default: {
	col->logWarning("roster::oocSort", "can not process column type %d",
			static_cast<int>(col->type()));
	break;}
    }

    remove(msrt.c_str());
    remove(mind.c_str());
    if (ierr < 0) {
	remove(nsrt.c_str());
	remove(nind.c_str());
	col->logWarning("roster::oocSort", "unable to complete the "
			"out-of-core sorting of %s. ierr = %d. all "
			"output files removed",
			datafile.c_str(), ierr);
	return;
    }
    else if (ibis::gVerbose > 2) {
	timer.stop();
	col->logMessage("roster::oocSort", "out-of-core sorting (%s -> %s "
			"(%s)) took %g sec(CPU), %g sec(elapsed)",
			datafile.c_str(), nsrt.c_str(), nind.c_str(),
			timer.CPUTime(), timer.realTime());
    }
    if (ibis::gVerbose > 4 &&
	(ibis::gVerbose > 30 || ((1U<<ibis::gVerbose) > ind.size()))) {
 	ibis::util::logger lg(4);
 	print(lg.buffer());
    }

    // open the ind file in read only mode for future operaions.
    inddes = UnixOpen(nind.c_str(), OPEN_READONLY);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(inddes, _O_BINARY);
#endif
} // ibis::roster::oocSort

/// Read the content of file @c src one block at a time, sort each block
/// and write it to file @c dest.  At the same time produce an index array
/// and write it to file @c ind.  The block size is determined by @c mblock.
template <class T>
long ibis::roster::oocSortBlocks(const char *src, const char *dest,
				const char *ind, const uint32_t mblock,
				array_t<T>& dbuf1, array_t<T>& dbuf2,
				array_t<uint32_t>& ibuf) const {
    int fdsrc = UnixOpen(src, OPEN_READONLY);
    if (fdsrc < 0) {
	ibis::util::logMessage("Warning",
			       "oocSortBlocks failed to open %s for reading",
			       src);
	return -1;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdsrc, _O_BINARY);
#endif
    int fddes = UnixOpen(dest, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fddes < 0) {
	ibis::util::logMessage("Warning",
			       "oocSortBlocks failed to open %s for writing",
			       dest);
	UnixClose(fdsrc);
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fddes, _O_BINARY);
#endif
    int fdind = UnixOpen(ind, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdind < 0) {
	ibis::util::logMessage("Warning",
			       "oocSortBlocks failed to open %s for writing",
			       ind);
	UnixClose(fddes);
	UnixClose(fdsrc);
	return -3;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdind, _O_BINARY);
#endif

    const uint32_t szi = sizeof(uint32_t);
    const uint32_t szd = sizeof(T);
    const uint32_t nrows = col->partition()->nRows();
    ibis::horometer timer;
    timer.start();
    ibuf.resize(mblock);
    dbuf1.resize(mblock);
    dbuf2.resize(mblock);
    long ierr = 0;
    for (uint32_t i = 0; ierr == 0 && i < nrows; i += mblock) {
	if (ibis::gVerbose > 12)
	    col->logMessage("roster::oocSortBlocks",
			    "sorting block %lu",
			    static_cast<long unsigned>(i));

	const uint32_t block = (i+mblock <= nrows ? mblock : nrows-i);
	ierr = dbuf1.read(fdsrc, i*szd, (i+block)*szd);
	if (static_cast<uint32_t>(ierr) != block*szd) {
	    ibis::util::logMessage
		("Warning", "oocSortBlocks expected to read "
		 "%lu bytes from %s at %lu, but only got %ld",
		 static_cast<long unsigned>(block*szd), src,
		 static_cast<long unsigned>(i*szd), ierr);
	    ierr = -11;
	    break;
	}
	for (uint32_t j = 0; j < block; ++ j)
	    ibuf[j] = j;
	ibuf.resize(block);
	dbuf1.sort(ibuf);

	// the indices need to be shifted by @c i.  Sorted values in @c dbuf2
	for (uint32_t j = 0; j < block; ++ j) {
	    dbuf2[j] = dbuf1[ibuf[j]];
	    ibuf[j] += i;
	}
	// write the sorted values.
	ierr = UnixWrite(fddes, dbuf2.begin(), szd*block);
	if (static_cast<uint32_t>(ierr) != block*szd) {
	    ibis::util::logMessage("Warning", "oocSortBlocks expected to "
				   "write %lu bytes to %s at %lu, but only "
				   "wrote %ld",
				   static_cast<long unsigned>(block*szd), dest,
				   static_cast<long unsigned>(i*szd), ierr);
	    ierr = -12;
	    break;
	}
	// write the indices.
	ierr = UnixWrite(fdind, ibuf.begin(), block*szi);
	if (static_cast<uint32_t>(ierr) != block*szi) {
	    ibis::util::logMessage("Warning", "oocSortBlocks expected to "
				   "write %lu bytes to %s at %lu, but only "
				   "wrote %ld",
				   static_cast<long unsigned>(block*szi),
				   ind, static_cast<long unsigned>(i*szi),
				   ierr);
	    ierr = -12;
	    break;
	}
	else {
	    ierr = 0;
	}
    }

#if defined(_WIN32) && defined(_MSC_VER)
    _commit(fddes);
    _commit(fdind);
#endif
    UnixClose(fdind);
    UnixClose(fddes);
    UnixClose(fdsrc);
    if (ierr < 0) { // remove the output files
	remove(ind);
	remove(dest);
	ibis::util::logMessage("Warning", "roster::oocSortBlocks failed with "
			       "ierr = %d", ierr);
    }
    else if (ibis::gVerbose > 3) {
	ierr = 0;
	timer.stop();
	double speed = 1e-6 * (szd + szd + szi) * nrows;
	speed /= (timer.realTime() > 1.0e-6 ? timer.realTime() : 1.0e-6);
	col->logMessage("roster::oocSortBlocks",
			"completed sorting all blocks (%lu) of %s, wrote "
			"results to %s and %s, used %g sec with %g MB/s",
			static_cast<long unsigned>(mblock), src, dest,
			ind, timer.realTime(), speed);
    }
    return ierr;
} // ibis::roster::oocSortBlocks

/// Merge two consecutive blocks of size @c stride from file @c dsrc and
/// write the results into a new file called @c dout.  An index file is
/// rearranged along with the data values.  The input index file is @c isrc
/// and the output index file is @c iout.  The content of the files are
/// read into memory one block at a time and the block size is defined by
/// @c mblock.  The temporary work arrays are passed in by the caller to
/// make sure there is no chance of running out of memory within this
/// function.
template <class T>
long ibis::roster::oocMergeBlocks(const char *dsrc, const char *dout,
				  const char *isrc, const char *iout,
				  const uint32_t mblock,
				  const uint32_t stride,
				  array_t<T>& dbuf1,
				  array_t<T>& dbuf2,
				  array_t<uint32_t>& ibuf1,
				  array_t<uint32_t>& ibuf2) const {
    const int fdsrc = UnixOpen(dsrc, OPEN_READONLY);
    if (fdsrc < 0) {
	ibis::util::logMessage("Warning",
			       "oocMergeBlocks failed to open %s for reading",
			       dsrc);
	return -1;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdsrc, _O_BINARY);
#endif
    const int fisrc = UnixOpen(isrc, OPEN_READONLY);
    if (fisrc < 0) {
	ibis::util::logMessage("Warning",
			       "oocMergeBlocks failed to open %s for reading",
			       isrc);
	UnixClose(fdsrc);
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fisrc, _O_BINARY);
#endif
    const int fdout = UnixOpen(dout, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdout < 0) {
	ibis::util::logMessage("Warning",
			       "oocMergeBlocks failed to open %s for writing",
			       dout);
	UnixClose(fisrc);
	UnixClose(fdsrc);
	return -3;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdout, _O_BINARY);
#endif
    const int fiout = UnixOpen(iout, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fiout < 0) {
	ibis::util::logMessage("Warning",
			       "oocMergeBlocks failed to open %s for writing",
			       iout);
	UnixClose(fdout);
	UnixClose(fisrc);
	UnixClose(fdsrc);
	return -4;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fiout, _O_BINARY);
#endif

    ibis::horometer timer;
    timer.start();
    ibuf1.resize(mblock);
    ibuf2.resize(mblock);
    dbuf1.resize(mblock);
    dbuf2.resize(mblock);

    const uint32_t szd = sizeof(T);
    const uint32_t szi = sizeof(uint32_t);
    const uint32_t bszd = szd*mblock;
    const uint32_t bszi = szi*mblock;
    const uint32_t nrows = col->partition()->nRows();
    long ierr = nrows / mblock;
    const uint32_t nblock = ierr + (nrows > mblock * ierr);

    ierr = 0;
    for (uint32_t i0 = 0; ierr == 0 && i0 < nrows; i0 += 2*stride) {
	uint32_t i1 = i0 + stride;
	if (i1 < nrows) { // have two large blocks to merge
	    // logically we are working with two large blocks next to each
	    // other.  The first one [i0:i1] is guaranteed to have @c stride
	    // elements and the second one [i1:i2] may have less.
	    const uint32_t i2 = (i1+stride <= nrows ? i1+stride : nrows);
	    uint32_t i01 = i0; // index for pages within the first block
	    uint32_t i12 = i1; // index for pages within the second block
	    uint32_t j01 = 0;
	    uint32_t j12 = 0;
	    uint32_t block = (i12+mblock <= i2 ? mblock : i2 - i12);
	    dbuf2.resize(block);
	    ibuf2.resize(block);
	    uint32_t cszd = block * szd;
	    uint32_t cszi = block * szi;
	    uint32_t szdi1 = i01 * szd;
	    uint32_t szii1 = i01 * szi;
	    uint32_t szdi2 = i12 * szd;
	    uint32_t szii2 = i12 * szi;

	    // read two pages from the input data file and two pages from
	    // the input index file
	    ierr = dbuf1.read(fdsrc, szdi1, szdi1+bszd);
	    if (static_cast<uint32_t>(ierr) != bszd) {
		ibis::util::logMessage
		    ("Warning", "oocMergeBlocks failed to read %lu bytes "
		     "at %lu from %s", static_cast<long unsigned>(bszd),
		     static_cast<long unsigned>(szdi1), dsrc);
		ierr = -19;
	    }
	    else {
		ierr = dbuf2.read(fdsrc, szdi2, szdi2+cszd);
		if (static_cast<uint32_t>(ierr) != cszd) {
		    ibis::util::logMessage
			("Warning", "oocMergeBlocks failed to read %lu bytes "
			 "at %lu from %s", static_cast<long unsigned>(cszd),
			 static_cast<long unsigned>(szdi2), dsrc);
		    ierr = -20;
		}
		else {
		    ierr = ibuf1.read(fisrc, szii1, szii1+bszi);
		    if (static_cast<uint32_t>(ierr) != bszi) {
			ibis::util::logMessage
			    ("Warning", "oocMergeBlocks failed to read %lu "
			     "bytes at %lu from %s",
			     static_cast<long unsigned>(bszi),
			     static_cast<long unsigned>(szii1), isrc);
			ierr = -21;
		    }
		    else {
			ierr = ibuf2.read(fisrc, szii2, szii2+cszi);
			if (static_cast<uint32_t>(ierr) != cszi) {
			    ibis::util::logMessage
				("Warning", "oocMergeBlocks failed to read %lu "
				 "bytes at %lu from %s",
				 static_cast<long unsigned>(cszi),
				 static_cast<long unsigned>(szii2), isrc);
			    ierr = -22;
			}
			else {
			    ierr = 0;
			}
		    }
		}
	    }

	    // loop over all pages in the two consecutive blocks
	    while (ierr == 0 && (i01 < i1 || i12 < i2)) {
		if (i01 < i1 && i12 < i2) { // both blocks have pages left
		    while (j01 < mblock && j12 < block) { // both pages useful
			if (dbuf1[j01] <= dbuf2[j12]) { // output j01
			    ierr = UnixWrite(fdout, &(dbuf1[j01]), szd);
			    if (static_cast<uint32_t>(ierr) != szd) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to "
				     "write data value # %lu to %s",
				     static_cast<long unsigned>(i01+j01),
				     dout);
				ierr = -23;
				break;
			    }
			    ierr = UnixWrite(fiout, &(ibuf1[j01]), szi);
			    if (static_cast<uint32_t>(ierr) != szi) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to "
				     "write data value # %lu to %s",
				     static_cast<long unsigned>(i01+j01),
				     iout);
				ierr = -24;
				break;
			    }
			    ierr = 0;
			    ++ j01;
			}
			else { // output the value at j12
			    ierr = UnixWrite(fdout, &(dbuf2[j12]), szd);
			    if (static_cast<uint32_t>(ierr) != szd) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to "
				     "write data value # %lu to %s",
				     static_cast<long unsigned>(i12+j12),
				     dout);
				ierr = -25;
				break;
			    }
			    ierr = UnixWrite(fiout, &(ibuf2[j12]), szi);
			    if (static_cast<uint32_t>(ierr) != szi) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to "
				     "write data value # %lu to %s",
				     static_cast<long unsigned>(i12+j12),
				     iout);
				ierr = -26;
				break;
			    }
			    ierr = 0;
			    ++ j12;
			}
		    } // j01 < mblock && j12 < block
		}
		else if (i01 < i1) { // block still have more pages
		    for (; j01 < mblock; ++ j01) { // output all elements
			ierr = UnixWrite(fdout, &(dbuf1[j01]), szd);
			if (static_cast<uint32_t>(ierr) != szd) {
			    ibis::util::logMessage
				("Warning", "oocMergeBlocks failed to "
				 "write data value # %lu to %s",
				 static_cast<long unsigned>(i01+j01), dout);
			    ierr = -27;
			    break;
			}
			ierr = UnixWrite(fiout, &(ibuf1[j01]), szi);
			if (static_cast<uint32_t>(ierr) != szi) {
			    ibis::util::logMessage
				("Warning", "oocMergeBlocks failed to "
				 "write data value # %lu to %s",
				 static_cast<long unsigned>(i01+j01), iout);
			    ierr = -28;
			    break;
			}
			ierr = 0;
		    } // j01
		}
		else { // i12 < i2, block two has more pages
		    for (; j12 < block; ++ j12) { // output all elements
			ierr = UnixWrite(fdout, &(dbuf2[j12]), szd);
			if (static_cast<uint32_t>(ierr) != szd) {
			    ibis::util::logMessage
				("Warning", "oocMergeBlocks failed to "
				 "write data value # %lu to %s",
				 static_cast<long unsigned>(i12+j12), dout);
			    ierr = -29;
			    break;
			}
			ierr = UnixWrite(fiout, &(ibuf2[j12]), szi);
			if (static_cast<uint32_t>(ierr) != szi) {
			    ibis::util::logMessage
				("Warning", "oocMergeBlocks failed to "
				 "write data value # %lu to %s",
				 static_cast<long unsigned>(i12+j12), iout);
			    ierr = -30;
			    break;
			}
			ierr = 0;
		    } // j12
		}

		if (ierr == 0) {
		    if (j01 >= mblock) { // read next page in block one
			i01 += mblock;
			if (i01 < i1) {
			    szdi1 += bszd;
			    szii1 += bszi;
			    j01 = 0;
			    ierr = dbuf1.read(fdsrc, szdi1, szdi1+bszd);
			    if (static_cast<uint32_t>(ierr) != bszd) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to read "
				     "%lu bytes at %lu from %s",
				     static_cast<long unsigned>(bszd),
				     static_cast<long unsigned>(szdi1), dsrc);
				ierr = -31;
			    }
			    else {
				ierr = ibuf1.read(fisrc, szii1, szii1+bszi);
				if (static_cast<uint32_t>(ierr) != bszi) {
				    ibis::util::logMessage
					("Warning", "oocMergeBlocks failed to "
					 "read %lu bytes at %lu from %s",
					 static_cast<long unsigned>(bszi),
					 static_cast<long unsigned>(szii1),
					 isrc);
				    ierr = -33;
				}
				else {
				    ierr = 0;
				}
			    }
			}
		    }
		    if (j12 >= block) { // read next page in block two
			j12 = 0;
			i12 += block;
			if (i12 < i2) {
			    szdi2 += cszd;
			    szii2 += cszi;
			    block = (i12+mblock <= i2 ? mblock : i2 - i12);
			    cszd = szd * block;
			    cszi = szi * block;
			    ierr = dbuf2.read(fdsrc, szdi2, szdi2+cszd);
			    if (static_cast<uint32_t>(ierr) != cszd) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to read "
				     "%lu bytes at %lu from %s",
				     static_cast<long unsigned>(cszd),
				     static_cast<long unsigned>(szdi2), dsrc);
				ierr = -32;
			    }
			    else {
				ierr = ibuf2.read(fisrc, szii2, szii2+cszi);
				if (static_cast<uint32_t>(ierr) != cszi) {
				    ibis::util::logMessage
					("Warning", "oocMergeBlocks failed to "
					 "read %lu bytes at %lu from %s",
					 static_cast<long unsigned>(cszi),
					 static_cast<long unsigned>(szii2),
					 isrc);
				    ierr = -34;
				}
				else {
				    ierr = 0;
				}
			    }
			}
		    }
		}
	    }
	}
	else { // only one block remain in the input files, copy the block
	    for (uint32_t i = i0; i+mblock <= nrows; i += mblock) {
		// copy of the pages in the last block
		const uint32_t szdi = szd * i;
		const uint32_t szii = szi * i;
		ierr = dbuf1.read(fdsrc, szdi, szdi + bszd);
		if (static_cast<uint32_t>(ierr) != bszd) {
		    ibis::util::logMessage
			("Warning", "oocMergeBlocks failed to read %lu bytes "
			 "at %lu from %s", static_cast<long unsigned>(bszd),
			 static_cast<long unsigned>(szdi), dsrc);
		    ierr = -11;
		    break;
		}
		ierr = UnixWrite(fdout, dbuf1.begin(), bszd);
		if (static_cast<uint32_t>(ierr) != bszd) {
		    ibis::util::logMessage
			("Warning", "oocMergeBlocks failed to read %lu bytes "
			 "at %lu from %s", static_cast<long unsigned>(bszd),
			 static_cast<long unsigned>(szdi), dout);
		    ierr = -12;
		    break;
		}
		ierr = ibuf1.read(fisrc, szii, szii + bszi);
		if (static_cast<uint32_t>(ierr) != bszd) {
		    ibis::util::logMessage
			("Warning", "oocMergeBlocks failed to read %lu bytes "
			 "at %lu from %s", static_cast<long unsigned>(bszi),
			 static_cast<long unsigned>(szii), isrc);
		    ierr = -13;
		    break;
		}
		ierr = UnixWrite(fiout, ibuf1.begin(), bszi);
		if (static_cast<uint32_t>(ierr) != bszi) {
		    ibis::util::logMessage
			("Warning", "oocMergeBlocks failed to write %lu bytes "
			 "at %lu from %s", static_cast<long unsigned>(bszi),
			 static_cast<long unsigned>(szii), iout);
		    ierr = -14;
		    break;
		}
		ierr = 0;
	    } // i
	    if (ierr == 0 && nblock > nrows / mblock) {
		// copy the last page that is partially full
		const uint32_t szdi = szd * mblock * (nblock - 1);
		const uint32_t szii = szi * mblock * (nblock - 1);
		const uint32_t block = nrows - mblock * (nblock - 1);
		const uint32_t cszd = block * szd;
		const uint32_t cszi = block * szi;
		dbuf1.resize(block);
		ibuf1.resize(block);
		ierr = dbuf1.read(fdsrc, szdi, szdi + cszd);
		if (static_cast<uint32_t>(ierr) != cszd) {
		    ibis::util::logMessage
			("Warning", "oocMergeBlocks failed to read %lu bytes "
			 "at %lu from %s", static_cast<long unsigned>(cszd),
			 static_cast<long unsigned>(szdi), dsrc);
		    ierr = -15;
		}
		else {
		    ierr = UnixWrite(fdout, dbuf1.begin(), cszd);
		    if (static_cast<uint32_t>(ierr) != cszd) {
			ibis::util::logMessage
			    ("Warning", "oocMergeBlocks failed to read %lu "
			     "bytes at %lu from %s",
			     static_cast<long unsigned>(cszd),
			     static_cast<long unsigned>(szdi), dout);
			ierr = -16;
		    }
		    else {
			ierr = ibuf1.read(fisrc, szii, szii + cszi);
			if (static_cast<uint32_t>(ierr) != cszi) {
			    ibis::util::logMessage
				("Warning", "oocMergeBlocks failed to read %lu "
				 "bytes at %lu from %s",
				 static_cast<long unsigned>(cszi),
				 static_cast<long unsigned>(szii), isrc);
			    ierr = -17;
			}
			else {
			    ierr = UnixWrite(fiout, ibuf1.begin(), cszi);
			    if (static_cast<uint32_t>(ierr) != cszi) {
				ibis::util::logMessage
				    ("Warning", "oocMergeBlocks failed to "
				     "write %lu bytes at %lu from %s",
				     static_cast<long unsigned>(cszi),
				     static_cast<long unsigned>(szii), iout);
				ierr = -18;
			    }
			    else {
				ierr = 0;
			    }
			}
		    }
		}
	    } // left overs
	}
    } // i0

#if defined(_WIN32) && defined(_MSC_VER)
    _commit(fiout);
    _commit(fdout);
#endif
    UnixClose(fiout);
    UnixClose(fdout);
    UnixClose(fisrc);
    UnixClose(fdsrc);

    if (ierr != 0) { // remove the output in case of error
	remove(dout);
	remove(iout);
	ibis::util::logMessage("Warning", "roster::oocMergeBlocks failed with "
			       "ierr = %d", ierr);
    }
    else if (ibis::gVerbose > 3) {
	ierr = 0;
	timer.stop();
	double speed = 2e-6 * ((szd+szi) * nrows);
	speed /= (timer.realTime() > 1e-6 ? timer.realTime() : 1e-6);
	col->logMessage("roster::oocMergeBlocks", "completed merging blocks "
			"of size %lu, written output to %s (%s), used %g sec "
			"with %g MB/s",	static_cast<long unsigned>(stride),
			dout, iout, timer.realTime(), speed);
    }
    return ierr;
} //ibis::roster::oocMergeBlocks

template <class T>
long ibis::roster::mergeBlock2(const char *dsrc, const char *dout,
			       const uint32_t segment, array_t<T>& buf1,
			       array_t<T>& buf2, array_t<T>& buf3) {
    const int fdsrc = UnixOpen(dsrc, OPEN_READONLY);
    if (fdsrc < 0) {
	ibis::util::logMessage("Warning",
			       "mergeBlock2 failed to open %s for reading",
			       dsrc);
	return -1;
    }
    const int fdout = UnixOpen(dout, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (fdout < 0) {
	ibis::util::logMessage("Warning",
			       "mergeBlock2 failed to open %s for writing",
			       dout);
	UnixClose(fdsrc);
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdout, _O_BINARY);
#endif

    uint32_t mblock = (buf1.size() <= buf2.size() ? buf1.size() : buf2.size());
    if (mblock > buf3.size())
	mblock = buf3.size();
    buf1.resize(mblock);
    buf2.resize(mblock);
    buf3.resize(mblock);
    std::less<T> cmp;

    ibis::horometer timer;
    timer.start();

    long ierr = 0;
    bool more = true;
    uint32_t totread = 0;
    const uint32_t szd = sizeof(T);
    const uint32_t bszd = sizeof(T)*mblock;

    for (uint32_t i0 = 0; more; i0 += 2*segment) {
	const uint32_t i1 = i0 + segment;
	ierr = UnixSeek(fdsrc, i1*szd, SEEK_SET);
	if (ierr == 0) {	// have two segments to merge
	    // logically we are working with two large blocks next to each
	    // other.  The first one [i0:i1] is guaranteed to have @c segment
	    // elements and the second one [i1:i2] may have less.
	    uint32_t i2 = i1+segment;
	    uint32_t i01 = i0; // index for pages within the first block
	    uint32_t i12 = i1; // index for pages within the second block
	    uint32_t j01 = 0;
	    uint32_t j12 = 0;
	    uint32_t block2 = mblock;
	    uint32_t szdi1 = i01 * szd;
	    uint32_t szdi2 = i12 * szd;

	    // read two pages from the input data file and two pages from
	    // the input index file
	    ierr = buf1.read(fdsrc, szdi1, szdi1+bszd);
	    if (ierr != static_cast<long>(bszd)) {
		ibis::util::logMessage
		    ("Warning", "mergeBlock2 failed to read %lu bytes "
		     "at %lu from %s", static_cast<long unsigned>(bszd),
		     static_cast<long unsigned>(szdi1), dsrc);
		ierr = -3;
		more = false;
		break;
	    }
	    totread += ierr;
	    ierr = buf2.read(fdsrc, szdi2, szdi2+bszd);
	    if (ierr >= 0) {
		block2 = ierr / szd;
		i2 = i12 + block2;
		more = (i01+mblock < i1);
		totread += ierr;
	    }
	    else {
		ibis::util::logMessage
		    ("Warning", "mergeBlock2 failed to read %lu bytes "
		     "at %lu from %s", static_cast<long unsigned>(bszd),
		     static_cast<long unsigned>(szdi2), dsrc);
		ierr = -4;
		more = false;
		break;
	    }

	    // loop over all pages in the two consecutive blocks
	    while (more && i01 < i1 && i12 < i2) {
		buf3.clear();
		for (uint32_t i3 = 0; i3 < mblock; ++ i3) {
		    if (j01 < mblock && j12 < block2) {
			if (cmp(buf2[j12], buf1[j01])) {
			    buf3.push_back(buf2[j12]);
			    ++ j12;
			}
			else {
			    buf3.push_back(buf1[j01]);
			    ++ j01;
			}
		    }
		    else if (j01 < mblock) {
			buf3.push_back(buf1[j01]);
			++ j01;
		    }
		    else if (j12 < block2) {
			buf3.push_back(buf2[j12]);
			++ j12;
		    }
		    else {
			break;
		    }
		    if (j01 >= mblock && i01+mblock < i1) {
			// read next block from segment 0
			i01 += mblock;
			szdi1 += bszd;
			ierr = buf1.read(fdsrc, szdi1, szdi1+bszd);
			if (ierr != static_cast<long>(bszd)) {
			    ibis::util::logMessage
				("Warning", "mergeBlock2 failed to read %lu "
				 "bytes at %lu from %s",
				 static_cast<long unsigned>(bszd),
				 static_cast<long unsigned>(szdi1), dsrc);
			    ierr = -5;
			    more = false;
			    break;
			}
			totread += ierr;
		    }
		    if (block2==mblock && j12 >= mblock) {
			i12 += mblock;
			szdi2 += bszd;
			ierr = buf1.read(fdsrc, szdi2, szdi2+bszd);
			if (ierr >= 0) {
			    block2 = ierr / szd;
			    i2 = i12 + block2;
			    more = (i01+mblock < i1);
			    totread += ierr;
			}
			else {
			    ibis::util::logMessage
				("Warning", "mergeBlock2 failed to read %lu "
				 "bytes at %lu from %s",
				 static_cast<long unsigned>(bszd),
				 static_cast<long unsigned>(szdi2), dsrc);
			    ierr = -6;
			    more = false;
			    break;
			}
		    }
		}
		ierr = UnixWrite(fdout, buf3.begin(), buf3.size()*szd);
	    }
	    buf3.resize(mblock);
	}
	else { // only one segment remain in the input file, copy the segment
	    long nread;
	    while ((nread = UnixRead(fdsrc, buf1.begin(), bszd) > 0)) {
		ierr = UnixWrite(fdout, buf1.begin(), nread);
		totread += nread;
	    }
	    more = false;
	}
    } // i0

#if defined(_WIN32) && defined(_MSC_VER)
    _commit(fdout);
#endif
    UnixClose(fdout);
    UnixClose(fdsrc);

    if (ierr > 0)
	ierr = 0;
    if (ibis::gVerbose > 3) {
	timer.stop();
	double speed = timer.realTime();
	if (speed < 1.0e-6)
	    speed = 1.0e-6;
	speed *= 2e-6 * totread;
	ibis::util::logMessage
	    ("roster::mergeBlock2", "completed merging blocks "
	     "of size %lu, written output to %s, used %g sec "
	     "with %g MB/s", static_cast<long unsigned>(segment),
	     dout, timer.realTime(), speed);
    }
    return ierr;
} //ibis::roster::mergeBlock2

// explicit inistantiations of mergeBlock2
template long ibis::roster::mergeBlock2(const char*, const char*,
					const uint32_t,
					array_t<ibis::rid_t>&,
					array_t<ibis::rid_t>&,
					array_t<ibis::rid_t>&);

// the printing function
void ibis::roster::print(std::ostream& out) const {
    out << "ibis::roster for " << col->partition()->name() << '.'
	<< col->name() << std::endl;
} // ibis::roster::print

uint32_t ibis::roster::size() const {
    return col->partition()->nRows();
} // ibis::roster::size

// return the smallest i such that val >= val[ind[i]]
uint32_t ibis::roster::locate(const double& v) const {
    uint32_t hit = ind.size();
    if (hit == 0) return hit;

    std::string fnm; // name of the data file
    fnm = col->partition()->currentDataDir();
    fnm += col->name();
    int ierr = 0;

    switch (col->type()) {
    case ibis::UINT: { // unsigned int
	array_t<uint32_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr == 0 && val.size() == ind.size()) {
	    unsigned bnd = static_cast<unsigned>(v);
	    if (bnd < v)
		++ bnd;
	    hit = val.find(ind, bnd);
	}
	else {
	    col->logWarning("roster::locate", "roster (%lu) and data array "
			    "(%lu) has different number of elements",
			    static_cast<long unsigned>(ind.size()),
			    static_cast<long unsigned>(val.size()));
	}
	break;
    }
    case ibis::INT: { // signed int
	array_t<int32_t> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr == 0 && val.size() == ind.size()) {
	    int bnd = static_cast<int>(v);
	    if (bnd < v)
		++ bnd;
	    hit = val.find(ind, bnd);
	}
	else {
	    col->logWarning("roster::locate", "index (%lu) and data array "
			    "(%lu) has different number of elements",
			    static_cast<long unsigned>(ind.size()),
			    static_cast<long unsigned>(val.size()));
	}
	break;
    }
    case ibis::FLOAT: { // float
	array_t<float> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr == 0 && val.size() == ind.size()) {
	    float bnd = static_cast<float>(v);
	    hit = val.find(ind, bnd);
	}
	else {
	    col->logWarning("roster::locate", "index (%lu) and data array "
			    "(%lu) has different number of elements",
			    static_cast<long unsigned>(ind.size()),
			    static_cast<long unsigned>(val.size()));
	}
	break;
    }
    case ibis::DOUBLE: { // double
	array_t<double> val;
	ierr = ibis::fileManager::instance().getFile(fnm.c_str(), val);
	if (ierr == 0 && val.size() == ind.size()) {
	    hit = val.find(ind, v);
	}
	else {
	    col->logWarning("roster::locate", "index (%lu) and data array "
			    "(%lu) has different number of elements",
			    static_cast<long unsigned>(ind.size()),
			    static_cast<long unsigned>(val.size()));
	}
	break;
    }
    default: {
	ibis::util::logger lg(4);
	lg.buffer() << "roster -- no ibis::roster index for ";
	col->print(lg.buffer());
	break;}
    }

    return hit;
} // ibis::roster::locate

template <typename T> void
ibis::roster::icSearch(const std::vector<T>& vals,
		       std::vector<uint32_t>& pos) const {
    const uint32_t nrows = col->partition()->nRows();
    if (ind.size() != nrows) { // not a valid index array
	oocSearch(vals, pos);
	return;
    }

    std::string fname = col->partition()->currentDataDir();
    fname += DIRSEP;
    fname += col->name();
    int len = fname.size();
    fname += ".srt";

    pos.clear();
    uint32_t iv = 0;
    uint32_t it = 0;
    const uint32_t nvals = vals.size();
    array_t<T> tmp;
    int ierr = ibis::fileManager::instance().getFile(fname.c_str(), tmp);
    if (ierr == 0) { // got the sorted values
	while (iv < nvals && it < nrows) {
	    // move iv so that vals[iv] is not less than tmp[it]
	    while (iv < nvals && vals[iv] < tmp[it])
		++ iv;
	    if (iv >= nvals)
		return;
	    // move it so that tmp[it] is not less than vals[iv]
	    while (it < nrows && vals[iv] > tmp[it])
		++ it;

	    while (it < nrows && vals[iv] == tmp[it]) { // found a match
		pos.push_back(ind[it]);
		++ it;
	    }
	}
	return;
    }

    // try to read the base data
    fname.erase(len);
    ierr = ibis::fileManager::instance().getFile(fname.c_str(), tmp);
    if (ierr == 0) { // got the base data in memory
	while (iv < nvals && it < nrows) {
	    // move iv so that vals[iv] is not less than tmp[ind[it]]
	    while (iv < nvals && vals[iv] < tmp[ind[it]])
		++ iv;
	    if (iv >= nvals)
		return;
	    // move it so that tmp[ind[it]] is not less than vals[iv]
	    while (it < nrows && vals[iv] > tmp[ind[it]])
		++ it;

	    // found a match
	    while (it < nrows && vals[iv] == tmp[ind[it]]) {
		pos.push_back(ind[it]);
		++ it;
	    }
	}
    }
    else {
	oocSearch(vals, pos);
    }
} // ibis::roster::icSearch

template <typename T> void
ibis::roster::oocSearch(const std::vector<T>& vals,
			std::vector<uint32_t>& pos) const {
    // explicitly generate the sorted values
    writeSorted(static_cast<const char*>(0));

    std::string fname = col->partition()->currentDataDir();
    fname += DIRSEP;
    fname += col->name();
    int len = fname.size();
    fname += ".srt";

    const uint32_t nvals = vals.size();
    const uint32_t nrows = col->partition()->nRows();
    int srtdes = UnixOpen(fname.c_str(), OPEN_READONLY);
    if (srtdes < 0) {
	ibis::util::logMessage("Warning", "failed to open the file "
			       "containing sorted values (%s)", fname.c_str());
	return;
    }

    int ierr;
    uint32_t iv = 0; // index for vals
    uint32_t ir = 0; // index for the rows to be read
    const unsigned int tbytes = sizeof(T);

    ibis::util::buffer<T> mybuf;
    char *cbuf = reinterpret_cast<char*>(mybuf.address());
    const uint32_t ncbuf = tbytes * mybuf.size();
    const uint32_t nbuf = mybuf.size();
    if (nbuf > 0 && ind.size() == nrows) {
	// each read operation fills the buffer, use in-memory ind array
	while (iv < nvals && ir < nrows) {
	    ierr = UnixRead(srtdes, cbuf, ncbuf);
	    if (ierr < static_cast<int>(tbytes)) {
		return;
	    }

	    const T* curr = reinterpret_cast<const T*>(cbuf);
	    const T* end = curr + ierr / tbytes;
	    while (curr < end) {
		while (iv < nvals && vals[iv] < *curr)
		    ++ iv;
		if (iv >= nvals) {
		    return;
		}
		while (curr < end && vals[iv] > *curr) {
		    ++ curr;
		    ++ ir;
		}
		while (curr < end && vals[iv] == *curr) {
		    pos.push_back(ind[ir]);
		    ++ curr;
		    ++ ir;
		}
	    }
	}

	return;
    }

    if (inddes < 0) {
	fname.erase(len);
	fname += ".ind";
	inddes = UnixOpen(fname.c_str(), OPEN_READONLY);
	if (inddes < 0) {
	    ibis::util::logMessage("Warning", "ibis::roster::oocSearch failed "
				   "to open index file %s",
				   fname.c_str());
	    return;
	}
    }
    if (nbuf > 0 && inddes > 0) {
	// bulk read, also need to read ind array
	while (iv < nvals && ir < nrows) {
	    ierr = UnixRead(srtdes, cbuf, ncbuf);
	    if (ierr < static_cast<int>(tbytes)) {
		return;
	    }

	    const T* curr = reinterpret_cast<const T*>(cbuf);
	    const T* end = curr + ierr / tbytes;
	    while (curr < end) {
		while (iv < nvals && vals[iv] < *curr)
		    ++ iv;
		if (iv >= nvals) {
		    return;
		}
		while (curr < end && vals[iv] > *curr) {
		    ++ curr;
		    ++ ir;
		}
		while (curr < end && vals[iv] == *curr) {
		    uint32_t tmp;
		    ierr = UnixSeek(inddes, ir*sizeof(tmp), SEEK_SET);
		    ierr = UnixRead(inddes, &tmp, sizeof(tmp));
		    if (ierr <= 0) {
			ibis::util::logMessage
			    ("Warning", "ibis::roster::oocSearch "
			     "failed to %lu-th index value",
			     static_cast<long unsigned>(ir));
			return;
		    }
		    pos.push_back(tmp);
		    ++ curr;
		    ++ ir;
		}
	    }
	}
    }
    else { // read one value at a time, very slow!
	cbuf = 0;

	T curr;
	ierr = UnixRead(srtdes, &curr, tbytes);
	if (ierr < static_cast<int>(tbytes)) {
	    ibis::util::logMessage
		("Warning", "ibis::roster::oocSearch failed to read "
		 "value %lu from the sorted file",
		 static_cast<long unsigned>(ir));
	    return;
	}

	while (iv < nvals && ir < nrows) {
	    while (iv < nvals && vals[iv] < curr)
		++ iv;
	    if (iv >= nvals)
		return;

	    while (ir < nrows && vals[iv] > curr) {
		ierr = UnixRead(srtdes, &curr, tbytes);
		if (ierr < static_cast<int>(tbytes)) {
		    ibis::util::logMessage
			("Warning", "ibis::roster::oocSearch failed to read "
			 "value %lu from the sorted file",
			 static_cast<long unsigned>(ir));
		    return;
		}
		++ ir;
	    }
	    while (ir < nrows && vals[iv] == curr) {
		if (ind.size() == nrows) {
		    pos.push_back(ind[ir]);
		}
		else {
		    uint32_t tmp;
		    ierr = UnixSeek(inddes, ir*sizeof(tmp), SEEK_SET);
		    ierr = UnixRead(inddes, &tmp, sizeof(tmp));
		    if (ierr <= 0) {
			ibis::util::logMessage
			    ("Warning", "ibis::roster::oocSearch "
			     "failed to %lu-th index value",
			     static_cast<long unsigned>(ir));
			return;
		    }
		    pos.push_back(tmp);
		}
		ierr = UnixRead(srtdes, &curr, tbytes);
		if (ierr < static_cast<int>(tbytes)) {
		    ibis::util::logMessage
			("Warning", "ibis::roster::oocSearch failed to read "
			 "value %lu from the sorted file",
			 static_cast<long unsigned>(ir));
		    return;
		}
		++ ir;
	    }
	}
    }
} // ibis::roster::oocSearch

/// Error code:
/// - -1: incorrect type of @c vals.
/// - -2: internal error, no column associated with the @c roster object.
int ibis::roster::locate(const std::vector<int32_t>& vals,
			 ibis::bitvector& positions) const {
    int ierr = 0;
    if (col == 0) {
	ierr = -2;
	return ierr;
    }
    if (col->type() != ibis::INT &&
	col->type() != ibis::BYTE &&
	col->type() != ibis::SHORT) {
	ierr = -1;
	return ierr;
    }

    std::vector<uint32_t> ipos; // integer positions
    writeSorted(static_cast<const char*>(0));
    if (col->type() == ibis::BYTE) {
	std::vector<signed char> cvals;
	for (std::vector<int32_t>::const_iterator it = vals.begin();
	     it != vals.end(); ++ it)
	    cvals.push_back(static_cast<signed char>(*it));
	icSearch(cvals, ipos);
    }
    else if (col->type() == ibis::SHORT) {
	std::vector<int16_t> svals;
	for (std::vector<int32_t>::const_iterator it = vals.begin();
	     it != vals.end(); ++ it)
	    svals.push_back(static_cast<short int>(*it));
	icSearch(svals, ipos);
    }
    else {
	icSearch(vals, ipos);
    }
    std::sort(ipos.begin(), ipos.end());
    positions.clear();
    for (std::vector<uint32_t>::const_iterator it = ipos.begin();
	 it != ipos.end(); ++ it)
	positions.setBit(*it, 1);
    positions.adjustSize(0, col->partition()->nRows());

    return ierr;
} // ibis::roster::locate

int ibis::roster::locate(const std::vector<uint32_t>& vals,
			 ibis::bitvector& positions) const {
    int ierr = 0;
    if (col == 0) {
	ierr = -2;
	return ierr;
    }
    if (col->type() != ibis::CATEGORY &&
	col->type() != ibis::UINT &&
	col->type() != ibis::UBYTE &&
	col->type() != ibis::USHORT) {
	ierr = -1;
	return ierr;
    }

    std::vector<uint32_t> ipos; // integer positions
    writeSorted(static_cast<const char*>(0));
    if (col->type() == ibis::UBYTE) {
	std::vector<unsigned char> cvals;
	for (std::vector<uint32_t>::const_iterator it = vals.begin();
	     it != vals.end(); ++ it)
	    cvals.push_back(static_cast<unsigned char>(*it));
	icSearch(cvals, ipos);
    }
    else if (col->type() == ibis::USHORT) {
	std::vector<uint16_t> svals;
	for (std::vector<uint32_t>::const_iterator it = vals.begin();
	     it != vals.end(); ++ it)
	    svals.push_back(static_cast<unsigned short int>(*it));
	icSearch(svals, ipos);
    }
    else {
	icSearch(vals, ipos);
    }
    std::sort(ipos.begin(), ipos.end());
    positions.clear();
    for (std::vector<uint32_t>::const_iterator it = ipos.begin();
	 it != ipos.end(); ++ it)
	positions.setBit(*it, 1);
    positions.adjustSize(0, col->partition()->nRows());

    return ierr;
} // ibis::roster::locate

int ibis::roster::locate(const std::vector<float>& vals,
			 ibis::bitvector& positions) const {
    int ierr = 0;
    if (col == 0) {
	ierr = -2;
	return ierr;
    }
    if (col->type() != ibis::FLOAT) {
	ierr = -1;
	return ierr;
    }

    std::vector<uint32_t> ipos; // integer positions
    writeSorted(static_cast<const char*>(0));
    icSearch(vals, ipos);
    std::sort(ipos.begin(), ipos.end());
    positions.clear();
    for (std::vector<uint32_t>::const_iterator it = ipos.begin();
	 it != ipos.end(); ++ it)
	positions.setBit(*it, 1);
    positions.adjustSize(0, col->partition()->nRows());

    return ierr;
} // ibis::roster::locate

int ibis::roster::locate(const std::vector<double>& vals,
			 ibis::bitvector& positions) const {
    int ierr = 0;
    if (col == 0) {
	ierr = -2;
	return ierr;
    }
    if (col->type() != ibis::DOUBLE) {
	ierr = -1;
	return ierr;
    }

    std::vector<uint32_t> ipos; // integer positions
    writeSorted(static_cast<const char*>(0));
    icSearch(vals, ipos);
    std::sort(ipos.begin(), ipos.end());
    positions.clear();
    for (std::vector<uint32_t>::const_iterator it = ipos.begin();
	 it != ipos.end(); ++ it)
	positions.setBit(*it, 1);
    positions.adjustSize(0, col->partition()->nRows());

    return ierr;
} // ibis::roster::locate

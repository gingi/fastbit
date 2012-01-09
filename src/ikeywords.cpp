// $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2006-2012 the Regents of the University of California
//
// This file implements the ibis::keywords class.
#include "ikeywords.h"
#include "iroster.h"	// ibis::iroster
#include "part.h"	// ibis::table
#include <fstream>	// std::ifstream
#include <cctype>	// std::isalnum

#include <algorithm>	// std::sort
#include <string.h>	// strlen, strchr

/// Constructor.  It first tries to read the terms (@c .terms) and the
/// tdmat (@c .idx) files if they both exist.  If that fails, it will
/// attempt to build an index using the externally provided term-document
/// list or parsing the text with a specified list of delimiters.
ibis::keywords::keywords(const ibis::column* c, const char* f)
    : ibis::index(c) {
    if (c == 0) return; // does nothing
    if (c->type() != ibis::CATEGORY &&
	c->type() != ibis::TEXT) {
	LOGGER(ibis::gVerbose >= 0)
	    << "keywords::keywords -- can only index categorical "
	    "values or string values";
	throw ibis::bad_alloc("wrong column type for ibis::keywords");
    }

    // try to read an existing keyword index
    int ierr;
    std::string fdic, fmat;
    dataFileName(f, fdic);
    fmat = fdic;
    fdic += ".terms";
    fmat += ".idx";
    if (ibis::util::getFileSize(fdic.c_str()) > 0 &&
	ibis::util::getFileSize(fmat.c_str()) > 0) {
	ierr = read(f);
	if (ierr >= 0) {
	    if (ibis::gVerbose > 4) {
		ibis::util::logger lg;
		print(lg());
	    }
	    return;
	}
    }

    std::string delim;
    reinterpret_cast<const ibis::text*>(c)->delimitersForKeywordIndex(delim);
    // fmat = name of the default tdlist file
    fmat.erase(fmat.size()-3);
    fmat += "tdlist";
    // fdic = name of the externally specified tdlist file
    reinterpret_cast<const ibis::text*>(c)->TDListForKeywordIndex(fdic);
    if (! fdic.empty()) {
	// read a tdlist file with an externally provided file name, first
	// check that id column is a valid one
	const ibis::column* idcol =
	    reinterpret_cast<const ibis::text*>(c)->IDColumnForKeywordIndex();
	if (idcol->type() != ibis::INT &&
	    idcol->type() != ibis::UINT) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- keywords::keywords -- the id column of "
		"keywords can only be 4-byte integers";
	    throw ibis::bad_alloc("keywords can only use 4-byte "
				  "integers as ids");
	}
	ierr = readTermDocFile(idcol, fdic.c_str());
	if (ierr == -1 && f != 0 && *f != 0)
	    ierr = readTermDocFile(idcol, f);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- keywords::keywords -- readTermDocFile failed "
		"with error code " << ierr;
	    ibis::index::clear();
	}
    }
    else if (delim.empty() && ibis::util::getFileSize(fmat.c_str()) > 0) {
	// read a tdlist file with the default file name, first check that
	// id column is a valid one
	const ibis::column* idcol =
	    reinterpret_cast<const ibis::text*>(c)->
	    IDColumnForKeywordIndex();
	if (idcol->type() == ibis::INT || idcol->type() == ibis::UINT) {
	    ierr = readTermDocFile(idcol, fmat.c_str());
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- keywords::keywords -- readTermDocFile "
		    "failed with error code " << ierr;
		clear();
	    }
	}
	else {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- keywords::keywords -- the id column of "
		"keywords can only be 4-byte integers";
	    ibis::index::clear();
	}
    }
    if (bits.empty()) { // still don't have an index already, try this
	ibis::keywords::tokenizer tkn(delim.c_str());
	ierr = parseTextFile(tkn, f);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- keywords::keywords failed to parse text "
		"file to build a keyword index, parseTextFile returned "
		<< ierr;
	    throw ibis::bad_alloc("keywords::ctr failed to parse text");
	}
    }

    optionalUnpack(bits, col->indexSpec());
    //write(f);
    if (ibis::gVerbose > 6) {
	ibis::util::logger lg;
	lg() << "keywords::ctor completed constructing a new index\n";
	print(lg());
    }
} // ibis::keywords::keywords

/// Constructor.  Construct a new keyword index using the user-provided
/// tokenizer.
ibis::keywords::keywords(const ibis::column *c, ibis::text::tokenizer &tkn,
			 const char *f) : ibis::index(c) {
    if (c == 0) return;

    int ierr = parseTextFile(tkn, f);
    if (ierr >= 0) {
	optionalUnpack(bits, col->indexSpec());
	//write(f);
	if (ibis::gVerbose > 6) {
	    ibis::util::logger lg;
	    lg() << "keywords::ctor completed constructing a new index\n";
	    print(lg());
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- keywords::keywords -- parseTextFile failed with "
	    "error code " << ierr;
	throw ibis::bad_alloc("keywords::ctor failed to parse text file");
    }
} // ibis::keywords::keywords

/// Constructor.  Reconstruct a keyword index from an existing file.
ibis::keywords::keywords(const ibis::column* c, ibis::fileManager::storage* st)
    : ibis::index(c, st) {
    read(st);
} // ibis::keywords::keywords

/// Reads a term-document list from an external file.  Returns the number
/// of terms found if successful, otherwise returns a negative number to
/// indicate error.
int ibis::keywords::readTermDocFile(const ibis::column* idcol, const char* f) {
    int ierr = 0;
    if (col == 0 || col->partition() == 0) return -1;

    std::ifstream tdf(f);
    if ((! tdf) && col->partition()->currentDataDir() != 0) {
	std::string fullname = col->partition()->currentDataDir();
	fullname += FASTBIT_DIRSEP;
	fullname += f;
	tdf.open(fullname.c_str());
    }
    if (! tdf) {
	LOGGER(ibis::gVerbose > 2)
	    << "keywords::readTermDocFile -- failed to open \""
	    << f << "\" for reading";
	return -2;
    }

    ibis::fileManager::buffer<char> mybuf;
    uint32_t nbuf = mybuf.size();
    char* buf = mybuf.address();
    if (nbuf == 0 || buf == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "keywords::readTermDocFile(" << f
	    << ") -- failed to acquire a buffer to reading";
	return -3;
    }
    nrows = col->partition()->nRows();
    size_t jline = 0;
    std::string kw;
    std::vector<uint32_t> idlist;
    typedef std::map<char*, ibis::bitvector*, ibis::lessi> TBMap;
    TBMap tbmap;
    if (idcol != 0) {
	ibis::roster ros(idcol);
	while ((ierr = readTDLine(tdf, kw, idlist, buf, nbuf)) == 0) {
	    ++ jline;
	    ibis::bitvector bvec;
	    ierr = ros.locate(idlist, bvec);
	    if (ierr < 0)
		setBits(idlist, bvec);
	    bvec.adjustSize(0, nrows);
	    if (bvec.cnt() > 0) {
		TBMap::iterator it = tbmap.find(const_cast<char*>(kw.c_str()));
		if (it == tbmap.end()) { // a new entry
		    tbmap[ibis::util::strnewdup(kw.c_str())] =
			new ibis::bitvector(bvec);
		}
		else {
		    *(it->second) |= bvec;
		}
		LOGGER(ibis::gVerbose > 1 && jline % 100000 == 0)
		    << "keywords::readTermDocFile -- reading keywords from "
		    << f << ", got " << tbmap.size();
	    }
	} // reading a line of the term-document list file
    }
    else {
	while ((ierr = readTDLine(tdf, kw, idlist, buf, nbuf)) == 0) {
	    ++ jline;
	    ibis::bitvector bvec;
	    setBits(idlist, bvec);
	    bvec.adjustSize(0, nrows);
	    if (bvec.cnt() > 0) {
		TBMap::iterator it = tbmap.find(const_cast<char*>(kw.c_str()));
		if (it == tbmap.end()) { // a new entry
		    tbmap[ibis::util::strnewdup(kw.c_str())] =
			new ibis::bitvector(bvec);
		}
		else {
		    *(it->second) |= bvec;
		}
		LOGGER(ibis::gVerbose > 1 && jline % 100000 == 0)
		    << "keywords::readTermDocFile -- reading keywords from "
		    << f << ", got " << tbmap.size();
	    }
	} // reading a line of the term-document list file
    }
    tdf.close();
    if (tbmap.empty())
	return ierr;
    if (ierr > 0) // ignore warnings
	ierr = 0;

    if (ibis::gVerbose > 1) {
	col->logMessage("keywords::readTermDocFile", "read %lu keyword%s "
			"from \"%s\" using \"%s\" as the ID column",
			static_cast<long unsigned>(tbmap.size()),
			(tbmap.size()>1U ? "s" : ""), f,
			(idcol ? idcol->name() : "the row number"));
    }
    // translates tbmap into a dictionary and a vector of pointers
    bits.resize(tbmap.size()+1);
    bits[0] = new ibis::bitvector;
    bits[0]->set(0, nrows);
    std::vector<char*> toDelete;
    for (TBMap::const_iterator it = tbmap.begin();
	 it != tbmap.end(); ++ it) {
	uint32_t sz0 = terms.size();
	uint32_t pos = terms.insertRaw((*it).first);
	*(bits[0]) |= *((*it).second);
	if (pos >= sz0) { // a new word in the dictionary
	    bits[pos] = (*it).second;
	}
	else {
	    *(bits[pos]) |= *((*it).second);
	    delete (*it).second;
	    toDelete.push_back((*it).first);
	}
    }
    for (unsigned i = 0; i < toDelete.size(); ++ i)
	delete [] toDelete[i];
    bits[0]->compress();
    bits[0]->flip();
    return ierr;
} // ibis::keywords::readTermDocFile

/// Read one line from the term-docuement file.  The caller has opened the
/// file already, read one line from the input stream.  Extract the keyword
/// and the list of ids.
int ibis::keywords::readTDLine(std::istream &in,
			       std::string &key,
			       std::vector<uint32_t> &idlist,
			       char* linebuf, uint32_t nbuf) const {
    int ierr = 0;
    char *str1, *str2;
    key.erase(); // empty the content of keyword
    idlist.clear(); // empty the content of the id list
    in.get(linebuf, nbuf);
    if (in.eof())
	return 3;
    if (linebuf[0] == 0)
	return 2;

    str1 = linebuf;
    char c = readTerm(const_cast<const char*&>(str1), key);
    if (c != ':') { // failed to find the required delimiter after keyword
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- keywords::readTDLine -- failed to find the "
	    "required delimiter ':' after the keyword \"" << key
	    << "\".  Skip the line";
	ierr = -1;
	return ierr;
    }

    while (*str1) {
	str2 = str1;
	unsigned int id = readUInt(const_cast<const char*&>(str2));
	if (*str2) { // not null, assume to be a correct delimiter
	    idlist.push_back(id);
	    str1 = str2;
	    if (*str2 == '\n') // end of line, done
		return ierr;
	}
	else { // end of current buffer
	    char eol;
	    in.get(eol);
	    if (eol == '\n') {
		idlist.push_back(id);
#if DEBUG+0 > 0 || _DEBUG+0 > 0
		LOGGER(ibis::gVerbose > 5)
		    << "DEBUG -- keywords::readTDLine -- keyword: " << key
		    << ", count: " << idlist.size() << " ("
		    << idlist[0] << (idlist.size()>1 ? ", ...)" : ")");
#endif
		return ierr;
	    }
	    else {
		for (str2=linebuf; *str1 != 0; ++ str1, ++ str2)
		    *str2 = *str1;
		in.unget();
	    }
	    in.get(str2, nbuf-(str2-linebuf));
	    if (in.eof())
		return ierr;
	    if (linebuf[0] == 0)
		return ierr;
	    str1 = linebuf;
	}
    }
    ierr = 1; // terminating without seeing the end of a line
    return ierr;
} // ibis::keywords::readTDLine

/// Turn on the specified positions in a bitvector.
void ibis::keywords::setBits(std::vector<uint32_t>& pos,
			     ibis::bitvector& bvec) const {
    bvec.clear(); // clear the current content
    std::sort(pos.begin(), pos.end());
    for (std::vector<uint32_t>::const_iterator it = pos.begin();
	 it != pos.end(); ++ it) {
	bvec.setBit(*it, 1);
    }
} // ibis::keywords::setBits

/// Parse the text file to build a keyword index.  This function is called
/// by the constructor of the class to build a new keyword index.
int ibis::keywords::parseTextFile(ibis::text::tokenizer &tkn,
				  const char *dir) {
    std::string tfname, spname;
    if (col == 0) return -1;
    if (0 == col->dataFileName(tfname, dir))
	return -2;

    spname = tfname;
    spname += ".sp";
    int tfdesc = UnixOpen(tfname.c_str(), OPEN_READONLY);
    if (tfdesc < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- keywords::parseTextFile failed to open file \""
	    << tfname << "\", the open function returned " << tfdesc;
	return -3;
    }

    IBIS_BLOCK_GUARD(UnixClose, tfdesc);
    int spdesc = UnixOpen(spname.c_str(), OPEN_READONLY);
    if (spdesc < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- keywords::parseTextFile failed to open file \""
	    << spname << "\", the open function returned " << spdesc;
	return -4;
    }

    IBIS_BLOCK_GUARD(UnixClose, spdesc);
    int64_t start, end;
    int64_t ierr = UnixRead(spdesc, &start, sizeof(start));
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 2)
	    << "Warning -- keywords::parseTextFile failed to read the first "
	    "value from " << spname;
	return -5;
    }
    nrows = 0;
    ibis::fileManager::buffer<char> buf(2048);
    // main loop to actually read the strings one row at a time
    while ((ierr = UnixRead(spdesc, &end, sizeof(end))) == 8) {
	if (start+1 >= end) { // null string
	    start = end;
	    ++ nrows;
	    continue;
	}

	const size_t sz = end - start;
	if (buf.size() < sz) { // buffer too small
	    buf.resize((sz+2047)/2048);
	    if (buf.size() < sz) {
		LOGGER(ibis::gVerbose > 2)
		    << "Warning -- keywords::parseTextFile failed to allocate "
		    "enough buffer space to read a " << sz << "-byte string";
		clear();
		return -6;
	    }
	}

	ierr = UnixSeek(tfdesc, start, SEEK_SET);
	if (ierr != start) {
	    LOGGER(ibis::gVerbose > 2)
		<< "Warning -- keywords::parseTextFile failed to seek to "
		<< start << ", function seek returned " << ierr;
	    clear();
	    return -6;
	}

	ierr = UnixRead(tfdesc, buf.address(), sz);
	if (ierr < (int64_t)sz) {
	    LOGGER(ibis::gVerbose > 2)
		<< "Warning -- keywords::parseTextFile expected to read "
		<< sz << " byte" << (sz > 1 ? "s" : "")
		<< ", but the function read returned " << ierr;
	    clear();
	    return -7;
	}

	std::vector<const char*> tokens;
	ierr = tkn(tokens, buf.address());
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- keywords::parseTextFile failed to tokenize "
		"entry # " << nrows << ", tokenizer returned " << ierr
		<< ", skipping the row";
	    tokens.clear();
	}

	for (size_t j = 0; j < tokens.size(); ++ j) {
	    uint32_t ibits = terms.insert(tokens[j]);
	    if (ibits >= bits.size()) {
		size_t jold = bits.size();
		if (bits.capacity() <= ibits+1)
		    bits.reserve(ibits+ibits+2);
		bits.resize(ibits+1);
		while (jold <= ibits) {
		    bits[jold] = 0;
		    ++ jold;
		}
	    }
	    if (bits[ibits] == 0)
		bits[ibits] = new ibis::bitvector;
	    bits[ibits]->setBit(nrows, 1);
	}
	start = end;
	++ nrows;
    } // read spdesc

    for (size_t j = 0; j < bits.size(); ++ j)
	if (bits[j] != 0)
	    bits[j]->adjustSize(0, nrows);

    if (col->partition() != 0 && ibis::gVerbose > 1) {
	ibis::util::logger lg;
	if (col->partition()->nRows() != nrows)
	    lg() << "Warning -- ";
	lg() << "keywords[" << col->partition()->name() << "." << col->name()
	     << "]::parseTextFile read " << nrows << " string value"
	     << (nrows>1?"s":"") << " from " << tfname;
	if (col->partition()->nRows() != nrows)
	    lg() << ", but expected " << col->partition()->nRows();
    }
    return 0;
} // ibis::keywords::parseTextFile

void ibis::keywords::binWeights(std::vector<uint32_t>& bw) const {
    bw.resize(bits.size());
    activate();
    for (uint32_t i = 0; i < bits.size(); ++ i)
	bw[i] = (bits[i] ? bits[i]->cnt() : 0);
} // ibis::keywords::binWeights

void ibis::keywords::print(std::ostream& out) const {
    if (ibis::gVerbose < 0) return;
    const uint32_t nobs = bits.size();
    if (terms.size()+1 == bits.size() && terms.size() > 0) {
	out << "The boolean term-document matrix for column ";
	if (col->partition() != 0)
	    out << col->partition()->name() << '.';
	out << col->name() << " contains the following terms (optionally "
	    "followed by term frequencies)\n";
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
	else {
	    skip = 1;
	}
	if (skip > 1) {
	    out << " (printing 1 out of every " << skip << ")\n";
	}
	for (uint32_t i = 1; i < bits.size(); i += skip) {
	    out << terms[i] << "\t";
	    if (bits[i])
		out << bits[i]->cnt();
	    out << "\n";
	}
    }
    else if (col != 0) {
	out << "The boolean term-document matrix for " << col->name()
	    << " is empty";
    }
    out << std::endl;
} // ibis::keywords::print

/// Write the boolean term-document matrix as two files, xx.terms
/// for the terms and xx.idx for the bitmaps that marks the positions.
int ibis::keywords::write(const char* dt) const {
    std::string fnm;
    dataFileName(dt, fnm);
    fnm += ".terms";
    terms.write(fnm.c_str());

    fnm.erase(fnm.size()-5);
    fnm += "idx";
    if (fname != 0 || str != 0)
	activate();
    int fdes = UnixOpen(fnm.c_str(), OPEN_WRITENEW, OPEN_FILEMODE);
    if (fdes < 0) {
	ibis::fileManager::instance().flushFile(fnm.c_str());
	fdes = UnixOpen(fnm.c_str(), OPEN_WRITENEW, OPEN_FILEMODE);
	if (fdes < 0) {
	    col->logWarning("keywords::write", "unable to open \"%s\"",
			    fnm.c_str());
	    return -1;
	}
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    off_t ierr = 0;
    const uint32_t nobs = bits.size();
#ifdef FASTBIT_USE_LONG_OFFSETS
    const bool useoffset64 = true;
#else
    const bool useoffset64 = (8+getSerialSize() > 0x80000000UL);
#endif
    char header[] = "#IBIS\7\0\0";
    header[5] = (char)ibis::index::KEYWORDS;
    header[6] = (char)(useoffset64 ? 8 : 4);
    ierr = UnixWrite(fdes, header, 8);
    if (ierr < 8) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- keywords[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write the 8-byte header, ierr = " << ierr;
	return -3;
    }
    ierr  = UnixWrite(fdes, &nrows, sizeof(uint32_t));
    ierr += UnixWrite(fdes, &nobs,  sizeof(uint32_t));
    if (ierr < (off_t)(sizeof(uint32_t)*2)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- keywords[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write nrows and nobs, ierr = " << ierr;
	return -4;
    }
    offset64.resize(nobs+1);
    offset64[0] = 16 + header[6]*(nobs+1);
    ierr = UnixSeek(fdes, header[6]*(nobs+1), SEEK_CUR);
    if (ierr != offset64[0]) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- keywords[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to seek to " << offset64[0] << ", ierr = " << ierr;
	return -5;
    }
    for (uint32_t i = 0; i < nobs; ++ i) {
	if (bits[i]) {
	    bits[i]->write(fdes);
	}
	offset64[i+1] = UnixSeek(fdes, 0, SEEK_CUR);
    }
    ierr = UnixSeek(fdes, 16, SEEK_SET);
    if (ierr != 16) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- keywords[" << col->partition()->name() << "."
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
	    << "Warning -- keywords[" << col->partition()->name() << "."
	    << col->name() << "]::write(" << fnm
	    << ") failed to write bitmap offsets, ierr = " << ierr;
	return -7;
    }
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    (void) UnixFlush(fdes); // write to disk
#endif

    LOGGER(ibis::gVerbose > 5)
	<< "keywords[" << col->partition()->name() << "."
	<< col->name() << "]::write -- wrote " << nobs << " bitmap"
	<< (nobs>1?"s":"") << " to " << fnm;
    return 0;
} // ibis::keywords::write

int ibis::keywords::read(const char* f) {
    std::string fnm;
    dataFileName(f, fnm);
    fnm += ".terms";
    terms.read(fnm.c_str());

    fnm.erase(fnm.size()-5);
    fnm += "idx";
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
		  header[5] == static_cast<char>(ibis::index::KEYWORDS) &&
		  (header[6] == 8 || header[6] == 4) &&
		  header[7] == static_cast<char>(0))) {
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg()
		<< "Warning -- keywords[" << col->partition()->name() << '.'
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
#if DEBUG+0 > 0 || _DEBUG+0 > 0
    if (ibis::gVerbose > 5) {
	unsigned nprt = (ibis::gVerbose < 30 ? (1 << ibis::gVerbose) : dim[1]);
	if (nprt > dim[1])
	    nprt = dim[1];
	ibis::util::logger lg;
	lg() << "DEBUG -- keywords[" << col->partition()->name() << '.'
		    << col->name() << "]::read(" << fnm
		    << ") got nobs = " << dim[1]
		    << ", the offsets of the bit vectors are\n";
	if (offset64.size() > dim[1]) {
	    for (unsigned i = 0; i < nprt; ++ i)
		lg() << offset64[i] << " ";
	}
	else {
	    for (unsigned i = 0; i < nprt; ++ i)
		lg() << offset32[i] << " ";
	}
	if (nprt < dim[1])
	    lg() << "... (skipping " << dim[1]-nprt << ") ... ";
	if (offset64.size() > dim[1])
	    lg() << offset64[dim[1]];
	else
	    lg() << offset32[dim[1]];
    }
#endif

    initBitmaps(fdes);
    str = 0;
    LOGGER(ibis::gVerbose > 7)
	<< "keywords[" << col->partition()->name() << '.' << col->name()
	<< "]::read(" << fnm << ") finished reading index header with nrows="
	<< nrows << " and bits.size()="
	<< bits.size();
    return 0;
} // ibis::keywords::read

// attempt to reconstruct an index from a piece of consecutive memory
int ibis::keywords::read(ibis::fileManager::storage* st) {
    if (st == 0) return -1;
    clear();

    const char offsetsize = st->begin()[6];
    nrows = *(reinterpret_cast<uint32_t*>(st->begin()+8));
    size_t pos = 8 + sizeof(uint32_t);
    size_t end;
    const uint32_t nobs = *(reinterpret_cast<uint32_t*>(st->begin()+pos));
    pos += sizeof(uint32_t);
    end = pos + offsetsize * (nobs + 1);
    if (offsetsize == 8) {
	array_t<int64_t> offs(st, pos, end);
	offset64.copy(offs);
    }
    else if (offsetsize == 4) {
	array_t<int32_t> offs(st, pos, end);
	offset32.copy(offs);
    }
    else {
	clear();
	return -2;
    }

    initBitmaps(st);
    if (terms.size() < bits.size()) { // need to read the dictionary
	std::string fnm;
	dataFileName(0, fnm);
	fnm += ".terms";
	terms.read(fnm.c_str());
    }
    return 0;
} // ibis::keywords::read

void ibis::keywords::clear() {
    terms.clear();
    ibis::index::clear();
} // ibis::keywords::clear

long ibis::keywords::append(const char* dt, const char* df, uint32_t nnew) {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- keywords::append not implemented yet";
    return -1L;
} // ibis::keywords::append

long ibis::keywords::evaluate(const ibis::qContinuousRange& expr,
			      ibis::bitvector& lower) const {
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- keywords::evaluate for qContinuousRange has "
	"not been implemented";
    return -1L;
} // ibis::keywords::evaluate

void ibis::keywords::estimate(const ibis::qContinuousRange& expr,
			      ibis::bitvector& lower,
			      ibis::bitvector& upper) const {
    throw "keywords::estimate not implemented yet";
} // ibis::keywords::estimate

uint32_t ibis::keywords::estimate(const ibis::qContinuousRange& expr) const {
    throw "keywords::estimate not implemented yet";
    return 0;
} // ibis::keywords::estimate

long ibis::keywords::search(const char* kw,
			    ibis::bitvector& hits) const {
    hits.clear();
    uint32_t pos = terms[kw];
    if (pos < bits.size()) {
	if (bits[pos] == 0)
	    activate(pos);
	if (bits[pos] != 0)
	    hits.copy(*bits[pos]);
	else
	    hits.set(0, nrows);
    }
    else {
	hits.set(0, nrows);
    }
    return hits.cnt();
} // ibis::keywords::search

long ibis::keywords::search(const char* kw) const {
    long cnt = 0;
    uint32_t pos = terms[kw];
    if (pos < bits.size()) {
	if (bits[pos] == 0)
	    activate(pos);
	if (bits[pos] != 0)
	    cnt = bits[pos]->cnt();
    }
    return cnt;
} // ibis::keywords::search

double ibis::keywords::estimateCost(const ibis::qContinuousRange& expr) const {
    double ret = 0.0;
    if (offset64.size() > bits.size()) {
	if (expr.leftOperator() == ibis::qExpr::OP_EQ) {
	    uint32_t h0 = static_cast<uint32_t>(expr.leftBound());
	    if (h0 < bits.size())
		ret = offset64[h0+1] - offset64[h0];
	}
	else if (expr.leftOperator() == ibis::qExpr::OP_EQ) {
	    uint32_t h1 = static_cast<uint32_t>(expr.rightBound());
	    if (h1 < bits.size())
		ret = offset64[h1+1] - offset64[h1];
	}
    }
    else if (offset32.size() > bits.size()) {
	if (expr.leftOperator() == ibis::qExpr::OP_EQ) {
	    uint32_t h0 = static_cast<uint32_t>(expr.leftBound());
	    if (h0 < bits.size())
		ret = offset32[h0+1] - offset32[h0];
	}
	else if (expr.leftOperator() == ibis::qExpr::OP_EQ) {
	    uint32_t h1 = static_cast<uint32_t>(expr.rightBound());
	    if (h1 < bits.size())
		ret = offset32[h1+1] - offset32[h1];
	}
    }
    return ret;
} // ibis::keywords::estimateCost

double ibis::keywords::estimateCost(const ibis::qDiscreteRange& expr) const {
    double ret = 0.0;
    if (offset64.size() > bits.size()) {
	const ibis::array_t<double>& vals = expr.getValues();
	for (unsigned j = 0; j < vals.size(); ++ j) {
	    uint32_t itmp = static_cast<uint32_t>(vals[j]);
	    if (itmp < bits.size())
		ret += offset64[itmp+1] - offset64[itmp];
	}
    }
    else if (offset32.size() > bits.size()) {
	const ibis::array_t<double>& vals = expr.getValues();
	for (unsigned j = 0; j < vals.size(); ++ j) {
	    uint32_t itmp = static_cast<uint32_t>(vals[j]);
	    if (itmp < bits.size())
		ret += offset32[itmp+1] - offset32[itmp];
	}
    }
    return ret;
} // ibis::keywords::estimateCost

/// Estimate the size of the .idx file.  The .idx file contains only the
/// bitmaps without the actual terms.  The bitmap offsets are assumed to be
/// 8-byte long.
size_t ibis::keywords::getSerialSize() const throw () {
    size_t res = 24 + (bits.size() << 3);
    for (unsigned j = 0; j < bits.size(); ++ j)
	if (bits[j] != 0)
	    res += bits[j]->getSerialSize();
    return res;
} // ibis::keywords::getSerialSize

/// Tokenizer.  Turn the buffer buf into a list of tokens based on the
/// following rules.
///
/// - If no delimiter is specified, it turns all non-alphanumeric
/// characters into the null character and returns the starting positions
/// of groups of alphanumeric characters as tokens.
///
/// - If a list of delimiters are provided, any of the delimiters will
/// terminate a token.  Blank spaces surrounding the delimiters will be
/// turned into null characters along with the delimiters.
///
/// This function returns 0 in normal cases.
int ibis::keywords::tokenizer::operator()
    (std::vector<const char*>& tkns, char *buf) {
    tkns.clear();
    if (buf == 0 || *buf == 0)
	return 0;

    if (delim_.empty()) {
	while (*buf != 0) {
	    while (*buf != 0 && std::isalnum(*buf) == 0) {
		*buf = 0;
		++ buf;
	    }
	    if (*buf != 0) {
		tkns.push_back(buf);
		for (++ buf; std::isalnum(*buf) != 0; ++ buf);
	    }
	}
    }
    else if (delim_.size() == 1) {
	while (*buf != 0) {
	    while (*buf != 0 && (*buf == delim_[0] ||
				 std::isspace(*buf) != 0)) { // leading spaces
		*buf = 0;
		++ buf;
	    }
	    if (*buf != 0) {
		tkns.push_back(buf);
		for (++ buf; *buf != 0 && *buf != delim_[0]; ++ buf);
		if (buf > tkns.back()) { // trailing spaces
		    for (char *t=buf-1; std::isspace(*t) != 0; -- t)
			*t = 0;
		}
	    }
	}
    }
    else {
	while (*buf != 0) {
	    while (*buf != 0 && (strchr(delim_.c_str(), *buf) != 0 ||
				 std::isspace(*buf) != 0)) { // leading spaces
		*buf = 0;
		++ buf;
	    }
	    if (*buf != 0) {
		tkns.push_back(buf);
		for (++ buf;
		     *buf != 0 && strchr(delim_.c_str(), *buf) == 0;
		     ++ buf);
		if (buf > tkns.back()) { // tailing spaces
		    for (char *t=buf-1; std::isspace(*t) != 0; -- t)
			*t = 0;
		}
	    }
	}
    }
    return 0;
} // ibis::keywords::tokenizer::operator()

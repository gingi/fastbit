//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2012 the Regents of the University of California
//
// Purpose: implementation of the two version of text fields ibis::category
// and ibis::text
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif

#include "part.h"
#include "category.h"
#include "ikeywords.h"

#include <algorithm>	// std::copy
#include <memory>	// std::auto_ptr

////////////////////////////////////////////////////////////////////////
// functions for ibis::category
ibis::category::category(const part* tbl, FILE* file)
    : text(tbl, file), dic() {
#ifdef FASTBIT_EAGER_INIT_TEXT
    prepareMembers();
    lower = 1;
    upper = dic.size();
#endif
} // ibis::category::category

/// Construct a category object from a name.
ibis::category::category(const part* tbl, const char* name)
    : text(tbl, name, ibis::CATEGORY), dic() {
#ifdef FASTBIT_EAGER_INIT_TEXT
    prepareMembers();
    lower = 1;
    upper = dic.size();
#endif
} // ibis::category::category

/// Copy constructor.  Copy from a collumn object with CATEGORY type.
ibis::category::category(const ibis::column& col) : ibis::text(col), dic() {
    if (m_type != ibis::CATEGORY) {
	throw ibis::bad_alloc("Must be type CATEGORY");
    }
#ifdef FASTBIT_EAGER_INIT_TEXT
    prepareMembers();
    lower = 1;
    upper = dic.size();
#endif
} // ibis::category::category

/// Construct a column that has only one possible value.  Also build the
/// corresponding index.
ibis::category::category(const part* tbl, const char* name,
			 const char* value, const char* dir,
			 uint32_t nevt)
    : text(tbl, name, ibis::CATEGORY), dic() {
    dic.insert(value);
    lower = 1;
    upper = 1;
    std::string df = (dir ? dir : tbl->currentDataDir());
    df += FASTBIT_DIRSEP;
    df += name;
    df += ".dic";
    dic.write(df.c_str());
    if (nevt == 0) nevt = tbl->nRows();
    if (dir == 0)  dir  = tbl->currentDataDir();
    if (nevt > 0 && dir != 0) { // generate a trivial index
	ibis::relic rlc(this, 1, nevt);
	rlc.write(dir);
    }
} // ibis::category::category

/// Destructor.  It also writes the dictionary to a file if the dictionary
/// is not empty.
ibis::category::~category() {
    unloadIndex();
    if (dic.size() > 0) {
	std::string dname;
	dataFileName(dname);
	if (! dname.empty()) {
	    dname += ".dic";
	    if (ibis::util::getFileSize(dname.c_str()) <= 0)
		dic.write(dname.c_str());
	}
    }
} // ibis::category::~category

ibis::array_t<uint32_t>*
ibis::category::selectUInts(const ibis::bitvector& mask) const {
    prepareMembers();
    std::string fname;
    bool tryintfile = (0 != dataFileName(fname));
    if (tryintfile) {
	fname += ".int";
	tryintfile = (thePart->nRows() ==
		      (ibis::util::getFileSize(fname.c_str()) >> 2));
    }
    if (tryintfile) {
	std::auto_ptr< ibis::array_t<uint32_t> >
	    tmp(new ibis::array_t<uint32_t>);
	if (selectValuesT(fname.c_str(), mask, *tmp) >= 0)
	    return tmp.release();
    }

    indexLock lock(this, "category::selectUInts");
    if (idx != 0)
	return static_cast<ibis::relic*>(idx)->keys(mask);
    else
	return 0;
} // ibis::category::selectUInts

/// Retrieve the string values from the rows marked 1 in mask.
///
/// @note FastBit does not track the memory usage of neither std::vector
/// nor std::string.
std::vector<std::string>*
ibis::category::selectStrings(const ibis::bitvector& mask) const {
    if (mask.cnt() == 0)
	return new std::vector<std::string>();
    if (dic.size() == 0)
	prepareMembers();
    if (dic.size() == 0) // return empty strings
	return new std::vector<std::string>(mask.cnt(), "");

    // The dictionary size may be 1 because some values are empty string
    // which will be mapped to integer id 0.  In this case, the dictionary
    // size is 1, but there are actually two different values.
    // if (dic.size() == 1) {
    // 	return new std::vector<std::string>(mask.cnt(), dic[1]);
    // }

    std::string fname;
    bool useintfile = (0 != dataFileName(fname));
    if (useintfile) { // chech the size of the .int file
	fname += ".int";
	useintfile = (thePart->nRows() ==
		      (ibis::util::getFileSize(fname.c_str()) >> 2));
    }
    if (useintfile) {
	std::auto_ptr< ibis::array_t<uint32_t> >
	    keys(new ibis::array_t<uint32_t>);
	if (selectValuesT(fname.c_str(), mask, *keys) >= 0) {
	    std::vector<std::string>* strings = new std::vector<std::string>();
	    strings->reserve(keys->size());
	    for (unsigned i = 0; i < keys->size(); ++i) {
		const char *ptr = dic[(*keys)[i]];
		strings->push_back(ptr!=0 ? ptr : "");
	    }
	    return strings;
	}
    }

    // the fallback option - read the strings from the raw data file
    return ibis::text::selectStrings(mask);
} // ibis::category::selectStrings

/// A function to read the dictionary and load the index.  This is a const
/// function because it only manipulates mutable data members.  This is
/// necessary to make it callable from const member function of this class.
void ibis::category::prepareMembers() const {
    if (thePart == 0 || thePart->currentDataDir() == 0) return;
    mutexLock lock(this, "category::prepareMembers");
    if (dic.size() == 0)
	readDictionary();

    if (idx == 0) {
	std::string idxf = thePart->currentDataDir();
	idxf += FASTBIT_DIRSEP;
	idxf += m_name;
	idxf += ".idx";
	idx = new ibis::relic(this, idxf.c_str());
    }
    if (idx == 0 || idx->getNRows() != thePart->nRows()) {
	delete idx;
	idx = 0;
	(void) fillIndex();
    }
} // ibis::category::prepareMembers

/// Read the dictionary from the specified directory.  If the incoming
/// argument is nil, the current directory of the data partition is used.
void ibis::category::readDictionary(const char *dir) const {
    std::string fnm;
    if (dir != 0 && *dir != 0) {
	fnm = dir;
    }
    else if (thePart != 0) { // default to the current dictionary
	if (thePart->currentDataDir() != 0)
	    fnm = thePart->currentDataDir();
	else
	    return;
    }
    else {
	return;
    }
    fnm += FASTBIT_DIRSEP;
    fnm += m_name;
    fnm += ".dic"; // suffix of the dictionary
    int ierr = dic.read(fnm.c_str());
    LOGGER(ierr < 0 && ibis::gVerbose > 2)
	<< "Warning -- category[" << (thePart ? thePart->name() : "?") << '.'
	<< m_name << "] failed to read dictionary file " << fnm << ", ierr = "
	<< ierr;
} // ibis::category::readDictionary

/// Build an ibis::relic index using the existing primary data.
/// If the dictionary exists and the size is one, it builds a dummy index.
/// Otherwise, it reads the primary data file to update the dictionary and
/// complete a new ibis::relic index.
ibis::relic* ibis::category::fillIndex(const char *dir) const {
    std::string dirstr;
    if (dir != 0 && *dir != 0) { // the name may be a filename
	unsigned ldir = strlen(dir);
	std::string idx = m_name;
	idx += ".idx";
	if (ldir > idx.size()) {
	    unsigned dlen = ldir - idx.size();
	    if (0 == strcmp(dir+dlen, idx.c_str())) {
		if (dir[dlen-1] == '/'
#if defined(_WIN32) && defined(_MSC_VER)
		    || dir[dlen-1] == '\\'
#endif
		    ) {
		    -- dlen;
		    for (unsigned i = 0; i < dlen; ++ i)
			dirstr += dir[i];
		    dir = dirstr.c_str();
		}
	    }
	}
	else if (ldir > m_name.size()) {
	    unsigned dlen = ldir - m_name.size();
	    if (0 == strcmp(dir+dlen, m_name.c_str()) &&
		dir[dlen-1] == '/'
#if defined(_WIN32) && defined(_MSC_VER)
		|| dir[dlen-1] == '\\'
#endif
		) {
		-- dlen;
		for (unsigned i = 0; i < dlen; ++ i)
		    dirstr += dir[i];
		dir = dirstr.c_str();
	    }
	}
    }
    else if (thePart != 0) {
	dir = thePart->currentDataDir();
    }
    if (dir == 0) return 0;
    if (dic.size() == 0)
	readDictionary(dir);

    ibis::relic *rlc = 0;
    if (dic.size() == 1) { // assume every entry has the given value
	rlc = new ibis::relic(this, 1);
    }
    else { // actually read the raw data to build an index
	std::string data = (dir ? dir : thePart->currentDataDir());
	data += FASTBIT_DIRSEP;
	data += m_name; // primary data file name
	int fdata = UnixOpen(data.c_str(), OPEN_READONLY);
	if (fdata < 0) {
	    if (ibis::gVerbose > 1)
		logMessage("fillIndex", "unable to open data file %s",
			   data.c_str());
	    return 0;
	}
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdata, _O_BINARY);
#endif

	int ret;
	ibis::fileManager::buffer<char> mybuf;
	char *buf = mybuf.address();
	uint32_t nbuf = mybuf.size();
	const bool iscurrent =
	    (strcmp(dir, thePart->currentDataDir()) == 0 &&
	     thePart->getStateNoLocking() != ibis::part::PRETRANSITION_STATE);
	array_t<uint32_t> ints;
	do {
	    array_t<uint32_t> tmp;
	    ret = string2int(fdata, dic, nbuf, buf, tmp);
	    if (ret > 0) {
		if (! ints.empty())
		    ints.insert(ints.end(), tmp.begin(), tmp.end());
		else
		    ints.swap(tmp);
	    }
	} while (ret > 0 && (! iscurrent || ints.size() < thePart->nRows()));
	(void)UnixClose(fdata);

	if (iscurrent) {
	    if (ints.size() > thePart->nRows()) {
		unsigned cnt = 0;
		const unsigned nints = ints.size();
		for (unsigned i = 0; i < nints; ++ i)
		    cnt += (ints[i] == 0);
		if (cnt + thePart->nRows() == nints) {
		    LOGGER(ibis::gVerbose > 1)
			<< "category["
			<< (thePart != 0 ? thePart->name() : "")
			<< "." << name() << "]::fillIndex -- found "
			<< nints << " strings while expecting "
			<< thePart->nRows() << "; remove "
			<< cnt << " null strings";

		    cnt = 0;
		    for (unsigned i = 0; i < nints; ++ i) {
			if (ints[i] != 0) {
			    ints[cnt] = ints[i];
			    ++ cnt;
			}
		    }
		}
		else if (thePart->getStateNoLocking()
			 != ibis::part::PRETRANSITION_STATE
			 && ibis::gVerbose > 1) {
		    logMessage("category::fillIndex", "found %u strings while "
			       "expecting %lu, truncating the list of values",
			       nints,
			       static_cast<long unsigned>(thePart->nRows()));
		}
	    }
	    else if (ints.size() < thePart->nRows()) {
		ints.insert(ints.end(), thePart->nRows() - ints.size(), 0);
	    }
	    if (thePart->getStateNoLocking() != ibis::part::PRETRANSITION_STATE)
		ints.resize(thePart->nRows());
	    //#if defined(WRITE_INT_VALUES_FOR_KEYS)
	    data += ".int";
	    ints.write(data.c_str());
	    //#endif
	}
	if (rlc != 0) {
	    ret = rlc->append(ints);
	}
	else {
	    rlc = new ibis::relic(this, 1+dic.size(), ints);
	    ret = ints.size();
	}
    }

    if (rlc) {
	rlc->write(dir);
	if (dir == thePart->currentDataDir() ||
	    strcmp(dir, thePart->currentDataDir()) == 0) {
	    idx = rlc;
	}
	else {
	    delete rlc;
	    rlc = 0;
	}
    }

    std::string dicfile = (dir ? dir : thePart->currentDataDir());
    dicfile += FASTBIT_DIRSEP;
    dicfile += m_name;
    dicfile += ".dic";
    dic.write(dicfile.c_str());
    return rlc;
} // ibis::category::fillIndex

long ibis::category::stringSearch(const char* str,
				  ibis::bitvector& hits) const {
    prepareMembers();
    uint32_t ind = dic[str];
    if (ind == 0) { // null value
	getNullMask(hits); // mask = 0 if null
	hits.flip();
    }
    else if (ind == 1 && dic.size() == 1) { // special case with one value
	hits.set(1, thePart->nRows()); // every record has this value
    }
    else if (ind <= dic.size()) { // found it in the dictionary
	indexLock lock(this, "category::stringSearch");
	if (idx != 0) {
	    ibis::qContinuousRange expr(m_name.c_str(),
					ibis::qExpr::OP_EQ, ind);
	    long ierr = idx->evaluate(expr, hits);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- category::stringSearch(" << str
		    << ") failed because idx->evaluate(" << expr
		    << ") returned " << ierr;
		return ierr;
	    }
	}
	else {
	    hits.set(0, thePart->nRows());
	}
    }
    else { // not in the dictionary
	hits.set(0, thePart->nRows());
    }
    return hits.cnt();
} // ibis::category::stringSearch

long ibis::category::stringSearch(const char* str) const {
    long ret;
    prepareMembers();
    uint32_t ind = dic[str];
    if (ind == 0) { // null value
	ret = 0;
    }
    else if (ind == 1 && dic.size() == 1) {
	ret = thePart->nRows();
    }
    else if (ind <= dic.size()) { // found it
	indexLock lock(this, "category::stringSearch");
	if (idx != 0) {
	    ibis::qContinuousRange expr(m_name.c_str(),
					ibis::qExpr::OP_EQ, ind);
	    ret = idx->estimate(expr);
	}
	else { // no index, use the number of rows
	    ret = thePart->nRows();
	}
    }
    else { // not in the dictionary
	ret = 0;
    }
    return ret;
} // ibis::category::stringSearch

double ibis::category::estimateCost(const ibis::qString& qstr) const {
    double ret;
    prepareMembers();
    const char* str = (stricmp(qstr.leftString(), m_name.c_str()) == 0 ?
		       qstr.rightString() : qstr.leftString());
    uint32_t ind = dic[str];
    if (ind <= dic.size()) {
	indexLock lock(this, "category::estimateCost");
	if (idx != 0) {
	    ibis::qContinuousRange expr(m_name.c_str(),
					ibis::qExpr::OP_EQ, ind);
	    ret = idx->estimateCost(expr);
	}
	else { // no index, use the number of rows
	    ret = static_cast<double>(thePart->nRows()) * sizeof(uint32_t);
	}
    }
    else {
	ret = 0;
    }
    return ret;
} // ibis::category::estimateCost

double ibis::category::estimateCost(const ibis::qMultiString& qstr) const {
    double ret = 0;
    prepareMembers();
    indexLock lock(this, "category::estimateCost");
    if (idx != 0) {
	const std::vector<std::string>& strs = qstr.valueList();
	std::vector<uint32_t> inds;
	inds.reserve(strs.size());
	for (unsigned j = 0; j < strs.size(); ++ j) {
	    uint32_t jnd = dic[strs[j].c_str()];
	    if (jnd < dic.size())
		inds.push_back(jnd);
	}
	ibis::qDiscreteRange expr(m_name.c_str(), inds);
	ret = idx->estimateCost(expr);
    }
    else { // no index, use the number of rows
	ret = static_cast<double>(thePart->nRows()) * sizeof(uint32_t);
    }
    return ret;
} // ibis::category::estimateCost

/// Estimate the cost of evaluating a Like expression.
double ibis::category::estimateCost(const ibis::qLike &cmp) const {
    return patternSearch(cmp.pattern());
} // ibis::category::estimateCost

long ibis::category::stringSearch(const std::vector<std::string>& strs,
				  ibis::bitvector& hits) const {
    if (strs.empty()) {
	hits.clear();
	return 0;
    }

    if (strs.size() == 1) // the list contains only one value
	return stringSearch(strs.back().c_str(), hits);

    prepareMembers();
    // there are more than one value in the list
    std::vector<uint32_t> inds;
    inds.reserve(strs.size());
    for (std::vector<std::string>::const_iterator it = strs.begin();
	 it != strs.end(); ++ it) {
	uint32_t ind = dic[(*it).c_str()];
	if (ind > 0 && ind < dic.size())
	    inds.push_back(ind);
    }

    if (inds.empty()) { // null value
	getNullMask(hits); // mask = 0 if null
	hits.flip();
    }
    else { // found some values in the dictionary
	indexLock lock(this, "category::stringSearch");
	if (idx != 0) {
	    ibis::qDiscreteRange expr(m_name.c_str(), inds);
	    long ierr = idx->evaluate(expr, hits);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- category::stringSearch on " << strs.size()
		    << " strings failed because idx->evaluate(" << expr
		    << ") failed with error code " << ierr;
		return ierr;
	    }
	}
	else { // index must exist! can not proceed
	    hits.set(0, thePart->nRows());
	    if (ibis::gVerbose >= 0)
		logWarning("category::stringSearch", "can not obtain a lock "
			   "on the index or there is no index");
	}
    }
    return hits.cnt();
} // ibis::category::stringSearch

long ibis::category::stringSearch(const std::vector<std::string>& strs) const {
    long ret = thePart->nRows();
    if (strs.empty()) {
	ret = 0;
    }
    else if (strs.size() == 1) {// the list contains only one value
	ret = stringSearch(strs.back().c_str());
    }
    else {
	// there are more than one value in the list
	prepareMembers();
	std::vector<uint32_t> inds;
	inds.reserve(strs.size());
	for (std::vector<std::string>::const_iterator it = strs.begin();
	     it != strs.end(); ++ it) {
	    uint32_t ind = dic[(*it).c_str()];
	    if (ind > 0 && ind < dic.size())
		inds.push_back(ind);
	}

	if (inds.empty()) { // null value
	    ibis::bitvector hits;
	    getNullMask(hits); // mask = 0 if null
	    ret = hits.size() - hits.cnt();
	}
	else { // found some values in the dictionary
	    indexLock lock(this, "category::stringSearch");
	    if (idx != 0) {
		ibis::qDiscreteRange expr(m_name.c_str(), inds);
		ret = idx->estimate(expr);
	    }
	    else { // index must exist
		ret = 0;
		if (ibis::gVerbose >= 0)
		    logWarning("category::stringSearch", "can not obtain a "
			       "lock on the index or there is no index");
	    }
	}
    }
    return ret;
} // ibis::category::stringSearch

/// Estimate the number of hits for a string pattern.
long ibis::category::patternSearch(const char *pat) const {
    if (pat == 0 || *pat == 0) return -1;
    prepareMembers();

    if (idx == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- category[" << (thePart != 0 ? thePart->name() : "??")
	    << '.' << m_name << "]::patternSearch can not proceed without "
	    "an index ";
	return -2;
    }

    const ibis::relic *rlc = dynamic_cast<const ibis::relic*>(idx);
    if (rlc == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- category[" << (thePart != 0 ? thePart->name() : "??")
	    << '.' << m_name << "]::patternSearch can not proceed without "
	    "an index ";
	return -3;
    }

    LOGGER(ibis::gVerbose > 5)
	<< "category[" << (thePart != 0 ? thePart->name() : "??")
	<< '.' << m_name << "]::patternSearch starting to match pattern "
	<< pat;
    long est = 0;
    const uint32_t nd = dic.size();
    for (uint32_t j = 1; j <= nd; ++ j) {
	if (ibis::util::strMatch(dic[j], pat)) {
	    const ibis::bitvector *bv = rlc->getBitvector(j);
	    if (bv != 0)
		est += bv->cnt();
	}
    }
    return est;
} // ibis::category::patternSearch

/// Find the records with string values that match the given pattern.
long ibis::category::patternSearch(const char *pat,
				   ibis::bitvector &hits) const {
    hits.clear();
    if (pat == 0 || *pat == 0) return -1;
    prepareMembers();

    if (idx == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- category[" << (thePart != 0 ? thePart->name() : "??")
	    << '.' << m_name << "]::patternSearch can not proceed without "
	    "an index ";
	return -2;
    }

    const ibis::relic *rlc = dynamic_cast<const ibis::relic*>(idx);
    if (rlc == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- category[" << (thePart != 0 ? thePart->name() : "??")
	    << '.' << m_name << "]::patternSearch can not proceed without "
	    "an index ";
	return -3;
    }

    LOGGER(ibis::gVerbose > 5)
	<< "category[" << (thePart != 0 ? thePart->name() : "??")
	<< '.' << m_name << "]::patternSearch starting to match pattern "
	<< pat;
    long est = 0;
    uint32_t cnt = 0;
    const uint32_t nd = dic.size();
    for (uint32_t j = 1; j <= nd; ++ j) {
	if (ibis::util::strMatch(dic[j], pat)) {
	    const ibis::bitvector *bv = rlc->getBitvector(j);
	    if (bv != 0) {
		++ cnt;
		est += bv->cnt();
		if (hits.empty()) {
		    hits.copy(*bv);
		}
		else {
		    if (cnt > 32 || (j > 3 && cnt*16 > j))
			hits.decompress();
		    hits |= *bv;
		}
	    }
	}
    }
    if (est > static_cast<long>(hits.size() >> 7))
	hits.compress();
    return est;
} // ibis::category::patternSearch

/// Retrieve the string value represented by the integer i.
void ibis::category::getString(uint32_t i, std::string &str) const {
    if (i > dic.size())
	prepareMembers();
    if (i > 0 && i <= dic.size())
	str = dic[i];
    else
	str.clear();
} // ibis::category::getString

/// This function checks to make sure the index is ready.
void ibis::category::loadIndex(const char*, int) const throw () {
    prepareMembers();
} // ibis::category::loadIndex

// read the string values (terminated with NULL) from the directory "dt" and
// extend the set of bitvectors representing the strings
long ibis::category::append(const char* dt, const char* df,
			    const uint32_t nold, const uint32_t nnew,
			    uint32_t nbuf, char* buf) {
    long ret = 0; // the return value
    long ierr = 0;
    uint32_t cnt = 0;
    if (nnew == 0)
	return ret;
    if (dt == 0 || df == 0)
	return ret;
    if (*dt == 0 || *df == 0)
	return ret;
    if (strcmp(dt, df) == 0)
	return ret;
    prepareMembers();
    // STEP 1: convert the strings to ibis::relic
    std::string dest = dt;
    std::string src = df;
    src += FASTBIT_DIRSEP;
    src += name();
    src += ".idx";
    dest += FASTBIT_DIRSEP;
    dest += name();
    //dest += ".idx";
    ibis::relic *binp = 0;
    ibis::fileManager::storage *st = 0;
    ierr = ibis::fileManager::instance().getFile(src.c_str(), &st);
    readDictionary(df);	// read the dictionary in df
    src.erase(src.size()-4); // remove .idx extension
    //dest.erase(dest.size()-4); // remove .idx
    if (ierr == 0 && st != 0 && st->size() > 0) {
	// read the previously built index
	binp = new ibis::relic(this, st);
	cnt = nnew;

	// copy the raw bytes to dt
	int fptr = UnixOpen(src.c_str(), OPEN_READONLY);
	if (fptr >= 0) {
	    IBIS_BLOCK_GUARD(UnixClose, fptr);
#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fptr, _O_BINARY);
#endif
	    int fdest = UnixOpen(dest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
	    if (fdest >= 0) { // copy raw bytes without any sanity check
		IBIS_BLOCK_GUARD(UnixClose, fdest);
#if defined(_WIN32) && defined(_MSC_VER)
		(void)_setmode(fdest, _O_BINARY);
#endif
		while ((ierr = UnixRead(fptr, buf, nbuf))) {
		    ret = UnixWrite(fdest, buf, ierr);
		    if (ret != ierr && ibis::gVerbose > 2)
			logMessage("append", "expected to write %ld bytes "
				   "to \"%s\" by only wrote %ld", ierr,
				   dest.c_str(), ret);
		}
#if defined(FASTBIT_SYNC_WRITE)
#if _POSIX_FSYNC+0 > 0
		    (void) UnixFlush(fdest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
		    (void) _commit(fdest);
#endif
#endif
	    }
	    else {
		logMessage("append", "unable to open \"%s\" to appending",
			   dest.c_str());
	    }
	}
	else if (ibis::gVerbose > 5) {
	    logMessage("append", "unable to open file \"%s\" "
		       "for reading ... %s, assume the attribute to "
		       "have only one value", src.c_str(),
		       (errno ? strerror(errno) : "no free stdio stream"));
	}
    }
    else {
	// first time accessing these strings, need to parse them
	int fptr = UnixOpen(src.c_str(), OPEN_READONLY);
	if (fptr >= 0) {
	    IBIS_BLOCK_GUARD(UnixClose, fptr);
#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fptr, _O_BINARY);
#endif
	    ret = 0;
	    array_t<uint32_t> ints;
	    do { // loop through the content of the file
		array_t<uint32_t> tmp;
		ret = string2int(fptr, dic, nbuf, buf, tmp);
		if (ret < 0) {
		    if (ibis::gVerbose >= 0)
			logWarning("append", "string2int returned %ld "
				   "after processed %lu strings from \"%s\"",
				   ret, static_cast<long unsigned>(cnt),
				   src.c_str());
		    return ret;
		}
		if (ret > 0) {
		    if (! ints.empty()) {
			ints.insert(ints.end(), tmp.begin(), tmp.end());
		    }
		    else {
			ints.swap(tmp);
		    }
		}
	    } while (ret > 0);
	    if (ints.size() > nnew) {
		// step 1: look through the values to find how many nil
		// strings
		cnt = 0;
		const unsigned long nints = ints.size();
		for (unsigned i = 0; i < nints; ++ i)
		    cnt += (ints[i] == 0);
		if (ints.size() == cnt + nnew) {
		    if (ibis::gVerbose > 1)
			logMessage("append", "found %lu element(s), but "
				   "expecting only %ld, extra ones are likely "
				   "nill strings, removing nill strings",
				   nints, ret);
		    cnt = 0;
		    for (unsigned i = 0; i < nints; ++ i) {
			if (ints[i] != 0) {
			    ints[cnt] = ints[i];
			    ++ cnt;
			}
		    }
		}
		else if (ibis::gVerbose > 1)
		    logMessage("append", "found %lu element(s), but expecting "
			       "only %ld, truncate the extra elements",
			       nints, ret);
		ints.resize(nnew);
	    }
	    else if (ints.size() < nnew) {
		if (ibis::gVerbose > 1)
		    logMessage("append", "found %lu element(s), but "
			       "expecting only %ld, adding nill strings to "
			       "make up the difference",
			       static_cast<long unsigned>(ints.size()), ret);

		ints.insert(ints.end(), nnew-ints.size(), (uint32_t)0);
	    }
	    cnt = ints.size();

	    if (binp != 0) {
		ierr = binp->append(ints);
	    }
	    else {
		binp = new ibis::relic(this, 1+dic.size(), ints);
		ierr = ints.size() * (binp != 0);
	    }
	    if (static_cast<uint32_t>(ierr) != ints.size() &&
		ibis::gVerbose >= 0) {
		logWarning("append", "string2int processed %lu "
			   "strings from \"%s\" but was only able "
			   "append %ld to the index",
			   static_cast<long unsigned>(ints.size()),
			   src.c_str(), ierr);
	    }

	    int fdest = UnixOpen(dest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
	    if (fdest >= 0) { // copy raw bytes without any sanity check
		IBIS_BLOCK_GUARD(UnixClose, fdest);
#if defined(_WIN32) && defined(_MSC_VER)
		(void)_setmode(fdest, _O_BINARY);
#endif
		ierr = UnixSeek(fptr, 0, SEEK_SET);
		if (ierr < 0) return -2;
		while ((ierr = UnixRead(fptr, buf, nbuf)) > 0) {
		    ret = UnixWrite(fdest, buf, ierr);
		    if (ret != ierr && ibis::gVerbose > 2)
			logMessage("append", "expected to write %ld bytes "
				   "to \"%s\" by only wrote %ld", ierr,
				   dest.c_str(), ret);
		}
#if defined(FASTBIT_SYNC_WRITE)
#if  _POSIX_FSYNC+0 > 0
		(void) UnixFlush(fdest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
		(void) _commit(fdest);
#endif
#endif
	    }
	    else {
		logMessage("append", "unable to open \"%s\" to appending",
			   dest.c_str());
	    }
	    if (ierr < 0) return -3;
	}
	else {
	    if (ibis::gVerbose > 5)
		logMessage("append", "unable to open file \"%s\" "
			   "for reading ... %s, assume the attribute to "
			   "have only one value", src.c_str(),
			   (errno ? strerror(errno) : "no free stdio stream"));
	    binp = new ibis::relic(this, 1, nnew);
	    cnt = nnew;
	}
	if (binp != 0)
	    binp->write(df); // record the bitmap
	src += ".dic";
	dic.write(src.c_str()); // write the dictionary to source directory
	src.erase(src.size()-4);
    }

    // write dictionary to the destination directory
    lower = 1;
    upper = dic.size();
    dest += ".dic";
    dic.write(dest.c_str());
    if (ibis::gVerbose > 4)
	logMessage("append", "appended %lu rows, new dictionary size is %lu",
		   static_cast<long unsigned>(cnt),
		   static_cast<long unsigned>(dic.size()));

    ////////////////////////////////////////
    // STEP 2: extend the null mask
    src += ".msk";
    ibis::bitvector mapp(src.c_str());
    if (mapp.size() != nnew)
	mapp.adjustSize(cnt, nnew);
    if (ibis::gVerbose > 7)
	logMessage("append", "mask file \"%s\" contains %lu set "
		   "bits out of %lu total bits", src.c_str(),
		   static_cast<long unsigned>(mapp.cnt()),
		   static_cast<long unsigned>(mapp.size()));

    dest.erase(dest.size()-3);
    dest += "msk";
    ibis::bitvector mtot(dest.c_str());
    if (mtot.size() == 0)
	mtot.set(1, nold);
    else if (mtot.size() != nold)
	mtot.adjustSize(0, nold);
    if (ibis::gVerbose > 7)
	logMessage("append", "mask file \"%s\" contains %lu set "
		   "bits out of %lu total bits", dest.c_str(),
		   static_cast<long unsigned>(mtot.cnt()),
		   static_cast<long unsigned>(mtot.size()));

    mtot += mapp; // append the new ones to the end of the old ones
    if (mtot.size() != nold+nnew) {
	if (ibis::gVerbose > 0)
	    logWarning("append", "combined mask (%lu-bits) is expected to "
		       "have %lu bits, but it is not.  Will force it to "
		       "the expected size",
		       static_cast<long unsigned>(mtot.size()),
		       static_cast<long unsigned>(nold+nnew));
	mtot.adjustSize(nold+nnew, nold+nnew);
    }
    if (mtot.cnt() != mtot.size()) {
	mtot.write(dest.c_str());
	if (ibis::gVerbose > 6) {
	    logMessage("append", "mask file \"%s\" indicates %lu "
		       "valid records out of %lu", dest.c_str(),
		       static_cast<long unsigned>(mtot.cnt()),
		       static_cast<long unsigned>(mtot.size()));
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    LOGGER(ibis::gVerbose >= 0) << mtot;
#endif
	}
    }
    else {
	remove(dest.c_str()); // no need to have the file
	ibis::fileManager::instance().flushFile(dest.c_str());
	if (ibis::gVerbose > 6)
	    logMessage("append", "mask file \"%s\" removed, all "
		       "%lu records valid", dest.c_str(),
		       static_cast<long unsigned>(mtot.size()));
    }

    ////////////////////////////////////////
    // extend the index
    try { // attempt to load the index from directory dt
	if (binp) {
	    ibis::relic ind(this, dt);

	    if (ind.getNRows() == nold && nold > 0) { // append the index
		ierr = ind.append(*binp);
		if (ierr == 0) {
		    ind.write(dt);
		    if (ibis::gVerbose > 6)
			logMessage("append", "successfully extended the index "
				   "in %s", dt);
		    if (ibis::gVerbose > 8) {
			ibis::util::logger lg;
			ind.print(lg());
		    }
		}
		else if (ibis::gVerbose > 0) {
		    logWarning("append", "failed to extend the index (%ld)",
			       ierr);
		    if (ind.getNRows() > 0)
			purgeIndexFile(dt);
		    (void) fillIndex(dt);
		}
	    }
	    else if (nold == 0) { // only need to copy the pointer
		binp->write(dt);
		ierr = 0;
	    }
	    else {
		if (ibis::gVerbose > 2)
		    logMessage("append", "unexpected index for existing "
			       "values in %s (nold=%lu, ind.nrows=%lu)",
			       dt, static_cast<long unsigned>(nold),
			       static_cast<long unsigned>(ind.getNRows()));
		if (ind.getNRows() > 0)
		    purgeIndexFile(dt);
		(void) fillIndex(dt);
	    }
	    delete binp;
	}
	else {
	    if (ibis::gVerbose > 2)
		logMessage("append", "failed to generate the index for "
			   "data in %s, start scanning all records in %s",
			   df, dt);
	    (void) fillIndex(dt);
	}
    }
    catch (...) {
	if (ibis::gVerbose > 2)
	    logMessage("append", "absorbed an exception while extending "
		       "the index, start scanning all records in %s", dt);
	(void) fillIndex(dt);
    }
    ret = cnt;

    return ret;
} // ibis::category::append

/// Write the current content to the metadata file for the data partition.
void ibis::category::write(FILE* file) const {
    fputs("\nBegin Column\n", file);
    fprintf(file, "name = \"%s\"\n", (const char*)m_name.c_str());
    if (m_desc.empty() || m_desc == m_name) {
	fprintf(file, "description = %s ", m_name.c_str());
	if (dic.size() > 10) {
	    fprintf(file, "= ");
	    for (int i = 1; i < 10; ++i)
		fprintf(file, "%s, ", dic[i]);
	    fprintf(file, "..., %s", dic[dic.size()]);
	}
	else if (dic.size() > 1) {
	    fprintf(file, "= %s", dic[1]);
	    for (unsigned int i = 2; i < dic.size(); ++ i)
		fprintf(file, ", %s", dic[i]);
	}
	fprintf(file, "\n");
    }
    else {
	fprintf(file, "description =\"%s\"\n",
	    (const char*)m_desc.c_str());
    }
    fprintf(file, "data_type = \"%s\"\n", TYPESTRING[m_type]);
    fprintf(file, "minimum = 1\nmaximum = %lu\n",
	    static_cast<long unsigned>(dic.size()));
    if (! m_bins.empty())
	fprintf(file, "index=%s\n", m_bins.c_str());
    fputs("End Column\n", file);
} // ibis::category::write

/// Print header info.
void ibis::category::print(std::ostream& out) const {
    out << m_name << ": " << m_desc << " (KEY) [";
    if (dic.size() > 21) {
	for (int i = 1; i < 20; ++ i)
	    out << dic[i] << ", ";
	out << "...(" << dic.size()-20 << " skipped), " << dic[dic.size()];
    }
    else if (dic.size() > 1) {
	out << dic[1];
	for (unsigned int i = 2; i < dic.size(); ++ i)
	    out << ", " << dic[i];
    }
    out << "]";
} // ibis::category::print

/// Return the number of key values.
uint32_t ibis::category::getNumKeys() const {
    if (dic.size() == 0)
	prepareMembers();
    return dic.size();
}

/// Return the ith value in the dictionary.
const char* ibis::category::getKey(uint32_t i) const {
    if (dic.size() == 0)
	prepareMembers();
    return dic[i];
}

/// Is the given string one of the keys in the dictionary?  Return a
/// null pointer if not.
const char* ibis::category::isKey(const char* str) const {
    if (dic.size() == 0)
	prepareMembers();
    return dic.find(str);
}

////////////////////////////////////////////////////////////////////////
// functions for ibis::text
ibis::text::text(const part* tbl, FILE* file) : ibis::column(tbl, file) {
#ifdef FASTBIT_EAGER_INIT_TEXT
    if (thePart != 0)
	startPositions(thePart->currentDataDir(), 0, 0);
#endif
}

/// Construct a text object for a data partition with the given name.
ibis::text::text(const part* tbl, const char* name, ibis::TYPE_T t)
    : ibis::column(tbl, t, name) {
#ifdef FASTBIT_EAGER_INIT_TEXT
    if (thePart != 0 && thePart->currentDataDir() != 0)
	startPositions(thePart->currentDataDir(), 0, 0);
#endif
}

/// Copy constructor.  Copy from a column with TEXT type.
ibis::text::text(const ibis::column& col) : ibis::column(col) {
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	throw "Must be either TEXT or CATEGORY";
    }
#ifdef FASTBIT_EAGER_INIT_TEXT
    if (thePart != 0 && thePart->urrentDataDir() != 0)
	startPositions(thePart->currentDataDir(), 0, 0);
#endif
} // copy constructor

/// Using the data file located in the named directory @c dir.  If @c dir
/// is a nil pointer, the directory defaults to the current working
/// directory of the data partition.
///
/// It writes the starting positions as int64_t integers to a file with .sp
/// as extension.
///
/// Argument @c buf (with @c nbuf bytes) is used as temporary work space.
/// If @c nbuf = 0, this function allocates its own working space.
void ibis::text::startPositions(const char *dir, char *buf,
				uint32_t nbuf) const {
    if (thePart == 0) return;
    if (dir == 0) // default to the current data directory
	dir = thePart->currentDataDir();
    if (dir == 0 || *dir == 0) return;

    std::string dfile = dir;
    dfile += FASTBIT_DIRSEP;
    dfile += m_name;
    std::string spfile = dfile;
    spfile += ".sp";
    mutexLock lock(this, "text::startPositions");
    FILE *fdata = fopen(dfile.c_str(), "r+b"); // mostly for reading
    FILE *fsp = fopen(spfile.c_str(), "r+b"); // mostly for writing
    if (fsp == 0) // probably because the file does not exist, try again
	fsp = fopen(spfile.c_str(), "wb");

    if (fdata == 0 || fsp == 0) { // at least one of the files did not open
	if (fdata == 0) {
	    if (ibis::gVerbose > 0)
		logWarning("startPositions", "failed to open file \"%s\"",
			   dfile.c_str());
	}
	else {
	    fclose(fdata);
	}
	if (fsp == 0) {
	    if (ibis::gVerbose > 0)
		logWarning("startPositions", "failed to open file \"%s\"",
			   spfile.c_str());
	}
	else {
	    fclose(fsp);
	    remove(spfile.c_str());
	}

	return;
    }

    std::string evt = "text[";
    evt += thePart->name();
    evt += '.';
    evt += m_name;
    evt += "]::startPositions";
    long ierr = fseek(fdata, 0, SEEK_END);
    int64_t dfbytes = ftell(fdata);
    if (dfbytes <= 0) { // empty data file
	fclose(fsp);
	fclose(fdata);
	remove(spfile.c_str());
	return;
    }
    const bool isActiveData =
	(thePart->getStateNoLocking() == ibis::part::STABLE_STATE &&
	 (dir == thePart->currentDataDir() ||
	  strcmp(dir, thePart->currentDataDir()) == 0));
    ierr = fseek(fsp, 0, SEEK_END);
    ierr = ftell(fsp);
    if (isActiveData && ierr > (long)(8 * thePart->nRows())) {
	fclose(fdata);
	fclose(fsp);
	return;
    }

    ibis::util::timer mytimer(evt.c_str(), 3);
    ibis::fileManager::buffer<char> mybuf(nbuf != 0);
    if (nbuf == 0) {
	nbuf = mybuf.size();
	buf = mybuf.address();
    }

    int64_t pos = 0;
    uint32_t nold = 0;
    if (ierr > (long)sizeof(uint64_t)) // .sp contains at least two integers
	ierr = fseek(fsp, -static_cast<long>(sizeof(int64_t)), SEEK_END);
    else
	ierr = -1;
    if (ierr == 0) { // try to read the last word in .sp file
	if (fread(&pos, sizeof(int64_t), 1, fsp) != 1) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " unable to read the last "
		"integer in file \"" << spfile << "\"";
	    fclose(fsp);
	    fclose(fdata);
	    return;
	}
	if (pos > 0 && pos <= dfbytes) {// within the valid range
	    nold = ftell(fsp) / sizeof(int64_t) - 1;
	    if (nold > thePart->nRows()) { // start from beginning
		pos = 0;
		nold = 0;
		fclose(fsp);
		/*remove(spfile.c_str());*/
		fsp = fopen(spfile.c_str(), "wb");
	    }
	}
	else { // start from scratch
	    pos = 0;
	}
    }

    if (nold > 0) { // ready to overwrite the last integer
	ierr = fseek(fsp, nold*sizeof(int64_t), SEEK_SET);
    }
    else {
	rewind(fsp);
	pos = 0;
    }

    ibis::fileManager::buffer<int64_t> sps;
    int64_t last = pos;
    int64_t offset = 0;
    uint32_t nnew = 0;
    ierr = fflush(fsp); // get ready for writing
    ierr = fseek(fdata, pos, SEEK_SET);
    if (sps.size() <= 1) {
	while (0 < (ierr = fread(buf+offset, 1, nbuf-offset, fdata))) {
	    const char* const end = buf + offset + ierr;
	    for (const char *s = buf+offset; s < end; ++ s, ++ pos) {
		if (*s == 0) { // find a terminator
		    if (1 > fwrite(&last, sizeof(last), 1, fsp)) {
			LOGGER(ibis::gVerbose >= 0)
			    << evt << " -- unable to write integer value "
			    << last << " to file \"" << spfile << "\"";
		    }
		    last = pos+1;
		    ++ nnew;
		    LOGGER(ibis::gVerbose > 4 && nnew % 1000000 == 0)
			<< evt << " -- processed "
			<< nnew << " strings from " << dfile;
		}
	    }
	    offset = pos - last;
	    if (static_cast<uint64_t>(offset) < nbuf) {
		// copy the string without a terminator
		const int tmp = ierr - offset;
		for (int i = 0; i < offset; ++ i)
		    buf[i] = buf[i+tmp];
	    }
	    else {
		offset = 0;
	    }
	}
    }
    else {
	const uint32_t nsps = sps.size();
	uint32_t jsps = 0;
	while (0 < (ierr = fread(buf+offset, 1, nbuf-offset, fdata))) {
	    const char* const end = buf + offset + ierr;
	    for (const char *s = buf+offset; s < end; ++ s, ++ pos) {
		if (*s == 0) { // find a terminator
		    sps[jsps] = last;
		    ++ jsps;
		    if (jsps >= nsps) {
			if (jsps >
			    fwrite(sps.address(), sizeof(last), jsps, fsp)) {
			    LOGGER(ibis::gVerbose >= 0)
				<< evt << " -- failed to write " << jsps
				<< " integers to file \"" << spfile << "\"";
			}
			jsps = 0;
		    }
		    last = pos+1;
		    ++ nnew;
		    LOGGER(ibis::gVerbose > 4 && nnew % 1000000 == 0)
			<< evt << " -- processed "
			<< nnew << " strings from " << dfile;
		}
	    }
	    offset = pos - last;
	    if (static_cast<uint64_t>(offset) < nbuf) {
		// copy the string without a terminator
		const int tmp = ierr - offset;
		for (int i = 0; i < offset; ++ i)
		    buf[i] = buf[i+tmp];
	    }
	    else {
		offset = 0;
	    }
	}
	if (jsps > 0) {
	    if (jsps >
		fwrite(sps.address(), sizeof(last), jsps, fsp)) {
		LOGGER(ibis::gVerbose >= 0)
		    << evt << " -- failed to write " << jsps
		    << " integers to file \"" << spfile << "\"";
	    }
	}
    }

    if (nold + nnew < thePart->nRows() && thePart->currentDataDir() != 0 &&
	strcmp(dir, thePart->currentDataDir()) == 0) {
	// make up missing values with a null string
	char zero = 0;
	// commit all read operations in preparation for write
	pos = ftell(fdata);
	ierr = fflush(fdata);
	ierr = fwrite(&zero, 1, 1, fdata);
	int64_t *tmp = (int64_t*) buf;
	uint32_t ntmp = nbuf / sizeof(int64_t);
	for (uint32_t i = 0; i < ntmp; ++ i)
	    tmp[i] = pos;
	const long missed = thePart->nRows() - nold - nnew + pos;
	for (long i = 0; i < missed; i += ntmp) {
	    ierr = fwrite(tmp, sizeof(int64_t),
			  (i+(long)ntmp<=missed?(long)ntmp:missed-i), fsp);
	}
    }
    if (nnew > 0) {
	pos = ftell(fdata);// current size of the data file
	(void) fwrite(&pos, sizeof(pos), 1, fsp);
    }
    (void) fclose(fdata);
    (void) fclose(fsp);

    LOGGER(ibis::gVerbose > 3)
	<< evt << " located the starting positions of " << nnew
	<< " new string" << (nnew > 1 ? "s" : "") << ", file " << spfile
	<< " now has " << (nnew+nold+1) << " 64-bit integers (total "
	<< sizeof(int64_t)*(nnew+nold+1) << " bytes)";

    if (isActiveData && nold + nnew > thePart->nRows()) {
	fsp = fopen(spfile.c_str(), "rb");
	ierr = fseek(fsp, thePart->nRows()*sizeof(int64_t), SEEK_SET);
	ierr = fread(&pos, sizeof(int64_t), 1, fsp);
	ierr = fclose(fsp);
	ierr = truncate(spfile.c_str(), (1+thePart->nRows())*sizeof(int64_t));
	ierr = truncate(dfile.c_str(), pos);
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " truncated files " << dfile << " and "
	    << spfile << " to contain only " << thePart->nRows() << " record"
	    << (thePart->nRows() > 1 ? "s" : "");
    }
} // ibis::text::startPositions

void ibis::text::loadIndex(const char* iopt, int ropt) const throw () {
    if (thePart != 0 && thePart->currentDataDir() != 0) {
	startPositions(thePart->currentDataDir(), 0, 0);
	ibis::column::loadIndex(iopt, ropt);
    }
} // ibis::text::loadIndex

/// Append the data file stored in directory @c df to the corresponding
/// data file in directory @c dt.  Use the buffer @c buf to copy data in
/// large chuncks.
///@note  No error checking is performed.
///@note  Does not check for missing entries.  May cause records to be
/// misaligned.
long ibis::text::append(const char* dt, const char* df,
			const uint32_t nold, const uint32_t nnew,
			uint32_t nbuf, char* buf) {
    long ret = 0; // the return value
    long ierr = 0;

    if (nnew == 0)
	return ret;
    if (dt == 0 || df == 0)
	return ret;
    if (*dt == 0 || *df == 0)
	return ret;
    if (strcmp(dt, df) == 0)
	return ret;

    // step 1: make sure the starting positions are updated
    if (nold > 0)
	startPositions(dt, buf, nbuf);

    // step 2: append the content of file in df to that in dt.
    std::string dest = dt;
    std::string src = df;
    src += FASTBIT_DIRSEP;
    src += name();
    dest += FASTBIT_DIRSEP;
    dest += name();

    int fsrc = UnixOpen(src.c_str(), OPEN_READONLY);
    if (fsrc < 0) {
	if (ibis::gVerbose >= 0)
	    logWarning("append", "unableto open file \"%s\" for reading",
		       src.c_str());
	return -1;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fsrc, _O_BINARY);
#endif
    int fdest = UnixOpen(dest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
    if (fdest < 0) {
	UnixClose(fsrc);
	if (ibis::gVerbose >= 0) {
	    logWarning("append", "unableto open file \"%s\" for appending",
		       dest.c_str());
	}
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdest, _O_BINARY);
#endif

    while (0 < (ierr = UnixRead(fsrc, buf, nbuf))) {
	ret = UnixWrite(fdest, buf, ierr);
	if (ret < ierr) {
	    if (ibis::gVerbose >= 0)
		logWarning("append", "failed to write %ld bytes to file "
			   "\"%s\", only wrote %ld", ierr, dest.c_str(), ret);
	    ret = -3;
	    break;
	}
    }
#if defined(FASTBIT_SYNC_WRITE)
#if  _POSIX_FSYNC+0 > 0
    (void) UnixFlush(fdest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
    (void) _commit(fdest);
#endif
#endif
    UnixClose(fdest);
    UnixClose(fsrc);
    if (ret < 0) return ret;
    if (! (lower < upper)) {
	lower = 0;
	upper = nnew + nold - 1;
    }
    else if (upper < nnew+nold-1) {
	upper = nnew + nold-1;
    }

    // step 3: update the starting positions after copying the values
    startPositions(dt, buf, nbuf);
    ret = nnew;
    return ret;
} // ibis::text::append

long ibis::text::patternSearch(const char*) const {
    return (thePart ? (long)thePart->nRows() : -1);
} // ibis::text::patternSearch

long ibis::text::stringSearch(const char*) const {
    return (thePart ? (long)thePart->nRows() : -1);
} // ibis::text::stringSearch

long ibis::text::stringSearch(const std::vector<std::string>&) const {
    return (thePart ? (long)thePart->nRows() : -1);
} // ibis::text::stringSearch

/// Given a string literal, return a bitvector that marks the strings that
/// matche it.
long ibis::text::stringSearch(const char* str, ibis::bitvector& hits) const {
    hits.clear(); // clear the existing content of hits
    if (thePart == 0) return -1L;

    std::string evt = "text[";
    if (thePart != 0 && thePart->name() != 0) {
	evt += thePart->name();
	evt += '.';
    }
    evt += m_name;
    evt += "]::stringSearch";
    std::string data = thePart->currentDataDir();
    data += FASTBIT_DIRSEP;
    data += m_name;
    FILE *fdata = fopen(data.c_str(), "rb");
    if (fdata == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " can not open data file \"" << data
	    << "\" for reading";
	return -2L;
    }

#if defined(DEBUG) || defined(_DEBUG) // DEBUG+0 > 0 || _DEBUG+0 > 0
    ibis::fileManager::buffer<char> mybuf(5000);
#else
    ibis::fileManager::buffer<char> mybuf;
#endif
    char *buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) return -3L;

    std::string sp = data;
    sp += ".sp";
    FILE *fsp = fopen(sp.c_str(), "rb");
    if (fsp == 0) { // try again
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " can not create or open file \""
		<< sp << "\"";
	    fclose(fdata);
	    return -4L;
	}
    }

#if defined(DEBUG) || defined(_DEBUG) // DEBUG+0 > 0 || _DEBUG+0 > 0
    ibis::fileManager::buffer<int64_t> spbuf(1000);
#else
    ibis::fileManager::buffer<int64_t> spbuf;
#endif
    uint32_t irow = 0; // row index
    long jbuf = 0; // number of bytes in buffer
    int64_t begin = 0; // beginning position (file offset) of the bytes in buf
    int64_t next = 0;
    int64_t curr, ierr;
    if (1 > fread(&curr, sizeof(curr), 1, fsp)) {
	// odd to be sure, but try again anyway
	fclose(fsp);
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt <<  " can not open or read file \""
		<< sp << "\"";
	    fclose(fdata);
	    return -5L;
	}
    }
    if (spbuf.size() > 1 && (str == 0 || *str == 0)) {
	// match empty strings, with a buffer for starting positions
	uint32_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " failed to read file " << sp;
	    fclose(fsp);
	    fclose(fdata);
	    return -6L;
	}
	next = spbuf[0];
	nsp = ierr;
	jsp = 1;
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " expects string # " << irow << " in file \""
		    << data << "\" to be " << (next-begin) << "-byte long, but "
		    << (jbuf<(long)nbuf ? "can only read " :
			"the internal buffer is only ")
		    << jbuf << ", skipping " << jbuf
		    << (jbuf > 1 ? " bytes" : " byte");
		curr += jbuf;
	    }
	    while (begin + jbuf >= next) {
		if (buf[curr-begin] == 0)
		    hits.setBit(irow, 1);
		++ irow;
		curr = next;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << evt << " -- processed " << irow
		    << " strings from file " << data;

		if (moresp) {
		    if (jsp >= nsp) {
			ierr = fread(spbuf.address(), sizeof(int64_t),
				     spbuf.size(), fsp);
			if (ierr <= 0) {
			    LOGGER(ibis::gVerbose >= 0)
				<< "Warning -- " << evt << " failed to read "
				<< sp;
			    moresp = false;
			    nsp = 0;
			    break;
			}
			else {
			    nsp = ierr;
			}
			jsp = 0;
		    }
		    moresp = (jsp < nsp);
		    next = spbuf[jsp];
		    ++ jsp;
		}
	    }
	    if (moresp) {// move back file pointer for fdata
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break;
	}
    }
    else if (spbuf.size() > 1)  { // normal strings, use the second buffer
	std::string pat = str; // convert to lower case
	for (uint32_t i = 0; i < pat.length(); ++ i)
	    pat[i] = tolower(pat[i]);
	const uint32_t slen = pat.length() + 1;
	uint32_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " failed to read file " << sp;
	    fclose(fsp);
	    fclose(fdata);
	    return -7L;
	}
	jsp = 1;
	nsp = ierr;
	next = spbuf[0];
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    for (long j = 0; j < jbuf; ++ j) // convert to lower case
		buf[j] = tolower(buf[j]);

	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " expects string # " << irow << " in file \""
		    << data << "\" to be " << (next-begin) << "-byte long, but "
		    << (jbuf<(long)nbuf ? "can only read " :
			"the internal buffer is only ")
		    << jbuf << ", skipping " << jbuf
		    << (jbuf > 1 ? " bytes" : " byte");
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) {
		bool match = (curr+(int64_t)slen == next); // same length?
		long j = curr;
		while (j+4 < next && match) {
		    match = (buf[j-begin] == pat[j-curr]) &&
			(buf[j-begin+1] == pat[j-curr+1]) &&
			(buf[j-begin+2] == pat[j-curr+2]) &&
			(buf[j-begin+3] == pat[j-curr+3]);
		    j += 4;
		}
		if (match) {
		    if (j+4 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]) &&
			    (buf[j-begin+2] == pat[j-curr+2]);
		    }
		    else if (j+3 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]);
		    }
		    else if (j+2 == next) {
			match = (buf[j-begin] == pat[j-curr]);
		    }
		}
		if (match)
		    hits.setBit(irow, 1);
#if _DEBUG+0 > 1 || DEBUG+0 > 1
		if (ibis::gVerbose > 5) {
		    ibis::util::logger lg(4);
		    lg()
			<< "DEBUG -- " << evt << " processing string "
			<< irow << " \'";
		    for (long i = curr; i < next-1; ++ i)
			lg() << buf[i-begin];
		    lg() << "\'";
		    if (match)
			lg() << " == ";
		    else
			lg() << " != ";
		    lg() << pat;
		}
#endif
		++ irow;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << evt << " -- processed " << irow
		    << " strings from file " << data;

		curr = next;
		if (moresp) {
		    if (jsp >= nsp) {
			if (feof(fsp) == 0) {
			    ierr = fread(spbuf.address(), sizeof(int64_t),
					 spbuf.size(), fsp);
			    if (ierr <= 0) {
				LOGGER(ibis::gVerbose >= 0)
				    << "Warning -- " << evt
				    << " -- failed to read file " << sp;
				moresp = false;
				break;
			    }
			    else {
				nsp = ierr;
			    }
			}
			else { // end of sp file
			    moresp = false;
			    break;
			}
			jsp = 0;
		    }
		    moresp = (jsp < nsp);
		    next = spbuf[jsp];
		    ++ jsp;
		}
	    }
	    if (moresp) {// move back file pointer in fdata
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break; // avoid reading the data file
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }
    else if (str == 0 || *str == 0) { // only match empty strings
	ierr = fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " expects string # " << irow << " in file \""
		    << data << "\" to be " << (next-begin) << "-byte long, but "
		    << (jbuf<(long)nbuf ? "can only read " :
			"the internal buffer is only ")
		    << jbuf << ", skipping " << jbuf
		    << (jbuf > 1 ? " bytes" : " byte");
		curr += jbuf;
	    }
	    while (begin + jbuf >= next) {
		if (buf[curr-begin] == 0)
		    hits.setBit(irow, 1);
		++ irow;
		curr = next;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << evt << " -- processed " << irow
		    << " strings from file " << data;

		moresp = (feof(fsp) == 0);
		if (moresp)
		    moresp = (1 == fread(&next, sizeof(next), 1, fsp));
		if (! moresp)
		    break;
	    }
	    if (moresp) {// move back file pointer for fdata
		//fseek(fsp, -static_cast<long>(sizeof(next)), SEEK_CUR);
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break;
	}
    }
    else { // normal null-terminated strings
	std::string pat = str; // convert the string to be search to lower case
	for (uint32_t i = 0; i < pat.length(); ++ i)
	    pat[i] = tolower(pat[i]);
	const uint32_t slen = pat.length() + 1;
	ierr = fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    for (long j = 0; j < jbuf; ++ j) // convert to lower case
		buf[j] = tolower(buf[j]);

	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " expects string # " << irow << " in file \""
		    << data << "\" to be " << (next-begin) << "-byte long, but "
		    << (jbuf<(long)nbuf ? "can only read " :
			"the internal buffer is only ")
		    << jbuf << ", skipping " << jbuf
		    << (jbuf > 1 ? " bytes" : " byte");
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) { // has a whole string
		bool match = (curr+(int64_t)slen == next); // same length?
		long j = curr;
		while (j+4 < next && match) {
		    match = (buf[j-begin] == pat[j-curr]) &&
			(buf[j-begin+1] == pat[j-curr+1]) &&
			(buf[j-begin+2] == pat[j-curr+2]) &&
			(buf[j-begin+3] == pat[j-curr+3]);
		    j += 4;
		}
		if (match) {
		    if (j+4 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]) &&
			    (buf[j-begin+2] == pat[j-curr+2]);
		    }
		    else if (j+3 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]);
		    }
		    else if (j+2 == next) {
			match = (buf[j-begin] == pat[j-curr]);
		    }
		}
		if (match)
		    hits.setBit(irow, 1);
		++ irow;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << evt << " -- processed " << irow
		    << " strings from file " << data;

		curr = next;
		moresp = (feof(fsp) == 0);
		if (moresp)
		    moresp = (1 == fread(&next, sizeof(next), 1, fsp));
		if (! moresp)
		    break;
	    }
	    if (moresp) {// move back file pointer for fdata
		// fseek(fsp, -static_cast<long>(sizeof(next)), SEEK_CUR);
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break; // avoid reading the data file
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }

    fclose(fsp);
    fclose(fdata);
    ibis::fileManager::instance().recordPages(0, next);
    ibis::fileManager::instance().recordPages
	(0, sizeof(uint64_t)*thePart->nRows());
    if (hits.size() != thePart->nRows()) {
	LOGGER(irow != thePart->nRows() && ibis::gVerbose >= 0)
	    << "Warning -- " << evt << "data file \"" << data
	    << "\" contains " << irow << " string" << (irow>1?"s":"")
	    << ", but expected " << thePart->nRows();
	if (irow < thePart->nRows())
	    startPositions(thePart->currentDataDir(), buf, nbuf);
	hits.adjustSize(0, thePart->nRows());
    }
    LOGGER(ibis::gVerbose > 4)
	<< evt << " found " << hits.cnt() << " string" << (hits.cnt()>1?"s":"")
	<< " in \"" << data << "\" matching " << str;
    return hits.cnt();
} // ibis::text::stringSearch

/// Given a group of string literals, return a bitvector that matches
/// anyone of the input strings.
long ibis::text::stringSearch(const std::vector<std::string>& strs,
			      ibis::bitvector& hits) const {
    if (strs.empty()) {
	hits.set(0, thePart->nRows());
	return 0;
    }

    if (strs.size() == 1) // the list contains only one value
	return stringSearch(strs[0].c_str(), hits);

    hits.clear();
    if (thePart == 0) return -1L;

    std::string evt = "text[";
    if (thePart != 0 && thePart->name() != 0) {
	evt += thePart->name();
	evt += '.';
    }
    evt += m_name;
    evt += "]::stringSearch";
    std::string data = thePart->currentDataDir();
    data += FASTBIT_DIRSEP;
    data += m_name;
    FILE *fdata = fopen(data.c_str(), "rb");
    if (fdata == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " can not open data file \"" << data
	    << "\" for reading";
	return -2L;
    }

#if defined(DEBUG) || defined(_DEBUG) // DEBUG+0 > 0 || _DEBUG+0 > 0
    ibis::fileManager::buffer<char> mybuf(5000);
#else
    ibis::fileManager::buffer<char> mybuf;
#endif
    char *buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) return -3L;

    std::string sp = data;
    sp += ".sp";
    FILE *fsp = fopen(sp.c_str(), "rb");
    if (fsp == 0) { // try again
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " can not create or open file \""
		<< sp << "\"";
	    fclose(fdata);
	    return -4L;
	}
    }

    unsigned irow = 0; // row index
    long jbuf = 0; // number of bytes in buffer
    long ierr;
    int64_t begin = 0; // beginning position (file offset) of the bytes in buf
    int64_t curr = 0;
    int64_t next = 0;
    if (1 > fread(&curr, sizeof(curr), 1, fsp)) {
	// odd to be sure, but try again anyway
	fclose(fsp);
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " can not open or read file \""
		<< sp << "\"";
	    fclose(fdata);
	    return -5L;
	}
    }

#if defined(DEBUG) || defined(_DEBUG) // DEBUG+0 > 0 || _DEBUG+0 > 0
    ibis::fileManager::buffer<int64_t> spbuf(1000);
#else
    ibis::fileManager::buffer<int64_t> spbuf;
#endif
    if (spbuf.size() > 1) { // try to use the spbuf for starting positions
	uint32_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt	<< " failed to read " << sp;
	    fclose(fsp);
	    fclose(fdata);
	    return -5L;
	}
	next = spbuf[0];
	nsp = ierr;
	jsp = 1;
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose>0)
		    << "Warning -- " << evt << " string " << irow
		    << " in file \"" << data << "\" is longer "
		    "than internal buffer (size " << jbuf
		    << "), skipping " << jbuf << " bytes";
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) {
		const char *str = buf + (curr - begin);
		bool match = false;
		for (uint32_t i = 0; i < strs.size() && match == false; ++ i) {
		    match = stricmp(strs[i].c_str(), str);
		}
		if (match)
		    hits.setBit(irow, 1);
		++ irow;
		curr = next;
		if (moresp) {
		    if (jsp >= nsp) {
			if (feof(fsp) == 0) {
			    ierr = fread(spbuf.address(), sizeof(int64_t),
					 spbuf.size(), fsp);
			    if (ierr <= 0) {
				LOGGER(ibis::gVerbose >= 0)
				    << "Warning -- " << evt
				    << " failed to read file " << sp;
				moresp = false;
				break;
			    }
			    else {
				nsp = ierr;
			    }
			}
			else { // end of sp file
			    moresp = false;
			    break;
			}
			jsp = 0;
		    }
		    moresp = (jsp < nsp);
		    next = spbuf[jsp];
		    ++ jsp;
		}
	    }
	    if (moresp) {// move back file pointer to reread unused bytes
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break;
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }
    else {
	ierr = fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " string " << irow
		    << " in file \"" << data << "\" is longer "
		    "than internal buffer (size " << jbuf << "), skipping "
		    << jbuf << " bytes";
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) {
		const char *str = buf + (curr - begin);
		bool match = false;
		for (uint32_t i = 0; i < strs.size() && match == false; ++ i) {
		    match = stricmp(strs[i].c_str(), str);
		}
		if (match)
		    hits.setBit(irow, 1);
		++ irow;
		curr = next;
		moresp = (feof(fsp) == 0);
		if (moresp)
		    moresp = (1 == fread(&next, sizeof(next), 1, fsp));
		if (! moresp)
		    break;
	    }
	    if (moresp) {// move back file pointer to reread some bytes
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break;
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }

    fclose(fsp);
    fclose(fdata);
    ibis::fileManager::instance().recordPages(0, next);
    ibis::fileManager::instance().recordPages
	(0, sizeof(uint64_t)*thePart->nRows());
    if (hits.size() != thePart->nRows()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt <<  " data file \"" << data 
	    << "\" contains " << hits.size() << " strings, but "
	    "expected " << thePart->nRows();
	if (hits.size() < thePart->nRows())
	    startPositions(thePart->currentDataDir(), buf, nbuf);
	hits.adjustSize(0, thePart->nRows());
    }
    LOGGER(ibis::gVerbose > 4)
	<< evt << " found " << hits.cnt() << " string" << (hits.cnt()>1?"s":"")
	<< " in \"" << data << "\" matching " << strs.size() << " strings";
    return hits.cnt();
} // ibis::text::stringSearch

long ibis::text::patternSearch(const char* pat, ibis::bitvector& hits) const {
    hits.clear(); // clear the existing content of hits
    if (thePart == 0 || pat == 0 || *pat == 0) return -1L;

    std::string evt = "text[";
    if (thePart != 0 && thePart->name() != 0) {
	evt += thePart->name();
	evt += '.';
    }
    evt += m_name;
    evt += "]::patternSearch";
    std::string data = thePart->currentDataDir();
    data += FASTBIT_DIRSEP;
    data += m_name;
    FILE *fdata = fopen(data.c_str(), "rb");
    if (fdata == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " can not open data file \"" << data
	    << "\" for reading";
	return -2L;
    }

    IBIS_BLOCK_GUARD(fclose, fdata);
#if defined(DEBUG) || defined(_DEBUG) // DEBUG+0 > 0 || _DEBUG+0 > 0
    ibis::fileManager::buffer<char> mybuf(5000);
#else
    ibis::fileManager::buffer<char> mybuf;
#endif
    char *buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) return -3L;

    std::string sp = data;
    sp += ".sp";
    FILE *fsp = fopen(sp.c_str(), "rb");
    if (fsp == 0) { // try again
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " can not create or open file \""
		<< sp << "\"";
	    return -4L;
	}
    }

#if defined(DEBUG) || defined(_DEBUG) // DEBUG+0 > 0 || _DEBUG+0 > 0
    ibis::fileManager::buffer<int64_t> spbuf(100);
#else
    ibis::fileManager::buffer<int64_t> spbuf;
#endif
    uint32_t irow = 0; // row index
    long jbuf = 0; // number of bytes in buffer
    int64_t begin = 0; // beginning position (file offset) of the bytes in buf
    int64_t next = 0;
    int64_t curr, ierr;
    if (1 > fread(&curr, sizeof(curr), 1, fsp)) {
	// odd to be sure, but try again anyway
	fclose(fsp);
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt <<  " can not open or read file \""
		<< sp << "\"";
	    return -5L;
	}
    }
    IBIS_BLOCK_GUARD(fclose, fsp);
    if (spbuf.size() > 1)  { // use the second buffer, spbuf
	uint32_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt << " failed to read file " << sp;
	    return -7L;
	}
	jsp = 1;
	nsp = ierr;
	next = spbuf[0];
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " expects string # " << irow << " in file \""
		    << data << "\" to be " << (next-begin) << "-byte long, but "
		    << (jbuf<(long)nbuf ? "can only read " :
			"the internal buffer is only ")
		    << jbuf << ", skipping " << jbuf
		    << (jbuf > 1 ? " bytes" : " byte");
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) { // whole string
		const bool match = ibis::util::strMatch(buf+(curr-begin), pat);
		if (match)
		    hits.setBit(irow, 1);
#if _DEBUG+0 > 1 || DEBUG+0 > 1
		if (ibis::gVerbose > 5) {
		    ibis::util::logger lg(4);
		    lg()
			<< "DEBUG -- " << evt << " processing string "
			<< irow << " \'";
		    for (long i = curr; i < next-1; ++ i)
			lg() << buf[i-begin];
		    lg() << "\'";
		    if (match)
			lg() << " matches ";
		    else
			lg() << " does not match ";
		    lg() << pat;
		}
#endif
		++ irow;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << evt << " -- processed " << irow
		    << " strings from file " << data;

		curr = next;
		if (moresp) {
		    if (jsp >= nsp) {
			if (feof(fsp) == 0) {
			    ierr = fread(spbuf.address(), sizeof(int64_t),
					 spbuf.size(), fsp);
			    if (ierr <= 0) {
				LOGGER(ibis::gVerbose >= 0)
				    << "Warning -- " << evt
				    << " -- failed to read file " << sp;
				moresp = false;
				break;
			    }
			    else {
				nsp = ierr;
			    }
			}
			else { // end of sp file
			    moresp = false;
			    break;
			}
			jsp = 0;
		    }
		    moresp = (jsp < nsp);
		    next = spbuf[jsp];
		    ++ jsp;
		}
	    }
	    if (moresp) {// move back file pointer in fdata
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break; // avoid reading the data file
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }
    else { // normal null-terminated strings, no spbuf
	ierr = fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " expects string # " << irow << " in file \""
		    << data << "\" to be " << (next-begin) << "-byte long, but "
		    << (jbuf<(long)nbuf ? "can only read " :
			"the internal buffer is only ")
		    << jbuf << ", skipping " << jbuf
		    << (jbuf > 1 ? " bytes" : " byte");
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) { // has a whole string
		const bool match = ibis::util::strMatch(buf+(curr-begin), pat);
		if (match)
		    hits.setBit(irow, 1);
		++ irow;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << evt << " -- processed " << irow
		    << " strings from file " << data;

		curr = next;
		moresp = (feof(fsp) == 0);
		if (moresp)
		    moresp = (1 == fread(&next, sizeof(next), 1, fsp));
		if (! moresp)
		    break;
	    }
	    if (moresp) {// move back file pointer for fdata
		// fseek(fsp, -static_cast<long>(sizeof(next)), SEEK_CUR);
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break; // avoid reading the data file
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }

    ibis::fileManager::instance().recordPages(0, next);
    ibis::fileManager::instance().recordPages
	(0, sizeof(uint64_t)*thePart->nRows());
    if (hits.size() != thePart->nRows()) {
	LOGGER(irow != thePart->nRows() && ibis::gVerbose >= 0)
	    << "Warning -- " << evt << "data file \"" << data
	    << "\" contains " << irow << " string" << (irow>1?"s":"")
	    << ", but expected " << thePart->nRows();
	if (irow < thePart->nRows())
	    startPositions(thePart->currentDataDir(), buf, nbuf);
	hits.adjustSize(0, thePart->nRows());
    }
    LOGGER(ibis::gVerbose > 4)
	<< evt << " found " << hits.cnt() << " string" << (hits.cnt()>1?"s":"")
	<< " in \"" << data << "\" matching " << pat;
    return hits.cnt();
} // ibis::text::patternSearch

/// Write the current metadata to -part.txt of the data partition.
void ibis::text::write(FILE* file) const {
    fputs("\nBegin Column\n", file);
    fprintf(file, "name = \"%s\"\n", (const char*)m_name.c_str());
    if (m_desc.empty() || m_desc == m_name) {
	fprintf(file, "description = %s ", m_name.c_str());
	fprintf(file, "\n");
    }
    else {
	fprintf(file, "description =\"%s\"\n",
	    (const char*)m_desc.c_str());
    }
    fprintf(file, "data_type = \"%s\"\n", TYPESTRING[m_type]);
//     fprintf(file, "minimum = %lu\n", static_cast<long unsigned>(lower));
//     fprintf(file, "maximum = %lu\n", static_cast<long unsigned>(upper));
    if (! m_bins.empty())
	fprintf(file, "index=%s\n", m_bins.c_str());
    fputs("End Column\n", file);
} // ibis::text::write

void ibis::text::print(std::ostream& out) const {
    out << m_name << ": " << m_desc << " (STRING)";
}

/// This indicates to ibis::bundle that every string value is distinct.  It
/// also forces the sorting procedure to produce an order following the
/// order of the entries in the table.  This makes the print out of an
/// ibis::text field quite less useful than others!
ibis::array_t<uint32_t>*
ibis::text::selectUInts(const ibis::bitvector& mask) const {
    array_t<uint32_t>* ret = new array_t<uint32_t>;
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *ind = ix.indices();
	if (ix.isRange()) {
	    for (unsigned i = *ind; i < ind[1]; ++ i)
		ret->push_back(i);
	}
	else {
	    for (uint32_t i = 0; i < ix.nIndices(); ++ i)
		ret->push_back(ind[i]);
	}
    }
    return ret;
} // ibis::text::selectUInts

/// The starting positions of the selected string values are stored in the
/// returned array.
ibis::array_t<int64_t>*
ibis::text::selectLongs(const ibis::bitvector& mask) const {
    std::string fnm = thePart->currentDataDir();
    fnm += FASTBIT_DIRSEP;
    fnm += m_name;
    fnm += ".sp"; // starting position file
    off_t spsize = ibis::util::getFileSize(fnm.c_str());
    if (spsize < 0 || (uint32_t)spsize != (mask.size()+1)*sizeof(int64_t))
	startPositions(thePart->currentDataDir(), (char*)0, 0U);
    array_t<int64_t> sp;
    int ierr = ibis::fileManager::instance().getFile(fnm.c_str(), sp);
    if (ierr != 0) return 0; // can not provide starting positions

    array_t<int64_t>* ret = new array_t<int64_t>;
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *ind = ix.indices();
	if (*ind >= sp.size()) {
	    break;
	}
	else if (ix.isRange()) {
	    const ibis::bitvector::word_t end =
		(ind[1] <= sp.size() ? ind[1] : sp.size());
	    for (unsigned i = *ind; i < end; ++ i)
		ret->push_back(sp[i]);
	}
	else {
	    for (uint32_t i = 0; i < ix.nIndices(); ++ i)
		if (ind[i] < sp.size())
		    ret->push_back(sp[ind[i]]);
	}
    }
    return ret;
} // ibis::text::selectLongs

/// Retrieve the string values from the rows marked 1 in mask.
///
/// @note FastBit does not track the memory usage of neither std::vector
/// nor std::string.
std::vector<std::string>*
ibis::text::selectStrings(const ibis::bitvector& mask) const {
    std::vector<std::string>* res = new std::vector<std::string>();
    if (mask.cnt() == 0) return res;

    std::string fname = thePart->currentDataDir();
    fname += FASTBIT_DIRSEP;
    fname += m_name;
    fname += ".sp";
    off_t spsize = ibis::util::getFileSize(fname.c_str());
    if (spsize < 0 || (uint32_t)spsize != (mask.size()+1)*sizeof(int64_t))
	startPositions(thePart->currentDataDir(), (char*)0, 0U);

    const array_t<int64_t>
	sp(fname.c_str(), static_cast<off_t>(0),
	   static_cast<off_t>((mask.size()+1)*sizeof(int64_t)));
    fname.erase(fname.size()-3); // remove .sp
    int fdata = UnixOpen(fname.c_str(), OPEN_READONLY);
    if (fdata < 0) {
	logWarning("selectStrings", "failed to open data file \"%s\"",
		   fname.c_str());
	delete res;
	return 0;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdata, _O_BINARY);
#endif

    int ierr;
    std::string tmp;
    off_t boffset = 0;
    uint32_t inbuf = 0;
    ibis::fileManager::buffer<char> mybuf;
    char* buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) {
	delete res;
	return 0;
    }

    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *ixval = ix.indices();
	if (ix.isRange()) {
	    const ibis::bitvector::word_t top =
		(ixval[1] <= sp.size()-1 ? ixval[1] : sp.size()-1);
	    for (ibis::bitvector::word_t i = *ixval; i < top; ++ i) {
		ierr = readString(tmp, fdata, sp[i], sp[i+1],
				  buf, nbuf, inbuf, boffset);
		if (ierr >= 0) {
		    res->push_back(tmp);
		}
		else {
		    if (ibis::gVerbose >= 0)
			logWarning("selectStrings", "failed to read strings "
				   "from file \"%s\" (position %ld), "
				   "readString returned ierr=%d",
				   fname.c_str(), static_cast<long>(sp[i]),
				   ierr);
		    UnixClose(fdata);
		    delete res;
		    return 0;
		}
	    }
	}
	else {
	    for (unsigned i = 0; i < ix.nIndices(); ++ i) {
		if (ixval[i] < sp.size()-1) {
		    ierr = readString(tmp, fdata, sp[ixval[i]], sp[ixval[i]+1],
				      buf, nbuf, inbuf, boffset);
		    if (ierr >= 0) {
			res->push_back(tmp);
		    }
		    else {
			if (ibis::gVerbose >= 0)
			    logWarning("selectStrings",
				       "failed to read strings from "
				       "file \"%s\" (position %ld), "
				       "readString returned ierr=%d",
				       fname.c_str(),
				       static_cast<long>(sp[ixval[i]]), ierr);
			UnixClose(fdata);
			delete res;
			return 0;
		    }
		}
	    }
	}
    }
    UnixClose(fdata);
    return res;
} // ibis::text::selectStrings

/// The string starts at position @c be and ends at @c en.  The content may
/// be in the array @c buf.
///
/// Returns 0 if successful, otherwise return a negative number to indicate
/// error.
int ibis::text::readString(std::string& res, int fdes, long be, long en,
			   char* buf, uint32_t nbuf, uint32_t& inbuf,
			   off_t& boffset) const {
    res.clear();
    if (boffset + (off_t)inbuf >= en) { // in buffer
	res = buf + (be - boffset);
    }
    else if (boffset + (off_t)inbuf > be) { // partially in buffer
	for (uint32_t j = be - boffset; j < inbuf; ++ j)
	    res += buf[j];
	
	off_t ierr = UnixSeek(fdes, boffset+inbuf, SEEK_SET);
	if (ierr != boffset+(long)inbuf) {
	    if (ibis::gVerbose > 1)
		logWarning("readString", "unable to move file pointer "
			   "to %ld", static_cast<long>(boffset+inbuf));
	    return -1;
	}
	ierr = UnixRead(fdes, buf, nbuf);
	if (ierr < 0) {
	    if (ibis::gVerbose > 1)
		logWarning("readString", "unable to read from data file "
			   "at position %ld",
			   static_cast<long>(boffset+inbuf));
	    inbuf = 0;
	    return -2;
	}
	boffset += static_cast<long>(inbuf);
	inbuf = static_cast<uint32_t>(ierr);
	be = boffset;
	while ((long)(boffset + inbuf) < en) {
	    for (uint32_t j = 0; j < inbuf; ++ j)
		res += buf[j];
	    ierr = UnixRead(fdes, buf, nbuf);
	    if (ierr < 0) {
		if (ibis::gVerbose > 1)
		    logWarning("readString", "unable to read from data file "
			       "at position %ld",
			       static_cast<long>(boffset+inbuf));
		inbuf = 0;
		return -3;
	    }
	    boffset += inbuf;
	    inbuf = static_cast<uint32_t>(ierr);
	}
	res += buf;
    }
    else { // start reading from @c be
	off_t ierr = UnixSeek(fdes, be, SEEK_SET);
	if (ierr != static_cast<off_t>(be)) {
	    if (ibis::gVerbose > 1)
		logWarning("readString", "unable to move file pointer "
			   "to %ld", static_cast<long>(be));
	    return -4;
	}
	ierr = UnixRead(fdes, buf, nbuf);
	if (ierr < 0) {
	    if (ibis::gVerbose > 1)
		logWarning("readString", "unable to read from data file "
			   "at position %ld", static_cast<long>(be));
	    inbuf = 0;
	    return -5;
	}
	boffset = be;
	inbuf = static_cast<uint32_t>(ierr);
	while (en > boffset+(off_t)inbuf) {
	    for (uint32_t j = 0; j < inbuf; ++ j)
		res += buf[j];
	    ierr = UnixRead(fdes, buf, nbuf);
	    if (ierr < 0) {
		if (ibis::gVerbose > 1)
		    logWarning("readString", "unable to read from data file "
			       "at position %ld", static_cast<long>(be));
		inbuf = 0;
		return -6;
	    }
	    boffset += inbuf;
	    inbuf = static_cast<uint32_t>(ierr);
	}
	res += buf;
    }
#if _DEBUG+0 > 2 || DEBUG+0 > 1
    LOGGER(ibis::gVerbose > 5)
	<< "DEBUG -- text[" << partition()->name() << '.' << m_name
	<< "]::readString got string value \"" << res
	<< "\" from file descriptor " << fdes << " offsets " << be << " -- "
	<< en;
#endif
    return 0;
} // ibis::text::readString

/// It goes through a two-stage process by reading from two files, first
/// from the .sp file to read the position of the string in the second file
/// and the second file contains the actual string values (with nil
/// terminators).  This can be quite slow!
void ibis::text::readString(uint32_t i, std::string &ret) const {
    ret.clear();
    if (thePart == 0 || i >= thePart->nRows() ||
	thePart->currentDataDir() == 0 ||
	*(thePart->currentDataDir()) == 0) return;
    std::string fnm = thePart->currentDataDir();
    fnm += FASTBIT_DIRSEP;
    fnm += m_name;
    fnm += ".sp"; // starting position file

    long ierr = 0;
    int64_t positions[2];
    // open the file explicitly to read two number
    int des = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (des < 0) {
	startPositions(thePart->currentDataDir(), 0, 0);
	des = UnixOpen(fnm.c_str(), OPEN_READONLY);
	if (des < 0) {
	    logWarning("readString", "failed to open file \"%s\"",
		       fnm.c_str());
	    return;
	}
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(des, _O_BINARY);
#endif
    ierr = UnixSeek(des, i*sizeof(int64_t), SEEK_SET);
    if (ierr != static_cast<long>(i*sizeof(int64_t))) {
	(void) UnixClose(des);
	startPositions(thePart->currentDataDir(), 0, 0);
	des = UnixOpen(fnm.c_str(), OPEN_READONLY);
	if (des < 0) {
	    logWarning("readString", "failed to open file \"%s\"",
		       fnm.c_str());
	    return;
	}

	ierr = UnixSeek(des, i*sizeof(int64_t), SEEK_SET);
	if (ierr != (long) (i*sizeof(int64_t))) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- text::readString(" << i << ") failed to seek to "
		<< i*sizeof(int64_t) << " in " << fnm;
	    (void) UnixClose(des);
	    return;
	}
    }
    ierr = UnixRead(des, &positions, sizeof(positions));
    if (ierr != static_cast<long>(sizeof(positions))) {
	(void) UnixClose(des);
	startPositions(thePart->currentDataDir(), 0, 0);
	des = UnixOpen(fnm.c_str(), OPEN_READONLY);
	if (des < 0) {
	    logWarning("readString", "failed to open file \"%s\"",
		       fnm.c_str());
	    return;
	}

	ierr = UnixSeek(des, i*sizeof(int64_t), SEEK_SET);
	if (ierr != static_cast<long>(i*sizeof(int64_t))) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- text::readString(" << i << ") failed to seek to "
		<< i*sizeof(int64_t) << " in " << fnm;
	    (void) UnixClose(des);
	    return;
	}

	ierr = UnixRead(des, &positions, sizeof(positions));
	if (ierr != static_cast<long>(sizeof(positions))) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- text::readString(" << i << ") failed to read "
		<< sizeof(positions) << " bytes from " << fnm;
	    (void) UnixClose(des);
	    return;
	}
    }
    (void) UnixClose(des);
    ibis::fileManager::instance().recordPages
	(i*sizeof(int64_t), i*sizeof(int64_t)+sizeof(positions));

    fnm.erase(fnm.size()-3); // remove ".sp"
    int datafile = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (datafile < 0) {
	logWarning("readString", "failed to open file \"%s\"",
		   fnm.c_str());
	return;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(datafile, _O_BINARY);
#endif
    ierr = UnixSeek(datafile, *positions, SEEK_SET);
    if (ierr != *positions) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- text::readString(" << i << ") failed to seek to "
	    << *positions << " in file " << fnm;
	return;
    }
    char buf[1025];
    buf[1024] = 0;
    for (long j = positions[0]; j < positions[1]; j += 1024) {
	long len = positions[1] - j;
	if (len > 1024)
	    len = 1024;
	ierr = UnixRead(datafile, buf, len);
	if (ierr > 0) {
	    LOGGER(ibis::gVerbose > 2 && ierr < len)
		<< "Warning -- text::readString(" << i << ") expected to read "
		<< len << " bytes, but only read " << ierr;
	    ret.insert(ret.end(), buf, buf + ierr - (buf[ierr-1] == 0));
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- text::readString(" << i << ") failed to read "
		<< len << " bytes from " << fnm << ", read returned " << ierr;
	}
    }
    (void) UnixClose(datafile);
    ibis::fileManager::instance().recordPages(positions[0], positions[1]);
} // ibis::text::readString

/// If the input string is found in the data file, it is returned, else
/// this function returns 0.  It needs to keep both the data file and the
/// starting position file open at the same time.
const char* ibis::text::findString(const char *str) const {
    std::string data = thePart->currentDataDir();
    data += FASTBIT_DIRSEP;
    data += m_name;
    FILE *fdata = fopen(data.c_str(), "rb");
    if (fdata == 0) {
	logWarning("findString", "can not open data file \"%s\" for reading",
		   data.c_str());
	return 0;
    }

    ibis::fileManager::buffer<char> mybuf;
    char *buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- text["
	    << (thePart != 0 ? thePart->name() : "") << "." << name()
	    << "]::findString(" << str << ") unable to allocate "
	    "enough work space";
	return 0;
    }

    std::string sp = data;
    sp += ".sp";
    FILE *fsp = fopen(sp.c_str(), "rb");
    if (fsp == 0) { // try again
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    logWarning("findString", "can not create or open file \"%s\"",
		       sp.c_str());
	    fclose(fdata);
	    return 0;
	}
    }

    uint32_t irow = 0; // row index
    long begin = 0; // beginning position (file offset) of the bytes in buf
    long jbuf = 0; // number of bytes in buffer
    long curr;
    if (1 > fread(&curr, sizeof(curr), 1, fsp)) {
	// odd to be sure, but try again anyway
	fclose(fsp);
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    logWarning("findString", "can not create, open or read starting "
		       "position file \"%s\"", sp.c_str());
	    fclose(fdata);
	    return 0;
	}
    }

    long ierr;
    long next = 0;
    bool found = false;
    if (str == 0 || *str == 0) { // only match empty strings
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0 && ! found) {
	    bool moresp = true;
	    ierr = fread(&next, sizeof(next), 1, fsp);
	    if (ierr < 1 || next > begin+jbuf) {
		logWarning("findString", "string %lu in file \"%s\" is longer "
			   "than internal buffer (size %ld), skipping %ld "
			   "bytes", static_cast<long unsigned>(irow),
			   data.c_str(), jbuf, jbuf);
		curr += jbuf;
	    }
	    while (begin + jbuf >= next) {
		if (buf[curr-begin] == 0) {
		    found = true;
		    break;
		}
		++ irow;
		curr = next;
		moresp = (feof(fsp) == 0);
		if (moresp)
		    moresp = (1 == fread(&next, sizeof(next), 1, fsp));
		if (! moresp)
		    break;
	    }
	    if (moresp) {// move back a word
		fseek(fsp, -static_cast<long>(sizeof(next)), SEEK_CUR);
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break;
	}
    }
    else { // normal null-terminated strings
	const long slen = strlen(str);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0 && ! found) {
	    bool moresp = true;
	    ierr = fread(&next, sizeof(next), 1, fsp);
	    if (ierr < 1 || next > begin+jbuf) {
		logWarning("findString", "string %lu in file \"%s\" is longer "
			   "than internal buffer (size %ld), skipping %ld "
			   "bytes", static_cast<long unsigned>(irow),
			   data.c_str(), jbuf, jbuf);
		curr += jbuf;
	    }
	    while (begin+jbuf >= next) {
		bool match = (curr+slen+1 == next); // the same length
		// the actual string match is case-insensitive
		for (long i = curr; i < next-1 && match; ++ i)
		    match = (buf[i-begin] == str[i-curr] ||
			     (islower(buf[i-begin]) &&
			      buf[i-begin] == tolower(str[i-curr])) ||
			     (isupper(buf[i-begin]) &&
			      buf[i-begin] == toupper(str[i-curr])));
		if  (match) {
		    found = true;
		    break;
		}
		++ irow;
		curr = next;
		moresp = (feof(fsp) == 0);
		if (moresp)
		    moresp = (1 == fread(&next, sizeof(next), 1, fsp));
		if (! moresp)
		    break;
	    }
	    if (moresp) {// move back a word
		fseek(fsp, -static_cast<long>(sizeof(next)), SEEK_CUR);
		fseek(fdata, curr, SEEK_SET);
		begin = curr;
	    }
	    else
		break; // avoid reading the data file
	} // while (jbuf > 0) -- as long as there are bytes to examine
    }

    fclose(fsp);
    fclose(fdata);
    ibis::fileManager::instance().recordPages(0, next);
    ibis::fileManager::instance().recordPages
	(0, sizeof(uint64_t)*thePart->nRows());

    if (found)
	return str;
    else
	return 0;
} // ibis::text::findString

/// Locate the ID column for processing term-document list provided by the
/// user.  This function checks indexSpec first for docIDName=xx for the
/// name of the ID column, then checks the global parameter
/// <table-name>.<column-name>.docIDName.
const ibis::column* ibis::text::IDColumnForKeywordIndex() const {
    const ibis::column* idcol = 0;
    const char* spec = indexSpec();
    if (spec && *spec != 0) {
	const char* str = strstr(spec, "docidname");
	if (str == 0) {
	    str = strstr(spec, "docIDName");
	    if (str == 0) {
		str = strstr(spec, "docIdName");
		if (str == 0)
		    str = strstr(spec, "DOCIDNAME");
	    }
	}
	if (str != 0 && *str != 0) {
	    str += 9;
	    str += strspn(str, " \t=");
	    char *tmp = ibis::util::getString(str);
	    if (tmp != 0 && *tmp != 0)
		idcol = partition()->getColumn(tmp);
	    delete [] tmp;
	}
	if (idcol == 0) {
	    str = strstr(spec, "docid");
	    if (str == 0) {
		str = strstr(spec, "docID");
		if (str == 0) {
		    str = strstr(spec, "docId");
		    if (str == 0)
			str = strstr(spec, "DOCID");
		}
	    }
	    if (str != 0 && *str != 0) {
		str += 5;
		str += strspn(str, " \t=");
		char *tmp = ibis::util::getString(str);
		if (tmp != 0 && *tmp != 0)
		    idcol = partition()->getColumn(tmp);
		delete [] tmp;
	    }
	}
    }
    if (idcol == 0) {
	std::string idcpar = partition()->name();
	idcpar += '.';
	idcpar += m_name;
	idcpar += ".docIDName";
	const char* idname = ibis::gParameters()[idcpar.c_str()];
	if (idname != 0)
	    idcol = partition()->getColumn(idname);
    }
    return idcol;
} // ibis::text::IDColumnForKeywordIndex

void ibis::text::TDListForKeywordIndex(std::string& fname) const {
    fname.erase(); // erase existing content in fname
    if (thePart != 0 && thePart->currentDataDir() != 0)
	startPositions(thePart->currentDataDir(), 0, 0);

    const char* spec = indexSpec();
    if (spec != 0 && *spec != 0) {
	const char* str = strstr(spec, "tdlist");
	if (str == 0) {
	    str = strstr(spec, "TDList");
	    if (str == 0) {
		str = strstr(spec, "tdList");
		if (str == 0)
		    str = strstr(spec, "TDLIST");
	    }
	}
	if (str != 0 && *str != 0) {
	    str += 5;
	    str += strspn(str, " \t=");
	    (void) ibis::util::readString(fname, str);
	}
    }
    if (fname.empty()) {
	std::string idcpar = partition()->name();
	idcpar += '.';
	idcpar += m_name;
	idcpar += ".TDList";
	const char* idname = ibis::gParameters()[idcpar.c_str()];
	if (idname != 0)
	    fname = idname;
    }
} // ibis::text::TDListForKeywordIndex

void ibis::text::delimitersForKeywordIndex(std::string& fname) const {
    fname.erase(); // erase existing content in fname
    const char* spec = indexSpec();
    if (spec != 0 && *spec != 0) {
	const char* str = strstr(spec, "delimiters");
	if (str == 0) {
	    str = strstr(spec, "Delimiters");
	    if (str == 0) {
		str = strstr(spec, "DELIMITERS");
	    }
	}
	if (str != 0 && *str != 0) {
	    str += 10;
	    str += strspn(str, " \t=");
	    (void) ibis::util::readString(fname, str);
	}
	else {
	    str = strstr(spec, "delim");
	    if (str == 0) {
		str = strstr(spec, "Delim");
		if (str == 0) {
		    str = strstr(spec, "DELIM");
		}
	    }
	    if (str != 0 && *str != 0) {
		str += 5;
		str += strspn(str, " \t=");
		(void) ibis::util::readString(fname, str);
	    }
	}
    }
    if (fname.empty()) {
	std::string idcpar = partition()->name();
	idcpar += '.';
	idcpar += m_name;
	idcpar += ".delimiters";
	const char* idname = ibis::gParameters()[idcpar.c_str()];
	if (idname != 0)
	    fname = idname;
    }
} // ibis::text::delimitersForKeywordIndex

long ibis::text::keywordSearch(const char* str, ibis::bitvector& hits) const {
    long ierr = 0;
    try {
	startPositions(0, 0, 0);
	indexLock lock(this, "keywordSearch");
	if (idx) {
	    ierr = reinterpret_cast<ibis::keywords*>(idx)->search(str, hits);
	}
	else {
	    ierr = -2;
	}
    }
    catch (...) {
	ierr = -1;
    }
    return ierr;
} // ibis::text::keywordSearch

long ibis::text::keywordSearch(const char* str) const {
    long ierr = 0;
    try {
	startPositions(0, 0, 0);
	indexLock lock(this, "keywordSearch");
	if (idx) {
	    ierr = reinterpret_cast<ibis::keywords*>(idx)->search(str);
	}
	else {
	    ierr = -2;
	}
    }
    catch (...) {
	ierr = -1;
    }
    return ierr;
} // ibis::text::keywordSearch

double ibis::text::estimateCost(const ibis::qString& cmp) const {
    double ret = partition()->nRows() *
	static_cast<double>(sizeof(uint64_t));
    return ret;
} // ibis::text::estimateCost

double ibis::text::estimateCost(const ibis::qMultiString& cmp) const {
    double ret = partition()->nRows() *
	static_cast<double>(sizeof(uint64_t));
    return ret;
} // ibis::text::estimateCost

/// Write the selected values to the specified directory.  If the
/// destination directory is the current data directory, the file
/// containing existing string values will be renamed to be
/// column-name.old, otherwise, the file in the destination directory is
/// simply overwritten.  In case of error, a negative number is returned,
/// otherwise, the number of rows saved to the new file is returned.
long ibis::text::saveSelected(const ibis::bitvector& sel, const char *dest,
			      char *buf, uint32_t nbuf) {
    if (thePart == 0 || thePart->currentDataDir() == 0)
	return -1;

    startPositions(thePart->currentDataDir(), 0, 0);
    long ierr = 0;
    ibis::bitvector msk;
    getNullMask(msk);
    if (dest == 0 || dest == thePart->currentDataDir() ||
	strcmp(dest, thePart->currentDataDir()) == 0) {
	// use the active directory, need a write lock
	std::string fname = thePart->currentDataDir();
	fname += FASTBIT_DIRSEP;
	fname += m_name;
	std::string gname = fname;
	gname += ".old";
	std::string sname = fname;
	sname += ".sp";
	std::string tname = sname;
	tname += ".old";

	writeLock lock(this, "saveSelected");
	if (idx != 0) {
	    const uint32_t idxc = idxcnt();
	    if (0 == idxc) {
		delete idx;
		idx = 0;
		purgeIndexFile(thePart->currentDataDir());
	    }
	    else {
		logWarning("saveSelected", "index files are in-use, "
			   "should not overwrite data files");
		return -2;
	    }
	}
	ibis::fileManager::instance().flushFile(fname.c_str());

	ierr = rename(fname.c_str(), gname.c_str());
	if (ierr != 0) {
	    logWarning("saveSelected", "failed to rename %s to %s -- %s",
		       fname.c_str(), gname.c_str(), strerror(errno));
	    return -3;
	}
	ierr = rename(sname.c_str(), tname.c_str());
	if (ierr != 0) {
	    logWarning("saveSelected", "failed to rename %s to %s -- %s",
		       sname.c_str(), tname.c_str(), strerror(errno));
	    return -4;
	}
	ierr = writeStrings(fname.c_str(), gname.c_str(),
			    sname.c_str(), tname.c_str(),
			    msk, sel, buf, nbuf);
    }
    else {
	// using two separate sets of files, need a read lock only
	std::string fname = dest;
	fname += FASTBIT_DIRSEP;
	fname += m_name;
	std::string gname = thePart->currentDataDir();
	gname += FASTBIT_DIRSEP;
	gname += m_name;
	std::string sname = fname;
	sname += ".sp";
	std::string tname = gname;
	tname += ".sp";

	purgeIndexFile(dest);
	readLock lock(this, "saveSelected");
	ierr = writeStrings(fname.c_str(), gname.c_str(),
			    sname.c_str(), tname.c_str(),
			    msk, sel, buf, nbuf);
    }
    return ierr;
} // ibis::text::saveSelected

/// Write the selected strings.  The caller manages the necessary locks for
/// accessing this function.
int ibis::text::writeStrings(const char *to, const char *from,
			     const char *spto, const char *spfrom,
			     ibis::bitvector &msk, const ibis::bitvector &sel,
			     char *buf, uint32_t nbuf) const {
    std::string evt = "text[";
    evt += thePart->name();
    evt += '.';
    evt += m_name;
    evt += "]::writeStrings";
    ibis::fileManager::buffer<char> mybuf(buf != 0);
    if (buf == 0) { // incoming buf is nil, use mybuf
	nbuf = mybuf.size();
	buf = mybuf.address();
    }
    if (buf == 0 || to == 0 || from == 0 || spfrom == 0 || spto == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " failed to allocate work space to read strings";
	return -10;
    }

    int rffile = UnixOpen(from, OPEN_READONLY);
    if (rffile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << from
	    << " for reading";
	return -11;
    }
    IBIS_BLOCK_GUARD(UnixClose, rffile);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(rffile, _O_BINARY);
#endif

    int sffile = UnixOpen(spfrom, OPEN_READONLY);
    if (sffile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << spfrom
	    << " for reading";
	return -12;
    }
    IBIS_BLOCK_GUARD(UnixClose, sffile);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(sffile, _O_BINARY);
#endif

    int rtfile = UnixOpen(to, OPEN_APPENDONLY, OPEN_FILEMODE);
    if (rtfile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << to
	    << " for writing";
	return -13;
    }
    IBIS_BLOCK_GUARD(UnixClose, rtfile);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(rtfile, _O_BINARY);
#endif

    int stfile = UnixOpen(spto, OPEN_APPENDONLY, OPEN_FILEMODE);
    if (rtfile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << spto
	    << " for writing";
	return -14;
    }
    IBIS_BLOCK_GUARD(UnixClose, stfile);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(stfile, _O_BINARY);
#endif

    int64_t ierr, pos;
    for (ibis::bitvector::indexSet ix = sel.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *idx = ix.indices();
	if (ix.isRange()) { // write many values together
	    ibis::bitvector::word_t irow = *idx;
	    pos = *idx * 8;
	    ierr = UnixSeek(sffile, pos, SEEK_SET);
	    if (pos == ierr) {
		// copy one string at a time
		int64_t rfbegin, rfend;
		ierr = UnixRead(sffile, &rfbegin, 8);
		if (ierr == 8) {
		    ierr = UnixSeek(rffile, rfbegin, SEEK_SET);
		    if (ierr == rfbegin)
			ierr = 8;
		    else
			ierr = 0;
		}
		while (irow < idx[1] && ierr == 8) {
		    ierr = UnixRead(sffile, &rfend, 8);
		    if (ierr != 8) break;

		    pos = UnixSeek(rtfile, 0, SEEK_CUR);
		    ierr = UnixWrite(stfile, &pos, 8);
		    if (ierr != 8) { // unrecoverable trouble
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- " << evt
			    << " failed to write the value " << pos
			    << " to " << spto << ", "
			    << (errno ? strerror(errno) : "??");
			return -15;
		    }

		    pos = rfend - rfbegin;
		    for (int jtmp = 0; jtmp < pos; jtmp += nbuf) {
			int bytes = (jtmp+nbuf < pos ? nbuf : pos-jtmp);
			ierr = UnixRead(rffile, buf, bytes);
			if (ierr == bytes) {
			    ierr = UnixWrite(rtfile, buf, bytes);
			    if (ierr != bytes) {
				LOGGER(ibis::gVerbose >= 0)
				    << "Warning -- " << evt
				    << " failed to write " << bytes
				    << " byte" << (bytes>1?"s":"") << " to "
				    << to << ", "
				    << (errno ? strerror(errno) : "??");
				return -16;
			    }
			}
			else {
			    LOGGER(ibis::gVerbose >= 0)
				<< "Warning -- " << evt << " failed to read "
				<< bytes << " byte" << (bytes>1?"s":"")
				<< " from " << from << ", "
				<< (errno ? strerror(errno) : "??");
			    return -17;
			}
		    }

		    rfbegin = rfend;
		    ierr = 8;
		    ++ irow;
		} // while (irow
	    }
	    else {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt << " failed to seek to " << pos
		    << " in file " << spfrom << ", seek function returned "
		    << ierr;
	    }

	    if (irow < idx[1]) {
		(void) memset(buf, 0, nbuf);
		pos = UnixSeek(rtfile, 0, SEEK_CUR);
		for (int jtmp = irow; jtmp < static_cast<long>(idx[1]);
		     ++ jtmp) {
		    ierr = UnixWrite(stfile, &pos, 8);
		    if (ierr != 8) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- " << evt
			    << " failed to write the value " << pos
			    << " to " << spto << ", unable to continue";
			return -18;
		    }
		}
		while (irow < idx[1]) {
		    int bytes = (idx[1]-irow > nbuf ? nbuf : idx[1]-irow);
		    ierr = UnixWrite(rffile, buf, bytes);
		    if (ierr != bytes) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- " << evt << " failed to write "
			    << bytes << " byte" << (bytes>1?"s":"") << " to "
			    << to << ", unable to continue";
			return -19;
		    }
		    irow += bytes;
		}
	    }
	}
	else {
	    for (unsigned jdx = 0; jdx < ix.nIndices(); ++ jdx) {
		pos = UnixSeek(rtfile, 0, SEEK_CUR);
		ierr = UnixWrite(stfile, &pos, 8);
		if (ierr != 8) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- " << evt
			<< " failed to write the value " << pos
			<< " to " << spto << ", unable to continue";
		    return -20;
		}

		pos = idx[jdx] * 8;
		ierr = UnixSeek(sffile, pos, SEEK_SET);
		if (ierr == pos) {
		    int64_t rfbegin;
		    ierr = UnixRead(sffile, &rfbegin, 8);
		    if (ierr == 8) {
			ierr = UnixSeek(rffile, rfbegin, SEEK_SET);
			bool more = (ierr == rfbegin);
			if (! more)
			    ierr = 0;
			while (more) {
			    ierr = UnixRead(rffile, buf, nbuf);
			    for (pos = 0; pos < ierr && buf[pos] != 0; ++ pos);
			    if (pos < ierr) {
				more = false;
				++ pos;
			    }
			    if (pos > 0) {
				ierr = UnixWrite(rtfile, buf, pos);
				if (ierr == pos) {
				    ierr = 8;
				}
				else {
				    LOGGER(ibis::gVerbose >= 0)
					<< "Warning -- " << evt
					<< " failed to write " << pos
					<< " byte" << (pos>1?"s":"")
					<< " to " << to
					<< ", unable to continue";
				    return -21;
				}
			    }
			} // while (more)
		    }
		}
		else {
		    ierr = 0;
		}

		if (ierr != 8) { // write a null string
		    buf[0] = 0;
		    ierr = UnixWrite(rtfile, buf, 1);
		    if (ierr != 1) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- " << evt
			    << " failed to write 1 byte to " << to
			    << ", unable to continue";
			return -22;
		    }
		}
	    } // for (unsigned jdx
	}
    } // for (ibis::bitvector::indexSet ix

    pos = UnixSeek(rtfile, 0, SEEK_CUR);
    ierr = UnixWrite(stfile, &pos, 8);
    LOGGER(ierr != 8 && ibis::gVerbose >= 0)
	<< "Warning -- " << evt << " failed to write the last position "
	<< pos << " to " << spto;
    if (ibis::gVerbose > 1) {
	int nr = sel.cnt();
	logMessage("writeStrings", "copied %d string%s from %s to %s",
		   nr, (nr > 1 ? "s" : ""), from, to);
    }

    ibis::bitvector bv;
    msk.subset(sel, bv);
    bv.adjustSize(0, sel.cnt());
    bv.swap(msk);
    return sel.cnt();
} // ibis::text::writeStrings

/// Contruct a blob by reading from a metadata file.
ibis::blob::blob(const part *prt, FILE *file) : ibis::column(prt, file) {
}

/// Construct a blob from a name.
ibis::blob::blob(const part *prt, const char *nm)
    : ibis::column(prt, ibis::BLOB, nm) {
}

/// Copy an existing column object of type ibis::BLOB.
ibis::blob::blob(const ibis::column &c) : ibis::column(c) {
    if (m_type != ibis::BLOB)
	throw "can not construct an ibis::blob from another type";
}

/// Write metadata about the column.
void ibis::blob::write(FILE *fptr) const {
    fprintf(fptr, "\nBegin Column\nname = %s\ndescription = %s\ntype = blob\n"
	    "End Column\n", m_name.c_str(),
	    (m_desc.empty() ? m_name.c_str() : m_desc.c_str()));
} // ibis::blob::write

/// Print information about this column.
void ibis::blob::print(std::ostream& out) const {
    out << m_name << ": " << m_desc << " (BLOB)";
} // ibis::blob::print

/// Append the content in @c df to the end of files in @c dt.  It returns
/// the number of rows appended or a negative number to indicate error
/// conditions.
long ibis::blob::append(const char* dt, const char* df, const uint32_t nold,
			const uint32_t nnew, uint32_t nbuf, char* buf) {
    if (nnew == 0 || dt == 0 || df == 0 || *dt == 0 || *df == 0 ||
	dt == df || strcmp(dt, df) == 0)
	return 0;
    std::string evt = "blob[";
    if (thePart != 0)
	evt += thePart->name();
    else
	evt += "?";
    evt += '.';
    evt += m_name;
    evt += "]::append";

    const char spelem = 8; // starting positions are 8-byte intergers
    writeLock lock(this, evt.c_str());
    std::string datadest, spdest;
    std::string datasrc, spfrom;
    datadest += dt;
    datadest += FASTBIT_DIRSEP;
    datadest += m_name;
    datasrc += df;
    datasrc += FASTBIT_DIRSEP;
    datasrc += m_name;
    spdest = datadest;
    spdest += ".sp";
    spfrom = datasrc;
    spfrom += ".sp";
    LOGGER(ibis::gVerbose > 3)
	<< evt << " -- source \"" << datasrc << "\" --> destination \""
	<< datadest << "\", nold=" << nold << ", nnew=" << nnew;

    // rely on .sp file for existing data size
    int sdest = UnixOpen(spdest.c_str(), OPEN_READWRITE, OPEN_FILEMODE);
    if (sdest < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " unable to open file \"" << spdest
	    << "\" for append ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -2;
    }
    ibis::util::guard gsdest = ibis::util::makeGuard(UnixClose, sdest);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(sdest, _O_BINARY);
#endif

    // verify the existing sizes of data file and start positions match
    long sj = UnixSeek(sdest, 0, SEEK_END);
    if (sj < 0 || sj % spelem != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects file " << spdest
	    << " to have a multiple of " << spelem << " bytes, but it is "
	    << sj << ", will not continue with corrupt data files";
	return -3;
    }
    int ierr;
    const uint32_t nsold = sj / spelem;
    const uint32_t nold0 = (nsold > 1 ? nsold-1 : 0);
    int64_t dfsize = 0;
    if (nsold == 0) {
	ierr = UnixWrite(sdest, &dfsize, spelem);
	if (ierr < (int)spelem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " expects to write " << spelem
		<< " to " << spdest << ", but the write function returned "
		<< ierr;
	    return -4;
	}
    }
    else if (nold0 < nold) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- data file " << spdest << " is expected to have"
	    << nold+1 << " entries, but found only " << nsold
	    << ", attempt to extend the file with the last value in it";

	ierr = UnixSeek(sdest, -spelem, SEEK_END);
	if (ierr < (int) (sj - spelem)) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to seek to position "
		<< sj-spelem << " in file " << spdest;
	    return -5;
	}
	ierr = UnixRead(sdest, &dfsize, spelem);
	if (ierr < (int)spelem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to read the last "
		<< spelem << " bytes from " << spdest;
	    return -6;
	}
	for (unsigned j = nold0; j < nold; ++ j) {
	    ierr = UnixWrite(sdest, &dfsize, spelem);
	    if (ierr < (int)spelem) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " failed to write the value "
		    << dfsize << " to the end of " << spdest;
		return -7;
	    }
	}
    }
    else if (nold0 > nold) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- data file " << spdest << " is expected to have "
	    << nold+1 << " entries, but found " << nsold
	    << ", the extra entries will be overwritten";

	ierr = UnixSeek(sdest, spelem*nold, SEEK_SET);
	if (ierr < (int) (spelem*nold)) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to seek to " << spelem*nold
		<< " in file " << spdest;
	    return -8;
	}
	ierr = UnixRead(sdest, &dfsize, spelem);
	if (ierr < (int)spelem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to read " << spelem
		<< " bytes from position " << nold*spelem << " in file "
		<< spdest;
	    return -9;
	}
    }

    // .sp file pointer should at at spelem*(nold+1)
    ierr = UnixSeek(sdest, 0, SEEK_CUR);
    if ((unsigned) ierr != spelem*(nold+1)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects file pointer to be at "
	    << spelem*(nold+1) << ", but it is actually at " << ierr;
	return -10;
    }

    int ssrc = UnixOpen(spfrom.c_str(), OPEN_READONLY);
    if (ssrc < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " failed to open file " << spfrom
	    << " for reading -- "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -11;
    }
    ibis::util::guard gssrc = ibis::util::makeGuard(UnixClose, ssrc);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(ssrc, _O_BINARY);
#endif

    // a buffer object is always decleared, but may be only 1-byte in size
    ibis::fileManager::buffer<char> dbuff((nbuf != 0));
    if (nbuf == 0) {
	nbuf = dbuff.size();
	buf = dbuff.address();
    }
    if (nbuf <= (unsigned)spelem) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " unable to continue because of "
	    "insufficient amount of available buffer space";
	return -1;
    }
    if ((unsigned long)nold+nnew >= (unsigned long)(INT_MAX / spelem)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " unable to continue because the "
	    "resulting .sp will be too large";
	return -1;
    }

    const uint32_t nspbuf = nbuf / spelem;
    uint64_t *spbuf = (uint64_t*) buf;
    int64_t dj = 0;
    uint32_t nnew0 = 0;
    for (uint32_t j = 0; j <= nnew; j += nspbuf) {
	ierr = UnixRead(ssrc, spbuf, nbuf);
	if (ierr <= 0) {
	    LOGGER(ierr < 0 && ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to read from "
		<< spfrom << ", function read returned " << ierr;
	    break;
	}
	int iread = ierr;
	if (j == 0) {
	    dj = dfsize - *spbuf;
	    iread -= spelem;
	    for (int i = 0; i < iread/spelem; ++ i)
		spbuf[i] = spbuf[i+1] + dj;
	}
	else {
	    for (int i = 0; i < iread/spelem; ++ i)
		spbuf[i] += dj;
	}
	int iwrite = UnixWrite(sdest, spbuf, iread);
	if (iwrite < iread) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " expects to write " << iread
		<< " byte" << (iread>1?"s":"") << ", but only wrote " << iwrite;
	    return -12;
	}
	nnew0 = iwrite / spelem;
    }
    // explicit close the read source file to reduce the number of open files
    (void) UnixClose(ssrc);
    gssrc.dismiss();

#if defined(FASTBIT_SYNC_WRITE)
#if _POSIX_FSYNC+0 > 0
    (void) UnixFlush(sdest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
    (void) _commit(sdest);
#endif
#endif
    // close the destination .sp file just in case we need to truncate it
    UnixClose(sdest);
    gsdest.dismiss();
    if (sj > static_cast<long>(spelem*(nold+nnew0))) {
	LOGGER(ibis::gVerbose > 3)
	    << evt << " truncating extra bytes in file " << spdest;
	ierr = truncate(spdest.c_str(), spelem*(nold+nnew0));
    }
    LOGGER(ibis::gVerbose > 4)
	<< evt << " appended " << nnew0 << " element" << (nnew0>1?"s":"")
	<< " from " << spfrom << " to " << spdest;

    // open destination data file
    int ddest = UnixOpen(datadest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
    if (ddest < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " unable to open file \"" << datadest
	    << "\" for append ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -13;
    }
    // this statement guarantees UnixClose will be called on ddest upon
    // termination of this function
    ibis::util::guard gddest = ibis::util::makeGuard(UnixClose, ddest);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(ddest, _O_BINARY);
#endif
    dj = UnixSeek(ddest, 0, SEEK_END);
    if (dj != dfsize) {
	if (dj < dfsize) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " expects " << datadest
		<< " to have " << dfsize << " byte" << (dfsize>1 ? "s" : "")
		<< ", but it actually has " << dj;
	    return -14;
	}
	else {
	    dj = UnixSeek(ddest, dfsize, SEEK_SET);
	    if (dj != dfsize) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " failed to seek to " << dfsize
		    << " in file " << datadest << ", function seek returned "
		    << dj;
		return -15;
	    }
	    else {
		LOGGER(ibis::gVerbose > 1)
		    << evt << " will overwrite the content after position "
		    << dfsize << " in file " << datadest;
	    }
	}
    }

    int dsrc = UnixOpen(datasrc.c_str(), OPEN_READONLY);
    if (dsrc < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " failed to open file \"" << datasrc
	    << "\" for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -16;
    }
    ibis::util::guard gdsrc = ibis::util::makeGuard(UnixClose, dsrc);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(dsrc, _O_BINARY);
#endif
    while ((ierr = UnixRead(dsrc, buf, nbuf)) > 0) {
	int iwrite = UnixWrite(ddest, buf, ierr);
	LOGGER(ibis::gVerbose > 1 && iwrite < ierr)
	    << "Warning -- " << evt << " expects to write " << ierr
	    << " byte" << (ierr > 1 ? "s" : "") << ", but only wrote "
	    << iwrite;
    }
#if defined(FASTBIT_SYNC_WRITE)
#if  _POSIX_FSYNC+0 > 0
    (void) UnixFlush(ddest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
    (void) _commit(ddest);
#endif
#endif
    (void) UnixClose(dsrc);
    gdsrc.dismiss();
    (void) UnixClose(ddest);
    gddest.dismiss();
    LOGGER(ibis::gVerbose > 4)
	<< evt << " appended " << nnew0 << " row" << (nnew0>1?"s":"");

    //////////////////////////////////////////////////
    // deals with the masks
    std::string filename;
    filename = datasrc;
    filename += ".msk";
    ibis::bitvector mapp;
    try {mapp.read(filename.c_str());} catch (...) {/* ok to continue */}
    mapp.adjustSize(nnew0, nnew0);
    LOGGER(ibis::gVerbose > 7)
	<< evt << " mask file \"" << filename << "\" contains "
	<< mapp.cnt() << " set bits out of " << mapp.size()
	<< " total bits";

    filename = datadest;
    filename += ".msk";
    ibis::bitvector mtot;
    try {mtot.read(filename.c_str());} catch (...) {/* ok to continue */}
    mtot.adjustSize(nold0, nold);
    LOGGER(ibis::gVerbose > 7)
	<< evt << " mask file \"" << filename << "\" contains " << mtot.cnt()
	<< " set bits out of " << mtot.size() << " total bits before append";

    mtot += mapp; // append the new ones at the end
    if (mtot.size() != nold+nnew0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects the combined mask to have "
	    << nold+nnew0 << " bits, but has " << mtot.size();
	mtot.adjustSize(nold+nnew0, nold+nnew0);
    }
    if (mtot.cnt() != mtot.size()) {
	mtot.write(filename.c_str());
	if (ibis::gVerbose > 6) {
	    logMessage("append", "mask file \"%s\" indicates %lu valid "
		       "records out of %lu", filename.c_str(),
		       static_cast<long unsigned>(mtot.cnt()),
		       static_cast<long unsigned>(mtot.size()));
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    LOGGER(ibis::gVerbose >= 0) << mtot;
#endif
	}
    }
    else {
	remove(filename.c_str()); // no need to have the file
	if (ibis::gVerbose > 6)
	    logMessage("append", "mask file \"%s\" removed, all "
		       "%lu records are valid", filename.c_str(),
		       static_cast<long unsigned>(mtot.size()));
    }
    if (thePart == 0 || thePart->currentDataDir() == 0)
	return nnew0;
    if (strcmp(dt, thePart->currentDataDir()) == 0) {
	// update the mask stored internally
	mutexLock lck(this, "column::append");
	mask_.swap(mtot);
    }

    return nnew0;
} // ibis::blob::append

/// Write the content of BLOBs packed into two arrays va1 and va2.  All
/// BLOBs are packed together one after another in va1 and their starting
/// positions are stored in va2.  The last element of va2 is the total
/// number of bytes in va1.  The array va2 is expected to hold (nnew+1)
/// 64-bit integers.
///
/// @note The array va2 is modified in this function to have a starting
/// position that is the end of the existing data file.
long ibis::blob::writeData(const char* dir, uint32_t nold, uint32_t nnew,
			   ibis::bitvector& mask, const void *va1, void *va2) {
    if (nnew == 0 || va1 == 0 || va2 == 0 || dir == 0 || *dir == 0)
	return 0;

    std::string evt = "blob[";
    if (thePart != 0)
	evt += thePart->name();
    else
	evt += "?";
    evt += '.';
    evt += m_name;
    evt += "]::writeData";

    const char spelem = 8; // starting positions are 8-byte intergers
    int64_t *sparray = static_cast<int64_t*>(va2);
    int ierr;
    int64_t dfsize = 0;
    std::string datadest, spdest;
    datadest += dir;
    datadest += FASTBIT_DIRSEP;
    datadest += m_name;
    spdest = datadest;
    spdest += ".sp";
    LOGGER(ibis::gVerbose > 3)
	<< evt << " starting to write " << nnew << " blob" << (nnew>1?"s":"")
	<< " to \"" << datadest << "\", nold=" << nold;

    // rely on .sp file for existing data size
    int sdest = UnixOpen(spdest.c_str(), OPEN_READWRITE, OPEN_FILEMODE);
    if (sdest < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " unable to open file \"" << spdest
	    << "\" for append ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -2;
    }
    ibis::util::guard gsdest = ibis::util::makeGuard(UnixClose, sdest);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(sdest, _O_BINARY);
#endif

    // make sure there are right number of start positions
    long sj = UnixSeek(sdest, 0, SEEK_END);
    if (sj < 0 || sj % spelem != 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects file " << spdest
	    << " to have a multiple of " << spelem << " bytes, but it is "
	    << sj << ", will not continue with corrupt data files";
	return -3;
    }
    const uint32_t nsold = sj / spelem;
    const uint32_t nold0 = (nsold > 1 ? nsold-1 : 0);
    if (nsold == 0) {
	ierr = UnixWrite(sdest, &dfsize, spelem);
	if (ierr < (int)spelem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " expects to write " << spelem
		<< " to " << spdest << ", but the write function returned "
		<< ierr;
	    return -4;
	}
    }
    else if (nold0 < nold) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- data file " << spdest << " is expected to have"
	    << nold+1 << " entries, but found only " << nsold
	    << ", attempt to extend the file with the last value in it";

	ierr = UnixSeek(sdest, -spelem, SEEK_END);
	if (ierr < (int) (sj - spelem)) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to seek to position "
		<< sj-spelem << " in file " << spdest;
	    return -5;
	}
	ierr = UnixRead(sdest, &dfsize, spelem);
	if (ierr < (int)spelem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to read the last "
		<< spelem << " bytes from " << spdest;
	    return -6;
	}
	for (unsigned j = nold0; j < nold; ++ j) {
	    ierr = UnixWrite(sdest, &dfsize, spelem);
	    if (ierr < (int)spelem) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " failed to write the value "
		    << dfsize << " to the end of " << spdest;
		return -7;
	    }
	}
    }
    else if (nold0 > nold) {
	LOGGER(ibis::gVerbose > 1)
	    << evt << " -- data file " << spdest << " is expected to have "
	    << nold+1 << " entries, but found " << nsold
	    << ", the extra entries will be overwritten";

	ierr = UnixSeek(sdest, spelem*nold, SEEK_SET);
	if (ierr < (int) (spelem*nold)) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to seek to " << spelem*nold
		<< " in file " << spdest;
	    return -8;
	}
	ierr = UnixRead(sdest, &dfsize, spelem);
	if (ierr < (int)spelem) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " failed to read " << spelem
		<< " bytes from position " << nold*spelem << " in file "
		<< spdest;
	    return -9;
	}
    }

    // .sp file pointer should at at spelem*(nold+1)
    ierr = UnixSeek(sdest, 0, SEEK_CUR);
    if ((unsigned) ierr != spelem*(nold+1)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects file pointer to be at "
	    << spelem*(nold+1) << ", but it is actually at " << ierr;
	return -10;
    }

    if (dfsize != *sparray) {
	int64_t offset = dfsize - *sparray;
	for (unsigned j = 0; j <= nnew; ++ j)
	    sparray[j] += offset;
    }
    int64_t dj = UnixWrite(sdest, sparray+1, spelem*nnew);
#if defined(FASTBIT_SYNC_WRITE)
#if  _POSIX_FSYNC+0 > 0
    (void) UnixFlush(sdest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
    (void) _commit(sdest);
#endif
#endif
    if (dj < static_cast<long>(spelem*nnew)) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects to write " << spelem*nnew
	    << " bytes to " << spdest << ", but the function write returned "
	    << dj;
	return -11;
    }

    // close the destination .sp file just in case we need to truncate it
    UnixClose(sdest);
    gsdest.dismiss();
    if (sj > static_cast<long>(spelem*(nold+nnew))) {
	LOGGER(ibis::gVerbose > 3)
	    << evt << " truncating extra bytes in file " << spdest;
	ierr = truncate(spdest.c_str(), spelem*(nold+nnew));
    }
    LOGGER(ibis::gVerbose > 4)
	<< evt << " appended " << nnew << " element" << (nnew>1?"s":"")
	<< " to " << spdest;

    // open destination data file
    int ddest = UnixOpen(datadest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
    if (ddest < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " unable to open file \"" << datadest
	    << "\" for append ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -13;
    }
    // this statement guarantees UnixClose will be called on ddest upon
    // termination of this function
    IBIS_BLOCK_GUARD(UnixClose, ddest);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(ddest, _O_BINARY);
#endif
    dj = UnixSeek(ddest, 0, SEEK_END);
    if (dj != dfsize) {
	if (dj < dfsize) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << evt << " expects " << datadest
		<< " to have " << dfsize << " byte" << (dfsize>1 ? "s" : "")
		<< ", but it actually has " << dj;
	    return -14;
	}
	else {
	    dj = UnixSeek(ddest, dfsize, SEEK_SET);
	    if (dj != dfsize) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " failed to seek to " << dfsize
		    << " in file " << datadest << ", function seek returned "
		    << dj;
		return -15;
	    }
	    else {
		LOGGER(ibis::gVerbose > 1)
		    << evt << " will overwrite the content after position "
		    << dfsize << " in file " << datadest;
	    }
	}
    }

    dfsize = sparray[nnew] - *sparray;
    dj = UnixWrite(ddest, va1, dfsize);
#if defined(FASTBIT_SYNC_WRITE)
#if _POSIX_FSYNC+0 > 0
    (void) UnixFlush(ddest); // write to disk
#elif defined(_WIN32) && defined(_MSC_VER)
    (void) _commit(ddest);
#endif
#endif
    if (dj < dfsize) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " expects to write " << dfsize
	    << " byte" << (dfsize>1?"s":"") << " to " << datadest
	    << ", but the function write returned " << dj;
    }
    LOGGER(ibis::gVerbose > 4)
	<< evt << " appended " << nnew << " row" << (nnew>1?"s":"");

    //////////////////////////////////////////////////
    // deals with the masks
    mask.adjustSize(nold0, nold);
    mask.adjustSize(nold+nnew, nold+nnew);

    return nnew;
} // ibis::blob::writeData

/// Count the number of bytes in the blobs selected by the mask.  This
/// function can be used to compute the memory requirement before actually
/// retrieving the blobs.
///
/// It returns a negative number in case of error.
long ibis::blob::countRawBytes(const ibis::bitvector& mask) const {
    if (mask.cnt() == 0)
	return 0;
    if (thePart == 0)
	return -1;
    if (mask.size() > thePart->nRows())
	return -2;

    const char* dir = thePart->currentDataDir();
    if (dir == 0 || *dir == 0)
	return -3;

    std::string spfile = dir;
    spfile += FASTBIT_DIRSEP;
    spfile += m_name;
    spfile += ".sp";
    array_t<int64_t> starts;
    long sum = 0;
    int ierr = ibis::fileManager::instance().getFile(spfile.c_str(), starts);
    if (ierr >= 0) {
	if (starts.size() <= thePart->nRows())
	    starts.clear();
    }
    else {
	starts.clear();
    }

    if (starts.size() > mask.size()) { // start positions are usable
	for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *idx = ix.indices();
	    if (ix.isRange()) {
		sum += starts[idx[1]] - starts[*idx];
	    }
	    else {
		for (unsigned jdx = 0; jdx < ix.nIndices(); ++ jdx) {
		    sum += starts[idx[jdx]+1] - starts[idx[jdx]];
		}
	    }
	}
    }
    else { // have to open the .sp file to read the starting positions
	int fsp = UnixOpen(spfile.c_str(), OPEN_READONLY);
	if (fsp < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- blob::countRawBytes failed to open file "
		<< spfile << " for reading ... "
		<< (errno ? strerror(errno) : "no free stdio stream");
	    return -4;
	}
	IBIS_BLOCK_GUARD(UnixClose, fsp);
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fsp, _O_BINARY);
#endif

	const char spelem = 8;
	long pos;
	for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *idx = ix.indices();
	    if (ix.isRange()) {
		int64_t start, end;
		pos = *idx * spelem;
		ierr = UnixSeek(fsp, pos, SEEK_SET);
		if (ierr != pos) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- blob::countRawBytes failed to seek to "
			<< pos << " in " << spfile;
		    return -5;
		}
		ierr = UnixRead(fsp, &start, spelem);
		if (ierr < spelem) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- blob::countRawBytes failed to read "
			<< spelem << " bytes from position " << pos
			<< " in " << spfile;
		    return -6;
		}
		pos = idx[1] * spelem; 
		ierr = UnixSeek(fsp, pos, SEEK_SET);
		if (ierr != pos) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- blob::countRawBytes failed to seek to "
			<< pos << " in " << spfile;
		    return -7;
		}
		ierr = UnixRead(fsp, &end, spelem);
		if (ierr < spelem) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- blob::countRawBytes failed to read "
			<< spelem << " bytes from position " << pos
			<< " in " << spfile;
		    return -8;
		}
		sum += (end - start);
	    }
	    else {
		int64_t buf[2];
		for (unsigned jdx = 0; jdx < ix.nIndices(); ++ jdx) {
		    pos = idx[jdx] * spelem;
		    ierr = UnixSeek(fsp, pos, SEEK_SET);
		    if (ierr != pos) {
			LOGGER(ibis::gVerbose > 0)
			    << "Warning -- blob::countRawBytes failed to "
			    "seek to" << pos << " in " << spfile;
			return -9;
		    }
		    ierr = UnixRead(fsp, buf, sizeof(buf));
		    if (ierr < (int)sizeof(buf)) {
			LOGGER(ibis::gVerbose > 0)
			    << "Warning -- blob::countRawBytes failed to"
			    " read " << sizeof(buf) << " bytes from position "
			    << pos << " in " << spfile;
			return -10;
		    }
		    sum += (buf[1] - buf[0]);
		}
	    }
	}
    }
    return sum;
} // ibis::blob::countRawBytes

/// Extract the blobs from the rows marked 1 in the mask.  Upon successful
/// completion, buffer will contain all the raw bytes packed together,
/// positions will contain the starting positions of all blobs, and the
/// return value will be the number of blobs retrieved.  The positions are
/// intentionally chosen to be 32-bit integers, so that it would not be
/// possible to retrieve very large objects this way.  The number of blobs
/// retrieved may be less than the number of rows marked 1 in mask if do so
/// will cause buffer to be more 4GB in size.  On a typical machine,
/// retrieving this function will attempt to use no more than half of the
/// free memory available to ibis::fileManager upon entering this function,
/// which usually would be much less than 4GB.  To determine how much
/// memory would be needed by the buffer to full retrieve all blobs marked
/// 1, use function ibis::blob::countRawBytes.
///
/// A negative value will be returned in case of error.
int ibis::blob::selectRawBytes(const ibis::bitvector& mask,
			       ibis::array_t<unsigned char>& buffer,
			       ibis::array_t<uint32_t>& positions) const {
    buffer.clear();
    positions.clear();
    if (mask.cnt() == 0)
	return 0;
    if (thePart == 0)
	return -1;
    if (mask.size() > thePart->nRows())
	return -2;

    const char* dir = thePart->currentDataDir();
    if (dir == 0 || *dir == 0)
	return -3;

    std::string datafile = dir;
    datafile += FASTBIT_DIRSEP;
    datafile += m_name;
    std::string spfile = datafile;
    spfile += ".sp";

    // we intend for buffer to not use more than bufferlimit number of bytes.
    const int64_t bufferlimit = buffer.capacity() +
	(ibis::fileManager::bytesFree() >> 1);
    array_t<int64_t> starts;
    int ierr = ibis::fileManager::instance().getFile(spfile.c_str(), starts);
    if (ierr >= 0) {
	if (starts.size() <= thePart->nRows())
	    starts.clear();
    }
    else {
	starts.clear();
    }

    try {
	uint32_t sum = 0;
	positions.reserve(mask.size()+1);
	if (starts.size() > mask.size()) { // array starts usable
	    // first determine the size of buffer
	    bool smll = true;
	    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
		 ix.nIndices() > 0 && smll; ++ ix) {
		const ibis::bitvector::word_t *idx = ix.indices();
		if (ix.isRange()) {
		    if (sum + (starts[idx[1]] - starts[*idx]) <= bufferlimit) {
			sum += (starts[idx[1]] - starts[*idx]);
		    }
		    else {
			for (unsigned jdx = *idx; smll && jdx < idx[1];
			     ++ jdx) {
			    if (sum + (starts[jdx+1] - starts[jdx]) <=
				bufferlimit)
				sum += (starts[jdx+1] - starts[jdx]);
			    else
				smll = false;
			}
		    }
		}
		else {
		    for (unsigned jdx = 0; jdx < ix.nIndices() && smll;
			 ++ jdx) {
			if (sum + (starts[idx[jdx]+1] - starts[idx[jdx]]) <=
			    bufferlimit)
			    sum += starts[idx[jdx]+1] - starts[idx[jdx]];
			else
			    smll = false;
		    }
		}
	    }

	    // reserve space for buffer
	    buffer.reserve(sum);
	    // attempt to put all bytes of datafile into an array_t
	    array_t<unsigned char> raw;
	    ierr = ibis::fileManager::instance().getFile(datafile.c_str(), raw);
	    if (ierr < 0) {
		raw.clear();
		LOGGER(ibis::gVerbose > 3)
		    << "blob::countRawBytes getFile(" << datafile
		    << ") returned " << ierr << ", will explicit read the file";
	    }
	    else if (raw.size() < (unsigned) starts.back()) {
		raw.clear();
		LOGGER(ibis::gVerbose > 3)
		    << "blob::countRawBytes getFile(" << datafile
		    << " returned an array with " << raw.size()
		    << " bytes, but " << starts.back()
		    << " are expected, will try explicitly reading the file";
	    }
	    if (smll) {
		if (raw.size() >= (unsigned) starts.back())
		    ierr = extractAll(mask, buffer, positions, raw, starts);
		else
		    ierr = extractAll(mask, buffer, positions,
				      datafile.c_str(), starts);
	    }
	    else {
		if (raw.size() >= (unsigned) starts.back())
		    ierr = extractSome(mask, buffer, positions, raw, starts,
				       sum);
		else
		    ierr = extractSome(mask, buffer, positions,
				       datafile.c_str(), starts, sum);
	    }
	}
	else { // have to open the .sp file to read the starting positions
	    buffer.reserve(bufferlimit);
	    ierr = extractSome(mask, buffer, positions, datafile.c_str(),
			       spfile.c_str(), bufferlimit);
	}
    }
    catch (const std::exception& e) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::selectRawBytes (" << datafile
	    << ") terminating due to a std::exception -- " << e.what();
	ierr = -4;
    }
    catch (const char* s) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::selectRawBytes (" << datafile
	    << ") terminating due to a string exception -- " << s;
	ierr = -5;
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::selectRawBytes (" << datafile
	    << ") terminating due to a unknown exception";
	ierr = -6;
    }

    if (ierr >= 0)
	ierr = (positions.size() > 1 ? positions.size() - 1 : 0);
    return ierr;
} // ibis::blob::selectRawBytes

/// Extract entries marked 1 in mask from raw to buffer.  Fill positions to
/// indicate the start and end positions of each raw binary object.  Caller
/// has determined that there is sufficient amount of space to perform this
/// operations and have reserved enough space for buffer.  Even though that
/// may not be a guarantee, we proceed as if it is.
int ibis::blob::extractAll(const ibis::bitvector& mask,
			   ibis::array_t<unsigned char>& buffer,
			   ibis::array_t<uint32_t>& positions,
			   const ibis::array_t<unsigned char>& raw,
			   const ibis::array_t<int64_t>& starts) const {
    positions.resize(1);
    positions[0] = 0;
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *ids = ix.indices();
	if (ix.isRange()) {
	    buffer.insert(buffer.end(), raw.begin()+starts[*ids],
			  raw.begin()+starts[ids[1]]);
	    for (unsigned j = *ids; j < ids[1]; ++ j) {
		positions.push_back(positions.back()+(starts[j+1]-starts[j]));
	    }
	}
	else {
	    for (unsigned j = 0; j < ix.nIndices(); ++ j) {
		buffer.insert(buffer.end(), raw.begin()+starts[ids[j]],
			      raw.begin()+starts[1+ids[j]]);
		positions.push_back(positions.back() +
				    (starts[1+ids[j]]-starts[ids[j]]));
	    }
	}
    }
    return (starts.size()-1);
} // ibis::blob::extractAll

/// Extract entries marked 1 in mask from raw to buffer subject to a limit
/// on the buffer size.  Fill positions to indicate the start and end
/// positions of each raw binary object.  Caller has determined that there
/// is the amount of space to perform this operations and have reserved
/// enough space for buffer.  Even though that may not be a guarantee, we
/// proceed as if it is.
int ibis::blob::extractSome(const ibis::bitvector& mask,
			    ibis::array_t<unsigned char>& buffer,
			    ibis::array_t<uint32_t>& positions,
			    const ibis::array_t<unsigned char>& raw,
			    const ibis::array_t<int64_t>& starts,
			    const uint32_t limit) const {
    positions.resize(1);
    positions[0] = 0;
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0 && buffer.size() < limit; ++ ix) {
	const ibis::bitvector::word_t *ids = ix.indices();
	if (ix.isRange()) {
	    for (unsigned j = *ids; j < ids[1] && buffer.size() < limit; ++ j) {
		buffer.insert(buffer.end(), raw.begin()+starts[j],
			      raw.begin()+starts[j+1]);
		positions.push_back(positions.back()+(starts[j+1]-starts[j]));
	    }
	}
	else {
	    for (unsigned j = 0; j < ix.nIndices() && buffer.size() < limit;
		 ++ j) {
		buffer.insert(buffer.end(), raw.begin()+starts[ids[j]],
			      raw.begin()+starts[1+ids[j]]);
		positions.push_back(positions.back() +
				    (starts[1+ids[j]]-starts[ids[j]]));
	    }
	}
    }
    return (starts.size()-1);
} // ibis::blob::extractSome

/// Retrieve all binary objects marked 1 in the mask.  The caller has
/// reserved enough space for buffer and positions.  This function simply
/// needs to open rawfile and read the content into buffer.  It also
/// assigns values in starts to mark the boundaries of the binary objects.
int ibis::blob::extractAll(const ibis::bitvector& mask,
			   ibis::array_t<unsigned char>& buffer,
			   ibis::array_t<uint32_t>& positions,
			   const char* rawfile,
			   const ibis::array_t<int64_t>& starts) const {
    int fdes = UnixOpen(rawfile, OPEN_READONLY);
    if (fdes < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::extractAll failed to open " << rawfile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -11;
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    positions.resize(1);
    positions[0] = 0;
    int64_t ierr;
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *ids = ix.indices();
	if (ix.isRange()) {
	    ierr = UnixSeek(fdes, starts[*ids], SEEK_SET);
	    if (ierr != starts[*ids]) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- blob::extractAll failed to seek to position "
		    << starts[*ids] << " in " << rawfile
		    << " to retrieve record # " << *ids << " -- " << ids[1];
		return -12;
	    }

	    const int64_t bytes = starts[ids[1]] - starts[*ids];
	    const uint32_t bsize = buffer.size();
	    buffer.resize(bsize+bytes);
	    ierr = UnixRead(fdes, buffer.begin()+bsize, bytes);
	    if (ierr < bytes) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- blob::extractAll expects to read " << bytes
		    << " byte" << (bytes>1?"s":"")
		    << ", but the read function returned " << ierr
		    << ", (reading started at " << starts[*ids]
		    << " in " << rawfile << ")";
		return -13;
	    }
	    for (unsigned j = *ids; j < ids[1]; ++ j) {
		positions.push_back(positions.back()+(starts[j+1]-starts[j]));
	    }
	}
	else {
	    for (unsigned j = 0; j < ix.nIndices(); ++ j) {
		int64_t curr = starts[ids[j]];
		ierr = UnixSeek(fdes, curr, SEEK_SET);
		if (ierr != curr) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractAll failed to seek to "
			<< curr << " in " << rawfile
			<< " to retrieve record # " << ids[j];
		    return -14;
		}

		const int64_t bytes = starts[1+ids[j]] - starts[ids[j]];
		const uint32_t bsize = buffer.size();
		buffer.resize(bsize+bytes);
		ierr = UnixRead(fdes, buffer.begin()+bsize, bytes);
		if (ierr < bytes) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractAll expects to read "
			<< bytes << " byte" << (bytes>1?"s":"")
			<< ", but the read function returned " << ierr
			<< ", (reading started at " << curr
			<< " in " << rawfile << ")";
		    return -15;
		}
		positions.push_back(positions.back() + bytes);
	    }
	}
    }
    return (positions.size()-1);
} // ibis::blob::extractAll

/// Retrieve binary objects marked 1 in the mask subject to the specified
/// limit on buffer size.  The caller has reserved enough space for buffer
/// and positions.  This function simply needs to open rawfile and read the
/// content into buffer.  It also assigns values in starts to mark the
/// boundaries of the binary objects.
int ibis::blob::extractSome(const ibis::bitvector& mask,
			    ibis::array_t<unsigned char>& buffer,
			    ibis::array_t<uint32_t>& positions,
			    const char* rawfile,
			    const ibis::array_t<int64_t>& starts,
			    const uint32_t limit) const {
    int fdes = UnixOpen(rawfile, OPEN_READONLY);
    if (fdes < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::extractSome failed to open " << rawfile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -11;
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    int64_t ierr;
    positions.resize(1);
    positions[0] = 0;
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0 && buffer.size() < limit; ++ ix) {
	const ibis::bitvector::word_t *ids = ix.indices();
	if (ix.isRange()) {
	    ierr = UnixSeek(fdes, starts[*ids], SEEK_SET);
	    if (ierr != starts[*ids]) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- blob::extractSome failed to seek to "
		    << starts[*ids] << " in " << rawfile
		    << " to retrieve record # " << *ids << " -- " << ids[1];
		return -12;
	    }

	    for (unsigned j = *ids; j < ids[1] && buffer.size() < limit; ++ j) {
		const int64_t bytes = starts[j+1] - starts[j];
		const uint32_t bsize = buffer.size();
		buffer.resize(bsize+bytes);
		ierr = UnixRead(fdes, buffer.begin()+bsize, bytes);
		if (ierr < bytes) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome expects to read "
			<< bytes << " byte" << (bytes>1?"s":"")
			<< ", but the read function returned " << ierr
			<< ", (reading started at " << starts[*ids]
			<< " in " << rawfile << ")";
		    return -13;
		}
		positions.push_back(bsize+bytes);
	    }
	}
	else {
	    for (unsigned j = 0; j < ix.nIndices() && buffer.size() < limit;
		 ++ j) {
		int64_t curr = starts[ids[j]];
		ierr = UnixSeek(fdes, curr, SEEK_SET);
		if (ierr != curr) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome failed to seek to "
			<< curr << " in " << rawfile
			<< " to retrieve record # " << ids[j];
		    return -14;
		}

		const int64_t bytes = starts[1+ids[j]] - starts[ids[j]];
		const uint32_t bsize = buffer.size();
		buffer.resize(bsize+bytes);
		ierr = UnixRead(fdes, buffer.begin()+bsize, bytes);
		if (ierr < bytes) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome expects to read "
			<< bytes << " byte" << (bytes>1?"s":"")
			<< ", but the read function returned " << ierr
			<< ", (reading started at " << curr
			<< " in " << rawfile << ")";
		    return -15;
		}
		positions.push_back(positions.back() + bytes);
	    }
	}
    }
    return (positions.size()-1);
} // ibis::blob::extractSome

/// Retrieve binary objects marked 1 in the mask subject to the specified
/// limit on buffer size.  The caller has reserved enough space for buffer
/// and positions.  This function needs to open both rawfile and spfile.
/// It reads starting positions in spfile to determine where to read the
/// content from rawfile into buffer.  It also assigns values in starts to
/// mark the boundaries of the binary objects in buffer.
int ibis::blob::extractSome(const ibis::bitvector& mask,
			    ibis::array_t<unsigned char>& buffer,
			    ibis::array_t<uint32_t>& positions,
			    const char* rawfile,
			    const char* spfile,
			    const uint32_t limit) const {
    // sdes - for spfile
    int sdes = UnixOpen(spfile, OPEN_READONLY);
    if (sdes < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::extractSome failed to open " << spfile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -11;
    }
    IBIS_BLOCK_GUARD(UnixClose, sdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(sdes, _O_BINARY);
#endif

    // rdes - for rawfile
    int rdes = UnixOpen(rawfile, OPEN_READONLY);
    if (rdes < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::extractSome failed to open " << rawfile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -12;
    }
    IBIS_BLOCK_GUARD(UnixClose, rdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(rdes, _O_BINARY);
#endif

    positions.resize(1);
    positions[0] = 0;
    int64_t ierr, stmp[2];
    for (ibis::bitvector::indexSet ix = mask.firstIndexSet();
	 ix.nIndices() > 0; ++ ix) {
	const ibis::bitvector::word_t *ids = ix.indices();
	if (ix.isRange()) {
	    stmp[0] = 8 * *ids;
	    ierr = UnixSeek(sdes, stmp[0], SEEK_SET);
	    if (ierr != stmp[0]) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- blob::extractSome failed to seek to "
		    << stmp[0] << " in " << spfile
		    << " to retrieve the starting positions for blob " << *ids;
		return -13;
	    }
	    ierr = UnixRead(sdes, stmp, 16);
	    if (ierr < 16) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- blob::extractSome failed to read start "
		    "and end positions for blob " << *ids << " from " << spfile;
		return -14;
	    }
	    ierr = UnixSeek(rdes, stmp[0], SEEK_SET);
	    if (ierr != stmp[0]) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- blob::extractSome failed to seek to "
		    << stmp[0] << " in " << rawfile
		    << " to retrieve record # " << *ids << " -- " << ids[1];
		return -15;
	    }

	    for (unsigned j = *ids; j < ids[1]; ++ j) {
		const int64_t bytes = stmp[1] - stmp[0];
		const uint32_t bsize = buffer.size();
		if (bsize+bytes > limit)
		    return (positions.size()-1);

		buffer.resize(bsize+bytes);
		ierr = UnixRead(rdes, buffer.begin()+bsize, bytes);
		if (ierr < bytes) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome expects to read "
			<< bytes << " byte" << (bytes>1?"s":"") << " from "
			<< rawfile << ", but the read function returned "
			<< ierr;
		    return -16;
		}
		positions.push_back(bsize+bytes);

		if (j+1 < ids[1]) { // read next end positions
		    stmp[0] = stmp[1];
		    ierr = UnixRead(sdes, stmp+1, 8);
		    if (ierr < 8) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- blob::extractSome failed to read "
			    "the ending position of blob " << j+1 << " from "
			    << spfile;
			return -17;
		    }
		}
	    }
	}
	else {
	    for (unsigned j = 0; j < ix.nIndices(); ++ j) {
		stmp[0] = 8 * ids[j];
		ierr = UnixSeek(sdes, stmp[0], SEEK_SET);
		if (ierr != stmp[0]) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome failed to seek to "
			<< stmp[0] << " in " << spfile
			<< " to retrieve positions of blob " << ids[j];
		    return -18;
		}
		ierr = UnixRead(sdes, stmp, 16);
		if (ierr < 16) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome failed to read "
			"start and end positions of blob " << ids[j]
			<< " from " << spfile;
		    return -19;
		}
		const int64_t bytes = stmp[1] - stmp[0];
		const uint32_t bsize = buffer.size();
		if (bsize+bytes > limit) {
		    return (positions.size()-1);
		}

		ierr = UnixSeek(rdes, stmp[0], SEEK_SET);
		if (ierr != stmp[0]) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome failed to seek to "
			<< stmp[0] << " in " << rawfile
			<< " to retrieve blob " << ids[j];
		    return -20;
		}

		buffer.resize(bsize+bytes);
		ierr = UnixRead(rdes, buffer.begin()+bsize, bytes);
		if (ierr < bytes) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- blob::extractSome expects to read "
			<< bytes << " byte" << (bytes>1?"s":"")
			<< ", but the read function returned " << ierr
			<< ", (reading started at " << stmp[0]
			<< " in " << rawfile << ")";
		    return -21;
		}
		positions.push_back(positions.back() + bytes);
	    }
	}
    }
    return (positions.size()-1);
} // ibis::blob::extractSome

/// Extract a single binary object.  This function is only defined for
/// ibis::blob, therefore the caller must explicitly case a column* to
/// blob*.  It needs to access two files, a file for start positions and
/// another for raw binary data.  Thus it has a large startup cost
/// associated with opening the files and seeking to the right places on
/// disk.  If there is enough memory available, it will attempt to make
/// these files available for later invocations of this function by making
/// their content available through array_t objects.  If it fails to create
/// the desired array_t objects, it will fall back to use explicit I/O
/// function calls.
int ibis::blob::getBlob(uint32_t ind, unsigned char *&buf, uint32_t &size)
    const {
    if (thePart == 0) return -1;
    if (ind > thePart->nRows()) return -2;
    const char* dir = thePart->currentDataDir();
    if (dir == 0 || *dir == 0)
	return -3;

    std::string datafile = dir;
    datafile += FASTBIT_DIRSEP;
    datafile += m_name;
    std::string spfile = datafile;
    spfile += ".sp";
    array_t<int64_t> starts;
    int ierr = ibis::fileManager::instance().getFile(spfile.c_str(), starts);
    if (ierr >= 0) {
	if (starts.size() <= thePart->nRows())
	    starts.clear();
    }
    else {
	starts.clear();
    }

    if (starts.size() > thePart->nRows()) {
	if (starts[ind+1] <= starts[ind]) {
	    size = 0;
	    return 0;
	}

	uint64_t diff = starts[ind+1]-starts[ind];
	if (buf == 0 || size < diff) {
	    delete buf;
	    buf = new unsigned char[diff];
	}
	size = diff;

	array_t<unsigned char> bytes;
	ierr = ibis::fileManager::instance().getFile(datafile.c_str(), bytes);
	if (ierr >= 0) {
	    if (bytes.size() >= (size_t)starts[ind+1]) {
		std::copy(bytes.begin()+starts[ind],
			  bytes.begin()+starts[ind+1], buf);
	    }
	    else {
		ierr = readBlob(ind, buf, size, starts, datafile.c_str());
	    }
	}
	else {
	    ierr = readBlob(ind, buf, size, starts, datafile.c_str());
	}
    }
    else {
	ierr = readBlob(ind, buf, size, spfile.c_str(), datafile.c_str());
    }
    return ierr;
} // ibis::blob::getBlob

/// Read a single binary object.  The starting position is available in an
/// array_t object.  It only needs to explicitly open the data file to
/// read.
int ibis::blob::readBlob(uint32_t ind, unsigned char *&buf, uint32_t &size,
			 const array_t<int64_t> &starts, const char *datafile)
    const {
    if (starts[ind+1] <= starts[ind]) {
	size = 0;
	return 0;
    }
    uint64_t diff = starts[ind+1]-starts[ind];
    if (buf == 0 || size < diff) {
	delete buf;
	buf = new unsigned char[diff];
    }
    if (buf == 0)
	return -10;

    int fdes = UnixOpen(datafile, OPEN_READONLY);
    if (fdes < 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob failed to open " << datafile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -11;
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    off_t ierr = UnixSeek(fdes, starts[ind], SEEK_SET);
    if (ierr != starts[ind]) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob(" << ind << ") failed to seek to "
	    << starts[ind] << " in " << datafile << ", seek returned "
	    << ierr;
	return -12;
    }

    ierr = UnixRead(fdes, buf, diff);
    if (ierr < (off_t)diff) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob(" << ind << ") failed to read "
	    << diff << " byte" << (diff>1?"s":"") << " from " << datafile
	    << ", read returned " << ierr;
	return -13;
    }
    size = diff;
    if (size == diff)
	ierr = 0;
    else
	ierr = -14;
    return ierr;
} // ibis::blob::readBlob

/// Read a single binary object.  This function opens both starting
/// position file and data file explicitly.
int ibis::blob::readBlob(uint32_t ind, unsigned char *&buf, uint32_t &size,
			 const char *spfile, const char *datafile) const {
    int64_t starts[2];
    const uint32_t spelem = 8;
    int sdes = UnixOpen(spfile, OPEN_READONLY);
    if (sdes < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- blob::readBlob failed to open " << spfile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -15;
    }
    IBIS_BLOCK_GUARD(UnixClose, sdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(sdes, _O_BINARY);
#endif
    off_t ierr = UnixSeek(sdes, ind*spelem, SEEK_SET);
    if (ierr != (off_t)(ind*spelem)) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob(" << ind << ") failed to seek to "
	    << ind*spelem << " in " << spfile << ", seek returned " << ierr;
	return -16;
    }
    ierr = UnixRead(sdes, starts, sizeof(starts));
    if (ierr < (off_t)sizeof(starts)) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob(" << ind << ") failed to read "
	    << sizeof(starts) << " bytes from " << ind*spelem << " in "
	    << spfile << ", read returned " << ierr;
	return -17;
    }

    if (starts[1] <= starts[0]) {
	size = 0;
	return 0;
    }
    uint64_t diff = starts[1]-starts[0];
    if (buf == 0 || size < diff) {
	delete buf;
	buf = new unsigned char[diff];
    }
    if (buf == 0)
	return -10;

    int fdes = UnixOpen(datafile, OPEN_READONLY);
    if (fdes < 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob failed to open " << datafile
	    << " for reading ... "
	    << (errno ? strerror(errno) : "no free stdio stream");
	return -11;
    }
    IBIS_BLOCK_GUARD(UnixClose, fdes);
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    ierr = UnixSeek(fdes, *starts, SEEK_SET);
    if (ierr != *starts) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob(" << ind << ") failed to seek to "
	    << *starts << " in " << datafile << ", seek returned "
	    << ierr;
	return -12;
    }

    ierr = UnixRead(fdes, buf, diff);
    if (ierr < (off_t)diff) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- blob::readBlob(" << ind << ") failed to read "
	    << diff << " byte" << (diff>1?"s":"") << " from " << datafile
	    << ", read returned " << ierr;
	return -13;
    }
    size = diff;
    if (size == diff)
	ierr = 0;
    else
	ierr = -14;
    return ierr;
} // ibis::blob::readBlob

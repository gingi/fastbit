//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2009 the Regents of the University of California
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

////////////////////////////////////////////////////////////////////////
// functions for ibis::dictionary
// copy constructor
ibis::dictionary::dictionary(const ibis::dictionary& dic) :
    svec(), s2i(), newentry(0) {
    // put in the null string
    svec.push_back(ibis::util::strnewdup("<NULL>"));
    //s2i[svec.back()] = 0;

    // if dic has only one entry, we are done
    const uint32_t nelm = dic.svec.size();
    if (nelm <= 1) return;

    // find out the size of the buf to allocate
    uint32_t i, sz = nelm;
    for (i = 1; i < nelm; ++ i)
	sz += strlen(dic.svec[i]);
    char* buf = new char[sz];
    for (i = 1; i < nelm; ++ i) {
	svec[i] = buf;
	for (const char *t = dic.svec[i]; *t != 0; ++ t, ++ buf)
	    *buf = *t;
	*buf = 0;
	++ buf;
	s2i[svec[i]] = i;
    }
    newentry = nelm;
} // copy constructory

void ibis::dictionary::copy(const ibis::dictionary& dic) {
    clear(); // clear the existing content

    // if dic has only one entry, we are done
    const uint32_t nelm = dic.svec.size();
    if (nelm <= 1) return;

    // find out the size of the buf to allocate
    uint32_t i, sz = nelm;
    for (i = 1; i < nelm; ++ i)
	sz += strlen(dic.svec[i]);
    char* buf = new char[sz];
    for (i = 1; i < nelm; ++ i) {
	svec[i] = buf;
	for (const char *t = dic.svec[i]; *t != 0; ++ t, ++ buf)
	    *buf = *t;
	*buf = 0;
	++ buf;
	s2i[svec[i]] = i;
    }
    newentry = nelm;
} // copy constructory

// read the content of a file
void ibis::dictionary::read(const char* name) {
    // open the file to read
    int32_t ierr = 0;
    FILE* fptr = fopen(name, "rb");
    if (fptr == 0) {
	if (ibis::gVerbose > 3) {
	    ibis::util::logMessage
		("dictionary::read", "failed to "
		 "open file %s ... %s\n  The dictionary is "
		 "not modified", name,
		 (errno ? strerror(errno) : "no free stdio stream"));
	}
	return;
    }

    if (svec.size() <= 1) { // an empty dictionary to start with
	if (svec.empty()) // make sure the null element is always there
	    svec.push_back(ibis::util::strnewdup("<NULL>"));

	ierr = fseek(fptr, 0, SEEK_END);
	uint32_t sz = ftell(fptr);
	if (sz == 0) { // an empty dictionary
	    ierr = fclose(fptr);
	    return;
	}

	// allocate enough space to read the whole dictionary in one shot
	char* buf = new char[sz];
	ierr = fseek(fptr, 0, SEEK_SET);
	ierr = fread(buf, 1, sz, fptr);
	(void) fclose(fptr);
	if (static_cast<uint32_t>(ierr) != sz) {
	    ibis::util::logMessage("Error", "dictionary(%s) expected to read "
				   "%lu bytes, but got %ld instead",
				   static_cast<long unsigned>(sz),
				   static_cast<long>(ierr));
	    throw ibis::bad_alloc("failed to read dictionary");
	}

	// strings are delimited by (char)0 -- the standard terminator
	char* bufend = buf + sz;
	char* tmp = buf;
	uint32_t ind = svec.size();
	while (tmp < bufend) {
	    wordList::const_iterator it = s2i.find(tmp);
	    if (it == s2i.end()) { // a new entry
		svec.push_back(tmp);
		s2i[tmp] = ind;
		++ ind;
	    }
	    while (tmp < bufend && *tmp != 0) // find next (char)0
		++ tmp;
	    ++ tmp;	// move tmp to pass the next (char)0
	}
	newentry = ind;
    }
    else { // append new entries to the end of the dictionary
	char buf[MAX_LINE];
	int leftover = 0;
	uint32_t ind = svec.size();
	memset(buf, 0,MAX_LINE);
	while ((ierr = fread(buf+leftover, 1, MAX_LINE-leftover, fptr)) > 0) {
	    const char *bufend = buf+(leftover+ierr);
	    char *str = buf;
	    while (str < bufend && *str == 0) // skip null characters
		++ str;
	    do {
		char *tmp = str; // tmp points to the end of the string
		while (tmp < bufend && *tmp != 0)
		    ++ tmp;
		if (tmp < bufend) { // a properly terminated string
		    wordList::const_iterator it = s2i.find(str);
		    if (it == s2i.end()) { // a new entry
			char * s2 = ibis::util::strnewdup(str);
			svec.push_back(s2);
			s2i[s2] = ind;
			++ ind;
		    }
		    for (str = tmp + 1; str < bufend && *str == 0; ++ str);
		}
		else if (str == buf) { // don't see an end yet,
				       // string too long
		    ibis::util::logMessage
			("dictionary::read",
			 "file \"%s\" constains string longer than %lu "
			 "characters, skipping %lu characters",
			 name, static_cast<long unsigned>(MAX_LINE),
			 static_cast<long unsigned>(MAX_LINE));
		    str = tmp;
		}
		else { // don't see an end yet,
		       // copy to the beginning of buf
		    leftover = tmp - str;
		    for (char *s = buf; str < tmp; ++ str, ++ s)
			*s = *str;
		}
	    } while (str < bufend);
	}
	ierr = fclose(fptr);
    }
} // ibis::dictionary::read

/// Write the content of the dictionary to the named file.
void ibis::dictionary::write(const char* name) const {
    FILE* fptr = fopen(name, "wb");
    if (fptr != 0) {
	for (uint32_t i = 1; i < svec.size(); ++ i) {
	    (void)fwrite(svec[i], 1, strlen(svec[i])+1, fptr);
	}
	(void)fclose(fptr);
	if (ibis::gVerbose > 7)
	    ibis::util::logMessage("dictionary::write",
				   "wrote %lu entr%s to %s",
				   static_cast<long unsigned>(svec.size()),
				   (svec.size()>1?"ies":"y"),
				   name);
    }
    else if (ibis::gVerbose > 3) {
	ibis::util::logMessage("Warning", "dictionary::write(%s) failed to "
			       "open the file ... %s", name,
			       (errno ? strerror(errno) :
				"no free stdio stream"));
    }
} // ibis::dictionary::write

// clear the allocated memory -- [1:newentry) must be free in one operation
void ibis::dictionary::clear() {
    if (svec.size() > 1) { // leave only the first entry
	std::vector<char*>::iterator it = svec.begin();
	++ it; // leave the <NULL> entry alone
	if (newentry > 1) {
	    delete [] *it; // all entries from [1:newentry) are deleted here
	    it += (newentry-1);
	}
	if (svec.size() > newentry) {
	    while (it != svec.end()) {
		delete [] *it;
		++ it;
	    }
	}
	svec.resize(1);
    }
    else if (svec.empty()) { // add the NULL entry
	svec.push_back(ibis::util::strnewdup("<NULL>"));
    }
    s2i.clear();
    //s2i[svec.back()] = 0;
    newentry = 1;
} // ibis::dictioinary::clear

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
    parepareIndex();
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
			 uint32_t nevt) :
    text(tbl, name, ibis::CATEGORY), dic() {
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

ibis::category::~category() {
    unloadIndex();
    std::string dname;
    dataFileName(dname);
    if (! dname.empty()) {
	dname += ".dic";
	if (ibis::util::getFileSize(dname.c_str()) <= 0)
	    dic.write(dname.c_str());
    }
} // ibis::category::~category

ibis::array_t<uint32_t>*
ibis::category::selectUInts(const ibis::bitvector& mask) const {
    indexLock lock(this, "category::selectInts");
    return static_cast<ibis::relic*>(idx)->keys(mask);
} // ibis::category::selectInts

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
	fillIndex();
    }
} // ibis::category::prepareMembers

/// Read the dictionary from the specified directory.  If the incoming
/// argument is nil, the current data directory of the data partition is
/// used.
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
    dic.read(fnm.c_str());
} // ibis::category::readDictionary

/// Build an ibis::relic index using the existing primary data.
/// If the dictionary exists and the size is one, it builds a dummy index.
/// Otherwise, it reads the primary data file to update the dictionary and
/// complete a new ibis::relic index.
void ibis::category::fillIndex(const char *dir) const {
    if (dir == 0 && thePart != 0)
	dir = thePart->currentDataDir();
    if (dir == 0) return;
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
	    return;
	}

	int ret;
	ibis::fileManager::buffer<char> mybuf;
	char *buf = mybuf.address();
	uint32_t nbuf = mybuf.size();
	const bool iscurrent =
	    (strcmp(dir, thePart->currentDataDir()) == 0 &&
	     thePart->getState() != ibis::part::PRETRANSITION_STATE);
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
			<< "ibis::category["
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
		else if (thePart->getState() != ibis::part::PRETRANSITION_STATE
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
	    if (thePart->getState() != ibis::part::PRETRANSITION_STATE)
		ints.resize(thePart->nRows());
#if defined(WRITE_INT_VALUES_FOR_KEYS)
	    data += ".int";
	    ints.write(data.c_str());
#endif
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
	    strcmp(dir, thePart->currentDataDir()) == 0)
	    idx = rlc;
	else
	    delete rlc;
    }

    std::string dicfile = (dir ? dir : thePart->currentDataDir());
    dicfile += FASTBIT_DIRSEP;
    dicfile += m_name;
    dicfile += ".dic";
    dic.write(dicfile.c_str());
} // ibis::category::fillIndex

long ibis::category::search(const char* str, ibis::bitvector& hits) const {
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
	indexLock lock(this, "category::search");
	if (idx != 0) {
	    ibis::qContinuousRange expr(m_name.c_str(),
					ibis::qExpr::OP_EQ, ind);
	    long ierr = idx->evaluate(expr, hits);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- category::search(" << str
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
} // ibis::category::search

long ibis::category::search(const char* str) const {
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
	indexLock lock(this, "category::search");
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
} // ibis::category::search

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

long ibis::category::search(const std::vector<std::string>& strs,
			    ibis::bitvector& hits) const {
    if (strs.empty()) {
	hits.clear();
	return 0;
    }

    if (strs.size() == 1) // the list contains only one value
	return search(strs.back().c_str(), hits);

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
	indexLock lock(this, "category::search");
	if (idx != 0) {
	    ibis::qDiscreteRange expr(m_name.c_str(), inds);
	    long ierr = idx->evaluate(expr, hits);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- category::search on " << strs.size()
		    << " strings failed because idx->evaluate(" << expr
		    << ") failed with error code " << ierr;
		return ierr;
	    }
	}
	else { // index must exist! can not proceed
	    hits.set(0, thePart->nRows());
	    if (ibis::gVerbose >= 0)
		logWarning("category::search", "can not obtain a lock on the "
			   "index or there is no index");
	}
    }
    return hits.cnt();
} // ibis::category::search

long ibis::category::search(const std::vector<std::string>& strs) const {
    long ret = thePart->nRows();
    if (strs.empty()) {
	ret = 0;
    }
    else if (strs.size() == 1) {// the list contains only one value
	ret = search(strs.back().c_str());
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
	    indexLock lock(this, "category::search");
	    if (idx != 0) {
		ibis::qDiscreteRange expr(m_name.c_str(), inds);
		ret = idx->estimate(expr);
	    }
	    else { // index must exist
		ret = 0;
		if (ibis::gVerbose >= 0)
		    logWarning("category::search", "can not obtain a lock on "
			       "the index or there is no index");
	    }
	}
    }
    return ret;
} // ibis::category::search

// read the string values (terminated with NULL) from the directory "dt" and
// extend the set of bitvectors representing the strings
long ibis::category::append(const char* dt, const char* df,
			    const uint32_t nold, const uint32_t nnew,
			    const uint32_t nbuf, char* buf) {
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
	    int fdest = UnixOpen(dest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
	    if (fdest >= 0) { // copy raw bytes without any sanity check
		while ((ierr = UnixRead(fptr, buf, nbuf))) {
		    ret = UnixWrite(fdest, buf, ierr);
		    if (ret != ierr && ibis::gVerbose > 2)
			logMessage("append", "expected to write %ld bytes "
				   "to \"%s\" by only wrote %ld", ierr,
				   dest.c_str(), ret);
		}
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
		(void) UnixFlush(fdest); // write to disk
#endif
		ierr = UnixClose(fdest);
	    }
	    else {
		logMessage("append", "unable to open \"%s\" to appending",
			   dest.c_str());
	    }
	    ierr = UnixClose(fptr);
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
		ierr = UnixSeek(fptr, 0, SEEK_SET);
		if (ierr < 0) return -2;
		while ((ierr = UnixRead(fptr, buf, nbuf)) > 0) {
		    ret = UnixWrite(fdest, buf, ierr);
		    if (ret != ierr && ibis::gVerbose > 2)
			logMessage("append", "expected to write %ld bytes "
				   "to \"%s\" by only wrote %ld", ierr,
				   dest.c_str(), ret);
		}
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
		(void) UnixFlush(fdest); // write to disk
#endif
		(void) UnixClose(fdest);
	    }
	    else {
		logMessage("append", "unable to open \"%s\" to appending",
			   dest.c_str());
	    }
	    (void) UnixClose(fptr);
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
#if defined(DEBUG)
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
			ind.print(lg.buffer());
		    }
		}
		else if (ibis::gVerbose > 0) {
		    logWarning("append", "failed to extend the index (%ld)",
			       ierr);
		    if (ind.getNRows() > 0)
			purgeIndexFile(dt);
		    fillIndex(dt);
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
		fillIndex(dt);
	    }
	    delete binp;
	}
	else {
	    if (ibis::gVerbose > 2)
		logMessage("append", "failed to generate the index for "
			   "data in %s, start scanning all records in %s",
			   df, dt);
	    fillIndex(dt);
	}
    }
    catch (...) {
	if (ibis::gVerbose > 2)
	    logMessage("append", "absorbed an exception while extending "
		       "the index, start scanning all records in %s", dt);
	fillIndex(dt);
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
    fprintf(file, "minimum = %lu\n", static_cast<long unsigned>(lower));
    fprintf(file, "maximum = %lu\n", static_cast<long unsigned>(upper));
    if (! m_bins.empty())
	fprintf(file, "index=%s\n", m_bins.c_str());
    fputs("End Column\n", file);
} // ibis::category::write

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

////////////////////////////////////////////////////////////////////////
// functions for ibis::text
ibis::text::text(const part* tbl, FILE* file) : column(tbl, file) {
#ifdef FASTBIT_EAGER_INIT_TEXT
    if (thePart != 0 && thePart->nRows() > 0U)
	startPositions(thePart->currentDataDir(), 0, 0);
#endif
}

/// Construct a text object for a data partition with the given name.
ibis::text::text(const part* tbl, const char* name, ibis::TYPE_T t) :
    column(tbl, t, name) {
#ifdef FASTBIT_EAGER_INIT_TEXT
    if (thePart != 0 && thePart->nRows() > 0U)
	startPositions(thePart->currentDataDir(), 0, 0);
#endif
}

/// Copy constructor.  Copy from a column with TEXT type.
ibis::text::text(const ibis::column& col) : ibis::column(col) {
    if (m_type != ibis::TEXT && m_type != ibis::CATEGORY) {
	throw "Must be either TEXT or CATEGORY";
    }
#ifdef FASTBIT_EAGER_INIT_TEXT
    if (thePart != 0 && thePart->nRows() > 0U)
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
    if (dir == 0) // default to the current data directory
	dir = (thePart != 0 ? thePart->currentDataDir() : 0);
    if (dir == 0) return;

    std::string dfile = dir;
    dfile += FASTBIT_DIRSEP;
    dfile += m_name;
    std::string spfile = dfile;
    spfile += ".sp";
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
	    if (ibis::gVerbose >= 0)
		logWarning("startPositions", "unable to read the last "
			   "integer in file \"%s\"", spfile.c_str());
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
		    if (1 > fwrite(&last, sizeof(last), 1, fsp))
			LOGGER(ibis::gVerbose >= 0)
			    << evt << " -- unable to write integer value "
			    << last << " to file \"" << spfile << "\"";
		    last = pos+1;
		    ++ nnew;
		    LOGGER(ibis::gVerbose > 2 && nnew % 1000000 == 0)
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
	const size_t nsps = sps.size();
	size_t jsps = 0;
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
		    LOGGER(ibis::gVerbose > 2 && nnew % 1000000 == 0)
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
			  (i+ntmp<=missed?ntmp:missed-i), fsp);
	}
    }
    if (nnew > 0) {
	pos = ftell(fdata);// current size of the data file
	(void) fwrite(&pos, sizeof(pos), 1, fsp);
    }
    (void) fclose(fdata);
    (void) fclose(fsp);

    LOGGER(ibis::gVerbose > 2)
	<< evt << " located the starting positions of " << nnew
	<< " new string" << (nnew > 1 ? "s" : "") << ", file " << spfile
	<< " now has " << (nnew+nold+1) << " 64-bit integers (total "
	<< sizeof(int64_t)*(nnew+nold+1) << " bytes)";

    if (isActiveData && nold + nnew > thePart->nRows()) {
	fsp = fopen(spfile.c_str(), "rb");
	ierr = fseek(fsp, thePart->nRows()*sizeof(int64_t), SEEK_SET);
	ierr = fread(&pos, sizeof(int64_t), 1, fsp);
	ierr = fclose(fsp);
	truncate(spfile.c_str(), (1+thePart->nRows())*sizeof(int64_t));
	truncate(dfile.c_str(), pos);
	LOGGER(ibis::gVerbose >= 0)
	    << evt << " truncated files " << dfile << " and " << spfile
	    << " to contain only " << thePart->nRows() << " record"
	    << (thePart->nRows() > 1 ? "s" : "");
    }
} // ibis::text::startPositions

/// Append the data file stored in directory @c df to the corresponding
/// data file in directory @c dt.  Use the buffer @c buf to copy data in
/// large chuncks.
///@note  No error checking is performed.
///@note  Does not check for missing entries.  May cause records to be
/// misaligned.
long ibis::text::append(const char* dt, const char* df,
			const uint32_t nold, const uint32_t nnew,
			const uint32_t nbuf, char* buf) {
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
    int fdest = UnixOpen(dest.c_str(), OPEN_APPENDONLY, OPEN_FILEMODE);
    if (fdest < 0) {
	UnixClose(fsrc);
	if (ibis::gVerbose >= 0) {
	    logWarning("append", "unableto open file \"%s\" for appending",
		       dest.c_str());
	}
	return -2;
    }

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
#if _POSIX_FSYNC+0 > 0 && defined(FASTBIT_SYNC_WRITE)
    (void) UnixFlush(fdest); // write to disk
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

/// Given a string literal, return a bitvector that marks the strings that
/// matches it.
long ibis::text::search(const char* str, ibis::bitvector& hits) const {
    hits.clear(); // clear the existing content of hits

    std::string data = thePart->currentDataDir();
    data += FASTBIT_DIRSEP;
    data += m_name;
    FILE *fdata = fopen(data.c_str(), "rb");
    if (fdata == 0) {
	if (ibis::gVerbose >= 0) {
	    logWarning("search", "can not open data file \"%s\" for reading",
		       data.c_str());
	}
	return -1L;
    }

    ibis::fileManager::buffer<char> mybuf;
    char *buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) return -2L;

    std::string sp = data;
    sp += ".sp";
    FILE *fsp = fopen(sp.c_str(), "rb");
    if (fsp == 0) { // try again
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    if (ibis::gVerbose >= 0)
		logWarning("search", "can not create or open file \"%s\"",
			   sp.c_str());
	    fclose(fdata);
	    return -3L;
	}
    }

    ibis::fileManager::buffer<int64_t> spbuf;
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
	    if (ibis::gVerbose >= 0)
		logWarning("search", "can not create, open or read file "
			   "\"%s\"", sp.c_str());
	    fclose(fdata);
	    return -4L;
	}
    }
    if (spbuf.size() > 1 && (str == 0 || *str == 0)) {
	// match empty strings, with a buffer for starting positions
	size_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- text[" << thePart->name() << '.'
		<< m_name << "]::search -- failed to read file " << sp;
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
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- text[" << thePart->name() << '.' << m_name
		    << "]::search -- string # " << irow << " in file \""
		    << data << "\" is expected to be " << (next-begin)
		    << "-byte long, but " << (jbuf<nbuf ? "can only read " :
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
		    << "text[" << thePart->name() << "." << m_name
		    << "]::search -- processed " << irow
		    << " strings from file " << data;

		if (moresp) {
		    if (jsp >= nsp) {
			ierr = fread(spbuf.address(), sizeof(int64_t),
				     spbuf.size(), fsp);
			if (ierr <= 0) {
			    LOGGER(ibis::gVerbose >= 0)
				<< "Warning -- text[" << thePart->name() << '.'
				<< m_name << "]::search -- failed to read file "
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
	std::string pat = str; // convert the string to be search to lower case
	for (size_t i = 0; i < pat.length(); ++ i)
	    pat[i] = tolower(pat[i]);
	const size_t slen = pat.length() + 1;
	size_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- text[" << thePart->name() << '.'
		<< m_name << "]::search -- failed to read file " << sp;
	    fclose(fsp);
	    fclose(fdata);
	    return -6L;
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
		    << "Warning -- text[" << thePart->name() << '.' << m_name
		    << "]::search -- string # " << irow << " in file \""
		    << data << "\" is expected to be " << (next-begin)
		    << "-byte long, but " << (jbuf<nbuf ? "can only read " :
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
		    if (j+3 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]) &&
			    (buf[j-begin+2] == pat[j-curr+2]);
		    }
		    else if (j+2 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]);
		    }
		    else if (j+1 == next) {
			match = (buf[j-begin] == pat[j-curr]);
		    }
		}
		if (match)
		    hits.setBit(irow, 1);
#if _DEBUG+0 > 1 || DEBUG+0 > 1
		if (ibis::gVerbose > 5) {
		    ibis::util::logger lg(4);
		    lg.buffer() << "DEBUG -- text[" << thePart->name() << "."
				<< m_name << "]::search processing string "
				<< irow << " \'";
		    for (long i = curr; i < next-1; ++ i)
			lg.buffer() << buf[i-begin];
		    lg.buffer() << "\'";
		    if (match)
			lg.buffer() << " == ";
		    else
			lg.buffer() << " != ";
		    lg.buffer() << pat;
		}
#endif
		++ irow;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << "text[" << thePart->name() << "." << m_name
		    << "]::search -- processed " << irow
		    << " strings from file " << data;

		curr = next;
		if (moresp) {
		    if (jsp >= nsp) {
			if (feof(fsp) == 0) {
			    ierr = fread(spbuf.address(), sizeof(int64_t),
					 spbuf.size(), fsp);
			    if (ierr <= 0) {
				LOGGER(ibis::gVerbose >= 0)
				    << "Warning -- text[" << thePart->name()
				    << '.' << m_name
				    << "]::search -- failed to read file "
				    << sp;
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
	fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- text[" << thePart->name() << '.' << m_name
		    << "]::search -- string # " << irow << " in file \""
		    << data << "\" is expected to be " << (next-begin)
		    << "-byte long, but " << (jbuf<nbuf ? "can only read " :
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
		    << "text[" << thePart->name() << "." << m_name
		    << "]::search -- processed " << irow
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
	for (size_t i = 0; i < pat.length(); ++ i)
	    pat[i] = tolower(pat[i]);
	const size_t slen = pat.length() + 1;
	fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    for (long j = 0; j < jbuf; ++ j) // convert to lower case
		buf[j] = tolower(buf[j]);

	    bool moresp = true;
	    if (next > begin+jbuf) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- text[" << thePart->name() << '.' << m_name
		    << "]::search -- string # " << irow << " in file \""
		    << data << "\" is expected to be " << (next-begin)
		    << "-byte long, but " << (jbuf<nbuf ? "can only read " :
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
		    if (j+3 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]) &&
			    (buf[j-begin+2] == pat[j-curr+2]);
		    }
		    else if (j+2 == next) {
			match = (buf[j-begin] == pat[j-curr]) &&
			    (buf[j-begin+1] == pat[j-curr+1]);
		    }
		    else if (j+1 == next) {
			match = (buf[j-begin] == pat[j-curr]);
		    }
		}
		if (match)
		    hits.setBit(irow, 1);
		++ irow;
		LOGGER(ibis::gVerbose > 2 && irow % 1000000 == 0)
		    << "text[" << thePart->name() << "." << m_name
		    << "]::search -- processed " << irow
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
	if (irow != thePart->nRows() && ibis::gVerbose >= 0)
	    logWarning("search", "data file \"%s\" contains %lu strings, "
		       "but expected %lu", data.c_str(),
		       static_cast<long unsigned>(irow),
		       static_cast<long unsigned>(thePart->nRows()));
	if (irow < thePart->nRows())
	    startPositions(thePart->currentDataDir(), buf, nbuf);
	hits.adjustSize(0, thePart->nRows());
    }
    return hits.cnt();
} // ibis::text::search

/// Given a group of string literals, return a bitvector that matches
/// anyone of the input strings.
long ibis::text::search(const std::vector<std::string>& strs,
			ibis::bitvector& hits) const {
    if (strs.empty()) {
	hits.set(0, thePart->nRows());
	return 0;
    }

    if (strs.size() == 1) // the list contains only one value
	return search(strs[0].c_str(), hits);

    hits.clear();
    std::string data = thePart->currentDataDir();
    data += FASTBIT_DIRSEP;
    data += m_name;
    FILE *fdata = fopen(data.c_str(), "rb");
    if (fdata == 0) {
	if (ibis::gVerbose >= 0)
	    logWarning("search", "can not open data file \"%s\" for reading",
		       data.c_str());
	return -1L;
    }

    ibis::fileManager::buffer<char> mybuf;
    char *buf = mybuf.address();
    uint32_t nbuf = mybuf.size();
    if (buf == 0 || nbuf == 0) return -2L;

    std::string sp = data;
    sp += ".sp";
    FILE *fsp = fopen(sp.c_str(), "rb");
    if (fsp == 0) { // try again
	startPositions(thePart->currentDataDir(), buf, nbuf);
	fsp = fopen(sp.c_str(), "rb");
	if (fsp == 0) { // really won't work out
	    if (ibis::gVerbose >= 0)
		logWarning("search", "can not create or open file \"%s\"",
			   sp.c_str());
	    fclose(fdata);
	    return -3L;
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
	    logWarning("search", "can not create, open or read file \"%s\"",
		       sp.c_str());
	    fclose(fdata);
	    return -4L;
	}
    }

    ibis::fileManager::buffer<int64_t> spbuf;
    if (spbuf.size() > 1) { // try to use the spbuf for starting positions
	size_t jsp, nsp;
	ierr = fread(spbuf.address(), sizeof(int64_t), spbuf.size(), fsp);
	if (ierr <= 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- text[" << thePart->name() << '.'
		<< m_name << "]::search -- failed to read file " << sp;
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
		logWarning("search", "string %lu in file \"%s\" is longer "
			   "than internal buffer (size %ld), skipping %ld "
			   "bytes", static_cast<long unsigned>(irow),
			   data.c_str(), jbuf, jbuf);
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
				    << "Warning -- text[" << thePart->name()
				    << '.' << m_name
				    << "]::search -- failed to read file "
				    << sp;
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
	fread(&next, sizeof(next), 1, fsp);
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0) {
	    bool moresp = true;
	    if (next > begin+jbuf) {
		logWarning("search", "string %lu in file \"%s\" is longer "
			   "than internal buffer (size %ld), skipping %ld "
			   "bytes", static_cast<long unsigned>(irow),
			   data.c_str(), jbuf, jbuf);
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
	logWarning("search", "data file \"%s\" contains %lu strings, but "
		   "expected %lu", data.c_str(),
		   static_cast<long unsigned>(hits.size()),
		   static_cast<long unsigned>(thePart->nRows()));
	if (hits.size() < thePart->nRows())
	    startPositions(thePart->currentDataDir(), buf, nbuf);
	hits.adjustSize(0, thePart->nRows());
    }
    return hits.cnt();
} // ibis::text::search

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
    if (spsize < 0 || (size_t)spsize != (mask.size()+1)*sizeof(int64_t))
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
    if (spsize < 0 || (size_t)spsize != (mask.size()+1)*sizeof(int64_t))
	startPositions(thePart->currentDataDir(), (char*)0, 0U);

    const array_t<int64_t>
	sp(fname.c_str(), static_cast<const off_t>(0),
	   static_cast<const off_t>((mask.size()+1)*sizeof(int64_t)));
    fname.erase(fname.size()-3); // remove .sp
    int fdata = UnixOpen(fname.c_str(), OPEN_READONLY);
    if (fdata < 0) {
	logWarning("selectStrings", "failed to open data file \"%s\"",
		   fname.c_str());
	delete res;
	return 0;
    }

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
    return 0;
} // ibis::text::readString

/// It goes through a two-stage process by reading from two files, first
/// from the .sp file to read the position of the string in the second file
/// and the second file contains the actual string values (with nil
/// terminators).  This can be quite slow!
void ibis::text::readString(uint32_t i, std::string &ret) const {
    ret.clear();
    if (i >= thePart->nRows()) return;
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
    ierr = UnixSeek(des, i*sizeof(int64_t), SEEK_SET);
    ierr = UnixRead(des, &positions, sizeof(positions));
    ierr = UnixClose(des);
    ibis::fileManager::instance().recordPages
	(i*sizeof(int64_t), i*sizeof(int64_t)+sizeof(positions));

    fnm.erase(fnm.size()-3); // remove ".sp"
    int datafile = UnixOpen(fnm.c_str(), OPEN_READONLY);
    if (datafile < 0) {
	logWarning("readString", "failed to open file \"%s\"",
		   fnm.c_str());
	return;
    }
    ierr = UnixSeek(datafile, positions[0], SEEK_SET);
    char buf[1025];
    buf[1024] = 0;
    for (long j = positions[0]; j < positions[1]; j += 1024) {
	long len = positions[1] - j;
	if (len > 1024)
	    len = 1024;
	ierr = UnixRead(datafile, buf, len);
	ret += buf;
    }
    ierr = UnixClose(datafile);
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
	    << "Warning -- ibis::text["
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

    long next = 0;
    bool found = false;
    if (str == 0 || *str == 0) { // only match empty strings
	while ((jbuf = fread(buf, 1, nbuf, fdata)) > 0 && ! found) {
	    bool moresp = true;
	    fread(&next, sizeof(next), 1, fsp);
	    if (next > begin+jbuf) {
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
	    fread(&next, sizeof(next), 1, fsp);
	    if (next > begin+jbuf) {
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

// check indexSpec first for docIDName=xxx for the name of the ID column,
// then check global parameter <table-name>.<column-name>.docIDName.
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
    }
    if (idcol == 0) {
	std::string idcpar = partition()->name();
	idcpar += '.';
	idcpar += m_name;
	idcpar += ".docIDName";
	const char* idname = ibis::gParameters()[idcpar.c_str()];
	if (idname)
	    idcol = partition()->getColumn(idname);
    }
    return idcol;
} // ibis::text::IDColumnForKeywordIndex

long ibis::text::keywordSearch(const char* str, ibis::bitvector& hits) const {
    long ierr = 0;
    try {
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
    int sffile = UnixOpen(spfrom, OPEN_READONLY);
    if (sffile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << spfrom
	    << " for reading";
	UnixClose(rffile);
	return -12;
    }
    int rtfile = UnixOpen(to, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (rtfile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << to
	    << " for writing";
	UnixClose(sffile);
	UnixClose(rffile);
	return -13;
    }
    int stfile = UnixOpen(spto, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (rtfile < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt << " failed to open file " << spto
	    << " for writing";
	UnixClose(rtfile);
	UnixClose(sffile);
	UnixClose(rffile);
	return -14;
    }

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
			(void) UnixClose(stfile);
			(void) UnixClose(rtfile);
			(void) UnixClose(sffile);
			(void) UnixClose(rffile);
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
				(void) UnixClose(stfile);
				(void) UnixClose(rtfile);
				(void) UnixClose(sffile);
				(void) UnixClose(rffile);
				return -16;
			    }
			}
			else {
			    LOGGER(ibis::gVerbose >= 0)
				<< "Warning -- " << evt << " failed to read "
				<< bytes << " byte" << (bytes>1?"s":"")
				<< " from " << from << ", "
				<< (errno ? strerror(errno) : "??");
			    (void) UnixClose(stfile);
			    (void) UnixClose(rtfile);
			    (void) UnixClose(sffile);
			    (void) UnixClose(rffile);
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
		for (int jtmp = irow; jtmp < idx[1]; ++ jtmp) {
		    ierr = UnixWrite(stfile, &pos, 8);
		    if (ierr != 8) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- " << evt
			    << " failed to write the value " << pos
			    << " to " << spto << ", unable to continue";
			(void) UnixClose(stfile);
			(void) UnixClose(rtfile);
			(void) UnixClose(sffile);
			(void) UnixClose(rffile);
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
			(void) UnixClose(stfile);
			(void) UnixClose(rtfile);
			(void) UnixClose(sffile);
			(void) UnixClose(rffile);
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
		    (void) UnixClose(stfile);
		    (void) UnixClose(rtfile);
		    (void) UnixClose(sffile);
		    (void) UnixClose(rffile);
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
				    (void) UnixClose(stfile);
				    (void) UnixClose(rtfile);
				    (void) UnixClose(sffile);
				    (void) UnixClose(rffile);
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
			(void) UnixClose(stfile);
			(void) UnixClose(rtfile);
			(void) UnixClose(sffile);
			(void) UnixClose(rffile);
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
    (void) UnixClose(stfile);
    (void) UnixClose(rtfile);
    (void) UnixClose(sffile);
    (void) UnixClose(rffile);
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

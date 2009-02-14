// $Id$
// Copyright 2000-2009 the Regents of the University of California
// Author: John Wu <John.Wu at acm.org>
//      Lawrence Berkeley National Laboratory
//
// implementation of the utility functions defined in util.h (namespace
// ibis::util)
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#include <direct.h>	// _rmdir
#endif
#include "util.h"
#include "horometer.h"
#include "resource.h"
#include <stdarg.h>	// vsprintf
#if defined(unix) || defined(__HOS_AIX__) || defined(__APPLE__) || defined(_XOPEN_SOURCE) || defined(_POSIX_C_SOURCE)
#include <pwd.h>	// getpwuid
#include <unistd.h>	// getuid, rmdir
#include <sys/stat.h>	// stat
#include <dirent.h>	// opendir, readdir
#endif

#include <limits>	// std::numeric_limits

// global variables
#if defined(DEBUG)
int ibis::gVerbose = 10;
#else
int ibis::gVerbose = 0;
#endif
// initialize the static member of ibis::util
pthread_mutex_t ibis::util::envLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ibis::util::ioLock::mutex = PTHREAD_MUTEX_INITIALIZER;
// 64 printable ASCII characters that are not special to most of the
// command interpreters, same set as RFC 1421, but not in the same order
const char* ibis::util::charTable =
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+=";
// maps back from ASCII to positions of the characters in the abobe list
const short unsigned ibis::util::charIndex[] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 64,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 64, 64, 64, 63, 64, 64,
    64, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 64, 64, 64, 64, 64,
    64, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64,
};

// file scope variables
static std::string ibis_util_logfilename("");
static FILE* ibis_util_logfilepointer = 0;

#if defined(_MSC_VER) && defined(_WIN32)
// Intended to serialize all time conversion calls on windows.  Only used
// in the file.
static pthread_mutex_t ibis_util_timeLock = PTHREAD_MUTEX_INITIALIZER;
#endif

//
// Return a null-terminated string from the beginning of input string str.
// The first apparence of any character from characters in tok_chars is
// turned into null.  The incoming argument is modified to point to the
// first character that is not in tok_chrs.  If no character in tok_chrs is
// found, str is returned and the first argument is changed to null.
const char* ibis::util::getToken(char*& str, const char* tok_chrs) {
    const char* token = static_cast<const char*>(0);
    if (!str || !*str) return token;
    token = str;
    char* pc = strpbrk(str, tok_chrs);
    if (pc > str) {
	str = pc + strspn(pc, tok_chrs);
	*pc = static_cast<char>(0);
    }
    else {
	str = static_cast<char*>(0);
    }
    return token;
}

//
// Recursivly create directory "dir".
int ibis::util::makeDir(const char* dir) {
    Stat_T st;
    if (dir == 0 || *dir == 0) return -1;
    if (UnixStat(dir, &st) == 0) return 0; // directory exists already

    // make a copy of the directory name so we can turn directory
    // separators into null.
    char *buf = ibis::util::strnewdup(dir);
    const char *cdir = buf;
    if (*buf == DIRSEP)
	cdir = buf + 1;
    else if (buf[1] == ':') // not the same as a typical local directory
	cdir = buf + 2;
    while (*cdir == DIRSEP) ++cdir; // look beyond the leading DIRSEP

    while (cdir != 0 && *cdir != 0) {
	char* tmp = const_cast<char*>(strchr(cdir, DIRSEP));
	if (tmp > cdir)
	    *tmp = 0; // change DIRSEP to null
	if (UnixStat(buf, &st) != 0) {
	    int pmode = OPEN_FILEMODE;
#if defined(S_IXUSR)
	    pmode = pmode | S_IXUSR;
#endif
#if defined(S_IRWXG) && defined(S_IRWXO)
	    pmode = pmode | S_IRWXG | S_IRWXO;
#elif defined(S_IXGRP) && defined(S_IXOTH)
	    pmode = pmode | S_IXGRP | S_IXOTH;
#endif
	    if (mkdir(buf, pmode) == -1 && errno != EEXIST) {
		ibis::util::logMessage("Warning", "makeDir failed to "
				       "create directory \"%s\"", buf);
		delete [] buf;
		return -2;
	    }
	}
	if (tmp > cdir) {
	    *tmp = DIRSEP; // change null back to DIRSEP
	    cdir = tmp + 1;
	    while (*cdir == DIRSEP) ++cdir; // skip consecutive DIRSEP
	}
	else {
	    cdir = static_cast<const char*>(0);
	}
    }
    delete [] buf;
    return 0;
} // ibis::util::makeDir

/// Extract a string from the given buf, remove leading and trailing spaces
/// and surrounding quotes.  Returns a copy of the string allocated with
/// the @c new operator.
char* ibis::util::getString(const char* buf) {
    char* s2 = 0;
    if (buf == 0)
	return s2;
    if (buf[0] == 0)
	return s2;

    // skip leading space
    const char* s1 = buf;
    while (*s1 && isspace(*s1))
	++ s1;
    if (*s1 == 0)
	return s2;

    if (*s1 == '\'') { // quoted by a single quote
	const char* tmp = 0;
	++ s1;
	do { // skip escaped quotation
	    tmp = strchr(s1, '\'');
	} while (tmp > s1 && tmp[-1] == '\\');
	if (tmp > s1) { // copy till the matching quote
	    const uint32_t len = tmp - s1;
	    s2 = new char[len+1];
	    strncpy(s2, s1, len);
	    s2[len] = 0;
	}
	else if (*s1) { // no matching quote, copy all characters
	    s2 = ibis::util::strnewdup(s1);
	}
    }
    else if (*s1 == '"') { // quoted by a double quote
	const char* tmp = 0;
	++ s1;
	do { // skip escaped quotation
	    tmp = strchr(s1, '"');
	} while (tmp > s1 && tmp[-1] == '\\');
	if (tmp > s1) { // copy till the matching quote
	    const uint32_t len = tmp - s1;
	    s2 = new char[len+1];
	    strncpy(s2, s1, len);
	    s2[len] = 0;
	}
	else if (*s1) { // no matching quote, copy all characters
	    s2 = ibis::util::strnewdup(s1);
	}
    }
    else { // not quoted, copy all characters
	s2 = ibis::util::strnewdup(s1);

	// remove retailing blanks
	char *tmp = s2 + strlen(s2) - 1;
	while (tmp>s2 && isspace(*tmp))
	    -- tmp;
	++ tmp;
	*tmp = static_cast<char>(0);
    }
    return s2;
} // ibis::util::getString

/// Copy the next string to the output variable str.  Leading blank spaces
/// are skipped.  In addition to blank space, delimiters supplied by the
/// third argument will also be skipped.  The content of str will be empty
/// if buf is nil or an empty string.  If the string is quoted, only spaces
/// before the quote is skipped, and the content of the string will be
/// everything after the first quote to the last character before the
/// matching quote or end of buffer.
void ibis::util::getString(std::string& str, const char *&buf,
			   const char *delim) {
    str.erase(); // erase the existing content
    if (buf == 0 || *buf == 0) return;

    while (*buf && isspace(*buf)) ++ buf; // skip leading space
    if (*buf == '\'') { // single quoted string
	++ buf; // skip the openning quote
	while (*buf) {
	    if (*buf != '\'')
		str += *buf;
	    else if (str.size() > 0 && str[str.size()-1] == '\\')
		str[str.size()-1] = '\'';
	    else {
		++ buf;
		return;
	    }
	    ++ buf;
	} // while (*buf)
    }
    else if (*buf == '"') { // double quoted string
	++ buf; // skip the openning quote
	while (*buf) {
	    if (*buf != '"')
		str += *buf;
	    else if (str.size() > 0 && str[str.size()-1] == '\\')
		str[str.size()-1] = '"';
	    else {
		++ buf;
		return;
	    }
	    ++ buf;
	} // while (*buf)
    }
    else { // space separated string
	if (delim == 0 || *delim == 0) {
	    while (*buf) {
		if (!isspace(*buf))
		    str += *buf;
		else if (str[str.size()-1] == '\\')
		    str[str.size()-1] = *buf;
		else
		    return;
		++ buf;
	    } // while (*buf)
	}
	else if (delim[1] == 0) {
	    while (*buf) {
		if (!isspace(*buf) && *delim != *buf)
		    str += *buf;
		else if (str[str.size()-1] == '\\')
		    str[str.size()-1] = *buf;
		else
		    return;
		++ buf;
	    } // while (*buf)
	}
	else if (delim[2] == 0) {
	    while (*buf) {
		if (!isspace(*buf) && *delim != *buf && delim[1] != *buf)
		    str += *buf;
		else if (str[str.size()-1] == '\\')
		    str[str.size()-1] = *buf;
		else
		    return;
		++ buf;
	    } // while (*buf)
	}
	else {
	    while (*buf) {
		if (!isspace(*buf) && (delim == 0 || 0 == strchr(delim, *buf)))
		    str += *buf;
		else if (str[str.size()-1] == '\\')
		    str[str.size()-1] = *buf;
		else
		    return;
		++ buf;
	    } // while (*buf)
	}
    }
} // ibis::util::getString

/// Attempt to convert the incoming string into an integer.
/// Return zero (0) if all characters in @c str are decimal integers and
/// the content does not overflow.
int ibis::util::readInt(int64_t& val, const char *&str, const char* del) {
    int64_t tmp = 0;
    val = 0;
    if (str == 0 || *str == 0) return -1;
    for (; isspace(*str); ++ str); // skip leading space

    const bool neg = (*str == '-');
    if (*str == '-' || *str == '+') ++ str;
    while (*str != 0 && isdigit(*str) != 0) {
	tmp = 10 * val + (*str - '0');
	if (tmp > val) {
	    val = tmp;
	}
	else if (val > 0) {
	    return -2; // overflow
	}
	++ str;
    }
    if (neg) val = -val;
    //for (; isspace(*str); ++ str);
    if (*str == 0 || isspace(*str) || strchr(del, *str) != 0) return 0;
    else return 1;
} // ibis::util::readInt

/// Attempt to convert the incoming string into a double.
int ibis::util::readDouble(double& val, const char *&str, const char* del) {
    val = 0;
    if (str == 0 || *str == 0) return -1;
    for (; isspace(*str); ++ str); // skip leading space

    double tmp;
    const char *s0 = str;
    const bool neg = (*str == '-');
    const unsigned width =  16;
    if (*str == '-' || *str == '+') ++ str;
    // limit the number of digits in the integer portion to 16
    while (*str != 0 && str <= s0+width && isdigit(*str) != 0) {
	val = 10 * val + static_cast<double>(*str - '0');
	++ str;
    }

    if (*str == '.') { // values after the decimal point
	tmp = 0.1;
	// constrain the effective accuracy ??? (str-s0) < width+1 &&
	for (++ str; isdigit(*str); ++ str) {
	    val += tmp * static_cast<double>(*str - '0');
	    tmp *= 0.1;
	}
    }

    if (*str == 'e' || *str == 'E') {
	++ str;
	int ierr;
	int64_t ex;
	ierr = ibis::util::readInt(ex, str, del);
	if (ierr == 0) {
	    val *= pow(1e1, static_cast<double>(ex));
	}
	else {
	    return ierr;
	}
    }

    //for (; isspace(*str); ++ str);
    if (*str == 0 || isspace(*str) || strchr(del, *str) != 0) {
	if (neg) val = - val;
	return 0;
    }
    else { // incorrect format
	return 2;
    }
} // ibis::util::readDouble

// return the size of file in bytes, file that does not exist has size zero
off_t ibis::util::getFileSize(const char* name) {
    Stat_T buf;
    if (name == 0) {
	return 0;
    }
    else if (*name == 0) {
	return 0;
    }
    else if (UnixStat(name, &buf) == 0) {
	if ((buf.st_mode & S_IFREG) == S_IFREG)
	    return buf.st_size;
	else
	    return 0;
    }
    else {
	if (ibis::gVerbose > 11 || errno != ENOENT)
	    ibis::util::logMessage("Warning", "getFileSize(%s) failed ... %s",
				   name, strerror(errno));
	return 0;
    }
}

// copy file named "from" to a file named "to" -- it overwrite the content of
// "to"
int ibis::util::copy(const char* to, const char* from) {
    Stat_T tmp;
    if (UnixStat(from, &tmp) != 0) return -1; // file does not exist
    if ((tmp.st_mode&S_IFDIR) == S_IFDIR // directory
#ifdef S_IFSOCK
	|| (tmp.st_mode&S_IFSOCK) == S_IFSOCK // socket
#endif
	) return -4;

    int fdes = UnixOpen(from, OPEN_READONLY);
    if (fdes < 0) {
	if (errno != ENOENT || ibis::gVerbose > 10)
	    ibis::util::logMessage
		("Warning", "ibis::util::copy(%s, %s) failed "
		 "to open %s ... %s", to, from, from,
		 (errno ? strerror(errno) : "no free stdio stream"));
	return -1;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(fdes, _O_BINARY);
#endif

    int tdes = UnixOpen(to, OPEN_WRITEONLY, OPEN_FILEMODE);
    if (tdes < 0) {
	ibis::util::logMessage
	    ("Warning", "ibis::util::copy(%s, %s) failed "
	     "to open %s ... %s", to, from, to,
	     (errno ? strerror(errno) : "no free stdio stream"));
	UnixClose(fdes);
	return -2;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(tdes, _O_BINARY);
#endif

    uint32_t nbuf = 16777216; // 16 MB
    char* buf = 0;
    try {buf = new char[nbuf];} // try hard to allocate some space for buf
    catch (const std::bad_alloc&) {
	nbuf >>= 1; // reduce the size by half and try again
	try {buf = new char[nbuf];}
	catch (const std::bad_alloc&) {
	    nbuf >>= 2; // reduce the size to a quarter and try again
	    buf = new char[nbuf];
	}
    }

    uint32_t i, j;
    if (buf) { // got a sizeable buffer to use
	while ((i = UnixRead(fdes, buf, nbuf))) {
	    j = UnixWrite(tdes, buf, i);
	    if (i != j) {
		ibis::util::logMessage("Warning", "ibis::util::copy(%s, %s) "
				       "failed to write %lu bytes, only %lu "
				       "bytes are written", to, from,
				       static_cast<long unsigned>(i),
				       static_cast<long unsigned>(j));
	    }
	}
	delete [] buf;
    }
    else {
	// use a static buffer of 256 bytes
	char sbuf[256];
	nbuf = 256;
	while ((i = UnixRead(fdes, sbuf, nbuf))) {
	    j = UnixWrite(tdes, sbuf, i);
	    if (i != j) {
		ibis::util::logMessage("Warning", "ibis::util::copy(%s, %s) "
				       "failed to write %lu bytes, only %lu "
				       "bytes are written", to, from,
				       static_cast<long unsigned>(i),
				       static_cast<long unsigned>(j));
	    }
	}
    }	

    Stat_T st;
    UnixFStat(tdes, &st);
//  ibis::fileManager::instance().recordPages(0, st.st_size); // once for read
//  ibis::fileManager::instance().recordPages(0, st.st_size); // once for write
    i = UnixClose(fdes);
    j = UnixClose(tdes);
    if (i == 0 && j == 0)
	return 0;
    else
	return -3;
} // ibis::util::copy

// remove the files in the named direcotry, if the directory is then become
// empty remove it as well
void ibis::util::removeDir(const char* name, bool leaveDir) {
    if (name == 0 || *name == 0) return; // can not do anything
    char* cmd = new char[strlen(name)+32];
    char buf[PATH_MAX];
    std::string event = "ibis::util::removeDir(";
    event += name;
    event += ")";
#if defined(USE_RM)
    if (leaveDir)
	sprintf(cmd, "/bin/rm -rf \"%s\"/*", name);
    else
	sprintf(cmd, "/bin/rm -rf \"%s\"", name);
    if (ibis::gVerbose > 4)
	ibis::util::logMessage(event.c_str(),
			       "issuing command \"%s\"", cmd);

    FILE* fptr = popen(cmd, "r");
    if (fptr) {
	while (fgets(buf, PATH_MAX, fptr)) {
	    if (ibis::gVerbose > 4)
		ibis::util::logMessage(event.c_str(), "got message -- %s",
				       buf);
	}

	int ierr = pclose(fptr);
	if (ierr && ibis::gVerbose > 0) {
	    ibis::util::logMessage("Warning ", "command \"%s\" returned with "
				   "error %d ... %s", cmd, ierr,
				   strerror(errno));
	}
	else if (ibis::gVerbose > 2) {
	    ibis::util::logMessage(event.c_str(), "command \"%s\" succeeded",
				   cmd);
	}
    }
    else if (ibis::gVerbose > 0) {
	ibis::util::logMessage("Warning", "%s failed to popen(%s) ... %s",
			       event.c_str(), cmd, strerror(errno));
    }
#elif defined(_WIN32) && defined(_MSC_VER)
    sprintf(cmd, "rmdir /s /q \"%s\"", name); // "/s /q" on available on NT
    if (ibis::gVerbose > 4)
 	ibis::util::logMessage(event.c_str(), "issuing command \"%s\"...",
			       cmd);

    FILE* fptr = _popen(cmd, "rt");
    if (fptr) {
 	while (fgets(buf, PATH_MAX, fptr)) {
 	    if (ibis::gVerbose > 4) {
		ibis::util::logMessage(event.c_str(), "get message -- %s",
				       buf);
	    }
 	}

 	int ierr = _pclose(fptr);
 	if (ierr && ibis::gVerbose > 0) { 
	    ibis::util::logMessage("Warning ", "command \"%s\" returned with "
				   "error %d ... %s", cmd, ierr,
				   strerror(errno));
 	}
 	else if (ibis::gVerbose > 2) {
	    ibis::util::logMessage(event.c_str(), "command \"%s\" succeeded",
				   cmd);
 	}
     }
     else if (ibis::gVerbose > 0) {
	ibis::util::logMessage("Warning", "%s failed to popen(%s) ... %s",
			       event.c_str(), cmd, strerror(errno));
     }
//       if (0 == rmdir(name)) {
//  	if (ibis::gVerbose > 4) {
//  	    ibis::util::logMessage("ibis::util::", "removeDir(%s) successful",
//  				   name);
//  	}
//      }
//      else if (ibis::gVerbose > 0) {
//  	ibis::util::logMessage("Warning", "removedir(%s) failed ... %s",
//  			       name, strerror(errno));
//      }
#elif defined(unix) || defined(__HOS_AIX__) || defined(__APPLE__) || defined(_XOPEN_SOURCE) || defined(_POSIX_C_SOURCE)
    char* olddir = getcwd(buf, PATH_MAX);
    if (olddir) {
	olddir = ibis::util::strnewdup(buf);
    }
    else {
	ibis::util::logMessage("ibis::util::removeDir()", "can not getcwd ... "
			       "%s ", strerror(errno));
    }

    int ierr = chdir(name);
    if (ierr != 0) {
	if (errno != ENOENT) // ok if directory does not exist
	    ibis::util::logMessage
		("ibis::util::removeDir()", "can not chdir to %s ... %s",
		 name, strerror(errno));
	if (errno == ENOTDIR) { // assume it to be a file
	    if (0 != remove(name))
		ibis::util::logMessage
		    ("ibis::util::removeDir()", "can not remove %s ... %s",
		     name, strerror(errno));
	}
	delete [] cmd;
	return;
    }

    (void) getcwd(buf, PATH_MAX);
    uint32_t len = strlen(buf);
    if (strncmp(buf, name, len)) { // names differ
	ibis::util::logMessage("ibis::util::removeDir()", "specified dir name "
			       "is %s, but CWD is actually %s", name, buf);
	strcpy(buf, name);
	len = strlen(buf);
    }
    if (buf[len-1] != DIRSEP) {
	buf[len] = DIRSEP;
	++len;
    }

    struct dirent* ent = 0;
    bool isEmpty = true;
    DIR* dirp = opendir(".");
    while ((ent = readdir(dirp)) != 0) {
	if (ent->d_name[0] == '.' &&
	    (ent->d_name[1] == static_cast<char>(0) ||
	     ent->d_name[1] == '.')) {
	    continue;	    // skip '.' and '..'
	}
	Stat_T fst;

	// construct the full name
	if (len+strlen(ent->d_name) >= PATH_MAX) {
	    ibis::util::logMessage("ibis::util::removeDir()", "file name "
				   "\"%s%s\" too long", buf, ent->d_name);
	    isEmpty = false;
	    continue;
	}
	strcpy(buf+len, ent->d_name);

	if (UnixStat(buf, &fst) != 0) {
	    ibis::util::logMessage("ibis::util::removeDir()",
				   "stat(%s) failed ... %s",
				   buf, strerror(errno));
	    if (0 != remove(buf)) {
		ibis::util::logMessage("ibis::util::removeDir()",
				       "can not remove %s ... %s",
				       buf, strerror(errno));
		if (errno != ENOENT) isEmpty = false;
	    }
	    continue;
	}
	bool isDir = ((fst.st_mode & S_IFDIR) == S_IFDIR);
	if (isDir) {
	    removeDir(buf);
	}
	else { // assume it is a regular file
	    if (0 != remove(buf)) {
		ibis::util::logMessage("ibis::util::removeDir()",
				       "can not remove %s ... %s",
				       buf, strerror(errno));
		if (errno != ENOENT) isEmpty = false;
	    }
	}
    }
    ierr = closedir(dirp);
    if (olddir) {
	ierr = chdir(olddir);
	if (0 != ierr) {
	    ibis::util::logMessage("Warning", "ibis::util::removeDir() cannot "
				   "return to %s ... %s", olddir,
				   strerror(errno));
	}
	delete [] olddir;
    }

    if (isEmpty && !leaveDir) {
	ierr = rmdir(name);
	if (ierr != 0) 
	    ibis::util::logMessage("ibis::util::removeDir()",
				   "directory %s can not removed ... %s",
				   name, strerror(errno));
    }
    else if (!isEmpty) {
	ibis::util::logMessage("ibis::util::removeDir()", "directory %s is "
			       "not removed because it is not empty", name);
    }
#else
    ibis::util::logMessage("Warning", "donot know how to delete directory");
#endif
    delete [] cmd;
}

/// Compute a compact 64-bit floating-point value in the range (left,
/// right].  The righ-end is inclusive because the computed value is used
/// to define bins based 'x<compactValue' and those bins are interpreted as
/// [low, upper) as in the typical c notion.
///
/// @note The shortest number is considered to be zero (0.0).
/// @note If the given start is not in the valid range, the middle point of
/// the range is used.
/// @note It returns 0.0 if either left or right is not-a-number.
double ibis::util::compactValue(double left, double right,
				double start) {
    if (left == right) return left;
    if (! (left != right)) return 0.0;
    if (left > right) {
	double tmp;
	tmp = left;
	left = right;
	right = tmp;
    }

    // if zero is in the range, return zero
    if (left < 0 && right >= 0)
	return 0.0;
    // if the range include 1, return 1
    if (left < 1.0 && right >= 1.0)
	return 1.0;
    // if the range include -1, return -1
    if (left < -1.0 && right >= -1.0)
	return -1.0;

    double diff, sep;
    if (left == 0.0) { // left == 0, right > 0
	diff = floor(log10(right));
	sep  = pow(10.0, diff);
	if (sep > right) {
	    if (diff >= -3.0 && diff < 3.0)
		sep *= 0.5;
	    else
		sep *= 0.1;
	}
    }
    else if (right < 0.0 && right * 10 > left) {
	// negative numbers with a large difference
	diff = ceil(log10(-right));
	sep  = -pow(10.0, diff);
	if (sep > right) {
	    if (diff >= -3.0 && diff <= 3.0)
		sep += sep;
	    else
		sep *= 10;
	}
    }
    else if (left > 0.0 && right > 10 * left) {
	// postitive numbers with a large difference
	diff = ceil(log10(left));
	sep  = pow(10.0, diff);
	if (sep <= left) {
	    if (sep >= -3.0 && diff <= 3.0)
		sep += sep;
	    else
		sep *= 10.0;
	}
    }
    else { // two values are within one order of magnitude
	diff = pow(10.0, ceil(FLT_EPSILON + log10(right - left)));
	if (!(start > left && start <= right))
	    start = 0.5 * (right + left);
	sep = floor(0.5 + start / diff) * diff;
	if (!(sep > left && sep <= right)) {
	    diff /= 2;
	    sep = floor(0.5 + start / diff) * diff;
	    if (!(sep > left && sep <= right)) {
		diff /= 5;
		sep = floor(0.5 + start / diff) * diff;
		if (! (sep > left && sep <= right)) {
		    diff /= 2;
		    sep = floor(0.5 + start / diff) * diff;
		    if (! (sep > left && sep <= right)) {
			diff /= 2;
			sep = floor(0.5 + start / diff) * diff;
		    }
		}
	    }
	}
    }
    if (! (sep > left && sep <= right)) {
	sep = right;
#if defined(_DEBUG) || defined(DEBUG)
	ibis::util::logMessage("Warning", "ibis::util::compactValue produced "
			       "a value, %g (%g) out of range (%g, %g]",
			       sep, diff, left, right);
#endif
    }
    return sep;
} // ibis::util::compactValue

/// This version round the number in binary and tries to use a few binary
/// digits in the mantissa as possible.  The resulting number should have a
/// better chance of producing exact results in simple arithmetic
/// operations.
///
/// @sa ibis::util::compactValue
double ibis::util::compactValue2(double left, double right,
				 double start) {
    if (left == right) return left;
    if (! (left != right)) return 0.0;
    if (left > right) {
	double tmp;
	tmp = left;
	left = right;
	right = tmp;
    }

    // if zero is in the range, return zero
    if (left < 0 && right >= 0)
	return 0.0;
    // if the range include 1, return 1
    if (left < 1.0 && right >= 1.0)
	return 1.0;
    // if the range include -1, return -1
    if (left < -1.0 && right >= -1.0)
	return -1.0;

    double diff, sep;
    if (left == 0.0) { // left == 0, right > 0
	diff = floor(1.4426950408889633870 * log(right));
	sep  = pow(2.0, diff);
	if (sep > right) {
	    sep *= 0.5;
	}
    }
    else if (right < 0.0 && right + right > left) {
	// negative numbers with a large difference
	diff = ceil(1.4426950408889633870 * log(-right));
	sep  = -pow(2.0, diff);
	if (sep > right) {
	    sep += sep;
	}
    }
    else if (left > 0.0 && right > left + left) {
	// postitive numbers with a large difference
	diff = ceil(1.4426950408889633870 * log(left));
	sep  = pow(2.0, diff);
	if (sep <= left) {
	    sep += sep;
	}
    }
    else { // two values are within a factor 2
	diff = pow(2.0, ceil(FLT_EPSILON + 1.4426950408889633870 *
			     log(right - left)));
	if (!(start > left && start <= right))
	    start = 0.5 * (right + left);
	sep = floor(0.5 + start / diff) * diff;
	if (!(sep > left && sep <= right)) {
	    diff *= 0.5;
	    sep = floor(0.5 + start / diff) * diff;
	    if (!(sep > left && sep <= right)) {
		diff *= 0.5;
		sep = floor(0.5 + start / diff) * diff;
		if (! (sep > left && sep <= right)) {
		    diff *= 0.5;
		    sep = floor(0.5 + start / diff) * diff;
		    if (! (sep > left && sep <= right)) {
			diff *= 0.5;
			sep = floor(0.5 + start / diff) * diff;
		    }
		}
	    }
	}
    }
    if (! (sep > left && sep <= right)) {
	sep = right;
#if defined(_DEBUG) || defined(DEBUG)
	ibis::util::logMessage("Warning", "ibis::util::compactValue2 produced "
			       "a value, %g (%g) out of range (%g, %g]",
			       sep, diff, left, right);
#endif
    }
    return sep;
} // ibis::util::compactValue2

void ibis::util::setNaN(float& val) {
    //static char tmp[5] = "\x7F\xBF\xFF\xFF";
    val = std::numeric_limits<float>::quiet_NaN();
} // ibis::util::setNaN

void ibis::util::setNaN(double& val) {
    //static char tmp[9] = "\x7F\xF7\xFF\xFF\xFF\xFF\xFF\xFF";
    val = std::numeric_limits<double>::quiet_NaN();
} // ibis::util::setNaN

// return a number that is different each time
unsigned long ibis::util::uniqueNumber() {
    static unsigned long int cnt = 0;
    static pthread_mutex_t numMutex = PTHREAD_MUTEX_INITIALIZER;

    ibis::util::mutexLock lock(&numMutex, "uniqueNumber");
    unsigned long int tmp = cnt;
    ++ cnt;
    return tmp;
} // ibis::util::uniqueNumber

// return a denominator and numerator pair to compute a uniform
// distribution of numbers in a given range
// the fraction (denominator / numerator) is uniformly distributed in [0, 1)
void ibis::util::uniformFraction(const long unsigned idx,
				 long unsigned &denominator,
				 long unsigned &numerator) {
    switch (idx) {
    case 0:
	denominator = 0;
	numerator = 1;
	break;
    case 1:
	denominator = 1;
	numerator = 2;
	break;
    case 2:
	denominator = 1;
	numerator = 4;
	break;
    case 3:
	denominator = 3;
	numerator = 4;
	break;
    default: // the general case
	if (idx <= INT_MAX) {
	    denominator = 4;
	    numerator = 8;
	    while (idx >= numerator) {
		denominator = numerator;
		numerator += numerator;
	    }
	    denominator = 2 * (idx - denominator) + 1;
	}
	else { // could not deal with very large numbers
	    denominator = 0;
	    numerator = 1;
	}
    }
} // unformMultiplier

// Fletcher's arithmetic checksum
uint32_t ibis::util::checksum(const char* str, uint32_t sz) {
    uint32_t c0 = 0;
    uint32_t c1 = 0;
    const unsigned ssz = sizeof(short);
    const short unsigned* ptr = reinterpret_cast<const unsigned short*>(str);
    while (sz >= ssz) {
	c1 += c0;
	c0 += *ptr;
	++ ptr;
	sz -= ssz;
    }
    if (sz > 0) {
	c1 += c0;
	c0 += *ptr;
    }
    uint32_t ret = (((c1 & 0xFFFF) << 16) ^ c0);
    return ret;
} // checksum for string

void ibis::util::int2string(std::string& str,
			    const std::vector<unsigned>& val) {
    uint32_t i;
    str.erase();
    std::string tmp;
    for (i = 0; i + 2 < val.size(); i += 3) {
	ibis::util::int2string(tmp, val[i], val[i+1], val[i+2]);
	str += tmp;
    }
    i -= 3;
    const uint32_t rem = val.size() - i;
    if (rem == 2) {
	ibis::util::int2string(tmp, val[i], val[i+1]);
	str += tmp;
    }
    else if (rem == 1) {
	ibis::util::int2string(tmp, val[i]);
	str += tmp;
    }
} // ibis::util::int2string

void ibis::util::int2string(std::string& str, unsigned v1, unsigned v2,
			    unsigned v3) {
    char name[17];
    name[16] = (char)0;
    name[15] = ibis::util::charTable[63 & v3]; v3 >>= 6;
    name[14] = ibis::util::charTable[63 & v3]; v3 >>= 6;
    name[13] = ibis::util::charTable[63 & v3]; v3 >>= 6;
    name[12] = ibis::util::charTable[63 & v3]; v3 >>= 6;
    name[11] = ibis::util::charTable[63 & v3]; v3 >>= 6;
    name[10] = ibis::util::charTable[63 & (v3 | (v2<<2))]; v2 >>= 4;
    name[9] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[8] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[7] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[6] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[5] = ibis::util::charTable[63 & (v2 | (v1<<4))]; v1 >>= 2;
    name[4] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[3] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[2] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[1] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[0] = ibis::util::charTable[63 & v1];
    str = name;
} // ibis::util::int2string

void ibis::util::int2string(std::string& str, unsigned v1, unsigned v2) {
    char name[12];
    name[11] = (char)0;
    name[10] = ibis::util::charTable[15 & v2]; v2 >>= 4;
    name[9] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[8] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[7] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[6] = ibis::util::charTable[63 & v2]; v2 >>= 6;
    name[5] = ibis::util::charTable[63 & (v2 | (v1<<4))]; v1 >>= 2;
    name[4] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[3] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[2] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[1] = ibis::util::charTable[63 & v1]; v1 >>= 6;
    name[0] = ibis::util::charTable[63 & v1];
    str = name;
} // ibis::util::int2string

void ibis::util::int2string(std::string& str, unsigned val) {
    char name[7];
    name[6] = (char)0;
    name[5] = ibis::util::charTable[ 3 & val]; val >>= 2;
    name[4] = ibis::util::charTable[63 & val]; val >>= 6;
    name[3] = ibis::util::charTable[63 & val]; val >>= 6;
    name[2] = ibis::util::charTable[63 & val]; val >>= 6;
    name[1] = ibis::util::charTable[63 & val]; val >>= 6;
    name[0] = ibis::util::charTable[63 & val];
    str = name;
} // ibis::util::int2string

// return the name of the user who is running this program
const char* ibis::util::userName() {
    static std::string uid;
    ibis::util::mutexLock lock(&ibis::util::envLock, "(::()::)");

    if (uid.empty()) {
#if defined(_WIN32) && defined(_MSC_VER)
	long unsigned int len = 63;
	char buf[64];
	if (GetUserName(buf, &len))
	    uid = buf;
#elif defined(__MINGW32__)
	// MinGW does not have support for user names?!
#elif defined(_POSIX_VERSION)
	// the trusted getpwuid(getuid()) combination
	struct passwd *pass = getpwuid(getuid());
	if (pass != 0)
	    uid = pass->pw_name;
#elif defined(L_cuserid) && defined(__USE_XOPEN)
	// in the unlikely case that we are not on a POSIX-compliant system
	// https://buildsecurityin.us-cert.gov/daisy/bsi-rules/home/g1/731.html
	char buf[L_cuserid+1];
	(void) cuserid(buf);
	if (*buf)
	    uid = buf;
#elif defined(_REENTRANT) || defined(_THREAD_SAFE) || defined(_POSIX_THREAD_SAFE_FUNCTIONS)
	// in case we are not on a posix-compliant system and no cuserid,
	// try getlogin, however
	// getlongin and variants need a TTY or utmp entry to function
	// correctly.  They may cause a core dump if this function is
	// called without a TTY (or utmp)
	char buf[64];
	if (getlogin_r(buf, 64) == 0) {
	    uid = buf;
	}
	else {
	    uid = getlogin();
	}
#elif defined(unix) || defined(__HOS_AIX__) || defined(__APPLE__)
	uid = getlogin();
#endif
	// final fall back option, assign a fixed string that is commonly
	// interpreted as robot
	if (uid.empty())
	    uid = "(::()::)";
    }
    return uid.c_str();
} // ibis::util::userName

/// A function to output printf style message to stdout.  The message is
/// preceeded with the current local time (as from function ctime) if the
/// macro TIMED_LOG is defined as compile time.
void ibis::util::logMessage(const char* event, const char* fmt, ...) {
    if (ibis::gVerbose < 0) return;

    FILE* fptr = ibis::util::getLogFile();
#if (defined(HAVE_VPRINTF) || defined(_WIN32)) && ! defined(DISABLE_VPRINTF)
#if defined(TIMED_LOG)
    char tstr[28];
    ibis::util::getLocalTime(tstr);

    ibis::util::ioLock lock;
    fprintf(fptr, "%s   %s -- ", tstr, event);
#else
    ibis::util::ioLock lock;
    fprintf(fptr, "%s -- ", event);
#endif
    va_list args;
    va_start(args, fmt);
    vfprintf(fptr, fmt, args);
    va_end(args);
    fprintf(fptr, "\n");
    fflush(fptr);
#else
    ibis::util::logger lg(4);
    lg.buffer() << event << " -- " << fmt << " ...";
#endif
} // ibis::util::logMessage

FILE* ibis::util::getLogFile() {
    if (ibis_util_logfilepointer != 0)
	return ibis_util_logfilepointer;

    ibis::util::ioLock lock;
    if (ibis_util_logfilepointer != 0)
	return ibis_util_logfilepointer;

    char tstr[28];
    ibis::util::getLocalTime(tstr);
    const char* fname = (ibis_util_logfilename.empty() ? 0 :
			 ibis_util_logfilename.c_str());
    int ierr = 0;
    if (fname != 0 && *fname != 0) {
	FILE* fptr = fopen(fname, "a");
	if (fptr != 0) {
	    ierr = fprintf(fptr,
			   "\nibis::util::getLogFile opened %s on %s\n",
			   fname, tstr);
	    if (ierr > 1) {
		ibis_util_logfilepointer = fptr;
		ibis_util_logfilename = fname;
		return fptr;
	    }
	}
    }

    fname = getenv("FASTBITLOGFILE");
    if (fname != 0 && *fname != 0) {
	FILE* fptr = fopen(fname, "a");
	if (fptr != 0) {
	    ierr = fprintf(fptr,
			   "\nibis::util::getLogFile opened %s on %s\n",
			   fname, tstr);
	    if (ierr > 1) {
		ibis_util_logfilepointer = fptr;
		ibis_util_logfilename = fname;
		return fptr;
	    }
	}
    }

    fname = ibis::gParameters()["logfile"];
    if (fname == 0 || *fname == 0)
	fname = ibis::gParameters()["mesgfile"];
    if (fname != 0 && *fname == 0) {
	FILE* fptr = fopen(fname, "a");
	if (fptr != 0) {
	    ierr = fprintf(fptr,
			   "\nibis::util::getLogFile opened %s on %s\n",
			   fname, tstr);
	    if (ierr > 1) {
		ibis_util_logfilepointer = fptr;
		ibis_util_logfilename = fname;
		return fptr;
	    }
	}
    }

    ibis_util_logfilepointer = stdout;
    ibis_util_logfilename.erase();
    return stdout;
} // ibis::util::getLogFile

/// @note A blank file name or a null pointer reset the log file to
/// standard output.
///
/// Error code
/// -  0: success;
/// - -1: filename is nil;
/// - -2: unable to open the named file;
/// - -3: unable to write to the named file.
int ibis::util::setLogFileName(const char* filename) {
    if (filename == 0 || *filename == 0) {
	if (ibis_util_logfilename.empty())
	    return 0;

	if (ibis_util_logfilepointer != 0 && ! ibis_util_logfilename.empty())
	    fclose(ibis_util_logfilepointer);
	ibis_util_logfilepointer = stdout;
	ibis_util_logfilename.erase();
	return 0;
    }

    char tstr[28];
    ibis::util::getLocalTime(tstr);

    ibis::util::ioLock lock;
    FILE* fptr = fopen(filename, "a");
    if (fptr != 0) {
	int ierr = fprintf(fptr,
			   "\nibis::util::setLogFileName opened %s on %s\n",
			   filename, tstr);
	if (ierr > 1) {
	    if (ibis_util_logfilepointer != 0 &&
		! ibis_util_logfilename.empty())
		fclose(ibis_util_logfilepointer);
	    ibis_util_logfilename = filename;
	    ibis_util_logfilepointer = fptr;
	    return 0;
	}
	else {
	    return -3;
	}
    }
    else {
	return -2;
    }
} // ibis::util::setLogFileName

const char* ibis::util::getLogFileName() {
    return ibis_util_logfilename.c_str();
} // ibis::util::getLogFileName

/// Return the return value of function fclose.  This function should be
/// called.  If not, typical OS system will close the file after the
/// termination of the process invoked FastBit, however, it is possible
/// that the last few messages may be lost.
int ibis::util::closeLogFile() {
    int ierr = 0;
    ibis::util::ioLock lock;
    if (ibis_util_logfilepointer != 0 && ! ibis_util_logfilename.empty()) {
	ierr = fclose(ibis_util_logfilepointer);
	ibis_util_logfilename.erase();
	ibis_util_logfilepointer = 0;
    }
    return ierr;
} // ibis::util::closeLogFile

ibis::util::logger::logger(int lvl) {
#if defined(TIMED_LOG)
    char tstr[28];
    ibis::util::getLocalTime(tstr);
    mybuffer << tster << " ";
    if (lvl > 0) {
	if (lvl > 1000) lvl = 10 + (int)sqrt(log((double)lvl));
	else if (lvl > 8) lvl = 6 + (int)log((double)lvl);
	for (int i = 0; i < lvl; ++ i)
	    mybuffer << " ";
    }
#endif
} // ibis::util::logger::logger

ibis::util::logger::~logger() {
    const std::string& mystr = mybuffer.str();
    if (ibis::gVerbose >= 0 && ! mystr.empty()) {
	FILE* fptr = ibis::util::getLogFile();
	// The lock is still necessary because other logging functions use
	// multiple fprintf statements.
	ibis::util::ioLock lock;
	fprintf(fptr, "%s\n", mystr.c_str());
#if defined(_DEBUG) || defined(DEBUG)
	fflush(fptr);
#endif
    }
} // ibis::util::logger::~logger

ibis::util::timer::timer(const char* msg, int lvl) :
    chrono_(ibis::gVerbose >= lvl ? new ibis::horometer : 0), mesg_(msg) {
    if (chrono_ != 0) {
	chrono_->start();
	ibis::util::logger(2).buffer()
	    << mesg_ << " -- start timer ...";
    }
} // ibis::util::timer::timer

ibis::util::timer::~timer() {
    if (chrono_ != 0) {
	chrono_->stop();
	ibis::util::logger(2).buffer()
	    << mesg_ << " -- stop timer ... duration: " << chrono_->CPUTime()
	    << " sec(CPU), " << chrono_->realTime() << " sec(elapsed)";
	delete chrono_;
    }
} // ibis::util::timer::~timer

// The meta characters used in ibis::util::strMatch.  
#define STRMATCH_META_CSH_ANY '*'
#define STRMATCH_META_CSH_ONE '?'
#define STRMATCH_META_SQL_ANY '%'
#define STRMATCH_META_SQL_ONE '_'
#define STRMATCH_META_ESCAPE '\\'

/// If the whole string matches the pattern, this function returns true,
/// otherwise, it returns false.  The special cases are (1) if the two
/// pointere are the same, it returns true; (2) if both arguments point to
/// an empty string, it returns true; (3) if one of the two argument is a
/// nil pointer, but the other is not, it returns false; (4) if one of the
/// two arguments is an empty string, but the other is not, it returns
/// false.
///
/// The pattern may contain five special characters, two matches any number
/// of characters, STRMATCH_META_CSH_ANY and STRMATCH_META_SQL_ANY, two
/// matches exactly one character, STRMATCH_META_CSH_ONE and
/// STRMATCH_META_SQL_ONE, and STRMATCH_META_ESCAPE.  This combines the
/// meta characters used in C-shell file name substitution and SQL LIKE
/// clause. 
///
/// @note The strings matched without considering the case, i.e., the match
/// is case insensitive.
/// @note This is not POSIX regular expression matching!
bool ibis::util::strMatch(const char *str, const char *pat) {
    static const char metaList[6] = "?*_%\\";
    /* Since the escape character is special to C/C++ too, the following initialization causes problem for some compilers!
    static const char metaList[] = {STRMATCH_META_CSH_ANY,
				    STRMATCH_META_SQL_ANY,
				    STRMATCH_META_CSH_ONE,
				    STRMATCH_META_SQL_ONE,
				    STRMATCH_META_ESCAPE}; */
    if (str == pat) {
	return true;
    }
    else if (pat == 0) {
	return (str == 0);
    }
    else if (*pat == 0) {
	return (str != 0 && *str == 0);
    }
    else if (str == 0) {
	return false;
    }
    else if (*str == 0) {
	return false;
    }

    const char *s1 = strpbrk(pat, metaList);
    const long int nhead = s1 - pat;
    if (s1 < pat) { // no meta character
	return (0 == stricmp(str, pat));
    }
    else if (s1 > pat && 0 != strnicmp(str, pat, nhead)) {
	// characters before the first meta character do not match
	return false;
    }

    if (*s1 == STRMATCH_META_ESCAPE) { // escape character
	if (str[nhead] == pat[nhead+1])
	    return strMatch(str+nhead+1, pat+nhead+2);
	else
	    return false;
    }
    else if (*s1 == STRMATCH_META_CSH_ONE || *s1 == STRMATCH_META_SQL_ONE) {
	// match exactly one character
	if (str[nhead] != 0)
	    return strMatch(str+nhead+1, pat+nhead+1);
	else
	    return false;
    }

    // found STRMATCH_META_*_ANY
    const char* s0 = str + nhead;
    do { // skip consecutive STRMATCH_META_*_ANY
	++ s1;
    } while (*s1 == STRMATCH_META_CSH_ANY || *s1 == STRMATCH_META_SQL_ANY);
    if (*s1 == 0) // pattern end with STRMATCH_META_ANY
	return true;

    const char* s2 = 0;
    bool  ret;
    if (*s1 == STRMATCH_META_ESCAPE) { // skip STRMATCH_META_ESCAPE
	++ s1;
	if (*s1 != 0)
	    s2 = strpbrk(s1+1, metaList);
	else
	    return true;
    }
    else if (*s1 == STRMATCH_META_CSH_ONE || *s1 == STRMATCH_META_SQL_ONE) {
	do {
	    if (*s0 != 0) { // STRMATCH_META_*_ONE matched
		++ s0;
		// STRMATCH_META_*_ANY STRMATCH_META_*_ONE
		// STRMATCH_META_*_ANY
		// ==> STRMATCH_META_*_ANY STRMATCH_META_*_ONE
		do {
		    ++ s1;
		} while (*s1 == STRMATCH_META_CSH_ANY ||
			 *s1 == STRMATCH_META_SQL_ANY);
	    }
	    else { // end of str, STRMATCH_META_*_ONE not matched
		return false;
	    }
	} while (*s1 == STRMATCH_META_CSH_ONE ||
		 *s1 == STRMATCH_META_SQL_ONE);
	if (*s1 == 0) {
	    // pattern end with STRMATCH_META_*_ANY plus a number of
	    // STRMATCH_META_*_ONE that have been matched.
	    return true;
	}
	if (*s1 == STRMATCH_META_ESCAPE) {
	    ++ s1;
	    if (*s1 != 0)
		s2 = strpbrk(s1+1, metaList);
	}
	else {
	    s2 = strpbrk(s1, metaList);
	}
    }
    else {
	s2 = strpbrk(s1, metaList);
    }

    if (s2 < s1) { // no more meta character
	const uint32_t ntail = strlen(s1);
	if (ntail <= 0U)
	    return true;

	uint32_t nstr = strlen(s0);
	if (nstr < ntail)
	    return false;
	else
	    return (0 == stricmp(s1, s0+(nstr-ntail)));
    }

    const std::string anchor(s1, s2);
    const char* tmp = strstr(s0, anchor.c_str());
    if (tmp < s0) return false;
    ret = strMatch(tmp+anchor.size(), s2);
    while (! ret) { // retry
	tmp = strstr(tmp+1, anchor.c_str());
	if (tmp > s0) {
	    ret = strMatch(tmp+anchor.size(), s2);
	}
	else {
	    break;
	}
    }
    return ret;
} // ibis::util::strMatch

/// Converts the given time in seconds (as returned by function time) into
/// the string (as from asctime_r).  The argument @c str must contain at 26
/// bytes.  The new line character is turned into null.
void ibis::util::secondsToString(const time_t sec, char *str) {
#if defined(_MSC_VER) && defined(_WIN32)
    ibis::util::quietLock lock(&ibis_util_timeLock);
    strcpy(str, asctime(localtime(&sec)));
    str[24] = 0;
#else
    struct tm stm;
    if (localtime_r(&sec, &stm)) {
	if (asctime_r(&stm, str))
	    str[24] = 0;
	else
	    *str = 0;
    }
    else
	*str = 0;
#endif
} // ibis::util::secondsToString

/// The new line character in position 24 is turned into a null character.
/// Therefore the returned string contains only 24 characters.
void ibis::util::getLocalTime(char *str) {
    time_t sec = time(0); // current time in seconds
#if defined(_MSC_VER) && defined(_WIN32)
    ibis::util::quietLock lock(&ibis_util_timeLock);
    strcpy(str, asctime(localtime(&sec)));
    str[24] = 0;
#else
    struct tm stm;
    if (localtime_r(&sec, &stm)) {
	if (asctime_r(&stm, str))
	    str[24] = 0;
	else
	    *str = 0;
    }
    else
	*str = 0;
#endif
} // ibis::util::getLocalTime

void ibis::util::getGMTime(char *str) {
    time_t sec = time(0); // current time in seconds
#if defined(_MSC_VER) && defined(_WIN32)
    ibis::util::quietLock lock(&ibis_util_timeLock);
    strcpy(str, asctime(gmtime(&sec)));
    str[24] = 0;
#else
    struct tm stm;
    if (gmtime_r(&sec, &stm)) {
	if (asctime_r(&stm, str))
	    str[24] = 0;
	else
	    *str = 0;
    }
    else {
	*str = 0;
    }
#endif
} // ibis::util::getGMTime

/// Constructor.  The incoming argument is the numbre of elements to be
/// allocated.  If it is zero, it defaults to use 16 MB of space, and the
/// number of elements 16 million divided by the size of the element.  If
/// it fails to allocate the requested memory, it will reduce the number of
/// elements by a half and then by a quarter for a total of seven times.
/// If it failed all eight tries, it will set the buffer address to nil and
/// the number of elements to zero.
template <typename T>
ibis::util::buffer<T>::buffer(uint32_t sz) : buf(0), nbuf(sz) {
    if (nbuf == 0)
	nbuf = 16777216/sizeof(T); // preferred buffer size is 16 MB

    try {buf = new T[nbuf];}
    catch (const std::bad_alloc&) {
	nbuf >>= 1; // reduce the size by half and try again
	try {buf = new T[nbuf];}
	catch (const std::bad_alloc&) {
	    nbuf >>= 2; // reduce the size by a quarter and try again
	    try {buf = new T[nbuf];}
	    catch (const std::bad_alloc&) {
		nbuf >>= 2; // reduce the size by a quarter and try again
		try {buf = new T[nbuf];}
		catch (const std::bad_alloc&) {
		    nbuf >>= 2;
		    try {buf = new T[nbuf];}
		    catch (const std::bad_alloc&) {
			nbuf >>= 2;
			try {buf = new T[nbuf];}
			catch (const std::bad_alloc&) {
			    nbuf = 0;
			    buf = 0;
			}
		    }
		}
	    }
	}
    }
} // ibis::util::buffer::ctor

// explicit template instantiation required
template class ibis::util::buffer<char>;
template class ibis::util::buffer<signed char>;
template class ibis::util::buffer<unsigned char>;
template class ibis::util::buffer<float>;
template class ibis::util::buffer<double>;
template class ibis::util::buffer<int16_t>;
template class ibis::util::buffer<uint16_t>;
template class ibis::util::buffer<int32_t>;
template class ibis::util::buffer<uint32_t>;
template class ibis::util::buffer<int64_t>;
template class ibis::util::buffer<uint64_t>;


#ifdef IBIS_REPLACEMENT_RWLOCK
//
// Adapoted from A. Sim's implementation called qthread
//
int pthread_rwlock_init(pthread_rwlock_t *rwp, void*) {
    pthread_mutex_init(&rwp->lock, 0);
    pthread_cond_init(&rwp->readers, 0);
    pthread_cond_init(&rwp->writers, 0);

    rwp->state = 0;
    rwp->waiters = 0;
    return(0);
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwp) {
    pthread_mutex_destroy(&rwp->lock);
    pthread_cond_destroy(&rwp->readers);
    pthread_cond_destroy(&rwp->writers);

    return(0);
}

void pthread_mutex_lock_cleanup(void* arg) {
    pthread_mutex_t* lock = (pthread_mutex_t *) arg;
    pthread_mutex_unlock(lock);
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwp) {
    pthread_mutex_t	*lkp	= &rwp->lock;

    pthread_mutex_lock(lkp);
    pthread_cleanup_push(pthread_mutex_lock_cleanup, lkp);
    // active or queued writers	
    while ((rwp->state < 0) || rwp->waiters)
	pthread_cond_wait(&rwp->readers, lkp);

    rwp->state++;
    pthread_cleanup_pop(1);
    return(0);
}

int pthread_rw_tryrdlock(pthread_rwlock_t *rwp) {
    int status = EBUSY;

    pthread_mutex_lock(&rwp->lock);
    // available and no writers queued
    if ((rwp->state >= 0) && !rwp->waiters) {
	rwp->state++;
	status = 0;
    }

    pthread_mutex_unlock(&rwp->lock);
    return(status);
}

void pthread_rwlock_wrlock_cleanup(void *arg) {
    pthread_rwlock_t	*rwp	= (pthread_rwlock_t *) arg;

    //
    // Was the only queued writer and lock is available for readers.
    // Called through cancellation clean-up so lock is held at entry.
    //
    if ((--rwp->waiters == 0) && (rwp->state >= 0))
	pthread_cond_broadcast(&rwp->readers);

    pthread_mutex_unlock(&rwp->lock);

    return;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwp) {
    pthread_mutex_t	*lkp	= &rwp->lock;

    pthread_mutex_lock(lkp);
    rwp->waiters++;		// another writer queued	
    pthread_cleanup_push(pthread_rwlock_wrlock_cleanup, rwp);

    while (rwp->state)
	pthread_cond_wait(&rwp->writers, lkp);

    rwp->state	= -1;
    pthread_cleanup_pop(1);
    return(0);
}

int pthread_rw_trywrlock(pthread_rwlock_t *rwp) {
    int    status  = EBUSY;

    pthread_mutex_lock(&rwp->lock);
    // no readers, no writers, no writers queued
    if (!rwp->state && !rwp->waiters) {
	rwp->state = -1;
	status     = 0;
    }

    pthread_mutex_unlock(&rwp->lock);
    return(status);
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwp) {
    pthread_mutex_lock(&rwp->lock);

    if (rwp->state == -1) {	// writer releasing
	rwp->state = 0;		// mark as available

	if (rwp->waiters)		// writers queued
	    pthread_cond_signal(&rwp->writers);
	else
	    pthread_cond_broadcast(&rwp->readers);
    }
    else {
	if (--rwp->state == 0)	// no more readers
	    pthread_cond_signal(&rwp->writers);
    }
    pthread_mutex_unlock(&rwp->lock);

    return(0);
}
#endif // IBIS_REPLACEMENT_RWLOCK

#if defined(WIN32) && ! defined(__CYGWIN__)
#include <conio.h>
char* ibis::util::getpass_r(const char *prompt, char *buff, uint32_t buflen) {
    uint32_t i;
    printf("%s ", prompt);

    for(i = 0; i < buflen; ++ i) {
	buff[i] = getch();
	if ( buff[i] == '\r' ) { /* return key */
	    buff[i] = 0;
	    break;
	}
	else {
	    if ( buff[i] == '\b')	/* backspace */
		i = i - (i>=1?2:1);
	}
    }

    if (i==buflen)    /* terminate buff */
	buff[buflen-1] = 0;

    return buff;
} // getpass_r

char* getpass(const char *prompt) {
    static char buf[256];
    ibis::util::mutexLock lock(&ibis::util::envLock, "ibis::util::getpass");
    return ibis::util::getpass_r(prompt, buf, 256);
} // getpass
#endif

#if !defined(unix) && defined(_WIN32)
// truncate the named file to specified bytes
int truncate(const char* name, uint32_t bytes) {
    int ierr = 0;
    int fh = _open(name, _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE );
    if (fh >= 0) { // open successful
	LOGGER(ibis::gVerbose > 3)
	    << "file \"" << name
	    << "\" length before truncation is " << _filelength(fh);
	ierr = _chsize(fh, bytes);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "ERROR *** _chsize(" << name << ", " << bytes
		<< ") returned " << ierr;
	}
	else {
	    LOGGER(ibis::gVerbose > 3)
		<< "file \"" << name
		<< "\" length after truncation is " << _filelength(fh);
	}
	_close(fh);
    }
    else {
	ierr = INT_MIN;
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- function truncate failed to open file \""
	    << name << "\"";
    }
    return ierr;
}
#endif

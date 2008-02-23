// File: $Id$
// Author: John Wu <John.Wu@nersc.gov> Lawrence Berkeley National Laboratory
// Copyright 2000-2008 the Regents of the University of California
#ifndef IBIS_UTIL_H
#define IBIS_UTIL_H
///@file
/// Defines minor utility functions and common classes used by
/// FastBit.
///
#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include "const.h"

#include <ctype.h>	// isspace
#include <stdio.h>	// sprintf, remove
//#if HAVE_SYS_STAT_H
#include <sys/stat.h>	// stat, mkdir, chmod
//#endif
//#if HAVE_FCNTL_H
#include <fcntl.h>	// open, close
//#endif
#include <map>		// std::map
#include <vector>	// std::vector
#include <string>	// std::string

#include <sstream>	// std::ostringstream used by ibis::util::logger
#include <float.h>
#include <math.h>	// fabs, floor, ceil, log10, ...
#if !defined(unix) && defined(_WIN32)
#include <windows.h>
#include <io.h>
#include <fcntl.h>	// _O_..
int truncate(const char*, uint32_t);
#include <direct.h>	// _rmdir
#include <sys/stat.h>	// _stat, mkdir, chmod
#define rmdir _rmdir
//#elif HAVE_UNISTD_H
#else
#include <unistd.h>	// read, lseek, truncate, rmdir
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16
#else
// some wrongly define it to be 1.1e-16
#undef  DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16
#endif

// mapping the names of the low level IO functions defined in <unistd.h>
#if defined(_MSC_VER) && defined(_WIN32)
#define UnixOpen  ::_open
#define UnixClose ::_close
#define UnixRead  ::_read
#define UnixWrite ::_write
#define UnixSnprintf ::_snprintf
#define UnixSeek  ::_lseek
#define UnixStat  ::_stat
#define UnixFStat ::_fstat
#define Stat_T    struct _stat
#else
#define UnixOpen  ::open
#define UnixClose ::close
#define UnixRead  ::read
#define UnixWrite ::write
#define UnixSnprintf ::snprintf
#define UnixSeek  ::lseek
#define UnixStat  ::stat
#define UnixFStat ::fstat
#define Stat_T    struct stat
#endif

// define the option for opening a file in read only mode
#if defined(O_RDONLY)
#if defined(O_LARGEFILE)
#if defined(O_BINARY)
#define OPEN_READONLY O_RDONLY | O_BINARY | O_LARGEFILE
#else
#define OPEN_READONLY O_RDONLY | O_LARGEFILE
#endif
#elif defined(O_BINARY)
#define OPEN_READONLY O_RDONLY | O_BINARY
#else
#define OPEN_READONLY O_RDONLY
#endif
#elif defined(_O_RDONLY)
#if defined(_O_LARGEFILE)
#define OPEN_READONLY _O_RDONLY | _O_LARGEFILE | _O_BINARY
#else
#define OPEN_READONLY _O_RDONLY | _O_BINARY
#endif
#endif
// define the option for opening a new file for writing
#if defined(O_WRONLY)
#if defined(O_LARGEFILE)
#if defined(O_BINARY)
#define OPEN_WRITEONLY O_WRONLY | O_BINARY | O_CREAT | O_TRUNC | O_LARGEFILE
#else
#define OPEN_WRITEONLY O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE
#endif
#elif defined(O_BINARY)
#define OPEN_WRITEONLY O_WRONLY | O_BINARY | O_CREAT | O_TRUNC
#else
#define OPEN_WRITEONLY O_WRONLY | O_CREAT | O_TRUNC
#endif
#elif defined(_O_WRONLY)
#if defined(_O_LARGEFILE)
#define OPEN_WRITEONLY _O_WRONLY|_O_CREAT|_O_TRUNC|_O_LARGEFILE|_O_BINARY
#else
#define OPEN_WRITEONLY _O_WRONLY|_O_CREAT|_O_TRUNC|_O_BINARY
#endif
#endif
// define the option for opening a file for reading and writing
#if defined(O_RDWR)
#if defined(O_LARGEFILE)
#if defined(O_BINARY)
#define OPEN_READWRITE O_RDWR | O_BINARY | O_CREAT | O_LARGEFILE
#else
#define OPEN_READWRITE O_RDWR | O_CREAT | O_LARGEFILE
#endif
#elif defined(O_BINARY)
#define OPEN_READWRITE O_RDWR | O_BINARY | O_CREAT
#else
#define OPEN_READWRITE O_RDWR | O_CREAT
#endif
#elif defined(_O_RDWR)
#if defined(_O_LARGEFILE)
#define OPEN_READWRITE _O_RDWR | _O_CREAT | _O_LARGEFILE | _O_BINARY
#else
#define OPEN_READWRITE _O_RDWR | _O_CREAT | _O_BINARY
#endif
#endif
// define the option for opening an existing file for appending
#if defined(O_WRONLY)
#if defined(O_LARGEFILE)
#if defined(O_BINARY)
#define OPEN_APPENDONLY O_WRONLY | O_BINARY | O_CREAT | O_APPEND | O_LARGEFILE
#else
#define OPEN_APPENDONLY O_WRONLY | O_CREAT | O_APPEND | O_LARGEFILE
#endif
#elif defined(O_BINARY)
#define OPEN_APPENDONLY O_WRONLY | O_BINARY | O_CREAT | O_APPEND
#else
#define OPEN_APPENDONLY O_WRONLY | O_CREAT | O_APPEND
#endif
#elif defined(_O_WRONLY)
#if defined(_O_LARGEFILE)
#define OPEN_APPENDONLY _O_WRONLY | _O_CREAT | _O_APPEND | _O_LARGEFILE | _O_BINARY
#else
#define OPEN_APPENDONLY _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY
#endif
#endif
// the default file mode (associate with _O_CREAT)
#if defined(_MSC_VER) && defined(_WIN32)
#define OPEN_FILEMODE _S_IREAD | _S_IWRITE
#elif defined(S_IRGRP) && defined(S_IWGRP) && defined(S_IROTH)
#define OPEN_FILEMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
#else
#define OPEN_FILEMODE S_IRUSR | S_IWUSR
#endif

#define LOGGER(v) \
if ((v) > ibis::gVerbose) ; else ibis::util::logger(v).buffer() 

namespace std { // extend namespace std slightly
    // specialization of less<> to work with char*
    template <> struct less< char* > {
	bool operator()(const char*x, const char*y) const {
	    return strcmp(x, y) < 0;
	}
    };

    // specialization of less<> on const char* (case sensitive comparison)
    template <> struct less< const char* > {
	bool operator()(const char* x, const char* y) const {
	    return strcmp(x, y) < 0;
	}
    };

    template <> struct less< ibis::rid_t > {
	bool operator()(const ibis::rid_t& x, const ibis::rid_t& y) const {
	    return (x < y);
	}
    };

    template <> struct less< const ibis::rid_t* > {
	bool operator()(const ibis::rid_t* x, const ibis::rid_t* y) const {
	    return (*x < *y);
	}
    };
} // namespace std

namespace ibis {

    // RIDSet
    typedef array_t< rid_t > RIDSet;

    class column;	///< One column/attribute of a table.
    class fileManager;	///< A simple file manager.
    class horometer;	///< A timer class.
    class index;	///< The base class of indices.
    class qExpr;	///< The base class of query expressions.
    class roster;	///< A projection of a column in ascending order.

    // col with (selected) values -- need to handle bundles
    class bundle;
    class colValues;
    typedef std::vector<colValues*> colList;

    class bitvector64; // the 64-bit version of bitvector class

    ///@brief Compute the outer product of @c a and @c b, add the result to @c c.
    const ibis::bitvector64& outerProduct(const ibis::bitvector& a,
					  const ibis::bitvector& b,
					  ibis::bitvector64& c);
    ///@brief Add the strict upper triangular portion of the outer production
    /// between @c a and @c b to @c c.
    const ibis::bitvector64& outerProductUpper(const ibis::bitvector& a,
					       const ibis::bitvector& b,
					       ibis::bitvector64& c);
    /// @brief The reference to the global configuration parameters.
    FASTBIT_DLLSPEC ibis::resource& gParameters();

    /// A data structure to store a small set of names.  The names are
    /// sorted in case-insensitive order.
    class FASTBIT_DLLSPEC nameList {
    public:
	nameList() : cstr(0), buff(0) {};
	nameList(const char* str) : cstr(0), buff(0) {select(str);}
	~nameList() {if (cstr) clear();}

	bool empty() const {return cstr == 0;}
	const char* operator*() const {return cstr;};
	size_t size() const {return cvec.size();};

	/// Add more names to the list.
	void select(const char* str);
	/// Find the order of the @c key in the list.  If the @c key is in
	/// the list it returns the position of the @c key, otherwise it
	/// returns the size of the name list.
	size_t find(const char* key) const;

	const char* operator[](size_t i) const {return cvec[i];}
	typedef std::vector< const char* >::const_iterator const_iterator;
	const_iterator begin() const {return cvec.begin();}
	const_iterator end() const {return cvec.end();}

	void clear()
	{cvec.clear(); delete [] cstr; delete [] buff; buff=0; cstr=0;}

    private:
	static const char* delimiters;
	typedef std::vector< const char * > compStore;
	char* cstr;	// copy of the component string
	char* buff;	// same as cstr, but delimiter is \0
	compStore cvec;	// contains pointers to buff, for easier access

	nameList(const nameList&);
	const nameList& operator=(const nameList&);
    }; // class nameList

    /// A data structure to store the select clause of a query.  A select
    /// clause may contain a list of names plus a list of simple functions.
    /// The supported functions are @c avg, @c max, @c min and @c sum.
    /// Each of these functions can only take a single name as it argument.
    class selected {
    public:
	selected() {}; ///< The default constructor leaves data members empty.
	selected(const char* str) {select(str);}
	~selected() {};

	bool empty() const {return names.empty();}
	size_t size() const {return names.size();}
	size_t nPlain() const {return nplain;}

	/// Parse the select clause.  By default, place the functions last.
	void select(const char *str, bool sort=true);
	void select(const std::vector<const char*>& nl, bool sort=true);
	/// Return the list of names stored internally.
	const std::vector<std::string>& getNames() const {return names;}
	/// Return the first occurrence of the string.  Returns the value of
	/// @c size if the given @c key in not in the list of selected
	/// components.
	size_t find(const char *key) const;
	void clear() {
	    names.clear(); functions.clear(); mystr_.erase(); nplain=0;}
	/// Remove the entries specified.
	void remove(const std::vector<size_t>& ents);

	/// Output a stringlized version of the select clause.
	const char* operator*() const {return mystr_.c_str();}

	/// Access the ith column name of the select clause.  In case of a
	/// function, it returns the name of the argument rather than the
	/// whole function.  This operator is only intended to be used to
	/// extract the column values.
	const char* getName(size_t i) const {return names[i].c_str();};
	const char* operator[](size_t i) const {return names[i].c_str();};
	/// Return all unique column names.
	std::string uniqueNames() const;
	/// Return the ith term, with the function name.
	std::string getTerm(size_t i) const;

	/// An iterator through the column names of the select clause.
	typedef std::vector<std::string>::const_iterator const_iterator;
	const_iterator begin() const {return names.begin();}
	const_iterator end() const {return names.end();}

	enum FUNCTION {NIL, AVG, MAX, MIN, SUM};
	FUNCTION getFunction(size_t i) const {return functions[i];}

    private:
	static const char* delimiters;

	std::vector<std::string> names;
	std::vector<FUNCTION> functions;
	std::string mystr_;
	uint32_t nplain;

	void toString(std::string& str) const;
	void print(size_t i, std::ostream& out) const;

	selected(const selected&);
	const selected& operator=(const selected&);
    }; // class selected

    // an associative array for data partitions
    typedef std::map< const char*, part*, lessi > partList;

    /// A specialization of std::bad_alloc.
    class bad_alloc : public std::bad_alloc {
    public:
	bad_alloc(const char *m="unknown") throw() : mesg_(m) {};
	virtual ~bad_alloc() throw() {}
	virtual const char* what() const throw() {return mesg_;}

    private:
	/// Only contains a static string so as not to invoke any dynamic
	/// memory operations.
	const char *mesg_;
    }; // bad_alloc

    // Organize the miscellaneous functions under the name util.
    namespace util {
	/// charTable lists the 64 printable characters to be used for names
	/// charIndex maps the characters (ASCII) back to integer [0-64]
	extern const char* charTable;
	extern const short unsigned charIndex[];
	/// A mutex intended to be used for ensuring there is only one
	/// function that modifies the environment and other conditions.
	/// Currently used by the functions that generating user name,
	/// asking for password, backing up active tables, cleaning up the
	/// list of tables and optionally used to lock the variable
	/// ibis::fileManager::totalBytes.
	extern FASTBIT_DLLSPEC pthread_mutex_t envLock;

	/// Remove leading and trailing blank space
	inline char* trim(char* str);
	/// duplicate string content with C++ default new operator
	inline char* strnewdup(const char* s);
	inline char* strnewdup(const char* s, const uint32_t n);
	/// Remove trailing character 'tail' from str
	inline void removeTail(char* str, char tail);
	///@brief Treat all bytes in buf as the string.
	FASTBIT_DLLSPEC char* getString(const char* buf);
	///@brief Extract the next quoted string or the blank delimited string.
	FASTBIT_DLLSPEC void
	getString(std::string& str, const char*& buf, const char *delim=0);
	const char* getToken(char*& str, const char* tok_chrs);
	int readInt(int64_t& val, const char *&str, const char* del);
	int readDouble(double& val, const char *&str, const char* del);

	/// Generate a reasonably sized buffer for storing temporary
	/// contents.  Return the size of the buffer.  Caller is
	/// responsible for deleting the buffer.
	uint32_t getBuffer(char *& buf);

	/// Remove the content of named directory.  The directory itself is
	/// removed unless the second argument is true.
	void removeDir(const char* name, bool leaveDir=false);
	/// Recursively create the name directory.
	int makeDir(const char*dir);
	/// Return size of the file in bytes.  The value 0 is returned if
	/// file does not exist.
	FASTBIT_DLLSPEC off_t getFileSize(const char* name);
	/// Copy "from" to "to".  Return zero (0) on success, a negative
	/// number otherwise.
	int copy(const char* to, const char* from);

	/// Return the name of the user who is running the program.
	FASTBIT_DLLSPEC const char* userName();
	/// Return an integer that is always increasing.
	unsigned long uniqueNumber();
	/// Compute a denominator and numerator pair to compute a uniform
	/// distribution of numbers in a given range.
	void uniformFraction(const long unsigned idx,
			     long unsigned &denominator,
			     long unsigned &numerator);
	inline double rand(); ///< A pseudo-random number generator (0,1).

	///@{
	/// Fletcher's arithmetic checksum with 32-bit result.
	FASTBIT_DLLSPEC uint32_t checksum(const char* str, uint32_t sz);
	inline uint32_t checksum(uint32_t a, uint32_t b);
	inline std::string shortName(const std::string& longname);
	///@}

	///@{
	/// convert 32-bit integers to base-64 printable characters
	void int2string(std::string &str, unsigned val);
	void int2string(std::string &str, unsigned v1, unsigned v2);
	void int2string(std::string &str, unsigned v1,
			unsigned v2, unsigned v3);
	void int2string(std::string &str, const std::vector<unsigned>& val);
	///@}

	///@{
	/// Sorting RID lists.  The first two use quick sort and the third
	/// one use the insertion sort.  None of them are stable sort
	void sortRIDs(ibis::RIDSet& rids);
	void sortRIDs(ibis::RIDSet&, uint32_t, uint32_t);
	void isortRIDs(ibis::RIDSet&, uint32_t, uint32_t);
	///@}

	///@{
	/// Functions to handle manipulation of floating-point numbers.  The
	/// success of these functions is high sensitive to the definition
	/// of DBL_EPSILON.  It should be defined as the smallest value x
	/// such that (1+x) is different from x.  For 64-bit IEEE
	/// floating-point number, it is approximately 2.2E-16 (2^{-52})
	/// (May 2, 2001)
	inline double incrDouble(const double& in) {
	    double tmp = fabs(in) * DBL_EPSILON;
	    if (tmp > 0.0) tmp += in;
	    else tmp = in + DBL_MIN;
	    return tmp;
	}
	inline double decrDouble(const double& in) {
	    double tmp = fabs(in) * DBL_EPSILON;
	    if (tmp > 0.0) tmp = in - tmp;
	    else tmp = in - DBL_MIN;
	    return tmp;
	}
	inline void eq2range(const double& in, double& left, double& right) {
	    double tmp = fabs(in) * DBL_EPSILON;
	    if (tmp > 0.0) {right = in + tmp;}
	    else {right = in + DBL_MIN;}
	    left = in;
	}
	/// Reduce the decimal precision of the incoming floating-point
	/// value to specified precision. 
	inline double coarsen(const double in, const unsigned prec=2);
	/// Compute a compact 64-bit floating-point value with a short
	/// decimal representation in the range (@c left, @c right].  The
	/// shortest number is considered to be zero (0.0).
	double compactValue(double left, double right,
			    double start=0.0);

	/// Set a double to NaN.
	void setNaN(double& val);
	void setNaN(float& val);
	///@}

	/// Print a message to standard output.  The format string is same
	/// as printf.  A global mutex lock is used to ensure printed
	/// messages are ordered.
	FASTBIT_DLLSPEC void
	logMessage(const char* event, const char* fmt, ...);
	/// Retrieve the pointer to the log file.  The value of stdout will
	/// be returned if no log file was specified.  A log file name be
	/// specified through the following means (in the specified order),
	/// -- setLogFile
	/// -- environment variable FASTBITLOGFILE
	/// -- configuration parameter logfile
	FASTBIT_DLLSPEC FILE* getLogFile();
	/// Close the log file.
	FASTBIT_DLLSPEC int closeLogFile();
	/// Change the current log file to the named file.  Log files are
	/// opened in append mode, so the existing content will be
	/// preserved.  The log file will be changed only if the named file
	/// can be opened correctly.
	FASTBIT_DLLSPEC int setLogFileName(const char* filename);
	/// Return name the of the current log file.  Blank string or null
	/// string indicate standard output.
	FASTBIT_DLLSPEC const char* getLogFileName();

	/// Match the string @c str against a simple pattern @c pat.
	bool strMatch(const char* str, const char* pat);

	/// Return the current time in string format as @c asctime_r.
	/// The argument @c str must have 26 or more bytes and is used to
	/// carry the time output.
	void getLocalTime(char *str);
	void getGMTime(char *str);
	void secondsToString(const time_t, char *str);

#if defined(WIN32) && ! defined(__CYGWIN__)
	char* getpass_r(const char *prompt, char *buffer, uint32_t buflen);
	char* getpass(const char* prompt);
#else
	inline char *itoa(int value, char *str, int /* radix */) {
	    sprintf(str,"%d",value);
	    return str;
	}
#endif
	/// A global IO mutex lock.
	class ioLock {
	public:
	    ioLock() {
		int ierr = pthread_mutex_lock(&mutex);
		if (ierr != 0)
		    throw "ioLock failed to obtain a lock";
	    }
	    ~ioLock() {
		(void) pthread_mutex_unlock(&mutex);
	    }
	private:
	    // every instantiation of this class locks on the same mutex
	    static pthread_mutex_t mutex;

	    ioLock(const ioLock&) {}; // can not copy
	    const ioLock& operator=(const ioLock&);
	};

	/// An wrapper class for perform pthread_mutex_lock/unlock.
	class mutexLock {
	public:
	    mutexLock(pthread_mutex_t* lk, const char* m)
		: mesg(m), lock(lk) {
		if (ibis::gVerbose > 17)
		    logMessage("mutexLock", "acquiring lock (0x%lx) for %s",
			       lock, mesg);
		int ierr = pthread_mutex_lock(lock);
		if (ierr != 0) {
		    throw "mutexLock failed to obtain a mutex lock";
		}
	    }
	    ~mutexLock() {
		if (ibis::gVerbose > 17)
		    logMessage("mutexLock", "releasing lock (0x%lx) for %s",
			       lock, mesg);
		(void) pthread_mutex_unlock(lock);
	    }

	private:
	    const char* const mesg;
	    pthread_mutex_t* const lock;

	    mutexLock() : mesg(0), lock(0) {}; // no default constructor
	    mutexLock(const mutexLock&); // can not copy
	    const mutexLock& operator=(const mutexLock&);
	}; // mutexLock

	/// An wrapper class for perform pthread_mutex_lock/unlock.  Avoid
	/// invoking ibis::util::logMessage so it can be used inside
	/// ibis::util::logMessage.
	class quietLock {
	public:
	    quietLock(pthread_mutex_t* lk, const char* m)
		: mesg(m), lock(lk) {
		int ierr = pthread_mutex_lock(lock);
		if (ierr != 0) {
		    throw "quietLock failed to obtain a mutex lock";
		}
	    }
	    ~quietLock() {
		(void) pthread_mutex_unlock(lock);
	    }

	private:
	    const char* const mesg;
	    pthread_mutex_t* const lock;

	    quietLock() : mesg(0), lock(0) {}; // no default constructor
	    quietLock(const quietLock&); // can not copy
	    const quietLock& operator=(const quietLock&);
	}; // quietLock

	/// An wrapper class for perform pthread_rwlock_rdlock/unlock.
	class readLock {
	public:
	    readLock(pthread_rwlock_t* lk, const char* m)
		: mesg(m), lock(lk) {
		int ierr = pthread_rwlock_rdlock(lock);
		if (ierr != 0) {
		    throw "readLock failed to obtain a lock";
		}
	    }
	    ~readLock() {
		(void) pthread_rwlock_unlock(lock);
	    }

	private:
	    const char* const mesg;
	    pthread_rwlock_t* const lock;

	    readLock() : mesg(0), lock(0) {}; // no default constructor
	    readLock(const readLock&); // can not copy
	    const readLock& operator=(const readLock&);
	}; // readLock

	/// An wrapper class for perform pthread_rwlock_wrlock/unlock.
	class writeLock {
	public:
	    writeLock(pthread_rwlock_t* lk, const char* m)
		: mesg(m), lock(lk) {
		int ierr = pthread_rwlock_wrlock(lock);
		if (ierr != 0) {
		    throw "writeLock failed to obtain a lock";
		}
	    }
	    ~writeLock() {
		int ierr = pthread_rwlock_unlock(lock);
		if (ierr != 0) {
		    throw "writeLock failed to release the lock";
		}
	    }

	private:
	    const char* const mesg;
	    pthread_rwlock_t* const lock;

	    writeLock() : mesg(0), lock(0) {}; // no default constructor
	    writeLock(const writeLock&); // can not copy
	    const writeLock& operator=(const writeLock&);
	}; // writeLock

	/// A simple global counter.  Each time the operator() is called,
	/// it is incremented by 1.  Calls from different threads are
	/// serialized through a mutual exclusion lock.
	class FASTBIT_DLLSPEC counter {
	public:
	    ~counter() {pthread_mutex_destroy(&lock_);}
	    counter(const char *m="ibis::util::counter");

	    /// Return the current count and increment the count.
	    uint32_t operator()() {
		ibis::util::quietLock lck(&lock_, mesg_);
		uint32_t ret = count_;
		++ count_;
		return ret;
	    }
	    /// Reset count to zero.
	    void reset() {
		ibis::util::quietLock lck(&lock_, mesg_);
		count_ = 0;
	    }
	    /// Return the current count value.
	    uint32_t value() const {
		ibis::util::quietLock lck(&lock_, mesg_);
		return count_;
	    }

	private:
	    mutable pthread_mutex_t lock_; // the mutex lock
	    const char *mesg_;
	    uint32_t count_;

	    counter(const counter&);
	    const counter& operator=(const counter&);
	}; // counter

	/// A class for logging error messages.  The caller can use the
	/// function buffer to get a reference to std::ostream and write
	/// error messages to it.  Note that the message is formed in this
	/// buffer and written out in the destructor of this class.  There
	/// is a macro LOGGER that can simplify some of the routine stuff.
	/// Use function ibis::util::setLogFile to explicit name of the
	/// log file or use RC file entry logfile to specify a file name.
	/// By default all messages are dump to stdout.
	class FASTBIT_DLLSPEC logger {
	public:
	    /// Constructor.  The argument to this constructor is taken to
	    /// be the number of spaces before the message.  Note, if a
	    /// time stamp is given, it is always given before the spaces.
	    /// The number of leading blanks is usually the verboseness
	    /// level.  Typically, a message is formed only if the global
	    /// verboseness level is higher than the level assigned to the
	    /// particular message (through the use of LOGGER macro or
	    /// explicit if statements).  In addition, the message is only
	    /// outputed if the global verboseness level is no less than 0.
	    logger(int blanks=0);
	    /// Destructor.  Output the message from this function.
	    ~logger();
	    /// Retrun an output stream for caller to build a message.
	    std::ostream& buffer() {return mybuffer;}

	protected:
	    /// The message is stored in this buffer.
	    std::ostringstream mybuffer;

	private:
	    logger(const logger&);
	    logger& operator=(const logger&);
	}; // logger
    } // namespace util
} // namespace ibis

#if defined(WIN32) && ! defined(__CYGWIN__)
char* getpass(const char* prompt);
#endif

/// A Linear Congruential Generator of pseudo-random numbers.
/// The integer variable @c seed is always an odd number.  Don't use it
/// directly.
inline double ibis::util::rand() {
    static uint32_t seed = 1;
    static const uint32_t alpha = 69069;
    static const double scale = ::pow(0.5, 32);
    seed = static_cast<uint32_t>(seed * alpha);
    return(scale * seed);
} // ibis::util::rand

/// Fletcher's checksum on two integers. Returns an integer.
inline uint32_t ibis::util::checksum(uint32_t a, uint32_t b) {
    uint32_t a0 = (a >> 16);
    uint32_t a1 = (a & 0xFFFF);
    uint32_t b0 = (b >> 16);
    uint32_t b1 = (b & 0xFFFF);
    return ((((a0<<2)+a1*3+(b0<<1)+b1) << 16) | ((a0+a1+b0+b1) & 0xFFFF));
} // ibis::util::checksum

/// Same as strdup() but uses 'new' instead of 'malloc' and if s == 0 then
/// the return 0
inline char* ibis::util::strnewdup(const char* s) {
    char* str = 0;
    if (s!=0 && *s!=static_cast<char>(0)) {
	str = new char[strlen(s)+1];
	strcpy(str, s);
    }
    return str;
} // ibis::util::strnewdup

inline char* ibis::util::strnewdup(const char* s, const uint32_t n) {
    char* str = 0;
    if (n > 0 && s != 0 && *s != static_cast<char>(0)) {
	uint32_t len = strlen(s);
	if (n < len)
	    len = n;
	str = new char[len+1];
	strncpy(str, s, len);
	str[len] = 0;
    }
    return str;
} // ibis::util::strnewdup

// remove all the trailing char 'tail'
inline void ibis::util::removeTail(char* str, char tail) {
    uint32_t j = strlen(str);
    while (j > 0 && str[j-1] == tail) {
	-- j;
	str[j] = static_cast<char>(0);
    }
} // ibis::util::removeTail

// remove the leading and trailing space of the incoming string
inline char* ibis::util::trim(char* str) {
    char* head = 0;
    if (str == 0) return head;
    if (*str == 0) return head;

    head = str;
    char* tail = str + strlen(str) - 1;
    while (*head) {
	if (isspace(*head))
	    ++head;
	else
	    break;
    }
    while (tail>=head && isspace(*tail)) {
	*tail=static_cast<char>(0);
	--tail;
    }
    return head;
} // ibis::util::trim

// reduce the precision of the incoming floating-point value
inline double ibis::util::coarsen(const double in, const unsigned prec) {
    if (in == 0.0) return in;
    if (prec > 15) return in;

    double tmp = floor(log10(fabs(in)));
    if (tmp < -307) {
	tmp = 0.0;
    }
    else if (! (tmp < 309)) {
	tmp = in;
    }
    else {
	const double xp = pow(1e1, tmp - (prec > 1 ? prec-1 : 0));
	tmp = floor(in / xp + 0.5) * xp;
    }
    return tmp;
} // ibis::util::coarsen

inline std::string ibis::util::shortName(const std::string& de) {
    std::string tn;
    uint32_t tmp = ibis::util::checksum(de.c_str(), de.size());
    ibis::util::int2string(tn, tmp);
    std::swap(tn[0], tn[5]);
    if (! isalpha(tn[0]))
	tn[0] = 'A' + (tn[0] % 26);
    return tn;
} // ibis::util::shortName

/// Print a rid_t to an output stream.
inline std::ostream& operator<<(std::ostream& out, const ibis::rid_t& rid) {
    out << '(' << rid.num.run << ", " << rid.num.event << ')';
    return out;
}

/// Read a rid_t from an input stream.
inline std::istream& operator>>(std::istream& is, ibis::rid_t& rid) {
    char c = 0;
    is >> c;
    if (c == '(') { // (runNumber, EventNumber)
	is >> rid.num.run >> c;
	if (c == ',')
	    is >> rid.num.event >> c;
	else
	    rid.num.event = 0;
	if (c != ')')
	    is.clear(std::ios::badbit); // forget the erro
    }
    else { // runNumber, EventNumber
	is.putback(c);
	is >> rid.num.run >> c;
	if (c != ',') // assume space separator
	    is.putback(c);
	is >> rid.num.event;
    }
    return is;
}
#endif // IBIS_UTIL_H

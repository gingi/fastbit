// File: $Id$
// Author: John Wu <John.Wu at acm.org> Lawrence Berkeley National Laboratory
// Copyright 2000-2009 the Regents of the University of California
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
#elif defined(HAVE_STDLIB_H)
#include <stdlib.h>
#elif defined(unix) || defined(linux) || defined(__APPLE__) || defined(__FreeBSD) || defined(__CYGWIN__)
#include <stdlib.h>
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
#include <string>	// std::string

#include <limits>	// std::numeric_limits
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

#if !defined(HAVE_GCC_ATOMIC32) && defined(WITHOUT_FASTBIT_CONFIG_H)
#if __GNUC__+0 >= 4 && !defined(__CYGWIN__) && !defined(__PATHCC__) && !defined(__APPLE__)
#define HAVE_GCC_ATOMIC32 2
#endif
#endif
#if !defined(HAVE_GCC_ATOMIC64) && defined(WITHOUT_FASTBIT_CONFIG_H)
#if defined(__IA64__) || defined(__x86_64__) || defined(__ppc64__)
#if __GNUC__+0 >= 4 && !defined(__CYGWIN__) && !defined(__PATHCC__) && !defined(__APPLE__)
#define HAVE_GCC_ATOMIC64 2
#endif
#endif
#endif

// mapping the names of the low level IO functions defined in <unistd.h>
#if defined(_MSC_VER) && defined(_WIN32)
#define UnixOpen  ::_open
#define UnixClose ::_close
#define UnixRead  ::_read
#define UnixWrite ::_write
#define UnixSeek  ::_lseek
#define UnixFlush  ::_commit
#define UnixSnprintf ::_snprintf
#define UnixStat  ::_stat
#define UnixFStat ::_fstat
#define Stat_T    struct _stat
#else
#define UnixOpen  ::open
#define UnixClose ::close
#define UnixRead  ::read
#define UnixWrite ::write
#define UnixSeek  ::lseek
#define UnixFlush ::fsync
#define UnixSnprintf ::snprintf
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
// define the option for opening an existing file for appending only
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

#if defined(_WIN32) && defined(_MSC_VER)
// needed for numeric_limits<>::max, min function calls
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

#define LOGGER(v) \
if (false == (v)) ; else ibis::util::logger(0).buffer() 

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
    /// @defgroup FastBitIBIS FastBit IBIS implementation core objects.
    /// @{
    class resource;	///< To store configuration parameters.
    class bitvector;	///< To store one bit sequence/bitmap.
    class column;	///< One column/attribute of a table.
    class fileManager;	///< A simple file manager.
    class horometer;	///< A timer class.
    class index;	///< The base class of indices.
    class roster;	///< A projection of a column in ascending order.
    class bitvector64;	///< The 64-bit version of bitvector class.

    // col with (selected) values -- need to handle bundles
    class bundle;
    class colValues;
    /// @}

    typedef std::vector<colValues*> colList;

    /// The reference to the global configuration parameters.
    FASTBIT_CXX_DLLSPEC ibis::resource& gParameters();

    /// A data structure to store a small set of names.  The names are
    /// sorted in case-insensitive order.
    class FASTBIT_CXX_DLLSPEC nameList {
    public:
	nameList() : cstr(0), buff(0) {};
	nameList(const char* str) : cstr(0), buff(0) {select(str);}
	~nameList() {if (cstr) clear();}

	bool empty() const {return cstr == 0;}
	const char* operator*() const {return cstr;};
	uint32_t size() const {return cvec.size();};

	/// Replace existing content with these names.  Remove existing names.
	void select(const char* str);
	/// Add more names to the list.  Keep existing content.
	void add(const char* str);
	/// Find the order of the @c key in the list.  If the @c key is in
	/// the list it returns the position of the @c key, otherwise it
	/// returns the size of the name list.
	uint32_t find(const char* key) const;

	const char* operator[](uint32_t i) const {return cvec[i];}
	typedef std::vector< const char* >::const_iterator const_iterator;
	const_iterator begin() const {return cvec.begin();}
	const_iterator end() const {return cvec.end();}

	void clear()
	{cvec.clear(); delete [] cstr; delete [] buff; buff=0; cstr=0;}

    private:
	typedef std::vector< const char * > compStore;
	char* cstr;	// copy of the component string
	char* buff;	// same as cstr, but delimiter is \0
	compStore cvec;	// contains pointers to buff, for easier access

	nameList(const nameList&);
	nameList& operator=(const nameList&);
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
	uint32_t size() const {return names.size();}
	uint32_t nPlain() const {return nplain;}

	/// Parse the select clause.  By default, place the functions last.
	void select(const char *str, bool sort=true);
	void select(const std::vector<const char*>& nl, bool sort=true);
	/// Return the list of names stored internally.
	const std::vector<std::string>& getNames() const {return names;}
	/// Return the first occurrence of the string.  Returns the value of
	/// @c size if the given @c key in not in the list of selected
	/// components.
	uint32_t find(const char *key) const;
	void clear() {
	    names.clear(); functions.clear(); mystr_.erase(); nplain=0;}
	/// Remove the entries specified.
	void remove(const std::vector<uint32_t>& ents);

	/// Output a stringlized version of the select clause.
	const char* operator*() const {return mystr_.c_str();}

	/// Access the ith column name of the select clause.  In case of a
	/// function, it returns the name of the argument rather than the
	/// whole function.  This operator is only intended to be used to
	/// extract the column values.
	const char* getName(uint32_t i) const {return names[i].c_str();};
	const char* operator[](uint32_t i) const {return names[i].c_str();};
	/// Return all unique column names.
	std::string uniqueNames() const;
	/// Return the ith term, with the function name.
	std::string getTerm(uint32_t i) const;

	/// An iterator through the column names of the select clause.
	typedef std::vector<std::string>::const_iterator const_iterator;
	const_iterator begin() const {return names.begin();}
	const_iterator end() const {return names.end();}

	enum FUNCTION {NIL, AVG, MAX, MIN, SUM};
	FUNCTION getFunction(uint32_t i) const {return functions[i];}

    private:
	std::vector<std::string> names;
	std::vector<FUNCTION> functions;
	std::string mystr_;
	uint32_t nplain;

	void toString(std::string& str) const;
	void print(uint32_t i, std::ostream& out) const;

	selected(const selected&);
	selected& operator=(const selected&);
    }; // class selected

    /// An associative array for data partitions.  Only used internally for
    /// sorting data partitions.
    typedef std::map< const char*, part*, lessi > partAssoc;

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

    /// Organize the miscellaneous functions under the name util.
    namespace util {
	/// charTable lists the 64 printable characters to be used for names
	extern const char* charTable;
	/// charIndex maps the characters (ASCII) back to integer [0-64]
	extern const short unsigned charIndex[];
	/// Delimiters used to separate a string of names. @sa ibis::nameList
	extern const char* delimiters;
	/// A mutex for serialize operations FastBit wide.  Currently it is
	/// used by the functions that generating user name, asking for
	/// password, backing up active tables, cleaning up the list of
	/// tables.  It is also used extensively in the implementation of C
	/// API functions to ensure the cache maintained for C users are
	/// manipulated by one user at a time.
	extern FASTBIT_CXX_DLLSPEC pthread_mutex_t envLock;

	/// Remove leading and trailing blank space.
	inline char* trim(char* str);
	/// Duplicate string content with C++ default new operator.
	inline char* strnewdup(const char* s);
	/// Duplicate no more than n characters.
	inline char* strnewdup(const char* s, const uint32_t n);
	/// Remove trailing character 'tail' from str.
	inline void removeTail(char* str, char tail);
	/// Treat all bytes in buf as the string.
	FASTBIT_CXX_DLLSPEC char* getString(const char* buf);
	/// Extract the next quoted string or the blank delimited string.
	FASTBIT_CXX_DLLSPEC void
	getString(std::string& str, const char*& buf, const char *delim=0);
	const char* getToken(char*& str, const char* tok_chrs);
	int readInt(int64_t& val, const char *&str, const char* del);
	int readDouble(double& val, const char *&str, const char* del);

	/// Remove the content of named directory.  The directory itself is
	/// removed unless the second argument is true.
	void removeDir(const char* name, bool leaveDir=false);
	/// Recursively create the name directory.
	int makeDir(const char*dir);
	/// Return size of the file in bytes.  The value 0 is returned if
	/// file does not exist.
	FASTBIT_CXX_DLLSPEC off_t getFileSize(const char* name);
	/// Copy "from" to "to".  Return zero (0) on success, a negative
	/// number otherwise.
	int copy(const char* to, const char* from);

	/// Return the user name.
	FASTBIT_CXX_DLLSPEC const char* userName();
	/// Return an integer that is always increasing.
	uint32_t serialNumber();
	/// Compute a denominator and numerator pair to compute a uniform
	/// distribution of numbers in a given range.
	void uniformFraction(const long unsigned idx,
			     long unsigned &denominator,
			     long unsigned &numerator);
	inline double rand(); ///< A pseudo-random number generator (0,1).

	///@{
	/// Fletcher's arithmetic checksum with 32-bit result.
	FASTBIT_CXX_DLLSPEC uint32_t checksum(const char* str, uint32_t sz);
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

	/// Functions to handle manipulation of floating-point numbers.
	///@{
	double incrDouble(const double&);
	double decrDouble(const double&);
	void   eq2range(const double&, double&, double&);
	/// Reduce the decimal precision of the incoming floating-point
	/// value to specified precision. 
	inline double coarsen(const double in, const unsigned prec=2);
	/// Compute a compact 64-bit floating-point value with a short
	/// decimal representation.
	double compactValue(double left, double right,
			    double start=0.0);

	/// Compute a compact 64-bit floating-point value with a short
	/// binary representation.
	double compactValue2(double left, double right,
			     double start=0.0);

	/// Set a double to NaN.
	void setNaN(double& val);
	void setNaN(float& val);

	/// Round the incoming value to the largest output value that is no
	/// more than the input.  Both Tin and Tout must be elementary data
	/// types, and Tout must be an elementary integral type.
	template <typename Tin, typename Tout>
	void round_down(const Tin& inval, Tout& outval) {
	    outval = ((double)std::numeric_limits<Tout>::min() > inval ?
		      std::numeric_limits<Tout>::min() :
		      (double)std::numeric_limits<Tout>::max() <= inval ?
		      std::numeric_limits<Tout>::max() :
		      static_cast<Tout>(inval));
	}
	/// Round the incoming value to the smallest output value that is
	/// no less than the input.  Both Tin and Tout must be elementary
	/// data types, and Tout must be an elementary integral type.
	template <typename Tin, typename Tout>
	void round_up(const Tin& inval, Tout& outval) {
	    outval = ((double)std::numeric_limits<Tout>::min() >= inval ?
		      std::numeric_limits<Tout>::min() :
		      (double) std::numeric_limits<Tout>::max() < inval ?
		      std::numeric_limits<Tout>::max() :
		      static_cast<Tout>(inval) +
		      ((inval-static_cast<Tin>(static_cast<Tout>(inval))) > 0));
	}
	/// A specialization of round_up for the output type float.
	template <typename Tin>
	void round_up(const Tin& inval, float&);
	/// A specialization of round_up for the output in double.
	template <typename Tin>
	void round_up(const Tin& inval, double& outval) {
	    outval = static_cast<double>(inval);
	}
	///@}

	/// Print a message to standard output.  The format string is same
	/// as printf.  A global mutex lock is used to ensure printed
	/// messages are ordered.
	FASTBIT_CXX_DLLSPEC void
	logMessage(const char* event, const char* fmt, ...);
	/// Retrieve the pointer to the log file.  The value of stdout will
	/// be returned if no log file was specified.  A log file name be
	/// specified through the following means (in the specified order),
	/// -- setLogFile
	/// -- environment variable FASTBITLOGFILE
	/// -- configuration parameter logfile
	FASTBIT_CXX_DLLSPEC FILE* getLogFile();
	/// Close the log file.
	FASTBIT_CXX_DLLSPEC int closeLogFile();
	/// Change the current log file to the named file.  Log files are
	/// opened in append mode, so the existing content will be
	/// preserved.  The log file will be changed only if the named file
	/// can be opened correctly.
	FASTBIT_CXX_DLLSPEC int setLogFileName(const char* filename);
	/// Return name the of the current log file.  Blank string or null
	/// string indicate standard output.
	FASTBIT_CXX_DLLSPEC const char* getLogFileName();

	/// Match the string @c str against a simple pattern @c pat.  The
	/// pattern may use two wild characters defined for SQL function
	/// LIKE, '_' and '%'.
	FASTBIT_CXX_DLLSPEC bool strMatch(const char* str, const char* pat);

	/// Compute the outer product of @c a and @c b, add the result to @c c.
	const ibis::bitvector64& outerProduct(const ibis::bitvector& a,
					      const ibis::bitvector& b,
					      ibis::bitvector64& c);
	/// Add the strict upper triangular portion of the outer production
	/// between @c a and @c b to @c c.
	const ibis::bitvector64& outerProductUpper(const ibis::bitvector& a,
						   const ibis::bitvector& b,
						   ibis::bitvector64& c);

	/// Intersect two sets of bit vectors.
	long intersect(const std::vector<ibis::bitvector> &bits1,
		       const std::vector<ibis::bitvector> &bits2,
		       std::vector<ibis::bitvector> &res);
	/// Intersect three sets of bit vectors.
	long intersect(const std::vector<ibis::bitvector> &bits1,
		       const std::vector<ibis::bitvector> &bits2,
		       const std::vector<ibis::bitvector> &bits3,
		       std::vector<ibis::bitvector> &res);
	/// Deallocate the bit vectors.
	void clean(std::vector<ibis::bitvector*> &bv) throw();
	/// Deallocate the list of data partitions.
	void clean(ibis::partList &pl) throw();
	/// Return the current time in string format as @c asctime_r.
	void getLocalTime(char *str);
	/// Return the current GMT time in string format.
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
	    ioLock& operator=(const ioLock&);
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
		    throw "mutexLock failed to obtain a lock";
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
	    mutexLock& operator=(const mutexLock&);
	}; // mutexLock

	/// An wrapper class for perform pthread_mutex_lock/unlock.  Avoid
	/// invoking ibis::util::logMessage so it can be used inside
	/// ibis::util::logMessage.
	class quietLock {
	public:
	    quietLock(pthread_mutex_t* lk) : lock(lk) {
		int ierr = pthread_mutex_lock(lock);
		if (ierr != 0) {
		    throw "quietLock failed to obtain a mutex lock";
		}
	    }
	    ~quietLock() {
		(void) pthread_mutex_unlock(lock);
	    }

	private:
	    pthread_mutex_t* const lock;

	    quietLock(); // no default constructor
	    quietLock(const quietLock&); // can not copy
	    quietLock& operator=(const quietLock&);
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
	    readLock& operator=(const readLock&);
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
	    writeLock& operator=(const writeLock&);
	}; // writeLock

	/// A simple shared counter.  Each time the operator() is called,
	/// it is incremented by 1.  Calls from different threads are
	/// serialized through a mutual exclusion lock or atomic
	/// operations.  Currently, it only knows about atomic operations
	/// provided by GCC and visual studio on WIN32.  The GCC automic
	/// functions are determined in the configure script.
	class FASTBIT_CXX_DLLSPEC counter {
	public:
	    ~counter() {
#if defined(HAVE_GCC_ATOMIC32)
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
#else
		(void)pthread_mutex_destroy(&lock_);
#endif
	    }
	    counter() : count_(0) {
#if defined(HAVE_GCC_ATOMIC32)
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
#else
		if (0 != pthread_mutex_init(&lock_, 0))
		    throw ibis::bad_alloc
			("ibis::util::counter failed to initialize mutex lock");
#endif
	    }

	    /// Return the current count and increment the count.
	    uint32_t operator()() {
#if defined(HAVE_GCC_ATOMIC32)
		return __sync_fetch_and_add(&count_, 1);
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
		return InterlockedIncrement((volatile long *)&count_)-1;
#else
		ibis::util::quietLock lck(&lock_);
		uint32_t ret = count_;
		++ count_;
		return ret;
#endif
	    }
	    /// Reset count to zero.
	    void reset() {
#if defined(HAVE_GCC_ATOMIC32)
		(void) __sync_fetch_and_sub(&count_, count_);
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
		(void) InterlockedExchange((volatile long *)&count_, 0);
#else
		ibis::util::quietLock lck(&lock_);
		count_ = 0;
#endif
	    }
	    /// Return the current count value.
	    uint32_t value() const {
		return count_;
	    }

	private:
#if defined(HAVE_GCC_ATOMIC32)
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
#else
	    mutable pthread_mutex_t lock_; ///< The mutex lock.
#endif
	    volatile uint32_t count_; ///< The counter value.

	    /// Copy constructor.  Decleared but not implemented.
	    counter(const counter&);
	    /// Assignment operator.  Decleared but not implemented.
	    counter& operator=(const counter&);
	}; // counter

	/// A shared integer class.  Multiply threads may safely perform
	/// different operations on this integer at the same time.  It
	/// serializes the operations by using the atomic operations
	/// provided by GCC extension.  The availability of automic
	/// operations is indicated by whether or not the compiler macro
	/// HAVE_GCC_ATOMIC32 is defined.  If the atomic extension is not
	/// available, it falls back on the mutual exclusion lock provided
	/// by pthread library.
	///
	/// @note The overhead of using mutual exclusion lock is large.  In
	/// one test that acquires and release three locks a million time
	/// each, using the locks took about 10 seconds, while using the
	/// atomic extension to perform the same arithmetic operations took
	/// about 0.1 seconds.
	class FASTBIT_CXX_DLLSPEC sharedInt32 {
	public:
	    sharedInt32() : val_(0) {
#if defined(HAVE_GCC_ATOMIC32)
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
#else
		if (pthread_mutex_init(&mytex, 0) != 0)
		    throw "pthread_mutex_init failed for sharedInt";
#endif
	    }

	    ~sharedInt32() {
#if defined(HAVE_GCC_ATOMIC32)
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
#else
		(void)pthread_mutex_destroy(&mytex);
#endif
	    }

	    /// Read the current value.
	    uint32_t operator()() const {return val_;}

	    /// Increment operator.
	    uint32_t operator++() {
#if defined(HAVE_GCC_ATOMIC32)
		return __sync_add_and_fetch(&val_, 1);
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
		return InterlockedIncrement((volatile long *)&val_);
#else
		ibis::util::quietLock lock(&mytex);
		++ val_;
		return val_;
#endif
	    }

	    /// Decrement operator.
	    uint32_t operator--() {
#if defined(HAVE_GCC_ATOMIC32)
		return __sync_add_and_fetch(&val_, -1);
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
		return InterlockedDecrement((volatile long *)&val_);
#else
		ibis::util::quietLock lock(&mytex);
		-- val_;
		return val_;
#endif
	    }

	    /// In-place addition operator.
	    void operator+=(const uint32_t rhs) {
#if defined(HAVE_GCC_ATOMIC32)
		(void) __sync_add_and_fetch(&val_, rhs);
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
		(void) InterlockedExchangeAdd((volatile long *)&val_, rhs);
#else
		ibis::util::quietLock lock(&mytex);
		val_ += rhs;
#endif
	    }

	    /// In-place subtraction operator.
	    void operator-=(const uint32_t rhs) {
#if defined(HAVE_GCC_ATOMIC32)
		(void) __sync_sub_and_fetch(&val_, rhs);
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
		(void) InterlockedExchangeAdd((volatile long *)&val_,
					      -(long)rhs);
#else
		ibis::util::quietLock lock(&mytex);
		val_ -= rhs;
#endif
	    }

	    /// Swap the contents of two integer variables.
	    void swap(sharedInt32 &rhs) {
		uint32_t tmp = rhs.val_;
		rhs.val_ = val_;
		val_ = tmp;
	    }

	private:
	    uint32_t volatile val_; ///< The actual integer value.
#if defined(HAVE_GCC_ATOMIC32)
#elif _MSC_VER+0 >= 1500 && defined(_WIN32)
#else
	    pthread_mutex_t mytex; ///< The mutex for this object.
#endif

	    sharedInt32(const sharedInt32&); // no copy constructor
	    sharedInt32& operator=(const sharedInt32&); // no assignment
	}; // sharedInt32

	/// A 64-bit shared integer class.  It allows multiple threads to
	/// safely operate on the integer through the use of the atomic
	/// operations provided by GCC extension.  If the atomic extension
	/// is not available, it falls back on the mutual exclusion lock
	/// provided by pthread library.
	/// @sa ibis::util::sharedInt32
	class sharedInt64 {
	public:
	    sharedInt64() : val_(0) {
#if defined(HAVE_GCC_ATOMIC64)
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
#else
		if (pthread_mutex_init(&mytex, 0) != 0)
		    throw "pthread_mutex_init failed for sharedInt";
#endif
	    }

	    ~sharedInt64() {
#if defined(HAVE_GCC_ATOMIC64)
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
#else
		(void)pthread_mutex_destroy(&mytex);
#endif
	    }

	    /// Read the current value.
	    uint64_t operator()() const {return val_;}

	    /// Increment operator.
	    uint64_t operator++() {
#if defined(HAVE_GCC_ATOMIC64)
		return __sync_add_and_fetch(&val_, 1);
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
		return InterlockedIncrement64((volatile LONGLONG *)&val_);
#else
		ibis::util::quietLock lock(&mytex);
		++ val_;
		return val_;
#endif
	    }

	    /// Decrement operator.
	    uint64_t operator--() {
#if defined(HAVE_GCC_ATOMIC64)
		return __sync_add_and_fetch(&val_, -1);
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
		return InterlockedDecrement64((volatile LONGLONG *)&val_);
#else
		ibis::util::quietLock lock(&mytex);
		-- val_;
		return val_;
#endif
	    }

	    /// In-place addition operator.
	    void operator+=(const uint64_t rhs) {
#if defined(HAVE_GCC_ATOMIC64)
		(void) __sync_add_and_fetch(&val_, rhs);
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
		(void) InterlockedExchangeAdd64((volatile LONGLONG *)&val_,
						rhs);
#else
		ibis::util::quietLock lock(&mytex);
		val_ += rhs;
#endif
	    }

	    /// In-place subtraction operator.
	    void operator-=(const uint64_t rhs) {
#if defined(HAVE_GCC_ATOMIC64)
		(void) __sync_sub_and_fetch(&val_, rhs);
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
		(void) InterlockedExchangeAdd64((volatile LONGLONG *)&val_,
						-(long)rhs);
#else
		ibis::util::quietLock lock(&mytex);
		val_ -= rhs;
#endif
	    }

	    /// Swap the contents of two integer variables.
	    void swap(sharedInt64 &rhs) {
		uint64_t tmp = rhs.val_;
		rhs.val_ = val_;
		val_ = tmp;
	    }

	private:
	    uint64_t volatile val_; ///< The actual integer value.
#if defined(HAVE_GCC_ATOMIC64)
#elif _MSC_VER+0 >= 1500 && (WINVER+0 >= 0x0600 || (defined(NTDDI_VISTA) && NTDDI_VERSION >= NTDDI_VISTA))
#else
	    pthread_mutex_t mytex; ///< The mutex for this object.
#endif

	    sharedInt64(const sharedInt64&); // no copy constructor
	    sharedInt64& operator=(const sharedInt64&); // no assignment
	}; // sharedInt64

	/// A class for logging messages.  The caller writes message to a
	/// std::ostream returned by the function buffer as if to
	/// std::cout.  Note that messages are stored in this buffer and
	/// written out in the destructor of this class.  There is a macro
	/// LOGGER that can simplify some of the routine stuff.  Use
	/// function ibis::util::setLogFile to explicit name of the log
	/// file or use RC file entry logfile to specify a file name.  By
	/// default all messages are dump to stdout.
	class FASTBIT_CXX_DLLSPEC logger {
	public:
	    /// Constructor.
	    logger(int blanks=0);
	    /// Destructor.
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

	/// Print simply-formated timing information.  It starts the clock
	/// in the constructor, stops the clock in the destructor, and
	/// reports the CPU time and elapsed time in between.  Typically
	/// one would declare an object of this class in a block of code,
	/// and let the object be cleaned up by compiler generated code at
	/// the end of its scope.  Upon destruction of this object, it
	/// prints its lifespan.  To distiguish the different time
	/// durations, the user should provide a meaningful description to
	/// the constructor.
	class timer {
	public:
	    explicit timer(const char* msg, int lvl=1);
	    ~timer();

	private:
	    ibis::horometer *chrono_; ///< The actual timer object.
	    std::string mesg_; ///< Holds a private copy of the message.

	    timer(); // no default constructor
	    timer(const timer&); // no copying
	    timer& operator=(const timer&); // no assignment
	}; // timer

	/// A class hierarchy for clean up after durable resources.  It is
	/// similar in spirit to Loki::ScopeGuard, but simpler.
	class guardBase {
	public:
	    /// Tell the guard that the user has manually invoked the
	    /// cleanup function.
	    void dismiss() const {done_ = true;}

	protected:
	    mutable volatile bool done_;

	    ~guardBase() {}; ///< Destructor.  No need to be virtual.
	    guardBase() : done_(false) {}; ///< Default constructor.
	    /// Copy constructor.
	    guardBase(const guardBase& rhs) : done_(rhs.done_) {
		rhs.dismiss();
	    }

	    /// A template function to absorb all exceptions.
	    template <typename T>
	    static void cleanup(T& task) throw () {
		if (task.done_)
		    return;

		try {
		    task.execute();
		}
		catch (const std::exception& e) {
		    LOGGER(ibis::gVerbose > 1)
			<< " ... caught a std::exception (" << e.what()
			<< ") in util::gard";
		}
		catch (const char* s) {
		    LOGGER(ibis::gVerbose > 1)
			<< " ... caught a string exception (" << s
			<< ") in util::guard";
		}
		catch (...) {
		    LOGGER(ibis::gVerbose > 1)
			<< " ... caught a unknown exception in util::guard";
		}
		task.done_ = true;
	    }
	}; // guardBase

	/// The type to be used by client code.  User code uses type
	/// ibis::util::guard along with the overloaded function
	/// ibis::util::makeGuard, as in
	/// @code
	/// ibis::util::guard myguard = ibis::util::makeGuard...;
	/// @endcode
	typedef const guardBase& guard;

	/// A concrete class for cleanup jobs that take a function without
	/// any argument.
	template <typename F>
	class guardImpl0 : public guardBase {
	public:
	    static guardImpl0<F> makeGuard(F f) {
		return guardImpl0<F>(f);
	    }

	    /// Destructor calls the cleanup function of the base class.
	    ~guardImpl0() {cleanup(*this);}

	protected:
	    friend class guardBase; // to call function execute
	    void execute() {fun_();}

	    /// Construct a guard object from a function.
	    explicit guardImpl0(F f) : fun_(f) {}

	private:
	    /// Copy of the function pointer.
	    F fun_;

	    guardImpl0();
	    //guardImpl0(const guardImpl0&);
	    guardImpl0& operator=(const guardImpl0&);
	}; // guardImpl0

	template <typename F>
	inline guardImpl0<F> makeGuard(F f) {
	    return guardImpl0<F>::makeGuard(f);
	}

	/// A concrete class for cleanup jobs that take a function with one
	/// argument.
	template <typename F, typename A>
	class guardImpl1 : public guardBase {
	public:
	    static guardImpl1<F, A> makeGuard(F f, A a) {
		return guardImpl1<F, A>(f, a);
	    }

	    /// Destructor calls the cleanup function of the base class.
	    ~guardImpl1() {cleanup(*this);}

	protected:
	    friend class guardBase; // to call function execute
	    void execute() {fun_(arg_);}

	    /// Construct a guard object from a function.
	    explicit guardImpl1(F f, A a) : fun_(f), arg_(a) {}

	private:
	    /// The function pinter.
	    F fun_;
	    /// The argument to the function.
	    A arg_;

	    guardImpl1();
	    //guardImpl1(const guardImpl1&);
	    guardImpl1& operator=(const guardImpl1&);
	}; // guardImpl1

	template <typename F, typename A>
	inline guardImpl1<F, A> makeGuard(F f, A a) {
	    return guardImpl1<F, A>::makeGuard(f, a);
	}
    } // namespace util
} // namespace ibis

#if defined(WIN32) && ! defined(__CYGWIN__)
char* getpass(const char* prompt);
#endif

/// A Linear Congruential Generator of pseudo-random numbers.  It produces
/// a floating-point in the range of [0, 1).  It is very simple, but does
/// not produce high-quality random numbers, nor is it thread-safe.
inline double ibis::util::rand() {
    /// The internal variable @c seed is always an odd number.  Don't use
    /// it directly.
    static uint32_t seed = 1;
    static const uint32_t alpha = 69069;
    static const double scale = ::pow(0.5, 32);
    seed = static_cast<uint32_t>(seed * alpha);
    return(scale * seed);
} // ibis::util::rand

/// Fletcher's checksum on two integers.  Returns an integer.
inline uint32_t ibis::util::checksum(uint32_t a, uint32_t b) {
    uint32_t a0 = (a >> 16);
    uint32_t a1 = (a & 0xFFFF);
    uint32_t b0 = (b >> 16);
    uint32_t b1 = (b & 0xFFFF);
    return ((((a0<<2)+a1*3+(b0<<1)+b1) << 16) | ((a0+a1+b0+b1) & 0xFFFF));
} // ibis::util::checksum

/// Increment the input value to the next larger value.  If the math
/// library has nextafter, it will use nextafter, otherwise, it will use
/// the unit round-off error to compute the next larger value.  The success
/// of this computation is high sensitive to the definition of DBL_EPSILON.
/// It should be defined as the smallest value x such that (1+x) is
/// different from x.  For 64-bit IEEE floating-point number, it is
/// approximately 2.2E-16 (2^{-52}) (May 2, 2001)
inline double ibis::util::incrDouble(const double& in) {
#if defined(HAVE_NEXTAFTER)
    return nextafter(in, DBL_MAX);
#elif defined(_MSC_VER) && defined(_WIN32)
    return _nextafter(in, DBL_MAX);
#else
    double tmp = fabs(in) * DBL_EPSILON;
    if (tmp > 0.0) tmp += in;
    else tmp = in + DBL_MIN;
    return tmp;
#endif
}

/// Decrease the input value to the next smaller value.
/// @sa ibis::util::incrDouble
inline double ibis::util::decrDouble(const double& in) {
#if defined(HAVE_NEXTAFTER)
    return nextafter(in, -DBL_MAX);
#elif defined(_MSC_VER) && defined(_WIN32)
    return _nextafter(in, -DBL_MAX);
#else
    double tmp = fabs(in) * DBL_EPSILON;
    if (tmp > 0.0) tmp = in - tmp;
    else tmp = in - DBL_MIN;
    return tmp;
#endif
}

/// Generate a range [left, right) that contains exactly the input value
/// in.  This is used to transform an expression expression "A = in" into
/// "left <= A < right".
/// @sa ibis::util::incrDouble
inline void ibis::util::eq2range(const double& in,
				 double& left, double& right) {
#if defined(HAVE_NEXTAFTER)
    right = nextafter(in, DBL_MAX);
#elif defined(_MSC_VER) && defined(_WIN32)
    right = _nextafter(in, DBL_MAX);
#else
    double tmp = fabs(in) * DBL_EPSILON;
    if (tmp > 0.0) {right = in + tmp;}
    else {right = in + DBL_MIN;}
#endif
    left = in;
} // ibis::util::eq2range

/// This function uses nextafterf if the macro HAVE_NEXTAFTER is defined,
/// otherwise it uses FLT_EPSILON to compute outval as
/// (float)(inval)*(1+FLT_EPSILON).
template <typename Tin>
inline void ibis::util::round_up(const Tin& inval, float& outval) {
    // perform the initial rounding
    outval = static_cast<float>(inval);
    if (static_cast<Tin>(outval) < inval) {
	// if the rounded value is less than the input value, compute the
	// next value
#if defined(HAVE_NEXTAFTER)
	outval = nextafterf(static_cast<float>(inval), FLT_MAX);
#else
	float tmp = fabsf(outval) * FLT_EPSILON;
	if (tmp > 0.0) outval += tmp;
	else outval += FLT_MIN;
#endif
    }
} // ibis::util::round_up

/// Same as strdup() but uses 'new' instead of 'malloc'.  If s == 0, then
/// it returns 0.
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

/// In an attempt to compute small values more consistently, small values
/// are computed through division of integer values.  Since these integer
/// values are computed through the function pow, the accuracy of the
/// results depend on the implementation of the math library.
///
/// The value zero is always rounded to zero.   Incoming value less than
/// 1E-300 or greater than 1E300 is rounded to zero.
inline double ibis::util::coarsen(const double in, const unsigned prec) {
    double ret;
    if (prec > 15) {
	ret = in;
    }
    else if (in == 0.0) {
	ret = in;
    }
    else {
	ret = fabs(in);
	if (ret < 1e-300) {
	    ret = 0.0;
	}
	else if (ret < 1e300) {
	    ret = log10(ret);
	    const int ixp = static_cast<int>(ret) - static_cast<int>(prec);
	    ret = floor(0.5 + pow(1e1, ret-ixp));
	    if (ixp > 0)
		ret *= pow(1e1, ixp);
	    else if (ixp < 0)
		ret /= pow(1e1, -ixp);
	    if (in < 0.0)
		ret = -ret;
	}
	else {
	    ret = in;
	}
    }
    return ret;
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

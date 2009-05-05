//File: $Id$
// Author: K. John Wu <John.Wu at acm.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2009 University of California
#ifndef IBIS_FILEMANAGER_H
#define IBIS_FILEMANAGER_H
/// @file
/// Defines a simple file manager.
///
/// @note Use malloc and realloc to manage memory when the file content is
/// actually in memory.  The main reason for doing so is to malloc for
/// resizing.  This may potentially cause problems with memory allocation
/// through the new operator provided by C++ compiler.
///
#include "util.h"

#include <set>		// std::set
#include <map>		// std::map
#include <math.h>	// sqrt

#if ! (defined(HAVE_MMAP) || defined(_MSC_VER))
#  if defined(_XOPEN_SOURCE)
#    define HAVE_MMAP _XOPEN_SOURCE - 0 >= 500
#  elif defined(_POSIX_C_SOURCE)
#    define HAVE_MMAP _POSIX_C_SOURCE - 0 >= 0
#  else
#    define HAVE_MMAP defined(unix)||defined(linux)||defined(__APPLE__)||defined(__CYGWIN__)
#  endif
#endif

#if (HAVE_MMAP+0>0) || (defined(_WIN32) && defined(_MSC_VER))
#define HAVE_FILE_MAP
#endif

/// @ingroup FastBitIBIS
/// This fileManager is intended to allow different objects to share the
/// same open file.  It does not manage writing of files.
class ibis::fileManager {
public:

    /// Hint passed to the function @c getFile.  The main choice is whether
    /// to use memory map or use the read function to access the content of
    /// a file.
    enum ACCESS_PREFERENCE {
	MMAP_LARGE_FILES,	// files > minMapSize are mapped if possible
	PREFER_READ,		// read the whole file into memory
	PREFER_MMAP		// try to use mmap if possible
    };

    /// Given a file name, place the content in an array_t<T>.
    /// The return value is zero (0) if the function is successful, otherwise
    /// returns a non-zero value.
    /// @{
    int getFile(const char* name, array_t<char>& arr);
    int getFile(const char* name, array_t<unsigned char>& arr);
    int getFile(const char* name, array_t<int32_t>& arr);
    int getFile(const char* name, array_t<uint32_t>& arr);
    int getFile(const char* name, array_t<int64_t>& arr);
    int getFile(const char* name, array_t<uint64_t>& arr);
    int getFile(const char* name, array_t<float>& arr);
    int getFile(const char* name, array_t<double>& arr);
    int getFile(const char* name, array_t<rid_t>& arr);
    template<typename T>
    int getFile(const char* name, array_t<T>& arr,
		ACCESS_PREFERENCE pref=MMAP_LARGE_FILES);
    template<typename T>
    int tryGetFile(const char* name, array_t<T>& arr,
		   ACCESS_PREFERENCE pref=MMAP_LARGE_FILES);
    /* int getFile(const char* name, array_t<FID_T>& arr); */
    /// @}

    /// Prints status information about the file manager.
    void printStatus(std::ostream& out) const;
    /// Close the file, remove the record about it from the file manager.
    void flushFile(const char* name);
    /// Close all files in the named directory, but not subdirectories.
    void flushDir(const char* name);
    /// Close all files and remove all records of them.
    void clear();

    /// Returns a pointer to the one and only file manager.
    static fileManager& instance();
    /// Returns the value of a simple counter.  It is not thread-safe!
    time_t iBeat() const {return _hbeat++;}
    /// Returns the number of pages accessed by function read from stdlib.h.
    const double& pageCount() const {return page_count;}
    /// Returns the page size (in bytes) used by the file system.
    static uint32_t pageSize() {return pagesize;}
    /// Given the starting and ending addresses, this function computes the
    /// number of pages involved.  Used by derived classes to record page
    /// accesses.
    inline void recordPages(off_t start, off_t stop);
    static inline void increaseUse(size_t inc, const char* evt);
    static inline void decreaseUse(size_t dec, const char* evt);
    /// Signal to the file manager that some memory have been freed.
    void signalMemoryAvailable() const;

    /// A function object to be used to register external cleaners.
    class cleaner {
    public:
	virtual void operator()() const = 0;
	virtual ~cleaner() {};
    };
    void addCleaner(const cleaner* cl);
    void removeCleaner(const cleaner* cl);

    class roFile; // forward declaration of fileManager::roFile
    class storage; // forward declaration of fileManager::storage
#if defined(HAVE_FILE_MAP)
    class rofSegment; // forward declaration of fileManager::rofSegment
#endif
    friend class roFile;
    friend class storage;
    int getFile(const char* name, storage** st,
		ACCESS_PREFERENCE pref=MMAP_LARGE_FILES);
    int tryGetFile(const char* name, storage** st,
		   ACCESS_PREFERENCE pref=MMAP_LARGE_FILES);
    storage* getFileSegment(const char* name, off_t b, off_t e);

    /// Obtain a read lock on the file manager.
    inline void gainReadAccess(const char* mesg) const;
    /// Release a read lock on the file manager.
    inline void releaseAccess(const char* mesg) const;
    /// An object who uses a file under the management of the file manager
    /// should hold a readLock.
    class readLock {
    public:
	readLock(const char* m) : mesg(m) {
	    ibis::fileManager::instance().gainReadAccess(m);
	}
	~readLock() {
	    ibis::fileManager::instance().releaseAccess(mesg);
	}
    private:
	const char* mesg; // mesg identifies the holder of the lock
    };

    /// Return the current cache size in bytes.
    static uint64_t currentCacheSize() {return maxBytes;}
    /// Change the size of memory cache allocated to the file manager.
    static int adjustCacheSize(uint64_t);
    /// Returns the number of bytes currently on records.
    static uint64_t bytesInUse() {return ibis::fileManager::totalBytes();}
    /// Return the number of bytes free.
    static uint64_t bytesFree() {
	return (maxBytes > ibis::fileManager::totalBytes() ?
		maxBytes - ibis::fileManager::totalBytes() : 0);
    }

    /// A buffer is intended to be a temporary workspace in memory.  The
    /// constructor allocates a certain amount of memory, default to 16 MB;
    /// the destructor release the memory.  Its size can not be changed.
    template <typename T>
    class buffer {
    public:
	/// Constructor.  Default size is 16 MB.
	buffer(uint32_t sz=0);
	/// Destructor.  Release the memory allocated.
	~buffer();

	/// Return the ith value.  It does not perform array bounds check!
	T& operator[](uint32_t i) {return buf[i];}
	/// Return the ith value.  It does not perform array bounds check!
	const T& operator[](uint32_t i) const {return buf[i];}
	/// Address of the buffer allocated.
	T* address() const {return buf;}
	/// The number of elements in the buffer.  NOT the number of bytes.
	uint32_t size() const {return nbuf;}

    private:
	T* buf; ///< The address of the buffer.
	uint32_t nbuf; ///< The number of elements in the buffer.
    }; // buffer

protected:
    fileManager();  // get its input parameter from ibis::gParameters()
    ~fileManager(); // it exists forever

    void recordFile(roFile*); // record a new storage

    // parameters for controlling the resource usage
    /// Total bytes of all managed objects.
    static ibis::util::sharedInt64 totalBytes;
    static uint64_t maxBytes;    ///< Maximum number of bytes allowed.
    static unsigned int maxOpenFiles; ///< Maximum number of files opened.

    // not implemented, to prevent compiler from generating these functions
    fileManager(const fileManager& rhs);
    const fileManager& operator=(const fileManager& rhs);

private:
    typedef std::map< const char*, roFile*,
		      std::less< const char* > > fileList;
    typedef std::set< const cleaner* > cleanerList;
    typedef std::set< const char*, std::less< const char* > > nameList;
    fileList mapped; // files that are memory mapped
    fileList incore; // files that have been read into the main memory
    nameList reading;// files that are being read by the function getFile
    cleanerList cleaners; // list of external cleaners
    mutable time_t _hbeat;	// a simple counter, no mutex lock
    /// The number of pages read by read from @c unistd.h.
    double page_count;
    /// The number of bytes in a page.
    static uint32_t pagesize;
    /// the minimum size of a file before it is memory mapped.
    uint32_t minMapSize;
    /// Number of threads waiting for memory.
    uint32_t nwaiting;
    /// The conditional variable for reading list.
    pthread_cond_t readCond;

    mutable pthread_rwlock_t lock; // the multiple read single write lock
    mutable pthread_mutex_t mutex; // control access to incore and mapped
    mutable pthread_cond_t cond;   // conditional variable -- unload(), etc..

    int unload(size_t size);	// try to unload size bytes
    void invokeCleaners() const;// invoke external cleaners
    inline void gainWriteAccess(const char* m) const;
    /// A write lock for controlling access to the two interval lists.
    class writeLock {
    public:
	writeLock(const fileManager* fm, const char* m) :
	    manager(fm), mesg(m) {manager->gainWriteAccess(mesg);}
	~writeLock() {manager->releaseAccess(mesg);}
    private:
	const fileManager* manager;
	const char* mesg;

	writeLock(const writeLock&);
	const writeLock& operator=(const writeLock&);
    };
    /// Used to prevent simultaneous modification of the two internal
    /// lists.
    class mutexLock {
    public:
	mutexLock(const fileManager& fm, const char* m)
	    : manager(fm), mesg(m) {
	    if (0 == pthread_mutex_lock(&(manager.mutex))) {
		if (ibis::gVerbose > 12)
		    ibis::util::logMessage("fileManager::mutexLock",
					   "obtain lock for %s", mesg);
	    }
	    else
		ibis::util::logMessage("Warning", "fileManager::mutexLock for "
				       "%s failed to initialize -- %s ",
				       mesg, strerror(errno));
	}
	~mutexLock() {
	    if (0 == pthread_mutex_unlock(&(manager.mutex))) {
		if (ibis::gVerbose > 12)
		    ibis::util::logMessage("fileManager::mutexLock",
					   "release lock for %s", mesg);
	    }
	    else
		ibis::util::logMessage("Warning", "failed to release lock for "
				       "%s -- %s", mesg,
				       strerror(errno));
	}
    private:
	const fileManager& manager;
	const char* mesg;

	mutexLock(const mutexLock&);
	const mutexLock& operator=(const mutexLock&);
    };
    friend class writeLock;
    friend class mutexLock;
}; // class fileManager

/// The storage class treats all memory as @a char*.
/// It only uses malloc family of functions to manage the memory allocation
/// and deallocation.
class ibis::fileManager::storage {
public:
    storage() : name(0), m_begin(0), m_end(0), nacc(0), nref() {};
    explicit storage(size_t n); // allocate n bytes
    virtual ~storage() {clear();}

    /// Read part of a file [start, end).
    storage(const int fdes, const off_t begin, const off_t end);
    // the following three functions all make in-memory copies
    storage(const storage& rhs);
    const storage& operator=(const storage& rhs);
    void copy(const storage& rhs);
    /// Make another copy of the content in the range [begin, end).
    storage(const char* begin, const char* end);

    /// Those storage not associated with files do not have names.  They
    /// are not tracked by the file manager and should be immediately freed
    /// after use.
    bool unnamed() {return (name == 0);}
    const char* filename() const {return name;}

    /// Is the storage object empty?
    bool empty() const {return (m_begin == 0 || m_begin >= m_end);}
    /// Return the size (bytes) of the object.
    size_t size() const {return (m_begin >= m_end ? 0 : m_end - m_begin);}
    /// Return the number of bytes contained in the object.
    size_t bytes() const {return (m_begin >= m_end ? 0 : m_end - m_begin);}
    /// Enlarge the current array by 61.8% if @c nelm is smaller than the
    /// current size, otherwise enlarge to the specified size.
    void enlarge(size_t nelm=0);

    /// Starting address of the storage object.
    char* begin() {return m_begin;}
    const char* end() const {return m_end;}
    const char* begin() const {return m_begin;}

    /// Record a new active reference to this object.
    virtual void beginUse() {
	++ nref;
    }
    /// Record the termination of an active reference.
    virtual void endUse() {
	-- nref;
	++ nacc;
    }
    unsigned inUse() const { ///< Number of current accesses to this object.
	return nref();
    }
    unsigned pastUse() const { ///< Number of past accesses to this object.
	return nacc;
    }

    /// Is the storage a file map ?
    virtual bool isFileMap() const {return false;}
    // IO functions
    virtual size_t printStatus(std::ostream& out) const;
    /// Read part of an open file.  Return the number of bytes read.
    off_t read(const int fdes, const off_t begin, const off_t end);
    /// Write the content to the named file.
    void write(const char* file) const;

    inline void swap(storage& rhs) throw ();
//      // compares storage objects according to starting addresses
//      struct less : std::binary_function< storage*, storage*, bool > {
//  	bool operator()(const storage* x, const storage* y) const {
//  	    return (x->begin() < y->begin());
//  	}
//      };

protected:
    char* name;		///< Name of the file.  NULL (0) if no file is involved.
    char* m_begin;	///< Beginning of the storage.
    char* m_end;	///< End of the storage.
    unsigned nacc;	///< Number of accesses in the past.
    /// Number of (active) references to this storage.
    ibis::util::sharedInt32 nref;

    virtual void clear(); // free memory/close file
}; // class fileManager::storage

/// This class manages content of a whole (read-only) file.
/// It inherits the basic information stored in fileManager::storage and is
/// intended to process read-only files.
class ibis::fileManager::roFile : public ibis::fileManager::storage {
public:
    roFile() : storage(), opened(0), lastUse(0), mapped(0)
#if defined(_WIN32) && defined(_MSC_VER)
	       , fdescriptor(INVALID_HANDLE_VALUE), fmap(INVALID_HANDLE_VALUE),
	       map_begin(0)
#elif (HAVE_MMAP+0 > 0)
	, fdescriptor(-1), fsize(0), map_begin(0)
#endif
    {};
    virtual ~roFile() {clear();}

    // functions for recording access statistics
    virtual void beginUse();
    virtual void endUse();
    // is the read-only file mapped ?
    virtual bool isFileMap() const {return (mapped != 0);}

    // IO functions
    virtual size_t printStatus(std::ostream& out) const;
    void read(const char* file);
#if defined(HAVE_FILE_MAP)
    void mapFile(const char* file);
#endif

    /// The function to give score to a file. Files with smallest scores
    /// are the target for removal.
    float score() const {
	float sc = FLT_MAX;
	time_t now = time(0);
	if (opened >= now) {
	    sc = static_cast<float>(1e-4 * size() + nacc);
	}
	else if (lastUse >= now) {
	    sc = static_cast<float>(sqrt(5e-6*size()) + nacc +
				    (now - opened));
	}
	else {
	    sc = static_cast<float>((sqrt(1e-6*size() + now - opened) +
				     (static_cast<double>(nacc) /
				      (now - opened))) / (now - lastUse));
	}
	return sc;
    }

//      // compares storage objects according to file names
//      struct less : std::binary_function< roFile*, roFile*, bool > {
//  	bool operator()(const roFile* x, const roFile* y) const {
//  	    return strcmp(x->filename(), y->filename()) < 0;
//  	}
//      };
protected:

    // Read the whole file into memory.
    void doRead(const char* file);
    // Read the specified segment of the file into memory.
    void doRead(const char* file, off_t b, off_t e);
#if defined(HAVE_FILE_MAP)
    void doMap(const char* file, off_t b, off_t e, int opt=0);
#endif

    friend class ibis::fileManager;
    virtual void clear(); // free memory/close file

    size_t printBody(std::ostream& out) const;

private:
    time_t opened; // time first created, presumably when the file was opened
    time_t lastUse; // time of last use
    unsigned mapped; // 0 not a mapped file, otherwise yes

#if defined(_WIN32) && defined(_MSC_VER)
    HANDLE fdescriptor; // HANDLE to the open file
    HANDLE fmap; // HANDLE to the mapping object
    LPVOID map_begin; // actual address returned by MapViewOfFile
#elif (HAVE_MMAP+0 > 0)
    int fdescriptor; // descriptor of the open file
    size_t fsize;    // the size of the mapped portion of file
    void *map_begin; // actual address returned by mmap
#endif

    // not implemented, to prevent automatic generation
    roFile(const roFile&);
    const roFile& operator=(const roFile&);
}; // class fileManager::roFile

#if defined(HAVE_FILE_MAP)
/// This class is used to store information a memory mapped portion of a
/// file.  The main reason this is a derived class of roFile is to make
/// this one not shareable.
class ibis::fileManager::rofSegment : public ibis::fileManager::roFile {
public:
    rofSegment(const char *fn, off_t b, off_t e);
    virtual ~rofSegment() {};
    virtual size_t printStatus(std::ostream& out) const;

private:
    rofSegment(); // no default constructor
    rofSegment(const rofSegment&); // no copy constructor
    const rofSegment& operator=(const rofSegment&); // no assignment operator

    std::string filename_; // name of the file
    off_t begin_, end_;    // the start and the end address of the file map
}; // ibis::fileManager::rofSegment
#endif

inline void ibis::fileManager::releaseAccess(const char* mesg) const {
    int ierr = pthread_rwlock_unlock(&lock);
    if (0 == ierr) {
	if (ibis::gVerbose > 12)
	    ibis::util::logMessage("ibis::fileManager", "releaseAccess to %s",
				   mesg);
    }
    else {
	ibis::util::logMessage("Warning", "ibis::fileManager::releaseAccess "
			       "to %s" " returned %d -- %s",
			       mesg, ierr, strerror(ierr));
    }
} // void ibis::fileManager::releaseAccess()

inline void ibis::fileManager::gainReadAccess(const char* mesg) const {
    int ierr = pthread_rwlock_rdlock(&lock);
    if (0 == ierr) {
	if (ibis::gVerbose > 12)
	    ibis::util::logMessage("ibis::fileManager",
				   "gainReadAccess to %s", mesg);
    }
    else {
	ibis::util::logMessage("Warning", "ibis::fileManager::gainReadAccess "
			       "to %s returned %d -- %s",
			       mesg, ierr, strerror(ierr));
    }
} // void ibis::fileManager::gainReadAccess()

inline void ibis::fileManager::gainWriteAccess(const char* mesg) const {
    int ierr = pthread_rwlock_wrlock(&lock);
    if (0 == ierr) {
	if (ibis::gVerbose > 12)
	    ibis::util::logMessage("ibis::fileManager",
				   "gainWriteAccess to %s", mesg);
    }
    else {
	ibis::util::logMessage("Warning", "ibis::fileManager::gainWriteAccess "
			       "to %s returned %i -- %s",
			       mesg, ierr, strerror(ierr));
    }
} // void ibis::fileManager::gainWriteAccess()

// record the number of pages touched by the range specified [start, stop)
inline void ibis::fileManager::recordPages(off_t start, off_t stop) {
    if (start < stop) {
	start -= (start % pagesize);
	if (stop % pagesize)
	    stop += pagesize - stop % pagesize;
	page_count += static_cast<double>((stop - start) / pagesize);
    }
} // ibis::fileManager::recordPages

inline void
ibis::fileManager::increaseUse(size_t inc, const char* evt) {
    if (inc > 0) {
	ibis::fileManager::totalBytes += inc;
	LOGGER(evt && ibis::gVerbose > 9)
	    << evt << " added " << inc << " to increase totalBytes to "
	    << ibis::fileManager::totalBytes();
    }
} // ibis::fileManager::increaseUse

inline void
ibis::fileManager::decreaseUse(size_t dec, const char* evt) {
    if (dec > 0) {
	ibis::fileManager::totalBytes -= dec;
	LOGGER(evt && ibis::gVerbose > 9)
	    << evt << " removed " << dec << " to decrease totalBytes to "
	    << ibis::fileManager::totalBytes();
    }
} // ibis::fileManager::decreaseUse

inline void
ibis::fileManager::storage::swap(ibis::fileManager::storage& rhs) throw () {
    {char* tmp = name; name = rhs.name; rhs.name = tmp;}
    {char* tmp = m_begin; m_begin = rhs.m_begin; rhs.m_begin = tmp;}
    {char* tmp = m_end; m_end = rhs.m_end; rhs.m_end = tmp;}
    {unsigned itmp = nacc; nacc = rhs.nacc; rhs.nacc = itmp;}
    nref.swap(rhs.nref);
} // ibis::fileManager::storage::swap
#endif // IBIS_FILEMANAGER_H

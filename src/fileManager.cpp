//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2009 Univeristy of California
//
// Purpose:
// This file contains an implementation of the fileManager used by IBIS.
//
// Note:
// use malloc and realloc to manage memory when the file content is
// actually in memory.  The main reason for doing so is to avoid malloc for
// resizing.  This may potentially cause problems with memory allocation
// through the new operator provided by the C++ compiler.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "fileManager.h"
#include "resource.h"
#include "array_t.h"

#include <string>	// std::string
#include <stdio.h>	// fopen, fread, remove
#include <stdlib.h>	// malloc, realloc, free
#include <sys/stat.h>	// stat, open
#include <time.h>
#include <limits.h>

#if defined(HAVE_SYS_SYSCTL) || defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h> // sysctl
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#  include <windows.h>
#  include <psapi.h>	// GetPerformanceInfo, struct PERFORMANCE_INFORMATION
#elif HAVE_MMAP
#  include <sys/mman.h>	// mmap
#endif

// initialize static varialbes (class members) of fileManager
uint64_t ibis::fileManager::maxBytes = 0;
uint32_t ibis::fileManager::pagesize = 8192;
unsigned int ibis::fileManager::maxOpenFiles = 0;
ibis::util::sharedInt64 ibis::fileManager::totalBytes;

// default to about 256 MB
#define _FASTBIT_DEFAULT_MEMORY_SIZE 256*1024*1024

// explicit instantiation required
template int ibis::fileManager::getFile<uint64_t>
(char const*, array_t<uint64_t>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<int64_t>
(char const*, array_t<int64_t>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<uint32_t>
(char const*, array_t<uint32_t>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<int32_t>
(char const*, array_t<int32_t>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<uint16_t>
(char const*, array_t<uint16_t>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<int16_t>
(char const*, array_t<int16_t>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<char>
(char const*, array_t<char>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<signed char>
(char const*, array_t<signed char>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<unsigned char>
(char const*, array_t<unsigned char>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<float>
(char const*, array_t<float>&, ACCESS_PREFERENCE);
template int ibis::fileManager::getFile<double>
(char const*, array_t<double>&, ACCESS_PREFERENCE);

// time to wait for other threads to unload files in use
#ifndef FILEMANAGER_UNLOAD_TIME
#if defined(DEBUG) || defined(_DEBUG)
#define FILEMANAGER_UNLOAD_TIME 5
#else
#define FILEMANAGER_UNLOAD_TIME 60
#endif
#endif
// minimum size for doMap
#ifndef MIN_DOMAP_SIZE
#define MIN_DOMAP_SIZE 1048576
#endif

// given the name of a file, returns its content in an array
template <typename T>
int ibis::fileManager::getFile(const char* name, array_t<T>& arr,
			       ACCESS_PREFERENCE pref) {
    storage* st = 0;
    int ierr = getFile(name, &st, pref);
    if (ierr == 0) {
	if (st) {
	    array_t<T>* tmp = new array_t<T>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12) {
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu ints from %s",
			       static_cast<long unsigned>(arr.size()), name);
    }
    return ierr;
} // ibis::fileManager::getFile

// given the name of a file, returns its content in an array
template <typename T>
int ibis::fileManager::tryGetFile(const char* name, array_t<T>& arr,
				  ACCESS_PREFERENCE pref) {
    storage* st = 0;
    int ierr = tryGetFile(name, &st, pref);
    if (ierr == 0) {
	if (st) {
	    array_t<T>* tmp = new array_t<T>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12) {
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu ints from %s",
			       static_cast<long unsigned>(arr.size()), name);
    }
    return ierr;
} // ibis::fileManager::tryGetFile

// given the name of a file, returns its content in an array
int ibis::fileManager::getFile(const char* name, array_t<char>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<char>* tmp = new array_t<char>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12) {
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu chars from %s",
			       static_cast<long unsigned>(arr.size()), name);
    }
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name,
			       array_t<unsigned char>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<unsigned char>*tmp = new array_t<unsigned char>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12) {
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu unsigned chars from %s",
			       static_cast<long unsigned>(arr.size()), name);
    }
    return ierr;
} // ibis::fileManager::getFile

// given the name of a file, returns its content in an array
int ibis::fileManager::getFile(const char* name, array_t<int32_t>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<int32_t>* tmp = new array_t<int32_t>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12) {
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu ints from %s",
			       static_cast<long unsigned>(arr.size()), name);
    }
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name, array_t<uint32_t>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<uint32_t>*tmp = new array_t<uint32_t>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12) {
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu unsigned ints from %s",
			       static_cast<long unsigned>(arr.size()), name);
    }
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name, array_t<int64_t>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<int64_t>* tmp = new array_t<int64_t>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12)
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu long ints from %s",
			       static_cast<long unsigned>(arr.size()), name);
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name, array_t<uint64_t>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<uint64_t>*tmp = new array_t<uint64_t>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12)
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu long unsigned ints from %s",
			       static_cast<long unsigned>(arr.size()), name);
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name, array_t<float>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<float>* tmp = new array_t<float>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12)
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu floats  from %s",
			       static_cast<long unsigned>(arr.size()), name);
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name, array_t<double>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<double>* tmp = new array_t<double>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12)
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu doubles from %s",
			       static_cast<long unsigned>(arr.size()), name);
    return ierr;
} // ibis::fileManager::getFile

int ibis::fileManager::getFile(const char* name, array_t<rid_t>& arr) {
    storage* st = 0;
    int ierr = getFile(name, &st);
    if (ierr == 0) {
	if (st) {
	    array_t<rid_t>* tmp = new array_t<rid_t>(*st);
	    arr.swap(*tmp);
	    delete tmp;
	}
	else {
	    arr.clear();
	}
    }

    if (ibis::gVerbose > 12)
	ibis::util::logMessage("ibis::fileManager::getFile",
			       "got %lu RIDs from %s",
			       static_cast<long unsigned>(arr.size()), name);
    return ierr;
} // ibis::fileManager::getFile

// print the current status of the file manager
void ibis::fileManager::printStatus(std::ostream& out) const {
    size_t mtot=0, itot=0;
    char tstr[28];
    ibis::util::getLocalTime(tstr);

    //readLock lck("printStatus"); // acquiring lock here may cause dead lock
    out << "\n--- " << tstr << "\nThe number of memory mapped files is "
	<< mapped.size() << ". (max = " << maxOpenFiles
	<< ")\n";
    for (fileList::const_iterator it0 = mapped.begin();
	 it0 != mapped.end(); ++it0) {
	mtot += (*it0).second->printStatus(out);
    }
    out << "\nSize of all mapped files is " << mtot << std::endl;
    out << "\n\nThe number of files read into memory is " << incore.size()
	<< ".\n";
    for (fileList::const_iterator it1 = incore.begin();
	 it1 != incore.end(); ++it1) {
	itot += (*it1).second->printStatus(out);
    }
    out << "\nThe total size of all files read into memory is " << itot
	<< std::endl;
    out << "\nSize of all named storages is " << itot + mtot
	<< "\nSize of all unnamed storages is "
	<< ibis::fileManager::totalBytes() - (itot + mtot)
	<< "\nThe total size of all named and unnamed storages is "
	<< ibis::fileManager::totalBytes()
	<< "\nThe prescribed maximum size is " << maxBytes
	<< "\nNumber of pages accessed (recorded so far) is "
	<< page_count << " (page size = " << pagesize << ")\n"
	<< std::endl;
} // ibis::fileManager::printStatus

// remove a file from cache
void ibis::fileManager::flushFile(const char* name) {
    if (name == 0 || *name == 0) return;
    mutexLock lck(*this, "flushFile");
    fileList::iterator it = mapped.find(name);
    if (it != mapped.end()) {
	if (ibis::gVerbose > 11)
	    ibis::util::logMessage("flushFile", "removing file \"%s\" from "
				   "the list of mapped files", (*it).first);
	if ((*it).second->inUse() == 0) {
	    delete (*it).second;
	    mapped.erase(it);
	}
	else if (ibis::gVerbose > 2) {
	    ibis::util::logMessage("flushFile", "can not remove \"%s\" "
				   "because it is in use", (*it).first);
	}
    }
    else if (incore.end() != (it = incore.find(name))) {
	if (ibis::gVerbose > 11)
	    ibis::util::logMessage("flushFile", "removing file \"%s\" from "
				   "the list of incore files", (*it).first);
	if ((*it).second->inUse() == 0) {
	    delete (*it).second;
	    incore.erase(it);
	}
	else if (ibis::gVerbose > 2) {
	    ibis::util::logMessage("flushFile", "can not remove \"%s\" "
				   "because it is in use", (*it).first);
	}
    }
} // ibis::fileManager::flushFile

// remove all files from the specified directory (include all sub
// directories)
void ibis::fileManager::flushDir(const char* name) {
    if (name == 0 || *name == 0) return;
    mutexLock lck(*this, "flushDir");
    if (ibis::gVerbose > 5)
	ibis::util::logMessage("flushDir",
			       "removing records of all files in %s", name);

    size_t deleted = 0;
    const size_t len = strlen(name);
    const size_t offset = len + (DIRSEP != name[len-1]);

    while (1) { // loop forever
	// is there any files within the directory
	size_t cnt = 0;
	fileList::iterator it = mapped.begin();
	while (it != mapped.end()) {
	    fileList::iterator next = it; ++next;
	    if (strncmp((*it).first, name, len)==0) {
		if (strchr((*it).first+offset, DIRSEP) == 0) {
		    if ((*it).second->inUse() > 0) {
			++ cnt;
			ibis::util::logMessage
			    ("Warning", "flushDir can not remove mapped "
			     "file (%s).  It is in use", (*it).first);
			if (ibis::gVerbose > 3) {
			    ibis::util::logger lg(3);
			    (*it).second->printStatus(lg.buffer());
			}
		    }
		    else {
			//writeLock wlck(this, "flushDir");
			delete (*it).second;
			mapped.erase(it);
			++ deleted;
		    }
		}
	    }
	    it = next;
	}

	it = incore.begin();
	while (it != incore.end()) {
	    fileList::iterator next = it; ++next;
	    if (strncmp((*it).first, name, len)==0) {
		if (strchr((*it).first+offset, DIRSEP) == 0) {
		    if ((*it).second->inUse()) {
			++ cnt;
			ibis::util::logMessage
			    ("Warning", "flushDir can not remove in-memory "
			     "file (%s).  It is in use", (*it).first);
			if (ibis::gVerbose > 3) {
			    ibis::util::logger lg(3);
			    (*it).second->printStatus(lg.buffer());
			}
		    }
		    else {
			//writeLock wlck(this, "flushDir");
			delete (*it).second;
			incore.erase(it);
			++ deleted;
		    }
		}
	    }
	    it = next;
	}

	if (cnt) {// there are files in use, wait for them to be released
	    ibis::util::logMessage("Warning", "flushDir(%s) finishes with "
				   "%lu file%s still in memory", name,
				   static_cast<long unsigned>(cnt),
				   (cnt>1?"s":""));
	    return;
	    //	    pthread_cond_wait(&cond, &mutex);
	}
	else {
	    if (ibis::gVerbose > 11)
		ibis::util::logMessage("flushDir",
				       "removed %lu file%s from %s",
				       static_cast<long unsigned>(deleted),
				       (deleted>1?"s":""), name);
	    return;
	}
    }
} // ibis::fileManager::flushDir

/// Clear the two lists of files.
void ibis::fileManager::clear() {
    if (ibis::gVerbose > 12 ||
	(ibis::fileManager::totalBytes() > 0 && ibis::gVerbose > 6)) {
	ibis::util::logger lg(2);
	lg.buffer() << "ibis::fileManager::clear -- starting ...";
	printStatus(lg.buffer());
    }

    std::vector<roFile*> tmp; // temporary contains the read-only files
    writeLock wlck(this, "clear");
    tmp.reserve(mapped.size()+incore.size());
    for (fileList::const_iterator it=mapped.begin();
	 it != mapped.end(); ++it) {
	tmp.push_back((*it).second);
    }
    mapped.clear();
    for (fileList::const_iterator it=incore.begin();
	 it != incore.end(); ++it) {
	tmp.push_back((*it).second);
    }
    incore.clear();
    // delete the read-only files stored in the std::vector because the
    // FileList uses the name of the file (part of the object to be
    // deleted) as the key (of a std::map).
    for (std::vector<roFile*>::iterator it = tmp.begin();
	 it != tmp.end(); ++ it)
	delete (*it);

    LOGGER((ibis::fileManager::totalBytes() != 0 && ibis::gVerbose > 0) ||
	   ibis::gVerbose > 12)
	<< "fileManager::clear -- There are " << ibis::fileManager::totalBytes()
	<< " byte" << (ibis::fileManager::totalBytes()>1 ? "s" : "")
	<< " of storage remain in memory after removing all managed objects";
} // ibis::fileManager::clear

void ibis::fileManager::addCleaner(const ibis::fileManager::cleaner* cl) {
    mutexLock lck(*this, "addCleaner");
    cleanerList::const_iterator it = cleaners.find(cl);
    if (it == cleaners.end())
	cleaners.insert(cl);
} // ibis::fileManager::addCleaner

void ibis::fileManager::removeCleaner(const ibis::fileManager::cleaner* cl) {
    mutexLock lck(*this, "removeCleaner");
    cleanerList::iterator it = cleaners.find(cl);
    if (it != cleaners.end())
	cleaners.erase(it);
} // ibis::fileManager::removeCleaner

/// The instance function of the fileManager singleton.
ibis::fileManager& ibis::fileManager::instance() {
    static ibis::fileManager theManager;
    return theManager;
} // ibis::fileManager::instance

/// The protected constructor of the ibis::fileManager class.  There are
/// three parameters that can be specified in a configuration file to
/// control this object, fileManager.maxBytes, fileManager.maxOpenFiles,
/// and fileManager.minMapSize.  If you are unsure of what to do, then
/// don't specify anything -- the default values are typically acceptable.
///
/// \arg filemanager.maxBytes The maximum number of bytes of all objects
/// under control of the file manager, e.g.,
/// \verbatim
/// fileManager.maxBytes = 500MB
/// \endverbatim
/// One may specify a number followed by KB, MB, or GB (without space in
/// between).  If not specified, this constructor attempts to determine the
/// size of the physical memory available and will use half of the memory
/// for caching FastBit objects.
///
/// \arg fileManager.maxOpenFiles This file manager will keep the number of
/// open files below this specified maximum.  Note that FastBit usually
/// invokes the lower level function open, which typical can use more file
/// handles than the higher level ones such as fopen.  If not specified, it
/// will use three quarters of maximum file halder defined by _SC_OPEN_MAX.
///
/// \arg fileManager.minMapSize The minimal size of a file before FastBit
/// will attempt to use memory map on it.  For smaller files, it is more
/// efficient to read the whole content into memory rather than keeping a
/// file open.  The default value is defined by the macro MIN_DOMAP_SIZE.
ibis::fileManager::fileManager()
    : _hbeat(0), page_count(0), minMapSize(MIN_DOMAP_SIZE), nwaiting(0) {
    {
	unsigned long sz = static_cast<unsigned long>
	    (ibis::gParameters().getNumber("fileManager.maxBytes"));
	if (sz > 0)
	    maxBytes = sz;
	sz = static_cast<unsigned long>
	    (ibis::gParameters().getNumber("fileManager.maxOpenFiles"));
	if (sz > 10)
	    maxOpenFiles = sz;
	sz = static_cast<unsigned long>
	    (ibis::gParameters().getNumber("fileManager.minMapSize"));
	if (sz != 0)
	    minMapSize = sz;
    }
    if (maxBytes < 10*1024*1024) {
#ifdef _SC_PHYS_PAGES
	// most *nix flavors defines this for physical number of pages
	uint64_t mem = 0;
#ifdef _SC_PAGESIZE
	pagesize = sysconf(_SC_PAGESIZE);
	mem = static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES)) * pagesize;
#elif defined(_SC_PAGE_SIZE)
	pagesize = sysconf(_SC_PAGE_SIZE);
	mem = static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES)) * pagesize;
#endif
	mem >>= 1; // allow half to be used by fileManager
	if (mem > ULONG_MAX)
	    maxBytes = (ULONG_MAX - (ULONG_MAX>>2));
	else if (mem > 0)
	    maxBytes = mem;
#elif defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM))
	// BSD flavored systems provides sysctl for finding out the
	// physical memory size
	uint64_t mem = 0;
	size_t len = sizeof(mem);
	int mib[2] = {CTL_HW, 0};
#ifdef HW_MEMSIZE
	mib[1] = HW_MEMSIZE;
#else
	mib[1] = HW_PHYSMEM;
#endif
	if (sysctl(mib, 2, &mem, &len, NULL, 0) == 0 && len <= sizeof(mem)) {
	    mem >>= 1;
	    if (mem > ULONG_MAX)
		maxBytes = (ULONG_MAX - (ULONG_MAX>>2));
	    else if (mem > 0)
		maxBytes = mem;
	}
	else {
	    LOGGER(ibis::gVerbose > 1)
		<< "fileManager failed to determine the physical memory size "
		<< "with sysctl(CTL_HW=" << CTL_HW << ", HW_PHYSMEM="
		<< HW_PHYSMEM << ") -- "
		<< (errno != 0 ? strerror(errno) :
		    (len != sizeof(mem)) ? "return value of incorrect size" :
		    "unknwon error")
		<< ", mem=" << mem;
	    maxBytes = _FASTBIT_DEFAULT_MEMORY_SIZE;
	}
#elif defined(_PSAPI_H_)
	// MS Windows uses psapi to physical memory size
	PERFORMANCE_INFORMATION pi;
	if (GetPerformanceInfo(&pi, sizeof(pi))) {
	    size_t avail = pi.PhysicalAvailable;
	    size_t mem = (pi.PhysicalTotal >> 1);
	    if (avail > mem) mem = avail; // take it if available
	    if (mem < (ULONG_MAX / pi.PageSize))
		maxBytes = mem * pi.PageSize;
	    else
		maxBytes = (ULONG_MAX - (ULONG_MAX >> 2));
	}
	else {
	    char *lpMsgBuf;
	    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			  FORMAT_MESSAGE_FROM_SYSTEM | 
			  FORMAT_MESSAGE_IGNORE_INSERTS,
			  NULL, GetLastError(),
			  MAKELANGID(LANG_NEUTRAL,
				     SUBLANG_DEFAULT),
			  (LPTSTR) &lpMsgBuf, 0, NULL);
	    ibis::util::logMessage("fileManager::ctor", "failed to "
				   "determine the physical memory size "
				   "-- %s", lpMsgBuf);
	    LocalFree(lpMsgBuf);	// Free the buffer.
	}
	pagesize = pi.PageSize;
// 	SYSTEM_INFO sysinfo;
// 	GetSystemInfo(&sysinfo);
// 	pagesize = sysinfo.dwPageSize;
#else
	maxBytes = _FASTBIT_DEFAULT_MEMORY_SIZE;
	if (ibis::gVerbose > 2)
	    ibis::util::logMessage("fileManager::ctor",
				   "using a default value of %lu bytes",
				   maxBytes);
#endif
    }
    //     if (maxBytes < 100*1024*1024)
    // 	maxBytes = 100*1024*1024; // make maxBytes no less than 100 MB

    if (maxOpenFiles < 8) { // maxOpenFiles is too small
#if defined(_SC_OPEN_MAX)
	// maximum number of open files is defined in sysconf
	uint32_t sz = sysconf(_SC_OPEN_MAX);
	maxOpenFiles = static_cast<uint32_t>(0.75*sz);
#else
	maxOpenFiles = 60;
#endif
    }
    // final adjustment based on stdio limitation
#if defined(OPEN_MAX)
    maxOpenFiles = static_cast<int>
	(maxOpenFiles<=0.8*OPEN_MAX?maxOpenFiles:0.8*OPEN_MAX);
#elif defined(STREAM_MAX)
    maxOpenFiles = static_cast<int>
	(maxOpenFiles<=0.9*STREAM_MAX?maxOpenFiles:0.9*STREAM_MAX);
#endif
#if defined(FOPEN_MAX)
    if (maxOpenFiles < FOPEN_MAX)
	maxOpenFiles = maxOpenFiles;
#endif
    if (pthread_rwlock_init(&lock, 0) != 0)
	throw ibis::bad_alloc("pthread_rwlock_init failed in fileManager "
			      "ctor");
    if (pthread_mutex_init(&mutex, 0) != 0)
	throw ibis::bad_alloc("pthread_mutex_init failed in "
			      "fileManager ctor");
    if (pthread_cond_init(&cond, 0) != 0)
	throw ibis::bad_alloc("pthread_cond_init(cond) failed in "
			      "fileManager ctor");
    if (pthread_cond_init(&readCond, 0) != 0)
	throw ibis::bad_alloc("pthread_cond_init(readCond) failed in "
			      "fileManager ctor");
    if (ibis::gVerbose > 1) {
	ibis::util::logMessage("fileManager::ctor", "maxBytes=%lu, "
			       "maxOpenFiles=%u", maxBytes, maxOpenFiles);
    }
} // ibis::fileManager::fileManager

// destructor
ibis::fileManager::~fileManager() {
    clear();
    (void)pthread_rwlock_destroy(&lock);
    (void)pthread_mutex_destroy(&mutex);
    (void)pthread_cond_destroy(&cond);
} // ibis::fileManager::~fileManager

// record a newly allocated storage in the two lists
void ibis::fileManager::recordFile(ibis::fileManager::roFile* st) {
    if (st == 0) return;
    if (st->begin() == st->end()) return;
    std::string evt = "fileManager::recordFile";
    if (ibis::gVerbose > 8) {
	std::ostringstream oss;
	oss << "(";
	if (st->filename() != 0)
	    oss << st->filename() << ", ";
	oss << static_cast<void*>(st->begin()) << ", " << st->size() << ")";
	evt += oss.str();
    }

    if (ibis::gVerbose > 10)
	ibis::util::logMessage(evt.c_str(), "record storage 0x%x", st);
    increaseUse(st->bytes(), evt.c_str());
    if (st->filename() == 0)
	return;

    mutexLock lck(*this, evt.c_str());
    //writeLock rock(this, "record");
    readLock rock(evt.c_str());
    if (st->mapped) {
	fileList::const_iterator it = mapped.find(st->filename());
	if (it == mapped.end()) {
	    if (incore.find(st->filename()) != incore.end()) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " trying to register a memory mapped storage object ("
		    << st->filename()
		    << ") while one with the same name is already in "
		    << "the incore list";
		throw "ibis::fileManager::recordFile trying to register two "
		    "storage related the same file (old incore, "
		    "new mapped)";
	    }
	    mapped[st->filename()] = st;
	}
	else if (st != (*it).second) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt
		<< " trying to register a memory mapped storage object ("
		<< st->filename()
		<< ") while one with the same name is already in "
		<< "the mapped list";
	    throw "ibis::fileManager::recordFile trying to register two "
		"storage related the same file (both mapped)";
	}
    }
    else {
	fileList::const_iterator it = incore.find(st->filename());
	if (it == incore.end()) {
	    if (mapped.find(st->filename()) != mapped.end()) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt
		    << " trying to register an incore storage object ("
		    << st->filename()
		    << ") while one with the same name is already in "
		    << "the mapped list";
		throw "ibis::fileManager::recordFile trying to register two "
		    "storage related the same file (old mapped, "
		    "new incore)";
	    }
	    incore[st->filename()] = st;
	}
	else if (st != (*it).second) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << evt
		<< " trying to register an incore storage object ("
		<< st->filename()
		<< ") while one with the same name is already in "
		<< "the mapped list";
	    throw "ibis::fileManager::recordFile trying to register two "
		"storage related the same file (both incore)";
	}
    }
} // ibis::fileManager::recordFile

// upon successful completion of the task, it returns zero; otherwise, it
// returns a non-zero value to indicate an error and it does not modify the
// content of storage object.
int ibis::fileManager::getFile(const char* name, storage** st,
			       ACCESS_PREFERENCE pref) {
    if (name == 0 || *name == 0) return -100;
    int ierr = 0;
    long unsigned bytes = 0; // the file size in bytes
    std::string evt = "fileManager::getFile";
    if (ibis::gVerbose >= 0) {
	evt += '(';
	evt += name;
	evt += ')';
    }
    {   // determine the file size, whether the file exist or not
	Stat_T tmp;
	if (0 == UnixStat(name, &tmp)) {
	    bytes = tmp.st_size;
	    if (bytes == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- " << evt << ": the named file is empty";
		return ierr;
	    }
	}
	else {
	    if (ibis::gVerbose > 11 || errno != ENOENT) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- ibis::fileManager::getFile(" << name
		    << ") -- command stat failed: " << strerror(errno);
	    }
	    ierr = -101;
	    return ierr;
	}
    }

    mutexLock lck(*this, evt.c_str()); // only one instance can run
    readLock rock(evt.c_str());
    // is the named file among those mapped ?
    fileList::iterator it = mapped.find(name);
    if (it != mapped.end()) { // found it
	*st = (*it).second;
	return ierr;
    }

    // is the named file among those incore
    it = incore.find(name);
    if (it != incore.end()) { // found it
	*st = (*it).second;
	return ierr;
    }

    // is the file being read by another thread?
    if (reading.find(name) != reading.end()) {
	do {
	    if (ibis::gVerbose > 5)
		ibis::util::logMessage
		    (evt.c_str(), "waiting for another thread "
		     "to read \"%s\"", name);
	    ierr = pthread_cond_wait(&readCond, &mutex);
	    if (ierr != 0) {
		ierr = -112;
		return ierr;
	    }
	} while (reading.find(name) != reading.end());

	it = mapped.find(name);
	if (it != mapped.end()) {
	    *st = (*it).second;
	    return ierr;
	}
	it = incore.find(name);
	if (it != incore.end()) {
	    *st = (*it).second;
	    return ierr;
	}
	ierr = -110; // the pending read did not succeed. retry?
	return ierr;
    }
    reading.insert(name); // add to the reading list
    if (ibis::gVerbose > 5)
	ibis::util::logMessage(evt.c_str(), "attempting to read %s "
			       "(%lu bytes)", name, bytes);

    //////////////////////////////////////////////////////////////////////
    // need to actually open it up -- need to modify the two lists
    // unload enough files to free up space
    if (bytes + ibis::fileManager::totalBytes() > maxBytes) {
	LOGGER(ibis::gVerbose > 5)
	    << evt << " -- need to unload " << bytes
	    << " bytes for \"" << name << "\", maxBytes=" << maxBytes
	    << ", totalBytes=" << ibis::fileManager::totalBytes();
	ierr = unload(bytes);
    }
    else if (mapped.size() >= maxOpenFiles && bytes >= minMapSize) {
	LOGGER(ibis::gVerbose > 7)
	    << evt << " -- need to unload some files before reading \""
	    << name << "\", maxBytes=" << maxBytes
	    << ", totalBytes=" << ibis::fileManager::totalBytes();
	ierr = unload(0); // unload whatever can be freed
    }
    if (ierr < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << evt << " -- unable to free up " << bytes
	    << " bytes to read the file " << name << ", ierr = -102";
	reading.erase(name);
	ierr = -102;
	return ierr;
    }

    ibis::fileManager::roFile* tmp = new ibis::fileManager::roFile();
    if (tmp == 0) {
	ibis::util::logMessage(evt.c_str(), "unable to allocate a new "
			       "roFile object for \"%s\"", name);
	reading.erase(name);
	ierr = -103;
	return ierr;
    }
    ibis::horometer timer;
    if (ibis::gVerbose > 7)
	timer.start();
    int readOrMap = 0; // default read
#if defined(HAVE_FILE_MAP)
    // now we can ask the question: "to map or not to map?"
    size_t sz = minMapSize;
    if (mapped.size() > (maxOpenFiles >> 1)) {
	// compute the maximum size of the first ten files
	fileList::const_iterator mit = mapped.begin();
	for (int cnt = 0; cnt < 10 && mit != mapped.end(); ++ cnt, ++ mit)
	    if (sz < (*it).second->size())
		sz = (*it).second->size();
	if (sz < MIN_DOMAP_SIZE)
	    sz = MIN_DOMAP_SIZE;
    }
    if (mapped.size() < maxOpenFiles && 
	(pref == PREFER_MMAP || (pref == MMAP_LARGE_FILES && bytes >= sz))) {
	// map the file read-only
	tmp->mapFile(name);
	if (tmp->begin()) {
	    if (tmp->isFileMap()) {
		mapped[tmp->filename()] = tmp;
		readOrMap = 1;
	    }
	    else {
		incore[tmp->filename()] = tmp;
		readOrMap = 0;
	    }
	}
	else {
	    // read the file into memory
	    tmp->doRead(name);
	    if (tmp->begin())
		incore[tmp->filename()] = tmp;
	}
    }
    else {
	// read the file into memory
	tmp->doRead(name);
	if (tmp->begin())
	    incore[tmp->filename()] = tmp;
    }
#else
    tmp->doRead(name);
    incore[tmp->filename()] = tmp;
#endif
    if (tmp->size() == bytes) {
	if (ibis::gVerbose > 5) {
	    ibis::util::logMessage
		(evt.c_str(), "completed %s %lu bytes", name,
		 (tmp->isFileMap()?"mmapping":"retrieving"),
		 static_cast<long unsigned>(tmp->size()));
	}
	if (ibis::gVerbose > 7) {
	    timer.stop();
	    double tcpu = timer.CPUTime();
	    double treal = timer.realTime();
	    double rt1 = tcpu > 0 ? (1e-6*tmp->size()/tcpu) : 0.0;
	    double rt2 = treal > 0 ? (1e-6*tmp->size()/treal) : 0.0;
	    ibis::util::logger lg(7);
	    lg.buffer() << evt << " took " << treal << " sec(elapsed) ["
			<< tcpu << " sec(CPU)] to "
			<< (tmp->isFileMap()?"mmap ":"read ") << tmp->size()
			<< " bytes at a speed of " << rt2
			<< " MB/s [" << rt1 << "]";
	    if (ibis::gVerbose > 11) {
		lg.buffer() << "\n";
		(void) tmp->printStatus(lg.buffer());
	    }
	}
	if (ibis::gVerbose > 8) {
	    std::ostringstream oss;
	    oss << " --> " << static_cast<void*>(tmp->begin());
	    evt == oss.str();
	}
	increaseUse(bytes, evt.c_str());

	*st = tmp; // pass tmp to the caller
	ierr = 0;
    }
    else {
	ibis::util::logMessage
	    ("Warning", "ibis::fileManager::getFile(%s) "
	     "failed retrieving %lu bytes (actual size %lu)",
	     name, bytes, static_cast<long unsigned>(tmp->size()));
	delete tmp;
	ierr = -104;
    }

    reading.erase(name); // no longer on the list reading
    (void) pthread_cond_broadcast(&readCond); // tell all others
    return ierr;
} // int ibis::fileManager::getFile

/// Returns 1 if there is not enough space to read the whole file into
/// memory.  Other return values are same as the function @c getFile.
int ibis::fileManager::tryGetFile(const char* name, storage** st,
				  ACCESS_PREFERENCE pref) {
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logMessage("tryGetFile", "attempt to retrieve \"%s\", "
			   "currently there are %lu in mapped files and %lu "
			   "incore files",
			   name, static_cast<long unsigned>(mapped.size()),
			   static_cast<long unsigned>(incore.size()));
#endif
    int ierr = 0;
    unsigned long bytes = 0; // the file size in bytes
    mutexLock lck(*this, "tryGetFile");
    readLock rock("tryGetFile");

    // is the named file among those mapped ?
    fileList::iterator it = mapped.find(name);
    if (it != mapped.end()) { // found it
	*st = (*it).second;
	return ierr;
    }

    // is the named file among those incore
    it = incore.find(name);
    if (it != incore.end()) { // found it
	*st = (*it).second;
	return ierr;
    }

    {   // first determine the file size, whether the file exist or not
	Stat_T tmp;
	if (0 == UnixStat(name, &tmp)) {
	    bytes = tmp.st_size;
	    if (bytes == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- ibis::fileManager::tryGetFile(" << name
		    << ") file is empty.";
		return ierr;
	    }
	}
	else {
	    if (ibis::gVerbose > 11 || errno != ENOENT) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- ibis::fileManager::tryGetFile(" << name
		    << ") -- command stat failed: " << strerror(errno);
	    }
	    ierr = -105;
	    return ierr;
	}
    }

    // not enough space to get the file
    if (bytes + ibis::fileManager::totalBytes() > maxBytes) {
	return -106; // not enough space
    }
    if (reading.find(name) != reading.end()) {
	ierr = -111; // another thread is reading the same file
	return ierr;
    }
    reading.insert(name); // record the name
    if (ibis::gVerbose > 5)
	ibis::util::logMessage("tryGetFile", "attempting to read %s "
			       "(%lu bytes)", name, bytes);

    //////////////////////////////////////////////////////////////////////
    // need to actually open it up -- need to modify the two lists
    ibis::fileManager::roFile* tmp = new ibis::fileManager::roFile();
    ibis::horometer timer;
    if (ibis::gVerbose > 7)
	timer.start();
    int readOrMap = 0; // default read
    // "to map or not to map", that is the question
#if defined(HAVE_FILE_MAP)
    size_t sz = minMapSize;
    if (mapped.size() > (maxOpenFiles >> 1)) {
	fileList::const_iterator mit = mapped.begin();
	for (int cnt = 0; cnt < 10 && mit != mapped.end(); ++ cnt, ++ mit)
	    if (sz < (*it).second->size())
		sz = (*it).second->size();
	if (sz < MIN_DOMAP_SIZE)
	    sz = MIN_DOMAP_SIZE;
    }
    if (mapped.size() < maxOpenFiles &&
	(pref == PREFER_MMAP || (pref == MMAP_LARGE_FILES && bytes >= sz))) {
	// map the file read-only
	tmp->mapFile(name);
	if (tmp->begin()) {
	    if (tmp->isFileMap()) {
		mapped[tmp->filename()] = tmp;
		readOrMap = 1;
	    }
	    else {
		incore[tmp->filename()] = tmp;
	    }
	}
	else {
	    // read the file into memory
	    tmp->doRead(name);
	    if (tmp->begin())
		incore[tmp->filename()] = tmp;
	}
    }
    else {
	// read the file into memory
	tmp->doRead(name);
	if (tmp->begin())
	    incore[tmp->filename()] = tmp;
    }
#else
    tmp->doRead(name);
    if (tmp->begin())
	incore[tmp->filename()] = tmp;
#endif
    if (tmp->size() == bytes) {
	increaseUse(tmp->size(), "ibis::fileManager::tryGetFile");
	if (ibis::gVerbose > 5) {
	    ibis::util::logMessage
		("ibis::fileManager", "tryGetFile(%s) completed %s %lu "
		 "bytes", name, (tmp->isFileMap()?"mmapping":"retrieving"),
		 static_cast<long unsigned>(tmp->size()));
	}
	if (ibis::gVerbose > 7) {
	    timer.stop();
	    double tcpu = timer.CPUTime();
	    double treal = timer.realTime();
	    double rt1 = tcpu > 0 ? (1e-6*tmp->size()/tcpu) : 0.0;
	    double rt2 = treal > 0 ? (1e-6*tmp->size()/treal) : 0.0;
	    ibis::util::logger lg(7);
	    lg.buffer() << "ibis::fileManager -- tryGetFile(" << name
			<< ") took " << treal << " sec(elapsed) ["
			<< tcpu  << " sec(CPU)] to "
			<< (tmp->isFileMap()?"mmap ":"read ") << tmp->size()
			<< " bytes at a speed of " << rt2
			<< " MB/s [" << rt1 << "]";
	    if (ibis::gVerbose > 11) {
		lg.buffer() << "\n";
		(void) tmp->printStatus(lg.buffer());
	    }
	}

	*st = tmp; // pass tmp to the caller
    }
    else {
	ibis::util::logMessage
	    ("Warning", "ibis::fileManager::tryGetFile(%s) "
	     "failed retrieving %lu bytes (actual size %lu)",
	     name, bytes, static_cast<long unsigned>(tmp->size()));
	delete tmp;
	ierr = -107;
    }

    (void) pthread_cond_broadcast(&readCond); // tell all others
    reading.erase(name); // no longer on the list reading
    return ierr;
} // ibis::fileManager::tryGetFile

/// Read or memory map a portion of the named file (@c name).  The file is
/// not tracked by tracking the name through a separate variable in the
/// class rofSegment.
ibis::fileManager::storage*
ibis::fileManager::getFileSegment(const char* name, off_t b, off_t e) {
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logMessage("getFileSegment", "attempt to retrieve \"%s\", "
			   "currently there are %lu in mapped files and "
			   "%lu incore files",
			   name, static_cast<long unsigned>(mapped.size()),
			   static_cast<long unsigned>(incore.size()));
#endif
    int ierr = 0;
    ibis::fileManager::storage *st = 0;
    if (name == 0 || *name == 0 || b >= e)
	return st;

    unsigned long bytes = e - b; // the size (in bytes) of the file segment
    mutexLock lck(*this, "getFileSegment");
    readLock rock("getFileSegment");
    if (ibis::gVerbose > 5)
	ibis::util::logMessage("getFileSegment", "attempting to read %s "
			       "(%lu bytes [%lu, %lu))", name, bytes, b, e);

    //////////////////////////////////////////////////////////////////////
    // need to actually open it up -- need to modify the two lists
    // unload enough files to free up space
    if (bytes + ibis::fileManager::totalBytes() > maxBytes) {
	LOGGER(ibis::gVerbose > 5)
	    << "getFileSegment -- need to unload " << bytes << " bytes for \""
	    << name << "\", maxBytes=" << static_cast<double>(maxBytes)
	    << ", totalBytes=" << ibis::fileManager::totalBytes();
	ierr = unload(bytes);
    }
    else if (mapped.size() >= maxOpenFiles && bytes >= minMapSize) {
	LOGGER(ibis::gVerbose > 7)
	    << "getFileSegment -- need to unload some files before reading \""
	    << name << "\", maxBytes=" << static_cast<double>(maxBytes)
	    << ", totalBytes=" << ibis::fileManager::totalBytes();
	ierr = unload(0); // unload whatever can be unload
    }
    if (ierr < 0) {
	ibis::util::logMessage("getFileSegment", "unable to free up %lu "
			       "bytes to read the file %s",
			       bytes, name);
	ierr = -108;
	return st;
    }

    ibis::horometer timer;
    if (ibis::gVerbose > 7)
	timer.start();
    // "to map or not to map", that is the question
#if defined(HAVE_FILE_MAP)
    size_t sz = (pagesize << 2); // more than 4 pages
    if (mapped.size() > (maxOpenFiles >> 1)) {
	// compute the average size of the first ten files
	sz = 0;
	int cnt = 0;
	fileList::const_iterator it = mapped.begin();
	for (; cnt < 10 && it != mapped.end(); ++ cnt, ++ it)
	    sz += ((*it).second->size() / MIN_DOMAP_SIZE);
	if (cnt > 0)
	    sz /= cnt;
	if (sz < 1)
	    sz = MIN_DOMAP_SIZE;
	else
	    sz *= MIN_DOMAP_SIZE;
    }
    if (mapped.size() < maxOpenFiles && bytes >= sz) {
	// map the file read-only
	st = new ibis::fileManager::rofSegment(name, b, e);
    }
    else {
	// read the file into memory
	int fdes = UnixOpen(name, OPEN_READONLY);
	if (fdes >= 0) {
#if defined(_WIN32) && defined(_MSC_VER)
	    (void)_setmode(fdes, _O_BINARY);
#endif
	    st = new ibis::fileManager::storage(fdes, b, e);
	    UnixClose(fdes);
	}
	else if (ibis::gVerbose >= 0) {
	    ibis::util::logMessage("Warning", "ibis::file::getFileSegment"
				   "(%s, %lu, %lu) failed to open the file",
				   name, static_cast<long unsigned>(b),
				   static_cast<long unsigned>(e));
	}
    }
#else
    int fdes = UnixOpen(name, OPEN_READONLY);
    if (fdes >= 0) {
#if defined(_WIN32) && defined(_MSC_VER)
	(void)_setmode(fdes, _O_BINARY);
#endif
	st = new ibis::fileManager::storage(fdes, b, e);
	UnixClose(fdes);
    }
    else if (ibis::gVerbose >= 0) {
	ibis::util::logMessage("Warning", "ibis::file::getFileSegment"
			       "(%s, %lu, %lu) failed to open the file",
			       name, static_cast<long unsigned>(b),
			       static_cast<long unsigned>(e));
    }
#endif
    if (st)
	increaseUse(st->size(), "ibis::fileManager::getFileSegment");
    if (st->size() == bytes) {
	if (ibis::gVerbose > 5) {
	    ibis::util::logMessage("ibis::fileManager", "getFileSegment(%s) "
				   "completed %s %lu bytes", name,
				   (st->isFileMap()?"mmapping":"reading"),
				   static_cast<long unsigned>(st->size()));
	}
	if (ibis::gVerbose > 7) {
	    timer.stop();
	    double tcpu = timer.CPUTime();
	    double treal = timer.realTime();
	    double rt1 = tcpu > 0 ? (1e-6*st->size()/tcpu) : 0.0;
	    double rt2 = treal > 0 ? (1e-6*st->size()/treal) : 0.0;
	    ibis::util::logger lg(7);
	    lg.buffer() << "ibis::fileManager -- getFileSegment(" << name
			<< ") took " << treal <<  " sec(elapsed) ["
			<< tcpu << " sec(CPU)] to "
			<< (st->isFileMap()?"mmap ":"read ") << st->size()
			<< " bytes at a speed of " << rt2 << " MB/s ["
			<< rt1 << "]";
	    if (ibis::gVerbose > 11) {
		lg.buffer() << "\n";
		(void) st->printStatus(lg.buffer());
	    }
	}
    }
    else if (ibis::gVerbose >= 0) {
	ibis::util::logMessage
	    ("Warning", "ibis::fileManager::getFileSegment(%s) "
	     "failed retrieving %lu bytes (actual size %lu)",
	     name, bytes, static_cast<long unsigned>(st ? st->size() : 0U));
    }
    return st;
} // ibis::fileManager::getFileSegment

// unload enough space so that a file of size bytes can be loaded.
// caller must hold a mutex lock to prevent simutaneous invocation of this
// function.
int ibis::fileManager::unload(size_t size) {
    if (size > 0 && maxBytes > ibis::fileManager::totalBytes() &&
	size+ibis::fileManager::totalBytes() <= maxBytes) {
	// there is enough space
	return 0;
    }
    if (size > maxBytes) {
	ibis::util::logMessage("Warning", "request of %lu bytes can not "
			       "be honered, maxBytes=%.3G",
			       static_cast<long unsigned>(size),
			       static_cast<double>(maxBytes));
	return -113;
    }
    if (ibis::gVerbose > 4) {
	if (ibis::gVerbose > 8) {
	    ibis::util::logger lg(8);
	    printStatus(lg.buffer());
	}
	if (size > 0)
	    ibis::util::logMessage("ibis::fileManager::unload",
				   "need to free up %lu bytes of space"
				   "(totalBytes=%lu, maxBytes=%lu)",
				   static_cast<long unsigned>(size),
				   ibis::fileManager::totalBytes(), maxBytes);
	else
	    ibis::util::logMessage("ibis::fileManager::unload",
				   "want to free up all unused space"
				   "(totalBytes=%lu, maxBytes=%lu)",
				   ibis::fileManager::totalBytes(), maxBytes);
    }

    // collect the list of files that can be unloaded
    std::vector<fileList::iterator> candidates;
    fileList::iterator it;
    time_t startTime = time(0);
    time_t current = startTime;

    while (current < startTime+FILEMANAGER_UNLOAD_TIME) { // will wait
	size_t sum = 0; // sum of the total bytes that can can be unloaded
	for (it=mapped.begin(); it!=mapped.end(); ++it) {
	    if ((*it).second->inUse() == 0 &&
		(*it).second->pastUse() > 0) {
		sum += (*it).second->size();
	    }
	}
	for (it=incore.begin(); it!=incore.end(); ++it) {
	    if ((*it).second->inUse() == 0 &&
		(*it).second->pastUse() > 0) {
		sum += (*it).second->size();
	    }
	}
	if (maxBytes <= ibis::fileManager::totalBytes() ||
	    ibis::fileManager::totalBytes()-sum > maxBytes-size)
	    // invoke the external cleaners and recompute the total
	    invokeCleaners();

	// collects the candidates to be removed
	for (it=mapped.begin(); it!=mapped.end(); ++it) {
	    if ((*it).second->inUse() == 0 &&
		(*it).second->pastUse() > 0) {
		candidates.push_back(it);
	    }
	}
	for (it=incore.begin(); it!=incore.end(); ++it) {
	    if ((*it).second->inUse() == 0 &&
		(*it).second->pastUse() > 0) {
		candidates.push_back(it);
	    }
	}

	if (candidates.size() > 1) {
	    const size_t ncand = candidates.size();
	    // sort the candidates in descending order of scores
	    std::vector<float> scores(candidates.size());
	    for (unsigned i = 0; i < ncand; ++ i) {
		scores[i] = (*(candidates[i])).second->score();
	    }
	    for (unsigned stride = ncand/2; stride > 0; stride /= 2) {
		for (unsigned i = 0; i < ncand - stride; ++ i) {
		    if (scores[i] < scores[i+stride]) {
			float tmp = scores[i];
			scores[i] = scores[i+stride];
			scores[i+stride] = tmp;
			it = candidates[i];
			candidates[i] = candidates[i+stride];
			candidates[i+stride] = it;
		    }
		}
	    }
	}
	if (size == 0) {
	    if (ibis::gVerbose > 4 && candidates.size() > 0) {
		ibis::util::logMessage
		    ("ibis::fileManager::unload",
		     "unloading all inactive files (%lu)",
		     static_cast<long unsigned>(candidates.size()));
		if (ibis::gVerbose > 6) {
		    ibis::util::logger lg(6);
		    (*it).second->printStatus(lg.buffer());
		}
	    }
	    for (size_t i = 0; i < candidates.size(); ++ i) {
		it = candidates[i];
		roFile *tmp = (*it).second; // fileManager::roFile to delete
		if (tmp->mapped) {
		    mapped.erase(it);
		}
		else {
		    incore.erase(it);
		}
		delete tmp;
	    }
	    return 0;
	}
	else if (candidates.size() > 0) {
	    // note: totalBytes is updated when an object is deleted
	    while (candidates.size() > 0 &&
		   maxBytes-size < ibis::fileManager::totalBytes())  {
		it = candidates.back();
		roFile *tmp = (*it).second;
		if (ibis::gVerbose > 4) {
		    ibis::util::logger lg(4);
		    lg.buffer() << "ibis::fileManager::unload -- "
			"unloading file \"" << (*it).first << "\"";
		    if (ibis::gVerbose > 7) {
			lg.buffer() << "\n";
			(*it).second->printStatus(lg.buffer());
		    }
		}

		if (tmp->mapped) {
		    mapped.erase(it);
		}
		else {
		    incore.erase(it);
		}
		delete tmp; // remove the target selected
		candidates.resize(candidates.size()-1);
	    }
	    if (maxBytes-size >= ibis::fileManager::totalBytes())
		return 0;
	}

	candidates.clear(); // remove space taken up by candiates
// 	if (mapped.size()+incore.size() < 3) { // allow at least three files
// 	    return 1;
// 	}

	if (nwaiting > 0) {
	    // a primitive strategy: only one thread can wait for any
	    // positive amount of space
	    if (ibis::gVerbose >= 0)
		ibis::util::logMessage("Warning", "ibis::fileManager: "
				       "Another thread is waiting for "
				       "memory already, yield ...");
	    return -108;
	}

	++ nwaiting;
	if (ibis::gVerbose > 3) {
	    ibis::util::logger lg(3);
	    lg.buffer() << "ibis::fileManager::unload unable to find " << size
			<< " bytes of free space (totalBytes="
			<< ibis::fileManager::totalBytes() << ", maxBytes="
			<< maxBytes << "), will wait...";
	    if (ibis::gVerbose > 6) {
		lg.buffer() << "\n";
		printStatus(lg.buffer());
	    }
	}
	int ierr = 0;
	// has to wait for condition change
#if defined(CLOCK_REALTIME) && (defined(HAVE_STRUCT_TIMESPEC) || defined(__USE_POSIX) || _POSIX_VERSION+0 > 199900)
	// has clock_gettime to get the current time
	struct timespec tsp;
	ierr = clock_gettime(CLOCK_REALTIME, &tsp);
	if (ierr == 0) {
	    tsp.tv_sec += (FILEMANAGER_UNLOAD_TIME > 4 ?
			   (FILEMANAGER_UNLOAD_TIME  >> 2) : 1);
	    ierr = pthread_cond_timedwait(&cond, &mutex, &tsp);
	}
	else {
	    tsp.tv_sec = current + (FILEMANAGER_UNLOAD_TIME > 4 ?
				    (FILEMANAGER_UNLOAD_TIME  >> 2) : 1) + 1;
	    tsp.tv_nsec = 0;
	    ierr = pthread_cond_timedwait(&cond, &mutex, &tsp);
	}
#elif (defined(_WIN32) && defined(_MSC_VER)) || defined(HAVE_STRUCT_TIMESPEC) || defined(__USE_POSIX) || _POSIX_VERSION+0 > 199900
	// assume pthread implementation has pthread_cond_timedwait
	struct timespec tsp;
	tsp.tv_sec = current + (FILEMANAGER_UNLOAD_TIME > 4 ?
				(FILEMANAGER_UNLOAD_TIME  >> 2) : 1) + 1;
	tsp.tv_nsec = 0;
	ierr = pthread_cond_timedwait(&cond, &mutex, &tsp);
#else
	ierr = pthread_cond_wait(&cond, &mutex);
#endif
	-- nwaiting;
	if (ierr != 0 && ierr != ETIMEDOUT) {
	    ibis::util::logMessage("Warning", "fileManager::unload unable "
				   "to wait for release of memory ... %s",
				   strerror(ierr));
	    break; // get out of the while loop
	}
	current = time(0);
    } // while (...)

    // time-out
    LOGGER(ibis::gVerbose >= 0)
	<< "Warning -- fileManager::unload unable to free enough space for "
	<< size << " byte" <<  (size>1 ? "s" : "") << " (totalBytes="
	<< ibis::fileManager::totalBytes() << ", maxBytes=" << maxBytes << ")";
    return -109;
} // ibis::fileManager::unload

void ibis::fileManager::invokeCleaners() const {
    if (ibis::gVerbose > 5)
	ibis::util::logMessage("ibis::fileManager", "invoking registered "
			       "external cleaners ...");
    const uint64_t before = ibis::fileManager::totalBytes();
    for (cleanerList::const_iterator it = cleaners.begin();
	 it != cleaners.end();
	 ++it)
	(*it)->operator()();

    if (ibis::fileManager::totalBytes() < before) {
	LOGGER(ibis::gVerbose > 7)
	    << "ibis::fileManager -- external cleaners reduce totalBytes from "
	    << before << " to " << ibis::fileManager::totalBytes();
    }
    else if (ibis::gVerbose > 5) {
	ibis::util::logger lg(5);
	lg.buffer() << "ibis::fileManager -- external cleaners "
		    << "did not reduce the total bytes ("
		    << ibis::fileManager::totalBytes() << ")";
	if (ibis::gVerbose > 10) {
	    lg.buffer() << "\n";
	    printStatus(lg.buffer());
	}
    }
} // ibis::fileManager::invokeCleaners

/// To be used by clients that are aware of the memory usages of in-memory
/// objects since the in-memory objects based on ibis::fileManager::storage
/// does not produce signals when they are freed.
void ibis::fileManager::signalMemoryAvailable() const {
    mutexLock lock(*this, "signalMemoryAvailable");
    if (nwaiting > 0) {
	int ierr =
	    pthread_cond_signal(&(ibis::fileManager::instance().cond));
	if (ierr != 0 && ibis::gVerbose >= 0) {
	    ibis::util::logMessage("Warning", "signalMemoryAvailable call "
				   "to pthread_cond_signal with return "
				   "code %d", ierr);
	}
    }
} // ibis::fileManager::signalMemoryAvailable

//////////////////////////////////////////////////////////////////////
// functions for the storage class
//
// allocate storage for an array of the specified size
ibis::fileManager::storage::storage(size_t n)
    : name(0), m_begin(0), m_end(0), nacc(0), nref() {
    if (n < 8) n = 8; // at least 8 bytes
    if (n+ibis::fileManager::totalBytes() > ibis::fileManager::maxBytes) {
	ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					 "storage::ctor");
	int ierr = ibis::fileManager::instance().unload(n);
	if (ierr < 0) {
	    ibis::util::logMessage("Warning", "storage::ctor unable to find "
				   "%lu bytes of space in memory",
				   static_cast<long unsigned>(n));
	    throw ibis::bad_alloc("storage::ctor(memory) failed");
	}
    }
    m_begin = static_cast<char*>(malloc(n));
    if (m_begin != 0) { // malloc was a success
	m_end = m_begin + n;
	std::string evt = "fileManager::storage";
	if (ibis::gVerbose > 8) {
	    std::ostringstream oss;
	    oss << "(" << n << " --> " << static_cast<void*>(m_begin) << ")";
	    evt += oss.str();
	}
	ibis::fileManager::increaseUse(n, evt.c_str());
    }
    else {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "Warning -- storage: unable to malloc(" << n
			<< ") bytes of storage";
	    if (ibis::gVerbose > 1) {
		lg.buffer() << "\n";
		printStatus(lg.buffer()); // dump the current list of files
	    }
	}
	throw ibis::bad_alloc("unable to allocate new storage object");
    }
} // ibis::fileManager::storage::storage

// read part of a open file [begin, end)
ibis::fileManager::storage::storage(const int fdes,
				    const off_t begin,
				    const off_t end)
    : name(0), m_begin(0), m_end(0), nacc(0), nref() {
    if (end <= begin) return;
    long nbytes = end - begin;
    if (nbytes+ibis::fileManager::totalBytes() > ibis::fileManager::maxBytes) {
	ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					 "storage::ctor");
	int ierr = ibis::fileManager::instance().unload(nbytes);
	if (ierr < 0) {
	    ibis::util::logMessage("Warning", "storage is unable to find "
				   "%ld bytes of space to read file file "
				   "descriptor %d", nbytes, fdes);
	    throw ibis::bad_alloc("storage::ctor(read file) failed");
	}
	else if (ibis::gVerbose > 10) {
	    ibis::util::logMessage("storage::read",
				   "allocated %ld bytes at 0x%lx",
				   nbytes, m_begin);
	}
    }
    m_begin = static_cast<char*>(malloc(nbytes));
    if (m_begin != 0) { // malloc was a success
	long nread = 0;
	nread = UnixSeek(fdes, begin, SEEK_SET);
	if (ibis::gVerbose < 8) {
	    nread = UnixRead(fdes, m_begin, nbytes);
	}
	else {
	    ibis::horometer timer;
	    timer.start();
	    nread = UnixRead(fdes, m_begin, nbytes);
	    timer.stop();
	    if (nread == nbytes) {
		double tcpu = timer.CPUTime();
		double treal = timer.realTime();
		double rt1 = tcpu > 0 ? (1e-6*nbytes/tcpu) : 0.0;
		double rt2 = treal > 0 ? (1e-6*nbytes/treal) : 0.0;
		ibis::util::logMessage("storage::ctor", "read %lu bytes "
				       "from file descriptor %d, "
				       "took %g sec(elapsed) [%g sec(CPU)] to "
				       "read at a speed of %g MB/s [%g]",
				       static_cast<long unsigned>(nbytes),
				       fdes, treal, tcpu, rt2, rt1);
	    }
	}
	if (nread != nbytes) {
	    ibis::util::logMessage("Warning", "ibis::fileManager::storage "
				   "reading (fdes=%i) expected %ld bytes "
				   "at 0x%lx but only read %ld", fdes,
				   nbytes, m_begin, nread);
	}

	m_end = m_begin + nread;
	ibis::fileManager::instance().recordPages(begin, end);
	std::string evt = "fileManager::storage";
	if (ibis::gVerbose > 8) {
	    std::ostringstream oss;
	    oss << "(fdes=" << fdes  << ", begin=" << begin << ", end="
		<< end << " --> " << static_cast<void*>(m_begin) << ")";
	    evt += oss.str();
	}
	ibis::fileManager::instance().increaseUse(nbytes, evt.c_str());
    }
    else {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "Warning -- ibis::fileManager::storage (fdes="
			<< fdes << ") is unable to allocate " << nbytes
			<< "bytes\n";
	    printStatus(lg.buffer()); // dump the current list of files
	}
	throw ibis::bad_alloc("unable to allocate space to read a file");
    }
} // ibis::fileManager::storage::storage

// copy only the part between begin and end [begin, end)
ibis::fileManager::storage::storage(const char* begin, const char* end)
    : name(0), m_begin(0), m_end(0), nacc(0), nref() {
    if (end <= begin) return;
    long nbytes = end - begin;
    if (nbytes+ibis::fileManager::totalBytes() > ibis::fileManager::maxBytes) {
	ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					 "storage::ctor");
	int ierr = ibis::fileManager::instance().unload(nbytes);
	if (ierr < 0) {
	    ibis::util::logMessage("Warning", "storage is unable to find "
				   "%ld bytes of space to copy from 0x%.8lx",
				   nbytes,
				   reinterpret_cast<long unsigned>(begin));
	    throw ibis::bad_alloc("storage::ctor(copy memory) failed");
	}
    }
    m_begin = static_cast<char*>(malloc(nbytes));
    if (m_begin != 0) { // malloc was a success
	(void)memcpy(m_begin, begin, nbytes);
	m_end = m_begin + nbytes;
	std::string evt = "fileManager::storage";
	if (ibis::gVerbose > 8) {
	    std::ostringstream oss;
	    oss << "(" << static_cast<const void*>(begin) << " -- "
		<< static_cast<const void*>(end) << " to "
		<< static_cast<const void*>(m_begin) << ")";
	    evt += oss.str();
	}
	ibis::fileManager::instance().increaseUse(nbytes, evt.c_str());
    }
    else {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "Warning -- ibis::fileManager copy "
		"constructor is unable to allocate " << nbytes << "bytes\n";
	    printStatus(lg.buffer()); // dump the current list of files
	}
	throw ibis::bad_alloc("unable to copy of in-memory object");
    }
} // ibis::fileManager::storage::storage

// copy constructor -- make an in-memory copy
ibis::fileManager::storage::storage(const ibis::fileManager::storage& rhs)
    : name(0), m_begin(0), m_end(0), nacc(0), nref() {
    unsigned long nbytes = rhs.size();
    if (nbytes+ibis::fileManager::totalBytes() > ibis::fileManager::maxBytes) {
	ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					 "storage::ctor");
	int ierr = ibis::fileManager::instance().unload(nbytes);
	if (ierr < 0) {
	    ibis::util::logMessage("Warning", "storage is unable to find "
				   "%lu bytes of space to make an "
				   "in-memory copy", nbytes);
	    throw ibis::bad_alloc("storage::ctor(copy) failed");
	}
    }
    m_begin = static_cast<char*>(malloc(nbytes));
    if (m_begin != 0) { // malloc was a success
	(void)memcpy(static_cast<void*>(m_begin),
		     static_cast<void*>(rhs.m_begin), nbytes);
	m_end = m_begin + nbytes;
	std::string evt = "fileManager::storage";
	if (ibis::gVerbose > 8) {
	    std::ostringstream oss;
	    oss << "(" << static_cast<void*>(rhs.m_begin) << ", "
		<< rhs.size() << " --> " << static_cast<void*>(m_begin)
		<< ")";
	    evt += oss.str();
	}
	ibis::fileManager::increaseUse(nbytes, evt.c_str());
    }
    else {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "Warning -- ibis::fileManager copy constructor "
		"is unable to allocate " << nbytes << " bytes\n";
	    printStatus(lg.buffer()); // dump the current list of files
	}
	throw ibis::bad_alloc("unable to copy a storage object");
    }
} // ibis::fileManager::storage::storage

// assignment -- make an in-memory copy through the copy constructor
const ibis::fileManager::storage&
ibis::fileManager::storage::operator=
(const ibis::fileManager::storage& rhs) {
    storage tmp(rhs);
    swap(tmp);
    return *this;
} // ibis::fileManager::storage::operator=

// copy -- make an in-meory copy
void ibis::fileManager::storage::copy
(const ibis::fileManager::storage& rhs) {
    clear(); // clear the current content

    unsigned long nbytes = rhs.size();
    if (nbytes+ibis::fileManager::totalBytes() > ibis::fileManager::maxBytes) {
	ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					 "storage::copy");
	int ierr = ibis::fileManager::instance().unload(nbytes);
	if (ierr < 0) {
	    ibis::util::logMessage("Warning", "storage is unable to find "
				   "%lu bytes of space to make an "
				   "in-memory copy", nbytes);
	    throw ibis::bad_alloc("storage::ctor(copy) failed");
	}
    }
    m_begin = static_cast<char*>(malloc(nbytes));
    if (m_begin != 0) {
	(void)memcpy(static_cast<void*>(m_begin),
		     static_cast<void*>(rhs.m_begin), nbytes);
	m_end = m_begin + nbytes;
	std::string evt = "fileManager::storage::copy";
	if (ibis::gVerbose > 8) {
	    std::ostringstream oss;
	    oss << "(" << static_cast<void*>(rhs.m_begin) << ", "
		<< rhs.size() << " --> " << static_cast<void*>(m_begin)
		<< ")";
	    evt += oss.str();
	}
	ibis::fileManager::increaseUse(nbytes, evt.c_str());
    }
    else {
	if (ibis::gVerbose >= 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "Warning -- ibis::fileManager::storage::copy() "
		"unable to allocate " << nbytes << " bytes\n";
	    printStatus(lg.buffer()); // dump the current list of files
	}
	throw ibis::bad_alloc("unable to allocate memory in copy");
    }
} // ibis::fileManager::storage::copy

// enlarge the current array by 61.8% if n is smaller than the current size
void ibis::fileManager::storage::enlarge(size_t nelm) {
    std::string evt = "fileManager::storage::enlarge";
    if (ibis::gVerbose > 8) {
	std::ostringstream oss;
	oss << "(" << static_cast<void*>(m_begin) << ": size() " << size()
	    << " --> " << nelm << ")";
	evt += oss.str();
    }
    if (m_begin == 0 || m_begin >= m_end) { // empty storage
	if (nelm < 8) nelm = 8;
	if (nelm+ibis::fileManager::totalBytes() >
	    ibis::fileManager::maxBytes) {
	    ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					     evt.c_str());
	    int ierr = ibis::fileManager::instance().unload(nelm);
	    if (ierr < 0) {
		ibis::util::logMessage("Warning", "%s is "
				       "unable to find %lu bytes of space "
				       "in memory", evt.c_str(),
				       static_cast<long unsigned>(nelm));
		throw ibis::bad_alloc("storage::enlarge failed");
	    }
	}
	m_begin = static_cast<char*>(malloc(nelm));
	if (m_begin != 0) {
	    m_end = m_begin + nelm;
	}
	else { // try malloc again
	    int ierr = 0;
	    {
		ibis::fileManager::mutexLock
		    lck(ibis::fileManager::instance(), evt.c_str());
		ierr = ibis::fileManager::instance().unload(nelm);
	    }
	    if (ierr < 0) {
		m_end = m_begin;
		ibis::util::logMessage("Warning", "%s is "
				       "unable to find %lu bytes of space "
				       "in memory", evt.c_str(),
				       static_cast<long unsigned>(nelm));
		throw ibis::bad_alloc("storage::enlarge (retry) failed");
	    }
	    m_begin = static_cast<char*>(malloc(nelm));
	    if (m_begin != 0) {
		m_end = m_begin + nelm;
	    }
	    else {
		m_begin = 0; m_end = 0;
		if (ibis::gVerbose >= 0) {
		    ibis::util::logger lg(0);
		    lg.buffer()
			<< "Warning -- " << evt << " unable to allocate "
			<< nelm << " bytes\n";
		    printStatus(lg.buffer()); // dump the current list of files
		}
		throw ibis::bad_alloc("failed allocation in enlarge");
	    }
	}
	LOGGER(ibis::gVerbose > 10)
	    << evt << " -- allocated " << nelm << " bytes at "
	    << static_cast<void*>(m_begin);
	ibis::fileManager::increaseUse(nelm, evt.c_str());
    }
    else { // resize the current storage object
	const size_t oldsize = size();
	size_t n = (nelm>oldsize ? nelm : oldsize<1024 ? oldsize+oldsize :
		    static_cast<size_t>(1.6180339887498948482*oldsize));
	if (n+ibis::fileManager::totalBytes() > ibis::fileManager::maxBytes) {
	    ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					     evt.c_str());
	    int ierr = ibis::fileManager::instance().unload(n);
	    if (ierr < 0) {
		ibis::util::logMessage("Warning", "%s is "
				       "unable to find %lu bytes of space "
				       "in memory", evt.c_str(),
				       static_cast<long unsigned>(n));
		throw ibis::bad_alloc("storage::enlarge failed");
	    }
	}
	const void* m_old = m_begin;
	m_begin =
	    static_cast<char*>(realloc(static_cast<void*>(m_begin), n));
	if (m_begin != 0) {
	    m_end = m_begin + n;
	}
	else if (oldsize == 0) {
	    // previously contains nothing, try malloc again
	    int ierr = 0;
	    {
		ibis::fileManager::mutexLock
		    lck(ibis::fileManager::instance(), evt.c_str());
		ierr = ibis::fileManager::instance().unload(n);
	    }
	    if (ierr < 0) {
		m_end = m_begin;
	    }
	    m_begin = static_cast<char*>(malloc(n));
	    if (m_begin != 0) {
		m_end = m_begin + n;
	    }
	    else {
		m_end = 0;
		m_begin = 0;
		if (ibis::gVerbose >= 0) {
		    ibis::util::logger lg(0);
		    lg.buffer()
			<< "Warning -- " << evt << " unable to allocate "
			<< n << " bytes\n";
		    printStatus(lg.buffer()); // dump the current list of
					      // files
		}
		throw ibis::bad_alloc("failed allocation in enlarge");
	    }
	}
	else { // lost old content, can not recover
	    m_end = 0;
	    m_begin = 0;
	    ibis::fileManager::decreaseUse(oldsize, evt.c_str());
	    if (ibis::gVerbose >= 0) {
		ibis::util::logger lg(0);
		lg.buffer() << "Warning -- " << evt << " unable to reallocate "
			    << n << " bytes, existing content lost\n";
		printStatus(lg.buffer()); // dump the current list of files
	    }
	    throw ibis::bad_alloc("failed realloc in enlarge");
	}
	LOGGER(ibis::gVerbose > 10)
	    << evt << " -- reallocated " << oldsize << " byte"
	    << (oldsize>1?"s":"") << " to " << static_cast<void*>(m_begin)
	    << " with capacity of " << n << " bytes";
	ibis::fileManager::increaseUse(n-oldsize, evt.c_str());
    }
} // ibis::fileManager::storage::enlarge

// actually freeing the storage allocated
void ibis::fileManager::storage::clear() {
    std::string evt = "fileManager::storage::clear";
    if (ibis::gVerbose > 8) {
	std::ostringstream oss;
	if (name)
	    oss << "(" << name << ", " << static_cast<void*>(m_begin) << ")";
	else
	    oss << "(" << static_cast<void*>(m_begin) << ")";
	evt += oss.str();
    }
    if (nref() > 0) {
	LOGGER(ibis::gVerbose > 3)
	    << "Warning -- " << evt << " -- storage object at 0x "
	    << static_cast<void*>(m_begin) << " busy (nref=" << nref() << ")";
	return;
    }

    ibis::fileManager::decreaseUse(size(), evt.c_str());
    free(m_begin);
    m_begin = 0;
    m_end = 0;
    nacc = 0;
    if (name) {
	delete [] name;
	name = 0;
    }
} // ibis::fileManager::storage::clear

size_t ibis::fileManager::storage::printStatus(std::ostream& out) const {
    if (name)
	out << "file name       " << name << "\n";
    out << "storage @ " << static_cast<void*>(m_begin);
    if (m_begin != 0)
	out << ", as int " << *reinterpret_cast<long*>(m_begin)
	    << ", as float " << *reinterpret_cast<float*>(m_begin)
	    << ", as double " << *reinterpret_cast<double*>(m_begin);
    out << "\n# of bytes      " << size()
	<< "\n# of past acc   " << nacc
	<< "\n# of active acc " << nref()
	<< std::endl;
    return size();
} // ibis::fileManager::storage::printStatus

// read part of a open file [begin, end)
off_t ibis::fileManager::storage::read(const int fdes,
				       const off_t begin,
				       const off_t end) {
    off_t nread = 0;
    if (end <= begin) return nread;

    std::string evt = "fileManager::storage::read";
    if (ibis::gVerbose >= 0) {
	std::ostringstream oss;
	oss << "(" << "fdes=" << fdes << ", begin=" << begin << ", end="
	    << end << ")";
	evt += oss.str();
    }
    off_t nbytes = end - begin;
    if (m_begin == 0U) {
	if (nbytes+ibis::fileManager::totalBytes() >
	    ibis::fileManager::maxBytes) {
	    ibis::fileManager::mutexLock lck(ibis::fileManager::instance(),
					     evt.c_str());
	    int ierr = ibis::fileManager::instance().unload(nbytes);
	    if (ierr < 0) {
		ibis::util::logMessage("Warning", "%s is "
				       "unable to find %lu bytes of space "
				       "in memory", evt.c_str(),
				       static_cast<long unsigned>(nbytes));
		throw ibis::bad_alloc("storage::read failed");
	    }
	}
	m_begin = static_cast<char*>(malloc(nbytes));
	if (m_begin == 0){
	    m_end = m_begin;
	    {
		ibis::util::logger lg(0);
		lg.buffer() << "Warning -- " << evt
			    << " is unable to allocate " << nbytes
			    << " bytes\n";
		printStatus(lg.buffer()); // dump the current list of files
	    }
	    throw ibis::bad_alloc("failed to allocate space for reading");
	}
	else if (ibis::gVerbose > 10) {
	    ibis::util::logMessage(evt.c_str(),
				   "allocated %lu bytes at 0x%lx",
				   static_cast<long unsigned>(nbytes),
				   m_begin);
	}
	ibis::fileManager::increaseUse(nbytes, evt.c_str());
	m_end = m_begin + nbytes;
    }
    else if (m_end < nbytes+m_begin) { // not enough space
	enlarge(nbytes);
    }

    nread = UnixSeek(fdes, begin, SEEK_SET);
    if (nread != begin) {
	ibis::util::logMessage("Warning", "ibis::fileManager::storage"
			       "read(fdes=%d) failed to seek to %lu ... %s",
			       fdes, static_cast<long unsigned>(begin),
			       (errno!=0 ? strerror(errno) : "???"));
	return 0;
    }

    if (ibis::gVerbose < 8) {
	nread = UnixRead(fdes, m_begin, nbytes);

	ibis::fileManager::instance().recordPages(begin, end);
	if (nread != nbytes) {
	    ibis::util::logMessage("Warning", "ibis::fileManager::storage "
				   "read(fdes=%i) allocated %ld bytes "
				   "at 0x%lx but only read %lu", fdes,
				   static_cast<long unsigned>(nbytes),
				   m_begin, static_cast<long unsigned>(nread));
	}
    }
    else {
	ibis::horometer timer;
	timer.start();
	nread = UnixRead(fdes, m_begin, nbytes);
	timer.stop();

	ibis::fileManager::instance().recordPages(begin, end);
	if (nread == nbytes) {
	    double tcpu = timer.CPUTime();
	    double treal = timer.realTime();
	    double rt1 = tcpu > 0 ? (1e-6*nbytes/tcpu) : 0.0;
	    double rt2 = treal > 0 ? (1e-6*nbytes/treal) : 0.0;
	    ibis::util::logMessage("storage::read", "read %lu bytes from "
				   "file descriptor %d, "
				   "took %g sec(elapsed) [%g sec(CPU)] to "
				   "read at a speed of %g MB/s [%g]",
				   static_cast<long unsigned>(nbytes),
				   fdes, treal, tcpu, rt2, rt1);
	}
	else {
	    ibis::util::logMessage("Warning", "ibis::fileManager::storage "
				   "read(fdes=%i) allocated %ld bytes "
				   "at 0x%lx but only read %lu", fdes,
				   static_cast<long unsigned>(nbytes),
				   m_begin, static_cast<long unsigned>(nread));
	}
    }
    return nread;
} // ibis::fileManager::storage::read

void ibis::fileManager::storage::write(const char* file) const {
    size_t n, i;
    FILE *in = fopen(file, "wb");
    if (in == 0) {
	ibis::util::logMessage("Warning", "storage::write() is unable open "
			       "file \"%s\" ... %s", file,
			       (errno!=0 ? strerror(errno) :
				"no free stdio stream"));
	return;
    }

    n = m_end - m_begin;
    i = fwrite(static_cast<void*>(m_begin), 1, n, in);
    fclose(in); // close the file
    if (i != n) {
	ibis::util::logMessage("Warning", "storage::write() expects to write "
			       "%lu bytes to \"%s\", but only wrote %lu",
			       static_cast<long unsigned>(n),
			       file, static_cast<long unsigned>(i));
	remove(file); // remove the file
    }
} // ibis::fileManager::storage::write

////////////////////////////////////////////////////////////////////////
// member functions of fileManager::roFile (the read-only file)
//
// the counters nref and nacc should be modified under the control of some
// mutex lock -- however doing so will significantly increate the execution
// time of these simple functions.  Currently we do not use this approach
// but rely on the fact that we should get the access counts correctly.
//
// starting to use a file
void ibis::fileManager::roFile::beginUse() {
    // acquire a read lock
    if (name != 0) {
	ibis::fileManager::instance().gainReadAccess(name);
    }
    lastUse = time(0);
    ++ nref;
} // ibis::fileManager::roFile::beginUse

// done using a file
void ibis::fileManager::roFile::endUse() {
    const uint32_t nr0 = -- nref; // number of current references
    ++ nacc; // number of past accesses

    // relinquish the read lock
    if (name != 0) {
	ibis::fileManager::instance().releaseAccess(name);
	// signal to ibis::fileManager that this file is ready for deletion
	if (nr0 == 0)
	    pthread_cond_signal(&(ibis::fileManager::instance().cond));
    }
} // ibis::fileManager::roFile::endUse

// actually freeing the storage allocated
void ibis::fileManager::roFile::clear() {
    std::string evt = "fileManager::roFile::clear";
    if (ibis::gVerbose > 8) {
	std::ostringstream oss;
	if (name)
	    oss << "(" << name << ", " << static_cast<void*>(m_begin) << ")";
	else
	    oss << "(" << static_cast<void*>(m_begin) << ")";
	evt += oss.str();
    }
    if (nref() > 0) {
	LOGGER(ibis::gVerbose > 3)
	    << evt << " -- storage 0x" << m_begin << " is busy (nref="
	    << nref() << ") and can't be cleared";
	return;
    }

    size_t sz = size();
    if (mapped == 0) {
	free(m_begin);
    }
    else {
#if defined(_WIN32) && defined(_MSC_VER)
	UnmapViewOfFile(map_begin);
	CloseHandle(fmap);
	CloseHandle(fdescriptor);
#elif (HAVE_MMAP+0 > 0)
	munmap((caddr_t)map_begin, fsize);
	UnixClose(fdescriptor);
#endif
    }

    if (name) {
	delete [] name;
	name = 0;
    }

    m_end = 0;
    m_begin = 0;
    ibis::fileManager::decreaseUse(sz, evt.c_str());
} // ibis::fileManager::roFile::clear

size_t ibis::fileManager::roFile::printStatus(std::ostream& out) const {
    if (name) 
	out << "file name: " << name << "\n";
    return printBody(out);
} // ibis::fileManager::roFile::printStatus

size_t ibis::fileManager::roFile::printBody(std::ostream& out) const {
    char tstr0[28], tstr1[28];
    ibis::util::secondsToString(opened, tstr0);
    ibis::util::secondsToString(lastUse, tstr1);
    out << "storage @ " << static_cast<void*>(m_begin);
    if (m_begin != 0)
	out << ", as int " << *reinterpret_cast<long*>(m_begin)
	    << ", as float " << *reinterpret_cast<float*>(m_begin)
	    << ", as double " << *reinterpret_cast<double*>(m_begin)
	    << "\n# of bytes      " << size();
#if (HAVE_MMAP+0 > 0)
    if (fdescriptor >= 0) {
	out << "\nfile descriptor " << fdescriptor
	    << "\nfmap size       " << fsize
	    << "\nbase address    " << map_begin;
    }
#elif defined(_WIN32) && defined(_MSC_VER)
    if (fdescriptor != INVALID_HANDLE_VALUE) {
	out << "\nfile descriptor " << fdescriptor
	    << "\nfmap handle     " << fmap
	    << "\nbase address    " << map_begin;
    }
#endif
    out << "\nopened at       " << tstr0
	<< "\nlast used at    " << tstr1
	<< "\n# of past acc   " << nacc
	<< "\n# of active acc " << nref()
	<< "\nmapped          " << (mapped?"y\n":"n\n") << std::endl;
    return size();
} // ibis::fileManager::roFile::printBody

void ibis::fileManager::roFile::read(const char* file) {
    if (file == 0 || *file == 0) return;
    if (nref() == 0) {
	if (name) {
	    ibis::fileManager::instance().flushFile(name);
	}
	else {
	    clear();
	}
    }
    else {
	ibis::util::logMessage("Warning", "ibis::fileManager::roFile is busy "
			       "and cann't read new content");
	return;
    }
    doRead(file);
    if (m_end > m_begin) { // presumably read the file correctly
	ibis::fileManager::instance().recordFile(this);
    }
    else {
	ibis::util::logMessage("ibis::fileManager::roFile", "did NOT "
			       "read anything from file \"%s\"", file);
	clear();
    }
} // ibis::fileManager::roFile::read

/// Read the content of a file into memory.
void ibis::fileManager::roFile::doRead(const char* file) {
    size_t n, i;
    // first find out the size of the file
    Stat_T tmp;
    if (0 == UnixStat(file, &tmp)) { // get stat correctly
	n = tmp.st_size;
    }
    else {
	ibis::util::logMessage("Warning", "roFile::read() is unable find out "
			       "the size of \"%s\"", file);
	return;
    }

    int in = UnixOpen(file, OPEN_READONLY);
    if (in < 0) {
	ibis::util::logMessage("Warning", "roFile::read() is unable open "
			       "file \"%s\" ... %s", file,
			       (errno ? strerror(errno) :
				"no free stdio stream"));
	return;
    }

    clear(); // clear the current content
    m_begin = static_cast<char *>(malloc(n));
    if (m_begin == 0) {
	LOGGER(ibis::gVerbose > 5)
	    << "roFile::read(" << file << ") -- failed to allocate " << n
	    << " bytes of memory, attempt to free all that can be freed";
	ibis::fileManager::instance().unload(0);

	m_begin = static_cast<char *>(malloc(n));
	if (m_begin == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- roFile::read(" << file << ") failed to allocate "
		<< n << " bytes of memory after two tries";
	    UnixClose(in);
	    m_end = 0;
	    return;
	}
    }

#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(in, _O_BINARY);
#endif
    i = UnixRead(in, static_cast<void*>(m_begin), n);
    ibis::fileManager::instance().recordPages(0, n);
    UnixClose(in); // close the file
    if (i == static_cast<size_t>(-1)) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- roFile::read(" << file
	    << ") encountered an error (errno=" << errno
	    << ") while calling function read ... " << strerror(errno);
	free(m_begin);
	m_begin = 0;
	m_end = 0;
	return;
    }
    else if (i != n) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- roFile::read(" << file << ") expects to read "
	    << n << " bytes from, but only got " << i;
    }
    else if (ibis::gVerbose > 6) {
	ibis::util::logMessage("roFile::doRead", "read %lu bytes from "
			       "file \"%s\" to 0x%lx",
			       static_cast<long unsigned>(n), file, m_begin);
    }
    name = ibis::util::strnewdup(file);
    m_end = m_begin + i;
    opened = time(0);
} // doRead

/// Read a portion of a file into memory.
/// Do NOT record the name of the file.  This is different from the one that
/// read the whole file which automatically records the name of the file.
void
ibis::fileManager::roFile::doRead(const char* file, off_t b, off_t e) {
    if (file == 0 || *file == 0 || b >= e)
	return;

    long i, n = e - b;
    int in = UnixOpen(file, OPEN_READONLY);
    if (in < 0) {
	ibis::util::logMessage("Warning", "roFile::read() is unable open "
			       "file \"%s\" ... %s", file,
			       (errno ? strerror(errno) :
				"no free stdio stream"));
	return;
    }

    clear(); // clear the current content
    m_begin = static_cast<char *>(malloc(n));
    if (m_begin == 0) {
	LOGGER(ibis::gVerbose > 5)
	    << "roFile::read(" << file << ") -- failed to allocate " << n
	    << " bytes of memory, attempt to free all that can be freed";
	ibis::fileManager::instance().unload(0);

	m_begin = static_cast<char *>(malloc(n));
	if (m_begin == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- roFile::read(" << file << ", " << b << ", "
		<< e << ") failed to allocate " << n
		<< " bytes of memory afte rtwo tries";
	    UnixClose(in);
	    m_end = 0;
	    return;
	}
    }

#if defined(_WIN32) && defined(_MSC_VER)
    (void)_setmode(in, _O_BINARY);
#endif
    i = UnixRead(in, static_cast<void*>(m_begin), n);
    ibis::fileManager::instance().recordPages(b, e);
    UnixClose(in); // close the file
    if (i == -1L) {
	if (ibis::gVerbose >= 0)
	    ibis::util::logMessage("Warning", "roFile::read(%s, %lu, %lu) "
				   "encountered an error (errno=%d) "
				   "calling function read ... %s",
				   file, static_cast<long unsigned>(b),
				   static_cast<long unsigned>(e),
				   errno, strerror(errno));
	free(m_begin);
	m_begin = 0;
	m_end = 0;
	return;
    }
    else if (i != n) {
	if (ibis::gVerbose >= 0)
	    ibis::util::logMessage("Warning", "roFile::read(%s, %lu, %lu) "
				   "expects to read %lu bytes, but only "
				   "got %lu", file,
				   static_cast<long unsigned>(b),
				   static_cast<long unsigned>(e),
				   static_cast<long unsigned>(n),
				   static_cast<long unsigned>(i));
    }
    else if (ibis::gVerbose > 6) {
	ibis::util::logMessage("roFile::doRead", "read %lu bytes from "
			       "file \"%s\"[%lu, %lu) to 0x%lx",
			       static_cast<long unsigned>(n), file,
			       static_cast<long unsigned>(b),
			       static_cast<long unsigned>(e), m_begin);
    }
    m_end = m_begin + i;
    opened = time(0);

    std::string evt = "fileManager::roFile::doRead";
    if (ibis::gVerbose > 8) {
	std::ostringstream oss;
	oss << "(" << name << ", " << b << ", " << e
	    << static_cast<void*>(m_begin) << ", " << n << ")";
	evt += oss.str();
    }
    ibis::fileManager::instance().increaseUse(n, evt.c_str());
} // doRead

#if defined(HAVE_FILE_MAP)
void ibis::fileManager::roFile::mapFile(const char* file) {
    if (file == 0 || *file == 0) return;
    if (nref() == 0) {
	if (name) {
	    ibis::fileManager::instance().flushFile(name);
	}
	clear();
    }
    else {
	ibis::util::logMessage("Warning", "ibis::fileManager::roFile is busy "
			       "and cann't read new content");
	return;
    }
    Stat_T tmp;
    if (0 != UnixStat(file, &tmp)) { // get stat correctly
	ibis::util::logMessage("Warning", "roFile::mapFile is unable find "
			       "out the size of \"%s\"", file);
	return;
    }
    if (tmp.st_size > 0) {
	doMap(file, 0, tmp.st_size, 0);
    }
    else if (ibis::gVerbose > 3) {
	ibis::util::logMessage("ibis::fileManager::roFile::mapFile",
			       "file %s exists but is empty", file);
	return;
    }
    if (m_end >= m_begin + tmp.st_size) {
	// doMap function has finished correctly
	name = ibis::util::strnewdup(file);
    }
    else {
	LOGGER(ibis::gVerbose > 5)
	    << "roFile::mapFile(" << file << ") failed on the 1st try, "
	    "see if anything can be freed before try again";
	clear();
	ibis::fileManager::instance().unload(0); // free whatever can be freed
	doMap(file, 0, tmp.st_size, 0);

	if (m_end >= m_begin + tmp.st_size) {
	    // doMap function has finished correctly on the second try
	    name = ibis::util::strnewdup(file);
	}
	else {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- roFile::mapFile failed twice to map file \""
		<< file << "\", will the plain read fair better?";
	    clear();
	    doRead(file);
	    if (m_end >= m_begin + tmp.st_size) {
		name = ibis::util::strnewdup(file);
	    }
	    else {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- roFile::mapFile(" << file
		    << ") did NOT read anything from file either";
		clear();
	    }
	}
    }
} // mapFile

ibis::fileManager::rofSegment::rofSegment(const char *fn, off_t b, off_t e)
    : ibis::fileManager::roFile(), filename_(fn), begin_(b), end_(e) {
    if (fn == 0 || *fn == 0 || b >= e)
	return;

    doMap(fn, b, e, 0);
    if (m_begin == 0 || m_begin + (e-b) != m_end) {
	// clear the partially filled object and try again
	clear();
	doRead(fn, b, e);
    }
} // rofSegment constructor

size_t ibis::fileManager::rofSegment::printStatus(std::ostream& out) const {
    if (! filename_.empty()) 
	out << "file name: " << filename_ << "[" << begin_ << ", "
	    << end_ << ")\n";
    return printBody(out);
} // ibis::fileManager::roFile::printStatus
#endif

/// This function maps the specified portion of the file in either
/// read_only (@c opt = 0) mode or read_write (@c opt != 0) mode.
/// @note It assumes the current object contain no valid information.  The
/// caller is responsible to calling function @c clear if necessary.
#if defined(_WIN32) && defined(_MSC_VER)
void ibis::fileManager::roFile::doMap(const char *file, off_t b, off_t e,
				      int opt) {
    if (file == 0 || *file == 0 || b >= e)
	return; // nothing to do

    if (opt == 0) {
	fdescriptor = CreateFile
	    (file, GENERIC_READ, FILE_SHARE_READ,
	     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    }
    else {
	fdescriptor = CreateFile
	    (file, GENERIC_WRITE, FILE_SHARE_READ,
	     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }	
    if (fdescriptor != INVALID_HANDLE_VALUE) {
	DWORD dhi, dlo;
	if (sizeof(off_t) > sizeof(DWORD)) {
	    dlo = (e & 0xFFFFFFFF);
	    dhi = (e >> 32);
	}
	else {
	    dlo = e;
	    dhi = 0;
	}
	if (opt == 0) { // create an unnamed file mapping object
	    fmap = CreateFileMapping(fdescriptor, NULL, PAGE_READONLY,
				     dhi, dlo, NULL);
	}
	else {
	    fmap = CreateFileMapping(fdescriptor, NULL, PAGE_READWRITE,
				     dhi, dlo, NULL);
	}
	if (fmap != INVALID_HANDLE_VALUE) {
	    DWORD offset = 0;
	    if (sizeof(off_t) > sizeof(DWORD)) {
		dlo = (b & 0xFFFFFFFF);
		dhi = (b >> 32);
	    }
	    else {
		dlo = b;
		dhi = 0;
	    }
	    if (dlo != 0) { // make sure it matches allocation granularity
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		offset = dlo;
		dlo = sysinfo.dwAllocationGranularity *
		    (dlo / sysinfo.dwAllocationGranularity);
		offset -= dlo;
	    }
	    if (opt == 0) {
		map_begin = MapViewOfFile(fmap, FILE_MAP_READ, dhi, dlo,
					  e-b+offset);
	    }
	    else {
		map_begin = MapViewOfFile(fmap, FILE_MAP_WRITE, dhi, dlo,
					  e-b+offset);
	    }
	    if (map_begin) {
		mapped  = 1;
		opened  = time(0);
		m_begin = reinterpret_cast<char*>(map_begin) + offset;
		m_end   = m_begin + (e - b);
		if (ibis::gVerbose > 6)
		    ibis::util::logMessage("roFile::doMap",
					   "map %lu bytes of file "
					   "\"%s\" to 0x%lx",
					   static_cast<long unsigned>(e-b),
					   file, m_begin);
	    }
	    else {
		if (ibis::gVerbose >= 0) {
		    char *lpMsgBuf;
		    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				  FORMAT_MESSAGE_FROM_SYSTEM | 
				  FORMAT_MESSAGE_IGNORE_INSERTS,
				  NULL, GetLastError(),
				  MAKELANGID(LANG_NEUTRAL,
					     SUBLANG_DEFAULT),
				  (LPTSTR) &lpMsgBuf, 0, NULL);
		    ibis::util::logMessage("Warning", "%s %s", lpMsgBuf, file);
		    LocalFree(lpMsgBuf);	// Free the buffer.
		}
		m_begin = 0; m_end = 0; mapped = 0;
	    }
	}
	else {
	    if (ibis::gVerbose >= 0) {
		char *lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			      FORMAT_MESSAGE_FROM_SYSTEM | 
			      FORMAT_MESSAGE_IGNORE_INSERTS,
			      NULL, GetLastError(),
			      MAKELANGID(LANG_NEUTRAL,
					 SUBLANG_DEFAULT),
			      (LPTSTR) &lpMsgBuf, 0, NULL);
		ibis::util::logMessage("Warning", "%s %s", lpMsgBuf, file);
		LocalFree(lpMsgBuf);	// Free the buffer.
	    }
	    m_begin = 0; m_end = 0; mapped = 0;
	}
    }
    else {
	if (ibis::gVerbose >= 0) {
	    char *lpMsgBuf;
	    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			  FORMAT_MESSAGE_FROM_SYSTEM | 
			  FORMAT_MESSAGE_IGNORE_INSERTS,
			  NULL, GetLastError(),
			  MAKELANGID(LANG_NEUTRAL,
				     SUBLANG_DEFAULT),
			  (LPTSTR) &lpMsgBuf, 0, NULL);
	    ibis::util::logMessage("Warning", "%s %s", lpMsgBuf, file);
	    LocalFree(lpMsgBuf);	// Free the buffer.
	}
	m_begin = 0; m_end = 0; mapped = 0;
    }
} // doMap on WIN32 under MSVC
#elif (HAVE_MMAP+0 > 0)
void ibis::fileManager::roFile::doMap(const char* file, off_t b, off_t e,
				      int opt) {
    if (file == 0 || *file == 0 || b >= e)
	return;

    if (opt == 0) {
	fdescriptor = open(file, O_RDONLY);
    }
    else {
	fdescriptor = open(file, O_RDWR);
    }
    if (fdescriptor < 0) {
	if (ibis::gVerbose >= 0)
	    ibis::util::logMessage("Warning", "roFile::doMap is unable "
				   "open file \"%s\" ... %s", file,
				   (errno ? strerror(errno) :
				    "no free stdio stream"));
	m_begin = 0; m_end = 0; mapped = 0;
	return;
    }

    off_t offset = b;
    // the start of the memory map must be on page boundary
    b = ibis::fileManager::instance().pagesize *
	(b / ibis::fileManager::instance().pagesize);
    offset -= b;
    fsize = e - b;
    if (opt == 0) {
	map_begin = mmap(0, fsize, PROT_READ, MAP_PRIVATE, fdescriptor, b);
    }
    else {
	map_begin = mmap(0, fsize, PROT_READ|PROT_WRITE, MAP_SHARED,
			 fdescriptor, b);
    }
    if (map_begin != MAP_FAILED) {
	mapped  = 1;
	opened  = time(0);
	m_begin = reinterpret_cast<char*>(map_begin) + offset;
	m_end   = reinterpret_cast<char*>(map_begin) + fsize;
	if (ibis::gVerbose > 6)
	    ibis::util::logMessage("roFile::doMap", "map %lu bytes of "
				   "file \"%s\" to 0x%lx",
				   static_cast<long unsigned>(fsize), file,
				   map_begin);
    }
    else {
	close(fdescriptor);
	if (ibis::gVerbose >= 0)
	    ibis::util::logMessage("Warning", "roFile::doMap failed to "
				   "map file \"%s\" on file descriptor "
				   "%d ... %s",
				   file, fdescriptor, strerror(errno));
	m_begin = 0; m_end = 0; mapped = 0;
	fdescriptor = -1;
    }
}
#endif


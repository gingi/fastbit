//File: $Id$
// Author: K. John Wu <John.Wu at acm.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2009 University of California
#ifndef IBIS_ARRAY_T_H
#define IBIS_ARRAY_T_H
#include "fileManager.h"
#include "horometer.h"
#include <iomanip>	// std::setw

///@file
/// Definition of template array_t.
///@note array_t is not in the name space ibis because the compilers used
/// during the early development of this project did not accept templates
/// inside a name space.

/// Template array_t implements a replacement of std::vector.  The main
/// difference is that the underlying memory of this object is managed by
/// ibis::fileManager.  In addition, it also implements read and write
/// functions that are not present in std::vector.
#ifdef __GNUC__
#pragma interface
#endif
template<class T> class array_t {
public:
    typedef T* iterator; // can be used to assign values
    typedef const T* const_iterator; // used to read from array_t

    // constructor and destructor
    ~array_t<T>() {freeMemory();}
    array_t<T>();
    explicit array_t<T>(uint32_t n); // donot convert integer to array_t
    array_t<T>(uint32_t n, const T& val);
    array_t<T>(const array_t<T>& rhs);
    array_t<T>(const array_t<T>& rhs, const uint32_t offset,
	       const uint32_t nelm=0);
    array_t<T>(ibis::fileManager::storage& rhs);
    array_t<T>(ibis::fileManager::storage* rhs, const uint32_t start,
	       const uint32_t nelm);
    /// Read a portion of an open file into an array.
    array_t<T>(const int fdes, const off_t begin, const off_t end);
    /// Retrieve a portion of the named file to an array.  Prefer memory
    /// map if possible.
    array_t<T>(const char *fn, const off_t begin, const off_t end);

    const array_t<T>& operator=(const array_t<T>& rhs);
    void copy(const array_t<T>& rhs);
    void deepCopy(const array_t<T>& rhs);

    // functions for the iterators
    T* begin() {return m_begin;};
    T* end() {return m_end;};
    T& front() {return *m_begin;};
    T& back() {return m_end[-1];};
    const T* begin() const {return m_begin;};
    const T* end() const {return m_end;};
    const T& front() const {return *m_begin;};
    const T& back() const {return m_end[-1];};

    int empty() const {return (m_begin == 0 || m_begin >= m_end);};
    uint32_t size() const {	///< Return the number of elements.
	return (m_begin > 0 && m_end>=m_begin ? m_end - m_begin : 0);
    };
    void clear() {m_end = m_begin;};	///< Reset the size to zero.

    void pop_back() {--m_end;};		///< Remove the last element.
    void resize(uint32_t n);	///< Resize array.
    void reserve(uint32_t n);	///< Reserve space.
    inline void swap(array_t<T>& rhs);	///< Exchange the content.
    inline void push_back(const T& elm);///< Add one element.

    /// Produce index for ascending order.  Uses the quicksort algorithm
    /// with introspection.
    void sort(array_t<uint32_t> &ind) const;
    /// Return the positions of the @c k largest elements.
    void topk(uint32_t k, array_t<uint32_t> &ind) const;
    /// Return the positions of the @c k smallest elements.
    void bottomk(uint32_t k, array_t<uint32_t> &ind) const;
    /// Return the smallest i such that [ind[i]] >= val.
    uint32_t find(const array_t<uint32_t>& ind, const T& val) const;
    /// Return the smallest i such that [i] >= val.
    uint32_t find(const T& val) const;
    /// Return the smallest i such that [i] > val.
    uint32_t find_upper(const T& val) const;
    /// A stable sort using the provided workspace.  The current content is
    /// modified to be in ascending order.  The argument @c tmp is only
    /// used as temporary storage.
    void stableSort(array_t<T>& tmp);
    /// A stable sort that does not modify the current array.  It uses two
    /// additional arrays for temporary storage.
    void stableSort(array_t<uint32_t>& ind) const;
    /// A stable sort.  It does not change this array, but produces a
    /// sorted version in @c sorted.
    void stableSort(array_t<uint32_t>& ind, array_t<T>& sorted) const;
    static void stableSort(array_t<T>& val, array_t<uint32_t>& ind,
			   array_t<T>& tmp, array_t<uint32_t>& itmp);

    /// Non-modifiable reference to an element of the array.
    const T& operator[](uint32_t i) const {return m_begin[i];}
    /// Modifiable reference to an element of the array.
    /// @note For efficiency reasons, this is not a copy-on-write
    /// implementation!  The caller has to call the function @c nosharing
    /// to make sure the underlying data is not shared with others.
    T& operator[](uint32_t i) {return m_begin[i];};
    /// Make a not-shared copy of the array if it is actually a shared
    /// array.
    void nosharing();
    /// Is the content of the array solely in memory?
    bool incore() const {return(actual != 0 ? actual->unnamed() != 0 : false);}

    /// Insert one value or a list of values before p.  Return pointer to
    /// new elem.
    iterator insert(iterator pos, const T& val);
    void insert(iterator p, uint32_t n, const T& val);
    void insert(iterator p, const_iterator i, const_iterator j);

    // erase one value or a range of values
    iterator erase(iterator p);
    iterator erase(iterator i, iterator j);

    // the IO functions
    void read(const char*); ////< Read from the named file.
    off_t read(const int fdes, const off_t begin,
	       const off_t end); ///< Read a portion of an open file.
    void write(const char*) const; ///< write whole array to the named file
    void write(FILE* fptr) const;  ///< write whole array to an opened file

    // print internal pointer addresses
    void printStatus(std::ostream& out) const;

    /// Export the actual storage object.
    /// @note  <b>Very dangarous</b>.  <b>Likely to be removed soon</b>.
    /// Don't rely on this function!
    ibis::fileManager::storage* getStorage() {return actual;}

private:
    ibis::fileManager::storage *actual; ///< Pointer to the actual space.
    T* m_begin;	///< The nominal starting point.
    T* m_end;	///< The nominal ending point.
    // ibis::horometer timer; // a timer to track usage

    void freeMemory(); ///< Free the associated memory.

    /// Standard two-way partitioning function to quicksort function qsort.
    uint32_t partition(array_t<uint32_t>& ind, uint32_t front,
		       uint32_t back) const;
    /// Insertionsort.
    void isort(array_t<uint32_t>& ind, uint32_t front, uint32_t back) const;
    /// Heapsort.
    void hsort(array_t<uint32_t>& ind, uint32_t front, uint32_t back) const;
    /// Quicksort with introspection.
    void qsort(array_t<uint32_t>& ind, uint32_t front, uint32_t back,
	       uint32_t lvl=0) const;
};

/// Swap the content of two array_t objects.
template<class T>
inline void array_t<T>::swap(array_t<T>& rhs) {
    ibis::fileManager::storage *a = rhs.actual;
    rhs.actual = actual;
    actual = a;
    T* b = rhs.m_begin;
    rhs.m_begin = m_begin;
    m_begin = b;
    T* e = rhs.m_end;
    rhs.m_end = m_end;
    m_end = e;
} // array_t<T>::swap

/// Add one element from the back.
template<class T> 
inline void array_t<T>::push_back(const T& elm) {
    if (actual == 0) { // allocate storage
	actual = new ibis::fileManager::storage(3*sizeof(T));
	actual->beginUse();
	m_begin = (T*)(actual->begin());
	m_end = m_begin + 1;
	*m_begin = elm;
    }
    else if ((char*)(m_end+1) <= actual->end() &&
	     actual->end() > actual->begin() &&
	     actual->begin() > 0) { // simply add value
	*m_end = elm;
	++ m_end;
    }
    else { // need to enlarge space
	uint32_t offset = (char*)(m_begin) - actual->begin();
	uint32_t n = (actual->empty() || m_end<=m_begin ? 0 : m_end - m_begin);
#if defined(DEBUG) && DEBUG + 0 > 1
	LOGGER(ibis::gVerbose >= 0)
	    << "DEBUG: ibis::array_t<" << typeid(T).name()
	    << ">::push_back(0x" << std::hex << elm
	    << ") actual address before calling enlarge 0x"
	    << std::setw(8) << std::setfill('0')
	    << (uint32_t)actual->begin() << std::dec
	    << " (offset=" << offset
	    << ", n=" << n << ")";
#endif
	actual->enlarge(); // increase the storage
	m_begin = (T*)(actual->begin() + offset);
#if defined(DEBUG) && DEBUG + 0 > 1
	LOGGER(ibis::gVerbose >= 0)
	    << "DEBUG: ibis::array_t<" << typeid(T).name()
	    << ">::push_back(0x" << std::hex << elm
	    << ") actual address after calling enlarge 0x"
	    << std::setw(8) << std::setfill('0')
	    << (uint32_t)actual->begin() << " (m_begin=0x"
	    << std::setw(8) << std::setfill('0')
	    << (uint32_t)m_begin << ")" << std::dec;
#endif
	m_end = m_begin + n;
	*m_end = elm;
	++ m_end;
    }
} // array_t<T>::push_back

/// Free the memory associated with the fileManager::storage.
template<class T>
inline void array_t<T>::freeMemory() {
    m_end = 0;
    m_begin = 0;
    if (actual) {
	const bool doDelete = (actual->unnamed() && 1 >= actual->inUse());
	actual->endUse();//timer.CPUTime()
	if (doDelete)
	    delete actual;
	actual = 0;
    }
} // array_t<T>::freeMemory
#endif // IBIS_ARRAY_T_H

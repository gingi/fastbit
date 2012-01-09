//File: $Id$
// Author: K. John Wu <John.Wu at acm.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2012 University of California
#ifndef IBIS_ARRAY_T_H
#define IBIS_ARRAY_T_H
#include "fileManager.h"
#include "horometer.h"
#include <cstddef>	// ptrdiff_t

/// Template array_t is a replacement of std::vector.  The main difference
/// is that the underlying memory of this object is reference-counted and
/// managed by ibis::fileManager.  It is intended to store arrays in
/// memory, and it's possible to have shallow copies of read-only arrays.
/// The memory is guaranteed to be contiguous.  It also implements read and
/// write functions that are not present in std::vector.
///
/// @note This implementation uses size_t integers for measuring the number
/// of elements, therefore, the maximum size it can handle is machine and
/// compiler dependent.
#ifdef __GNUC__
#pragma interface
#endif
template<class T> class ibis::array_t {
public:
    typedef       T* iterator; ///< Iterator type.
    typedef const T* const_iterator; ///< Const iterator type.
    typedef       T* pointer; ///< Pointer to a value.
    typedef const T* const_pointer; ///< Pointer to a constant value.
    typedef       T& reference; ///< Reference to a value.
    typedef const T& const_reference; ///< Reference to a constant value.
    typedef       T  value_type; ///< Type of values.
    typedef  size_t  size_type; ///< For array size.
    typedef std::ptrdiff_t difference_type;///< For difference between pointers.

    // constructor and destructor
    ~array_t<T>() {freeMemory();}
    array_t<T>();
    explicit array_t<T>(size_t n); // donot convert integer to array_t
    array_t<T>(size_t n, const T& val);
    array_t<T>(const array_t<T>& rhs);
    array_t<T>(const std::vector<T>& rhs);
    array_t<T>(const array_t<T>& rhs, const size_t begin,
	       const size_t end=0);
    explicit array_t<T>(ibis::fileManager::storage* rhs);
    array_t<T>(ibis::fileManager::storage* rhs,
	       const size_t start, const size_t end);
    array_t<T>(const int fdes, const off_t begin, const off_t end);
    array_t<T>(const char *fn, const off_t begin, const off_t end);
    array_t<T>(const char *fn, const int fdes,
	       const off_t begin, const off_t end);
    array_t<T>(T* addr, size_t nelm);

    array_t<T>& operator=(const array_t<T>& rhs);
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

    bool empty() const {return (m_begin == 0 || m_begin >= m_end);};
    size_t size() const {	///< Return the number of elements.
	return (m_begin > 0 && m_end > m_begin ? m_end - m_begin : 0);
    };
    inline void clear();

    void pop_back() {--m_end;};		///< Remove the last element.
    void resize(size_t n);	///< Resize array.
    void reserve(size_t n);	///< Reserve space.
    void truncate(size_t keep, size_t start);
    size_t capacity() const;
    inline void swap(array_t<T>& rhs);	///< Exchange the content.
    inline void push_back(const T& elm);///< Add one element.

    void deduplicate();
    void sort(array_t<uint32_t> &ind) const;
    void topk(uint32_t k, array_t<uint32_t> &ind) const;
    void bottomk(uint32_t k, array_t<uint32_t> &ind) const;
    uint32_t find(const array_t<uint32_t>& ind, const T& val) const;
    size_t find(const T& val) const;
    size_t find_upper(const T& val) const;
    void stableSort(array_t<T>& tmp);
    void stableSort(array_t<uint32_t>& ind) const;
    void stableSort(array_t<uint32_t>& ind, array_t<T>& sorted) const;
    static void stableSort(array_t<T>& val, array_t<uint32_t>& ind,
			   array_t<T>& tmp, array_t<uint32_t>& itmp);

    bool equal_to(const array_t<T>&) const;

    /// Non-modifiable reference to an element of the array.
    const T& operator[](size_t i) const {return m_begin[i];}
    /// Modifiable reference to an element of the array.
    /// @note For efficiency reasons, this is not a copy-on-write
    /// implementation!  The caller has to call the function @c nosharing
    /// to make sure the underlying data is not shared with others.
    T& operator[](size_t i) {return m_begin[i];};
    void nosharing();
    /// Is the content of the array solely in memory?
    bool incore() const {return(actual != 0 ? actual->filename() == 0 : false);}

    iterator insert(iterator pos, const T& val);
    void insert(iterator p, size_t n, const T& val);
    void insert(iterator p, const_iterator i, const_iterator j);

    // erase one value or a range of values
    iterator erase(iterator p);
    iterator erase(iterator i, iterator j);

    // the IO functions
    void  read(const char*);
    off_t read(const char*, const off_t, const off_t);
    off_t read(const int, const off_t, const off_t);
    void write(const char*) const;
    void write(FILE* fptr) const;

    // print internal pointer addresses
    void printStatus(std::ostream& out) const;

    /// Export the actual storage object.
    ///
    /// @note This function returns the internal storage representation of
    /// the array_t object.  <b>In general, it is very dangarous to expose
    /// internal implementation details to clients</b>.  <b>Likely to be
    /// removed soon</b>.  Don't rely on this function!
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
}; // ibis::array_t

/// Reset the size to zero.
template<class T>
inline void ibis::array_t<T>::clear() {
#if defined(DEBUG) || defined(_DEBUG)
    LOGGER(ibis::gVerbose > 10)
	<< "array_t::clear -- (" << static_cast<const void*>(this) << ", "
	<< static_cast<const void*>(m_begin) << ") resets m_end from "
	<< static_cast<const void*>(m_end) << " to "
	<< static_cast<const void*>(m_begin);
#endif
    m_end = m_begin;
} // ibis::array_t<T>::clear

/// Swap the content of two array_t objects.
template<class T>
inline void ibis::array_t<T>::swap(array_t<T>& rhs) {
    ibis::fileManager::storage *a = rhs.actual;
    rhs.actual = actual;
    actual = a;
    T* b = rhs.m_begin;
    rhs.m_begin = m_begin;
    m_begin = b;
    T* e = rhs.m_end;
    rhs.m_end = m_end;
    m_end = e;
} // ibis::array_t<T>::swap

/// Add one element from the back.
template<class T> 
inline void ibis::array_t<T>::push_back(const T& elm) {
    if (actual == 0) { // allocate storage
	actual = new ibis::fileManager::storage(3*sizeof(T));
	actual->beginUse();
	m_begin = (T*)(actual->begin());
	m_end = m_begin + 1;
	*m_begin = elm;
    }
    else if (m_begin != 0 && m_end != 0 && actual->begin() > 0 &&
	     actual->end() > actual->begin() && actual->inUse() <= 1 &&
	     (char*)(m_end+1) <= actual->end()) { // simply add value
	*m_end = elm;
	++ m_end;
    }
    else { // copy-and-swap
	const difference_type nexist = (m_end - m_begin);
	const size_t newsize = (nexist >= 7 ? nexist : 7) + nexist;
	if ((long long) newsize < nexist) {
	    throw "array_t must have less than 2^31 elements";
	}

	array_t<T> tmp(newsize); // allocate new array
	tmp.resize(static_cast<size_t>(nexist+1));
	for (difference_type j = 0; j < nexist; ++ j) // copy
	    tmp.m_begin[j] = m_begin[j];
	tmp.m_begin[nexist] = elm;
	swap(tmp); // swap
    }
} // ibis::array_t<T>::push_back

/// The maximum number of elements can be stored with the current memory.
template <class T>
inline size_t ibis::array_t<T>::capacity() const {
    return (actual != 0 ? (const T*)actual->end() - m_begin : 0);
} // ibis::array_t<T>::capacity
#endif // IBIS_ARRAY_T_H

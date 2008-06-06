// File: $Id$
// Author: John Wu <John.Wu at nersc.gov> Lawrence Berkeley National Laboratory
// Copyright 2000-2008 the Regents of the University of California
#ifndef IBIS_COLVALUES_H
#define IBIS_COLVALUES_H
#include "column.h"
#include "utilidor.h"	// ibis::util::reorder

///@file
/// A set of utility classes for storing the selected values.

/// A pure virtual base class.
class FASTBIT_CXX_DLLSPEC ibis::colValues {
public:
    virtual ~colValues() {}

    /// Construct from a hit vector.
    static colValues* create(const ibis::column* c,
			     const ibis::bitvector& hits);
    /// Construct from content of the file (pointed by @c store).
    static colValues* create(const ibis::column* c,
			     ibis::fileManager::storage* store,
			     const uint32_t start, const uint32_t nelm);
    /// Construct from content of an array_t.
    static colValues* create(const ibis::column* c, void* vals);

    /// Provide a pointer to the column containing the selected values.
    virtual const ibis::column* operator->() const = 0;
    virtual bool empty() const = 0;
    virtual void reduce(const array_t<uint32_t>& starts) = 0;
    virtual void reduce(const array_t<uint32_t>& starts,
			ibis::selected::FUNCTION func) = 0;
    virtual void erase(uint32_t i, uint32_t j) = 0;
    virtual void swap(uint32_t i, uint32_t j) = 0;
    virtual uint32_t size() const = 0;
    virtual uint32_t elementSize() const = 0;
    /// Return the type of the data stored.
    virtual ibis::TYPE_T getType() const =0;
    /// Return the pointer to the pointer to underlying array_t<T> object.
    virtual void* getArray() const =0;
    /// Make sure the content of the underlying storage is not shared.
    virtual void nosharing() =0;

    bool canSort() const
    {return (col ? col->type() != ibis::TEXT : false);}

    void swap(colValues& rhs) { // swap two colValues
	const ibis::column* c = rhs.col;
	rhs.col = col;
	col = c;
    }

    /// Write out whole array as binary.
    virtual uint32_t write(FILE* fptr) const  = 0;
    /// Write ith element as text.
    virtual void write(std::ostream& out, uint32_t i) const = 0;

    /// Sort rows in the range <code>[i, j)</code>.
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl) = 0;
    /// Sort rows in the range <code>[i, j)</code>.  Also sort the columns
    /// between <code>[head, tail)</code>.
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail) = 0;
    /// Sort rows in the range <code>[i, j)</code>.  Output the new order
    /// in array @c neworder.
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const = 0;
    /// Reorder the values according to the specified indices.  <code>
    /// New[i] = Old[ind[i]]</code>.
    virtual void reorder(const array_t<uint32_t>& ind) = 0;
    /// Produce an array of the starting positions of values that are the
    /// same.
    virtual array_t<uint32_t>*
    segment(const array_t<uint32_t>* old=0) const = 0;
    /// Truncate the number element to no more than @c keep.
    virtual long truncate(uint32_t keep) = 0;
    /// Return the positions of the @c k largest elements.
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const = 0;
    /// Return the positions of the @c k smallest elements.
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const = 0;

    virtual double getMin() const = 0;
    virtual double getMax() const = 0;
    virtual double getSum() const = 0;
    virtual int32_t getInt(uint32_t) const = 0;
    virtual uint32_t getUInt(uint32_t) const = 0;
    virtual int64_t getLong(uint32_t) const = 0;
    virtual uint64_t getULong(uint32_t) const = 0;
    virtual float getFloat(uint32_t) const = 0;
    virtual double getDouble(uint32_t) const = 0;

protected:
    const ibis::column* col; ///< The column where the value is from.

    colValues() : col(0) {}
    colValues(const ibis::column* c) : col(c) {};

private:
    const colValues& operator=(const colValues&);
}; // ibis::colValues

/// A class to store integer values.
class FASTBIT_CXX_DLLSPEC ibis::colInts : public ibis::colValues {
public:
    colInts() : colValues(), array(0) {};
    colInts(const ibis::column* c, const ibis::bitvector& hits)
	: colValues(c), array(c->selectInts(hits)) {}
    colInts(const ibis::column* c, ibis::fileManager::storage* store,
	    const uint32_t start, const uint32_t nelm)
	: colValues(c), array(new array_t<int32_t>(store, start, nelm)) {}
    colInts(const ibis::column* c, void* vals);
    virtual ~colInts() {delete array;}

    virtual const  ibis::column* operator->() const {return col;}
    virtual bool   empty() const {return (col==0 || array==0);}
    virtual uint32_t size() const {return (array ? array->size() : 0);}
    virtual uint32_t elementSize() const {return sizeof(int);}
    virtual ibis::TYPE_T getType() const {return ibis::INT;}
    virtual void* getArray() const {return array;}
    virtual void nosharing() {array->nosharing();}

    virtual void   reduce(const array_t<uint32_t>& starts);
    virtual void   reduce(const array_t<uint32_t>& starts,
			  ibis::selected::FUNCTION func);
    virtual void   erase(uint32_t i, uint32_t j) {
	array->erase(array->begin()+i, array->begin()+j);}
    virtual void   swap(uint32_t i, uint32_t j) {
	int tmp = (*array)[i]; (*array)[i] = (*array)[j]; (*array)[j] = tmp;}

    void swap(colInts& rhs) { // swap two colInts
	const ibis::column* c = rhs.col; rhs.col = col; col = c;
	array_t<int32_t>* a = rhs.array; rhs.array = array; array = a;}

    // write out whole array as binary
    virtual uint32_t write(FILE* fptr) const {
	if (array) {
	    uint32_t nelm = array->size();
	    return nelm - fwrite(array->begin(), sizeof(int), nelm, fptr);
	}
	else {
	    return 0;
	}
    }
    // write ith element as text
    virtual void write(std::ostream& out, uint32_t i) const
    {out << (*array)[i];}

    virtual void sort(uint32_t i, uint32_t j, bundle* bdl);
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail);
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const;
    virtual array_t<uint32_t>* segment(const array_t<uint32_t>* old=0) const;
    virtual void reorder(const array_t<uint32_t>& ind)
    {ibis::util::reorder(*array, ind);}
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const
    {array->topk(k, ind);}
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const
    {array->bottomk(k, ind);}
    virtual long truncate(uint32_t keep) {
	if (array == 0) return 0;
	if (array->size() > keep) {
	    array->resize(keep);
	    return keep;
	}
	else {
	    return array->size();
	}
    }

    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;
    virtual int32_t getInt(uint32_t i) const {return (*array)[i];}
    virtual uint32_t getUInt(uint32_t i) const {return (uint32_t)(*array)[i];}
    virtual int64_t getLong(uint32_t i) const {return (int64_t)(*array)[i];}
    virtual uint64_t getULong(uint32_t i) const {return (uint64_t)(*array)[i];}
    virtual float getFloat(uint32_t i) const {return (float)(*array)[i];};
    virtual double getDouble(uint32_t i) const {return (double)(*array)[i];};

private:
    array_t<int32_t>* array;

    colInts(const colInts&);
    const colInts& operator=(const colInts&);
}; // ibis::colInts

/// A class to store unsigned integer values.
class FASTBIT_CXX_DLLSPEC ibis::colUInts : public ibis::colValues {
public:
    colUInts() : colValues(), array(0) {};
    colUInts(const ibis::column* c, const ibis::bitvector& hits)
	: colValues(c), array(c->selectUInts(hits)) {}
    colUInts(const ibis::column* c, ibis::fileManager::storage* store,
	    const uint32_t start, const uint32_t nelm)
	: colValues(c), array(new array_t<uint32_t>(store, start, nelm)) {}
    colUInts(const ibis::column* c, void* vals);
    virtual ~colUInts() {delete array;}

    virtual const  ibis::column* operator->() const {return col;}
    virtual bool   empty() const {return (col==0 || array==0);}
    virtual uint32_t size() const {return (array ? array->size() : 0);}
    virtual uint32_t elementSize() const {return sizeof(unsigned);}
    virtual ibis::TYPE_T getType() const {return ibis::UINT;}
    virtual void* getArray() const {return array;}
    virtual void nosharing() {array->nosharing();}

    virtual void   erase(uint32_t i, uint32_t j) {
	array->erase(array->begin()+i, array->begin()+j);}
    virtual void   swap(uint32_t i, uint32_t j) {
	unsigned tmp = (*array)[i]; (*array)[i] = (*array)[j];
	(*array)[j] = tmp;}

    virtual void   reduce(const array_t<uint32_t>& starts);
    virtual void   reduce(const array_t<uint32_t>& starts,
			  ibis::selected::FUNCTION func);
    void swap(colUInts& rhs) { // swap two colUInts
	const ibis::column* c = rhs.col; rhs.col = col; col = c;
	array_t<uint32_t>* a = rhs.array; rhs.array = array; array = a;}

    /// Write out the whole array as binary.
    virtual uint32_t write(FILE* fptr) const {
	if (array) {
	    uint32_t nelm = array->size();
	    return nelm - fwrite(array->begin(), sizeof(unsigned), nelm, fptr);
	}
	else {
	    return 0;
	}
    }
    /// Write the ith element as text.
    virtual void write(std::ostream& out, uint32_t i) const {
	if (col->type() == ibis::CATEGORY || col->type() == ibis::TEXT) {
	    std::string str;
	    col->getString((*array)[i], str);
	    if (str.empty()) {
		out << "<NULL>";
	    }
	    else {
		out << str;
	    }
	}
	else {
	    out << (*array)[i];
	}
    }

    virtual void sort(uint32_t i, uint32_t j, bundle* bdl);
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail);
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const;
    virtual array_t<uint32_t>* segment(const array_t<uint32_t>* old=0) const;
    virtual void reorder(const array_t<uint32_t>& ind)
    {ibis::util::reorder(*array, ind);}
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const
    {array->topk(k, ind);}
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const
    {array->bottomk(k, ind);}
    virtual long truncate(uint32_t keep) {
	if (array == 0) return 0;
	if (array->size() > keep) {
	    array->resize(keep);
	    return keep;
	}
	else {
	    return array->size();
	}
    }

    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;
    virtual int32_t getInt(uint32_t i) const {return (int32_t)(*array)[i];}
    virtual uint32_t getUInt(uint32_t i) const {return (*array)[i];}
    virtual int64_t getLong(uint32_t i) const {return (int64_t)(*array)[i];}
    virtual uint64_t getULong(uint32_t i) const {return (uint64_t)(*array)[i];}
    virtual float getFloat(uint32_t i) const {return (float)(*array)[i];};
    virtual double getDouble(uint32_t i) const {return (double)(*array)[i];};

private:
    array_t<uint32_t>* array;

    colUInts(const colUInts&);
    const colUInts& operator=(const colUInts&);
}; // ibis::colUInts

/// A class to store integer values.
class FASTBIT_CXX_DLLSPEC ibis::colLongs : public ibis::colValues {
public:
    colLongs() : colValues(), array(0) {};
    colLongs(const ibis::column* c, const ibis::bitvector& hits)
	: colValues(c), array(c->selectLongs(hits)) {}
    colLongs(const ibis::column* c, ibis::fileManager::storage* store,
	    const uint32_t start, const uint32_t nelm)
	: colValues(c), array(new array_t<int64_t>(store, start, nelm)) {}
    colLongs(const ibis::column* c, void* vals);
    virtual ~colLongs() {delete array;}

    virtual const  ibis::column* operator->() const {return col;}
    virtual bool   empty() const {return (col==0 || array==0);}
    virtual uint32_t size() const {return (array ? array->size() : 0);}
    virtual uint32_t elementSize() const {return sizeof(int);}
    virtual ibis::TYPE_T getType() const {return ibis::LONG;}
    virtual void* getArray() const {return array;}
    virtual void nosharing() {array->nosharing();}

    virtual void   reduce(const array_t<uint32_t>& starts);
    virtual void   reduce(const array_t<uint32_t>& starts,
			  ibis::selected::FUNCTION func);
    virtual void   erase(uint32_t i, uint32_t j) {
	array->erase(array->begin()+i, array->begin()+j);}
    virtual void   swap(uint32_t i, uint32_t j) {
	int64_t tmp = (*array)[i];
	(*array)[i] = (*array)[j];
	(*array)[j] = tmp;}

    void swap(colLongs& rhs) { // swap two colLongs
	const ibis::column* c = rhs.col; rhs.col = col; col = c;
	array_t<int64_t>* a = rhs.array; rhs.array = array; array = a;}

    // write out whole array as binary
    virtual uint32_t write(FILE* fptr) const {
	if (array) {
	    uint32_t nelm = array->size();
	    return nelm - fwrite(array->begin(), sizeof(int), nelm, fptr);
	}
	else {
	    return 0;
	}
    }
    // write ith element as text
    virtual void write(std::ostream& out, uint32_t i) const
    {out << (*array)[i];}

    virtual void sort(uint32_t i, uint32_t j, bundle* bdl);
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail);
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const;
    virtual array_t<uint32_t>* segment(const array_t<uint32_t>* old=0) const;
    virtual void reorder(const array_t<uint32_t>& ind)
    {ibis::util::reorder(*array, ind);}
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const
    {array->topk(k, ind);}
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const
    {array->bottomk(k, ind);}
    virtual long truncate(uint32_t keep) {
	if (array == 0) return 0;
	if (array->size() > keep) {
	    array->resize(keep);
	    return keep;
	}
	else {
	    return array->size();
	}
    }

    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;
    virtual int32_t getInt(uint32_t i) const {return (int32_t)(*array)[i];}
    virtual uint32_t getUInt(uint32_t i) const {return (uint32_t)(*array)[i];}
    virtual int64_t getLong(uint32_t i) const {return (*array)[i];}
    virtual uint64_t getULong(uint32_t i) const {return (uint64_t)(*array)[i];}
    virtual float getFloat(uint32_t i) const {return (float)(*array)[i];};
    virtual double getDouble(uint32_t i) const {return (double)(*array)[i];};

private:
    array_t<int64_t>* array;

    colLongs(const colLongs&);
    const colLongs& operator=(const colLongs&);
}; // ibis::colLongs

/// A class to store unsigned integer values.
class FASTBIT_CXX_DLLSPEC ibis::colULongs : public ibis::colValues {
public:
    colULongs() : colValues(), array(0) {};
    colULongs(const ibis::column* c, const ibis::bitvector& hits)
	: colValues(c), array(c->selectULongs(hits)) {}
    colULongs(const ibis::column* c, ibis::fileManager::storage* store,
	    const uint32_t start, const uint32_t nelm)
	: colValues(c), array(new array_t<uint64_t>(store, start, nelm)) {}
    colULongs(const ibis::column* c, void* vals);
    virtual ~colULongs() {delete array;}

    virtual const  ibis::column* operator->() const {return col;}
    virtual bool   empty() const {return (col==0 || array==0);}
    virtual uint32_t size() const {return (array ? array->size() : 0);}
    virtual uint32_t elementSize() const {return sizeof(unsigned);}
    virtual ibis::TYPE_T getType() const {return ibis::ULONG;}
    virtual void* getArray() const {return array;}
    virtual void nosharing() {array->nosharing();}

    virtual void   erase(uint32_t i, uint32_t j) {
	array->erase(array->begin()+i, array->begin()+j);}
    virtual void   swap(uint32_t i, uint32_t j) {
	uint64_t tmp = (*array)[i]; (*array)[i] = (*array)[j];
	(*array)[j] = tmp;}

    virtual void   reduce(const array_t<uint32_t>& starts);
    virtual void   reduce(const array_t<uint32_t>& starts,
			  ibis::selected::FUNCTION func);
    void swap(colULongs& rhs) { // swap two colULongs
	const ibis::column* c = rhs.col; rhs.col = col; col = c;
	array_t<uint64_t>* a = rhs.array; rhs.array = array; array = a;}

    /// Write out the whole array as binary.
    virtual uint32_t write(FILE* fptr) const {
	if (array) {
	    uint32_t nelm = array->size();
	    return nelm - fwrite(array->begin(), sizeof(unsigned), nelm, fptr);
	}
	else {
	    return 0;
	}
    }
    /// Write the ith element as text.
    virtual void write(std::ostream& out, uint32_t i) const {
	if (col->type() == ibis::CATEGORY || col->type() == ibis::TEXT) {
	    std::string str;
	    col->getString((*array)[i], str);
	    if (str.empty()) {
		out << "<NULL>";
	    }
	    else {
		out << str;
	    }
	}
	else {
	    out << (*array)[i];
	}
    }

    virtual void sort(uint32_t i, uint32_t j, bundle* bdl);
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail);
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const;
    virtual array_t<uint32_t>* segment(const array_t<uint32_t>* old=0) const;
    virtual void reorder(const array_t<uint32_t>& ind)
    {ibis::util::reorder(*array, ind);}
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const
    {array->topk(k, ind);}
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const
    {array->bottomk(k, ind);}
    virtual long truncate(uint32_t keep) {
	if (array == 0) return 0;
	if (array->size() > keep) {
	    array->resize(keep);
	    return keep;
	}
	else {
	    return array->size();
	}
    }

    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;
    virtual int32_t getInt(uint32_t i) const {return (int32_t)(*array)[i];}
    virtual uint32_t getUInt(uint32_t i) const {return (uint32_t)(*array)[i];}
    virtual int64_t getLong(uint32_t i) const {return (int64_t)(*array)[i];}
    virtual uint64_t getULong(uint32_t i) const {return (*array)[i];}
    virtual float getFloat(uint32_t i) const {return (float)(*array)[i];};
    virtual double getDouble(uint32_t i) const {return (double)(*array)[i];};

private:
    array_t<uint64_t>* array;

    colULongs(const colULongs&);
    const colULongs& operator=(const colULongs&);
}; // ibis::colULongs

/// A class to store single precision float-point values.
class FASTBIT_CXX_DLLSPEC ibis::colFloats : public ibis::colValues {
public:
    colFloats() : colValues(), array(0) {};
    colFloats(const ibis::column* c, const ibis::bitvector& hits)
	: colValues(c), array(c->selectFloats(hits)) {}
    colFloats(const ibis::column* c, ibis::fileManager::storage* store,
	      const uint32_t start, const uint32_t nelm)
	: colValues(c), array(new array_t<float>(store, start, nelm)) {}
    colFloats(const ibis::column* c, void* vals);
    virtual ~colFloats() {delete array;}

    virtual const  ibis::column* operator->() const {return col;}
    virtual bool   empty() const {return (col==0 || array==0);}
    virtual uint32_t size() const {return (array ? array->size() : 0);}
    virtual uint32_t elementSize() const {return sizeof(float);}
    virtual ibis::TYPE_T getType() const {return ibis::FLOAT;}
    virtual void* getArray() const {return array;}
    virtual void nosharing() {array->nosharing();}

    virtual void   erase(uint32_t i, uint32_t j) {
	array->erase(array->begin()+i, array->begin()+j);}
    virtual void   swap(uint32_t i, uint32_t j) {
	float tmp = (*array)[i];
	(*array)[i] = (*array)[j];
	(*array)[j] = tmp;}

    void swap(colFloats& rhs) { // swap two colFloats
	const ibis::column* c = rhs.col; rhs.col = col; col = c;
	array_t<float>* a = rhs.array; rhs.array = array; array = a;}
    virtual void   reduce(const array_t<uint32_t>& starts);
    virtual void   reduce(const array_t<uint32_t>& starts,
			  ibis::selected::FUNCTION func);

    // write out whole array as binary
    virtual uint32_t write(FILE* fptr) const {
	if (array) {
	    uint32_t nelm = array->size();
	    return nelm - fwrite(array->begin(), sizeof(float), nelm, fptr);
	}
	else {
	    return 0;
	}
    }
    // write ith element as text
    virtual void write(std::ostream& out, uint32_t i) const {
	out << (*array)[i];}

    virtual void sort(uint32_t i, uint32_t j, bundle* bdl);
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail);
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const;
    virtual array_t<uint32_t>* segment(const array_t<uint32_t>* old=0) const;
    virtual void reorder(const array_t<uint32_t>& ind)
    {ibis::util::reorder(*array, ind);}
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const
    {array->topk(k, ind);}
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const
    {array->bottomk(k, ind);}
    virtual long truncate(uint32_t keep) {
	if (array == 0) return 0;
	if (array->size() > keep) {
	    array->resize(keep);
	    return keep;
	}
	else {
	    return array->size();
	}
    }

    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;
    virtual int32_t getInt(uint32_t i) const {return (int32_t)(*array)[i];}
    virtual uint32_t getUInt(uint32_t i) const {return (uint32_t)(*array)[i];}
    virtual int64_t getLong(uint32_t i) const {return (int64_t)(*array)[i];}
    virtual uint64_t getULong(uint32_t i) const {return (uint64_t)(*array)[i];}
    virtual float getFloat(uint32_t i) const {return (float)(*array)[i];};
    virtual double getDouble(uint32_t i) const {return (double)(*array)[i];};

private:
    array_t<float>* array;

    colFloats(const colFloats&);
    const colFloats& operator=(const colFloats&);
}; // ibis::colFloats

/// A class to store double precision floating-point values.
class FASTBIT_CXX_DLLSPEC ibis::colDoubles : public ibis::colValues {
public:
    colDoubles() : colValues(), array(0) {};
    colDoubles(const ibis::column* c, const ibis::bitvector& hits)
	: colValues(c), array(c->selectDoubles(hits)) {}
    colDoubles(const ibis::column* c, ibis::fileManager::storage* store,
	       const uint32_t start, const uint32_t nelm)
	: colValues(c), array(new array_t<double>(store, start, nelm)) {}
    colDoubles(const ibis::column* c, void* vals);
    virtual ~colDoubles() {delete array;}

    virtual const  ibis::column* operator->() const {return col;}
    virtual bool   empty() const {return (col==0 || array==0);}
    virtual uint32_t size() const {return (array ? array->size() : 0);}
    virtual uint32_t elementSize() const {return sizeof(double);}
    virtual ibis::TYPE_T getType() const {return ibis::DOUBLE;}
    virtual void* getArray() const {return array;}
    virtual void nosharing() {array->nosharing();}

    virtual void   erase(uint32_t i, uint32_t j) {
	array->erase(array->begin()+i, array->begin()+j);}
    virtual void   swap(uint32_t i, uint32_t j) {
	double tmp = (*array)[i]; (*array)[i] = (*array)[j];
	(*array)[j] = tmp;}

    void swap(colDoubles& rhs) { // swap two colDoubles
	const ibis::column* c = rhs.col; rhs.col = col; col = c;
	array_t<double>* a = rhs.array; rhs.array = array; array = a;}
    virtual void   reduce(const array_t<uint32_t>& starts);
    virtual void   reduce(const array_t<uint32_t>& starts,
			  ibis::selected::FUNCTION func);

    // write out whole array as binary
    virtual uint32_t write(FILE* fptr) const {
	if (array) {
	    uint32_t nelm = array->size();
	    return nelm - fwrite(array->begin(), sizeof(double), nelm, fptr);
	}
	else {
	    return 0;
	}
    }
    // write ith element as text
    virtual void write(std::ostream& out, uint32_t i) const {
	out.precision(12); out << (*array)[i];}

    virtual void sort(uint32_t i, uint32_t j, bundle* bdl);
    virtual void sort(uint32_t i, uint32_t j, bundle* bdl,
		      colList::iterator head, colList::iterator tail);
    virtual void sort(uint32_t i, uint32_t j,
		      array_t<uint32_t>& neworder) const;
    virtual array_t<uint32_t>* segment(const array_t<uint32_t>* old=0) const;
    virtual void reorder(const array_t<uint32_t>& ind)
    {ibis::util::reorder(*array, ind);}
    virtual void topk(uint32_t k, array_t<uint32_t> &ind) const
    {array->topk(k, ind);}
    virtual void bottomk(uint32_t k, array_t<uint32_t> &ind) const
    {array->bottomk(k, ind);}
    virtual long truncate(uint32_t keep) {
	if (array == 0) return 0;
	if (array->size() > keep) {
	    array->resize(keep);
	    return keep;
	}
	else {
	    return array->size();
	}
    }

    virtual double getMin() const;
    virtual double getMax() const;
    virtual double getSum() const;
    virtual int32_t getInt(uint32_t i) const {return (int32_t)(*array)[i];}
    virtual uint32_t getUInt(uint32_t i) const {return (uint32_t)(*array)[i];}
    virtual int64_t getLong(uint32_t i) const {return (int64_t)(*array)[i];}
    virtual uint64_t getULong(uint32_t i) const {return (uint64_t)(*array)[i];}
    virtual float getFloat(uint32_t i) const {return (float)(*array)[i];};
    virtual double getDouble(uint32_t i) const {return (double)(*array)[i];};

private:
    array_t<double>* array;

    colDoubles(const colDoubles&);
    const colDoubles& operator=(const colDoubles&);
}; // ibis::colDoubles
#endif

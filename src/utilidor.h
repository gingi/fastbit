// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008 the Regents of the University of California
#ifndef IBIS_UTILIDOR_H
#define IBIS_UTILIDOR_H
/**@file
   @brief FastBit sorting functions and other utilities.

   This is a collection of sorting function in the name space of
   ibis::util.

   @note About the name: I was going to name this utilsort.h, but what is
   the fun in that.  According to answers.com, utilsort might be a
   misspelling of utilidor, which is an civil engineering term describing
   an insulated, heated conduit built below the ground surface or supported
   above the ground surface to protect the contained water, steam, sewage,
   and fire lines from freezing.
 */
#include "array_t.h"	// array_t

namespace ibis {
    namespace util {
	/// Reorder the array arr according to the indices given in ind.
	template <typename T>
	void reorder(array_t<T> &arr, const array_t<uint32_t> &ind);
	template <typename T>
	void reorder(array_t<T*> &arr, const array_t<uint32_t> &ind);
	/// Sort two arrays together.  Order arr1 in ascending order first,
	/// then when arr1 has the same value, order arr2 in ascending
	/// order as well.
	template <typename T1, typename T2>
	void sortAll(array_t<T1>& arr1, array_t<T2>& arr2);
	/// Shell sort.  Sort both arrays arr1 and arr2.
	template <typename T1, typename T2>
	void sortAll_shell(array_t<T1>& arr1, array_t<T2>& arr2);
	/// The parititioning function for ibis::util::sortAll.  Uses
	/// the standard two-way partitioning.
	template <typename T1, typename T2>
	uint32_t sortAll_split(array_t<T1>& arr1, array_t<T2>& arr2);

	/// An in-memory sort merge join function.  Sort the input arrays,
	/// valR and valS.  Count the number of results from join.
	template <typename T>
	int64_t sortMerge(array_t<T>& valR, array_t<uint32_t>& indR,
			  array_t<T>& valS, array_t<uint32_t>& indS);
	/// An in-memory sort merge join function with string values.
	int64_t sortMerge(std::vector<std::string>& valR,
			  array_t<uint32_t>& indR,
			  std::vector<std::string>& valS,
			  array_t<uint32_t>& indS);

	/// Sorting function with payload.  Sort keys in ascending order,
	/// move the vals accordingly.  This is an in-place sorting
	/// procedure.
	template <typename T1, typename T2>
	void sortKeys(array_t<T1>& keys, array_t<T2>& vals);
	/// Quicksort.  Sort the keys only.  Use the standard two-way
	/// partitioning.
	template <typename T1, typename T2>
	void sort_quick(array_t<T1>& keys, array_t<T2>& vals);
	/// Quicksort.  Sort the keys only.  Use a nonstandard three-way
	/// partitioning.
	template <typename T1, typename T2>
	void sort_quick3(array_t<T1>& keys, array_t<T2>& vals);
	/// Insertion sort.  It has relatively straightforward memory
	/// access pattern and may be useful to sort a few numbers at the
	/// end of a recursive procedure.
	template <typename T1, typename T2>
	void sort_insertion(array_t<T1>& keys, array_t<T2>& vals);
	/// Shell sort.  It has relatively straightforward memory access
	/// pattern and may be useful to sort a few numbers at the end of a
	/// recursive sorting function.
	template <typename T1, typename T2>
	void sort_shell(array_t<T1>& keys, array_t<T2>& vals);
	/// Partition function for quicksort.  The return value p separates
	/// keys into two parts, keys[..:p-1] < keys[p:..].  A return value
	/// equal to the size of keys indicates all keys are sorted.
	template <typename T1, typename T2>
	uint32_t sort_partition(array_t<T1>& keys, array_t<T2>& vals);
	/// Three-way partitioning algorithm for quicksort.  Upon return
	/// from this function, keys satisfying the following order
	/// keys[0:starteq] < keys[starteq:stargt-1] < keys[startgt:..].
	/// The keys are ordered if starteq = startgt = keys.size().
	template <typename T1, typename T2>
	void sort_partition3(array_t<T1>& keys, array_t<T2>& vals,
			     uint32_t& starteq, uint32_t& startgt);
	/// Sorting function with string keys.  Due to a lack of shallow
	/// copying for std::vector, this is simply a wrapper for
	/// sort_shell, a Shell sorting function.
	void sortStrings(std::vector<std::string>& keys,
			 array_t<uint32_t>& vals);
	void sortStrings(std::vector<std::string>& keys,
			 array_t<uint32_t>& vals, uint32_t begin,
			 uint32_t end);
	/// Shell sorting procedure.  To clean up after the quick sort
	/// procedure.
	void sortStrings_shell(std::vector<std::string>& keys,
			       array_t<uint32_t>& vals,
			       uint32_t begin, uint32_t end);
	/// The partitioning procedure for quick sort.  It implements the
	/// standard two-way partitioning with median-of-three pivot.
	uint32_t sortStrings_partition(std::vector<std::string>& keys,
				       array_t<uint32_t>& vals,
				       uint32_t begin, uint32_t end);

	/// Radix sort.  Allocates buffers needed for copying data.
	/// @{
	void sort_radix(array_t<char>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<unsigned char>& keys,
			array_t<uint32_t>& vals);
	void sort_radix(array_t<uint16_t>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<int16_t>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<uint32_t>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<int32_t>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<uint64_t>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<int64_t>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<float>& keys, array_t<uint32_t>& vals);
	void sort_radix(array_t<double>& keys, array_t<uint32_t>& vals);
	/// @}
    } // namespace util
} // namespace ibis
#endif


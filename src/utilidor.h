// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008-2009 the Regents of the University of California
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
    typedef array_t< rid_t > RIDSet;    // RIDSet

    namespace util {

	///@{
	/// Sorting RID lists.  The first two use quick sort and the third
	/// one use the insertion sort.  None of them are stable sort
	void sortRIDs(ibis::RIDSet& rids);
	void sortRIDs(ibis::RIDSet&, uint32_t, uint32_t);
	void isortRIDs(ibis::RIDSet&, uint32_t, uint32_t);
	///@}

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
	/// move the vals accordingly.
	template <typename T1, typename T2>
	void sortKeys(array_t<T1>& keys, array_t<T2>& vals);
	/// Sorting function with string keys and uint32_t as payload.
	void sortStrings(std::vector<std::string>& keys,
			 array_t<uint32_t>& vals);
    } // namespace util
} // namespace ibis
#endif


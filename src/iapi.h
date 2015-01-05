/* File: $Id$
   Author: John Wu <John.Wu at acm.org>
      Lawrence Berkeley National Laboratory
   Copyright (c) 2001-20154-2014 the Regents of the University of California
*/
#ifndef IBIS_IAPI_H
#define IBIS_IAPI_H
/**
@file This header file defines an in-memory C API for accessing the
querying functionality of FastBit IBIS implementation.  It is primarily for
in memory data.

@note Following the convension established in capi.h, all functions are in
lower case letters mixed with underscores, and all custom data types are in
camel cases with the first letter capitalized.

@note For functions that return integer error code, 0 always indicate
success, a negative number indicate error, a positive number may also be
returned to carry results, such as in fastbit_get_result_size.

@note For functions that returns pointers, a nil pointer is returned in
case of error.

@note About the name: IAPI was original intended to be "In-memory API".
The word iapi appears to be a Dekota word for "word" or "language".
 */
#include "const.h"	// common definitions and declarations
#include "capi.h"	// reuse the definitions from capi.h

/** An enum for data types supported by this interface.

    @note Would have preferred to reuse the enum type ibis::TYPE_T,
    however, there isn't a good way known to the author.
*/
typedef enum FastBitDataType {
    FastBitDataTypeUnknown=0,
    FastBitDataTypeByte=2,
    FastBitDataTypeUByte=3,
    FastBitDataTypeShort=4,
    FastBitDataTypeUShort=5,
    FastBitDataTypeInt=6,
    FastBitDataTypeUInt=7,
    FastBitDataTypeLong=8,
    FastBitDataTypeULong=9,
    FastBitDataTypeFloat=10,
    FastBitDataTypeDouble=11
} FastBitDataType;

/** An enum for comparison operators supported.
 */
typedef enum FastBitCompareType {
    FastBitCompareLess,
    FastBitCompareLessEqual,
    FastBitCompareGreater,
    FastBitCompareGreaterEqual,
    FastBitCompareEqual,
    FastBitCompareNotEqual,
} FastBitCompareType;

/** An enum for specifying how selection conditions are to be
    combined.
 */
typedef enum FastBitCombineType {
    FastBitCombineAnd,
    FastBitCombineOr,
    FastBitCombineXor,
    FastBitCombineNand,
    FastBitCombineNor,
} FastBitCombineType;

/** An opaque pointer to the selection object. */
#ifdef __cplusplus
typedef ibis::qExpr* FastBitSelectionHandle;
#else
typedef void* FastBitSelectionHandle;
#endif
typedef void* FastBitIndexHandle;

/**
@defgroup FastBitIAPI FastBit In-memory API.
@{
*/
#ifdef __cplusplus
extern "C" {
#endif

    /** Create a simple one-sided range condition.
    */
    FastBitSelectionHandle fastbit_selection_osr
    (const char*, FastBitCompareType, double);

    /** Free/destroy the selection object. */
    void fastbit_selection_free(FastBitSelectionHandle);

    /** Combining two selection conditions into one. */
    FastBitSelectionHandle fastbit_selection_combine
    (FastBitSelectionHandle, FastBitCombineType, FastBitSelectionHandle);

    /** Provide an upper bound on the number of hits. */
    int64_t fastbit_selection_estimate(FastBitSelectionHandle);

    /** Compute the number of hits. */
    int64_t fastbit_selection_evaluate(FastBitSelectionHandle);

    /** Extract the coordinates of the elements of arrays satisfying the
        selection conditions. */
    int64_t fastbit_selection_get_coordinates
    (FastBitSelectionHandle, uint64_t*, uint64_t, uint64_t);

    int64_t fastbit_selection_read
    (FastBitDataType, const void *, uint64_t, FastBitSelectionHandle,
     void *, uint64_t, uint64_t);

    /** Free all cached object for IAPI. */
    void fastbit_iapi_free_all();
    /** Remove an array from the list of known variables. */
    void fastbit_iapi_free_array(const char *);
    /** Remove an array from the list of known variables.  The given
        address is that of the data buffer passed to functions
        fastbit_iapi_register_array and fastbit_iapi_register_array_nd. */
    void fastbit_iapi_free_array_by_addr(void *);
    /** Register a simple array under the specified name. */
    int fastbit_iapi_register_array
    (const char*, FastBitDataType, void*, uint64_t);
    /** Register a n-dimensional array under the specified name. */
    int fastbit_iapi_register_array_nd
    (const char*, FastBitDataType, void*, uint64_t*, uint64_t);
    /** Register an external array under the specified name.  The content
        of the array is available through FastBitReadExtArray. */
    int fastbit_iapi_register_array_ext
    (const char*, FastBitDataType, uint64_t*, uint64_t, void*,
     FastBitReadExtArray);
    /** Register an array under the specified name.  Only the index for the
        array is actually available. */
    int fastbit_iapi_register_array_index_only
    (const char*, FastBitDataType, uint64_t*, uint64_t,
     double*, uint64_t, int64_t*, uint64_t, void*, FastBitReadBitmaps);

    /** Build index. */
    int fastbit_iapi_build_index(const char*, const char*);

    /** Write index into three arrays.  This function allocates the memory
        space for three arrays named keys, offsets and bms.  The caller is
        responsible for freeing these three arrays.
    */
    int fastbit_iapi_deconstruct_index
    (const char*, double**keys, uint64_t*nkeys,
     int64_t**offsets, uint64_t*noffsets,
     uint32_t**bms, uint64_t*nbms);

    /** Attach an index to a column already registered.

        @note The current implementation avoids copying the arrays passed
        to this function, therefore, these arrays can not be freed until
        the indexing data structure is cleared with fastbit_iapi_free_all.
    */
    int fastbit_iapi_attach_full_index
    (const char*, double*, uint64_t, int64_t*, uint64_t, uint32_t*, uint64_t);

    /** Attach an index to a column already registered.

        @note The current implementation avoids copying the arrays passed
        to this function, therefore, these arrays can not be freed until
        the indexing data structure is cleared with fastbit_iapi_free_all.
    */
    int fastbit_iapi_attach_index
    (const char*, double*, uint64_t, int64_t*, uint64_t,
     void*, FastBitReadBitmaps);

    /** Reconstitute the index data structure from the first two arrays
        produced by fastbit_iapi_write_index.  The 3rd array is larger and
        is to be read in pieces as needed.

        @Warning To be removed.  Do not use.
    */
    FastBitIndexHandle fastbit_iapi_reconstruct_index
    (double*, uint64_t, int64_t*, uint64_t);

    /** Evalute a range condition on an index data structure. 

        @Warning To be removed.  Do not use.
    */
    int fastbit_iapi_resolve_range
    (FastBitIndexHandle, FastBitCompareType, double, uint32_t *,
     uint32_t *, uint32_t *, uint32_t *);

    /** Retrieve the numbers of values in the given range. 

        @Warning To be removed.  Do not use.
    */
    int64_t fastbit_iapi_get_number_of_hits
    (FastBitIndexHandle, uint32_t, uint32_t, uint32_t*);

    /** Create a simple one-sided range condition. 

        @Warning To be removed.  Do not use.
    */
    FastBitSelectionHandle fastbit_selection_create
    (FastBitDataType, void*, uint64_t, FastBitCompareType, void*);

    /** Create a simple one-sided range condition on a n-dimensional array. 

        @Warning To be removed.  Do not use.
    */
    FastBitSelectionHandle fastbit_selection_create_nd
    (FastBitDataType, void*, uint64_t*, uint64_t, FastBitCompareType, void*);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif


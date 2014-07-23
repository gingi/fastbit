/*
 File: $Id$
 Author: John Wu <John.Wu at acm.org>
      Lawrence Berkeley National Laboratory
 Copyright 20014-2014 the Regents of the University of California
*/
/**
   @file tiapi.c

   A simple test program for functions defined in iapi.h.

    @ingroup FastBitExamples
*/
#include <iapi.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void usage(const char *name) {
    fprintf(stdout, "A simple tester for the in-memory API of %s\n\nusage\n"
	    "%s [maxsize] [verboseness-level]",
            fastbit_get_version_string(), name);
} /* usage */

/** Generate three arrays of specified sizes.
    The three arrays have the following values:
    - a1 has non-negative 16-bit integers going from 0 to 32637 and then
      repeat.
    - a2 contains 32-bit signed integer.  It increaments every two steps.
    - a3 contains 64-bit floating-point numbers that goes from 0 in
      increment of 1/4.
 */
static void fillarrays(size_t n, int16_t *a1, int32_t *a2, double *a3) {
    for (size_t j = 0; j < n; ++ j) {
        a1[j] = (j & 0x7FFFU);
        a2[j] = (j >> 1);
        a3[j] = j * 0.25;
    }
} /* fillarrays */

/** A simple reader to be used by FastBit for index reconstruction.  In
    this simple case, the first argument is the whole array storing all the
    serialized bitmaps.  This first argument can be used to point to a data
    structure pointing to any complex object type necassary.
 */
static int mybmreader(void *ctx, uint64_t start, uint64_t count, uint32_t *buf) {
    unsigned j;
    const uint32_t *bms = (uint32_t*)ctx + start;
    for (j = 0; j < count; ++ j)
        buf[j] = bms[j];
    return 0;
} // mybmreader

typedef struct {
    uint64_t n;
    double *ptr;
} dblbuffer;
static int mydblreader(void *ctx, uint64_t nd, uint64_t *starts,
                       uint64_t *counts, void *data) {
    size_t j;
    double *out = (double*)data;
    dblbuffer *db = (dblbuffer*)ctx;

    if (ctx == 0 || nd != 1 || starts == 0 || counts == 0 || data == 0)
        return -1;

    if (*starts >= db->n) {
        *counts = 0;
        return 0; // nothing to copy
    }

    if (db->n - *starts < *counts)
        *counts = db->n - *starts;
    for (j = 0; j < *counts; ++ j)
        out[j] = db->ptr[j + *starts];
    return 0;
} // mydblread

static void queryarrays(size_t n, int16_t *a1, int32_t *a2, double *a3) {
    int16_t b1 = 5;
    int32_t b2 = 11;
    double b31 = 2.0, b32 = 3.5;
    long int i, ierr;
    double *keys; /* NOTE: keys, offsets and bms are used by FastBit */
    int64_t *offsets;  /* after invoking fastbit_iapi_attach_index */
    uint32_t *bms; /* they can only be freed after the index is no longer needed */
    FastBitSelectionHandle h1, h2, h3, h4, h5;
    dblbuffer dbl;
    dbl.n = n;
    dbl.ptr = a3;

    ierr = fastbit_iapi_register_array("a1", FastBitDataTypeShort, a1, n);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_iapi_register_array failed to register a1, "
               "ierr = %ld\n", ierr);
        return;
    }
    ierr = fastbit_iapi_register_array("a2", FastBitDataTypeInt, a2, n);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_iapi_register_array failed to register a2, "
               "ierr = %ld\n", ierr);
        return;
    }
    ierr = fastbit_iapi_register_array_ext
        ("a3", FastBitDataTypeDouble, &(dbl.n), 1, &dbl, mydblreader);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_iapi_register_array failed to register a3, "
               "ierr = %ld\n", ierr);
        return;
    }

    /* build index on a1, automatically attached */
    ierr = fastbit_iapi_build_index("a1", "<binning none/>");
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_iapi_build_index failed to create index "
               "for a1, ierr=%ld\n", ierr);
    }

    h1 = fastbit_selection_osr("a1", FastBitCompareLess, b1);
    ierr = fastbit_selection_estimate(h1);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_selection_estimate(a1 < %d) returned %ld\n",
                (int)b1, ierr);
    }
    ierr = fastbit_selection_evaluate(h1);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_selection_evaluate(a1 < %d) returned %ld\n",
                (int)b1, ierr);
    }
    else {
        long int n1 = ierr;
        int16_t *buf1 = (int16_t*)malloc(2*n1);
        double  *buf3 = (double*)malloc(8*n1);
        long int expected = (n & 0x7FFFL);
        if (expected > b1)
            expected = b1;
        expected += b1 * (n >> 15);
        if (ierr != expected) {
            fflush(0);
            printf("Warning -- fastbit_selection_evaluate(a1 < %d) expected "
                   "%ld, but got %ld\n", (int)b1, expected, ierr);
        }
        else {
            fflush(0);
            printf("fastbit_selection_evaluate(a1 < %d) returned %ld as "
                   "expected\n", (int)b1, ierr);
        }

        if (n1 > 0) {
            ierr = fastbit_selection_read
                (FastBitDataTypeShort, a1, n, h1, buf1, n1, 0U);
            if (ierr != n1) {
                fflush(0);
                printf("Warning -- fastbit_selection_read expected to read %ld "
                       "element(s) of a1, but %ld\n", n1, ierr);
            }
            if (ierr > 0) {
                printf("read a1 where (a1 < %d) got:", (int)b1);
                for (i = 0; i < ierr; ++ i)
                    printf(" %d", (int)buf1[i]);
                printf("\n");
            }

            ierr = fastbit_selection_read
                (FastBitDataTypeDouble, a3, n, h1, buf3, n1, 0U);
            if (ierr != n1) {
                fflush(0);
                printf("Warning -- fastbit_selection_read expected to read %ld "
                       "element(s) of a3, but %ld\n", n1, ierr);
            }
            if (ierr > 0) {
                printf("read a3 where (a1 < %d) got:", (int)b1);
                for (i = 0; i < ierr; ++ i)
                    printf(" %.2lf", buf3[i]);
                printf("\n\n");
            }
        }
        free(buf1);
        free(buf3);
    }
    fastbit_selection_free(h1);


    { /* Serialize the index for a1 and re-attach it */
        uint64_t nk, nf, nb, nv=n;
        ierr = fastbit_iapi_deconstruct_index
            ("a1", &keys, &nk, &offsets, &nf, &bms, &nb);
        if (ierr >= 0) {
            fastbit_iapi_free_array("a1");
            ierr = fastbit_iapi_register_array_index_only
                ("a1", FastBitDataTypeShort, &nv, 1,
                 keys, nk, offsets, nf, bms, mybmreader);
            if (ierr < 0) {
                fflush(0);
                printf("Warning -- fastbit_iapi_attach_index failed "
                       "to attach the index to a1, ierr = %ld\n", ierr);
                fflush(0);
            }
        }
        else {
            fflush(0);
            printf("Warning -- fastbit_iapi_deconstruct_index failed "
                   "to serialize the index of a1, ierr = %ld\n", ierr);
            fflush(0);
        }
    }

    ierr = fastbit_iapi_build_index("a2", (const char*)0);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_iapi_build_index failed to create index "
               "for a2, ierr=%ld\n", ierr);
        fflush(0);
    }
    /* a1 < b1 */
    h1 = fastbit_selection_osr("a1", FastBitCompareLess, b1);
    /* a2 <= b2 */
    h2 = fastbit_selection_osr("a2", FastBitCompareLessEqual, b2);
    /* b31 <= a3 < b32 */
    h3 = fastbit_selection_combine
        (fastbit_selection_osr("a3", FastBitCompareGreaterEqual, b31),
         FastBitCombineAnd,
         fastbit_selection_osr("a3", FastBitCompareLess, b32));
    /* a1 < b1 OR b31 <= a3 < b32 */
    h4 = fastbit_selection_combine(h1, FastBitCombineOr, h3);
    /* a2 <= b2 AND (a1 < b1 OR b31 <= a3 < b32) */
    h5 = fastbit_selection_combine(h2, FastBitCombineAnd, h4);

    ierr = fastbit_selection_evaluate(h5);
    if (ierr < 0) {
        fflush(0);
        printf("Warning -- fastbit_selection_evaluate(...) returned %ld\n",
               ierr);
        fflush(0);
    }
    else {
        long int n1 = ierr;
        int16_t *buf1 = (int16_t*)malloc(2*n1);
        double  *buf3 = (double*)malloc(8*n1);
        long int expected = (n < b1 ? n : b1);

        if (n > 7) {
            if (n > 13) {
                expected += 6;
            }
            else {
                expected += (n - 7);
            }
        }
        if (ierr != expected) {
            fflush(0);
            printf("Warning -- fastbit_selection_evaluate(...) expected %ld, "
                   "but got %ld\n", expected, ierr);
        }
        else {
            fflush(0);
            printf("fastbit_selection_evaluate(...) returned %ld as expected\n",
                   ierr);
        }

        if (n1 > 0) {
            ierr = fastbit_selection_read
                (FastBitDataTypeShort, a1, n, h5, buf1, n1, 0U);
            if (ierr != n1) {
                fflush(0);
                printf("Warning -- fastbit_selection_read expected to read %ld "
                       "element(s) of a1, but %ld\n", n1, ierr);
            }
            if (ierr > 0) {
                printf("read a1 where (...) got:");
                for (i = 0; i < ierr; ++ i)
                    printf(" %d", (int)buf1[i]);
                printf("\n");
            }

            ierr = fastbit_selection_read
                (FastBitDataTypeDouble, a3, n, h5, buf3, n1, 0U);
            if (ierr != n1) {
                fflush(0);
                printf("Warning -- fastbit_selection_read expected to read %ld "
                       "element(s) of a3, but %ld\n", n1, ierr);
            }
            if (ierr > 0) {
                printf("read a3 where (a1 < %d) got:", (int)b1);
                for (i = 0; i < ierr; ++ i)
                    printf(" %.2lf", buf3[i]);
                printf("\n\n");
            }
        }

        free(buf1);
        free(buf3);
    }
    fastbit_selection_free(h5);
    fastbit_iapi_free_all();
    free(offsets);
    free(keys);
    free(bms);
} /* queryarrays */

int main(int argc, char **argv) {
    long int k, nmax=1000;
    int ierr, msglvl=0;
    const char *conffile=0;
    int16_t *a1;
    int32_t *a2;
    double  *a3;
    if (argc > 1) {
        double tmp;
        ierr = sscanf(argv[1], "%lf", &tmp);
        if (ierr == 1) {
            nmax = tmp;
        }
    }
    if (argc > 2) {
        ierr = sscanf(argv[2], "%d", &msglvl);
        if (ierr <= 0)
            msglvl = 0;
    }
    if (argc > 3)
        conffile = argv[3];
#if defined(DEBUG) || defined(_DEBUG)
#if DEBUG + 0 > 10 || _DEBUG + 0 > 10
    msglvl = INT_MAX;
#elif DEBUG + 0 > 0
    msglvl += 7 * DEBUG;
#elif _DEBUG + 0 > 0
    msglvl += 5 * _DEBUG;
#else
    msglvl += 3;
#endif
#endif
    msglvl += 1;
    a1 = (int16_t*)malloc(2*nmax);
    a2 = (int32_t*)malloc(4*nmax);
    a3 = (double*)malloc(8*nmax);
    fastbit_init(conffile);
    fastbit_set_verbose_level(msglvl);
    for (k = 1; k <= nmax; k=((k>(nmax/4)&&k<nmax) ? nmax : 4*k)) {
        printf("\n%s -- testing with k = %ld\n", *argv, k);
        fillarrays(k, a1, a2, a3);
        //indexarrays(k, a1, a2, a3);
        queryarrays(k, a1, a2, a3);
        // need to clear all cached objects so that we can reuse the same
        // pointers a1, a2, a3
        fastbit_iapi_free_all();
    }
    fastbit_cleanup();
    free(a3);
    free(a2);
    free(a1);
    return 0;
} /* main */

/*
 File: $Id$
 Author: John Wu <John.Wu@nersc.gov>
      Lawrence Berkeley National Laboratory
 Copyright 2006-2008 the Regents of the University of California

 A simple test program for functions defined in capi.h.  The basic command
 line options are

 datadir selection-conditions [<column type> <column type>...]

 Types recognized are: i (for integers), u (for unsigned integers), f (for
 floats) and d for (doubles).  Unrecognized types are treated as integers.
*/
#include <capi.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void usage(const char *name) {
    fprintf(stdout, "A simple tester for the C API of FastBit\n\nusage\n"
	    "%s [-c conffile] [-l logfile] [-v [verboseness-level]] "
	    "datadir [conditions] [<column type> ...]\n"
            "In SQL this is equivalent to\n\tFROM datadir "
            "[WHERE conditions [SELECT column type ...]]\n\n"
	    "If only datadir is present, %s indexes all columns in "
	    "the named directory.\n"
	    "If conditions are provided without columns to print, "
	    "%s will print the number of hits.\n"
	    "If any variable is to be printed, it must be specified as "
            "a <name type> pair, where only i, u, f, and d are recognized.\n\n"
	    "Example:\n"
	    "%s dir 'c1 = 15 and c2 > 23' c1 i c3 u\n\n",
            name, name, name, name);
} /* usage */

/** Create a set of sample data and run some canned queries.

    The sample data contains 100 rows and 3 columns.  The columns are named
    'a', 'b', and 'c'.  They are of types 'int', 'short', and 'float'
    respectively.   The columns a and b have values 0, ..., 99 and column c
    has values 100, 99, ..., 1.
 */
static void buildin(const char *nm, FILE* output) {
    int nerrors = 0;
    int i, mult;
    int32_t ivals[100];
    int16_t svals[100];
    float fvals[100];
    const char *dir = "tmp";
    int counts[] = {5, 24, 19, 10, 50};
    const char* conditions[] =
	{"a<5", "a+b>150", "a < 60 and c < 60", "c > 90", "c > a"};

    // prepare a sample data
    for (i = 0; i < 100; ++ i) {
	ivals[i] = i;
	svals[i] = (int16_t) i;
	fvals[i] = (float) (1e2 - i);
    }
    fastbit_add_values("a", "int", ivals, 100, 0);
    fastbit_add_values("b", "short", svals, 100, 0);
    fastbit_add_values("c", "float", fvals, 100, 0);
    fastbit_flush_buffer(dir);
    // test the queries
    mult = fastbit_rows_in_partition(dir);
    if (mult % 100 != 0) { /* no an exact multiple */
	fprintf(output, "Directory %s contains %d rows, but expected 100, "
		"remove the directory and try again\n", dir, mult);
	return;
    }

    mult /= 100;
    for (i = 0; i < 5; ++ i) {
	FastBitQueryHandle h = fastbit_build_query(0, dir, conditions[i]);
	int nhits = fastbit_get_result_rows(h);
	if (nhits != mult * counts[i]) {
	    ++ nerrors;
	    fprintf(output, "%s: query \"%s\" on %d build-in records found "
		    "%d hits, but %d were expected\n", nm, conditions[i],
		    (int)(mult*100), nhits, (int)(mult*counts[i]));
	}
	fastbit_destroy_query(h);
    }
    fprintf(output, "%s: build-in tests finished with nerrors = %d\n",
	    nm, nerrors);
}

int main(int argc, char **argv) {
    int ierr, nhits, vselect;
    const char *conffile;
    const char *logfile;
    FILE* output;
    FastBitQueryHandle qh;
    FastBitResultSetHandle rh;

    ierr = 0;
    vselect = 1;
    logfile = 0;
    conffile = 0;
#if defined(DEBUG) || defined(_DEBUG)
#if DEBUG + 0 > 10 || _DEBUG + 0 > 10
    ierr = INT_MAX;
#elif DEBUG + 0 > 0
    ierr += 7 * DEBUG;
#elif _DEBUG + 0 > 0
    ierr += 5 * _DEBUG;
#else
    ierr += 3;
#endif
#endif
    while (vselect < argc && argv[vselect][0] == '-') {
	if (argv[vselect][1] == 'c' || argv[vselect][1] == 'C') {
	    if (vselect+1 < argc) {
		conffile = argv[vselect+1];
		vselect += 2;
	    }
	    else {
		vselect += 1;
	    }
	}
	else if (argv[vselect][1] == 'h' || argv[vselect][1] == 'H') {
	    usage(*argv);
	    vselect += 1;
	}
	else if (argv[vselect][1] == 'l' || argv[vselect][1] == 'L') {
	    if (vselect+1 < argc) {
		logfile = argv[vselect+1];
		vselect += 2;
	    }
	    else {
		vselect += 1;
	    }
	}
	else if (argv[vselect][1] == 'm' || argv[vselect][1] == 'M' ||
		 argv[vselect][1] == 'v' || argv[vselect][1] == 'V') {
	    if (vselect+1 < argc &&
		(isdigit(argv[vselect+1][0]) != 0 ||
		 (argv[vselect+1][0] == '-' &&
		  isdigit(argv[vselect+1][1]) != 0))) {
		ierr += atoi(argv[vselect+1]);
		vselect += 2;
	    }
	    else {
		ierr += 1;
		vselect += 1;
	    }
	}
	else {
	    fprintf(stderr, "%s: unknown option %s\n", *argv, argv[vselect]);
	    ++ vselect;
	}
    }

    fastbit_init((const char*)conffile);
    fastbit_set_verbose_level(ierr);
    fastbit_set_logfile(logfile);
    output = fastbit_get_logfilepointer();
    if (argc <= vselect) {
	buildin(*argv, output);
	return -1;
    }

    if (argc == vselect+1) /* buld indexes */
	return fastbit_build_indexes(argv[vselect], (const char*)0);

    qh = fastbit_build_query(0, argv[vselect], argv[vselect+1]);
    if (qh == 0) {
	fprintf(output, "%s failed to process query \"%s\" on data in %s\n",
		argv[0], argv[vselect+1], argv[vselect]);
	fastbit_cleanup();
	return -2;
    }

    nhits = fastbit_get_result_rows(qh);
    fprintf(output, "%s: applying \"%s\" on data in %s produced %d hit%s\n",
	    argv[0], argv[vselect+1], argv[vselect], nhits,
	    (nhits>1 ? "s" : ""));
    if (nhits <= 0)
	return 0;

    /* print the selected valued specified in the query string (select
       clause) */
    rh = fastbit_build_result_set(qh);
    if (rh != 0) {
	int ncols = fastbit_get_result_columns(qh);
	fprintf(output, "%s\n", fastbit_get_select_clause(qh));
	while (fastbit_result_set_next(rh) == 0) {
	    int i;
	    fprintf(output, "%s", fastbit_result_set_getString(rh, 0));
	    for (i = 1; i < ncols; ++ i)
		fprintf(output, ", %s", fastbit_result_set_getString(rh, i));
	    fprintf(output, "\n");
	}
	fastbit_destroy_result_set(rh);
    }
    fflush(output);

    vselect += 2;
    /* print attributes explicitly specified on command line */
    if (argc > vselect) {
	int i, j;
	for (i = vselect; i < argc; i += 2) {
	    char t = (i+1<argc ? argv[i+1][0] : 'i');
	    switch (t) {
	    default:
	    case 'i':
	    case 'I': {
		const int32_t *const tmp =
		    fastbit_get_qualified_ints(qh, argv[i]);
		if (tmp != 0) {
		    fprintf(output, "%s[%d]=", argv[i], nhits);
		    for (j = 0; j < nhits; ++ j)
			fprintf(output, "%d ", tmp[j]);
		    fprintf(output, "\n");
		}
		else {
		    fprintf(output, "%s: failed to retrieve values for "
			    "column %s (requested type %c)\n",
			    argv[0], argv[i], t);
		}
		break;}
	    case 'u':
	    case 'U': {
		const uint32_t *const tmp =
		    fastbit_get_qualified_uints(qh, argv[i]);
		if (tmp != 0) {
		    fprintf(output, "%s[%d]=", argv[i], nhits);
		    for (j = 0; j < nhits; ++ j)
			fprintf(output, "%u ", tmp[j]);
		    fprintf(output, "\n");
		}
		else {
		    fprintf(output, "%s: failed to retrieve value for "
			    "column %s (requested type %c)\n",
			    argv[0], argv[i], t);
		}
		break;}
	    case 'r':
	    case 'R':
	    case 'f':
	    case 'F': {
		const float *const tmp =
		    fastbit_get_qualified_floats(qh, argv[i]);
		if (tmp != 0) {
		    fprintf(output, "%s[%d]=", argv[i], nhits);
		    for (j = 0; j < nhits; ++ j)
			fprintf(output, "%g ", tmp[j]);
		    fprintf(output, "\n");
		}
		else {
		    fprintf(output, "%s: failed to retrieve value for "
			    "column %s (requested type %c)\n",
			    argv[0], argv[i], t);
		}
		break;}
	    case 'd':
	    case 'D': {
		const double *const
		    tmp = fastbit_get_qualified_doubles(qh, argv[i]);
		if (tmp != 0) {
		    fprintf(output, "%s[%d]=", argv[i], nhits);
		    for (j = 0; j < nhits; ++ j)
			fprintf(output, "%lG ", tmp[j]);
		    fprintf(output, "\n");
		}
		else {
		    fprintf(output, "%s: failed to retrieve value for "
			    "column %s (requested type %c)\n",
			    argv[0], argv[i], t);
		}
		break;}
	    }
	}
    }

    ierr = fastbit_destroy_query(qh);
    fastbit_cleanup();
    return ierr;
} /* main */

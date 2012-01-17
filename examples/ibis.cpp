// $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2001-2012 the Regents of the University of California
//
/** @file ibis.cpp

    IBIS -- Interactive Bitmap Index Search

    A sample code to exercises the main features of the FastBit bitmap
    indexing and search capabilities.  It can ingest data through append
    operations, build indexes, and answer a limited version of SQL select
    statement.  These SQL statments may be entered either as command line
    arguments or from standard input.

    The queries are specified in a simplified SQL statement of the form:
    <pre>
    [SELECT ...] [FROM ...] WHERE ... [ORDER BY colname [ASC | DESC] [colname [ASC | DESC]]] [LIMIT ...]
    </pre>

    The SELECT clause contains a list of column names and some of the
    following one-argument functions, AVG, MAX, MIN, SUM, VARPOP, VARSAMP,
    STDPOP, STDSAMP, DISTINCT, e.g., "SELECT a, b, AVG(c), MIN(d)."  If
    specified, the named columns of qualified records will be displayed as
    the result of the query.  The unqualified variables will be used to
    group the selected records; for each group the values of the functions
    are evaluated.  This is equivalent to use all unqualified variables in
    the "GROUP BY" clause.  Note the print out always orders the unqualified
    variables first followed by the values of the functions.  It always has
    an implicit "count(*)" as the end of each line of print out.

    The FROM clause contains a list of data partition names.  If specified,
    the search will be performed only on the named partitions.  Otherwise,
    the search is performed on all known tables.

    The column names and partition names can be delimited by either ',', or
    ';'.  The leading space and trailing space of each name will be removed
    and no space is allowed in the middle of the names.

    The WHERE clause specifies the condition of the query.  It is specified
    as range queries of the form
    <pre>
    RANGE LOGICAL_OP RANGE
    </pre>
    where LOGICAL_OP can be one of "and", "or", "xor", "minus", "&&", "&",
    "||", "|", "^", and "-".  Note the logical "minus" operations can be
    viewed as a short-hand for "AND NOT," i.e., "A minus B" is exactly the
    same as "A AND NOT B."

    A range is specifed on one column of the form
    <pre>
    ColumnA CMP Constant
    </pre>
    where CMP can be one of =, ==, !=, >, >=, <, <=.

    The ranges and expressions can also be negated with either '!' or '~'.

    The ORDER BY clause and the LIMIT clause are applied after the implicit
    GROUP BY operation has been performed.  The expressions in the ORDER BY
    clause must be a proper subset of the SELECT clause.  The modifiers ASC
    and DESC are optional.  By default ASC (ascending) order is used.  One
    may use DESC to change to use the descending order.

    The LIMIT clause limits the maximum number of output rows.  Only number
    may follow the LIMIT keyword.  This clause has effects only if the
    preceeding WHERE clause selected less than or equal to the specified
    number of rows (after applying the implicit group by clause).

    Command line options:
    <pre>
    -append data_dir [output_dir / partition_name]
    -build-indexes [numThreads|indexSpec] -z[ap-existing-indexes]
    -conf conf_file
    -datadir data_dir
    -estimation-only
    -f query-file-name
    -help
    -interactive
    -join part1 part2 join-column conditions1 conditions2 [columns ...]
    -keep-temporary-files
    -log logfilename
    -no-estimation
    -output-file filename
    -query [SELECT ...] [FROM ...] WHERE ...
    -squential-scan
    -rid-check [filename]
    -reorder data_dir[:colname1,colname2...]
    -t[=| ]n
    -v[=| ]n
    -yank filename|conditions
    </pre>

    An explanation of these command line arguments are provided at
    <http://lbl.gov/~kwu/fastbit/doc/ibisCommandLine.html>.

    @note Options can be specified with the minimal distinguishing prefixes,
    which in most cases is just the first letter.

    @note Options -no-estimation and -estimation-only are mutually
    exclusive, the one that appears later will overwrite the one that
    appears early on the same command line.

    @note Option -t is interpreted as self-testing if no query is specified
    on the same command line; however if there are any query, it is
    interpreted as indicating the number of threads to use.

    @note The select clause of "count(*)" produces a result table with one
    row and one column to hold the content of "count(*)" following the SQL
    standard.  If no select clause is specified at all, this program will
    print the number of hits.  In either case, one gets back the number of
    hits, but different handling is required.

    @ingroup FastBitExamples
*/
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include <ibis.h>
#include <mensa.h>	// ibis::mensa
#include <twister.h>

#include <sstream>	// std::ostringstream
#include <algorithm>	// std::sort
#include <memory>	// std::auto_ptr
#include <iomanip>	// std::setprecision

/// The data structure for holding information about query jobs for
/// multi-threaded testing.
struct thArg {
    const char* uid;
    const std::vector<const char*>& qlist;
    ibis::util::counter& task;

    thArg(const char* id, const std::vector<const char*>& ql,
	  ibis::util::counter& tc)
	: uid(id), qlist(ql), task(tc) {}
};

// global varialbes defined in this file
static unsigned testing = 0;
static unsigned threading = 0;
static unsigned build_index = 0;
// <0 skip estimation, =0 do estimation, >0 estimation only
static int estimate_opt = -1;
static bool sequential_scan = false;
static bool verify_rid = false;
static bool zapping = false;
static bool appendToOutput = false;
static bool outputnamestoo = false;
static const char *ridfile = 0;
static const char *appendto = 0;
static const char *outputfile = 0;
static const char *indexingOption = 0;
static const char *yankstring = 0;
static const char *keepstring = 0;
typedef std::pair<const char*, const char*> namepair;

namespace ibis {
#if defined(TEST_SCAN_OPTIONS)
    // a temporary option for testing various options of performing scan for
    // candidate check
    int _scan_option = 0; // default to the old way
#endif
#if defined(TEST_SUMBINS_OPTIONS)
    // a temporary option for controlling the various options of performing
    // the sumBins operation in index.cpp
    int _sumBins_option = 0; // default to the old way
#endif

    /// A simple data structure to hold information about a request for
    /// join operation.
    struct joinspec {
	const char *part1; ///< Name of the first/left data partition.
	const char *part2; ///< Name of the second/right data partition.
	const char *jcol;  ///< Name of the join column (part1.jcol=part2.jcol).
	const char *cond1; ///< Constraints on part1.
	const char *cond2; ///< Constraints on part2.
	std::string selcol; ///< Selected columns.

	joinspec() : part1(0), part2(0), jcol(0), cond1(0), cond2(0) {}
	void print(std::ostream& out) const;
    }; // joinspec
    typedef std::vector<joinspec*> joinlist;
}

// printout the usage string
static void usage(const char* name) {
    std::cout << "\n"
#ifdef FASTBIT_STRING
	      << FASTBIT_STRING
#else
	      << "FastBit ibis1.2"
#endif
	      << "\nList of options for " << name
	      << "\n\t[-a[ppend] data_dir [output_dir / partition_name]]"
	"\n\t[-b[uild-indexes] [numThreads|indexSpec] -z[ap-existing-indexes]]"
	"\n\t[-c[onf] conf_file]"
	"\n\t[-d[atadir] data_dir]"
	"\n\t[-e[stimation]]"
	"\n\t[-f query-file]"
	"\n\t[-h[elp]]"
	"\n\t[-i[nteractive]]"
	"\n\t[-j[oin] part1 part2 join-column conditions1 conditions2 [columns ...]]"
	"\n\t[-k[eep-temporary-files]]"
	"\n\t[-l logfilename]"
	"\n\t[-n[o-estimation]]"
	"\n\t[-o[utput-file] filename]"
	"\n\t[-p[rint] options]"
	"\n\t[-q[uery] [SELECT ...] [FROM ...] WHERE ...]"
	"\n\t[-ri[d-check] [filename]]"
	"\n\t[-r[eorder] data_dir[:colname1,colname2...]]"
	"\n\t[-s[quential-scan]]"
	"\n\t[-t[=n]]"
	"\n\t[-v[=n]]"
	"\n\t[-y[ank] filename|conditions]"
	"\n\t[-z[ap]]\n\n"
	"NOTE: multiple -c -d -f -q and -v options may be specified.  "
	"Queries are applied to all data partitions by default.  "
	"Verboseness levels are cumulated.\n\n"
	"NOTE: options -n and -e are mutually exclusive, the one that appears "
	"later overwrites the earlier ones on the same command line.\n\n"
	"NOTE: option -t is interpreted as testing if specified alone, "
	"however if any query is also specified, it is interpreted as "
	"number of threads\n\n"
	"NOTE: option -y must be followed by either a file name or a list "
	"of conditions.  The named file may contain arbitrary number of "
	"non-negative integers that are treated as row numbers (starting "
	"from 0).  The rows whose numbers are specified in the file will "
	"be marked inactive and will not participate in any further queries.  "
	"If a set of conditions are specified, all rows satisfying the "
	"conditions will be marked inactive.  Additionally, if the -z option "
	"is also specified, all inactive rows will be purged permanently "
	"from the data files.\n\n"
	"NOTE: option -y is applied to all data partitions known to this "
        "program.  Use with care!\n\n"
	"NOTE: the output file stores the results selected by queries, the "
	"log file is for the rest of the messages such error messages and "
	"debug information\n"
	      << std::endl;
} // usage

// printout the help message
static void help(const char* name) {
    std::cout << name << " accepts the following commands:\n"
	"help, exit, quit, append and query of the form\n\n"
	"[SELECT column_names] [FROM dataset_names] WHERE ranges\n\n"
	"The WHERE clause of a query must be specified.  "
	"It is used to determine what records qualify the query.\n"
	"If SELECT clause is present in a query, the qualified "
	"records named columns will be printed, otherwise only "
	"information about number of hits will be printed.\n"
	"If FROM clause is present, the WHERE clause will be "
	"only apply on the named datasets, otherwise, all "
	"available datasets will be used.\n\n"
	"append dir -- add the data in dir to database.\n"
	"print [Parts|Columns|Distributions|column-name [: conditions]]\n"
	"           -- print information about partition names, column names "
	"or an individual column.\n"
	"           -- For an individual column, a set of range conditions "
	"may also be added following a colon (:, denoting such that)\n"
	"exit, quit -- terminate this program.\n"
	"help -- print this message.\n"
	      << std::endl;
} // help

void ibis::joinspec::print(std::ostream& out) const {
    if (!selcol.empty()) {
	out << "Select " << selcol << " ";
    }
    out << "From " << part1 << " Join "	<< part2 << " Using(" << jcol << ")";
    if (cond1 != 0) {
	if (cond2 != 0) {
	    out << " Where " << cond1 << " And " << cond2;
	}
	else {
	    out << " Where " << cond1;
	}
    }
    else if (cond2 != 0) {
	out << " Where " << cond2;
    }
} // ibis::joinspec::print

// std::ostream& operator<<(std::ostream& out, const ibis::joinspec& js) {
//     js.print(out);
//     return out;
// }

// show column names
static void printNames() {
    ibis::part::info* tinfo;
    ibis::util::logger lg;
    for (ibis::partList::const_iterator it = ibis::datasets.begin();
	 it != ibis::datasets.end(); ++it) {
	tinfo = new ibis::part::info(**it);
	lg() << "Partition " << tinfo->name << ":\n";
	std::vector<ibis::column::info*>::const_iterator vit;
	for (vit = tinfo->cols.begin(); vit != tinfo->cols.end(); ++vit)
	    lg() << (*vit)->name << ' ';
	lg() << "\n";
	delete tinfo;
    }
} // printNames

// print all partitions and columns
static void printAll() {
    ibis::util::logger lg;
    ibis::partList::const_iterator it;
    for (it = ibis::datasets.begin(); it != ibis::datasets.end(); ++it)
	(*it)->print(lg());
} // printAll

// Print the detailed information about a specific column.  It will use a
// more detailed distribution than that printed by function
// printDistribution.
static void printColumn(const ibis::part& tbl, const char* cname,
			const char* cond) {
    ibis::column* col = tbl.getColumn(cname);
    if (col == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "printColumn: " << cname << " is not a known column name";
	return;
    }

    std::vector<double> bounds;
    std::vector<uint32_t> counts;
    double amin = col->getActualMin();
    double amax = col->getActualMax();
    long nb = tbl.get1DDistribution(cond, cname, 256, bounds, counts);

    if (nb <= 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "printColumn(" << tbl.name() << ", " << cname << ", " << cond
	    << ") get1DDistribution returned error code " << nb;
	return;
    }
    else if (nb != (long)counts.size() || bounds.size() != counts.size()+1) {
	ibis::util::logger lg;
	lg() << "get1DDistribution return value (" << nb
	     << ") does match the size of array counts ("
	     << counts.size() << ") or bounds.size(" << bounds.size()
	     << ") does not equual to 1+counts.size (" << counts.size();
	return;
    }
    else {
	uint32_t tot = 0;
	ibis::util::logger lg;
	lg() << "Column " << cname << " in Partition "
	     << tbl.name() << ":\n";
	col->print(lg());
	lg() << ", actual range <" << amin << ", " << amax
	     << ">\nHistogram [" << nb << "]";
	if (cond != 0 && *cond != 0)
	    lg() << " under the condition of \"" << cond
		 << "\"";
	lg() << "\n(bounds,\t# records in bin)\n";
	for (int j = 0; j < nb; ++ j) {
	    if (! (fabs(bounds[j] - bounds[j+1]) >
		   1e-15*(fabs(bounds[j])+fabs(bounds[j+1]))))
		lg() << "*** Error *** bounds[" << j << "] ("
		     << bounds[j] << ") is too close to bounds[" << j+1
		     << "] (" << bounds[j+1] << ")\n";
	    lg() << "[" << bounds[j] << ", " << bounds[j+1] << ")\t"
		 << counts[j] << "\n";
	    tot += counts[j];
	}
	lg() << "  total count = " << tot << ", tbl.nRows() = "
	     << tbl.nRows();
    }
    if (nb > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<ibis::bitvector> bins;
	std::vector<double> boundt;
	ibis::util::logger lg;
	long ierr = tbl.get1DBins(cond, cname, nb, boundt, bins);
	lg() << "\nprintColumn(" << cname << ") -- \n";
	if (ierr < 0) {
	    lg() << "get1DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get1DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (bounds.size() != boundt.size() ||
		 counts.size() != bins.size()) {
	    lg() << "get1DDistribution returned " << counts.size()
		 << " bin" << (counts.size() > 1 ? "s" : "")
		 << ", but get1DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "")
		 << "; bounds.size(" << bounds.size()
		 << "), boundt.size(" << boundt.size()
		 << "), counts.size(" << counts.size()
		 << "), bins.size(" << bins.size() << ")";
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < bounds.size(); ++ i)
		if (bounds[i] != boundt[i]) {
		    lg() << "bounds[" << i << "] (" << bounds[i]
			 << ") != boundt[" << i << "] (" << boundt[i]
			 << ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < counts.size(); ++ i)
		if (bins[i].cnt() != counts[i]) {
		    lg() << "counts[" << i << "] (" << counts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg() << "matching arrays counts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	}
    }
} // printColumn

// This version uses the deprecated getCumulativeDistribution
static void printColumn0(const ibis::part& tbl, const char* cname,
			 const char* cond) {
    ibis::column* col = tbl.getColumn(cname);
    if (col) {
	std::vector<double> bounds;
	std::vector<uint32_t> counts;
	double amin = col->getActualMin();
	double amax = col->getActualMax();
	long nb = tbl.getCumulativeDistribution(cond, cname, bounds, counts);

	ibis::util::logger lg;
	lg() << "Column " << cname << " in Partition "
	     << tbl.name() << ":\n";
	if (nb > 0) {
	    col->print(lg());
	    lg() << ", actual range <" << amin << ", " << amax
		 << ">\ncumulative distribution [" << nb
		 << "]";
	    if (cond != 0 && *cond != 0)
		lg() << " under the condition of \"" << cond
		     << "\"";
	    lg() << "\n(bound,\t# records < bound)\n";
	    for (int j = 0; j < nb; ++ j) {
		if (j > 0 && ! (fabs(bounds[j] - bounds[j-1]) >
				1e-15*(fabs(bounds[j])+fabs(bounds[j-1]))))
		    lg() << "*** Error *** bounds[" << j
			 << "] is too close to bounds[" << j-1
			 << "]\n";
		lg() << bounds[j] << ",\t" << counts[j] << "\n";
	    }
	}
	else {
	    col->print(lg());
	    lg() << " -- getCumulativeDistribution(" << cname
		 << ") failed with error code " << nb;
	}
    }
} // printColumn0

// Print the distribution of each column in the specified partition.  It
// uses two fixed size arrays for storing distributions.  This causes
// coarser distributions to printed.
static void printDistribution(const ibis::part& tbl) {
    std::vector<double> bounds;
    std::vector<uint32_t> counts;
    ibis::part::info tinfo(tbl);
    {
	ibis::util::logger lg;
	lg() << "Partition " << tinfo.name << " (" << tinfo.description
	     << ") -- nRows=" << tinfo.nrows << ", nCols="
	     << tinfo.cols.size() << "\nColumn names: ";
	for (uint32_t i = 0; i < tinfo.cols.size(); ++ i) {
	    lg() << tinfo.cols[i]->name << " ";
	}
    }
    for (uint32_t i = 0; i < tinfo.cols.size(); ++ i) {
	double amin = tbl.getActualMin(tinfo.cols[i]->name);
	double amax = tbl.getActualMax(tinfo.cols[i]->name);
	long ierr = tbl.get1DDistribution(tinfo.cols[i]->name,
					  100, bounds, counts);

	ibis::util::logger lg; // use an IO lock
	lg() << "  Column " << tinfo.cols[i]->name << " ("
	     << tinfo.cols[i]->description << ") "
	     << ibis::TYPESTRING[tinfo.cols[i]->type]
	     << " expected range [" << tinfo.cols[i]->expectedMin
	     << ", " << tinfo.cols[i]->expectedMax << "]";
	if (ierr > 1) {
	    lg() <<", actual range <" << amin << ", " << amax
		 << ">\n # bins " << ierr << "\n";
	    for (int j = 0; j < ierr; ++ j) {
		if (! (fabs(bounds[j] - bounds[j+1]) >
		       1e-15*(fabs(bounds[j])+fabs(bounds[j+1]))))
		    lg() << "*** Error *** bounds[" << j << "] ("
			 << bounds[j] << ") is too close to bounds["
			 << j+1 << "] (" << bounds[j+1] << ")\n";
		lg() << "[" << bounds[j] << ", " << bounds[j+1] << ")\t"
		     << counts[j] << "\n";
	    }
	}
	else {
	    lg() << "\ngetCumulativeDistribution returned ierr="
		 << ierr << ", skip ...";
	}
    }
} // printDistribution

static void printDistribution() {
    ibis::partList::const_iterator it;
    for (it = ibis::datasets.begin(); it != ibis::datasets.end(); ++it) {
	printDistribution(**it);
    }
} // printDistribution

// print 1D weighted distribution -- exercise the new get2DDistribution that
// uses (begin, end, stride) triplets
static void print1DDistribution(const ibis::part& tbl, const char *cond,
				const char *col1, const char *wt) {
    const uint32_t NB1 = 100;
    const ibis::column *cptr1 = tbl.getColumn(col1);
    const ibis::column *cptrw = tbl.getColumn(wt);
    std::string evt = "print1DDistribution(";
    evt += tbl.name();
    evt += ", ";
    evt += col1;
    evt += ", ";
    evt += wt;
    if (cond != 0) {
	evt += ", ";
	evt += cond;
    }
    evt += ')';
    if (cptr1 == 0 || cptrw == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " can not proceed because some of the names are not found "
	    << "in data partition " << tbl.name();
	return;
    }

    double amin1 = cptr1->getActualMin();
    double amax1 = cptr1->getActualMax();
    if (amin1 > amax1) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " can not proceed due to failure to determine min/max values";
	return;
    }

    double stride1;
    if (amin1 >= amax1) {
	stride1 = 1.0;
    }
    else if (cptr1->isFloat()) {
	stride1 = (amax1 - amin1) / NB1;
	stride1 = ibis::util::compactValue2(stride1, stride1*(1.0+0.75/NB1));
    }
    else {
	stride1 = ibis::util::compactValue2((amax1 - amin1) / NB1,
					    (amax1 + 1 - amin1) / NB1);
    }
    long ierr;
    std::vector<double> weights;
    ierr = tbl.get1DDistribution(cond,
				 col1, amin1, amax1, stride1,
				 wt, weights);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == weights.size()) {
	ibis::util::logger lg;
	lg() << "\n1D-Weighted distribution of " << col1
	     << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << weights.size() << " bin"
	     << (weights.size() > 1 ? "s" : "") << "\n";

	uint32_t cnt = 0;
	double tot = 0.0;
	for (uint32_t i = 0; i < weights.size(); ++ i) {
	    if (weights[i] > 0) {
		lg() << i << "\t[" << amin1+stride1*i << ", "
		     << amin1+stride1*(i+1)
		     << ")\t" << weights[i] << "\n";
		tot += weights[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total weight = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "Warning -- " << evt
	     << " get1DDistribution returned with ierr = " << ierr
	     << ", weights.size() = " << weights.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<double> sum2;
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get1DBins(cond,
			     col1, amin1, amax1, stride1,
			     wt, sum2, bins);
	ibis::util::logger lg;
	lg() << "\n" << evt << "-- \n";
	if (ierr < 0) {
	    lg() << "get1DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size() || ierr != (long)sum2.size()) {
	    lg() << "get1DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << " and sum2.size() is " << sum2.size()
		 << "; these two values are expected to be the same";
	}
	else if (weights.size() != bins.size()) {
	    lg() << "get1DDistribution returned " << weights.size()
		 << " bin" << (weights.size() > 1 ? "s" : "")
		 << ", but get1DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < weights.size(); ++ i) {
		if (sum2[i] != weights[i]) {
		    lg() << "weights[" << i << "] (" << weights[i]
			 << ") != sum2[" << i << "] (" << sum2[i]
			 << ")\n";
		}
		if (bins[i] != 0) {
		    ibis::array_t<double> *tmp =
			cptrw->selectDoubles(*(bins[i]));
		    if (tmp == 0) {
			lg() << "** failed to retrieve "
			     << bins[i]->cnt() << " value"
			     << (bins[i]->cnt() > 1 ? "s" : "")
			     << " from " << wt << "for bin " << i
			     << "\n";
			++ ierr;
		    }
		    else {
			double w = 0.0;
			for (size_t j = 0; j < tmp->size(); ++ j)
			    w += (*tmp)[j];
			if (w != weights[i]) {
			    lg() << "weights[" << i << "] ("
				 << weights[i]
				 << ") != sum of bins[" << i << "] ("
				 << w << ") from " << bins[i]->cnt()
				 << " value"
				 << (bins[i]->cnt() > 1 ? "s" : "")
				 << "\n";
			    ++ ierr;
			}
		    }
		}
		else if (bins[i] == 0 && weights[i] != 0) {
		    lg() << "weights[" << i << "] (" << weights[i]
			 << "), but bins[" << i << "] is nil (0)\n";
		    ++ ierr;
		}
	    }
	    if (ierr > 0)
		lg() << "Warning -- ";
	    lg() << "matching arrays weights and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "") << "\n";
	}
	ibis::util::clearVec(bins);
    }
} // print1DDistribution

// print 2D weighted distribution -- exercise the new get2DDistribution that
// uses (begin, end, stride) triplets
static void print2DDistribution(const ibis::part& tbl, const char *cond,
				const char *col1, const char *col2,
				const char *wt) {
    const uint32_t NB1 = 20;
    const ibis::column *cptr1 = tbl.getColumn(col1);
    const ibis::column *cptr2 = tbl.getColumn(col2);
    const ibis::column *cptrw = tbl.getColumn(wt);
    std::string evt = "print2DDistribution(";
    evt += tbl.name();
    evt += ", ";
    evt += col1;
    evt += ", ";
    evt += col2;
    evt += ", ";
    evt += wt;
    if (cond != 0) {
	evt += ", ";
	evt += cond;
    }
    evt += ')';

    if (cptr1 == 0 || cptr2 == 0 || cptrw == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " can not proceed because some of the names are not found "
	    << "in data partition " << tbl.name();
	return;
    }

    double amin1 = cptr1->getActualMin();
    double amin2 = cptr2->getActualMin();
    double amax1 = cptr1->getActualMax();
    double amax2 = cptr2->getActualMax();
    if (amin1 > amax1 || amin2 > amax2) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " can not proceed due to failure to determine min/max values";
	return;
    }

    double stride1, stride2;
    if (amin1 >= amax1) {
	stride1 = 1.0;
    }
    else if (cptr1->isFloat()) {
	stride1 = (amax1 - amin1) / NB1;
	stride1 = ibis::util::compactValue2(stride1, stride1*(1.0+0.75/NB1));
    }
    else {
	stride1 = ibis::util::compactValue2((amax1 - amin1) / NB1,
					    (amax1 + 1 - amin1) / NB1);
    }
    if (amin2 >= amax2) {
	stride2 = 1.0;
    }
    else if (cptr2->isFloat()) {
	stride2 = (amax2 - amin2) / NB1;
	stride2 = ibis::util::compactValue2(stride2, stride2*(1.0+0.75/NB1));
    }
    else {
	stride2 = ibis::util::compactValue2((amax2 - amin2) / NB1,
					    (amax2 + 1 - amin2) / NB1);
    }
    long ierr;
    std::vector<double> weights;
    ierr = tbl.get2DDistribution(cond,
				 col1, amin1, amax1, stride1,
				 col2, amin2, amax2, stride2,
				 wt, weights);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == weights.size()) {
	ibis::util::logger lg;
	lg() << "\n2D-Weighted distribution of " << col1 << " and "
	     << col2 << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << weights.size() << " bin"
	     << (weights.size() > 1 ? "s" : "") << " on " << NB1
	     << " x " << NB1 << " cells\n";

	uint32_t cnt = 0;
	double tot = 0.0;
	for (uint32_t i = 0; i < weights.size(); ++ i) {
	    if (weights[i] > 0) {
		const uint32_t i1 = i / NB1;
		const uint32_t i2 = i % NB1;
		lg() << i << "\t[" << amin1+stride1*i1 << ", "
		     << amin1+stride1*(i1+1)
		     << ") [" << amin2+stride2*i2 << ", "
		     << amin2+stride2*(i2+1)
		     << ")\t" << weights[i] << "\n";
		tot += weights[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total weight = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "Warning -- part[" << tbl.name()
	     << "].get2DDistribution returned with ierr = " << ierr
	     << ", weights.size() = " << weights.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<double> sum2;
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get2DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     wt, sum2, bins);
	ibis::util::logger lg;
	lg() << "\n" << evt << " -- \n";
	if (ierr < 0) {
	    lg() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size() || ierr != (long)sum2.size()) {
	    lg() << "get2DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << " and sum2.size() is " << sum2.size()
		 << "; these two values are expected to be the same";
	}
	else if (weights.size() != bins.size()) {
	    lg() << "get2DDistribution returned " << weights.size()
		 << " bin" << (weights.size() > 1 ? "s" : "")
		 << ", but get2DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < weights.size(); ++ i) {
		if (sum2[i] != weights[i]) {
		    lg() << "weights[" << i << "] (" << weights[i]
			 << ") != sum2[" << i << "] (" << sum2[i]
			 << ")\n";
		}
		if (bins[i] != 0) {
		    ibis::array_t<double> *tmp =
			cptrw->selectDoubles(*(bins[i]));
		    if (tmp == 0) {
			lg() << "** failed to retrieve "
			     << bins[i]->cnt() << " value"
			     << (bins[i]->cnt() > 1 ? "s" : "")
			     << " from " << wt << "for bin " << i
			     << "\n";
			++ ierr;
		    }
		    else {
			double w = 0.0;
			for (size_t j = 0; j < tmp->size(); ++ j)
			    w += (*tmp)[j];
			if (w != weights[i]) {
			    lg() << "weights[" << i << "] ("
				 << weights[i]
				 << ") != sum of bins[" << i << "] ("
				 << w << ") from " << bins[i]->cnt()
				 << " value"
				 << (bins[i]->cnt() > 1 ? "s" : "")
				 << "\n";
			    ++ ierr;
			}
		    }
		}
		else if (bins[i] == 0 && weights[i] != 0) {
		    lg() << "weights[" << i << "] (" << weights[i]
			 << "), but bins[" << i << "] is nil (0)\n";
		    ++ ierr;
		}
	    }
	    if (ierr > 0)
		lg() << "Warning -- ";
	    lg() << "matching arrays weights and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "") << "\n";
	}
	ibis::util::clearVec(bins);
    }
} // print2DDistribution

// print 3D weighted distribution -- exercise the new get2DDistribution that
// uses (begin, end, stride) triplets
static void print3DDistribution(const ibis::part& tbl, const char *cond,
				const char *col1, const char *col2,
				const char *col3, const char *wt) {
    const uint32_t NB1 = 10;
    const ibis::column *cptr1 = tbl.getColumn(col1);
    const ibis::column *cptr2 = tbl.getColumn(col2);
    const ibis::column *cptr3 = tbl.getColumn(col3);
    const ibis::column *cptrw = tbl.getColumn(wt);
    std::string evt = "print3DDistribution(";
    evt += tbl.name();
    evt += ", ";
    evt += col1;
    evt += ", ";
    evt += col2;
    evt += ", ";
    evt += col3;
    evt += ", ";
    evt += wt;
    if (cond != 0) {
	evt += ", ";
	evt += cond;
    }
    evt += ')';

    if (cptr1 == 0 || cptr2 == 0 || cptr3 == 0 || cptrw == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " can not proceed because some of the names are not found "
	    << "in data partition " << tbl.name();
	return;
    }

    double amin1 = cptr1->getActualMin();
    double amin2 = cptr2->getActualMin();
    double amin3 = cptr3->getActualMin();
    double amax1 = cptr1->getActualMax();
    double amax2 = cptr2->getActualMax();
    double amax3 = cptr3->getActualMax();
    if (amin1 > amax1 || amin2 > amax2 || amin3 > amax3) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << evt
	    << " can not proceed due to failure to determine min/max values";
	return;
    }

    double stride1, stride2, stride3;
    if (amin1 >= amax1) {
	stride1 = 1.0;
    }
    else if (cptr1->isFloat()) {
	stride1 = (amax1 - amin1) / NB1;
	stride1 = ibis::util::compactValue2(stride1, stride1*(1.0+0.75/NB1));
    }
    else {
	stride1 = ibis::util::compactValue2((amax1 - amin1) / NB1,
					    (amax1 + 1 - amin1) / NB1);
    }
    if (amin2 >= amax2) {
	stride2 = 1.0;
    }
    else if (cptr2->isFloat()) {
	stride2 = (amax2 - amin2) / NB1;
	stride2 = ibis::util::compactValue2(stride2, stride2*(1.0+0.75/NB1));
    }
    else {
	stride2 = ibis::util::compactValue2((amax2 - amin2) / NB1,
					    (amax2 + 1 - amin2) / NB1);
    }
    if (amin3 >= amax3) {
	stride3 = 1.0;
    }
    else if (cptr3->isFloat()) {
	stride3 = (amax3 - amin3) / NB1;
	stride3 = ibis::util::compactValue2(stride3, stride3*(1.0+0.75/NB1));
    }
    else {
	stride3 = ibis::util::compactValue2((amax3 - amin3) / NB1,
					    (amax3 + 1 - amin3) / NB1);
    }
    long ierr;
    std::vector<double> weights;
    ierr = tbl.get3DDistribution(cond,
				 col1, amin1, amax1, stride1,
				 col2, amin2, amax2, stride2,
				 col3, amin3, amax3, stride3,
				 wt, weights);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == weights.size()) {
	ibis::util::logger lg;
	lg() << "\n3D-Weighted distribution of " << col1 << ", "
	     << col2 << " and " << col3 << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << weights.size() << " bin"
	     << (weights.size() > 1 ? "s" : "") << " on " << NB1
	     << " x " << NB1 << " x " << NB1 << " cells\n";

	uint32_t cnt = 0;
	double tot = 0.0;
	for (uint32_t i = 0; i < weights.size(); ++ i) {
	    if (weights[i] > 0) {
		const uint32_t i1 = i / (NB1 * NB1);
		const uint32_t i2 = (i / NB1) % NB1;
		const uint32_t i3 = i % NB1;
		lg() << i << "\t[" << amin1+stride1*i1 << ", "
		     << amin1+stride1*(i1+1)
		     << ") [" << amin2+stride2*i2 << ", "
		     << amin2+stride2*(i2+1)
		     << ") [" << amin3+stride3*i3 << ", "
		     << amin3+stride3*(i3+1)
		     << ")\t" << weights[i] << "\n";
		tot += weights[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total weight = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "Warning -- part[" << tbl.name()
	     << "].get3DDistribution returned with ierr = " << ierr
	     << ", weights.size() = " << weights.size();
	return;
    }

    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<double> sum2;
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get3DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     col3, amin3, amax3, stride3,
			     wt, sum2, bins);
	ibis::util::logger lg;
	lg() << "\n" << evt << " -- \n";
	if (ierr < 0) {
	    lg() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size() || ierr != (long)sum2.size()) {
	    lg() << "get3DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << " and sum2.size() is " << sum2.size()
		 << "; these two values are expected to be the same";
	}
	else if (weights.size() != bins.size()) {
	    lg() << "get3DDistribution returned " << weights.size()
		 << " bin" << (weights.size() > 1 ? "s" : "")
		 << ", but get3DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < weights.size(); ++ i) {
		if (sum2[i] != weights[i]) {
		    lg() << "weights[" << i << "] (" << weights[i]
			 << ") != sum2[" << i << "] (" << sum2[i]
			 << ")\n";
		}
		if (bins[i] != 0) {
		    ibis::array_t<double> *tmp =
			cptrw->selectDoubles(*(bins[i]));
		    if (tmp == 0) {
			lg() << "** failed to retrieve "
			     << bins[i]->cnt() << " value"
			     << (bins[i]->cnt() > 1 ? "s" : "")
			     << " from " << wt << "for bin " << i
			     << "\n";
			++ ierr;
		    }
		    else {
			double w = 0.0;
			for (size_t j = 0; j < tmp->size(); ++ j)
			    w += (*tmp)[j];
			if (w != weights[i]) {
			    lg() << "weights[" << i << "] ("
				 << weights[i]
				 << ") != sum of bins[" << i << "] ("
				 << w << ") from " << bins[i]->cnt()
				 << " value"
				 << (bins[i]->cnt() > 1 ? "s" : "")
				 << "\n";
			    ++ ierr;
			}
		    }
		}
		else if (bins[i] == 0 && weights[i] != 0) {
		    lg() << "weights[" << i << "] (" << weights[i]
			 << "), but bins[" << i << "] is nil (0)\n";
		    ++ ierr;
		}
	    }
	    if (ierr > 0)
		lg() << "Warning -- ";
	    lg() << "matching arrays weights and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "") << "\n";
	}
	ibis::util::clearVec(bins);
    }
} // print3DDistribution

// print the joint distribution -- exercise the new get2DDistribution that
// uses (begin, end, stride) triplets
static void print2DDistribution(const ibis::part& tbl, const char *col1,
				const char *col2, const char *cond) {
    const uint32_t NB1 = 25;
    const ibis::column *cptr1 = tbl.getColumn(col1);
    const ibis::column *cptr2 = tbl.getColumn(col2);
    if (cptr1 == 0 || cptr2 == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "print2DDistribution(" << tbl.name() << ", " << col1 << ", "
	    << col2 << ", "<< (cond != 0 ? cond : "")
	    << ") can not proceed because some of the names are not found "
	    << "in the named data partition";
	return;
    }

    double amin1 = cptr1->getActualMin();
    double amin2 = cptr2->getActualMin();
    double amax1 = cptr1->getActualMax();
    double amax2 = cptr2->getActualMax();
    if (amin1 > amax1 || amin2 > amax2) {
	LOGGER(ibis::gVerbose >= 0)
	    << "print2DDistribution(" << tbl.name() << ", " << col1 << ", "
	    << col2 << ", "<< (cond != 0 ? cond : "")
	    << ") can not proceed due to failure to determine min/max values";
	return;
    }

    double stride1, stride2;
    if (amin1 >= amax1) {
	stride1 = 1.0;
    }
    else if (cptr1->isFloat()) {
	stride1 = (amax1 - amin1) / NB1;
	stride1 = ibis::util::compactValue2(stride1, stride1*(1.0+0.75/NB1));
    }
    else {
	stride1 = ibis::util::compactValue2((amax1 - amin1) / NB1,
					    (amax1 + 1 - amin1) / NB1);
    }
    if (amin2 >= amax2) {
	stride2 = 1.0;
    }
    else if (cptr2->isFloat()) {
	stride2 = (amax2 - amin2) / NB1;
	stride2 = ibis::util::compactValue2(stride2, stride2*(1.0+0.75/NB1));
    }
    else {
	stride2 = ibis::util::compactValue2((amax2 - amin2) / NB1,
					    (amax2 + 1 - amin2) / NB1);
    }
    long ierr;
    std::vector<uint32_t> cnts;
    ierr = tbl.get2DDistribution(cond,
				 col1, amin1, amax1, stride1,
				 col2, amin2, amax2, stride2,
				 cnts);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == cnts.size()) {
	ibis::util::logger lg;
	lg() << "\n2D-Joint distribution of " << col1 << " and " << col2
	     << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << cnts.size() << " bin"
	     << (cnts.size() > 1 ? "s" : "") << " on " << NB1
	     << " x " << NB1 << " cells\n";

	uint32_t cnt = 0, tot = 0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		const uint32_t i1 = i / NB1;
		const uint32_t i2 = i % NB1;
		lg() << i << "\t[" << amin1+stride1*i1 << ", "
		     << amin1+stride1*(i1+1)
		     << ") [" << amin2+stride2*i2 << ", "
		     << amin2+stride2*(i2+1)
		     << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total count = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "part[" << tbl.name()
	     << "].get2DDistribution returned with ierr = " << ierr
	     << ", cnts.size() = " << cnts.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
#if defined(TEST_CONTAINER_OF_OBJECTS)
	std::vector<ibis::bitvector> bins;
	ierr = tbl.get2DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     bins);
	ibis::util::logger lg;
	lg() << "\nprint2DDistribution(" << col1 << ", " << col2
	     << ") -- \n";
	if (ierr < 0) {
	    lg() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get2DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg() << "get2DDistribution returned " << cnts.size()
		 << " bin" << (cnts.size() > 1 ? "s" : "")
		 << ", but get2DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg() << "matching arrays cnts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	}
#else
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get2DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     bins);
	ibis::util::logger lg;
	lg() << "\nprint2DDistribution(" << col1 << ", " << col2
	     << ") -- \n";
	if (ierr < 0) {
	    lg() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get2DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg() << "get2DDistribution returned " << cnts.size()
		 << " bin" << (cnts.size() > 1 ? "s" : "")
		 << ", but get2DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i] != 0 && bins[i]->cnt() != cnts[i]) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << bins[i]->cnt() << ")\n";
		    ++ ierr;
		}
		else if (bins[i] == 0 && cnts[i] != 0) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "] (0)\n";
		    ++ ierr;
		}
	    lg() << "matching arrays cnts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	}
	ibis::util::clearVec(bins);
#endif
    }
} // print2DDistribution

// print the joint distribution -- exercise the new get2DDistribution
static void print2DDist(const ibis::part& tbl, const char *col1,
			const char *col2, const char *cond) {
    const uint32_t NB1 = 25;
    std::vector<double> bds1, bds2;
    std::vector<uint32_t> cnts;
    long ierr;
    if (cond == 0 || *cond == 0)
	ierr = tbl.get2DDistribution(col1, col2, NB1, NB1, bds1, bds2, cnts);
    else
	ierr = tbl.get2DDistribution(cond, col1, col2, NB1, NB1, bds1, bds2,
				     cnts);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == cnts.size()) {
	ibis::util::logger lg;
	const uint32_t nbin2 = bds2.size() - 1;
	lg() << "\n2D-Joint distribution of " << col1 << " and " << col2
	     << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << cnts.size() << " bin"
	     << (cnts.size() > 1 ? "s" : "") << " on " << bds1.size()-1
	     << " x " << bds2.size()-1 << " cells\n";

	uint32_t cnt = 0, tot=0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		uint32_t i1 = i / nbin2;
		uint32_t i2 = i % nbin2;
		lg() << i << "\t[" << bds1[i1] << ", " << bds1[i1+1]
		     << ") [" << bds2[i2] << ", " << bds2[i2+1]
		     << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total count = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "part[" << tbl.name()
	     << "].get2DDistribution returned with ierr = " << ierr
	     << ", bds1.size() = " << bds1.size() << ", bds2.size() = "
	     << bds2.size() << ", cnts.size() = " << cnts.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<ibis::bitvector> bins;
	std::vector<double> bdt1, bdt2;
	ierr = tbl.get2DBins(cond, col1, col2, NB1, NB1, bdt1, bdt2, bins);
	ibis::util::logger lg;
	lg() << "\nprint2DDistribution(" << col1 << ", " << col2
	     << ") -- \n";
	if (ierr < 0) {
	    lg() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get2DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (bds1.size() != bdt1.size() || bds2.size() != bdt2.size() ||
		 cnts.size() != bins.size()) {
	    lg() << "get2DDistribution returned a " << bds1.size()-1
		 << " x " << bds2.size()-1 << " 2D mesh with "
		 << cnts.size() << " element"
		 << (cnts.size() > 1 ? "s" : "")
		 << ", but get2DBins returned a " << bdt1.size()-1
		 << " x " << bdt2.size()-1 << " 2D mesh with "
		 << bins.size() << " element"
		 << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < bds1.size(); ++ i)
		if (bds1[i] != bdt1[i]) {
		    lg() << "bds1[" << i << "] (" << bds1[i]
			 << ") != bdt1[" << i << "] (" << bdt1[i]
			 << ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < bds2.size(); ++ i)
		if (bds2[i] != bdt2[i]) {
		    lg() << "bds2[" << i << "] (" << bds2[i]
			 << ") != bdt2[" << i << "] (" << bdt2[i]
			 << ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg() << "matching arrays cnts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	    if (ierr > 0)
		lg() << "\nNOTE: due to the different numbers of "
		    "internal bins used for the adaptive histograms, "
		    "get2DDistribution and get2DBins may not produce "
		    "exactly the same answers";
	}
    }
} // print2DDist

// the joint distribution may subject to some conditions -- exercises the
// old getJointDistribution
static void printJointDistribution(const ibis::part& tbl, const char *col1,
				   const char *col2, const char *cond) {
    std::vector<double> bds1, bds2;
    std::vector<uint32_t> cnts;
    ibis::util::logger lg;
    long ierr = tbl.getJointDistribution(cond, col1, col2, bds1, bds2, cnts);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == cnts.size()) {
	const uint32_t nb2p1 = bds2.size() + 1;
	lg() << "\nJoint distribution of " << col1 << " and " << col2
	     << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << cnts.size() << " bin"
	     << (cnts.size() > 1 ? "s" : "") << " on " << bds1.size()+1
	     << " x " << bds2.size()+1 << " cells\n";

	uint32_t cnt = 0, tot=0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		uint32_t i1 = i / nb2p1;
		uint32_t i2 = i % nb2p1;
		if (i1 == 0)
		    lg() << "(..., " << bds1[0] << ")";
		else if (i1 < bds1.size())
		    lg() << "[" << bds1[i1-1] << ", " << bds1[i1]
			 << ")";
		else
		    lg() << "[" << bds1.back() << ", ...)";
		if (i2 == 0)
		    lg() << "(..., " << bds2[0] << ")";
		else if (i2 < bds2.size())
		    lg() << "[" << bds2[i2-1] << ", " << bds2[i2]
			 << ")";
		else
		    lg() << "[" << bds2.back() << ", ...)";
		lg() << "\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total count = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	lg() << "part[" << tbl.name()
	     << "].getJointDistribution returned with ierr = " << ierr
	     << ", bds1.size() = " << bds1.size() << ", bds2.size() = "
	     << bds2.size() << ", cnts.size() = " << cnts.size();
    }
} // printJointDistribution

// print the joint distribution -- exercise the new get3DDistribution that
// uses (begin, end, stride) triplets
static void print3DDistribution(const ibis::part& tbl, const char *col1,
				const char *col2, const char *col3,
				const char *cond) {
    const uint32_t NB1 = 12;
    const ibis::column *cptr1 = tbl.getColumn(col1);
    const ibis::column *cptr2 = tbl.getColumn(col2);
    const ibis::column *cptr3 = tbl.getColumn(col3);
    if (cptr1 == 0 || cptr2 == 0 || cptr3 == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "print3DDistribution(" << tbl.name() << ", " << col1 << ", "
	    << col2 << ", " << col3 << ", "<< (cond != 0 ? cond : "")
	    << ") can not proceed because some of the names are not found "
	    << "in the named data partition";
	return;
    }

    double amin1 = cptr1->getActualMin();
    double amin2 = cptr2->getActualMin();
    double amin3 = cptr3->getActualMin();
    double amax1 = cptr1->getActualMax();
    double amax2 = cptr2->getActualMax();
    double amax3 = cptr3->getActualMax();
    if (amin1 > amax1 || amin2 > amax2 || amin3 > amax3) {
	LOGGER(ibis::gVerbose >= 0)
	    << "print3DDistribution(" << tbl.name() << ", " << col1 << ", "
	    << col2 << ", " << col3 << ", "<< (cond != 0 ? cond : "")
	    << ") can not proceed due to failure to determine min/max values";
	return;
    }

    double stride1, stride2, stride3;
    if (amin1 >= amax1) {
	stride1 = 1.0;
    }
    else if (cptr1->isFloat()) {
	stride1 = (amax1 - amin1) / NB1;
	stride1 = ibis::util::compactValue2(stride1, stride1*(1.0+0.75/NB1));
    }
    else {
	stride1 = ibis::util::compactValue2((amax1 - amin1) / NB1,
					    (amax1 + 1 - amin1) / NB1);
    }
    if (amin2 >= amax2) {
	stride2 = 1.0;
    }
    else if (cptr2->isFloat()) {
	stride2 = (amax2 - amin2) / NB1;
	stride2 = ibis::util::compactValue2(stride2, stride2*(1.0+0.75/NB1));
    }
    else {
	stride2 = ibis::util::compactValue2((amax2 - amin2) / NB1,
					    (amax2 + 1 - amin2) / NB1);
    }
    if (amin3 >= amax3) {
	stride3 = 1.0;
    }
    else if (cptr3->isFloat()) {
	stride3 = (amax3 - amin3) / NB1;
	stride3 = ibis::util::compactValue2(stride3, stride3*(1.0+0.75/NB1));
    }
    else {
	stride3 = ibis::util::compactValue2((amax3 - amin3) / NB1,
					    (amax3 + 1 - amin3) / NB1);
    }
    long ierr;
    std::vector<uint32_t> cnts;
    ierr = tbl.get3DDistribution(cond,
				 col1, amin1, amax1, stride1,
				 col2, amin2, amax2, stride2,
				 col3, amin3, amax3, stride3,
				 cnts);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == cnts.size()) {
	const uint32_t nb23 = NB1 * NB1;
	ibis::util::logger lg;
	lg() << "\n3D-Joint distribution of " << col1 << ", " << col2
	     << ", and " << col3 << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << cnts.size() << " bin"
	     << (cnts.size() > 1 ? "s" : "") << " on " << NB1
	     << " x " << NB1 << " x " << NB1 << " cells\n";

	uint32_t cnt = 0, tot = 0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		const uint32_t i1 = i / nb23;
		const uint32_t i2 = (i - i1 * nb23) / NB1;
		const uint32_t i3 = i % NB1;
		lg() << i << "\t[" << amin1+stride1*i1 << ", "
		     << amin1+stride1*(i1+1)
		     << ") [" << amin2+stride2*i2 << ", "
		     << amin2+stride2*(i2+1)
		     << ") [" << amin3+stride3*i3 << ", "
		     << amin3+stride3*(i3+1)
		     << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total count = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "part[" << tbl.name()
	     << "].get3DDistribution returned with ierr = " << ierr
	     << ", cnts.size() = " << cnts.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
#if defined(TEST_CONTAINER_OF_OBJECTS)
	std::vector<ibis::bitvector> bins;
	ierr = tbl.get3DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     col3, amin3, amax3, stride3,
			     bins);
	ibis::util::logger lg;
	lg() << "\nprint3DDistribution(" << col1 << ", " << col2
	     << ", " << col3 << ") -- \n";
	if (ierr < 0) {
	    lg() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get3DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg() << "get3DDistribution returned " << cnts.size()
		 << " bin" << (cnts.size() > 1 ? "s" : "")
		 << ", but get3DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg() << "matching arrays cnts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	}
#else
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get3DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     col3, amin3, amax3, stride3,
			     bins);
	ibis::util::logger lg;
	lg() << "\nprint3DDistribution(" << col1 << ", " << col2
	     << ", " << col3 << ") -- \n";
	if (ierr < 0) {
	    lg() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get3DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg() << "get3DDistribution returned " << cnts.size()
		 << " bin" << (cnts.size() > 1 ? "s" : "")
		 << ", but get3DBins returned " << bins.size()
		 << " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i] != 0 ? bins[i]->cnt() != cnts[i] : cnts[i] != 0) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << (bins[i]!=0 ? bins[i]->cnt() : 0) << ")\n";
		    ++ ierr;
		}
	    lg() << "matching arrays cnts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	}
	ibis::util::clearVec(bins);
#endif
    }
} // print3DDistribution

/// The version that uses adaptive bins.
static void print3DDist(const ibis::part& tbl, const char *col1,
			const char *col2, const char *col3,
			const char *cond) {
    const uint32_t NB1 = 12;
    std::vector<double> bds1, bds2, bds3;
    std::vector<uint32_t> cnts;
    long ierr;
    if (cond == 0 || *cond == 0)
	ierr = tbl.get3DDistribution(col1, col2, col3, NB1, NB1, NB1,
				     bds1, bds2, bds3, cnts);
    else
	ierr = tbl.get3DDistribution(cond, col1, col2, col3, NB1, NB1, NB1,
				     bds1, bds2, bds3, cnts);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == cnts.size()) {
	const uint32_t nbin2 = bds2.size() - 1;
	const uint32_t nbin3 = bds3.size() - 1;
	const uint32_t nb23 = nbin2 * nbin3;
	ibis::util::logger lg;
	lg() << "\n3D-Joint distribution of " << col1 << ", " << col2
	     << ", and " << col3 << " from table " << tbl.name();
	if (cond && *cond)
	    lg() << " subject to the condition " << cond;
	lg() << " with " << cnts.size() << " bin"
	     << (cnts.size() > 1 ? "s" : "") << " on " << bds1.size()-1
	     << " x " << nbin2 << " x " << nbin3 << " cells\n";

	uint32_t cnt = 0, tot = 0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		const uint32_t i1 = i / nb23;
		const uint32_t i2 = (i - i1 * nb23) / nbin3;
		const uint32_t i3 = i % nbin3;
		lg() << i << "\t[" << bds1[i1] << ", " << bds1[i1+1]
		     << ") [" << bds2[i2] << ", " << bds2[i2+1]
		     << ") [" << bds3[i3] << ", " << bds3[i3+1]
		     << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg() << "  Number of occupied cells = " << cnt
	     << ", total count = " << tot << ", number of rows in "
	     << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg;
	lg() << "part[" << tbl.name()
	     << "].get3DDistribution returned with ierr = " << ierr
	     << ", bds1.size() = " << bds1.size() << ", bds2.size() = "
	     << bds2.size() << ", bds3.size() = " << bds3.size()
	     << ", cnts.size() = " << cnts.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<ibis::bitvector> bins;
	std::vector<double> bdt1, bdt2, bdt3;
	ierr = tbl.get3DBins(cond, col1, col2, col3, NB1, NB1, NB1,
			     bdt1, bdt2, bdt3, bins);
	ibis::util::logger lg;
	lg() << "\nprint3DDistribution(" << col1 << ", " << col2
	     << ", " << col3 << ") -- \n";
	if (ierr < 0) {
	    lg() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg() << "get3DBins returned " << ierr
		 << ", but bins.size() is " << bins.size()
		 << "; these two values are expected to be the same";
	}
	else if (bds1.size() != bdt1.size() || bds2.size() != bdt2.size() ||
		 bds3.size() != bdt3.size() || cnts.size() != bins.size()) {
	    lg() << "get3DDistribution returned a " << bds1.size()-1
		 << " x " << bds2.size()-1 << " x " << bds3.size()-1
		 << " 3D mesh with " << cnts.size() << " element"
		 << (cnts.size() > 1 ? "s" : "")
		 << ", but get3DBins returned a " << bdt1.size()-1
		 << " x " << bdt2.size()-1 << " x " << bdt3.size()-1
		 << " 3D mesh with " << bins.size() << " element"
		 << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < bds1.size(); ++ i)
		if (bds1[i] != bdt1[i]) {
		    lg() << "bds1[" << i << "] (" << bds1[i]
			 << ") != bdt1[" << i << "] (" << bdt1[i]
			 << ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < bds2.size(); ++ i)
		if (bds2[i] != bdt2[i]) {
		    lg() << "bds2[" << i << "] (" << bds2[i]
			 << ") != bdt2[" << i << "] (" << bdt2[i]
			 << ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < bds3.size(); ++ i)
		if (bds3[i] != bdt3[i]) {
		    lg() << "bds3[" << i << "] (" << bds3[i]
			 << ") != bdt3[" << i << "] (" << bdt3[i]
			 << ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg() << "cnts[" << i << "] (" << cnts[i]
			 << ") != bins[" << i << "].cnt() ("
			 << bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg() << "matching arrays cnts and bins produces "
		 << ierr << " error" << (ierr > 1 ? "s" : "");
	    if (ierr > 0)
		lg() << "\nNOTE: due to the different numbers of "
		    "internal bins used for the adaptive histograms, "
		    "get3DDistribution and get3DBins may not produce "
		    "exactly the same answers";
	}
    }
} // print3DDist

// print some helpful information
static void print(const char* cmd) {
    if (cmd == 0 || *cmd == 0) return;
    LOGGER(ibis::gVerbose > 3) << "\nprint(" << cmd << ") -- ...";

    const char* names = cmd;
    if (strnicmp(cmd, "print ", 6) == 0)
	names += 6;
    while (*names && isspace(*names))
	++ names;
    const char *cond = strchr(names, ':');
    if (cond > names) {
	*const_cast<char*>(cond) = 0; // add a null terminator
	// skip to the next non-space character
	for (++ cond; *cond != 0 && isspace(*cond); ++ cond);
    }
    if (strnicmp(names, "joint ", 6) == 0) {
	names += 6;
	std::string name1, name2, name3;
	int ierr = ibis::util::readString(name1, names);
	if (ierr < 0 || name1.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print joint' needs two "
		"column names as arguments";
	    return;
	}
	ierr = ibis::util::readString(name2, names);
	if (ierr < 0 || name2.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print joint' needs two "
		"column names as arguments";
	    return;
	}
	ierr = ibis::util::readString(name3, names);
	if (ierr < 0 || name3.empty()) { // got two names, 2D distributions
	    for (ibis::partList::const_iterator tit = ibis::datasets.begin();
		 tit != ibis::datasets.end(); ++ tit) {
		print2DDistribution(**tit, name1.c_str(), name2.c_str(), cond);
		if (ibis::gVerbose > 6)
		    print2DDist(**tit, name1.c_str(), name2.c_str(), cond);
		if (ibis::gVerbose > 9)
		    printJointDistribution(**tit, name1.c_str(),
					   name2.c_str(), cond);
	    }
	}
	else {
	    for (ibis::partList::const_iterator tit = ibis::datasets.begin();
		 tit != ibis::datasets.end(); ++ tit) {
		print3DDistribution(**tit, name1.c_str(),
				    name2.c_str(), name3.c_str(), cond);
		if (ibis::gVerbose > 6)
		    print3DDist(**tit, name1.c_str(),
				name2.c_str(), name3.c_str(), cond);
	    }
	}
    }
    else if (strnicmp(names, "weighted", 8) == 0) {
	names += 8;
	std::string nm1, nm2, nm3, nm4;
	int ierr = ibis::util::readString(nm1, names);
	if (ierr < 0 || nm1.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print weighted' needs at least two names "
		"as arguments";
	    return;
	}
	ierr = ibis::util::readString(nm2, names);
	if (ierr < 0 || nm2.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print weighted' needs at least two names "
		"as arguments";
	    return;
	}
	ierr = ibis::util::readString(nm3, names);
	if (ierr < 0 || nm3.empty()) {
	    for (ibis::partList::const_iterator tit = ibis::datasets.begin();
		 tit != ibis::datasets.end(); ++ tit) {
		print1DDistribution(**tit, cond, nm1.c_str(), nm2.c_str());
	    }
	    return;
	}
	ierr = ibis::util::readString(nm4, names);
	if (ierr < 0 || nm4.empty()) {
	    for (ibis::partList::const_iterator tit = ibis::datasets.begin();
		 tit != ibis::datasets.end(); ++ tit) {
		print2DDistribution(**tit, cond, nm1.c_str(),
				    nm2.c_str(), nm3.c_str());
	    }
	}
	else {
	    for (ibis::partList::const_iterator tit = ibis::datasets.begin();
		 tit != ibis::datasets.end(); ++ tit) {
		print3DDistribution(**tit, cond, nm1.c_str(),
				    nm2.c_str(), nm3.c_str(), nm4.c_str());
	    }
	}
    }
    else if (names) { // there are arguments after the print command
	ibis::nameList nlist(names); // split using the space as delimiter
	for (ibis::nameList::const_iterator it = nlist.begin();
	     it != nlist.end(); ++it) { // go through each name
	    ibis::partList::const_iterator tit = ibis::datasets.begin();
	    for (; tit != ibis::datasets.end() && 
		     stricmp((*tit)->name(), *it) != 0 &&
		     ibis::util::strMatch((*tit)->name(), *it) == false;
		 ++ tit);
	    if (tit != ibis::datasets.end()) { // it's a data partition
		ibis::util::logger lg;
		lg() << "Partition " << (*tit)->name() << ":\n";
		(*tit)->print(lg());
	    }
	    else if ((*it)[0] == '*') {
		printAll();
	    }
	    else if (stricmp(*it, "parts") == 0) {
		ibis::util::logger lg;
		lg() << "Name(s) of all data partitioins\n";
		for (tit = ibis::datasets.begin();
		     tit != ibis::datasets.end(); ++tit)
		    lg() << (*tit)->name() << ' ';
	    }
	    else if (stricmp(*it, "names") == 0 ||
		     stricmp(*it, "columns") == 0) {
		printNames();
	    }
	    else if (stricmp(*it, "distributions") == 0) {
		printDistribution();
	    }
	    else { // assume it to be a column name
		for (tit = ibis::datasets.begin();
		     tit != ibis::datasets.end(); ++tit) {
		    printColumn(**tit, *it, cond);
		    if (ibis::gVerbose > 9)
			printColumn0(**tit, *it, cond);
		}
	    }
	}
    }
    else {
	ibis::util::logger lg;
	lg() << "Name(s) of all partitions\n";
	for (ibis::partList::const_iterator tit = ibis::datasets.begin();
	     tit != ibis::datasets.end(); ++tit)
	    lg() << (*tit)->name() << ' ';
    }
} // print

// Read SQL query statements terminated with semicolon (;).
static void readQueryFile(const char *fname, std::vector<std::string> &queff) {
    std::ifstream qfile(fname);
    if (! qfile) {
	ibis::util::logMessage("readQueryFile", "unable to open file \"%s\"",
			       fname);
	return;
    }

    char buf[MAX_LINE];
    std::string qtemp;
    while (qfile.getline(buf, MAX_LINE)) {
	if (*buf != 0 || *buf != '#') { // line started with # is a comment
	    char *ch = buf;
	    while (*ch != 0 && isspace(*ch)) ++ ch; // skip leading space
	    if (ch != buf || ! qtemp.empty())
		qtemp += ' '; // add a space

	    while (*ch != 0) {
		if (*ch == ';') { // terminating a SQL statement
		    if (! qtemp.empty()) {
			bool onlyspace = true;
			for (unsigned i = 0; onlyspace && i < qtemp.size();
			     ++ i)
			    onlyspace = (isspace(qtemp[i]) != 0);
			if (! onlyspace) {
			    queff.push_back(qtemp);
			}
		    }
		    qtemp.clear();
		    ++ ch;
		}
		else if (*ch == '-' && ch[1] == '-') {
		    *ch = 0; // ignore the rest of the line
		}
		else {
		    qtemp += *ch;
		    ++ ch;
		}
	    }
	}
    }
    if (! qtemp.empty()) {
	bool onlyspace = true;
	for (unsigned i = 0; onlyspace && i < qtemp.size(); ++ i)
	    onlyspace = (isspace(qtemp[i]) != 0);
	if (! onlyspace) {
	    queff.push_back(qtemp);
	}
    }
} // readQueryFile

// function to parse the command line arguments
static void parse_args(int argc, char** argv, int& mode,
		       std::vector<const char*>& alist,
		       std::vector<const char*>& slist,
		       std::vector<const char*>& qlist,
		       std::vector<std::string>& queff,
		       ibis::joinlist& joins) {
#if defined(DEBUG) || defined(_DEBUG)
#if DEBUG + 0 > 10 || _DEBUG + 0 > 10
    ibis::gVerbose = INT_MAX;
#elif DEBUG + 0 > 0
    ibis::gVerbose += 7 * DEBUG;
#elif _DEBUG + 0 > 0
    ibis::gVerbose += 5 * _DEBUG;
#else
    ibis::gVerbose += 3;
#endif
#endif

    mode = -1;
    alist.clear(); // list of append operations
    qlist.clear(); // list of query strings
    slist.clear(); // list of sort request

    int accessIndexInWhole = 0;
    std::vector<const char*> confs; // name of the configuration files
    std::vector<const char*> dirs;  // directories specified on command line
    std::vector<const char*> rdirs; // directories to be reordered
    std::vector<const char*> printcmds; // printing commands
    const char* mesgfile = 0;
    for (int i=1; i<argc; ++i) {
	if (*argv[i] == '-') { // normal arguments starting with -
	    switch (argv[i][1]) {
	    case 'a': // append a directory of data (must have a directory
	    case 'A': // name, optionally specify data partition name with "to
		      // name", where the name can be a data partition name
		      // or a directory name)
		if (i+1 < argc) {
		    alist.push_back(argv[i+1]);
		    if (i+3 < argc && stricmp(argv[i+2], "to")==0 &&
			argv[i+3][0] != '-') {
			appendto = argv[i+3];
			i += 3;
		    }
		    else if (i+2 < argc && argv[i+2][0] != '-') {
			appendto = argv[i+2];
			i += 2;
		    }
		    else {
			++ i;
		    }
		}
	    break;
	    case 'b':
	    case 'B': { // build indexes,
		// if this argument is followed by an integer, the integer
		// is taken to be the number of threads to use for building
		// indexes; if this argument is followed by anything else,
		// it is assumed to be an index speicification.
		char *ptr = strchr(argv[i], '=');
		if (ptr == 0) {
		    if (i+1 < argc) {
			if (isdigit(*argv[i+1])) {
			    build_index += atoi(argv[i+1]);
			    i = i + 1;
			}
			else {
			    ++ build_index;
			    if (*argv[i+1] != '-') {
				// assume to be an index specification
				indexingOption = argv[i+1];
				i = i + 1;
			    }
			}
		    }
		    else {
			++ build_index;
		    }
		}
		else {
		    build_index += atoi(++ptr);
		    if (i+1 < argc && *argv[i+1] != '-') {
			// assume to be an index specification
			indexingOption = argv[i+1];
			i = i + 1;
		    }
		}
		break;}
	    case 'c':
	    case 'C': // configuration file, multiple files allowed
		if (i+1 < argc) {
		    confs.push_back(argv[i+1]);
		    ++ i;
		}
	    break;
	    case 'd':
	    case 'D': // data directory, multiple directory allowed
		if (i+1 < argc && argv[i+1][0] != '-') {
		    dirs.push_back(argv[i+1]);
		    i = i + 1;
		}
		else {
		    std::clog << "Warning -- argument -d must be followed by "
			      << "a directory name" << std::endl;
		}
	    break;
	    case 'e':
	    case 'E': // estiamtion option
		estimate_opt += 1;
	    break;
	    case 'f':
	    case 'F': // query file, multiple files allowed
		if (i+1 < argc) {
		    readQueryFile(argv[i+1], queff);
		    ++ i;
		}
	    break;
	    default:
	    case 'h':
	    case 'H': // print usage
		usage(*argv);
	    if (argc <= 2)
		exit(0);
	    break;
	    case 'i':
	    case 'I': // interactive mode
		mode = 1;
	    break;
	    case 'j':
	    case 'J': {// join part1 part2 join-column constraints1 constratint2
		ibis::joinspec js;
		if (i+3 < argc) {
		    js.part1 = argv[i+1];
		    js.part2 = argv[i+2];
		    js.jcol  = argv[i+3];
		    i += 3;
		}
		if (i+1 < argc && *argv[i+1] != '-') {
		    ++ i;
		    if (*argv[i] != '*' && *argv[i] != 0 && !isspace(*argv[i]))
			js.cond1 = argv[i];
		}
		if (i+1 < argc && *argv[i+1] != '-') {
		    ++ i;
		    if (*argv[i] != '*' && *argv[i] != 0 && !isspace(*argv[i]))
			js.cond2 = argv[i];
		}
		while (i+1 < argc && *argv[i+1] != '-') {
		    ++ i;
		    if (js.selcol.empty()) {
			js.selcol = argv[i];
		    }
		    else {
			js.selcol += ", ";
			js.selcol += argv[i];
		    }
		}
		if (js.part1 != 0 && js.part2 != 0 && js.jcol != 0) {
		    joins.push_back(new ibis::joinspec(js));
		}
		else {
		    LOGGER(1) << *argv << " -j option did not specify a "
			"complete join operation, discard it.\nUsage\n\t-j "
			"part1 part2 join-column conditions1 conditions2 "
			"[columns ...]\n\nNote: Table care not to have any "
			"of the strings start with -";
		}
		break;}
	    case 'k':
	    case 'K': // keep temporary query files or reverse -y
		if (i+1 < argc && *argv[i+1] != '-') { // reverse -y
		    keepstring = argv[i+1];
		    i = i + 1;
		}
		else { // keep temporary files
		    ibis::query::keepQueryRecords();
		}
	    break;
	    case 'l':
	    case 'L': // logfile or load index in one-shot
		if (i+1 < argc && argv[i+1][0] != '-') {
		    mesgfile = argv[i+1];
		    ++ i;
		}
		else if ((argv[i][2] == 'o' || argv[i][2] == 'O') &&
			 (argv[i][3] == 'g' || argv[i][3] == 'G')) {
		    mesgfile = 0; // reset the log file to stdout
		}
		else {
		    accessIndexInWhole = 1;
		}
	    break;
#if defined(TEST_SUMBINS_OPTIONS)
	    case 'm':
	    case 'M': {// _sumBins_option
		char* ptr = strchr(argv[i], '=');
		if (ptr != 0) {
		    ++ ptr; // skip '='
		    ibis::_sumBins_option = atoi(ptr);
		}
		else if (i+1 < argc) {
		    if (isdigit(*argv[i+1])) {
			ibis::_sumBins_option = atoi(argv[i+1]);
			i = i + 1;
		    }
		}
		break;}
#endif
	    case 'n':
	    case 'N': {
		// skip estimation, directly call function evaluate
		estimate_opt = -1;
		break;}
	    case 'o':
	    case 'O':
		if (argv[i][2] == 'n' || argv[i][2] == 'N') {
		    // skip estimation, directly call function evaluate
		    estimate_opt = -1;
		}
		else if (i+1 < argc && argv[i+1][0] != '-') {
		    // output file specified
		    if (! outputnamestoo)
			outputnamestoo =
			    (0 == strnicmp(argv[i]+2, "utput-", 6));
		    outputfile = argv[i+1];
		    i = i + 1;
		}
	    break;
	    case 'p':
	    case 'P': // collect the print options
		if (i+1 < argc) {
		    if (argv[i+1][0] != '-') {
			printcmds.push_back(argv[i+1]);
			++ i;
		    }
		    else if (printcmds.empty()) {
			printcmds.push_back("parts");
		    }
		}
		else  if (printcmds.empty()) { // at least print partition names
		    printcmds.push_back("parts");
		}
	    break;
	    case 'q':
	    case 'Q': // specify a query "[select ...] [from ...] where ..."
		if (i+1 < argc) {
		    qlist.push_back(argv[i+1]);
		    ++ i;
		}
	    break;
	    case 'r':
	    case 'R': // RID/result check or reorder
		if (argv[i][2] == 'i' || argv[i][2] == 'I') { // rid
		    verify_rid = true;
		    if (i+1 < argc) { // there is one more argument
			if (argv[i+1][0] != '-') { // assume to be a file name
			    ridfile = argv[i+1];
			    ++ i;
			}
		    }
		}
		else if (i+1 < argc && argv[i+1][0] != '-') { // reorder
		    rdirs.push_back(argv[i+1]);
		    ++ i;
		}
		else { // rid
		    verify_rid = true;
		}
	    break;
	    case 's':
	    case 'S': // sequential scan, or scan option
#if defined(TEST_SCAN_OPTIONS)
		if (i+1 < argc) {
		    if (isdigit(*argv[i+1])) {
			ibis::_scan_option = atoi(argv[i+1]);
			i = i + 1;
		    }
		    else if (isalpha(*argv[i+1])) {
			slist.push_back(argv[i+1]);
			i = i + 1;
		    }
		    else {
			sequential_scan = true;
		    }
		}
		else {
		    sequential_scan = true;
		}
#else
	    if (i+1 < argc) {
		if (isalpha(*argv[i+1])) {
		    slist.push_back(argv[i+1]);
		    i = i + 1;
		}
		else {
		    sequential_scan = true;
		}
	    }
	    else {
		sequential_scan = true;
	    }
#endif
	    break;
	    case 't':
	    case 'T': { // self-testing mode or number of threads
		bool thr = (argv[i][2] == 'h' || argv[i][2] == 'H'); // thread
		char *ptr = strchr(argv[i], '=');
		if (ptr == 0) {
		    if (i+1 < argc) {
			if (isdigit(*argv[i+1])) {
			    if (thr)
				threading += atoi(argv[i+1]);
			    else
				testing += atoi(argv[i+1]);
			    i = i + 1;
			}
			else if (thr) {
			    ++ threading;
			}
			else {
			    ++ testing;
			}
		    }
		    else if (thr) {
			++ threading;
		    }
		    else {
			++ testing;
		    }
		}
		else if (thr) { // override previous values
		    threading = atoi(++ptr);
		}
		else { // override previous values
		    testing = atoi(++ptr);
		}
		break;}
	    case 'v':
	    case 'V': { // verboseness
		char *ptr = strchr(argv[i], '=');
		if (ptr == 0) {
		    if (i+1 < argc) {
			if (isdigit(*argv[i+1])) {
			    ibis::gVerbose += atoi(argv[i+1]);
			    i = i + 1;
			}
			else {
			    ++ ibis::gVerbose;
			}
		    }
		    else {
			++ ibis::gVerbose;
		    }
		}
		else { // override previous values
		    ibis::gVerbose = atoi(++ptr);
		}
		break;}
	    case 'y':
	    case 'Y': // yank some rows of every data partition available
		// must have an argument after the flag to indicate a file
		// containing row numbers or a string indicate conditions
		// on rows to mark as inactive/junk
		if (i+1 < argc) {
		    yankstring = argv[i+1];
		    i = i + 1;
		}
	    break;
	    case 'z':
	    case 'Z': {
		zapping = true;
		break;}
	    } // switch (argv[i][1])
	} // normal arguments
	else { // argument not started with '-' and not following
	       // apropriate '-' operations are assumed to be names of the
	       // data directories and are read one at a time
	    dirs.push_back(argv[i]);
	}
    } // for (inti=1; ...)

    for (unsigned i = 0; i < queff.size(); ++ i) {
	qlist.push_back(queff[i].c_str());
    }
    if (mode < 0) {
	mode = (qlist.empty() && testing <= 0 && build_index <= 0 &&
		alist.empty() && slist.empty() && printcmds.empty() &&
		rdirs.empty() && joins.empty() &&
		yankstring == 0 && keepstring == 0);
    }
    if (qlist.size() > 1U && threading == 0) {
	if (testing > 0) {
	    threading = testing;
	    testing = 0;
	}
	else {
#if defined(_SC_NPROCESSORS_ONLN)
	    threading = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_WIN32) && defined(_MSC_VER)
	    SYSTEM_INFO myinfo;
	    GetSystemInfo(&myinfo);
	    threading = myinfo.dwNumberOfProcessors;
#else
	    threading = 0;
#endif
	}
    }
    if (mesgfile != 0 && *mesgfile != 0) {
	int ierr = ibis::util::setLogFileName(mesgfile);
	if (ierr < 0)
	    std::clog << *argv << " failed to open file " << mesgfile
		      << " for logging error messages" << std::endl;
	else if (ibis::gVerbose > 2)
	    std::clog << *argv << " will write messages to " << mesgfile
		      << std::endl;
    }
    if (ibis::gVerbose > 1) {
	ibis::util::logger lg;
	lg() << "\n" << *argv;
	if (ibis::gVerbose > 5) {
	    for (int i = 1; i < argc; ++ i)
		lg() << ' ' << argv[i];
	    lg() << "\n";
	}
	lg() << "\nOptions summary: "
	     << (mode ? "interactive mode" : "batch mode")
	     << ", log level " << ibis::gVerbose;
	if (build_index > 0) {
	    lg() << ", building indexes";
	    if (zapping)
		lg() << " (remove any existing indexes)";
	}
	if (testing > 0)
	    lg() << ", testing " << testing;
	if (threading > 0)
	    lg() << ", threading " << threading;
	if (estimate_opt < 0)
	    lg() << ", skipping estimation";
	else if (estimate_opt > 0)
	    lg() << ", computing only bounds";
	else
	    lg() << ", with estimation";
	if (! alist.empty()) {
	    lg() << "\nappending data in the following director"
		 << (alist.size()>1 ? "ies" : "y");
	    if (appendto)
		lg() << " to " << appendto;
	    for (uint32_t i = 0; i < alist.size(); ++ i)
		lg() << "\n" << alist[i];
	}
	lg() << "\n";
    }
    if (confs.size() > 1) {
	// read all configuration files except the last one
	for (uint32_t i = 0; i < confs.size()-1; ++ i)
	    ibis::gParameters().read(confs[i]);
    }
    if (accessIndexInWhole > 0) {
	ibis::gParameters().add("all.preferMMapIndex", "T");
    }
    // process data directories specified in the rc files
    ibis::init(confs.empty()?(const char*)0:confs.back());

    // reorder the data directories first, a data directory may be followed
    // by ':' and column names
    for (unsigned i = 0; i < rdirs.size(); ++ i) {
	long ierr = 0;
	LOGGER(ibis::gVerbose >= 0)
	    << *argv << " -reorder " << rdirs[i];
	char* str = const_cast<char*>(strrchr(rdirs[i], ':'));
	if (str != 0 && str > rdirs[i] && str[1] != '/' && str[1] != '\\') {
	    std::string dir;
	    for (const char* tmp = rdirs[i]; tmp < str; ++ tmp)
		dir += *tmp;
	    str = ibis::util::strnewdup(str+1);
	    ibis::table::stringList slist;
	    ibis::table::parseNames(str, slist);
	    uint32_t nr = 0;
	    {
		ibis::part tbl(dir.c_str(), static_cast<const char*>(0));
		ierr = tbl.reorder(slist);
		nr = tbl.nRows();
	    }
	    delete [] str;
	    if ((long)nr == ierr && nr > 0U)
		ibis::util::gatherParts(ibis::datasets, dir.c_str());
	}
	else {
	    uint32_t nr = 0;
	    {
		ibis::part tbl(rdirs[i], static_cast<const char*>(0));
		ierr = tbl.reorder();
		nr = tbl.nRows();
	    }
	    if (nr > 0U && (long)nr == ierr)
		ibis::util::gatherParts(ibis::datasets, rdirs[i]);
	}
    }

    // construct the paritions using both the command line arguments and
    // the resource files
    for (std::vector<const char*>::const_iterator it = dirs.begin();
	 it != dirs.end(); ++ it) {
	ibis::util::gatherParts(ibis::datasets, *it);
    }

    if (ibis::gVerbose > 1) {
	ibis::util::logger lg;
	if (ibis::datasets.size()) {
	    lg() << "Partition" << (ibis::datasets.size()>1 ? "s" : "")
		 << "[" << ibis::datasets.size() << "]:\n";
	    for (ibis::partList::const_iterator it = ibis::datasets.begin();
		 it != ibis::datasets.end(); ++it)
		lg() << "  " << (*it)->name() << "\n";
	}
	if (qlist.size() > 0) {
	    lg() << "Quer" << (qlist.size()>1 ? "ies" : "y")
		 << "[" << qlist.size() << "]:\n";
	    for (std::vector<const char*>::const_iterator it = qlist.begin();
		 it != qlist.end(); ++it)
		lg() << "  " << *it << "\n";
	}
	if (joins.size() > 0) {
	    lg() << "Join" << (joins.size() > 1 ? "s" : "")
		 << "[" << joins.size() << "]:\n";
	    for (size_t j = 0; j < joins.size(); ++ j) {
		lg() << "  ";
		joins[j]->print(lg());
		lg() << "\n";
	    }
	}
    }

    if (ibis::gVerbose > 1 &&
	(testing > 1 || build_index > 0 || ! printcmds.empty())) {
	for (ibis::partList::const_iterator it = ibis::datasets.begin();
	     it != ibis::datasets.end(); ++it) {
	    bool recompute = (testing>5 && ibis::gVerbose>7);
	    // check to see if the nominal min and max are different
	    ibis::part::info *info = (*it)->getInfo();
	    for (uint32_t i = 0; i < info->cols.size() && ! recompute; ++i)
		recompute = (info->cols[i]->type != ibis::CATEGORY &&
			     info->cols[i]->type != ibis::TEXT &&
			     info->cols[i]->expectedMin >
			     info->cols[i]->expectedMax);
	    delete info;   // no use for it any more
	    if (recompute) {// acutally compute the min and max of attributes
		LOGGER(ibis::gVerbose > 1)
		    << *argv << ": recomputing the min/max for partition "
		    << (*it)->name();
		(*it)->computeMinMax();
	    }
	}
    }
    for (std::vector<const char*>::const_iterator it = printcmds.begin();
	 it != printcmds.end(); ++ it) {
	print(*it);
    }
} // parse_args

// This print function takes the most general option in getting the values
// out of a query.  All supported FastBit types can be retrieved as
// strings, therefore, it is always fine to use getString to retrieve a
// value.  However, if the values in the select clause are of known type,
// those types should be used instead of @c getString.
static void printQueryResults(std::ostream &out, ibis::query &q) {
    ibis::query::result cursor(q);
    const unsigned w = cursor.width();
    out << "printing results of query " << q.id() << " (numHits="
	<< q.getNumHits() << ")\n";
    cursor.printColumnNames(out);
    out << "\n";
    if (w == 0) return;

    while (cursor.next()) {
	out << cursor.getString(static_cast<uint32_t>(0U));
	for (uint32_t i = 1; i < w; ++ i)
	    out << ", " << cursor.getString(i);
	out << "\n";
    }
} // printQueryResults

// evaluate a single query -- directly retrieve values of selected columns
static void xdoQuery(ibis::part* tbl, const char* uid, const char* wstr,
		     const char* sstr) {
    LOGGER(ibis::gVerbose > 0)
	<< "xdoQuery -- processing query " << wstr
	<< " on partition " << tbl->name();

    ibis::query aQuery(uid, tbl); // in case of exception, content of query
				  // will be automatically freed
    long num1, num2;
    aQuery.setWhereClause(wstr);
    if (aQuery.getWhereClause() == 0)
	return;
    if (zapping) {
	std::string old = aQuery.getWhereClause();
	std::string comp = aQuery.removeComplexConditions();
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg;
	    if (! comp.empty())
		lg() << "xdoQuery -- the WHERE clause \"" << old.c_str()
		     << "\" is split into \"" << comp.c_str()
		     << "\" AND \"" << aQuery.getWhereClause() << "\"";
	    else
		lg() << "xdoQuery -- the WHERE clause \""
		     << aQuery.getWhereClause()
		     << "\" is considered simple";
	}
    }
    const char* asstr = 0;
    if (sstr != 0) {
	aQuery.setSelectClause(sstr);
	asstr = aQuery.getSelectClause();
    }

    if (estimate_opt >= 0) {
	num2 = aQuery.estimate();
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "xdoQuery -- failed to estimate \"" << wstr
		<< "\", error code = " << num2;
	    return;
	}
	num1 = aQuery.getMinNumHits();
	num2 = aQuery.getMaxNumHits();
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg() << "xdoQuery -- the number of hits is ";
	    if (num2 > num1) 
		lg() << "between " << num1 << " and ";
	    lg() << num2;
	}
	if (estimate_opt > 0)
	    return;
    }

    num2 = aQuery.evaluate();
    if (num2 < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "xdoQuery -- failed to evaluate \"" << wstr
	    << "\", error code = " << num2;
	return;
    }
    num1 = aQuery.getNumHits();
    LOGGER(ibis::gVerbose > 0) << "xdoQuery -- the number of hits = " << num1;

    if (asstr != 0 && *asstr != 0 && num1 > 0) {
	if (outputfile != 0 && *outputfile != 0) {
	    std::ofstream output(outputfile,
				 std::ios::out |
				 (appendToOutput ? std::ios::app :
				  std::ios::trunc));
	    if (output) {
		LOGGER(ibis::gVerbose >= 0)
		    << "xdoQuery -- query (" <<  aQuery.getWhereClause()
		    << ") results written to file \"" <<  outputfile << "\"";
		printQueryResults(output, aQuery);
	    }
	    else {
		ibis::util::logger lg;
		lg() << "Warning ** xdoQuery failed to open \""
		     << outputfile << "\" for writing query ("
		     << aQuery.getWhereClause() << ")";
		printQueryResults(lg(), aQuery);
	    }
	}
	else {
	    ibis::util::logger lg;
	    printQueryResults(lg(), aQuery);
	}
    } // if (asstr != 0 && num1 > 0)
} // xdoQuery

template<typename T>
void findMissingValuesT(const ibis::column &col,
			const ibis::bitvector &ht0,
			const ibis::bitvector &ht1) {
    ibis::array_t<T> vals0, vals1;
    long ierr = col.selectValues(ht0, &vals0);
    if (ierr <= 0 || static_cast<long unsigned>(ierr) < ht0.cnt()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- findMissingValues did not receive the expected "
	    "number of values for query 0, expected " << ht0.cnt()
	    << ", received " << ierr;
	return;
    }
    ierr = col.selectValues(ht1, &vals1);
    if (ierr <= 0 || static_cast<long unsigned>(ierr) < ht1.cnt()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- findMissingValues did not receive the expected "
	    "number of values for query 1, expected " << ht1.cnt()
	    << ", received " << ierr;
	return;
    }

    std::sort(vals0.begin(), vals0.end());
    std::sort(vals1.begin(), vals1.end());
    size_t j0 = 0;
    size_t j1 = 0;
    const int prec = 1 + 2*sizeof(T);
    const size_t n0 = vals0.size();
    const size_t n1 = vals1.size();
    while (j0 < n0 && j1 < n1) {
	while (j0 < n0 && vals0[j0] < vals1[j1]) {
	    size_t cnt = 1;
	    const T tgt = vals0[j0];
	    for (++ j0; j0 < n0 && vals0[j0] == tgt; ++ j0, ++ cnt);
	    LOGGER(ibis::gVerbose >= 0)
		<< "  " << std::setprecision(prec) << tgt << " appeared " << cnt
		<< " times in query 0, but not in query 1";
	}
	while (j0 < n0 && j1 < n1 && vals1[j1] < vals0[j0]) {
	    size_t cnt = 1;
	    const T tgt = vals1[j1];
	    for (++ j1; j1 < n1 && vals1[j1] == tgt; ++ j1, ++ cnt);
	    LOGGER(ibis::gVerbose >= 0)
		<< "  " << std::setprecision(prec) << tgt << " appeared " << cnt
		<< " times in query 1, but not in query 0";
	}
	while (j0 < n0 && j1 < n1 && vals0[j0] == vals1[j1]) {
	    const T tgt = vals0[j0];
	    size_t cnt0 = 1, cnt1 = 1;
	    for (++ j0; j0 < n0 && vals0[j0] == tgt; ++ j0, ++ cnt0);
	    for (++ j1; j1 < n1 && vals1[j1] == tgt; ++ j1, ++ cnt1);
	    LOGGER(ibis::gVerbose >= 0 && cnt1 < cnt0)
		<< "  " << std::setprecision(prec) << tgt << " appeared "
		<< cnt1 << " times in query 1, but appeared " << cnt0
		<< " times in query 0";
	}
    }

    while (j0 < n0) {
	size_t cnt = 1;
	const T tgt = vals0[j0];
	for (++ j0; j0 < n0 && vals0[j0] == tgt; ++ j0, ++ cnt);
	LOGGER(ibis::gVerbose >= 0)
	    << "  " << std::setprecision(prec) << tgt << " appeared " << cnt
	    << " times in query 0, but not in query 1";
    }
    while (j1 < n1) {
	size_t cnt = 1;
	const T tgt = vals1[j1];
	for (++ j1; j1 < n1 && vals1[j1] == tgt; ++ j1, ++ cnt);
	LOGGER(ibis::gVerbose >= 0)
	    << "  " << std::setprecision(prec) << tgt << " appeared " << cnt
	    << " times in query 1, but not in query 0";
    }
} // findMissingValuesT

static void findMissingValues(const ibis::part &pt, const char *cnm,
			      const ibis::bitvector &ht0,
			      const ibis::bitvector &ht1) {
    const ibis::column *col = pt.getColumn(cnm);
    if (col == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- findMissingValues can not procede because " << cnm
	    << " is not a column of data partition " << pt.name();
	return;
    }

    switch (col->type()) {
    default: {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- findMissingValues can not handle column type "
	    << col->type() << '(' << ibis::TYPESTRING[col->type()] << ')';
	break;}
    case ibis::BYTE: {
	findMissingValuesT<char>(*col, ht0, ht1);
	break;}
    case ibis::UBYTE: {
	findMissingValuesT<unsigned char>(*col, ht0, ht1);
	break;}
    case ibis::SHORT: {
	findMissingValuesT<int16_t>(*col, ht0, ht1);
	break;}
    case ibis::USHORT: {
	findMissingValuesT<uint16_t>(*col, ht0, ht1);
	break;}
    case ibis::INT: {
	findMissingValuesT<int32_t>(*col, ht0, ht1);
	break;}
    case ibis::UINT: {
	findMissingValuesT<uint32_t>(*col, ht0, ht1);
	break;}
    case ibis::LONG: {
	findMissingValuesT<int64_t>(*col, ht0, ht1);
	break;}
    case ibis::ULONG: {
	findMissingValuesT<uint64_t>(*col, ht0, ht1);
	break;}
    case ibis::FLOAT: {
	findMissingValuesT<float>(*col, ht0, ht1);
	break;}
    case ibis::DOUBLE: {
	findMissingValuesT<double>(*col, ht0, ht1);
	break;}
    }
} // findMissingValues

// Execute a query using the new ibis::table interface
static void tableSelect(const ibis::partList &pl, const char* uid,
			const char* wstr, const char* sstr,
			const char* ordkeys,
			uint32_t limit, uint32_t start) {
    std::auto_ptr<ibis::table> tbl(ibis::table::create(pl));
    std::string sqlstring; //
    {
	std::ostringstream ostr;
	if (sstr != 0 && *sstr != 0)
	    ostr << "SELECT " << sstr;
	ostr << " FROM " << tbl->name();
	if (wstr != 0 && *wstr != 0) {
	    const int nwstr = strlen(wstr);
	    if (nwstr < 80) {
		ostr << " WHERE " << wstr;
	    }
	    else {
		ostr << " WHERE ";
		int i = 0;
		while (i < 40) {
		    ostr << wstr[i];
		    ++ i;
		}
		while (i < nwstr && isspace(wstr[i]) == 0) {
		    ostr << wstr[i];
		    ++ i;
		}
		if (i+20 < nwstr) {
		    ostr << " ...";
		}
		else {
		    while (i < nwstr) {
			ostr << wstr[i];
			++ i;
		    }
		}
	    }
	}
	if (ordkeys && *ordkeys) {
	    ostr << " ORDER BY " << ordkeys;
	}
	if (limit > 0) {
	    ostr << " LIMIT ";
	    if (start > 0) ostr << start << ", ";
	    ostr << limit;
	}
	sqlstring = ostr.str();
    }
    LOGGER(ibis::gVerbose > 1)
	<< "tableSelect -- processing \"" << sqlstring << '\"';

    ibis::horometer timer;
    timer.start();

    if (estimate_opt >= 0) {
	uint64_t num1, num2;
	tbl->estimate(wstr, num1, num2);
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg() << "tableSelect -- the number of hits is ";
	    if (num2 > num1)
		lg() << "between " << num1 << " and ";
	    lg() << num2;
	}
	if (estimate_opt > 0 || num2 == 0) {
	    if (ibis::gVerbose >= 0) {
		timer.stop();
		ibis::util::logger lg;
		lg() << "tableSelect:: estimate(" << wstr << ") took "
		     << timer.CPUTime() << " CPU seconds, "
		     << timer.realTime() << " elapsed seconds";
	    }
	    return; // stop here is only want to estimate
	}
    }

    std::auto_ptr<ibis::table> sel1(tbl->select(sstr, wstr));
    if (sel1.get() == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "tableSelect:: select(" << sstr << ", " << wstr
	    << ") failed on table " << tbl->name();
	return;
    }

    LOGGER(ibis::gVerbose >= 0)
	<< "tableSelect -- select(" << sstr << ", " << wstr
	<< ") on table " << tbl->name() << " produced a table with "
	<< sel1->nRows() << " row" << (sel1->nRows() > 1 ? "s" : "")
	<< " and " << sel1->nColumns() << " column"
	<< (sel1->nColumns() > 1 ? "s" : "");
    if (sel1->nRows() > 1 && ((ordkeys && *ordkeys) || limit > 0)) {
	// top-K query
	sel1->orderby(ordkeys);
    }

    if (outputfile != 0 && *outputfile != 0) {
	if (limit == 0)
	    limit = static_cast<uint32_t>(sel1->nRows());
	if (0 != strcmp(outputfile, "/dev/null")) {
	    std::ofstream output(outputfile, std::ios::out |
				 (appendToOutput ? std::ios::app :
				  std::ios::trunc));
	    if (outputnamestoo)
		sel1->dumpNames(output, ", ");
	    sel1->dump(output, start, limit, ", ");
	}
    }
    else if (ibis::gVerbose >= 0) {
	ibis::util::logger lg;
	if (limit == 0 && sel1->nColumns() > 0) {
	    limit = (sel1->nRows() >> ibis::gVerbose) > 0 ?
		1 << ibis::gVerbose : static_cast<uint32_t>(sel1->nRows());
	    if (limit > (sel1->nRows() >> 1))
		limit = sel1->nRows();
	}
	if (limit > 0 && limit < sel1->nRows()) {
	    lg() << "tableSelect -- the first ";
	    if (limit > 1)
		lg() << limit << " rows ";
	    else
		lg() << "row ";
	    lg() << "(of " << sel1->nRows()
		 << ") from the result table for \""
		 << sqlstring << "\"\n";
	}
	else {
	    lg() << "tableSelect -- the result table (" << sel1->nRows()
		 << " x " << sel1->nColumns() << ") for \""
		 << sqlstring << "\"\n";
	}
	if (outputnamestoo)
	    sel1->dumpNames(lg(), ", ");
	sel1->dump(lg(), start, limit, ", ");
    }

    if (verify_rid && sel1->nRows() > 1 && sel1->nColumns() > 0) {
	// query the list of values selected by the 1st column
	std::vector<double> svals;
	const ibis::table::stringList cnames = sel1->columnNames();
	int64_t ierr = sel1->getColumnAsDoubles(cnames[0], svals);
	if (ierr < 0 || static_cast<uint64_t>(ierr) != sel1->nRows()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "tableSelect -- can not verify the answers returned for "
		<< sqlstring << ", because of failure to retrieve values "
		"from an intermediate table object named " << sel1->name()
		<< ", ierr = " << ierr;
	}
	else {
	    ibis::qDiscreteRange dr(cnames[0], svals);
	    ibis::query qq0(uid), qq1(uid);
	    ierr = qq0.setWhereClause(wstr);
	    ierr = qq1.setWhereClause(&dr);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "tableSelect -- failed to set where clause expressed as "
		    << "a qDiscreteRange(" << cnames[0] << ", double["
		    << sel1->nRows() << "])";
	    }
	    else {
		uint64_t cnt = 0;
		for (ibis::partList::const_iterator it = pl.begin();
		     it != pl.end(); ++ it) {
		    if (0 == qq0.setPartition(*it) &&
			0 == qq1.setPartition(*it)) {
			if (0 == qq0.evaluate() && 0 == qq1.evaluate()) {
			    if (qq0.getNumHits() > qq1.getNumHits()) {
				// not expecting this -- find out which
				// value is not present
				const ibis::bitvector *ht0 = qq0.getHitVector();
				const ibis::bitvector *ht1 = qq1.getHitVector();
				LOGGER(ibis::gVerbose >= 0)
				    << "Warning -- query 1 (" << qq1.id()
				    << ": " << cnames[0] << " IN ...) is "
				    "expected to produce no less hits than "
				    "query 0 (" << qq0.id() << ": "
				    << qq0.getWhereClause()
				    << ") on data partition " << (*it)->name()
				    << ", but query 1 has " << qq1.getNumHits()
				    << ", while query 0 has "
				    << qq0.getNumHits();
				if (ht0 != 0 && ht1 != 0) {
				    findMissingValues(*(*it), cnames[0],
						      *ht0, *ht1);
				}
			    }
			    cnt += qq1.getNumHits();
			}
		    }
		}
		if (cnt != sel1->nRows()) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- tableSelect -- qDiscreteRange("
			<< cnames[0] << ", double[" << sel1->nRows()
			<< "]) has " << cnt << " hit" << (cnt > 1 ? "s" : "")
			<< ", but should have " << sel1->nRows();
		}
		else {
		    LOGGER(ibis::gVerbose > 1)
			<< "tableSelect -- qDiscreteRange(" << cnames[0]
			<< ", double[" << sel1->nRows() << "]) has " << cnt
			<< " hits as expected";
		}
	    }
	}
    }

    timer.stop();
    LOGGER(ibis::gVerbose > 0)
	<< "tableSelect:: complete evaluation of " << sqlstring
	<< " took " << timer.CPUTime() << " CPU seconds, "
	<< timer.realTime() << " elapsed seconds";
} // tableSelect

// New style query.
static void doQuaere(const char *sstr, const char *fstr, const char *wstr,
		     const char *ordkeys, uint32_t limit, uint32_t start) {
    ibis::horometer timer;
    timer.start();
    std::string sqlstring; //
    {
	std::ostringstream ostr;
	if (sstr != 0 && *sstr != 0)
	    ostr << "SELECT " << sstr;
	if (fstr != 0 && *fstr != 0)
	    ostr << " FROM " << fstr;
	if (wstr != 0 && *wstr != 0)
	    ostr << " WHERE " << wstr;
	if (ordkeys && *ordkeys) {
	    ostr << " ORDER BY " << ordkeys;
	}
	if (limit > 0) {
	    ostr << " LIMIT ";
	    if (start > 0) ostr << start << ", ";
	    ostr << limit;
	}
	sqlstring = ostr.str();
    }
    LOGGER(ibis::gVerbose > 1)
	<< "doQuaere -- processing \"" << sqlstring << '\"';

    std::auto_ptr<ibis::table> res;
    if (estimate_opt < 0) { // directly evaluate the select clause
	std::auto_ptr<ibis::quaere> qq(ibis::quaere::create(0, fstr, wstr));
	if (qq.get() == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") failed to create an ibis::quaere object";
	    return;
	}
	res.reset(qq->select(sstr));
    }
    else {
	std::auto_ptr<ibis::quaere> qq(ibis::quaere::create(sstr, fstr, wstr));
	if (qq.get() == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") failed to create an ibis::quaere object";
	    return;
	}

	uint64_t nhits=1, hmax=0;
	qq->roughCount(nhits, hmax);
	if (nhits < hmax) {
	    LOGGER(ibis::gVerbose > 0)
		<< "doQuaere -- " << wstr << " --> [" << nhits << ", "
		<< hmax << ']';
	}
	else {
	    LOGGER(ibis::gVerbose > 0)
		<< "doQuaere -- " << wstr << " --> " << nhits
		<< " hit" << (hmax>1?"s":"");
	}
	if (estimate_opt > 0) return;

	int64_t cnts = qq->count();
	if (cnts < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") failed to produce a count of the number of hits"
		<< ", ierr = " << cnts;
	    return;
	}
	else if (nhits < hmax) {
	    LOGGER(ibis::gVerbose >= 0 &&
		   ((uint64_t)cnts < nhits || (uint64_t)cnts > hmax))
		<< "Warning -- doQuaere(" << sqlstring
		<< ") expects the return of count to be between "
		<< nhits << " and " << hmax
		<< ", but the actual return value is " << cnts;
	    nhits = cnts;
	}
	else if ((uint64_t)cnts != nhits) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") expects the return of count to be " << nhits
		<< ", but the actual return value is " << cnts;
	    nhits = cnts;
	}

	res.reset(qq->select());
    }
    if (res.get() == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- doQuaere(" << sqlstring
	    << ") failed to produce a result table";
	return;
    }

    if (res->nRows() > 1 && ((ordkeys && *ordkeys) || limit > 0)) {
	// top-K query
	res->orderby(ordkeys);
    }

    timer.stop();
    LOGGER(ibis::gVerbose >= 0)
	<< "doQuaere -- \"" << sqlstring << "\" produced a table with "
	<< res->nRows() << " row" << (res->nRows() > 1 ? "s" : "")
	<< " and " << res->nColumns() << " column"
	<< (res->nColumns() > 1 ? "s" : "") << ", took "
	<< timer.CPUTime() << " CPU seconds, "
	<< timer.realTime() << " elapsed seconds";

    int64_t ierr;
    if (outputfile != 0 && *outputfile != 0) {
	if (limit == 0)
	    limit = static_cast<uint32_t>(res->nRows());
	if (0 != strcmp(outputfile, "/dev/null")) {
	    std::ofstream output(outputfile, std::ios::out |
				 (appendToOutput ? std::ios::app :
				  std::ios::trunc));
	    if (outputnamestoo)
		res->dumpNames(output, ", ");
	    res->dump(output, start, limit, ", ");
	}
    }
    else if (ibis::gVerbose >= 0) {
	ibis::util::logger lg;
	if (limit == 0 && res->nColumns() > 0) {
	    limit = ((res->nRows() >> ibis::gVerbose) > 0 ?
		     1 << ibis::gVerbose : static_cast<uint32_t>(res->nRows()));
	    if (limit > (res->nRows() >> 1))
		limit = res->nRows();
	}
	if (limit > 0 && limit < res->nRows()) {
	    lg() << "doQuaere -- the first ";
	    if (limit > 1)
		lg() << limit << " rows ";
	    else
		lg() << "row ";
	    lg() << "(of " << res->nRows()
		 << ") from the result table for \""
		 << sqlstring << "\"\n";
	}
	else {
	    lg() << "doQuaere -- the result table (" << res->nRows()
		 << " x " << res->nColumns() << ") for \""
		 << sqlstring << "\"\n";
	}
	if (outputnamestoo)
	    res->dumpNames(lg(), ", ");
	res->dump(lg(), start, limit, ", ");
    }

    ibis::table::stringList cn = res->columnNames();
    ibis::table::typeList ct = res->columnTypes();
    if (cn.size() > 1 && ct.size() == cn.size() &&
	(ct[0] == ibis::TEXT || ct[0] == ibis::CATEGORY) &&
	(ct.back() != ibis::TEXT && ct.back() != ibis::CATEGORY)) {
	const char* s = cn[0];
	cn[0] = cn.back();
	cn.back() = s;
	ibis::TYPE_T t = ct[0];
	ct[0] = ct.back();
	ct.back() = t;
    }
    if (ibis::gVerbose > 3 && res->nRows() > 1 && !cn.empty() && !ct.empty() &&
	(ct.back() == ibis::BYTE || ct.back() == ibis::UBYTE ||
	 ct.back() == ibis::SHORT || ct.back() == ibis::USHORT ||
	 ct.back() == ibis::INT || ct.back() == ibis::UINT ||
	 ct.back() == ibis::LONG || ct.back() == ibis::ULONG ||
	 ct.back() == ibis::FLOAT || ct.back() == ibis::DOUBLE)) {
	// try a silly query on res
	std::string cnd3, sel1, sel3;
	sel1 = "max(";
	sel1 += cn.back();
	sel1 += ") as mx, min(";
	sel1 += cn.back();
	sel1 += ") as mn";
	if (cn.size() > 1) {
	    sel3 = cn[0];
	    sel3 += ", avg(";
	    sel3 += cn[1];
	    sel3 += ')';
	}
	else {
	    sel3 = "floor(";
	    sel3 += cn[0];
	    sel3 += "/10), avg(";
	    sel3 += cn[0];
	    sel3 += ')';
	}

	std::auto_ptr<ibis::table> res1(res->select(sel1.c_str(), "1=1"));
	if (res1.get() == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") failed to find the min and max of " << cn.back()
		<< " in the result table " << res->name();
	    return;
	}
	double minval, maxval;
	ierr = res1->getColumnAsDoubles("mx", &maxval);
	if (ierr != 1) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") expects to retrieve exactly one value from res1, "
		"but getColumnAsDoubles returned " << ierr;
	    return;
	}
	ierr = res1->getColumnAsDoubles("mn", &minval);
	if (ierr != 1) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") expects to retrieve exactly one value from res1, "
		"but getColumnAsDoubles returned " << ierr;
	    return;
	}

	std::ostringstream oss;
	oss << "log(" << (0.5*(minval+maxval)) << ") <= log("
	    << cn.back() << ')';
	std::auto_ptr<ibis::table>
	    res3(res->select(sel3.c_str(), oss.str().c_str()));
	if (res3.get() == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") failed to evaluate query " << oss.str()
		<< " on the table " << res->name();
	    return;
	}

	ibis::util::logger lg;
	res3->describe(lg());
	res3->dump(lg(), ", ");
    }
    else if (ibis::gVerbose > 3 && res->nRows() > 1 &&
	     cn.size() > 1 && ct.size() > 1 &&
	     (ct.back() == ibis::CATEGORY ||
	      ct.back() == ibis::TEXT)) {
	// try a silly query on res
	std::string cnd2, sel2;
	if (cn.size() > 1) {
	    sel2 = "floor(";
	    sel2 += cn[0];
	    sel2 += ")/3, min(";
	    sel2 += cn[0];
	    sel2 += "), avg(";
	    sel2 += cn[1];
	    sel2 += ')';
	}
	else {
	    sel2 = "floor(";
	    sel2 += cn[0];
	    sel2 += "/10, avg(";
	    sel2 += cn[0];
	    sel2 += ')';
	}
	{
	    std::auto_ptr<ibis::table::cursor> cur(res->createCursor());
	    if (cur.get() == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- doQuaere(" << sqlstring
		    << ") failed to create a cursor from the result table";
		return;
	    }
	    std::string tmp;
	    for (size_t j = 0; tmp.empty() && j < cur->nRows(); ++ j) {
		ierr = cur->fetch();
		if (ierr != 0) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- doQuaere(" << sqlstring
			<< ") failed to fetch row " << j
			<< " for the cursor from table " << res->name();
		    return;
		}
		ierr = cur->getColumnAsString(cn.back(), tmp);
		if (ierr != 0) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- doQuaere(" << sqlstring
			<< ") failed to retrieve row " << j << " of column "
			<< cn.back() << " from the cursor for table "
			<< res->name();
		    return;
		}
	    }
	    if (tmp.empty()) {
		LOGGER(ibis::gVerbose > 0)
		    << "doQuaere(" << sqlstring
		    << ") can not find a non-empty string for column "
		    << cn.back() << " from the table " << res->name();
		return;
	    }
	    cnd2 = cn.back();
	    cnd2 += " = \"";
	    cnd2 += tmp;
	    cnd2 += '"';
	}

	std::auto_ptr<ibis::table>
	    res2(res->select(sel2.c_str(), cnd2.c_str()));
	if (res2.get() == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuaere(" << sqlstring
		<< ") failed to evaluate query " << cnd2
		<< " on the table " << res->name();
	    return;
	}

	ibis::util::logger lg;
	res2->describe(lg());
	res2->dump(lg(), ", ");
    }
} // doQuaere

// evaluate a single query -- print selected columns through ibis::bundle
static void doQuery(ibis::part* tbl, const char* uid, const char* wstr,
		    const char* sstr, const char* ordkeys,
		    uint32_t limit, uint32_t start) {
    std::string sqlstring; //
    {
	std::ostringstream ostr;
	if (sstr != 0 && *sstr != 0)
	    ostr << "SELECT " << sstr;
	ostr << " FROM " << tbl->name();
	if (wstr != 0 && *wstr != 0)
	    ostr << " WHERE " << wstr;
	if (ordkeys && *ordkeys) {
	    ostr << " ORDER BY " << ordkeys;
	}
	if (limit > 0) {
	    ostr << " LIMIT ";
	    if (start > 0)
		ostr << start << ", ";
	    ostr << limit;
	}
	sqlstring = ostr.str();
    }
    LOGGER(ibis::gVerbose > 1)
	<< "doQuery -- processing \"" << sqlstring << '\"';

    long num1, num2;
    const char* asstr = 0;
    ibis::horometer timer;
    timer.start();
    // the third argument is needed to make sure a private directory is
    // created for the query object to store the results produced by the
    // select clause.
    ibis::query aQuery(uid, tbl,
		       ((sstr != 0 && *sstr != 0 &&
			 ((ordkeys != 0 && *ordkeys != 0) || limit > 0 ||
			  verify_rid || testing > 0)) ?
			"ibis" : static_cast<const char*>(0)));
    if (ridfile != 0) {
	ibis::ridHandler handle(0); // a sample ridHandler
	ibis::RIDSet rset;
	handle.read(rset, ridfile);
	aQuery.setRIDs(rset);
    }
    if (wstr != 0 && *wstr != 0) {
	num2 = aQuery.setWhereClause(wstr);
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose > 3)
		<< "Warning -- doQuery failed to assigned the where clause \""
		<< wstr << "\" on partition " << tbl->name()
		<< ", setWhereClause returned " << num2;
	    return;
	}
    }
    if (sstr != 0 && *sstr != 0) {
	num2 = aQuery.setSelectClause(sstr);
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose > 3)
		<< "Warning -- doQuery failed to assign the select clause \""
		<< sstr << "\" on partition " << tbl->name()
		<< ", setSelectClause returned " << num2;
	    return;
	}

	asstr = aQuery.getSelectClause();
    }
    if (aQuery.getWhereClause() == 0 && ridfile == 0
	&& asstr == 0)
	return;
    if (zapping && aQuery.getWhereClause()) {
	std::string old = aQuery.getWhereClause();
	std::string comp = aQuery.removeComplexConditions();
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg;
	    if (! comp.empty())
		lg() << "doQuery -- the WHERE clause \""
		     <<  old.c_str() << "\" is split into \""
		     << comp.c_str()  << "\" AND \""
		     << aQuery.getWhereClause() << "\"";
	    else
		lg() << "doQuery -- the WHERE clause \""
		     << aQuery.getWhereClause()
		     << "\" is considered simple";
	}
    }

    if (sequential_scan) {
	num2 = aQuery.countHits();
	if (num2 < 0) {
	    ibis::bitvector btmp;
	    num2 = aQuery.sequentialScan(btmp);
	    if (num2 < 0) {
		ibis::util::logger lg;
		lg() << "doQuery:: sequentialScan("
		     << aQuery.getWhereClause() << ") failed";
		return;
	    }

	    num2 = btmp.cnt();
	}
	if (ibis::gVerbose >= 0) {
	    timer.stop();
	    ibis::util::logger lg;
	    lg() << "doQuery:: sequentialScan("
		 << aQuery.getWhereClause() << ") produced "
		 << num2 << " hit" << (num2>1 ? "s" : "") << ", took "
		 << timer.CPUTime() << " CPU seconds, "
		 << timer.realTime() << " elapsed seconds";
	}
	return;
    }

    if (estimate_opt >= 0) {
	num2 = aQuery.estimate();
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "doQuery -- failed to estimate \"" << wstr
		<< "\", error code = " << num2;
	    return;
	}
	num1 = aQuery.getMinNumHits();
	num2 = aQuery.getMaxNumHits();
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg;
	    lg() << "doQuery -- the number of hits is ";
	    if (num2 > num1)
		lg() << "between " << num1 << " and ";
	    lg() << num2;
	}
	if (estimate_opt > 0 || num2 == 0) {
	    if (ibis::gVerbose >= 0) {
		timer.stop();
		ibis::util::logger lg;
		lg() << "doQuery:: estimate("
		     << aQuery.getWhereClause() << ") took "
		     << timer.CPUTime() << " CPU seconds, "
		     << timer.realTime() << " elapsed seconds.";
		if (num1 == num2)
		    lg() << "  The number of hits is " << num1;
		else
		    lg() << "  The number of hits is between "
			 << num1 << " and " << num2;
	    }
	    return; // stop here is only want to estimate
	}
    }

    num2 = aQuery.evaluate();
    if (num2 < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "doQuery -- failed to evaluate \"" << wstr
	    << "\", error code = " << num2;
	return;
    }
    num1 = aQuery.getNumHits();

    if (asstr != 0 && *asstr != 0 && num1 > 0 && ibis::gVerbose >= 0) {
	std::auto_ptr<ibis::bundle> bdl(ibis::bundle::create(aQuery));
	if (bdl.get() == 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doQuery(" << sqlstring
		<< ") failed to create the bundle object for output operations";
	    return;
	}

	if (ordkeys && *ordkeys) { // top-K query
	    bdl->reorder(ordkeys);
	}
	if (limit > 0 || start > 0) {
	    num2 = bdl->truncate(limit, start);
	    if (num2 < 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "doQuery -- failed to truncate the bundle object";
		return;
	    }
	}
	if (0 != outputfile && 0 == strcmp(outputfile, "/dev/null")) {
	    // no need to actually write anything to the output file
	}
	else if (outputfile != 0 && *outputfile != 0) {
	    std::ofstream output(outputfile,
				 std::ios::out | 
				 (appendToOutput ? std::ios::app :
				  std::ios::trunc));
	    if (output) {
		LOGGER(ibis::gVerbose >= 0)
		    << "doQuery -- query (" << aQuery.getWhereClause()
		    << ") results written to file \""
		    <<  outputfile << "\"";
		if (ibis::gVerbose > 8 || verify_rid)
		    bdl->printAll(output);
		else
		    bdl->print(output);
	    }
	    else {
		ibis::util::logger lg;
		lg() << "Warning ** doQuery failed to open file \""
		     << outputfile << "\" for writing query ("
		     << aQuery.getWhereClause() << ")\n";
		if (ibis::gVerbose > 8 || verify_rid)
		    bdl->printAll(lg());
		else
		    bdl->print(lg());
	    }
	    appendToOutput = true; // all query output go to the same file
	}
	else {
	    ibis::util::logger lg;
	    if (ibis::gVerbose > 8 || verify_rid)
		bdl->printAll(lg());
	    else
		bdl->print(lg());
	}
    }
    if (ibis::gVerbose >= 0) {
	timer.stop();
	ibis::util::logger lg;
	lg() << "doQuery:: evaluate(" << sqlstring
	     << ") produced " << num1 << (num1 > 1 ? " hits" : " hit")
	     << ", took " << timer.CPUTime() << " CPU seconds, "
	     << timer.realTime() << " elapsed seconds";
    }

    if (ibis::gVerbose > 0 && (sstr == 0 || *sstr == 0) &&
	aQuery.getWhereClause()) {
	ibis::countQuery cq(tbl);
	num2 = cq.setWhereClause(aQuery.getWhereClause());
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "doQuery -- failed to set \"" << aQuery.getWhereClause()
		<< "\" on a countQuery";
	}
	else {
	    num2 = cq.evaluate();
	    if (num2 < 0) {
		LOGGER(ibis::gVerbose > 0)
		    << "doQuery -- failed to evaluate the count query on "
		    << aQuery.getWhereClause();
	    }
	    else if (cq.getNumHits() != num1) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- countQuery.getNumHits returned "
		    << cq.getNumHits() << ", while query.getNumHits returned "
		    << num1;
	    }
	}
    }
    if (ibis::gVerbose > 5 || (verify_rid && ibis::gVerbose >= 0)) {
	ibis::bitvector btmp;
	num2 = aQuery.sequentialScan(btmp);
	if (num2 < 0) {
	    ibis::util::logger lg;
	    lg() << "doQuery:: sequentialScan("
		 << aQuery.getWhereClause() << ") failed";
	}
	else {
	    num2 = btmp.cnt();
	    if (num1 != num2 && ibis::gVerbose >= 0) {
		ibis::util::logger lg;
		lg() << "Warning ** query \"" << aQuery.getWhereClause()
		     << "\" generated " << num1
		     << " hit" << (num1 >1  ? "s" : "")
		     << " with evaluate(), but generated "
		     << num2 << " hit" << (num2 >1  ? "s" : "")
		     << " with sequentialScan";
	    }
	}
    }

    if (ibis::gVerbose >= 0 && (verify_rid || testing > 1)) {
	// retrieve RIDs as bundles
	uint32_t nbdl = 0;
	ibis::RIDSet* rid0 = new ibis::RIDSet;
	const ibis::RIDSet *tmp = aQuery.getRIDsInBundle(0);
	while (tmp != 0) {
	    rid0->insert(rid0->end(), tmp->begin(), tmp->end());
	    delete tmp;
	    ++ nbdl;
	    tmp = aQuery.getRIDsInBundle(nbdl);
	}
	if (rid0->size() == 0) {
	    delete rid0;
	    return;
	}
	ibis::util::sortRIDs(*rid0);

	// retrieve the RIDs in one shot
	ibis::RIDSet* rid1 = aQuery.getRIDs();
	if (rid1 == 0) {
	    delete rid0;
	    return;
	}

	ibis::util::sortRIDs(*rid1);
	if (rid1->size() == rid0->size()) {
	    uint32_t i, cnt=0;
	    ibis::util::logger lg;
	    for (i=0; i<rid1->size(); ++i) {
		if ((*rid1)[i].value != (*rid0)[i].value) {
		    ++cnt;
		    lg() << i << "th RID (" << (*rid1)[i]
			 << ") != (" << (*rid0)[i] << ")\n";
		}
	    }
	    if (cnt > 0)
		lg() << "Warning -- " << cnt
		     << " mismatches out of a total of "
		     << rid1->size();
	    else
		lg() << "Successfully verified " << rid0->size()
		     << " RID" << (rid0->size()>1?"s":"");
	}
	else if (sstr != 0) {
	    ibis::util::logger lg;
	    lg() << "sent " << rid1->size() << " RIDs, got back "
		 << rid0->size();
	    uint32_t i=0, cnt;
	    cnt = (rid1->size() < rid0->size()) ? rid1->size() :
		rid0->size();
	    while (i < cnt) {
		lg() << "\n(" << (*rid1)[i] << ") >>> (" << (*rid0)[i];
		++i;
	    }
	    if (rid1->size() < rid0->size()) {
		while (i < rid0->size()) {
		    lg() << "\n??? >>> (" << (*rid0)[i] << ")";
		    ++i;
		}
	    }
	    else {
		while (i < rid1->size()) {
		    lg() << "\n(" << (*rid1)[i] << ") >>> ???";
		    ++i;
		}
	    }
	}
	delete rid0;

	if (rid1->size() > 1024) {
	    // select no more than 1024 RIDs -- RID2Hits is slow
	    uint32_t len = 512 + (511 & rid1->size());
	    if (len == 0) len = 1024;
	    rid1->resize(len);
	}

	ibis::RIDSet* rid2 = new ibis::RIDSet;
	rid2->deepCopy(*rid1);
	delete rid1; // setRIDs removes the underlying file for rid1
	aQuery.setRIDs(*rid2);
	rid1 = rid2; // setRIDs has copied rid2
	aQuery.evaluate();
	rid2 = aQuery.getRIDs();
	if (rid2 == 0) { // make sure the pointer is valid
	    rid2 = new ibis::RIDSet;
	}
	ibis::util::sortRIDs(*rid2);
	if (rid1->size() == rid2->size()) {
	    uint32_t i, cnt=0;
	    ibis::util::logger lg;
	    for (i=0; i<rid1->size(); ++i) {
		if ((*rid1)[i].value != (*rid2)[i].value) {
		    ++cnt;
		    lg() << i << "th RID (" << (*rid1)[i]
			 << ") != (" << (*rid2)[i] << ")\n";
		}
	    }
	    if (cnt > 0)
		lg() << "Warning -- " << cnt
		     << " mismatches out of a total of "
		     << rid1->size();
	    else
		lg() << "Successfully verified " << rid1->size()
		     << " RID" << (rid1->size()>1?"s":"");
	}
	else {
	    ibis::util::logger lg;
	    lg() << "sent " << rid1->size() << " RIDs, got back "
		 << rid2->size();
	    uint32_t i=0, cnt;
	    cnt = (rid1->size() < rid2->size()) ? rid1->size() :
		rid2->size();
	    while (i < cnt) {
		lg() << "\n(" << (*rid1)[i] << ") >>> (" << (*rid2)[i]
		     << ")";
		++i;
	    }
	    if (rid1->size() < rid2->size()) {
		while (i < rid2->size()) {
		    lg() << "\n??? >>> (" << (*rid2)[i] << ")";
		    ++i;
		}
	    }
	    else {
		while (i < rid1->size()) {
		    lg() << "\n(" << (*rid1)[i] << ") >>> ???";
		    ++i;
		}
	    }
	}
	delete rid1;
	delete rid2;
    }
} // doQuery

// evaluate a single query -- only work on partitions that have defined
// column shapes, i.e., they contain data computed on meshes.
static void doMeshQuery(ibis::part* tbl, const char* uid, const char* wstr,
			const char* sstr) {
    const std::vector<uint32_t>& dim = tbl->getMeshShape();
    if (dim.empty()) {
	doQuery(tbl, uid, wstr, sstr, 0, 0, 0);
	return;
    }

    LOGGER(ibis::gVerbose > 0)
	<< "doMeshQuery -- processing query " << wstr
	<< " on partition " << tbl->name();

    long num1, num2;
    ibis::horometer timer;
    timer.start();
    ibis::meshQuery aQuery(uid, tbl);
    aQuery.setWhereClause(wstr);
    if (aQuery.getWhereClause() == 0)
	return;
    if (zapping && aQuery.getWhereClause()) {
	std::string old = aQuery.getWhereClause();
	std::string comp = aQuery.removeComplexConditions();
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg;
	    if (! comp.empty())
		lg() << "doMeshQuery -- the WHERE clause \""
		     << old.c_str() << "\" is split into \""
		     << comp.c_str() << "\" AND \""
		     << aQuery.getWhereClause() << "\"";
	    else
		lg() << "doMeshQuery -- the WHERE clause \""
		     << aQuery.getWhereClause()
		     << "\" is considered simple";
	}
    }

    const char* asstr = 0;
    if (sstr != 0 && *sstr != 0) {
	aQuery.setSelectClause(sstr);
	asstr = aQuery.getSelectClause();
    }
    if (estimate_opt >= 0) {
	num2 = aQuery.estimate();
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "doMeshQuery -- failed to estimate \"" << wstr
		<< "\", error code = " << num2;
	    return;
	}
	num1 = aQuery.getMinNumHits();
	num2 = aQuery.getMaxNumHits();
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg;
	    lg() << "doMeshQuery -- the number of hits is ";
	    if (num1 < num2)
		lg() << "between " << num1 << " and ";
	    lg() << num2;
	}
	if (estimate_opt > 0 || num2 == 0) {
	    if (ibis::gVerbose >= 0) {
		timer.stop();
		ibis::util::logger lg;
		lg() << "doMeshQuery:: estimate("
		     << aQuery.getWhereClause() << ") took "
		     << timer.CPUTime() << " CPU seconds, "
		     << timer.realTime() << " elapsed seconds";
	    }
	    return; // stop here is only want to estimate
	}
    }

    num2 = aQuery.evaluate();
    if (num2 < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- doMeshQuery -- failed to evaluate \"" << wstr
	    << "\", error code = " << num2;
	return;
    }
    num1 = aQuery.getNumHits();
    if (ibis::gVerbose > 0) {
	timer.stop();
	ibis::util::logger lg;
	lg() << "doMeshQuery:: evaluate(" << aQuery.getWhereClause() 
	     << ") produced " << num1 << (num1 > 1 ? " hits" : " hit")
	     << ", took " << timer.CPUTime() << " CPU seconds, "
	     << timer.realTime() << " elapsed seconds";
    }

    std::vector<uint32_t> lines;
    num2 = aQuery.getHitsAsLines(lines);
    if (num2 < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- aQuery.getHitsAsLines returned " << num2;
	return;
    }
    else if (lines.empty()) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- aQuery.getHitsAsLines returned no lines";
	return;
    }
    if (ibis::gVerbose > 0) {
	ibis::util::logger lg;
	lg() << "doMeshQuery:: turned " << num1 << " hit" << (num1>1?"s":"")
	     << " into " << num2 << " query lines on a " << dim[0];
	for (unsigned j = 1; j < dim.size(); ++ j)
	    lg() << " x " << dim[j];
	lg() << " mesh";
    }
    std::vector<uint32_t> label1;
    num2 = aQuery.labelLines(dim.size(), lines, label1);
    if (num2 < 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- aQuery.labelLines failed with error code " << num2;
	return;
    }
    LOGGER(ibis::gVerbose > 0)
	<< "doMeshQuery: identified " << num2 << " connected component"
	<< (num2>1?"s":"") << " among the query lines";

    if (ibis::gVerbose >= 0 || testing > 0) {
	std::vector< std::vector<uint32_t> > blocks;
	std::vector<uint32_t> label2;
	num2 = aQuery.getHitsAsBlocks(blocks);
	if (num2 < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- aQuery.getHitsAsBlocks returned " << num2;
	    return;
	}
	else if (blocks.empty()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- aQuery.getHitsAsBlocks returned no blocks";
	    return;
	}

	num2 = aQuery.labelBlocks(blocks, label2);
	if (num2 < 0) {
	    LOGGER(num2 < 0)
		<< "Warning -- aQuery.labelBlocks failed with error code "
		<< num2;
	    return;
	}
	LOGGER(ibis::gVerbose > 0)
	    << "doMeshQuery:: converted " << num1 << " hit" << (num1>1?"s":"")
	    << " into " << blocks.size() << " block" << (blocks.size()>1?"s":"")
	    << " and identified " << num2 << " connected component"
	    << (num2>1?"s":"") << " among the blocks";

	/// compare the output from labeling lines against those from labeling
	/// the blocks
	const unsigned ndim = dim.size();
	const unsigned ndm1 = dim.size() - 1;
	const unsigned ndp1 = dim.size() + 1;
	size_t jb = 0;	// pointing to a block
	size_t jl = 0;	// pointing to a line
	num1 = 0;	// number of mismatches
	ibis::util::logger lg;
	lg() << "\ndoMeshQuery -- Compare the two sets of labels";
	while (jb < blocks.size() || jl < lines.size()) {
	    if (jb < blocks.size()) { // has a valid block
		if (jl < lines.size()) { // has a valid line
		    int cmp = (lines[jl] < blocks[jb][0] ? -1 :
			       lines[jl] >= blocks[jb][1] ? 1 : 0);
		    for (unsigned j3 = 1; cmp == 0 && j3 < ndm1; ++ j3)
			cmp = (lines[jl+j3] < blocks[jb][j3+j3] ? -1 :
			       lines[jl+j3] >= blocks[jb][j3+j3+1] ? 1 : 0);
		    if (cmp == 0)
			cmp = (lines[jl+ndim] <= blocks[jb][ndm1+ndm1] ? -1 :
			       lines[jl+ndm1] >= blocks[jb][ndm1+ndm1+1] ? 1 :
			       0);
		    if (cmp > 0) { // extra block?
			lg() << "\nblock[" << jb << "] (" << blocks[jb][0]
			     << ", " << blocks[jb][1];
			for (unsigned j3 = 2; j3 < ndim+ndim; ++ j3)
			    lg() << ", " << blocks[jb][j3];
			lg() << "\tline[??]( )";
			++ jb;
			++ num1;
		    }
		    else if (cmp < 0) { // extra line?
			lg() << "\nblock[??]( )\tline[" << jl/ndp1 << "] ("
			     << lines[jl];
			for (unsigned j4 = jl+1; j4 < jl+ndp1; ++ j4)
			    lg() << ", " << lines[j4];
			lg() << ")";
			jl += ndp1;
			++ num1;
		    }
		    else { // matching block and line
			size_t j3;
			unsigned linecount = 0;
			unsigned labelcount = 0;
			unsigned expectedcount = blocks[jb][1] - blocks[jb][0];
			for (unsigned jj = 2; jj+3 < blocks[jb].size(); jj += 2)
			    expectedcount *=
				(blocks[jb][jj+1] - blocks[jb][jj]);
			linecount = (blocks[jb][ndm1+ndm1] == lines[jl+ndm1] &&
				     blocks[jb][ndm1+ndim] == lines[jl+ndim]);
			labelcount = (label2[jb] == label1[jl/ndp1]);
			for (j3 = jl+ndp1; j3 < lines.size(); j3 += ndp1) {
			    bool match =
				(blocks[jb][ndm1+ndm1] == lines[j3+ndm1] &&
				 blocks[jb][ndm1+ndim] == lines[j3+ndim]);
			    for (unsigned jj = 0;
				 match && jj < ndm1;
				 ++ jj) {
				match = (blocks[jb][jj+jj] <= lines[j3+jj] &&
					 blocks[jb][jj+jj+1] > lines[j3+jj]);
			    }
			    if (match) {
				labelcount += (label2[jb] == label1[j3/ndp1]);
				++ linecount;
			    }
			    else {
				break;
			    }
			}
			if (linecount != expectedcount ||
			    labelcount != expectedcount || ibis::gVerbose > 6) {
			    lg() << "\nblock[" << jb << "] (" << blocks[jb][0]
				 << ", " << blocks[jb][1];
			    for (unsigned j3 = 2; j3 < ndim+ndim; ++ j3)
				lg() << ", " << blocks[jb][j3];
			    lg() << ")\tline[" << jl << "] (" << lines[jl];
			    for (unsigned j4 = jl+1; j4 < jl+ndp1; ++ j4)
				lg() << ", " << lines[j4];
			    lg() << "),\tlabelb = " << label2[jb]
				 << "\tlabell = " << label1[jl/ndp1];
			    if (expectedcount > 1)
				lg() << "\t... expected " << expectedcount
				     << " lines, found " << linecount
				     << " matching line" << (linecount>1?"s":"")
				     << " with " << labelcount
				     << " correct label"
				     << (labelcount>1?"s":"");
			    if (linecount != expectedcount ||
				labelcount != expectedcount) lg() << " ??";
			}
			num1 += (linecount != expectedcount ||
				 labelcount != expectedcount);
			jl = j3;
			++ jb;
		    }
		}
		else { // no more lines
		    lg() << "\nblock[" << jb << "] (" << blocks[jb][0]
			 << ", " << blocks[jb][1];
		    for (unsigned j3 = 2; j3 < ndim+ndim; ++ j3)
			lg() << ", " << blocks[jb][j3];
		    lg() << ")\tline[??]( )";
		    ++ jb;
		    ++ num1;
		}
	    }
	    else { // no more blocks
		lg() << "\nblock[??]( )\tline[" << jl << "] ("
		     << lines[jl];
		for (unsigned j4 = jl+1; j4 < jl+ndp1; ++ j4)
		    lg() << ", " << lines[j4];
		lg() << ")";
		jl += ndp1;
		++ num1;
	    }
	}
	lg() << "\n" << (num1>0?"Warning (!__!) --":"(^o^)") << " found "
	     << num1 << " mismatch" << (num1>1 ? "es" : "") << "\n";
    }

    if (asstr != 0 && *asstr != 0 && ibis::gVerbose > 0) {
	if (outputfile != 0 && *outputfile != 0) {
	    std::ofstream output(outputfile,
				 std::ios::out | std::ios::app);
	    if (output) {
		LOGGER(ibis::gVerbose > 0)
		    << "doMeshQuery -- query (" << aQuery.getWhereClause()
		    << ") results written to file \""
		    << outputfile << "\"";
		if (ibis::gVerbose > 8 || verify_rid)
		    aQuery.printSelectedWithRID(output);
		else
		    aQuery.printSelected(output);
	    }
	    else {
		ibis::util::logger lg;
		lg() << "Warning ** doMeshQuery failed to "
		     << "open file \"" << outputfile
		     << "\" for writing query ("
		     << aQuery.getWhereClause() << ") output\n";
		if (ibis::gVerbose > 8 || verify_rid)
		    aQuery.printSelectedWithRID(lg());
		else
		    aQuery.printSelected(lg());
	    }
	}
	else {
	    ibis::util::logger lg;
	    if (ibis::gVerbose > 8 || verify_rid)
		aQuery.printSelectedWithRID(lg());
	    else
		aQuery.printSelected(lg());
	}
    } // if (asstr != 0 && num1>0 && ibis::gVerbose > 0)
} // doMeshQuery

// append the content of the named directory to the existing partitions
static void doAppend(const char* dir) {
    long ierr = 0;
    ibis::part *tbl = 0;
    bool newtable = true;
    if (dir == 0 || *dir == 0) return;
    {
	Stat_T tmp;
	if (UnixStat(dir, &tmp) != 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- doAppend can not find the status of directory "
		<< dir;
	    return;
	}
	if ((tmp.st_mode & S_IFDIR) != S_IFDIR) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- doAppend expects the argument \"" << dir
		<< "\" to be a directory name, but it is not";
	    return;
	}
    }
    if (appendto != 0) {
	Stat_T tmp;
	if (UnixStat(appendto, &tmp) == 0) { // appendto is a directory name
	    tbl = new ibis::part(appendto, static_cast<const char*>(0));
	    if (tbl != 0) {
		for (unsigned i = 0; i < ibis::datasets.size(); ++ i) {
		    if (stricmp(tbl->name(), ibis::datasets[i]->name()) == 0) {
			delete tbl;
			tbl = ibis::datasets[i];
			newtable = false;
			break;
		    }
		}
	    }
	}
	if (tbl == 0) { // try appendto as a partition name
	    for (unsigned i = 0; i < ibis::datasets.size(); ++ i) {
		if (stricmp(appendto, ibis::datasets[i]->name()) == 0) {
		    // found an existing partition
		    tbl = ibis::datasets[i];
		    newtable = false;
		    break;
		}
	    }
	}
    }

    if (tbl == 0) { // try the metaTags next
	char *tmp = ibis::part::readMetaTags(dir);
	if (tmp != 0) {
	    ibis::partList::iterator itt;
	    itt = ibis::datasets.begin();
	    ibis::resource::vList mtags;
	    ibis::resource::parseNameValuePairs(tmp, mtags);
	    while (itt != ibis::datasets.end()) {
		if ((*itt)->matchMetaTags(mtags))
		    break;
		++ itt;
	    }

	    if (itt != ibis::datasets.end()) { // matched the meta tags
		tbl = (*itt);
		newtable = false;
	    }
	    else if (appendto == 0) { // user did not specify an name
		tbl = new ibis::part(mtags);
		newtable = true;
	    }
	    delete [] tmp;
	}
    }

    if (tbl == 0) { // need to allocate a new partition
	if (appendto != 0) { // use externally specified name
	    tbl = new ibis::part(appendto);
	}
	else { // generate an random name based on user name and dir
	    char tmp[128];
	    const char* name = ibis::util::userName();
	    sprintf(tmp, "%c%lX", (isalpha(*name) ? toupper(*name) : 'T'),
		    static_cast<long unsigned>
		    (ibis::util::checksum(dir, strlen(dir))));
	    tbl = new ibis::part(tmp);
	}
	newtable = true;
    }
    if (tbl == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- doAppend(" << dir << ") failed to allocate an "
	    "ibis::part object. Can NOT continue.\n";
	return;
    }

    ibis::horometer timer;
    timer.start();
    ierr = tbl->append(dir);
    timer.stop();
    if (ierr < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- doAppend(" << dir
	    << "): failed to append to data partition \""
	    << tbl->name() << "\", ierr = " << ierr;
	if (newtable)
	    delete tbl;
	return;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "doAppend(" << dir << "): adding " << ierr
	    << " rows took "  << timer.CPUTime() << " CPU seconds, "
	    << timer.realTime() << " elapsed seconds";
    }
    const long napp = ierr;
    if (tbl->getState() != ibis::part::STABLE_STATE) {
	if (ibis::gVerbose > 3 ||
	    (ibis::gVerbose >= 0 && testing > 0)) {// self test after append
	    int nth = static_cast<int>(ibis::gVerbose < 20
				       ? 1+sqrt((double)ibis::gVerbose)
				       : 3+log((double)ibis::gVerbose));
	    tbl->buildIndexes();
	    ierr = tbl->selfTest(nth);
	}
	else { // very quiet, skip self testing
	    ierr = 0;
	}
	if (ierr != 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "doAppend(" << dir << "): selfTest encountered "
		<< ierr << " error" << (ierr > 1 ? "s." : ".")
		<< " Will attempt to roll back the changes.";
	    ierr = tbl->rollback();
	    LOGGER(ierr <= 0 && ibis::gVerbose >= 0)
		<< "doAppend(" << dir << "): rollback returned with "
		<< ierr << "\n";
	    if (newtable)
		delete tbl;
	    return;
	}

	timer.start();
	ierr = tbl->commit(dir);
	timer.stop();
	if (ierr != napp) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- doAppend(" << dir
		<< "): expected commit to return "
		<< napp << ", but it actually retruned " << ierr
		<< ".  Unrecoverable error!\n";
	    return;
	}
	else {
	    LOGGER(ibis::gVerbose >= 0)
		<< "doAppend(" << dir << "): committing " << napp
		<< " rows to partition \"" << tbl->name() << "\" took "
		<< timer.CPUTime() << " CPU seconds, "
		<< timer.realTime() << " elapsed seconds.  "
		"Total number of rows is " << tbl->nRows() << ".";
	}

	if (ierr <= 0) {
	    if (newtable) // new partition, delete it
		delete tbl;
	    return;
	}

	// self test after commit,
	if (ibis::gVerbose > 4 || (ibis::gVerbose > 0 && testing > 0)) {
	    tbl->buildIndexes();
	    ierr = tbl->selfTest(0);
	    LOGGER(ibis::gVerbose > 0)
		<< "doAppend(" << dir << "): selfTest on partition \""
		<< tbl->name() << "\" (after committing " << napp
		<< (napp > 1 ? " rows" : " row")
		<< ") encountered " << ierr
		<< (ierr > 1 ? " errors\n" : " error\n");
	}
    }
    else if (ibis::gVerbose > 3 || (ibis::gVerbose >= 0 && testing > 0)) {
	tbl->buildIndexes();
	ierr = tbl->selfTest(0);
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- doAppend(" << dir << "): selfTest on partition \""
	    << tbl->name() << "\" (after appending " << napp
	    << (napp > 1 ? " rows" : " row")
	    << ") encountered " << ierr
	    << (ierr > 1 ? " errors\n" : " error\n");
    }
    if (newtable) // new partition, add it to the list of partitions
	ibis::datasets.push_back(tbl);
} // doAppend

static void doJoin(const char* uid, ibis::joinspec& js) {
    std::ostringstream oss;
    oss << "doJoin(";
    js.print(oss);
    oss << ')';
    ibis::util::timer tm(oss.str().c_str(), 1);
    ibis::partList::const_iterator pt1 = ibis::datasets.begin();
    for (; pt1 != ibis::datasets.end() &&
	     stricmp((*pt1)->name(), js.part1) != 0;
	 ++ pt1);
    if (pt1 == ibis::datasets.end()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str() << ": " << js.part1
	    << " is not a know data partition";
	return;
    }
    ibis::partList::const_iterator pt2 = ibis::datasets.begin();
    for (; pt2 != ibis::datasets.end() &&
	     stricmp((*pt2)->name(), js.part2) != 0;
	 ++ pt2);
    if (pt2 == ibis::datasets.end()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str() << ": " << js.part2
	    << " is not a know data partition";
	return;
    }
    ibis::quaere *jn = ibis::quaere::create
	(*pt1, *pt2, js.jcol, js.cond1, js.cond2, js.selcol.c_str());
    if (jn == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str()
	    << ": unable to construct an ibis::join object";
	return;
    }

    int64_t nhits = jn->count();
    LOGGER(ibis::gVerbose >= 0)
	<< oss.str() << " -- counted " << nhits << " hit" << (nhits>1?"s":"");
    if (nhits <= 0 || js.selcol.empty()) {
	delete jn;
	return;
    }

    ibis::table *res = jn->select();
    if (res == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str()
	    << ": failed to create a table representing the join result";
	delete jn;
	return;
    }

    // print the columns name
    res->describe(std::cout);
    size_t nprint = ((nhits >> ibis::gVerbose) > 2 ? (2 << ibis::gVerbose) :
		     nhits);
    // print the first few rows of the result
    int ierr = res->dump(std::cout, nprint);
    LOGGER(ierr < 0 && ibis::gVerbose > 0)
	<< "Warning -- " << oss.str() << ": failed to print " << nprint
	<< " row" << (nprint > 1 ? "s" : "")
	<< "from the joined table, ierr = " << ierr;
    delete res;
    delete jn;
} // doJoin

static void readInts(const char* fname, std::vector<uint32_t> &ints) {
    std::ifstream sfile(fname);
    if (! sfile) {
	LOGGER(ibis::gVerbose >= 0)
	    << "readInts unable to open file \"" << fname
	    << "\" for reading";
	return;
    }

    uint32_t tmp;
    while (sfile >> tmp) {
	ints.push_back(tmp);
    }
} // readInts

static void doDeletion() {
    if (yankstring == 0 || *yankstring == 0) return;

    if (ibis::util::getFileSize(yankstring) > 0) {
	// assume the file contain a list of numbers that are row numbers
	std::vector<uint32_t> rows;
	readInts(yankstring, rows);
	if (rows.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "doDeletion -- file \"" << yankstring
		<< "\" does not start with integers, integer expected";
	    return;
	}
	LOGGER(ibis::gVerbose > 0)
	    << "doDeletion will invoke deactive on " << ibis::datasets.size()
	    << " data partition" << (ibis::datasets.size() > 1 ? "s" : "")
	    << " with " << rows.size() << " row number"
	    << (rows.size() > 1 ? "s" : "");

	for (ibis::partList::iterator it = ibis::datasets.begin();
	     it != ibis::datasets.end(); ++ it) {
	    long ierr = (*it)->deactivate(rows);
	    LOGGER(ibis::gVerbose >= 0)
		<< "doDeletion -- deactivate(" << (*it)->name()
		<< ") returned " << ierr;
	    if (zapping) {
		ierr = (*it)->purgeInactive();
		LOGGER(ibis::gVerbose > 0 || ierr < 0)
		    << "doDeletion purgeInactive(" << (*it)->name()
		    << ") returned " << ierr;
	    }
	}
    }
    else {
	LOGGER(ibis::gVerbose > 0)
	    << "doDeletion will invoke deactive on " << ibis::datasets.size()
	    << " data partition" << (ibis::datasets.size() > 1 ? "s" : "")
	    << " with \"" << yankstring << "\"";

	for (ibis::partList::iterator it = ibis::datasets.begin();
	     it != ibis::datasets.end(); ++ it) {
	    long ierr = (*it)->deactivate(yankstring);
	    LOGGER(ibis::gVerbose >= 0)
		<< "doDeletion -- deactivate(" << (*it)->name()
		<< ", " << yankstring << ") returned " << ierr;

	    if (zapping) {
		ierr = (*it)->purgeInactive();
		LOGGER(ibis::gVerbose > 0 || ierr < 0)
		    << "doDeletion purgeInactive(" << (*it)->name()
		    << ") returned " << ierr;
	    }
	}
    }
    zapping = false;
} // doDeletion

static void reverseDeletion() {
    if (keepstring == 0 || *keepstring == 0) return;

    if (ibis::util::getFileSize(keepstring) > 0) {
	// assume the file contain a list of numbers that are row numbers
	std::vector<uint32_t> rows;
	readInts(keepstring, rows);
	if (rows.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "reverseDeletion -- file \"" << keepstring
		<< "\" does not start with integers, integer expected";
	    return;
	}
	LOGGER(ibis::gVerbose > 0)
	    << "reverseDeletion will invoke deactive on "
	    << ibis::datasets.size()
	    << " data partition" << (ibis::datasets.size() > 1 ? "s" : "")
	    << " with " << rows.size() << " row number"
	    << (rows.size() > 1 ? "s" : "");

	for (ibis::partList::iterator it = ibis::datasets.begin();
	     it != ibis::datasets.end(); ++ it) {
	    long ierr = (*it)->reactivate(rows);
	    LOGGER(ibis::gVerbose >= 0)
		<< "reverseDeletion -- reactivate(" << (*it)->name()
		<< ") returned " << ierr;
	}
    }
    else {
	LOGGER(ibis::gVerbose > 0)
	    << "reverseDeletion will invoke deactive on "
	    << ibis::datasets.size()
	    << " data partition" << (ibis::datasets.size() > 1 ? "s" : "")
	    << " with \"" << keepstring << "\"";

	for (ibis::partList::iterator it = ibis::datasets.begin();
	     it != ibis::datasets.end(); ++ it) {
	    long ierr = (*it)->reactivate(keepstring);
	    LOGGER(ibis::gVerbose >= 0)
		<< "reverseDeletion -- reactivate(" << (*it)->name()
		<< ", " << keepstring << ") returned " << ierr;
	}
    }
} // reverseDeletion

// parse the query string and evaluate the specified query
static void parseString(const char* uid, const char* qstr) {
    if (qstr == 0) return;
    if (*qstr == 0) return;

    // got a valid string
    const char* str = qstr;
    const char* end;
    std::string fstr; // from clause
    std::string sstr; // select clause
    std::string wstr; // where clause
    std::string ordkeys; // order by clause (the order keys)
    uint32_t start = 0; // the 1st row to print
    uint32_t limit = 0; // the limit on the number of output rows
    const bool hasdot = (strchr(qstr, '.') != 0);

    // skip leading space
    while (isspace(*str)) ++str;
    // look for key word SELECT
    if (0 == strnicmp(str, "select ", 7)) {
	str += 7;
	while (isspace(*str)) ++str;
	// look for the next key word (either FROM or WHERE)
	end = strstr(str, " from ");
	if (end == 0) {
	    end = strstr(str, " FROM ");
	    if (end == 0)
		end = strstr(str, " From ");
	}
	if (end) { // found FROM clause
	    while (str < end) {
		sstr += *str;
		++ str;
	    }
	    str = end + 1;
	}
	else { // no FROM clause, try to locate WHERE
	    end = strstr(str, " where ");
	    if (end == 0) {
		end = strstr(str, " WHERE ");
		if (end == 0)
		    end = strstr(str, " Where ");
	    }
	    if (end == 0) {
		sstr = str;
		str = 0;
	    }
	    else {
		while (str < end) {
		    sstr += *str;
		    ++ str;
		}
		str = end + 1;
	    }
	}
    }

    // look for key word FROM
    if (str != 0 && 0 == strnicmp(str, "from ", 5)) {
	str += 5;
	while (isspace(*str)) ++str;
	end = strstr(str, " where "); // look for key word WHERE
	if (end == 0) {
	    end = strstr(str, " WHERE ");
	    if (end == 0)
		end = strstr(str, " Where ");
	}
	if (end == 0 && sstr.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "parseString(" << qstr << ") is unable to locate "
		<< "key word WHERE following FROM clause";
	    return;
	}
	else if (end != 0) {
	    fstr.append(str, end);
	    str = end + 1;
	}
	else {
	    fstr = str;
	    str = 0;
	}
    }

    // check for the WHERE clause
    if (str == 0 || *str == 0) {
	if (sstr.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Unable to fund a where clause in the query string \""
		<< qstr << "\"";
	    return;
	}
    }
    else if (0 == strnicmp(str, "where ", 6)) {
	str += 6;
    }
    else if (ibis::gVerbose > 1) {
	ibis::util::logger lg;
	lg() << "parseString(" << qstr
	     << ") is unable to locate key word WHERE.  "
	     << "assume the string is the where clause.";
    }
    // the end of the where clause is marked by the key words "order by" or
    // "limit" or the end of the string
    if (str != 0) {
	end = strstr(str, "order by");
	if (end == 0) {
	    end = strstr(str, "ORDER BY");
	    if (end == 0)
		end = strstr(str, "Order By");
	    if (end == 0)
		end = strstr(str, "Order by");
	    if (end == 0)
		end = strstr(str, "limit");
	    if (end == 0)
		end = strstr(str, "Limit");
	    if (end == 0)
		end = strstr(str, "LIMIT");
	}
	if (end != 0) {
	    while (str < end) {
		wstr += *str;
		++ str;
	    }
	}
	else {
	    while (*str != 0) {
		wstr += *str;
		++ str;
	    }
	}
    }

    if (str != 0 && 0 == strnicmp(str, "order by ", 9)) { // order by clause
	// the order by clause may be the last clause or followed by the
	// key word "LIMIT"
	str += 9;
	end = strstr(str, "limit");
	if (end == 0) {
	    end = strstr(str, "Limit");
	    if (end == 0)
		end = strstr(str, "LIMIT");
	}
	if (end != 0) {
	    while (str < end) {
		ordkeys += *str;
		++ str;
	    }
	}
	else {
	    while (*str) {
		ordkeys += *str;
		++ str;
	    }
	}
    }
    while (str != 0 && *str && isspace(*str)) // skip blank spaces
	++ str;
    if (str != 0 && 0 == strnicmp(str, "limit ", 6)) {
	str += 6;
	uint64_t tmp;
	int ierr = ibis::util::readUInt(tmp, str, ", ");
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- parseString(" << qstr
		<< ") expects a unsigned interger following "
		"the keyword LIMIT, but got '" << *str
		<< "', skip the limit clause";
	}
	else if (isspace(*str) || *str == ',') {
	    for (++ str; *str != 0 && (isspace(*str) || *str == ','); ++ str);
	    limit = static_cast<uint32_t>(tmp);
	    ierr = ibis::util::readUInt(tmp, str, 0);
	    if (ierr >= 0) {
		start = limit;
		limit = static_cast<uint32_t>(tmp);
	    }
	}
	else if (*str == 0) {
	    limit = static_cast<uint32_t>(tmp);
	}
	else {
	    ibis::util::logger()()
		<< "Warning -- parseString(" << qstr
		<< ") reached a unexpected end of string \"" << str << "\"";
	}
    }
    else if (str != 0 && *str != 0 && ibis::gVerbose >= 0) {
	ibis::util::logger lg;
	lg() << "Warning -- parseString(" << qstr
	     << ") expects the key word LIMIT, but got " << str;
    }

    ibis::nameList qtables(fstr.c_str());
    if (hasdot) {
	doQuaere(sstr.c_str(), fstr.c_str(), wstr.c_str(),
		 ordkeys.c_str(), limit, start);
    }
    else if (! sstr.empty() && (sstr.find('(') < sstr.size() ||
				sstr.find(" as ") < sstr.size())) {
	//  || verify_rid || !ordkeys.empty() || limit > 0
	// more complex select clauses need tableSelect
	if (! qtables.empty()) {
	    ibis::partList tl2;
	    for (unsigned k = 0; k < ibis::datasets.size(); ++ k) {
		for (unsigned j = 0; j < qtables.size(); ++ j) {
		    if (stricmp(ibis::datasets[k]->name(), qtables[j]) == 0 ||
			ibis::util::strMatch(ibis::datasets[k]->name(),
					     qtables[j])) {
			tl2.push_back(ibis::datasets[k]);
			break;
		    }
		}
	    }
	    try {
		tableSelect(tl2, uid, wstr.c_str(), sstr.c_str(),
			    ordkeys.c_str(), limit, start);
	    }
	    catch (...) {
		if (ibis::util::serialNumber() % 3 == 0) {
		    ibis::util::quietLock lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
		    sleep(1);
#endif
		}
		for (ibis::partList::iterator it = tl2.begin();
		     it != tl2.end(); ++ it) {
		    (*it)->emptyCache();
		}
		tableSelect(tl2, uid, wstr.c_str(), sstr.c_str(),
			    ordkeys.c_str(), limit, start);
	    }
	}
	else {
	    try {
		tableSelect(ibis::datasets, uid, wstr.c_str(), sstr.c_str(),
			    ordkeys.c_str(), limit, start);
	    }
	    catch (...) {
		if (ibis::util::serialNumber() % 3 == 0) {
		    ibis::util::quietLock lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
		    sleep(1);
#endif
		}
		for (ibis::partList::iterator it = ibis::datasets.begin();
		     it != ibis::datasets.end(); ++ it) {
		    (*it)->emptyCache();
		}
		tableSelect(ibis::datasets, uid, wstr.c_str(), sstr.c_str(),
			    ordkeys.c_str(), limit, start);
	    }
	}
    }
    else if (! qtables.empty()) {
	// simple select clauses can be handled through doQuery
	for (unsigned k = 0; k < ibis::datasets.size(); ++ k) {
	    // go through each partition the user has specified
	    for (unsigned j = 0; j < qtables.size(); ++ j) {
		if (stricmp(ibis::datasets[k]->name(), qtables[j]) == 0 ||
		    ibis::util::strMatch(ibis::datasets[k]->name(),
					 qtables[j])) {
		    if (verify_rid || sequential_scan ||
			ibis::datasets[k]->getMeshShape().empty()) {
			try {
			    doQuery(ibis::datasets[k], uid, wstr.c_str(),
				     sstr.c_str(), ordkeys.c_str(),
				    limit, start);
			}
			catch (...) {
			    if (ibis::util::serialNumber() % 3 == 0) {
				ibis::util::quietLock
				    lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
				sleep(1);
#endif
			    }
			    ibis::datasets[k]->emptyCache();
			    doQuery(ibis::datasets[k], uid, wstr.c_str(),
				    sstr.c_str(), ordkeys.c_str(),
				    limit, start);
			}
		    }
		    else {
			try {
			    doMeshQuery(ibis::datasets[k], uid, wstr.c_str(),
					sstr.c_str());
			}
			catch (...) {
			    if (ibis::util::serialNumber() % 3 == 0) {
				ibis::util::quietLock
				    lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
				sleep(1);
#endif
			    }
			    ibis::datasets[k]->emptyCache();
			    doMeshQuery(ibis::datasets[k], uid, wstr.c_str(),
					sstr.c_str());
			}
		    }

		    if (ibis::gVerbose > 7 || testing > 0) {
			try {
			    xdoQuery(ibis::datasets[k], uid, wstr.c_str(), sstr.c_str());
			}
			catch (...) {
			    if (ibis::util::serialNumber() % 3 == 0) {
				ibis::util::quietLock
				    lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
				sleep(1);
#endif
			    }
			    ibis::datasets[k]->emptyCache();
			}
		    }
		    break;
		}
	    }
	}
    }
    else {
	for (ibis::partList::iterator tit = ibis::datasets.begin();
	     tit != ibis::datasets.end(); ++ tit) {
	    // go through every partition and process the user query
	    if (verify_rid || sequential_scan ||
		(*tit)->getMeshShape().empty()) {
		try {
		    doQuery((*tit), uid, wstr.c_str(), sstr.c_str(),
			    ordkeys.c_str(), limit, start);
		}
		catch (...) {
		    if (ibis::util::serialNumber() % 3 == 0) {
			ibis::util::quietLock lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
			sleep(1);
#endif
		    }
		    (*tit)->emptyCache();
		    doQuery((*tit), uid, wstr.c_str(), sstr.c_str(),
			    ordkeys.c_str(), limit, start);
		}
	    }
	    else {
		try {
		    doMeshQuery((*tit), uid, wstr.c_str(), sstr.c_str());
		}
		catch (...) {
		    if (ibis::util::serialNumber() % 3 == 0) {
			ibis::util::quietLock lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
			sleep(1);
#endif
		    }
		    (*tit)->emptyCache();
		    doMeshQuery((*tit), uid, wstr.c_str(), sstr.c_str());
		}
	    }

	    if (ibis::gVerbose > 7 || testing > 0) {
		try {
		    xdoQuery((*tit), uid, wstr.c_str(), sstr.c_str());
		}
		catch (...) {
		    if (ibis::util::serialNumber() % 3 == 0) {
			ibis::util::quietLock lock(&ibis::util::envLock);
#if defined(unix) || defined(linux) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD)
			sleep(1);
#endif
		    }
		    (*tit)->emptyCache();
		}
	    }
	}
    }
} // parseString

extern "C" void* thFun(void* arg) {
    thArg* myArg = (thArg*)arg; // recast the argument to the right type
    for (unsigned j = myArg->task(); j < myArg->qlist.size();
	 j = myArg->task()) {
	LOGGER(ibis::gVerbose > 0) << " ... processing qlist[" << j << "]";
	parseString(myArg->uid, myArg->qlist[j]);
    }
    return 0;
}

// read a line inputed from the user
static void readInput(std::string& str) {
    str.erase(); // empty the current content
    int wait = 0;
    char buf[MAX_LINE];
    do {
	std::cout << (wait ? "more > " : "ibis > ");
	std::flush(std::cout);

	if (0 == fgets(buf, MAX_LINE, stdin)) *buf = 0;
	// remove trailing space
	char* tmp = buf + strlen(buf) - 1;
	while (tmp>=buf && isspace(*tmp)) {
	    *tmp = 0; -- tmp;
	}

	if (tmp < buf) {
	    wait = 1;
	}
	else {
	    wait = 0;
	    if (*tmp == '\\') {
		int cnt = 0;
		char* t2 = tmp;
		while (t2 > buf && *t2 == '\\') {
		    --t2; ++cnt;
		}
		wait = (cnt % 2);
		if (wait) *tmp = ' ';
	    }
	    str += buf + strspn(buf, " \t");
	}
    } while (wait);
} // readInput

/// Generate random queries for testing.
static void randomQueries(std::vector<const char*> &qlist,
			  std::vector<std::string> &queff) {
    if (ibis::datasets.empty()) return;
    qlist.clear();
    queff.clear();

#define maxselect 8
#define maxwhere 5
    const char selstr[][maxselect] =
	{"floor", "sum", "stdev", "avg", "ceil", "min", "max", "var"};
    ibis::part::info p0(*ibis::datasets[0]);
    ibis::MersenneTwister mt; // a random number generator
    const unsigned mq = (threading > 1 ? threading : 2) *
	(testing > 0 ? testing+1 : 5);
    for (unsigned j = 0; j < mq; ++ j) {
	std::ostringstream oss;
	unsigned nsel = mt.next(maxselect * maxwhere);
	unsigned nwhr = 1 + nsel % maxwhere;
	nsel = nsel / maxwhere;
	oss << "SELECT count(*) as _0";
	while (nsel > 0) {
	    -- nsel;
	    const unsigned ic = mt.next(p0.cols.size());
	    oss << ", " << selstr[nsel] << '(' << p0.cols[ic]->name;
	    if (nsel == 0) {
		const unsigned den = 2 + mt.next(4);
		oss << " % " << den;
	    }
	    else if (nsel == 4) {
		const unsigned den = 3 + mt.next(7);
		oss << " % " << den;
	    }
	    oss << ')';
	}
	oss << " WHERE ";
	for (unsigned j = 0; j < nwhr; ++ j) {
	    unsigned ic = mt.next(p0.cols.size());
	    if (p0.cols[ic]->type == ibis::BLOB ||
		p0.cols[ic]->type == ibis::TEXT) {
		unsigned jc = ic + 1;
		if (jc >= p0.cols.size()) jc = 0;
		while ((p0.cols[jc]->type == ibis::BLOB ||
			p0.cols[jc]->type == ibis::TEXT) && jc != ic) {
		    ++ jc;
		    if (jc >= p0.cols.size()) jc = 0;
		}
		if (jc == ic) {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- function randomQueries can not find "
			"a suitable column to form queries using data "
			"partition " << p0.name;
		    return;
		}
		ic = jc;
	    }

	    double b0 = p0.cols[ic]->expectedMin + mt() *
		(p0.cols[ic]->expectedMax - p0.cols[ic]->expectedMin);
	    double b1 = p0.cols[ic]->expectedMin + mt() *
		(p0.cols[ic]->expectedMax - p0.cols[ic]->expectedMin);
	    if (b0 > b1) {
		double t = b0;
		b0 = b1;
		b1 = t;
	    }
	    if (j > 0)
		oss << " and ";
	    if (p0.cols[ic]->type != ibis::DOUBLE &&
		p0.cols[ic]->type != ibis::FLOAT &&
		b1 <= b0 + 1.0) {
		oss << ceil(b0) << " == " << p0.cols[ic]->name;
	    }
	    else if (b0 < b1) {
		oss << b0 << " <= " << p0.cols[ic]->name
		    << " < " << b1;
	    }
	    else if (b0 == b1) {
		oss << b0 << " == " << p0.cols[ic]->name;
	    }
	    else {
		oss << p0.cols[ic]->name << " > 0";
	    }
	}
	oss << " ORDER BY _0 desc";

	queff.push_back(oss.str());
    }

    qlist.reserve(queff.size());
    for (unsigned j = 0; j < queff.size(); ++ j)
	qlist.push_back(queff[j].c_str());
    LOGGER(ibis::gVerbose > 2)
	<< "randomQueries generated " << qlist.size() << " random quer"
	<< (qlist.size()>1?"ies":"y");
} // randomQueries

static void clean_up(bool sane=true) {
    { // use envLock to make sure only one thread is deleting the partitions
	ibis::util::quietLock lock(&ibis::util::envLock);
	const size_t np = ibis::datasets.size();
	if (np == 0) return;
	for (unsigned i = 0; i < np; ++ i) {
	    delete ibis::datasets[i];
	    ibis::datasets[i] = 0;
	}
	ibis::datasets.clear();
    }
    LOGGER(ibis::gVerbose > 1)
	<< "Cleaning up the file manager\n"
	"Total pages accessed through read(unistd.h) is estimated to be "
	<< ibis::fileManager::instance().pageCount();

    if (sane)
	ibis::fileManager::instance().clear();
    if (ibis::gVerbose > 2) {
	ibis::util::logger lg;
	ibis::fileManager::instance().printStatus(lg());
    }

#if defined(RUSAGE_SELF) && defined(RUSAGE_CHILDREN)
    if (ibis::gVerbose > 1) {
	// getrusage might not fill all the fields
	struct rusage ruse0, ruse1;
	int ierr = getrusage(RUSAGE_SELF, &ruse0);
	ierr |= getrusage(RUSAGE_CHILDREN, &ruse1);
	LOGGER(ierr == 0)
	    << "Report from getrusage: maxrss = "
	    << ruse0.ru_maxrss + ruse1.ru_maxrss
	    << " pages (" << getpagesize() << " bytes/page)"
	    << ", majflt = " << ruse0.ru_majflt + ruse1.ru_majflt
	    << ", minflt = " << ruse0.ru_minflt + ruse1.ru_minflt
	    << ", inblock = " << ruse0.ru_inblock + ruse1.ru_inblock
	    << ", outblock = " << ruse0.ru_oublock + ruse1.ru_oublock;
    }
#endif
#if defined(_MSC_VER) && defined(_WIN32) && (defined(_DEBUG) || defined(DEBUG))
    std::cout << "\n*** DEBUG: report from _CrtMemDumpAllObjectsSince\n";
    _CrtMemDumpAllObjectsSince(NULL);
    _CrtDumpMemoryLeaks();
#endif
} // clean_up

int main(int argc, char** argv) {
    if (argc <= 1) {
	usage(*argv);
	return 0;
    }

    try {
	int interactive;
	std::vector<const char*> alist, qlist, slist;
	ibis::joinlist joins;
	std::vector<std::string> queff; // queries read from files (-f)
	const char* uid = ibis::util::userName();
	ibis::horometer timer; // total elapsed time
	timer.start();

	// parse the command line arguments
	parse_args(argc, argv, interactive, alist, slist, qlist,
		   queff, joins);

	// append new data one directory at a time, the same directory may
	// be used many times, all incoming directories appended to the
	// same output directory specified by appendto
	for (std::vector<const char*>::const_iterator it = alist.begin();
	     it != alist.end();
	     ++ it) { // add new data before doing anything else
	    doAppend(*it);
	}
	alist.clear(); // no more use for it

	if (yankstring != 0 && *yankstring != 0)
	    doDeletion();
	if (keepstring != 0 && *keepstring != 0)
	    reverseDeletion();

	// build new indexes
	if (build_index > 0 && ! ibis::datasets.empty()) {
	    LOGGER(ibis::gVerbose > 0)
		<< *argv << ": start building indexes (nthreads="
		<< build_index << ", indexingOption="
		<< (indexingOption ? indexingOption : "-") << ") ...";
	    ibis::horometer timer1;
	    timer1.start();
	    for (ibis::partList::const_iterator it = ibis::datasets.begin();
		 it != ibis::datasets.end(); ++ it) {
		if (indexingOption != 0 &&
		    ((*it)->indexSpec() == 0 ||
		     stricmp(indexingOption, (*it)->indexSpec()) != 0)) {
		    (*it)->indexSpec(indexingOption);
		    (*it)->purgeIndexFiles();
		}
		else if (zapping) {
		    (*it)->purgeIndexFiles();
		}
		(*it)->buildIndexes(indexingOption, build_index);
		//(*it)->loadIndexes(indexingOption);
	    }
	    timer1.stop();
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << ": building indexes for " << ibis::datasets.size()
		<< " data partition" << (ibis::datasets.size()>1 ? "s" : "")
		<< " took " << timer1.CPUTime() << " CPU seconds, "
		<< timer1.realTime() << " elapsed seconds\n";
	    zapping = false;
	}
	// sort the specified columns
	if (slist.size() > 0) {
	    ibis::horometer timer2;
	    timer2.start();
	    for (ibis::partList::const_iterator it = ibis::datasets.begin();
		 it != ibis::datasets.end(); ++ it) {
		for (size_t j = 0; j < slist.size(); ++ j)
		    (*it)->buildSorted(slist[j]);
	    }
	    timer2.stop();
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << ": building sorted versions of " << slist.size()
		<< " column" << (slist.size()>1 ? "s" : "")
		<< ibis::datasets.size() << " data partition"
		<< (ibis::datasets.size()>1 ? "s" : "") << " took "
		<< timer2.CPUTime() << " CPU seconds, "
		<< timer2.realTime() << " elapsed seconds\n";
	    slist.clear(); // no longer needed
	}

	if (testing > 0 && ! ibis::datasets.empty() && threading > 0 &&
	    qlist.empty()) { // generate random queries
	    randomQueries(qlist, queff);
	}
	else if (testing > 0 && ! ibis::datasets.empty()) {
	    // performing self test
	    LOGGER(ibis::gVerbose > 0) << *argv << ": start testing ...";
	    ibis::horometer timer3;
	    timer3.start();
	    for (ibis::partList::const_iterator it = ibis::datasets.begin();
		 it != ibis::datasets.end(); ++ it) {
		// tell the partition to perform self tests
		long nerr = (*it)->selfTest(testing);
		(*it)->unloadIndexes();

		if (ibis::gVerbose >= 0) {
		    ibis::util::logger lg;
		    lg() << "self tests on " << (*it)->name();
		    if (nerr == 0)
			lg() << " found no error";
		    else if (nerr == 1)
			lg() << " found 1 error";
		    else if (nerr > 1)
			lg() << " found " << nerr << " errors";
		    else
			lg() << " returned unexpected value " << nerr;
		}
	    }
	    timer3.stop();
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << ": testing " << ibis::datasets.size()
		<< " data partition" << (ibis::datasets.size()>1 ? "s" : "")
		<< " took " << timer3.CPUTime() << " CPU seconds, "
		<< timer3.realTime() << " elapsed seconds\n";
	}

	if (ibis::datasets.empty() && !qlist.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << " must have at least one data partition "
		"to process any query.";
	}
	else if (qlist.size() > 1 && threading > 1) {
#if defined(_DEBUG) || defined(DEBUG)
	    for (std::vector<const char*>::const_iterator it = qlist.begin();
		 it != qlist.end(); ++it) {
		parseString(uid, *it);
	    }
#else
	    // process queries in a thread pool
	    const int nth =
		(threading <= qlist.size() ? threading : qlist.size()) - 1;
	    ibis::util::counter taskpool;
	    thArg args(uid, qlist, taskpool);
	    std::vector<pthread_t> tid(nth);
	    for (int i = 0; i < nth; ++ i) { // 
		int ierr = pthread_create(&(tid[i]), 0, thFun, (void*)&args);
		if (ierr != 0) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- pthread_create failed to create thread "
			<< i << " -- " << strerror(ierr);
		    return(-5);
		}
	    }
	    thFun((void*)&args); // this thread do something too
	    for (int i = 0; i < nth; ++ i) {
		void *status;
		int ierr = pthread_join(tid[i], (void**)&status);
		LOGGER(ibis::gVerbose >= 0 && ierr != 0)
		    << "pthread_join failed on thread " << i
		    << " -- " << strerror(ierr);
	    }
#endif
	    queff.clear();
	    qlist.clear();
	}
	else if (qlist.size() > 0) { // no new threads
	    for (std::vector<const char*>::const_iterator it = qlist.begin();
		 it != qlist.end(); ++it) {
		parseString(uid, *it);
	    }
	    queff.clear();
	    qlist.clear();
	}
	else if (ridfile != 0) {
	    for (ibis::partList::iterator itt = ibis::datasets.begin();
		 itt != ibis::datasets.end();
		 ++ itt)
		doQuery((*itt), uid, 0, 0, 0, 0, 0);
	}
	ridfile = 0;

	// process the joins one at a time
	for (size_t j = 0; j < joins.size(); ++j) {
	    doJoin(uid, *joins[j]);
	}

	if (interactive) {	// interactive operations
	    std::string str;
	    if (ibis::gVerbose >= 0) {
		// entering the interactive mode, print the help message
		std::cout << "\nEntering interactive mode\n";
		help(*argv);
	    }

	    while (1) {
		readInput(str);
		switch (*(str.c_str())) {
		case 'h': // help
		case 'H':
		case '?':
		default:
		    help(*argv);
		break;
		case 'e': // exit
		case 'E':
		case 'q':
		case 'Q':
		    clean_up(true);
		return(0);
		case 'p': // print command
		case 'P':
		    print(str.c_str()); break;
		case 's': // a select statement
		case 'f':
		case 'w':
		case 'S':
		case 'F':
		case 'W': {
		    parseString(uid, str.c_str());
		    break;}
		case 'a':
		case 'A': {
		    const char* dir = str.c_str();
		    while(isalpha(*dir)) ++dir; // skip key word append
		    while(isspace(*dir)) ++dir; // skip space
		    doAppend(dir);
		    break;}
		}
	    }
	}

	timer.stop();
	LOGGER(timer.realTime() > 0.001 && ibis::gVerbose > 0)
	    << *argv << " -- total CPU time " << timer.CPUTime()
	    << " s, total elapsed time " << timer.realTime() << " s";

	clean_up(true);
	// last thing -- close the file logging the messages
	//ibis::util::closeLogFile();
	return 0;
    }
    catch (const std::exception& e) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** " << *argv
	    << " received a standard exception\n" << e.what();
	return -10;
    }
    catch (const char* s) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** " << *argv
	    << " received a string exception\n" << s;
	return -11;
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** " << *argv
	    << " received an unexpected exception";
	return -12;
    }
} // main

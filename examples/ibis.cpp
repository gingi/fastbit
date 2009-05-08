// $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2001-2009 the Regents of the University of California
//
/** @file ibis.cpp

   IBIS -- Interactive Bitmap Index Search

   A sample code that exercises the main features of the FastBit bitmap
   indexing search capabilities in this directory.  It provides the basic
   functionality of creating a database, accepts a limited version of SQL
   for query processing.  The queries may be entered either as command line
   arguments or from standard input.

   The queries are specified in a much simplified SQL format.  A query is a
   basically a SQL select statement of the form
   [SELECT ...] [FROM ...] WHERE ... [ORDER BY ... [ASC | DESC]] [LIMIT ...]

   The SELECT clause contains a list of column names and any of the four
   one-argument functions, AVG, MAX, MIN, and SUM, e.g., "SELECT a, b,
   AVG(c), MIN(d)."  If specified, the named columns of qualified records
   will be displayed as the result of the query.  The unqualified variables
   will be used to group the selected records; for each group the values of
   the functions are evaluated.  This is equivalent to use all unqualified
   variables in the "GROUP BY" clause.  Note the print out always orders
   the unqualified variables first followed by the values of the functions.
   It always has an implicit "count(*)" as the end of each line of print
   out.

   The FROM clause contains a list of data partition names.  If specified,
   the search will be performed only on the named partitions.  Otherwise,
   the search is performed on all known tables.

   The column names and partition names can be delimited by either ',', or ';'.
   The leading space and trailing space of each name will be removed but
   space in the middle of the names will be preserved.

   The WHERE clause specifies the condition of the query.  It is specified
   as range queries of the form

   RANGE LOGICAL_OP RANGE
   LOGICAL_OP can be one of "and", "or", "xor", "minus", "&&", "&",
   "||", "|", "^", "-"

   A range is specifed on one column of the form "ColumnA CMP Constant"
   CMP can be one of =, ==, !=, >, >=, <, <=
   The ranges and expressions can also be negated with either '!' or '~'.

   The expressions in the ORDER BY clause must be a proper subset of the
   SELECT clause.  The modifiers ASC and DESC are optional.  By default ASC
   (ascending) order is used.  One may use DESC to change to use the
   descending order.

   The LIMIT clause limits the maximum number of output rows.  Only number
   may follow the LIMIT keyword.  This clause has effects only if the
   preceeding WHERE clause selected less than or equal to the specified
   number of rows (after applying the implicit group by clause).


   Command line options:
   ibis [-a[ppend] data_dir [to partition_name]]
	[-c[onf] conf_file] [-d[atadir] data_dir]
        [-q[uery] [SELECT ...] [FROM ...] WHERE ... [ORDER BY ...] [LIMIT ...]]
        [-ou[tput-file] filename] [-l logfilename] [-i[nteractive]]
        [-b[uild-indexes]] [-k[eep-tempory-files]]
 	[-n[o-estimation]] [-e[stimation-only]] [-s[quential-scan]]
        [-r[id-check] [filename]] [-reorder data_dir]
        [-v[=n]] [-t[est]] [-h[elp]]

   NOTE: options -one-step-evaluation and -estimation-only are mutually
   exclusive, the one that appears later will overwrite the one that
   appears early on the same command line.

   NOTE: option -t is interpreted as self-testing if specified alone, if
   any query is also specified, it is interpreted as indicating the number
   of threads to use.

   NOTE: the select clause of "count(*)" will produce a result table with
   one row and one column to hold the content of "count(*)" following the
   SQL standard.  This may take some getting used too since one might have
   expect the number of hits to be printed directly as in the case of
   omitting the select clause.

   @ingroup FastBitExamples
*/
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include <ibis.h>
#include <mensa.h>	// ibis::mensa
#include <sstream>	// std::ostringstream
#include <algorithm>	// std::sort

// local data types
typedef std::vector<const char*> stringList;

/// The data structure for holding information about query jobs for
/// multi-threaded testing.
struct thArg {
    const char* uid;
    const stringList& qlist;
    ibis::partList tlist;
    ibis::util::counter& task;

    thArg(const char* id, const stringList& ql, ibis::partList& tl,
	  ibis::util::counter& tc) : uid(id), qlist(ql), tlist(tl), task(tc) {}
};

// global varialbes defined in this file
static unsigned testing = 0;
static unsigned threading = 0;
static unsigned build_index = 0;
static bool estimate_only = false;
static bool skip_estimation = false;
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
	std::vector<const char*> selcol; ///< Selected columns.

	joinspec() : part1(0), part2(0), jcol(0), cond1(0), cond2(0) {}
	void print(std::ostream& out) const;
    }; // joinspec
    typedef std::vector<joinspec> joinlist;

    /// A temporary holder of data partitions for query evaluations.
    class mensa2 : public mensa {
    public:
	mensa2(const partList&);
	~mensa2();

    private:
	// limit the automatically generated constructors
	mensa2();
	mensa2(const mensa2&);
	mensa2& operator=(const mensa2&);
    }; // mensa2
}

// printout the usage string
static void usage(const char* name) {
    std::cout << "usage:\n" << name << " [-c[onf] conf_file] "
	"[-d[atadir] data_dir] [-i[nteractive]]\n"
	"[-q[uery] [SELECT ...] [FROM ...] WHERE ...]\n"
	"[-j[oin] part1 part2 join-column conditions1 conditions2 [columns ...]]\n"
	"[-ou[tput-file] filename] [-l logfilename] "
	"[-s[quential-scan]] [-r[id-check] [filename]]\n"
	"[-n[o-estimation]] [-e[stimation-only]] [-k[eep-temporary-files]]"
	"[-a[ppend] data_dir [partition_name]] [-reorder data_dir]\n"
	"[-b[uild-indexes] [numThreads|indexSpec] -z[ap-existing-indexes]]\n"
	"[-v[=n]] [-t[=n]] [-h[elp]] [-y[ank] filename|conditions]\n\n"
	"NOTE: multiple -c -d -q and -v options may be specified.  "
	"Queries are applied to all data partitions by default.  "
	"Verboseness levels are cumulated.\n\n"
	"NOTE: options -n and -e are mutually exclusive, the one that appears, "
	"a later one overwrites the earlier one on the same command line.\n"
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
        "program.  Use with care.\n\n"
	"NOTE: the output file stores the results selected by queries, the "
	"log file is for the rest of the messages such error messages and "
	"debug information\n"
	      << std::endl;
} // usage

// printout the help message
static void help(const char* name) {
    std::cout << name << " accepts the following commands:\n"
	"help, exit, quit, append\nand query of the form\n\n"
	"[SELECT column_names] [FROM dataset_names] WHERE ranges\n\n"
	"The WHERE clause of a query must be specified.  "
	"It is used to determine\nwhat records qualify the query.\n"
	"If SELECT clause is present in a query, the qualified "
	"records named\ncolumns will be printed, otherwise only "
	"information about number of\nhits will be printed.\n"
	"If FROM clause is present, the WHERE clause will be "
	"only apply on the\nnamed datasets, otherwise, all "
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
    if (selcol.size() > 0) {
	out << "Select " << selcol[0];
	for (size_t i = 1; i < selcol.size(); ++ i)
	    out << ", " << selcol[i];
	out << " ";
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

std::ostream& operator<<(std::ostream& out, const ibis::joinspec& js) {
    js.print(out);
    return out;
}

// show column names
static void printNames(const ibis::partList& tlist) {
    ibis::part::info* tinfo;
    ibis::util::logger lg(0);
    for (ibis::partList::const_iterator it = tlist.begin();
	 it != tlist.end(); ++it) {
	tinfo = new ibis::part::info(**it);
	lg.buffer() << "Partition " << tinfo->name << ":\n";
	std::vector<ibis::column::info*>::const_iterator vit;
	for (vit = tinfo->cols.begin(); vit != tinfo->cols.end(); ++vit)
	    lg.buffer() << (*vit)->name << ' ';
	lg.buffer() << "\n";
	delete tinfo;
    }
} // printNames

// print all partitions and columns
static void printAll(const ibis::partList& tlist) {
    ibis::util::logger lg(0);
    ibis::partList::const_iterator it;
    for (it = tlist.begin(); it != tlist.end(); ++it)
	(*it)->print(lg.buffer());
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
	ibis::util::logger lg(0);
	lg.buffer() << "get1DDistribution return value (" << nb
		    << ") does match the size of array counts ("
		    << counts.size() << ") or bounds.size(" << bounds.size()
		    << ") does not equual to 1+counts.size (" << counts.size();
	return;
    }
    else {
	uint32_t tot = 0;
	ibis::util::logger lg(0);
	lg.buffer() << "Column " << cname << " in Partition "
		    << tbl.name() << ":\n";
	col->print(lg.buffer());
	lg.buffer() << ", actual range <" << amin << ", " << amax
		    << ">\nHistogram [" << nb << "]";
	if (cond != 0 && *cond != 0)
	    lg.buffer() << " under the condition of \"" << cond
			<< "\"";
	lg.buffer() << "\n(bounds,\t# records in bin)\n";
	for (int j = 0; j < nb; ++ j) {
	    if (! (fabs(bounds[j] - bounds[j+1]) >
		   1e-15*(fabs(bounds[j])+fabs(bounds[j+1]))))
		lg.buffer() << "*** Error *** bounds[" << j << "] ("
			    << bounds[j] << ") is too close to bounds[" << j+1
			    << "] (" << bounds[j+1] << ")\n";
	    lg.buffer() << "[" << bounds[j] << ", " << bounds[j+1] << ")\t"
			<< counts[j] << "\n";
	    tot += counts[j];
	}
	lg.buffer() << "  total count = " << tot << ", tbl.nRows() = "
		    << tbl.nRows();
    }
    if (nb > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<ibis::bitvector> bins;
	std::vector<double> boundt;
	ibis::util::logger lg(0);
	long ierr = tbl.get1DBins(cond, cname, nb, boundt, bins);
	lg.buffer() << "\nprintColumn(" << cname << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get1DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get1DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (bounds.size() != boundt.size() ||
		 counts.size() != bins.size()) {
	    lg.buffer() << "get1DDistribution returned " << counts.size()
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
		    lg.buffer() << "bounds[" << i << "] (" << bounds[i]
				<< ") != boundt[" << i << "] (" << boundt[i]
				<< ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < counts.size(); ++ i)
		if (bins[i].cnt() != counts[i]) {
		    lg.buffer() << "counts[" << i << "] (" << counts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays counts and bins produces "
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

	ibis::util::logger lg(0);
	lg.buffer() << "Column " << cname << " in Partition "
		    << tbl.name() << ":\n";
	if (nb > 0) {
	    col->print(lg.buffer());
	    lg.buffer() << ", actual range <" << amin << ", " << amax
			<< ">\ncumulative distribution [" << nb
			<< "]";
	    if (cond != 0 && *cond != 0)
		lg.buffer() << " under the condition of \"" << cond
			    << "\"";
	    lg.buffer() << "\n(bound,\t# records < bound)\n";
	    for (int j = 0; j < nb; ++ j) {
		if (j > 0 && ! (fabs(bounds[j] - bounds[j-1]) >
				1e-15*(fabs(bounds[j])+fabs(bounds[j-1]))))
		    lg.buffer() << "*** Error *** bounds[" << j
				<< "] is too close to bounds[" << j-1
				<< "]\n";
		lg.buffer() << bounds[j] << ",\t" << counts[j] << "\n";
	    }
	}
	else {
	    col->print(lg.buffer());
	    lg.buffer() << " -- getCumulativeDistribution(" << cname
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
	ibis::util::logger lg(0);
	lg.buffer() << "Partition " << tinfo.name << " (" << tinfo.description
		    << ") -- nRows=" << tinfo.nrows << ", nCols="
		    << tinfo.cols.size() << "\nColumn names: ";
	for (uint32_t i = 0; i < tinfo.cols.size(); ++ i) {
	    lg.buffer() << tinfo.cols[i]->name << " ";
	}
    }
    for (uint32_t i = 0; i < tinfo.cols.size(); ++ i) {
	double amin = tbl.getActualMin(tinfo.cols[i]->name);
	double amax = tbl.getActualMax(tinfo.cols[i]->name);
	long ierr = tbl.get1DDistribution(tinfo.cols[i]->name,
					  100, bounds, counts);

	ibis::util::logger lg(0); // use an IO lock
	lg.buffer() << "  Column " << tinfo.cols[i]->name << " ("
		    << tinfo.cols[i]->description << ") "
		    << ibis::TYPESTRING[tinfo.cols[i]->type]
		    << " expected range [" << tinfo.cols[i]->expectedMin
		    << ", " << tinfo.cols[i]->expectedMax << "]";
	if (ierr > 1) {
	    lg.buffer() <<", actual range <" << amin << ", " << amax
			<< ">\n # bins " << ierr << "\n";
	    for (int j = 0; j < ierr; ++ j) {
		if (! (fabs(bounds[j] - bounds[j+1]) >
		       1e-15*(fabs(bounds[j])+fabs(bounds[j+1]))))
		    lg.buffer() << "*** Error *** bounds[" << j << "] ("
				<< bounds[j] << ") is too close to bounds["
				<< j+1 << "] (" << bounds[j+1] << ")\n";
		lg.buffer() << "[" << bounds[j] << ", " << bounds[j+1] << ")\t"
			    << counts[j] << "\n";
	    }
	}
	else {
	    lg.buffer() << "\ngetCumulativeDistribution returned ierr="
			<< ierr << ", skip ...";
	}
    }
} // printDistribution

static void printDistribution(const ibis::partList& tlist) {
    ibis::partList::const_iterator it;
    for (it = tlist.begin(); it != tlist.end(); ++it) {
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n1D-Weighted distribution of " << col1
		    << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << weights.size() << " bin"
		    << (weights.size() > 1 ? "s" : "") << "\n";

	uint32_t cnt = 0;
	double tot = 0.0;
	for (uint32_t i = 0; i < weights.size(); ++ i) {
	    if (weights[i] > 0) {
		lg.buffer() << i << "\t[" << amin1+stride1*i << ", "
			    << amin1+stride1*(i+1)
			    << ")\t" << weights[i] << "\n";
		tot += weights[i];
		++ cnt;
	    }
	}
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total weight = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "Warning -- " << evt
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n" << evt << "-- \n";
	if (ierr < 0) {
	    lg.buffer() << "get1DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size() || ierr != (long)sum2.size()) {
	    lg.buffer() << "get1DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< " and sum2.size() is " << sum2.size()
			<< "; these two values are expected to be the same";
	}
	else if (weights.size() != bins.size()) {
	    lg.buffer() << "get1DDistribution returned " << weights.size()
			<< " bin" << (weights.size() > 1 ? "s" : "")
			<< ", but get1DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < weights.size(); ++ i) {
		if (sum2[i] != weights[i]) {
		    lg.buffer() << "weights[" << i << "] (" << weights[i]
				<< ") != sum2[" << i << "] (" << sum2[i]
				<< ")\n";
		}
		if (bins[i] != 0) {
		    array_t<double> *tmp = cptrw->selectDoubles(*(bins[i]));
		    if (tmp == 0) {
			lg.buffer() << "** failed to retrieve "
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
			    lg.buffer() << "weights[" << i << "] ("
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
		    lg.buffer() << "weights[" << i << "] (" << weights[i]
				<< "), but bins[" << i << "] is nil (0)\n";
		    ++ ierr;
		}
	    }
	    if (ierr > 0)
		lg.buffer() << "Warning -- ";
	    lg.buffer() << "matching arrays weights and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "") << "\n";
	}
	ibis::util::clean(bins);
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n2D-Weighted distribution of " << col1 << " and "
		    << col2 << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << weights.size() << " bin"
		    << (weights.size() > 1 ? "s" : "") << " on " << NB1
		    << " x " << NB1 << " cells\n";

	uint32_t cnt = 0;
	double tot = 0.0;
	for (uint32_t i = 0; i < weights.size(); ++ i) {
	    if (weights[i] > 0) {
		const uint32_t i1 = i / NB1;
		const uint32_t i2 = i % NB1;
		lg.buffer() << i << "\t[" << amin1+stride1*i1 << ", "
			    << amin1+stride1*(i1+1)
			    << ") [" << amin2+stride2*i2 << ", "
			    << amin2+stride2*(i2+1)
			    << ")\t" << weights[i] << "\n";
		tot += weights[i];
		++ cnt;
	    }
	}
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total weight = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "Warning -- part[" << tbl.name()
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n" << evt << " -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size() || ierr != (long)sum2.size()) {
	    lg.buffer() << "get2DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< " and sum2.size() is " << sum2.size()
			<< "; these two values are expected to be the same";
	}
	else if (weights.size() != bins.size()) {
	    lg.buffer() << "get2DDistribution returned " << weights.size()
			<< " bin" << (weights.size() > 1 ? "s" : "")
			<< ", but get2DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < weights.size(); ++ i) {
		if (sum2[i] != weights[i]) {
		    lg.buffer() << "weights[" << i << "] (" << weights[i]
				<< ") != sum2[" << i << "] (" << sum2[i]
				<< ")\n";
		}
		if (bins[i] != 0) {
		    array_t<double> *tmp = cptrw->selectDoubles(*(bins[i]));
		    if (tmp == 0) {
			lg.buffer() << "** failed to retrieve "
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
			    lg.buffer() << "weights[" << i << "] ("
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
		    lg.buffer() << "weights[" << i << "] (" << weights[i]
				<< "), but bins[" << i << "] is nil (0)\n";
		    ++ ierr;
		}
	    }
	    if (ierr > 0)
		lg.buffer() << "Warning -- ";
	    lg.buffer() << "matching arrays weights and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "") << "\n";
	}
	ibis::util::clean(bins);
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n3D-Weighted distribution of " << col1 << ", "
		    << col2 << " and " << col3 << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << weights.size() << " bin"
		    << (weights.size() > 1 ? "s" : "") << " on " << NB1
		    << " x " << NB1 << " x " << NB1 << " cells\n";

	uint32_t cnt = 0;
	double tot = 0.0;
	for (uint32_t i = 0; i < weights.size(); ++ i) {
	    if (weights[i] > 0) {
		const uint32_t i1 = i / (NB1 * NB1);
		const uint32_t i2 = (i / NB1) % NB1;
		const uint32_t i3 = i % NB1;
		lg.buffer() << i << "\t[" << amin1+stride1*i1 << ", "
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
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total weight = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "Warning -- part[" << tbl.name()
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n" << evt << " -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size() || ierr != (long)sum2.size()) {
	    lg.buffer() << "get3DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< " and sum2.size() is " << sum2.size()
			<< "; these two values are expected to be the same";
	}
	else if (weights.size() != bins.size()) {
	    lg.buffer() << "get3DDistribution returned " << weights.size()
			<< " bin" << (weights.size() > 1 ? "s" : "")
			<< ", but get3DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < weights.size(); ++ i) {
		if (sum2[i] != weights[i]) {
		    lg.buffer() << "weights[" << i << "] (" << weights[i]
				<< ") != sum2[" << i << "] (" << sum2[i]
				<< ")\n";
		}
		if (bins[i] != 0) {
		    array_t<double> *tmp = cptrw->selectDoubles(*(bins[i]));
		    if (tmp == 0) {
			lg.buffer() << "** failed to retrieve "
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
			    lg.buffer() << "weights[" << i << "] ("
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
		    lg.buffer() << "weights[" << i << "] (" << weights[i]
				<< "), but bins[" << i << "] is nil (0)\n";
		    ++ ierr;
		}
	    }
	    if (ierr > 0)
		lg.buffer() << "Warning -- ";
	    lg.buffer() << "matching arrays weights and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "") << "\n";
	}
	ibis::util::clean(bins);
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n2D-Joint distribution of " << col1 << " and " << col2
		    << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << cnts.size() << " bin"
		    << (cnts.size() > 1 ? "s" : "") << " on " << NB1
		    << " x " << NB1 << " cells\n";

	uint32_t cnt = 0, tot = 0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		const uint32_t i1 = i / NB1;
		const uint32_t i2 = i % NB1;
		lg.buffer() << i << "\t[" << amin1+stride1*i1 << ", "
			    << amin1+stride1*(i1+1)
			    << ") [" << amin2+stride2*i2 << ", "
			    << amin2+stride2*(i2+1)
			    << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total count = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "part[" << tbl.name()
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
	ibis::util::logger lg(0);
	lg.buffer() << "\nprint2DDistribution(" << col1 << ", " << col2
		    << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get2DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg.buffer() << "get2DDistribution returned " << cnts.size()
			<< " bin" << (cnts.size() > 1 ? "s" : "")
			<< ", but get2DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays cnts and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "");
	}
#else
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get2DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     bins);
	ibis::util::logger lg(0);
	lg.buffer() << "\nprint2DDistribution(" << col1 << ", " << col2
		    << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get2DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg.buffer() << "get2DDistribution returned " << cnts.size()
			<< " bin" << (cnts.size() > 1 ? "s" : "")
			<< ", but get2DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i] != 0 && bins[i]->cnt() != cnts[i]) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< bins[i]->cnt() << ")\n";
		    ++ ierr;
		}
		else if (bins[i] == 0 && cnts[i] != 0) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "] (0)\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays cnts and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "");
	}
	ibis::util::clean(bins);
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
	ibis::util::logger lg(0);
	const uint32_t nbin2 = bds2.size() - 1;
	lg.buffer() << "\n2D-Joint distribution of " << col1 << " and " << col2
		    << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << cnts.size() << " bin"
		    << (cnts.size() > 1 ? "s" : "") << " on " << bds1.size()-1
		    << " x " << bds2.size()-1 << " cells\n";

	uint32_t cnt = 0, tot=0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		uint32_t i1 = i / nbin2;
		uint32_t i2 = i % nbin2;
		lg.buffer() << i << "\t[" << bds1[i1] << ", " << bds1[i1+1]
			    << ") [" << bds2[i2] << ", " << bds2[i2+1]
			    << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total count = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "part[" << tbl.name()
		    << "].get2DDistribution returned with ierr = " << ierr
		    << ", bds1.size() = " << bds1.size() << ", bds2.size() = "
		    << bds2.size() << ", cnts.size() = " << cnts.size();
	return;
    }
    if (ierr > 0 && (verify_rid || ibis::gVerbose > 10)) {
	std::vector<ibis::bitvector> bins;
	std::vector<double> bdt1, bdt2;
	ierr = tbl.get2DBins(cond, col1, col2, NB1, NB1, bdt1, bdt2, bins);
	ibis::util::logger lg(0);
	lg.buffer() << "\nprint2DDistribution(" << col1 << ", " << col2
		    << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get2DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get2DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (bds1.size() != bdt1.size() || bds2.size() != bdt2.size() ||
		 cnts.size() != bins.size()) {
	    lg.buffer() << "get2DDistribution returned a " << bds1.size()-1
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
		    lg.buffer() << "bds1[" << i << "] (" << bds1[i]
				<< ") != bdt1[" << i << "] (" << bdt1[i]
				<< ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < bds2.size(); ++ i)
		if (bds2[i] != bdt2[i]) {
		    lg.buffer() << "bds2[" << i << "] (" << bds2[i]
				<< ") != bdt2[" << i << "] (" << bdt2[i]
				<< ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays cnts and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "");
	    if (ierr > 0)
		lg.buffer() << "\nNOTE: due to the different numbers of "
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
    ibis::util::logger lg(0);
    long ierr = tbl.getJointDistribution(cond, col1, col2, bds1, bds2, cnts);
    if (ierr > 0 && static_cast<uint32_t>(ierr) == cnts.size()) {
	const uint32_t nb2p1 = bds2.size() + 1;
	lg.buffer() << "\nJoint distribution of " << col1 << " and " << col2
		    << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << cnts.size() << " bin"
		    << (cnts.size() > 1 ? "s" : "") << " on " << bds1.size()+1
		    << " x " << bds2.size()+1 << " cells\n";

	uint32_t cnt = 0, tot=0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		uint32_t i1 = i / nb2p1;
		uint32_t i2 = i % nb2p1;
		if (i1 == 0)
		    lg.buffer() << "(..., " << bds1[0] << ")";
		else if (i1 < bds1.size())
		    lg.buffer() << "[" << bds1[i1-1] << ", " << bds1[i1]
				<< ")";
		else
		    lg.buffer() << "[" << bds1.back() << ", ...)";
		if (i2 == 0)
		    lg.buffer() << "(..., " << bds2[0] << ")";
		else if (i2 < bds2.size())
		    lg.buffer() << "[" << bds2[i2-1] << ", " << bds2[i2]
				<< ")";
		else
		    lg.buffer() << "[" << bds2.back() << ", ...)";
		lg.buffer() << "\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total count = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	lg.buffer() << "part[" << tbl.name()
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n3D-Joint distribution of " << col1 << ", " << col2
		    << ", and " << col3 << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << cnts.size() << " bin"
		    << (cnts.size() > 1 ? "s" : "") << " on " << NB1
		    << " x " << NB1 << " x " << NB1 << " cells\n";

	uint32_t cnt = 0, tot = 0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		const uint32_t i1 = i / nb23;
		const uint32_t i2 = (i - i1 * nb23) / NB1;
		const uint32_t i3 = i % NB1;
		lg.buffer() << i << "\t[" << amin1+stride1*i1 << ", "
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
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total count = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "part[" << tbl.name()
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
	ibis::util::logger lg(0);
	lg.buffer() << "\nprint3DDistribution(" << col1 << ", " << col2
		    << ", " << col3 << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get3DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg.buffer() << "get3DDistribution returned " << cnts.size()
			<< " bin" << (cnts.size() > 1 ? "s" : "")
			<< ", but get3DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays cnts and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "");
	}
#else
	std::vector<ibis::bitvector*> bins;
	ierr = tbl.get3DBins(cond,
			     col1, amin1, amax1, stride1,
			     col2, amin2, amax2, stride2,
			     col3, amin3, amax3, stride3,
			     bins);
	ibis::util::logger lg(0);
	lg.buffer() << "\nprint3DDistribution(" << col1 << ", " << col2
		    << ", " << col3 << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get3DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (cnts.size() != bins.size()) {
	    lg.buffer() << "get3DDistribution returned " << cnts.size()
			<< " bin" << (cnts.size() > 1 ? "s" : "")
			<< ", but get3DBins returned " << bins.size()
			<< " bin" << (bins.size() > 1 ? "s" : "");
	}
	else {
	    ierr = 0;
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i] != 0 ? bins[i]->cnt() != cnts[i] : cnts[i] != 0) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< (bins[i]!=0 ? bins[i]->cnt() : 0) << ")\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays cnts and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "");
	}
	ibis::util::clean(bins);
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
	ibis::util::logger lg(0);
	lg.buffer() << "\n3D-Joint distribution of " << col1 << ", " << col2
		    << ", and " << col3 << " from table " << tbl.name();
	if (cond && *cond)
	    lg.buffer() << " subject to the condition " << cond;
	lg.buffer() << " with " << cnts.size() << " bin"
		    << (cnts.size() > 1 ? "s" : "") << " on " << bds1.size()-1
		    << " x " << nbin2 << " x " << nbin3 << " cells\n";

	uint32_t cnt = 0, tot = 0;
	for (uint32_t i = 0; i < cnts.size(); ++ i) {
	    if (cnts[i] > 0) {
		const uint32_t i1 = i / nb23;
		const uint32_t i2 = (i - i1 * nb23) / nbin3;
		const uint32_t i3 = i % nbin3;
		lg.buffer() << i << "\t[" << bds1[i1] << ", " << bds1[i1+1]
			    << ") [" << bds2[i2] << ", " << bds2[i2+1]
			    << ") [" << bds3[i3] << ", " << bds3[i3+1]
			    << ")\t" << cnts[i] << "\n";
		tot += cnts[i];
		++ cnt;
	    }
	}
	lg.buffer() << "  Number of occupied cells = " << cnt
		    << ", total count = " << tot << ", number of rows in "
		    << tbl.name() << " = " << tbl.nRows() << "\n";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "part[" << tbl.name()
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
	ibis::util::logger lg(0);
	lg.buffer() << "\nprint3DDistribution(" << col1 << ", " << col2
		    << ", " << col3 << ") -- \n";
	if (ierr < 0) {
	    lg.buffer() << "get3DBins failed with error " << ierr;
	}
	else if (ierr != (long)bins.size()) {
	    lg.buffer() << "get3DBins returned " << ierr
			<< ", but bins.size() is " << bins.size()
			<< "; these two values are expected to be the same";
	}
	else if (bds1.size() != bdt1.size() || bds2.size() != bdt2.size() ||
		 bds3.size() != bdt3.size() || cnts.size() != bins.size()) {
	    lg.buffer() << "get3DDistribution returned a " << bds1.size()-1
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
		    lg.buffer() << "bds1[" << i << "] (" << bds1[i]
				<< ") != bdt1[" << i << "] (" << bdt1[i]
				<< ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < bds2.size(); ++ i)
		if (bds2[i] != bdt2[i]) {
		    lg.buffer() << "bds2[" << i << "] (" << bds2[i]
				<< ") != bdt2[" << i << "] (" << bdt2[i]
				<< ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < bds3.size(); ++ i)
		if (bds3[i] != bdt3[i]) {
		    lg.buffer() << "bds3[" << i << "] (" << bds3[i]
				<< ") != bdt3[" << i << "] (" << bdt3[i]
				<< ")\n";
		    ++ ierr;
		}
	    for (size_t i = 0; i < cnts.size(); ++ i)
		if (bins[i].cnt() != cnts[i]) {
		    lg.buffer() << "cnts[" << i << "] (" << cnts[i]
				<< ") != bins[" << i << "].cnt() ("
				<< bins[i].cnt() << ")\n";
		    ++ ierr;
		}
	    lg.buffer() << "matching arrays cnts and bins produces "
			<< ierr << " error" << (ierr > 1 ? "s" : "");
	    if (ierr > 0)
		lg.buffer() << "\nNOTE: due to the different numbers of "
		    "internal bins used for the adaptive histograms, "
		    "get3DDistribution and get3DBins may not produce "
		    "exactly the same answers";
	}
    }
} // print3DDist

// print some helpful information
static void print(const char* cmd, const ibis::partList& tlist) {
    if (cmd == 0 || *cmd == 0) return;
    LOGGER(ibis::gVerbose >= 4) << "\nprint(" << cmd << ") -- ...";

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
	ibis::util::getString(name1, names);
	if (name1.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print joint' needs two "
		"column names as arguments";
	    return;
	}
	ibis::util::getString(name2, names);
	if (name2.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print joint' needs two "
		"column names as arguments";
	    return;
	}
	ibis::util::getString(name3, names);
	if (name3.empty()) { // got two names, 2D distributions
	    for (ibis::partList::const_iterator tit = tlist.begin();
		 tit != tlist.end(); ++ tit) {
		print2DDistribution(**tit, name1.c_str(), name2.c_str(), cond);
		if (ibis::gVerbose > 6)
		    print2DDist(**tit, name1.c_str(), name2.c_str(), cond);
		if (ibis::gVerbose > 9)
		    printJointDistribution(**tit, name1.c_str(),
					   name2.c_str(), cond);
	    }
	}
	else {
	    for (ibis::partList::const_iterator tit = tlist.begin();
		 tit != tlist.end(); ++ tit) {
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
	ibis::util::getString(nm1, names);
	if (nm1.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print weighted' needs at least two names "
		"as arguments";
	    return;
	}
	ibis::util::getString(nm2, names);
	if (nm2.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "the command 'print weighted' needs at least two names "
		"as arguments";
	    return;
	}
	ibis::util::getString(nm3, names);
	if (nm3.empty()) {
	    for (ibis::partList::const_iterator tit = tlist.begin();
		 tit != tlist.end(); ++ tit) {
		print1DDistribution(**tit, cond, nm1.c_str(), nm2.c_str());
	    }
	    return;
	}
	ibis::util::getString(nm4, names);
	if (nm4.empty()) {
	    for (ibis::partList::const_iterator tit = tlist.begin();
		 tit != tlist.end(); ++ tit) {
		print2DDistribution(**tit, cond, nm1.c_str(),
				    nm2.c_str(), nm3.c_str());
	    }
	}
	else {
	    for (ibis::partList::const_iterator tit = tlist.begin();
		 tit != tlist.end(); ++ tit) {
		print3DDistribution(**tit, cond, nm1.c_str(),
				    nm2.c_str(), nm3.c_str(), nm4.c_str());
	    }
	}
    }
    else if (names) { // there are arguments after the print command
	ibis::nameList nlist(names); // split using the space as delimiter
	for (ibis::nameList::const_iterator it = nlist.begin();
	     it != nlist.end(); ++it) { // go through each name
	    ibis::partList::const_iterator tit = tlist.begin();
	    for (; tit != tlist.end() && 
		     stricmp((*tit)->name(), *it) != 0 &&
		     ibis::util::strMatch((*tit)->name(), *it) == false;
		 ++ tit);
	    if (tit != tlist.end()) { // it's a data partition
		ibis::util::logger lg(0);
		lg.buffer() << "Partition " << (*tit)->name() << ":\n";
		(*tit)->print(lg.buffer());
	    }
	    else if ((*it)[0] == '*') {
		printAll(tlist);
	    }
	    else if (stricmp(*it, "parts") == 0) {
		ibis::util::logger lg(0);
		lg.buffer() << "Name(s) of all data partitioins\n";
		for (tit = tlist.begin(); tit != tlist.end(); ++tit)
		    lg.buffer() << (*tit)->name() << ' ';
	    }
	    else if (stricmp(*it, "names") == 0 ||
		     stricmp(*it, "columns") == 0) {
		printNames(tlist);
	    }
	    else if (stricmp(*it, "distributions") == 0) {
		printDistribution(tlist);
	    }
	    else { // assume it to be a column name
		for (tit = tlist.begin(); tit != tlist.end(); ++tit) {
		    printColumn(**tit, *it, cond);
		    if (ibis::gVerbose > 9)
			printColumn0(**tit, *it, cond);
		}
	    }
	}
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "Name(s) of all partitions\n";
	for (ibis::partList::const_iterator tit = tlist.begin();
	     tit != tlist.end(); ++tit)
	    lg.buffer() << (*tit)->name() << ' ';
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
	    if (ch != buf)
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
static void parse_args(int argc, char** argv,
		       int& mode, ibis::partList& tlist,
		       stringList& qlist, stringList& alist, stringList& slist,
		       std::vector<std::string> &queff, ibis::joinlist& joins) {
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
    tlist.clear(); // list of data partitions

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
		      // name")
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
		// it also accepts an optional argument to indicate the
		// number of threads to use
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
	    case 'E': // estiamtion only
		estimate_only = true;
		if (skip_estimation)
		    skip_estimation = false;
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
		    js.selcol.push_back(argv[i]);
		}
		if (js.part1 != 0 && js.part2 != 0 && js.jcol != 0) {
		    joins.push_back(js);
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
		skip_estimation = true;
		if (estimate_only)
		    estimate_only = false;
		break;}
	    case 'o':
	    case 'O':
		if (argv[i][2] == 'n' || argv[i][2] == 'N') {
		    // skip estimation, directly call function evaluate
		    skip_estimation = true;
		    if (estimate_only)
			estimate_only = false;
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
		if (argv[i][2] == 'i' || argv[i][2] == 'I') {
		    verify_rid = true;
		    if (i+1 < argc) { // there is one more argument
			if (argv[i+1][0] != '-') { // assume to be a file name
			    ridfile = argv[i+1];
			    ++ i;
			}
		    }
		}
		else if (i+1 < argc && argv[i+1][0] != '-') {
		    rdirs.push_back(argv[i+1]);
		    ++ i;
		}
		else {
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
		char *ptr = strchr(argv[i], '=');
		if (ptr == 0) {
		    if (i+1 < argc) {
			if (isdigit(*argv[i+1])) {
			    testing += atoi(argv[i+1]);
			    i = i + 1;
			}
			else {
			    ++ testing;
			}
		    }
		    else {
			++ testing;
		    }
		}
		else {
		    testing += atoi(++ptr);
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
		else {
		    ibis::gVerbose += atoi(++ptr);
		}
		break;}
	    case 'y':
	    case 'Y': // yank some rows of every data partition available
		// must have an argument after the flag to indicate a file
		// containing row numbers or a string indicate conditions
		// on rows to mark as inactive/junk
		if (i+1 < argc && *argv[i+1] != '-') {
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
	       // data directories and are read two at a time
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
    if (qlist.size() > 1U) {
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
#endif
	    if (threading > 2) // more than two processor, leave one for OS
		-- threading;
	}
	if (threading > qlist.size())
	    threading = static_cast<unsigned>
		(ceil(sqrt(static_cast<double>(qlist.size()))));
	if (threading <= 1) // make sure it exactly 0
	    threading = 0;
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
    if (ibis::gVerbose > 0) {
	ibis::util::logger lg(1);
	lg.buffer() << "\n" << argv[0] << ": "
		    << (mode ? "interactive mode" : "batch mode")
		    << ", log level " << ibis::gVerbose;
	if (build_index > 0) {
	    lg.buffer() << ", building indexes";
	    if (zapping)
		lg.buffer() << " (remove any existing indexes)";
	}
	if (testing > 0)
	    lg.buffer() << ", performing self test";
	if (threading > 0)
	    lg.buffer() << ", threading " << threading;
	if (skip_estimation)
	    lg.buffer() << ", skipping estimation";
	else if (estimate_only)
	    lg.buffer() << ", computing only bounds";
	if (! alist.empty()) {
	    lg.buffer() << "\nappending data in the following director"
			<< (alist.size()>1 ? "ies" : "y");
	    if (appendto)
		lg.buffer() << " to partition " << appendto;
	    for (uint32_t i = 0; i < alist.size(); ++ i)
		lg.buffer() << "\n" << alist[i];
	}
	lg.buffer() << "\n";
    }
    if (! confs.empty()) {
	// read all configuration files
	for (uint32_t i = 0; i < confs.size(); ++ i)
	    ibis::gParameters().read(confs[i]);
    }
    else if (ibis::gParameters().empty()) {
	// try default configuration files
	ibis::gParameters().read();
    }
    if (accessIndexInWhole > 0) {
	ibis::gParameters().add("all.preferMMapIndex", "T");
    }

    // reorder the data directories first, a data directory may be followed
    // by ':' and column names
    for (unsigned i = 0; i < rdirs.size(); ++ i) {
	char* str = const_cast<char*>(strrchr(rdirs[i], ':'));
	if (str != 0 && str > rdirs[i] && str[1] != '/' && str[1] != '\\') {
	    std::string dir;
	    for (const char* tmp = rdirs[i]; tmp < str; ++ tmp)
		dir += *tmp;
	    str = ibis::util::strnewdup(str+1);
	    ibis::table::stringList slist;
	    ibis::table::parseNames(str, slist);
	    ibis::part tbl(dir.c_str(), static_cast<const char*>(0));
	    tbl.reorder(slist);
	    delete [] str;
	}
	else {
	    ibis::part tbl(rdirs[i], static_cast<const char*>(0));
	    tbl.reorder();
	}
    }

    // construct the paritions using both the command line arguments and
    // the resource files
    ibis::util::tablesFromResources(tlist, ibis::gParameters());
    for (std::vector<const char*>::const_iterator it = dirs.begin();
	 it != dirs.end(); ++ it) {
	ibis::util::tablesFromDir(tlist, *it);
    }

    if (ibis::gVerbose > 1) {
	ibis::util::logger lg(2);
	if (tlist.size()) {
	    lg.buffer() << "Partition" << (tlist.size()>1 ? "s" : "")
			<< "[" << tlist.size() << "]:\n";
	    for (ibis::partList::const_iterator it = tlist.begin();
		 it != tlist.end(); ++it)
		lg.buffer() << (*it)->name() << "\n";
	}
	if (qlist.size() > 0) {
	    lg.buffer() << "Quer" << (qlist.size()>1 ? "ies" : "y")
			<< "[" << qlist.size() << "]:\n";
	    for (stringList::const_iterator it = qlist.begin();
		 it != qlist.end(); ++it)
		lg.buffer() << *it << "\n";
	}
	if (joins.size() > 0) {
	    lg.buffer() << "Join" << (joins.size() > 1 ? "s" : "")
			<< "[" << joins.size() << "]:\n";
	    for (size_t j = 0; j < joins.size(); ++ j) {
		joins[j].print(lg.buffer());
		lg.buffer() << "\n";
	    }
	}
    }

    if (ibis::gVerbose > 1 &&
	(testing > 1 || build_index > 0 || ! printcmds.empty())) {
	for (ibis::partList::const_iterator it = tlist.begin();
	     it != tlist.end(); ++it) {
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
		LOGGER(ibis::gVerbose >= 2)
		    << *argv << ": recomputing the min/max for partition "
		    << (*it)->name();
		(*it)->computeMinMax();
	    }
	}
    }
    for (std::vector<const char*>::const_iterator it = printcmds.begin();
	 it != printcmds.end(); ++ it) {
	print(*it, tlist);
    }
} // parse_args

/// Ibis::mensa2 is constructed from a list of data partitions.
ibis::mensa2::mensa2(const ibis::partList &l) : ibis::mensa() {
    if (l.empty()) return;

    parts.insert(parts.end(), l.begin(), l.end());
    for (ibis::partList::const_iterator it = parts.begin();
	 it != parts.end(); ++ it) {
	(*it)->combineNames(naty);
	nrows += (*it)->nRows();
    }
    if (name_.empty() && ! parts.empty()) {
	// take on the name of the first partition
	ibis::partList::const_iterator it = parts.begin();
	name_ = "T-";
	name_ += (*it)->name();
	if (desc_.empty()) {
	    
	}
    }
    if (ibis::gVerbose > 0 && ! name_.empty()) {
	ibis::util::logger lg(0);
	lg.buffer() << "ibis::mensa2 -- constructed table "
		    << name_ << " (" << desc_ << ") from a list of "
		    << l.size() << " data partition"
		    << (l.size()>1 ? "s" : "")
		    << ", with " << naty.size() << " column"
		    << (naty.size()>1 ? "s" : "") << " and "
		    << nrows << " row" << (nrows>1 ? "s" : "");
    }
} // ibis::mensa2::mensa2

/// Ibis::mensa2 does not own the data partitions and does not free the
/// resources in those partitions.
ibis::mensa2::~mensa2 () {
    parts.clear();
} // ibis::mensa2::~mensa2

// evaluate a single query -- directly retrieve values of selected columns
static void xdoQuery(ibis::part* tbl, const char* uid, const char* wstr,
		     const char* sstr) {
    LOGGER(ibis::gVerbose >= 1)
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
	    ibis::util::logger lg(1);
	    if (! comp.empty())
		lg.buffer() << "xdoQuery -- the WHERE clause \"" << old.c_str()
			    << "\" is split into \"" << comp.c_str()
			    << "\" AND \"" << aQuery.getWhereClause() << "\"";
	    else
		lg.buffer() << "xdoQuery -- the WHERE clause \""
			    << aQuery.getWhereClause()
			    << "\" is considered simple";
	}
    }
    const char* asstr = 0;
    if (sstr != 0) {
	aQuery.setSelectClause(sstr);
	asstr = aQuery.getSelectClause();
    }

    if (! skip_estimation) {
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
	    ibis::util::logger lg(0);
	    lg.buffer() << "xdoQuery -- the number of hits is ";
	    if (num2 > num1) 
		lg.buffer() << "between " << num1 << " and ";
	    lg.buffer() << num2;
	}
	if (estimate_only)
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
    LOGGER(ibis::gVerbose >= 1) << "xdoQuery -- the number of hits = " << num1;

    if (asstr != 0 && *asstr != 0 && num1 > 0) {
	ibis::nameList names(asstr);
	for (ibis::nameList::const_iterator it = names.begin();
	     it != names.end(); ++it) {
	    ibis::column* col = tbl->getColumn(*it);
	    if (col) {
		LOGGER(ibis::gVerbose >= 1)
		    << "xdoQuery -- retrieving qualified values of " << *it;

		switch (col->type()) {
		case ibis::UBYTE:
		case ibis::BYTE:
		case ibis::USHORT:
		case ibis::SHORT:
		case ibis::UINT:
		case ibis::INT: {
		    array_t<int32_t>* intarray;
		    intarray = aQuery.getQualifiedInts(*it);
		    ibis::util::logger lg(0);
		    if (intarray->size() != static_cast<uint32_t>(num1))
			lg.buffer()
			    << "expected to retrieve " << num1
			    << " entries, but got " << intarray->size();
		    if (num1 < (2 << ibis::gVerbose) ||
			ibis::gVerbose > 30) {
			lg.buffer() << "selected entries of column " << *it
				    << "\n";
			for (array_t<int32_t>::const_iterator ait =
				 intarray->begin();
			     ait != intarray->end(); ++ait)
			    lg.buffer() << *ait << "\n";
		    }
		    else {
			lg.buffer() << "xdoQuery -- retrieved "
				    << intarray->size()
				    << " ints (expecting " << num1 << ")\n";
		    }
		    delete intarray;
		    break;}

		case ibis::FLOAT: {
		    array_t<float>* floatarray;
		    floatarray = aQuery.getQualifiedFloats(*it);

		    ibis::util::logger lg(0);
		    if (floatarray->size() !=
			static_cast<uint32_t>(num1))
			lg.buffer() << "expected to retrieve " << num1
				    << " entries, but got "
				    << floatarray->size();
		    if (num1 < (2 << ibis::gVerbose) ||
			ibis::gVerbose > 30) {
			lg.buffer() << "selected entries of column " << *it;
			for (array_t<float>::const_iterator ait =
				 floatarray->begin();
			     ait != floatarray->end(); ++ait)
			    lg.buffer() << "\n" << *ait;
		    }
		    else {
			lg.buffer() << "xdoQuery -- retrieved "
				    << floatarray->size()
				    << " floats (expecting " << num1 << ")";
		    }
		    delete floatarray;
		    break;}

		case ibis::DOUBLE: {
		    array_t<double>* doublearray;
		    doublearray = aQuery.getQualifiedDoubles(*it);

		    ibis::util::logger lg(0);
		    if (doublearray->size() !=
			static_cast<uint32_t>(num1))
			lg.buffer() << "expected to retrieve " << num1
				    << " entries, but got "
				    << doublearray->size();
		    if (num1<(2<<ibis::gVerbose) || ibis::gVerbose>30) {
			lg.buffer() << "selected entries of column " << *it;
			for (array_t<double>::const_iterator ait =
				 doublearray->begin();
			     ait != doublearray->end(); ++ait)
			    lg.buffer() << "\n" << *ait;
		    }
		    else {
			lg.buffer() << "xdoQuery -- retrieved "
				    << doublearray->size()
				    << " doubles (expecting " << num1 << ")";
		    }
		    delete doublearray;
		    break;}
		default:
		    LOGGER(ibis::gVerbose >= 0)
			<< "column " << *it << " has an unsupported "
			<< "type(" << static_cast<int>(col->type()) << ")";
		}
	    } // if (col)...
	} // for ...
    } // if (asstr != 0 && num1 > 0)
} // xdoQuery

// This print function takes the most general option in getting the values
// out of a query.  All supported FastBit types can be retrieved as
// strings, therefore, it is always fine to use getString to retrieve a
// value.  However, if the values in the select clause are of known type,
// those types should be used instead of @c getString.
static void printQueryResults(std::ostream &out, ibis::query &q) {
    ibis::query::result cursor(q);
    out << "printing results of query " << q.id() << "(numHits="
	<< q.getNumHits() << ")\n"
	<< q.getSelectClause() << std::endl;
    const ibis::selected& sel = q.components();
    if (sel.size() == 0) return;

    while (cursor.next()) {
	out << cursor.getString(static_cast<uint32_t>(0U));
	for (uint32_t i = 1; i < sel.size(); ++ i)
	    out << ", " << cursor.getString(i);
	out << "\n";
    }
} // printQueryResults

template<typename T>
void findMissingValuesT(const ibis::column &col,
			const ibis::bitvector &ht0,
			const ibis::bitvector &ht1) {
    array_t<T> vals0, vals1;
    long ierr = col.selectValues(ht0, &vals0);
    if (ierr <= 0 || static_cast<long unsigned>(ierr) < ht0.cnt()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- findMissingValues did received expected number "
	    "of values for query 0, expected " << ht0.cnt()
	    << ", received " << ierr;
	return;
    }
    ierr = col.selectValues(ht1, &vals1);
    if (ierr <= 0 || static_cast<long unsigned>(ierr) < ht1.cnt()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- findMissingValues did received expected number "
	    "of values for query 1, expected " << ht1.cnt()
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
			const char* ordkeys, int direction,
			uint32_t limit) {
    ibis::mensa2 tbl(pl); // construct a temporary ibis::table object
    std::string sqlstring; //
    {
	std::ostringstream ostr;
	if (sstr != 0 && *sstr != 0)
	    ostr << "SELECT " << sstr;
	ostr << " FROM " << tbl.name();
	if (wstr != 0 && *wstr != 0)
	    ostr << " WHERE " << wstr;
	if (ordkeys && *ordkeys) {
	    ostr << " ORDER BY " << ordkeys;
	    if (direction >= 0)
		ostr << " ASC";
	    else
		ostr << " DESC";
	}
	if (limit > 0)
	    ostr << " LIMIT " << limit;
	sqlstring = ostr.str();
    }
    LOGGER(ibis::gVerbose >= 2)
	<< "tableSelect -- processing \"" << sqlstring << '\"';

    ibis::horometer timer;
    timer.start();

    if (! skip_estimation) {
	uint64_t num1, num2;
	tbl.estimate(wstr, num1, num2);
	if (ibis::gVerbose > 0) {
	    ibis::util::logger lg(1);
	    lg.buffer() << "tableSelect -- the number of hits is ";
	    if (num2 > num1)
		lg.buffer() << "between " << num1 << " and ";
	    lg.buffer() << num2;
	}
	if (estimate_only) {
	    if (ibis::gVerbose >= 0) {
		timer.stop();
		ibis::util::logger lg(0);
		lg.buffer() << "tableSelect:: estimate(" << wstr << ") took "
			    << timer.CPUTime() << " CPU seconds, "
			    << timer.realTime() << " elapsed seconds";
	    }
	    return; // stop here is only want to estimate
	}
    }

    ibis::table *sel1 = tbl.select(sstr, wstr);
    if (sel1 == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "tableSelect:: select(" << sstr << ", " << wstr
	    << ") failed on table " << tbl.name();
	return;
    }

    LOGGER(ibis::gVerbose >= 0)
	<< "tableSelect -- select(" << sstr << ", " << wstr
	<< ") on table " << tbl.name() << " produced a table with "
	<< sel1->nRows() << " row" << (sel1->nRows() > 1 ? "s" : "")
	<< " and " << sel1->nColumns() << " column"
	<< (sel1->nColumns() > 1 ? "s" : "");
    if ((ordkeys && *ordkeys) || limit > 0) { // top-K query
	sel1->orderby(ordkeys);
	if (direction < 0)
	    sel1->reverseRows();
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
	    sel1->dump(output, limit, ", ");
	}
    }
    else if (ibis::gVerbose >= 0) {
	ibis::util::logger lg(0);
	if (limit == 0 && sel1->nColumns() > 0) {
	    limit = (sel1->nRows() >> ibis::gVerbose) > 0 ?
		1 << ibis::gVerbose : static_cast<uint32_t>(sel1->nRows());
	    if (limit > (sel1->nRows() >> 1))
		limit = sel1->nRows();
	}
	if (limit < sel1->nRows()) {
	    lg.buffer() << "tableSelect -- the first ";
	    if (limit > 1)
		lg.buffer() << limit << " rows ";
	    else
		lg.buffer() << " row ";
	    lg.buffer() << "(out of " << sel1->nRows()
			<< ") from the result table for \""
			<< sqlstring << "\"\n";
	}
	else {
	    lg.buffer() << "tableSelect -- the result table (" << sel1->nRows()
			<< " x " << sel1->nColumns() << ") for \""
			<< sqlstring << "\"\n";
	}
	if (outputnamestoo)
	    sel1->dumpNames(lg.buffer(), ", ");
	sel1->dump(lg.buffer(), limit, ", ");
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
    delete sel1;
    timer.stop();
    LOGGER(ibis::gVerbose > 0)
	<< "tableSelect:: complete evaluation of " << sqlstring
	<< " took " << timer.CPUTime() << " CPU seconds, "
	<< timer.realTime() << " elapsed seconds";
} // tableSelect

// evaluate a single query -- print selected columns through ibis::bundle
static void doQuery(ibis::part* tbl, const char* uid, const char* wstr,
		    const char* sstr, const char* ordkeys, int direction,
		    uint32_t limit) {
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
	    if (direction >= 0)
		ostr << " ASC";
	    else
		ostr << " DESC";
	}
	if (limit > 0)
	    ostr << " LIMIT " << limit;
	sqlstring = ostr.str();
    }
    LOGGER(ibis::gVerbose >= 2)
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
    aQuery.setWhereClause(wstr);
    if (sstr != 0 && *sstr != 0) {
	aQuery.setSelectClause(sstr);
	asstr = aQuery.getSelectClause();
    }
    if (aQuery.getWhereClause() == 0 && ridfile == 0
	&& aQuery.getSelectClause() == 0)
	return;
    if (zapping && aQuery.getWhereClause()) {
	std::string old = aQuery.getWhereClause();
	std::string comp = aQuery.removeComplexConditions();
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg(1);
	    if (! comp.empty())
		lg.buffer() << "doQuery -- the WHERE clause \""
			    <<  old.c_str() << "\" is split into \""
			    << comp.c_str()  << "\" AND \""
			    << aQuery.getWhereClause() << "\"";
	    else
		lg.buffer() << "doQuery -- the WHERE clause \""
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
		ibis::util::logger lg(0);
		lg.buffer() << "doQuery:: sequentialScan("
			    << aQuery.getWhereClause() << ") failed";
		return;
	    }

	    num2 = btmp.cnt();
	}
	if (ibis::gVerbose >= 0) {
	    timer.stop();
	    ibis::util::logger lg(0);
	    lg.buffer() << "doQuery:: sequentialScan("
			<< aQuery.getWhereClause() << ") produced "
			<< num2 << " hit" << (num2>1 ? "s" : "") << ", took "
			<< timer.CPUTime() << " CPU seconds, "
			<< timer.realTime() << " elapsed seconds";
	}
	return;
    }

    if (! skip_estimation) {
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
	    ibis::util::logger lg(1);
	    lg.buffer() << "doQuery -- the number of hits is ";
	    if (num2 > num1)
		lg.buffer() << "between " << num1 << " and ";
	    lg.buffer() << num2;
	}
	if (estimate_only) {
	    if (ibis::gVerbose >= 0) {
		timer.stop();
		ibis::util::logger lg(0);
		lg.buffer() << "doQuery:: estimate("
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
	    << "doQuery -- failed to evaluate \"" << wstr
	    << "\", error code = " << num2;
	return;
    }
    if ((ordkeys && *ordkeys) || limit > 0) { // top-K query
	aQuery.limit(ordkeys, direction, limit, (verify_rid || testing > 0));
    }
    num1 = aQuery.getNumHits();

    if (asstr != 0 && *asstr != 0 && num1 > 0 && ibis::gVerbose >= 0) {
	if (0 != outputfile && 0 == strcmp(outputfile, "/dev/null")) {
	    // read the values into memory, but avoid sorting the values
	    const ibis::selected& cmps = aQuery.components();
	    const uint32_t ncol = cmps.size();
	    const ibis::bitvector* hits = aQuery.getHitVector();
	    for (uint32_t i=0; i < ncol; ++i) {
		const ibis::column* cptr = tbl->getColumn(cmps[i]);
		if (cptr != 0) {
		    ibis::colValues* tmp;
		    tmp = ibis::colValues::create(cptr, *hits);
		    delete tmp;
		}
	    }
	}
	else if (testing > 1) { // use the new cursor class for print
	    if (outputfile != 0 && *outputfile != 0) {
		std::ofstream output(outputfile,
				     std::ios::out |
				     (appendToOutput ? std::ios::app :
				      std::ios::trunc));
		if (output) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "doQuery -- query (" <<  aQuery.getWhereClause()
			<< ") results written to file \""
			<<  outputfile << "\"";
		    printQueryResults(output, aQuery);
		}
		else {
		    ibis::util::logger lg(0);
		    lg.buffer() << "Warning ** doQuery failed to open \""
				<< outputfile << "\" for writing query ("
				<< aQuery.getWhereClause() << ")";
		    printQueryResults(lg.buffer(), aQuery);
		}
	    }
	    else {
		ibis::util::logger lg(0);
		printQueryResults(lg.buffer(), aQuery);
	    }
	}
	else if (outputfile != 0 && *outputfile != 0) {
	    std::ofstream output(outputfile,
				 std::ios::out | 
				     (appendToOutput ? std::ios::app :
				      std::ios::trunc));
	    if (output) {
		LOGGER(ibis::gVerbose >= 0)
		    << "doQuery -- query (" <<  aQuery.getWhereClause()
		    << ") results written to file \""
		    <<  outputfile << "\"";
		if (ibis::gVerbose > 8 || verify_rid)
		    aQuery.printSelectedWithRID(output);
		else
		    aQuery.printSelected(output);
	    }
	    else {
		ibis::util::logger lg(0);
		lg.buffer() << "Warning ** doQuery failed to open file \""
			    << outputfile << "\" for writing query ("
			    << aQuery.getWhereClause() << ")\n";
		if (ibis::gVerbose > 8 || verify_rid)
		    aQuery.printSelectedWithRID(lg.buffer());
		else
		    aQuery.printSelected(lg.buffer());
	    }
	}
	else {
	    ibis::util::logger lg(0);
	    if (ibis::gVerbose > 8 || verify_rid)
		aQuery.printSelectedWithRID(lg.buffer());
	    else
		aQuery.printSelected(lg.buffer());
	}
	appendToOutput = true; // all query output go to the same file
    }
    if (ibis::gVerbose >= 0) {
	timer.stop();
	ibis::util::logger lg(0);
	lg.buffer() << "doQuery:: evaluate(" << sqlstring
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
    if (ibis::gVerbose > 5 || verify_rid) {
	ibis::bitvector btmp;
	num2 = aQuery.sequentialScan(btmp);
	if (num2 < 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "doQuery:: sequentialScan("
			<< aQuery.getWhereClause() << ") failed";
	}
	else {
	    num2 = btmp.cnt();
	    if (num1 != num2 && ibis::gVerbose >= 0) {
		ibis::util::logger lg(0);
		lg.buffer() << "Warning ** query \"" << aQuery.getWhereClause()
			    << "\" generated " << num1
			    << " hit" << (num1 >1  ? "s" : "")
			    << " with evaluate(), but generated "
			    << num2 << " hit" << (num2 >1  ? "s" : "")
			    << " with sequentialScan";
	    }

	    if (asstr != 0 && *asstr != 0) {
		// create bundles, i.e., retrieve the selected values
		timer.start();
		ibis::bundle* bdl = ibis::bundle::create(aQuery, btmp);
		delete bdl;
		timer.stop();
		ibis::util::logger lg(0);
		lg.buffer() << "doQuery ibis::bundle::create generated "
			    << num2 << " bundles in " << timer.CPUTime()
			    << " CPU seconds, " << timer.realTime()
			    << " elapsed seconds";
	    }
	}
    }

    if (verify_rid || testing > 1) {
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
	ibis::util::sortRIDs(*rid0);

	// retrieve the RIDs in one shot
	ibis::RIDSet* rid1 = aQuery.getRIDs();
	if (rid1 == 0) {
	    delete rid0;
	    return;
	}

	if (rid1->size() == rid0->size()) {
	    uint32_t i, cnt=0;
	    ibis::util::logger lg(0);
	    for (i=0; i<rid1->size(); ++i) {
		if ((*rid1)[i].value != (*rid0)[i].value) {
		    ++cnt;
		    lg.buffer() << i << "th RID (" << (*rid1)[i]
				<< ") != (" << (*rid0)[i] << ")\n";
		}
	    }
	    if (cnt > 0)
		lg.buffer() << "Warning -- " << cnt
			    << " mismatches out of a total of "
			    << rid1->size();
	    else
		lg.buffer() << "RID query test successful";
	}
	else if (sstr != 0) {
	    ibis::util::logger lg(0);
	    lg.buffer() << "sent " << rid1->size() << " RIDs, got back "
			<< rid0->size();
	    uint32_t i=0, cnt;
	    cnt = (rid1->size() < rid0->size()) ? rid1->size() :
		rid0->size();
	    while (i < cnt) {
		lg.buffer() << "\n(" << (*rid1)[i] << ") >>> (" << (*rid0)[i];
		++i;
	    }
	    if (rid1->size() < rid0->size()) {
		while (i < rid0->size()) {
		    lg.buffer() << "\n??? >>> (" << (*rid0)[i] << ")";
		    ++i;
		}
	    }
	    else {
		while (i < rid1->size()) {
		    lg.buffer() << "\n(" << (*rid1)[i] << ") >>> ???";
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
	ibis::util::sortRIDs(*rid1);
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
	    ibis::util::logger lg(0);
	    for (i=0; i<rid1->size(); ++i) {
		if ((*rid1)[i].value != (*rid2)[i].value) {
		    ++cnt;
		    lg.buffer() << i << "th RID (" << (*rid1)[i]
				<< ") != (" << (*rid2)[i] << ")\n";
		}
	    }
	    if (cnt > 0)
		lg.buffer() << "Warning -- " << cnt
			    << " mismatches out of a total of "
			    << rid1->size();
	    else
		lg.buffer() << "RID query test successful";
	}
	else {
	    ibis::util::logger lg(0);
	    lg.buffer() << "sent " << rid1->size() << " RIDs, got back "
			<< rid2->size();
	    uint32_t i=0, cnt;
	    cnt = (rid1->size() < rid2->size()) ? rid1->size() :
		rid2->size();
	    while (i < cnt) {
		lg.buffer() << "\n(" << (*rid1)[i] << ") >>> (" << (*rid2)[i]
			    << ")";
		++i;
	    }
	    if (rid1->size() < rid2->size()) {
		while (i < rid2->size()) {
		    lg.buffer() << "\n??? >>> (" << (*rid2)[i] << ")";
		    ++i;
		}
	    }
	    else {
		while (i < rid1->size()) {
		    lg.buffer() << "\n(" << (*rid1)[i] << ") >>> ???";
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
    LOGGER(ibis::gVerbose >= 1)
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
	    ibis::util::logger lg(0);
	    if (! comp.empty())
		lg.buffer() << "doMeshQuery -- the WHERE clause \""
			    << old.c_str() << "\" is split into \""
			    << comp.c_str() << "\" AND \""
			    << aQuery.getWhereClause() << "\"";
	    else
		lg.buffer() << "doMeshQuery -- the WHERE clause \""
			    << aQuery.getWhereClause()
			    << "\" is considered simple";
	}
    }

    const char* asstr = 0;
    if (sstr != 0 && *sstr != 0) {
	aQuery.setSelectClause(sstr);
	asstr = aQuery.getSelectClause();
    }
    if (! skip_estimation) {
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
	    ibis::util::logger lg(1);
	    lg.buffer() << "doMeshQuery -- the number of hits is ";
	    if (num1 < num2)
		lg.buffer() << "between " << num1 << " and ";
	    lg.buffer() << num2;
	}
	if (estimate_only) {
	    if (ibis::gVerbose >= 0) {
		timer.stop();
		ibis::util::logger lg(0);
		lg.buffer() << "doMeshQuery:: estimate("
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
	    << "doMeshQuery -- failed to evaluate \"" << wstr
	    << "\", error code = " << num2;
	return;
    }
    num1 = aQuery.getNumHits();
    if (ibis::gVerbose >= 0) {
	timer.stop();
	ibis::util::logger lg(0);
	lg.buffer() << "doMeshQuery:: evaluate(" << aQuery.getWhereClause() 
		    << ") produced " << num1 << (num1 > 1 ? " hits" : " hit")
		    << ", took " << timer.CPUTime() << " CPU seconds, "
		    << timer.realTime() << " elapsed seconds";
    }

    std::vector< std::vector<uint32_t> > ranges;
    num2 = aQuery.getHitsAsBlocks(ranges);
    if (num2 < 0) {
	LOGGER(ibis::gVerbose >= 1)
	    << "aQuery.getHitsAsBlocks() returned " << num2;
    }
    else if (ranges.empty()) {
	LOGGER(ibis::gVerbose >= 2)
	    << "aQuery.getHitsAsBlocks() returned empty ranges";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "aQuery.getHitsAsBlocks() returned " << ranges.size()
		    << " range" << (ranges.size() > 1 ? "s" : "") << " in "
		    << ranges[0].size()/2 << "-D space\n";
	if (ibis::gVerbose > 3) { // print all the ranges
	    uint32_t tot = (ibis::gVerbose >= 30 ? ranges.size() :
			  (1U << ibis::gVerbose));
	    if (tot > ranges.size())
		tot = ranges.size();
	    for (uint32_t i = 0; i < tot; ++i) {
		lg.buffer() << i << "\t(";
		for (uint32_t j = 0; j < ranges[i].size(); ++j) {
		    if (j > 0)
			lg.buffer() << ", ";
		    lg.buffer() << ranges[i][j];
		}
		lg.buffer() << ")\n";
	    }
	    if (tot < ranges.size()) {
		tot = ranges.size() - 1;
		lg.buffer() << "...\n" << tot << "\t(";
		for (uint32_t j = 0; j < ranges[tot].size(); ++j) {
		    if (j > 0)
			lg.buffer() << ", ";
		    lg.buffer() << ranges[tot][j];
		}
		lg.buffer() << ")";
	    }
	}
    }

    num2 = aQuery.getPointsOnBoundary(ranges);
    if (num2 < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** aQuery.getPointsOnBoundary() returned " << num2;
    }
    else if (ranges.empty()) {
	LOGGER(ibis::gVerbose >= 2)
	    << "Warning ** aQuery.getPointsOnBoundary() "
	    "returned empty ranges";
    }
    else {
	ibis::util::logger lg(0);
	lg.buffer() << "aQuery.getPointsOnBoundary() returned "
		    << ranges.size() << " point"
		    << (ranges.size() > 1 ? "s" : "") << " in "
		    << ranges[0].size() << "-D space\n";
	if (ibis::gVerbose > 3) { // print all the points
	    uint32_t tot = (ibis::gVerbose >= 30 ? ranges.size() :
			  (1U << ibis::gVerbose));
	    if (tot > ranges.size())
		tot = ranges.size();
	    if (tot < ranges.size()) {
		for (uint32_t i = 0; i < tot; ++i) {
		    lg.buffer() << i << "\t(" << ranges[i][0];
		    for (uint32_t j = 1; j < ranges[i].size(); ++j) {
			lg.buffer() << ", " << ranges[i][j];
		    }
		    lg.buffer() << ")\n";
		}
		tot = ranges.size() - 1;
		lg.buffer() << "...\n" << tot << "\t(" << ranges[tot][0];
		for (uint32_t j = 1; j < ranges[tot].size(); ++j) {
		    lg.buffer() << ", " << ranges[tot][j];
		}
		lg.buffer() << ")";
	    }
	    else {
		for (uint32_t i = 0; i < ranges.size(); ++ i) {
		    lg.buffer() << "(" << ranges[i][0];
		    for (uint32_t j = 1; j < ranges[i].size(); ++ j)
			lg.buffer() << ", " << ranges[i][j];
		    lg.buffer() << ")";
		}
	    }
	}
    }

    if (asstr != 0 && *asstr != 0 && num1 > 0 && ibis::gVerbose > 0) {
	if (outputfile != 0 && *outputfile != 0) {
	    std::ofstream output(outputfile,
				 std::ios::out | std::ios::app);
	    if (output) {
		LOGGER(ibis::gVerbose >= 1)
		    << "doMeshQuery -- query (" << aQuery.getWhereClause()
		    << ") results written to file \""
		    << outputfile << "\"";
		if (ibis::gVerbose > 8 || verify_rid)
		    aQuery.printSelectedWithRID(output);
		else
		    aQuery.printSelected(output);
	    }
	    else {
		ibis::util::logger lg(0);
		lg.buffer() << "Warning ** doMeshQuery failed to "
			    << "open file \"" << outputfile
			    << "\" for writing query ("
			    << aQuery.getWhereClause() << ") output\n";
		if (ibis::gVerbose > 8 || verify_rid)
		    aQuery.printSelectedWithRID(lg.buffer());
		else
		    aQuery.printSelected(lg.buffer());
	    }
	}
	else {
	    ibis::util::logger lg(0);
	    if (ibis::gVerbose > 8 || verify_rid)
		aQuery.printSelectedWithRID(lg.buffer());
	    else
		aQuery.printSelected(lg.buffer());
	}
    } // if (asstr != 0 && num1>0 && ibis::gVerbose > 0)
} // doMeshQuery

// append the content of the named directory to the existing partitions
static void doAppend(const char* dir, ibis::partList& tlist) {
    long ierr = 0;
    ibis::part *tbl = 0;
    bool newtable = true;
    if (appendto != 0) { // try to use the specified partition name
	for (unsigned i = 0; i < tlist.size(); ++ i) {
	    if (stricmp(appendto, tlist[i]->name()) == 0) {
		// found an existing partition
		tbl = tlist[i];
		newtable = false;
		break;
	    }
	}
    }

    if (tbl == 0) { // try the metaTags next
	char *tmp = ibis::part::readMetaTags(dir);
	if (tmp != 0) {
	    ibis::partList::iterator itt;
	    itt = tlist.begin();
	    ibis::resource::vList mtags;
	    ibis::resource::parseNameValuePairs(tmp, mtags);
	    while (itt != tlist.end()) {
		if ((*itt)->matchMetaTags(mtags))
		    break;
		++ itt;
	    }

	    if (itt != tlist.end()) { // matched the meta tags
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
	    << "doAppend(" << dir << ") failed to allocate an "
	    "ibis::part object. Can NOT continue.\n";
	return;
    }

    ibis::horometer timer;
    timer.start();
    ierr = tbl->append(dir);
    timer.stop();
    if (ierr < 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "doAppend(" << dir << "): appending to data partition \""
	    << tbl->name() << "\" failed (ierr = " << ierr << ")\n";
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
	if (ibis::gVerbose > 2 || testing > 0) {// self test after append
	    int nth = static_cast<int>(ibis::gVerbose < 20
				       ? ibis::gVerbose * 0.25
				       : 3+log((double)ibis::gVerbose));
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
	    if (ierr <= 0)
		LOGGER(ibis::gVerbose >= 0)
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
		<< "doAppend(" << dir << "): expected commit command to return "
		<< napp << ", but it actually retruned " << ierr
		<< ".  Unrecoverable error!\n";
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
	if (ibis::gVerbose > 1 && testing > 0) {
	    ierr = tbl->selfTest(0);
	    LOGGER(ibis::gVerbose >= 1)
		<< "doAppend(" << dir << "): selfTest on partition \""
		<< tbl->name() << "\" (after committing " << napp
		<< (napp > 1 ? " rows" : " row")
		<< ") encountered " << ierr
		<< (ierr > 1 ? " errors\n" : " error\n");
	}
    }
    else {
	if (ibis::gVerbose > 1 && testing > 0) {
	    ierr = tbl->selfTest(0);
	    LOGGER(ibis::gVerbose >= 1)
		<< "doAppend(" << dir << "): selfTest on partition \""
		<< tbl->name() << "\" (after appending " << napp
		<< (napp > 1 ? " rows" : " row")
		<< ") encountered " << ierr
		<< (ierr > 1 ? " errors\n" : " error\n");
	}
    }
    if (newtable) // new partition, add it to the list of partitions
	tlist.push_back(tbl);
} // doAppend

static void doJoin(const char* uid, const ibis::partList& parts,
		   ibis::joinspec& js) {
    std::ostringstream oss;
    oss << "doJoin(" << js << ")";
    ibis::util::timer tm(oss.str().c_str(), 1);
    ibis::partList::const_iterator pt1 = parts.begin();
    for (; pt1 != parts.end() && stricmp((*pt1)->name(), js.part1) != 0;
	 ++ pt1);
    if (pt1 == parts.end()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str() << ": " << js.part1
	    << " is not a know data partition";
	return;
    }
    ibis::partList::const_iterator pt2 = parts.begin();
    for (; pt2 != parts.end() && stricmp((*pt2)->name(), js.part2) != 0;
	 ++ pt2);
    if (pt2 == parts.end()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str() << ": " << js.part2
	    << " is not a know data partition";
	return;
    }
    ibis::join *jn = ibis::join::create(*(*pt1), *(*pt2), js.jcol, js.cond1,
					js.cond2);
    if (jn == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str()
	    << ": unable to construct an ibis::join object";
	return;
    }

    int64_t nhits = jn->evaluate();
    LOGGER(ibis::gVerbose >= 0)
	<< oss.str() << " -- function evaluate() returned " << nhits;
    if (nhits <= 0 || js.selcol.empty()) {
	delete jn;
	return;
    }

    ibis::join::result *res = jn->select(js.selcol);
    if (res == 0) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- " << oss.str()
	    << ": failed to create an ibis::join::result object";
	delete jn;
	return;
    }

    // print the columns name
    res->describe(std::cout);
    size_t nprint = ((nhits >> ibis::gVerbose) > 1 ? (2 << ibis::gVerbose) :
		     nhits);
    // print the first few rows of the result
    for (size_t j = 0; j < nprint; ++ j) {
	int ierr = res->fetch();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- " << oss.str() << ": failed to fetch row " << j
		<< " from the joined table with " << nhits << " row"
		<< (nhits > 1 ? "s" : "") << ", ierr = " << ierr;
	    delete res;
	    delete jn;
	    return;
	}
	ierr = res->dump(std::cout);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 0)
		<< "Warning -- " << oss.str() << ": failed to print row " << j
		<< " from the joined table, ierr = " << ierr;
	}
    }
    if (nhits > (int64_t)nprint) {
	std::cout << " ... " << nhits - nprint << " skipped" << std::endl;
    }
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

static void doDeletion(ibis::partList& tlist) {
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
	LOGGER(ibis::gVerbose >= 1)
	    << "doDeletion will invoke deactive on " << tlist.size()
	    << " data partition" << (tlist.size() > 1 ? "s" : "")
	    << " with " << rows.size() << " row number"
	    << (rows.size() > 1 ? "s" : "");

	for (ibis::partList::iterator it = tlist.begin();
	     it != tlist.end(); ++ it) {
	    long ierr = (*it)->deactivate(rows);
	    LOGGER(ibis::gVerbose >= 0)
		<< "doDeletion -- deactivate(" << (*it)->name()
		<< ") returned " << ierr;
	    if (zapping) {
		ierr = (*it)->purgeInactive();
		if (ierr < 0) {
		    LOGGER(ibis::gVerbose >= 1)
			<< "doDeletion purgeInactive(" << (*it)->name()
			<< ") returned " << ierr;
		}
	    }
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 1)
	    << "doDeletion will invoke deactive on " << tlist.size()
	    << " data partition" << (tlist.size() > 1 ? "s" : "")
	    << " with \"" << yankstring << "\"";

	for (ibis::partList::iterator it = tlist.begin();
	     it != tlist.end(); ++ it) {
	    long ierr = (*it)->deactivate(yankstring);
	    LOGGER(ibis::gVerbose >= 0)
		<< "doDeletion -- deactivate(" << (*it)->name()
		<< ", " << yankstring << ") returned " << ierr;

	    if (zapping) {
		ierr = (*it)->purgeInactive();
		if (ibis::gVerbose > 0 || ierr < 0) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "doDeletion purgeInactive(" << (*it)->name()
			<< ") returned " << ierr;
		}
	    }
	}
    }
} // doDeletion

static void reverseDeletion(ibis::partList& tlist) {
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
	LOGGER(ibis::gVerbose >= 1)
	    << "reverseDeletion will invoke deactive on " << tlist.size()
	    << " data partition" << (tlist.size() > 1 ? "s" : "")
	    << " with " << rows.size() << " row number"
	    << (rows.size() > 1 ? "s" : "");

	for (ibis::partList::iterator it = tlist.begin();
	     it != tlist.end(); ++ it) {
	    long ierr = (*it)->reactivate(rows);
	    LOGGER(ibis::gVerbose >= 0)
		<< "reverseDeletion -- reactivate(" << (*it)->name()
		<< ") returned " << ierr;
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 1)
	    << "reverseDeletion will invoke deactive on " << tlist.size()
	    << " data partition" << (tlist.size() > 1 ? "s" : "")
	    << " with \"" << keepstring << "\"";

	for (ibis::partList::iterator it = tlist.begin();
	     it != tlist.end(); ++ it) {
	    long ierr = (*it)->reactivate(keepstring);
	    LOGGER(ibis::gVerbose >= 0)
		<< "reverseDeletion -- reactivate(" << (*it)->name()
		<< ", " << keepstring << ") returned " << ierr;
	}
    }
} // reverseDeletion

// parse the query string and evaluate the specified query
static void parseString(ibis::partList& tlist, const char* uid,
			const char* qstr) {
    if (qstr == 0) return;
    if (*qstr == 0) return;

    // got a valid string
    const char* str = qstr;
    const char* end;
    std::string sstr; // select clause
    std::string wstr; // where clause
    std::string ordkeys; // order by clause (the order keys)
    int direction = 0; // direction of the order by clause
    uint32_t limit = 0; // the limit on the number of output rows
    ibis::nameList qtables;

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
	    throw "unable to locate key word WHERE in query string";
	}
	else if (end != 0) {
	    char* fstr = new char[sizeof(char) * (end - str + 1)];
	    (void) strncpy(fstr, str, end-str);
	    fstr[end-str] = 0;
	    qtables.select(fstr);
	    delete [] fstr;
	    str = end + 1;
	}
	else {
	    qtables.select(str);
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
	ibis::util::logger lg(2);
	lg.buffer() << "parseString(" << qstr
		  << ") is unable to locate key word WHERE.  "
		  << "assume the string is the where clause.";
    }
    // the end of the where clause is marked by the key words "order by" or
    // "limit" or the end of the string
    if (str != 0) {
	end = strstr(str, "order by");
	if (end == 0) {
	    end = strstr(str, "Order by");
	    if (end == 0)
		end = strstr(str, "Order By");
	    if (end == 0)
		end = strstr(str, "ORDER BY");
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
	// the order by clause may be terminated by key words "ASC", "DESC"
	// or "LIMIT"
	str += 9;
	end = strstr(str, "desc");
	if (end == 0) {
	    end = strstr(str, "Desc");
	    if (end == 0)
		end = strstr(str, "DESC");
	    if (end == 0)
		end = strstr(str, "asc");
	    if (end == 0)
		end = strstr(str, "Asc");
	    if (end == 0)
		end = strstr(str, "ASC");
	    if (end == 0)
		end = strstr(str, "limit");
	    if (end == 0)
		end = strstr(str, "Limit");
	    if (end == 0)
		end = strstr(str, "LIMIT");
	}
	if (end != 0) {
	    while (str < end) {
		ordkeys += *str;
		++ str;
	    }

	    if (0 == strnicmp(str, "desc ", 5)) {
		direction = -1;
		str += 5;
	    }
	    else if (0 == strnicmp(str, "asc ", 4)) {
		direction = 1;
		str += 4;
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
	double tmp = atof(str);
	if (tmp > 0.0)
	    limit = (uint32_t)tmp;
    }
    else if (str != 0 && *str != 0 && ibis::gVerbose >= 0) {
	ibis::util::logger lg(0);
	lg.buffer() << "Warning parseString(" << qstr
		  << ") expects the key word LIMIT, but got " << str;
    }

    // remove count(*) from select clause
    
    if (! sstr.empty()) {
	if (! qtables.empty()) {
	    ibis::partList tl2;
	    for (unsigned k = 0; k < tlist.size(); ++ k) {
		for (unsigned j = 0; j < qtables.size(); ++ j) {
		    if (stricmp(tlist[k]->name(), qtables[j]) == 0 ||
			ibis::util::strMatch(tlist[k]->name(), qtables[j])) {
			tl2.push_back(tlist[k]);
			break;
		    }
		}
	    }
	    tableSelect(tl2, uid, wstr.c_str(), sstr.c_str(),
			ordkeys.c_str(), direction, limit);
	}
	else {
	    tableSelect(tlist, uid, wstr.c_str(), sstr.c_str(),
			ordkeys.c_str(), direction, limit);
	}
    }
    else if (! qtables.empty()) {
	for (unsigned k = 0; k < tlist.size(); ++ k) {
	    // go through each partition the user has specified and process
	    // the queries
	    for (unsigned j = 0; j < qtables.size(); ++ j) {
		if (stricmp(tlist[k]->name(), qtables[j]) == 0 ||
		    ibis::util::strMatch(tlist[k]->name(), qtables[j])) {
		    if (verify_rid || sequential_scan ||
			tlist[k]->getMeshShape().empty())
			doQuery(tlist[k], uid, wstr.c_str(), sstr.c_str(),
				ordkeys.c_str(), direction, limit);
		    else
			doMeshQuery(tlist[k], uid, wstr.c_str(), sstr.c_str());

		    if (ibis::gVerbose > 10 || testing > 0)
			xdoQuery(tlist[k], uid, wstr.c_str(), sstr.c_str());
		    break;
		}
	    }
	}
    }
    else { // go through every partition and process the user query
	for (ibis::partList::iterator tit = tlist.begin();
	     tit != tlist.end(); ++tit) {
	    if (verify_rid || sequential_scan || (*tit)->getMeshShape().empty())
		doQuery((*tit), uid, wstr.c_str(), sstr.c_str(),
			ordkeys.c_str(), direction, limit);
	    else
		doMeshQuery((*tit), uid, wstr.c_str(), sstr.c_str());

	    if (ibis::gVerbose > 10 || testing > 0)
		xdoQuery((*tit), uid, wstr.c_str(), sstr.c_str());
	}
    }
} // parseString

extern "C" void* thFun(void* arg) {
    thArg* myArg = (thArg*)arg; // recast the argument to the right type
    for (unsigned j = myArg->task(); j < myArg->qlist.size();
	 j = myArg->task()) {
	parseString(myArg->tlist, myArg->uid, myArg->qlist[j]);
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

static void clean_up(ibis::partList& tlist, bool sane=true) {
    { // use envLock to make sure only one thread is deleting the partitions
	const size_t np = tlist.size();
	ibis::util::quietLock lock(&ibis::util::envLock);
	if (tlist.empty())
	    return;
	for (unsigned i = 0; i < np; ++ i) {
	    delete tlist[i];
	    tlist[i] = 0;
	}
	tlist.clear();
    }
    LOGGER(ibis::gVerbose >= 2)
	<< "Cleaning up the file manager\n"
	"Total pages accessed through read(unistd.h) is estimated to be "
	<< ibis::fileManager::instance().pageCount();

    if (sane)
	ibis::fileManager::instance().clear();
    if (ibis::gVerbose >= 4) {
	ibis::util::logger lg(4);
	ibis::fileManager::instance().printStatus(lg.buffer());
    }

#if defined(RUSAGE_SELF) && defined(RUSAGE_CHILDREN)
    if (ibis::gVerbose >= 2) {
	// getrusage might not fill all the fields
	struct rusage ruse0, ruse1;
	int ierr = getrusage(RUSAGE_SELF, &ruse0);
	ierr |= getrusage(RUSAGE_CHILDREN, &ruse1);
	if (ierr == 0) {
	    ibis::util::logger lg(2);
	    lg.buffer()
		<< "Report from getrusage: maxrss = "
		<< ruse0.ru_maxrss + ruse1.ru_maxrss
		<< " pages (" << getpagesize() << " bytes/page)"
		<< ", majflt = " << ruse0.ru_majflt + ruse1.ru_majflt
		<< ", minflt = " << ruse0.ru_minflt + ruse1.ru_minflt
		<< ", inblock = " << ruse0.ru_inblock + ruse1.ru_inblock
		<< ", outblock = " << ruse0.ru_oublock + ruse1.ru_oublock;
	}
    }
#endif
#if defined(_MSC_VER) && defined(_WIN32) && (defined(_DEBUG) || defined(DEBUG))
    std::cout << "\n*** DEBUG: report from _CrtMemDumpAllObjectsSince\n";
    _CrtMemDumpAllObjectsSince(NULL);
    _CrtDumpMemoryLeaks();
#endif
} // clean_up

int main(int argc, char** argv) {
    ibis::partList tlist;
    if (argc <= 1) {
	usage(*argv);
	return 0;
    }

    try {
	int interactive;
	stringList alist, qlist, slist;
	ibis::joinlist joins;
	std::vector<std::string> queff; // queries read from files (-f)
	const char* uid = ibis::util::userName();
	ibis::horometer timer; // total elapsed time
	timer.start();

	// parse the command line arguments
	parse_args(argc, argv, interactive, tlist, qlist, alist, slist,
		   queff, joins);

	// add new data if any
	for (stringList::const_iterator it = alist.begin();
	     it != alist.end();
	     ++ it) { // add new data before doing anything else
	    doAppend(*it, tlist);
	}
	alist.clear(); // no more use for it

	if (yankstring != 0 && *yankstring != 0)
	    doDeletion(tlist);
	if (keepstring != 0 && *keepstring != 0)
	    reverseDeletion(tlist);

	// build new indexes
	if (build_index > 0 && ! tlist.empty()) {
	    LOGGER(ibis::gVerbose >= 1)
		<< *argv << ": start building indexes (nthreads="
		<< build_index << ", indexingOption="
		<< (indexingOption ? indexingOption : "-") << ") ...";
	    ibis::horometer timer1;
	    timer1.start();
	    for (ibis::partList::const_iterator it = tlist.begin();
		 it != tlist.end(); ++ it) {
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
		<< *argv << ": building indexes for " << tlist.size()
		<< " data partition" << (tlist.size()>1 ? "s" : "") << " took "
		<< timer1.CPUTime() << " CPU seconds, "
		<< timer1.realTime() << " elapsed seconds\n";
	}
	// sort the specified columns
	if (slist.size() > 0) {
	    ibis::horometer timer2;
	    timer2.start();
	    for (ibis::partList::const_iterator it = tlist.begin();
		 it != tlist.end(); ++ it) {
		for (size_t j = 0; j < slist.size(); ++ j)
		    (*it)->buildSorted(slist[j]);
	    }
	    timer2.stop();
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << ": building sorted versions of " << slist.size()
		<< " column" << (slist.size()>1 ? "s" : "") << tlist.size()
		<< " data partition" << (tlist.size()>1 ? "s" : "") << " took "
		<< timer2.CPUTime() << " CPU seconds, "
		<< timer2.realTime() << " elapsed seconds\n";
	    slist.clear(); // no longer needed
	}

	// performing self test
	if (testing > 0 && ! tlist.empty()) {
	    LOGGER(ibis::gVerbose >= 1) << *argv << ": start testing ...";
	    ibis::horometer timer3;
	    timer3.start();
	    for (ibis::partList::const_iterator it = tlist.begin();
		 it != tlist.end(); ++ it) {
		// tell the partition to perform self tests
		long nerr = (*it)->selfTest(testing);
		(*it)->unloadIndexes();

		if (ibis::gVerbose >= 0) {
		    ibis::util::logger lg(0);
		    lg.buffer() << "self tests on " << (*it)->name();
		    if (nerr == 0)
			lg.buffer() << " found no error";
		    else if (nerr == 1)
			lg.buffer() << " found 1 error";
		    else if (nerr > 1)
			lg.buffer() << " found " << nerr << " errors";
		    else
			lg.buffer() << " returned unexpected value " << nerr;
		}
	    }
	    timer3.stop();
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << ": testing " << tlist.size() << " data partition"
		<< (tlist.size()>1 ? "s" : "") << " took "
		<< timer3.CPUTime() << " CPU seconds, "
		<< timer3.realTime() << " elapsed seconds\n";
	}

	if (tlist.empty() && !qlist.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< *argv << " must have at least one data partition "
		"to process any query.";
	}
	else if (qlist.size() > 1 && threading > 0) {
#if defined(_DEBUG) || defined(DEBUG)
	    for (stringList::const_iterator it = qlist.begin();
		 it != qlist.end(); ++it) {
		parseString(tlist, uid, *it);
	    }
#else
	    // process queries in a thread pool
	    const int nth =
		(threading < qlist.size() ? threading : qlist.size()-1);
	    ibis::util::counter taskpool;
	    thArg args(uid, qlist, tlist, taskpool);
	    std::vector<pthread_t> tid(nth);
	    for (int i =0; i < nth; ++ i) { // 
		int ierr = pthread_create(&(tid[i]), 0, thFun, (void*)&args);
		if (ierr != 0) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "pthread_create failed to create " << i
			<< "th thread";
		    return(-5);
		}
	    }
	    thFun((void*)&args); // this thread do something too
	    for (int i = 0; i < nth; ++ i) {
		int status;
		int ierr = pthread_join(tid[i], (void**)&status);
		if (ierr != 0) {
		    LOGGER(ibis::gVerbose >= 0)
			<< "pthread_join failed on the " << i
			<< "th thread";
		}
	    }
#endif
	    queff.clear();
	    qlist.clear();
	}
	else if (qlist.size() > 0) { // no new threads
	    for (stringList::const_iterator it = qlist.begin();
		 it != qlist.end(); ++it) {
		parseString(tlist, uid, *it);
	    }
	    queff.clear();
	    qlist.clear();
	}
	else if (ridfile != 0) {
	    for (ibis::partList::iterator itt = tlist.begin();
		 itt != tlist.end();
		 ++ itt)
		doQuery((*itt), uid, 0, 0, 0, 0, 0);
	}
	ridfile = 0;

	// process the joins one at a time
	for (size_t j = 0; j < joins.size(); ++j) {
	    doJoin(uid, tlist, joins[j]);
	}

	if (interactive) {	// iteractive operations
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
		    clean_up(tlist);
		    return(0);
		case 'p': // print command
		case 'P':
		    print(str.c_str(), tlist); break;
		case 's': // query must start with of the key words
		case 'f':
		case 'w':
		case 'S':
		case 'F':
		case 'W':
		    //std::cout << str << std::endl;
		    parseString(tlist, uid, str.c_str()); break;
		case 'a':
		case 'A': {
		    const char* dir = str.c_str();
		    while(isalpha(*dir)) ++dir; // skip key word append
		    while(isspace(*dir)) ++dir; // skip space
		    doAppend(dir, tlist);
		    break;}
		}
	    }
	}

	clean_up(tlist);
	timer.stop();
	if (timer.realTime() > 0.001)
	    LOGGER(ibis::gVerbose >= 2)
		<< *argv << ":: total CPU time " << timer.CPUTime()
		<< " s, total elapsed time " << timer.realTime() << " s";

	// last thing -- close the file logging the messages
	ibis::util::closeLogFile();
	return 0;
    }
    catch (const std::exception& e) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** " << *argv
	    << " received a standard exception\n" << e.what();
	//clean_up(tlist, false);
	return -10;
    }
    catch (const char* s) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** " << *argv
	    << " received a string exception\n" << s;
	//clean_up(tlist, false);
	return -11;
    }
    catch (...) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning ** " << *argv
	    << " received an unexpected exception";
	//clean_up(tlist, false);
	return -12;
    }
} // main

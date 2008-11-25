// File $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2000-2008 the Regents of the University of California
//
// Implements ibis::part histogram functions.
#include "index.h"	// ibis::index::divideCounts
#include "query.h"	// ibis::query
#include "part.h"

#include <cmath>	// std::ceil, std::log, ...
#include <limits>	// std::numeric_limits
#include <typeinfo>	// typeid

// This file definte does not use the min and max macro.  Their presence
// could cause the calls to numeric_limits::min and numeric_limits::max to
// be misunderstood!
#undef max
#undef min

/// Count the number of records falling in the regular bins defined by the
/// <tt>begin:end:stride</tt> triplet.  The triplets defines
/// <tt> 1 + floor((end-begin)/stride) </tt> bins:
/// @code
/// [begin, begin+stride)
/// [begin+stride, begin+stride*2)
/// ...
/// [begin+stride*floor((end-begin)/stride), end].
/// @endcode
///
/// When this function completes successfully, the array @c counts shall
/// have <tt> 1+floor((end-begin)/stride) </tt> elements, one for each bin.
/// The return value shall be the number of bins.  Any other value
/// indicates an error.  If array @c counts has the same size as the number
/// of bins on input, the count values will be added to the array.  This is
/// intended to be used to accumulate counts from different data
/// partitions.  If the array @c counts does not have the correct size, it
/// will be resized to the correct size and initialized to zero before
/// counting the the current data partition.
///
/// This function proceeds by first evaluate the constraints, then retrieve
/// the selected values, and finally count the number of records in each
/// bin.
///
/// @sa ibis::table::getHistogram
long ibis::part::get1DDistribution(const char *constraints, const char *cname,
				   double begin, double end, double stride,
				   std::vector<uint32_t> &counts) const {
    if (cname == 0 || *cname == 0 || (begin >= end && !(stride < 0.0)) ||
	(begin <= end && !(stride > 0.0)))
	return -1L;

    const ibis::column* col = getColumn(cname);
    if (col == 0)
	return -2L;

    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get1DDistribution attempting to compute a histogram of "
	    << cname << " with regular binning "
	    << (constraints && *constraints ? " subject to " :
		" without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }
    const size_t nbins = 1 + 
	static_cast<uint32_t>(std::floor((end - begin) / stride));
    if (counts.size() != nbins) {
	counts.resize(nbins);
	for (size_t i = 0; i < nbins; ++i)
	    counts[i] = 0;
    }

    long ierr;
    ibis::bitvector mask;
    {
	ibis::query qq(ibis::util::userName(), this);
	qq.setSelectClause(cname);
	std::ostringstream oss;
	if (constraints != 0 && *constraints != 0)
	    oss << "(" << constraints << ") AND ";
	oss << cname << " between " << std::setprecision(18) << begin
	    << " and " << std::setprecision(18) << end;
	qq.setWhereClause(oss.str().c_str());

	ierr = qq.evaluate();
	if (ierr < 0)
	    return ierr;
	ierr = nbins;
	mask.copy(*(qq.getHitVector()));
	if (mask.cnt() == 0)
	    return ierr;
    }

    switch (col->type()) {
    case ibis::BYTE:
    case ibis::SHORT:
    case ibis::INT: {
	array_t<int32_t>* vals = col->selectInts(mask);
	if (vals != 0) {
	    for (size_t i = 0; i < vals->size(); ++ i) {
		++ counts[static_cast<uint32_t>(((*vals)[i] - begin) / stride)];
	    }
	    delete vals;
	}
	else {
	    ierr = -4;
	}
	break;}
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::UINT: {
	array_t<uint32_t>* vals = col->selectUInts(mask);
	if (vals != 0) {
	    for (size_t i = 0; i < vals->size(); ++ i) {
		++ counts[static_cast<uint32_t>(((*vals)[i] - begin) / stride)];
	    }
	    delete vals;
	}
	else {
	    ierr = -4;
	}
	break;}
    case ibis::ULONG:
    case ibis::LONG: {
	array_t<int64_t>* vals = col->selectLongs(mask);
	if (vals != 0) {
	    for (size_t i = 0; i < vals->size(); ++ i) {
		++ counts[static_cast<uint32_t>(((*vals)[i] - begin) / stride)];
	    }
	    delete vals;
	}
	else {
	    ierr = -4;
	}
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals = col->selectFloats(mask);
	if (vals != 0) {
	    for (size_t i = 0; i < vals->size(); ++ i) {
		++ counts[static_cast<uint32_t>(((*vals)[i] - begin) / stride)];
	    }
	    delete vals;
	}
	else {
	    ierr = -4;
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals = col->selectDoubles(mask);
	if (vals != 0) {
	    for (size_t i = 0; i < vals->size(); ++ i) {
		++ counts[static_cast<uint32_t>(((*vals)[i] - begin) / stride)];
	    }
	    delete vals;
	}
	else {
	    ierr = -4;
	}
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::get1DDistribution -- unable to "
	    "handle column (" << cname << ") type "
	    << ibis::TYPESTRING[(int)col->type()];

	ierr = -3;
	break;}
    }
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("get1DDistribution", "computing the distribution of column "
		   "%s%s%s took %g sec(CPU), %g sec(elapsed)",
		   cname, (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::get1DDistribution

template <typename T1, typename T2>
long ibis::part::count2DBins(array_t<T1> &vals1,
			     const double &begin1, const double &end1,
			     const double &stride1,
			     array_t<T2> &vals2,
			     const double &begin2, const double &end2,
			     const double &stride2,
			     std::vector<uint32_t> &counts) const {
    const size_t dim2 = 1+
	static_cast<uint32_t>(std::floor((end2-begin2)/stride2));
    const size_t nr = (vals1.size() <= vals2.size() ?
		       vals1.size() : vals2.size());
#if defined(SORT_VALUES_BEFORE_COUNT)
    ibis::util::sortall(vals1, vals2);
// #else
//     if (counts.size() > 4096)
// 	ibis::util::sortall(vals1, vals2);
#endif
    for (size_t ir = 0; ir < nr; ++ ir) {
	++ counts[dim2 * static_cast<uint32_t>((vals1[ir]-begin1)/stride1) +
		  static_cast<uint32_t>((vals2[ir]-begin2)/stride2)];
    }
    return counts.size();
} // ibis::part::count2DBins

template <typename T1, typename T2, typename T3>
long ibis::part::count3DBins(const array_t<T1> &vals1,
			     const double &begin1, const double &end1,
			     const double &stride1,
			     const array_t<T2> &vals2,
			     const double &begin2, const double &end2,
			     const double &stride2,
			     const array_t<T3> &vals3,
			     const double &begin3, const double &end3,
			     const double &stride3,
			     std::vector<uint32_t> &counts) const {
    const size_t dim3 = 1 +
	static_cast<uint32_t>(std::floor((end3 - begin3)/stride3));
    const size_t dim2 = 1 +
	static_cast<uint32_t>(std::floor((end2 - begin2)/stride2));
    const size_t nr = (vals1.size() <= vals2.size() ?
		       (vals1.size() <= vals3.size() ?
			vals1.size() : vals3.size()) :
		       (vals2.size() <= vals3.size() ?
			vals2.size() : vals3.size()));
    for (size_t ir = 0; ir < nr; ++ ir) {
	++ counts[(static_cast<uint32_t>((vals1[ir]-begin1)/stride1) * dim2 +
		   static_cast<uint32_t>((vals2[ir]-begin2)/stride2)) * dim3 +
		  static_cast<uint32_t>((vals3[ir]-begin3)/stride3)];
    }
    return counts.size();
} // ibis::part::count3DBins

/// Count the number of values in 2D regular bins.
/// @sa ibis::part::get1DDistribution
/// @sa ibis::table::getHistogram2D
long ibis::part::get2DDistribution(const char *constraints, const char *cname1,
				   double begin1, double end1, double stride1,
				   const char *cname2,
				   double begin2, double end2, double stride2,
				   std::vector<uint32_t> &counts) const {
    if (cname1 == 0 || *cname1 == 0 || (begin1 >= end1 && !(stride1 < 0.0)) ||
	(begin1 <= end1 && !(stride1 > 0.0)) ||
	cname2 == 0 || *cname2 == 0 || (begin2 >= end2 && !(stride2 < 0.0)) ||
	(begin2 <= end2 && !(stride2 > 0.0)))
	return -1L;

    const ibis::column* col1 = getColumn(cname1);
    const ibis::column* col2 = getColumn(cname2);
    if (col1 == 0 || col2 == 0)
	return -2L;

    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistribution attempting to compute a histogram of "
	    << cname1 << " and " << cname2 << " with regular binning "
	    << (constraints && *constraints ? " subject to " :
		" without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }
    const size_t nbins =
	(1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	(1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2)));
    if (counts.size() != nbins) {
	counts.resize(nbins);
	for (size_t i = 0; i < nbins; ++i)
	    counts[i] = 0;
    }

    long ierr;
    ibis::bitvector hits;
    {
	ibis::query qq(ibis::util::userName(), this);
	std::string sel = cname1;
	sel += ',';
	sel += cname2;
	qq.setSelectClause(sel.c_str());

	// add constraints on the two selected variables
	std::ostringstream oss;
	if (constraints != 0 && *constraints != 0)
	    oss << "(" << constraints << ") AND ";
	oss << cname1 << " between " << std::setprecision(18) << begin1
	    << " and " << std::setprecision(18) << end1 << " AND " << cname2
	    << std::setprecision(18) << " between " << std::setprecision(18)
	    << begin2 << " and " << std::setprecision(18) << end2;
	qq.setWhereClause(oss.str().c_str());

	ierr = qq.evaluate();
	if (ierr < 0)
	    return ierr;
	ierr = nbins;
	if (qq.getNumHits() == 0)
	    return ierr;
	hits.copy(*(qq.getHitVector()));
    }

    switch (col1->type()) {
    case ibis::BYTE:
    case ibis::SHORT:
    case ibis::INT: {
	array_t<int32_t>* vals1 = col1->selectInts(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::UINT: {
	array_t<uint32_t>* vals1 = col1->selectUInts(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::ULONG:
    case ibis::LONG: {
	array_t<int64_t>* vals1 = col1->selectLongs(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals1 = col1->selectFloats(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals1 = col1->selectDoubles(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::get2DDistribution -- unable to "
	    "handle column (" << cname1 << ") type "
	    << ibis::TYPESTRING[(int)col1->type()];

	ierr = -3;
	break;}
    }
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("get2DDistribution", "computing the joint distribution of "
		   "column %s and %s%s%s took %g sec(CPU), %g sec(elapsed)",
		   cname1, cname2, (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::get2DDistribution

/// This function defines exactly @code
/// (1 + floor((end1-begin1)/stride1)) *
/// (1 + floor((end2-begin2)/stride2)) *
/// (1 + floor((end3-begin3)/stride3))
/// @endcode regularly spaced bins.
/// On successful completion of this function, the return value shall be
/// the number of bins.  Any other value indicates an error.
///
/// @sa ibis::part::get1DDistribution
/// @sa ibis::table::getHistogram2D
long ibis::part::get3DDistribution(const char *constraints, const char *cname1,
				   double begin1, double end1, double stride1,
				   const char *cname2,
				   double begin2, double end2, double stride2,
				   const char *cname3,
				   double begin3, double end3, double stride3,
				   std::vector<uint32_t> &counts) const {
    if (cname1 == 0 || *cname1 == 0 || (begin1 >= end1 && !(stride1 < 0.0)) ||
	(begin1 <= end1 && !(stride1 > 0.0)) ||
	cname2 == 0 || *cname2 == 0 || (begin2 >= end2 && !(stride2 < 0.0)) ||
	(begin2 <= end2 && !(stride2 > 0.0)) ||
	cname3 == 0 || *cname3 == 0 || (begin3 >= end3 && !(stride3 < 0.0)) ||
	(begin3 <= end3 && !(stride3 > 0.0)))
	return -1L;

    const ibis::column* col1 = getColumn(cname1);
    const ibis::column* col2 = getColumn(cname2);
    const ibis::column* col3 = getColumn(cname3);
    if (col1 == 0 || col2 == 0 || col3 == 0)
	return -2L;

    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get3DDistribution attempting to compute a histogram of "
	    << cname1 << ", " << cname2 << ", and " << cname3
	    << " with regular binning "
	    << (constraints && *constraints ? " subject to " :
		" without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }
    const size_t nbins =
	(1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	(1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2))) *
	(1 + static_cast<uint32_t>(std::floor((end3 - begin3) / stride3)));
    if (counts.size() != nbins) {
	counts.resize(nbins);
	for (size_t i = 0; i < nbins; ++i)
	    counts[i] = 0;
    }

    long ierr;
    ibis::bitvector hits;
    {
	ibis::query qq(ibis::util::userName(), this);
	std::string sel = cname1;
	sel += ',';
	sel += cname2;
	sel += ',';
	sel += cname3;
	qq.setSelectClause(sel.c_str());

	// add constraints on the two selected variables
	std::ostringstream oss;
	if (constraints != 0 && *constraints != 0)
	    oss << "(" << constraints << ") AND ";
	oss << cname1 << " between " << std::setprecision(18) << begin1
	    << " and " << std::setprecision(18) << end1
	    << " AND " << cname2 << " between " << std::setprecision(18)
	    << begin2 << " and " << std::setprecision(18) << end2
	    << " AND " << cname3 << " between " << std::setprecision(18)
	    << begin3 << " and " << std::setprecision(18) << end3;
	qq.setWhereClause(oss.str().c_str());
	ierr = qq.evaluate();
	if (ierr < 0)
	    return ierr;
	ierr = nbins;
	hits.copy(*(qq.getHitVector()));
	if (hits.cnt() == 0)
	    return ierr;
    }

    switch (col1->type()) {
    case ibis::BYTE:
    case ibis::SHORT:
    case ibis::INT: {
	array_t<int32_t>* vals1 = col1->selectInts(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get3DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::UINT: {
	array_t<uint32_t>* vals1 = col1->selectUInts(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get3DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::ULONG:
    case ibis::LONG: {
	array_t<int64_t>* vals1 = col1->selectLongs(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get3DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals1 = col1->selectFloats(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get3DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals1 = col1->selectDoubles(hits);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2->type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2->selectInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2->selectUInts(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2->selectLongs(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2->selectFloats(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2->selectDoubles(hits);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }

	    switch (col3->type()) {
	    case ibis::BYTE:
	    case ibis::SHORT:
	    case ibis::INT: {
		array_t<int32_t>* vals3 =
		    col3->selectInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::UBYTE:
	    case ibis::USHORT:
	    case ibis::UINT: {
		array_t<uint32_t>* vals3 =
		    col3->selectUInts(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::ULONG:
	    case ibis::LONG: {
		array_t<int64_t>* vals3 =
		    col3->selectLongs(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end2, stride3, counts);
		delete vals3;
		break;}
	    case ibis::FLOAT: {
		array_t<float>* vals3 =
		    col3->selectFloats(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals3, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    case ibis::DOUBLE: {
		array_t<double>* vals3 =
		    col3->selectDoubles(hits);
		if (vals3 == 0) {
		    ierr = -6;
		    break;
		}
		ierr = count3DBins(*vals1, begin1, end1, stride1,
				   *vals2, begin2, end2, stride2,
				   *vals2, begin3, end3, stride3, counts);
		delete vals3;
		break;}
	    default: {
		LOGGER(ibis::gVerbose >= 4)
		    << "ibis::part::get3DDistribution -- unable to "
		    "handle column (" << cname3 << ") type "
		    << ibis::TYPESTRING[(int)col3->type()];

		ierr = -3;
		break;}
	    }
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get3DDistribution -- unable to "
		"handle column (" << cname2 << ") type "
		<< ibis::TYPESTRING[(int)col2->type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::get3DDistribution -- unable to "
	    "handle column (" << cname1 << ") type "
	    << ibis::TYPESTRING[(int)col1->type()];

	ierr = -3;
	break;}
    }
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("get3DDistribution", "computing the joint distribution of "
		   "columns %s, %s, and %s%s%s took %g sec(CPU), %g "
		   "sec(elapsed)", cname1, cname2, cname2,
		   (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::get3DDistribution

/// The caller specify the number of bins, but not the where to place the
/// bins.  The bounds array contains one more element than the counts array
/// and all the bins defined by the bounds are closed ranges.  More
/// specifically, the number of elements with values between
/// @code [bounds[i], bounds[i+1]) @endcode
/// is stored in @c counts[i].  Note that the lower bound of a range is
/// included in the bin, but the upper bound of a bin is excluded from the
/// bin.
/// @note The output number of bins may not be the input value nbin.
long ibis::part::get1DDistribution(const char* cname, uint32_t nbin,
				   std::vector<double> &bounds,
				   std::vector<uint32_t> &counts) const {
    if (cname == 0 || *cname == 0 || nEvents == 0) {
	return -1;
    }

    const ibis::column* col = getColumn(cname);
    if (col == 0) {
	return -2;
    }

    return get1DDistribution(*col, nbin, bounds, counts);
} // ibis::part::get1DDistribution

/// Calls function ibis::column::getDistribution to create the internal
/// histogram first, then pack them into a smaller number of bins if
/// necessary.
/// @note The output number of bins may not be the input value nbin.
long ibis::part::get1DDistribution(const ibis::column &col, uint32_t nbin,
				   std::vector<double> &bounds,
				   std::vector<uint32_t> &counts) const {
    const double amin = col.getActualMin();
    const double amax = col.getActualMax();
    long ierr = col.getDistribution(bounds, counts);
    if (ierr < 0) return ierr;

    if (static_cast<unsigned>(ierr) > nbin*3/2) {
	// too many bins returned, combine some of them
	ibis::util::buffer<double> bbs(nbin+1);
	ibis::util::buffer<uint32_t> cts(nbin+1);
	double* pbbs = bbs.address();
	uint32_t* pcts = cts.address();
	if (pbbs != 0 && pcts != 0) {
	    ierr = packDistribution(bounds, counts, nbin, pbbs, pcts);
	    if (ierr > 1) { // use the packed bins
		bounds.resize(ierr+1);
		bounds[0] = amin;
		for (int i = 0; i < ierr; ++ i)
		    bounds[i+1] = pbbs[i];
		bounds[ierr] = (col.isFloat() ? ibis::util::incrDouble(amax) :
				std::floor(amax)+1.0);
		counts.resize(ierr);
		for (int i = 0; i < ierr; ++ i)
		    counts[i] = pcts[i];
		return ierr;
	    }
	}
    }

    if (counts[0] > 0) { // add the actual minimal as the bounds[0]
	bounds.reserve(counts.size()+1);
	bounds.resize(bounds.size()+1);
	for (size_t i = bounds.size()-1; i > 0; -- i)
	    bounds[i] = bounds[i-1];
	bounds[0] = amin;
    }
    else {
	const size_t nc = counts.size() - 1;
	for (size_t i = 0; i < nc; ++ i)
	    counts[i] = counts[i+1];
	counts.resize(nc);
    }
    if (counts.back() > 0) { // add the largest values as the end of last bin
	if (amax - bounds.back() >= 0.0) {
	    if (col.isFloat()) {
		double tmp;
		if (bounds.size() > 1)
		    tmp = ibis::util::compactValue
			(amax, amax + (bounds[bounds.size()-1] -
				       bounds[bounds.size()-2]));
		else
		    tmp = ibis::util::incrDouble(amax);
		bounds.push_back(tmp);
	    }
	    else {
		bounds.push_back(std::floor(amax) + 1.0);
	    }
	}
	else {
	    bounds.push_back(ibis::util::compactValue(bounds.back(), DBL_MAX));
	}
    }
    else {
	counts.resize(counts.size()-1);
    }
    return counts.size();
} // ibis::part::get1DDistribution

/// @note The output number of bins may not be the input value nbins.
long ibis::part::get1DDistribution(const char* constraints,
				   const char* cname, uint32_t nbins,
				   std::vector<double> &bounds,
				   std::vector<uint32_t> &counts) const {
    if (cname == 0 || *cname == 0 || nEvents == 0) {
	return -1L;
    }

    const ibis::column* col = getColumn(cname);
    if (col == 0)
	return -2L;
    if (constraints == 0 || *constraints == 0 || *constraints == '*')
	return get1DDistribution(*col, nbins, bounds, counts);

    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get1DDistribution attempting to compute a histogram of "
	    << cname << " with adaptive binning subject to " << constraints;
	timer.start();
    }

    long ierr;
    ibis::bitvector mask;
    {
	ibis::query qq(ibis::util::userName(), this);
	ierr = qq.setSelectClause(cname);
	if (ierr < 0)
	    return -3;
	ierr = qq.setWhereClause(constraints);
	if (ierr < 0) {
	    return -4;
	}

	ierr = qq.evaluate();
	if (ierr < 0) {
	    return -5;
	}
	if (qq.getNumHits() == 0) {
	    bounds.clear();
	    counts.clear();
	    return 0;
	}

	mask.copy(*(qq.getHitVector()));
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get1DDistribution -- the constraints \"" << constraints
	    << "\" selects " << mask.cnt() << " record"
	    << (mask.cnt() > 1 ? "s" : "") << " out of " << nEvents;
    }

    switch (col->type()) {
    case ibis::BYTE: {
	array_t<char> *vals = col->selectBytes(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "bytes" : "byte")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "bytes" : "byte")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	ierr = adaptiveInts<char>(*vals, (char)-128, (char)127,
				  nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char> *vals = col->selectUBytes(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "bytes" : "byte")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "bytess" : "byte")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	ierr = adaptiveInts<unsigned char>
	    (*vals, (unsigned char)0, (unsigned char)255,
	     nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::SHORT: {
	array_t<int16_t> *vals = col->selectShorts(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "int16_ts" : "int16_t")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "int16_ts" : "int16_t")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	int16_t vmin = (int16_t)-32768;
	int16_t vmax = (int16_t)32767;
	if (vals->size() < static_cast<uint32_t>(vmax)) {
	    // compute the actual min and max
	    vmin = (*vals)[0];
	    vmax = (*vals)[1];
	    for (uint32_t i = 1; i < vals->size(); ++ i) {
		if ((*vals)[i] > vmax)
		    vmax = (*vals)[i];
		if ((*vals)[i] < vmin)
		    vmin = (*vals)[i];
	    }
	}
	ierr = adaptiveInts(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::USHORT: {
	array_t<uint16_t> *vals = col->selectUShorts(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "uint16_ts" : "uint16_t")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "uint16_ts" : "uint16_t")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	uint16_t vmin = 0;
	uint16_t vmax = (uint16_t)65535;
	if (vals->size() < 32767) {
	    // compute the actual min and max
	    vmin = (*vals)[0];
	    vmax = (*vals)[1];
	    for (uint32_t i = 1; i < vals->size(); ++ i) {
		if ((*vals)[i] > vmax)
		    vmax = (*vals)[i];
		if ((*vals)[i] < vmin)
		    vmin = (*vals)[i];
	    }
	}
	ierr = adaptiveInts(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::INT: {
	array_t<int32_t> *vals = col->selectInts(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "int32_ts" : "int32_t")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "int32_ts" : "int32_t")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	int32_t vmin = (*vals)[0];
	int32_t vmax = (*vals)[0];
	for (uint32_t i = 1; i < vals->size(); ++ i) {
	    if ((*vals)[i] > vmax)
		vmax = (*vals)[i];
	    if ((*vals)[i] < vmin)
		vmin = (*vals)[i];
	}
	if (static_cast<uint32_t>(vmax-vmin) < vals->size())
	    ierr = adaptiveInts(*vals, vmin, vmax, nbins, bounds, counts);
	else
	    ierr = adaptiveFloats(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::UINT: {
	array_t<uint32_t> *vals = col->selectUInts(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "uint32_ts" : "uint32_t")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "uint32_ts" : "uint32_t")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	uint32_t vmin = (*vals)[0];
	uint32_t vmax = (*vals)[0];
	for (uint32_t i = 1; i < vals->size(); ++ i) {
	    if ((*vals)[i] > vmax)
		vmax = (*vals)[i];
	    if ((*vals)[i] < vmin)
		vmin = (*vals)[i];
	}
	if (vmax-vmin < vals->size())
	    ierr = adaptiveInts(*vals, vmin, vmax, nbins, bounds, counts);
	else
	    ierr = adaptiveFloats(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::LONG: {
	array_t<int64_t> *vals = col->selectLongs(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "int64_ts" : "int64_t")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "int64_ts" : "int64_t")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	int64_t vmin = (*vals)[0];
	int64_t vmax = (*vals)[0];
	for (uint32_t i = 1; i < vals->size(); ++ i) {
	    if ((*vals)[i] > vmax)
		vmax = (*vals)[i];
	    if ((*vals)[i] < vmin)
		vmin = (*vals)[i];
	}
	if (vmax-vmin < static_cast<int64_t>(vals->size()))
	    ierr = adaptiveInts(*vals, vmin, vmax, nbins, bounds, counts);
	else
	    ierr = adaptiveFloats(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> *vals = col->selectULongs(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "uint64_ts" : "uint64_t")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "uint64_ts" : "uint64_t")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	uint64_t vmin = (*vals)[0];
	uint64_t vmax = (*vals)[0];
	for (uint32_t i = 1; i < vals->size(); ++ i) {
	    if ((*vals)[i] > vmax)
		vmax = (*vals)[i];
	    if ((*vals)[i] < vmin)
		vmin = (*vals)[i];
	}
	if (vmax-vmin < static_cast<uint64_t>(vals->size()))
	    ierr = adaptiveInts(*vals, vmin, vmax, nbins, bounds, counts);
	else
	    ierr = adaptiveFloats(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::FLOAT: {
	array_t<float> *vals = col->selectFloats(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "floats" : "float")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "floats" : "float")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	float vmin = (*vals)[0];
	float vmax = (*vals)[0];
	for (uint32_t i = 1; i < vals->size(); ++ i) {
	    if ((*vals)[i] > vmax)
		vmax = (*vals)[i];
	    if ((*vals)[i] < vmin)
		vmin = (*vals)[i];
	}
	ierr = adaptiveFloats(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    case ibis::DOUBLE: {
	array_t<double> *vals = col->selectDoubles(mask);
	if (vals == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "doubles" : "double")
		<< ", but got nothing";
	    return -5;
	}
	else if (vals->size() != mask.cnt()) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::part[" << (m_name ? m_name : "")
		<< "]::get1DDistribution expected to retrieve "
		<< mask.cnt() << (mask.cnt() > 1 ? "doubles" : "double")
		<< ", but got " << vals->size() << " instead";
	    delete vals;
	    return -6;
	}

	double vmin = (*vals)[0];
	double vmax = (*vals)[0];
	for (uint32_t i = 1; i < vals->size(); ++ i) {
	    if ((*vals)[i] > vmax)
		vmax = (*vals)[i];
	    if ((*vals)[i] < vmin)
		vmin = (*vals)[i];
	}
	ierr = adaptiveFloats(*vals, vmin, vmax, nbins, bounds, counts);
	delete vals;
	break;}
    default: {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::part[" << (m_name ? m_name : "")
	    << "]::get1DDistribution does not currently support column type "
	    << ibis::TYPESTRING[(int) col->type()];
	return -7;}
    } // switch (col->type())
    if (ibis::gVerbose > 0) {
	timer.stop();
	ibis::util::logger lg(1);
	lg.buffer() << "ibis::part[" << (m_name ? m_name : "")
		    << "]::get1DDistribution computed histogram of column "
		    << cname;
	if (constraints != 0 && *constraints != 0)
	    lg.buffer() << " subject to " << constraints;
	lg.buffer() << " in " << timer.CPUTime() << " sec(CPU), "
		    << timer.realTime() << " sec(elapsed)";
    }

    return ierr;
} // ibis::part::get1DDistribution

/// The adaptive binning function for integer values.  It is intended for
/// values within a relatively narrow range.  The input arguments vmin and
/// vmax must be the correct minimum and maximum values -- it uses the
/// minimum and maximum valuse to decided whether an exact histogram can be
/// used internally; incorrect values for vmin or vmax may cuase this
/// function to misbehave!
///
/// It counts the frequency of each distinct value before deciding how to
/// produce the equal-weight bins for output.  Because it has the most
/// detailed information possible, the output bins are mostly to be about
/// equal.  This comes with a cost of a detailed frequency count, which
/// takes time and memory space to compute.
///
/// @note The output number of bins may not be the input value nbins
/// because of following reasons.
/// - If nbins is 0 or 1, it is set to 1000 in this function.
/// - If nbins is larger than 2/3rds of the number of distinct values as
/// indicated by vmin and vmax, each value will have its own bin.
/// - In other cases, this function calls the function
/// ibis::index::divideCounts to determine how to coalesce different fine
/// bins into nbins bins on output.  However, it is possible that the
/// function ibis::index::divideCounts may have trouble produce exactly
/// nbins as requested.
template <typename T> long
ibis::part::adaptiveInts(const array_t<T> &vals, const T vmin, const T vmax,
			 uint32_t nbins, std::vector<double> &bounds,
			 std::vector<uint32_t> &counts) {
    if (vals.size() == 0) {
	return 0L;
    }
    if (vmin >= vmax) { // same min and max
	bounds.resize(2);
	counts.resize(1);
	bounds[0] = vmin;
	bounds[1] = vmin+1;
	counts[0] = vals.size();
	return 1L;
    }

    size_t nfine = static_cast<size_t>(1 + (vmax-vmin));
    LOGGER(ibis::gVerbose > 4)
	<< "ibis::part::adaptiveInts<" << typeid(T).name() << "> counting "
	<< nfine << " distinct values to compute " << nbins
	<< " adaptively binned histogram in the range of [" << vmin
	<< ", " << vmax << "]";

    array_t<uint32_t> fcnts(nfine, 0U);
    for (uint32_t i = 0; i < vals.size(); ++ i)
	++ fcnts[vals[i]-vmin];

    if (nbins <= 1) // too few bins, use 1000
	nbins = 1000;
    if (nbins > (nfine+nfine)/3) {
	bounds.resize(nfine+1);
	counts.resize(nfine);
	nbins = nfine;
	for (uint32_t i = 0; i < nfine; ++ i) {
	    bounds[i] = static_cast<double>(vmin + i);
	    counts[i] = fcnts[i];
	}
	bounds[nfine] = static_cast<double>(vmax+1);
    }
    else {
	array_t<uint32_t> fbnds(nbins);
	ibis::index::divideCounts(fbnds, fcnts);
	nbins = fbnds.size();
	bounds.resize(nbins+1);
	counts.resize(nbins);
	if (fcnts[0] > 0) {
	    bounds[0] = static_cast<double>(vmin);
	}
	else {
	    bool nonzero = false;
	    for (uint32_t i = 0; i < fbnds[0]; ++ i) {
		if (fcnts[i] != 0) {
		    nonzero = true;
		    bounds[0] = static_cast<double>(vmin+i);
		}
	    }
	    if (! nonzero) // impossible
		bounds[0] = static_cast<double>(vmin);
	}
	bounds[1] = static_cast<double>(vmin+fbnds[0]);
	counts[0] = 0;
	for (uint32_t i = 0; i < fbnds[0]; ++ i)
	    counts[0] += fcnts[i];
	for (uint32_t j = 1; j < nbins; ++ j) {
	    bounds[j+1] = static_cast<double>(vmin+fbnds[j]);
	    counts[j] = 0;
	    for (uint32_t i = fbnds[j-1]; i < fbnds[j]; ++ i)
		counts[j] += fcnts[i];
	}
    }
    return nbins;
} // ibis::part::adaptiveInts

/// The adaptive binning function for floats and integers in wide ranges.
/// This function first constructs a number of fine uniform bins and then
/// merge the fine bins to generate nearly equal-weight bins.  This is
/// likely to produce final bins that are not as equal in their weights as
/// those produced from ibis::part::adaptiveInts, but because it usually
/// does less work and takes less time.
////
/// The number of fine bins used is the larger one of 8 times the number of
/// desired bins and the geometric mean of the number of desired bins and
/// the number of records in vals.
///
/// @note This function still relies on the caller to compute vmin and
/// vmax, but it assumes there are many distinct values in each bin.
///
/// @note The output number of bins may not be the input value nbins for
/// the following reasons.
/// - If nbins is 0 or 1, it is reset to 1000 in this function;
/// - if nbins is greater than 1/4 of vals.size(), it is set ot
///   vals.size()/4;
/// - in all other cases, the final number of bins is determine by the
///   function that partitions the fine bins into coarse bins,
///   ibis::index::divideCounts.  This partition process may not produce
///   exactly nbins bins.
///
/// @sa ibis::part::adaptiveInts.
template <typename T> long
ibis::part::adaptiveFloats(const array_t<T> &vals, const T vmin,
			   const T vmax, uint32_t nbins,
			   std::vector<double> &bounds,
			   std::vector<uint32_t> &counts) {
    if (vals.size() == 0) {
	return 0L;
    }
    if (vmax == vmin) {
	bounds.resize(2);
	counts.resize(1);
	bounds[0] = vmin;
	bounds[1] = ibis::util::incrDouble(vmin);
	counts[0] = vals.size();
	return 1L;
    }

    if (nbins <= 1)
	nbins = 1000;
    else if (nbins > 2048 && nbins > (vals.size() >> 2))
	nbins = (vals.size() >> 2);
    const uint32_t nfine = (vals.size()>8*nbins) ? static_cast<uint32_t>
	(std::sqrt(static_cast<double>(vals.size()) * nbins)) : 8*nbins;
    // try to make sure the 2nd bin boundary do not round down to a value
    // that is actually included in the 1st bin
    double scale = 1.0 /
	(ibis::util::incrDouble((double)vmin + (double)(vmax - vmin) /
				nfine) - vmin);
    LOGGER(ibis::gVerbose > 4)
	<< "ibis::part::adaptiveFloats<" << typeid(T).name() << "> using "
	<< nfine << " fine bins to compute " << nbins
	<< " adaptively binned histogram in the range of [" << vmin
	<< ", " << vmax << "] with fine bin size " << 1.0/scale;

    array_t<uint32_t> fcnts(nfine, 0);
    for (uint32_t i = 0; i < vals.size(); ++ i)
	++ fcnts[static_cast<uint32_t>((vals[i]-vmin)*scale)];

    array_t<uint32_t> fbnds(nbins);
    ibis::index::divideCounts(fbnds, fcnts);
    nbins = fbnds.size();
    bounds.resize(nbins+1);
    counts.resize(nbins);
    bounds[0] = vmin;
    bounds[1] = vmin + 1.0 / static_cast<double>(scale);
    counts[0] = 0;
    for (uint32_t i = 0; i < fbnds[0]; ++ i)
	counts[0] += fcnts[i];
    for (uint32_t j = 1; j < nbins; ++ j) {
	bounds[j+1] = vmin + static_cast<double>(j+1) / scale;
	counts[j] = 0;
	for (uint32_t i = fbnds[j-1]; i < fbnds[j]; ++ i)
	    counts[j] += fcnts[i];
    }
    return nbins;
} // ibis::part::adaptiveFloats

/// Adaptive binning through regularly spaced bins.  It goes through the
/// arrays twice, once to compute the actual minimum and maximum values and
/// once to count the entries in each bins.  It produces three sets of
/// bins: the 1-D bins for vals1 and vals2, and a 2-D bin at a high
/// resolution.  It then combine the 1-D bins to form nearly equal-weight
/// bins and use that grouping to decide how to combine the 2-D bins to
/// form the final output.
///
/// @note The number of fine bins used internally is dynamically determined
/// based on the number of desired bins on output, nb1 and nb2, as well as
/// the number of records in vals1 and vals2.
///
/// @note The output number of bins may not be exactly nb1*nb2.  Here are
/// some of the reasons.
/// - If either nb1 or nb2 is less than or equal to one, it is set 100.
/// - If either nb1 or nb2 is larger than 2048, it may be reset to a
///   smaller value so that the number of records in each bin might be about
///   cubic root of the total number of records on input.
/// - It may be necessary to use a few more or less bins (along each
///   dimension) to avoid grouping very popular values with very unpopular
///   values into the same bin.
template <typename T1, typename T2> long
ibis::part::adaptive2DBins(const array_t<T1> &vals1,
			   const array_t<T2> &vals2,
			   uint32_t nb1, uint32_t nb2,
			   std::vector<double> &bounds1,
			   std::vector<double> &bounds2,
			   std::vector<uint32_t> &counts) {
    const uint32_t nrows = (vals1.size() <= vals2.size() ?
			    vals1.size() : vals2.size());
    if (nrows == 0) {
	bounds1.clear();
	bounds2.clear();
	counts.clear();
	return 0L;
    }

    T1 vmin1, vmax1;
    T2 vmin2, vmax2;
    vmin1 = vals1[0];
    vmax1 = vals1[0];
    vmin2 = vals2[0];
    vmax2 = vals2[0];
    for (uint32_t i = 1; i < nrows; ++ i) {
	if (vmin1 > vals1[i])
	    vmin1 = vals1[i];
	if (vmax1 < vals1[i])
	    vmax1 = vals1[i];
	if (vmin2 > vals2[i])
	    vmin2 = vals2[i];
	if (vmax2 < vals2[i])
	    vmax2 = vals2[i];
    }
    if (vmin1 >= vmax1) { // vals1 has only one single value
	bounds1.resize(2);
	bounds1[0] = vmin1;
	bounds1[1] = ibis::util::incrDouble(static_cast<double>(vmin1));
	if (vmin2 >= vmax2) { // vals2 has only one single value as well
	    bounds2.resize(2);
	    bounds2[0] = vmin2;
	    bounds2[1] = ibis::util::incrDouble(static_cast<double>(vmin2));
	    counts.resize(1);
	    counts[0] = nrows;
	}
	else { // one-dimensional adaptive binning
	    adaptiveFloats(vals2, vmin2, vmax2, nb2, bounds2, counts);
	}
	return counts.size();
    }
    else if (vmin2 >= vmax2) { // vals2 has one one single value, bin vals2
	bounds2.resize(2);
	bounds2[0] = vmin2;
	bounds2[1] = ibis::util::incrDouble(static_cast<double>(vmin2));
	return adaptiveFloats(vals1, vmin1, vmax1, nb1, bounds1, counts);
    }

    // normal case, both vals1 and vals2 have multiple distinct values
    // ==> nrows > 1
    std::string mesg;
    {
	std::ostringstream oss;
	oss << "ibis::part::adaptive2DBins<" << typeid(T1).name() << ", "
	    << typeid(T2).name() << ">";
	mesg = oss.str();
    }
    ibis::util::timer atimer(mesg.c_str(), 3);
    if (nb1 <= 1) nb1 = 100;
    if (nb2 <= 1) nb2 = 100;
    double tmp = std::exp(std::log((double)nrows)/3.0);
    if (nb1 > 2048 && (double)nb1 > tmp) {
	if (nrows > 10000000)
	    nb1 = static_cast<uint32_t>(0.5 + tmp);
	else
	    nb1 = 2048;
    }
    if (nb2 > 2048 && (double)nb2 > tmp) {
	if (nrows > 10000000)
	    nb2 = static_cast<uint32_t>(0.5 + tmp);
	else
	    nb2 = 2048;
    }
    tmp = std::exp(std::log((double)nrows/(double)(nb1*nb2))/3.0);
    if (tmp < 2.0) tmp = 2.0;
    const uint32_t nfine1 = static_cast<uint32_t>(0.5 + tmp * nb1);
    const uint32_t nfine2 = static_cast<uint32_t>(0.5 + tmp * nb2);
    const double scale1 = 1.0 /
	(ibis::util::incrDouble((double)vmin1 + (double)(vmax1 - vmin1) /
				nfine1) - vmin1);
    const double scale2 = 1.0 /
	(ibis::util::incrDouble((double)vmin2 + (double)(vmax2 - vmin2) /
				nfine2) - vmin2);
    LOGGER(ibis::gVerbose > 3)
	<< mesg << " internally uses " << nfine1 << " x " << nfine2
	<< " uniform bins for " << nrows << " records in the range of ["
	<< vmin1 << ", " << vmax1 << "] x [" << vmin2 << ", " << vmax2 << "]";

    array_t<uint32_t> cnts1(nfine1,0), cnts2(nfine2,0), cntsa(nfine1*nfine2,0);
    // loop to count values in fine bins
    for (uint32_t i = 0; i < nrows; ++ i) {
	const uint32_t j1 = static_cast<uint32_t>((vals1[i]-vmin1)*scale1);
	const uint32_t j2 = static_cast<uint32_t>((vals2[i]-vmin2)*scale2);
	++ cnts1[j1];
	++ cnts2[j2];
	++ cntsa[j1*nfine2+j2];
    }
    // divide the fine bins into final bins
    array_t<uint32_t> bnds1(nb1), bnds2(nb2);
    ibis::index::divideCounts(bnds1, cnts1);
    ibis::index::divideCounts(bnds2, cnts2);
    nb1 = bnds1.size(); // the final size
    nb2 = bnds2.size();
    LOGGER(ibis::gVerbose > 4)
	<< mesg << " is to use " << nb1 << " x " << nb2
	<< " adaptive bins for a 2D histogram";

    bounds1.resize(nb1+1);
    bounds1[0] = vmin1;
    for (uint32_t i = 0; i < nb1; ++ i)
	bounds1[i+1] = vmin1 + bnds1[i] / scale1;

    bounds2.resize(nb2+1);
    bounds2[0] = vmin2;
    for (uint32_t i = 0; i < nb2; ++ i)
	bounds2[i+1] = vmin2 + bnds2[i] / scale2;

    counts.resize(nb1*nb2);
    counts[0] = 0;
    for (uint32_t i1 = 0; i1 < bnds1[0]; ++ i1) {
	uint32_t off1 = i1 * nfine2;
	for (uint32_t i2 = off1; i2 < off1+bnds2[0]; ++ i2) {
	    counts[0] += cntsa[i2];
	}
    }
    for (uint32_t j2 = 1; j2 < nb2; ++ j2) {
	counts[j2] = 0;
	for (uint32_t i1 = 0; i1 < bnds1[0]; ++ i1) {
	    uint32_t off1 = i1 * nfine2;
	    for (uint32_t i2 = off1+bnds2[j2-1]; i2 < off1+bnds2[j2]; ++ i2)
		counts[j2] += cntsa[i2];
	}
    }
    for (uint32_t j1 = 1; j1 < nb1; ++ j1) {
	uint32_t joff = j1 * nb2;
	counts[joff] = 0;
	for (uint32_t i1 = bnds1[j1-1]; i1 < bnds1[j1]; ++ i1) {
	    uint32_t ioff = i1 * nfine2;
	    for (uint32_t i2 = ioff; i2 < ioff+bnds2[0]; ++ i2)
		counts[joff] += cntsa[i2];
	}

	for (uint32_t j2 = 1; j2 < nb2; ++ j2) {
	    ++ joff;
	    counts[joff] = 0;
	    for (uint32_t i1 = bnds1[j1-1]; i1 < bnds1[j1]; ++ i1) {
		uint32_t ioff = i1 * nfine2;
		for (uint32_t i2 = ioff+bnds2[j2-1]; i2 < ioff+bnds2[j2]; ++ i2)
		    counts[joff] += cntsa[i2];
	    }
	}
    }

    return counts.size();
} // ibis::part::adaptive2DBins

/// Adaptive binning through regularly spaced bins.
///
/// @note Here are the special cases that are different from
/// ibis::part::adaptive2DBins.
/// - If the number of desired bins along any of the three dimensions,
///   nb1, nb2, or nb3, is zero (0) or one (1), it is set to 32.  If all
///   three dimensions are using 32 bins, there is a total of 32,768 bins
///   altogether.
/// - If the number of desired bins along any of the three dimensions, nb1,
///   nb2, or nb3, is greater than 128, it may be reduced to about
///   fourth root of the number of records in input.
///
/// @sa ibis::part::adaptive2DBins
template <typename T1, typename T2, typename T3> long
ibis::part::adaptive3DBins(const array_t<T1> &vals1,
			   const array_t<T2> &vals2,
			   const array_t<T3> &vals3,
			   uint32_t nb1, uint32_t nb2, uint32_t nb3,
			   std::vector<double> &bounds1,
			   std::vector<double> &bounds2,
			   std::vector<double> &bounds3,
			   std::vector<uint32_t> &counts) {
    const uint32_t nrows = (vals1.size() <= vals2.size() ?
			    (vals1.size() <= vals3.size() ?
			     vals1.size() : vals3.size()) :
			    (vals2.size() <= vals3.size() ?
			     vals2.size() : vals3.size()));
    bounds1.clear();
    bounds2.clear();
    bounds3.clear();
    counts.clear();
    if (nrows == 0)
	return 0L;

    T1 vmin1, vmax1;
    T2 vmin2, vmax2;
    T3 vmin3, vmax3;
    vmin1 = vals1[0];
    vmax1 = vals1[0];
    vmin2 = vals2[0];
    vmax2 = vals2[0];
    vmin3 = vals3[0];
    vmax3 = vals3[0];
    for (uint32_t i = 1; i < nrows; ++ i) {
	if (vmin1 > vals1[i])
	    vmin1 = vals1[i];
	if (vmax1 < vals1[i])
	    vmax1 = vals1[i];
	if (vmin2 > vals2[i])
	    vmin2 = vals2[i];
	if (vmax2 < vals2[i])
	    vmax2 = vals2[i];
	if (vmin3 > vals3[i])
	    vmin3 = vals3[i];
	if (vmax3 < vals3[i])
	    vmax3 = vals3[i];
    }
    // degenerate cases where one of the three dimensions has only one
    // single distinct value --- DO NOT use these special cases to compute
    // lower dimensional histograms because of the extra computations of
    // minimum and maximum values.
    if (vmin1 >= vmax1) { // vals1 has only one single value
	bounds1.resize(2);
	bounds1[0] = vmin1;
	bounds1[1] = ibis::util::incrDouble(static_cast<double>(vmin1));
	if (vmin2 >= vmax2) { // vals2 has only one single value as well
	    bounds2.resize(2);
	    bounds2[0] = vmin2;
	    bounds2[1] = ibis::util::incrDouble(static_cast<double>(vmin2));
	    if (vmin3 >= vmax3) { // vals3 has only one single value
		bounds3[0] = vmin3;
		bounds3[1] = ibis::util::incrDouble(static_cast<double>(vmin3));
		counts.resize(1);
		counts[0] = nrows;
	    }
	    else { // one-dimensional adaptive binning
		adaptiveFloats(vals3, vmin3, vmax3, nb2, bounds3, counts);
	    }
	}
	else { // one-dimensional adaptive binning
	    if (vmin3 >= vmax3) {
		bounds3.resize(2);
		bounds3[0] = vmin3;
		bounds3[1] = ibis::util::incrDouble(static_cast<double>(vmin3));
		adaptiveFloats(vals2, vmin2, vmax2, nb2, bounds2, counts);
	    }
	    else {
		adaptive2DBins(vals2, vals3, nb2, nb3,
			       bounds2, bounds3, counts);
	    }
	}
	return counts.size();
    }
    else if (vmin2 >= vmax2) { // vals2 has one one single value, bin vals2
	bounds2.resize(2);
	bounds2[0] = vmin2;
	bounds2[1] = ibis::util::incrDouble(static_cast<double>(vmin2));
	if (vmin3 >= vmax3) { // vals3 has only one single value
	    bounds3.resize(2);
	    bounds3[0] = vmin3;
	    bounds3[1] = ibis::util::incrDouble(static_cast<double>(vmin3));
	    adaptiveFloats(vals1, vmin1, vmax1, nb1, bounds1, counts);
	}
	else {
	    adaptive2DBins(vals1, vals3, nb1, nb3, bounds1, bounds3, counts);
	}
	return counts.size();
    }
    else if (vmin3 >= vmax3) { // vals3 has only one distinct value
	bounds3.resize(2);
	bounds3[0] = vmin3;
	bounds3[1] = ibis::util::incrDouble(static_cast<double>(vmin3));
	return adaptive2DBins(vals1, vals2, nb1, nb2, bounds1, bounds2, counts);
    }

    // normal case,  vals1, vals2, and vals3 have multiple distinct values
    // ==> nrows > 1
    std::string mesg;
    {
	std::ostringstream oss;
	oss << "ibis::part::adaptive3DBins<" << typeid(T1).name() << ", "
	    << typeid(T2).name() << ", " << typeid(T3).name() << ">";
	mesg = oss.str();
    }
    ibis::util::timer atimer(mesg.c_str(), 3);
    if (nb1 <= 1) nb1 = 32;
    if (nb2 <= 1) nb2 = 32;
    if (nb3 <= 1) nb2 = 32;
    double tmp = std::exp(std::log((double)nrows)*0.25);
    if (nb1 > 128 && nb1 > (uint32_t)tmp) {
	if (nrows > 10000000)
	    nb1 = static_cast<uint32_t>(0.5 + tmp);
	else
	    nb1 = 128;
    }
    if (nb2 > 128 && nb2 > (uint32_t)tmp) {
	if (nrows > 10000000)
	    nb2 = static_cast<uint32_t>(0.5 + tmp);
	else
	    nb2 = 128;
    }
    if (nb3 > 128 && nb3 > (uint32_t)tmp) {
	if (nrows > 10000000)
	    nb3 = static_cast<uint32_t>(0.5 + tmp);
	else
	    nb3 = 128;
    }
    tmp = std::exp(std::log((double)nrows/((double)nb1*nb2*nb3))*0.25);
    if (tmp < 2.0) tmp = 2.0;
    const uint32_t nfine1 = static_cast<uint32_t>(0.5 + tmp * nb1);
    const uint32_t nfine2 = static_cast<uint32_t>(0.5 + tmp * nb2);
    const uint32_t nfine3 = static_cast<uint32_t>(0.5 + tmp * nb3);
    // try to make sure the 2nd bin boundary do not round down to a value
    // that is actually included in the 1st bin
    const double scale1 = 1.0 /
	(ibis::util::incrDouble((double)vmin1 + (double)(vmax1 - vmin1) /
				nfine1) - vmin1);
    const double scale2 = 1.0 /
	(ibis::util::incrDouble((double)vmin2 + (double)(vmax2 - vmin2) /
				nfine2) - vmin2);
    const double scale3 = 1.0 /
	(ibis::util::incrDouble((double)vmin3 + (double)(vmax3 - vmin3) /
				nfine3) - vmin3);
    LOGGER(ibis::gVerbose > 3)
	<< mesg << " internally uses "<< nfine1 << " x " << nfine2 << " x " 
	<< nfine3 << " uniform bins for " << nrows
	<< " records in the range of [" << vmin1 << ", " << vmax1
	<< "] x [" << vmin2 << ", " << vmax2 << "]"
	<< "] x [" << vmin3 << ", " << vmax3 << "]";

    array_t<uint32_t> cnts1(nfine1,0), cnts2(nfine2,0), cnts3(nfine2,0),
	cntsa(nfine1*nfine2*nfine3,0);
    // loop to count values in fine bins
    for (uint32_t i = 0; i < nrows; ++ i) {
	const uint32_t j1 = static_cast<uint32_t>((vals1[i]-vmin1)*scale1);
	const uint32_t j2 = static_cast<uint32_t>((vals2[i]-vmin2)*scale2);
	const uint32_t j3 = static_cast<uint32_t>((vals3[i]-vmin3)*scale3);
	++ cnts1[j1];
	++ cnts2[j2];
	++ cnts3[j3];
	++ cntsa[(j1*nfine2+j2)*nfine3+j3];
    }
    // divide the fine bins into final bins
    array_t<uint32_t> bnds1(nb1), bnds2(nb2), bnds3(nb3);
    ibis::index::divideCounts(bnds1, cnts1);
    ibis::index::divideCounts(bnds2, cnts2);
    ibis::index::divideCounts(bnds3, cnts3);
    nb1 = bnds1.size(); // the final size
    nb2 = bnds2.size();
    nb3 = bnds3.size();
    LOGGER(ibis::gVerbose > 4)
	<< mesg << " is to use " << nb1 << " x " << nb2 << " x "
	<< nb3 << " advative bins for a 3D histogram";

    // insert the value 0 as the first element of bnds[123]
    bnds1.resize(nb1+1);
    bounds1.resize(nb1+1);
    for (uint32_t i = nb1; i > 0; -- i) {
	bnds1[i] = bnds1[i-1];
	bounds1[i] = vmin1 + bnds1[i-1] / scale1;
    }
    bnds1[0] = 0;
    bounds1[0] = vmin1;

    bnds2.resize(nb2+1);
    bounds2.resize(nb2+1);
    for (uint32_t i = nb2; i > 0; -- i) {
	bnds2[i] = bnds2[i-1];
	bounds2[i] = vmin2 + bnds2[i-1] / scale2;
    }
    bnds2[0] = 0;
    bounds2[0] = vmin2;

    bnds3.resize(nb3+1);
    bounds3.resize(nb3+1);
    for (uint32_t i = nb3; i > 0; -- i) {
	bnds3[i] = bnds3[i-1];
	bounds3[i] = vmin3 + bnds3[i-1] / scale3;
    }
    bnds3[0] = 0;
    bounds3[0] = vmin3;
#if defined(_DEBUG) || defined(DEBUG)
    if (ibis::gVerbose >= 5) {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- " << mesg
		    << " scale1 = " << std::setprecision(18) << scale1
		    << ", scale2 = " << std::setprecision(18) << scale2
		    << ", scale3 = " << std::setprecision(18) << scale3
		    << "\n  bounds1[" << bounds1.size()
		    << "]: " << bounds1[0];
	for (uint32_t i = 1; i < bounds1.size(); ++ i)
	    lg.buffer() << ", " << bounds1[i];
	lg.buffer() << "\n  bounds2[" << bounds2.size()
		    << "]: " << bounds2[0];
	for (uint32_t i = 1; i < bounds2.size(); ++ i)
	    lg.buffer() << ", " << bounds2[i];
	lg.buffer() << "\n  bounds3[" << bounds3.size()
		    << "]: " << bounds3[0];
	for (uint32_t i = 1; i < bounds3.size(); ++ i)
	    lg.buffer() << ", " << bounds3[i];
	lg.buffer() << "\n  bnds1[" << bnds1.size()
		    << "]: " << bnds1[0];
	for (uint32_t i = 1; i < bnds1.size(); ++ i)
	    lg.buffer() << ", " << bnds1[i];
	lg.buffer() << "\n  bnds2[" << bnds2.size()
		    << "]: " << bnds2[0];
	for (uint32_t i = 1; i < bnds2.size(); ++ i)
	    lg.buffer() << ", " << bnds2[i];
	lg.buffer() << "\n  bnds3[" << bnds3.size()
		    << "]: " << bnds3[0];
	for (uint32_t i = 1; i < bnds3.size(); ++ i)
	    lg.buffer() << ", " << bnds3[i];
    }
#endif

    counts.resize(nb1*nb2*nb3);
    for (uint32_t j1 = 0; j1 < nb1; ++ j1) { // j1
	const uint32_t joff1 = j1 * nb2;
	for (uint32_t j2 = 0; j2 < nb2; ++ j2) { // j2
	    const uint32_t joff2 = (joff1 + j2) * nb3;
	    for (uint32_t j3 = 0; j3 < nb3; ++ j3) { // j3
		uint32_t &tmp = (counts[joff2+j3]);
		tmp = 0;
		for (uint32_t i1 = bnds1[j1]; i1 < bnds1[j1+1]; ++ i1) {
		    const uint32_t ioff1 = i1 * nfine2;
		    for (uint32_t i2 = ioff1 + bnds2[j2];
			 i2 < ioff1 + bnds2[j2+1]; ++ i2) {
			const uint32_t ioff2 = i2 * nfine3;
			for (uint32_t i3 = ioff2 + bnds3[j3];
			     i3 < ioff2 + bnds3[j3+1]; ++ i3)
			    tmp += cntsa[i3];
		    } // i2
		} // i1
	    } // j3
	} // j2
    } // j1

    return counts.size();
} // ibis::part::adaptive3DBins

/// Bins the given values so that each each bin is nearly equal weight.
/// Instead of counting the number entries in each bin return bitvectors
/// that mark the positions of the records.  This version is for integer
/// values in relatively narrow ranges.  It will count each distinct value
/// separately, which gives the most accurate information for deciding how
/// to produce equal-weight bins.  If there are many dictinct values, this
/// function will require considerable amount of internal memory to count
/// each distinct value.
///
/// On successful completion of this function, the return value is the
/// number of bins used.  If the input array is empty, it returns 0 without
/// modifying the content of the output arrays, bounds and detail.  Either
/// mask and vals have the same number of elements, or vals has as many
/// elements as the number of ones (1) in mask, otherwise this function
/// will return -51.
///
/// @sa ibis::part::adaptiveInts
template <typename T> long
ibis::part::adaptiveIntsDetailed(const ibis::bitvector &mask,
				 const array_t<T> &vals,
				 const T vmin, const T vmax, uint32_t nbins,
				 std::vector<double> &bounds,
				 std::vector<ibis::bitvector> &detail) {
    if (mask.size() != vals.size() && mask.cnt() != vals.size())
	return -51L;
    if (vals.size() == 0) {
	return 0L;
    }
    if (vmin >= vmax) { // same min and max
	bounds.resize(2);
	detail.resize(1);
	bounds[0] = vmin;
	bounds[1] = vmin+1;
	detail[0].copy(mask);
	return 1L;
    }

    size_t nfine = static_cast<size_t>(1 + (vmax-vmin));
    LOGGER(ibis::gVerbose > 4)
	<< "ibis::part::adaptiveIntsDetailed<" << typeid(T).name()
	<< "> counting " << nfine << " distinct values to compute " << nbins
	<< " adaptively binned histogram in the range of [" << vmin
	<< ", " << vmax << "]";

    array_t<uint32_t> fcnts(nfine, 0U);
    std::vector<ibis::bitvector*> pos(nfine);
    for (size_t i = 0; i < nfine; ++ i)
	pos[i] = new ibis::bitvector;
    if (mask.cnt() == vals.size()) {
	uint32_t j = 0; // index into array vals
	for (ibis::bitvector::indexSet is = mask.firstIndexSet();
	     is.nIndices() > 0; ++ is) {
	    const uint32_t nind = is.nIndices();
	    const ibis::bitvector::word_t *idx = is.indices();
	    if (is.isRange()) {
		for (uint32_t i = *idx; i < idx[1]; ++ i) {
		    const T ifine = vals[j] - vmin;
		    ++ j;
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(i, 1);
		}
	    }
	    else {
		for (uint32_t i = 0; i < nind; ++ i) {
		    const T ifine = vals[j] - vmin;
		    ++ j;
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(idx[i], 1);
		}
	    }
	}
    }
    else {
	for (ibis::bitvector::indexSet is = mask.firstIndexSet();
	     is.nIndices() > 0; ++ is) {
	    const uint32_t nind = is.nIndices();
	    const ibis::bitvector::word_t *idx = is.indices();
	    if (is.isRange()) {
		for (uint32_t i = *idx; i < idx[1]; ++ i) {
		    const T ifine = vals[i] - vmin;
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(i, 1);
		}
	    }
	    else {
		for (uint32_t i = 0; i < nind; ++ i) {
		    const ibis::bitvector::word_t j = idx[i];
		    const T ifine = vals[j] - vmin;
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(j, 1);
		}
	    }
	}
    }
    for (size_t i = 0; i < nfine; ++ i)
	pos[i]->adjustSize(0, mask.size());

    if (nbins <= 1) // too few bins, use 1000
	nbins = 1000;
    if (nbins > (nfine+nfine)/3) {
	bounds.resize(nfine+1);
	detail.resize(nfine);
	nbins = nfine;
	for (uint32_t i = 0; i < nfine; ++ i) {
	    bounds[i] = static_cast<double>(vmin + i);
	    detail[i].swap(*pos[i]);
	}
	bounds[nfine] = static_cast<double>(vmax+1);
    }
    else {
	array_t<uint32_t> fbnds(nbins);
	ibis::index::divideCounts(fbnds, fcnts);
	nbins = fbnds.size();
	bounds.resize(nbins+1);
	detail.resize(nbins);
	if (fcnts[0] > 0) {
	    bounds[0] = static_cast<double>(vmin);
	}
	else {
	    bool nonzero = false;
	    for (uint32_t i = 0; i < fbnds[0]; ++ i) {
		if (fcnts[i] != 0) {
		    nonzero = true;
		    bounds[0] = static_cast<double>(vmin+i);
		}
	    }
	    if (! nonzero) // should never be true
		bounds[0] = static_cast<double>(vmin);
	}
	bounds[1] = static_cast<double>(vmin+fbnds[0]);
	if (fbnds[0] > 1)
	    ibis::index::sumBits(pos, 0, fbnds[0], detail[0]);
	else
	    detail[0].swap(*pos[0]);
	for (uint32_t j = 1; j < nbins; ++ j) {
	    bounds[j+1] = static_cast<double>(vmin+fbnds[j]);
	    if (fbnds[j] > fbnds[j-1]+1)
		ibis::index::sumBits(pos, fbnds[j-1], fbnds[j], detail[j]);
	    else
		detail[j].swap(*pos[fbnds[j-1]]);
	}
    }

    for (size_t i = 0; i < nfine; ++ i)
	delete pos[i];
    return detail.size();
} // ibis::part::adaptiveIntsDetailed

/// Bins the given values so that each each bin is nearly equal weight.
/// Instead of counting the number entries in each bin return bitvectors
/// that mark the positions of the records.  This version is for
/// floating-point values and integer values with wide ranges.  This
/// function first bins the values into a relatively large number of fine
/// equal-width bins and then coalesce nearby fines bins to for nearly
/// equal-weight bins.  The final bins produced this way are less likely to
/// be very uniform in their weights, but it requires less internal work
/// space and therefore may be faster than
/// ibis::part::adaptiveIntsDetailed.
///
/// @sa ibis::part::adapativeFloats
template <typename T> long
ibis::part::adaptiveFloatsDetailed(const ibis::bitvector &mask,
				   const array_t<T> &vals, const T vmin,
				   const T vmax, uint32_t nbins,
				   std::vector<double> &bounds,
				   std::vector<ibis::bitvector> &detail) {
    if (mask.size() != vals.size() && mask.cnt() != vals.size())
	return -51L;
    if (vals.size() == 0) {
	return 0L;
    }
    if (vmax == vmin) {
	bounds.resize(2);
	detail.resize(1);
	bounds[0] = vmin;
	bounds[1] = ibis::util::incrDouble(vmin);
	detail[0].copy(mask);
	return 1L;
    }

    if (nbins <= 1) // default to 1000 bins
	nbins = 1000;
    else if (nbins > 2048 && nbins > (vals.size() >> 2))
	nbins = (vals.size() >> 2);
    const uint32_t nfine = (vals.size()>8*nbins) ? static_cast<uint32_t>
	(std::sqrt(static_cast<double>(vals.size()) * nbins)) : 8*nbins;
    // try to make sure the 2nd bin boundary do not round down to a value
    // that is actually included in the 1st bin
    double scale = 1.0 /
	(ibis::util::incrDouble((double)vmin + (double)(vmax - vmin) /
				nfine) - vmin);
    LOGGER(ibis::gVerbose > 4)
	<< "ibis::part::adaptiveFloatsDetailed<" << typeid(T).name()
	<< "> using " << nfine << " fine bins to compute " << nbins
	<< " adaptively binned histogram in the range of [" << vmin
	<< ", " << vmax << "] with fine bin size " << 1.0/scale;

    array_t<uint32_t> fcnts(nfine, 0);
    std::vector<ibis::bitvector*> pos(nfine);
    for (size_t i = 0; i < nfine; ++ i)
	pos[i] = new ibis::bitvector;
    if (mask.cnt() == vals.size()) {
	uint32_t j = 0; // index into array vals
	for (ibis::bitvector::indexSet is = mask.firstIndexSet();
	     is.nIndices() > 0; ++ is) {
	    const uint32_t nind = is.nIndices();
	    const ibis::bitvector::word_t *idx = is.indices();
	    if (is.isRange()) {
		for (uint32_t i = *idx; i < idx[1]; ++ i) {
		    const uint32_t ifine =
			static_cast<uint32_t>((vals[j]-vmin)*scale);
		    ++ j;
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(i, 1);
		}
	    }
	    else {
		for (uint32_t i = 0; i < nind; ++ i) {
		    const uint32_t ifine =
			static_cast<uint32_t>((vals[j]-vmin)*scale);
		    ++ j;
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(idx[i], 1);
		}
	    }
	}
    }
    else {
	for (ibis::bitvector::indexSet is = mask.firstIndexSet();
	     is.nIndices() > 0; ++ is) {
	    const uint32_t nind = is.nIndices();
	    const ibis::bitvector::word_t *idx = is.indices();
	    if (is.isRange()) {
		for (uint32_t i = *idx; i < idx[1]; ++ i) {
		    const uint32_t ifine =
			static_cast<uint32_t>((vals[i]-vmin)*scale);
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(i, 1);
		}
	    }
	    else {
		for (uint32_t i = 0; i < nind; ++ i) {
		    const ibis::bitvector::word_t j = idx[i];
		    const uint32_t ifine =
			static_cast<uint32_t>((vals[j]-vmin)*scale);
		    ++ fcnts[ifine];
		    pos[ifine]->setBit(j, 1);
		}
	    }
	}
    }
    for (size_t i = 0; i < nfine; ++ i)
	pos[i]->adjustSize(0, mask.size());

    array_t<uint32_t> fbnds(nbins);
    ibis::index::divideCounts(fbnds, fcnts);
    nbins = fbnds.size();
    bounds.resize(nbins+1);
    detail.resize(nbins);
    bounds[0] = vmin;
    bounds[1] = vmin + 1.0 / scale;
    if (fbnds[0] > 1)
	ibis::index::sumBits(pos, 0, fbnds[0], detail[0]);
    else
	detail[0].swap(*pos[0]);
    for (uint32_t j = 1; j < nbins; ++ j) {
	bounds[j+1] = vmin + static_cast<double>(j+1) / scale;
	if (fbnds[j+1] > fbnds[j]+1)
	    ibis::index::sumBits(pos, fbnds[j-1], fbnds[j], detail[j]);
	else
	    detail[j].swap(*pos[fbnds[j-1]]);
    }

    for (size_t i = 0; i < nfine; ++ i)
	delete pos[i];
    return detail.size();
} // ibis::part::adaptiveFloatsDetailed

/// The user only specify the name of the variables/columns and the number
/// of bins for each variable.  This function is free to decide where to
/// place the bin boundaries to count the bins as fast as possible.  If the
/// indexes are available and are smaller than the raw data files, then the
/// indexes are used to compute the histogram, otherwise, it reads the raw
/// data files into memory and count the number of records in each bin.
///
/// Bin @c i1 in the first dimension is defined as
/// @code bounds1[i1] <= cname1 < bounds1[i1+1] @endcode
/// and bin @c i2 in the second dimension is defined as
/// @code bounds2[i2] <= cname2 < bounds2[i2+1] @endcode.
/// The 2D bins are linearized in @c counts with the second dimension as the
/// faster varying dimension.
///
/// The return value is the number of bins, i.e., the size of array counts.
/// Normally, the number of bins should be @code nb1 * nb2 @endcode.  For
/// example, if the indexes are used, but there are less bins in the indexes
/// than nb1 or nb2, then the number of bins in the indexes will be used.
///
/// The last three arguments bounds1, bounds2, and counts are for output
/// only.  Their input values are ignored.
///
/// The argument option can be either "index", "data" or "uniform".  The
/// option "index" indicates the user prefer to use indexes to compute
/// histograms.  The indexes will be used in this case if they exist
/// already.  If either "data" or "uniform" is specified, it will attempt
/// to use the base data to compute a histogram, with "uniform" indicating
/// a equally spaced (uniform) bins and the other indicating adaptive bins.
/// If the option is none of above choices, this function will choose one
/// based on their relative sizes.
///
/// @note The number of bins are not guaranteed to be the nb1 and nb2.  The
/// adaptive procedure may decide to use a few bins more or less than
/// specified in each dimension.
///
/// @sa get2DDistributionA.
/// @sa get2DDistributionU.
/// @sa get2DDistributionI.
long ibis::part::get2DDistribution(const char *cname1, const char *cname2,
				   uint32_t nb1, uint32_t nb2,
				   std::vector<double> &bounds1,
				   std::vector<double> &bounds2,
				   std::vector<uint32_t> &counts,
				   const char* const option) const {
    if (cname1 == 0 || *cname1 == 0 || cname2 == 0 || *cname2 == 0)
	return -1L;

    const ibis::column* col1 = getColumn(cname1);
    const ibis::column* col2 = getColumn(cname2);
    if (col1 == 0 || col2 == 0)
	return -2L;

    const long idx1 = col1->indexSize();
    const long idx2 = col2->indexSize();
    const int elem1 = col1->elementSize();
    const int elem2 = col2->elementSize();
    if ((elem1 <= 0 && idx1 <= 0) || (elem2 <= 0 && idx2 <= 0))
	// string values must be indexed
	return -3L;

    if (option != 0 && (*option == 'i' || *option == 'I') &&
	idx1 > 0 && idx2 > 0) {
	// use indexes
	return get2DDistributionI(*col1, *col2, nb1, nb2,
				  bounds1, bounds2, counts);
    }
    else if (option != 0 && (*option == 'd' || *option == 'D') &&
	     elem1 > 0 && elem2 > 0) {
	// use base data with adaptive bins
	return get2DDistributionA(*col1, *col2, nb1, nb2,
				  bounds1, bounds2, counts);
    }
    else if (option != 0 && (*option == 'u' || *option == 'U') &&
	     elem1 > 0 && elem2 > 0) {
	// use base data with uniform bins
	return get2DDistributionU(*col1, *col2, nb1, nb2,
				  bounds1, bounds2, counts);
    }
    else if ((elem1 <= 0 || elem2 <= 0) ||
	     (idx1 > 0 && idx2 > 0 && ((double)idx1*nb2+(double)idx2*nb1)*0.1 <
	      static_cast<double>(elem1+elem2)*nEvents)) {
	// use indexes because they exist and are much smaller in sizes
	return get2DDistributionI(*col1, *col2, nb1, nb2,
				  bounds1, bounds2, counts);
    }
    else {
	// use base data with adaptive bins
	return get2DDistributionA(*col1, *col2, nb1, nb2,
				  bounds1, bounds2, counts);
    }
} // ibis::part::get2DDistribution

/// Compute a set of adaptive bins based on a fine level uniform bins.
long ibis::part::get2DDistributionA(const ibis::column &col1,
				    const ibis::column &col2,
				    uint32_t nb1, uint32_t nb2,
				    std::vector<double> &bounds1,
				    std::vector<double> &bounds2,
				    std::vector<uint32_t> &counts) const {
    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionA attempting to compute a " << nb1 << " x "
	    << nb2 << " histogram of " << col1.name() << " and "
	    << col2.name() << " using base data";
	timer.start();
    }

    ibis::bitvector mask;
    col1.getNullMask(mask);
    if (mask.size() == nEvents) {
	ibis::bitvector tmp;
	col2.getNullMask(tmp);
	mask &= tmp;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::part[" << m_name
	    << "]::get2DDistributionA - null mask of " << col1.name()
	    << " has " << mask.size() << " bits, but " << nEvents
	    << " are expected";
	return -5L;
    }
    if (mask.cnt() == 0) {
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << m_name
	    << "]::get2DDistributionA - null mask contains only 0 ";
	bounds1.resize(0);
	bounds2.resize(0);
	counts.resize(0);
	return 0L;
    }

    long ierr;
    switch (col1.type()) {
    case ibis::BYTE:
    case ibis::SHORT:
    case ibis::INT: {
	array_t<int32_t>* vals1 = col1.selectInts(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionA -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::UINT: {
	array_t<uint32_t>* vals1 = col1.selectUInts(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionA -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::ULONG:
    case ibis::LONG: {
	array_t<int64_t>* vals1 = col1.selectLongs(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionA -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals1 = col1.selectFloats(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionA -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals1 = col1.selectDoubles(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = adaptive2DBins(*vals1, *vals2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionA -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::get2DDistributionA -- unable to "
	    "handle column (" << col1.name() << ") type "
	    << ibis::TYPESTRING[(int)col1.type()];

	ierr = -3;
	break;}
    }
    if (ibis::gVerbose > 0) {
	timer.stop();
	ibis::util::logger(0).buffer()
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionA completed filling a " << nb1 << " x "
	    << nb2 << " histogram on " << col1.name() << " and "
	    << col2.name() << " with " << counts.size() << " cell"
	    << (counts.size() > 1 ? "s" : "") << " using " << timer.CPUTime()
	    << " sec (CPU), " << timer.realTime() << " sec (elapsed)";
    }
    return ierr;
} // ibis::part::get2DDistributionA

/// Read the base data, then count how many values fall in each bin.  The
/// binns are defined with regular spacing.
long ibis::part::get2DDistributionU(const ibis::column &col1,
				    const ibis::column &col2,
				    uint32_t nb1, uint32_t nb2,
				    std::vector<double> &bounds1,
				    std::vector<double> &bounds2,
				    std::vector<uint32_t> &counts) const {
    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionU attempting to compute a " << nb1
	    << " x " << nb2 << " histogram of " << col1.name() << " and "
	    << col2.name() << " using base data";
	timer.start();
    }
    uint32_t nbmax = static_cast<uint32_t>(0.5*sqrt((double)nEvents));
    if (nbmax < 1000) nbmax = 1000;
    if (nb1 <= 1) nb1 = 100;
    else if (nb1 > nbmax) nb1 = nbmax;
    if (nb2 <= 1) nb2 = 100;
    else if (nb2 > nbmax) nb2 = nbmax;
    const double begin1 = col1.getActualMin();
    const double begin2 = col2.getActualMin();
    const double end1 = col1.getActualMax();
    const double end2 = col2.getActualMax();
    if (end1 <= begin1) { // a single bin
	bounds1.resize(2);
	bounds1[0] = begin1;
	bounds1[1] = end1;
	if (end2 <= begin2) {
	    bounds2.resize(2);
	    bounds2[0] = begin2;
	    bounds2[1] = end2;
	    counts.resize(1);
	    counts[0] = nEvents;
	    return 1L;
	}
	else { // col1 has 1 distinct value
	    double stride2 = ((col2.isFloat() ? ibis::util::incrDouble(end2) :
			       end2+1) - begin2) / nb2;
	    bounds2.resize(nb2+1);
	    for (uint32_t i = 0; i <= nb2; ++ i)
		bounds2[i] = begin2 + i * stride2;
	    return get1DDistribution(0, col2.name(), begin2, end2, stride2,
				     counts);
	}
    }
    else if (end2 <= begin2) { // col2 has 1 distinct value
	bounds2.resize(2);
	bounds2[0] = begin2;
	bounds2[1] = end2;
	double stride1 = ((col1.isFloat() ? ibis::util::incrDouble(end1) :
			   end1+1) - begin1) / nb1;
	bounds1.resize(nb1+1);
	for (uint32_t i = 0; i < nb1; ++ i)
	    bounds1[i] = begin1 + i * stride1;
	return get1DDistribution(0, col1.name(), begin1, end1, stride1,
				 counts);
    }

    // normal case -- both columns have more than one distinct value
    double stride1;
    double stride2;
    if (col1.isFloat()) {
	stride1 = (end1 - begin1) / nb1;
	stride1 = ibis::util::compactValue(stride1, stride1*(1.0+0.5/nb1));
    }
    else if (end1 > begin1 + nb1*1.25) {
	stride1 = (1.0 + end1 - begin1) / nb1;
    }
    else {
	nb1 = static_cast<uint32_t>(1 + end1 - begin1);
	stride1 = 1.0;
    }
    if (col2.isFloat()) {
	stride2 = (end2 - begin2) / nb2;
	stride2 = ibis::util::compactValue(stride2, stride2*(1.0+0.5/nb2));
    }
    else if (end2 > begin2 + nb2*1.25) {
	stride2 = (1.0 + end2 - begin2) / nb2;
    }
    else {
	nb2 = static_cast<uint32_t>(1.0 + end2 - begin2);
	stride2 = 1.0;
    }
    const size_t nbins =
	(1 + static_cast<uint32_t>(std::floor((end1 - begin1) / stride1))) *
	(1 + static_cast<uint32_t>(std::floor((end2 - begin2) / stride2)));
    if (nbins != nb1 * nb2) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::part[" << m_name
	    << "]::get2DDistributionU - nbins (" << nbins
	    << ") is expected to be the product of nb1 (" << nb1
	    << ") and nb2 (" << nb2 << "), but is actually " << nbins;
	return -4L;
    }

    ibis::bitvector mask;
    col1.getNullMask(mask);
    if (mask.size() == nEvents) {
	ibis::bitvector tmp;
	col2.getNullMask(tmp);
	mask &= tmp;
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::part[" << m_name
	    << "]::get2DDistributionU - null mask of " << col1.name()
	    << " has " << mask.size() << " bits, but " << nEvents
	    << " are expected";
	return -5L;
    }
    if (mask.cnt() == 0) {
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << m_name
	    << "]::get2DDistributionU - null mask contains only 0 ";
	bounds1.resize(0);
	bounds2.resize(0);
	counts.resize(0);
	return 0L;
    }

    long ierr;
    counts.resize(nbins);
    for (size_t i = 0; i < nbins; ++i)
	counts[i] = 0;
    bounds1.resize(nb1+1);
    for (uint32_t i = 0; i <= nb1; ++ i)
	bounds1[i] = begin1 + i * stride1;
    bounds2.resize(nb2+1);
    for (uint32_t i = 0; i <= nb2; ++ i)
	bounds2[i] = begin2 + i * stride2;

    switch (col1.type()) {
    case ibis::BYTE:
    case ibis::SHORT:
    case ibis::INT: {
	array_t<int32_t>* vals1 = col1.selectInts(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionU -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::UBYTE:
    case ibis::USHORT:
    case ibis::UINT: {
	array_t<uint32_t>* vals1 = col1.selectUInts(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionU -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::ULONG:
    case ibis::LONG: {
	array_t<int64_t>* vals1 = col1.selectLongs(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionU -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals1 = col1.selectFloats(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionU -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals1 = col1.selectDoubles(mask);
	if (vals1 == 0) {
	    ierr = -4;
	    break;
	}

	switch (col2.type()) {
	case ibis::BYTE:
	case ibis::SHORT:
	case ibis::INT: {
	    array_t<int32_t>* vals2 = col2.selectInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::UBYTE:
	case ibis::USHORT:
	case ibis::UINT: {
	    array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::ULONG:
	case ibis::LONG: {
	    array_t<int64_t>* vals2 = col2.selectLongs(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::FLOAT: {
	    array_t<float>* vals2 = col2.selectFloats(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double>* vals2 = col2.selectDoubles(mask);
	    if (vals2 == 0) {
		ierr = -5;
		break;
	    }
	    ierr = count2DBins(*vals1, begin1, end1, stride1,
			       *vals2, begin2, end2, stride2, counts);
	    delete vals2;
	    break;}
	default: {
	    LOGGER(ibis::gVerbose >= 4)
		<< "ibis::part::get2DDistributionU -- unable to "
		"handle column (" << col2.name() << ") type "
		<< ibis::TYPESTRING[(int)col2.type()];

	    ierr = -3;
	    break;}
	}
	delete vals1;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::get2DDistributionU -- unable to "
	    "handle column (" << col1.name() << ") type "
	    << ibis::TYPESTRING[(int)col1.type()];

	ierr = -3;
	break;}
    }
    if (ibis::gVerbose > 0) {
	timer.stop();
	ibis::util::logger(0).buffer()
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionU completed filling a " << nb1 << " x "
	    << nb2 << " histogram on " << col1.name() << " and "
	    << col2.name() << " with " << counts.size() << " cell"
	    << (counts.size() > 1 ? "s" : "") << " using " << timer.CPUTime()
	    << " sec (CPU), " << timer.realTime() << " sec (elapsed)";
    }
    return ierr;
} // ibis::part::get2DDistributionU

long ibis::part::get2DDistributionI(const ibis::column &col1,
				    const ibis::column &col2,
				    uint32_t nb1, uint32_t nb2,
				    std::vector<double> &bounds1,
				    std::vector<double> &bounds2,
				    std::vector<uint32_t> &counts) const {
    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI attempting to compute a " << nb1
	    << " x " << nb2 << " histogram of " << col1.name() << " and "
	    << col2.name() << " using indexes";
	timer.start();
    }

    uint32_t nbmax = static_cast<uint32_t>(0.5*sqrt((double)nEvents));
    if (nbmax < 1000) nbmax = 1000;
    if (nb1 <= 1) nb1 = 100;
    else if (nb1 > nbmax) nb1 = nbmax;
    if (nb2 <= 1) nb2 = 100;
    else if (nb2 > nbmax) nb2 = nbmax;
    const double begin1 = col1.getActualMin();
    const double begin2 = col2.getActualMin();
    const double end1 = col1.getActualMax();
    const double end2 = col2.getActualMax();
    if (end1 <= begin1) { // col1 has one distinct value
	bounds1.resize(2);
	bounds1[0] = begin1;
	bounds1[1] = end1;
	if (end2 <= begin2) { // col2 has one distinct value
	    bounds2.resize(2);
	    bounds2[0] = begin2;
	    bounds2[1] = end2;
	    counts.resize(1);
	    counts[0] = nEvents;
	    return 1L;
	}
	else { // col1 has 1 distinct value, but not col2
	    return get1DDistribution(col2, nb2, bounds2, counts);
	}
    }
    else if (end2 <= begin2) { // col2 has 1 distinct value
	bounds2.resize(2);
	bounds2[0] = begin2;
	bounds2[1] = end2;
	return get1DDistribution(col1, nb1, bounds2, counts);
    }

    // normal case -- both columns have more than one distinct value
    ibis::column::indexLock idxlock1(&col1, "get2DDistributionI");
    const ibis::index* idx1 = idxlock1.getIndex();
    if (idx1 == 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI can not proceed with index for "
	    << col1.name();
	return -1L;
    }

    array_t<uint32_t> w1bnds(nb1);
    std::vector<double> idx1bin;
    idx1->binBoundaries(idx1bin);
    while (idx1bin.size() > 1 && idx1bin.back() >= end1)
	idx1bin.pop_back();
    if (idx1bin.empty()) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI can not proceed because column "
	    << col1.name() << " contains no valid values or only one value";
	return -2L;
    }
    else if (idx1bin.size() > nb1*3/2) {
	std::vector<uint32_t> idx1wgt;
	idx1->binWeights(idx1wgt);
	if (idx1bin.size() > idx1wgt.size()) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::get2DDistributionI can not count the number of values "
		"in column " << col1.name();
	    return -3L;
	}

	array_t<uint32_t> wgt2(idx1wgt.size());
	std::copy(idx1wgt.begin(), idx1wgt.end(), wgt2.begin());
	
	ibis::index::divideCounts(w1bnds, wgt2);
	while (w1bnds.size() > 1 && w1bnds[w1bnds.size()-2] >= idx1bin.size())
	    w1bnds.pop_back();
	if (w1bnds.size() < 2) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::get2DDistributionI can not divide " << idx1bin.size()
		<< "bins into " << nb1 << " coarser bins";
	    return -4L;
	}
    }
    else {
	w1bnds.resize(idx1bin.size());
	for (unsigned i = 0; i < idx1bin.size(); ++ i)
	    w1bnds[i] = i+1;
    }

    bounds1.resize(w1bnds.size()+1);
    bounds1[0] = begin1;
    for (unsigned i = 1; i < w1bnds.size(); ++ i)
	bounds1[i] = idx1bin[w1bnds[i-1]];
    bounds1.back() = (col1.isFloat() ? ibis::util::incrDouble(end1) : end1+1);

    std::vector<ibis::bitvector*> bins2;
    try {
	long ierr = coarsenBins(col2, nb2, bounds2, bins2);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::get2DDistributionI can not coarsen bins of "
		<< col2.name() << ", ierr=" << ierr;
	    return -5L;
	}
	else {
	    bounds2.resize(bins2.size()+1);
	    double prev = begin2;
	    for (unsigned i = 0; i < bins2.size(); ++ i) {
		double tmp = bounds2[i];
		bounds2[i] = prev;
		prev = tmp;
	    }
	    bounds2.back() = (col2.isFloat() ? ibis::util::incrDouble(end2) :
			      end2+1);
	}

	counts.resize((bounds1.size()-1) * bins2.size());
	ibis::qContinuousRange rng1(col1.name(), ibis::qExpr::OP_LT,
				    bounds1[1]);
	ibis::bitvector bv;
	LOGGER(ibis::gVerbose >= 5)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI evaluating " << rng1
	    << " for bin 0 in " << col1.name();
	ierr = idx1->evaluate(rng1, bv);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::get2DDistributionI failed to evaluate range condition \""
		<< rng1 << "\", ierr=" << ierr;
	    return -6L;
	}

	if (ierr > 0) {
	    for (unsigned i = 0; i < bins2.size(); ++ i) {
		counts[i] = bv.count(*bins2[i]);
#if defined(DEBUG)
		ibis::bitvector *tmp = bv  &(*bins2[i]);
		if (tmp != 0) {
		    if (tmp->cnt() != counts[i]) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- function ibis::bitvector::count "
			    "did not produce correct answer";
			(void) bv.count(*bins2[i]);
		    }
		    delete tmp;
		}
		else {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- bitwise AND didnot produce a valid "
			"bitvector";
		}
#endif
	    }
	}
	else { // no records
	    for (unsigned i = 0; i < bins2.size(); ++ i)
		counts[i] = 0;
	}

	rng1.leftOperator() = ibis::qExpr::OP_LE;
	rng1.rightOperator() = ibis::qExpr::OP_LT;
	for (unsigned j = 1; j < bounds1.size()-2; ++ j) {
	    size_t jc = j * bins2.size();
	    rng1.leftBound() = bounds1[j];
	    rng1.rightBound() = bounds1[j+1];
	    LOGGER(ibis::gVerbose >= 5)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::get2DDistributionI evaluating " << rng1
		<< " for bin " << j << " in " << col1.name();
	    ierr = idx1->evaluate(rng1, bv);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose >= 3)
		    << "ibis::part[" << (m_name ? m_name : "")
		    << "]::get2DDistributionI failed to evaluate \""
		    << rng1 << "\", ierr=" << ierr;
		return -6L;
	    }
	    if (ierr > 0) {
		for (unsigned i = 0; i < bins2.size(); ++ i) {
		    counts[jc + i] = bv.count(*bins2[i]);
#if defined(DEBUG)
		    ibis::bitvector *tmp = bv  &(*bins2[i]);
		    if (tmp != 0) {
			if (tmp->cnt() != counts[jc+i]) {
			    LOGGER(ibis::gVerbose >= 0)
				<< "Warning -- function ibis::bitvector::count "
				"did not produce correct answer";
			    (void) bv.count(*bins2[i]);
			}
			delete tmp;
		    }
		    else {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- bitwise AND didnot produce an "
			    "invalid bitvector";
		    }
#endif
		}
	    }
	    else {
		for (unsigned i = 0; i < bins2.size(); ++ i)
		    counts[jc + i] = 0;
	    }
	}

	rng1.rightOperator() = ibis::qExpr::OP_UNDEFINED;
	rng1.leftBound() = bounds1[bounds1.size()-2];
	LOGGER(ibis::gVerbose >= 5)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI evaluating " << rng1
	    << " for bin " << bounds1.size()-1 << " in " << col1.name();
	ierr = idx1->evaluate(rng1, bv);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::get2DDistributionI failed to evaluate range condition \""
		<< rng1 << "\", ierr=" << ierr;
	    return -6L;
	}
	if (ierr > 0) {
	    const size_t jc = (bounds1.size()-2) * bins2.size();
	    for (unsigned i = 0; i < bins2.size(); ++ i) {
		counts[jc + i] = bv.count(*bins2[i]);
#if defined(DEBUG)
		ibis::bitvector *tmp = bv  &(*bins2[i]);
		if (tmp != 0) {
		    if (tmp->cnt() != counts[jc+i]) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- function ibis::bitvector::count "
			    "did not produce correct answer, entering it for "
			    "debugging purpose ...";
			(void) bv.count(*bins2[i]);
		    }
		    delete tmp;
		}
		else {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- bitwise AND didnot produce an invalid "
			"bitvector";
		}
#endif
	    }
	}
	else {
	    const size_t jc = (bounds1.size()-2) * bins2.size();
	    for (unsigned i = 0; i < bins2.size(); ++ i)
		counts[jc + i] = 0;
	}

	// clean up bins2
	for (unsigned i = 0; i < bins2.size(); ++ i) {
	    delete bins2[i];
	    bins2[i] = 0;
	}
    }
    catch (...) {
	// clean up bins2
	for (unsigned i = 0; i < bins2.size(); ++ i) {
	    delete bins2[i];
	    bins2[i] = 0;
	}

	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI received an exception, stopping";
	return -7L;
    }

    if (ibis::gVerbose > 0) {
	timer.stop();
	ibis::util::logger(0).buffer()
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistributionI completed filling a " << nb1 << " x "
	    << nb2 << " histogram on " << col1.name() << " and "
	    << col2.name() << " with " << counts.size() << " cell"
	    << (counts.size() > 1 ? "s" : "") << " using " << timer.CPUTime()
	    << " sec (CPU), " << timer.realTime() << " sec (elapsed)";
    }
    return counts.size();
} // ibis::part::get2DDistributionI

/// This function makes use of an existing index to produce bitmaps
/// representing a set of bins defined by bnds.  Following the private
/// convention used in FastBit, there are two open bins at the two ends.
int ibis::part::coarsenBins(const ibis::column &col, uint32_t nbin,
			    std::vector<double> &bnds,
			    std::vector<ibis::bitvector*> &btmp) const {
    ibis::column::indexLock lock(&col, "ibis::part::coarsenBins");
    const ibis::index* idx = lock.getIndex();
    if (idx == 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::coarsenBins can not proceed with index for "
	    << col.name();
	return -1;
    }

    array_t<uint32_t> wbnds(nbin);
    // retrieve bins used by idx
    std::vector<double> idxbin;
    idx->binBoundaries(idxbin);
    const double maxval = col.getActualMax();
    while (idxbin.size() > 1 && idxbin.back() >= maxval)
	idxbin.pop_back();
    if (idxbin.empty()) { // too few bins to be interesting
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::coarsenBins can not proceed because column "
	    << col.name() << " has either no valid values or a single value";
	return -2;
    }
    if (idxbin.size() > nbin*3/2) { // coarsen the bins
	std::vector<uint32_t> idxwgt;
	idx->binWeights(idxwgt);
	if (idxwgt.size() < idxbin.size()) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::coarsenBins failed to count the values of "
		<< col.name();
	    return -3;
	}

	array_t<uint32_t> wgt2(idxwgt.size());
	std::copy(idxwgt.begin(), idxwgt.end(), wgt2.begin());

	ibis::index::divideCounts(wbnds, wgt2);
	while (wbnds.size() > 1 && wbnds[wbnds.size()-2] >= idxbin.size())
	    wbnds.pop_back();
	if (wbnds.size() < 2) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::coarsenBins failed to divide the values into "
		<< nbin << " bins";
	    return -4;
	}
    }
    else { // no need to coarsen anything
	wbnds.resize(idxbin.size());
	for (unsigned i = 0; i < idxbin.size(); ++ i)
	    wbnds[i] = i+1;
    }

    bnds.resize(wbnds.size());
    btmp.reserve(wbnds.size());
    // first bin: open to the left
    bnds[0] = idxbin[wbnds[0]];
    ibis::qContinuousRange rng(col.name(), ibis::qExpr::OP_LT, bnds[0]);
    ibis::bitvector bv;
    LOGGER(ibis::gVerbose >= 6) << "ibis::part[" << (m_name ? m_name : "")
	      << "]::coarsenBins evaluating " << rng << " for bin 0 in "
	      << col.name();
    long ierr = idx->evaluate(rng, bv);
    if (ierr < 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::coarsenBins failed to evaluate query " << rng
	    << ", ierr=" << ierr;
	return -6;
    }
    btmp.push_back(new ibis::bitvector(bv));

    // middle bins: two-sided, inclusive left, exclusive right
    rng.leftOperator() = ibis::qExpr::OP_LE;
    rng.rightOperator() = ibis::qExpr::OP_LT;
    for (unsigned i = 1; i < wbnds.size()-1; ++ i) {
	rng.leftBound() = idxbin[wbnds[i-1]];
	rng.rightBound() = idxbin[wbnds[i]];
	bnds[i] = idxbin[wbnds[i]];
	LOGGER(ibis::gVerbose >= 6)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::coarsenBins evaluating " << rng << " for bin "
	    << i << " in " << col.name();

	ierr = idx->evaluate(rng, bv);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose >= 3)
		<< "ibis::part[" << (m_name ? m_name : "")
		<< "]::coarsenBins failed to evaluate query " << rng
		<< ", ierr=" << ierr;
	    return -6;
	}

	btmp.push_back(new ibis::bitvector(bv));
    }
    bnds.resize(wbnds.size()-1); // remove the last element

    // last bin: open to the right
    rng.rightOperator() = ibis::qExpr::OP_UNDEFINED;
    rng.leftBound() = idxbin[wbnds[wbnds.size()-2]];
    LOGGER(ibis::gVerbose >= 6)
	<< "ibis::part[" << (m_name ? m_name : "")
	<< "]::coarsenBins evaluating " << rng << " for bin "
	<< wbnds.size()-1 << " in " << col.name();
    ierr = idx->evaluate(rng, bv);
    if (ierr < 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::coarsenBins failed to evaluate query " << rng
	    << ", ierr=" << ierr;
	return -6;
    }

    btmp.push_back(new ibis::bitvector(bv));
    return btmp.size();
} // ibis::part::coarsenBins

/// The caller specifies only the number of bins, but let this function
/// decide where to place the bin boundaries.  This function attempts to
/// make sure the 1D bins for each dimension are equal-weight bins, which
/// is likely to produce evenly distributed 2D bins but does not guarantee
/// the uniformity.  It uses the templated function adaptive2DBins, which
/// starts with a set of regularly spaced bins and coalesces the regular
/// bins to produce the desired number of bins.
///
/// @note It return the number of actual bins used on success.  Caller
/// needs to check the sizes of bounds1 and bounds2 for the actual bin
/// bounaries.
long ibis::part::get2DDistribution(const char *constraints,
				   const char *name1, const char *name2,
				   uint32_t nb1, uint32_t nb2,
				   std::vector<double> &bounds1,
				   std::vector<double> &bounds2,
				   std::vector<uint32_t> &counts) const {
    if (constraints == 0 || *constraints == 0 || *constraints == '*')
	// unconditional histogram
	return get2DDistribution(name1, name2, nb1, nb2,
				 bounds1, bounds2, counts);

    long ierr = -1;
    columnList::const_iterator it1 = columns.find(name1);
    columnList::const_iterator it2 = columns.find(name2);
    if (it1 == columns.end() || it2 == columns.end()) {
	if (it1 == columns.end())
	    logWarning("get2DDistribution", "%s is not a known column name",
		       name1);
	if (it2 == columns.end())
	    logWarning("get2DDistribution", "%s is not a known column name",
		       name2);
	return ierr;
    }

    const ibis::column *col1 = (*it1).second;
    const ibis::column *col2 = (*it2).second;
    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistribution attempting to compute a " << nb1 << " x "
	    << nb2 << " histogram on " << name1 << " and " << name2
	    << " subject to \"" << (constraints ? constraints : "") << "\"";
	timer.start();
    }

    ibis::bitvector mask;
    if (constraints != 0 && *constraints != 0) {
	ibis::query q(ibis::util::userName(), this);
	q.setWhereClause(constraints);
	ierr = q.evaluate();
	if (ierr < 0)
	    return ierr;
	const ibis::bitvector *hits = q.getHitVector();
	if (hits->cnt() == 0) // nothing to do any more
	    return 0;
	mask.copy(*hits);
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get2DDistribution -- the constraints \"" << constraints
	    << "\" selects " << mask.cnt() << " record"
	    << (mask.cnt() > 1 ? "s" : "") << " out of " << nEvents;
    }
    else {
	col1->getNullMask(mask);
	ibis::bitvector tmp;
	col2->getNullMask(tmp);
	mask &= tmp;
    }

    counts.clear();
    switch (col1->type()) {
    case ibis::SHORT:
    case ibis::BYTE:
    case ibis::INT: {
	array_t<int32_t> *val1 = col1->selectInts(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}
	array_t<int32_t> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *val2 = col2->selectLongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *val2 = col2->selectULongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("get2DDistribution", "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::USHORT:
    case ibis::UBYTE:
    case ibis::UINT:
    case ibis::CATEGORY: {
	array_t<uint32_t> *val1 = col1->selectUInts(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<uint32_t> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *val2 = col2->selectLongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *val2 = col2->selectULongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("get2DDistribution", "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::LONG: {
	array_t<int64_t> *val1 = col1->selectLongs(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}
	array_t<int32_t> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *val2 = col2->selectLongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *val2 = col2->selectULongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("get2DDistribution", "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::ULONG: {
	array_t<uint64_t> *val1 = col1->selectULongs(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<uint32_t> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *val2 = col2->selectLongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *val2 = col2->selectULongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("get2DDistribution", "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	for (size_t i = 0; i < bounds1.size(); ++ i)
	    bounds1[i] = std::ceil(bounds1[i]);
	break;}
    case ibis::FLOAT: {
	array_t<float> *val1 = col1->selectFloats(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<float> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    delete val2;
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *val2 = col2->selectLongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *val2 = col2->selectULongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("get2DDistribution", "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::DOUBLE: {
	array_t<double> *val1 = col1->selectDoubles(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<double> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::LONG: {
	    array_t<int64_t> *val2 = col2->selectLongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::ULONG: {
	    array_t<uint64_t> *val2 = col2->selectULongs(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ierr = adaptive2DBins(*val1, *val2, nb1, nb2,
				  bounds1, bounds2, counts);
	    delete val2;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("get2DDistribution", "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    default: {
	ierr = -3;
	logWarning("get2DDistribution", "unable to handle column type %d",
		   static_cast<int>(col1->type()));
	break;}
    }

    if ((bounds1.size()-1) * (bounds2.size()-1) == counts.size())
	ierr = counts.size();
    else
	ierr = -2;
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("get2DDistribution",
		   "computing the joint distribution of column %s and "
		   "%s%s%s took %g sec(CPU), %g sec(elapsed)",
		   (*it1).first, (*it2).first,
		   (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::get2DDistribution

/// The old implementation that uses binary lookup.  For floating-point
/// values, this function will go through the intermediate arrays three
/// times, once to compute the actual minimum and maximum values, once to
/// count the 1D distributions, and finally to count the number of values
/// in the 2D bins.  The last step is more expensive then the first two
/// because it involves two binary searches, one on each each set of the
/// boundaries.
long ibis::part::old2DDistribution(const char *constraints,
				   const char *name1, const char *name2,
				   uint32_t nb1, uint32_t nb2,
				   std::vector<double> &bounds1,
				   std::vector<double> &bounds2,
				   std::vector<uint32_t> &counts) const {
    if (constraints == 0 || *constraints == 0 || *constraints == '*')
	return get2DDistribution(name1, name2, nb1, nb2,
				 bounds1, bounds2, counts);

    long ierr = -1;
    columnList::const_iterator it1 = columns.find(name1);
    columnList::const_iterator it2 = columns.find(name2);
    if (it1 == columns.end() || it2 == columns.end()) {
	if (it1 == columns.end())
	    logWarning("old2DDistribution", "%s is not a known column name",
		       name1);
	if (it2 == columns.end())
	    logWarning("old2DDistribution", "%s is not a known column name",
		       name2);
	return ierr;
    }

    const ibis::column *col1 = (*it1).second;
    const ibis::column *col2 = (*it2).second;
    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::old2DDistribution attempting to compute a " << nb1
	    << " x " << nb2 << " histogram on "
	    << name1 << " and " << name2 << " subject to \""
	    << (constraints ? constraints : "") << "\"";
	timer.start();
    }

    ibis::bitvector mask;
    if (constraints != 0 && *constraints != 0) {
	ibis::query q(ibis::util::userName(), this);
	q.setWhereClause(constraints);
	ierr = q.evaluate();
	if (ierr < 0)
	    return ierr;
	const ibis::bitvector *hits = q.getHitVector();
	if (hits->cnt() == 0) // nothing to do any more
	    return 0;
	mask.copy(*hits);
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::old2DDistribution -- the constraints \"" << constraints
	    << "\" selects " << mask.cnt() << " record"
	    << (mask.cnt() > 1 ? "s" : "") << " out of " << nEvents;
    }
    else {
	col1->getNullMask(mask);
	ibis::bitvector tmp;
	col2->getNullMask(tmp);
	mask &= tmp;
    }

    counts.clear();
    switch (col1->type()) {
    case ibis::SHORT:
    case ibis::BYTE:
    case ibis::INT: {
	array_t<int32_t> *val1 = col1->selectInts(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}
	array_t<int32_t> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("old2DDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::USHORT:
    case ibis::UBYTE:
    case ibis::UINT:
    case ibis::CATEGORY: {
	array_t<uint32_t> *val1 = col1->selectUInts(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<uint32_t> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<double> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("old2DDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::FLOAT: {
	array_t<float> *val1 = col1->selectFloats(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<float> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<double> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("old2DDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::DOUBLE: {
	array_t<double> *val1 = col1->selectDoubles(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<double> bnd1;
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<double> bnd2;
	    ibis::part::mapValues(*val1, *val2, nb1, nb2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("old2DDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    default: {
	ierr = -3;
	logWarning("old2DDistribution",
		   "unable to handle column type %d",
		   static_cast<int>(col1->type()));
	break;}
    }

    if ((bounds1.size()-1) * (bounds2.size()-1) == counts.size())
	ierr = counts.size();
    else
	ierr = -2;
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("old2DDistribution",
		   "computing the joint distribution of column %s and "
		   "%s%s%s took %g sec(CPU), %g sec(elapsed)",
		   (*it1).first, (*it2).first,
		   (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::old2DDistribution

/// Upon successful completion of this function, the return value shall be
/// the number of bins produced, which is equal to the number of elements
/// in array counts.
/// Error codes:
/// - -1: one or more of the column names are nil strings;
/// - -2: one or more column names are not present in the data partition;
/// - -5: error in column masks;
/// - [-100, -160]: error detected by get3DDistributionA.
long ibis::part::get3DDistribution(const char *cname1, const char *cname2,
				   const char *cname3,
				   uint32_t nb1, uint32_t nb2, uint32_t nb3,
				   std::vector<double> &bounds1,
				   std::vector<double> &bounds2,
				   std::vector<double> &bounds3,
				   std::vector<uint32_t> &counts,
				   const char* const option) const {
    if (cname1 == 0 || *cname1 == 0 ||
	cname2 == 0 || *cname2 == 0 ||
	cname3 == 0 || *cname3 == 0) return -1L;

    const ibis::column* col1 = getColumn(cname1);
    const ibis::column* col2 = getColumn(cname2);
    const ibis::column* col3 = getColumn(cname3);
    if (col1 == 0 || col2 == 0 || col3 == 0)
	return -2L;

    ibis::bitvector mask;
    col1->getNullMask(mask);
    if (mask.size() == nEvents) {
	ibis::bitvector tmp;
	col2->getNullMask(tmp);
	mask &= tmp;
	col3->getNullMask(tmp);
	mask &= tmp;
	if (mask.cnt() == 0) {
	    bounds1.clear();
	    bounds2.clear();
	    bounds3.clear();
	    counts.clear();
	    return 0L;
	}
    }
    else {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::part[" << m_name
	    << "]::get3DDistributionA - null mask of " << col1->name()
	    << " has " << mask.size() << " bits, but " << nEvents
	    << " are expected";
	return -5L;
    }

    long ierr = get3DDistributionA(mask, *col1, *col2, *col3, nb1, nb2, nb3,
				   bounds1, bounds2, bounds3, counts);
    if (ierr <= 0)
	ierr -= 100;
    return ierr;
} // ibis::part::get3DDistribution

/// Upon successful completion of this function, the return value shall be
/// the number of bins produced, which is equal to the number of elements
/// in array counts.
/// Error codes:
/// - -1: one or more of the column names are nil strings;
/// - -2: one or more column names are not present in the data partition;
/// - -3: contraints contain invalid expressions or invalid column names;
/// - -4: contraints can not be evaluated correctly;
/// - [-100, -160]: error detected by get3DDistributionA.
long ibis::part::get3DDistribution(const char *constraints,
				   const char *cname1, const char *cname2,
				   const char *cname3,
				   uint32_t nb1, uint32_t nb2, uint32_t nb3,
				   std::vector<double> &bounds1,
				   std::vector<double> &bounds2,
				   std::vector<double> &bounds3,
				   std::vector<uint32_t> &counts) const {
    if (cname1 == 0 || *cname1 == 0 ||
	cname2 == 0 || *cname2 == 0 ||
	cname3 == 0 || *cname3 == 0) return -1L;
    if (constraints == 0 || *constraints == 0 || *constraints == '*')
	return get3DDistribution(cname1, cname2, cname3, nb1, nb2, nb3,
				 bounds1, bounds2, bounds3, counts);

    const ibis::column* col1 = getColumn(cname1);
    const ibis::column* col2 = getColumn(cname2);
    const ibis::column* col3 = getColumn(cname3);
    if (col1 == 0 || col2 == 0 || col3 == 0)
	return -2L;

    long ierr;
    ibis::bitvector mask;
    { // a block for finding out which records satisfy the constraints
	std::string sel = cname1;
	sel += ", ";
	sel += cname2;
	sel += ", ";
	sel += cname3;
	ibis::query qq(ibis::util::userName(), this);
	ierr = qq.setSelectClause(sel.c_str());
	if (ierr < 0)
	    return -2L;
	ierr = qq.setWhereClause(constraints);
	if (ierr < 0)
	    return -3L;

	ierr = qq.evaluate();
	if (ierr < 0)
	    return -4L;
	if (qq.getNumHits() == 0) {
	    bounds1.clear();
	    bounds2.clear();
	    bounds3.clear();
	    counts.clear();
	    return 0;
	}

	mask.copy(*(qq.getHitVector()));
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "") << "]::get3DDistribution"
	    << " -- the constraints \"" << constraints << "\" selects "
	    << mask.cnt() << " record" << (mask.cnt() > 1 ? "s" : "")
	    << " out of " << nEvents;
    }

    ierr = get3DDistributionA(mask, *col1, *col2, *col3, nb1, nb2, nb3,
			      bounds1, bounds2, bounds3, counts);
    if (ierr <= 0)
	ierr -= 100;
    return ierr;
} // ibis::part::get3DDistribution

/// Compute 3D distribution with adaptive bins.  It is layered on top of
/// three templated functions, get3DDistributionA1, get3DDistributionA2,
/// and adaptive3DBins.  The last function, which is a class function if
/// ibis::part, performs the actual counting, the others are mainly
/// responsible for retrieving values from disk.
///
/// This function either returns a negative between -1 and -11 to indicate
/// error detected here, or a value returned by get3DDistributionA1.  On
/// successful completion of this function, it should return the number of
/// bins in array counts, which should be exactly
/// @code bounds1.size() * bounds2.size() * bounds3.size(). @endcode
long ibis::part::get3DDistributionA(const ibis::bitvector &mask,
				    const ibis::column &col1,
				    const ibis::column &col2,
				    const ibis::column &col3,
				    uint32_t nb1, uint32_t nb2, uint32_t nb3,
				    std::vector<double> &bounds1,
				    std::vector<double> &bounds2,
				    std::vector<double> &bounds3,
				    std::vector<uint32_t> &counts) const {
    long ierr = -1;
    switch (col1.type()) {
    default: {
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get3DDistributionA -- does not suport column type "
	    << ibis::TYPESTRING[(int)col1.type()] << " for column "
	    << col1.name();
	ierr = -1;
	break;}
    case ibis::BYTE: {
	array_t<char>* vals1 = col1.selectBytes(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -2;
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char>* vals1 = col1.selectUBytes(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -3;
	}
	break; }
    case ibis::SHORT: {
	array_t<int16_t>* vals1 = col1.selectShorts(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -4;
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t>* vals1 = col1.selectUShorts(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -5;
	}
	break;}
    case ibis::INT: {
	array_t<int32_t>* vals1 = col1.selectInts(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -6;
	}
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	array_t<uint32_t>* vals1 = col1.selectUInts(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -7;
	}
	break;}
    case ibis::LONG: {
	array_t<int64_t>* vals1 = col1.selectLongs(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -8;
	}
	break;}
    case ibis::ULONG: {
	array_t<uint64_t>* vals1 = col1.selectULongs(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	    for (size_t i = 0; i < bounds1.size(); ++ i)
		bounds1[i] = std::ceil(bounds1[i]);
	}
	else {
	    ierr = -9;
	}
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals1 = col1.selectFloats(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	}
	else {
	    ierr = -10;
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals1 = col1.selectDoubles(mask);
	if (vals1 != 0) {
	    ierr = get3DDistributionA1(mask, *vals1, col2, col3, nb1, nb2, nb3,
				       bounds1, bounds2, bounds3, counts);
	    delete vals1;
	}
	else {
	    ierr = -11;
	}
	break;}
    } // col1.type()
    return ierr;
} // ibis::part::get3DDistributionA

/// Read the value of the second column.  Call get3DDistributionA2 to
/// process the next column and eventually compute the histogram.
/// This function may return a value between -20 and -30 to indicate an
/// error, or a value returned by get3DDistributionA2.
template <typename E1>
long ibis::part::get3DDistributionA1(const ibis::bitvector &mask,
				     const array_t<E1> &vals1,
				     const ibis::column &col2,
				     const ibis::column &col3,
				     uint32_t nb1, uint32_t nb2, uint32_t nb3,
				     std::vector<double> &bounds1,
				     std::vector<double> &bounds2,
				     std::vector<double> &bounds3,
				     std::vector<uint32_t> &counts) const {
    long ierr = -20;
    switch (col2.type()) {
    default:
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get3DDistributionA -- does not suport column type "
	    << ibis::TYPESTRING[(int)col2.type()] << " for column "
	    << col2.name();
	ierr = -20;
	break;
  case ibis::BYTE: {
	array_t<char>* vals2 = col2.selectBytes(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -21;
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char>* vals2 = col2.selectUBytes(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -22;
	}
	break; }
    case ibis::SHORT: {
	array_t<int16_t>* vals2 = col2.selectShorts(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -23;
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t>* vals2 = col2.selectUShorts(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -24;
	}
	break;}
    case ibis::INT: {
	array_t<int32_t>* vals2 = col2.selectInts(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -25;
	}
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	array_t<uint32_t>* vals2 = col2.selectUInts(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -26;
	}
	break;}
    case ibis::LONG: {
	array_t<int64_t>* vals2 = col2.selectLongs(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -27;
	}
	break;}
    case ibis::ULONG: {
	array_t<uint64_t>* vals2 = col2.selectULongs(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	    for (size_t i = 0; i < bounds2.size(); ++ i)
		bounds2[i] = std::ceil(bounds2[i]);
	}
	else {
	    ierr = -28;
	}
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals2 = col2.selectFloats(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	}
	else {
	    ierr = -29;
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals2 = col2.selectDoubles(mask);
	if (vals2 != 0) {
	    ierr = get3DDistributionA2(mask, vals1, *vals2, col3, nb1, nb2,
				       nb3, bounds1, bounds2, bounds3, counts);
	    delete vals2;
	}
	else {
	    ierr = -30;
	}
	break;}
    } // col2.type()
    return ierr;
} // ibis::part::get3DDistributionA1

/// Read the values of the third column.  Call the actual adaptive
/// binning function adaptive3DBins to compute the histogram.
/// Return the number of bins in the histogram or a negative value in the
/// range of -40 to -60 to indicate errors.
template <typename E1, typename E2>
long ibis::part::get3DDistributionA2(const ibis::bitvector &mask,
				     const array_t<E1> &vals1,
				     const array_t<E2> &vals2,
				     const ibis::column &col3,
				     uint32_t nb1, uint32_t nb2, uint32_t nb3,
				     std::vector<double> &bounds1,
				     std::vector<double> &bounds2,
				     std::vector<double> &bounds3,
				     std::vector<uint32_t> &counts) const {
    long ierr = -40;
    switch (col3.type()) {
    default:
	LOGGER(ibis::gVerbose >= 2)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::get3DDistributionA -- does not suport column type "
	    << ibis::TYPESTRING[(int)col3.type()] << " for column "
	    << col3.name();
	ierr = -40;
	break;
  case ibis::BYTE: {
	array_t<char>* vals3 = col3.selectBytes(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -51;
	    }
	    delete vals3;
	}
	else {
	    ierr = -41;
	}
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char>* vals3 = col3.selectUBytes(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -52;
	    }
	    delete vals3;
	}
	else {
	    ierr = -42;
	}
	break; }
    case ibis::SHORT: {
	array_t<int16_t>* vals3 = col3.selectShorts(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -53;
	    }
	    delete vals3;
	}
	else {
	    ierr = -43;
	}
	break;}
    case ibis::USHORT: {
	array_t<uint16_t>* vals3 = col3.selectUShorts(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -54;
	    }
	    delete vals3;
	}
	else {
	    ierr = -44;
	}
	break;}
    case ibis::INT: {
	array_t<int32_t>* vals3 = col3.selectInts(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
			       bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -55;
	    }
	    delete vals3;
	}
	else {
	    ierr = -45;
	}
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	array_t<uint32_t>* vals3 = col3.selectUInts(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -56;
	    }
	    delete vals3;
	}
	else {
	    ierr = -46;
	}
	break;}
    case ibis::LONG: {
	array_t<int64_t>* vals3 = col3.selectLongs(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -57;
	    }
	    delete vals3;
	}
	else {
	    ierr = -47;
	}
	break;}
    case ibis::ULONG: {
	array_t<uint64_t>* vals3 = col3.selectULongs(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
		for (size_t i = 0; i < bounds3.size(); ++ i)
		    bounds3[i] = std::ceil(bounds3[i]);
	    }
	    catch (...) {
		ierr = -58;
	    }
	    delete vals3;
	}
	else {
	    ierr = -48;
	}
	break;}
    case ibis::FLOAT: {
	array_t<float>* vals3 = col3.selectFloats(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
	    }
	    catch (...) {
		ierr = -59;
	    }
	    delete vals3;
	}
	else {
	    ierr = -49;
	}
	break;}
    case ibis::DOUBLE: {
	array_t<double>* vals3 = col3.selectDoubles(mask);
	if (vals3 != 0) {
	    try {
		ierr = adaptive3DBins(vals1, vals2, *vals3, nb1, nb2, nb3,
				      bounds1, bounds2, bounds3, counts);
	    }
	    catch (...) {
		ierr = -60;
	    }
	    delete vals3;
	}
	else {
	    ierr = -50;
	}
	break;}
    } // col3.type()
    return ierr;
} // ibis::part::get3DDistributionA2

/// Based on the column type, decide how to retrieve the values and
/// invokethe lower level support functions.
long ibis::part::get1DBins_(const ibis::bitvector &mask,
			    const ibis::column &col,
			    uint32_t nbin, std::vector<double> &bounds,
			    std::vector<ibis::bitvector> &bins,
			    const char *mesg) const {
    if (mask.cnt() == 0) return 0L;
    if (mask.size() != nEvents) return -6L;
    if (mesg == 0 || *mesg == 0)
	mesg = ibis::util::userName();
    LOGGER(ibis::gVerbose >= 4)
	<< mesg << " -- invoking get1DBins_ on column " << col.name()
	<< " type " << ibis::TYPESTRING[(int)col.type()] << "(" << col.type()
	<< ") with mask of " << mask.cnt() << " out of " << mask.size();

    long ierr;
    switch (col.type()) {
    default: {
	LOGGER(ibis::gVerbose >= 1)
	    << mesg << " -- unable to work with column " << col.name()
	    << " of type " << ibis::TYPESTRING[(int)col.type()] << "("
	    << col.type() << ")";
	ierr = -7;
	break;}
    case ibis::BYTE: {
	char vmin, vmax;
	array_t<char> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<char>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -8L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -9L;
	    }
	    vmin = 127; //std::numeric_limits<char>::max();
	    vmax = -128; //std::numeric_limits<char>::min();
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectBytes(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -10L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -11L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
				    bounds, bins);
	delete vals;
	break;}
    case ibis::UBYTE: {
	unsigned char vmin, vmax;
	array_t<unsigned char> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<unsigned char>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -12L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -13L;
	    }
	    vmin = 255;
	    vmax = 0;
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectUBytes(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -14L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -15L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
				    bounds, bins);
	delete vals;
	break;}
    case ibis::SHORT: {
	int16_t vmin, vmax;
	array_t<int16_t> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<int16_t>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -16L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -17L;
	    }
	    vmin = std::numeric_limits<int16_t>::max();
	    vmax = std::numeric_limits<int16_t>::min();
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectShorts(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -18L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -19L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
				    bounds, bins);
	delete vals;
	break;}
    case ibis::USHORT: {
	uint16_t vmin, vmax;
	array_t<uint16_t> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<uint16_t>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -20L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -21L;
	    }
	    vmin = std::numeric_limits<uint16_t>::max();
	    vmax = 0;
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectUShorts(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -22L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -23L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
				    bounds, bins);
	delete vals;
	break;}
    case ibis::INT: {
	int32_t vmin, vmax;
	array_t<int32_t> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<int32_t>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -24L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -25L;
	    }
	    vmin = std::numeric_limits<int32_t>::max();
	    vmax = std::numeric_limits<int32_t>::min();
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectInts(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -26L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -27L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	if (static_cast<uint32_t>(vmax-vmin) < vals->size()) {
	    ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
					bounds, bins);
	}
	else {
	    ierr = adaptiveFloatsDetailed(mask, *vals, vmin, vmax, nbin,
					  bounds, bins);
	    for (size_t i = 0; i < bounds.size(); ++ i)
		bounds[i] = std::ceil(bounds[i]);
	}
	delete vals;
	break;}
    case ibis::CATEGORY:
    case ibis::UINT: {
	uint32_t vmin, vmax;
	array_t<uint32_t> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<uint32_t>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -28L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -29L;
	    }
	    vmin = std::numeric_limits<uint32_t>::max();
	    vmax = 0;
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectUInts(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -30L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -31L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	if (vmax - vmin < vals->size()) {
	    ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
					bounds, bins);
	}
	else {
	    ierr = adaptiveFloatsDetailed(mask, *vals, vmin, vmax, nbin,
					  bounds, bins);
	    for (size_t i = 0; i < bounds.size(); ++ i)
		bounds[i] = std::ceil(bounds[i]);
	}
	delete vals;
	break;}
    case ibis::LONG: {
	int64_t vmin, vmax;
	array_t<int64_t> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<int64_t>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -32L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -33L;
	    }
	    vmin = std::numeric_limits<int64_t>::max();
	    vmax = std::numeric_limits<int64_t>::min();
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectLongs(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -34L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -35L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	if (static_cast<uint32_t>(vmax-vmin) < vals->size()) {
	    ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
					bounds, bins);
	}
	else {
	    ierr = adaptiveFloatsDetailed(mask, *vals, vmin, vmax, nbin,
					  bounds, bins);
	    for (size_t i = 0; i < bounds.size(); ++ i)
		bounds[i] = std::ceil(bounds[i]);
	}
	delete vals;
	break;}
    case ibis::ULONG: {
	uint64_t vmin, vmax;
	array_t<uint64_t> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<uint64_t>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -36L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -37L;
	    }
	    vmin = std::numeric_limits<uint64_t>::max();
	    vmax = 0;
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectULongs(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -38L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -39L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	if (vmax - vmin < vals->size()) {
	    ierr = adaptiveIntsDetailed(mask, *vals, vmin, vmax, nbin,
					bounds, bins);
	}
	else {
	    ierr = adaptiveFloatsDetailed(mask, *vals, vmin, vmax, nbin,
					  bounds, bins);
	    for (size_t i = 0; i < bounds.size(); ++ i)
		bounds[i] = std::ceil(bounds[i]);
	}
	delete vals;
	break;}
    case ibis::FLOAT: {
	float vmin, vmax;
	array_t<float> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<float>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -40L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -41L;
	    }
	    vmin = std::numeric_limits<float>::max();
	    vmax = std::numeric_limits<float>::min();
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectFloats(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -42L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -43L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	ierr = adaptiveFloatsDetailed(mask, *vals, vmin, vmax, nbin,
				      bounds, bins);
	delete vals;
	break;}
    case ibis::DOUBLE: {
	double vmin, vmax;
	array_t<double> *vals=0;
	ibis::fileManager::ACCESS_PREFERENCE acc = accessHint(mask, 1);
	if (acc == ibis::fileManager::PREFER_READ) {
	    vals = new array_t<double>;
	    ierr = col.getRawData(*vals);
	    if (ierr < 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		delete vals;
		return -44L;
	    }
	    else if (vals->size() != nEvents) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve "
		    << nEvents << " byte" << (nEvents > 1 ? "s" : "")
		    << ", but got " << vals->size();
		delete vals;
		return -45L;
	    }
	    vmin = std::numeric_limits<double>::max();
	    vmax = std::numeric_limits<double>::min();
	    for (ibis::bitvector::indexSet is = mask.firstIndexSet();
		 is.nIndices() > 0; ++ is) {
		const ibis::bitvector::word_t nind = is.nIndices();
		const ibis::bitvector::word_t *idx = is.indices();
		if (is.isRange()) {
		    for (ibis::bitvector::word_t ii = *idx;
			 ii < idx[1]; ++ ii) {
			if (vmin > (*vals)[ii])
			    vmin = (*vals)[ii];
			if (vmax < (*vals)[ii])
			    vmax = (*vals)[ii];
		    }
		}
		else {
		    for (size_t ii = 0; ii < nind; ++ ii) {
			const size_t jj = idx[ii];
			if (vmin > (*vals)[jj])
			    vmin = (*vals)[jj];
			if (vmax < (*vals)[jj])
			    vmax = (*vals)[jj];
		    }
		}
	    }
	}
	else {
	    ibis::bitvector::word_t nsel = mask.cnt();
	    vals = col.selectDoubles(mask);
	    if (vals == 0) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg
		    << " failed to retrieve any values for column "
		    << col.name();
		return -46L;
	    }
	    else if (vals->size() != nsel) {
		LOGGER(ibis::gVerbose > 1)
		    << "Warning -- " << mesg << " expected to retrieve " << nsel
		    << " byte" << (nsel > 1 ? "s" : "") << ", but got "
		    << vals->size();
		delete vals;
		return -47L;
	    }
	    vmin = (*vals)[0];
	    vmax = (*vals)[0];
	    for (uint32_t i = 1; i < nsel; ++ i) {
		if (vmin > (*vals)[i])
		    vmin = (*vals)[i];
		if (vmax < (*vals)[i])
		    vmax = (*vals)[i];
	    }
	}
	ierr = adaptiveFloatsDetailed(mask, *vals, vmin, vmax, nbin,
				      bounds, bins);
	delete vals;
	break;}
    }
    return ierr;
} // ibis::part::get1DBins_

/// If the string constraints is nil or an empty string or starting with an
/// asterisk (*), it is assumed every valid record of the named column is
/// used.  Arrays bounds1 and bins are both for output only.  Upon
/// successful completion of this function, the return value shall be the
/// number of bins actually used.  A return value of 0 indicates no record
/// satisfy the constraints.  A negative return indicates error.
///
/// @sa ibis::part::get1DDistribution
/// @sa ibis::part::part::adaptiveInts
long ibis::part::get1DBins(const char *constraints, const char *cname1,
			   uint32_t nb1, std::vector<double> &bounds1,
			   std::vector<ibis::bitvector> &bins) const {
    if (cname1 == 0 || *cname1 == 0) return -1L;
    ibis::column *col1 = getColumn(cname1);
    if (col1 == 0) return -2L;
    std::string mesg;
    {
	std::ostringstream oss;
	oss << "ibis::part[" << (m_name ? m_name : "") << "]::get1DBins("
	    << cname1 << ", " << nb1 << ")";
	mesg = oss.str();
    }
    ibis::util::timer atimer(mesg.c_str(), 1);
    ibis::bitvector mask;
    long ierr;
    if (constraints == 0 || *constraints == 0 || *constraints == '*') {
	// use all valid records
	col1->getNullMask(mask);
    }
    else { // process the constraints to compute the mask
	ibis::query qq(ibis::util::userName(), this);
	ierr = qq.setSelectClause(cname1);
	if (ierr < 0)
	    return -3L;
	ierr = qq.setWhereClause(constraints);
	if (ierr < 0)
	    return -4L;
	ierr = qq.evaluate();
	if (ierr < 0)
	    return -5L;

	if (qq.getNumHits() == 0) {
	    bounds1.clear();
	    bins.clear();
	    return 0L;
	}
	mask.copy(*(qq.getHitVector()));
	LOGGER(ibis::gVerbose >= 2)
	    << mesg << " -- constraints \"" << constraints << "\" select "
	    << mask.cnt() << " record" << (mask.cnt() > 1 ? "s" : "")
	    << " out of " << nEvents;
    }

    ierr = get1DBins_(mask, *col1, nb1, bounds1, bins, mesg.c_str());
    return ierr;
} // ibis::part::get1DBins

/// If the string constraints is nil or an empty string or starting with an
/// asterisk (*), it is assumed every valid record of the named column is
/// used.  Arrays bounds1 and bins are both for output only.  Upon
/// successful completion of this function, the return value shall be the
/// number of bins actually used.  A return value of 0 indicates no record
/// satisfy the constraints.  A negative return indicates error.
///
/// @sa ibis::part::get2DDistribution
long ibis::part::get2DBins(const char *constraints,
			   const char *cname1, const char *cname2,
			   uint32_t nb1, uint32_t nb2,
			   std::vector<double> &bounds1,
			   std::vector<double> &bounds2,
			   std::vector<ibis::bitvector> &bins) const {
    if (cname1 == 0 || *cname1 == 0 || cname2 == 0 || *cname2 == 0) return -1L;
    ibis::column *col1 = getColumn(cname1);
    ibis::column *col2 = getColumn(cname2);
    if (col1 == 0 || col2 == 0) return -2L;
    std::string mesg;
    {
	std::ostringstream oss;
	oss << "ibis::part[" << (m_name ? m_name : "") << "]::get2DBins("
	    << cname1 << ", " << cname2 << ", " << nb1 << ", " << nb2 << ")";
	mesg = oss.str();
    }
    ibis::util::timer atimer(mesg.c_str(), 1);
    ibis::bitvector mask;
    long ierr;
    if (constraints == 0 || *constraints == 0 || *constraints == '*') {
	// use all valid records
	col1->getNullMask(mask);
	ibis::bitvector tmp;
	col2->getNullMask(tmp);
	mask &= tmp;
    }
    else { // process the constraints to compute the mask
	ibis::query qq(ibis::util::userName(), this);
	std::string sel = cname1;
	sel += ", ";
	sel += cname2;
	ierr = qq.setSelectClause(sel.c_str());
	if (ierr < 0)
	    return -3L;
	ierr = qq.setWhereClause(constraints);
	if (ierr < 0)
	    return -4L;
	ierr = qq.evaluate();
	if (ierr < 0)
	    return -5L;

	if (qq.getNumHits() == 0) {
	    bounds1.clear();
	    bins.clear();
	    return 0L;
	}
	mask.copy(*(qq.getHitVector()));
	LOGGER(ibis::gVerbose >= 2)
	    << mesg << " -- constraints \"" << constraints << "\" select "
	    << mask.cnt() << " record" << (mask.cnt() > 1 ? "s" : "")
	    << " out of " << nEvents;
    }

    if (mask.cnt() > 1) { // determine the number of bins to use
	if (nb1 <= 1) nb1 = 100;
	if (nb2 <= 1) nb2 = 100;
	const uint32_t nrows = mask.cnt();
	double tmp = std::exp(std::log((double)nrows)/3.0);
	if (nb1 > 2048 && (double)nb1 > tmp) {
	    if (nrows > 10000000)
		nb1 = static_cast<uint32_t>(0.5 + tmp);
	    else
		nb1 = 2048;
	}
	if (nb2 > 2048 && (double)nb2 > tmp) {
	    if (nrows > 10000000)
		nb2 = static_cast<uint32_t>(0.5 + tmp);
	    else
		nb2 = 2048;
	}
    }

    std::vector<ibis::bitvector> bins1;
    ierr = get1DBins_(mask, *col1, nb1, bounds1, bins1, mesg.c_str());
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose > 0)
	    << mesg << " -- get1DBins_ on " << cname1 << " failed with error "
	    << ierr;
	return ierr;
    }

    std::vector<ibis::bitvector> bins2;
    ierr = get1DBins_(mask, *col2, nb2, bounds2, bins2, mesg.c_str());
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose > 0)
	    << mesg << " -- get1DBins_ on " << cname2 << " failed with error "
	    << ierr;
	return ierr;
    }

    ierr = ibis::util::intersect(bins1, bins2, bins);
    return ierr;
} // ibis::part::get2DBins

/// If the string constraints is nil or an empty string or starting with an
/// asterisk (*), it is assumed every valid record of the named column is
/// used.  Arrays bounds1 and bins are both for output only.  Upon
/// successful completion of this function, the return value shall be the
/// number of bins actually used.  A return value of 0 indicates no record
/// satisfy the constraints.  A negative return indicates error.
///
/// @sa ibis::part::get2DDistribution
long ibis::part::get3DBins(const char *constraints, const char *cname1,
			   const char *cname2, const char *cname3,
			   uint32_t nb1, uint32_t nb2, uint32_t nb3,
			   std::vector<double> &bounds1,
			   std::vector<double> &bounds2,
			   std::vector<double> &bounds3,
			   std::vector<ibis::bitvector> &bins) const {
    if (cname1 == 0 || *cname1 == 0 || cname2 == 0 || *cname2 == 0
	|| cname3 == 0 || *cname3 == 0) return -1L;
    ibis::column *col1 = getColumn(cname1);
    ibis::column *col2 = getColumn(cname2);
    ibis::column *col3 = getColumn(cname3);
    if (col1 == 0 || col2 == 0 || col3 == 0) return -2L;
    std::string mesg;
    {
	std::ostringstream oss;
	oss << "ibis::part[" << (m_name ? m_name : "") << "]::get3DBins("
	    << cname1 << ", " << cname2 << ", " << cname3 << ", "
	    << nb1 << ", " << nb2 << ", " << nb3 << ")";
	mesg = oss.str();
    }
    ibis::util::timer atimer(mesg.c_str(), 1);
    ibis::bitvector mask;
    long ierr;
    if (constraints == 0 || *constraints == 0 || *constraints == '*') {
	// use all valid records
	col1->getNullMask(mask);
	ibis::bitvector tmp;
	col2->getNullMask(tmp);
	mask &= tmp;
	col3->getNullMask(tmp);
	mask &= tmp;
    }
    else { // process the constraints to compute the mask
	ibis::query qq(ibis::util::userName(), this);
	std::string sel = cname1;
	sel += ", ";
	sel += cname2;
	sel += ", ";
	sel += cname3;
	ierr = qq.setSelectClause(sel.c_str());
	if (ierr < 0)
	    return -3L;
	ierr = qq.setWhereClause(constraints);
	if (ierr < 0)
	    return -4L;
	ierr = qq.evaluate();
	if (ierr < 0)
	    return -5L;

	if (qq.getNumHits() == 0) {
	    bounds1.clear();
	    bins.clear();
	    return 0L;
	}
	mask.copy(*(qq.getHitVector()));
	LOGGER(ibis::gVerbose >= 2)
	    << mesg << " -- constraints \"" << constraints << "\" select "
	    << mask.cnt() << " record" << (mask.cnt() > 1 ? "s" : "")
	    << " out of " << nEvents;
    }

    if (mask.cnt() > 1) { // determine the number of bins to use
	const uint32_t nrows = mask.cnt();
	if (nb1 <= 1) nb1 = 32;
	if (nb2 <= 1) nb2 = 32;
	if (nb3 <= 1) nb2 = 32;
	double tmp = std::exp(std::log((double)nrows)*0.25);
	if (nb1 > 128 && nb1 > (uint32_t)tmp) {
	    if (nrows > 10000000)
		nb1 = static_cast<uint32_t>(0.5 + tmp);
	    else
		nb1 = 128;
	}
	if (nb2 > 128 && nb2 > (uint32_t)tmp) {
	    if (nrows > 10000000)
		nb2 = static_cast<uint32_t>(0.5 + tmp);
	    else
		nb2 = 128;
	}
	if (nb3 > 128 && nb3 > (uint32_t)tmp) {
	    if (nrows > 10000000)
		nb3 = static_cast<uint32_t>(0.5 + tmp);
	    else
		nb3 = 128;
	}
    }

    std::vector<ibis::bitvector> bins1;
    ierr = get1DBins_(mask, *col1, nb1, bounds1, bins1, mesg.c_str());
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose > 0)
	    << mesg << " -- get1DBins_ on " << cname1 << " failed with error "
	    << ierr;
	return ierr;
    }

    std::vector<ibis::bitvector> bins2;
    ierr = get1DBins_(mask, *col2, nb2, bounds2, bins2, mesg.c_str());
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose > 0)
	    << mesg << " -- get1DBins_ on " << cname2 << " failed with error "
	    << ierr;
	return ierr;
    }

    std::vector<ibis::bitvector> bins3;
    ierr = get1DBins_(mask, *col3, nb3, bounds3, bins3, mesg.c_str());
    if (ierr <= 0) {
	LOGGER(ibis::gVerbose > 0)
	    << mesg << " -- get1DBins_ on " << cname3 << " failed with error "
	    << ierr;
	return ierr;
    }

    ierr = ibis::util::intersect(bins1, bins2, bins3, bins);
    return ierr;
} // ibis::part::get3DBins

/// The templated function to decide the bin boundaries and count the number
/// of values fall in each bin.  This function differs from the one used by
/// getJointDistribution in that the bounds are defined with only closed
/// bins.
///
/// @note It goes through each data value twice, once to count each
/// individial values and once to put them into the specified bins.
///
/// @note The results of first counting may take up more memory than the
/// input data!
template <typename E1, typename E2>
void ibis::part::mapValues(array_t<E1> &val1, array_t<E2> &val2,
			   uint32_t nb1, uint32_t nb2,
			   array_t<E1> &bnd1, array_t<E2> &bnd2,
			   std::vector<uint32_t> &cnts) {
    if (val1.size() == 0 || val2.size() == 0 || val1.size() != val2.size())
	return;
    const size_t nr = (val1.size() <= val2.size() ?
		       val1.size() : val2.size());
    ibis::horometer timer;
    if (ibis::gVerbose > 3) {
	LOGGER(ibis::gVerbose >= 5)
	    << "ibis::part::mapValues(" << typeid(E1).name() << "["
	    << val1.size() << "], " << typeid(E2).name() << "["
	    << val2.size() << "], " << nb1 << ", " << nb2 << ") starting ...";
	timer.start();
    }
#if defined(SORT_VALUES_BEFORE_COUNT)
    ibis::util::sortall(val1, val2);
// #else
//     if (nb1*nb2 > 4096)
// 	ibis::util::sortall(val1, val2);
#endif
    equalWeightBins(val1, nb1, bnd1);
    equalWeightBins(val2, nb2, bnd2);
    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose >= 0)
	    << "ibis::part::mapValues(" << typeid(E1).name() << "["
	    << val1.size() << "], " << typeid(E2).name() << "["
	    << val2.size() << "], " << nb1 << ", " << nb2
	    << ") spent " << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed) to determine bin boundaries";
	timer.start();
    }

    const uint32_t nbnd1 = bnd1.size() - 1;
    const uint32_t nbnd2 = bnd2.size() - 1;
    cnts.resize(nbnd2 * nbnd1);
    for (uint32_t i = 0; i < nbnd2 * nbnd1; ++ i)
	cnts[i] = 0;

    for (uint32_t i = 0; i < nr; ++ i) {
	const uint32_t j1 = bnd1.find(val1[i]);
	const uint32_t j2 = bnd2.find(val2[i]);
	++ cnts[(j1 - (bnd1[j1]>val1[i]))*nbnd2 + j2 - (bnd2[j2]>val2[i])];
    }
    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(true)
	    << "ibis::part::mapValues(" << typeid(E1).name() << "["
	    << val1.size() << "], " << typeid(E2).name() << "["
	    << val2.size() << "], " << nb1 << ", " << nb2
	    << ") spent " << timer.CPUTime() << " sec(CPU), "
	    << timer.realTime() << " sec(elapsed) to count the number "
	    "of values in each bin";
    }
} // ibis::part::mapValues

template <typename T>
void ibis::part::equalWeightBins(const array_t<T> &vals, uint32_t nbins,
				 array_t<T> &bounds) {
    typename std::map<T, uint32_t> hist;
    ibis::part::mapValues(vals, hist);
    const size_t ncard = hist.size();
    array_t<uint32_t> ctmp;
    array_t<T> vtmp;
    ctmp.reserve(ncard);
    vtmp.reserve(ncard);
    for (typename std::map<T, uint32_t>::const_iterator it = hist.begin();
	 it != hist.end(); ++ it) {
	vtmp.push_back((*it).first);
	ctmp.push_back((*it).second);
    }
    hist.clear();

    array_t<uint32_t> hbnd(nbins);
    ibis::index::divideCounts(hbnd, ctmp);
    bounds.clear();
    bounds.reserve(hbnd.size()+1);
    bounds.push_back(vtmp[0]);
    for (size_t i = 0; i < hbnd.size() && hbnd[i] < ncard; ++ i)
	bounds.push_back(vtmp[hbnd[i]]);

    if (bounds.size() > 1) {
	T end1 = bounds.back() - bounds[bounds.size()-2];
	T end2 = vtmp.back() + end1;
	end1 += bounds.back();
	bounds.push_back(end1 > vtmp.back() ? end1 : end2);
    }
    else {
	bounds.push_back(vtmp.back()+1);
    }
} // ibis::part::equalWeightBins

template <typename T>
void ibis::part::mapValues(const array_t<T> &vals,
			   std::map<T, uint32_t> &hist) {
    for (size_t i = 0; i < vals.size(); ++ i) {
	typename std::map<T, uint32_t>::iterator it = hist.find(vals[i]);
	if (it != hist.end())
	    ++ (*it).second;
	else
	    hist.insert(std::make_pair(vals[i], 1));
    }
} // ibis::part::mapValues

/// Explicit specialization for float arrays.  Goes through the data twice,
/// once to find the actual min and max values, and once to place the
/// values in ten times as many bins as desired.  It then coalesces the
/// finer bins into desired number of bins.
template <>
void ibis::part::equalWeightBins(const array_t<float> &vals,
				 uint32_t nbins, array_t<float> &bounds) {
    float amax = vals[0];
    float amin = vals[0];
    // first compute the actual min and max
    for (unsigned i = 1; i < vals.size(); ++ i) {
	if (amax < vals[i]) amax = vals[i];
	if (amin > vals[i]) amin = vals[i];
    }
    if (amin >= amax) {  // a single value
	bounds.resize(2);
	bounds[0] = amin;
	bounds[1] = ibis::util::compactValue(amin, DBL_MAX);
	return;
    }
    if (nbins <= 1) nbins = 16;
    uint32_t nb2 = nbins * 10;
    const float stride = ibis::util::incrDouble((amax - amin) / nb2);
    array_t<uint32_t> cnts(nb2, 0U);
    for (unsigned i = 0; i < vals.size(); ++ i)
	++ cnts[(unsigned) ((vals[i]-amin)/stride)];

    array_t<uint32_t> hbnd(nbins);
    ibis::index::divideCounts(hbnd, cnts);
    bounds.clear();
    bounds.reserve(hbnd.size()+1);
    bounds.push_back(amin);
    for (size_t i = 0; i < hbnd.size() && hbnd[i] < nb2; ++ i)
	bounds.push_back(amin + stride *hbnd[i]);
    bounds.push_back(amin+stride*nb2);
} // ibis::part::equalWeightBins

/// Explicit specialization for double arrays.  Goes through the data
/// twice, once to find the actual min and max values, and once to place
/// the values in ten times as many bins as desired.  It then coalesces the
/// finer bins into desired number of bins.
template <>
void ibis::part::equalWeightBins(const array_t<double> &vals,
				 uint32_t nbins, array_t<double> &bounds) {
    double amax = vals[0];
    double amin = vals[0];
    // first compute the actual min and max
    for (unsigned i = 1; i < vals.size(); ++ i) {
	if (amax < vals[i]) amax = vals[i];
	if (amin > vals[i]) amin = vals[i];
    }
    if (amin >= amax) {  // a single value
	bounds.resize(2);
	bounds[0] = amin;
	bounds[1] = ibis::util::compactValue(amin, DBL_MAX);
	return;
    }
    if (nbins <= 1) nbins = 16;
    uint32_t nb2 = nbins * 10;
    const double stride = ibis::util::incrDouble((amax - amin) / nb2);
    array_t<uint32_t> cnts(nb2, 0U);
    for (unsigned i = 0; i < vals.size(); ++ i)
	++ cnts[(unsigned) ((vals[i]-amin)/stride)];

    array_t<uint32_t> hbnd(nbins);
    ibis::index::divideCounts(hbnd, cnts);
    bounds.clear();
    bounds.reserve(hbnd.size()+1);
    bounds.push_back(amin);
    for (size_t i = 0; i < hbnd.size() && hbnd[i] < nb2; ++ i)
	bounds.push_back(amin + stride *hbnd[i]);
    bounds.push_back(amin+stride*nb2);
} // ibis::part::equalWeightBins

///  The array @c bounds defines the following bins:
/// @code
/// (..., bounds[0]) [bounds[0], bounds[1]) ... [bounds.back(), ...).
/// @endcode
/// or alternatively,
/// @verbatim
/// bin 0: (..., bounds[0]) -> counts[0]
/// bin 1: [bounds[0], bounds[1]) -> counts[1]
/// bin 2: [bounds[1], bounds[2]) -> counts[2]
/// bin 3: [bounds[2], bounds[3]) -> counts[3]
/// ...
/// @endverbatim
/// In other word, @c bounds[n] defines (n+1) bins, with two open bins
/// at the two ends.  The array @c counts contains the number of rows
/// fall into each bin.  On a successful return from this function, the
/// return value of this function is the number of bins defined, which
/// is the same as the size of array @c counts but one larger than the
/// size of array @c bounds.
///
/// Return the number of bins (i.e., counts.size()) on success.
long
ibis::part::getDistribution(const char *name,
			    std::vector<double> &bounds,
			    std::vector<uint32_t> &counts) const {
    long ierr = -1;
    columnList::const_iterator it = columns.find(name);
    if (it != columns.end()) {
	ierr = (*it).second->getDistribution(bounds, counts);
	if (ierr < 0)
	    ierr -= 10;
    }
    return ierr;
} // ibis::part::getDistribution

/// Because most of the binning scheme leaves two bins for overflow, one
/// for values less than the expected minimum and one for values greater
/// than the expected maximum.  The minimum number of bins expected is
/// four (4).  This function will return error code -1 if the value of nbc
/// is less than 4.
long
ibis::part::getDistribution
(const char *name, uint32_t nbc, double *bounds, uint32_t *counts) const {
    if (nbc < 4) // work space too small
	return -1;
    std::vector<double> bds;
    std::vector<uint32_t> cts;
    long mbc = getDistribution(name, bds, cts);
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- getDistribution(" << name
		  << ") returned ierr=" << mbc << ", bds.size()="
		  << bds.size() << ", cts.size()=" << cts.size() << "\n";
	if (mbc > 0 && bds.size()+1 == cts.size() &&
	    static_cast<uint32_t>(mbc) == cts.size()) {
	    lg.buffer() << "(..., " << bds[0] << ")\t" << cts[0] << "\n";
	    for (int i = 1; i < mbc-1; ++ i) {
		lg.buffer() << "[" << bds[i-1] << ", "<< bds[i] << ")\t"
			  << cts[i] << "\n";
	    }
	    lg.buffer() << "[" << bds.back() << ", ...)\t" << cts.back()
		      << "\n";
	}
    }
#endif
    mbc = packDistribution(bds, cts, nbc, bounds, counts);
    return mbc;
} // ibis::part::getDistribution

/// Compute the distribution of the named variable under the specified
/// constraints.  If the input array @c bounds contains distinct values in
/// ascending order, the array will be used as bin boundaries.  Otherwise,
/// the bin boundaries are automatically determined by this function.  The
/// basic rule for determining the number of bins is that if there are less
/// than 10,000 distinct values, than every value is counted separatly,
/// otherwise 1000 bins will be used and each bin will contain roughly the
/// same number of records.
///
/// @note Deprecated.
long
ibis::part::getDistribution(const char *constraints,
			    const char *name,
			    std::vector<double> &bounds,
			    std::vector<uint32_t> &counts) const {
    long ierr = -1;
    columnList::const_iterator it = columns.find(name);
    if (it == columns.end())
	return ierr;

    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::getDistribution attempting to compute a histogram of "
	    << name << (constraints && *constraints ? " subject to " :
			" without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }
    if (constraints == 0 || *constraints == 0 || *constraints == '*') {
	ierr = (*it).second->getDistribution(bounds, counts);
	if (ierr > 0 && ibis::gVerbose > 0) {
	    timer.stop();
	    logMessage("getDistribution",
		       "computing the distribution of column %s took %g "
		       "sec(CPU), %g sec(elapsed)",
		       (*it).first, timer.CPUTime(), timer.realTime());
	}
	return ierr;
    }

    ibis::bitvector mask;
    mask.set(1, nEvents);
    const ibis::column *col = (*it).second;
    if (constraints != 0 && *constraints != 0) {
	ibis::query q(ibis::util::userName(), this);
	q.setWhereClause(constraints);
	ierr = q.evaluate();
	if (ierr < 0) {
	    ierr = -2;
	    return ierr;
	}

	mask.copy(*(q.getHitVector()));
	if (mask.cnt() == 0) {
	    if (ibis::gVerbose > 2)
		logMessage("getDistribution", "no record satisfied the "
			   "user specified constraints \"%s\"", constraints);
	    return 0;
	}
    }
    bool usebnds = ! bounds.empty();
    for (uint32_t i = 1; usebnds && i < bounds.size(); ++ i)
	usebnds = (bounds[i] > bounds[i-1]);

    if (usebnds) { // use the input bin boundaries
	switch ((*it).second->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *vals = col->selectInts(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    array_t<int32_t> bnds(bounds.size());
	    for (uint32_t i = 0; i < bounds.size(); ++ i)
		bnds[i] = static_cast<int32_t>(bounds[i]);
	    ibis::index::mapValues<int32_t>(*vals, bnds, counts);
	    delete vals;
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *vals = col->selectUInts(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    array_t<uint32_t> bnds(bounds.size());
	    for (uint32_t i = 0; i < bounds.size(); ++ i)
		bnds[i] = static_cast<uint32_t>(bounds[i]);
	    ibis::index::mapValues<uint32_t>(*vals, bnds, counts);
	    delete vals;
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *vals = col->selectFloats(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    array_t<float> bnds(bounds.size());
	    for (uint32_t i = 0; i < bounds.size(); ++ i)
		bnds[i] = static_cast<float>(bounds[i]);
	    ibis::index::mapValues<float>(*vals, bnds, counts);
	    delete vals;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *vals = col->selectDoubles(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    array_t<double> bnds(bounds.size());
	    for (uint32_t i = 0; i < bounds.size(); ++ i)
		bnds[i] = bounds[i];
	    ibis::index::mapValues<double>(*vals, bnds, counts);
	    delete vals;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("getDistribution",
		       "unable to handle column type %d",
		       static_cast<int>((*it).second->type()));
	    break;}
	}
    }
    else { // need to determine bin boundaries in this function
	ibis::index::histogram hist;
	bounds.clear();
	counts.clear();
	switch ((*it).second->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *vals = col->selectInts(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    ibis::index::mapValues<int32_t>(*vals, hist);
	    delete vals;
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *vals = col->selectUInts(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    ibis::index::mapValues<uint32_t>(*vals, hist);
	    delete vals;
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *vals = col->selectFloats(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    ibis::index::mapValues<float>(*vals, hist);
	    delete vals;
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> *vals = col->selectDoubles(mask);
	    if (vals == 0) {
		ierr = -4;
		break;
	    }
	    ibis::index::mapValues<double>(*vals, hist);
	    delete vals;
	    break;}
	default: {
	    ierr = -3;
	    logWarning("getDistribution",
		       "unable to handle column type %d",
		       static_cast<int>((*it).second->type()));
	    break;}
	}

	if (hist.size() == 1) { // special case of a single value
	    ibis::index::histogram::const_iterator it1 =
		hist.begin();
	    bounds.resize(2);
	    counts.resize(3);
	    bounds[0] = (*it1).first;
	    bounds[1] = ((*it1).first + 1.0);
	    counts[0] = 0;
	    counts[1] = (*it1).second;
	    counts[2] = 0;
	}
	else if (hist.size() < 10000) {
	    // convert the histogram into two arrays
	    bounds.reserve(mask.cnt());
	    counts.reserve(mask.cnt()+1);
	    ibis::index::histogram::const_iterator it1 =
		hist.begin();
	    counts.push_back((*it1).second);
	    for (++ it1; it1 != hist.end(); ++ it1) {
		bounds.push_back((*it1).first);
		counts.push_back((*it1).second);
	    }
	}
	else if (hist.size() > 0) { // too many values, reduce to 1000 bins
	    array_t<double> vals(hist.size());
	    array_t<uint32_t> cnts(hist.size());
	    vals.clear();
	    cnts.clear();
	    for (ibis::index::histogram::const_iterator it1 =
		     hist.begin();
		 it1 != hist.end(); ++ it1) {
		vals.push_back((*it1).first);
		cnts.push_back((*it1).second);
	    }
	    array_t<uint32_t> dvd(1000);
	    ibis::index::divideCounts(dvd, cnts);
	    for (uint32_t i = 0; i < dvd.size(); ++ i) {
		uint32_t cnt = 0;
		for (uint32_t j = (i>0?dvd[i-1]:0); j < dvd[i]; ++ j)
		    cnt += cnts[j];
		counts.push_back(cnt);
		if (i > 0) {
		    double bd;
		    if (dvd[i] < vals.size())
			bd = ibis::util::compactValue
			    (vals[dvd[i]-1], vals[dvd[i]]);
		    else
			bd = ibis::util::compactValue
			    (vals.back(), DBL_MAX);
		    bounds.push_back(bd);
		}
	    }
	}
    }
    if (ierr >= 0)
	ierr = counts.size();
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("getDistribution",
		   "computing the distribution of "
		   "column %s with restriction \"%s\" took %g "
		   "sec(CPU), %g sec(elapsed)", (*it).first,
		   constraints, timer.CPUTime(), timer.realTime());
    }
    if (ierr < 0)
	ierr -= 10;
    return ierr;
} // ibis::part::getDistribution

/// Because most of the binning scheme leaves two bins for overflow, one
/// for values less than the expected minimum and one for values greater
/// than the expected maximum.  The minimum number of bins expected is
/// four (4).  This function will return error code -1 if the value of nbc
/// is less than 4.
///
/// @note Deprecated.
long
ibis::part::getDistribution
(const char *constraints, const char *name, uint32_t nbc,
 double *bounds, uint32_t *counts) const {
    if (nbc < 4) // work space too small
	return -1;

    std::vector<double> bds;
    std::vector<uint32_t> cts;
    bool useinput = true;
    for (uint32_t i = 1; i < nbc && useinput; ++ i)
	useinput = (bounds[i] > bounds[i-1]);
    if (useinput) {
	bds.resize(nbc);
	for (uint32_t i = 0; i < nbc; ++i)
	    bds[i] = bounds[i];
    }
    long mbc = getDistribution(constraints, name, bds, cts);
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- getDistribution(" << name << ", "
		    << constraints << ") returned ierr=" << mbc
		    << ", bds.size()=" << bds.size() << ", cts.size()="
		    << cts.size() << "\n";
	if (mbc > 0 && bds.size()+1 == cts.size() &&
	    static_cast<uint32_t>(mbc) == cts.size()) {
	    lg.buffer() << "(..., " << bds[0] << ")\t" << cts[0] << "\n";
	    for (int i = 1; i < mbc-1; ++ i) {
		lg.buffer() << "[" << bds[i-1] << ", "<< bds[i] << ")\t"
			  << cts[i] << "\n";
	    }
	    lg.buffer() << "[" << bds.back() << ", ...)\t" << cts.back()
		      << "\n";
	}
    }
#endif
    mbc = packDistribution(bds, cts, nbc, bounds, counts);
    return mbc;
} // ibis::part::getDistribution

/// It returns the number of entries in arrays @c bounds and @c counts.
/// The content of @c counts[i] will be the number of records in the named
/// column that are less than @c bounds[i].  The last element in array @c
/// bounds is larger than returned by function getColumnMax.
///
/// @note Deprecated.
long
ibis::part::getCumulativeDistribution(const char *name,
				      std::vector<double> &bounds,
				      std::vector<uint32_t> &counts) const {
    long ierr = -1;
    columnList::const_iterator it = columns.find(name);
    if (it != columns.end()) {
	ierr = (*it).second->getCumulativeDistribution(bounds, counts);
	if (ierr < 0)
	    ierr -= 10;
    }
    return ierr;
} // ibis::part::getCumulativeDistribution

/// The actual number of elements filled by this function is the return
/// value, which is guaranteed to be no larger than the input value of @c
/// nbc.
///
/// @note Because most of the binning scheme leaves two bins for overflow,
/// one for values less than the expected minimum and one for values
/// greater than the expected maximum.  The minimum number of bins expected
/// is four (4).  This function will return error code -1 if the value of
/// nbc is less than 4.
///
/// @note Deprecated.
long
ibis::part::getCumulativeDistribution
(const char *name, uint32_t nbc, double *bounds, uint32_t *counts) const {
    if (nbc < 4) // work space too small
	return -1;
    std::vector<double> bds;
    std::vector<uint32_t> cts;
    long mbc = getCumulativeDistribution(name, bds, cts);
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg;
	lg.buffer() << "DEBUG -- getCumulativeDistribution(" << name
		  << ") returned ierr=" << mbc << "\n";
	if (mbc > 0)
	    lg.buffer() << "histogram\n(bound,\tcount)\n";
	for (int i = 0; i < mbc; ++ i) {
	    lg.buffer() << bds[i] << ",\t" << cts[i] << "\n";
	}
    }
#endif
    mbc = packCumulativeDistribution(bds, cts, nbc, bounds, counts);
    return mbc;
} // ibis::part::getCumulativeDistribution

/// @note  The constraints have the same syntax as the where-clause of
/// the queries.  Here are two examples, "a < 5 and 3.5 >= b >= 1.9" and
/// "a * a + b * b > 55 and sqrt(c) > 2."
/// @note This function does not accept user input bin boundaries.
///
/// @note Deprecated.
long
ibis::part::getCumulativeDistribution(const char *constraints,
				      const char *name,
				      std::vector<double> &bounds,
				      std::vector<uint32_t> &counts) const {
    long ierr = -1;
    columnList::const_iterator it = columns.find(name);
    if (it == columns.end())
	return ierr;

    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::getCumulativeDistribution attempting to compute the "
	    "cummulative distribution of "
	    << name << (constraints && *constraints ? " subject to " :
			" without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }
    if (constraints == 0 || *constraints == 0 || *constraints == '*') {
	ierr = (*it).second->getCumulativeDistribution(bounds, counts);
	if (ierr > 0 && ibis::gVerbose > 0) {
	    timer.stop();
	    logMessage("getCumulativeDistribution",
		       "computing the distribution of column %s took %g "
		       "sec(CPU), %g sec(elapsed)",
		       (*it).first, timer.CPUTime(), timer.realTime());
	}
    }
    else {
	ibis::bitvector hits;
	const ibis::column *col = (*it).second;
	{
	    ibis::query q(ibis::util::userName(), this);
	    q.setWhereClause(constraints);
	    ierr = q.evaluate();
	    if (ierr < 0)
		return ierr;
	    hits.copy(*(q.getHitVector()));
	    if (hits.cnt() == 0)
		return 0;
	}
	ibis::index::histogram hist;
	bounds.clear();
	counts.clear();
	if (hits.cnt() > 0) {
	    switch ((*it).second->type()) {
	    case ibis::SHORT:
	    case ibis::BYTE:
	    case ibis::INT: {
		array_t<int32_t> *vals = col->selectInts(hits);
		if (vals == 0) {
		    ierr = -4;
		    break;
		}
		ibis::index::mapValues<int32_t>(*vals, hist);
		delete vals;
		break;}
	    case ibis::USHORT:
	    case ibis::UBYTE:
	    case ibis::UINT:
	    case ibis::CATEGORY: {
		array_t<uint32_t> *vals = col->selectUInts(hits);
		if (vals == 0) {
		    ierr = -4;
		    break;
		}
		ibis::index::mapValues<uint32_t>(*vals, hist);
		delete vals;
		break;}
	    case ibis::FLOAT: {
		array_t<float> *vals = col->selectFloats(hits);
		if (vals == 0) {
		    ierr = -4;
		    break;
		}
		ibis::index::mapValues<float>(*vals, hist);
		delete vals;
		break;}
	    case ibis::DOUBLE: {
		array_t<double> *vals = col->selectDoubles(hits);
		if (vals == 0) {
		    ierr = -4;
		    break;
		}
		ibis::index::mapValues<double>(*vals, hist);
		delete vals;
		break;}
	    default: {
		ierr = -3;
		logWarning("getCumulativeDistribution",
			   "unable to handle column type %d",
			   static_cast<int>((*it).second->type()));
		break;}
	    }

	    if (hist.empty()) {
		if (ierr >= 0)
		    ierr = -7;
	    }
	    else if (hist.size() < 10000) {
		// convert the histogram into cumulative distribution
		bounds.reserve(hits.cnt()+1);
		counts.reserve(hits.cnt()+1);
		counts.push_back(0);
		for (ibis::index::histogram::const_iterator hit =
			 hist.begin();
		     hit != hist.end(); ++ hit) {
		    bounds.push_back((*hit).first);
		    counts.push_back((*hit).second + counts.back());
		}
		bounds.push_back(ibis::util::compactValue
				 (bounds.back(), DBL_MAX));
	    }
	    else { // too many values, reduce to 1000 bins
		array_t<double> vals(hist.size());
		array_t<uint32_t> cnts(hist.size());
		vals.clear();
		cnts.clear();
		for (ibis::index::histogram::const_iterator hit =
			 hist.begin();
		     hit != hist.end(); ++ hit) {
		    vals.push_back((*hit).first);
		    cnts.push_back((*hit).second);
		}
		array_t<uint32_t> dvd(1000);
		ibis::index::divideCounts(dvd, cnts);
		bounds.push_back(vals[0]);
		counts.push_back(0);
		for (uint32_t i = 0; i < dvd.size(); ++ i) {
		    uint32_t cnt = counts.back();
		    for (uint32_t j = (i>0?dvd[i-1]:0); j < dvd[i]; ++ j)
			cnt += cnts[j];
		    counts.push_back(cnt);
		    double bd;
		    if (dvd[i] < vals.size())
			bd = ibis::util::compactValue(vals[dvd[i]-1],
						      vals[dvd[i]]);
		    else
			bd = ibis::util::compactValue
			    (vals.back(), DBL_MAX);
		    bounds.push_back(bd);
		}
	    }
	}
	if (ierr >= 0)
	    ierr = counts.size();
	if (ierr > 0 && ibis::gVerbose > 0) {
	    timer.stop();
	    logMessage("getCumulativeDistribution",
		       "computing the distribution of "
		       "column %s with restriction \"%s\" took %g "
		       "sec(CPU), %g sec(elapsed)", (*it).first,
		       constraints, timer.CPUTime(), timer.realTime());
	}
    }
    if (ierr < 0)
	ierr -= 10;

    return ierr;
} // ibis::part::getCumulativeDistribution

/// Because most of the binning scheme leaves two bins for overflow, one
/// for values less than the expected minimum and one for values greater
/// than the expected maximum.  The minimum number of bins expected is
/// four (4).  This function will return error code -1 if the value of nbc
/// is less than 4.
long
ibis::part::getCumulativeDistribution
(const char *constraints, const char *name, uint32_t nbc,
 double *bounds, uint32_t *counts) const {
    if (nbc < 4) // work space too small
	return -1;

    std::vector<double> bds;
    std::vector<uint32_t> cts;
    long mbc = getCumulativeDistribution(constraints, name, bds, cts);
    mbc = packCumulativeDistribution(bds, cts, nbc, bounds, counts);
    return mbc;
} // ibis::part::getCumulativeDistribution

/// It returns three arrays, @c bounds1, @c bounds2, and @c counts.  The
/// arrays @c bounds1 and@c bounds2 defines two sets of bins one for each
/// variable.  Together they define
/// @code (bounds1.size()+1) (bounds2.size()+1) @endcode
/// bins for the 2-D joint distributions.
///
/// On successful completion of this function, it return the number of
/// bins.
///
/// @note The arrays @c bounds1 and @c bounds2 are used if they contain
/// values in ascending order.  If they are empty or their values are not
/// in ascending order, then a simple linear binning will be used.  By
/// default, no more than 256 bins are used for each variable.
///
/// @note Deprecated.
long
ibis::part::getJointDistribution(const char *constraints,
				 const char *name1, const char *name2,
				 std::vector<double> &bounds1,
				 std::vector<double> &bounds2,
				 std::vector<uint32_t> &counts) const {
    long ierr = -1;
    columnList::const_iterator it1 = columns.find(name1);
    columnList::const_iterator it2 = columns.find(name2);
    if (it1 == columns.end() || it2 == columns.end()) {
	if (it1 == columns.end())
	    logWarning("getJointDistribution", "%s is not a known column name",
		       name1);
	if (it2 == columns.end())
	    logWarning("getJointDistribution", "%s is not a known column name",
		       name2);
	return ierr;
    }

    const ibis::column *col1 = (*it1).second;
    const ibis::column *col2 = (*it2).second;
    ibis::horometer timer;
    if (ibis::gVerbose > 0) {
	LOGGER(ibis::gVerbose >= 3)
	    << "ibis::part[" << (m_name ? m_name : "")
	    << "]::getJointDistribution attempting to compute a histogram of "
	    << name1 << " and " << name2
	    << (constraints && *constraints ? " subject to " :
		" without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }
    ibis::bitvector mask;
    if (constraints != 0 && *constraints != 0) {
	ibis::query q(ibis::util::userName(), this);
	q.setWhereClause(constraints);
	ierr = q.evaluate();
	if (ierr < 0)
	    return ierr;
	const ibis::bitvector *hits = q.getHitVector();
	if (hits->cnt() == 0) // nothing to do any more
	    return 0;
	mask.copy(*hits);
    }
    else {
	col1->getNullMask(mask);
	ibis::bitvector tmp;
	col2->getNullMask(tmp);
	mask &= tmp;
    }

    counts.clear();
    switch (col1->type()) {
    case ibis::SHORT:
    case ibis::BYTE:
    case ibis::INT: {
	array_t<int32_t> *val1 = col1->selectInts(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}
	array_t<int32_t> bnd1;
	if (bounds1.size() > 0) {
	    bnd1.resize(bounds1.size());
	    for (uint32_t i = 0; i < bounds1.size(); ++ i)
		bnd1[i] = static_cast<int32_t>(bounds1[i]);
	}
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<int32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<uint32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<float>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<double>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("getJointDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::USHORT:
    case ibis::UBYTE:
    case ibis::UINT:
    case ibis::CATEGORY: {
	array_t<uint32_t> *val1 = col1->selectUInts(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<uint32_t> bnd1;
	if (bounds1.size() > 0) {
	    bnd1.resize(bounds1.size());
	    for (unsigned i = 0; i < bounds1.size(); ++ i)
		bnd1[i] = static_cast<uint32_t>(bounds1[i]);
	}
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<int32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<uint32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<float>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<double>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("getJointDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::FLOAT: {
	array_t<float> *val1 = col1->selectFloats(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<float> bnd1;
	if (bounds1.size() > 0) {
	    bnd1.resize(bounds1.size());
	    for (uint32_t i = 0; i < bounds1.size(); ++ i)
		bnd1[i] = static_cast<float>(bounds1[i]);
	}
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<int32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<uint32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<float>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<double>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("getJointDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    case ibis::DOUBLE: {
	array_t<double> *val1 = col1->selectDoubles(mask);
	if (val1 == 0) {
	    ierr = -4;
	    break;
	}

	array_t<double> bnd1;
	if (bounds1.size() > 0) {
	    bnd1.resize(bounds1.size());
	    for (uint32_t i = 0; i < bounds1.size(); ++ i)
		bnd1[i] = bounds1[i];
	}
	switch (col2->type()) {
	case ibis::SHORT:
	case ibis::BYTE:
	case ibis::INT: {
	    array_t<int32_t> *val2 = col2->selectInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<int32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<int32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::USHORT:
	case ibis::UBYTE:
	case ibis::UINT:
	case ibis::CATEGORY: {
	    array_t<uint32_t> *val2 = col2->selectUInts(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<uint32_t> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<uint32_t>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::FLOAT: {
	    array_t<float> *val2 = col2->selectFloats(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    array_t<float> bnd2;
	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<float>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	case ibis::DOUBLE: {
	    array_t<double> bnd2;
	    array_t<double> *val2 = col2->selectDoubles(mask);
	    if (val2 == 0) {
		ierr = -5;
		break;
	    }

	    if (bounds2.size() > 0) {
		bnd2.resize(bounds2.size());
		for (uint32_t i = 0; i < bounds2.size(); ++ i)
		    bnd2[i] = static_cast<double>(bounds2[i]);
	    }
	    ibis::index::mapValues(*val1, *val2, bnd1, bnd2, counts);
	    delete val2;
	    bounds1.resize(bnd1.size());
	    for (uint32_t i = 0; i < bnd1.size(); ++ i)
		bounds1[i] = bnd1[i];
	    bounds2.resize(bnd2.size());
	    for (uint32_t i = 0; i < bnd2.size(); ++ i)
		bounds2[i] = bnd2[i];
	    break;}
	default: {
	    ierr = -3;
	    logWarning("getJointDistribution",
		       "unable to handle column type %d",
		       static_cast<int>(col2->type()));
	    break;}
	}
	delete val1;
	break;}
    default: {
	ierr = -3;
	logWarning("getJointDistribution",
		   "unable to handle column type %d",
		   static_cast<int>(col1->type()));
	break;}
    }

    if ((bounds1.size()+1) * (bounds2.size()+1) == counts.size())
	ierr = counts.size();
    else
	ierr = -2;
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("getJointDistribution",
		   "computing the joint distribution of "
		   "column %s and %s%s%s took %g "
		   "sec(CPU), %g sec(elapsed)",
		   (*it1).first, (*it2).first,
		   (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::getJointDistribution

long ibis::part::packDistribution
(const std::vector<double> &bds, const std::vector<uint32_t> &cts,
 uint32_t nbc, double *bptr, uint32_t *cptr) const {
    uint32_t mbc = bds.size();
    if (mbc <= 0)
	return mbc;
    if (static_cast<uint32_t>(mbc+1) != cts.size()) {
	ibis::util::logMessage
	    ("Warning", "packDistribution expects the size "
	     "of bds[%lu] to be the one less than that of "
	     "cts[%lu], but it is not",
	     static_cast<long unsigned>(bds.size()),
	     static_cast<long unsigned>(cts.size()));
	return -1;
    }
    if (nbc < 2) {
	ibis::util::logMessage
	    ("Warning", "a binned distribution needs "
	     "two arrays of size at least 2, caller has "
	     "provided two arrays of size %lu",
	     static_cast<long unsigned>(nbc));
	return -2;
    }
    if (static_cast<uint32_t>(mbc) <= nbc) {
	// copy the values in bds and cts
	for (uint32_t i = 0; i < mbc; ++ i) {
	    bptr[i] = bds[i];
	    cptr[i] = cts[i];
	}
	cptr[mbc] = cts[mbc];
	++ mbc;
    }
    else { // make the distribution fit the given space
	// the first entry is always copied
	bptr[0] = bds[0];
	cptr[0] = cts[0];

	uint32_t top = 0; // the total number of entries to be redistributed
	uint32_t cnt = 0; // entries already redistributed
	for (uint32_t k = 1; k < mbc; ++ k)
	    top += cts[k];
	uint32_t i = 1; // index to output bins
	uint32_t j = 1; // index to input bins
	while (i < nbc-1 && nbc+j < mbc+i) {
	    uint32_t next = j + 1;
	    uint32_t tgt = (top - cnt) / (nbc-i-1);
	    bptr[i] = bds[j];
	    cptr[i] = cts[j];
	    while (cptr[i] < tgt && nbc+next <= mbc+i) {
		cptr[i] += cts[next];
		++ next;
	    }
#if defined(DEBUG) && DEBUG + 0 > 1
	    LOGGER(ibis::gVerbose >= 0)
		<< "DEBUG -- i=" << i << ", j = " << j << ", bds[j]="
		<< bds[j] << ", next=" << next << ", bds[next]="
		<< bds[next] << ", cts[next]=" << cts[next];
#endif
	    j = next;
	    ++ i;
	}
	++ j;
	if (mbc - j > nbc - i)
	    j = 1 + mbc - nbc + i;
	// copy the last few bins
	while (i < nbc && j < static_cast<uint32_t>(mbc)) {
	    bptr[i] = bds[j];
	    cptr[i] = cts[j];
	    ++ i;
	    ++ j;
	}
	if (j == static_cast<uint32_t>(mbc) && i < nbc) {
	    cptr[i] = cts[mbc];
	    mbc = i + 1;
	}
	else {
	    mbc = i;
	}
    }
    return mbc;
} // ibis::part::packDistribution

long ibis::part::packCumulativeDistribution
(const std::vector<double> &bds, const std::vector<uint32_t> &cts,
 uint32_t nbc, double *bptr, uint32_t *cptr) const {
    long mbc = bds.size();
    if (mbc <= 0)
	return mbc;
    if (static_cast<uint32_t>(mbc) != cts.size()) {
	ibis::util::logMessage
	    ("Warning", "packCumulativeDistribution expects "
	     "the size of bds[%lu] to be the same as that "
	     "of cts[%lu], but they are not",
	     static_cast<long unsigned>(bds.size()),
	     static_cast<long unsigned>(cts.size()));
	return -1;
    }
    if (nbc < 2) {
	ibis::util::logMessage
	    ("Warning", "a cumulative distribution needs "
	     "two arrays of size at least 2, caller has "
	     "provided two arrays of size %lu",
	     static_cast<long unsigned>(nbc));
	return -2;
    }
    if (static_cast<uint32_t>(mbc) <= nbc) {
	// copy the values in bds and cts
	for (int i = 0; i < mbc; ++ i) {
	    bptr[i] = bds[i];
	    cptr[i] = cts[i];
	}
    }
//     else if (static_cast<uint32_t>(mbc) <= nbc+nbc-3) {
// 	// less than two values in a bin on average
// 	uint32_t start = nbc + nbc - mbc - 2;
// 	for (uint32_t i = 0; i <= start; ++ i) {
// 	    bptr[i] = bds[i];
// 	    cptr[i] = cts[i];
// 	}
// 	for (uint32_t i = start+1; i < nbc-1; ++ i) {
// 	    uint32_t j = i + i - start;
// 	    bptr[i] = bds[j];
// 	    cptr[i] = cts[j];
// 	}
// 	bptr[nbc-1] = bds[mbc-1];
// 	cptr[nbc-1] = cts[mbc-1];
// 	mbc = nbc; // mbc is the return value
//     }
    else { // make the distribution fit the given space
	// the first entries are always copied
	bptr[0] = bds[0];
	cptr[0] = cts[0];
	bptr[1] = bds[1];
	cptr[1] = cts[1];

	uint32_t top = cts[mbc-2];
	uint32_t i = 2, j = 1;
	while (i < nbc-1 && nbc+j < mbc+i-1) {
	    uint32_t next = j + 1;
	    uint32_t tgt = cts[j] + (top - cts[j]) / (nbc-i-1);
	    while (cts[next] < tgt && nbc+next <= mbc+i-1)
		++ next;
#if defined(DEBUG) && DEBUG + 0 > 1
	    LOGGER(ibis::gVerbose >= 0)
		<< "DEBUG -- i=" << i << ", next=" << next << ", bds[next]="
		<< bds[next] << ", cts[next]=" << cts[next];
#endif
	    bptr[i] = bds[next];
	    cptr[i] = cts[next];
	    j = next;
	    ++ i;
	}
	++ j;
	if (mbc - j > nbc - i)
	    j = mbc - nbc + i;
	while (i < nbc && j < static_cast<uint32_t>(mbc)) {
	    bptr[i] = bds[j];
	    cptr[i] = cts[j];
	    ++ i;
	    ++ j;
	}
	mbc = i;
    }
    return mbc;
} // ibis::part::packCumulativeDistribution

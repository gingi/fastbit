// File $Id$
// Author: John Wu <John.Wu at ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2000-2009 the Regents of the University of California
//
// Implements ibis::part 3D histogram functions.
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
    LOGGER(ibis::gVerbose > 5)
	<< "ibis::part::count3DBins<" << typeid(T1).name() << ", "
	<< typeid(T2).name() << ", " << typeid(T3).name() << ">("
	<< "vals1[" << vals1.size() << "], " << begin1 << ", "
	<< end1 << ", " << stride1
	<< ", vals2[" << vals2.size() << "], " << begin2 << ", "
	<< end2 << ", " << stride2
	<< ", vals3[" << vals3.size() << "], " << begin3 << ", "
	<< end3 << ", " << stride3 << ", counts[" << counts.size()
	<< "]) ... ("
	<< 1 + static_cast<size_t>(std::floor((end1-begin1)/stride1))
	<< ", "
	<< 1 + static_cast<size_t>(std::floor((end2-begin2)/stride2))
	<< ", "
	<< 1 + static_cast<size_t>(std::floor((end3-begin3)/stride3))
	<< ")";
    const size_t dim3 = 1 +
	static_cast<size_t>(std::floor((end3 - begin3)/stride3));
    const size_t dim2 = 1 +
	static_cast<size_t>(std::floor((end2 - begin2)/stride2));
    const size_t nr = (vals1.size() <= vals2.size() ?
		       (vals1.size() <= vals3.size() ?
			vals1.size() : vals3.size()) :
		       (vals2.size() <= vals3.size() ?
			vals2.size() : vals3.size()));
    for (size_t ir = 0; ir < nr; ++ ir) {
	const size_t pos =
	    (static_cast<size_t>((vals1[ir]-begin1)/stride1) * dim2 +
	     static_cast<size_t>((vals2[ir]-begin2)/stride2)) * dim3 +
	    static_cast<size_t>((vals3[ir]-begin3)/stride3);
	++ counts[pos];
#if (defined(_DEBUG) && _DEBUG+0 > 1) || (defined(DEBUG) && DEBUG+0 > 1)
	LOGGER(ibis::gVerbose > 5)
	    << "DEBUG: count3DBins -- vals1[" << ir << "]=" << vals1[ir]
	    << ", vals2[" << ir << "]=" << vals2[ir]
	    << ", vals3[" << ir << "]=" << vals3[ir]
	    << " --> bin (" << static_cast<uint32_t>((vals1[ir]-begin1)/stride1)
	    << ", " << static_cast<uint32_t>((vals2[ir]-begin2)/stride2)
	    << ", " << static_cast<uint32_t>((vals3[ir]-begin3)/stride3)
	    << ") counts[" << pos << "]=" << counts[pos];
#endif
    }
    return counts.size();
} // ibis::part::count3DBins

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
	    << (constraints && *constraints ? "subject to " :
		"without constraints")
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
	ierr = qq.getNumHits();
	if (ierr <= 0)
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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
				   *vals3, begin3, end3, stride3, counts);
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

/// The three triplets, (begin1, end1, stride1), (begin2, end2, stride2),
/// and (begin3, end3, stride3), defines <tt> (1 + floor((end1 - begin1) /
/// stride1)) (1 + floor((end2 - begin2) / stride2)) (1 + floor((end3 -
/// begin3) / stride3)) </tt> 3D bins.  The 3D bins are packed into the 1D
/// array bins in raster scan order, with the 3rd dimension as the fastest
/// varying dimension and the 1st dimension as the slowest varying dimension.
///
/// @note All bitmaps that are empty are left with size() = 0.  All other
/// bitmaps have the same size() as mask.size().  When use these returned
/// bitmaps, please make sure to NOT mix empty bitmaps with non-empty
/// bitmaps in bitwise logical operations!
///
/// @sa ibis::part::fill1DBins, ibis::part::fill2DBins.
template <typename T1, typename T2, typename T3>
long ibis::part::fill3DBins(const ibis::bitvector &mask,
			    const array_t<T1> &vals1,
			    const double &begin1, const double &end1,
			    const double &stride1,
			    const array_t<T2> &vals2,
			    const double &begin2, const double &end2,
			    const double &stride2,
			    const array_t<T3> &vals3,
			    const double &begin3, const double &end3,
			    const double &stride3,
			    std::vector<bitvector> &bins) const {
    if ((end1-begin1) * (end2-begin2) * (end3-begin3) >
	1e9 * stride1 * stride2 * stride3 ||
	(end1-begin1) * stride1 < 0.0 || (end2-begin2) * stride2 < 0.0 ||
	(end3-begin3) * stride3 < 0.0)
	return -10L;
    LOGGER(ibis::gVerbose > 5)
	<< "ibis::part::fill3DBins<" << typeid(T1).name() << ", "
	<< typeid(T2).name() << ", " << typeid(T3).name() << ">("
	<< "vals1[" << vals1.size() << "], " << begin1 << ", "
	<< end1 << ", " << stride1
	<< ", vals2[" << vals2.size() << "], " << begin2 << ", "
	<< end2 << ", " << stride2
	<< ", vals3[" << vals3.size() << "], " << begin3 << ", "
	<< end3 << ", " << stride3 << ", bins[" << bins.size()
	<< "]) ... ("
	<< 1 + static_cast<size_t>(std::floor((end1-begin1)/stride1))
	<< ", "
	<< 1 + static_cast<size_t>(std::floor((end2-begin2)/stride2))
	<< ", "
	<< 1 + static_cast<size_t>(std::floor((end3-begin3)/stride3))
	<< ")";
    const size_t nbin3 = (1 + static_cast<size_t>((end3-begin3)/stride3));
    const size_t nbin23 = (1 + static_cast<size_t>((end2-begin2)/stride2)) *
	nbin3;
    const size_t nbins = (1 + static_cast<size_t>((end1-begin1)/stride1)) *
	nbin23;
    size_t nvals = (vals1.size() <= vals2.size() ?
		    (vals1.size() <= vals3.size() ?
		     vals1.size() : vals3.size()) :
		    (vals2.size() <= vals3.size() ?
		     vals2.size() : vals3.size()));
    if (mask.size() == nvals) {
	bins.resize(nbins);
	for (ibis::bitvector::indexSet is = mask.firstIndexSet();
	     is.nIndices() > 0; ++ is) {
	    const ibis::bitvector::word_t *idx = is.indices();
	    if (is.isRange()) {
		for (uint32_t j = *idx; j < idx[1]; ++ j) {
		    const size_t ibin1 =
			static_cast<size_t>((vals1[j]-begin1)/stride1);
		    const size_t ibin2 =
			static_cast<size_t>((vals2[j]-begin2)/stride2);
		    const size_t ibin3 =
			static_cast<size_t>((vals3[j]-begin3)/stride3);
		    bins[ibin1*nbin23+ibin2*nbin3+ibin3].setBit(j, 1);
		}
	    }
	    else {
		for (uint32_t k = 0; k < is.nIndices(); ++ k) {
		    const ibis::bitvector::word_t j = idx[k];
		    const size_t ibin1 =
			static_cast<size_t>((vals1[j]-begin1)/stride1);
		    const size_t ibin2 =
			static_cast<size_t>((vals2[j]-begin2)/stride2);
		    const size_t ibin3 =
			static_cast<size_t>((vals3[j]-begin3)/stride3);
		    bins[ibin1*nbin23+ibin2*nbin3+ibin3].setBit(j, 1);
		}
	    }
	}
	for (size_t i = 0; i < nbins; ++ i)
	    if (bins[i].size() > 0)
		bins[i].adjustSize(0, mask.size());
    }
    else if (mask.cnt() == nvals) {
	bins.resize(nbins);
	size_t ivals = 0;
	for (ibis::bitvector::indexSet is = mask.firstIndexSet();
	     is.nIndices() > 0; ++ is) {
	    const ibis::bitvector::word_t *idx = is.indices();
	    if (is.isRange()) {
		for (uint32_t j = *idx; j < idx[1]; ++j, ++ ivals) {
		    const size_t ibin1 =
			static_cast<size_t>((vals1[ivals]-begin1)/stride1);
		    const size_t ibin2 =
			static_cast<size_t>((vals2[ivals]-begin2)/stride2);
		    const size_t ibin3 =
			static_cast<size_t>((vals3[ivals]-begin3)/stride3);
		    bins[ibin1*nbin23+ibin2*nbin3+ibin3].setBit(j, 1);
		}
	    }
	    else {
		for (uint32_t k = 0; k < is.nIndices(); ++ k, ++ ivals) {
		    const ibis::bitvector::word_t j = idx[k];
		    const size_t ibin1 =
			static_cast<size_t>((vals1[ivals]-begin1)/stride1);
		    const size_t ibin2 =
			static_cast<size_t>((vals2[ivals]-begin2)/stride2);
		    const size_t ibin3 =
			static_cast<size_t>((vals3[ivals]-begin3)/stride3);
		    bins[ibin1*nbin23+ibin2*nbin3+ibin3].setBit(j, 1);
#if (defined(_DEBUG) && _DEBUG+0 > 1) || (defined(DEBUG) && DEBUG+0 > 1)
		    const size_t pos = ibin1*nbin23+ibin2*nbin3+ibin3;
		    LOGGER(ibis::gVerbose > 5)
			<< "DEBUG: fill3DBins -- vals1[" << ivals << "]="
			<< vals1[ivals]
			<< ", vals2[" << ivals << "]=" << vals2[ivals]
			<< ", vals3[" << ivals << "]=" << vals3[ivals]
			<< " --> bin ("
			<< static_cast<uint32_t>((vals1[ivals]-begin1)/stride1)
			<< ", "
			<< static_cast<uint32_t>((vals2[ivals]-begin2)/stride2)
			<< ", "
			<< static_cast<uint32_t>((vals3[ivals]-begin3)/stride3)
			<< ") bins[" << pos << "]=" << bins[pos].cnt();
#endif
		}
	    }
	}
	for (size_t i = 0; i < nbins; ++ i)
	    if (bins[i].size() > 0)
		bins[i].adjustSize(0, mask.size());
    }
    else {
	return -11L;
    }
    return nbins;
} // ibis::part::fill3DBins

/// Resolve the 3rd column involved in the 3D bins.  The finally binning
/// work is performed by ibis::part::fill3DBins.
template <typename T1, typename T2>
long ibis::part::fill3DBins3(const ibis::bitvector &mask,
			     const array_t<T1> &val1,
			     const double &begin1, const double &end1,
			     const double &stride1,
			     const array_t<T2> &val2,
			     const double &begin2, const double &end2,
			     const double &stride2,
			     const ibis::column &col3,
			     const double &begin3, const double &end3,
			     const double &stride3,
			     std::vector<ibis::bitvector> &bins) const {
    long ierr = 0;
    switch (col3.type()) {
    case ibis::BYTE: {
	array_t<char>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<char>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectBytes(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<unsigned char>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectUBytes(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::SHORT: {
	array_t<int16_t>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<int16_t>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectShorts(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::USHORT: {
	array_t<uint16_t>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<uint16_t>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectUShorts(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::INT: {
	array_t<int32_t>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<int32_t>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectInts(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::UINT: {
	array_t<uint32_t>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<uint32_t>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectUInts(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1, 
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::LONG: {
	array_t<int64_t>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<int64_t>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectLongs(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::ULONG: {
	array_t<uint64_t>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<uint64_t>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectULongs(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::FLOAT: {
	array_t<float>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<float>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectFloats(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* val3;
	if (mask.cnt() > (nEvents >> 4)) {
	    val3 = new array_t<double>;
	    ierr = col3.getRawData(*val3);
	    if (ierr < 0) {
		delete val3;
		val3 = 0;
	    }
	}
	else {
	    val3 = col3.selectDoubles(mask);
	}
	if (val3 == 0) return -8L;
	ierr = fill3DBins(mask, val1, begin1, end1, stride1,
			  val2, begin2, end2, stride2,
			  *val3, begin3, end3, stride3, bins);
	delete val3;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::fill3DBins3 -- unable to "
	    "handle column (" << col3.name() << ") type "
	    << ibis::TYPESTRING[(int)col3.type()];

	ierr = -7;
	break;}
    }
    return ierr;
} // ibis::part::fill3DBins3

/// Resolve the 2nd column of the 3D bins.  It invokes
/// ibis::part::fill3DBins3 to resolve the 3rd dimension and finally
/// ibis::part::fill3DBins to perform the actual binning.
template <typename T1>
long ibis::part::fill3DBins2(const ibis::bitvector &mask,
			     const array_t<T1> &val1,
			     const double &begin1, const double &end1,
			     const double &stride1,
			     const ibis::column &col2,
			     const double &begin2, const double &end2,
			     const double &stride2,
			     const ibis::column &col3,
			     const double &begin3, const double &end3,
			     const double &stride3,
			     std::vector<ibis::bitvector> &bins) const {
    long ierr = 0;
    switch (col2.type()) {
    case ibis::BYTE: {
	array_t<char>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<char>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectBytes(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<unsigned char>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectUBytes(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::SHORT: {
	array_t<int16_t>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<int16_t>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectShorts(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::USHORT: {
	array_t<uint16_t>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<uint16_t>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectUShorts(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::INT: {
	array_t<int32_t>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<int32_t>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectInts(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::UINT: {
	array_t<uint32_t>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<uint32_t>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectUInts(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1, 
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::LONG: {
	array_t<int64_t>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<int64_t>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectLongs(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::ULONG: {
	array_t<uint64_t>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<uint64_t>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectULongs(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::FLOAT: {
	array_t<float>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<float>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectFloats(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* val2;
	if (mask.cnt() > (nEvents >> 4)) {
	    val2 = new array_t<double>;
	    ierr = col2.getRawData(*val2);
	    if (ierr < 0) {
		delete val2;
		val2 = 0;
	    }
	}
	else {
	    val2 = col2.selectDoubles(mask);
	}
	if (val2 == 0) return -6L;
	ierr = fill3DBins3(mask, val1, begin1, end1, stride1,
			   *val2, begin2, end2, stride2,
			   col3, begin3, end3, stride3, bins);
	delete val2;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::fill3DBins2 -- unable to "
	    "handle column (" << col2.name() << ") type "
	    << ibis::TYPESTRING[(int)col2.type()];

	ierr = -5;
	break;}
    }
    return ierr;
} // ibis::part::fill3DBins2

/// This function calls ibis::part::fill3DBins and other helper functions
/// to compute the 3D bins.  On successful completion, it returns the
/// number of elements in variable bins.  In other word, it returns the
/// number of bins generated, which should be exactly @code
/// (1 + floor((end1-begin1)/stride1)) *
/// (1 + floor((end2-begin2)/stride2)) *
/// (1 + floor((end3-begin3)/stride3))
/// @endcode
/// It returns a negative value to indicate error.  Please refer to the
/// documentation of ibis::part::fill3DBins for additional information
/// about the objects returned in bins.
long ibis::part::get3DBins(const char *constraints, const char *cname1,
			   double begin1, double end1, double stride1,
			   const char *cname2,
			   double begin2, double end2, double stride2,
			   const char *cname3,
			   double begin3, double end3, double stride3,
			   std::vector<ibis::bitvector> &bins) const {
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
	    << (constraints && *constraints ? "subject to " :
		"without constraints")
	    << (constraints ? constraints : "");
	timer.start();
    }

    long ierr;
    ibis::bitvector mask;
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
	ierr = qq.getNumHits();
	if (ierr <= 0) return ierr;
	mask.copy(*(qq.getHitVector()));
    }


    switch (col1->type()) {
    case ibis::BYTE: {
	array_t<char>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<char>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectBytes(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::UBYTE: {
	array_t<unsigned char>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<unsigned char>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectUBytes(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::SHORT: {
	array_t<int16_t>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<int16_t>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectShorts(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::USHORT: {
	array_t<uint16_t>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<uint16_t>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectUShorts(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::INT: {
	array_t<int32_t>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<int32_t>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectInts(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::UINT: {
	array_t<uint32_t>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<uint32_t>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectUInts(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1, 
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::LONG: {
	array_t<int64_t>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<int64_t>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectLongs(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::ULONG: {
	array_t<uint64_t>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<uint64_t>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectULongs(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::FLOAT: {
	array_t<float>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<float>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectFloats(mask);
	}
	if (val1 == 0) return -4L;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    case ibis::DOUBLE: {
	array_t<double>* val1;
	if (mask.cnt() > (nEvents >> 4)) {
	    val1 = new array_t<double>;
	    ierr = col1->getRawData(*val1);
	    if (ierr < 0) {
		delete val1;
		val1 = 0;
	    }
	}
	else {
	    val1 = col1->selectDoubles(mask);
	}
	if (val1 == 0) return -4;
	ierr = fill3DBins2(mask, *val1, begin1, end1, stride1,
			   *col2, begin2, end2, stride2,
			   *col3, begin3, end3, stride3, bins);
	delete val1;
	break;}
    default: {
	LOGGER(ibis::gVerbose >= 4)
	    << "ibis::part::get3DBins -- unable to "
	    "handle column (" << cname1 << ") type "
	    << ibis::TYPESTRING[(int)col1->type()];

	ierr = -3;
	break;}
    }
    if (ierr > 0 && ibis::gVerbose > 0) {
	timer.stop();
	logMessage("get3DBins", "computing the distribution of column "
		   "%s, %s and %s%s%s took %g sec(CPU), %g sec(elapsed)",
		   cname1, cname2, cname3,
		   (constraints ? " with restriction " : ""),
		   (constraints ? constraints : ""),
		   timer.CPUTime(), timer.realTime());
    }
    return ierr;
} // ibis::part::get3DBins

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
		if (sizeof(T3) >= 4)
		    adaptiveFloats(vals3, vmin3, vmax3, nb3, bounds3, counts);
		else
		    adaptiveInts(vals3, vmin3, vmax3, nb3, bounds3, counts);
	    }
	}
	else { // one-dimensional adaptive binning
	    if (vmin3 >= vmax3) {
		bounds3.resize(2);
		bounds3[0] = vmin3;
		bounds3[1] = ibis::util::incrDouble(static_cast<double>(vmin3));
		if (sizeof(T2) >= 4)
		    adaptiveFloats(vals2, vmin2, vmax2, nb2, bounds2, counts);
		else
		    adaptiveInts(vals2, vmin2, vmax2, nb2, bounds2, counts);
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
	    if (sizeof(T1) >= 4)
		adaptiveFloats(vals1, vmin1, vmax1, nb1, bounds1, counts);
	    else
		adaptiveInts(vals1, vmin1, vmax1, nb1, bounds1, counts);
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

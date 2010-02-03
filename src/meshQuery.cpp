// File $Id$
// Author: John Wu <John.Wu at ACM.org>
//         Lawrence Berkeley National Laboratory
// Copyright 2000-2010 the Regents of the University of California
//
//  The implementation of class meshQuery.
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "meshQuery.h"	// class query (prototypes for all functions here)

#include <algorithm>	// std::swap
#include <sstream> // std::ostringstream

// A mesh query must has a known mesh, i.e., the data table must have a
// known mesh shape.
ibis::meshQuery::meshQuery(const char* uid, const ibis::part* et,
			   const char* pref) : ibis::query(uid, et, pref) {
    if (et == 0)
	throw "ibis::meshQuery can not be constructed on a nil table";
    else if (et->getMeshShape().empty())
	throw "ibis::meshQuery must have a table with a mesh";
} // constructor

ibis::meshQuery::~meshQuery() {
} // desctuctor

/**
   This function converts the input bitmap to a list of blocks.
   The bitmap is assumed to be a mapping of a regular mesh with dimensions
   specified in variable dim.  A row-major ordering (C style multiple
   dimensional arrays, NOT Fortran style) is assumed, that is the slowest
   varying dimension is dim[0].

   @param reg The return value that contains the list of blocks as
   hypercubes.  Following the convention of typical C/C++ indexing scheme,
   the lower bounds of the blocks are inclusive and the upper bounds of the
   blocks are exclusive.  For example, a 2D block [2, 3, 5, 10] covers
   points with coordinates [2, 5], [2, 6], [2, 7], [2, 8] and [2, 9].  This
   is an example of a line segment with five points.  This function may
   generate hypercubes of any shape or size.  In general, the lower and
   upper bounds of each dimension is specified together, where the lower
   bound is inclusive but the upper bound is exclusive.
 
   @param dim The size of the grid.  The value dim.size() is the dimension
   of grid.  Input argument, nor modified.

   @param merge An optional argument.  If true, will attempt to merge line
   segments generated to form larger hypercubes.  Default to false because
   it make take a significant amount of time to merge the blocks.

   This function returns an integer value with the following definition.
   -  0 -- conversion succeeded (no warning)
   - -1 -- the bitvector length does not match the product of values in @c dim
   - -2 -- product of values in @c dim overflows an unsigned integer
   - -3 -- no hit vector to work with
   - -4 -- array @c dim is empty

   @note
   It can only be called after the functions @c estimate or @c evaluate
   have been called.  The blocks computed after calling @c estimate may be
   smaller than that computed after calling @c evaluate becaue @c estimate
   may not generate the exact answer to the query.
*/
int ibis::meshQuery::getHitsAsBlocks(std::vector< std::vector<uint32_t> >& reg,
				     const std::vector<uint32_t>& dim,
				     const bool merge) const {
    if (dim.empty()) return -4;
    if (state == FULL_EVALUATE || state == QUICK_ESTIMATE) {
	if (hits == 0) {
	    reg.clear(); // empty regions of interest
	    return 0;
	}
    }
    else { // no hit vector to work with
	return -3;
    }

    ibis::horometer timer;
    timer.start();

    int ierr = toBlocks(*hits, dim, reg);
    double t1 = 0;
    if (ibis::gVerbose > 3) {
	timer.stop();
	t1 = timer.realTime();
	timer.resume();
    }

    const uint32_t nold = reg.size(); // number of blocks on input
    if (merge) {
	if (dim.size() == 2) {
	    merge2DBlocks(reg);
	}
	else if (dim.size() == 3) {
	    merge3DBlocks(reg);
	}
	else if (dim.size() > 3) {
	    mergeNDBlocks(reg);
	}
    }

    if (ibis::gVerbose > 2) {
	timer.stop();
	double t2 = timer.realTime();
	ibis::util::logger lg;
	if (merge && dim.size() > 1 && ibis::gVerbose > 3)
	    lg.buffer() <<"query[" << id() << "]::getHitsAsBlocks -- merging "
			<< nold << " " << dim.size() << "-D block"
			<< (nold>1 ? "s" : "") << " into " << reg.size()
			<< " used " << t2-t1 << " sec (elapsed)";

	lg.buffer() << "\nquery[" << id() << "getHitsAsBlocks -- converting "
		    << hits->cnt() << (hits->cnt() > 1 ? " hits" : " hit")
		    << " into " << reg.size()
		    << (reg.size()>1 ? " blocks" : " block") << " on a ("
		    << dim[0];
	for (uint32_t i = 1; i < dim.size(); ++ i)
	    lg.buffer() << " x " << dim[i];
	lg.buffer() << ") mesh took " << t2 << " sec (elapsed)";
    }
    return ierr;
} // ibis::meshQuery::getHitsAsBlocks

/**
   This variant of getHitsAsBlocks uses the dimensions defined by
   ibis::table::getMeshShape().

   @see ibis::meshQuery::getHitsAsBlocks
   @see ibis::table::getMeshShape
*/
int ibis::meshQuery::getHitsAsBlocks(std::vector< std::vector<uint32_t> >& reg,
				     const bool merge) const {
    if (state == FULL_EVALUATE || state == QUICK_ESTIMATE) {
	if (hits == 0) {
	    reg.clear(); // empty regions of interest
	    return 0;
	}
    }
    else { // no hit vector to work with
	return -3;
    }

    ibis::horometer timer;
    timer.start();

    const std::vector<uint32_t>& shape = partition()->getMeshShape();
    if (shape.empty()) return -4;
    int ierr = toBlocks(*hits, shape, reg);
    double t1 = 0;
    if (ibis::gVerbose > 3) {
	timer.stop();
	t1 = timer.realTime();
	timer.resume();
    }

    const uint32_t nold = reg.size(); // number of blocks on input
    if (merge) {
	if (shape.size() == 2) {
	    merge2DBlocks(reg);
	}
	else if (shape.size() == 3) {
	    merge3DBlocks(reg);
	}
	else if (shape.size() > 3) {
	    mergeNDBlocks(reg);
	}
    }

    if (ibis::gVerbose > 2) {
	timer.stop();
	double t2 = timer.realTime();
	ibis::util::logger lg;
	if (merge && shape.size() > 1 && ibis::gVerbose > 3)
	    lg.buffer() << "query[" << id() << "]::getHitsAsBlocks -- merging "
			<< nold << " " << shape.size() << "-D block"
			<< (nold>1 ? "s" : "") << " into " << reg.size()
			<< " used " << t2-t1 << " sec (elapsed)";

	lg.buffer() << "query[" << id() << "]::getHitsAsBlocks -- converting "
		    << hits->cnt() << " into " << reg.size() << " block"
		    << (reg.size() > 1 ? "s" : "") << " on a (" << shape[0];
	for (uint32_t i = 1; i < shape.size(); ++ i)
	    lg.buffer() << " x " << shape[i];
	lg.buffer() << ") mesh took " << t2 << " sec (elapsed)";
    }
    return ierr;
} // ibis::meshQuery::getHitsAsBlocks

/**
   The main routine for converting the bitvector into a list of blocks.  It
   produces a list of blocks.  A block is represented as a rectangle on the
   underlying grid defined by dim.  The grid points are number in a simple
   raster scan order with dim[0] being the slowest varying dimension.  A
   line in the following description refers to a list of consecutive points
   that only differ in their last coordinates.  A block may either be a
   line segment if it only contains part of a line or an area if it
   contains multiple lines.  Both line segments and areas may be arbitrary
   sizes.  In particular, an area may be hypercube of any dimensions.
*/
int ibis::meshQuery::toBlocks
(const ibis::bitvector& bv, const std::vector<uint32_t>& dim,
 std::vector< std::vector<uint32_t> >& reg) const {
    uint32_t nb = 0;
    if (dim.size() > 0) {
	nb = dim[0];
	for (uint32_t i = 1; i < dim.size(); ++ i) {
	    const uint32_t tmp = nb * dim[i];
	    if (dim[i] > 0 && nb == tmp / dim[i]) {
		nb = tmp;
	    }
	    else {
		return -2;
	    }
	}
    }
    if (nb != bv.size())
	return -1;

    ibis::horometer timer;
    timer.start();

    reg.clear();
    if (nb == 0)
	return 0;
    if (bv.cnt() >= bv.size()) { // every point is in the block
	std::vector<uint32_t> tmp(dim.size() + dim.size(), 0);
	for (uint32_t i = 0; i < dim.size(); ++ i)
	    tmp[i+i+1] = dim[i];
	reg.push_back(tmp);
	return 0;
    }

    ibis::bitvector::indexSet ix = bv.firstIndexSet();
    if (ix.nIndices() == 0)
	return 0;
    const ibis::bitvector::word_t *ind = ix.indices();

    switch (dim.size()) {
    case 1: {// 1-d
	std::vector<uint32_t> tmp(2);
	// handling the first index set
	tmp[0] = ind[0];
	if (ix.isRange()) {
	    tmp[1] = ind[1];
	}
	else {
	    tmp[1] = ind[0] + 1;
	    for (uint32_t i = 1; i < ix.nIndices(); ++ i) {
		if (ind[i] == tmp[1]) {
		    ++ tmp[1];
		}
		else {
		    reg.push_back(tmp);
		    tmp[0] = ind[i];
		    tmp[1] = ind[i] + 1;
		}
	    }
	}
	++ ix;

	// all the rest index sets
	while (ix.nIndices() > 0) {
	    if (ix.isRange()) {
		if (tmp[1] == ind[0]) { // extend the current block
		    tmp[1] = ind[1];
		}
		else { // save the current block and start a new one
		    reg.push_back(tmp);
		    tmp[0] = ind[0];
		    tmp[1] = ind[1];
		}
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    if (ind[i] == tmp[1]) {
			++ tmp[1];
		    }
		    else {
			reg.push_back(tmp);
			tmp[0] = ind[i];
			tmp[1] = ind[i] + 1;
		    }
		}
	    }
	    ++ ix; // next group of nonzero bits
	}
	reg.push_back(tmp); // record the last block
	break;}

    case 2: {// 2-d
	uint32_t last;
	std::vector<uint32_t> tmp(4);
	// handle to first index set
	tmp[0] = ind[0] / dim[1];
	tmp[2] = ind[0] - tmp[0] * dim[1];
	if (ix.isRange()) {
	    last = ind[1];
	    block2d(ind[1], dim, tmp, reg);
	}
	else {
	    for (uint32_t i = 1; i < ix.nIndices(); ++ i) {
		if (ind[i] > ind[i-1]+1) {
		    block2d(ind[i-1]+1, dim, tmp, reg);
		    reg.push_back(tmp);
		    tmp[0] = ind[i] / dim[1];
		    tmp[2] = ind[i] - tmp[0] * dim[1];
		}
	    }
	    last = ind[ix.nIndices() - 1] + 1;
	    block2d(last, dim, tmp, reg);
	}
	++ ix;

	// handling the rest of the index sets
	while (ix.nIndices() > 0) {
	    if (ix.isRange()) {
		if (ind[0] > last) { // a new block
		    reg.push_back(tmp);
		    tmp[0] = ind[0] / dim[1];
		    tmp[2] = ind[0] - tmp[0] * dim[1];
		}
		last = ind[1];
		block2d(ind[1], dim, tmp, reg);
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    if (ind[i] > last) { // a new block
			block2d(last, dim, tmp, reg);
			reg.push_back(tmp);
			tmp[0] = ind[i] / dim[1];
			tmp[2] = ind[i] - tmp[0] * dim[1];
		    }
		    last = ind[i] + 1;
		}
		block2d(last, dim, tmp, reg);
	    }
	    ++ ix; // next group of nonzero bits
	}
	reg.push_back(tmp); // record the last block
	break;}

    case 3: {// 3-d
	uint32_t last;
	const uint32_t n3 = dim[2];
	const uint32_t n2 = dim[2] * dim[1];
	std::vector<uint32_t> tmp(6);
	// handle the first index set
	tmp[0] = ind[0] / n2;
	tmp[2] = (ind[0] - tmp[0] * n2) / n3;
	tmp[4] = ind[0] % n3;
	if (ix.isRange()) {
	    last = ind[1];
	    block3d(ind[1], n2, n3, dim, tmp, reg);
	}
	else {
	    for (uint32_t i = 1; i < ix.nIndices(); ++ i) {
		if (ind[i] > ind[i-1]+1) {
		    block3d(ind[i-1]+1, n2, n3, dim, tmp, reg);
		    reg.push_back(tmp);
		    tmp[0] = ind[i] / n2;
		    tmp[2] = (ind[i] - n2 * tmp[0]) / n3;
		    tmp[4] = ind[i] % n3;
		}
	    }
	    last = ind[ix.nIndices() - 1] + 1;
	    block3d(last, n2, n3, dim, tmp, reg);
	}
	++ ix;

	// handling the rest of the index sets
	while (ix.nIndices() > 0) {
	    if (ix.isRange()) {
		if (ind[0] > last) {
		    reg.push_back(tmp);
		    tmp[0] = ind[0] / n2;
		    tmp[2] = (ind[0] - tmp[0] * n2) / n3;
		    tmp[4] = ind[0] % n3;
		}
		last = ind[1];
		block3d(ind[1], n2, n3, dim, tmp, reg);
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    if (ind[i] > last) {
			if (i > 0) {
			    block3d(last, n2, n3, dim, tmp, reg);
			}
			reg.push_back(tmp);
			tmp[0] = ind[i] / n2;
			tmp[2] = (ind[i] - tmp[0] * n2) / n3;
			tmp[4] = ind[i] % n3;
		    }
		    last = ind[i] + 1;
		}
		block3d(last, n2, n3, dim, tmp, reg);
	    }
	    ++ ix; // next group of nonzero bits
	}
	reg.push_back(tmp); // record the last block
	break;}

    default: {// the general case
	std::vector<uint32_t> tmp(2*dim.size());
	std::vector<uint32_t> scl(dim.size());
	uint32_t last;
	scl.back() = 1;
	for (uint32_t j = dim.size()-1; j > 0;) {
	    scl[j-1] = scl[j] * dim[j];
	    -- j;
	}
	// handle the first index set
	uint32_t xx = ind[0];
	for (uint32_t j = 0; j < dim.size(); ++ j) {
	    tmp[j+j] = xx / scl[j];
	    xx %= scl[j];
	}
	if (ix.isRange()) {
	    last = ind[1];
	    blocknd(ind[1], scl, dim, tmp, reg);
	}
	else {
	    for (uint32_t i = 1; i < ix.nIndices(); ++ i) {
		if (ind[i] > ind[i-1]+1) { // a new reange
		    blocknd(ind[i-1]+1, scl, dim, tmp, reg);
		    reg.push_back(tmp);
		    xx = ind[i];
		    for (uint32_t j = 0; j < dim.size(); ++j) {
			tmp[j+j] = xx / scl[j];
			xx %= scl[j];
		    }
		}
	    }
	    last = ind[ix.nIndices() - 1] + 1;
	    blocknd(last, scl, dim, tmp, reg);
	}
	++ ix;

	// handle the rest of the index sets
	while (ix.nIndices() > 0) {
	    if (ix.isRange()) {
		if (ind[0] > last) { // a new block
		    reg.push_back(tmp);
		    xx = ind[0];
		    for (uint32_t j = 0; j < dim.size(); ++j) {
			tmp[j+j] = xx / scl[j];
			xx %= scl[j];
		    }
		}
		last = ind[1];
		blocknd(ind[1], scl, dim, tmp, reg);
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    if (ind[i] > last) { // start a new block
			if (i > 0) {
			    blocknd(last, scl, dim, tmp, reg);
			}
			reg.push_back(tmp); // record the last block
			xx = ind[i];
			for (uint32_t j = 0; j < dim.size(); ++j) {
			    tmp[j+j] = xx / scl[j];
			    xx %= scl[j];
			}
		    }
		    last = ind[i] + 1;
		}
		blocknd(last, scl, dim, tmp, reg);
	    }
	    ++ ix; // next group of nonzero bits
	}

	reg.push_back(tmp); // record the last block
	break;}
    } // switch(dim.size())

    if (ibis::gVerbose > 3) {
	timer.stop();
	LOGGER(ibis::gVerbose > 3)
	    << "query[" << id() << "]::toBlocks -- converting the bitmap ("
	    << bv.size() << ", " << bv.cnt() << ") to " << reg.size()
	    << " block" << (reg.size()>1?"s":"") << " in " << dim.size()
	    << "-D space took " << timer.realTime() << " sec (elapsed)";
    }
    return 0; // successfully completed the conversion
} // ibis::meshQuery::toBlocks

// deal with one (single) 2D block, the last block generated is not
// recorded, other blocks generated here are recorded in reg
void ibis::meshQuery::block2d
(uint32_t last,
 const std::vector<uint32_t>& dim,
 std::vector<uint32_t>& block,
 std::vector< std::vector<uint32_t> >& reg) const {
    if (dim.size() != 2) return; // dimension must be two
    const uint32_t next = (last-1) / dim[1];
    const uint32_t rem  = last - next * dim[1];
    if (next > block[0]) {// need to record lines in the middle
	if (block[2] > 0) {
	    block[1] = block[0] + 1;
	    block[3] = dim[1];
	    reg.push_back(block);
	    block[0] = block[1];
	}
	if (next > block[0]) {
	    if (rem < dim[1]) {
		block[1] = next;
		block[2] = 0;
		block[3] = dim[1];
		reg.push_back(block);
		block[0] = next;
		block[1] = next + 1;
		block[2] = 0;
		block[3] = rem;
	    }
	    else {
		block[1] = next + 1;
		block[2] = 0;
		block[3] = dim[1];
	    }
	}
	else {
	    block[0] = next;
	    block[1] = next + 1;
	    block[2] = 0;
	    block[3] = rem;
	}
    }
    else {
	block[1] = block[0] + 1;
	block[3] = rem;
    }
} // ibis::meshQuery::block2d

// deal with one (single) 3D block
void ibis::meshQuery::block3d
(uint32_t last, const uint32_t n2, const uint32_t n3,
 const std::vector<uint32_t>& dim, std::vector<uint32_t>& block,
 std::vector< std::vector<uint32_t> >& reg) const {
    if (dim.size() != 3) return; // dimension must be three (3)
    std::vector<uint32_t> next(3); // the 3-d coordinate of (last-1)
    -- last;
    next[0] = last / n2;
    last %= n2;
    next[1] = last / n3;
    next[2] = (last % n3) + 1;

    if (next[0] > block[0]) { // on different planes
	if (block[4] > 0) {
	    // complete the line containing the starting point
	    block[1] = block[0] + 1;
	    block[3] = block[2] + 1;
	    block[5] = dim[2];
	    reg.push_back(block);
	    if (block[3] < dim[1]) {
		block[2] = block[3];
	    }
	    else {
		block[2] = 0;
		block[0] = block[1];
	    }
	}
	if (block[2] > 0) { // complete the plan
	    block[1] = block[0] + 1;
	    block[3] = dim[1];
	    block[4] = 0;
	    block[5] = dim[2];
	    reg.push_back(block);
	    block[0] = block[1];
	}
	// the following blocks start with fresh planes
	if (block[0] < next[0]) { // record the planes in-between
	    if (next[2] < dim[2]) {
		block[1] = next[0];
		block[2] = 0;
		block[3] = dim[1];
		block[4] = 0;
		block[5] = dim[2];
		reg.push_back(block);
		// the plane containing the point (last-1)
		block[0] = next[0];
		block[1] = next[0] + 1;
		if (next[1] > 0) {// record the whole lines
		    block[2] = 0;
		    block[3] = next[1];
		    block[4] = 0;
		    block[5] = dim[2];
		    reg.push_back(block);
		}
		// record the line containing the point (last-1)
		block[2] = next[1];
		block[3] = next[1] + 1;
		block[4] = 0;
		block[5] = next[2];
	    }
	    else if (next[1]+1 < dim[1]) {
		block[1] = next[0];
		block[2] = 0;
		block[3] = dim[1];
		block[4] = 0;
		block[5] = dim[2];
		reg.push_back(block);
		// the plane containing the point (last-1) has only whole
		// lines
		block[0] = next[0];
		block[1] = next[0] + 1;
		block[2] = 0;
		block[3] = next[1] + 1;
		block[4] = 0;
		block[5] = dim[2];
	    }
	    else { // there is only whole planes
		block[1] = next[0] + 1;
		block[2] = 0;
		block[3] = dim[1];
		block[4] = 0;
		block[5] = dim[2];
	    }
	}
	else if (next[2] < dim[2]) { // all within one plan
	    block[1] = next[0] + 1;
	    if (next[1] > 0) {// record the whole lines
		block[2] = 0;
		block[3] = next[1];
		block[4] = 0;
		block[5] = dim[2];
		reg.push_back(block);
	    }
	    // record the line containing the point (last-1)
	    block[2] = next[1];
	    block[3] = next[1] + 1;
	    block[4] = 0;
	    block[5] = next[2];
	}
	else {
	    // the plane containing the point (last-1) has only whole
	    // lines
	    block[1] = next[0] + 1;
	    block[2] = 0;
	    block[3] = next[1] + 1;
	    block[4] = 0;
	    block[5] = dim[2];
	}
    }
    else if (next[1] > block[2]) { // in the same plane, different lines
	block[1] = block[0] + 1;
	// complete the line containing the starting point
	if (block[4] > 0) {
	    block[3] = block[2] + 1;
	    block[5] = dim[2];
	    reg.push_back(block);
	    if (block[3] < dim[1]) {
		block[2] = block[3];
	    }
	    else {
		block[2] = 0;
		block[0] = block[1];
	    }
	}
	if (next[1] > block[2]) { // the whole lines in-between
	    if (next[2] < dim[2]) {
		block[3] = next[1];
		block[4] = 0;
		block[5] = dim[2];
		reg.push_back(block);
		block[2] = next[1];
		block[3] = next[1] + 1;
		block[4] = 0;
		block[5] = next[2];
	    }
	    else {
		block[3] = next[1] + 1;
		block[4] = 0;
		block[5] = dim[2];
	    }
	}
	else { // record the last line as a separate block
	    block[2] = next[1];
	    block[3] = next[1] + 1;
	    block[4] = 0;
	    block[5] = next[2];
	}
    }
    else { // on the same line
	block[1] = block[0] + 1;
	block[3] = block[2] + 1;
	block[5] = next[2];
    }
} // ibis::meshQuery::block3d

// deal with one (single) N-Dimensional block
void ibis::meshQuery::blocknd
(uint32_t last, const std::vector<uint32_t>& scl,
 const std::vector<uint32_t>& dim, std::vector<uint32_t>& block,
 std::vector< std::vector<uint32_t> >& reg) const {
    if (dim.size() < 3) return; // only handle dimensions higher than 3
    std::vector<uint32_t> next(dim.size());
    uint32_t xx = last - 1;
    for (uint32_t j = 0; j < dim.size(); ++j) {
	next[j] = xx / scl[j];
	xx %= scl[j];
    }

    // shrd counts the dimensions that are same
    uint32_t shrd = 0;
    while (shrd < dim.size()) {
	if (next[shrd] > block[shrd+shrd]) {
	    break;
	}
	else {
	    if (next[shrd] < block[shrd+shrd]) {
		logWarning("blocknd", "end point coordinate[%lu](=%lu) less "
			   "than that of the starting point of the block "
			   "(%lu), reset to %lu",
			   static_cast<long unsigned>(shrd),
			   static_cast<long unsigned>(next[shrd]),
			   static_cast<long unsigned>(block[shrd+shrd]),
			   static_cast<long unsigned>(block[shrd+shrd]));
		next[shrd] = block[shrd+shrd];
	    }
	    ++ shrd;
	}
    }

    // for all the dimensions that are the same, the end point coordinates
    // can be assigned
    for (uint32_t j = 0; j < shrd; ++ j)
	block[j+j+1] = block[j+j] + 1;

    if (shrd+1 < dim.size()) { // the block go across multiple lines
	for (uint32_t j = dim.size()-1; j > shrd; -- j) {
	    if (block[j+j]+1 < dim[j] || j+1 == dim.size()) {
		// make up lines, planes, etc. starting with the first point
		for (uint32_t k = shrd; k < j; ++ k)
		    // first few dimensions only cover one value each
		    block[k+k+1] = block[k+k] + 1;
		// dimension j: from the known start point to the end
		block[j+j+1] = dim[j];
		for (uint32_t k = j + 1; k < dim.size(); ++ k) {
		    // the faster varying dimensions are covered in whole
		    block[k+k] = 0;
		    block[k+k+1] = dim[k];
		}
#if DEBUG+0 > 0 || _DEBUG+0 > 0
		if (block[dim.size()+dim.size()-2] == 0 &&
		    block.back() == dim.back()) {
		    ibis::util::logger lg(4);
		    lg.buffer() << "DEBUG -- ibis::meshQuery[" << id()
				<< "]::blocknd -- " << reg.size() << "\t(";
		    for (uint32_t k = 0; k < block.size(); ++k) {
			if (k > 0) lg.buffer() << ", ";
			lg.buffer() << block[k];
		    }
		    lg.buffer() << ")";
		}
#endif
		reg.push_back(block); // record it
	    }
	}
	if (next[shrd] > block[shrd+shrd]+1) { // the largest chunck
	    ++ block[shrd+shrd];
	    block[shrd+shrd+1] = next[shrd];
	    for (uint32_t k = shrd+1; k < dim.size(); ++ k) {
		block[k+k] = 0;
		block[k+k+1] = dim[k];
	    }
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	    if (block[dim.size()+dim.size()-2] == 0 &&
		block.back() == dim.back()) {
		ibis::util::logger lg(4);
		lg.buffer() << "DEBUG -- ibis::meshQuery[" << id()
			    << "]::blocknd -- " << reg.size() << "\t(";
		for (uint32_t k = 0; k < block.size(); ++k) {
		    if (k > 0) lg.buffer() << ", ";
		    lg.buffer() << block[k];
		}
		lg.buffer() << ")";
	    }
#endif
	    reg.push_back(block);
	}
	for (uint32_t j = shrd+1; j < dim.size(); ++j) {
	    if (next[j] > 0 || j+1 == dim.size()) {
		// line, plane, and so on related to the last point
		for (uint32_t k = shrd; k < j; ++ k) {
		    block[k+k] = next[k];
		    block[k+k+1] = next[k]+1;
		}
		block[j+j] = 0;
		block[j+j+1] = next[j] + 1;
		for (uint32_t k = j+1; k < dim.size(); ++k) {
		    block[k+k] = 0;
		    block[k+k+1] = dim[k];
		}
#if DEBUG+0 > 0 || _DEBUG+0 > 0
		if (block[dim.size()+dim.size()-2] == 0 &&
		    block.back() == dim.back()) {
		    ibis::util::logger lg(4);
		    lg.buffer() << "DEBUG -- ibis::meshQuery[" << id()
				<< "]::blocknd -- " << reg.size() << "\t(";
		    for (uint32_t k = 0; k < block.size(); ++k) {
			if (k > 0) lg.buffer() << ", ";
			lg.buffer() << block[k];
		    }
		    lg.buffer() << ")";
		}
#endif
		if (j+1 < dim.size()) // record all except the last one
		    reg.push_back(block);
	    }
	}
    }
    else if (shrd+1 == dim.size()) {
	// the first point and the last point are one the same line
	block[shrd+shrd+1] = next[shrd] + 1;
    }
} // ibis::meshQuery::blocknd

// blocks with one dimension that has connecting coordinates and the same
// coordinates on all other dimensions can be merged
//
// assumptions/requirements
// (1) the incoming reg is assumed to be sorted
// (2) no dimension reaches the maximum value of UINT_MAX, which is used to
//     denote a invalid block to be removed
void ibis::meshQuery::merge2DBlocks
(std::vector< std::vector<uint32_t> >& reg) const {
    if (reg.empty()) return;
    if (reg[0].size() != 4) return; // dimension must be two (2)

    uint32_t remove = 0;
    uint32_t match = 0;
    /* UNNACESSARY
    // loop 1 -- if the first dimension matches, try to merge the second one
    for (uint32_t i = 1; i < reg.size(); ++ i) {
    if (reg[i][0] == reg[match][0] && reg[i][1] == reg[match][1]) {
    for (uint32_t j = match; j < i; ++j) {
    if (reg[j][3] == reg[i][2]) {
    reg[j][3] = reg[i][3];
    reg[i][0] = UINT_MAX; // mark ith block for removal
    ++ remove;
    break;
    }
    }
    }
    else if (reg[i][0] != UINT_MAX) {
    match = i;
    }
    }
    */

    // loop 2 -- for groups with the connecting first dimension, check if
    // the second dimensions match
    match = 0;
    uint32_t end = 0;
    for (uint32_t i = 0; i < reg.size();) {
	// skip to the next entry not marked for removal
	while (i < reg.size() && reg[i][0] == UINT_MAX) ++i;
	if (i >= reg.size()) continue;

	if (match >= reg.size() || reg[match][0] != reg[i][1]) {
	    // first search for blocks with connecting first dimension
	    match = i+1;
	    while (match < reg.size() && (reg[match][0] < reg[i][1] ||
					  reg[match][0] == UINT_MAX))
		++ match;
	    if (match<reg.size() && reg[match][0] == reg[i][1]) {
		end = match+1;
		while (end < reg.size() && (reg[end][0] == reg[i][1] ||
					    reg[end][0] == UINT_MAX))
		    ++ end;
	    }
	    else {
		// skip all blocks with the same first dimension
		end = i;
		do {
		    ++ i;
		} while (i < reg.size() && (reg[i][0] == UINT_MAX ||
					    reg[end][1] == reg[i][1]));
		continue;
	    }
	}

	// a loop to search for matching second dimension
	uint32_t j = 0;
	for (j = match; j < end; ++j) {
	    if (reg[i][1] == reg[j][0] && reg[i][2] == reg[j][2] &&
		reg[i][3] == reg[j][3]) {
		reg[i][1] = reg[j][1];
		reg[j][0] = UINT_MAX;
		++ remove;
		break;
	    }
	}
	if (j >= end) // fall through the above loop, i.e., no match
	    ++i;
    }

    // loop 3 -- to remove the entries marked for removal
    if (remove == 0) return;
    end = reg.size() - remove;
    match = 0;
    for (uint32_t i = 0; i < end; ++ i) {
	if (reg[i][0] == UINT_MAX) {
	    // search for the next valid entry
	    if (match <= i)
		match = i + 1;
	    while (reg[match][0] == UINT_MAX)
		++ match;
	    reg[i][0] = reg[match][0];
	    reg[i][1] = reg[match][1];
	    reg[i][2] = reg[match][2];
	    reg[i][3] = reg[match][3];
	    reg[match][0] = UINT_MAX;
	    ++ match;
	}
    }
    reg.resize(end);
} // ibis::meshQuery::merge2DBlocks

void ibis::meshQuery::merge3DBlocks
(std::vector< std::vector<uint32_t> >& reg) const {
    if (reg.empty()) return;
    if (reg[0].size() != 6) return; // dimension must be three (3)

    uint32_t remove = 0;
    uint32_t match = 0;
    /* UNNACESSARY
    // loop 1 -- if the first two dimensions match, try to merge the third
    for (uint32_t i = 1; i < reg.size(); ++ i) {
    if (reg[i][0] == reg[match][0] && reg[i][1] == reg[match][1] &&
    reg[i][2] == reg[match][2] && reg[i][3] == reg[match][3]) {
    for (uint32_t j = match; j < i; ++j) {
    if (reg[j][5] == reg[i][4]) {
    reg[j][5] = reg[i][5];
    reg[i][0] = UINT_MAX; // mark ith block for removal
    ++ remove;
    break;
    }
    }
    }
    else if (reg[i][0] != UINT_MAX) {
    match = i;
    }
    }
    */

    // loop 2 -- if the first dimension matches, the second dimension
    // connects, merge those with the same thrid dimension
    match = 0;
    uint32_t end = 0;
    for (uint32_t i = 0; i < reg.size();) {
	// skip till next entry not marked for removal
	while (i < reg.size() && reg[i][0] == UINT_MAX) ++i;
	if (i >= reg.size()) continue;

	if (match <= i || match >= reg.size() || reg[match][0] != reg[i][0] ||
	    reg[match][1] != reg[i][1] || reg[match][2] != reg[i][3]) {
	    // need to look for new bunch of blocks with the same first
	    // dimension and connected second dimension
	    match = i + 1;
	    while (match < reg.size() &&
		   ((reg[match][0] == reg[i][0] &&
		     reg[match][1] == reg[i][1] &&
		     reg[match][2] < reg[i][3]) ||
		    (reg[match][0] == UINT_MAX)))
		++ match;
	    if (match < reg.size() && reg[match][0] == reg[i][0] &&
		reg[match][1] == reg[i][1] && reg[match][2] == reg[i][3]) {
		// found one that connects, search for the end of the group
		end = match + 1;
		while (end < reg.size() && 
		       ((reg[end][1] == reg[i][1] &&
			 reg[end][2] == reg[i][3] &&
			 reg[end][0] == reg[i][0]) ||
			reg[end][0] == UINT_MAX))
		    ++ end;
	    }
	    else { // no block connects to the reg[i]
		end = i;
		do {
		    ++ i;
		} while (i < reg.size() && ((reg[i][1] == reg[end][1] &&
					     reg[i][3] == reg[end][3] &&
					     reg[i][0] == reg[end][0]) ||
					    reg[i][0] == UINT_MAX));
		continue;
	    }
	}

	// look between [match, end) for matching third dimension
	uint32_t j = 0;
	for (j = match; j < end; ++j) {
	    if (reg[i][0] == reg[j][0] && reg[i][4] == reg[j][4] &&
		reg[i][5] == reg[j][5]) {
		reg[i][3] = reg[j][3];
		reg[j][0] = UINT_MAX;
		++ remove;
		break;
	    }
	}
	if (j >= end) // fall through the above loop, i.e., no match
	    ++i;
    }

    // loop 3 -- among the blocks where the first dimension connects, look
    // for matching second and third dimensions
    match = 0;
    end = 0;
    for (uint32_t i = 0; i < reg.size();) {
	// skip those marked for removal
	while (i < reg.size() && reg[i][0] == UINT_MAX) ++ i;
	if (i >= reg.size()) continue;

	// find blocks that connects on the first dimension
	if (match <= i || match >= reg.size() || reg[match][0] != reg[i][1]) {
	    match = i + 1;
	    while (match < reg.size() && (reg[match][0] < reg[i][1] ||
					  reg[match][0] == UINT_MAX))
		++ match;
	    if (match < reg.size() && reg[match][0] == reg[i][1]) {
		// found a connecting block
		end = match + 1;
		while (end < reg.size() && (reg[end][0] == reg[i][1] ||
					    reg[end][0] == UINT_MAX))
		    ++ end;
	    }
	    else { // no connecting blocks
		end = i;
		do {
		    ++ i;
		} while (i < reg.size() && (reg[i][0] == UINT_MAX ||
					    reg[i][1] == reg[end][1]));
		continue;
	    }
	}

	// search through all those with connecting first dimension for
	// matching 2nd and 3rd dimensions
	uint32_t j = 0;
	for (j = match; j < end; ++j) {
	    if (reg[i][2] == reg[j][2] && reg[i][3] == reg[j][3] &&
		reg[i][4] == reg[j][4] && reg[i][5] == reg[j][5] &&
		reg[i][1] == reg[j][0]) {
		reg[i][1] = reg[j][1];
		reg[j][0] = UINT_MAX;
		++ remove;
		break;
	    }
	}
	if (j >= end) // no match
	    ++ i;
    }

    // loop 4 -- to remove the entries matched for removal
    if (remove == 0) return;
    end = reg.size() - remove;
    match = 0;
    for (uint32_t i = 0; i < end; ++ i) {
	if (reg[i][0] == UINT_MAX) {
	    // search for the next valid entry
	    if (match <= i)
		match = i + 1;
	    while (reg[match][0] == UINT_MAX)
		++ match;
	    reg[i][0] = reg[match][0];
	    reg[i][1] = reg[match][1];
	    reg[i][2] = reg[match][2];
	    reg[i][3] = reg[match][3];
	    reg[i][4] = reg[match][4];
	    reg[i][5] = reg[match][5];
	    reg[match][0] = UINT_MAX;
	    ++ match;
	}
    }
    reg.resize(end);
} // ibis::meshQuery::merge3DBlocks

void ibis::meshQuery::mergeNDBlocks
(std::vector< std::vector<uint32_t> >& reg) const {
    if (reg.empty()) return;
    // dimension must be higher than 3, bounding box must have even number
    // of elements
    if (reg[0].size() < 6 || (reg[0].size() % 2 != 0)) return;
    const uint32_t width = reg[0].size();
    uint32_t remove = 0;

    // outer loop -- loop through dimensions from the back to the front
    for (uint32_t d = width-1; d > 1; d -= 2) {
	const uint32_t d0 = d - 1;
	uint32_t j=0, match=0, end=0;
	// examine every block
	for (uint32_t i = 0; i < reg.size();) {
	    // skip those marked for removal
	    while (i < reg.size() && reg[i][0] == UINT_MAX)
		++ i;
	    if (i >= reg.size()) continue;

	    bool tst = true;
	    // does reg[i] connect with reg[match] ?
	    tst = (match > i && match < reg.size());
	    if (tst && d0 > 0) {
		if (reg[match][0] != UINT_MAX) {
		    for (j = 0; j < d0 && tst; ++j)
			tst = (reg[match][j] == reg[i][j]);
		}
		else {
		    tst = false;
		}
	    }
	    if (! tst || (reg[match][d0] != reg[i][d])) {
		// need to find a new connecting block
		bool eq = true;
		match = i + 1;
		tst = (match < reg.size());
		while (tst) {
		    if (d0 > 0) {
			if (reg[match][0] != UINT_MAX) {
			    eq = true;
			    for (j = 0; j < d0 && eq; ++ j)
				eq = (reg[match][j] == reg[i][j]);
			    if (eq)
				tst = (reg[match][d0] < reg[i][d]);
			}
			if (tst) {
			    ++ match;
			    tst = (match < reg.size());
			}
		    }
		    else if (reg[match][d0] < reg[i][d]) {
			++ match;
			tst = (match < reg.size());
		    }
		    else {
			tst = false;
		    }
		}

		if (match < reg.size() && eq && reg[match][d0] == reg[i][d]) {
		    // found a connecting block
		    end = match + 1;
		    tst = (end < reg.size());
		    while (tst) {
			if (reg[end][0] != UINT_MAX) {
			    for (j = 0; j < d0; ++j)
				tst = (reg[end][j] == reg[i][j]);
			    if (tst)
				tst = (reg[end][d0] == reg[i][d]);
			}
			if (tst) {
			    ++ end;
			    tst = (end < reg.size());
			}
		    }
		}
		else { // nothing connects
		    end = i;
		    tst = true;
		    while (tst) {
			++ i;
			if (i < reg.size()) {
			    if (reg[i][0] != UINT_MAX) {
				for (j = 1; j <= d && tst; ++j)
				    tst = (reg[i][j] == reg[end][j]);
			    }
			}
			else {
			    tst = false;
			}
		    }
		    continue;
		}
	    }

	    // search through blocks between match and end
	    if (end-match > 100) {
		// perform the search one dimension at a time to take
		// advantage of the fact that the lower bounds of each
		// dimension remains sorted
		typedef std::map<uint32_t, uint32_t> myList;
		myList cand; // possible matches
		cand[match] = end; // initialize with the whole list
		for (j = d+1; j < width-1 && cand.size() > 0; j += 2) {
		    myList refined;
		    for (myList::const_iterator it = cand.begin();
			 it != cand.end(); ++ it) {
			uint32_t k0 = (*it).first, k1 = (*it).second;
			while (k0+1 < k1) {
			    uint32_t mid = (k0 + k1) / 2;
			    if (reg[mid][j] < reg[i][j]) {
				k0 = mid;
			    }
			    else if (reg[mid][j] > reg[i][j]) {
				k1 = mid;
			    }
			    else { // found an equal entry
				// k0 - left end of the equal entries
				uint32_t n0 = k0, n1 = mid;
				if (reg[k0][j] < reg[i][j]) {
				    while (n0 < n1) {
					k0 = (n0 + n1) / 2;
					if (k0 == n0) {
					    if (reg[k0][j] < reg[i][j])
						++ n0;
					    else
						-- n1;
					}
					else if (reg[k0][j] < reg[i][j])
					    n0 = k0;
					else
					    n1 = k0;
				    }
				    k0 = n1;
				}
				// k1 - right end of the equal entries
				n0 = mid; n1 = k1;
				if (reg[k1][j] > reg[i][j]) {
				    while (n0 < n1) {
					k1 = (n0 + n1) / 2;
					if (k1 == n0) {
					    if (reg[k1][j] > reg[i][j])
						-- n1;
					    else
						++ n0;
					}
					else if (reg[k1][j] > reg[i][j])
					    n1 = k1;
					else
					    n0 = k1;
				    }
				    k1 = n0;
				}
				// record the list of entries, manipulate
				// k0 and k1 to terminate the while loop
				refined[k0] = k1;
				mid = k1;
				k1 = k0;
				k0 = mid;
			    }
			}
			if (k0+1 == k1) {
			    if (reg[k0][j] == reg[i][j]) {
				refined[k0] = k1;
			    }
			}
		    }
		    std::swap(cand, refined);
		}

		tst = false; // tst = true if found an equal entry
		for (myList::const_iterator it = cand.begin();
		     ! tst && it != cand.end(); ++ it) {
		    for (j = (*it).first; j < (*it).second && ! tst; ++ j) {
			tst = (reg[j][0] == reg[i][0]);
			for (uint32_t k = d+2; k < width && tst; k += 2)
			    tst = (reg[j][k] == reg[i][k]);
		    }
		}
		if (tst) { // found a matching entry
		    reg[i][d] = reg[j][d];
		    reg[j][0] = UINT_MAX;
		    ++ remove;
		}
		else {
		    j = end;
		}
	    }
	    else {
		for (j = match; j < end; ++j) {
		    tst = (reg[j][0] == reg[i][0]);
		    for (uint32_t k = d+1; tst && k < width; ++ k)
			tst = (reg[j][k] == reg[i][k]);
		    if (tst) {
			reg[i][d] = reg[j][d];
			reg[j][0] = UINT_MAX;
			++ remove;
			break;
		    }
		}
	    }
	    if (j >= end) // fall through the previous loop, i.e., no match
		++ i;
	} // for (uint32_t i = 0; i < reg.size();)
    }
    if (remove == 0) return;

    // the clean up loop -- remove those marked for removal;
    const uint32_t rem = reg.size() - remove;
    for (uint32_t i=0, j=0; i < rem; ++ i) {
	if (reg[i][0] == UINT_MAX) {
	    if (j <= i)
		j = i + 1;
	    while (reg[j][0] == UINT_MAX)
		++ j;
	    for (uint32_t k = 0; k < width; ++ k)
		reg[i][k] = reg[j][k];
	    reg[j][0] = UINT_MAX;
	    ++ j;
	}
    }
    reg.resize(rem); // resize reg to remove the last few elements
} // ibis::meshQuery::mergeNDBlocks

/**
   Assume the records are a linearization of points on a simple regular
   mesh, the function @c getPointsOnBoundary computes all points that
   satisfy the conditions specified by function @c setWhereClause but have
   at least one neighboring mesh point that does not satisfy the
   conditioins.

   @param bdy The return value that contains the list of points.

   @param dim The size of the grid.  The value dim.size() is the dimension
   of the grid.  Each element of bdy is a std::vector with the same size as
   dim.

   @see ibis::meshQuery::getHitsAsBlocks

   @note The inner array has the same number of elements as argument @c dim
   and the dimensions are ordered the same way as in @c dim as well.  All
   functions in this class assumes that the mesh points are linearized
   using a raster scan order where @c dim[0] varies the slowest.
*/
int ibis::meshQuery::getPointsOnBoundary
(std::vector< std::vector<uint32_t> >& bdy,
 const std::vector<uint32_t>& dim) const {
    if (dim.empty()) return -4;
    if (state == FULL_EVALUATE || state == QUICK_ESTIMATE) {
	if (hits == 0) {
	    bdy.clear(); // empty regions of interest
	    return 0;
	}
    }
    else { // no hit vector to work with
	return -3;
    }

    ibis::horometer timer;
    timer.start();

    std::vector< std::vector<uint32_t> > reg;
    int ierr = toBlocks(*hits, dim, reg);
    double t1 = 0;
    if (ibis::gVerbose > 3) {
	timer.stop();
	t1 = timer.realTime();
	timer.resume();
    }

    if (dim.size() == 2) {
	boundary2d(dim, reg, bdy);
    }
    else if (dim.size() == 3) {
	boundary3d(dim, reg, bdy);
    }
    else if (dim.size() > 3) {
	boundarynd(dim, reg, bdy);
    }
    else if (dim.size() == 1) { // 1-d case handle here
	std::vector<uint32_t> tmp(1);
	bdy.reserve(reg.size()*2);
	bdy.clear();
	for (uint32_t i = 0; i < reg.size(); ++ i) {
	    std::vector<uint32_t>& t2 = reg[i];
	    tmp[0] = t2[0];
	    bdy.push_back(tmp);
	    if (t2[1] > t2[0]+1) {
		tmp[0] = t2[1] - 1;
		bdy.push_back(tmp);
	    }
	}
    }

    if (ibis::gVerbose > 2) {
	timer.stop();
	double t2 = timer.realTime();
	ibis::util::logger lg;
	if (dim.size() > 1 && ibis::gVerbose > 3)
	    lg.buffer()
		<< "query[" << id() << "]::getPointsOnBoundary -- extracting "
		<< bdy.size() << " boundary point" << (bdy.size()>1?"s":"")
		<< " from " << reg.size() << " " << dim.size() << "-D block"
		<< (reg.size()>1 ? "s" : "") << " took " << t2-t1
		<< " sec (elapsed)";

	lg.buffer() << "query[" << id() << "]::getPointsOnBoundary -- "
		    << bdy.size() << " point" << (bdy.size()>1?"s":"")
		    << " on a (" << dim[0];
	for (uint32_t i = 1; i < dim.size(); ++ i)
	    lg.buffer() << " x " << dim[i];
	lg.buffer() << " mesh took " << t2 << " sec (elapsed)";
    }
    return ierr;
} // ibis::meshQuery::getPointsOnBoundary

/**
   The variant of getPointsOnBoundary use dimensions returned by
   ibis::table::getMeshShape().

   @see ibis::meshQuery::getPointsOnBoundary
   @see ibis::table::getMeshShape
*/
int ibis::meshQuery::getPointsOnBoundary
(std::vector< std::vector<uint32_t> >& bdy) const {
    if (state == FULL_EVALUATE || state == QUICK_ESTIMATE) {
	if (hits == 0) {
	    bdy.clear(); // empty regions of interest
	    return 0;
	}
    }
    else { // no hit vector to work with
	return -3;
    }

    ibis::horometer timer;
    timer.start();

    const std::vector<uint32_t>& dim = partition()->getMeshShape();
    if (dim.empty()) return -4;

    std::vector< std::vector<uint32_t> > reg;
    int ierr = toBlocks(*hits, dim, reg);
    double t1 = 0;
    if (ibis::gVerbose > 3) {
	timer.stop();
	t1 = timer.realTime();
	timer.resume();
    }

    if (dim.size() == 2) {
	boundary2d(dim, reg, bdy);
    }
    else if (dim.size() == 3) {
	boundary3d(dim, reg, bdy);
	//boundarynd(dim, reg, bdy);
    }
    else if (dim.size() > 3) {
	boundarynd(dim, reg, bdy);
    }
    else if (dim.size() == 1) { // 1-d case handle here
	std::vector<uint32_t> tmp(1);
	bdy.reserve(reg.size()*2);
	bdy.clear();
	for (uint32_t i = 0; i < reg.size(); ++ i) {
	    std::vector<uint32_t>& t2 = reg[i];
	    tmp[0] = t2[0];
	    bdy.push_back(tmp);
	    if (t2[1] > t2[0]+1) {
		tmp[0] = t2[1] - 1;
		bdy.push_back(tmp);
	    }
	}
    }

    if (ibis::gVerbose > 2) {
	timer.stop();
	double t2 = timer.realTime();
	ibis::util::logger lg;
	if (dim.size() > 1 && ibis::gVerbose > 3)
	    lg.buffer()
		<< "query[" << id() << "]::getPointsOnBoundary -- extracting "
		<< bdy.size() << " boundary point" << (bdy.size()>1?"s":"")
		<< " from " << reg.size() << " " << dim.size() << "-D block"
		<< (reg.size()>1 ? "s" : "") << " took " << t2-t1
		<< " sec (elapsed)";

	lg.buffer()
	    << "query[" << id() << "]::getPointsOnBoundary -- extracting "
	    << bdy.size() << " boundary point" << (bdy.size()>1?"s":"")
	    << " from " << hits->cnt() << " hit" << (hits->cnt()>1 ? "s" : "")
	    << " on a (" << dim[0];
	for (uint32_t i = 1; i < dim.size(); ++ i)
	    lg.buffer() << " x " << dim[i];
	lg.buffer() << ") mesh took " << t2 << " sec (elapsed)";
    }
    return ierr;
} // ibis::meshQuery::getPointsOnBoundary

// Given an input list produced by toBlocks, this function extracts points
// that have outside nearest neighbores.
//
// NOTE: it also makes use the fact that all blocks are sorted.
// In describing the algorithm, we assume a mapping of the 2D grid.  The
// first dimension (the slow varying dimension) is assumed to go from south
// to north and the second (fast varying) dimension is assumed to go from
// west to east.  A line segment is part of a horizontal line that only
// varies in east-west direction.  A block may consist multiple lines along
// eat-west direction.
// 
// This implementation uses three pointers to store the starting positions
// of line segments surrounding the current line segment.
void ibis::meshQuery::boundary2d
(const std::vector<uint32_t>& dim,
 const std::vector< std::vector<uint32_t> >& reg,
 std::vector< std::vector<uint32_t> >& bdy) const {
    if (dim.size() != 2) return;
    bdy.clear();
    const uint32_t nreg = reg.size();
    if (nreg == 0) return;
    bdy.reserve(nreg * 2); // a reasonable size to expect, not exact

    std::vector<uint32_t> point(2); // workspace
    uint32_t lineb = 0; // the first line segment on the line before the
    // current line (to the south)
    uint32_t linec = 0; // the first line segment on the current line
    uint32_t linea = 0; // the first line segment on the line after the
    // current one (to the north)

    // the main loop goes through one line segment or block at a time
    for (uint32_t j = 0; j < nreg; ++j) {
	if (j == linea) {	// update linea, lineb, and linec
	    lineb = linec;
	    linec = linea;
	    for (++linea;
		 linea < nreg && reg[linea][0] == reg[j][0];
		 ++linea);
	}

	const std::vector<uint32_t>& seg = reg[j];
	if (reg[lineb][1] == seg[0] &&
	    linea < nreg && reg[linea][0] == seg[1]) {
	    // the three nearest blocks in fact touch each other
	    if (seg[0]+1 == seg[1]) { // it is a line segment
		point[0] = seg[0];
		uint32_t south = lineb;
		uint32_t north = linea;
		// skip to the next line segement that overlaps with
		// the current one
		while (south < linec && reg[south][3] <= seg[2])
		    ++ south;
		while (north < nreg && reg[north][0] == seg[1] &&
		       reg[north][3] <= seg[2])
		    ++ north;
		if (south < linec && reg[south][2] < seg[3] &&
		    north < nreg && reg[north][0] == seg[1] &&
		    reg[north][2] <= seg[3]) { // found overlap
		    uint32_t tmp = (reg[south][2] >= reg[north][2] ?
				    reg[south][2] : reg[north][2]);
		    if (tmp > seg[2]) {
			for (uint32_t i = seg[2]; i < tmp; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }
		    else { // only the west end
			point[1] = seg[2];
			bdy.push_back(point);
		    }

		    while (1) {
			if (reg[south][3] < reg[north][3]) {
			    tmp = reg[south][3];
			    ++ south;
			    if (south >= linec) break;
			    if (reg[south][2] >= seg[3]) break;
			}
			else if (reg[south][3] > reg[north][3]) {
			    tmp = reg[north][3];
			    ++ north;
			    if (north >= nreg) break;
			    if (reg[north][0] > seg[1]) break;
			    if (reg[north][2] >= seg[3]) break;
			}
			else {
			    tmp = reg[south][3];
			    ++ south;
			    ++ north;
			    if (south >= linec) break;
			    if (reg[south][2] >= seg[3]) break;
			    if (north >= nreg) break;
			    if (reg[north][0] > seg[1]) break;
			    if (reg[north][2] >= seg[3]) break;
			}
			if (tmp >= seg[3]) {
			    break;
			}

			if (tmp <= point[1]) // skip points already recorded
			    tmp = point[1] + 1;
			for (uint32_t i = tmp;
			     i < (reg[south][2] >= reg[north][2] ?
				  reg[south][2] : reg[north][2]);
			     ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp <= point[1]) // skip points already recorded
			tmp = point[1] + 1;
		    if (tmp < seg[3]) {
			for (uint32_t i = tmp; i < seg[3]; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[1]+1 < seg[3]) { // east end always exposed
			point[1] = seg[3] - 1;
			bdy.push_back(point);
		    }
		}
		else { // no overlap, the whole line segement is exposed
		    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}
	    }
	    else { // it is a block, the first and last line may be covered
		// deal with the first line
		point[0] = seg[0];
		uint32_t south = lineb;
		// seek a line segement (south) that overlaps with the
		// current one
		while (south < linec && reg[south][3] <= seg[2])
		    ++ south;
		if (south < linec && reg[south][2] < seg[3]) {
		    // found overlap, implicitly reg[outh][3] > seg[2]
		    uint32_t tmp = reg[south][2];
		    if (tmp > seg[2]) {
			for (uint32_t i = seg[2]; i < tmp; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }
		    else { // only the west end
			point[1] = seg[2];
			bdy.push_back(point);
		    }

		    tmp = reg[south][3];
		    while (reg[south][3] < seg[3]) {
			++ south;
			if (south >= linec) break;
			if (reg[south][2] >= seg[3]) break;
			for (uint32_t i = tmp; i < reg[south][2]; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
			tmp = reg[south][3];
		    }
		    if (tmp < seg[3]) {
			for (uint32_t i = tmp; i < seg[3]; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[1]+1 < seg[3]) { // east end always exposed
			point[1] = seg[3] - 1;
			bdy.push_back(point);
		    }
		}
		else { // no overlap, all points exposed
		    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}

		// intermediate lines only exposes two end points
		for (uint32_t i = seg[0]+1; i+1 < seg[1]; ++ i) {
		    point[0] = i;
		    point[1] = seg[2];
		    bdy.push_back(point);
		    if (seg[2]+1 < seg[3]) {
			point[1] = seg[3] - 1;
			bdy.push_back(point);
		    }
		}

		// the last line may connect to some line segements on linea
		point[0] = seg[1] - 1;
		uint32_t north = linea;
		// seek an overlapping line segment
		while (north < nreg && reg[north][0] == seg[1] &&
		       reg[north][3] <= seg[2])
		    ++ north;
		if (north < nreg && reg[north][0] == seg[1] &&
		    reg[north][2] < seg[3]) {
		    // found some overlap, implicitly reg[north][3] > seg[2]
		    uint32_t tmp = reg[north][2];
		    if (tmp > seg[2]) {
			for (uint32_t i = seg[2]; i < tmp; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }
		    else { // only the west end is exposed
			point[1] = seg[2];
			bdy.push_back(point);
		    }

		    tmp = reg[north][3];
		    while (reg[north][3] < seg[3]) {
			++ north;
			if (north >= nreg) break;
			if (reg[north][0] > seg[1]) break;
			if (reg[north][2] >= seg[3]) break;
			for (uint32_t i = tmp; i < reg[north][2]; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
			tmp = reg[north][3];
		    }
		    if (tmp < seg[3]) {
			for (uint32_t i = tmp; i < seg[3]; ++ i) {
			    point[1] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[1]+1 < seg[3]) {
			// east end is always exposed
			point[1] = seg[3] - 1;
			bdy.push_back(point);
		    }
		}
		else {
		    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}
	    }
	}
	else if (seg[0]+1 == seg[1]) {
	    // the three closest blocks don't touch, copy all points of this
	    // line segment
	    point[0] = seg[0];
	    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		point[1] = i;
		bdy.push_back(point);
	    }
	}
	else if (reg[lineb][1] == seg[0]) {
	    // a block with its first line next to some other line segments
	    point[0] = seg[0];
	    uint32_t south = lineb;
	    // seek a line segement (south) that overlaps with the
	    // current one
	    while (south < linec && reg[south][3] <= seg[2])
		++ south;
	    if (south < linec && reg[south][2] < seg[3]) {
		// found overlap, implicitly reg[south][3] > seg[2]
		uint32_t tmp = reg[south][2];
		if (tmp > seg[2]) {
		    for (uint32_t i = seg[2]; i < tmp; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}
		else { // only the west end
		    point[1] = seg[2];
		    bdy.push_back(point);
		}

		tmp = reg[south][3];
		while (reg[south][3] < seg[3]) {
		    ++ south;
		    if (south >= linec) break;
		    if (reg[south][2] >= seg[3]) break;
		    for (uint32_t i = tmp; i < reg[south][2]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		    tmp = reg[south][3];
		}
		if (tmp < seg[3]) {
		    for (uint32_t i = tmp; i < seg[3]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}
		else if (point[1]+1 < seg[3]) {
		    // east end is always exposed
		    point[1] = seg[3] - 1;
		    bdy.push_back(point);
		}
	    }
	    else { // all points exposed
		for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		    point[1] = i;
		    bdy.push_back(point);
		}
	    }

	    // intermediate lines only exposes two end points
	    for (uint32_t i = seg[0]+1; i+1 < seg[1]; ++ i) {
		point[0] = i;
		point[1] = seg[2];
		bdy.push_back(point);
		if (seg[2]+1 < seg[3]) {
		    point[1] = seg[3] - 1;
		    bdy.push_back(point);
		}
	    }

	    // the last line is totally exposed
	    point[0] = seg[1] - 1;
	    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		point[1] = i;
		bdy.push_back(point);
	    }
	}
	else if (linea < nreg && reg[linea][0] == seg[1]) {
	    // a block with its last line next to some other line segments

	    // the first line is totally exposed
	    point[0] = seg[0];
	    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		point[1] = i;
		bdy.push_back(point);
	    }

	    // intermediate lines only exposes two end points
	    for (uint32_t i = seg[0]+1; i+1 < seg[1]; ++ i) {
		point[0] = i;
		point[1] = seg[2];
		bdy.push_back(point);
		if (seg[2]+1 < seg[3]) {
		    point[1] = seg[3] - 1;
		    bdy.push_back(point);
		}
	    }

	    // the last line may connect to some line segements on linea
	    point[0] = seg[1] - 1;
	    uint32_t north = linea;
	    // seek for an overlapping line segment
	    while (north < nreg && reg[north][0] == seg[1] &&
		   reg[north][3] <= seg[2])
		++ north;
	    if (north < nreg && reg[north][0] == seg[1] &&
		reg[north][2] < seg[3]) {
		// found overlap, implicitly reg[north][3] > seg[2]
		uint32_t tmp = reg[north][2];
		if (tmp > seg[2]) {
		    for (uint32_t i = seg[2]; i < tmp; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}
		else { // only the west end is exposed
		    point[1] = seg[2];
		    bdy.push_back(point);
		}

		tmp = reg[north][3];
		while (reg[north][3] < seg[3]) {
		    ++ north;
		    if (north >= nreg) break;
		    if (reg[north][0] > seg[1]) break;
		    if (reg[north][2] >= seg[3]) break;
		    for (uint32_t i = tmp; i < reg[north][2]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		    tmp = reg[north][3];
		}
		if (tmp < seg[3]) {
		    for (uint32_t i = tmp; i < seg[3]; ++ i) {
			point[1] = i;
			bdy.push_back(point);
		    }
		}
		else if (point[1]+1 < seg[3]) { // east end is always exposed
		    point[1] = seg[3] - 1;
		    bdy.push_back(point);
		}
	    }
	    else {
		for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		    point[1] = i;
		    bdy.push_back(point);
		}
	    }
	}
	else { // an isolated block
	    // the first line is totally exposed
	    point[0] = seg[0];
	    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		point[1] = i;
		bdy.push_back(point);
	    }

	    // intermediate lines only exposes two end points
	    for (uint32_t i = seg[0]+1; i+1 < seg[1]; ++ i) {
		point[0] = i;
		point[1] = seg[2];
		bdy.push_back(point);
		if (seg[2]+1 < seg[3]) {
		    point[1] = seg[3] - 1;
		    bdy.push_back(point);
		}
	    }

	    // the last line is also totally exposed
	    point[0] = seg[1] - 1;
	    for (uint32_t i = seg[2]; i < seg[3]; ++ i) {
		point[1] = i;
		bdy.push_back(point);
	    }
	}
    }
} // ibis::meshQuery::boundary2d


// This implementation uses an array of pointers to store the starting
// position of lines -- multiple level of nested loops required to
// accomodate those blocks covering multiple lines
void ibis::meshQuery::boundary2d1
(const std::vector<uint32_t>& dim,
 const std::vector< std::vector<uint32_t> >& reg,
 std::vector< std::vector<uint32_t> >& bdy) const {
    if (dim.size() != 2) return;
    bdy.clear();
    if (reg.size() == 0) return;
    bdy.reserve(reg.size() * 2); // a reasonable size to expect, not exact

    std::vector<uint32_t> point(2); // workspace
    // positions of first line seg in each line.  When a block contains
    // multiple whole lines, the value of start[i] points to the same
    // block.  This means extra checks are required when start[i] ==
    // start[i+1].
    std::vector<uint32_t> start(dim[0]+1);
    for (uint32_t j = 0; j < reg.size();) {
	// record the current position, check to see if any lines are skipped
	const uint32_t lst = (j > 0 ? reg[j-1][1] : 0);
	for (uint32_t i = lst; i < reg[j][1]; ++ i)
	    start[i] = j;
	// skip line segments that are on the same line
	const uint32_t line = reg[j][0];
	for (++j; j < reg.size() && reg[j][0] == line; ++j);
    }
    for (uint32_t i = reg.back()[1]; i <= dim[0]; ++ i)
	start[i] = reg.size();

    for (uint32_t i = 0; i < reg.size(); ++ i) {
	const std::vector<uint32_t>& seg = reg[i];
	for (uint32_t line = seg[0]; line < seg[1]; ++ line) {
	    point[0] = line;
	    if (line > 0 && line+1 < dim[0]) {
		// normal case: needs to look at two neighboring lines
		uint32_t north = start[line+1];
		uint32_t south = start[line-1];
		bool exposed = false;		// completely exposed
		if ((north == start[line+2] && reg[north][0] > seg[1]) ||
		    (south == start[line] && line == seg[0])) {
		    // one of two neighboring lines are empty
		    exposed = true;
		}
		else {
		    // move south and north to find line segments that overlap
		    // with the current one (seg)
		    while (south < start[line] && reg[south][3] <= seg[2])
			++ south;
		    if (south > start[line] ||
			(south == start[line] && seg[0] == line)) {
			exposed = true;
		    }
		    else if (reg[south][2] >= seg[3]) {
			exposed = true;
		    }
		    else { // reg[south][2] < seg[3] && reg[south][3] > seg[2]
			while (north < start[line+2] &&
			       reg[north][3] <= seg[2])
			    ++ north;
			if (north > start[line+2] ||
			    (north == start[line+2] &&
			     reg[north][0] > seg[1])) {
			    exposed = true;
			}
			else if (reg[north][2] >= seg[3]) {
			    exposed = true;
			}
			else {
			    // reg[north][2] < seg[3] && reg[north][3] > seg[2]
			    bool more = true;
			    while (more) {
				if (reg[north][2] < reg[south][3] &&
				    reg[north][3] > reg[south][2]) {
				    // found overlap
				    more = false;
				}
				else if (reg[north][2] >= reg[south][3]) {
				    ++ south;
				    if (south > start[line] ||
					(south == start[line] &&
					 seg[0] == line)) {
					exposed = true;
					more = false;
				    }
				    else if (reg[south][2] >= seg[3]) {
					exposed = true;
					more = false;
				    }
				}
				else if (reg[north][3] <= reg[south][2]) {
				    ++ north;
				    if (north > start[line+2] ||
					(north == start[line+2] &&
					 reg[north][0] > seg[1])) {
					exposed = true;
					more = false;
				    }
				    else if (reg[north][2] >= seg[3]) {
					exposed = true;
					more = false;
				    }
				}
			    } // while (more)
			}
		    }
		}

		if (exposed) { // completely exposed
		    for (uint32_t j = seg[2]; j < seg[3]; ++j) {
			point[1] = j;
			bdy.push_back(point);
		    }
		}
		else { // both neighboring lines have overlapping segments
		    uint32_t tmp = (reg[south][2] >= reg[north][2] ?
				    reg[south][2] : reg[north][2]);
		    if (tmp > seg[2]) {
			for (uint32_t j = seg[2]; j < tmp; ++j) {
			    point[1] = j;
			    bdy.push_back(point);
			}
		    }
		    else {
			point[1] = seg[2];
			bdy.push_back(point);
		    }

		    if (reg[south][3] < reg[north][3]) {
			tmp = reg[south][3];
			++ south;
		    }
		    else if (reg[south][3] > reg[north][3]) {
			tmp = reg[north][3];
			++ north;
		    }
		    else {
			tmp = reg[south][3];
			++ south;
			++ north;
		    }
		    while (tmp < seg[3]) {
			if ((south < start[line] ||
			     (south == start[line] && seg[0] < line)) &&
			    (north < start[line+2] ||
			     (north == start[line+2] &&
			      reg[north][0] <= seg[1]))) {
			    // more line segments to cover the current one
			    uint32_t nxt = (reg[south][2] >= reg[north][2] ?
					    reg[south][2] : reg[north][2]);
			    if (nxt > seg[3])
				nxt = seg[3];
			    if (tmp <= point[1])
				tmp = point[1] + 1;
			    for (uint32_t j = tmp; j < nxt; ++j) {
				point[1] = j;
				bdy.push_back(point);
			    }

			    if (reg[south][3] < reg[north][3]) {
				tmp =  reg[south][3];
				++ south;
			    }
			    else if (reg[south][3] > reg[north][3]) {
				tmp =  reg[north][3];
				++ north;
			    }
			    else {
				tmp =  reg[south][3];
				++ south;
				++ north;
			    }
			}
			else { // no more line segement cover both sides
			    if (tmp <= point[1])
				tmp = point[1] + 1;
			    for (uint32_t j = tmp; j < seg[3]; ++j) {
				point[1] = j;
				bdy.push_back(point);
			    }
			    tmp = seg[3];
			}
		    }
		    if (seg[3] > point[1]+1) {
			// record the last point of this line segment
			point[1] = seg[3] - 1;
			bdy.push_back(point);
		    }
		}
	    }
	    else { // first or last line
		for (uint32_t j = seg[2]; j < seg[3]; ++j) {
		    point[1] = j;
		    bdy.push_back(point);
		}
	    }
	} // for (line ..
    } // for (i ...
} // ibis::meshQuery::boundary2d

// 3D case, the three dimension are named as dim[0]=z, dim[1]=y, dim[2]=x
// This function tracks four line at +/- y, +/- z
void ibis::meshQuery::boundary3d
(const std::vector<uint32_t>& dim,
 const std::vector< std::vector<uint32_t> >& reg,
 std::vector< std::vector<uint32_t> >& bdy) const {
    if (dim.size() != 3) return; // only 3D cases
    bdy.clear();
    if (reg.empty()) return;
    bdy.reserve(reg.size() * 2); // a reasonable size to expect

    const uint32_t nreg = reg.size(); // short-hand for reg.size()
    std::vector<uint32_t> point(3); // workspace
    uint32_t bmy=0; // location of the first line segment on the line at -y
    uint32_t emy=0; // location after the last line segment on the line at -y
    uint32_t bpy=0; // location of the first line segment on the line at +y
    uint32_t epy=0; // location after the last line segment on the line at +y
    uint32_t bmz=0; // location of the first line segment on the line at -z
    uint32_t emz=0; // location after the last line segment on the line at -z
    uint32_t bpz=0; // location of the first line segment on the line at +z
    uint32_t epz=0; // location after the last line segment on the line at +z

    // the main loop through each block stored in reg
    for (uint32_t j = 0; j < nreg; ++j) {
	const std::vector<uint32_t>& seg = reg[j]; // current block

	if (j == bpy) { // step 1: update markers
	    bmy = emy;
	    emy = bpy;
	    bpy = epy;
	    // alway move bpy to the first line segment on the next line
	    for (bpy = (bpy <= j ? j + 1 : bpy);
		 bpy < nreg && reg[bpy][0] == seg[0] && reg[bpy][2] == seg[2];
		 ++ bpy);
	    // epy points to the line segment after the last segment on the
	    // same line as bpy
	    for (epy = (bpy < nreg ? bpy + 1 : nreg);
		 epy < nreg && reg[epy][0] == reg[bpy][0] &&
		     reg[epy][2] == reg[bpy][2];
		 ++ epy);

	    const bool check = (seg[3] > seg[2]+1 ||
				(bpy < nreg && seg[3]-1 == seg[2] &&
				 reg[bpy][0] == seg[0] &&
				 reg[bmy][0] == seg[0] &&
				 reg[bpy][2] == seg[3] &&
				 reg[bmy][3] == seg[2])); 
	    if (seg[0] > 0 && check) {
		// check the neighbors along -z direction
		if (reg[bmz][1] == seg[0] && reg[bmz][2] <= seg[2] &&
		    reg[bmz][3] > seg[2]) {
		    if (emz <= bmz)
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		}
		else if (reg[bmz][1] < seg[0] ||
			 (reg[bmz][1] == seg[0] && reg[bmz][3] <= seg[2])) {
		    // bmz should point to the first line segment on -z line
		    for (bmz = emz;
			 bmz < emy &&
			     (reg[bmz][1] < seg[0] ||
			      (reg[bmz][1] == seg[0] &&
			       reg[bmz][3] <= seg[2]));
			 ++ bmz);
		    if (bmz < emy && reg[bmz][1] == seg[0] &&
			reg[bmz][2] <= seg[2] && reg[bmz][3] > seg[2]) {
			// bmz indeed points to the first line segment on -z
			// line, now find the right value for emz
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		    }
		    else { // no line segment on -z line
			emz = bmz;
		    }
		}
		else if (emz < bmz) {
		    emz = bmz;
		}
		else if (bmz < emz) {
		    bmz = emz;
		}
	    }

	    if (seg[1] < dim[0] && check && bpz < nreg) {
		// check the neighbors along +z directoion
		if (reg[bpz][0] == seg[1] &&
		    reg[bpz][2] <= seg[2] && reg[bpz][3] > seg[2]) {
		    if (epz <= bpz)
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		}
		else if (reg[bpz][0] < seg[1] ||
			 (reg[bpz][0] == seg[1] && reg[bpz][3] <= seg[2])) {
		    // bpz should point to the first line segment on +z line
		    for (bpz = (epz >= bpy ? epz : bpy);
			 bpz < nreg && (reg[bpz][0] < seg[1] ||
					(reg[bpz][0] == seg[1] &&
					 reg[bpz][3] <= seg[2]));
			 ++ bpz);
		    epz = bpz;
		    if (bpz < nreg && reg[bpz][0] == seg[1] &&
			reg[bpz][2] <= seg[2] && reg[bpz][3] > seg[2]) {
			// bpz indeed points to the first line segment on +z
			// line, now find the right value for epz
			for (++ epz;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		    }
		}
	    }
	    else if (epz < bpz) {
		epz = bpz;
	    }
	    else if (bpz < epz) {
		bpz = epz;
	    }
	} // updated markers

	if (bpy < nreg && bpz < nreg && emz > bmz && epz > bpz &&
	    seg[0] > 0 && seg[1] < dim[0] && seg[2] > 0 && seg[3] < dim[1] &&
	    reg[bmy][0] == seg[0] && reg[bmy][3] == seg[2] &&
	    reg[bpy][0] == seg[0] && reg[bpy][2] == seg[3]) {
	    // step 2: the block is in the same plane and is surrounded on
	    // all four sides
	    uint32_t imz = bmz, imy = bmy, ipy = bpy, ipz = bpz;
	    point[0] = seg[0]; // z is definitely fixed in this block
	    if (seg[2] == seg[3]-1) { // step 2a: the block is a line segment
		point[1] = seg[2]; // y is fixed in this block
		// find the maximum of the four left ends
		uint32_t tmp = reg[imz][4];
		if (tmp < reg[imy][4]) tmp = reg[imy][4];
		if (tmp < reg[ipy][4]) tmp = reg[ipy][4];
		if (tmp < reg[ipz][4]) tmp = reg[ipz][4];
		if (tmp > seg[4]) { // add points from seg[4] to tmp
		    if (tmp > seg[5])
			tmp = seg[5];
		    for (uint32_t i = seg[4]; i < tmp; ++i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
		else { // only the left end of this line segement is exposed
		    point[2] = seg[4];
		    bdy.push_back(point);
		}

		while (tmp < seg[5]) { // *** many break statements ***
		    // set tmp to the minimum of the right ends
		    tmp = reg[imz][5];
		    if (tmp > reg[imy][5]) tmp = reg[imy][5];
		    if (tmp > reg[ipy][5]) tmp = reg[ipy][5];
		    if (tmp > reg[ipz][5]) tmp = reg[ipz][5];
		    if (tmp <= point[2]) tmp = point[2] + 1;
		    if (tmp >= reg[imz][5]) {
			for (++ imz; imz < emz && reg[imz][5] <= tmp;
			     ++ imz);
			if (imz >= emz) break;
			if (reg[imz][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[imy][5]) {
			for (++ imy; imy < emy && reg[imy][5] <= tmp;
			     ++ imy);
			if (imy >= emy) break;
			if (reg[imy][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[ipy][5]) {
			for (++ ipy; ipy < epy && reg[ipy][5] <= tmp;
			     ++ ipy);
			if (ipy >= epy) break;
			if (reg[ipy][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[ipz][5]) {
			for (++ ipz; ipz < epz && reg[ipz][5] <= tmp;
			     ++ ipz);
			if (ipz >= epz) break;
			if (reg[ipz][4] >= seg[5]) break;
		    }

		    // set nxt to the maximum of four left ends
		    uint32_t nxt = reg[imz][4];
		    if (nxt < reg[imy][4]) nxt = reg[imy][4];
		    if (nxt < reg[ipy][4]) nxt = reg[ipy][4];
		    if (nxt < reg[ipz][4]) nxt = reg[ipz][4];
		    for (uint32_t i = tmp; i < nxt; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}

		if (tmp < seg[5]) {
		    // points on the last portion of the line segment are
		    // exposed
		    for (uint32_t i = tmp; i < seg[5]; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
		else if (point[2]+1 < seg[5]) {
		    // the right end is alway exposed
		    point[2] = seg[5] - 1;
		    bdy.push_back(point);
		}
	    }
	    else { // step 2b: multiple lines in a z-plane
		// first deal with the first line in the block, need to
		// examine three neighbors at -y and -z and +z
		point[1] = seg[2];
		imy = bmy, imz = bmz, ipz = bpz;
		// assign tmp to be the maximum of the three left ends
		uint32_t tmp = reg[imy][4];
		if (tmp < reg[imz][4]) tmp = reg[imz][4];
		if (tmp < reg[ipz][4]) tmp = reg[ipz][4];
		if (tmp > seg[4]) { // add the first group of points to bdy
		    if (tmp > seg[5]) tmp = seg[5];
		    for (uint32_t i = seg[4]; i < tmp; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
		else { // add the left end to bdy
		    point[2] = seg[4];
		    bdy.push_back(point);
		}

		while (tmp < seg[5]) {
		    // re-assign tmp to be the minimum of the three right
		    // ends
		    tmp = reg[imy][5];
		    if (tmp > reg[imz][5]) tmp = reg[imz][5];
		    if (tmp > reg[ipz][5]) tmp = reg[ipz][5];
		    if (tmp <= point[2]) tmp = point[2] + 1;
		    if (tmp >= reg[imy][5]) {
			for (++ imy; imy < emy && tmp >= reg[imy][5];
			     ++ imy);
			if (imy >= emy) break;
			if (reg[imy][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[imz][5]) {
			for (++ imz; imz < emz && tmp >= reg[imz][5];
			     ++ imz);
			if (imz >= emz) break;
			if (reg[imz][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[ipz][5]) {
			for (++ ipz; ipz < epz && tmp >= reg[ipz][5];
			     ++ ipz);
			if (ipz >= epz) break;
			if (reg[ipz][4] >= seg[5]) break;
		    }

		    // nxt is the maximum of the right ends
		    uint32_t nxt = reg[imy][4];
		    if (nxt < reg[imz][4]) nxt = reg[imz][4];
		    if (nxt < reg[ipz][4]) nxt = reg[ipz][4];
		    for (uint32_t i = tmp; i < nxt; ++ i) {
			// points between tmp and nxt are exposed
			point[2] = i;
			bdy.push_back(point);
		    }
		}

		if (tmp < seg[5]) {
		    // multiple points at the end of the line are exposed
		    for (uint32_t i = tmp; i < seg[5]; ++i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
		else if (point[2]+1 < seg[5]) { // the right end
		    point[2] = seg[5] - 1;
		    bdy.push_back(point);
		}

		// the middle lines of the block
		for (uint32_t k = seg[2]+1; k+1 < seg[3]; ++ k) {
		    point[1] = k;

		    // examine markers along -z
		    if (reg[bmz][1] == seg[0] && reg[bmz][2] <= k &&
			reg[bmz][3] > k) {
			if (emz <= bmz)
			    for (emz = bmz + 1;
				 emz < emy && reg[emz][1] == seg[0] &&
				     reg[emz][2] == reg[bmz][2];
				 ++ emz);
		    }
		    else if (reg[bmz][1] < seg[0] ||
			     (reg[bmz][1] == seg[0] && reg[bmz][3] <= k)) {
			for (bmz = emz;
			     bmz < emy && (reg[bmz][1] < seg[0] ||
					   (reg[bmz][1] == seg[0] &&
					    reg[bmz][3] <= k));
			     ++ bmz);
			if (reg[bmz][1] == seg[0] && reg[bmz][2] <= k &&
			    reg[bmz][3] > k) {
			    for (emz = bmz + 1;
				 emz < emy && reg[emz][1] == seg[0] &&
				     reg[emz][2] == reg[bmz][2];
				 ++ emz);
			}
			else {
			    emz = bmz;
			}
		    }
		    // examine markers along +z
		    if (bpz < nreg && reg[bpz][0] == seg[1] &&
			reg[bpz][2] <= k && reg[bpz][3] > k) {
			if (epz <= bpz)
			    for (epz = bpz + 1;
				 epz < nreg && reg[epz][0] == seg[1] &&
				     reg[epz][2] == reg[bpz][2];
				 ++ epz);
		    }
		    else if (bpz < nreg &&
			     (reg[bpz][0] < seg[1] ||
			      (reg[bpz][0] == seg[1] &&
			       reg[bpz][3] <= k))) {
			for (bpz = (epz >= bpy ? epz : bpy);
			     bpz < nreg && (reg[bpz][0] < seg[1] ||
					    (reg[bpz][0] == seg[1] &&
					     reg[bpz][3] <= k));
			     ++ bpz);
			if (bpz < nreg && reg[bpz][0] == seg[1] &&
			    reg[bpz][3] > k && reg[bpz][2] <= k) {
			    for (epz = bpz + 1;
				 epz < nreg && reg[epz][0] == seg[1] &&
				     reg[epz][2] == reg[bpz][2];
				 ++ epz);
			}
			else {
			    epz = bpz;
			}
		    }

		    // check to see whether line segments along +/- z still
		    // cover the current line segment
		    if (bpz < epz && bmz < emz) { // cover
			imz = bmz, ipz = bpz;
			tmp = (reg[imz][4] >= reg[ipz][4] ?
			       reg[imz][4] : reg[ipz][4]);
			if (tmp > seg[4]) {
			    if (tmp > seg[5]) tmp = seg[5];
			    for (uint32_t i = seg[4]; i < tmp; ++ i) {
				point[2] = i;
				bdy.push_back(point);
			    }
			}
			else {
			    point[2] = seg[4];
			    bdy.push_back(point);
			}

			while (tmp < seg[5]) {
			    tmp = (reg[imz][5] <= reg[ipz][5] ?
				   reg[imz][5] : reg[ipz][5]);
			    if (tmp <= point[2]) tmp = point[2] + 1;
			    if (tmp >= reg[imz][5]) {
				for (++ imz;
				     imz < emz && tmp >= reg[imz][5];
				     ++ imz);
				if (imz >= emz) break;
				if (reg[imz][4] >= seg[5]) break;
			    }
			    if (tmp >= reg[ipz][5]) {
				for (++ ipz;
				     ipz < epz && tmp >= reg[ipz][5];
				     ++ ipz);
				if (ipz >= epz) break;
				if (reg[ipz][4] >= seg[5]) break;
			    }

			    uint32_t nxt = (reg[imz][4] >= reg[ipz][4] ?
					    reg[imz][4] : reg[ipz][4]);
			    for (uint32_t i = tmp; i < nxt; ++ i) {
				point[2] = i;
				bdy.push_back(point);
			    }
			}

			if (tmp < seg[5]) {
			    for (uint32_t i = tmp; i < seg[5]; ++ i) {
				point[2] = i;
				bdy.push_back(point);
			    }
			}
			else if (point[2]+1 < seg[5]) {
			    point[2] = seg[5] - 1;
			    bdy.push_back(point);
			}
		    }
		    else { // the line segment is total exposed
			for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		}

		// the last line of the block, need to deal with three
		// lines, +y, +z, and -z
		point[1] = seg[3] - 1;

		// examine markers along -z
		if (reg[bmz][1] == seg[0] && reg[bmz][2] <= point[1] &&
		    reg[bmz][3] > point[1]) {
		    if (emz <= bmz)
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		}
		else if (reg[bmz][1] < seg[0] ||
			 (reg[bmz][1] == seg[0] && reg[bmz][3] <= point[1])) {
		    for (bmz = emz;
			 bmz < emy && (reg[bmz][1] < seg[0] ||
				       (reg[bmz][1] == seg[0] &&
					reg[bmz][3] <= point[1]));
			 ++ bmz);
		    if (bmz < emy && reg[bmz][1] == seg[0] &&
			reg[bmz][3] > point[1] && reg[bmz][2] <= point[1]) {
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		    }
		    else {
			emz = bmz;
		    }
		}
		// examine markers along +z
		if (bpz < nreg && reg[bpz][0] == seg[1] &&
		    reg[bpz][2] <= point[1] && reg[bpz][3] > point[1]) {
		    if (epz <= bpz)
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		}
		else if (bpz < nreg &&
			 (reg[bpz][0] < seg[1] ||
			  (reg[bpz][0] == seg[1] &&
			   reg[bpz][3] <= point[1]))) {
		    for (bpz = (epz >= bpy ? epz : bpy);
			 bpz < nreg && (reg[bpz][0] < seg[1] ||
					(reg[bpz][0] == seg[1] &&
					 reg[bpz][3] <= point[1]));
			 ++ bpz);
		    if (bpz < nreg && reg[bpz][0] == seg[1] &&
			reg[bpz][3] > point[1] &&
			reg[bpz][2] <= point[1]) {
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		    }
		    else {
			epz = bpz;
		    }
		}

		if (bpz < epz && bmz < emz) {
		    // found line segments on three lines along -z, +y, +z
		    imz = bmz, ipz = bpz, ipy = bpy;
		    tmp = reg[imz][4];
		    if (tmp < reg[ipy][4]) tmp = reg[ipy][4];
		    if (tmp < reg[ipz][4]) tmp = reg[ipz][4];
		    if (tmp > seg[4]) {
			if (tmp > seg[5]) tmp = seg[5];
			for (uint32_t i = seg[4]; i < tmp; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else {
			point[2] = seg[4];
			bdy.push_back(point);
		    }

		    while (tmp < seg[5]) {
			// assign tmp to be the smaller of element xx[5]
			tmp = reg[imz][5];
			if (tmp > reg[ipy][5]) tmp = reg[ipy][5];
			if (tmp > reg[ipz][5]) tmp = reg[ipz][5];
			if (tmp <= point[2]) tmp = point[2] + 1;
			if (tmp >= reg[imz][5]) {
			    for (++ imz;
				 imz < emz && tmp >= reg[imz][5];
				 ++ imz);
			    if (imz >= emz) break;
			    if (reg[imz][4] >= seg[5]) break;
			}
			if (tmp >= reg[ipy][5]) {
			    for (++ ipy;
				 ipy < epy && tmp >= reg[ipy][5];
				 ++ ipy);
			    if (ipy >= epy) break;
			    if (reg[ipy][4] >= seg[5]) break;
			}
			if (tmp >= reg[ipz][5]) {
			    for (++ ipz;
				 ipz < epz && tmp >= reg[ipz][5];
				 ++ ipz);
			    if (ipz >= epz) break;
			    if (reg[ipz][4] >= seg[5]) break;
			}

			uint32_t nxt = reg[imz][4]; // max of xx[4]
			if (nxt < reg[ipy][4]) nxt = reg[ipy][4];
			if (nxt < reg[ipz][4]) nxt = reg[ipz][4];
			for (uint32_t i = tmp; i < nxt; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp < seg[5]) {
			for (uint32_t i = tmp; i < seg[5]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[2]+1 < seg[5]) {
			point[2] = seg[5] - 1;
			bdy.push_back(point);
		    }
		}
		else {
		    for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
	    }
	}
	else if (seg[0] == seg[1]-1 && seg[2] == seg[3]-1) {
	    // step 3a: the block is a line segment, all points are exposed
	    point[0] = seg[0];
	    point[1] = seg[2];
	    for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
		point[2] = i;
		bdy.push_back(point);
	    }
	}
	else if (seg[0] == seg[1]-1) { // step 3b: multiple lines in a plane
	    point[0] = seg[0];
	    // deal with the first line, need to look at three directions,
	    // -y and +/-z
	    point[1] = seg[2];
	    if (bmz < emz && bpz < epz && reg[bmy][0] == seg[0] &&
		reg[bmy][3] == seg[2]) { // covered
		uint32_t imy = bmy, imz = bmz, ipz = bpz;
		// assign tmp to be the maximum of the three left ends
		uint32_t tmp = reg[imy][4];
		if (tmp < reg[imz][4]) tmp = reg[imz][4];
		if (tmp < reg[ipz][4]) tmp = reg[ipz][4];
		if (tmp > seg[4]) { // add the first group of points to bdy
		    if (tmp > seg[5]) tmp = seg[5];
		    for (uint32_t i = seg[4]; i < tmp; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
		else { // add the left end to bdy
		    point[2] = seg[4];
		    bdy.push_back(point);
		}

		while (tmp < seg[5]) {
		    // re-assign tmp to be the minimum of the three right
		    // ends
		    tmp = reg[imy][5];
		    if (tmp > reg[imz][5]) tmp = reg[imz][5];
		    if (tmp > reg[ipz][5]) tmp = reg[ipz][5];
		    if (tmp <= point[2]) tmp = point[2] + 1;
		    if (tmp >= reg[imy][5]) {
			for (++ imy; imy < emy && tmp >= reg[imy][5];
			     ++ imy);
			if (imy >= emy) break;
			if (reg[imy][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[imz][5]) {
			for (++ imz; imz < emz && tmp >= reg[imz][5];
			     ++ imz);
			if (imz >= emz) break;
			if (reg[imz][4] >= seg[5]) break;
		    }
		    if (tmp >= reg[ipz][5]) {
			for (++ ipz; ipz < epz && tmp >= reg[ipz][5];
			     ++ ipz);
			if (ipz >= epz) break;
			if (reg[ipz][4] >= seg[5]) break;
		    }

		    // nxt is the maximum of the right ends
		    uint32_t nxt = reg[imy][4];
		    if (nxt < reg[imz][4]) nxt = reg[imz][4];
		    if (nxt < reg[ipz][4]) nxt = reg[ipz][4];
		    if (nxt > seg[5]) nxt = seg[5];
		    for (uint32_t i = tmp; i < nxt; ++ i) {
			// points between tmp and nxt are exposed
			point[2] = i;
			bdy.push_back(point);
		    }
		}

		if (tmp < seg[5]) {
		    // multiple points at the end of the line are exposed
		    for (uint32_t i = tmp; i < seg[5]; ++i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
		else if (point[2]+1 < seg[5]) { // the right end
		    point[2] = seg[5] - 1;
		    bdy.push_back(point);
		}
	    }
	    else { // all points on the first line are exposed
		for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
		    point[2] = i;
		    bdy.push_back(point);
		}
	    }

	    // the middle lines of the block
	    for (uint32_t k = seg[2]+1; k+1 < seg[3]; ++ k) {
		point[1] = k;

		// examine markers along -z
		if (reg[bmz][1] == seg[0] && reg[bmz][2] <= k &&
		    reg[bmz][3] > k) {
		    if (emz <= bmz)
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		}
		else if (reg[bmz][1] < seg[0] ||
			 (reg[bmz][1] == seg[0] && reg[bmz][3] <= k)) {
		    for (bmz = emz;
			 bmz < emy && (reg[bmz][1] < seg[0] ||
				       (reg[bmz][1] == seg[0] &&
					reg[bmz][3] <= k));
			 ++ bmz);
		    if (reg[bmz][1] == seg[0] && reg[bmz][2] <= k &&
			reg[bmz][3] > k) {
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		    }
		    else {
			emz = bmz;
		    }
		}
		// examine markers along +z
		if (bpz < nreg && reg[bpz][0] == seg[1] &&
		    reg[bpz][2] <= k && reg[bpz][3] > k) {
		    if (epz <= bpz)
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		}
		else if (bpz < nreg &&
			 (reg[bpz][0] < seg[1] ||
			  (reg[bpz][0] == seg[1] && reg[bpz][3] <= k))) {
		    for (bpz = (epz >= bpy ? epz : bpy);
			 bpz < nreg && (reg[bpz][0] < seg[1] ||
					(reg[bpz][0] == seg[1] &&
					 reg[bpz][3] <= k));
			 ++ bpz);
		    if (bpz < nreg && reg[bpz][0] == seg[1] &&
			reg[bpz][3] > k && reg[bpz][2] <= k) {
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		    }
		    else {
			epz = bpz;
		    }
		}

		// check to see whether line segments along +/- z still
		// cover the current line segment
		if (bpz < epz && bmz < emz) { // covered
		    uint32_t imz = bmz, ipz = bpz;
		    uint32_t tmp = (reg[imz][4] >= reg[ipz][4] ?
				    reg[imz][4] : reg[ipz][4]);
		    if (tmp > seg[4]) {
			if (tmp > seg[5]) tmp = seg[5];
			for (uint32_t i = seg[4]; i < tmp; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else {
			point[2] = seg[4];
			bdy.push_back(point);
		    }

		    while (tmp < seg[5]) {
			tmp = (reg[imz][5] <= reg[ipz][5] ?
			       reg[imz][5] : reg[ipz][5]);
			if (tmp <= point[2]) tmp = point[2] + 1;
			if (tmp >= reg[imz][5]) {
			    for (++ imz; imz < emz && tmp >= reg[imz][5];
				 ++ imz);
			    if (imz >= emz) break;
			    if (reg[imz][4] >= seg[5]) break;
			}
			if (tmp >= reg[ipz][5]) {
			    for (++ ipz; ipz < epz && tmp >= reg[ipz][5];
				 ++ ipz);
			    if (ipz >= epz) break;
			    if (reg[ipz][4] >= seg[5]) break;
			}

			uint32_t nxt = (reg[imz][4] >= reg[ipz][4] ?
					reg[imz][4] : reg[ipz][4]);
			for (uint32_t i = tmp; i < nxt; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp < seg[5]) {
			for (uint32_t i = tmp; i < seg[5]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[2]+1 < seg[5]) {
			point[2] = seg[5] - 1;
			bdy.push_back(point);
		    }
		}
		else { // the line segment is total exposed
		    for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
	    } // loop for the middle lines

	    // the last line of the block, need to deal with three lines
	    // along +y, +z, and -z
	    point[1] = seg[3] - 1;
	    if (bpz < nreg && bpy < nreg && reg[bpy][0] == seg[0] &&
		reg[bpy][2] == seg[3]) {
		// examine markers along -z
		if (reg[bmz][1] == seg[0] && reg[bmz][2] <= point[1] &&
		    reg[bmz][3] > point[1]) {
		    if (emz <= bmz)
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		}
		else if (reg[bmz][1] < seg[0] ||
			 (reg[bmz][1] == seg[0] && reg[bmz][3] <= point[1])) {
		    for (bmz = emz;
			 bmz < emy && (reg[bmz][1] < seg[0] ||
				       (reg[bmz][1] == seg[0] &&
					reg[bmz][3] <= point[1]));
			 ++ bmz);
		    if (reg[bmz][1] == seg[0] && reg[bmz][3] > point[1] &&
			reg[bmz][2] <= point[1]) {
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		    }
		    else {
			emz = bmz;
		    }
		}
		// examine markers along +z
		if (bpz < nreg && reg[bpz][0] == seg[1] &&
		    reg[bpz][2] <= point[1] && reg[bpz][3] > point[1]) {
		    if (epz <= bpz)
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		}
		else if (bpz < nreg && (reg[bpz][0] < seg[1] ||
					(reg[bpz][0] == seg[1] &&
					 reg[bpz][3] <= point[1]))) {
		    for (bpz = (epz >= bpy ? epz : bpy);
			 bpz < nreg && (reg[bpz][0] < seg[1] ||
					(reg[bpz][0] == seg[1] &&
					 reg[bpz][3] <= point[1]));
			 ++ bpz);
		    if (bpz < nreg && reg[bpz][0] == seg[1] &&
			reg[bpz][3] > point[1] &&
			reg[bpz][2] <= point[1]) {
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		    }
		    else {
			epz = bpz;
		    }
		}

		if (bpz < epz && bmz < emz) {
		    uint32_t imz = bmz, ipz = bpz, ipy = bpy;
		    uint32_t tmp = reg[imz][4];
		    if (tmp < reg[ipy][4]) tmp = reg[ipy][4];
		    if (tmp < reg[ipz][4]) tmp = reg[ipz][4];
		    if (tmp > seg[4]) {
			if (tmp > seg[5]) tmp = seg[5];
			for (uint32_t i = seg[4]; i < tmp; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else {
			point[2] = seg[4];
			bdy.push_back(point);
		    }

		    while (tmp < seg[5]) {
			tmp = reg[imz][5];
			if (tmp > reg[ipy][5]) tmp = reg[ipy][5];
			if (tmp > reg[ipz][5]) tmp = reg[ipz][5];
			if (tmp <= point[2]) tmp = point[2] + 1;
			if (tmp >= reg[imz][5]) {
			    for (++ imz;
				 imz < emz && tmp >= reg[imz][5];
				 ++ imz);
			    if (imz >= emz) break;
			    if (reg[imz][4] >= seg[5]) break;
			}
			if (tmp >= reg[ipy][5]) {
			    for (++ ipy;
				 ipy < epy && tmp >= reg[ipy][5];
				 ++ ipy);
			    if (ipy >= epy) break;
			    if (reg[ipy][4] >= seg[5]) break;
			}
			if (tmp >= reg[ipz][5]) {
			    for (++ ipz;
				 ipz < epz && tmp >= reg[ipz][5];
				 ++ ipz);
			    if (ipz >= epz) break;
			    if (reg[ipz][4] >= seg[5]) break;
			}

			uint32_t nxt = reg[imz][4];
			if (nxt < reg[ipy][4]) nxt = reg[ipy][4];
			if (nxt < reg[ipz][4]) nxt = reg[ipz][4];
			for (uint32_t i = tmp; i < nxt; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp < seg[5]) {
			for (uint32_t i = tmp; i < seg[5]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[2]+1 < seg[5]) {
			point[2] = seg[5] - 1;
			bdy.push_back(point);
		    }
		}
		else {
		    for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
	    }
	    else {
		for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
		    point[2] = i;
		    bdy.push_back(point);
		}
	    }
	}
	else { // step 3c: a block that spans multiple planes
	    // deal with the first plane
	    point[0] = seg[0];
	    // the first line is definitely exposed
	    point[1] = seg[2];
	    for (uint32_t i = seg[4];  i < seg[5]; ++ i) {
		point[2] = i;
		bdy.push_back(point);
	    }

	    // dealing with the middle lines of the first plane
	    for (uint32_t k = seg[2]+1; k+1 < seg[3]; ++ k) {
		point[1] = k;

		// examine markers along -z
		if (reg[bmz][1] == seg[0] && reg[bmz][2] <= k &&
		    reg[bmz][3] > k) {
		    if (emz <= bmz)
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		}
		else if (reg[bmz][1] < seg[0] ||
			 (reg[bmz][1] == seg[0] && reg[bmz][3] <= k)) {
		    for (bmz = emz;
			 bmz < emy && (reg[bmz][1] < seg[0] ||
				       (reg[bmz][1] == seg[0] &&
					reg[bmz][3] <= k));
			 ++ bmz);
		    if (reg[bmz][1] == seg[0] && reg[bmz][2] <= k &&
			reg[bmz][3] > k) {
			for (emz = bmz + 1;
			     emz < emy && reg[emz][1] == seg[0] &&
				 reg[emz][2] == reg[bmz][2];
			     ++ emz);
		    }
		    else {
			emz = bmz;
		    }
		}

		// check to see whether line segments along -z still
		// cover the current line segment
		if (bmz < emz) { // cover
		    uint32_t imz = bmz;
		    uint32_t tmp = reg[imz][4];
		    if (tmp > seg[4]) { // first part of the line exposed
			if (tmp > seg[5]) tmp = seg[5];
			for (uint32_t i = seg[4]; i < tmp; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else { // only the the first point is exposed
			point[2] = seg[4];
			bdy.push_back(point);
		    }

		    while (tmp < seg[5]) {
			tmp = reg[imz][5];
			if (tmp <= point[2]) tmp = point[2] + 1;
			++ imz;
			if (imz >= emz) break;
			if (reg[imz][4] >= seg[5]) break;

			for (uint32_t i = tmp; i < reg[imz][4]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp < seg[5]) { // last part of the line exposed
			for (uint32_t i = tmp; i < seg[5]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[2] < seg[5]-1) { // add the last point
			point[2] = seg[5] - 1;
			bdy.push_back(point);
		    }
		}
		else { // the line segment is total exposed
		    for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
	    } // loop for middle lines of the first plane

	    // the last line of the first plane is exposed
	    point[1] = seg[3] - 1;
	    for (uint32_t i = seg[4];  i < seg[5]; ++ i) {
		point[2] = i;
		bdy.push_back(point);
	    }

	    // the planes in the middle
	    for (uint32_t p = seg[0]+1; p+1 < seg[1]; ++ p) {
		point[0] = p;
		// the first line is exposed
		point[1] = seg[2];
		for (uint32_t i = seg[4];  i < seg[5]; ++ i) {
		    point[2] = i;
		    bdy.push_back(point);
		}

		// the end points of the middle lines are posed
		for (uint32_t k = seg[2]+1; k+1 < seg[3]; ++ k) {
		    point[1] = k;
		    point[2] = seg[4];
		    bdy.push_back(point);
		    if (seg[4] < seg[5]-1) {
			point[2] = seg[5] - 1;
			bdy.push_back(point);
		    }
		}

		// the last line is exposed
		point[1] = seg[3] - 1;
		for (uint32_t i = seg[4];  i < seg[5]; ++ i) {
		    point[2] = i;
		    bdy.push_back(point);
		}
	    } // the planes in the middle

	    // the last plane
	    point[0] = seg[1] - 1;
	    // the first line of the last plane is exposed
	    point[1] = seg[2];
	    for (uint32_t i = seg[4];  i < seg[5]; ++ i) {
		point[2] = i;
		bdy.push_back(point);
	    }

	    // dealing with the middle lines of the last plane
	    for (uint32_t k = seg[2]+1; k+1 < seg[3]; ++ k) {
		point[1] = k;

		// examine markers along +z
		if (bpz < nreg && reg[bpz][0] == seg[1] &&
		    reg[bpz][2] <= k && reg[bpz][3] > k) {
		    if (epz <= bpz)
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		}
		else if (bpz < nreg &&
			 (reg[bpz][0] < seg[1] ||
			  (reg[bpz][0] == seg[1] && reg[bpz][3] <= k))) {
		    for (bpz = (epz >= bpy ? epz : bpy);
			 bpz < nreg && (reg[bpz][0] < seg[1] ||
					(reg[bpz][0] == seg[1] &&
					 reg[bpz][3] <= k));
			 ++ bpz);
		    if (bpz < nreg && reg[bpz][0] == seg[1] &&
			reg[bpz][3] > k && reg[bpz][2] <= k) {
			for (epz = bpz + 1;
			     epz < nreg && reg[epz][0] == seg[1] &&
				 reg[epz][2] == reg[bpz][2];
			     ++ epz);
		    }
		    else {
			epz = bpz;
		    }
		}

		// check to see whether line segments along +z still
		// cover the current line segment
		if (bpz < epz) { // cover
		    uint32_t ipz = bpz;
		    uint32_t tmp = reg[ipz][4];
		    if (tmp > seg[4]) {
			if (tmp > seg[5]) tmp = seg[5];
			for (uint32_t i = seg[4]; i < tmp; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else {
			point[2] = seg[4];
			bdy.push_back(point);
		    }

		    while (tmp < seg[5]) {
			tmp = reg[ipz][5];
			if (tmp <= point[2]) tmp = point[2] + 1;
			++ ipz;
			if (ipz >= epz) break;
			if (reg[ipz][4] >= seg[5]) break;

			for (uint32_t i = tmp; i < reg[ipz][4]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp < seg[5]) {
			for (uint32_t i = tmp; i < seg[5]; ++ i) {
			    point[2] = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point[2] < seg[5]-1) {
			point[2] = seg[5] - 1;
			bdy.push_back(point);
		    }
		}
		else { // the line segment is total exposed
		    for (uint32_t i = seg[4]; i < seg[5]; ++ i) {
			point[2] = i;
			bdy.push_back(point);
		    }
		}
	    } // loop for middle lines of the first plane

	    // the last line of the last plane is exposed
	    point[1] = seg[3] - 1;
	    for (uint32_t i = seg[4];  i < seg[5]; ++ i) {
		point[2] = i;
		bdy.push_back(point);
	    }
	}
    } // end of the main loop
} // ibis::meshQuery::boundary3d

void ibis::meshQuery::boundarynd
(const std::vector<uint32_t>& dim,
 const std::vector< std::vector<uint32_t> >& reg,
 std::vector< std::vector<uint32_t> >& bdy) const {
    if (dim.size() < 3) return; // dimension must be higher than 2
    bdy.clear();
    if (reg.empty()) return;
    bdy.reserve(reg.size() * 2); // a reasonable size to expect

    const uint32_t nreg = reg.size();	// a short-hand
    const uint32_t ndim = dim.size();
    const uint32_t ndm1 = ndim - 1;
    const uint32_t tdm1 = ndm1 + ndm1;
    const uint32_t ndm2 = ndim - 2;
    const uint32_t tdm2 = ndm2 + ndm2;
    uint32_t bpre = 0; // markers for the line segments on the line
    uint32_t epre = 0; // preceding the current line segment
    uint32_t bfol = 0; // markers for the line segments on the line
    uint32_t efol = 0; // following the current line segment
    std::vector<uint32_t> mrkbm(ndm2, 0);
    std::vector<uint32_t> mrkem(ndm2, 0);
    std::vector<uint32_t> mrkbp(ndm2, 0);
    std::vector<uint32_t> mrkep(ndm2, 0);
    std::vector<uint32_t> point(dim.size()); // workspace

    // the main loop through each block in the vector reg
    for (uint32_t j = 0; j < nreg; ++ j) {
	bool covered = true;
	const std::vector<uint32_t>& seg = reg[j]; // the current block
	if (j == bfol) { // need to update the four markers
	    bpre = epre;
	    epre = bfol;
	    for (bfol = efol; bfol < nreg; ++ bfol) {
		bool same = true;
		for (uint32_t d = 0; d < tdm1 && same; d+=2)
		    same = (reg[bfol][d] == seg[d]);
		if (! same) break; // found the new line
	    }
	    for (efol = (bfol < nreg ? bfol + 1 : nreg); efol < nreg;
		 ++ efol) {
		bool same = true;
		for (uint32_t d = 0; d < tdm1 && same; d+=2)
		    same = (reg[efol][d] == reg[bfol][d]);
		if (! same) break;
	    }
	}

	// count the dimensions that are trivial
	uint32_t sdim = 0;
	while (sdim < tdm1 && seg[sdim]+1 == seg[sdim+1])
	    sdim += 2;
	sdim >>= 1; // divide by two

	if (sdim == ndm1) { // CASE I: all points in a line
	    for (uint32_t i = 0; i < ndm1; ++ i)
		point[i] = seg[i+i];

	    covered = (bfol < nreg);
	    for (uint32_t d = 0; d < tdm2 && covered; d+=2)
		covered = (seg[d] > 0 && seg[d+1] < dim[(d>>1)] &&
			   reg[bpre][d]<=seg[d] && reg[bfol][d]<=seg[d] &&
			   reg[bpre][d+1]>seg[d] && reg[bfol][d+1]>seg[d]);
	    if (covered)
		covered = (reg[bpre][tdm2+1]==seg[tdm2] &&
			   reg[bfol][tdm2]==seg[tdm2+1]);
	    // deal with markers from - dim[b] direction
	    for (uint32_t b = 0; b < ndm2 && covered; ++ b) {
		uint32_t marker = mrkbm[b];
		covered = (mrkem[b] > marker);
		for (uint32_t d = 0; d < ndm1 && covered; ++ d) {
		    if (d != b)
			covered = (reg[marker][d+d] <= seg[d+d] &&
				   reg[marker][d+d+1] > seg[d+d]);
		    else
			covered = (reg[marker][d+d+1] == seg[d+d]);
		}
		if (! covered) { // attempt to move mrkbm[b]
		    bool more = true;
		    for (marker = (b > 0 && mrkem[b] < mrkem[b-1] ?
				   mrkem[b-1] : mrkem[b]);
			 more && marker < epre; marker += more) {
			more = false;
			for (uint32_t d = 0; !more && d < tdm1; d+=2) {
			    if (d != b+b) {
				if (reg[marker][d] > seg[d]) break;
				more = (reg[marker][d+1] <= seg[d]);
			    }
			    else {
				if (reg[marker][d+1] > seg[d]) break;
				more = (reg[marker][d+1] < seg[d]);
			    }
			}
		    }

		    covered = true;
		    mrkbm[b] = marker;
		    mrkem[b] = marker;
		    for (uint32_t d = 0; d < tdm1 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d+1] == seg[d]);
		    }
		    if (covered) { // update mrkem[b]
			more = true;
			for (++ mrkem[b]; mrkem[b] < epre && more;
			     mrkem[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; d+=2)
				more = (reg[mrkem[b]][d] == reg[marker][d]);
			}
		    }
		}
		else if (mrkem[b] <= marker) { // update mrkem[b]
		    bool more = true;
		    for (mrkem[b] = marker+1; mrkem[b] < epre && more;
			 mrkem[b] += more) {
			for (uint32_t d = 0; d < tdm1 && more; d+=2)
			    more = (reg[mrkem[b]][d] == reg[marker][d]);
		    }
		}
	    }

	    // deal with markers from + dim[b] direction
	    for (uint32_t b = ndm2; b > 0 && covered;) {
		-- b;
		uint32_t marker = mrkbp[b];
		covered = (mrkbp[b] < nreg && mrkep[b] > mrkbp[b]);
		for (uint32_t d = 0; d < tdm1 && covered; d+=2) {
		    if (d != b+b)
			covered = (reg[marker][d] <= seg[d] &&
				   reg[marker][d+1] > seg[d]);
		    else
			covered = (reg[marker][d] == seg[d+1]);
		}
		if (! covered) { // attempt to move mrkbp[b]
		    bool more = true;
		    if (b+1 < ndm2)
			marker = (mrkep[b+1] > mrkep[b] ?
				  mrkep[b+1] : mrkep[b]);
		    else if (mrkep[b] < bfol)
			marker = bfol;
		    else
			marker = mrkep[b];
		    for (; marker < nreg && more; marker += more) {
			more = false;
			for (uint32_t d = 0; d < tdm1 && !more; d+=2) {
			    if (d != b+b) {
				if (reg[marker][d] > seg[d]) break;
				more = (reg[marker][d+1] <= seg[d]);
			    }
			    else {
				if (reg[marker][d] > seg[d+1]) break;
				more = (reg[marker][d] < seg[d+1]);
			    }
			}
		    }

		    covered = (marker < nreg);
		    mrkbp[b] = marker;
		    mrkep[b] = marker;
		    for (uint32_t d = 0; d < tdm1 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d] == seg[d+1]);
		    }
		    if (covered) { // update mrkep[b]
			more = true;
			for (++ mrkep[b]; mrkep[b] < nreg && more;
			     mrkep[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; ++ d)
				more = (reg[mrkep[b]][d] == reg[marker][d]);
			}
		    }
		}
		else if (mrkep[b] <= marker) { // update mrkep[b]
		    bool more = true;
		    for (mrkep[b] = marker+1; mrkep[b] < nreg && more;
			 mrkep[b] += more) {
			for (uint32_t d = 0; d < tdm1 && more; d+=2)
			    more = (reg[mrkep[b]][d] == reg[marker][d]);
		    }
		}
	    }

	    if (covered) {
		// the current line segment is surronded on all sides
		uint32_t ipre = bpre;
		uint32_t ifol = bfol;
		std::vector<uint32_t> ilm(mrkbm);
		std::vector<uint32_t> ilp(mrkbp);
		// assigned tmp to be the maximum of xx[tdm1]
		uint32_t tmp = reg[bpre][tdm1];
		if (reg[ifol][tdm1] > tmp) tmp = reg[ifol][tdm1];
		for (uint32_t d = 0; d < ndm2; ++ d) {
		    if (reg[ilm[d]][tdm1] > tmp) tmp = reg[ilm[d]][tdm1];
		    if (reg[ilp[d]][tdm1] > tmp) tmp = reg[ilp[d]][tdm1];
		}
		if (tmp > seg[tdm1]) {
		    if (tmp > seg.back()) tmp = seg.back();
		    for (uint32_t i = seg[tdm1]; i < tmp; ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
		else {
		    point.back() = seg[tdm1];
		    bdy.push_back(point);
		}

		while (tmp < seg.back()) {
		    // re-assign tmp to the minimum of xx.back()
		    tmp = reg[ipre].back();
		    if (tmp > reg[ifol].back()) tmp = reg[ifol].back();
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (tmp > reg[ilm[d]].back())
			    tmp = reg[ilm[d]].back();
			if (tmp > reg[ilp[d]].back())
			    tmp = reg[ilp[d]].back();
		    }
		    if (tmp <= point.back()) tmp = point.back() + 1;
		    if (reg[ipre].back() <= tmp) {
			for (++ ipre;
			     ipre < epre && reg[ipre].back() <= tmp;
			     ++ ipre);
			if (ipre >= epre) break;
			if (reg[ipre][tdm1] >= seg.back()) break;
		    }
		    if (reg[ifol].back() <= tmp) {
			for (++ ifol;
			     ifol < efol && reg[ifol].back() <= tmp;
			     ++ ifol);
			if (ifol >= efol) break;
			if (reg[ifol][tdm1] >= seg.back()) break;
		    }
		    bool more = true;
		    for (uint32_t d = 0; d < ndm2 && more; ++ d) {
			if (reg[ilm[d]].back() <= tmp) {
			    for (++ ilm[d];
				 ilm[d] < mrkem[d] &&
				     reg[ilm[d]].back() <= tmp;
				 ++ ilm[d]);
			    more = (ilm[d] < mrkem[d] &&
				    reg[ilm[d]][tdm1] < seg.back());
			}
			if (reg[ilp[d]].back() <= tmp && more) {
			    for (++ ilp[d];
				 ilp[d] < mrkep[d] &&
				     reg[ilp[d]].back() <= tmp;
				 ++ ilp[d]);
			    more = (ilp[d] < mrkep[d] &&
				    reg[ilp[d]][tdm1] < seg.back());
			}
		    }
		    if (! more) break;

		    // assign nxt to the maximum of xx[tdm1]
		    uint32_t nxt = reg[ipre][tdm1];
		    if (nxt < reg[ifol][tdm1]) nxt = reg[ifol][tdm1];
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (nxt < reg[ilm[d]][tdm1])
			    nxt = reg[ilm[d]][tdm1];
			if (nxt < reg[ilp[d]][tdm1])
			    nxt = reg[ilp[d]][tdm1];
		    }
		    for (uint32_t i = tmp; i < nxt; ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}

		if (tmp < seg.back()) {
		    for (uint32_t i = tmp; i < seg.back(); ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
		else if (point.back() < seg.back()-1) {
		    point.back() = seg.back() - 1;
		    bdy.push_back(point);
		}
	    }
	    else {
		for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
		    point.back() = i;
		    bdy.push_back(point);
		}
	    }
	} // CASE I: all points in a line
	else if (sdim == ndm2) { // CASE II: all points in a plane
	    for (uint32_t d = 0; d < ndm1; ++ d)
		point[d] = seg[d+d];

	    // Case II-a: the first line, need to check for the line just
	    // before it in the same place
	    covered = (seg[tdm2] > 0 && reg[bpre][tdm2+1] == seg[tdm2]);
	    for (uint32_t d = 0; d < ndm2 && covered; ++ d)
		covered = (seg[d+d] > 0 && seg[d+d+1] < dim[d] &&
			   reg[bpre][d+d] <= seg[d] &&
			   reg[bpre][d+d+1] > seg[d]);

	    // deal with markers from - dim[b] direction
	    for (uint32_t b = 0; b < ndm2 && covered; ++ b) {
		uint32_t marker = mrkbm[b];
		// check whether the current markers are good
		covered = (mrkem[b] > marker);
		for (uint32_t d = 0; d < ndm1 && covered; ++ d) {
		    if (d != b)
			covered = (reg[marker][d+d] <= seg[d+d] &&
				   reg[marker][d+d+1] > seg[d+d]);
		    else
			covered = (reg[marker][d+d+1] == seg[d+d]);
		}
		if (! covered) { // not good, attempt to move mrkdm[b]
		    bool more = true;
		    for (marker = (b > 0 && mrkem[b] < mrkem[b-1] ?
				   mrkem[b-1] : mrkem[b]);
			 more && marker < epre; marker += more) {
			more = false;
			for (uint32_t d = 0; !more && d < tdm1; d+=2) {
			    if (d != b+b) {
				if (reg[marker][d] > seg[d]) break;
				more = (reg[marker][d+1] <= seg[d]);
			    }
			    else {
				if (reg[marker][d+1] > seg[d]) break;
				more = (reg[marker][d+1] < seg[d]);
			    }
			}
		    }

		    covered = true;
		    mrkbm[b] = marker;
		    mrkem[b] = marker;
		    for (uint32_t d = 0; d < tdm1 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d+1] == seg[d]);
		    }
		    if (covered) { // update mrkem[b]
			more = true;
			for (++ mrkem[b]; mrkem[b] < epre && more;
			     mrkem[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; d+=2)
				more = (reg[mrkem[b]][d] == reg[marker][d]);
			}
		    }
		}
		else if (mrkem[b] <= marker) { // update mrkem[b]
		    bool more = true;
		    for (mrkem[b] = marker+1; mrkem[b] < epre && more;
			 mrkem[b] += more) {
			for (uint32_t d = 0; d < tdm1 && more; d+=2)
			    more = (reg[mrkem[b]][d] == reg[marker][d]);
		    }
		}
	    }

	    for (uint32_t b = ndm2; b > 0 && covered;) {
		// deal with markers from + dim[b] direction
		-- b;
		uint32_t marker = mrkbp[b];
		if (covered)
		    covered = (mrkbp[b] < nreg && mrkep[b] > mrkbp[b]);
		for (uint32_t d = 0; d < tdm1 && covered; d+=2) {
		    if (d != b+b)
			covered = (reg[marker][d] <= seg[d] &&
				   reg[marker][d+1] > seg[d]);
		    else
			covered = (reg[marker][d] == seg[d+1]);
		}
		if (! covered) { // attempt to move mrkbp[b]
		    bool more = true;
		    if (b+1 < ndm2)
			marker = (mrkep[b+1] > mrkep[b] ?
				  mrkep[b+1] : mrkep[b]);
		    else if (mrkep[b] < bfol)
			marker = bfol;
		    else
			marker = mrkep[b];
		    for (; marker < nreg && more; marker += more) {
			more = false;
			for (uint32_t d = 0; d < tdm1 && !more; d+=2) {
			    if (d != b+b) {
				if (reg[marker][d] > seg[d]) break;
				more = (reg[marker][d+1] <= seg[d]);
			    }
			    else {
				if (reg[marker][d] > seg[d+1]) break;
				more = (reg[marker][d] < seg[d+1]);
			    }
			}
		    }

		    covered = (marker < nreg);
		    mrkbp[b] = marker;
		    mrkep[b] = marker;
		    for (uint32_t d = 0; d < tdm1 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d] == seg[d+1]);
		    }
		    if (covered) {
			more = true;
			for (++ mrkep[b]; mrkep[b] < nreg && more;
			     mrkep[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; ++ d)
				more = (reg[mrkep[b]][d] == reg[marker][d]);
			}
		    }
		}
		else if (mrkep[b] <= marker) { // update mrkep[b]
		    bool more = true;
		    for (mrkep[b] = marker+1; mrkep[b] < nreg && more;
			 mrkep[b] += more) {
			for (uint32_t d = 0; d < tdm1 && more; d+=2)
			    more = (reg[mrkep[b]][d] == reg[marker][d]);
		    }
		}
	    }

	    if (covered) {
		// the current line segment is surronded on all sides
		uint32_t ipre = bpre;
		std::vector<uint32_t> ilm(mrkbm);
		std::vector<uint32_t> ilp(mrkbp);
		// assigned tmp to be the maximum of xx[tdm1]
		uint32_t tmp = reg[ipre][tdm1];
		for (uint32_t d = 0; d < ndm2; ++ d) {
		    if (reg[ilm[d]][tdm1] > tmp) tmp = reg[ilm[d]][tdm1];
		    if (reg[ilp[d]][tdm1] > tmp) tmp = reg[ilp[d]][tdm1];
		}
		if (tmp > seg[tdm1]) {
		    if (tmp > seg.back()) tmp = seg.back();
		    for (uint32_t i = seg[tdm1]; i < tmp; ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
		else {
		    point.back() = seg[tdm1];
		    bdy.push_back(point);
		}

		while (tmp < seg.back()) {
		    // re-assign tmp to the minimum of xx.back()
		    tmp = reg[ipre].back();
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (tmp > reg[ilm[d]].back())
			    tmp = reg[ilm[d]].back();
			if (tmp > reg[ilp[d]].back())
			    tmp = reg[ilp[d]].back();
		    }
		    if (tmp <= point.back()) tmp = point.back() + 1;
		    if (reg[ipre].back() <= tmp) {
			for (++ ipre;
			     ipre < epre && reg[ipre].back() <= tmp;
			     ++ ipre);
			if (ipre >= epre) break;
			if (reg[ipre][tdm1] >= seg.back()) break;
		    }
		    bool more = true;
		    for (uint32_t d = 0; d < ndm2 && more; ++ d) {
			if (reg[ilm[d]].back() <= tmp) {
			    for (++ ilm[d];
				 ilm[d] < mrkem[d] &&
				     reg[ilm[d]].back() <= tmp;
				 ++ ilm[d]);
			    more = (ilm[d] < mrkem[d] &&
				    reg[ilm[d]][tdm1] < seg.back());
			}
			if (reg[ilp[d]].back() <= tmp && more) {
			    for (++ ilp[d];
				 ilp[d] < mrkep[d] &&
				     reg[ilp[d]].back() <= tmp;
				 ++ ilp[d]);
			    more = (ilp[d] < mrkep[d] &&
				    reg[ilp[d]][tdm1] < seg.back());
			}
		    }
		    if (! more) break;

		    // assign nxt to the maximum of xx[tdm1]
		    uint32_t nxt = reg[ipre][tdm1];
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (nxt < reg[ilm[d]][tdm1])
			    nxt = reg[ilm[d]][tdm1];
			if (nxt < reg[ilp[d]][tdm1])
			    nxt = reg[ilp[d]][tdm1];
		    }
		    for (uint32_t i = tmp; i < nxt; ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}

		if (tmp < seg.back()) {
		    for (uint32_t i = tmp; i < seg.back(); ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
		else if (point.back() < seg.back()-1) {
		    point.back() = seg.back() - 1;
		    bdy.push_back(point);
		}
	    }
	    else {
		for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
		    point.back() = i;
		    bdy.push_back(point);
		}
	    }

	    // Case II-b: for the lines in the middle, we need to examine
	    // their neighbors outside of the plane
	    for (uint32_t k = seg[tdm2]+1; k+1 < seg[tdm2+1]; ++ k) {
		point[ndm2] = k;

		covered = true;
		for (uint32_t d = 0; d < ndm2 && covered; ++ d)
		    covered = (seg[d+d] > 0 && seg[d+d+1] < dim[d]);

		// deal with markers from - dim[b] direction
		for (uint32_t b = 0; b < ndm2 && covered; ++ b) {
		    uint32_t marker = mrkbm[b];
		    // check whether the current markers are good
		    covered = (mrkem[b] > marker);
		    for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
			if (d != b)
			    covered = (reg[marker][d+d] <= seg[d+d] &&
				       reg[marker][d+d+1] > seg[d+d]);
			else
			    covered = (reg[marker][d+d+1] == seg[d+d]);
		    }
		    if (covered)
			covered = (reg[marker][tdm2] <= k &&
				   reg[marker][tdm2+1] > k);
		    if (! covered) { // not good, attempt to move mrkdm[b]
			bool more = true;
			for (marker = (b > 0 && mrkem[b] < mrkem[b-1] ?
				       mrkem[b-1] : mrkem[b]);
			     more && marker < epre; marker += more) {
			    bool ok = true;
			    more = false;
			    for (uint32_t d = 0; !more && ok && d < tdm2;
				 d+=2) {
				if (d != b+b) {
				    ok = (reg[marker][d] <= seg[d]);
				    more = (reg[marker][d+1] <= seg[d]);
				}
				else {
				    ok = (reg[marker][d+1] <= seg[d]);
				    more = (reg[marker][d+1] < seg[d]);
				}
			    }
			    if (!more && ok)
				more = (reg[marker][tdm2+1] <= k);
			}

			covered = true;
			mrkbm[b] = marker;
			mrkem[b] = marker;
			for (uint32_t d = 0; d < tdm2 && covered; d+=2) {
			    if (d != b+b)
				covered = (reg[marker][d] <= seg[d] &&
					   reg[marker][d+1] > seg[d]);
			    else
				covered = (reg[marker][d+1] == seg[d]);
			}
			if (covered)
			    covered = (reg[marker][tdm2] <= k &&
				       reg[marker][tdm2+1] > k);
			if (covered) {
			    more = true;
			    for (++ mrkem[b]; mrkem[b] < epre && more;
				 mrkem[b] += more) {
				for (uint32_t d = 0; d < tdm1 && more; d+=2)
				    more = (reg[mrkem[b]][d] ==
					    reg[marker][d]);
			    }
			}
		    }
		    else if (mrkem[b] <= marker) { // update mrkem[b]
			bool more = true;
			for (mrkem[b] = marker+1; mrkem[b] < epre && more;
			     mrkem[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; d+=2)
				more = (reg[mrkem[b]][d] ==
					reg[marker][d]);
			}
		    }
		}

		for (uint32_t b = ndm2; b > 0 && covered;) {
		    // deal with markers from + dim[b] direction
		    -- b;
		    uint32_t marker = mrkbp[b];
		    if (covered)
			covered = (mrkbp[b] < nreg && mrkep[b] > mrkbp[b]);
		    for (uint32_t d = 0; d < tdm2 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d] == seg[d+1]);
		    }
		    if (covered)
			covered = (reg[marker][tdm2] <= k &&
				   reg[marker][tdm2+1] > k);
		    if (! covered) { // attempt to move mrkbp[b]
			bool more = true;
			if (b+1 < ndm2)
			    marker = (mrkep[b+1] > mrkep[b] ?
				      mrkep[b+1] : mrkep[b]);
			else if (mrkep[b] < bfol)
			    marker = bfol;
			else
			    marker = mrkep[b];
			for (; marker < nreg && more; marker += more) {
			    bool ok = true;
			    more = false;
			    for (uint32_t d = 0; ok && d < tdm2 && !more;
				 d+=2) {
				if (d != b+b) {
				    ok = (reg[marker][d] <= seg[d]);
				    more = (reg[marker][d+1] <= seg[d]);
				}
				else {
				    ok = (reg[marker][d] <= seg[d+1]);
				    more = (reg[marker][d] < seg[d+1]);
				}
			    }
			    if (ok && !more)
				more = (reg[marker][tdm2+1] <= k);
			}

			covered = (marker < nreg);
			mrkbp[b] = marker;
			mrkep[b] = marker;
			for (uint32_t d = 0; d < tdm2 && covered; d+=2) {
			    if (d != b+b)
				covered = (reg[marker][d] <= seg[d] &&
					   reg[marker][d+1] > seg[d]);
			    else
				covered = (reg[marker][d] == seg[d+1]);
			}
			if (covered)
			    covered = (reg[marker][tdm2] <= k &&
				       reg[marker][tdm2+1] > k);
			if (covered) {
			    more = true;
			    for (++ mrkep[b]; mrkep[b] < nreg && more;
				 mrkep[b] += more) {
				for (uint32_t d = 0; d < tdm1 && more; ++ d)
				    more = (reg[mrkep[b]][d] ==
					    reg[marker][d]);
			    }
			}
		    }
		    else if (mrkep[b] <= marker) { // update mrkep[b]
			bool more = true;
			for (mrkep[b] = marker+1; mrkep[b] < nreg && more;
			     mrkep[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; d+=2)
				more = (reg[mrkep[b]][d] == reg[marker][d]);
			}
		    }
		}

		if (covered) {
		    // the current line segment is surronded on all sides
		    std::vector<uint32_t> ilm(mrkbm);
		    std::vector<uint32_t> ilp(mrkbp);
		    // assigned tmp to be the maximum of xx[tdm1]
		    uint32_t tmp = seg[tdm1];
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (reg[ilm[d]][tdm1] > tmp)
			    tmp = reg[ilm[d]][tdm1];
			if (reg[ilp[d]][tdm1] > tmp)
			    tmp = reg[ilp[d]][tdm1];
		    }
		    if (tmp > seg[tdm1]) {
			if (tmp > seg.back()) tmp = seg.back();
			for (uint32_t i = seg[tdm1]; i < tmp; ++ i) {
			    point.back() = i;
			    bdy.push_back(point);
			}
		    }
		    else {
			point.back() = seg[tdm1];
			bdy.push_back(point);
		    }

		    while (tmp < seg.back()) {
			// re-assign tmp to the minimum of xx.back()
			tmp = reg[ilm[0]].back();
			for (uint32_t d = 0; d < ndm2; ++ d) {
			    if (tmp > reg[ilm[d]].back())
				tmp = reg[ilm[d]].back();
			    if (tmp > reg[ilp[d]].back())
				tmp = reg[ilp[d]].back();
			}
			if (tmp <= point.back()) tmp = point.back() + 1;
			bool more = true;
			for (uint32_t d = 0; d < ndm2 && more; ++ d) {
			    if (reg[ilm[d]].back() <= tmp) {
				for (++ ilm[d];
				     ilm[d] < mrkem[d] &&
					 reg[ilm[d]].back() <= tmp;
				     ++ ilm[d]);
				more = (ilm[d] < mrkem[d] &&
					reg[ilm[d]][tdm1] < seg.back());
			    }
			    if (reg[ilp[d]].back() <= tmp && more) {
				for (++ ilp[d];
				     ilp[d] < mrkep[d] &&
					 reg[ilp[d]].back() <= tmp;
				     ++ ilp[d]);
				more = (ilp[d] < mrkep[d] &&
					reg[ilp[d]][tdm1] < seg.back());
			    }
			}
			if (! more) break;

			// assign nxt to the maximum of xx[tdm1]
			uint32_t nxt = tmp;
			for (uint32_t d = 0; d < ndm2; ++ d) {
			    if (nxt < reg[ilm[d]][tdm1])
				nxt = reg[ilm[d]][tdm1];
			    if (nxt < reg[ilp[d]][tdm1])
				nxt = reg[ilp[d]][tdm1];
			}
			for (uint32_t i = tmp; i < nxt; ++ i) {
			    point.back() = i;
			    bdy.push_back(point);
			}
		    }

		    if (tmp < seg.back()) {
			for (uint32_t i = tmp; i < seg.back(); ++ i) {
			    point.back() = i;
			    bdy.push_back(point);
			}
		    }
		    else if (point.back() < seg.back()-1) {
			point.back() = seg.back() - 1;
			bdy.push_back(point);
		    }
		}
		else {
		    for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
	    } // for lines in the middle

	    // Case II-c: the last line, need to examine the block
	    // immediately following the current one (plus those from other
	    // dimensions)
	    point[ndm2] = seg[tdm2+1] - 1;
	    covered = (bfol < nreg && reg[bfol][tdm2] == seg[tdm2+1]);
	    for (uint32_t d = 0; d < ndm2 && covered; ++ d)
		covered = (seg[d+d] > 0 && seg[d+d+1] < dim[d] &&
			   reg[bfol][d+d] <= seg[d+d] &&
			   reg[bfol][d+d+1] > seg[d+d]);

	    // deal with markers from - dim[b] direction
	    for (uint32_t b = 0; b < ndm2 && covered; ++ b) {
		uint32_t marker = mrkbm[b];
		// check whether the current markers are good
		covered = (mrkem[b] > marker);
		for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
		    if (d != b)
			covered = (reg[marker][d+d] <= seg[d+d] &&
				   reg[marker][d+d+1] > seg[d+d]);
		    else
			covered = (reg[marker][d+d+1] == seg[d+d]);
		}
		if (covered)
		    covered = (reg[marker][tdm2] <= point[ndm2] &&
			       reg[marker][tdm2+1] > point[ndm2]);
		if (! covered) { // not good, attempt to move mrkdm[b]
		    bool more = true;
		    for (marker = (b > 0 && mrkem[b] < mrkem[b-1] ?
				   mrkem[b-1] : mrkem[b]);
			 more && marker < epre; marker += more) {
			bool ok = true;
			more = false;
			for (uint32_t d = 0; ok && !more && d < tdm2;
			     d+=2) {
			    if (d != b+b) {
				ok = (reg[marker][d] <= seg[d]);
				more = (reg[marker][d+1] <= seg[d]);
			    }
			    else {
				ok = (reg[marker][d+1] <= seg[d]);
				more = (reg[marker][d+1] < seg[d]);
			    }
			}
			if (ok && !more)
			    more = (reg[marker][tdm2+1] <= point[ndm2]);
		    }

		    covered = true;
		    mrkbm[b] = marker;
		    mrkem[b] = marker;
		    for (uint32_t d = 0; d < tdm2 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d+1] == seg[d]);
		    }
		    if (covered)
			covered = (reg[marker][tdm2] <= point[ndm2] &&
				   reg[marker][tdm2+1] > point[ndm2]);
		    if (covered) {
			more = true;
			for (++ mrkem[b]; mrkem[b] < epre && more;
			     mrkem[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; d+=2)
				more = (reg[mrkem[b]][d] == reg[marker][d]);
			}
		    }
		}
		else if (mrkem[b] <= marker) { // update mrkem[b]
		    bool more = true;
		    for (mrkem[b] = marker+1; mrkem[b] < epre && more;
			 mrkem[b] += more) {
			for (uint32_t d = 0; d < tdm1 && more; d+=2)
			    more = (reg[mrkem[b]][d] == reg[marker][d]);
		    }
		}
	    }

	    for (uint32_t b = ndm2; b > 0 && covered;) {
		// deal with markers from + dim[b] direction
		-- b;
		uint32_t marker = mrkbp[b];
		if (covered)
		    covered = (mrkbp[b] < nreg && mrkep[b] > mrkbp[b]);
		for (uint32_t d = 0; d < tdm2 && covered; d+=2) {
		    if (d != b+b)
			covered = (reg[marker][d] <= seg[d] &&
				   reg[marker][d+1] > seg[d]);
		    else
			covered = (reg[marker][d] == seg[d+1]);
		}
		if (covered)
		    covered = (reg[marker][tdm2] <= point[ndm2] &&
			       reg[marker][tdm2+1] > point[ndm2]);
		if (! covered) { // attempt to move mrkbp[b]
		    bool more = true;
		    if (b+1 < ndm2)
			marker = (mrkep[b+1] > mrkep[b] ?
				  mrkep[b+1] : mrkep[b]);
		    else if (mrkep[b] < bfol)
			marker = bfol;
		    else
			marker = mrkep[b];
		    for (; marker < nreg && more; marker += more) {
			bool ok = true;
			more = false;
			for (uint32_t d = 0; d < tdm2 && ok && !more;
			     d+=2) {
			    if (d != b+b) {
				ok = (reg[marker][d] <= seg[d]);
				more = (reg[marker][d+1] <= seg[d]);
			    }
			    else {
				ok = (reg[marker][d] <= seg[d+1]);
				more = (reg[marker][d] < seg[d+1]);
			    }
			}
			if (ok && !more)
			    more = (reg[marker][tdm2+1] <= point[ndm2]);
		    }

		    covered = (marker < nreg);
		    mrkbp[b] = marker;
		    mrkep[b] = marker;
		    for (uint32_t d = 0; d < tdm2 && covered; d+=2) {
			if (d != b+b)
			    covered = (reg[marker][d] <= seg[d] &&
				       reg[marker][d+1] > seg[d]);
			else
			    covered = (reg[marker][d] == seg[d+1]);
		    }
		    if (covered)
			covered = (reg[marker][tdm2] <= point[ndm2] &&
				   reg[marker][tdm2+1] > point[ndm2]);
		    if (covered) {
			more = true;
			for (++ mrkep[b]; mrkep[b] < nreg && more;
			     mrkep[b] += more) {
			    for (uint32_t d = 0; d < tdm1 && more; ++ d)
				more = (reg[mrkep[b]][d] == reg[marker][d]);
			}
		    }
		}
		else if (mrkep[b] <= marker) { // update mrkep[b]
		    bool more = true;
		    for (mrkep[b] = marker+1; mrkep[b] < nreg && more;
			 mrkep[b] += more) {
			for (uint32_t d = 0; d < tdm1 && more; d+=2)
			    more = (reg[mrkep[b]][d] == reg[marker][d]);
		    }
		}
	    }

	    if (covered) {
		// the current line segment is surronded on all sides
		uint32_t ifol = bfol;
		std::vector<uint32_t> ilm(mrkbm);
		std::vector<uint32_t> ilp(mrkbp);
		// assigned tmp to be the maximum of xx[tdm1]
		uint32_t tmp = reg[ifol][tdm1];
		for (uint32_t d = 0; d < ndm2; ++ d) {
		    if (reg[ilm[d]][tdm1] > tmp)
			tmp = reg[ilm[d]][tdm1];
		    if (reg[ilp[d]][tdm1] > tmp)
			tmp = reg[ilp[d]][tdm1];
		}
		if (tmp > seg[tdm1]) {
		    if (tmp > seg.back()) tmp = seg.back();
		    for (uint32_t i = seg[tdm1]; i < tmp; ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
		else {
		    point.back() = seg[tdm1];
		    bdy.push_back(point);
		}

		while (tmp < seg.back()) {
		    // re-assign tmp to the minimum of xx.back()
		    tmp = reg[ifol].back();
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (tmp > reg[ilm[d]].back())
			    tmp = reg[ilm[d]].back();
			if (tmp > reg[ilp[d]].back())
			    tmp = reg[ilp[d]].back();
		    }
		    if (tmp <= point.back()) tmp = point.back() + 1;
		    if (reg[ifol].back() <= tmp) {
			for (++ ifol; ifol < efol && reg[ifol].back() <= tmp;
			     ++ ifol);
			if (ifol >= efol) break;
			if (reg[ifol][tdm1] >= seg.back()) break;
		    }
		    bool more = true;
		    for (uint32_t d = 0; d < ndm2 && more; ++ d) {
			if (reg[ilm[d]].back() <= tmp) {
			    for (++ ilm[d];
				 ilm[d] < mrkem[d] &&
				     reg[ilm[d]].back() <= tmp;
				 ++ ilm[d]);
			    more = (ilm[d] < mrkem[d] &&
				    reg[ilm[d]][tdm1] < seg.back());
			}
			if (reg[ilp[d]].back() <= tmp && more) {
			    for (++ ilp[d];
				 ilp[d] < mrkep[d] &&
				     reg[ilp[d]].back() <= tmp;
				 ++ ilp[d]);
			    more = (ilp[d] < mrkep[d] &&
				    reg[ilp[d]][tdm1] < seg.back());
			}
		    }
		    if (! more) break;

		    // assign nxt to the maximum of xx[tdm1]
		    uint32_t nxt = reg[ifol][tdm1];
		    for (uint32_t d = 0; d < ndm2; ++ d) {
			if (nxt < reg[ilm[d]][tdm1])
			    nxt = reg[ilm[d]][tdm1];
			if (nxt < reg[ilp[d]][tdm1])
			    nxt = reg[ilp[d]][tdm1];
		    }
		    for (uint32_t i = tmp; i < nxt; ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}

		if (tmp < seg.back()) {
		    for (uint32_t i = tmp; i < seg.back(); ++ i) {
			point.back() = i;
			bdy.push_back(point);
		    }
		}
		else if (point.back() < seg.back()-1) {
		    point.back() = seg.back() - 1;
		    bdy.push_back(point);
		}
	    }
	    else {
		for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
		    point.back() = i;
		    bdy.push_back(point);
		}
	    }
	} // CASE II: all points in a plane
	else { // CASE III: points span multiple dimensions
	    // the basic strategy: use a std::vector for loop index, make
	    // each loop index corresponds to a plane, the loop body deals
	    // with one plane at a time
	    std::vector<uint32_t> loin(ndm2); // the loop index
	    for (uint32_t d = 0; d < ndm2; ++ d)
		loin[d] = seg[d+d];
	    bool more = true;
	    bool first = true;
	    uint32_t bmp=0, emp=0, bpp=0, epp=0;
	    while (more) {
		// the first line of the plane is always exposed
		for (uint32_t d = 0; d < ndm2; ++ d)
		    point[d] = loin[d];
		point[ndm2] = seg[tdm2]; // should be zero
		for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
		    point.back() = i;
		    bdy.push_back(point);
		}

		bool last = true; // is this the last plane ?
		for (uint32_t d = sdim; d < ndm2 && last; ++ d)
		    last = (loin[d]+1 == seg[d+d+1]);

		// lines in the middle of the plane
		for (uint32_t k = seg[tdm2]+1; k+1 < seg[tdm2+1]; ++ k) {
		    // are all indices in the middle ?, yes ==> covered = true
		    covered = true;
		    for (uint32_t d = 0; d < ndm2 && covered; ++ d)
			covered = (loin[d] > 0 && loin[d]+1 < dim[d]);

		    // Is the current line surround with neighbors from the
		    // first sdim directions? This take two separate
		    // for-loops to deal with the neighbors ordered before
		    // and after the current block in the list of blocks
		    for (uint32_t b = 0; b < sdim && covered; ++ b) {
			// deal with markers from - dim[b] direction
			uint32_t marker = mrkbm[b];
			// check whether the current markers are good
			covered = (mrkem[b] > marker);
			for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
			    if (d != b)
				covered = (reg[marker][d+d] <= loin[d] &&
					   reg[marker][d+d+1] > loin[d]);
			    else
				covered = (reg[marker][d+d+1] == seg[d+d]);
			}
			if (covered)
			    covered = (reg[marker][tdm2] <= k &&
				       reg[marker][tdm2+1] > k);
			if (! covered) { // not good, attempt to move mrkdm[b]
			    more = true;
			    for (marker = (b > 0 && mrkem[b] < mrkem[b-1] ?
					   mrkem[b-1] : mrkem[b]);
				 more && marker < epre;
				 marker += more) {
				bool ok = true;
				more = false;
				for (uint32_t d = 0; ok && !more && d < ndm2;
				     ++ d) {
				    if (d != b) {
					ok = (reg[marker][d+d] <= loin[d]);
					more = (reg[marker][d+d+1] <= loin[d]);
				    }
				    else {
					ok = (reg[marker][d+d+1] <= seg[d+d]);
					more = (reg[marker][d+d+1] < seg[d+d]);
				    }
				}
				if (ok && !more)
				    more = (reg[marker][tdm2+1] <= k);
			    }

			    covered = true;
			    mrkbm[b] = marker;
			    mrkem[b] = marker;
			    for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
				if (d != b)
				    covered = (reg[marker][d+d] <= loin[d] &&
					       reg[marker][d+d+1] > loin[d]);
				else
				    covered = (reg[marker][d+d+1] == seg[d+d]);
			    }
			    if (covered)
				covered = (reg[marker][tdm2] <= k &&
					   reg[marker][tdm2+1] > k);
			    if (covered) {
				more = true;
				for (++ mrkem[b]; mrkem[b] < epre && more;
				     mrkem[b] += more) {
				    for (uint32_t d = 0; d < tdm1 && more;
					 d+=2)
					more = (reg[mrkem[b]][d] ==
						reg[marker][d]);
				}
			    }
			}
			else if (mrkem[b] <= marker) { // update mrkem[b]
			    more = true;
			    for (mrkem[b] = marker+1; mrkem[b] < epre && more;
				 mrkem[b] += more) {
				for (uint32_t d = 0; d < tdm1 && more; d+=2)
				    more = (reg[mrkem[b]][d] ==
					    reg[marker][d]);
			    }
			}
		    }

		    for (uint32_t b = sdim; b > 0 && covered;) {
			// deal with markers from + dim[b] direction
			-- b;
			uint32_t marker = mrkbp[b];
			if (covered)
			    covered = (mrkbp[b] < nreg &&
				       mrkep[b] > mrkbp[b]);
			for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
			    if (d != b)
				covered = (reg[marker][d+d] <= loin[d] &&
					   reg[marker][d+d+1] > loin[d]);
			    else
				covered = (reg[marker][d+d] == seg[d+d+1]);
			}
			if (covered)
			    covered = (reg[marker][tdm2] <= k &&
				       reg[marker][tdm2+1] > k);
			if (! covered) { // attempt to move mrkbp[b]
			    more = true;
			    if (b+1 < ndm2)
				marker = (mrkep[b+1] > mrkep[b] ?
					  mrkep[b+1] : mrkep[b]);
			    else if (mrkep[b] < bfol)
				marker = bfol;
			    else
				marker = mrkep[b];
			    for (; marker < nreg && more; marker += more) {
				bool ok = true;
				more = false;
				for (uint32_t d = 0; ok && d < ndm2 && !more;
				     ++ d) {
				    if (d != b) {
					ok = (reg[marker][d+d] <= loin[d]);
					more = (reg[marker][d+d+1] <= loin[d]);
				    }
				    else {
					ok = (reg[marker][d+d] <= seg[d+d+1]);
					more = (reg[marker][d+d] < seg[d+d+1]);
				    }
				}
				if (ok && !more)
				    more = (reg[marker][tdm2+1] <= k);
			    }

			    covered = (marker < nreg);
			    mrkbp[b] = marker;
			    mrkep[b] = marker;
			    for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
				if (d != b)
				    covered = (reg[marker][d+d] <= loin[d] &&
					       reg[marker][d+d+1] > loin[d]);
				else
				    covered = (reg[marker][d+d] == seg[d+d+1]);
			    }
			    if (covered)
				covered = (reg[marker][tdm2] <= k &&
					   reg[marker][tdm2+1] > k);
			    if (covered) {
				more = true;
				for (++ mrkep[b]; mrkep[b] < nreg && more;
				     mrkep[b] += more) {
				    for (uint32_t d = 0; d < tdm1 && more;
					 ++ d)
					more = (reg[mrkep[b]][d] ==
						reg[marker][d]);
				}
			    }
			}
			else if (mrkep[b] <= marker) { // update mrkep[b]
			    more = true;
			    for (mrkep[b] = marker+1; mrkep[b] < nreg && more;
				 mrkep[b] += more) {
				for (uint32_t d = 0; d < tdm1 && more; d+=2)
				    more = (reg[mrkep[b]][d] ==
					    reg[marker][d]);
			    }
			}
		    }

		    point[ndm2] = k;
		    if (! covered) { // all points are exposed
			for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
			    point.back() = i;
			    bdy.push_back(point);
			}
		    }
		    else if (first) { // covered on first sdim dimensions
			// the first plane, need to check one additioinal
			// neighboring plane
			covered = (bmp < emp);
			for (uint32_t d = 0; d < ndm2 && covered; ++ d)
			    if (d != sdim)
				covered = (reg[bmp][d+d] <= loin[d] &&
					   reg[bmp][d+d+1] > loin[d]);
			    else
				covered = (reg[bmp][d+d+1] == seg[d+d]);
			if (covered)
			    covered = (reg[bmp][tdm2] <= k &&
				       reg[bmp][tdm2+1] > k);
			if (! covered) {
			    more = true;
			    for (bmp = (sdim>0 && mrkem[sdim-1] > emp ?
					mrkem[sdim] : emp);
				 more && bmp < epre;
				 bmp += more) {
				bool ok = true;
				more = false;
				for (uint32_t d = 0; ok && d < ndm2 && !more;
				     ++ d) {
				    if (d != sdim) {
					ok = (reg[bmp][d+d] <= loin[d]);
					more = (reg[bmp][d+d+1] <= loin[d]);
				    }
				    else {
					ok = (reg[bmp][d+d+1] <= seg[d+d]);
					more = (reg[bmp][d+d+1] < seg[d+d]);
				    }
				}
				if (ok && !more)
				    more = (reg[bmp][tdm2+1] <= k);
			    }

			    emp = bmp;
			    covered = true;
			    for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
				if (d != sdim)
				    covered = (reg[bmp][d+d] <= loin[d] &&
					       reg[bmp][d+d+1] > loin[d]);
				else
				    covered = (reg[bmp][d+d+1] == seg[d+d]);
			    }
			    if (covered)
				covered = (reg[bmp][tdm2] <= k &&
					   reg[bmp][tdm2+1] > k);
			    if (covered) {
				more = true;
				for (++ emp; emp < epre && more; emp += more) {
				    for (uint32_t d = 0; d < tdm1 && more;
					 d+=2)
					more = (reg[emp][d] == reg[bmp][d]);
				}
			    }
			}

			if (covered) {
			    if (sdim > 0) { // need to check with neighbors
				// the current line segment is surronded
				// on all sides
				uint32_t imp = bmp;
				std::vector<uint32_t> ilm(mrkbm);
				std::vector<uint32_t> ilp(mrkbp);
				// assigned tmp to be the maximum of xx[tdm1]
				uint32_t tmp = reg[imp][tdm1];
				for (uint32_t d = 0; d < sdim; ++ d) {
				    if (reg[ilm[d]][tdm1] > tmp)
					tmp = reg[ilm[d]][tdm1];
				    if (reg[ilp[d]][tdm1] > tmp)
					tmp = reg[ilp[d]][tdm1];
				}
				if (tmp > seg[tdm1]) {
				    if (tmp > seg.back())
					tmp = seg.back();
				    for (uint32_t i = seg[tdm1]; i < tmp;
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else {
				    point.back() = seg[tdm1];
				    bdy.push_back(point);
				}

				while (tmp < seg.back()) {
				    // re-assign tmp to the minimum of
				    // xx.back()
				    tmp = reg[imp].back();
				    for (uint32_t d = 0; d < ndm2; ++ d) {
					if (tmp > reg[ilm[d]].back())
					    tmp = reg[ilm[d]].back();
					if (tmp > reg[ilp[d]].back())
					    tmp = reg[ilp[d]].back();
				    }
				    if (tmp <= point.back())
					tmp = point.back() + 1;
				    if (reg[imp].back() <= tmp) {
					for (++ imp;
					     reg[imp].back() <= tmp &&
						 imp < emp;
					     ++ imp);
					if (imp >= emp) break;
					if (reg[imp][tdm1] >= seg.back())
					    break;
				    }
				    more = true;
				    for (uint32_t d = 0; d < sdim && more;
					 ++ d) {
					if (reg[ilm[d]].back() <= tmp) {
					    for (++ ilm[d];
						 ilm[d] < mrkem[d] &&
						     reg[ilm[d]].back() <= tmp;
						 ++ ilm[d]);
					    more = (ilm[d] < mrkem[d] &&
						    reg[ilm[d]][tdm1] <
						    seg.back());
					}
					if (reg[ilp[d]].back() <= tmp &&
					    more) {
					    for (++ ilp[d];
						 ilp[d] < mrkep[d] &&
						     reg[ilp[d]].back() <= tmp;
						 ++ ilp[d]);
					    more = (ilp[d] < mrkep[d] &&
						    reg[ilp[d]][tdm1] <
						    seg.back());
					}
				    }
				    if (! more) break;

				    // assign nxt to the maximum of xx[tdm1]
				    uint32_t nxt = reg[imp][tdm1];
				    for (uint32_t d = 0; d < ndm2; ++ d) {
					if (nxt < reg[ilm[d]][tdm1])
					    nxt = reg[ilm[d]][tdm1];
					if (nxt < reg[ilp[d]][tdm1])
					    nxt = reg[ilp[d]][tdm1];
				    }
				    for (uint32_t i = tmp; i < nxt; ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}

				if (tmp < seg.back()) {
				    for (uint32_t i = tmp; i < seg.back();
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else if (point.back() < seg.back()-1) {
				    point.back() = seg.back() - 1;
				    bdy.push_back(point);
				}
			    }
			    else { // only check one neighboring plane
				uint32_t imp = bmp;
				uint32_t tmp = reg[imp][tdm1];
				if (tmp > seg[tdm1]) {
				    if (tmp > seg.back()) tmp = seg.back();
				    for (uint32_t i = seg[tdm1];
					 i < tmp; ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else {
				    point.back() = seg[tdm1];
				    bdy.push_back(point);
				}

				while (tmp < seg.back()) {
				    tmp = reg[imp].back();
				    if (tmp <= point.back())
					tmp = point.back() + 1;
				    for (++ imp;
					 reg[imp].back() <= tmp &&
					     imp < emp;
					 ++ imp);
				    if (imp >= emp) break;
				    if (reg[imp][tdm1] >= seg.back())
					break;
				    for (uint32_t i = tmp;
					 i < reg[imp][tdm1];
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}

				if (tmp < seg.back()) {
				    for (uint32_t i = tmp; i < seg.back();
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else if (point.back() < seg.back()-1) {
				    point.back() = seg.back() - 1;
				    bdy.push_back(point);
				}
			    }
			}
			else { // all points are exposed
			    for (uint32_t i = seg[tdm1]; i < seg.back();
				 ++ i) {
				point.back() = i;
				bdy.push_back(point);
			    }
			}
		    }
		    else if (last) { // covered on first sdim dimensions
			// the last plane, need to check one additional
			// neighboring plane
			covered = (bpp < epp);
			for (uint32_t d = 0; d < ndm2 && covered; ++ d)
			    if (d != sdim)
				covered = (reg[bpp][d+d] <= loin[d] &&
					   reg[bpp][d+d+1] > loin[d]);
			    else
				covered = (reg[bpp][d+d] == seg[d+d+1]);
			if (covered)
			    covered = (reg[bpp][tdm2] <= k &&
				       reg[bpp][tdm2+1] > k);
			if (! covered) {
			    more = true;
			    for (bpp = (bfol >= epp ? bfol : epp);
				 more && bpp < nreg;
				 bpp += more) {
				bool ok = true;
				more = false;
				for (uint32_t d = 0; ok && d < ndm2 && !more;
				     ++ d) {
				    if (d != sdim) {
					ok = (reg[bpp][d+d] < loin[d]);
					more = (reg[bpp][d+d+1] <= loin[d]);
				    }
				    else {
					ok = (reg[bpp][d+d] <= seg[d+d+1]);
					more = (reg[bpp][d+d] < seg[d+d+1]);
				    }
				}
				if (ok && !more)
				    more = (reg[bpp][tdm2+1] <= k);
			    }

			    epp = bpp;
			    covered = (bpp < nreg);
			    for (uint32_t d = 0; d < ndm2 && covered; ++ d) {
				if (d != sdim)
				    covered = (reg[bpp][d+d] <= loin[d] &&
					       reg[bpp][d+d+1] > loin[d]);
				else
				    covered = (reg[bpp][d+d] == seg[d+d+1]);
			    }
			    if (covered)
				covered = (reg[bpp][tdm2] <= k &&
					   reg[bpp][tdm2+1] > k);
			    if (covered) {
				more = true;
				for (++ epp; epp < nreg && more; epp += more) {
				    for (uint32_t d = 0; d < tdm1 && more;
					 d+=2)
					more = (reg[epp][d] == reg[bpp][d]);
				}
			    }
			}

			if (covered) {
			    if (sdim > 0) { // need to check with neighbors
				// the current line segment is surronded
				// on all sides
				uint32_t ipp = bpp;
				std::vector<uint32_t> ilm(mrkbm);
				std::vector<uint32_t> ilp(mrkbp);
				// assigned tmp to be the maximum of xx[tdm1]
				uint32_t tmp = reg[bpp][tdm1];
				for (uint32_t d = 0; d < sdim; ++ d) {
				    if (reg[ilm[d]][tdm1] > tmp)
					tmp = reg[ilm[d]][tdm1];
				    if (reg[ilp[d]][tdm1] > tmp)
					tmp = reg[ilp[d]][tdm1];
				}
				if (tmp > seg[tdm1]) {
				    if (tmp > seg.back())
					tmp = seg.back();
				    for (uint32_t i = seg[tdm1]; i < tmp;
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else {
				    point.back() = seg[tdm1];
				    bdy.push_back(point);
				}

				while (tmp < seg.back()) {
				    // re-assign tmp to the minimum of
				    // xx.back()
				    tmp = reg[ipp].back();
				    for (uint32_t d = 0; d < ndm2; ++ d) {
					if (tmp > reg[ilm[d]].back())
					    tmp = reg[ilm[d]].back();
					if (tmp > reg[ilp[d]].back())
					    tmp = reg[ilp[d]].back();
				    }
				    if (tmp <= point.back())
					tmp = point.back() + 1;
				    if (reg[ipp].back() <= tmp) {
					for (++ ipp;
					     reg[ipp].back() <= tmp &&
						 ipp < epp;
					     ++ ipp);
					if (ipp >= epp) break;
					if (reg[ipp][tdm1] >= seg.back())
					    break;
				    }
				    more = true;
				    for (uint32_t d = 0; d < sdim && more;
					 ++ d) {
					if (reg[ilm[d]].back() <= tmp) {
					    for (++ ilm[d];
						 ilm[d] < mrkem[d] &&
						     reg[ilm[d]].back() <= tmp;
						 ++ ilm[d]);
					    more = (ilm[d] < mrkem[d] &&
						    reg[ilm[d]][tdm1] <
						    seg.back());
					}
					if (reg[ilp[d]].back() <= tmp &&
					    more) {
					    for (++ ilp[d];
						 ilp[d] < mrkep[d] &&
						     reg[ilp[d]].back() <= tmp;
						 ++ ilp[d]);
					    more = (ilp[d] < mrkep[d] &&
						    reg[ilp[d]][tdm1] <
						    seg.back());
					}
				    }
				    if (! more) break;

				    // assign nxt to the maximum of xx[tdm1]
				    uint32_t nxt = reg[ipp][tdm1];
				    for (uint32_t d = 0; d < ndm2; ++ d) {
					if (nxt < reg[ilm[d]][tdm1])
					    nxt = reg[ilm[d]][tdm1];
					if (nxt < reg[ilp[d]][tdm1])
					    nxt = reg[ilp[d]][tdm1];
				    }
				    for (uint32_t i = tmp; i < nxt; ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}

				if (tmp < seg.back()) {
				    for (uint32_t i = tmp; i < seg.back();
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else if (point.back() < seg.back()-1) {
				    point.back() = seg.back() - 1;
				    bdy.push_back(point);
				}
			    }
			    else { // only check one plane 
				uint32_t ipp = bpp;
				uint32_t tmp = reg[bpp][tdm1];
				if (tmp > seg[tdm1]) {
				    if (tmp > seg.back()) tmp = seg.back();
				    for (uint32_t i = seg[tdm1];
					 i < tmp; ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else {
				    point.back() = seg[tdm1];
				    bdy.push_back(point);
				}

				while (tmp < seg.back()) {
				    tmp = reg[ipp].back();
				    if (tmp <= point.back())
					tmp = point.back() + 1;
				    for (++ ipp;
					 reg[ipp].back() <= tmp &&
					     ipp < epp;
					 ++ ipp);
				    if (ipp >= epp) break;
				    if (reg[ipp][tdm1] >= seg.back())
					break;

				    for (uint32_t i = tmp;
					 i < reg[ipp][tdm1];
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}

				if (tmp < seg.back()) {
				    for (uint32_t i = tmp; i < seg.back();
					 ++ i) {
					point.back() = i;
					bdy.push_back(point);
				    }
				}
				else if (point.back() < seg.back()-1) {
				    point.back() = seg.back() - 1;
				    bdy.push_back(point);
				}
			    }
			}
			else { // all points are exposed
			    for (uint32_t i = seg[tdm1]; i < seg.back();
				 ++ i) {
				point.back() = i;
				bdy.push_back(point);
			    }
			}
		    }
		    else if (sdim > 0) { // check with 2 * sdim neighbors
			// the current line segment is surronded on all sides
			std::vector<uint32_t> ilm(mrkbm);
			std::vector<uint32_t> ilp(mrkbp);
			// assigned tmp to be the maximum of xx[tdm1]
			uint32_t tmp = seg[tdm1];
			for (uint32_t d = 0; d < sdim; ++ d) {
			    if (reg[ilm[d]][tdm1] > tmp)
				tmp = reg[ilm[d]][tdm1];
			    if (reg[ilp[d]][tdm1] > tmp)
				tmp = reg[ilp[d]][tdm1];
			}
			if (tmp > seg[tdm1]) {
			    if (tmp > seg.back()) tmp = seg.back();
			    for (uint32_t i = seg[tdm1]; i < tmp; ++ i) {
				point.back() = i;
				bdy.push_back(point);
			    }
			}
			else {
			    point.back() = seg[tdm1];
			    bdy.push_back(point);
			}

			while (tmp < seg.back()) {
			    // re-assign tmp to the minimum of xx.back()
			    tmp = reg[ilm[0]].back();
			    for (uint32_t d = 0; d < ndm2; ++ d) {
				if (tmp > reg[ilm[d]].back())
				    tmp = reg[ilm[d]].back();
				if (tmp > reg[ilp[d]].back())
				    tmp = reg[ilp[d]].back();
			    }
			    if (tmp <= point.back())
				tmp = point.back() + 1;
			    more = true;
			    for (uint32_t d = 0; d < sdim && more; ++ d) {
				if (reg[ilm[d]].back() <= tmp) {
				    for (++ ilm[d];
					 ilm[d] < mrkem[d] &&
					     reg[ilm[d]].back() <= tmp;
					 ++ ilm[d]);
				    more = (ilm[d] < mrkem[d] &&
					    reg[ilm[d]][tdm1] < seg.back());
				}
				if (reg[ilp[d]].back() <= tmp && more) {
				    for (++ ilp[d];
					 ilp[d] < mrkep[d] &&
					     reg[ilp[d]].back() <= tmp;
					 ++ ilp[d]);
				    more = (ilp[d] < mrkep[d] &&
					    reg[ilp[d]][tdm1] < seg.back());
				}
			    }
			    if (! more) break;

			    // assign nxt to the maximum of xx[tdm1]
			    uint32_t nxt = tmp;
			    for (uint32_t d = 0; d < ndm2; ++ d) {
				if (nxt < reg[ilm[d]][tdm1])
				    nxt = reg[ilm[d]][tdm1];
				if (nxt < reg[ilp[d]][tdm1])
				    nxt = reg[ilp[d]][tdm1];
			    }
			    for (uint32_t i = tmp; i < nxt; ++ i) {
				point.back() = i;
				bdy.push_back(point);
			    }
			}

			if (tmp < seg.back()) {
			    for (uint32_t i = tmp; i < seg.back(); ++ i) {
				point.back() = i;
				bdy.push_back(point);
			    }
			}
			else if (point.back() < seg.back()-1) {
			    point.back() = seg.back() - 1;
			    bdy.push_back(point);
			}
		    }
		    else {
			// covered on all sides; only the end points are
			// exposed
			point.back() = seg[tdm1];
			bdy.push_back(point);
			if (point.back() < seg.back()-1) {
			    point.back() = seg.back() - 1;
			    bdy.push_back(point);
			}
		    }
		} // lines in the middle of the plane

		// the last line of the plane is also exposed
		point[ndm2] = seg[tdm2+1] - 1;
		for (uint32_t i = seg[tdm1]; i < seg.back(); ++ i) {
		    point.back() = i;
		    bdy.push_back(point);
		}

		// increament the loop index
		++ loin.back();
		first = false;
		for (uint32_t d = ndm2; d > sdim;) {
		    -- d;
		    if (loin[d] < seg[d+d+1]) {
			break;
		    }
		    else if (d <= sdim) {
			more = false;
		    }
		    else {
			loin[d] = seg[d+d];
			++ loin[d-1];
		    }
		}
	    }
	} // points span multiple dimensions
    } // main loop through each block in the vector reg
} // ibis::meshQuery::boundarynd

int ibis::meshQuery::bitvectorToCoordinates(const ibis::bitvector& bv,
					    const std::vector<uint32_t>& dim,
					    std::vector<uint32_t>& coords) {
    int cnt = 0;
    coords.clear();
    if (bv.cnt() == 0)
	return cnt;

    const uint32_t ndim = dim.size();
    unsigned npoints = 1;
    for (uint32_t i = 0; i < dim.size(); ++ i)
	npoints *= dim[i];
    if (npoints != bv.size()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- query::bitvectorToCoordinates -- "
	    << "the mesh dimensions (" << npoints
	    << ") do not match the bitvector size (" << bv.size() << ")";
	return -1;
    }

    switch (ndim) {
    case 0: {
	LOGGER(ibis::gVerbose > 1)
	    << "query::bitvectorToCoordinates -- nothing to do for a "
	    "0-dimensional mesh";
	break;}
    case 1: {
	for (ibis::bitvector::indexSet ix = bv.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *ind = ix.indices();
	    if (ix.isRange()) {
		for (uint32_t i = *ind; i < ind[1]; ++ i)
		    coords.push_back(i);
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i)
		    coords.push_back(ind[i]);
	    }
	}
	cnt = coords.size();
	break;}
    case 2: {
	for (ibis::bitvector::indexSet ix = bv.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *ind = ix.indices();
	    if (ix.isRange()) {
		for (unsigned i = *ind;
		     i < ind[1]; ++ i) {
		    coords.push_back(i / dim[1]);
		    coords.push_back(i % dim[1]);
		}
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    coords.push_back(ind[i] / dim[1]);
		    coords.push_back(ind[i] % dim[1]);
		}
	    }
	}
	cnt = coords.size() / ndim;
#if DEBUG+0 > 0 || _DEBUG+0 > 0
	ibis::util::logger lg(4);
	lg.buffer() << "DEBUG -- ibis::meshQuery::bitvectorToCoordinates "
		    << "produced " << cnt << " points";
	for (int i = 0; i < cnt; ++ i)
	    lg.buffer() << "\n" << coords[i+i] << ", " << coords[i+i+1];
#endif
	break;}
    case 3: {
	uint32_t tmp[3];
	for (ibis::bitvector::indexSet ix = bv.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *ind = ix.indices();
	    if (ix.isRange()) {
		for (unsigned i = *ind;
		     i < ind[1]; ++ i) {
		    tmp[2] = i % dim[2];
		    tmp[1] = i / dim[2] % dim[1];
		    tmp[0] = i / dim[2] / dim[1];
		    coords.push_back(tmp[0]);
		    coords.push_back(tmp[1]);
		    coords.push_back(tmp[2]);
		}
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    tmp[2] = ind[i] % dim[2];
		    tmp[1] = ind[i] / dim[2] % dim[1];
		    tmp[0] = ind[i] / dim[2] / dim[1];
		    coords.push_back(tmp[0]);
		    coords.push_back(tmp[1]);
		    coords.push_back(tmp[2]);
		}
	    }
	}
	cnt = coords.size() / ndim;
	break;}
    case 4: {
	uint32_t tmp[4];
	for (ibis::bitvector::indexSet ix = bv.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *ind = ix.indices();
	    if (ix.isRange()) {
		for (unsigned i = *ind;
		     i < ind[1]; ++ i) {
		    tmp[3] = i % dim[3];
		    tmp[2] = i / dim[3] % dim[2];
		    tmp[1] = i / dim[3] / dim[2] % dim[1];
		    tmp[0] = i / dim[3] / dim[2] / dim[1];
		    coords.push_back(tmp[0]);
		    coords.push_back(tmp[1]);
		    coords.push_back(tmp[2]);
		    coords.push_back(tmp[3]);
		}
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    tmp[3] = ind[i] % dim[3];
		    tmp[2] = ind[i] / dim[3] % dim[2];
		    tmp[1] = ind[i] / dim[3] / dim[2] % dim[1];
		    tmp[0] = ind[i] / dim[3] / dim[2] / dim[1];
		    coords.push_back(tmp[0]);
		    coords.push_back(tmp[1]);
		    coords.push_back(tmp[2]);
		    coords.push_back(tmp[3]);
		}
	    }
	}
	cnt = coords.size() / ndim;
	break;}
    default: { // higher dimensions
	std::vector<uint32_t> tmp(ndim);
	for (ibis::bitvector::indexSet ix = bv.firstIndexSet();
	     ix.nIndices() > 0; ++ ix) {
	    const ibis::bitvector::word_t *ind = ix.indices();
	    if (ix.isRange()) {
		for (unsigned i = *ind;
		     i < ind[1]; ++ i) {
		    unsigned k = i;
		    for (uint32_t j = ndim-1; j > 0; -- j) {
			tmp[j] = k % dim[j];
			k /= dim[j];
		    }
		    coords.push_back(k);
		    for (uint32_t j = 1; j < ndim; ++ j)
			coords.push_back(tmp[j]);
		}
	    }
	    else {
		for (uint32_t i = 0; i < ix.nIndices(); ++ i) {
		    unsigned k = ind[i];
		    for (uint32_t j = ndim-1; j > 0; -- j) {
			tmp[j] = k % dim[j];
			k /= dim[j];
		    }
		    coords.push_back(k);
		    for (uint32_t j = 1; j < ndim; ++ j)
			coords.push_back(tmp[j]);
		}
	    }
	}
	cnt = coords.size() / ndim;
	break;}
    }
    return cnt;
} // ibis::meshQuery::bitvectorToCoordinates

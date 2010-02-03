// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008-2010 the Regents of the University of California
#ifndef IBIS_JOININ_H
#define IBIS_JOININ_H
/**@file
   @brief In-memory Natual Join.

   This is a concrete implementation of the join operation involving two
   data partitions that can fit in memory.
 */
#include "join.h"	// ibis::join

namespace ibis {
    class jNatural; // forward definition
} // namespace ibis

/// In-memory Natual Join.
class ibis::jNatural : public ibis::join {
public:
    jNatural(const ibis::part& partr, const ibis::part& parts,
	     const char* colname, const char* condr, const char* conds);
    virtual ~jNatural();

    virtual void estimate(uint64_t& nmin, uint64_t& nmax);
    virtual int64_t evaluate();

    virtual ibis::table* select(const std::vector<const char*>& colnames);

protected:
    const ibis::part& R_;
    const ibis::part& S_;
    const ibis::column *colR_;
    const ibis::column *colS_;
    ibis::bitvector maskR_;
    ibis::bitvector maskS_;
    array_t<uint32_t> *orderR_;
    array_t<uint32_t> *orderS_;
    void *valR_;
    void *valS_;
    std::string desc_;
    int64_t nrows;

private:
    jNatural(const jNatural&); // no copying
    jNatural& operator=(const jNatural&); // no assignment
}; // class ibis::jNatural
#endif

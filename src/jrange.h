// File: $Id: jrange.h,v 1.1 2010/03/03 18:48:11 kewu Exp $
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2010 the Regents of the University of California
#ifndef IBIS_JRANGE_H
#define IBIS_JRANGE_H
/**@file
   @brief In-memory Range Join.

   This is a concrete implementation of the range join operation involving two
   data partitions that can fit in memory.
 */
#include "quaere.h"	// ibis::quaere

namespace ibis {
    class jRange; // forward definition
} // namespace ibis

/// In-memory Range Join.  A range join is of a SQL statement of the form
///@code
/// SELECT count(*) FROM partR, partS WHERE partR.colR between partS.colS - delta1 and partS.colS + delta2 and conditions-on-partR and conditions-on-partS;
///@endcode
/// where delta1 and delta2 are constants.
class ibis::jRange : public ibis::quaere {
public:
    jRange(const ibis::part& partr, const ibis::part& parts,
	   const ibis::column& colr, const ibis::column& cols,
	   double delta1, double delta2,
	   const ibis::qExpr* condr, const ibis::qExpr* conds,
	   const char* desc);
    virtual ~jRange();

    virtual void roughCount(uint64_t& nmin, uint64_t& nmax) const;
    virtual int64_t count() const;

    virtual ibis::table* select(const char *sel) const;
    virtual ibis::table* select(const std::vector<const char*>& colnames) const;

protected:
    const ibis::part& partr_;
    const ibis::part& parts_;
    const ibis::column& colr_;
    const ibis::column& cols_;
    ibis::bitvector maskr_;
    ibis::bitvector masks_;
    array_t<uint32_t> *orderr_;
    array_t<uint32_t> *orders_;
    void *valr_;
    void *vals_;
    const double delta1_;
    const double delta2_;
    std::string desc_;
    int64_t nrows;

private:
    jRange(const jRange&); // no copying
    jRange& operator=(const jRange&); // no assignment
}; // class ibis::jRange
#endif

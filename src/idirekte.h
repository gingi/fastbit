//File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2006-2012 the Regents of the University of California
#ifndef IBIS_DIREKTE_H
#define IBIS_DIREKTE_H
///@file
/// This is an implementation of the the simple bitmap index without the
/// first binning step.  It directly uses the integer values as bin number.
/// The word @c direkte in Danish means @c direct.
#include "index.h"

/// Directly use the integer values as bin number to avoid some intemdiate
/// steps.
class ibis::direkte : public ibis::index {
public:
    virtual INDEX_TYPE type() const {return DIREKTE;}
    virtual const char* name() const {return "direct";}

    using ibis::index::evaluate;
    using ibis::index::estimate;
    virtual long evaluate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& hits) const;
    virtual void estimate(const ibis::qContinuousRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qContinuousRange& expr) const;
    virtual float undecidable(const ibis::qContinuousRange& expr,
			      ibis::bitvector& iffy) const {
	iffy.clear();
	return 0.0;
    }

    virtual long evaluate(const ibis::qDiscreteRange& expr,
			  ibis::bitvector& hits) const;
    virtual void estimate(const ibis::qDiscreteRange& expr,
			  ibis::bitvector& lower,
			  ibis::bitvector& upper) const;
    virtual uint32_t estimate(const ibis::qDiscreteRange& expr) const;
    virtual float undecidable(const ibis::qDiscreteRange& expr,
			      ibis::bitvector& iffy) const {
	iffy.clear();
	return 0.0;
    }

    virtual double estimateCost(const ibis::qContinuousRange& expr) const;
    virtual double estimateCost(const ibis::qDiscreteRange& expr) const;

    virtual long select(const ibis::qContinuousRange&, void*) const {
	return -1;}
    virtual long select(const ibis::qContinuousRange&, void*,
			ibis::bitvector&) const {
	return -1;}

    virtual void print(std::ostream& out) const;
    virtual int write(const char* name) const;
    virtual int read(const char* name);
    virtual int read(ibis::fileManager::storage* st);

    /// Extend the index.
    virtual long append(const char* dt, const char* df, uint32_t nnew);

    /// Time some logical operations and print out their speed.
    virtual void speedTest(std::ostream& out) const {};

    virtual void binBoundaries(std::vector<double>&) const;
    virtual void binWeights(std::vector<uint32_t>&) const;

    virtual double getMin() const {return 0.0;}
    virtual double getMax() const {return(bits.size()-1.0);}
    virtual double getSum() const;
    virtual long getCumulativeDistribution
    (std::vector<double>& bds, std::vector<uint32_t>& cts) const;
    virtual long getDistribution
    (std::vector<double>& bbs, std::vector<uint32_t>& cts) const;

    virtual ~direkte() {clear();}
    direkte(const ibis::column* c, const char* f = 0);
    direkte(const ibis::column* c, ibis::fileManager::storage* st);

protected:
    template <typename T>
    int construct(const char* f);

    void locate(const ibis::qContinuousRange& expr,
		uint32_t& hit0, uint32_t& hit1) const;
    virtual size_t getSerialSize() const throw();

    direkte();
    direkte(const direkte&);
    direkte& operator=(const direkte&);
}; // ibis::direkte

#endif

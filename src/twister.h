/* $Id$ */
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2000-2010 the Regents of the University of California
#ifndef IBIS_TWISTER_H
#define IBIS_TWISTER_H
/**@file
 Pseudorandom number generators.

 MersenneTwister:
 A C++ class that use the similar interface as java.util.Random.  The
 basic algorithm is based on the Mersenne Twister by M. Matsumoto and
 T. Nishimura.

 MersenneTwister also include a function called nextZipf to generate Zipf
 distributed random numbers (floats).

 This file also contains additional classes that generate discrete Zipf and
 Poisson distributions (named discreteZipf and discretePoisson)
*/
#include <time.h>	// time_t time clock_t clock
#include <math.h>	// sqrt, log
#include <limits.h>	// DBL_MIN, ULONG_MAX
#include <float.h>	// DBL_MIN

#include <vector>	// std::vector<double> used by discrateZip1

namespace ibis {
    class uniformRandomNumber;	// the abstract base class
    class MersenneTwister;	// concrete uniform random number generator
    class discretePoisson;	// discrete Poisson
    class discretePoisson1;	// the spcial case for exp(-x)
    class discreteZipf;		// discrete Zipf 1/x^a (a > 1)
    class discreteZipf1;	// 1/x
    class discreteZipf2;	// 1/x^2
};

/// A functor to generate uniform random number in the range [0, 1).
class ibis::uniformRandomNumber {
public:
    virtual double operator()() = 0;
};

/// Mersenne Twister generates uniform random numbers efficiently.
class ibis::MersenneTwister : public ibis::uniformRandomNumber {
public:
    // two constructors -- the default constructor uses a seed based on
    // the current time
    MersenneTwister() {setSeed(time(0) ^ clock());}
    MersenneTwister(unsigned seed) {setSeed(seed);}

    virtual double operator()() {return nextDouble();}
    int nextInt() {return next();}
    long nextLong() {return next();}
    float nextFloat() {return 2.3283064365386962890625e-10*next();}
    double nextDouble() {return 2.3283064365386962890625e-10*next();}
    // return integers in the range of [0, r)
    unsigned next(unsigned r) {return static_cast<unsigned>(r*nextDouble());}

    // return a random number with Poisson distribution
    // f(x) = exp(-x)
    double nextPoisson() {return -log(nextDouble());}

    // return a random number with Zipf distribution
    // f(x) = 1 / (1 + x)^2, x >= 0
    double nextZipf() {return (1.0 / (1.0 - nextDouble()) - 1.0);}

    // return a random number with Zipf distribution
    // f(x) = {(a-1) \over (1 + x)^a}, a > 1 and x >= 0
    double nextZipf(double a) {
	if (a > 1.0)
	    return (exp(-log(1-nextDouble())/(a-1)) - 1);
	else
	    return -1.0;
    }

    // returns random numbers in Gaussian distribution -- based on the
    // Box-Mueller transformation
    double nextGaussian() {
	if (has_gaussian != 0) { /* has extra value from the previous run */
	    has_gaussian = 0;
	    return gaussian_extra;
	}
	else {
	    double v1, v2, r, fac;
	    do {
		v1 = 4.656612873077392578125e-10*next() - 1.0;
		v2 = 4.656612873077392578125e-10*next() - 1.0;
		r = v1 * v1 + v2 * v2;
	    } while (r >= 1.0);
	    fac = sqrt(-2.0 * log((double) r)/r);
	    gaussian_extra = v2 * fac;
	    v1 *= fac;
	    has_gaussian = 1;
	    return v1;
	}
    }

    // Initializing the array with a seed
    void setSeed(unsigned seed) {
	for (int i=0; i<624; i++) {
	    mt[i] = seed & 0xffff0000;
	    seed = 69069 * seed + 1;
	    mt[i] |= (seed & 0xffff0000) >> 16;
	    seed = 69069 * seed + 1;
	}
	mti = 624;
	has_gaussian = 0;
    }

    // generate the next random integer in the range of 0-2^{32}-1
    unsigned next() {
	unsigned y;
	if (mti >= 624) { /* generate 624 words at one time */
	    static unsigned mag01[2]={0x0, 0x9908b0df};
	    int kk;

	    for (kk=0; kk<227; kk++) {
		y = (mt[kk]&0x80000000)|(mt[kk+1]&0x7fffffff);
		mt[kk] = mt[kk+397] ^ (y >> 1) ^ mag01[y & 0x1];
	    }
	    for (; kk<623; kk++) {
		y = (mt[kk]&0x80000000)|(mt[kk+1]&0x7fffffff);
		mt[kk] = mt[kk-227] ^ (y >> 1) ^ mag01[y & 0x1];
	    }
	    y = (mt[623]&0x80000000)|(mt[0]&0x7fffffff);
	    mt[623] = mt[396] ^ (y >> 1) ^ mag01[y & 0x1];
	    mti = 0;
	}
  
	y = mt[mti++];
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680;
	y ^= (y << 15) & 0xefc60000;
	y ^= (y >> 18);

	return y; 
    }

private:
    // all data members are private
    int mti;
    int has_gaussian;
    double gaussian_extra;
    unsigned mt[624]; /* the array for the state vector  */
}; // class MersenneTwister

// ********** the following random number generators need a uniformation
// ********** random number generator as the input

/// Discrete random number with Poisson distribution exp(-x/lambda).
/// Use the rejection-inversion algorithm of W. Hormann and G. Derflinger.
class ibis::discretePoisson {
public:
    discretePoisson(ibis::uniformRandomNumber* ur,
		    const double lam=1.0, long m=0)
	: min0(m), lambda(lam), urand(ur) {init();}

    long operator()() {return next();}
    long next() {
	long k;
	double u, x;
	while (true) {
	    u = ym * (*urand)();
	    x = - lambda * log(u * laminv);
	    k = static_cast<long>(x + 0.5);
	    if (k <= k0 && k-x <= xm)
		return k;
	    else if (u >= -exp(laminv*k+laminv2)*lambda-exp(laminv*k))
		return k;
	}
    } // next integer random number

private:
    // private member variables
    long min0, k0;
    double lambda, laminv, laminv2, xm, ym;
    uniformRandomNumber* urand;

    // private functions
    void init() { // check input parameters and initialize three constants
	if (! (lambda > DBL_MIN))
	    lambda = 1.0;
	laminv = -1.0 / lambda;
	laminv2 = 0.5*laminv;
	k0 = static_cast<long>(1.0+min0+1.0/(1.0-exp(laminv)));
	ym = -exp((min0+0.5)*laminv)*lambda - exp(min0*laminv);
	xm = min0 - log(ym*laminv);
    }
}; // class discretePoisson

/// Specialized version of the Poisson distribution exp(-x).
class ibis::discretePoisson1 {
public:
    discretePoisson1(ibis::uniformRandomNumber* ur) : urand(ur) {init();}

    long operator()() {return next();}
    long next() {
	long k;
	double u, x;
	while (true) {
	    u = ym * (*urand)();
	    x = - log(-u);
	    k = static_cast<long>(x + 0.5);
	    if (k <= k0 && k-x <= xm)
		return k;
	    else if (u >= -exp(-static_cast<double>(k)-0.5) -
		     exp(-static_cast<double>(k)))
		return k;
	}
    } // next integer random number

private:
    // private member variables
    double xm, ym;
    long k0;
    uniformRandomNumber* urand;

    // private functions
    void init() { // check input parameters and initialize three constants
	k0 = static_cast<long>(1.0+1.0/(1.0-exp(-1.0)));
	ym = - exp(-0.5) - 1.0;
	xm = - log(-ym);
    }
}; // class discretePoisson1

/// Discrete Zipf distribution: p(k) is proportional to (v+k)^(-a) where a
/// > 1, k >= 0.  It uses the rejection-inversion algorithm of W. Hormann
/// and G. Derflinger.  The values generated are in the range of [0, imax]
/// (inclusive, both ends are included).
class ibis::discreteZipf {
public:
    discreteZipf(ibis::uniformRandomNumber* ur, double a=2.0,
		 unsigned long v=1, unsigned long imax = ULONG_MAX) :
	min0(v), max0(imax), alpha(a), urand(ur) {init();}

    // return a discrete random number in the range of [0, imax]
    unsigned long operator()() {return next();}
    unsigned long next() {
	while (true) {
	    double ur = (*urand)();
	    ur = hxm + ur * hx0;
	    double x = Hinv(ur);
	    unsigned long k = static_cast<unsigned long>(0.5+x);
	    if (k - x <= ss)
		return k;
	    else if (ur >= H(0.5+k) - exp(-log(static_cast<double>
					       (k+min0))*alpha))
		return k;
	}
    } // next

private:
    // private member variables
    long unsigned min0, max0;
    double alpha, alpha1, alphainv, hx0, hxm, ss;
    uniformRandomNumber* urand;

    // private member function
    double H(double x) {return (exp(alpha1*log(min0+x)) * alphainv);}
    double Hinv(double x) {return exp(alphainv*log(alpha1*x)) - min0;}
    void init() {
	// enforce the condition that alpha > 1 and min0 >= 1
	if (! (alpha > 1.0))
	    alpha = 2.0;
	if (min0 < 1)
	    min0 = 1;
	alpha1 = 1.0 - alpha;
	alphainv = 1.0 / alpha1;
	hxm = H(max0+0.5);
	hx0 = H(0.5) - exp(log(static_cast<double>(min0))*(-alpha)) - hxm;
	ss = 1 - Hinv(H(1.5)-exp(-alpha*log(static_cast<double>(min0)+1.0)));
    }
}; // Zipf distribution

/// A specialized version of the Zipf distribution f(x) = 1/(1+x)^2.
/// Should be much faster than using discreteZipf(2,1,imax).
class ibis::discreteZipf2 {
public:
    discreteZipf2(ibis::uniformRandomNumber* ur,
		  unsigned long imax = ULONG_MAX) :
	max0(imax), urand(ur) {init();}

    /// Return a discrete random number in the range of [0, imax]
    unsigned long operator()() {return next();}
    unsigned long next() {
	while (true) {
	    double ur = (*urand)();
	    ur = hxm + ur * hx0;
	    double x = Hinv(ur);
	    unsigned long k = static_cast<unsigned long>(0.5+x);
	    if (k - x <= ss)
		return k;
	    else if (ur >= H(0.5+k) - 1.0/((1.0+x)*(1.0+x)))
		return k;
	}
    } // next

private:
    // private member variables
    double hx0, hxm, ss;
    long unsigned max0;
    uniformRandomNumber* urand;

    // private member function
    double H(double x) {return -1.0 / (1.0 + x);}
    double Hinv(double x) {return (- 1.0 / x) - 1.0;}
    void init() {
	hxm = H(max0+0.5);
	hx0 = - 5.0/3.0 - hxm;
	ss = 1 - Hinv(H(1.5)-0.25);
    }
}; // Zipf2 distribution

/// A specialized case of the Zipf distribution f(x) = 1/(1+x).  The
/// general case discrateZipf requires a > 1.
class ibis::discreteZipf1 {
public:
    discreteZipf1(ibis::uniformRandomNumber* ur, unsigned long imax = 100) :
	card(imax+1), cpd(imax+1), urand(ur) {init();}

    // return a discrete random number in the range of [0, imax]
    unsigned long operator()() {return next();}
    unsigned long next() {
	double ur = (*urand)();
	if (ur <= cpd[0]) return 0;
	// return the minimal i such that cdf[i] >= ur
	unsigned long i, j, k;
	i = 0;
	j = card-1;
	k = (i + j) / 2;
	while (i < k) {
	    if (cpd[k] > ur)
		j = k;
	    else if (cpd[k] < ur)
		i = k;
	    else
		return k;
	    k = (i + j) / 2;
	}
	if (cpd[i] >= ur)
	    return i;
	else
	    return j;
    } // next

private:
    // private member variables
    const unsigned long card;
    std::vector<double> cpd; // cumulative probability distribution
    uniformRandomNumber* urand;

    // private member function
    void init() { // generates the cpd
	const unsigned n = cpd.size();
	if (n < 2 || n > 1024*1024 || card != n)
	    throw "imax must be in [2, 1000000]";

	cpd[0] = 1.0;
	for (unsigned i = 1; i < n; ++i)
	    cpd[i] = cpd[i-1] + 1.0 / (1.0 + i);
	double ss = 1.0 / cpd.back();
	for (unsigned i = 0; i < n; ++i)
	    cpd[i] *= ss;
    } // init
}; // Zipf1 distribution
#endif // IBIS_TWISTER_H

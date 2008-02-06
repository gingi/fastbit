// $Id$
// Author: John Wu <John.Wu@ACM.org>
// Copyright 2000-2008 the Regents of the University of California
#ifndef IBIS_HOROMETER_H
#define IBIS_HOROMETER_H
#include <stdio.h>
#include <time.h> // clock, clock_gettime
#if defined(sun) || defined(linux) || defined(__HOS_AIX__) || \
    defined(__CYGWIN__) || defined(__APPLE__) || defined(__FreeBSD__)
#   include <limits.h> // CLK_TCK
#   include <sys/time.h> // gettimeofday, timeval
#   include <sys/times.h> // times, struct tms
#   include <sys/resource.h> // getrusage
#   ifndef RUSAGE_SELF
#       define RUSAGE_SELF 0
#   endif
#   ifndef RUSAGE_CHILDREN
#       define RUSAGE_CHILDRED -1
#   endif
#elif defined(CRAY)
#   include <sys/times.h> // times
#elif defined(sgi)
#   include <limits.h> // CLK_TCK
#   define RUSAGE_SELF      0         /* calling process */
#   define RUSAGE_CHILDREN  -1        /* terminated child processes */
#   include <sys/times.h> // times
//#   include <sys/types.h> // struct tms
#   include <sys/time.h> // gettimeofday, getrusage
#   include <sys/resource.h> // getrusage
#elif defined(_WIN32)
#   include <windows.h>
#elif defined(VMS)
#   include <unistd.h>
#endif

/// @file
/// Defines a simple timer class.
namespace ibis {
    class horometer;
}

/**
 Horometer -- a primitive timing instrument.

 This is intented to be a simple timer.  It must be explicitly started by
 calling function start.  The same function start may be called to restart
 the timer which will discard previously marked starting point.  The
 function stop must be called before functions realTime and CPUTime can
 report the correct time.  After a horometer is stopped, it may restart by
 calling start, or it may resume timing by calling resume.

 Timing accuracy depends on the underlying implementation.  On most unix
 systems, the time resolution is about 0.01 seconds.  The timing function
 itself may take ~10,000 clock cycles to execute, which is about 30
 microseconds on a 400 MHz machine.
*/
class ibis::horometer {
public:
    horometer() : startRealTime(0), totalRealTime(0),
		  startCPUTime(0), totalCPUTime(0) {
#if defined(_WIN32)
	// the frequency of the high-resolution performance counter
	LARGE_INTEGER lFrequency;
	BOOL ret = QueryPerformanceFrequency(&lFrequency);
	if (ret != 0 && lFrequency.QuadPart != 0)
	    countPeriod = 1.0/static_cast<double>(lFrequency.QuadPart);
	else
	    countPeriod = 0.0;
#endif
    };
    void start() {///< Start the timer.  Clear the internal counters.
	startRealTime = readWallClock();
	startCPUTime = readCPUClock();
	totalRealTime = 0;
	totalCPUTime = 0;
    };
    void stop() { ///< Stop the timer.  May resume later.
	totalRealTime += readWallClock() - startRealTime;
	totalCPUTime += readCPUClock() - startCPUTime;
    };
    void resume() {///< Continue after being stopped.
	startRealTime = readWallClock();
	startCPUTime = readCPUClock();
    }
    /// Return the elapsed time in seconds.
    double realTime() const {return totalRealTime;}
    /// Return the CPU time in seconds.
    double CPUTime() const {return totalCPUTime;}

private:
    double startRealTime;   // wall clock start time
    double totalRealTime;   // total real time
    double startCPUTime;    // cpu start time
    double totalCPUTime;    // total cpu time
#if defined(_WIN32)
    double countPeriod;     // time of one high-resolution count
#endif

    // The following two functions are the guts of this horometer.  They
    // read the system clock to report the current wall clock time and the
    // CPU clock time -- only these two functions are machine dependend.
    inline double readWallClock(); ///< Read the system's wallclock time.
    inline double readCPUClock();  ///< Read the CPU time.
};

// read the system's wall clock time
inline double ibis::horometer::readWallClock() {
#if defined(CLOCK_REALTIME) && !defined(__CYGWIN__)
    struct timespec tb;
    if (0 == clock_gettime(CLOCK_REALTIME, &tb)) {
	return (double) tb.tv_sec + tb.tv_nsec * 1e-9;
    }
    else {
	struct timeval cpt;
	gettimeofday(&cpt, 0);
	return static_cast<double>(cpt.tv_sec) + 1e-6*cpt.tv_usec;
    }
#elif defined(unix) || defined(CRAY) || defined(linux) || defined(__HOS_AIX__) || defined(__APPLE__) || defined(__FreeBSD__)
    struct timeval cpt;
    gettimeofday(&cpt, 0);
    return static_cast<double>(cpt.tv_sec) + 1e-6*cpt.tv_usec;
#elif defined(_WIN32)
    double ret = 0.0;
    if (countPeriod != 0) {
	LARGE_INTEGER cnt;
	if (QueryPerformanceCounter(&cnt)) {
	    ret = countPeriod * cnt.QuadPart;
	}
    }
    if (ret == 0.0) { // fallback option -- use GetSystemTime
	union {
	    FILETIME ftFileTime;
	    __int64  ftInt64;
	} ftRealTime;
	GetSystemTimeAsFileTime(&ftRealTime.ftFileTime);
	ret = (double) ftRealTime.ftInt64 * 1e-7;
    }
    return ret;
#elif defined(VMS)
    return(double) clock() * 0.001;
#else
    return(double) clock() / CLOCKS_PER_SEC;
#endif
} //  double horometer::readWallClock()

// read the value of the CPU clock time
inline double ibis::horometer::readCPUClock() {
#if defined(sun) || defined(sgi) || defined(linux) || defined(__APPLE__) \
    || defined(__HOS_AIX__) || defined(__CYGWIN__) || defined(__FreeBSD__)
    // on sun and linux, we can access getrusage to get more accurate time
    double time=0;
    struct rusage ruse;
    if (0 == getrusage(RUSAGE_SELF, &ruse)) {
	time = (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec) * 1e-6 +
	    ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec;
    }
    else {
	fputs("Warning -- horometer::readCPUClock(): getrusage failed "
	      "on RUSAGE_SELF", stderr);
    }
    if (0 == getrusage(RUSAGE_CHILDREN, &ruse)) {
	time += (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec) * 1e-6 +
	    ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec;
    }
    else {
	fputs("Warning -- horometer::readCPUClock(): getrusage failed on "
	      "RUSAGE_CHILDRED", stderr);
    }
    return time;
#elif defined(unix) || defined(CRAY)
#if defined(__STDC__)
    struct tms cpt;
    times(&cpt);
    return (cpt.tms_utime + cpt.tms_stime + cpt.tms_cutime +
	    (double)cpt.tms_cstime) / CLK_TCK;
#else
    return (double) times() / CLK_TCK;
#endif
#elif defined(_WIN32)
    return (double) clock() / CLOCKS_PER_SEC;
#elif defined(VMS)
    return (double) clock() * 0.001;
#else
    return (double) clock() / CLOCKS_PER_SEC;
#endif
} // double horometer::readCPUClock()
#endif // IBIS_HOROMETER_H

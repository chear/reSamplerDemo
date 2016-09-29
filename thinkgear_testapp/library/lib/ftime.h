/*
 * @(#)ftime.h    1.0    Jun 02, 2008
 *
 * Copyright (c) 2008 NeuroSky, Inc. All Rights Reserved
 * NEUROSKY PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/* Ensure this header is only included once */
#ifndef FTIME_H_
#define FTIME_H_

/**
 * @file ftime.h
 *
 * This header provides the TIMEB structure and FTIME(x) function, which
 * bridge the portability gap between Win32 and UNIX platforms for the
 * struct _timeb/timeb and _ftime(x)/ftime(x) symbols respectively.  See
 * sample code below.
 *
 * @author Kelvin Soo
 * @version 1.0 Jun 02, 2008 Kelvin Soo
 *   - Initial version.
 */

#if 0
    /* SAMPLE USAGE CODE: Declare TIMEB object, get current time, print time */
    struct TIMEB currTime;
    FTIME( &currTime );
    printf( "Timestamp: %ld.%03u\n", currTime.time, currTime.millitm );
#endif

/* Include all external libraries required by this header */

/* Disable name-mangling when compiling as C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32_WCE)

#include <windows.h>
struct timeb {
    time_t         time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};
#define TIMEB timeb
#define FTIME(x) \
    SYSTEMTIME ___t;  \
    GetLocalTime( &___t );  \
    (x)->time    = ___t.wHour*60*60 + ___t.wMinute*60 + ___t.wSecond; \
    (x)->millitm = ___t.wMilliseconds;

#elif defined(_WIN32)

#include <sys/types.h>
#include <sys/timeb.h>
#define TIMEB _timeb
#define FTIME(x) _ftime(x)

#else /* !_WIN32_WCE and !_WIN32 */

#include <sys/types.h>
#include <sys/timeb.h>
#define TIMEB timeb
#define FTIME(x) ftime(x)

#endif /* !_WIN32_WCE and !_WIN32 */

/* NOTE:
 * GNU Linux uses gettimeofday() returning 0 or -1+errno for FTIME(), and
 *   struct timeval {
 *     long int tv_sec  - number of whole seconds of elapsed time.
 *     long int tv_usec - This is the rest of the elapsed time (a fraction
 *                        of a second), represented as the number of
 *                        microseconds. It is always less than one million.
 *   } for TIMEB.
 */

/*
 * Internals of the struct TIMEB as described by OSX man page and MSDN,
 * reprinted here for reference:
 *
 * struct TIMEB {
 *     time_t         time;
 *     unsigned short millitm;
 *     short          timezone;
 *     short          dstflag;
 * };
 *
 * The structure contains the time since the epoch in seconds, up to 1000
 * milliseconds of more-precise interval, the local time zone (measured in
 * minutes of time westward from Greenwich), and a flag that, if nonzero,
 * indicates that Daylight Saving time applies locally during the appropri-
 * ate part of the year.
 *
 * time     - Time in seconds since midnight (00:00:00), January 1, 1970,
 *            coordinated universal time (UTC).
 * millitm  - Fraction of a second in milliseconds.
 * timezone - Difference in minutes, moving westward, between UTC and
 *            local time. The value of timezone is set from the value
 *            of the global variable _timezone (see _tzset).
 * dstflag  - Nonzero if daylight savings time is currently in effect for
 *            the local time zone. (See _tzset for an explanation of how
 *            daylight savings time is determined.)
 *
 */

#if defined(__cplusplus)
}  /* extern "C" */
#endif

#endif /* FTIME_H_ */

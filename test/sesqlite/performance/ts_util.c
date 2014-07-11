/* $Id: ts_util.c,v 1.18 2010/03/08 18:38:53 alex Exp $ */
/*******************************************************************************

File:

    ts_util.c


Author:    Alex Measday


Purpose:

    These are utility functions used to manipulate (e.g., add and
    subtract) POSIX "timespec"s, which represent time in seconds
    and nanoseconds.


Procedures:


    tsAdd() - adds two TIMESPECs.
    tsCompare() - compares two TIMESPECs.
    tsCreate() - creates a TIMESPEC from a time expressed in seconds and
        nanoseconds.
    tsCreateF() - creates a TIMESPEC from a time expressed as a
        floating-point number of seconds.
    tsFloat() - converts a TIMESPEC to a floating-point number of seconds.
    tsShow() - returns a printable representation of a TIMESPEC.
    tsSubtract() - subtracts one TIMESPEC from another.
    tsTOD() - returns the current time-of-day (GMT).

*******************************************************************************/

#include  <stdio.h>			/* Standard I/O definitions. */
#include  <stdlib.h>			/* Standard C library definitions. */
#include  <string.h>			/* Standard C string functions. */
#include  <time.h>			/* Time definitions. */
#include  "ts_util.h"           /* "timespec" manipulation functions. */
// #include  "tv_util.h"			/* "timeval" manipulation functions. */

/*!*****************************************************************************

Procedure:

    tsAdd ()


Purpose:

    Function tsAdd() returns the sum of two TIMESPECs.


    Invocation:

        result = tsAdd (time1, time2) ;

    where

        <time1>		- I
            is the first operand, a time represented by a POSIX TIMESPEC
            structure.
        <time2>		- I
            is the second operand, a time represented by a POSIX TIMESPEC
            structure.
        <result>	- O
            returns, in a POSIX TIMESPEC structure, the sum of TIME1 and
            TIME2.

*******************************************************************************/


struct  timespec  tsAdd (
    struct  timespec  time1,
    struct  timespec  time2
){    

/* Local variables. */
    struct  timespec  result ;

/* Add the two times together. */

    result.tv_sec = time1.tv_sec + time2.tv_sec ;
    result.tv_nsec = time1.tv_nsec + time2.tv_nsec ;
    if (result.tv_nsec >= 1000000000L) {		/* Carry? */
        result.tv_sec++ ;  result.tv_nsec = result.tv_nsec - 1000000000L ;
    }

    return (result) ;

}

/*!*****************************************************************************

Procedure:

    tsFloat ()


Purpose:

    Function tsFloat() converts a TIMESPEC structure (seconds and nanoseconds)
    into the equivalent, floating-point number of seconds.


    Invocation:

        realTime = tsFloat (time) ;

    where

        <time>		- I
            is the TIMESPEC structure that is to be converted to floating point.
        <realTime>	- O
            returns the floating-point representation in seconds of the time.

*******************************************************************************/


double  tsFloat (
    struct  timespec  time
){

    return ((double) time.tv_sec + (time.tv_nsec / 1000000000.0)) ;

}

/*!*****************************************************************************

Procedure:

    tsSubtract ()


Purpose:

    Function tsSubtract() subtracts one TIMESPEC from another TIMESPEC.


    Invocation:

        result = tsSubtract (time1, time2) ;

    where

        <time1>		- I
            is the minuend, a time represented by a POSIX TIMESPEC structure.
        <time2>		- I
            is the subtrahend, a time represented by a POSIX TIMESPEC structure.
        <result>	- O
            returns, in a POSIX TIMESPEC structure, TIME1 minus TIME2.  If
            TIME2 is greater than TIME1, then a time of zero is returned.

*******************************************************************************/


struct  timespec  tsSubtract (
    struct  timespec  time1,
    struct  timespec  time2
){    

/* Local variables. */
    struct  timespec  result ;

/* Subtract the second time from the first. */

    if ((time1.tv_sec < time2.tv_sec) ||
        ((time1.tv_sec == time2.tv_sec) &&
         (time1.tv_nsec <= time2.tv_nsec))) {		/* TIME1 <= TIME2? */
        result.tv_sec = result.tv_nsec = 0 ;
    } else {						/* TIME1 > TIME2 */
        result.tv_sec = time1.tv_sec - time2.tv_sec ;
        if (time1.tv_nsec < time2.tv_nsec) {
            result.tv_nsec = time1.tv_nsec + 1000000000L - time2.tv_nsec ;
            result.tv_sec-- ;				/* Borrow a second. */
        } else {
            result.tv_nsec = time1.tv_nsec - time2.tv_nsec ;
        }
    }

    return (result) ;

}

/*!*****************************************************************************

Procedure:

    tsTOD ()


Purpose:

    Function tsTOD() returns the current time-of-day (GMT).  tsTOD()
    simply calls the POSIX function, CLOCK_GETTIME(3), but it provides
    a simple means of using the current time in "timespec" calculations
    without having to allocate a special variable for it and making a
    separate call (i.e., you can "inline" a call to tsTOD() within a
    call to one of the other TS_UTIL functions).


    Invocation:

        currentGMT = tsTOD () ;

    where

        <currentGMT>	- O
            returns, in a POSIX TIMESPEC structure, the current GMT.

*******************************************************************************/


struct  timespec  tsTOD ()
{

/* Local variables. */
    struct  timespec  timeOfDay ;

    clock_gettime (CLOCK_MONOTONIC, &timeOfDay) ;
    return (timeOfDay) ;

}

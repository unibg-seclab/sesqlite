/* $Id: ts_util.h,v 1.11 2009/09/09 22:38:13 alex Exp $ */
/*******************************************************************************

    ts_util.h

    "timespec" Manipulation Definitions.

*******************************************************************************/

#ifndef  TS_UTIL_H		/* Has the file been INCLUDE'd already? */
#define  TS_UTIL_H

#include  <time.h>		/* System time definitions. */

/*******************************************************************************
    Public functions.
*******************************************************************************/

extern  struct  timespec tsAdd (struct timespec time1,
                                struct timespec time2);

extern  double  tsFloat (struct timespec time);

extern  struct  timespec  tsSubtract (struct timespec time1,
                                      struct timespec time2);

extern  struct  timespec  tsTOD();

#endif				/* If this file was not INCLUDE'd previously. */

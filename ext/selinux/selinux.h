/*
 * sesqlite.h
 *
 *  Created on: Jun 10, 2014
 *      Author: mutti
 */

#include "sqlite3.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int sqlite3SelinuxInit(sqlite3 *db);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

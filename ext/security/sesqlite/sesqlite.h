/* * sesqlite.h
 *
 *  Created on: Jun 10, 2014
 *      Author: mutti
 */

#include "sqlite3.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <selinux/selinux.h>

#include "sesqlite_hash.h"

extern security_context_t scon;
extern security_context_t tcon;
extern int scon_id;
extern int tcon_id;

extern seSQLiteHash *hash;
extern seSQLiteBiHash *hash_id;

extern sqlite3_stmt *stmt_insert;
extern sqlite3_stmt *stmt_update;
extern sqlite3_stmt *stmt_con_insert;

extern int set_vacuum(int type); 
extern int is_vacuum(); 

#define SECURITY_CONTEXT_COLUMN_NAME "security_context"
#define SECURITY_CONTEXT_COLUMN_TYPE "hidden INT"
#define SECURITY_CONTEXT_COLUMN_DEFINITION SECURITY_CONTEXT_COLUMN_NAME " " SECURITY_CONTEXT_COLUMN_TYPE

#define SELINUX_CONTEXT "selinux_context"
#define SELINUX_ID "selinux_id"

const char *authtype[] = { "SQLITE_COPY", "SQLITE_CREATE_INDEX",
		"SQLITE_CREATE_TABLE", "SQLITE_CREATE_TEMP_INDEX",
		"SQLITE_CREATE_TEMP_TABLE", "SQLITE_CREATE_TEMP_TRIGGER",
		"SQLITE_CREATE_TEMP_VIEW", "SQLITE_CREATE_TRIGGER",
		"SQLITE_CREATE_VIEW", "SQLITE_DELETE", "SQLITE_DROP_INDEX",
		"SQLITE_DROP_TABLE", "SQLITE_DROP_TEMP_INDEX", "SQLITE_DROP_TEMP_TABLE",
		"SQLITE_DROP_TEMP_TRIGGER", "SQLITE_DROP_TEMP_VIEW",
		"SQLITE_DROP_TRIGGER", "SQLITE_DROP_VIEW", "SQLITE_INSERT",
		"SQLITE_PRAGMA", "SQLITE_READ", "SQLITE_SELECT", "SQLITE_TRANSACTION",
		"SQLITE_UPDATE", "SQLITE_ATTACH", "SQLITE_DETACH", "SQLITE_ALTER_TABLE",
		"SQLITE_REINDEX", "SQLITE_ANALYZE", "SQLITE_CREATE_VTABLE",
		"SQLITE_DROP_VTABLE", "SQLITE_FUNCTION", "SQLITE_SAVEPOINT",
		"SQLITE_RECURSIVE" };

/* SELinux classes */
#define SELINUX_DB_DATABASE      	0
#define SELINUX_DB_TABLE      		1
#define SELINUX_DB_COLUMN     		2
#define SELINUX_DB_TUPLE    		3

/* SELinux permissions */
/* common database */
#define SELINUX_CREATE 			0
#define SELINUX_DROP 			1
#define SELINUX_GETATTR 		2
#define SELINUX_SETATTR 		3
#define SELINUX_RELABEL_FROM 		4
#define SELINUX_RELABEL_TO 		5

/* db_xxx */
#define SELINUX_SELECT 			6
#define SELINUX_UPDATE 			7
#define SELINUX_INSERT 			8
#define SELINUX_DELETE 			9

/* other */
#define SELINUX_LOAD_MODULE 		10

#define SELINUX_NELEM_CLASS		3
#define SELINUX_NELEM_PERM		10



extern struct sesqlite_context *sesqlite_contexts;

int lookup_security_context(seSQLiteBiHash *hash, 
	char *db_name, 
	char *tbl_name);

int lookup_security_label(sqlite3 *db, 
	sqlite3_stmt *stmt, 
	seSQLiteBiHash *hash, 
	int type, 
	char *db_name, 
	char *tbl_name, 
	char *col_name);


/* */
int sqlite3SelinuxInit(sqlite3 *db);

struct sesqlite_context {

	int ndb_context;
	int ntable_context;
	int ncolumn_context;
	int ntuple_context;

	struct sesqlite_context_element *db_context;
	struct sesqlite_context_element *table_context;
	struct sesqlite_context_element *column_context;
	struct sesqlite_context_element *tuple_context;
};

/**
 * Used to store
 */
struct sesqlite_context_element {
	char *origin;
	char *fparam;
	char *sparam;
	char *tparam;
	char *security_context;
	struct sesqlite_context_element *next;
};

/* struct for the management of selinux classes and the relative permissions*/
static struct {
	const char *c_name;
	uint16_t c_code;
	struct {
		const char *p_name;
		uint16_t p_code;
	} perm[32];

} access_vector[] = { { "db_database", SELINUX_DB_DATABASE, { { "create",
SELINUX_CREATE }, { "drop", SELINUX_DROP }, { "getattr", SELINUX_GETATTR }, {
		"setattr", SELINUX_SETATTR }, { "relabelfrom",
SELINUX_RELABEL_FROM }, { "relabelto",
SELINUX_RELABEL_TO }, } }, { "db_table",
SELINUX_DB_TABLE, { { "create", SELINUX_CREATE }, { "drop",
SELINUX_DROP }, { "getattr", SELINUX_GETATTR }, { "setattr", SELINUX_SETATTR },
		{ "relabelfrom",
		SELINUX_RELABEL_FROM }, { "relabelto",
		SELINUX_RELABEL_TO }, { "select", SELINUX_SELECT }, { "update",
		SELINUX_UPDATE }, { "insert", SELINUX_INSERT }, { "delete",
		SELINUX_DELETE }, } }, { "db_column",
SELINUX_DB_COLUMN, { { "create", SELINUX_CREATE }, { "drop", SELINUX_DROP }, {
		"getattr", SELINUX_GETATTR }, { "setattr",
SELINUX_SETATTR }, { "relabelfrom", SELINUX_RELABEL_FROM }, { "relabelto",
SELINUX_RELABEL_TO }, { "select", SELINUX_SELECT },
		{ "update", SELINUX_UPDATE }, { "insert", SELINUX_INSERT }, } }, {
		"db_tuple",
		SELINUX_DB_TUPLE, { { "relabelfrom", SELINUX_RELABEL_FROM }, {
				"relabelto",
				SELINUX_RELABEL_TO }, { "select", SELINUX_SELECT }, { "update",
		SELINUX_UPDATE }, { "insert", SELINUX_INSERT }, { "delete",
		SELINUX_DELETE }, } }, };





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

/* comment the following line to exit on errors */
#define NOOP_EXIT

#ifdef NOOP_EXIT
  #define EXIT(...)
#else
  #define EXIT(...) exit(__VA_ARGS)
#endif

typedef struct Row {
  int n;
  char **cols;
} Row;

typedef struct Table {
  int m;
  Row *rows;
} Table;

typedef struct ResultSet {
  int cursor;
  Table table;
} ResultSet;

#define NUMARGS(T, ...) (sizeof((T[]){__VA_ARGS__})/sizeof(T))
#define ROW(...)   (Row)   {.n = NUMARGS(char*,__VA_ARGS__), .cols = (char*[]){__VA_ARGS__}}
#define TABLE(...) (Table) {.m = NUMARGS(Row,  __VA_ARGS__), .rows = (Row[])  {__VA_ARGS__}}
#define RESULTSET(...) (ResultSet) {.cursor = 0, .table = TABLE(__VA_ARGS__)}

#define SQLITE_INIT \
  char *zErrMsg = 0; \
  int test_rc; \
  ResultSet *test_rs;

#define SQLITE_OPEN( DB, FILE ) \
  ({ \
    test_rc = sqlite3_open(FILE, &DB); \
    if( test_rc ){ \
      fprintf(stderr, "Error open: %s\n", sqlite3_errmsg(DB)); \
      EXIT(1); \
    } \
    test_rc; \
  })

#define SQLITE_EXEC_CALLBACK( DB, SQL, CALLBACK, ARG ) \
  ({ \
    test_rc = sqlite3_exec(DB, SQL, CALLBACK, ARG, &zErrMsg); \
    test_rc; \
  })

#define SQLITE_EXEC( DB, SQL ) SQLITE_EXEC_CALLBACK(DB, SQL, 0, 0)

#define SQLITE_ASSERT( DB, SQL, ... ) \
  ({ \
    test_rs = &RESULTSET(__VA_ARGS__); \
    test_rc = SQLITE_EXEC_CALLBACK(DB, SQL, check_select, test_rs); \
    if ( test_rc == SQLITE_OK && test_rs->table.m > test_rs->cursor ){ \
      fprintf(stderr, "SQLite returned less rows than expected.\n"); \
      test_rc = SQLITE_ERROR \
      EXIT(1); \
    } \
  test_rc; \
  })

int check_select(
  void *pArg,
  int argc,
  char **argv,
  char **columns
){
  int j;
  ResultSet *rs = (ResultSet*) pArg;

  /* Check if the expected resultset has more rows */
  if( rs->table.m <= rs->cursor ){
    fprintf(stderr, "SQLite returned more rows than expected.\n");
    return SQLITE_ABORT;
  }

  Row row = rs->table.rows[rs->cursor];
  /* Check if the row has the right number of columns */
  if( argc != row.n ){
    fprintf(stderr, "Column number mismatch. row: %d, expected: %d, got: %d\n",
            rs->cursor, row.n, argc);
    return SQLITE_ABORT;
  }

  /* Check values */
  for( j = 0; j < argc; ++j ){
    if( strcmp(argv[j], row.cols[j]) != 0 ){
      fprintf(stderr, "Value mismatch. row: %d, col: %d, expected: %s, got: %s\n",
              rs->cursor, j, row.cols[j], argv[j]);
      return SQLITE_ABORT;
    }
  }

  /* Move cursor to next line */
  rs->cursor++;
  return SQLITE_OK;
}


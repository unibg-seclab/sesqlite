#include "sqlite_test_utils.h"

int main( int argc, char **argv ){
  SQLITE_INIT
  int rc;

  rc = SQLITE_OPEN( db, ":memory:" );
  rc = SQLITE_EXEC( db, "CREATE TABLE test(a int, b int, c int, d int)" );
  rc = SQLITE_EXEC( db, "INSERT INTO test VALUES (2,2,3,4), (3,4,5,6)" );
  rc = SQLITE_ASSERT( db, "SELECT * FROM test",
    ROW( "1", "2", "3", "4" ),
    ROW( "3", "4", "5", "6" ),
  );

  fprintf(stdout, "Test completed.\n");
  return 0;
}

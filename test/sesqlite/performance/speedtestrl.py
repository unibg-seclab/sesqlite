#!/usr/bin/env python

import sqlite3
import time
import random

class SQL:

    def __init__(self, db=':memory:'):
        self.connection = sqlite3.connect(db, isolation_level=None)
        self.cursor = self.connection.cursor()

    def __del__(self):
        self.cursor.execute("PRAGMA exit_selinux;")
        self.connection.commit()
        self.connection.close()

    def test1(cursor):
        cursor.execute("CREATE TABLE t1(a INTEGER, b INTEGER, c VARCHAR(100))")
        cursor.execute('COMMIT') # re-enable auto-commit
        cursor.executemany("INSERT INTO t1 VALUES(?, ?, ?)",
                ((i, r, "'%s'" % r) for i in xrange(1000) for r in (random.randint(0, 100000),)))
        cursor.execute('BEGIN')

    def test2(cursor):
        cursor.execute("CREATE TABLE t2(a INTEGER, b INTEGER, c VARCHAR(100))")
        cursor.executemany("INSERT INTO t2 VALUES(?, ?, ?)",
                ((i, r, "'%s'" % r) for i in xrange(25000) for r in (random.randint(0, 100000),)))

    def test3(cursor):
        cursor.execute("CREATE TABLE t3(a INTEGER, b INTEGER, c VARCHAR(100))")
        cursor.execute("CREATE INDEX i3 ON t3(c)")
        cursor.executemany("INSERT INTO t3 VALUES(?, ?, ?)",
                ((i, r, "'%s'" % r) for i in xrange(25000) for r in (random.randint(0, 100000),)))

    def test4(cursor):
        for i in xrange(100):
            cursor.execute("SELECT count(*), avg(b) FROM t2 WHERE b>=%d AND b<%d" % (i * 100, i * 100 + 1000))

    def test5(cursor):
        for i in xrange(100):
            cursor.execute("SELECT count(*), avg(b) FROM t2 WHERE c LIKE '%%%d%%'" % random.randint(0, 999))

    def test6(cursor):
        cursor.execute("CREATE INDEX i2a ON t2(a)")
        cursor.execute("CREATE INDEX i2b ON t2(b)")

    def test7(cursor):
        for i in xrange(5000):
            cursor.execute("SELECT count(*), avg(b) FROM t2 WHERE b>=%d AND b<%d" % (i * 100, (i + 1) * 100))

    def test8(cursor):
        cursor.executemany("UPDATE t1 SET b=b*2 WHERE a>=? AND a<?",
                ((i, (i + 1) * 10) for i in xrange(1000)))

    def test9(cursor):
        cursor.executemany("UPDATE t2 SET b=? WHERE a=?",
                ((random.randint(0, 100000), i + 1) for i in xrange(25000)));

    def test10(cursor):
        cursor.execute("INSERT INTO t1 SELECT b,a,c FROM t2")
        cursor.execute("INSERT INTO t2 SELECT b,a,c FROM t1")

    def test11(cursor):
        cursor.execute("DELETE FROM t2 WHERE c LIKE '%5%'")

    def test12(cursor):
        cursor.execute("DELETE FROM t2 WHERE a>10 AND a<20000")

    def test13(cursor):
        cursor.execute("DROP TABLE t1")
        cursor.execute("DROP TABLE t2")
        cursor.execute("DROP TABLE t3")

    tests = [
             (test1,  '01 - 1000 INSERTs'),
             (test2,  '02 - 25000 INSERTs in a transaction'),
             (test3,  '03 - 25000 INSERTs into an indexed table in a transaction'),
             (test4,  '04 - 100 SELECTs without an index'),
             (test5,  '05 - 100 SELECTs on a string comparison'),
             (test6,  '06 - Creating an index'),
             (test7,  '07 - 5000 SELECTs with an index'),
             (test8,  '08 - 1000 UPDATEs without an index'),
             (test9,  '09 - 25000 UPDATEs with an index'),
             (test10, '10 - INSERTs from a SELECT'),
             (test11, '11 - DELETE without an index'),
             (test12, '12 - DELETE with an index'),
             (test13, '13 - DROP TABLE')
            ]

    def runtest(self, testfn):
        start_time = time.time()
        self.cursor.execute('BEGIN')
        testfn(self.cursor)
        self.cursor.execute('COMMIT')
        return time.time() - start_time

    def runtests(self):
        template = '{:<60} | {}s'
        total = 0
        for testfn, description in self.tests:
            elapsed = self.runtest(testfn)
            print template.format(description, elapsed)
            total += elapsed
        print template.format('TOTAL', total)
        return total

if __name__ == '__main__':
    SQL().runtests()


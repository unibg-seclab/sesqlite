#!/usr/bin/env python

from sqlite3 import connect
from random import random, randint
from time import time
import argparse
import json
import sys

class Test:

    def __init__(self, db=":memory:", inlimit=None):
        self.connection = connect(db)
        self.cursor = self.connection.cursor()
        self.cursor.execute("CREATE TABLE TEST(A INTEGER, B REAL, C TEXT)")
        if inlimit is not None:
            self.cursor.execute('PRAGMA inlimit=%d;' % inlimit)

    def __del__(self):
        try:
            self.connection.commit()
            self.connection.close()
        except:
            pass

    def __enter__(self):
        return self

    def __exit__(self, type, value, tb):
        del self

    def insert(self, n):
        start = time()
        self.cursor.executemany("INSERT INTO TEST VALUES (?, ?, ?)",
            ((i, random(), "'%s'" % randint(1, 10 ** 6)) for i in xrange(n)))
        return time() - start

    def select(self, where="1=1"):
        start = time()
        self.cursor.execute("SELECT A FROM TEST WHERE %s" % where)
        self.cursor.fetchall()
        return time() - start

    def update(self, where="1=1"):
        start = time()
        self.cursor.execute("UPDATE TEST SET B = 2*B WHERE %s" % where)
        return time() - start

    def delete(self, where="1=1"):
        start = time()
        self.cursor.execute("DELETE FROM TEST WHERE %s" % where)
        return time() - start

    def execute(self, sql):
        start = time()
        self.cursor.execute(sql)
        return time() - start

def avg(lst, drop=0):
    if drop:
        lst = sorted(lst)[drop:-drop]
    return float(sum(lst)) / len(lst)

def test(fr, to, st, reps, drop, exp, inlimit):
    ns = range(fr, to + 1, st)
    if exp: ns = [10 ** n for n in ns]
    out = {}
    for n in ns:
        sys.stderr.write('Testing with n=%d\n' % n)
        res = [(t.insert(n), t.select(), t.update(), t.delete())
               for t in [Test(inlimit=inlimit)] for rep in xrange(reps)]
        res = [avg(lst, drop) for lst in zip(*res)]
        out[n] = dict(zip(('insert', 'select', 'update', 'delete'), res))
    return out

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test SQLite")
    parser.add_argument("-f", type=int, default=1000, help="from tuples")
    parser.add_argument("-t", type=int, default=100000, help="to tuples")
    parser.add_argument("-s", type=int, default=1000, help="step tuples")
    parser.add_argument("-r", type=int, default=10, help="repetitions")
    parser.add_argument("-d", type=int, default=0, help="drop d per tail")
    parser.add_argument("-i", type=int, default=None, help="in limit")
    parser.add_argument("-e", action="store_true", help="use 10^i")
    args = parser.parse_args()

    out = test(args.f, args.t, args.s, args.r, args.d, args.e, args.i)
    print json.dumps(out)

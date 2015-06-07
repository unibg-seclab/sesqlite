#!/usr/bin/env python

from time import time
import subprocess
import argparse
import json
import sys
import ast

def avg(lst, drop=0):
    if drop:
        lst = sorted(lst)[drop:-drop]
    return float(sum(lst)) / len(lst)

def test(fr, to, st, reps, drop, exp, inlimit, fn):
    ns = range(fr, to + 1, st)
    if exp: ns = [10 ** n for n in ns]
    out = {}
    for n in ns:
        sys.stderr.write('Testing with n=%d\n' % n)
        res = [fn(n, inlimit) for rep in xrange(reps)]
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
    parser.add_argument("-c", type=str, required=True, help="executable")
    args = parser.parse_args()

    def fn(n, inlimit):
        return ast.literal_eval(subprocess.check_output([args.c, str(n), str(inlimit)]))

    out = test(args.f, args.t, args.s, args.r, args.d, args.e, args.i, fn)
    print json.dumps(out)


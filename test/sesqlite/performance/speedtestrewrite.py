#!/usr/bin/env python

from subprocess import check_output
import argparse
import json
import sys
import re

def avg(lst, drop=0):
    lst = list(lst)
    if drop:
        lst = sorted(lst)[drop:-drop]
    return float(sum(lst)) / len(lst)

def run(command, n, drop):
    sys.stderr.write('%s\n' % command)
    res = avg([float(re.findall(r'TOTAL.*?(\d+\.\d+)s', check_output(command, shell=True))[0])
              for _ in xrange(n)], drop)
    sys.stderr.write('%f\n' % res)
    return res

def test(fr, to, st, reps, drop):
    xs = range(fr, to + 1, st)
    ys_base = [run('make test1', reps, drop)] * len(xs)
    ys_se_sca = [run('make test1_se ARGS1_SE="-inlimit 0"', reps, drop)] * len(xs)
    ys_se_in = [run('make test1_se ARGS1_SE="-inlimit %d"' % i, reps, drop) for i in xs]

    return {'xs': xs, 'ys_base': ys_base, 'ys_se_sca': ys_se_sca, 'ys_se_in': ys_se_in}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Test Rewrite")
    parser.add_argument("-f", type=int, default=1, help="from inlimit")
    parser.add_argument("-t", type=int, default=20, help="to inlimit")
    parser.add_argument("-s", type=int, default=1, help="step inlimit")
    parser.add_argument("-r", type=int, default=10, help="repetitions")
    parser.add_argument("-d", type=int, default=0, help="drop d per tail")
    args = parser.parse_args()

    print json.dumps(test(args.f, args.t, args.s, args.r, args.d))


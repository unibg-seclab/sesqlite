#!/usr/bin/env python

from subprocess import check_output
import argparse
import json
import re

def avg(lst, drop=0):
    lst = list(lst)
    if drop:
        lst = sorted(lst)[drop:-drop]
    return float(sum(lst)) / len(lst)

def run(command, n, drop):
    return avg([float(re.findall(r'TOTAL.*?(\d+\.\d+)s',
                     check_output(command, shell=True))[0]) for _ in xrange(n)], drop)

def test(fr, to, st, reps, drop):
    xs = range(fr, to + 1, st)
    ys_base, ys_se_sca, ys_se_in = [], [], []

    for i in xs:
        print 'inlimit = %d' % i
        ys_base.append(run('make test1', reps, drop))
        ys_se_sca.append(run('make test1_se', reps, drop))
        ys_se_in.append(run('make test1_se ARGS1="-inlimit %d"' % i, reps, drop))

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


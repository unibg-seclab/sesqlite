#!/usr/bin/env python

import matplotlib as mpl
mpl.use('Agg')

import matplotlib.pyplot as plt
from collections import namedtuple
from subprocess import check_output
import json

class Args(namedtuple('Args', 'f, t, s, r, d, i')):
    def cmd(self):
        return ('./speedtestnum.py -f%d -t%d -s%d -r%d -d%d -i%d' % self).split()

env_base = {'LD_PRELOAD': './libsqlite3.so'}
env_se = {'LD_PRELOAD': './libsesqlite3.so'}
args = Args(100000, 100000, 1, 20, 4, 0)

if __name__ == '__main__':

    xs = range(1, 101)
    ys_base, ys_se_sca, ys_se_in = [], [], []

    for i in xs:
        print 'inlimit = %d' % i
        base = json.loads(check_output(args.cmd(), env=env_base))
        se_sca = json.loads(check_output(args.cmd(), env=env_se))
        se_in = json.loads(check_output(args._replace(i=i).cmd(), env=env_se))

        ys_base.append(sum(sum(test.values()) for test in base.values()))
        ys_se_sca.append(sum(sum(test.values()) for test in se_sca.values()))
        ys_se_in.append(sum(sum(test.values()) for test in se_in.values()))

    print json.dumps()

    plt.plot(xs, ys_base, label='base')
    plt.plot(xs, ys_se_sca, label='selinux_check_access')
    plt.plot(xs, ys_se_in, label='selinux_in')
    plt.savefig('results/rewrite/rewrite.pdf')


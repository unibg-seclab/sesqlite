#!/usr/bin/env python

import matplotlib as mpl
mpl.use('Agg')

import matplotlib.pyplot as plt
import argparse
import os.path
import json

def plot(filename, testname, **kwargs):
    with open(filename) as f:
        data = json.load(f)
    xs = sorted(map(int, data.keys()))
    ys = [data[str(x)][testname] for x in xs]
    plt.plot(xs, ys, **kwargs)

def plot_both(sqlite, sesqlite, testname, outdir, log=False):
    print 'plotting test %s to %s' % (testname, outdir)
    plot(sqlite, testname, label='SQLite', color='blue', linestyle=':')
    plot(sesqlite, testname, label='SeSQLite', color='red', linestyle='-')
    plt.xlabel('tuples')
    plt.ylabel('time')
    plt.xscale('log' if log else 'linear')
    plt.yscale('log' if log else 'linear')
    plt.legend(loc='best')
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, '%s.pdf' % testname))
    plt.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='plot the data')
    parser.add_argument('SQLite', help='SQLite json test output')
    parser.add_argument('SeSQLite', help='SeSQLite json test output')
    parser.add_argument('--log', action='store_true', help='use logscale')
    parser.add_argument('--outdir', default='.', help='output directory')
    args = parser.parse_args()

    plot_both(args.SQLite, args.SeSQLite, 'insert', args.outdir, args.log)
    plot_both(args.SQLite, args.SeSQLite, 'update', args.outdir, args.log)
    plot_both(args.SQLite, args.SeSQLite, 'select', args.outdir, args.log)
    plot_both(args.SQLite, args.SeSQLite, 'delete', args.outdir, args.log)


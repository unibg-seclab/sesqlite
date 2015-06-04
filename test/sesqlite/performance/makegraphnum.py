#!/usr/bin/env python

import matplotlib as mpl
mpl.use('Agg')

from matplotlib.ticker import FuncFormatter
import matplotlib.pyplot as plt
import argparse
import os.path
import json

def plot(filename, testname, ax, **kwargs):
    with open(filename) as f:
        data = json.load(f)
    xs = sorted(map(int, data.keys()))
    ys = [data[str(x)][testname] for x in xs]
    ax.plot(xs, ys, **kwargs)
    ax.set_xlim(0, max(max(xs), ax.get_xlim()[1]))
    ax.set_ylim(0, max(max(ys), ax.get_ylim()[1]))
    return xs, ys

def plot_both(sqlite, sesqlite, test, ax=None):
    if not ax: _, ax = plt.subplots()
    x1, y1 = plot(sesqlite, test, ax, label='SeSQLite', color='r', linestyle='-')
    x2, y2 = plot(sqlite, test, ax, label='SQLite', color='b', linestyle='--')
    ax.fill_between(x1, 0, y2, interpolate=True, color='b', alpha=0.1)
    ax.fill_between(x1, y1, y2, interpolate=True, color='r', alpha=0.3)

def plot_all(sqlite, sesqlite, outdir, ext='pdf', log=False):
    for test in ('insert', 'update', 'select', 'delete'):
        print 'generating test %s' % test
        plot_both(sqlite, sesqlite, test, log)
        plt.xlabel('tuples')
        plt.ylabel('time')
        plt.legend(loc='best')
        plt.xscale('log' if log else 'linear')
        plt.yscale('log' if log else 'linear')
        plt.tight_layout()
        plt.savefig(os.path.join(outdir, '%s.%s' % (test, ext)))
        plt.close()

def plot_single(sqlite, sesqlite, outdir, ext='pdf', log=False):
    fig, axarr = plt.subplots(4, sharex=True, figsize=(8,12))
    for ax, test in zip(axarr, ('insert', 'select', 'update', 'delete')):
        print 'generating test %s' % test
        plot_both(sqlite, sesqlite, test, ax)
        ax.set_title(test.upper())
        ax.legend(loc='upper left')
        ax.yaxis.set_major_formatter(FuncFormatter(lambda x, pos: '%gms' % (x*1e3)))

    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    plt.gca().xaxis.get_major_ticks()[0].label1.set_visible(False)
    plt.xlabel('tuples')
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, 'plot.%s' % ext))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='plot the data')
    parser.add_argument('SQLite', help='SQLite json test output')
    parser.add_argument('SeSQLite', help='SeSQLite json test output')
    parser.add_argument('--log', action='store_true', help='use logscale')
    parser.add_argument('--outdir', default='.', help='output directory')
    parser.add_argument('--ext', default='pdf', help='file extension')
    args = parser.parse_args()
    plot_single(args.SQLite, args.SeSQLite, args.outdir, args.ext, args.log)
    #plot_all(args.SQLite, args.SeSQLite, args.outdir, args.ext, args.log)


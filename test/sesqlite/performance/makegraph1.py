#!/usr/bin/env python2

from collections import defaultdict, OrderedDict
from os.path import basename
import numpy as np
import glob
import re

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

base = 'results/1/base.out'
se = 'results/1/se.out'
no_opt = 'results/1/no-opt.out'

files = glob.glob('results/1/*.out')
width = .8 / len(files)
cm = plt.get_cmap('afmhot')

def filter_keys(dct, keys):
    return OrderedDict((key, dct[key]) for key in keys)

def filename(file):
    return re.findall(r'\w+', basename(file))[0]

def parsefile(filename):
    data = OrderedDict()
    with open(filename) as f:
        for key, value in re.findall(r'^\s*(\d+).*?\s(\d+\.\d+)s\s*$', f.read(), re.M):
            data[key] = data.get(key, []) + [float(value)]
    return data

def getdata(filename):
    data = parsefile(filename)
    xs = np.arange(len(data.keys()))
    ys = map(np.mean, data.values())
    return xs, ys, data

def incr(fr, to):
    return (to - fr) / float(fr)

def printstat(ys, base_ys):
    print '& $%.2f\\si{\\second}$ & $+%.2f\\%%$ \\\\' % (
          sum(ys),
          100 * incr(sum(base_ys), sum(ys)))
    return [100 * incr(b, y) for b, y in zip(ys_base, ys)]

xs_base, ys_base, data_base = getdata(base)
xs_se, ys_se, data_se = getdata(se)
xs_no_opt, ys_no_opt, data_no_opt = getdata(no_opt)
ys_base = np.array(ys_base)
ys_se = np.maximum(np.array(ys_se), ys_base)
ys_no_opt = np.maximum(np.array(ys_no_opt), ys_base)

print base
printstat(ys_base, ys_base)
print se
over_se = printstat(ys_se, ys_base)
print no_opt
over_no_opt = printstat(ys_no_opt, ys_base)

for yb, ys, os, yn, on in zip(ys_base, ys_se, over_se, ys_no_opt, over_no_opt):
    print ( '& $%.2f\\si{\\second}$ '
            '& $%.2f\\si{\\second}$ ($+%.2f\\%%$) '
            '& $%.2f\\si{\\second}$ ($+%.2f\\%%$) \\\\' ) % (yb, ys, os, yn, on)

ys_base, ys_se = np.minimum(ys_base, ys_se), np.maximum(ys_base, ys_se)
rect_base = ys_base
rect_se = ys_se - ys_base
rect_no_opt = ys_no_opt - ys_se

plt.xticks(xs_base +.4, data_base.keys(), rotation=90)
plt.xlim(-.2, xs_base[-1] + 1)
plt.ylim(0, 2.5)

plt.bar(xs_se,   rect_se,   color=plt.get_cmap('Greens')(0.6), label='SeSQLite optimized', width=.8, bottom=rect_base)
plt.bar(xs_base, rect_base, color=plt.get_cmap('YlOrRd')(0.1), label='SQLite base', width=.8)

plt.ylabel('time [s]')
plt.tight_layout()
plt.legend(loc='upper left')
plt.savefig('results/1/graph1.pdf')
plt.savefig('results/1/graph1.png')
plt.clf()

plt.xticks(xs_base +.4, data_base.keys(), rotation=90)
plt.ylim(0, 2.5)

plt.bar(xs_no_opt, rect_no_opt, color=plt.get_cmap('Reds')(0.8), label='SeSQLite not optimized', width=.8, bottom=rect_base)
plt.bar(xs_base,   rect_base,   color=plt.get_cmap('YlOrRd')(0.1), label='SQLite base', width=.8)

plt.ylabel('time [s]')
plt.tight_layout()
plt.legend(loc='upper left')
plt.savefig('results/1/graph2.pdf')
plt.savefig('results/1/graph2.png')
plt.clf()


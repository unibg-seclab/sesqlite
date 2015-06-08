#!/usr/bin/env python2

from collections import defaultdict, OrderedDict
from os.path import basename
import numpy as np
import re

# use matplotlib even without X
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

base = 'results/1/1.base.out'
se = 'results/1/2.se.out'
se_sca = 'results/1/3.se_sca.out'
width = .25

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

def overhead(fr, to):
    return (to - fr) / fr

def mean(lst):
    lst = list(lst)
    return sum(lst) / float(len(lst))

def printstat(ys, base_ys, file):
    print '%s & $%.2f\\si{\\second}$ & $+%.2f\\%%$ \\\\' % (
          basename(file), sum(ys), 100 * overhead(sum(base_ys), sum(ys)))
    return [100 * overhead(b, y) for b, y in zip(ys_base, ys)]

def graph_with_options(xs, xticks, name):
    plt.xticks(xs +.4, data_base.keys(), rotation=90)
    plt.xlim(-.2, xs[-1] + 1)
    #plt.ylim(0, 2.5)
    plt.ylabel('time [s]')
    plt.tight_layout()
    plt.legend(loc='upper left')
    plt.savefig('results/1/%s.pdf' % name)
    plt.savefig('results/1/%s.png' % name)
    plt.clf()

# get data
xs_base, ys_base, data_base = getdata(base)
xs_se, ys_se, data_se = getdata(se)
xs_se_sca, ys_se_sca, data_se_sca = getdata(se_sca)

# create arrays
ys_base = np.array(ys_base)
ys_se = np.array(ys_se)
ys_se_sca = np.array(ys_se_sca)

# print data
print '\n===== SeSQLite Overhead ====='
print '\n'.join('test {} -- base:{:4.3f} - se:{:4.3f} ({:+3.3%}) - sca:{:4.3f} ({:+3.3%})'.format(t, b, se, ose, sca, osca)
    for t, b, se, ose, sca, osca in zip(data_base.keys(), ys_base,
        ys_se, overhead(ys_base, ys_se),
        ys_se_sca, overhead(ys_base, ys_se_sca)))

print '\n=========== Total ==========='
print 'base:{:4.3f} - se:{:4.3f} ({:+3.3%}) - sca:{:4.3f} ({:+3.3%})'.format(
    sum(ys_base), sum(ys_se), overhead(sum(ys_base), sum(ys_se)),
    sum(ys_se_sca), overhead(sum(ys_base), sum(ys_se_sca)))

# we cannot improve!
ys_se = np.maximum(ys_se, ys_base)
ys_se_sca = np.maximum(ys_se_sca, ys_se)

# graph optimized
rect_se = ys_se - ys_base
rect_se_sca = ys_se_sca - ys_se
plt.bar(xs_se, rect_se_sca, color=plt.get_cmap('Blues')(0.5), label='SeSQLite (100 contexts)', hatch='xx', width=.8, bottom=ys_se)
plt.bar(xs_se, rect_se, color=plt.get_cmap('Greens')(0.3), label='SeSQLite (2 contexts)', hatch='//', width=.8, bottom=ys_base)
plt.bar(xs_base, ys_base, color=plt.get_cmap('YlOrRd')(0.1), label='SQLite base', width=.8, hatch='..')
graph_with_options(xs_base, data_base.keys(), 'graph')


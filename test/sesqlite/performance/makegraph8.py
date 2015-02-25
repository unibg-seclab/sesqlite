#!/usr/bin/env python2

from collections import defaultdict, OrderedDict
from os.path import basename
import numpy as np
import glob
import re

# make plots even without X
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

keys1 = ['Total prepare', 'Total run', 'Total finalize', 'Total']
keys2 = ['Open', 'Reopen', 'Close']

files = glob.glob('results/8/*.out')
width = .8 / len(files)
cm = plt.get_cmap('summer')

def filter_keys(dct, keys):
    return OrderedDict((key, dct[key]) for key in keys)

def filename(file):
    return re.findall(r'\w+', basename(file))[0]

def parsefile(filename):
    data = defaultdict(list)
    with open(filename) as f:
        for key, value in re.findall(r'(.+?)\stime:\s*([\deE.+-]+)', f.read()):
            data[key].append(float(value))
    return data

for i, file in enumerate(files, start=0):
    data = filter_keys(parsefile(file), keys1)
    print file, zip(data.keys(), map(np.mean, data.values()))
    xs = np.arange(len(data.keys()))
    plt.xticks(xs +.4, data.keys(), rotation=90)
    plt.xlim(-.2, xs[-1] + 1)
    plt.bar(xs + i * width, map(np.mean, data.values()), color=cm(i * width / .8),
            yerr=map(np.std, data.values()), width=width, label=filename(file), ecolor='black')

plt.ylabel('time [s]')
plt.tight_layout()
plt.legend(loc='best')
plt.savefig('results/8/graph1.pdf')
plt.savefig('results/8/graph1.png')
plt.clf()

for i, file in enumerate(files, start=0):
    data = filter_keys(parsefile(file), keys2)
    print file, zip(data.keys(), map(np.mean, data.values()))
    xs = np.arange(len(data.keys()))
    plt.xticks(xs +.4, data.keys(), rotation=90)
    plt.xlim(-.2, xs[-1] + 1)
    plt.bar(xs + i * width, map(np.mean, data.values()), color=cm(i * width / .8),
            yerr=map(np.std, data.values()), width=width, label=filename(file), ecolor='black')

plt.ylabel('time [s]')
plt.tight_layout()
plt.legend(loc='best')
plt.savefig('results/8/graph2.pdf')
plt.savefig('results/8/graph2.png')
plt.clf()

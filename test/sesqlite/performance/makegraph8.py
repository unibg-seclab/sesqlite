#!/usr/bin/env python2

from collections import defaultdict
from os.path import basename
import numpy as np
import glob
import re

# make plots even without X
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

files = glob.glob('results/8/*.out')
width = .8 / len(files)
cm = plt.get_cmap('summer')

def parsefile(filename):
    data = defaultdict(list)
    with open(filename) as f:
        for key, value in re.findall(r'(.+?)\stime:\s*([\deE.+-]+)', f.read()):
            data[key].append(float(value))
    return data

for i, file in enumerate(files, start=0):
    data = parsefile(file)
    print file, zip(data.keys(), map(np.mean, data.values()))
    xs = np.arange(len(data.keys()))
    plt.xticks(xs +.4, data.keys(), rotation=90)
    plt.xlim(-.2, xs[-1] + 1)
    plt.bar(xs + i * width, map(np.mean, data.values()), color=cm(i * width / .8),
            yerr=map(np.std, data.values()), width=width, label=basename(file), ecolor='black')

plt.ylabel('time [s]')
plt.tight_layout()
plt.legend(loc='best')
plt.savefig('results/8/graph.pdf')
plt.savefig('results/8/graph.png')
plt.clf()

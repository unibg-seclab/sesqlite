#!/usr/bin/env python2

from collections import defaultdict
from os.path import basename
import numpy as np
import glob
import re

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

files = glob.glob('results/*.out')
width = .8 / len(files)

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
    plt.bar(xs + i * width, map(np.mean, data.values()), width=width,
            color=str(float(i) / len(files)), label=basename(file))

plt.ylabel('time [s]')
plt.tight_layout()
plt.legend(loc='best')
plt.savefig('results/graph.pdf')
plt.savefig('results/graph.png')

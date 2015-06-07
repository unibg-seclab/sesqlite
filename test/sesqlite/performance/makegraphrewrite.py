#!/usr/bin/env python

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import json

if __name__ == '__main__':
    with open('results/rewrite/rewrite.json') as f:
        data = json.load(f)

    xs = data['xs']
    ys_base = data['ys_base']
    ys_se_sca = data['ys_se_sca']
    ys_se_in = data['ys_se_in']

    plt.plot(xs, ys_base, label='base')
    plt.plot(xs, ys_se_sca, label='selinux_check_access')
    plt.plot(xs, ys_se_in, label='selinux_in')
    plt.savefig('results/rewrite/rewrite.pdf')


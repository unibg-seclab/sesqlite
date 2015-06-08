#!/usr/bin/env python

import matplotlib as mpl
mpl.use('Agg')

import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import numpy as np
import json

if __name__ == '__main__':
    with open('results/rewrite/rewrite.json') as f:
        data = json.load(f)

    cmap = plt.get_cmap('YlGnBu')
    xs = np.array(data['xs'])
    ys_base = np.array(data['ys_base'])
    ys_se_sca = np.array(data['ys_se_sca'])
    ys_se_in = np.array(data['ys_se_in'])

    ys_over_sca = 100 * (ys_se_sca - ys_base) / ys_se_sca
    ys_over_in = 100 * (ys_se_in - ys_base) / ys_se_in
    ys_over_min = np.minimum(ys_over_sca, ys_over_in)
    top = max(np.max(ys_over_in), np.max(ys_over_sca))

    cross = max(xs)
    for i in xrange(len(xs)):
        if ys_over_sca[i] < ys_over_in[i]:
            cross = i + .5
            break

    plt.plot(xs, ys_over_sca, color=cmap(.4), linewidth=1.5, marker='D', markersize=8, linestyle='--', label='selinux_check_access')
    plt.plot(xs, ys_over_in, color=cmap(.8), linewidth=1.5, marker='o', markersize=8, linestyle='--', label='SQL IN clause')
    plt.plot(xs, ys_over_min, color= 'r', linewidth=3, label='SeSQLite')
    plt.axvspan(1, cross, color=cmap(.8), alpha=0.2)
    plt.axvspan(cross, max(xs), color=cmap(.4), alpha=0.2)
    plt.xlim(1, max(xs))
    plt.ylim(bottom=0, top=top+2.5)
    plt.legend(loc='lower right')
    plt.ylabel('overhead')
    plt.xlabel('number of db_tuple contexts in the database')
    plt.gca().yaxis.set_major_formatter(mtick.FormatStrFormatter('%.0f%%'))

    ary = top + 2
    plt.annotate('', xy=(1, ary), xytext=(cross, ary), arrowprops={'arrowstyle':'->'})
    plt.annotate('', xy=(max(xs), ary), xytext=(cross, ary), arrowprops={'arrowstyle':'->'})
    plt.text(.5*(1+cross), ary -1, 'SQL IN', va='bottom', ha='center')
    plt.text(.5*(cross+max(xs)), ary -1, 'selinux_check_access', va='bottom', ha='center')
    plt.tight_layout()
    plt.savefig('results/rewrite/rewrite.pdf')


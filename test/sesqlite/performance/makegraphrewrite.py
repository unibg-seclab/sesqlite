#!/usr/bin/env python

import matplotlib as mpl
import numpy as np
mpl.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
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
            cross = i + 1
            break

    plt.plot(xs, ys_over_sca, color=cmap(.4), linewidth=2, marker='^', label='selinux_check_access')
    plt.plot(xs, ys_over_in, color=cmap(.8), linewidth=2, marker='o', label='SQL IN clause')
    plt.plot(xs, ys_over_min, color= 'r', linewidth=4, linestyle='--', label='SeSQLite')
    plt.axvspan(1, cross, color=cmap(.8), alpha=0.2)
    plt.axvspan(cross, max(xs), color=cmap(.4), alpha=0.2)
    plt.xlim(1, max(xs))
    plt.ylim(bottom=0, top=top+2.5)
    plt.legend(loc='lower right')
    plt.ylabel('overhead')
    plt.xlabel('tuple contexts')
    plt.gca().yaxis.set_major_formatter(mtick.FormatStrFormatter('%.0f%%'))

    ary = top + 2
    plt.annotate('', xy=(1, ary), xytext=(cross, ary), arrowprops={'arrowstyle':'->'})
    plt.annotate('', xy=(max(xs), ary), xytext=(cross, ary), arrowprops={'arrowstyle':'->'})
    plt.text(.5*(1+cross), ary -1, 'SQL IN', va='bottom', ha='center')
    plt.text(.5*(cross+max(xs)), ary -1, 'selinux_check_access', va='bottom', ha='center')
    plt.tight_layout()
    plt.savefig('results/rewrite/rewrite.pdf')


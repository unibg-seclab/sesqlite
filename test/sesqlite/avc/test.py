#!/usr/bin/env python

from subprocess import check_output
import numpy as np
from sys import argv

def get_output(command):
    return map(int, check_output(command.split()).split())

tests = argv[1] if len(argv) > 1 else 1000

avc_first = get_output("./avctest/avctest %s 1" % tests)
avc_other = get_output("./avctest/avctest %s 0" % tests)
uavc_first = get_output("./uavctest/uavctest %s 1" % tests)
uavc_other = get_output("./uavctest/uavctest %s 0" % tests)

mean_avc_first = np.mean(avc_first)
mean_avc_other = np.mean(avc_other)
mean_uavc_first = np.mean(uavc_first)
mean_uavc_other = np.mean(uavc_other)

std_avc_first = np.std(avc_first)
std_avc_other = np.std(avc_other)
std_uavc_first = np.std(uavc_first)
std_uavc_other = np.std(uavc_other)

print "=== AVC ==="
print "first: %f (std. %f)" % (mean_avc_first, std_avc_first)
print "other: %f (std. %f)" % (mean_avc_other, std_avc_other)
print "=== uAVC ==="
print "first: %f (std. %f)" % (mean_uavc_first, std_uavc_first)
print "other: %f (std. %f)" % (mean_uavc_other, std_uavc_other)

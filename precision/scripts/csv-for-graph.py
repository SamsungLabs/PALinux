import os
import sys
import struct
import ctypes
import operator
from ctypes import *
from os import walk

def count(filename, idx):
    count_dict = {}

    count_dict[0] = 0
    with open(filename) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            ss = line.split(',')
            ct = int(ss[idx])
            if ct in count_dict:
                c = count_dict[ct]
                count_dict[ct] = c + 1
            else:
                count_dict[ct] = 1

    count_dict[1] = count_dict[1] + count_dict[0]
    del count_dict[0]

    for key, value in sorted(count_dict.items(), key=operator.itemgetter(0), reverse=False):
        print(str(key) + "," + str(value))

if __name__ == "__main__":
    if len(sys.argv) == 3:

        if sys.argv[1] == "allowed-target":  
            count(sys.argv[2], -1)
            sys.exit(0)
        elif sys.argv[1] == "diversity":
            count(sys.argv[2], -3)
            sys.exit(0)

    print "USAGE : python csv-for-graph.py diversity|allowed-target <target csv file>"
    sys.exit(0)

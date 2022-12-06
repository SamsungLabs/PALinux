import os
import sys
import struct
import ctypes
import operator
from ctypes import *
from os import walk

def print_percent(filename):
    total = 0
    hundred = 0
    five = 0
    max_num = 0

    with open(filename) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            ss = line.split(',')
            ct = int(ss[-1])
            target = int(ss[0])

            if target > max_num:
                max_num = target

            total = total + ct
            if target <= 5:
                five = five + ct
            elif target > 100:
                hundred = hundred + ct

    print("Total: " + str(total))
    print(">100: " + str(hundred) + " " + str(round(float(hundred) / total * 100, 2)) + "%")
    print("<=5: " + str(five) + " " + str(round(float(five) / total * 100, 2)) + "%")
    print("Max: " + str(max_num))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "USAGE : python print-percent.py <graph csv file>"
        sys.exit(0)
    
    print_percent(sys.argv[1])

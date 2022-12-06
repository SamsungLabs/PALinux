import os
import sys
import struct
import ctypes
import operator
from ctypes import *
from os import walk

def build_map(file, percent):
    objbind_structs = {}

    with open(file) as f:
        lines = f.readlines()

        for num, line in enumerate(lines):
            line = line.strip()
            idx = line.find("AllowedTarget: ")
            if idx == -1:
                continue

            start_idx = line.find("struct.")
            l1 = line[start_idx:]
            l2 = l1.split(" ")[0]
            tokens = l2.split(".")
            if len(tokens) < 2:
                continue
            name = tokens[0] + "." + tokens[1]

            start_idx = line.find("AllowedTarget: ") + len("AllowedTarget: ")
            allowed_target = int(line[start_idx:])

            if name in objbind_structs:
                current = objbind_structs[name]
                if allowed_target > current:
                    objbind_structs[name] = allowed_target
            else:
                objbind_structs[name] = allowed_target
    
    last_idx = len(objbind_structs) * percent / 100
    cur = 0
    for key, value in sorted(objbind_structs.items(), key=operator.itemgetter(1), reverse=True):
        #print(key + "," + str(value))
        print(key)
        cur += 1
        if cur > last_idx:
            break

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("USAGE : python extract_top.py <log file> <percent>")
        sys.exit(0)
    
    build_map(sys.argv[1], int(sys.argv[2]))

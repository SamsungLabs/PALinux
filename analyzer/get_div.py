import os
import sys
import struct
import ctypes
import operator
from ctypes import *
from os import walk

def print_diversity_score(file):
    objbind_structs = {}
    with open(file) as f:
        lines = f.readlines()
        div_start = False
        struct_name = ""
        max_score = 0

        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("==== Struct: ") != -1:
                start_idx = len("==== Struct: ")
                end_idx = line.find(" ====")
                struct_name = line[start_idx:end_idx]
                div_start = True
                max_score = 0
            elif line.find("GlobalAddrBindScore: ") != -1:
                start_idx = len("GlobalAddrBindScore: ")
                score_str = line[start_idx:]
                if max_score < int(score_str):
                    max_score = int(score_str)

                print(struct_name + "," + str(max_score))
                
                div_start = False
                struct_name = ""
                max_score = 0
            else:
                if div_start == True:
                    if line.find("DiversityScore: ") != -1:
                        start_idx = len("DiversityScore: ")
                        score_str = line[start_idx:]
                        if max_score < int(score_str):
                            max_score = int(score_str)
                    if line == "":
                        print(struct_name + "," + str(max_score))
                        div_start = False
                        struct_name = ""
                        max_score = 0

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE : python get_div.py <log file>")
        sys.exit(0)
    
    print_diversity_score(sys.argv[1])

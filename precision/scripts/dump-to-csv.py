import os
import sys
import struct
import ctypes
import operator
from ctypes import *
from os import walk

aut_dict = {}
pac_dict = {}
aut_type_dict = {}
aut_obj_dict = {}

name_dict = {}

def build_aut_dict(filename):
    with open(filename) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            idx1 = line.find("Result/")
            idx2 = line.find("/AUT")
            idx3 = line.find("/PAC")
            runtime = line.find("Runtime dependent")
            if idx1 != -1 and idx2 != -1 and runtime == -1:
                ss = line.split('/')
                ctx = ss[1] + ss[2]
                aut = ss[6]
                aut_int = int(aut)
                obj_name = ss[5]
                type_ctx = ss[1]
                #ctx = line[idx1+7:idx2]
                #aut = line[idx2+5:]
                #aut_int = int(aut)

                if ctx in aut_dict:
                    s = aut_dict[ctx]
                    aut_dict[ctx] = s + aut_int
                else:
                    aut_dict[ctx] = aut_int
                
                if len(obj_name) < 1:
                    aut_obj_dict[ctx] = "NONE"
                else:
                    aut_obj_dict[ctx] = obj_name

                aut_type_dict[ctx] = type_ctx

            if idx1 != -1 and idx3 != -1 and runtime == -1:
                ss = line.split('/')
                ctx = ss[1] + ss[2]
                pac = ss[6]
                pac_int = int(pac)
                obj_name = ss[5]
                type_ctx = ss[1]

                if ctx in pac_dict:
                    s = pac_dict[ctx]
                    pac_dict[ctx] = s + pac_int
                else:
                    pac_dict[ctx] = pac_int

            if line.find("Context/") != -1 and runtime == -1:
                name_idx = line.rfind("/")
                ctx_start = line.find("Context/")
                ctx = line[ctx_start+8:name_idx]
                name = line[name_idx+1:]
                if ctx not in name_dict:
                    name_dict[ctx] = name

def build_func_dict(filename):
    with open(filename) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()   
            if line.find("Function/") != -1:
                ss = line.split('/')
                func_hash = ss[1]
                if func_hash in func_dict:
                    c = func_dict[func_hash]
                    func_dict[func_hash] = c + 1
                else:
                    func_dict[func_hash] = 0

def print_output():
    allowed_target_dict = {}

    for key, value in sorted(aut_dict.items(), key=operator.itemgetter(1), reverse=True):
        type_ctx = aut_type_dict[key]
        obj_name = aut_obj_dict[key]
        obj_ctx = key
        if obj_name == "NONE":
            obj_ctx = "NONE"

        pac_count = 0
        if key in pac_dict:
            pac_count = pac_dict[key]

        # one row: type name, type hash, obj name, obj hash, aut count, pac count, allowed target
        allowed_target_dict[key] = value * pac_count

        #out_str = name_dict[type_ctx] + "," + type_ctx + "," + obj_name + "," + obj_ctx + "," + str(value) + "," + str(pac_count) + "," + str(value * pac_count)
        #print(out_str)
    
    for key, value in sorted(allowed_target_dict.items(), key=operator.itemgetter(1), reverse=True):
        type_ctx = aut_type_dict[key]
        obj_name = aut_obj_dict[key]
        obj_ctx = key
        if obj_name == "NONE":
            obj_ctx = "NONE"

        pac_count = 0
        if key in pac_dict:
            pac_count = pac_dict[key]
        aut_value = aut_dict[key]

        out_str = name_dict[type_ctx] + "," + type_ctx + "," + obj_name + "," + obj_ctx + "," + str(aut_value) + "," + str(pac_count) + "," + str(aut_value * pac_count)
        print(out_str)

def ind_target(filename):
    ind_vector = []
    with open(filename) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()   
            ss = line.split(',')
            idx = len(ss) - 3
            ind = ss[idx]
            ind_vector.append(int(ind))

    sorted_ind = sorted(ind_vector, reverse=True)
    for val in sorted_ind:
        print(val)

def print_pa_operation(filename):
    pac = 0
    aut = 0
    repac = 0
    with open(filename) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("PAC/") != -1:
                ss = line.split('/')
                pac += int(ss[-1])
            elif line.find("AUT/") != -1:
                ss = line.split('/')
                aut += int(ss[-1])
            elif line.find("Repac/") != -1:
                ss = line.split('/')
                repac += int(ss[-1])
    print("PAC: " + str(pac))
    print("AUT: " + str(aut))
    print("REPAC: " + str(repac))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "USAGE : python dump-to-csv.py <dump file>"
        sys.exit(0)
    
    #ind_target(sys.argv[1])
    #print_pa_operation(sys.argv[1])

    build_aut_dict(sys.argv[1])
    build_func_dict(sys.argv[1])
    print_output()

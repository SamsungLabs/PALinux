import os
import sys
import struct
import ctypes
import operator
from ctypes import *
from os import walk

type_id_dict = {}
graph_id_dict = {}
max_dist = 6

objbind_structs = {}
diversity_score_dict = {}

def add_type_id(type_id):
    if type_id in type_id_dict:
        val = type_id_dict[type_id] + 1
        type_id_dict[type_id] = val
    else:
        type_id_dict[type_id] = 1

def get_context_register(line, insn):
    if line.find("<") != -1:
        return "none"
    if line.find(insn) != -1:
        token = line.split(' ')
        if len(token) != 3:
            return "none"
        
        op = token[-1]
        return op
    return "none"

def get_context_from_mov(line, ctx):
    if line.find(".inst") != -1:
        return "none"
    if line.find("movk\tx") != -1 and line.find("lsl #48") != -1 and line.find(ctx) != -1:
        token = line.split(',')
        ctx_str = token[1]
        ctx_str = ctx_str.replace('#', '')
        return ctx_str
    if line.find("mov\tw") != -1 and line.find("#0x") != -1 and line.find("//") != -1:
        new_ctx = ctx.replace('x', 'w')
        if line.find(new_ctx) != -1:
            token = line.split(',')
            ctx_str = token[-1]
            eidx = ctx_str.find('\t')
            ctx_str2 = ctx_str[0:eidx]
            ctx_str2 = ctx_str2.replace('#', '')
            return ctx_str2
    return "none"

def count_countext_blraaz(dump):
    total = 0
    with open(dump) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("blraaz\tx") != -1:
                add_type_id(0)
                total += 1
    return total

def count_countext_autiaz(dump):
    total = 0
    with open(dump) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("autiaz\tx") != -1:
                add_type_id(0)
                total += 1
    return total

def count_countext_blraa(dump):
    dist = 8
    total = 0
    with open(dump) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("blraa\tx") != -1:
                ctx = get_context_register(line, "blraa")
                if ctx != "none":
                    if num < max_dist:
                        dist = num - 1
                    else:
                        dist = max_dist - 1
                    for idx in range(dist):
                        ctx_str = get_context_from_mov(lines[num-idx-1], ctx)
                        if ctx_str != "none":
                            ctx_long = long(ctx_str, 0)
                            add_type_id(ctx_long)
                            total += 1
                            break
    return total

def count_countext_autia(dump):
    dist = 8
    total = 0
    with open(dump) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("autia\tx") != -1:
                ctx = get_context_register(line, "autia")
                if ctx != "none":
                    if num < max_dist:
                        dist = num - 1
                    else:
                        dist = max_dist - 1
                    for idx in range(dist):
                        ctx_str = get_context_from_mov(lines[num-idx-1], ctx)
                        if ctx_str != "none":
                            ctx_long = long(ctx_str, 0)
                            add_type_id(ctx_long)
                            total += 1
                            break
    return total

def count_countext_braa(dump):
    dist = 8
    total = 0
    with open(dump) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            if line.find("braa\tx") != -1:
                ctx = get_context_register(line, "braa")
                if ctx != "none":
                    if num < max_dist:
                        dist = num - 1
                    else:
                        dist = max_dist - 1
                    for idx in range(dist):
                        ctx_str = get_context_from_mov(lines[num-idx-1], ctx)
                        if ctx_str != "none":
                            ctx_long = long(ctx_str, 0)
                            add_type_id(ctx_long)
                            total += 1
                            break
    return total

def print_csv(out):
    raw_csv = out + ".csv"
    graph_csv = out + "_graph.csv"
    total = 0

    wf = open(raw_csv, "w")
    for key, value in sorted(type_id_dict.items(), key=operator.itemgetter(1), reverse=True):
        if value > 0:
            print_str = str(key) + "," + str(value) + "\n"
            wf.write(print_str)
            total += value
    wf.close()

    for key, value in sorted(type_id_dict.items(), key=operator.itemgetter(1), reverse=False):
        if value > 0:
            if value in graph_id_dict:
                val = graph_id_dict[value] + 1
                graph_id_dict[value] = val
            else:
                graph_id_dict[value] = 1

    wf = open(graph_csv, "w")
    for key, value in sorted(graph_id_dict.items(), key=operator.itemgetter(0), reverse=False):
        if value > 0:
            print_str = str(key) + "," + str(value) + "\n"
            wf.write(print_str)
    wf.close()

def build_objbind_struct(struct_list):
    with open(struct_list) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            line = line.strip()
            objbind_structs[line] = 0

def build_diversity_score(diversity):
    with open(diversity) as f:
        lines = f.readlines()
        for num, line in enumerate(lines):
            name = ""
            line = line.strip()
            result = line.rfind('.')
            comma = line.find(',')
            if result != 6: # struct.obj.1234
                name = line[0:result]
            else:
                name = line[0:comma]
            
            score = int(line[comma+1:])

            if name in diversity_score_dict:
                s = diversity_score_dict[name]
                diversity_score_dict[name] = s + score
            else:
                diversity_score_dict[name] = score

def print_objbind_struct():
    print("==== objbind structs ====")
    for key, value in objbind_structs.items():
        print(key)
    print("==========================")

def analyze_diversity_score(percentage):
    idx = 0
    last = len(diversity_score_dict) / 100 * percentage
    last_score = 0
    for key, value in sorted(diversity_score_dict.items(), key=operator.itemgetter(1), reverse=True):
        if key in objbind_structs:
            objbind_structs[key] = 1  # '1': included, '0': not included, '2': exceptions (e.g., operations struct)

        idx += 1
        if idx > last:
            last_score = value
            break
    
    print("dict count: " + str(len(diversity_score_dict)))
    print("last index: " + str(last))
    print("last score: " + str(last_score))

    for key, value in objbind_structs.items():
        if value == 0 and key not in diversity_score_dict:
            objbind_structs[key] = 2

    # print score only
    for key, value in sorted(diversity_score_dict.items(), key=operator.itemgetter(1), reverse=True):
        print(key + "," + str(value))

def print_output():
    not_inc = 0
    inc = 0
    exception = 0

    print("==== result ====")
    for key, value in objbind_structs.items():
        output = key + " / "
        if value == 0:
            output += "Not included"
            not_inc += 1
        elif value == 1:
            output += "Included"
            inc += 1
        elif value == 2:
            output += "Exception"
            exception += 1

        if value != 2:
            score = diversity_score_dict[key]
            output += (" / Score: " + str(score))
        print(output)
    print("==========================")
    print("total: " + str(len(objbind_structs)))
    print("not included: " + str(not_inc))
    print("included: " + str(inc))
    print("exception: " + str(exception))

    incf = inc + 0.0
    totalf = len(objbind_structs) + 0.0
    result = incf / totalf
    result_ex = (incf + exception) / (totalf)
    print("include-percent: " + str(result))
    print("include-percent-w/exception: " + str(result_ex))

def print_without_exception():
     for key, value in objbind_structs.items():
         if value != 2:
             print(key)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("USAGE : python stat.py <objbind struct list> <diversity score file> <diversity score percentage>")
        sys.exit(0)
    
    build_objbind_struct(sys.argv[1])
    build_diversity_score(sys.argv[2])
    percentage = int(sys.argv[3])

    analyze_diversity_score(percentage)
    print_output()
    #print_without_exception()

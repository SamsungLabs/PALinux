import os
import sys
import struct
import ctypes
from ctypes import *
from os import walk

'''
USAGE: python static-validator.py <objdump file> <output file>
OUTPUT: [output file].p1
        [output file].p2
        [output file].p3
        [output file].p4
        [output file].p5
'''

function_identifier = "symbol"
key_op_identifier = "pac"

# Utils
def is_contain_xor_key_op(lines, reverse, idx = 0):
    try:
        if reverse == True:
            if lines[-4].find("eor") == -1 or lines[-4].find("0x3ffff0003ffff") == -1:
                return "none"
            if lines[-3].find("eor") == -1 or lines[-3].find("0x3f003f003f003f") == -1:
                return "none"
            if lines[-2].find("eor") == -1 or lines[-2].find("0x8001800180018001") == -1:
                return "none"
            if lines[-1].find("eor") == -1:
                return "none"
            token = lines[-4].strip().split(',')
            if len(token) != 3:
                return "none"
            op = token[1]
            op = op.strip()
            op = op.replace(' ', '')
            return op
        else:
            if lines[idx].find("eor") == -1 or lines[idx].find("0x3ffff0003ffff") == -1:
                return "none"
            if lines[idx+1].find("eor") == -1 or lines[idx+1].find("0x3f003f003f003f") == -1:
                return "none"
            if lines[idx+2].find("eor") == -1 or lines[idx+2].find("0x8001800180018001") == -1:
                return "none"
            if lines[idx+3].find("eor") == -1:
                return "none"
            if lines[idx+4].find("blr") != -1 or lines[idx+4].find("br") != -1:
                return "none"

            token = lines[idx].strip().split(',')
            if len(token) != 3:
                return "none"
            op = token[1]
            op = op.strip()
            op = op.replace(' ', '')
            return op
        '''
        for num, line in list(enumerate(lines)):
            if lines[num].find("eor") == -1 or lines[num].find("0x3ffff0003ffff") == -1:
                continue
            if lines[num+1].find("eor") == -1 or lines[num+1].find("0x3f003f003f003f") == -1:
                continue
            if lines[num+2].find("eor") == -1 or lines[num+2].find("0x8001800180018001") == -1:
                continue
            
            token = lines[num].strip().split(',')
            if len(token) != 3:
                return "none"
            op = token[1]
            op = op.strip()
            op = op.replace(' ', '')
            return op
        '''
    except:
        return "none"
    return "none"

def reg_is_callee_saved(src_reg):
    if src_reg >= 19:
        return True
    return False

def find_where_pac_is_from(func_body, target, idx):
    dist = 0
    target_list = []
    target_list.append(target)
    has_bl = False
    add_reg_pair = {}
    orig_target = target

    for num, line in reversed(list(enumerate(func_body))):
        if num >= idx:
            continue

        val_flag = False
        if (line.find("adrp") != -1 and line.find("0xff") != -1) or (line.find("autib") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adrp") != -1 and line.find("0xff") != -1) or (line.find("autia") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adr") != -1 and line.find("0xff") != -1) or (line.find("autib") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adr") != -1 and line.find("0xff") != -1) or (line.find("autia") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adrp") != -1 and line.find(", ff") != -1) or (line.find("autib") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adrp") != -1 and line.find(", ff") != -1) or (line.find("autia") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adr") != -1 and line.find(", ff") != -1) or (line.find("autib") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("adr") != -1 and line.find(", ff") != -1) or (line.find("autia") != -1 and line.find("<") == -1):
            val_flag = True
        elif (line.find("mov") != -1 and line.find("#0xff") != -1) or (line.find("autia") != -1 and line.find("<") == -1):
            val_flag = True
        elif line.find("orr") != -1 and line.find("wzr") != -1 and line.find("0x7") != -1:
            val_flag = True

        if line.find("bl\t") != -1 and line.find("<") != -1 and line.find(">") != -1:
            has_bl = True
            tmp_target = orig_target.replace('x', '')
            if int(tmp_target) in add_reg_pair:
                src_reg = add_reg_pair[int(tmp_target)]
                if has_bl == True and reg_is_callee_saved(src_reg) == True:
                    return "non-validated2"
            ## below check must not be executed for PARTS.
            src_reg = int(tmp_target)
            if has_bl == True and reg_is_callee_saved(src_reg) == True:
                return "non-validated2"
            continue
        
        if val_flag == True:
            for tnum, target in enumerate(target_list):
                if line.find(target) != -1: 
                    return "validated"
                '''
                else:
                    tmp_target = target.replace('x', '')
                    if int(tmp_target) in add_reg_pair:
                        src_reg = add_reg_pair[int(tmp_target)]
                        if has_bl == True and reg_is_callee_saved(src_reg) == True:
                            return "non-validated2"
                    elif orig_target == target:
                        tmp_target = target.replace('x', '')
                        src_reg = int(tmp_target)
                        if has_bl == True and reg_is_callee_saved(src_reg) == True:
                            return "non-validated2"
                '''

        if line.find("mov") != -1:
            token = line.split(' ')
            op_dest = token[1]
            op_src = token[2]
            if op_src.find("#") == -1:
                op_dest = op_dest.strip()
                op_dest = op_dest.replace(',', '')
                op_dest_tok = op_dest.split('\t')
                dest_reg = op_dest_tok[1]
                src_reg = op_src
                add_target = False
                for tnum, target in enumerate(target_list):
                    if target == dest_reg:
                        add_target = True
                        break
                if add_target == True:
                    target_list.append(src_reg)
                    tmp_dest_reg = dest_reg.replace('x', '')
                    tmp_src_reg = src_reg.replace('x', '')
                    try:
                        add_reg_pair[int(tmp_dest_reg)] = int(tmp_src_reg)
                    except:
                        dummy = 0
                    if key_op_identifier == "xor" and src_reg == "sp":
                        return "validated"
        elif line.find("add") != -1 and line.find("#0x0") != -1:
            token = line.split(',')
            if len(token) == 3:
                op_dest = token[0]
                op_src = token[1]
                op_dest = op_dest.strip()
                op_dest_si = op_dest.find("add\tx")
                dest_reg = op_dest[op_dest_si+4:]
                op_src = op_src.strip()
                src_reg = op_src.replace(' ', '')
                add_target = False
                for tnum, target in enumerate(target_list):
                    if target == dest_reg:
                        add_target = True
                        break
                if add_target == True:
                    target_list.append(src_reg)
                    tmp_dest_reg = dest_reg.replace('x', '')
                    tmp_src_reg = src_reg.replace('x', '')
                    add_reg_pair[int(tmp_dest_reg)] = int(tmp_src_reg)

        dist += 1
        if dist > 40:
            return "non-validated"
    return "validated"

def get_ret(line):
    if line.find("<") != -1:
        return "none"
    token = line.split(' ')
    op = token[-1]
    op = op.strip()
    if op == "ret":
        return "ret"
    return "none"

def get_pac_target_register(line):
    insn_list = ["pacib", "pacia"]
    if line.find("<") != -1:
        return "none"
    for num, insn in enumerate(insn_list):
        if line.find(insn) != -1:
            token = line.split(' ')
            if len(token) != 3:
                continue
            
            op = token[1]
            op = op.strip()
            op = op.replace(',', '')
            op_tok = op.split('\t')
            return op_tok[1]
    return "none"

def get_aut_target_register(line):
    insn_list = ["autib", "autia", "autiza", "autizb"]
    if line.find("<") != -1:
        return "none"
    for num, insn in enumerate(insn_list):
        if line.find("autiza") != -1 or line.find("autizb") != -1:
            token = line.split(' ')
            op = token[-1]
            op = op.strip()
            return op
        if line.find(insn) != -1:
            token = line.split(' ')
            if len(token) != 3:
                continue
            
            op = token[1]
            op = op.strip()
            op = op.replace(',', '')
            op_tok = op.split('\t')
            return op_tok[1]
    return "none"

def get_xor_aut_target_register(lines, idx):
    return is_contain_xor_key_op(lines, False, idx)

def get_xor_pac_target_register(lines, idx):
    return is_contain_xor_key_op(lines, False, idx)

def get_xpac_target_register(line):
    insn_list = ["xpaci"]
    if line.find("<") != -1:
        return "none"
    for num, insn in enumerate(insn_list):
        if line.find(insn) != -1:
            token = line.split(' ')
            if len(token) != 2:
                continue

            op = token[-1]
            op = op.strip()
            op_tok = op.split('\t')
            return op_tok[1]
    return "none"

def is_load_contains_lr(line):
    target_list = ["x30", "lr"]
    insn_list = ["ldr", "ldp"]
    if line.find("<") != -1:
        return "no"
    
    for num, insn in enumerate(insn_list):
        for reg_num, reg in enumerate(target_list):
            target = reg + ","
            if line.find(insn) != -1 and line.find(target) != -1:
                if line.find("sp") != -1 or line.find("x29") != -1:
                    return "yes"
                else:
                    return "yes_without_sp"
    return "no"

def is_str_contains_target_for_p3(line, target):
    insn_list = ["str", "stp"]
    if line.find("<") != -1:
        return "no"
    for num, insn in list(enumerate(insn_list)):
        if key_op_identifier == "xor":
            #if line.find(insn) != -1 and line.find(target) != -1 and line.find("sp") == -1:
            if line.find(insn) != -1 and line.find(target) != -1:
                token = line.split(' ')
                return "yes"
        else:
            if line.find(insn) != -1 and line.find(target) != -1:
                token = line.split(' ')
                return "yes"
        
        #target_str = " " + target + ","
        #if line.find(insn) != -1 and line.find(target_str) != -1:
        #    return "yes"
    return "no"

def is_str_contains_target_for_p2(line, target):
    insn_list = ["str", "stp"]
    if line.find("<") != -1:
        return "no"
    for num, insn in enumerate(insn_list):
        #target_str = " " + target + ","
        target_str = target + ","
        if line.find(insn) != -1 and line.find(target_str) != -1:
            return "yes"
    return "no"

def get_branch_target_register(line):
    branch_list = ["blr", "br"]
    exception = "brk"
    for num, insn in enumerate(branch_list):
        if line.find(insn) != -1 and line.find(exception) == -1 and line.find("blraa") == -1 and line.find("braa") == -1:
            token = line.split(' ')
            if len(token) != 2:
                continue

            op = token[-1]
            op = op.strip()
            op_tok = op.split('\t')
            return op_tok[1]
    return "none"

confirmed_aut_dict = set()

def find_aut_target_register(func_body, reg):
    global confirmed_aut_dict

    #aut_list = "autib"
    #aut_list = "autia"
    if key_op_identifier == "xor":
        target = is_contain_xor_key_op(func_body, True)
        if target == reg:
            return "validated"
        return "non-validated"

    aut_list = ["autib", "autia"]
    distance = 0
    for nn, insn in enumerate(aut_list):
        for num, line in reversed(list(enumerate(func_body))):
            if line.find(insn) != -1:
                token = line.split(' ')
                if len(token) != 3:
                    continue

                op = token[1]
                op = op.strip()
                op = op.replace(',', '')
                op_tok = op.split('\t')
                
                aut_reg = op_tok[-1]
                if aut_reg == reg:
                    confirmed_aut_dict.add(line)
                    return "validated"

            distance += 1
            if distance > 80:
                break

    return "non-validated"

def find_constant_target_register(func_body, reg, debug = False):
    insn_list = ["ldr", "adr"]
    #insn_list = ["adr"]

    for insn_num, insn in enumerate(insn_list):
        distance = 0
        target_list = []
        target_list.append(reg)

        for num, line in reversed(list(enumerate(func_body))):
            if (line.find("0xff") != -1 and line.find(insn) != -1) or (line.find("<") != -1 and line.find(insn) != -1):
                token = line.split(' ')
                if len(token) != 3 and len(token) != 4:
                    continue

                op = token[1]
                op = op.strip()
                op = op.replace(',', '')
                op_tok = op.split('\t')
                target_reg = op_tok[-1]

                for tnum, target in list(enumerate(target_list)):
                    if target == target_reg:
                        return "validated"
                #if target_reg == reg:
                #    return "validated"
            elif line.find("ldr") != -1:
                for tnum, target in list(enumerate(target_list)):
                    if line.find(target) != -1:
                        # extract new target register
                        si = line.find("[")
                        ei = line.find("]")
                        new_target = line[si+1:ei-1]
                        new_token = new_target.split(',')
                        if len(new_target) < 2:
                            continue
                        if target != new_token[0]:
                            target_list.append(new_token[0])
                            break

            distance += 1
            if distance > 60:
                break
    
    return "non-validated"

# __P1. unprotected control flows (violate R1)__
def find_p1(dump, out):
    outf = open(out + '.p1', "w+")
    with open(dump) as f:
        lines = f.readlines()
        in_func = 0
        target = "none"
        func_body = []
        func_name = ""

        for num, line in enumerate(lines):
            line = line.strip()

            if function_identifier == "symbol":
                if line.find(">:") != -1:
                    if line.find(".") != -1:
                        continue
                    in_func = 1
                    si = line.find("<")
                    ei = line.find(">")
                    line = line[si+1:ei]
                    func_name = line
                    continue
                elif line == "":
                    in_func = 0
                    func_body = []
                    func_name = ""
                    continue
                else:
                    if in_func == 0:
                        func_body = []
                        func_name = ""
                        continue
            elif function_identifier == "pac":
                if line.find("pacibsp") != -1:
                    in_func = 1
                    func_name = "NULL"
                    continue
                elif line.find("retab") != -1:
                    in_func = 0
                    func_body = []
                    func_name = "NULL"
                    continue
                else:
                    if in_func == 0:
                        func_body = []
                        func_name = "NULL"
                        continue

            target = get_branch_target_register(line)
            if target == "none":
                func_body.append(line)
                continue

            ret = find_aut_target_register(func_body, target)
            ret2 = find_constant_target_register(func_body, target)

            if ret == "non-validated" and ret2 == "non-validated":
                ostr = '[nv] func: ' + func_name + ' / line : ' + line + '\n'
                outf.write(ostr)
            
            func_body.append(line)
    outf.close()

#__P2. store a stripped pointer onto memory (violate R2)__
def find_p2(dump, out):
    global confirmed_aut_dict

    outf = open(out + '.p2', "w+")
    with open(dump) as f:
        lines = f.readlines()
        in_func = 0
        target = "none"
        func_body = []
        func_name = ""
        func_inspect = "no"

        for num, line in enumerate(lines):
            line = line.strip()

            if function_identifier == "symbol":
                if line.find(">:") != -1:
                    if line.find(".") != -1:
                        continue
                    in_func = 1
                    si = line.find("<")
                    ei = line.find(">")
                    line = line[si+1:ei]
                    func_name = line
                    func_inspect = "no"
                    continue
                elif line == "":
                    in_func = 0
                    func_inspect = "yes"
                else:
                    if in_func == 0:
                        func_inspect = "yes"
            elif function_identifier == "pac":
                if line.find("pacibsp") != -1:
                    in_func = 1
                    func_name = "NULL"
                    func_inspect = "no"
                    continue
                elif line.find("retab") != -1:
                    in_func = 0
                    func_inspect = "yes"
                else:
                    if in_func == 0:
                        func_inspect = "yes"
            
            if func_inspect == "no":
                func_body.append(line)
            else:
                # inspect p2
                for fnum, fline in enumerate(func_body):
                    if func_body[fnum].find("fffffff0080ccd08:") != -1:
                        print(func_body[fnum+1])

                    xpac_target = get_xpac_target_register(fline)
                    if key_op_identifier == "xor":
                        aut_target = get_xor_aut_target_register(func_body, fnum)
                    else:
                        aut_target = get_aut_target_register(fline)

                    target = ""
                    if xpac_target != "none":
                        target = xpac_target
                    elif aut_target != "none":
                        target = aut_target
                    else:
                        continue
                    if fnum >= (len(func_body) - 8):
                        continue
                    
                    str_ret = is_str_contains_target_for_p2(func_body[fnum+1], target)
                    str_ret2 = is_str_contains_target_for_p2(func_body[fnum+2], target)
                    str_ret3 = is_str_contains_target_for_p2(func_body[fnum+3], target)
                    str_ret4 = is_str_contains_target_for_p2(func_body[fnum+4], target)
                    str_ret5 = is_str_contains_target_for_p2(func_body[fnum+5], target)
                    str_ret6 = is_str_contains_target_for_p2(func_body[fnum+6], target)
                    #if str_ret == "no" and str_ret2 == "no" and str_ret3 == "no":
                    #    continue
                    
                    # xpac(or aut) - str
                    val_flag = "NULL"
                    if str_ret == "yes" or str_ret2 == "yes" or str_ret3 == "yes" or str_ret4 == "yes" or str_ret5 == "yes" or str_ret6 == "yes":
                        val_flag = "XPAC(AUT)-STR"
                    if func_body[fnum+1].find("cmp") != -1 and func_body[fnum+1].find(target) != -1 and func_body[fnum+2].find("b.") != -1 and func_body[fnum+2].find(">") != -1:
                        val_flag = "NULL"  # RePAC case through a conditional branch
                    if fline in confirmed_aut_dict:
                        val_flag = "NULL"

                    #elif func_body[fnum+1].find("b") != -1 and func_body[fnum+1].find(", ff") != -1 and func_body[fnum+1].find("adr") == -1:
                    #    val_flag = "XPAC(AUT)-STR"
                    #elif func_body[fnum+1].find("ret") != -1:
                    #    val_flag = "XPAC(AUT)-STR"
                    if val_flag != "NULL":
                        ostr = '[nv][' + val_flag + '] func: ' + func_name + ' / line : ' + fline + '\n'
                        outf.write(ostr)
                    
                    # xpac - pac
                    val_flag = "NULL"
                    if target == xpac_target:
                        if func_body[fnum+1].find("paci") != -1 and func_body[fnum+1].find(target) != -1 and func_body[fnum+1].find("xpaci") == -1:
                            val_flag = "XPAC-PAC"
                        elif func_body[fnum+2].find("paci") != -1 and func_body[fnum+2].find(target) != -1 and func_body[fnum+2].find("xpaci") == -1:
                            val_flag = "XPAC-PAC"
                        elif func_body[fnum+3].find("paci") != -1 and func_body[fnum+3].find(target) != -1 and func_body[fnum+3].find("xpaci") == -1:
                            val_flag = "XPAC-PAC"
                    if val_flag != "NULL":
                        ostr = '[nv][' + val_flag + '] func: ' + func_name + ' / line : ' + fline + '\n'
                        outf.write(ostr)

                    # xpac - bl / xpac - blr
                    val_flag = "NULL"
                    if target == xpac_target:
                        if (func_body[fnum+1].find("bl") != -1 and func_body[fnum+1].find("0x0ffff") != -1) or (func_body[fnum+1].find("blraa") != -1 and func_body[fnum+1].find(target) != -1):
                            val_flag = "XPAC-BL"
                        elif (func_body[fnum+2].find("bl") != -1 and func_body[fnum+2].find("0x0ffff") != -1) or (func_body[fnum+2].find("blraa") != -1 and func_body[fnum+2].find(target) != -1):
                            val_flag = "XPAC-BL"
                        elif (func_body[fnum+3].find("bl") != -1 and func_body[fnum+3].find("0x0ffff") != -1) or (func_body[fnum+3].find("blraa") != -1 and func_body[fnum+3].find(target) != -1):
                            val_flag = "XPAC-BL" 
                        elif (func_body[fnum+4].find("bl") != -1 and func_body[fnum+4].find("0x0ffff") != -1) or (func_body[fnum+4].find("blraa") != -1 and func_body[fnum+4].find(target) != -1):
                            val_flag = "XPAC-BL"
                        elif (func_body[fnum+5].find("blraa") != -1 and func_body[fnum+5].find(target) != -1):
                            val_flag = "XPAC-BL"
                        elif (func_body[fnum+6].find("blraa") != -1 and func_body[fnum+6].find(target) != -1):
                            val_flag = "XPAC-BL"
                        elif (func_body[fnum+7].find("blraa") != -1 and func_body[fnum+7].find(target) != -1):
                            val_flag = "XPAC-BL"
                        elif (func_body[fnum+8].find("blraa") != -1 and func_body[fnum+8].find(target) != -1):
                            val_flag = "XPAC-BL"
                    if val_flag != "NULL":
                        ostr = '[nv][' + val_flag + '] func: ' + func_name + ' / line : ' + fline + '\n'
                        outf.write(ostr)

                func_body = []
                func_name = ""
    outf.close()

# __P3. store a PACed pointer onto memory (violate R3)__
def find_p3(dump, out):
    outf = open(out + '.p3', "w+")
    with open(dump) as f:
        lines = f.readlines()
        in_func = 0
        target = "none"
        func_body = []
        func_name = ""
        func_inspect = "no"
        one_more_success = False

        for num, line in enumerate(lines):
            line = line.strip()

            if function_identifier == "symbol":
                if line.find(">:") != -1:
                    if line.find(".") != -1:
                        continue
                    in_func = 1
                    si = line.find("<")
                    ei = line.find(">")
                    line = line[si+1:ei]
                    func_name = line
                    one_more_success = False
                    func_inspect = "no"
                    continue
                elif line == "":
                    in_func = 0
                    func_inspect = "yes"
                else:
                    if in_func == 0:
                        func_inspect = "yes"
            elif function_identifier == "pac":
                if line.find("pacibsp") != -1:
                    in_func = 1
                    func_name = "NULL"
                    func_inspect = "no"
                    continue
                elif line.find("retab") != -1:
                    in_func = 0
                    func_inspect = "yes"
                else:
                    if in_func == 0:
                        func_inspect = "yes"
            
            if func_inspect == "no":
                func_body.append(line)
            else:
                # inspect p3
                dist = 8
                if key_op_identifier == "xor":
                    dist = 24
                dist = 24
                for fnum, fline in enumerate(func_body):
                    if key_op_identifier == "xor":
                        target = get_xor_pac_target_register(func_body, fnum)
                    else:
                        target = get_pac_target_register(fline)
                    if target == "none":
                        continue
                    if fnum >= (len(func_body) - dist):
                        continue
                    
                    val_flag = False
                    for idx in range(dist):
                        str_ret = is_str_contains_target_for_p3(func_body[fnum+idx+1], target)
                        if str_ret == "yes":
                            val_flag = True
                            break
                    
                    if val_flag == False:
                        continue
                    if func_body[fnum+1].find("<") != -1:   # branch instruction
                        continue
                    if func_body[fnum-1].find("paci") != -1:  # exception case for parts-llvm
                        continue

                    '''
                    str_ret = is_str_contains_target_for_p3(func_body[fnum+1], target)
                    str_ret2 = is_str_contains_target_for_p3(func_body[fnum+2], target)
                    str_ret3 = is_str_contains_target_for_p3(func_body[fnum+3], target)
                    if str_ret == "no" and str_ret2 == "no" and str_ret3 == "no":
                        continue
                    if func_body[fnum+1].find("<") != -1:   # branch instruction
                        continue
                    '''

                    pret = find_where_pac_is_from(func_body, target, fnum)
                    if pret == "non-validated":
                        if key_op_identifier == "xor" and one_more_success == True:
                            continue
                        ostr = '[nv] func: ' + func_name + ' / line : ' + fline + '\n'
                        outf.write(ostr)
                    elif pret == "non-validated2":
                        ostr = '[nv-pac2] func: ' + func_name + ' / line : ' + fline + '\n'
                        outf.write(ostr)
                    else:
                        one_more_success = True

                func_body = []
                func_name = ""
    outf.close()

def find_p4(dump, out):
    outf = open(out + '.p4', "w+")
    with open(dump) as f:
        lines = f.readlines()
        in_func = 0
        target = "none"
        func_body = []
        func_name = ""
        func_inspect = "no"

        for num, line in enumerate(lines):
            line = line.strip()

            if line.find(">:") != -1:
                if line.find(".") != -1:
                    continue
                in_func = 1
                si = line.find("<")
                ei = line.find(">")
                line = line[si+1:ei]
                func_name = line
                func_inspect = "no"
                continue
            elif line == "":
                in_func = 0
                func_inspect = "yes"
            else:
                if in_func == 0:
                    func_inspect = "yes"
            
            if func_inspect == "no":
                func_body.append(line)
            else:
                # inspect p5
                for fnum, fline in enumerate(func_body):
                    insn = get_ret(fline)
                    if insn == "none":
                        continue
                    if fnum < 5:
                        continue
                    ldr_ret = is_load_contains_lr(func_body[fnum-1])
                    ldr_ret2 = is_load_contains_lr(func_body[fnum-2])
                    ldr_ret3 = is_load_contains_lr(func_body[fnum-3])
                    ldr_ret4 = is_load_contains_lr(func_body[fnum-4])
                    ldr_ret5 = is_load_contains_lr(func_body[fnum-5])
                    '''
                    if ldr_ret == "yes" or ldr_ret2 == "yes" or ldr_ret3 == "yes":  # in the case of delivering return address by stack.
                        continue
                    if ldr_ret == "no" and ldr_ret2 == "no" and ldr_ret3 == "no":  # in the case of delivering return address by register only.
                        continue
                    '''
                    if ldr_ret == "yes_without_sp" or ldr_ret2 == "yes_without_sp" or ldr_ret3 == "yes_without_sp" or ldr_ret4 == "yes_without_sp" or ldr_ret5 == "yes_without_sp":
                        ostr = '[nv] func: ' + func_name + ' / line : ' + fline + '\n'
                        outf.write(ostr)

                func_body = []
                func_name = ""
    outf.close()

# __Spill__
def find_p6(dump, out):
    outf = open(out + '.p4', "a")  # we put it in p4 family
    with open(dump) as f:
        lines = f.readlines()
        in_func = 0
        target = "none"
        func_body = []
        func_name = "NULL"
        func_inspect = "no"

        for num, line in enumerate(lines):
            line = line.strip()
            if num >= (len(lines) - 4):
                continue

            # Detect saving all registers
            str_ret = is_str_contains_target_for_p3(lines[num].strip(), "x10")
            str_ret2 = is_str_contains_target_for_p3(lines[num+1].strip(), "x12")
            str_ret3 = is_str_contains_target_for_p3(lines[num+2].strip(), "x14")
            str_ret4 = is_str_contains_target_for_p3(lines[num+3].strip(), "x16")
            if str_ret == "yes" and str_ret2 == "yes" and str_ret3 == "yes" and str_ret4 == "yes":
                ostr = '[nv] func: ' + func_name + ' / line : ' + line + '\n'
                outf.write(ostr)
    outf.close()

# XPAC + PAC
def find_p7(dump, out):
    outf = open(out + '.p1', "a")  # we put it in p1 family
    with open(dump) as f:
        lines = f.readlines()
        in_func = 0
        target = "none"
        func_body = []
        func_name = "NULL"
        func_inspect = "no"

        for fnum, fline in enumerate(lines):
            fline = fline.strip()
            if fnum >= (len(lines) - 4):
                continue

            xpac_target = get_xpac_target_register(fline)
            if xpac_target == "none":
                continue

            val_flag = "NULL"
            if lines[fnum+1].find("paci") != -1 and lines[fnum+1].find(xpac_target) != -1 and lines[fnum+1].find("xpaci") == -1:
                val_flag = "XPAC-PAC"
            elif lines[fnum+2].find("paci") != -1 and lines[fnum+2].find(xpac_target) != -1 and lines[fnum+2].find("xpaci") == -1:
                val_flag = "XPAC-PAC"
            elif lines[fnum+3].find("paci") != -1 and lines[fnum+3].find(xpac_target) != -1 and lines[fnum+3].find("xpaci") == -1:
                val_flag = "XPAC-PAC"
            if val_flag != "NULL":
                ostr = '[nv][' + val_flag + '] func: ' + func_name + ' / line : ' + fline + '\n'
                outf.write(ostr)
    outf.close()

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("USAGE : python static-validator.py <objdump file> <output file> <function identifier>")
        sys.exit(0)
    
    # function identifier:  "symbol",  ffff000010081440 <do_sysinstr>:  (start) -- new line (end)
    # function identifier:  "pac",  pacibsp (start) -- retab (end)
    # key operation identifier: "pac", blraa, pac, aut
    # key operation identifier: "xor", using xor instead of pac, aut
    function_identifier = sys.argv[3]
    key_op_identifier = "pac" 
    
    find_p1(sys.argv[1], "out/"+sys.argv[2])
    find_p2(sys.argv[1], "out/"+sys.argv[2])
    #find_p7(sys.argv[1], sys.argv[2])

    find_p3(sys.argv[1], "out/"+sys.argv[2])
    find_p4(sys.argv[1], "out/"+sys.argv[2])

    #find_p6(sys.argv[1], "out/"+sys.argv[2])
    find_p7(sys.argv[1], "out/"+sys.argv[2])

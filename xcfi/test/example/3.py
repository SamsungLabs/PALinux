#!/usr/bin/python3
import subprocess
import os
import sys
import subprocess as sp

print("==============================================================")
print("Req-F-03 : Test for Pointer Authentication\n")
print("Total test case : 2")
print("==============================================================\n")

#set env
#TODO: replace to absolute path
TEST_ROOT = "./"
CC_PATH = "aarch64-linux-gnu-gcc"

TARGET = "target"
TEST_SOURCE = "3.cpp"
EXECUTE_PATH = TEST_ROOT + TARGET
PLUGIN_PATH = "../../xcfi.so"

#TODO : TOOL_ROOT has to be changed for test
TOOL_ROOT="./"
TOOL_PATH=TOOL_ROOT+"qemu-aarch64"

success_cnt=0
fail_cnt=0

def clean_resource(res):
    if os.path.isfile(res):
        subprocess.call(["rm", res])

def success_call():
    global success_cnt
    print("> Success\n")
    success_cnt+=1

def fail_call():
    global fail_cnt
    print("> Fail\n")
    fail_cnt+=1

def disassamble_func(exe_path, func, ret):
    readelf = sp.Popen(['readelf', '-s', exe_path], stdout=sp.PIPE)
    grep = sp.Popen(['grep',func], stdin=readelf.stdout, stdout=sp.PIPE, encoding='utf8')
    result = grep.communicate()[0].split('\n')
    for line in result:
        sym = line.split()
        if len(sym) < 8:
            continue
        if sym[7].split('@')[0] == func:
            saddr = int(sym[1], 16)
            size = int(sym[2])
            eaddr = saddr + size

            startaddr = "--start-address="+str(saddr)
            stopaddr = "--stop-address="+str(eaddr)
            retfile = open(ret, 'w')
            dump = sp.Popen(["aarch64-linux-gnu-objdump", "-d", exe_path, startaddr, stopaddr], stdout=sp.PIPE, encoding='utf8')
            for line in dump.stdout:
                sys.stdout.write(line)
                retfile.write(line)
            dump.wait()
            retfile.close()


#Start to test
print("1. Check if the pointer is signed with PAC key ")
clean_resource(EXECUTE_PATH)
subprocess.call([CC_PATH, TEST_SOURCE, "-o", TARGET, "-fplugin="+PLUGIN_PATH, "--static", "-march=armv8.3-a"])

TARGET_FUNC = "main"

#Disassemble enabled target
ENABLE_DUMP = "dump-pac"
print("\n--- Enabled target dump -------------------------------------------------")
print("--- Target function : ", TARGET_FUNC)

disassamble_func(EXECUTE_PATH, TARGET_FUNC, ENABLE_DUMP)

#Disassemble disabled target
DISABLE_DUMP = "dump-non-pac"
subprocess.call([CC_PATH, TEST_SOURCE, "-o", TARGET, "--static", "-march=armv8.3-a"])

print("\n--- Disabled target dump------------------------------------------------")
print("--- Target function : ", TARGET_FUNC)

disassamble_func(EXECUTE_PATH, TARGET_FUNC, DISABLE_DUMP)

print("--------------------------------------------------------------------------\n")
print("\n")

result = subprocess.call(["diff", ENABLE_DUMP, DISABLE_DUMP, "-q"])
if result != 0:
    success_call()
else:
    fail_call()

clean_resource(ENABLE_DUMP)
clean_resource(DISABLE_DUMP)

print("\n2. Check if the pointer is signed with PAC key on runtime")

if os.path.isfile(TOOL_PATH) is False:
    clean_resource(TARGET)
    print("> Please build qemu-aarch64 from paclinux git")
    sys.exit(0)

subprocess.call([CC_PATH, TEST_SOURCE, "-o", TARGET, "-fplugin="+PLUGIN_PATH, "--static", "-march=armv8.3-a"])
tmp = subprocess.call([TOOL_PATH, EXECUTE_PATH])
if tmp == 0:
    success_call()
else:
    fail_call()

print("==========================================================================")
print("[Total tests : 2] ", "- success : ", success_cnt, " / fail : ", fail_cnt)
print("============================ Test end ====================================")

clean_resource(TARGET)

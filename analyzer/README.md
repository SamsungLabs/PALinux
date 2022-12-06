# Context analyzer evaluation

## 0. Context analyzer

Usage (just for your information, you don't need to look at this usage at this phase)
```
USAGE: ./analyzer [bitcode file name] [objbind or retbind or stat] [pointer-only-flag or retbind threshold] [objbind struct name]

objbind (pointer-only) for a certain struct: ./analyzer vmlinux.bc objbind 1 struct.irqaction
objbind (all-field) for a certain struct: ./analyzer vmlinux.bc objbind 0 struct.irqaction
objbind for all structs: ./analyzer vmlinux.bc objbind 1
objbind for specified structs: ./analyzer vmlinux.bc objbind 1 objbind_structs.csv
--- pointer-only mode performs static analysis only for pointer-type fields ---
For retbind: ./analyzer vmlinux.bc retbind 20
For retbind: ./analyzer vmlinux.bc retbind 20 retbind_functions.csv
--- here, 20 is a threshold value on the number of callers ---
stat: ./analyzer vmlinux.bc stat 100
--- 'stat' computes and reports allowed targets ---
```

## 1. Reproducing TAT of Table 6 in the paper

* TAT: the number of structs that rank top 10% in allowed targets

1. Unzip `vmlinux_bc.zip` fine in prebuilt directory
```
$ cd ./prebuilt/
$ 7z x vmlinux_bc.zip
```

2. Producing report on allowed targets for each context
```
// USAGE : ./analyzer <bc file> stat <threshold allowed target>
// stat: computes and reports allowed targets

// INPUT  FILE : bc file
// OUTPUT FILE : vmlinux-stat-log.txt

$ ./analyzer-tool/build/analyzer ./prebuilt/vmlinux.bc stat 0 > vmlinux-stat-log.txt
// tip: you can see its progress using `tail -f vmlinux-stat-log.txt`.
```

3. Run python script to extract structs that rank top 10% in allowed target.
* The result is used to get TDS.
```
// USAGE : python extract_top.py <log file> <percent>

// INPUT  FILE : vmlinux-stat-log.txt (from the previous phase)
// OUTPUT FILE : vmlinux-stat-10-percent.txt

$ python extract_top.py vmlinux-stat-log.txt 10 > vmlinux-stat-10-percent.txt
$ wc -l vmlinux-stat-10-percent.txt | awk '{ print $1 }'
89   // 'wc' outputs a number that matches with TAT of Table 6 in the paper.
```

## 2. Reproducing TDS of Table 6 in the paper

* TDS at k: the number of struct that rank top k% in diversity scores

1. Run analyzer to estimate diversity score of all structs
* Note: This phase takes many hours (about 6-8, but that depends) so we recommend to run it as a background task.
* Note: To save your time, you can skip this phase and go to the phase 3 along with prebuilt/vmlinux-diversity-score.csv
```
// USAGE : ./analyzer <bc file> objbind 1

// INPUT  FILE : bc file
// OUTPUT FILE : vmlinux-objbind-log.txt

$ ./build/analyzer ../prebuilt/vmlinux_bc/vmlinux.bc objbind 1 > vmlinux-objbind-log.txt
```

2. Run a helper script to calculate diversity score from the objbind results
```
// USAGE : python get_div.py <log file>

// INPUT  FILE : vmlinux-objbind-log.txt
// OUTPUT FILE : vmlinux-diversity-score.csv

$ python get_div.py vmlinux-objbind-log.txt > vmlinux-diversity-score.csv
```

3. Run python script to calculate TDS and check its result
```
// USAGE : python stat.py <objbind struct list> <diversity score file> <diversity score percentage>

// INPUT  FILE : vmlinux-stat-10-percent.txt (derived from the phase of reproducing TAT), vmlinux-diversity-score.csv
//               (if you've skipped both of phase1 and 2, please use prebuilt/vmlinux-diversity-score.csv as a second input)

$ python stat.py vmlinux-stat-10-percent.txt  vmlinux-diversity-score.csv 10
```

After you run the above command `python stat.py vmlinux-stat-10-percent.txt vmlinux-diversity-score.csv 10`, you can see in console the following log.  
The key result that you need to check is
- `total: 89`: it corresponds to linux's TAT, specified in Table 6.
- `included: 33`: it corresponds to linux's TDS at 10%, specified in Table 6.

```
// First column: struct name
// Second column: Included / Not included / Exception
    // Included/Not included : indicate whether or not its diversity score is in range of the given percentage.
    // Exception: indicate a case that a score that analyzer estimates is not that high, but in reality, the score has to be as high as to be in 'Included'.
// Third column: diversity score

struct.spi_nor / Not included / Score: 7
struct.ahash_request / Exception
struct.phy_driver / Included / Score: 18
struct.work_struct / Included / Score: 59
struct.sdhci_acpi_slot / Not included / Score: 6
struct.fb_ops / Not included / Score: 2
struct.rpc_procinfo / Included / Score: 176
==========================
total: 89                           // TAT of linux, First low first column of table 6
not included: 52
included: 33                        // TDS at 10% of linux, First low second column of table 6         
include-percent: 0.370786516854     
include-percent-w/exception: 0.415730337079   
```

In general, you can reproduce the result of `TDS at K %` in Table 6, by executing `python stat.py vmlinux-stat-10-percent.txt vmlinux-diversity-score.csv K` and checking its result in the same way above.

# PAL's full workflow (Figure2 in paper)

This guides will help you walk through all functions PAL presents,
in the same order to that of Figure2 (i.e., Section4 Design) in our paper.
With this workflow, you can evaluate all functional aspects of PAL.

## Workflow

**1. Running context analyzer with a desired CFI precision level**

Context analyzer runs on LLVM IR on a whole kernel binary.
So, you need to derive LLVM IR file (.bc - bitcode file) from kernel build using LLVM.
NOTE: it takes about 1 hour. if you want to skip this procedure (or get in a trouble with something), use `analyzer/prebuilt/vmlinux.bc` instead.
```
$ make config-static-analysis
$ cd linux
$ BINUTILS_TARGET_PREFIX=aarch64-linux-gnu ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make CC="gclang -w" olddefconfig
$ BINUTILS_TARGET_PREFIX=aarch64-linux-gnu ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make CC="gclang -w" -j4  // build command
...... // kernel build... wait until it is done...
...... // troubleshoot: if you encounter a build error (e.g., fixdep:), just run the build command again and again until
       //     the final 'vmlinux' comes out. this is an issue of gclang.
$ ./gen_bc.sh
$ ls -l vmlinux.bc  // vmlinux.bc is a LLVM IR file for a whole kernel binary.
```

And then, produce a CFI precision report by giving the analyzer `vmlinux.bc`.
NOTE: it takes about 10-20 minutes.
```
$ cd analyzer
$ ./analyzer-tool/build/analyzer ../linux/vmlinux.bc stat 0 > vmlinux-stat-log.txt
// vmlinux-stat-log.txt is a CFI precision report that shows the number of allowed targets involved in contexts
// for example,
// '[objtype] Type: struct.irq_chip.64301 / GEN: 37 / USE: 374 / AllowedTarget: 13838'
// --> this line means that the allowed target of struct.irq_chip is 13838 even in which objtype is applied.
//
// tip: you can see its progress using `tail -f vmlinux-stat-log.txt`.
```

Next, run a helper script with the CFI precision level you want.
In this example, it is assumed that you want to protect structs that rank top 2% in allowed target. (i.e., bottom 2% in CFI precision)
This script will show a list of such structs, meaning that they need to be refined further using dynamic contexts (objbind).
```
$ cd analyzer
$ python extract_top.py vmlinux-stat-log.txt 2 > vmlinux-stat-2-percent.txt
// arg '2' means that percentage here.
// `vmlinux-stat-2-percent.txt` shows a list of structs that rank top 2% in allowed target.
```

Lastly, estimate diversity score of the structs that need further protection.
It is to identify the best field for objbind to be applied. (Section 4.3)
On top of that, pick out what to protect using retbind.
```
$ cd analyzer
$ ./analyzer-tool/build/analyzer ../linux/vmlinux.bc objbind 0 vmlinux-stat-2-percent.txt
// this command estimates diversity score of the structs in vmlinux-stat-2-percent.txt (i.e., top 2% in allowed target)
$ ./analyzer-tool/build/analyzer ../linux/vmlinux.bc retbind 50 retbind_functions.csv
// this command means that only functions that has more than 50 callers are in interest.
$ ls -l objbind_score.csv retbind_score.csv
// once analyzer is done with it, you can see objbind_score.csv, which records diversity score of each struct, and retbind_score.csv, which records the number of callers.
// for example,
// 'struct.work_struct,1,59' in objbind_score.csv means that the maximum of diversity score of work_struct is 59 at field-1.
// 'struct.file_operations,-2,280' --> -2 is a special keyword, indicating that there are 280 global variables of file_operations.
```

**2. Building PAL-protected kernel along with the guide that analyzer produces**

Let kernel-makefile know where the guide is, and build the kernel with our compiler instrumentation plugin.
The compiler plugin would interpret `objbind_score.csv`/`retbind_score.csv` and apply them accordingly.

NOTE: if you get in a trouble here, just directly go to the Option-2 of the next part. (`3. Run static validator...`). You can use a prebuilt file we made in advance.

```
$ cp -f analyzer/objbind_score.csv linux/objbind_score.csv
$ cp -f analyzer/retbind_score.csv linux/retbind_score.csv
$ rm -f linux/pal_dump.txt
$ make clean-linux
$ make config-all
$ cd linux
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- oldconfig
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j4 Image dtbs
...... // kernel build... wait until it is done...
$ ls -l vmlinux   // if it goes successful, vmlinux (the final kernel binary) comes out
```

**3. Run static validator to find out insecure uses of PA in the final kernel binary**

Option-1: Run static validator (with vmlinux you made)
```
$ cd linux/
$ aarch64-linux-gnu-objdump -D vmlinux > vmlinux.dump
$ cd ../static-validator
$ python static-validator.py ../linux/vmlinux.dump pal_report symbol
.... // wait for analysis to complete, it takes about 5 mins.
$ ls -l out/pal_report*
// pal_report.* are reports of potential violations that need to be confirmed by hand.
// see static-validator/README.md for more information about what these reports mean
```

Option-2: Run static validator (with pal.dump prebuilt we made)
```
$ cd ../static-validator/prebuilt/
$ unzip pal-dump.zip
$ cd ../
$ python static-validator.py prebuilt/pal.dump pal_report symbol
.... // wait for analysis to complete, it takes about 5 mins.
$ ls -l out/pal_report*
// pal_report.* are reports of potential violations that need to be confirmed by hand.
// see static-validator/README.md for more information about what these reports mean
```

Validate each item in the reports by manual analysis, and fix them if needed.
(Note: This is out of scope in our paper. We do not present any developer-friendly solution for this procedure)

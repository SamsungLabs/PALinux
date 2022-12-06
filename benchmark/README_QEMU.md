# Reproducing benchmark results (on QEMU)

With QEMU, it's not possible replicating the benchmark results in our paper.
This guide's been made only for the case of not being able to access mac mini.

## Prerequisite: building QEMU

```
# in the root directory of pal-ae repo
$ make build-qemu  # build qemu
```

## Run micro benchmarks (lmbench, perf)

#### Commands

Without PAL protection (Table C1 - Stock)
```
$ cd benchmark/
$ ./run-qemu-linux.sh qemu/kernel/Image.not_protected.gz rootfs/initramfs_micro_bench.cpio.gz
// arg0: gzipped prebuilt kernel image
// arg1: root filesystem that contains macro benchmark binaries
```

With PAL protection (Table C1 - w/PAL)
```
$ cd benchmark/
$ ./run-qemu-linux.sh qemu/kernel/Image.protected.gz rootfs/initramfs_micro_bench.cpio.gz
```

#### Checking results

Once run the above command, kernel will boot up and in turn micro benchmarks automatically start.
You can see the following messages coming out to a console.
```
==== perf: start ====    // perf benchmark begins
./perf bench sched messaging
....
Total time: 0.154 [sec]  // it corresponds to 'Table C1:Linux perf:messaging' in the paper
....
./perf bench sched pipe
Total time: 17.681 [sec] // it corresponds to 'Table C1:Linux perf:pipe' in the paper
....
==== perf: end ====
....
==== lmbench: start ====  // lmbench begins
....  // benchmarks will be performed in the same order to 'Table C1' in the paper
....  // note: pipe and fork+execve benchmark would take few minutes.
==== lmbench: start ====
```

## Run macro benchmarks (apache, blogbench, leveldb)

NOTE: there is an issue in running macro benchmarks on QEMU. Please do not try this yet.

#### Commands

Without PAL protection (Table C1 - Stock)
```
$ cd benchmark/
$ ./run-qemu-linux.sh qemu/kernel/Image.not_protected.gz rootfs/initramfs_macro_bench.cpio.gz
```

With PAL protection (Table C1 - w/PAL)
```
$ cd benchmark/
$ ./run-qemu-linux.sh qemu/kernel/Image.protected.gz rootfs/initramfs_macro_bench.cpio.gz
```

#### Checking results

Once run the above command, kernel will boot up and in turn macro benchmarks automatically start.
You can see the following messages coming out to a console.
```
==== apache (1kb): start ==== // apache benchmark starts
....
Time per request: 0.153 [msg] (mean) // it corresponds to 'Table C1:apache:1kb' in the paper
....
==== apache (1kb): end ==== 
....
==== leveldb: start ==== // leveldb starts
fillseq:   4.006 micros/op:   // it corresponds to 'Table C1:leveldb:fillseq' in the paper
.... 
==== leveldb: end ====
....
==== blogbench: start ==== // blogbench starts
write(): No space left on devicewrite()  // it shows up a lot but you can simply ignore it
....
Final score for writes:  392  // it corresponds to 'Table C1:blogbench:write' in the paper
Final score for reads:  433915  // it corresponds to 'Table C1:blogbench:read' in the paper
==== blogbench: end ====
```

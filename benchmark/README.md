# Reproducing benchmark results (on Mac mini with M1)

## Hardware dependency

It is required to physically access a recent Mac mini built on top of M1 chip.
Also, you need a USB type C cable and HDMI cable for connection between the mac mini and your host PC.

## Prerequisite: configuring your macOS

NOTE: this step is necessary even if you are using VM image.

The first step needed is to adjust your macOS to be amenable to Linux, more precisely, Asahi Linux
that is customized Linux for apple silicon family.
There are two guides to do this.
We used the first one during our development, because the second one was not available at that time.
But, as the second one is much easier to follow, we recommend the second one.
These guides are ultimately about how to divide your macOS into multiple partitions
to make a space for the boot-relevant stuffs (e.g., boot loader) of AsahiLinux to be stored in.
This means, after configuration goes successful, that you can choose which OS to boot up, between mac and linux.

- [Asahi Linux Developer Quick Guide](https://github.com/AsahiLinux/docs/wiki/Developer-Quickstart#setup)
- [Asahi Linux Developer Quick Guide 2.0](https://github.com/AsahiLinux/docs/wiki/Developer-Quickstart-2.0)
    - Do follow instructions in '__Hardware Requirements__' and '__Installing m1n1 on your Apple Silicon Mac__'. (and ignore others)

If everything works OK, you can see [this screen](https://raw.githubusercontent.com/amworsley/asahi-wiki/main/images/usb-setup.png) when you power on mac mini.

## Run micro benchmarks (lmbench, perf)

NOTE: please keep in mind that you might not be able to the exactly same number to that of Table C1 in the paper.
Benchmark numbers would in nature fluctuate.

#### Commands

Without PAL protection (Table C1 - Stock)
```
// Make sure that mac mini is powered up, before running this command
$ cd benchmark/
$ ./run-m1-linux.sh m1/kernel/Image.not_protected.gz m1/kernel/t8103-j274.dtb rootfs/initramfs_micro_bench.cpio.gz
// arg0: gzipped prebuilt kernel image
// arg1: dtb
// arg2: root filesystem that contains macro benchmark binaries
```

With PAL protection (Table C1 - w/PAL)
```
// Make sure that mac mini is powered up, before running this command
$ cd benchmark/
$ ./run-m1-linux.sh m1/kernel/Image.protected.gz m1/kernel/t8103-j274.dtb rootfs/initramfs_micro_bench.cpio.gz
```

#### Checking results

Once run the above command, kernel will boot up and in turn micro benchmarks automatically start.
You can see the following messages coming out to a monitor connected to Mac mini via HDMI.
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

#### Commands

Without PAL protection (Table C1 - Stock)
```
// Make sure that mac mini is powered up, before running this command
$ cd benchmark/
$ ./run-m1-linux.sh m1/kernel/Image.not_protected.gz m1/kernel/t8103-j274.dtb rootfs/initramfs_macro_bench.cpio.gz
```

With PAL protection (Table C1 - w/PAL)
```
// Make sure that mac mini is powered up, before running this command
$ cd benchmark/
$ ./run-m1-linux.sh m1/kernel/Image.protected.gz m1/kernel/t8103-j274.dtb rootfs/initramfs_macro_bench.cpio.gz
```

#### Checking results

Once run the above command, kernel will boot up and in turn macro benchmarks automatically start.
You can see the following messages coming out to a monitor connected to Mac mini via HDMI.
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

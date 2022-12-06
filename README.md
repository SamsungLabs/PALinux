# PALinux
In-Kernel Control-Flow Integrity on Commodity OSes using ARM Pointer Authentication

## PAL Artifact Evaluation

We've confirmed this artifact on Ubuntu 18.04/20.04 host machines, not virtual guest machines.  

NOTE: We also tried our artifact on virtual guest Ubuntu but found some can go wrong.  
So we recommend running this on host machines if possible.  

## Install dependencies

For ubuntu 18.04, `$./bin/install_18.sh`
For ubuntu 20.04, `$./bin/install_20.sh`

## Build and additional setup

Build modified GCC 7.3, GCC plugin for instrumentation, LLVM plugin for context analyzer  
```
$ ./bin/build.sh
```

After build, please set up `gclang` according with `bin/gclang-setup.md`.

## Functional evaluation

The all functions making up PAL are illustrated in Figure2 (i.e., Section4 Design) in our paper.  
To evaluate all functional aspects in Figure2, see and follow instructions in [full-workflow/README.md](full-workflow/README.md).

## Reproducing results

#### Context analyzer (Table6)

See [analyzer/README.md](analyzer/README.md).

#### CFI precision (Table2 and Table3)

See 'Reproducing results of Table2 and Table3 in the paper' in [precision/README.md](precision/README.md).

#### Static validator (Section 4.5 - Results)

See 'Confirmed cases' in [static-validator/README.md](static-validator/README.md).

#### Benchmarks on Mac mini (Table C1 and Section 6.3 - Performance Overhead)

See [benchmark/README.md](benchmark/README.md).

Note:
Please try it on host machine rather than virtual guest machine if possible.  
We've not confirmed it on between mac mini and VM.

Note:
We're not sure if you would be able to have a physical access to M1 mac mini and go successful with macOS configuration.  
Any other apple silicon built on top of M1 chip would probably work with the above guide. But we have confirmed our evaluation only on Mac mini. If something goes wrong or you do not want to try this on your apple machine, let us know.  
We can prepare a QEMU-based environment to help you run those benchmarks. (but, it won't be able to reproduce numbers in paper)

#### Benchmarks on QEMU

See [benchmark/README_QEMU.md](benchmark/README_QEMU.md)

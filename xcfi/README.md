# Control Flow Integrity
## Environment
- <del>This project was tested on gcc 5.4.0, Ubuntu 16.04</del>
- This project was tested on gcc 7.3.0, Ubuntu 18.04
## Prerequisite for build
- Install GCC development library on Ubuntu
  - Ubuntu 16.04
  ```
  $ sudo apt-get install libgcc-5-dev
  ```
  - Ubuntu 18.04
  ```
  $ sudo apt-get install gcc-7-plugin-dev
  ```
## How to build and test
- Build gcc plugin like the follows, You can see xcfi.so
  ```
  $ make
  ```
- After build, "xcfi.so" will be created. Use this plugin when compiling somethin by gcc
  ```
  $ gcc -fplugin=./xcfi.so <C++ files> -o <Binary output>
  ```
- Build simple tests
  ```
  $ make test
  ```
- Build a benchmark as a LKM to check if ARM PAC operations work
  ```
  $ make KDIR=<Kernel source directory> pacbench
- Build examples as a LKM
  ```
  $ cd test
  $ make KDIR=<CFI-applied kernel source directory> lkm-example
  $ mv example.ko example-cfi.ko
  $ make KDIR=<Base kernel source directory> lkm-example
  $ mv example.ko example-nocfi.ko
  ```
  In the target,
  ```
  # insmod example-<cfi or nocfi>.ko
  # cat /proc/pac-example
  ```

#!/bin/sh

sudo apt-get -y install make cmake wget
make build-dep
sudo apt-get -y install clang-9 llvm-9 llvm-9-dev llvm-9-tools binutils-aarch64-linux-gnu
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-9 100
sudo apt-get -y install python3 python3-pip
pip3 install --user pyserial construct serial.tool

sudo apt-get -y install gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 20
sudo update-alternatives --config gcc

sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 20
sudo update-alternatives --config g++

SCRIPT_DIR=$(dirname $0)
$SCRIPT_DIR/install_gclang.sh


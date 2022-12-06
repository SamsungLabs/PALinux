#!/usr/bin/sudo bash

sudo sed -i "s/# deb-src http:\/\/archive.ubuntu.com\//deb-src http:\/\/archive.ubuntu.com\//g" /etc/apt/sources.list

if [[ -z `grep "bionic" /etc/apt/sources.list` ]] ; then
	sudo echo "deb http://archive.ubuntu.com/ubuntu/ bionic main restricted" >> /etc/apt/sources.list
	sudo echo "deb-src http://archive.ubuntu.com/ubuntu/ bionic main restricted" >> /etc/apt/sources.list
	sudo echo "deb http://archive.ubuntu.com/ubuntu/ bionic-updates main restricted" >> /etc/apt/sources.list
	sudo echo "deb-src http://archive.ubuntu.com/ubuntu/ bionic-updates main restricted" >> /etc/apt/sources.list
fi

SCRIPT_DIR=$(dirname $0)
$SCRIPT_DIR/install_18.sh

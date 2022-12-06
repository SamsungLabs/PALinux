#!/bin/bash

if ! command -v go &>/dev/null; then
	wget https://go.dev/dl/go1.16.14.linux-amd64.tar.gz
	sudo tar -C /usr/share/ -xzf go1.16.14.linux-amd64.tar.gz
	rm go1.16.14.linux-amd64.tar.gz

	echo "export PATH=\$PATH:/usr/share/go/bin" >> ~/.bashrc
	echo "export GOPATH=/usr/share/go" >> ~/.bashrc
	echo "export GOROOT=/usr/share/go" >> ~/.bashrc

	source ~/.bashrc
fi

go get github.com/SRI-CSL/gllvm/cmd/...

sudo rm -f /usr/bin/objcopy
sudo ln -s /usr/bin/aarch64-linux-gnu-objcopy /usr/bin/objcopy
gclang --version
sudo rm -f /usr/bin/llvm-link
sudo ln -s /usr/bin/llvm-link-9 /usr/bin/llvm-link


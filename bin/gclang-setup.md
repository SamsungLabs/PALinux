# gclang setup guide

Download go. If your system has already go, skip this procedure.
```
$ wget https://go.dev/dl/go1.16.14.linux-amd64.tar.gz
```

Extract tar.gz. `/your/path/go` would come out after decompression.
You have to replace `/your/path` with where you want to install go.
```
$ tar -C /your/path/ -xzf go1.16.14.linux-amd64.tar.gz
```

Add it to the PATH environment variables
```
$ vim ~/.bashrc  // or something like that, e.g., ~/.profile)
$ export PATH=$PATH:/your/path/go/bin
$ export GOPATH=/your/path/go
$ export GOROOT=/your/path/go
$ source ~/.bashrc
```

Download gclang
```
go get github.com/SRI-CSL/gllvm/cmd/...
```

Replace objcopy
```
$ sudo rm -f /usr/bin/objcopy
$ sudo ln -s /usr/bin/aarch64-linux-gnu-objcopy /usr/bin/objcopy
$ gclang --version  (assume that version is 9)
$ sudo rm -f /usr/bin/llvm-link
$ sudo ln -s /usr/bin/llvm-link-9 /usr/bin/llvm-link (please use the equivalent version to gclang's)
```

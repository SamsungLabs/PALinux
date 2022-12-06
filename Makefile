NCPUS := $(shell ncpus)
DEPS  := linux
OUT   := out
QEMU  := $(OUT)/qemu-system-aarch64
PKGS  := mdm gcc-7-plugin-dev gcc-7-aarch64-linux-gnu python3-termcolor \
	 gdb-multiarch python3-numpy python3-matplotlib
BCFI_BLACKLIST :=

define check_config
	@cat linux/.config | grep "CONFIG_ARM64_PTR_AUTH"
	@cat linux/.config | grep "KPAC"
	@cat linux/.config | grep "STACKPROTECTOR"
endef


all:
	@echo "[!] available build commands:"
	@cat Makefile | grep "^build" | cut -d: -f1

build-all:
	make build-gcc
	make build-xcfi
	make build-qemu
	make build-initrd
	make build-linux
	make build-microbench

build-dep:
	sudo apt update
	sudo apt -y build-dep $(DEPS) || true
	sudo apt -y install $(PKGS) || true

	pip install --user pwntools

build-gcc:
	bin/build-gcc.sh

build-xcfi:
	(cd xcfi; make -j $(NCPUS))
	(ln -f -s ../xcfi/out/xcfi.so $(OUT)/xcfi.so)

build-linux:
	@if [ ! -e linux/.config ]; then \
	  echo "Please select a proper config"; \
	  cat Makefile | grep "^config"; \
	  exit 1; \
	fi
	$(check_config)
	bin/build-linux.sh
	ln -f -s ../linux/vmlinux out/vmlinux
	ln -f -s ../linux/arch/arm64/boot/Image out/vmlinuz

	# NB. don't need for now as we exclude all 'init' functions
	# bin/patch-ssp-blacklist.py linux/vmlinux $(BCFI_BLACKLIST)
	# bin/build-linux.sh Image

build-linux-llvm:
	@if [ ! -e linux/.config ]; then \
	  echo "Please select a proper config"; \
	  cat Makefile | grep "^config"; \
	  exit 1; \
	fi
	$(check_config)
	bin/build-linux-llvm.sh
	ln -f -s ../linux/vmlinux out/vmlinux
	ln -f -s ../linux/arch/arm64/boot/Image out/vmlinuz

build-linux-gllvm:
	@if [ ! -e linux/.config ]; then \
	  echo "Please select a proper config"; \
	  cat Makefile | grep "^config"; \
	  exit 1; \
	fi
	$(check_config)
	bin/build-linux-gllvm.sh
	ln -f -s ../linux/vmlinux out/vmlinux
	ln -f -s ../linux/arch/arm64/boot/Image out/vmlinuz
	cd out/
	extract-bc ./vmlinux

build-initrd:
	./bin/build-initrd.sh
	ln -f -s ../busybox/initrd.img $(OUT)/initrd.img

build-qemu:
	sudo rm -f /usr/bin/objcopy
	sudo ln -s /usr/bin/x86_64-linux-gnu-objcopy /usr/bin/objcopy
	./bin/build-qemu.sh
	ln -f -s ../qemu/build/aarch64-softmmu/qemu-system-aarch64 $(OUT)/qemu-system-aarch64
	ln -f -s ../qemu/build/aarch64-linux-user/qemu-aarch64 $(OUT)/qemu-aarch64
	sudo rm -f /usr/bin/objcopy
	sudo ln -s /usr/bin/aarch64-linux-gnu-objcopy /usr/bin/objcopy

build-microbench:
	(cd benchmark/micro; make -j $(NCPUS) KDIR=../../linux; \
		mkdir -p ../../busybox/_install/bench ; \
		cp -f pacbench.ko ../../busybox/_install/bench/.)
	./bin/rebuild-initrd.sh

build-bruteforce:
	(cd benchmark/bruteforce; make -j $(NCPUS) KDIR=../../linux; \
		mkdir -p ../../busybox/_install/bench ; \
		cp -f bruteforce.ko ../../busybox/_install/bench/.)
	./bin/rebuild-initrd.sh

build-macrobench:
	(cd benchmark/perf; \
		mkdir -p ../../busybox/_install/bench ; \
		cp -rf lmbench nbench perf spec2006 run-bench.sh \
			 ../../busybox/_install/bench/.)
	./bin/rebuild-initrd.sh

run-qemu:
	QEMU=$(QEMU) KERN=$(OUT)/vmlinuz INIT=$(OUT)/initrd.img ./bin/run-qemu.sh

config-static-analysis:
	cp -f config linux/.config
	(cd linux; ./scripts/config -d ARM64_PTR_AUTH \
								-d KPAC \
								-d KPAC_SIGN_RETURN_ADDRESS \
								-d ALTERNATIVE_KPAC \
								-d ALTERNATIVE_KPAC_SIGN_RETURN_ADDRESS \
								-d CC_IS_GCC \
								-d CC_HAS_ASM_GOTO \
								-d EFI_STUB \
                                -d EFI \
								-e CC_IS_CLANG \
								-d STACKPROTECTOR \
								-d STACKPROTECTOR_STRONG)

config-none:
	cp -f config linux/.config
	(cd linux; ./scripts/config -d ARM64_PTR_AUTH \
								-d KPAC \
								-d KPAC_SIGN_RETURN_ADDRESS \
                                -d KPAC_PROTECT_PREEMPTION \
								-e STACKPROTECTOR \
								-e STACKPROTECTOR_STRONG)
	$(check_config)

config-all:
	cp -f config linux/.config
	(cd linux; ./scripts/config -e ARM64_PTR_AUTH \
								-e KPAC \
								-e KPAC_SIGN_RETURN_ADDRESS \
								-e STACKPROTECTOR \
                                -e KPAC_PROTECT_PREEMPTION \
								-e STACKPROTECTOR_STRONG)
	$(check_config)

config-backward-cfi:
	cp -f config linux/.config
	(cd linux; ./scripts/config -e ARM64_PTR_AUTH \
								-e KPAC_SIGN_RETURN_ADDRESS \
								-e DEBUG_INFO \
								-e GDB_SCRIPTS \
								-e FRAME_POINTER \
								-e STACKPROTECTOR \
								-e STACKPROTECTOR_STRONG)
	$(check_config)

config-forward-cfi:
	cp -f config linux/.config
	(cd linux; ./scripts/config -e ARM64_PTR_AUTH \
								-e KPAC)
	$(check_config)

config-alternative-all:
	cp -f config linux/.config
	(cd linux; ./scripts/config -e ALTERNATIVE_KPAC \
								-e ALTERNATIVE_KPAC_SIGN_RETURN_ADDRESS \
								-e STACKPROTECTOR \
								-e STACKPROTECTOR_STRONG)
	$(check_config)

config-alternative-forward-cfi:
	cp -f config linux/.config
	(cd linux; ./scripts/config -e ALTERNATIVE_KPAC)
	$(check_config)

config-alternative-backward-cfi:
	cp -f config linux/.config
	(cd linux; ./scripts/config -e ALTERNATIVE_KPAC_SIGN_RETURN_ADDRESS \
								-e STACKPROTECTOR \
								-e STACKPROTECTOR_STRONG)
	$(check_config)

clean-linux:
	(cd linux; make ARCH=arm64 clean)

.PHONY: build-dep build-gcc build-xcfi build-linux build-initrd build-qemu run-qemu clean-linux

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Compilação para AOSP
ARCH = arm64

# vazio CROSS_COMPILE pois estamos utilizando clang
CROSS_COMPILE = 

AOSP_KERNEL = /home/$(USER)/raspberry_kernel/common


# Usando o clang-17 

LLVM = 1
LLVM_IAS = 1
OBJCOPY = llvm-objcopy-17
OBJDUMP = llvm-objdump-17
AR = llvm-ar-17
NM = llvm-nm-17
STRIP = llvm-strip-17

# RPi4 
RPI4_DEFCONFIG = android_rpi4_defconfig
RPI4_DEFCONFIG_PATH = $(AOSP_KERNEL)/arch/arm64/configs/$(RPI4_DEFCONFIG)

BEAR := $(shell command -v bear 2>/dev/null)

.PHONY: all clean aosp config aosp-full aosp-kernel

# Bazel targets
RPI4_BAZEL_TARGET = //common:rpi4
BAZEL_CONFIGS = --config=fast --config=stamp


# Regras de compilação para x86 
all:
ifdef BEAR
	@echo "[+] Building module + generating compile_commands.json"
	@rm -f compile_commands.json kernel/compile_commands.json
	@bear -- $(MAKE) -C $(KDIR) M=$(PWD)/kernel modules
	@ln -s ../compile_commands.json kernel/compile_commands.json
else
	@echo "[+] Building module (Bear not found)"
	@$(MAKE) -C $(KDIR) M=$(PWD)/kernel modules
endif

# Regras de compilação para aosp 

# Configuração do kernel aosp
config:
	@echo "[+] Configuring kernel source for module building"
	cd $(AOSP_KERNEL) && \
	make ARCH=arm64 $(RPI4_DEFCONFIG) && \
	make ARCH=arm64 oldconfig && \
	make ARCH=arm64 prepare && \
	make ARCH=arm64 modules_prepare 


# Compilação do kernel com Bazel

aosp-kernel:
	@echo "[+] Smart cleaning for Bazel"
	cd $(AOSP_KERNEL) && \
	rm -f .config .config.old && \
	rm -rf include/config/ include/generated/ && \
	rm -f Module.symvers modules.order && \
	find . -type f -name "*.o" -delete && \
	find . -type f -name ".*.cmd" -delete && \
	find . -type f -name "*.ko" -delete
	@echo "[+] Building with Bazel"
	cd $(AOSP_KERNEL) && cd .. && \
	tools/bazel build --config=fast --config=stamp //common:rpi4

aosp-full: aosp-kernel config
	@echo "[+] Building AOSP module"
	@$(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		LLVM=1 \
		LLVM_IAS=1 \
		KCFLAGS="-Wno-error=unknown-argument -Wno-error=unused-command-line-argument" \
		M=$(PWD)/kernel \
		modules# Compila somente o driver se o kernel já está compilado COM BEAR
aosp: 
ifdef BEAR
	@echo "[+] Building AOSP module only + generating compile_commands.json"
	@rm -f compile_commands.json kernel/compile_commands.json
	@bear -- $(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		CC="$(CC)" \
		LD="$(LD)" \
		KCFLAGS="-Wno-error=attributes" \
		M=$(PWD)/kernel \
		modules
	@ln -s ../compile_commands.json kernel/compile_commands.json
else
	@echo "[+] Building AOSP module only (Bear not found)"
	@$(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		CC="$(CC)" \
		LD="$(LD)" \
		KCFLAGS="-Wno-error=attributes" \
		M=$(PWD)/kernel \
		modules
endif

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/kernel clean
	@rm -f compile_commands.json kernel/compile_commands.json@rm -f compile_commands.json kernel/compile_commands.json

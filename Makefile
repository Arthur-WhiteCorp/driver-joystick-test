KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Compilação para AOSP
ARCH = arm64

# Usando o clang-17 
CLANG = clang-17
CLANG_TARGET = aarch64-linux-gnu
CC = $(CLANG) -target $(CLANG_TARGET)
LD = ld.lld-17
OBJCOPY = llvm-objcopy-17
OBJDUMP = llvm-objdump-17
AR = llvm-ar-17
NM = llvm-nm-17
STRIP = llvm-strip-17

# vazio CROSS_COMPILE pois estamos utilizando clang
CROSS_COMPILE = 

AOSP_KERNEL = /home/arthur/raspberry_kernel/common

# RPi4 
RPI4_DEFCONFIG = android_rpi4_defconfig
RPI4_DEFCONFIG_PATH = $(AOSP_KERNEL)/arch/arm64/configs/$(RPI4_DEFCONFIG)

BEAR := $(shell command -v bear 2>/dev/null)

.PHONY: all clean aosp config aosp-full aosp-kernel

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
	cd $(AOSP_KERNEL) && \
	make ARCH=$(ARCH) \
	     CC="$(CC)" \
	     LD="$(LD)" \
	     OBJCOPY="$(OBJCOPY)" \
	     OBJDUMP="$(OBJDUMP)" \
	     AR="$(AR)" \
	     NM="$(NM)" \
	     STRIP="$(STRIP)" \
	     distclean && \
	make ARCH=$(ARCH) \
	     CC="$(CC)" \
	     LD="$(LD)" \
	     OBJCOPY="$(OBJCOPY)" \
	     OBJDUMP="$(OBJDUMP)" \
	     AR="$(AR)" \
	     NM="$(NM)" \
	     STRIP="$(STRIP)" \
	     $(RPI4_DEFCONFIG) && \
	./scripts/config \
		--enable CONFIG_MODVERSIONS \
		--enable CONFIG_MODULES

# Compila todos os módulos do kernel AOSP (incluindo exports)
aosp-kernel: config 
	cd $(AOSP_KERNEL) && \
	make ARCH=$(ARCH) \
	     CC="$(CC)" \
	     LD="$(LD)" \
	     OBJCOPY="$(OBJCOPY)" \
	     OBJDUMP="$(OBJDUMP)" \
	     AR="$(AR)" \
	     NM="$(NM)" \
	     STRIP="$(STRIP)" \
	     KCFLAGS="-Wno-error=attributes" \
	     Image.gz modules 

# Compilação para aosp junto com o kernel
aosp-full: aosp-kernel
	$(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		CC="$(CC)" \
		LD="$(LD)" \
		KCFLAGS="-Wno-error=attributes" \
		M=$(PWD)/kernel \
		modules

# Compila somente o driver se o kernel já está compilado
aosp: 
	$(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		CC="$(CC)" \
		LD="$(LD)" \
		KCFLAGS="-Wno-error=attributes" \
		M=$(PWD)/kernel \
		modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/kernel clean
	@rm -f compile_commands.json kernel/compile_commands.json@rm -f compile_commands.json kernel/compile_commands.json

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)


#Compilação para AOSP
ARCH = arm64
CROSS_COMPILE = aarch64-linux-gnu-
AOSP_KERNEL = /home/arthur/Kernel-Aosp14/common




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
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) defconfig && \
	./scripts/config --enable CONFIG_MODVERSIONS

# Compila todos os módulos do kernel AOSP (incluindo exports)
aosp-kernel: config 
	cd $(AOSP_KERNEL) && \
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) Image.gz modules 


# Compilação para aosp junto com o kernel
aosp-full: aosp-kernel
	$(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD)/kernel \
		modules

#Compila somente o driver se o kernel já está compilado
aosp: 
	$(MAKE) -C $(AOSP_KERNEL) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD)/kernel \
		modules


clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/kernel clean
	@rm -f compile_commands.json kernel/compile_commands.json


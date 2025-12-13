KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

BEAR := $(shell command -v bear 2>/dev/null)

.PHONY: all clean

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

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/kernel clean
	@rm -f compile_commands.json kernel/compile_commands.json


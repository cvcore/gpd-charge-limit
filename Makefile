# Out-of-tree build for gpd_charge_limit.
#
# Kept deliberately sandbox-clean: no depmod, no $(src) tricks, and every
# install path overridable. Nix and distro packagers should not need to patch
# this file.

obj-m := gpd_charge_limit.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
KVER      ?= $(shell basename $$(dirname $(KERNELDIR)))

# Where `make install` puts the module. DESTDIR is prepended so staged
# installs (packaging) work without touching the live tree.
INSTALL_MOD_DIR ?= /lib/modules/$(KVER)/extra

.PHONY: all modules clean install

all: modules

modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

# Plain install rather than `modules_install`: the latter runs depmod, which
# needs -b handling and fails in sandboxed builds. Callers that want depmod
# can run it themselves.
install: modules
	install -D -m 0644 gpd_charge_limit.ko \
		$(DESTDIR)$(INSTALL_MOD_DIR)/gpd_charge_limit.ko

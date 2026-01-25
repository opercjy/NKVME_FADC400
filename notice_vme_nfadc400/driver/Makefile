# Rocky Linux 9 Kernel Module Makefile

obj-m := nkusbvme.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c modules.order Module.symvers
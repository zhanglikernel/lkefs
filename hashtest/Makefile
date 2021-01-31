ifeq ($(DEBUG),y)
    DEBFLAGS = -DTREETESTDEBUG
else
    DEBFLAGS = 
endif
KBUILD_EXTRA_SYMBOLS += /home/gosadmin/lkefs/btree/Module.symvers
export KBUILD_EXTRA_SYMBOLS

ifneq ($(KERNELRELEASE),)

.PHONY := clean

obj-m := hashtest.o
hashtest-objs := hash_test.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CURRDIR := $(PWD)
ccflags-y := -I$(CURRDIR)/include
ccflags-y += $(DEBFLAGS)

modules:
	$(MAKE) -C $(KERNELDIR) ccflags-y="$(ccflags-y)" M=$(PWD) modules

endif

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -rf modules.order *.mod

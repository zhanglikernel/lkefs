DEBUG:=y
ifeq ($(DEBUG),y)
    DEBFLAGS = -DDEBUG
else
    DEBFLAGS = 
endif

ifneq ($(KERNELRELEASE),)

obj-m :=lkebtree.o
lkebtree-y := btree.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CURRDIR := $(PWD)
ccflags-y := -I$(CURRDIR)/include
ccflags-y += $(DEBFLAGS)

modules:
	$(MAKE) -C $(KERNELDIR) ccflags-y="$(ccflags-y)" M=$(PWD) modules

endif

.PHONY := clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

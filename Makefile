DEBUG:=y
ifeq ($(DEBUG),y)
	DEBFLAGS = -DDEBUG
else
	DEBFLAGS = 
endif

ifneq ($(KERNELRELEASE),)

obj-m := lke_fs.o
obj-m += lkebtree.o
obj-m += lkemd4.o
lke_fs-y := super.o	inode.o dir.o file.o namei.o
lkebtree-y := btree/btree.o
lkemd4-y := hash/md4.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CURRDIR := $(PWD)

ccflags-y := -I$(CURRDIR)/include
ccflags-y += $(DEBFLAGS)

modules:
	$(MAKE)	-C $(KERNELDIR) ccflags-y="$(ccflags-y)" M=$(PWD) modules

endif

.PHONY := clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

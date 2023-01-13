#
# Makefile for the FST-2482P Fan device drivers.
#
# Advantech eAutomation Division
#

ifndef kversion
	kversion=$(shell uname -r)
endif

MODULES := /lib/modules/${kversion}/

ifndef KERNEL_DIR
    KERNEL_DIR:=$(MODULES)/build
endif

obj-m := adv_fan_drv.o
PWD := $(shell pwd)
ADVMOD := adv_fan_drv
KBUILD_EXTRA_SYMBOLS := $(src)/../mfd-ec/Module.symvers
default:
	@sleep 1
	@make -C $(KERNEL_DIR) M=$(PWD) modules

install: default
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	@insmod $(ADVMOD).ko
		
uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	@echo ""

clean:
	@make -C$(KERNEL_DIR) M=$(PWD) clean


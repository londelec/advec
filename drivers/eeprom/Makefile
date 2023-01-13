#
# Makefile for the Advantech TPC-1x51T eeprom driver.
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

obj-m := adv_eeprom_drv.o
PWD := $(shell pwd)
ADVMOD := adv_eeprom_drv

KBUILD_EXTRA_SYMBOLS := $(src)/../mfd-ec/Module.symvers

default:
	@sleep 1
	@make -C $(KERNEL_DIR) M=$(PWD) modules

install:default
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	$(shell [ -c /dev/$(ADVMOD) ] && rm -f /dev/$(ADVMOD))
	@insmod $(ADVMOD).ko
		
uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	$(shell [ -c /dev/$(ADVMOD) ] && rm -f /dev/$(ADVMOD))
	@echo ""

clean:
	@make -C $(KERNEL_DIR) M=$(PWD) clean

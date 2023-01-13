#
# Makefile for the UNO DIO device drivers.
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

obj-m := adv_common_drv.o
PWD := $(shell pwd)
ADVMOD := adv_common_drv

KBUILD_EXTRA_SYMBOLS := $(src)/../mfd-ec/Module.symvers

default:
	@sleep 1
	@make -C $(KERNEL_DIR) M=$(PWD) modules

install: default
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	@insmod $(ADVMOD).ko
	@sleep 1
	@rm -f /dev/watchdog
	@if [ $$(awk '$$2=="adv_watchdog" {print $$1}' /proc/devices) ]; then \
        mknod /dev/watchdog c $$(awk '$$2=="adv_watchdog" {print $$1}' /proc/devices) 0; \
	fi
		
uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	$(shell [ -c /dev/advcommon ] && rm -f /dev/advcommon)
	@echo ""

clean:
	@make -C $(KERNEL_DIR) M=$(PWD) clean

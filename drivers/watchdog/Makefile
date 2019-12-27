#
# Makefile for the UNO DIO device drivers.
#
# Advantech eAutomation Division
#

obj-m := adv_wdt_drv.o
UNAME := $(shell uname -r)
KDIR := /home/dell/Documents/Firmware/linux-source-4.4.0/
PWD := $(shell pwd)
ADVMOD := adv_wdt_drv

default:
	@cp ../mfd-ec/Module.symvers ./
	@sleep 1
#	@make -C /lib/modules/$(UNAME)/build SUBDIRS=$(PWD) modules
	@make -C $(KDIR) SUBDIRS=$(PWD) modules

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
	$(shell [ -c /dev/watchdog ] && rm -f /dev/watchdog)
	@echo ""

clean:
	@rm -f *.o
	@rm -f *.ko
	@rm -f *.mod.c
	@rm -f .*.cmd
	@rm -rf .tmp_versions
	@rm -rf Module.*
	@rm -rf *~
	@rm -rf *order

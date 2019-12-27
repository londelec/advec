#
# Makefile for the Advantech TPC-8100TR brightness driver.
#
# Advantech eAutomation Division
#

UNAME := $(shell uname -r)
KDIR := /home/dell/Documents/Firmware/linux-source-4.4.0/
PWD := $(shell pwd)
ADVMOD := adv_ec
obj-m := adv_ec.o

default:
#	@make -C /lib/modules/$(UNAME)/build SUBDIRS=$(PWD) modules
	@make -C $(KDIR) SUBDIRS=$(PWD) modules

install: default
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	rmmod $(ADVMOD) ; fi)
	@insmod $(ADVMOD).ko
		
uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	rmmod $(ADVMOD) ; fi)

clean:
	@rm -f *.o
	@rm -f *~
	@rm -f *.ko
	@rm -f *.mod.c
	@rm -f .*.cmd	
	@rm -rf .tmp_versions
	@rm -rf Module*
	@rm -rf module*

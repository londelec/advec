#
# Makefile for the Advantech TPC-8100TR brightness driver.
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

PWD := $(shell pwd)
ADVMOD := adv_ec
obj-m := adv_ec.o

default:
	@make -C $(KERNEL_DIR) M=$(PWD) modules

install: default
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	rmmod $(ADVMOD) ; fi)
	@insmod $(ADVMOD).ko
		
uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	rmmod $(ADVMOD) ; fi)

clean:
	@make -C $(KERNEL_DIR) M=$(PWD) clean

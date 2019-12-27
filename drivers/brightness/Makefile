#
# Makefile for the Advantech TPC-8100TR brightness driver.
#
# Advantech eAutomation Division
#

UNAME := $(shell uname -r)
PWD := $(shell pwd)
ADVMOD := adv_brightness_drv

obj-m := adv_brightness_drv.o

default:
	@cp ../mfd-ec/Module.symvers ./
	@sleep 1
	@make -C /lib/modules/$(UNAME)/build SUBDIRS=$(PWD) modules
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
	@rm -f *.o
	@rm -f *~
	@rm -f *.ko
	@rm -f *.mod.c
	@rm -f .*.cmd	
	@rm -rf .tmp_versions
	@rm -rf Module*
	@rm -rf module*


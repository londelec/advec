#
# Makefile for the Advantech TPC-1x51T eeprom driver.
#
# Advantech eAutomation Division
#

obj-m := adv_eeprom_drv.o
UNAME := $(shell uname -r)
PWD := $(shell pwd)
ADVMOD := adv_eeprom_drv

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
	@rm -f *.ko
	@rm -f *.mod.c
	@rm -f .*.cmd	
	@rm -rf .tmp_versions
	@rm -rf Module*
	@rm -f *~
	@rm -rf module*


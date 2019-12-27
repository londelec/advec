UNAME := $(shell uname -r)
KDIR := /home/dell/Documents/Firmware/linux-source-4.4.0/
PWD := $(shell pwd)
ADVMOD := adv_gpio_drv

obj-m := adv_gpio_drv.o

default:
	@cp ../mfd-ec/Module.symvers ./
	@sleep 1
#	@make -C /lib/modules/${UNAME}/build SUBDIRS=${PWD} modules
	@make -C $(KDIR) SUBDIRS=${PWD} modules

install: default
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	$(shell [ -c /dev/advgpio ] && rm -f /dev/advgpio)
	@insmod $(ADVMOD).ko
	@sleep 1
		
uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then \
	 rmmod $(ADVMOD) ; fi)
	$(shell [ -c /dev/advgpio ] && rm -f /dev/advgpio)
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


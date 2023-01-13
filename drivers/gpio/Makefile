
ifndef kversion
	kversion=$(shell uname -r)
endif

MODULES := /lib/modules/${kversion}/

ifndef KERNEL_DIR
    KERNEL_DIR:=$(MODULES)/build
endif

PWD := $(shell pwd)
ADVMOD := adv_gpio_drv

obj-m := adv_gpio_drv.o
KBUILD_EXTRA_SYMBOLS := $(src)/../mfd-ec/Module.symvers

default:
	@sleep 1
	@make -C $(KERNEL_DIR) M=${PWD} modules

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
	@make -C $(KERNEL_DIR) M=$(PWD) clean

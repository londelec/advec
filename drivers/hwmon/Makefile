ifndef kversion
	kversion=$(shell uname -r)
endif

MODULES := /lib/modules/${kversion}/

ifndef KERNEL_DIR
    KERNEL_DIR:=$(MODULES)/build
endif

PWD := $(shell pwd)
ADVMOD := adv_hwmon_drv
f75375sModule := /lib/modules/$(UNAME)/kernel/drivers/hwmon/f75375s.ko
obj-m := adv_hwmon_drv.o
KBUILD_EXTRA_SYMBOLS := $(src)/../mfd-ec/Module.symvers
default:
	@sleep 1
	@make -C $(KERNEL_DIR) M=$(PWD) modules
install:default
	@if [ -f $(f75375sModule) ]; then rm -f $(f75375sModule); fi
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then rmmod $(ADVMOD) ; fi)
	@modprobe i2c-i801
	@insmod $(ADVMOD).ko

uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then rmmod $(ADVMOD) ; fi)
	@echo ""

clean:
	@make -C $(KERNEL_DIR) M=$(PWD) clean

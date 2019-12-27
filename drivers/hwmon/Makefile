UNAME := $(shell uname -r)
KDIR := /home/dell/Documents/Firmware/linux-source-4.4.0/
PWD := $(shell pwd)
ADVMOD := adv_hwmon_drv
f75375sModule := /lib/modules/$(UNAME)/kernel/drivers/hwmon/f75375s.ko
obj-m := adv_hwmon_drv.o

default:
	@cp ../mfd-ec/Module.symvers ./
	@sleep 1
#	@make -C /lib/modules/$(UNAME)/build SUBDIRS=$(PWD) modules
	@make -C $(KDIR) SUBDIRS=$(PWD) modules

install: default
	@if [ -f $(f75375sModule) ]; then rm -f $(f75375sModule); fi
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then rmmod $(ADVMOD) ; fi)
	@modprobe i2c-i801
	@insmod $(ADVMOD).ko

uninstall:
	$(shell if grep $(ADVMOD) /proc/modules > /dev/null ; then rmmod $(ADVMOD) ; fi)
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


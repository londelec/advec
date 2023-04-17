#
## Makefile for the Advantech ec driver.
#
## Advantech eAutomation Division
#
#
UNAME := $(shell uname -r)
subdir := mfd-ec brightness gpio led common watchdog hwmon eeprom fan
f75375sModule := /lib/modules/$(UNAME)/kernel/drivers/hwmon/f75375s.ko

KERNEL_DIR := /home/dell/Documents/Code/linux
export KERNEL_DIR

ifneq ($(KERNELRELEASE),)
# When building from kernel tree.
# Don't build from advec source directory, because
# ARCH=i386 has to be specified.
# This command extracts "obj-m := xxx" from subdirectory Makefiles.
obj-m := $(foreach N,$(subdir),$(N)/$(shell sed -n s/"obj-m :=[ ]*"//p $(KBUILD_EXTMOD)/$(N)/Makefile))
endif

default:
	$(foreach N,$(subdir),make -C $(N);)

install:default
	$(shell if grep adv_brightness_drv /proc/modules > /dev/null ; then \
	 rmmod adv_brightness_drv ; fi)
	$(shell if grep adv_gpio_drv /proc/modules > /dev/null ; then \
         rmmod adv_gpio_drv ; fi)
	$(shell if grep adv_led_drv /proc/modules > /dev/null ; then \
         rmmod adv_led_drv ; fi)
	$(shell if grep adv_common_drv /proc/modules > /dev/null ; then \
         rmmod adv_common_drv ; fi)
	$(shell if grep adv_wdt_drv /proc/modules > /dev/null ; then \
         rmmod adv_wdt_drv ; fi)
	$(shell if grep adv_hwmon_drv /proc/modules > /dev/null ; then \
         rmmod adv_hwmon_drv ; fi)
	$(shell if grep adv_eeprom_drv /proc/modules > /dev/null ; then \
         rmmod adv_eeprom_drv ; fi)
	$(shell if grep adv_fan_drv /proc/modules > /dev/null ; then \
         rmmod adv_fan_drv ; fi)
	$(shell if grep adv_ec /proc/modules > /dev/null ; then \
         rmmod adv_ec ; fi)
	@if [ -f $(f75375sModule) ]; then rm -f $(f75375sModule); fi
	@-modprobe i2c-i801
	@-insmod mfd-ec/adv_ec.ko
	@-insmod brightness/adv_brightness_drv.ko
	@-insmod gpio/adv_gpio_drv.ko
	@-insmod led/adv_led_drv.ko
	@-insmod common/adv_common_drv.ko
	@-insmod watchdog/adv_wdt_drv.ko
	@-insmod hwmon/adv_hwmon_drv.ko
	@-insmod eeprom/adv_eeprom_drv.ko
	@-insmod fan/adv_fan_drv.ko
	@-rm -f /dev/advled
	@if [ $$(awk '$$2=="adv_led" {print $$1}' /proc/devices) ]; then \
        mknod /dev/advled c $$(awk '$$2=="adv_led" {print $$1}' /proc/devices) 0; fi
	@-rm -f /dev/advcommon
	@if [ $$(awk '$$2=="adv_common" {print $$1}' /proc/devices) ]; then \
        mknod /dev/advcommon c $$(awk '$$2=="adv_common" {print $$1}' /proc/devices) 0; fi
	@-rm -f /dev/watchdog
	@if [ $$(awk '$$2=="adv_watchdog" {print $$1}' /proc/devices) ]; then \
        mknod /dev/watchdog c $$(awk '$$2=="adv_watchdog" {print $$1}' /proc/devices) 0; fi

uninstall:
	$(shell if grep adv_brightness_drv /proc/modules > /dev/null ; then \
		rmmod adv_brightness_drv ; fi)
	$(shell if grep adv_gpio_drv /proc/modules > /dev/null ; then \
		rmmod adv_gpio_drv ; fi)
	$(shell if grep adv_led_drv /proc/modules > /dev/null ; then \
		rmmod adv_led_drv ; fi)
	$(shell if grep adv_common_drv /proc/modules > /dev/null ; then \
		rmmod adv_common_drv ; fi)
	$(shell if grep adv_wdt_drv /proc/modules > /dev/null ; then \
		rmmod adv_wdt_drv ; fi)
	$(shell if grep adv_hwmon_drv /proc/modules > /dev/null ; then \
		rmmod adv_hwmon_drv ; fi)
	$(shell if grep adv_eeprom_drv /proc/modules > /dev/null ; then \
		rmmod adv_eeprom_drv ; fi)
	$(shell if grep adv_fan_drv /proc/modules > /dev/null ; then \
		rmmod adv_fan_drv ; fi)
	$(shell if grep adv_ec /proc/modules > /dev/null ; then \
		rmmod adv_ec ; fi)
	@rm -f /dev/advled
	@rm -f /dev/advcommon
	@rm -f /dev/watchdog
clean:
	$(foreach N,$(subdir),make clean -C $(N);)

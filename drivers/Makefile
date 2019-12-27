#
## Makefile for the Advantech ec driver.
#
## Advantech eAutomation Division
#
#
UNAME := $(shell uname -r)
subdir := mfd-ec brightness gpio led common watchdog hwmon eeprom
f75375sModule := /lib/modules/$(UNAME)/kernel/drivers/hwmon/f75375s.ko

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
	$(shell if grep adv_ec /proc/modules > /dev/null ; then \
		rmmod adv_ec ; fi)
	@rm -f /dev/advled
	@rm -f /dev/advcommon
	@rm -f /dev/watchdog
clean:
	@rm -f watchdog/*.o
	@rm -f watchdog/*.ko
	@rm -f watchdog/*.mod.c
	@rm -f watchdog/.*.cmd
	@rm -rf watchdog/.tmp_versions
	@rm -rf watchdog/Module.*
	@rm -rf watchdog/module*
	@rm -rf watchdog/*~
	@rm -f gpio/*.o
	@rm -f gpio/*.ko
	@rm -f gpio/*.mod.c
	@rm -f gpio/.*.cmd
	@rm -rf gpio/.tmp_versions
	@rm -rf gpio/Module.*
	@rm -rf gpio/module*
	@rm -rf gpio/*~
	@rm -f led/*.o
	@rm -f led/*.ko
	@rm -f led/*.mod.c
	@rm -f led/.*.cmd
	@rm -rf led/.tmp_versions
	@rm -rf led/Module.*
	@rm -rf led/module*
	@rm -rf led/*~
	@rm -f common/*.o
	@rm -f common/*.ko
	@rm -f common/*.mod.c
	@rm -f common/.*.cmd
	@rm -rf common/.tmp_versions
	@rm -rf common/Module.*
	@rm -rf common/module*
	@rm -rf common/*~
	@rm -f brightness/*.o
	@rm -f brightness/*.ko
	@rm -f brightness/*.mod.c
	@rm -f brightness/.*.cmd
	@rm -rf brightness/.tmp_versions
	@rm -rf brightness/Module.*
	@rm -rf brightness/module*
	@rm -rf brightness/*~
	@rm -f hwmon/*.o
	@rm -f hwmon/*.ko
	@rm -f hwmon/*.mod.c
	@rm -f hwmon/.*.cmd
	@rm -rf hwmon/.tmp_versions
	@rm -rf hwmon/Module.*
	@rm -rf hwmon/module*
	@rm -rf hwmon/*~
	@rm -f eeprom/*.o
	@rm -f eeprom/*.ko
	@rm -f eeprom/*.mod.c
	@rm -f eeprom/.*.cmd
	@rm -rf eeprom/.tmp_versions
	@rm -rf eeprom/Module.*
	@rm -rf eeprom/module*
	@rm -rf eeprom/*~
	@rm -f mfd-ec/*.o
	@rm -f mfd-ec/*.ko
	@rm -f mfd-ec/*.mod.c
	@rm -f mfd-ec/.*.cmd
	@rm -rf mfd-ec/.tmp_versions
	@rm -rf mfd-ec/Module.*
	@rm -rf mfd-ec/module*
	@rm -rf mfd-ec/*~

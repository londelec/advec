#!/bin/bash 
#
## Auto-loader for the Advantech ec drivers.
## Advantech eAutomation Division
#

DRIVERDIR=$(cd `dirname $0`; pwd)

if [ "x`cat /proc/modules |grep adv_brightness_drv`" != "x" ]; then
	rmmod adv_brightness_drv
fi
if [ "x`cat /proc/modules |grep adv_gpio_drv`" != "x" ]; then
	rmmod adv_gpio_drv
fi
if [ "x`cat /proc/modules |grep adv_led_drv`" != "x" ]; then
	rmmod adv_led_drv
fi
if [ "x`cat /proc/modules |grep adv_common_drv`" != "x" ]; then
	rmmod adv_common_drv
fi
if [ "x`cat /proc/modules |grep adv_wdt_drv`" != "x" ]; then
	rmmod adv_wdt_drv
fi
if [ "x`cat /proc/modules |grep adv_hwmon_drv`" != "x" ]; then
	rmmod adv_hwmon_drv
fi
if [ "x`cat /proc/modules |grep adv_eeprom_drv`" != "x" ]; then
	rmmod adv_eeprom_drv
fi
if [ "x`cat /proc/modules |grep adv_ec`" != "x" ]; then
	rmmod adv_ec
fi

modprobe i2c-i801

modprobe adv_ec
modprobe adv_eeprom_drv
modprobe adv_hwmon_drv
modprobe adv_brightness_drv
modprobe adv_gpio_drv
modprobe adv_led_drv
modprobe adv_common_drv
modprobe adv_wdt_drv


#<<!MAKELEDNODE
rm -f /dev/advled
if [ $(cat /proc/devices |awk '$2=="adv_led" {print $1}') ]; then
	mknod /dev/advled c $(cat /proc/devices |awk '$2=="adv_led" {print $1}') 0
fi
#!MAKELEDNODE

#<<!MAKECOMMONNODE
rm -f /dev/advcommon
if [ $(cat /proc/devices |awk '$2=="adv_common" {print $1}') ]; then
	mknod /dev/advcommon c $(cat /proc/devices |awk '$2=="adv_common" {print $1}') 0
fi
#!MAKECOMMONNODE

#<<!MAKEWDTNODE
rm -f /dev/watchdog
if [ $(cat /proc/devices |awk '$2=="adv_watchdog" {print $1}') ]; then
	mknod /dev/watchdog c $(cat /proc/devices |awk '$2=="adv_watchdog" {print $1}') 0
fi
#!MAKEWDTNODE


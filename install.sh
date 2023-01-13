#!/bin/bash

DIR=`readlink -f $0`
BASEDIR=$(dirname $DIR)
VERSION="2.20"
PACKAGE_NAME="advec-${VERSION}"
PLATFORM="linux-amd64"
KERNEL=`uname -r`

echo $VERSION

mkdir -p /usr/src/advantech

# remove old driver
if [ -d /usr/src/advantech/advec* ]; then
	rm -rf /usr/src/advantech/advec*
	echo "Remove old folder. Copy files"
else
	echo "Copy files"
fi

mkdir -p /lib/modules/${KERNEL}/kernel/advantech/
# remove old driver
if [ -f /lib/modules/${KERNEL}/kernel/advantech/adv_ec ]; then
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_ec.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_eeprom_drv.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_hwmon_drv.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_brightness_drv.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_gpio_drv.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_led_drv.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_common_drv.ko
        rm -rf /lib/modules/${KERNEL}/kernel/advantech/adv_wdt_drv.ko
        echo "Remove old drivers from kernel. Copy drivers to kernel"
fi

# copy driver
cp -rf $BASEDIR/${PACKAGE_NAME} /usr/src/advantech

# Build drivers 
cd /usr/src/advantech/${PACKAGE_NAME}/drivers
make


# Build example
cd /usr/src/advantech/${PACKAGE_NAME}/example
make

# Copy drivers to kernel
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/mfd-ec/adv_ec.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/eeprom/adv_eeprom_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/hwmon/adv_hwmon_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/brightness/adv_brightness_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/gpio/adv_gpio_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/led/adv_led_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/common/adv_common_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
cp /usr/src/advantech/${PACKAGE_NAME}/drivers/watchdog/adv_wdt_drv.ko /lib/modules/${KERNEL}/kernel/advantech/
depmod -a

sudo chmod 777 /usr/src/advantech/${PACKAGE_NAME}/AdvLoader.sh
sh /usr/src/advantech/${PACKAGE_NAME}/AdvLoader.sh

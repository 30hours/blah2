#!/bin/bash
echo "This script will add the SDRplay IDs to the local USB database"
echo "and will require sudo permissions. Enter your password if prompted"
echo " "
if [ -f "/var/lib/usbutils/usb.ids" ]; then
	if grep -q SDRplay /var/lib/usbutils/usb.ids; then
		echo "Database already contains SDRplay devices"
		exit 0
	else
		sudo cp -f /var/lib/usbutils/usb.ids /var/lib/usbutils/usb.ids.bak
		echo "cat /usr/local/bin/sdrplay_ids.txt /var/lib/usbutils/usb.ids.bak > /var/lib/usbutils/usb.ids" | sudo bash
		echo "Now when you type lsusb you will see the correct SDRplay device type"
		exit 0
	fi
else
	echo "No USB database found. Exiting..."
	exit 1
fi

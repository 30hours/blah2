#!/bin/sh

VERS="3.07"
MAJVERS="3"

echo "Installing SDRplay RSP API library ${VERS}..."
read -p "Press RETURN to view the license agreement" ret

#more sdrplay_license.txt

while true; do
    echo "Press y and RETURN to accept the license agreement and continue with"
    read -p "the installation, or press n and RETURN to exit the installer [y/n] " yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer y or n";;
    esac
done

ARCH=`uname -m`
OSDIST="Unknown"

if [ -f "/etc/os-release" ]; then
    OSDIST=`sed '1q;d' /etc/os-release`
    echo "DISTRIBUTION ${OSDIST}"
    case "$OSDIST" in
        *Alpine*)
            ARCH="Alpine64"
        ;;
    esac
fi

echo "Architecture: ${ARCH}"
echo "API Version: ${VERS}"

if [ "${ARCH}" != "x86_64" ]; then
    if [ "${ARCH}" != "i386" ]; then
        if [ "${ARCH}" != "i686" ]; then
            if [ "${ARCH}" != "Alpine64" ]; then
                echo "The architecture on this device (${ARCH}) is not currently supported."
                echo "Please contact software@sdrplay.com for details on platform support."
                exit 1
            fi
        fi
    fi
fi

echo "If not installing as root, you will be prompted for your password"
echo "for sudo access to the /usr/local area..."
sudo mkdir -p /usr/local/lib >> /dev/null 2>&1
echo "The rest of the installation will continue with root permission..."

if [ -d "/etc/udev/rules.d" ]; then
	echo -n "Udev rules directory found, adding rules..."
	sudo cp -f 66-mirics.rules /etc/udev/rules.d/66-mirics.rules
	sudo chmod 644 /etc/udev/rules.d/66-mirics.rules
    echo "Done"
else
	echo " "
	echo "ERROR: udev rules directory not found, add udev support and run the"
	echo "installer again. udev support can be added by running..."
	echo "sudo apt install libudev-dev"
	echo " "
	exit 1
fi

TYPE="LOCAL"
if [ -d "/usr/local/lib" ]; then
    if [ -d "/usr/local/include" ]; then
        if [ -d "/usr/local/bin" ]; then
            echo "Installing files into /usr/local/... (/lib, /bin, /include)"
            INSTALLLIBDIR="/usr/local/lib"
            INSTALLINCDIR="/usr/local/include"
            INSTALLBINDIR="/usr/local/bin"
        else
            TYPE="USR"
        fi
    else
        TYPE="USR"
    fi
else
    TYPE="USR"
fi

if [ "${TYPE}" != "LOCAL" ]; then
    echo "Installing files into /usr/... (/lib, /bin, /include)"
    INSTALLLIBDIR="/usr/lib"
    INSTALLINCDIR="/usr/include"
    INSTALLBINDIR="/usr/bin"
fi

echo -n "Installing ${INSTALLLIBDIR}/libsdrplay_api.so.${VERS}..."
sudo rm -f ${INSTALLLIBDIR}/libsdrplay_api.so.${VERS}
sudo cp -f ${ARCH}/libsdrplay_api.so.${VERS} ${INSTALLLIBDIR}/.
sudo chmod 644 ${INSTALLLIBDIR}/libsdrplay_api.so.${VERS}
sudo rm -f ${INSTALLLIBDIR}/libsdrplay_api.so.${MAJVERS}
sudo ln -s ${INSTALLLIBDIR}/libsdrplay_api.so.${VERS} ${INSTALLLIBDIR}/libsdrplay_api.so.${MAJVERS}
sudo rm -f ${INSTALLLIBDIR}/libsdrplay_api.so
sudo ln -s ${INSTALLLIBDIR}/libsdrplay_api.so.${MAJVERS} ${INSTALLLIBDIR}/libsdrplay_api.so
echo "Done"

echo -n "Installing header files in ${INSTALLINCDIR}..."
sudo cp -f inc/sdrplay_api*.h ${INSTALLINCDIR}/.
sudo chmod 644 ${INSTALLINCDIR}/sdrplay_api*.h
echo "Done"

echo -n "Installing API Service in ${INSTALLBINDIR}..."
sudo cp -f ${ARCH}/sdrplay_apiService ${INSTALLBINDIR}/sdrplay_apiService
sudo chmod 755 ${INSTALLBINDIR}/sdrplay_apiService
echo "Done"

echo -n "Installing Service scripts and starting daemon..."
if [ -d "/etc/systemd/system" ]; then
    SRVTYPE="systemd"
    if [ "${TYPE}" != "LOCAL" ]; then
        sudo cp -f scripts/sdrplay.service.usr /etc/systemd/system/sdrplay.service
    else
        sudo cp -f scripts/sdrplay.service.local /etc/systemd/system/sdrplay.service
    fi
    sudo chmod 644 /etc/systemd/system/sdrplay.service
    sudo systemctl start sdrplay
    sudo systemctl enable sdrplay
    sudo systemctl daemon-reload
    sudo systemctl restart sdrplay
else
    SRVTYPE="initd"
    if [ "${TYPE}" != "LOCAL" ]; then
        sudo cp -f scripts/sdrplayService_usr /etc/init.d/sdrplayService
    else
        sudo cp -f scripts/sdrplayService_local /etc/init.d/sdrplayService
    fi
    sudo chmod 755 /etc/init.d/sdrplayService
    sudo update-rc.d sdrplayService defaults
    sudo service sdrplayService start
fi

sudo ldconfig

echo "Done"

echo " "
echo "Service has been installed and started. This device should be rebooted"
echo "before the service is used. After the installation script finishes,"
echo "reboot this device."

echo " "
echo "To start and stop the API service, use the following commands..."
echo " "
if [ "${SRVTYPE}" != "systemd" ]; then
    echo "sudo service sdrplayService start"
    echo "sudo service sdrplayService stop"
else
    echo "sudo systemctl start sdrplay"
    echo "sudo systemctl stop sdrplay"
fi
echo " "
echo "SDRplay API ${VERS} Installation Finished"
echo " "
while true; do
    echo "Would you like to add SDRplay USB IDs to the local database for easier"
    read -p "identification in applications such as lsusb? [y/n] " yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer y or n";;
    esac
done
sudo cp scripts/sdrplay_usbids.sh ${INSTALLBINDIR}/.
sudo chmod 755 ${INSTALLBINDIR}/sdrplay_usbids.sh
sudo cp scripts/sdrplay_ids.txt ${INSTALLBINDIR}/.
sudo chmod 644 ${INSTALLBINDIR}/sdrplay_ids.txt
${INSTALLBINDIR}/sdrplay_usbids.sh

echo "SDRplay IDs added. Try typing lsusb with an SDRplay device connected."
echo "If the USB IDs get updated from the central reprository, just type"
echo "the following command to add the SDRplay devices again..."
echo " "
echo "sdrplay_usbids.sh"
echo " "
echo "Installation Finished"

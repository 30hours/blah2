#!/bin/bash
PATH=/bin:$PATH
VERS="3.15"
MAJVERS="3"
YUMTOOL=$( command -v yum )
APTTOOL=$( command -v apt )
APKTOOL=$( command -v apk )
PACTOOL=$( command -v pacman )
ZYPTOOL=$( command -v zypper )
REBOOT="n"
echo "SDRplay API ${VERS} Installation"
echo "============================="
echo " "
#     0--------1---------2---------3---------4---------5---------6---------7---------8
echo "This installation requires to either be run as root, or it will request root"
echo "access through the sudo program. This is required to install the daemon into"
echo "the system files."
echo " "

read -p "Press RETURN to view the license agreement" ret
more -d sdrplay_license.txt
while true; do
    echo "Press y and RETURN to accept the license agreement and continue with"
    read -p "the installation, or press n and RETURN to exit the installer [y/n] " yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer y or n";;
    esac
done

echo " "
echo "A copy of the license agreement can be found here: ${HOME}/sdrplay_license.txt"
cp sdrplay_license.txt ${HOME}/.
chmod 644 ${HOME}/sdrplay_license.txt
echo " "

ARCH=$(uname -m|sed -e 's/x86_64/64/' -e 's/aarch64/64/' -e 's/arm64/64/' -e 's/i.86/32/')
INIT=$(file -L /sbin/init|sed -e 's/^.* \(32\|64\)-bit.*$/\1/')
COMPILER=$(getconf LONG_BIT)
ARCHM=$(uname -m)
INSTALLARCH=$(dpkg --print-architecture)

if [ "${ARCHM}" = "aarch64" ] || [ "${ARCHM}" = "arm64" ]; then
    ARCHM="arm64"
fi

echo " "
echo "Architecture reported as being $ARCH bit"
echo "System reports $INIT bit files found"
echo "System is also setup to produce $COMPILER bit files"
echo "Architecture reports machine as being $ARCHM compliant"
echo " "

if [ "${INSTALLARCH}" != "amd64" ]; then
    if [ "${INSTALLARCH}" != "arm64" ]; then
        if [ "${INSTALLARCH}" != "armhf" ]; then
#                 0--------1---------2---------3---------4---------5---------6---------7---------8
            echo " "
            echo "This installer does not support the architecture you are installing on."
            echo "Please check https://www.sdrplay.com/api to see if there's an installer"
            echo "for your architecture."
            exit 1
        fi
    fi
fi

#     0--------1---------2---------3---------4---------5---------6---------7---------8
echo " "
echo "Checking for root permissions. You may be prompted for your password..."
echo " "
sudo ls >> /dev/null 2>&1
echo "The rest of the installation will continue with root permission..."
echo " "


echo " "
echo "This API requires the following system dependencies..."
echo "libusb and lidudev"
echo "If the installer cannot detect the presence of these, you will"
echo "have the option to continue and you will need to install them"
echo " "
echo "Checking for package tools..."

LIBUSBPKG="libusb-1.0"
UDEVPKG="libudev1"
CHECKDEP="y"

if [ "${YUMTOOL}" != "" ]; then
    PKTOOL="yum install"
    PKSEARCH="yum list"
    echo "yum found"
    LIBUSBPKG="libusb1"
    ASOUND2PKG="alsa-lib"
    UUIDPKG="libuuid"
    UDEVPKG="systemd-libs"
elif [ "${APTTOOL}" != "" ]; then
    PKTOOL="apt install"
    PKSEARCH="apt-cache search"
    echo "apt found"
elif [ "${APKTOOL}" != "" ]; then
    PKTOOL="apk add"
    PKSEARCH="apk search -v"
    echo "apk found"
elif [ "${PACTOOL}" != "" ]; then
    PKTOOL="pacman -S"
    PKSEARCH="pacman -Qs"
    echo "pacman found"
    LIBUSBPKG="libusb"
    ASOUND2PKG="alsa-lib"
    UUIDPKG="util-linux-libs"
    UDEVPKG="systemd-libs"
elif [ "${ZYPTOOL}" != "" ]; then
    PKTOOL="zypper in"
    PKSEARCH="zypper se -i"
    LIBUSBPKG="libusb-1_0"
    echo "zypper found"
else
    CHECKDEP="n"
    echo "no package tool found, dependency search will be skipped."
fi

echo " "
echo "Checking for packages..."
LIBLIST=()
LIBLIST[0]=${LIBUSBPKG}
LIBLIST[1]=${UDEVPKG}

if [ "${CHECKDEP}" != "n" ]; then
    for lib in "${LIBLIST[@]}"
    do
        ${PKSEARCH} ${lib} > /tmp/sdr.$$
        if grep -q ${lib} /tmp/sdr.$$; then
            echo "${lib} found"
        else
            echo "${lib} not found. Install using: sudo ${PKTOOL} ${lib}"
        fi
        rm /tmp/sdr.$$
    done
fi

if [ -d "/etc/systemd/system" ]; then
    SRVTYPE="systemd"
else
    SRVTYPE="initd"
fi

if [ -d "/etc/udev" ]; then
    echo -n "Udev directory found, adding rules..."
    sudo bash -c 'cat > /etc/udev/rules.d/66-sdrplay.rules' << EOF
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="2500",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3000",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3010",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3020",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3030",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3050",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3060",MODE:="0666"
EOF
    sudo chmod 644 /etc/udev/rules.d/66-sdrplay.rules
    sudo rm -f /etc/udev/rules.d/66-mirics.rules
    echo "rules added"
    REBOOT="y"

    if [ -d "/etc/udev/hwdb.d" ]; then
        echo -n "Adding SDRplay devices to the local hardware database..."
        sudo bash -c 'cat > /etc/udev/hwdb.d/20-sdrplay.hwdb' << EOF
usb:v1DF7*
 ID_VENDOR_FROM_DATABASE=SDRplay

usb:v1DF7p2500*
 ID_MODEL_FROM_DATABASE=RSP1

usb:v1DF7p3000*
 ID_MODEL_FROM_DATABASE=RSP1A

usb:v1DF7p3010*
 ID_MODEL_FROM_DATABASE=RSP2/RSP2pro

usb:v1DF7p3020*
 ID_MODEL_FROM_DATABASE=RSPduo

usb:v1DF7p3030*
 ID_MODEL_FROM_DATABASE=RSPdx

usb:v1DF7p3050*
 ID_MODEL_FROM_DATABASE=RSP1B

usb:v1DF7p3060*
 ID_MODEL_FROM_DATABASE=RSPdxR2
EOF
        sudo chmod 644 /etc/udev/hwdb.d/20-sdrplay.hwdb
        sudo systemd-hwdb update
        sudo udevadm trigger
        if [ "${SRVTYPE}" != "initd" ]; then
            sudo systemctl restart udev
        else
            sudo service udev restart
        fi
        echo "Done"
    fi
else
    echo " "
    echo "ERROR: udev rules directory not found, add udev support and run the"
    echo "installer script again. udev support can be added by running..."
    if [ "${INSTALLARCH}" == "ppc64le" ]; then
        echo "sudo ${PKG_TOOL} eudev"
    else
        echo "sudo ${PKG_TOOL} libudev-dev"
    fi
    echo " "
    exit 1
fi

if [ -f "/var/lib/usbutils/usb.ids" ]; then
    if grep -q SDRplay /var/lib/usbutils/usb.ids; then
        echo "SDRplay devices found in the USB database"
        if grep -q RSP1B /var/lib/usbutils/usb.ids; then
            echo "RSP1B already in the local USB database, continuing..."
        else
            sudo cp /var/lib/usbutils/usb.ids /var/lib/usbutils/usb.ids.bak
            sudo bash -c "awk '/^1e/ && !x {print \"	3050  RSP1B\"; x=1} 1' /var/lib/usbutils/usb.ids.bak > /var/lib/usbutils/usb.ids"
            echo "Added RSP1B to USB name database"
        fi
        if grep -q RSPdxR2 /var/lib/usbutils/usb.ids; then
            echo "RSPdxR2 already in the local USB database, continuing..."
        else
            sudo cp /var/lib/usbutils/usb.ids /var/lib/usbutils/usb.ids.bak
            sudo bash -c "awk '/^1e/ && !x {print \"	3060  RSPdxR2\"; x=1} 1' /var/lib/usbutils/usb.ids.bak > /var/lib/usbutils/usb.ids"
            echo "Added RSPdxR2 to USB name database"
        fi
    else
        sudo cp /var/lib/usbutils/usb.ids /var/lib/usbutils/usb.ids.bak
        sudo bash -c "awk '/^1e/ && !x {print \"1df7  SDRplay\n	2500  RSP1\n	3000  RSP1A\n	3010  RSP2/RSP2pro\n	3020  RSPduo\n	3030  RSPdx\n	3050  RSP1B\n	3060  RSPdxR2\"; x=1} 1' /var/lib/usbutils/usb.ids.bak > /var/lib/usbutils/usb.ids"
        echo "SDRplay devices added to the local USB name database"
    fi
else
    echo "USB name database not found, continuing..."
fi

echo " "
locservice="/opt/sdrplay_api"
locheader="/usr/local/include"
loclib="/usr/local/lib"
locscripts="/etc/systemd/system"
DAEMON_SYS="SystemD"

if [ ! -d ${locscripts} ]; then
locscripts="/etc/init.d"
DAEMON_SYS="Init"
fi

#     0--------1---------2---------3---------4---------5---------6---------7---------8
echo "Installing API files, the default locations are..."
echo "API service : ${locservice}"
echo "API header files : ${locheader}"
echo "API shared library : ${loclib}"
echo "Daemon start scripts : ${locscripts}"
echo "Daemon start system : ${DAEMON_SYS}"
echo " "

#         0--------1---------2---------3---------4---------5---------6---------7---------8
while true; do
    echo "To continue the installation with these defaults press y and RETURN"
    read -p "or press n and RETURN to change them [y/n] " yn
    case $yn in
        [Yy]* ) change="n";break;;
        [Nn]* ) change="y";break;;
        * ) echo "Please answer y or n";;
    esac
done

if [ "${change}" == "y" ]; then
    echo "Changing default locations..."
    read -p "API service location [${locservice}]: " newloc
    if [ "${newloc}" != "" ]; then
        locservice=${newloc}
    fi
    read -p "API header files location [${locheader}]: " newloc
    if [ "${newloc}" != "" ]; then
        locheader=${newloc}
    fi
    read -p "API shared library location [${loclib}]: " newloc
    if [ "${newloc}" != "" ]; then
        loclib=${newloc}
    fi

    echo "API service : ${locservice}"
    echo "API header files : ${locheader}"
    echo "API shared library : ${loclib}"
    while true; do
        read -p "Please confirm these are correct [y/n] " yn
        case $yn in
            [Yy]* ) break;;
            [Nn]* ) echo "paths not confirmed. Exiting...";exit 1;;
            * ) echo "Please answer y or n";;
        esac
    done
fi

sudo mkdir -p -m 755 ${locservice} >> /dev/null 2>&1
sudo mkdir -p -m 755 ${locheader} >> /dev/null 2>&1
sudo mkdir -p -m 755 ${loclib} >> /dev/null 2>&1

echo -n "Cleaning old API files..."
sudo rm -f /usr/local/lib/libsdrplay_api.so*
echo "Done."

echo -n "Installing ${loclib}/libsdrplay_api.so.${VERS}..."
sudo rm -f ${loclib}/libsdrplay_api.so.${VERS}
sudo cp -f ${INSTALLARCH}/libsdrplay_api.so.${VERS} ${loclib}/.
sudo chmod 644 ${loclib}/libsdrplay_api.so.${VERS}
sudo rm -f ${loclib}/libsdrplay_api.so.${MAJVERS}
sudo ln -s ${loclib}/libsdrplay_api.so.${VERS} ${loclib}/libsdrplay_api.so.${MAJVERS}
sudo rm -f ${loclib}/libsdrplay_api.so
sudo ln -s ${loclib}/libsdrplay_api.so.${MAJVERS} ${loclib}/libsdrplay_api.so
echo "Done"

echo -n "Installing header files in ${locheader}..."
sudo cp -f inc/sdrplay_api*.h ${locheader}/.
sudo chmod 644 ${locheader}/sdrplay_api*.h
echo "Done"

echo -n "Installing API Service in ${locservice}..."
sudo cp -f ${INSTALLARCH}/sdrplay_apiService ${locservice}/sdrplay_apiService
sudo chmod 755 ${locservice}/sdrplay_apiService
echo "Done"

echo -n "Installing Service scripts and starting daemon..."
if [ -d "/etc/systemd/system" ]; then
    SRVTYPE="systemd"
    if [ -f "/etc/systemd/system/sdrplay.service" ]; then
        sudo systemctl stop sdrplay
        sudo systemctl disable sdrplay
    fi
    sudo bash -c 'cat > /etc/systemd/system/sdrplay.service' << EOF
[Unit]
Description=SDRplay API Service
After=network.target
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=on-failure
RestartSec=1
User=root
ExecStart=${locservice}/sdrplay_apiService

[Install]
WantedBy=multi-user.target
EOF

    sudo chmod 644 /etc/systemd/system/sdrplay.service
    sudo systemctl enable sdrplay
    sudo systemctl start sdrplay
else
    SRVTYPE="initd"
    if [ -f "/etc/init.d/sdrplayService" ]; then
        sudo service sdrplayService stop
    fi
    sudo bash -c 'cat > /etc/init.d/sdrplayService' << EOF
#!/bin/sh

### BEGIN INIT INFO
# Provides:          sdrplayServicedaemon
# Required-Start:    \$local_fs \$network \$syslog
# Required-Stop:     \$local_fs \$network \$syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: sdrplay_apiService
# Description:       Service for SDRplay API
### END INIT INFO

NAME="sdrplay_apiService"
PATH="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"
APPDIR="/"
APPBIN="${locservice}/sdrplay_apiService"
APPARGS=""

# Include functions
set -e
if test -f /lib/lsb/init-functions; then
    . /lib/lsb/init-functions
fi

start() {
  printf "Starting '\$NAME'... "
  start-stop-daemon --start --background --make-pidfile --pidfile /var/run/\$NAME.pid --chdir "\$APPDIR" --exec "\$APPBIN" -- \$APPARGS || true
  printf "done\n"
}

#We need this function to ensure the whole process tree will be killed
killtree() {
    local _pid=\$1
    local _sig=\${2-TERM}
    for _child in \$(ps -o pid --no-headers --ppid \${_pid}); do
        killtree \${_child} \${_sig}
    done
    kill -\${_sig} \${_pid}
}

stop() {
  printf "Stopping '\$NAME'... "
  [ -z \`cat /var/run/\$NAME.pid 2>/dev/null\` ] || \\
  while test -d /proc/\$(cat /var/run/\$NAME.pid); do
    killtree \$(cat /var/run/\$NAME.pid) 15
    sleep 0.5
  done
  [ -z \`cat /var/run/\$NAME.pid 2>/dev/null\` ] || rm /var/run/\$NAME.pid
  printf "done\n"
}

status() {
  status_of_proc -p /var/run/\$NAME.pid "" \$NAME && exit 0 || exit \$?
}

case "\$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    stop
    start
    ;;
  status)
    status
    ;;
  *)
    echo "Usage: \$NAME {start|stop|restart|status}" >&2
    exit 1
    ;;
esac

exit 0
EOF

    sudo chmod 755 /etc/init.d/sdrplayService
    UPRC=$( command -v update-rc.d )
    if [ "${UPRC}" != "" ]; then
        sudo update-rc.d sdrplayService defaults
    fi
    sudo service sdrplayService start
fi

sudo ldconfig

echo "Done"

#     0--------1---------2---------3---------4---------5---------6---------7---------8
echo " "
echo "The API Service has been installed and started. After the installation has"
echo "finished, please reboot this device."

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
echo "If supported on your system, lsusb will now show the RSP name"
echo " "
echo "SDRplay API ${VERS} Installation Finished"
echo " "

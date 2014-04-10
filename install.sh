#!/bin/bash

#Get the directory where the script resides
install_base="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function usage {
	echo "Usage ./install.sh Options"
    echo " -i install"
    echo " -r set udev rules (requires root access)"
    echo " -c clean"
    echo " -d directory clean (remove driver directories for re-download)"
    echo " -h help (This page)"
}

i_called=0
r_called=0
c_called=0
d_called=0

while getopts ":ircdh" opt; do

  case $opt in

    "i" )
    	if [ $i_called -gt 0 ]; then
    		continue
    	fi
    	i_called=1
    	
        #Get and install usblib library
        cd $install_base
        if [ ! -d $install_base/libusb-1.0.9 ]; then
        	wget http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/libusb-1.0.9.tar.bz2
        	tar xvjf libusb-1.0.9.tar.bz2
        	rm libusb-1.0.9.tar.bz2
        fi
        cd $install_base/libusb-1.0.9
        ./configure --prefix=$install_base/libusb-1.0.9
        make
        make install
        export LD_LIBRARY_PATH=$install_base/libusb-1.0.9/lib/:${LD_LIBRARY_PATH}

        #Get and install libsub library
        cd $install_base
        if [ ! -d $install_base/SUB-20-snap-110319 ]; then
        	wget http://www.xdimax.net/download/SUB-20-snap-110319.tgz
        	tar xvzf SUB-20-snap-110319.tgz
        	rm SUB-20-snap-110319.tgz
        fi
        cd $install_base/SUB-20-snap-110319/lib
        #We must fix the hardcode in the Makefile to the libusb include
        sed -i "s|\(CFLAGS += -DLIBUSB_1_0 -I\).*|\1$install_base/libusb-1.0.9/include/libusb-1.0|" Makefile
        echo ${LD_LIBRARY_PATH}
        make
        export LD_LIBRARY_PATH=$install_base/SUB-20-snap-110319/lib:${LD_LIBRARY_PATH}
        cd $install_base

        #install test code
        #Set directories to libraries
        sed -i "s|\(SUB20BASE=\).*|\1$install_base/SUB-20-snap-110319/|" $install_base/Makefile
        sed -i "s|\(LIBUSBBASE=\).*|\1$install_base/libusb-1.0.9/|" $install_base/Makefile
        make
        
        if [ $? -gt 0 ]; then
        	echo "!!!!!!! INSTALL FAILED !!!!!!!"
        fi

        echo 'if [[ ! ${LD_LIBRARY_PATH} == '"*\"$install_base/libusb-1.0.9/lib\"* ]]; then" > usblibcfg.sh
        echo "    export LD_LIBRARY_PATH=$install_base/libusb-1.0.9/lib:"'${LD_LIBRARY_PATH}' >> usblibcfg.sh
        echo "fi" >> usblibcfg.sh
        echo "" >> usblibcfg.sh
        echo 'if [[ ! ${LD_LIBRARY_PATH} == '"*\"$install_base/SUB-20-snap-110319/lib\"* ]]; then" >> usblibcfg.sh
        echo "    export LD_LIBRARY_PATH=$install_base/SUB-20-snap-110319/lib:"'${LD_LIBRARY_PATH}' >> usblibcfg.sh
        echo "fi" >> usblibcfg.sh

        echo
        echo "|=====================================|"
        echo "| run the following once per terminal |"
        echo "| \"source usblibcfg.sh\"               |"
        echo "|=====================================|"
        echo
        ;;
        
    "r" )
    	if [ $r_called -gt 0 ]; then
    		continue
    	fi
    	r_called=1
    	
    	#Create rules file to set sub20 privlages
    	echo SYSFS{idVendor}=="04d8", MODE="666" > /etc/udev/rules.d/53-sub20.rules
    	echo ACTION=="add", SYSFS{idVendor}=="ffc3", MODE="666", RUN+="/usr/local/sbin/dftdrvdtch %s{busnum} %s{devnum}" >> /etc/udev/rules.d/53-sub20.rules
    	echo SUBSYSTEM=="usb_endpoint", ACTION=="add", OPTIONS="ignore_device" >> /etc/udev/rules.d/53-sub20.rules
    	echo SUBSYSTEM=="usb_endpoint", ACTION=="remove", OPTIONS="ignore_device" >> /etc/udev/rules.d/53-sub20.rules
    	
        /sbin/udevcontrol reload_rules
    	;;
    
    "c" )
    	if [ $c_called -gt 0 ]; then
    		continue
    	fi
    	c_called=1
    	
    	if [ -d $install_base/libusb-1.0.9 ]; then
    		cd $install_base/libusb-1.0.9
    		make clean
    	fi
    	if [ -d $install_base/libusb-1.0.9 ]; then
	    	cd $install_base/SUB-20-snap-110319/lib
    		make clean
    	fi
    	cd $install_base
    	make clean
    	if [ -e $install_base/usblibcfg.sh ]; then
    		rm $install_base/usblibcfg.sh
    	fi
    	;;
    	
    "d" )
    	if [ $d_called -gt 0 ]; then
    		continue
    	fi
    	d_called=1
    	
    	if [ -d $install_base/libusb-1.0.9 ]; then
    		rm -rf $install_base/libusb-1.0.9
    	fi
    	if [ -d $install_base/libusb-1.0.9 ]; then
	    	rm -rf $install_base/SUB-20-snap-110319/lib
    	fi
    	cd $install_base
    	make clean
    	if [ -e $install_base/usblibcfg.sh ]; then
    		rm $install_base/usblibcfg.sh
    	fi
    	;;
    
    \? | h)
    	if [ $i_called -gt 0 ] || [ $c_called -gt 0 ] || [ $d_called -gt 0 ] || [ $r_called -gt 0 ]; then
    		continue
    	fi
    	
    	usage
    	exit 0
    	;;
    
    esac
    
done

if [ $# -lt 1 ] ; then
	usage
fi

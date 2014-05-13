#! /bin/sh
# /etc/init.d/runServer.sh
#

# Some things that run always
touch /var/lock/runServer.sh

# Carry out specific functions when asked to by the system
case "$1" in
    start)
	/home/hcal/hcalUHTR/tool/moduleCheckSUB20/uHTR_PowerMezz_Server.exe 1338
	;;
    stop)
	echo "Stopping script blah"
	echo "Could do more here"
	;;
    *)
	echo "Usage: /etc/init.d/runServer.sh {start|stop}"
	exit 1
	;;
esac

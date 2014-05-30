#! /bin/sh
# /etc/init.d/runServer.sh

# Some things that run always
touch /var/lock/runServer.sh

prog="runServer.sh"

# Carry out specific functions when asked to by the system

start() {
    echo  "Starting $prog"
    modprobe -r i2c_bcm2708
    modprobe i2c_bcm2708 baudrate=100000
    /home/hcal/hcalUHTR/tool/moduleCheckSUB20/uHTR_PowerMezz_Server.exe 1338 1> /tmp/mezzlog 2> /tmp/mezzerr &
}

stop() {
    echo "Stopping $prog"

    killall uHTR_PowerMezz_Server.exe

}

case "$1" in
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
    *)
        echo "Usage: /etc/init.d/runServer.sh {start|stop|restart}"
        exit 1
        ;;
esac

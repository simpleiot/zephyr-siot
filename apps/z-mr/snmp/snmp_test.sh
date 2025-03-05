#!/bin/bash
#
# This is a script that will test 2 things:
#
# 1. It will send a polling message to an Z-MR and show the answer
# 2. It will simulate a switch between source A/B and show the SNMP trap
#
# Usage: snmp_test.sh <IP-address> <USB device>
#        snmp_test.sh    "Z-MR"    /dev/ttyUSB0
#        snmp_test.sh 192.168.2.13 /dev/ttyUSB0

# IP-address or name of the ESP32 device
IP_ADDRESS="Z-MR"
USB_DEVICE="/dev/ttyUSB0"

function quick_sleep {
	read -t 0.5
}

if [ $# -ge 1 ]; then
    echo IP-address $1
    IP_ADDRESS=$1
else
    echo IP_ADDRESS=$IP_ADDRESS
fi

if [ $# -ge 2 ]; then
    echo USB device $2
    USB_DEVICE=$2
else
    echo USB_DEVICE=$USB_DEVICE
fi

echo
echo Test-1: polling a Z-MR at ${IP_ADDRESS}
echo =======================================

snmpget -v2c -c public ${IP_ADDRESS} 1.3.6.1.4.1.62530.2.2.4.0

# Get my Linux IP address
IFS=', ' read -r -a ip_array <<< $( hostname -I )
# Get the first IP found
MY_IP="${ip_array[0]}"

echo MY_IP=${MY_IP}

echo p snmpServer 0 STR ${MY_IP} > ${USB_DEVICE}

quick_sleep

sudo echo

# Start the SNMP-daemon for a while.
sudo snmptrapd -f -Lo &

# remember the pid of the daemon
PID=$!

echo
echo Test-2: Sending source A/B switching
echo =======================================

quick_sleep
echo p atsA 0 INT 1 > ${USB_DEVICE}
echo p atsB 0 INT 1 > ${USB_DEVICE}
sleep 1

echo kill $PID
kill $PID

# wait for the termination
sleep 1

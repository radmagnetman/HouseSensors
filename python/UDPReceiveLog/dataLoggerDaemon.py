#!/usr/bin/env python
# Setup

import socket
import datetime
import operator
import os
from DataLoggingClass import DataLogger

file = open("/home/pi/Documents/python/UDPReceiveLog/didThisWork.txt","a")
file.write("Hello?")
file.close()

#import os.path
#os.path.isfile("dataLogging")

#os.system('clear')

print("Logging service")

# Init variables and UDP socket
sensorList = []

UDP_IP = "" #Which is my local ip for my computer
UDP_PORT = 53394
sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

# When the second clock rolls back to zero, write
# data to file.
lastLoggedSec = -1
while True:
    # Get UDP data
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
    now = datetime.datetime.now()

    # Parse data
    dataStrList = data.split()
    timeStampStr = now.strftime("%H:%M:%S ")
    currentSecond = now.second
    boardCode = dataStrList[0]
    locationCode = dataStrList[1]
    sensorDataType = dataStrList[2]
    sensorDataValue = dataStrList[3]
    uniqueID = dataStrList[0]+dataStrList[1]+dataStrList[2]

    # Check to see if the last sensor read is in the current list
    found = False
    for sensors in sensorList:
        if uniqueID in sensors.name:
            found = True
            sensors.addDataValue(sensorDataValue)

    # If sensor isn't in list, add it
    if found == False:
        sensorList.append(DataLogger(uniqueID,boardCode,locationCode,sensorDataType,sensorDataValue))

    # If minute has passed, write data to file and flush sensor list
    if currentSecond < lastLoggedSec:
        print("Writing data to file")
        for sensors in sensorList:
            minSinceMidnight = now.hour*60+now.minute
            print("{},{},{},{},{:.2f}".format(minSinceMidnight, \
            sensors.boardCode,sensors.locationCode,sensors.dataType,\
            sensors.returnAverageData()))
            file = open("/home/pi/Documents/python/UDPReceiveLog/testfileservice.txt","a")
            file.write("{},{},{},{},{:.2f}\n".format(minSinceMidnight, \
            sensors.boardCode,sensors.locationCode,sensors.dataType,\
            sensors.returnAverageData()))
            file.close()
            sensors.removeSensor
        del sensorList
        sensorList = []


    lastLoggedSec = currentSecond

import socket
import datetime
import operator
import os
from DataLoggingClass import DataLogger

os.system('clear')

print("123Testing UDP writing.")

sensorList = []

UDP_IP = "" #Which is my local ip for my computer
UDP_PORT = 53394
sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

#while True:
# Check UDP stack
c = 100
lastLoggedSec = -1
while c> 0:
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
    now = datetime.datetime.now()
    #print (now.strftime("%H:%M:%S ")),
    #print (data)

    dataStrList = data.split()
    timeStampStr = now.strftime("%H:%M:%S ")
    currentSecond = now.second
    boardCode = dataStrList[0]
    locationCode = dataStrList[1]
    sensorDataType = dataStrList[2]
    sensorDataValue = dataStrList[3]
    uniqueID = dataStrList[0]+dataStrList[1]+dataStrList[2]

    found = False
    for sensors in sensorList:
        if uniqueID in sensors.name:
            found = True
            sensors.addDataValue(sensorDataValue)

    if found == False:
        sensorList.append(DataLogger(uniqueID,sensorDataType,sensorDataValue))

    if currentSecond < lastLoggedSec:
        print("Writing data to file")
        for sensors in sensorList:
            print("At {}:{} sensor {} has an average {} of {}".format(now.hour, \
            now.minute,sensors.name,sensors.dataType,sensors.returnAverageData()))
            minSinceMidnight = now.hour*60+now.minute
            sensors.removeSensor
        del sensorList
        sensorList = []


    lastLoggedSec = currentSecond

    #c-=1

print
#print("----")
#for s in sensorList:
#    s.displayDataValues()
#    print
#print("----")

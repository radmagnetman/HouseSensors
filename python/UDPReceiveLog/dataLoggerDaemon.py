#!/usr/bin/env python
# Setup
print("Logging service")
import socket
import datetime
import operator
import os
import sys
#import configparser
from DataLoggingClass import DataLogger
###########################
import logging
import logging.handlers
import argparse
import time  # this is only being used as part of the example

# Deafults
LOG_FILENAME = "/tmp/myservice.log"
LOG_LEVEL = logging.INFO  # Could be e.g. "DEBUG" or "WARNING"

# Define and parse command line arguments
parser = argparse.ArgumentParser(description="My simple Python service")
parser.add_argument("-l", "--log", help="file to write log to (default '" + LOG_FILENAME + "')")

# If the log file is specified on the command line then override the default
args = parser.parse_args()
if args.log:
        LOG_FILENAME = args.log
# Configure logging to log to a file, making a new file at midnight and keeping the last 3 day's data
# Give the logger a unique name (good practice)
logger = logging.getLogger(__name__)
# Set the log level to LOG_LEVEL
logger.setLevel(LOG_LEVEL)
# Make a handler that writes to a file, making a new file at midnight and keeping 3 backups
handler = logging.handlers.TimedRotatingFileHandler(LOG_FILENAME, when="midnight", backupCount=3)
# Format each log message like this
formatter = logging.Formatter('%(asctime)s %(levelname)-8s %(message)s')
# Attach the formatter to the handler
handler.setFormatter(formatter)
# Attach the handler to the logger
logger.addHandler(handler)

# Make a class we can use to capture stdout and sterr in the log
class MyLogger(object):
        def __init__(self, logger, level):

                self.logger = logger
                self.level = level

        def write(self, message):
                # Only log if there is a message (not just a new line)
                if message.rstrip() != "":
                        self.logger.log(self.level, message.rstrip())

# Replace stdout with logging to file at INFO level
sys.stdout = MyLogger(logger, logging.INFO)
# Replace stderr with logging to file at ERROR level
sys.stderr = MyLogger(logger, logging.ERROR)
###########################
#settingPath = "/etc/opt/"
#settingFile = settingPath + "udpLoggerDaemon.ini"
#config = configparser.ConfigParser()
#config.read(settingFile)
#s = config['SETTINGS']
#logPath = s['logPath']
logPath = "/home/pi/Documents/loggerData/"
now = datetime.datetime.now()
dateFileName = "{}{:02}{:02}.log".format(now.year,now.month,now.day)
print(logPath+dateFileName)

sys.stdout.write("I should see this in the service")

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
        print("Writing data to file: "+logPath+dateFileName)
        for sensors in sensorList:
            minSinceMidnight = now.hour*60+now.minute
            print("{},{},{},{},{:.2f}".format(minSinceMidnight, \
            sensors.boardCode,sensors.locationCode,sensors.dataType,\
            sensors.returnAverageData()))
            #file = open("/home/pi/Documents/python/UDPReceiveLog/testfileservice.txt","a")
            file = open(logPath+dateFileName,"a")
            file.write("{},{},{},{},{:.2f}\n".format(minSinceMidnight, \
            sensors.boardCode,sensors.locationCode,sensors.dataType,\
            sensors.returnAverageData()))
            file.close()
            sensors.removeSensor
        del sensorList
        sensorList = []


    lastLoggedSec = currentSecond

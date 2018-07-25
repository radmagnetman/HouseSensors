class DataLogger:
    numSensors = 0

    def __init__(self,name,boardCode,locationCode,dataType,dataVal):
        self.name = name
        self.boardCode = boardCode
        self.locationCode = locationCode
        self.dataType = dataType
        self.dataVal = []
        self.dataVal.append(float(dataVal))

        print("Initializing {} with {} sensor".format(self.name,dataType))
        DataLogger.numSensors += 1

    def sayHi(self):
        print("{} says 'I exist! There are a total of {:d} sensors.'".format(self.name,DataLogger.numSensors))

    def addDataValue(self,dataVal):
        self.dataVal.append(float(dataVal))

    def displayDataValues(self):
        print("Values stored for {} are".format(self.name))
        for val in self.dataVal:
            print("{}".format(val))
        print("With an average of {}.".format(sum(self.dataVal)/len(self.dataVal)))

    def returnAverageData(self):
        return sum(self.dataVal)/len(self.dataVal)

    def removeSensor(self):
        if DataLogger.numSensors == 0:
            print("There are no sensors to remove")
        elif DataLogger.numSensors < 0:
            print("You got BIG problems")
        else:
            print("Removing {}.".format(self.name))
            print("There are still {:d} sensors.".format(DataLogger.numSensors-1))

        DataLogger.numSensors -= 1

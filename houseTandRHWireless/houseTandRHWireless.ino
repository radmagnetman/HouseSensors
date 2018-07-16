//**************************************************
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include "SparkFun_Si7021_Breakout_Library.h"

const char* verStr = "0.9";
const char* testing = "123"; // I'll be honest, I don't understand why
                             // this has to be here. Without it the 
                             // verStr displayed in the HELP command
                             // shows 'ing'. Likely have a memory
                             // issue :( 
//**************************************************
// Flags to signal data type to send
#define sendT 1
#define sendRH 2
#define sendP 3

// EEPROM addresses
#define eepromAddrConfigByte1 0 // Config byte 1
#define eepromAddrConfigByte2 1 // Config byte 2 (unused)
#define eepromAddrMyIP        2 // Last byte of my IP address
#define eepromAddrPortNum     3 // Port number high byte
                           // 4    Port number low byte
#define eepromAddrLoggerIP    5 // Last byte of logger IP address
#define eepromAddrBoardCode   6 // Board code char 1
#define eepromAddrLocCode     9 // Location code char 1
                          // 10    Location code char 2
                          // 11    Location code char 3
#define eepromAddrSSID       64 // Location of SSID name (max 32 bytes)
#define eepromAddrPWD        96 // Location of AP password

const int eepromSize = 160;
//**************************************************
// Bit masks to poll individual config byte data
const byte startRunningBitMask = B00000001;
const byte sendUDPBitMask      = B00000010;
const byte ipaddressSetMask    = B00000100;
const byte portSetBitmask      = B00001000;
const byte loggerIPSetBitmask  = B00010000;
const byte boardCodeSetBitmask = B00100000;
const byte locCodeSetBitmask   = B01000000;
const byte wifiSetBitmask      = B10000000;

//**************************************************
// Define IP and port settings
// IP, Gateway, DNS, and subnet mask settings
IPAddress myip(192, 168, 1, 30); // where xx is the desired IP Address
IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your
IPAddress dns(8, 8, 8, 8); // Google DNS server
IPAddress dstIP(192, 168, 1, 65); //logging server

uint16_t myPortNum = 53394;

const char* ssid     = "SSIDNotSet";
const char* password = "PWDNotSet";

// Used to test if online, can be replaced with any
// host name or ip address
const char* host = "www.google.com";

//**************************************************
// Define other globals
static bool runState = 0;     // Get sensor data
static bool sendUDPState = 0; // Send data via UDP

//**************************************************
// Board specific data
char thisBoard[] = "NA "; // Board configuration code
char boardLoc[] = "NA ";  // Location code

//**************************************************
// additional declarations and inits
const int ledPin = 13;
const int enterCommandModePin = 14;

static unsigned long currentms; // Timer variable

// LED blink settings
unsigned long prevBlink_ms = 0;
const long blinkRate = 1000;
int ledState = LOW;

// UDP poll settings
unsigned long prevRecUDP_ms = 0;
const long checkUDPRecRate = 10;

// Sensor poll settings
unsigned long prevSendWx_ms = 0;
const long sendWxRate = 2000;

// Sensor data
static float RH;
static float T;
//**************************************************
Weather sensor;
WiFiUDP myUDP;
//**************************************************

void setup() {
  Serial.begin(9600);
  //Serial.print("\n\n");
  //strcpy(testing,"blah");
  //Serial.println(testing);
  Serial.print("\n\n");
  Serial.print("Firmware version: "); Serial.println(verStr);
  Serial.println("Beginning setup...");
  // ********Handle reset*********
  // If pin is held low on start, reset to no run state

  // Get eeprom data
  // Note that EEPROM handling is different on ESP8266
  EEPROM.begin(eepromSize);
  pinMode(enterCommandModePin, INPUT);
  if (digitalRead(enterCommandModePin) == 0)
  {
    Serial.println(F("******Resetting start/stop and UDP broadcast bits******"));
    EEPROM.write(eepromAddrConfigByte1, 0x00);
    EEPROM.end();
  }

  // *****************************
  // ****Handle eeprom reload*****
  // Read configuration byte from EEPROM and handle cases
  byte eepromByte = 0xFF;
  eepromByte = EEPROM.read(eepromAddrConfigByte1);
  Serial.print("Config byte: "); Serial.println(eepromByte, BIN);

  if ((eepromByte & startRunningBitMask) > 0)
  {
    runState = 1;
    Serial.println(F("Starting in running state"));
  }
  else
  {
    runState = 0;
    Serial.println(F("Starting in stopped state"));
  }

  if ((eepromByte & sendUDPBitMask) > 0)
  {
    sendUDPState = 1;
    Serial.println(F("Starting with UDP broadcast ON"));
  }
  else
  {
    sendUDPState = 0;
    Serial.println(F("Starting with UDP broadcast OFF"));
  }

  if ((eepromByte & ipaddressSetMask) > 0)
  {
    byte storedIP;
    EEPROM.get(eepromAddrMyIP, storedIP);
    Serial.print("IP address preprogrammed: "); Serial.println(storedIP);
    myip[3] = storedIP;
  }
  else
  {Serial.println("IP address not preprogrammed...");}

  if ((eepromByte & portSetBitmask) > 0)
  {
    byte high = EEPROM.read(eepromAddrPortNum);
    byte low = EEPROM.read(eepromAddrPortNum + 1);
    myPortNum = word(high, low);
    Serial.print("Port number preprogrammed: "); Serial.println(myPortNum);
  }
  else
  {Serial.println("Port number not preprogrammed...");}

  if ((eepromByte & boardCodeSetBitmask) > 0)
  {
    for (int i = 0; i < 3; i++)
    {EEPROM.get(eepromAddrBoardCode + i, thisBoard[i]);}
    Serial.print("Board code set: "); Serial.println(thisBoard);
  }
  else
  {Serial.println(F("Board code not preprogrammed..."));}

  if ((eepromByte & locCodeSetBitmask) > 0)
  {
    for (int i = 0; i < 3; i++)
    {EEPROM.get(eepromAddrLocCode + i, boardLoc[i]);}
    Serial.print("Location code set: "); Serial.println(boardLoc);
  }
  else
  {Serial.println(F("Location code not preprogrammed..."));}

  if ((eepromByte & loggerIPSetBitmask) > 0)
  {
    byte storedIP;
    EEPROM.get(eepromAddrLoggerIP, storedIP);
    Serial.print("Logger IP address preprogrammed: "); Serial.println(storedIP);
    dstIP[3] = storedIP;
  }
  else
  {Serial.println(F("Logger IP address not set..."));}

  if ((eepromByte & wifiSetBitmask) > 0)
  {
    for(int i = 0;i < 32;i++)
    {
      EEPROM.get(eepromAddrSSID+i,ssid[i]);
      if(ssid[i] == 0)
      {break;}
    }
    for(int i = 0;i < 32;i++)
    {
      EEPROM.get(eepromAddrPWD+i,password[i]);
      if(password[i] == 0)
      {break;}
    }

  }
  else
  {Serial.println(F("Wifi settings not stored"));}
  
  // *****************************
  // Test ethernet and ping google
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(myip, dns, gateway, subnet);
  WiFi.begin(ssid, password);

  int numAttempts = 30;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(numAttempts-- <= 0)
    {break;}
  }
  if(numAttempts > 0)
  {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Connecting to ");
    Serial.println(host);
  }
  else
  {Serial.println("NOT CONNECTED TO WIFI");}

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) 
  {Serial.println("connection failed");}
  else
  {Serial.println("Online");}

  // *****************************
  // Init UDP
  myUDP.begin(myPortNum);

  // Start blinking and sensor
  pinMode(ledPin, OUTPUT);
  // Start wx sensor
  sensor.begin();
}

void loop() {
  // Get current time
  currentms = millis();

  // If time to blink has passed, change LED state.
  if (currentms - prevBlink_ms >= blinkRate)
  {
    prevBlink_ms = currentms;
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
    digitalWrite(ledPin, ledState);
  }
  //----------------------
  // If time to check for UDP/Serial incoming
  if (currentms - prevRecUDP_ms >= checkUDPRecRate)
  {
    prevRecUDP_ms = currentms;
    udpHandle();
    serialHandle();
  }
  //----------------------
  // Handle get sensor and send data
  if ((currentms - prevSendWx_ms >= sendWxRate) && runState)
  {
    prevSendWx_ms = currentms;
    RH = sensor.getRH();
    T = sensor.getTemp();

    bool sendSerial = 1;
    //sendUDPState = 1;
    sendData(sendT, T, sendSerial, sendUDPState);
    sendData(sendRH, RH, sendSerial, sendUDPState);
  }
}
//**************************************************
static char toSend[19];
void sendData(int sendWhat, float value, bool sendSerial, bool sendUDP)
{
  // Determine what type of data being set and preload send string
  // appropriatly, if x,y, or n show up in data, there was bad formatting
  switch (sendWhat)
  {
    case sendT:
      {strcpy(toSend, "xxx yyy  T nnnnnnn");break;}
    case sendRH:
      {strcpy(toSend, "xxx yyy RH nnnnnnn");break;}
    default:
      {Serial.println("default");}
  }
  // Copy 3 char board code
  char dataChar[7];
  for (int i = 0; i < 3; i++)
  {toSend[i] = thisBoard[i];}
  
  // Copy 3 char board location code
  int c = 0;
  for (int i = 4; i < 7; i++)
  {toSend[i] = boardLoc[c++];}
  
  // Convert value to send to a string
  dtostrf(value, 8, 2, dataChar);
  c = 0;
  for (int i = 11; i < 18; i++)
  {toSend[i] = dataChar[c++];}
  
  // If send serial flag is true, send serial
  if (sendSerial)
  {Serial.println(toSend);}

  // If send UDP flag is true, send UDP
  if (sendUDP)
  {
    myUDP.beginPacket(dstIP, myPortNum);
    myUDP.write(toSend);
    myUDP.endPacket();
  }
}

//**************************************************
char incomingPacket[12];

// Reads the UDP buffer and copies the contents 
// to incomingPacket with null termination
void udpHandle()
{
  int packetSize = myUDP.parsePacket();
  int len;
  if (packetSize)
  {
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, myUDP.remoteIP().toString().c_str(), myUDP.remotePort());
    len = myUDP.read(incomingPacket, 11);
    if (len > 0)
    {incomingPacket[len] = 0;}
    processCommand();
  }
}

// Reads the serial buffer and copies the contents 
// to incomingPacket with null termination
void serialHandle()
{
  int NSerialBytes = Serial.available();
  int c = 0;
  if (NSerialBytes > 0)
  {
    Serial.printf("Received %d bytes from serial port\n", NSerialBytes);
    for (int i = 0; i < NSerialBytes; i++)
    {incomingPacket[i] = (char)Serial.read();c++;}
    incomingPacket[c] = 0;
    processCommand();
  }
}

// Takes the incomingPacket variable and splits it into 2 variables based
// on the space character. The split puts the first array into the command
// array an the second (if it exists) into the data array. Both arrays are
// null terminated.
// After splitting, an element wise comparison is done to idenitify which
// command was sent.
void processCommand()
{
  Serial.print("Buffer: '"); Serial.print(incomingPacket);Serial.println("'");
  bool endCommand = 0;
  int c = 0;
  char commandStr[6];
  char commandData[7];
  int nCommandBytes = 0;
  int nDataBytes = 0;
  
  for (int i = 0; i <= 12; i++)
  {
    if(incomingPacket[i] == 0)
    {break;}
    
    if(incomingPacket[i] == ' ')
    {endCommand = 1;continue;}

    if(endCommand == 0)
    {commandStr[nCommandBytes++] = incomingPacket[i];}
    else
    {commandData[nDataBytes++] = incomingPacket[i];}
  }
  commandStr[nCommandBytes] = 0;
  commandData[nDataBytes] = 0;
  
  if (matchCommand(commandStr, "START", nCommandBytes))
  {
    Serial.println("Starting...");
    runState = 1;
  }
  else if (matchCommand(commandStr, "STOP", nCommandBytes))
  {
    Serial.println("Stopping...");
    runState = 0;
  }
  else if (matchCommand(commandStr, "SFLAG", nCommandBytes))
  {
    EEPROM.begin(eepromSize);
    byte eepromByte = 0xFF;
    eepromByte = EEPROM.read(eepromAddrConfigByte1);
    byte testByte = eepromByte & startRunningBitMask;
    if (testByte > 0)
    {
      eepromByte = eepromByte & ~startRunningBitMask;
      Serial.println(F("Turning OFF start in running mode..."));
    }
    else
    {
      eepromByte = eepromByte | startRunningBitMask;
      Serial.println("Turning ON start in running mode...");
    }
    EEPROM.write(eepromAddrConfigByte1, eepromByte);
    EEPROM.end();
  }
  else if (matchCommand(commandStr, "UDP", nCommandBytes-1))
  {
    if (sendUDPState)
    {
      Serial.println(F("Stopping UDP broadcast..."));
      sendUDPState = 0;
    }
    else
    {
      Serial.print(F("Starting UDP broadcast to 192.168.1."));
      Serial.println(dstIP[3]);
      sendUDPState = 1;
    }
  }
  else if (matchCommand(commandStr, "UFLAG", nCommandBytes))
  {
    EEPROM.begin(eepromSize);
    byte eepromByte = 0xFF;
    eepromByte = EEPROM.read(eepromAddrConfigByte1);
    byte testByte = eepromByte & sendUDPBitMask;
    if (testByte > 0)
    {
      eepromByte = eepromByte & ~sendUDPBitMask;
      Serial.println(F("Turning OFF start with UDP broadcast..."));
    }
    else
    {
      eepromByte = eepromByte | sendUDPBitMask;
      Serial.println("Turning ON start with UDP broadcast...");
    }
    EEPROM.write(eepromAddrConfigByte1, eepromByte);
    EEPROM.end();

  }
  else if (matchCommand(commandStr, "SMYIP", nCommandBytes))
  {
    EEPROM.begin(eepromSize);
    Serial.print(F("Updating device IP address to: ")); Serial.println(commandData);
    myip[3] = atoi(commandData);
    EEPROM.write(eepromAddrMyIP, myip[3]);
    byte configByte = 0xFF;
    EEPROM.get(eepromAddrConfigByte1, configByte);
    configByte = configByte | ipaddressSetMask;
    EEPROM.write(eepromAddrConfigByte1, configByte);
    EEPROM.end();
  }
  else if (matchCommand(commandStr, "SPORT", nCommandBytes))
  {
    EEPROM.begin(eepromSize);
    Serial.print(F("Updating UDP port: ")); Serial.println(commandData);
    myPortNum = atoi(commandData);
    Serial.println(myPortNum);

    EEPROM.write(eepromAddrPortNum, highByte(myPortNum));
    EEPROM.write(eepromAddrPortNum + 1, lowByte(myPortNum));

    byte configByte = 0xFF;
    EEPROM.get(eepromAddrConfigByte1, configByte);
    configByte = configByte | portSetBitmask;
    EEPROM.write(eepromAddrConfigByte1, configByte);
    EEPROM.end();
  }
  else if (matchCommand(commandStr, "SETBC", nCommandBytes))
  {
    Serial.print(F("Updating board code: ")); Serial.println(commandData);
    EEPROM.begin(eepromSize);
    for (int i = 0; i < 3; i++)
    {
      EEPROM.put(eepromAddrBoardCode + i, commandData[i]);
      thisBoard[i] = commandData[i];
    }
    byte configByte = 0xFF;
    EEPROM.get(eepromAddrConfigByte1, configByte);
    configByte = configByte | boardCodeSetBitmask;
    EEPROM.write(eepromAddrConfigByte1, configByte);
    EEPROM.end();
  }
  else if (matchCommand(commandStr, "SLOC", nCommandBytes))
  {
    Serial.print(F("Updating board location: ")); Serial.println(commandData);
    EEPROM.begin(eepromSize);
    for (int i = 0; i < 3; i++)
    {
      EEPROM.put(eepromAddrLocCode + i, commandData[i]);
      boardLoc[i] = commandData[i];
    }
    byte configByte = 0xFF;
    EEPROM.get(eepromAddrConfigByte1, configByte);
    configByte = configByte | locCodeSetBitmask;
    EEPROM.write(eepromAddrConfigByte1, configByte);
    EEPROM.end();
  }
  else if (matchCommand(commandStr, "SLIP", nCommandBytes))
  {

    EEPROM.begin(eepromSize);
    Serial.print(F("Updating logger IP address to: ")); Serial.println(commandData);
    dstIP[3] = atoi(commandData);
    EEPROM.write(eepromAddrLoggerIP, dstIP[3]);
    byte configByte = 0xFF;
    EEPROM.get(eepromAddrConfigByte1, configByte);
    configByte = configByte | loggerIPSetBitmask;
    EEPROM.write(eepromAddrConfigByte1, configByte);
    EEPROM.end();
  }
  else if (matchCommand(commandStr, "CBYTE", nCommandBytes))
  {
    EEPROM.begin(eepromSize);
    byte configByte = 0xFF;
    EEPROM.get(eepromAddrConfigByte1, configByte);
    Serial.print("Config byte 1: "); Serial.println(configByte, BIN);

    if (configByte == 0)
    {Serial.println("No configuration data saved");}
    if (configByte & startRunningBitMask)
    {Serial.println("Starts sensor polling on power up");}
    if (configByte & sendUDPBitMask)
    {Serial.println("Starts UDP broadcast on power up");}
    if (configByte & ipaddressSetMask)
    {Serial.println("Last byte of sensor's IP address is stored");}
    if (configByte & portSetBitmask)
    {Serial.println("UDP port is set");}
    if (configByte & loggerIPSetBitmask)
    {Serial.println("Last byte of logger's IP address is stored");}
    if (configByte & boardCodeSetBitmask)
    {Serial.println("Board code is set");}
    if (configByte & locCodeSetBitmask)
    {Serial.println("Board location code is set");}
    if (configByte & wifiSetBitmask)
    {Serial.println("This flag should not be set, somthing is wrong");}
  }
  else if (matchCommand(commandStr, "WIFI", nCommandBytes))
  {setSSIDandPWord();}
  else if (matchCommand(commandStr, "HELP", nCommandBytes))
  {displayHelp();}
  else if (matchCommand(commandStr, "RESET", nCommandBytes))
  {Serial.println("Software reset....");ESP.reset();}
  else
  {
    Serial.println("Code not found");
    Serial.print("Command: '"); Serial.print(commandStr); Serial.println("'");
    Serial.print("   Code: '"); Serial.print(commandData); Serial.println("'");
  }
  incomingPacket[0] = 0; // clear the buffer
}

// Loop through str1 doing element wise comparison to str2
bool matchCommand(const char* str1, const char* str2, int len)
{
  for (int i = 0; i < len; i++)
  {
    if (str1[i] != str2[i])
    {return 0;}
  }
  return 1;
}
//**************************************************
void setSSIDandPWord()
{
  char incomingSSID[32];
  char incomingPWD[64];
  int NSSIDBytes = 0;
  int NPWDBytes = 0;
  
  Serial.println("\nConfiguring wifi SSID and password");
  Serial.println("Serial commands only, reset device to cancel");
  Serial.print("Enter SSID: ");

  NSSIDBytes = Serial.available();
  while(NSSIDBytes == 0)
  {
    NSSIDBytes = Serial.available();
    delay(100);
  }
  for (int i = 0; i < NSSIDBytes; i++)
  {incomingSSID[i] = (char)Serial.read();}
  incomingSSID[NSSIDBytes] = 0;
  Serial.println(incomingSSID);

  Serial.print("Enter password: ");

  NPWDBytes = Serial.available();
  while(NPWDBytes == 0)
  {
    NPWDBytes = Serial.available();
    delay(100);
  }
  for (int i = 0; i < NPWDBytes; i++)
  {incomingPWD[i] = (char)Serial.read();}
  incomingPWD[NPWDBytes] = 0;
  Serial.println(incomingPWD);



  EEPROM.begin(eepromSize);
  for(int i = 0;i <= NSSIDBytes;i++)
  {EEPROM.write(eepromAddrSSID+i, incomingSSID[i]);}
  for(int i = 0;i <= NPWDBytes;i++)
  {EEPROM.write(eepromAddrPWD+i, incomingPWD[i]);}
  
  
  byte configByte = 0xFF;
  EEPROM.get(eepromAddrConfigByte1, configByte);
  configByte = configByte | wifiSetBitmask;
    EEPROM.write(eepromAddrConfigByte1, configByte);
  

  EEPROM.end();
  Serial.println("\nEnd wifi configuration, reset for changes to take effect\n");
}
//**************************************************
void displayHelp()
{
  Serial.print("\nFirmware version: "); Serial.println(verStr);
  Serial.println(F("----COMMAND LIST----"));
  Serial.println(F("START - Start polling sensors"));
  Serial.println(F(" STOP - Stop polling sensors"));
  Serial.println(F("SFLAG - Toggle run on start command"));
  Serial.println(F("  UDP - Toggle UDP broadcast"));
  Serial.println(F("UFLAG - Toggle UDP broadcast on start"));
  Serial.println(F("SMYIP - Set final byte of board's IP address"));
  Serial.println(F("SPORT - Set UDP port number"));
  Serial.println(F("SETBC - Set board code (3 char)"));
  Serial.println(F(" SLOC - Set location code (3 char)"));
  Serial.println(F(" SLIP - Set final byte of logger's IP address"));
  Serial.println(F("CBYTE - Display configuration byte"));
  Serial.println(F(" WIFI - Enter SSID/password for wifi config (serial mode only)"));
  Serial.println(F("--------------------"));
}



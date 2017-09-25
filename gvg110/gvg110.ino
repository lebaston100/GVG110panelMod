#include <ArduinoHttpClient.h>
#include <SPI.h>
#include <Ethernet.h>

//Pins
byte dataPins[] = {26, 29, 31, 33, 32, 30, 28, 27};
byte addressPins[] = {23, 22, 25, 24};
byte specialPins[] = {35, 34, 36, 37, 39}; //35 ReadButton; 34 WriteLamp; 36 Convert; 37 ReadAnalog; 39 Displayclock

//Ethernet
byte mac[] = { 0x5A, 0xA2, 0xDA, 0x0D, 0x56, 0x7A }; //The MAC address of your ethernet shield
boolean isConnected;

//Input
#define BtBffSize 80
boolean buttonBuffer[BtBffSize];
boolean savebuttonBuffer[BtBffSize];
byte onButtonBuffer[BtBffSize];
byte offButtonBuffer[BtBffSize];

unsigned int analogBuffer[16];
unsigned int oldanalogBuffer[16];
unsigned int SendanalogBuffer[32];

//Output
boolean lampBuffer[100];
byte displayBuffer[7];

//Networking
char serverAddress[] = "10.2.1.3"; //IP Address of websocket server for communication
int port = 1234; //Port of websocket server
byte ip[] = {10, 2, 1, 4 }; //Ethernet shield IP
byte dnsIP[] = {10, 1, 1, 1 }; //Ethernet shield DNS
byte gateway[] = {10, 1, 1, 1 }; //Ethernet shield gateway
byte subnet[] = {255, 0, 0, 0}; //Ethernet shield subnetmask
EthernetClient lan;
WebSocketClient client = WebSocketClient(lan, serverAddress, port);

int abs2(int input) {
  if (input < 0) {
    return input * -1;
  } else {
    return input;
  }
}

void setup() {
  byte i = 0;
  while (i < 5) {
    pinMode(addressPins[i], OUTPUT);
    i++;
  }
  i = 0;
  while (i < 5) {
    pinMode(specialPins[i], OUTPUT);
    i++;
  }

  i = 0;
  while (i < 16) {
    analogBuffer[i] = 0;
    i++;
  }
  i = 0;
  while (i < 7) {
    displayBuffer[i] = 15;
    i++;
  }
  i = 0;
  while (i < 100) {
    lampBuffer[i] = 0;
    if (i == 42) {
      lampBuffer[i] = 1;
    }
    i++;
  }
  i = 0;
  while (i < BtBffSize) {
    buttonBuffer[i] = 1;
    savebuttonBuffer[i] = 1;
    i++;
  }
  writeLamps();
  SetDisplay();
  Serial.begin(2000000); //Debug
  Serial.println(F("Hello"));
  Ethernet.begin(mac, ip, dnsIP, gateway);
  //Ethernet.begin(mac); //Comment out to use DHCP
}

void loop() {
  if (!isConnected) { //If websocket is not connected, do that
    client.begin();
  }

  if (client.connected()) {
    if (!isConnected) {
      client.beginMessage(TYPE_TEXT);
      client.print(F("x:"));
      client.endMessage();
      isConnected = 1;
    }
  }

  if (client.parseMessage() > 0) {
    String msg = client.readString();
    msg.trim();
    if (msg.startsWith("a:")) { //Lamps on
      msg.remove(0, 2);
      do {
        lampBuffer[msg.substring(0, msg.indexOf(":")).toInt()] = 1;
        msg.remove(0, msg.indexOf(":") + 1);
      } while (msg.length() > 1);
      writeLamps();
    } else if (msg.startsWith("b:")) { //Lamps off
      msg.remove(0, 2);
      do {
        lampBuffer[msg.substring(0, msg.indexOf(":")).toInt()] = 0;
        msg.remove(0, msg.indexOf(":") + 1);
      } while (msg.length() > 1);
      writeLamps();
    } else if (msg.startsWith("c:")) { //All lamps off
      for (byte i; i < 100; i++) {
        lampBuffer[i] = 0;
      }
      writeLamps();
    } else if (msg.startsWith("d:")) { //Update Display //0-9;-;E;H;L;P;Blank
      msg.remove(0, 2);
      byte couter = 0;
      do {
        displayBuffer[couter] = msg.substring(0, msg.indexOf(":")).toInt();
        msg.remove(0, msg.indexOf(":") + 1);
        couter++;
      } while (msg.length() > 1);
      SetDisplay();
    }
  }

  readButtons(); //Read buttons
  ReadAnalog(); //Read analog

  //Compare old and new buttonstate
  byte onButtonCounter = 0;
  byte offButtonCounter = 0;
  byte i = 0;
  while (i < BtBffSize) {
    if (buttonBuffer[i] != savebuttonBuffer[i]) {
      if (!buttonBuffer[i]) {
        onButtonBuffer[onButtonCounter] = i;
        onButtonCounter++;
      } else {
        offButtonBuffer[offButtonCounter] = i;
        offButtonCounter++;
      }
    }
    savebuttonBuffer[i] = buttonBuffer[i];
    i++;
  }

  //compares analog values
  byte PotHasChangedCounter = 0;
  for (byte i; i < 16; i++) {
    if (abs2(analogBuffer[i] - oldanalogBuffer[i]) > 3) {
      SendanalogBuffer[PotHasChangedCounter] = i;
      SendanalogBuffer[PotHasChangedCounter + 1] = analogBuffer[i];
      PotHasChangedCounter += 2;
    }
    oldanalogBuffer[i] = analogBuffer[i];
  }

  //Send data
  if (PotHasChangedCounter) { //If analog changed, then send
    client.beginMessage(TYPE_TEXT);
    client.print(F("a"));
    for (byte i; i < PotHasChangedCounter; i++) {
      client.print(F(":"));
      client.print(SendanalogBuffer[i]);
    }
    client.endMessage();
  }
  if (onButtonCounter) { //If buttons changed to on, then send
    client.beginMessage(TYPE_TEXT);
    client.print(F("b1"));
    for (byte i; i < onButtonCounter; i++) {
      client.print(F(":"));
      client.print(onButtonBuffer[i]);
      if (i == 40) {
        client.endMessage();
        client.beginMessage(TYPE_TEXT);
        client.print(F("b2"));
      }
    }
    client.endMessage();
  }
  if (offButtonCounter) { //If buttons changed to off, then send
    client.beginMessage(TYPE_TEXT);
    client.print(F("c1"));
    for (byte i; i < offButtonCounter; i++) {
      client.print(F(":"));
      client.print(offButtonBuffer[i]);
      if (i == 40) {
        client.endMessage();
        client.beginMessage(TYPE_TEXT);
        client.print(F("c2"));
      }
    }
    client.endMessage();
  }
}


void SetDisplay() {
  DatalinesAsOutput();
  delayMicroseconds(30);
  byte i = 0;
  while (i < 8) {
    SetDisplayAddressBus(i);
    delayMicroseconds(10);
    digitalWrite(dataPins[3], (displayBuffer[i] & 1));
    digitalWrite(dataPins[4], (displayBuffer[i] & 2) >> 1);
    digitalWrite(dataPins[5], (displayBuffer[i] & 4) >> 2);
    digitalWrite(dataPins[6], (displayBuffer[i] & 8) >> 3);
    i++;
    ActivateSpecialFunction(4);
    delayMicroseconds(10);
    endSpecialfunction();
  }
}

void writeLamps() {
  DatalinesAsOutput();
  byte tmp1_counter = 0;
  delayMicroseconds(50);
  byte i = 0;
  while (i < 10) {
    SetAddressBus(i);
    byte g = 0;
    while (g < 8) {
      digitalWrite(dataPins[g], lampBuffer[tmp1_counter + g]);
      g++;
      delayMicroseconds(3);
    }
    ActivateSpecialFunction(1);
    tmp1_counter += 8;
    delayMicroseconds(30);
    endSpecialfunction();
    i++;
  }
  endSpecialfunction();
  delayMicroseconds(300);
}

void readButtons() {
  delayMicroseconds(50);
  DatalinesAsInput2();
  delayMicroseconds(50);
  byte tmp1_counter = 0;
  byte i = 0;
  while (i < 10) {
    SetAddressBus(i);
    ActivateSpecialFunction(0);
    delayMicroseconds(30);
    byte g = 0;
    while (g < 8) {
      buttonBuffer[tmp1_counter + g] = digitalRead(dataPins[g]);
      g++;
      delayMicroseconds(10);
    }
    endSpecialfunction();
    tmp1_counter += 8;
    i++;
  }
}

void ReadAnalog() {
  DatalinesAsInput();
  byte i = 0;
  while (i < 15) {
    SetAddressBus(i);
    delayMicroseconds(30);
    ActivateSpecialFunction(2);
    delayMicroseconds(30);
    endSpecialfunction();
    delayMicroseconds(30);
    ActivateSpecialFunction(3);
    delayMicroseconds(30);
    byte a = 7;
    byte b = 9;
    analogBuffer[i] = 0;
    while (a != 255) {
      analogBuffer[i] |= (digitalRead(dataPins[a]) << b);
      a--;
      b--;
    }
    endSpecialfunction();
    delayMicroseconds(30);
    ActivateSpecialFunction(3);
    delayMicroseconds(30);
    a = 7;
    while (a > 5) {
      analogBuffer[i] |= (digitalRead(dataPins[a]) << b);
      a--;
      b--;
    }
    endSpecialfunction();
    i++;
  }
}


void ActivateSpecialFunction(byte pin) {  //0 ReadButton; 1 WriteLamp; 2 Convert; 3 ReadAnalog; 4 Displayclock
  byte i = 0;
  while (i < 5) {
    digitalWrite(specialPins[i], HIGH);
    i++;
  }
  digitalWrite(specialPins[pin], LOW);
  delayMicroseconds(30);
}

void endSpecialfunction() {
  byte i = 0;
  while (i < 5) {
    digitalWrite(specialPins[i], HIGH);
    i++;
    delayMicroseconds(30);
  }
}

void SetDisplayAddressBus(byte number) {
  digitalWrite(dataPins[0], (number & 1));
  digitalWrite(dataPins[1], (number & 2) >> 1);
  digitalWrite(dataPins[2], (number & 4) >> 2);
}

void SetAddressBus(byte number) {
  digitalWrite(addressPins[0], (number & 1));
  digitalWrite(addressPins[1], (number & 2) >> 1);
  digitalWrite(addressPins[2], (number & 4) >> 2);
  digitalWrite(addressPins[3], (number & 8) >> 3);
  delayMicroseconds(10);
}

void DatalinesAsInput() {
  byte i = 0;
  while (i < 8) {
    pinMode(dataPins[i], INPUT);
    i++;
    delayMicroseconds(10);
  }
}

void DatalinesAsInput2() {
  byte i = 0;
  while (i < 8) {
    pinMode(dataPins[i], INPUT_PULLUP);
    i++;
    delayMicroseconds(10);
  }
}

void DatalinesAsOutput() {
  byte i = 0;
  while (i < 8) {
    pinMode(dataPins[i], OUTPUT);
    i++;
    delayMicroseconds(10);
  }
}

void AllDatalinesLow() {
  byte i = 0;
  while (i < 8) {
    digitalWrite(dataPins[i], LOW);
    i++;
    delayMicroseconds(10);
  }
}

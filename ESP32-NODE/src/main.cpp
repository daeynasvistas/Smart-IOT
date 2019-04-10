/*
  LoRa Duplex communication based on Tom Igoe example of Arduino LoRa library
  Note: while sending, LoRa radio is not listening for incoming messages.

  IPG - MCM 2019
  THIS is THE MASTER node (WIth WIFI pseudo-gateway)
*/
#include <SPI.h>     // include libraries
#include <LoRa.h>    //https://github.com/sandeepmistry/arduino-LoRa
#include "SSD1306.h" //https://github.com/ThingPulse/esp8266-oled-ssd1306

//Pinout! Customized for TTGO LoRa32 V2.0 Oled Board!
#define SX1278_SCK 5   // GPIO5  -- SX1278's SCK
#define SX1278_MISO 19 // GPIO19 -- SX1278's MISO
#define SX1278_MOSI 27 // GPIO27 -- SX1278's MOSI
#define SX1278_CS 18   // GPIO18 -- SX1278's CS
#define SX1278_RST 14  // GPIO14 -- SX1278's RESET
#define SX1278_DI0 26  // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define OLED_ADDR 0x3c // OLED's ADDRESS
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

#define LORA_BAND 868.9E6 // LoRa Band (Europe)

#include <WiFiClientSecure.h>

WiFiClientSecure client;

char ssid[] = "EPT-HOTSPOT"; //  your network SSID (name of wifi network)
char pass[] = "";            // your network password

char server[] = "www.howsmyssl.com"; // Server URL
// You can use x.509 certificates if you want
//unsigned char test_ca_cert[] = "";      //For the usage of verifying server
//unsigned char test_client_key[] = "";   //For the usage of verifying client
//unsigned char test_client_cert[] = "";  //For the usage of verifying client

/*
//////////////////////CONFIG 1///////////////////////////
byte localAddress = 8;     // address of this device
byte destination = 18;     // destination to send to
int interval = 3000;       // interval between sends
String message = "Ping!";    // send a message
///////////////////////////////////////////////////////
*/

//////////////////////CONFIG 2///////////////////////////
byte localAddress = 18;   // address of this device
byte destination = 8;     // destination to send to
int interval = 4000;      // interval between sends
String message = "OK"; // send a message
///////////////////////////////////////////////////////

String outgoing;       // outgoing message
byte msgCount = 0;     // count of outgoing messages
long lastSendTime = 0; // last send time

SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);

void printScreen()
{
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.setColor(BLACK);
  display.fillRect(0, 0, 127, 30);
  display.display();

  display.setColor(WHITE);
  display.drawString(0, 00, String(LORA_BAND / 10000000) + " LoRa sender " + String(localAddress));

  display.drawString(0, 10, "Me: " + String(localAddress) + "  To: " + String(destination) + " N: " + String(msgCount));
  display.drawString(0, 20, "Tx: " + message);
  display.display();
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();            // start packet
  LoRa.write(destination);       // add destination address
  LoRa.write(localAddress);      // add sender address
  LoRa.write(msgCount);          // add message ID
  LoRa.write(outgoing.length()); // add payload length
  LoRa.print(outgoing);          // add payload
  LoRa.endPacket();              // finish packet and send it
  printScreen();

  Serial.println("Enviar Mensagem " + String(msgCount) + " para Node: " + String(destination));
  Serial.println("Mensagem: " + message);
  Serial.println();
  delay(1000);
  msgCount++; // increment message ID
}

void onReceive(int packetSize)
{
  // Serial.println("error");
  if (packetSize == 0)
    return; // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();       // recipient address
  byte sender = LoRa.read();         // sender address
  byte incomingMsgId = LoRa.read();  // incoming msg ID
  byte incomingLength = LoRa.read(); // incoming msg length

  String incoming = ""; // payload of packet

  while (LoRa.available())
  {                                // can't use readString() in callback, so
    incoming += (char)LoRa.read(); // add bytes one by one
  }

  if (incomingLength != incoming.length())
  { // check length for error
    Serial.println("error: message length does not match length");
    incoming = "message length error";
    return; // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF)
  {
    Serial.println("Esta mensagem não é para mim.");
    incoming = "message is not for me";
    return; // skip rest of function
  }

  display.setColor(BLACK);
  display.fillRect(0, 32, 127, 61);
  display.display();

  display.setColor(WHITE);
  display.drawLine(0, 31, 127, 31);
  display.drawString(0, 32, "Rx: " + incoming);

  display.drawString(0, 42, "FR:" + String(sender) + " TO:" + String(recipient) + " LG:" + String(incomingLength) + " ID:" + String(incomingMsgId));
  display.drawString(0, 52, "RSSI: " + String(LoRa.packetRssi()) + " SNR: " + String(LoRa.packetSnr()));

  display.display();

  // if message is for this device, or broadcast, print details:
  Serial.println("Recebido de: 0x" + String(sender, HEX));
  Serial.println("Enviar Para: 0x" + String(recipient, HEX));
  Serial.println("Mensagem ID: " + String(incomingMsgId));
  Serial.println("Mensagem length: " + String(incomingLength));
  Serial.println("Msg: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  delay(1000);
}


void connectWIFI(){
  // HTTPS secure Access API
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    // wait 1 second for re-trying
    display.drawString(00, 00, "Connecting...");
    display.display();
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.println("Connected to ");
  display.drawString(00, 10, ssid);
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  display.drawString(00, 20, WiFi.localIP().toString().c_str());
  display.display();
  delay(1500);

  Serial.println("\nStarting connection to server...");
  if (client.connect(server, 443))
  { //client.connect(server, 443, test_ca_cert, test_client_cert, test_client_key)
    Serial.println("Connected to server!");
    // Make a HTTP request:
    client.println("GET https://www.howsmyssl.com/a/check HTTP/1.0");
    client.println("Host: www.howsmyssl.com");
    client.println("Connection: close");
    client.println();
  }
  else
    Serial.println("Connection failed!");

  Serial.print("Waiting for response "); //WiFiClientSecure uses a non blocking implementation
  while (!client.available())
  {
    delay(50); //
    Serial.print(".");
  }
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available())
  {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected())
  {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
  }
  display.drawString(00, 30, "SSL OK!");
  display.display();
  delay(1500);
}

void setup()
{

  Serial.begin(115200); // initialize serial

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(OLED_RST, HIGH); // while OLED running, must set GPIO16 in high
  delay(1000);

  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.clear();

  connectWIFI();

  while (!Serial)
    ;
  Serial.println("TTGO LoRa32 V2.0 P2P");
  display.drawString(00, 50, "TTGO LoRa32 V2.0 P2P");
  display.display();

  //SPI.begin(SX1278_SCK, SX1278_MISO, SX1278_MOSI, SX1278_CS);
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(SX1278_CS, SX1278_RST, SX1278_DI0); // set CS, reset, IRQ pin

  if (!LoRa.begin(LORA_BAND))
  { // initialize ratio at 868 MHz
    Serial.println("LoRa init failed. Check your connections.");
    display.drawString(0, 30, "LoRa init failed");
    display.drawString(0, 40, "Check connections");
    display.display();
    while (true)
      ; // if failed, do nothing
  }

  /*
  // DAEY change for BOOST performance 
  // The larger the spreading factor the greater the range but slower data rate
  // Send and receive radios need to be set the same
  LoRa.setSpreadingFactor(12); // ranges from 6-12, default 7 see API docs

  // Change the transmit power of the radio
  // Default is LoRa.setTxPower(17, PA_OUTPUT_PA_BOOST_PIN);
  // Most modules have the PA output pin connected to PA_BOOST, gain 2-17
  // TTGO and some modules are connected to RFO_HF, gain 0-14
  // If your receiver RSSI is very weak and little affected by a better antenna, change this!
  LoRa.setTxPower(14, PA_OUTPUT_RFO_PIN);
  // DAEY change for BOOST performance
*/
  LoRa.setSpreadingFactor(8); // ranges from 6-12, default 7 see API docs
  LoRa.setTxPower(13, PA_OUTPUT_RFO_PIN);

  //LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
  display.drawString(00, 40, "LoRa init succeeded.");
  display.display();
  delay(1500);
  display.clear();
  display.display();
}

void loop()
{
  if (millis() - lastSendTime > interval)
  {
    sendMessage(message);
    lastSendTime = millis();        // timestamp the message
    interval = random(4000) + 1000; // 3-4 seconds
    LoRa.receive();                 // go back into receive mode
  }
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    onReceive(packetSize);
  }
}
/*
  -----------------------
  "ENC28J60 Module" arduino UNO connection
  VCC -   3.3V
  GND -    GND
  SCK - Pin 13
  SO  - Pin 12
  SI  - Pin 11
  CS  - Pin 10
  ------------------
*/

#include <EtherCard.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/*
// NRF radio setup 
// https://maniacbug.github.io/RF24/classRF24.html
// CE_PIN, CS_PIN
// VCC - 3.3v
// GND - GND
// SCK - 13
// MISO - 12
// MOSI -  11
// CE - 6
// CS - 8
*/
RF24 radio(6, 8);

#define STATIC 1  // set to 1 to disable DHCP (adjust myip/gwip values below)

// MAC Address
static byte mymac[] = {0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A };

//Gate IP
static byte gwip[] = { 192, 168, 1, 1 };
// ip address ENC module
static byte myip[] = {192, 168, 1, 50 };

// transmision pipe
const uint64_t pipe = 0xE8E8F0F0E1LL;

byte Ethernet::buffer[900];
BufferFiller bfill;

boolean PinStatus[] = { 0,0,0,0};

boolean any_on = false;

//-------------

const char http_OK[] PROGMEM =
  "HTTP/1.0 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Pragma: no-cache\r\n\r\n";

const char http_Found[] PROGMEM =
  "HTTP/1.0 302 Found\r\n"
  "Location: /\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
  "HTTP/1.0 401 Unauthorized\r\n"
  "Content-Type: text/html\r\n\r\n"
  "<h1>401 Unauthorized</h1>";

//------------

// Load home page
void homePage()
{
  bfill.emit_p(PSTR("$F"
                    "<title>ArduinoPIN Webserver</title>"
                    "Relay 1: <a href=\"?ArduinoPIN1=$F\">$F</a><br />"
                    "Relay 2: <a href=\"?ArduinoPIN2=$F\">$F</a><br />"
                    "Relay 3: <a href=\"?ArduinoPIN3=$F\">$F</a><br />"
                    "Relay 4: <a href=\"?ArduinoPIN4=$F\">$F</a><br />"
                    "All OFF: <a href=\"?Arduino=$F\">$F</a>"),

               http_OK,
               PinStatus[0] ? PSTR("off") : PSTR("on"),
               PinStatus[0] ? PSTR("<font color=\"green\"><b>ON</b></font>") : PSTR("<font color=\"red\">OFF</font>"),
               PinStatus[1] ? PSTR("off") : PSTR("on"),
               PinStatus[1] ? PSTR("<font color=\"green\"><b>ON</b></font>") : PSTR("<font color=\"red\">OFF</font>"),
               PinStatus[2] ? PSTR("off") : PSTR("on"),
               PinStatus[2] ? PSTR("<font color=\"green\"><b>ON</b></font>") : PSTR("<font color=\"red\">OFF</font>"),
               PinStatus[3] ? PSTR("off") : PSTR("on"),
               PinStatus[3] ? PSTR("<font color=\"green\"><b>ON</b></font>") : PSTR("<font color=\"red\">OFF</font>"),
               any_on ? PSTR("off") : PSTR("on"),
               any_on ? PSTR("<font color=\"green\"><b>ON</b></font>") : PSTR("<font color=\"red\">OFF</font>"));
}


void setup()
{

  Serial.begin(9600);
  radio.begin();
  radio.setPALevel( RF24_PA_MAX) ;
  radio.setDataRate( RF24_250KBPS ) ;
  radio.setCRCLength( RF24_CRC_8 ) ;
  radio.openWritingPipe(pipe);

// !!!!  10 - CS PIN on ENC module !!!!
// doesn't work without it. Probably default pin is set
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0);
  
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
#endif

  ether.printIp("My SET IP: ", ether.myip); // Виводимо в Serial монітор статичну IP адресу.

  //set all pinStatus to 0 - false
  for (int i = 0; i < 4; i++)
  {
    PinStatus[i] = false;
  }
}

void loop()
{
  delay(1);
  word len = ether.packetReceive();   // check for ethernet packet 
  word pos = ether.packetLoop(len);   // check for tcp packet 

  if (pos) {
    bfill = ether.tcpOffset();
    char *data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET /", data, 5) != 0) {
      bfill.emit_p(http_Unauthorized);
    }

    else {
      data += 5;
      if (data[0] == ' ') {
        homePage(); // Return home page
      }

      // "16" = string length > "?ArduinoPIN1=on ".
      else if (strncmp("?ArduinoPIN1=on ", data, 16) == 0) {
        PinStatus[0] = true;
        bfill.emit_p(http_Found);
        radio.write( PinStatus, 4);
        any_on = true;
      }
      else if (strncmp("?ArduinoPIN2=on ", data, 16) == 0) {
        //radio.write( 0, 2);
        PinStatus[1] = true;
        radio.write( PinStatus, 4);
        bfill.emit_p(http_Found);
        any_on = true;
      }
      else if (strncmp("?ArduinoPIN3=on ", data, 16) == 0) {
        PinStatus[2] = true;
        bfill.emit_p(http_Found);
        radio.write( PinStatus, 4);
        bfill.emit_p(http_Found);
        any_on = true;
      }
      else if (strncmp("?ArduinoPIN4=on ", data, 16) == 0) {
        PinStatus[3] = true;
        bfill.emit_p(http_Found);
        radio.write( PinStatus, 4);
        bfill.emit_p(http_Found);
        any_on = true;
      }
      else if (strncmp("?ArduinoPIN1=off ", data, 17) == 0) {
        PinStatus[0] = false;
        bfill.emit_p(http_Found);
        radio.write( PinStatus, 4);
      }
      else if (strncmp("?ArduinoPIN2=off ", data, 17) == 0) {
        PinStatus[1] = false;
        radio.write( PinStatus, 4);
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?ArduinoPIN3=off ", data, 17) == 0) {
        PinStatus[2] = false;
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?ArduinoPIN4=off ", data, 17) == 0) {
        PinStatus[3] = false;
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?Arduino=off ", data, 13) == 0) {
        PinStatus[0] = false;
        PinStatus[1] = false;
        PinStatus[2] = false;
        PinStatus[3] = false;
        any_on = false;
        radio.write( PinStatus, 4);
        bfill.emit_p(http_Found);
      }
      else {
        // Page not found
        bfill.emit_p(http_Unauthorized);
      }
      
    }
 
    ether.httpServerReply(bfill.position());    // send http response
  }

}



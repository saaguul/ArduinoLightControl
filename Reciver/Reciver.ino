#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 (CE & CS)

RF24 radio(9,10);

// Pins on the remote for buttons
const uint8_t button_pins[] = { 4,2,3,5 };
const uint8_t num_button_pins = sizeof(button_pins);

// Pins on the LED board for LED's
const uint8_t led_pins[] = { 4,2,3,5 };
const uint8_t num_led_pins = sizeof(led_pins);

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

// The various roles supported by this sketch
typedef enum { role_remote = 1, role_led } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Remote", "LED Board"};

// The role of the current running sketch
role_e role;

uint8_t button_states[num_button_pins];
uint8_t led_states[num_led_pins];

//
// Setup
//

void setup(void)
{

  delay(20); // Just to get a solid reading on the role pin

    role = role_led;

  //
  // Print preamble
  //

  Serial.begin(115200);
  printf_begin();
  printf("\n\rRF24/examples/led_remote/\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);

  //
  // Setup and configure rf radio
  //

  radio.begin();
  // Max power 
radio.setPALevel( RF24_PA_MIN ) ; 
 
// Min speed (for better range I presume)
radio.setDataRate( RF24_250KBPS ) ; 
 
// 8 bits CRC
radio.setCRCLength( RF24_CRC_8 ) ; 

  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens a single pipes for these two nodes to communicate
  // back and forth.  One listens on it, the other talks to it.

    radio.openReadingPipe(1,pipe);

  //
  // Start listening
  //

  if ( role == role_led )
    radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();


  //Blink leds
    int i = num_led_pins;
    while(i--)
    {
      pinMode(led_pins[i],OUTPUT);
      led_states[i] = HIGH;
      digitalWrite(led_pins[i],led_states[i]);  
      delay(350);
      led_states[i] = LOW;
      digitalWrite(led_pins[i],led_states[i]);
     delay (350);
    }
}
//
// Loop
//

void loop(void)
{
 delay (5);

  //
  // LED role.  Receive the state of all buttons, and reflect that in the LEDs
  //

  if ( role == role_led )
  {
    // if there is data ready
    if ( radio.available() )
    {
    Serial.println("got something");
      // Dump the payloads until we've gotten everything
      while (radio.available())
      {
        // Fetch the payload, and see if this was the last one.
        radio.read( button_states, num_button_pins );

        // Spew it
         // For each button, if the button now on, then toggle the LED
        int i = num_led_pins;
        while(i--)
        {
          digitalWrite(led_pins[i], button_states[i]);
        }
      }
    }
  }
}
// vim:ai:cin:sts=2 sw=2 ft=cpp

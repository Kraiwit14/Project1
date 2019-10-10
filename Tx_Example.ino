// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <SPI.h>
#include <Keypad.h>     // For Test 4x4 Keypad
#include <RH_RF95.h>

/* for feather32u4 
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
*/

// for feather m0  
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

/* for shield 
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 7
*/

/* Feather 32u4 w/wing
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     2    // "SDA" (only SDA/SCL/RX/TX have IRQ!)
*/

/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/

#if defined(ESP8266)
  /* for ESP w/featherwing */ 
  #define RFM95_CS  2    // "E"
  #define RFM95_RST 16   // "D"
  #define RFM95_INT 15   // "B"

#elif defined(ESP32)  
  /* ESP32 feather w/wing */
  #define RFM95_RST     27   // "A"
  #define RFM95_CS      33   // "B"
  #define RFM95_INT     12   //  next to A

#elif defined(NRF52)  
  /* nRF52832 feather w/wing */
  #define RFM95_RST     7   // "A"
  #define RFM95_CS      11   // "B"
  #define RFM95_INT     31   // "C"
  
#elif defined(TEENSYDUINO)
  /* Teensy 3.x w/wing */
  #define RFM95_RST     9   // "A"
  #define RFM95_CS      10   // "B"
  #define RFM95_INT     4    // "C"
#endif

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// For test 4x4 Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {24, 19, 18, 17};
byte colPins[COLS] = {1, 0, 16, 23};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

char a[6], b[6];
int answer;
bool a_state, b_state, action_state; 
uint8_t count = 0;
uint8_t action = 0;

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }

  delay(100);

  Serial.println("Start...");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  a_state = false;
  b_state = false;
  action_state = false;
}

uint16_t packetnum;

void loop() {
  
  char key = keypad.getKey();
  char radiopacket[20];
  
  if (key == 'A' && !action_state && a_state) {
    action = 1;
    Serial.print("+ ");
    action_state = true;
  } else if (key == 'B' && !action_state && a_state) {
    action = 2;
    Serial.print("- ");
    action_state = true;
  } else if (key == 'C' && !action_state && a_state) {
    action = 3;
    Serial.print("x ");
    action_state = true;
  } else if (key == 'D' && !action_state && a_state) {
    action = 4;
    Serial.print("/ ");
    action_state = true;
  } else if (key == '*') {
    // clear a, b, answer
    // clear state
    strcpy(a, "00000");
    strcpy(b, "00000");
    answer = 0;
    a_state = false;
    b_state = false;
    action_state = false;
    Serial.println(" ");
    
  } else if (key == '#' && a_state && b_state && action_state) {
    if (action == 1) {
      answer = atoi(a) + atoi(b); 
    } else if (action == 2) {
      answer = atoi(a) - atoi(b);
    } else if (action == 3) {
      answer = atoi(a) * atoi(b);
    } else if (action == 4) {
      answer = atoi(a) / atoi(b);
    }

    Serial.print("= ");
    Serial.println(answer);
    
    sprintf(radiopacket, "%d", answer);
    radiopacket[19] = 0;
    
    Serial.println("Sending...");
    delay(10);
    
    rf95.send((uint8_t *)radiopacket, 11);
    
    Serial.println("Waiting for packet to complete..."); 
    delay(10);
    
    rf95.waitPacketSent();
    
    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    Serial.println("Waiting for reply...");
    if (rf95.waitAvailableTimeout(1000)) { 
    // Should be a reply message for us now   
      if (rf95.recv(buf, &len)) {
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);    
      } else {
        Serial.println("Receive failed");
      }
    } else {
      Serial.println("No reply, is there a listener around?");
    }
    // Clear
    strcpy(a, "00000");
    strcpy(b, "00000");
    answer = 0;
    a_state = false;
    b_state = false;
    action_state = false;
    Serial.println(" ");
    
    delay(1000);
  } else if (key >= 48 && key <= 57) {
    if (!a_state && !b_state) {
      if (count < 4) {
        a[count] = key;
        Serial.print(key);
        count++;
      } else {
        a[count] = key;
        a[count+1] = '\0';
        Serial.print(key);
        count = 0;
        Serial.print(" ");
        a_state = true; 
      }
    } else if (a_state && !b_state && action_state) {
      if (count < 4) {
        b[count] = key;
        Serial.print(key);
        count++;
      } else {
        b[count] = key;
        b[count+1] = '\0';
        Serial.print(key);
        count = 0;
        Serial.print(" ");
        b_state = true;
      }
    } 
  }
  

  /*
  delay(1000); // Wait 1 second between transmits, could also 'sleep' here!
  Serial.println("Transmitting..."); // Send a message to rf95_server
  
  char radiopacket[20] = "Hello World #      ";
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[19] = 0;
  
  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)radiopacket, 20);

  Serial.println("Waiting for packet to complete..."); 
  delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
  */
}

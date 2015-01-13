/*
  The Emotional City
  -Searches twitter for all tweets within 15 miles of a location (Glen Elynn in this case), submits to Alchemy API
  -and plots the resulting sentiment on an RGB LED light strip.
  hscott.net/the-emotional-city for more.
  
  By Hunter Scott
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 21 May 2011
 modified 9 Apr 2012
 Original ethernet code by Tom Igoe
 
 This code is in the public domain.
 
 */
#include <SPI.h>
#include <Ethernet.h>
#include "LPD8806.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif


// Number of RGB LEDs in strand:
int nLEDs = 159;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;

float lastNumber = .5;
float diff = 0;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// You can optionally use hardware SPI for faster writes, just leave out
// the data and clock pin parameters.  But this does limit use to very
// specific pins on the Arduino.  For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.
//LPD8806 strip = LPD8806(nLEDs);


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01 };
IPAddress ip(192,168,1,20);

// initialize the library instance:
EthernetClient client;

const unsigned long requestInterval = 300000;  // delay between requests
//const unsigned long requestInterval = 60000;  // delay between requests


char serverName[] = "access.alchemyapi.com";  // twitter URL

boolean requested;                   // whether you've made a request since connecting
unsigned long lastAttemptTime = 0;            // last time you connected to the server, in milliseconds

String currentLine = "";            // string to hold the text from server
String tweet = "";                  // string to hold the tweet
boolean readingTweet = false;       // if you're currently reading the tweet

void setup() {
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  
  
  // reserve space for the strings:
  currentLine.reserve(256);
  tweet.reserve(150);

 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  // attempt a DHCP connection:
  Serial.println("Attempting to get an IP address using DHCP:");
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
    Serial.println("failed to get an IP address using DHCP, trying manually");
    Ethernet.begin(mac, ip);
  }
   // colorWipe(strip.Color(127, 127, 127), 1000);
//if all numbers are the same, it's white
//127,0,0 = blue red green
//0,127,0 = green blue red,
//0,0,127 = red green blue
//127,127,0 = blue blue green

//strip is divided in 3 subsegments
// 127 is always blue


  Serial.print("My address:");
  Serial.println(Ethernet.localIP());
  colorWipe(strip.Color(50,50,50), 1000); //alert that we got an IP, in case a serial terminal isn't open
  colorWipe(strip.Color(0,0,0), 1000);
  //colorWipe(strip.Color(127,127,127), 1000);
  //( 0 - 50) green
  // (50 -127) white
  // (127-255 ) red
  // connect to Twitter:
  connectToServer();
}



void loop()
{
  
  if (client.connected()) {
    if (client.available()) {
      // read incoming bytes:
      char inChar = client.read();

      // add incoming byte to end of line:
      currentLine += inChar; 

      // if you get a newline, clear the line:
      if (inChar == '\n') {
        currentLine = "";
      } 
      // if the current line ends with <text>, it will
      // be followed by the tweet:
      if ( currentLine.endsWith("<score>")) {
        // tweet is beginning. Clear the tweet string:
        readingTweet = true; 
        tweet = "";
      }
      // if you're currently reading the bytes of a tweet,
      // add them to the tweet String:
      if (readingTweet) {
        if (inChar != '<') {
          tweet += inChar;
        } 
        else {
          // if you got a "<" character,
          // you've reached the end of the tweet:
          readingTweet = false;
          tweet = tweet.substring(1, tweet.length());
          //Serial.println(tweet); 
         char carray[tweet.length() + 1]; //determine size of the array
         tweet.toCharArray(carray, sizeof(carray)); //put readStringinto an array
         float f = atof(carray); //convert the array into a float
         Serial.println("Score is");
         Serial.println(f,7);
         //diff = abs(f-lastNumber)/((f+lastNumber)/2); //differential change from 0 to 1
         diff  = abs((f-lastNumber)/lastNumber);        
         Serial.println(diff);
         lastNumber = f;
         int scale = ((35)*(diff+1))/2;
         if(scale>35) scale = 35;
         //int scale = 53*diff;
         //lastNumber = f;
         Serial.print("Lighting segments: ");
         Serial.println(scale);
         colorWipePartial(strip.Color(127,127,127), 50, scale); //This fills the first 5 segments. Total number of segments is 53.  
          // close the connection to the server:
          client.stop(); 
        }
      }
    }   
  }
  else if (millis() - lastAttemptTime > requestInterval) {
    // if you're not connected, and two minutes have passed since
    // your last connection, then attempt to connect again:
    connectToServer();
  }
  
  
   

  
}

void connectToServer() {
  // attempt to connect, and wait a millisecond:
  Serial.println("connecting to server...");
  if (client.connect(serverName, 80)) {
    Serial.println("making HTTP request...");
    // make HTTP GET request to twitter:
    client.println("GET /calls/url/URLGetTextSentiment?apikey=ALCHEMY-API-KEY-HERE&url=https://twitter.com/search?q=near%3A%22Glen%20Ellyn%2C%20IL%22%20within%3A15mi&src=typd HTTP/1.1");
    client.println("HOST: access.alchemyapi.com");
    client.println("Connection: close");
    client.println();
  }
  // note the time of this connect attempt:
  lastAttemptTime = millis();
}   

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 384; j++) {     // 3 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 384));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Fill the dots progressively along the strip.
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Fill the dots progressively along the strip.
void colorWipePartial(uint32_t c, uint8_t wait, int number) {
  int i;

  for (i=0; i < number; i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Chase one dot down the full strip.
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  // Start by turning all pixels off:
  for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c); // Set new pixel 'on'
    strip.show();              // Refresh LED states
    strip.setPixelColor(i, 0); // Erase pixel, but don't refresh!
    delay(wait);
  }

  strip.show(); // Refresh to turn off last pixel
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 384; j++) {     // cycle all 384 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 384));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}
/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}
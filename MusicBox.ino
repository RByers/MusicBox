// include SPI, MP3 and SD libraries
  
#include <WiFi101.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include "secrets.h"

#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

WiFiClient client;
IPAddress lutronBridge(192,168,86,43);

bool active = false;
bool flushed = true;
const unsigned LIGHT_ON = 50;
const unsigned LIGHT_OFF = 30;
const unsigned WATER_DOWN = 100;
const unsigned WATER_UP = 300;

void log(String msg) {
  Serial.println(msg);  
}

void setup() {
  //Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);

  Serial.begin(115200);

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  // Set volume for left, right channels. lower numbers == louder volume!
  // Right channel has noise, just use left.
  musicPlayer.setVolume(1,100);
  
  // If DREQ is on an interrupt pin we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  // BJ4.MP3
  // TITLE.MP3
  // JUMP.MP3
  // DAYO.MP3  
  //musicPlayer.playFullFile("/dayo.mp3");  
}

String WiFiStatus(int status) {
  switch(status) {
  case WL_IDLE_STATUS:
    return "WL_IDLE_STATUS";  
  case WL_NO_SSID_AVAIL:
    return "WL_NO_SSID_AVAIL";  
  case WL_CONNECTED:
    return "WL_CONNECTED";  
  case WL_CONNECT_FAILED:
    return "WL_CONNECT_FAILED";  
  case WL_CONNECTION_LOST:
    return "WL_CONNECTION_LOST";  
  case WL_DISCONNECTED:
    return "WL_DISCONNECTED";  
  default:
    return String(status);
  }
}

void activate() {
  log("Activated");
  // Attempt to connect to WiFi network
  if (WiFi.status() != WL_CONNECTED) {
    log(String("Trying to connect to SSID: ") + SECRET_SSID);
    int status = WiFi.begin(SECRET_SSID, SECRET_PASS);
    if (status == WL_CONNECTED) {
      log(String("Connected to WiFi, RSSI:") + WiFi.RSSI());
    } else {
      log("WiFi connect failed: " + WiFiStatus(status));
    }
  }

  if (client.connect(lutronBridge, 23)) {
    // Initial login - seems to always fail
    client.println("foo");
    client.println("bar");
    // Actual login
    client.println("lutron");
    client.println("integration");
    // Connection/login takes a while, don't block reading
    log("Connection complete");
  } else {
    log("Connection failed");
  }
}

void deactivate() {
  log("Deactivated");
  client.stop();
  WiFi.disconnect();
}

void lightOff() {
  if (client.connected()) {
    client.println("#OUTPUT,4,1,0,0:00");
  }
}

void lightOn() {
  if (client.connected()) {
    client.println("#OUTPUT,4,1,100,0:05");
  }  
}

void loop() {
  // Save power in the common case
  if (!active)
    delay(1000);

  // Average readings over 0.2 second
  unsigned light = 0;
  unsigned water = 0;
  for(int i = 0; i < 4; i++) {
    light += analogRead(A0);
    water += analogRead(A1);
    delay(50);
  }
  light /= 4;
  water /= 4;
  //Serial.println(light);

  if (active) {
    if (light < LIGHT_OFF) {
      musicPlayer.stopPlaying();
      deactivate();
      active = false;
    }
  } else {
    if (light > LIGHT_ON) {
      musicPlayer.startPlayingFile("/DAYO.MP3");
      activate();
      active = true;
    }
  }

  if (flushed) {
    if (water > WATER_UP) {
      flushed = false;
    }
  } else {
    if (water < WATER_DOWN) {
      if (active) {
        musicPlayer.stopPlaying();
        lightOff();
        delay(1000);
        musicPlayer.playFullFile("/BJ-ZADA.MP3");
        delay(2000);
        lightOn();
        musicPlayer.startPlayingFile("/JUMP.MP3");
        delay(5000); // time for lights to turn back on
      }
      flushed = true;      
    }
  }  
}

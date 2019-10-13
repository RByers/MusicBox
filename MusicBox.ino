// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

void setup() {
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

bool active = false;
bool flushed = true;
const unsigned LIGHT_ON = 50;
const unsigned LIGHT_OFF = 30;
const unsigned WATER_DOWN = 100;
const unsigned WATER_UP = 300;

void loop() {
  // Average readings over    second
  unsigned light = 0;
  unsigned water = 0;
  for(int i = 0; i < 10; i++) {
    light += analogRead(A0);
    water += analogRead(A1);
    delay(100);
  }
  light /= 10;
  water /= 10;
  Serial.println(light);

  if (active) {
    if (light < LIGHT_OFF) {
      musicPlayer.stopPlaying();
      active = false;
    }
  } else {
    if (light > LIGHT_ON) {
      musicPlayer.startPlayingFile("/DAYO.MP3");
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
        musicPlayer.playFullFile("/BJ-ZADA.MP3");
        delay(1000);
        musicPlayer.startPlayingFile("/JUMP.MP3");
      }
      flushed = true;      
    }
  }  
}

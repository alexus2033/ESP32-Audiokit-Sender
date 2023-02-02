/**
 * @file audiokit-bluetooth-sender.ino
 * @author alexus2033
 * @brief SD-Card AudioPlayer with Bluetooth sender
 * 
 * @version 0.1
 * @date 2023-01-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#define AUDIOKIT_BOARD 5
#define SD_CARD_INTR_GPIO 34
#define HELIX_LOGGING_ACTIVE false

#include "AudioTools.h"
#include "AudioLibs/AudioA2DP.h"
#include "AudioLibs/AudioSourceSDFAT.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioLibs/AudioKit.h"

//additional serial output
#define DEBUG 1

#if DEBUG == 1
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x)
  #define debugln(x)
#endif

const char* device = "Supergroove";

int ledPin = 0;
bool bt_connected = false;
bool blinker = false;
int playerIDX = -1;
byte playPos = 0;
unsigned long dTimer = 0;
int buffer_count = 30;
int buffer_size = 512;

char displayName[60];
const char *startFilePath="/";
const char* ext="mp3";
AudioKitStream kit;
SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS, DEDICATED_SPI, SD_SCK_MHZ(20) , &AUDIOKIT_SD_SPI);
AudioSourceSDFAT source(startFilePath, ext, sdcfg);
MP3DecoderHelix decoder;

//Setup synchronized buffer
SynchronizedNBuffer<uint8_t> buffer(buffer_size,buffer_count, portMAX_DELAY, 10);
QueueStream<uint8_t> out(buffer); // convert Buffer to Stream
AudioPlayer player(source, out, decoder);

BluetoothA2DPSource a2dp;

// Provide data to A2DP
int32_t get_data(uint8_t *data, int32_t bytes) {
   size_t result_bytes = buffer.readArray(data, bytes);
   //LOGI("get_data_channels %d -> %d of (%d)", bytes, result_bytes , buffer.available());
   return result_bytes;
}

void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // setup I2S based on sampling rate provided by decoder
  decoder.setNotifyAudioChange(kit);

  // start QueueStream
  out.begin();

  // setup player
  player.setDelayIfOutputFull(0);
  player.setMetadataCallback(player_metadata_callback);
  player.setVolume(0.5);
  player.begin();

  // fill buffer with some data
  player.getStreamCopy().copyN(5); 

  kit.addAction(PIN_KEY1, btnStopResume);
  // kit.addAction(PIN_KEY2, previous); //GPIO 19, unstable
  kit.addAction(PIN_KEY3, btnPrevious);
  kit.addAction(PIN_KEY4, btnRewind);
  kit.addAction(PIN_KEY5, btnForward);
  kit.addAction(PIN_KEY6, btnNext);
  ledPin = kit.pinGreenLed();
  if(ledPin>0){
      pinMode(ledPin, OUTPUT);
      digitalWrite(ledPin, LOW); //inverted output
  }
  while(!SDCard_Available){
    delay(1000);
  }
  // start a2dp source
  Serial.println("starting A2DP...");
  a2dp.set_local_name(device); 
  a2dp.set_on_connection_state_changed(connection_state_changed);
  a2dp.start_raw(get_data);  
  Serial.println("Started");
}

///  MAIN LOOP  ///
void loop() {
  player.copy();
  kit.processActions();
  if(!bt_connected){
    blinkLED();
  } else if(playerIDX != source.index()){
    updateTitleInfo(); //title changed
  } else {
    updatePosition();
  }
}

void connection_state_changed(esp_a2d_connection_state_t state, void *ptr){
  if(state == ESP_A2D_CONNECTION_STATE_CONNECTED){
    Serial.println("Audio Source connected!");
    bt_connected = true;
    if (!player.isActive()){
      player.play();
    }
    if(ledPin>0){
      digitalWrite(ledPin,HIGH);
    }
  } else {
    Serial.println("Disconnected!");
    bt_connected = false;
    if (player.isActive()){
      player.stop();
    }
  }
}

void player_metadata_callback(MetaDataType type, const char* str, int len){
  debug("META ");
  debug(type);
  debug(" (");
  debug(toStr(type));
  debug("): ");
  debugln(str);
}

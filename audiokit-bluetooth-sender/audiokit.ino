bool SDCard_Available(){
  int detectSDPin = SD_CARD_INTR_GPIO;
  if(detectSDPin>0 && digitalRead(detectSDPin) == 0){
    return true;
  }
  Serial.println("Please insert SD-Card");
  if(ledPin>0)
    digitalWrite(ledPin,LOW);
  return false;
}

void btnStopResume(bool, int, void*){
  if (player.isActive()){
    debugln("Player pause");
    player.stop();
  } else{
    player.play();
  } 
}

void btnNext(bool, int, void*) {
  debugln("Player next");
  player.next();
}

void btnPrevious(bool, int, void*) {
  if(!bt_connected) return;
  debugln("Player prev");
  player.previous();
}

void btnSkip(bool pinStatus, int, void*) {
  if(!bt_connected) return;
  debug(pinStatus);
  debugln(" Skip frames");
  source.seek(4096);
}

void blinkLED(){
  if(ledPin > 0 && millis() - ledTimer > 888){
    ledTimer = millis();
    blinker = !blinker;
    if(blinker == true){
      digitalWrite(ledPin,HIGH);
    } else {
      digitalWrite(ledPin,LOW);
    }
  }  
}

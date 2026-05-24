#pragma once
#include <Arduino.h>

class TTP229 {
private:
  int scl;
  int sda;

public:
  TTP229() {}
  TTP229(int cl, int da): scl(cl), sda(da) {
    pinMode(cl, OUTPUT);
    pinMode(da, INPUT);  
  }

  byte readKeypad(void) {
    byte count;
    byte key_state = 0;

    for (count = 1; count <= 16; count++) {
      digitalWrite(scl, LOW);

      if (!digitalRead(sda)) {
        key_state = count;  
      }

      digitalWrite(scl, HIGH);
    }

    return key_state;
  }
  
};

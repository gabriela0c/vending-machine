#pragma once
#define FORWARDS 2
#define BACKWARDS 1
#define STOP 0

#include <Arduino.h>

class DcMotor {
private:
  int input1;
  int input2;

  bool usable;

public:
  DcMotor();
  DcMotor(int inp1, int inp2);
  ~DcMotor();
  
  void runMotor(int direction = FORWARDS);
  void runForMs(int direction, int sleep);
};

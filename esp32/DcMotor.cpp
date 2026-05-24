#include "DcMotor.h"

DcMotor::DcMotor() {
  usable = 0;  
}

DcMotor::DcMotor(int inp1, int inp2):
//enable(en),
input1(inp1),
input2(inp2)
{
  Serial.println("adasdasd");
  usable = 1;
//  enable = en;
//  input1 = inp1;
//  input2 = inp2;
  
  //pinMode(en, OUTPUT);
  pinMode(inp1, OUTPUT);
  pinMode(inp2, OUTPUT);

  //digitalWrite(en, LOW);
  digitalWrite(inp1, LOW);
  digitalWrite(inp2, LOW);
}

DcMotor::~DcMotor() {}

void DcMotor::runMotor(int direction) {
  if (!usable) return;
  //digitalWrite(enable, speed); //Depois mudar pra pwm ledcwrite
  if (direction == FORWARDS) {
    digitalWrite(input1, HIGH);
    digitalWrite(input2, LOW);
  } else if (direction == BACKWARDS) {
    digitalWrite(input1, LOW);
    digitalWrite(input2, HIGH);
  } else {
    digitalWrite(input1, LOW);
    digitalWrite(input2, LOW);
  }
}

void DcMotor::runForMs(int direction, int sleep) {
   runMotor(direction);
   delay(sleep);
   runMotor(STOP);
}

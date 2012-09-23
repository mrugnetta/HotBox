// Blocking.pde// -*- mode: C++ -*-
//
// Shows how to use the blocking call runToNewPosition
// Which sets a new target position and then waits until the stepper has
// achieved it.
//
// Copyright (C) 2009 Mike McCauley
// $Id: Blocking.pde,v 1.1 2011/01/05 01:51:01 mikem Exp mikem $

#include <AccelStepper.h>
#include <EEPROM.h>

// Define a stepper and the pins it will use
AccelStepper stepperDolly(1, 9, 8); //step is pin 9, dir is pin 8
AccelStepper stepperPan(1, 10, 11); //step is pin 10, dir is pin 11

//globe vars
int _DOLLY_ACCELERATION = 50;
int _PAN_ACCELERATION = 50;

boolean GOTTASTOP = false;
boolean NOTIFY_DOLLY = false;
boolean NOTIFY_PAN = false;
boolean NO_BOUNDS = false;

int stopCount = 0;
int dollyYes = 0;
int panYes = 0;

int goToThereDolly;
int goToTherePan = -25000;

int newDollySpeed;
int newPanSpeed;

int dollyMinPos = 0;
int dollyMaxPos = 31000;

int panMinPos = -25000;
int panMaxPos = 15000; 

int motorToMove = 4;

//~~~//

void dollyPosCalc(int oneByteDollyPos) {
  NOTIFY_DOLLY = true;
  goToThereDolly = (oneByteDollyPos * (dollyMaxPos / 255));
}

void dollySpeedCalc(int oneByteMoveTime) {
  int finalSpeed = (abs(stepperDolly.currentPosition() - goToThereDolly) / (oneByteMoveTime - 2));
  newDollySpeed = constrain(finalSpeed, 1, 1000);
  stepperDolly.setMaxSpeed(newDollySpeed); 
}

//~~~//

void panPosCalc(int oneBytePanPos) {
  NOTIFY_PAN = true;
  goToTherePan = (map(oneBytePanPos, 0, 255, panMinPos, panMaxPos));
}

void panSpeedCalc(int oneByteMoveTime) {
  int finalSpeed = (abs(stepperPan.currentPosition() - goToTherePan) / oneByteMoveTime);
  newPanSpeed = constrain(finalSpeed, 1, 1000);
  stepperPan.setMaxSpeed(newPanSpeed);  
}

//~~~//

void somethingStopped (String whatStopped) {
  if (NOTIFY_DOLLY == true && NOTIFY_PAN == true) {
    if (whatStopped == "dolly") {
      stopCount = stopCount + 1;
      dollyYes = 1;
    } else if (whatStopped == "pan") {
      stopCount = stopCount + 1;
      panYes = 1;
    }
      
    if (stopCount >= 2 && dollyYes == 1 && panYes == 1) {
      Serial.print(3);
      //~~
      stopCount = 0;
      dollyYes = 0;
      panYes = 0;
      //~~
      NOTIFY_PAN = false;
      NOTIFY_DOLLY = false;
    }
  } 
   
  if (NOTIFY_DOLLY == true || NOTIFY_PAN == true) { 
    if (stopCount == 0) {
      if (whatStopped == "dolly") {
        Serial.print(2);
        NOTIFY_DOLLY = false;
      } else if (whatStopped == "pan") {
        Serial.print(1);
        NOTIFY_PAN = false;
      }
    }
  }
}

//~~~//

void setup()
{ 
  Serial.begin(9600);   

  stepperDolly.setMaxSpeed(1000.0);
  stepperDolly.setAcceleration(_DOLLY_ACCELERATION);
  stepperDolly.setCurrentPosition(0);

  stepperPan.setMaxSpeed(1000.0);
  stepperPan.setAcceleration(_PAN_ACCELERATION);
  stepperPan.setCurrentPosition(-25000);
}

void loop()
{   
  // read the sensor:
  while ((Serial.available() > 1)) {

    int motorID = Serial.read();    //ID #1 == Dolly | ID #2 == Pan

    //SHUT. DOWN. EVERYTHING.
    if (motorID == 0) {
      NO_BOUNDS = false;
      motorToMove = 4;
      goToThereDolly = stepperDolly.currentPosition();
      goToTherePan = stepperPan.currentPosition();
    }

    //~~~~~~~~~GENERAL MOVEMENT~~~~~~~~~~//

    //Dolly Pos
    if (motorID == 1) {
      int motorDollyPos = Serial.read();   //One byte in, 0 - 26000 out
      dollyPosCalc(motorDollyPos);
    }

    //Dolly Speed
    if (motorID == 2) {
      int moveDollyTime = Serial.read();   //One byte in, 0 - 1000 out
      dollySpeedCalc(moveDollyTime);
    }

    //~~~~//

    //Pan Pos
    if (motorID == 3) {
      int motorPanPos = Serial.read();   //One byte in, 0 - 40000 out
      panPosCalc(motorPanPos);
    }

    //Pan Speed
    if (motorID == 4) {
      int movePanTime = Serial.read();   //One byte in, 0 - 1000 out
      panSpeedCalc(movePanTime);
    }

    //~~~~~~~~~NO BOUNDS CONTROL~~~~~~~~~~//

    //Dolly and Pan Move Left and Right NO BOUNDS
    if (motorID == 5) {

      int whichMotorWhichWay = Serial.read(); 
      
      NO_BOUNDS = true;    

      if (whichMotorWhichWay == 0 || whichMotorWhichWay == 1) {
        motorToMove = 0;
        if (whichMotorWhichWay == 0) {
          newDollySpeed = 1000;
          goToThereDolly = 32000;
        } else if (whichMotorWhichWay == 1) {
          newDollySpeed = 1000;
          goToThereDolly = -32000;
        }
      } else if (whichMotorWhichWay == 2 || whichMotorWhichWay == 3)
        motorToMove = 1;
        if (whichMotorWhichWay == 2) {
          newPanSpeed = 1000;
          goToTherePan = 30000;
        } else if (whichMotorWhichWay == 3) {
          newPanSpeed = 1000;
          goToTherePan = -  30000;
        }
    }  

    //~~~~~~~~~SET 0 OR 255~~~~~~~~~~//

    //Dolly and Pan Set 0s and 255s
    if (motorID == 6) {
      
      int setWhatWhere = Serial.read();
      
      if (setWhatWhere == 0) {
        stepperDolly.setCurrentPosition(0);
        goToThereDolly = 0;
      } else if (setWhatWhere == 1) {
        dollyMaxPos = stepperDolly.currentPosition();
      } else if (setWhatWhere == 2) {
        stepperPan.setCurrentPosition(-25000);
        goToTherePan = -25000;    
      } else if (setWhatWhere == 3) {
        stepperPan.setCurrentPosition(15000);
        goToTherePan = 15000;    
      }
    }  
  
      //~~~~~~~~~PRINT POSITION~~~~~~~~~~//

    //PRINT POSITION OF THE PAN MOTOR
    if (motorID == 7) {  
      int rightnowPosition = stepperPan.currentPosition();
      Serial.print(rightnowPosition);        
      Serial.print('-');        
    }
  }
  //~~//  
  
    if (NO_BOUNDS == true) {
      if (motorToMove == 0) {
        stepperDolly.move(goToThereDolly);
      } else if (motorToMove == 1) {
        stepperPan.move(goToTherePan);
      }
    } else {
      stepperDolly.moveTo(goToThereDolly);
      stepperPan.moveTo(goToTherePan);
    }
    
  //~~//  

  if (GOTTASTOP != true) { 
    if (stepperDolly.distanceToGo() != 0) {
      stepperDolly.run();
    } else if (stepperDolly.distanceToGo() == 0 && NOTIFY_DOLLY == true) {
      somethingStopped("dolly");
    }

    if (stepperPan.distanceToGo() != 0) {
      stepperPan.run();
    } else if (stepperPan.distanceToGo() == 0 && NOTIFY_PAN == true) {
      somethingStopped("pan");
    }
  }    
}


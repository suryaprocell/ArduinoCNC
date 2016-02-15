//CNC router
//
//Code to run on the command computer see:
//commands-plotter.c
//
//Source & info: www.HomoFaciens.de


//#include <avr/io.h>
//#include <util/delay.h>
#include <Servo.h> 

//Change values below to adapt your motor

#define STEP_MARGIN 1L       //10 - 1000 (2)
#define MIN_DUTYCYCLE 75     //0 - 255 (125)
#define MAX_DUTYCYCLE 255    //0 - 255 (255)

#define M1_SENSOR_1 12       //Sensor input 1
#define M1_SENSOR_2 13       //Sensor input 2
#define M1_DIRECTION 4       //Direction output motor
#define M1_PWM 5             //PWM output motor

#define M2_SENSOR_1 10       //Sensor input 1
#define M2_SENSOR_2 11       //Sensor input 2
#define M2_DIRECTION 2       //Direction output motor
#define M2_PWM 3             //PWM output motor

#define M3_SENSOR_1 A4       //Sensor input 1
#define M3_SENSOR_2 A5       //Sensor input 2
#define M3_DIRECTION 7       //Direction output motor
#define M3_PWM 6             //PWM output motor

#define M4_SENSOR_1 A3       //Sensor input 1
#define M4_SENSOR_2 A2       //Sensor input 2
#define M4_DIRECTION 8       //Direction output motor
#define M4_PWM 9             //PWM output motor


#define RELAY A1          //Relay used to turn on milling machine


int M1Sensor02 = 0;
int M1Sensor01 = 0;
int M1DutyCycle = 100; // 10 - 255
unsigned long M1SetPoint = 0;
unsigned long M1IsPoint = 0;
byte M1Step = 0;
byte M1StepDone = 0;

int M2Sensor02 = 0;
int M2Sensor01 = 0;
int M2DutyCycle = 100; // 10 - 255
unsigned long M2SetPoint = 0;
unsigned long M2IsPoint = 0;
byte M2Step = 0;
byte M2StepDone = 0;

int M3Sensor02 = 0;
int M3Sensor01 = 0;
int M3DutyCycle = 100; // 10 - 255
unsigned long M3SetPoint = 0;
unsigned long M3IsPoint = 0;
byte M3Step = 0;
byte M3StepDone = 0;

int M4Sensor02 = 0;
int M4Sensor01 = 0;
int M4DutyCycle = 100; // 10 - 255
unsigned long M4SetPoint = 0;
unsigned long M4IsPoint = 0;
byte M4Step = 0;
byte M4StepDone = 0;

byte readByte = 0;


unsigned long stepTime = 1;

unsigned long millisStart = 0;


void establishContact() {
  while (Serial.available() <= 0) {
    Serial.print('X');   // send a capital X
    delay(300);
  }
}

void setup(){ 

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, 0);
  
  pinMode(M1_DIRECTION, OUTPUT);     
  pinMode(M1_PWM, OUTPUT);     
  analogWrite(M1_PWM, 0);
  digitalWrite(M1_DIRECTION, 0);
  
  pinMode(M1_SENSOR_1, INPUT);
  pinMode(M1_SENSOR_2, INPUT);


  pinMode(M2_DIRECTION, OUTPUT);     
  pinMode(M2_PWM, OUTPUT);     
  analogWrite(M2_PWM, 0);
  digitalWrite(M2_DIRECTION, 0);
  
  pinMode(M2_SENSOR_1, INPUT);
  pinMode(M2_SENSOR_2, INPUT);
  
  pinMode(M3_DIRECTION, OUTPUT);     
  pinMode(M3_PWM, OUTPUT);     
  analogWrite(M3_PWM, 0);
  digitalWrite(M3_DIRECTION, 0);
  
  pinMode(M3_SENSOR_1, INPUT);
  pinMode(M3_SENSOR_2, INPUT);

  pinMode(M4_DIRECTION, OUTPUT);     
  pinMode(M4_PWM, OUTPUT);     
  analogWrite(M4_PWM, 0);
  digitalWrite(M4_DIRECTION, 0);
  
  pinMode(M4_SENSOR_1, INPUT);
  pinMode(M4_SENSOR_2, INPUT);
  
  stepTime = 1;
  readByte = 0;
  
  M1Step = 0;
  M2Step = 0;
  M3Step = 0;
  M4Step = 0;

  M1StepDone = 1;
  M2StepDone = 1;
  M3StepDone = 1;
  M4StepDone = 1;
  
  M1SetPoint = 300000;
  M1IsPoint  = 300000;

  M2SetPoint = 300000;
  M2IsPoint  = 300000;

  M3SetPoint = 300000;
  M3IsPoint =  300000;

  M4SetPoint = 300000;
  M4IsPoint =  300000;
  // start serial port at 115200 bps:
  Serial.begin(115200);
  establishContact();  // send a byte to establish contact until receiver responds 
}


void loop(){  

  millisStart = millis();
  while(millis() - millisStart < stepTime){
    do{
      M1Sensor01 = digitalRead(M1_SENSOR_1);
      M1Sensor02 = digitalRead(M1_SENSOR_2);
    
      if(M1Sensor02 == 0 && M1Sensor01 == 0){
        if(M1Step == 1){
          M1IsPoint--;
        }
        if(M1Step == 3){
          M1IsPoint++;
        }
        M1Step = 0;
      }
    
      if(M1Sensor02 == 1 && M1Sensor01 == 0){
        if(M1Step == 0){
          M1IsPoint++;
        }
        if(M1Step == 2){
          M1IsPoint--;
        }
        M1Step = 1;
      }
    
      if(M1Sensor02 == 1 && M1Sensor01 == 1){
        if(M1Step == 3){
          M1IsPoint--;
        }
        if(M1Step == 1){
          M1IsPoint++;
        }
        M1Step = 2;
      }
    
      if(M1Sensor02 == 0 && M1Sensor01 == 1){
        if(M1Step == 2){
          M1IsPoint++;
        }
        if(M1Step == 0){
          M1IsPoint--;
        }
        M1Step = 3;
      }

      if(abs(M1SetPoint - M1IsPoint) < STEP_MARGIN){
        analogWrite(M1_PWM, 0);
        digitalWrite(M1_DIRECTION, 0);
        M1StepDone = 1;
        M1DutyCycle = MIN_DUTYCYCLE;
      }
      else{
        if(M1IsPoint < M1SetPoint){
          digitalWrite(M1_DIRECTION, 1);
          analogWrite(M1_PWM, 255 - M1DutyCycle);
        }
        if(M1IsPoint > M1SetPoint){
          digitalWrite(M1_DIRECTION, 0);
          analogWrite(M1_PWM, M1DutyCycle);
        }
      }

      M2Sensor01 = digitalRead(M2_SENSOR_1);
      M2Sensor02 = digitalRead(M2_SENSOR_2);
    
      if(M2Sensor02 == 0 && M2Sensor01 == 0){
        if(M2Step == 1){
          M2IsPoint--;
        }
        if(M2Step == 3){
          M2IsPoint++;
        }
        M2Step = 0;
      }
    
      if(M2Sensor02 == 1 && M2Sensor01 == 0){
        if(M2Step == 0){
          M2IsPoint++;
        }
        if(M2Step == 2){
          M2IsPoint--;
        }
        M2Step = 1;
      }
    
      if(M2Sensor02 == 1 && M2Sensor01 == 1){
        if(M2Step == 3){
          M2IsPoint--;
        }
        if(M2Step == 1){
          M2IsPoint++;
        }
        M2Step = 2;
      }
    
      if(M2Sensor02 == 0 && M2Sensor01 == 1){
        if(M2Step == 2){
          M2IsPoint++;
        }
        if(M2Step == 0){
          M2IsPoint--;
        }
        M2Step = 3;
      }

      if(abs(M2SetPoint - M2IsPoint) < STEP_MARGIN){
        analogWrite(M2_PWM, 0);
        digitalWrite(M2_DIRECTION, 0);
        M2StepDone = 1;
        M2DutyCycle = MIN_DUTYCYCLE;
      }
      else{
        if(M2IsPoint < M2SetPoint){
          digitalWrite(M2_DIRECTION, 1);
          analogWrite(M2_PWM, 255 - M2DutyCycle);
        }
        if(M2IsPoint > M2SetPoint){
          digitalWrite(M2_DIRECTION, 0);
          analogWrite(M2_PWM, M2DutyCycle);
        }
      }

      M3Sensor01 = digitalRead(M3_SENSOR_1);
      M3Sensor02 = digitalRead(M3_SENSOR_2);
    
      if(M3Sensor02 == 0 && M3Sensor01 == 0){
        if(M3Step == 1){
          M3IsPoint--;
        }
        if(M3Step == 3){
          M3IsPoint++;
        }
        M3Step = 0;
      }
    
      if(M3Sensor02 == 1 && M3Sensor01 == 0){
        if(M3Step == 0){
          M3IsPoint++;
        }
        if(M3Step == 2){
          M3IsPoint--;
        }
        M3Step = 1;
      }
    
      if(M3Sensor02 == 1 && M3Sensor01 == 1){
        if(M3Step == 3){
          M3IsPoint--;
        }
        if(M3Step == 1){
          M3IsPoint++;
        }
        M3Step = 2;
      }
    
      if(M3Sensor02 == 0 && M3Sensor01 == 1){
        if(M3Step == 2){
          M3IsPoint++;
        }
        if(M3Step == 0){
          M3IsPoint--;
        }
        M3Step = 3;
      }

      if(abs(M3SetPoint - M3IsPoint) < STEP_MARGIN){
        analogWrite(M3_PWM, 0);
        digitalWrite(M3_DIRECTION, 0);
        M3StepDone = 1;
        M3DutyCycle = MIN_DUTYCYCLE;
      }
      else{
        if(M3IsPoint < M3SetPoint){
          digitalWrite(M3_DIRECTION, 1);
          analogWrite(M3_PWM, 255 - M3DutyCycle);
        }
        if(M3IsPoint > M3SetPoint){
          digitalWrite(M3_DIRECTION, 0);
          analogWrite(M3_PWM, M3DutyCycle);
        }
      }

      M4Sensor01 = digitalRead(M4_SENSOR_1);
      M4Sensor02 = digitalRead(M4_SENSOR_2);
    
      if(M4Sensor02 == 0 && M4Sensor01 == 0){
        if(M4Step == 1){
          M4IsPoint--;
        }
        if(M4Step == 3){
          M4IsPoint++;
        }
        M4Step = 0;
      }
    
      if(M4Sensor02 == 1 && M4Sensor01 == 0){
        if(M4Step == 0){
          M4IsPoint++;
        }
        if(M4Step == 2){
          M4IsPoint--;
        }
        M4Step = 1;
      }
    
      if(M4Sensor02 == 1 && M4Sensor01 == 1){
        if(M4Step == 3){
          M4IsPoint--;
        }
        if(M4Step == 1){
          M4IsPoint++;
        }
        M4Step = 2;
      }
    
      if(M4Sensor02 == 0 && M4Sensor01 == 1){
        if(M4Step == 2){
          M4IsPoint++;
        }
        if(M4Step == 0){
          M4IsPoint--;
        }
        M4Step = 3;
      }

      if(abs(M4SetPoint - M4IsPoint) < STEP_MARGIN){
        analogWrite(M4_PWM, 0);
        digitalWrite(M4_DIRECTION, 0);
        M4StepDone = 1;
        M4DutyCycle = MIN_DUTYCYCLE;
      }
      else{
        if(M4IsPoint < M4SetPoint){
          digitalWrite(M4_DIRECTION, 1);
          analogWrite(M4_PWM, 255 - M4DutyCycle);
        }
        if(M4IsPoint > M4SetPoint){
          digitalWrite(M4_DIRECTION, 0);
          analogWrite(M4_PWM, M4DutyCycle);
        }
      }
      
    }while(M1StepDone == 0 || M2StepDone == 0 || M3StepDone == 0 || M4StepDone == 0);
    if(readByte == 0){
      millisStart = 0L;
    }
  }//while(millis() - millisStart < stepTime){
  
  readByte = 0;
  if (Serial.available() > 0){//if we get a valid byte, read from serial:
    //get incoming byte:
    readByte = Serial.read();
    Serial.print('r');   // send a 'r' to initiate next data from computer
  }//if (Serial.available() > 0){


  
  if(readByte > 0){
    if(readByte == 255){
      digitalWrite(RELAY, 1);
      readByte = 0;
    }
    if(readByte == 254){
      digitalWrite(RELAY, 0);
      readByte = 0;
    }

    if(readByte == 128){
      if(stepTime < 1000){
        stepTime+=10;
      }
    }
    if(readByte == 64){
      if(stepTime > 10){
        stepTime-=10;
      }
    }
    if(readByte == 32){
      M1SetPoint++;
      M1DutyCycle = MAX_DUTYCYCLE;
      M1StepDone = 0;
    }
    if(readByte == 16){
      M1SetPoint--;
      M1DutyCycle = MAX_DUTYCYCLE;
      M1StepDone = 0;
    }
    if(readByte == 8){
      M2SetPoint++;
      M2DutyCycle = MAX_DUTYCYCLE;
      M2StepDone = 0;
    }
    if(readByte == 4){
      M2SetPoint--;
      M2DutyCycle = MAX_DUTYCYCLE;
      M2StepDone = 0;
    }
    if(readByte == 2){
      M3SetPoint++;
      M4SetPoint++;
      M3DutyCycle = MAX_DUTYCYCLE;
      M4DutyCycle = MAX_DUTYCYCLE;
      M3StepDone = 0;
      M4StepDone = 0;
    }
    if(readByte == 1){
      M3SetPoint--;
      M4SetPoint--;
      M3DutyCycle = MAX_DUTYCYCLE;
      M4DutyCycle = MAX_DUTYCYCLE;
      M3StepDone = 0;
      M4StepDone = 0;
    }
      
  }//if(readByte > 0){

}



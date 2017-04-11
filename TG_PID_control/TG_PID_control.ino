/*
  openWM--2016
  testing PID regulation of speed control
  using arduino nano with ATmega328
  Main board Rev.5
  Triac module Rev.4
*/
#include "TimerOne.h"
#include "Adafruit_MCP23017.h" //for expander
#include "PID_v1.h"

Adafruit_MCP23017 mcp;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 1, 2, 1.5, REVERSE);//d=0.75

volatile int crosszero_i = 0; // Variable to use as a counter volatile as it is in an interrupt
volatile boolean zero_cross = 0; // Boolean to store a "switch" to tell us if we have crossed zero
// Dimming level (0-128)  0 = on, 128 = 0ff  50-veryfast
int inc = 1;                    // counting up or down, 1=up, -1=down
int freqStep = 75;    // This is the delay-per-brightness step in microseconds.

double dim = 130;
int TG_data ; //tachometric data

uint8_t motor_pin = 11; // Output to Opto Triac (motor control)
uint8_t crossZero_arduino = 3; //interrupt for corss zero detection (control motor) (set pin mode)
uint8_t crossZero = 1; //interrupt for corss zero detection (control motor)
uint8_t PTC_pin = 12; //ptc doorlock
uint8_t mcp_RR1_pin = 15; //relay1 rotor1
uint8_t mcp_RR2_pin = 14; //relay2 rotor2
uint8_t mcp_HR3_pin = 13; //relay3 heating

void setup() {
  Serial.begin(115200);
  pinMode(motor_pin, OUTPUT);
  pinMode(PTC_pin, OUTPUT);//-------PTC
  //start mcp expander:
  mcp.begin(); // use default address 0
  mcp.setupInterrupts(true, false, LOW); //access to interrupts
  mcp.pinMode(mcp_RR1_pin, OUTPUT);
  mcp.pinMode(mcp_RR2_pin, OUTPUT);
  mcp.pinMode(mcp_HR3_pin, OUTPUT);
  //attach interrupt on arduino
  attachInterrupt(crossZero , zero_cross_detect, RISING);
  Timer1.initialize(freqStep);// Initialize TimerOne library for the freq
  Timer1.attachInterrupt(dim_check, freqStep);
  //set relays to right position
  mcp.digitalWrite(mcp_HR3_pin, HIGH);//off heating
  //set rotor relays:
  mcp.digitalWrite(mcp_RR2_pin, LOW);
  mcp.digitalWrite(mcp_RR1_pin, HIGH);
  delay (1000);
  digitalWrite(PTC_pin, HIGH);
  delay (5000);
  //set PID__________________________________
  //initialize the variables we're linked to
  //TG_data = analogRead(1);
  //Input = map(analogRead(1), 0, 1023, 0, 130);
  Input = analogRead(1);
  Setpoint = 120;
  //myPID.SetSampleTime(10);
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(85, 130);
  //myPID.SetControllerDirection(REVERSE);
}

void loop() {
  Serial.print(Input);
  Serial.print("   /  ");
  Serial.print(dim);
  Serial.println("   /  ");
  //Serial.println(TG_data);
  //TG_data = analogRead(1);
  Input = analogRead(1); //map(TG_data, 0, 800, 130, 75);
  myPID.Compute();
  dim = Output;

}

void zero_cross_detect() {
  zero_cross = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  crosszero_i = 0;
  digitalWrite(motor_pin, LOW);       // turn off TRIAC (and AC)
}

// Turn on the TRIAC at the appropriate time
void dim_check() {
  if (zero_cross == true) {
    if (crosszero_i >= dim) {
      digitalWrite(motor_pin, HIGH); // turn on light
      crosszero_i = 0; // reset time step counter
      zero_cross = false; //reset zero cross detection
    }
    else {
      crosszero_i++; // increment time step counter
    }
  }
}

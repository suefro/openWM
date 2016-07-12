/*
  openWM--2016
  testing simple washing program
  using arduino nano with ATmega328
*/

//includes:
#include "TimerOne.h"
#include "SPI.h"
#include "Adafruit_GFX.h" //display graphic
#include "Adafruit_PCD8544.h"
#include "Wire.h"
#include "Adafruit_MCP23017.h" //for expander
#include "RTClib.h" //for RTC time module
#include "math.h" //for NTC thermometer

//rename library:
Adafruit_MCP23017 mcp;
RTC_DS1307 rtc;
Adafruit_PCD8544 display = Adafruit_PCD8544(4, 5, 6, 13, 8); //set display pins

//global variables:
volatile int crosszero_i = 0; // Variable to use as a counter volatile as it is in an interrupt
volatile boolean zero_cross = 0; // Boolean to store a "switch" to tell us if we have crossed zero
int dim = 130;                    // Dimming level (0-128)  0 = on, 128 = 0ff  50-veryfast
int inc = 1;                    // counting up or down, 1=up, -1=down
int freqStep = 75;    // This is the delay-per-brightness step in microseconds.
unsigned long TG_data; //data from tachometric generator
unsigned long PS_data; //data from preassure switch
double NTC_temp; //data from NTC thermistor
long dispb; //timing for display backlight
long time_motor;
volatile boolean INTsig = false; //signal flag from interrupt
String openWM_ver = "1.0";//verze firmware openWM
int change_direction = 0;
int test = 0;//just for testing!!!!!!!!!!!!!!(remove later)
long temp_heat = 25;

//define pins:
int motor_pin = 11; // Output to Opto Triac (motor control)
int TG_pin = 7; //(signal in) from tachometric generator
int controlINT_arduino = 2; //interrupt for buttons and rotary encoder (set pin mode)
int crossZero_arduino = 3; //interrupt for corss zero detection (control motor) (set pin mode)
int contorlINT = 0; //interrupt for buttons and rotary encoder
int crossZero = 1; //interrupt for corss zero detection (control motor)
int PS_pin = 10; //PS-input from preassure switch
int PTC_pin = 12; //ptc doorlock
int Buzz_pin = 9; //piezo buzzer pin
int NTCtherm_pin = 0; //analog ntc thermistor pin
int mcp_BL_pin = 7; //backlight for display
int mcp_RR1_pin = 15; //relay1 rotor1
int mcp_RR2_pin = 14; //relay2 rotor2
int mcp_HR3_pin = 13; //relay3 heating
int mcp_drainPump_pin = 12; //drain pump
int mcp_Valve1_pin = 10; //valve1 washing
int mcp_Valve2_pin = 11; //vlave2 prewash
int mcp_CM3_pin = 6; //change mode pin-using jumper select IO pin(dt_rotary encoder) or relay4
int mcp_PLD_pin = 9; //power load detection
int mcp_DP_pin = 8; //WM drum posistion
int mcp_DT_pin = 6; //rotary encoder DT pin
int mcp_CLK_pin = 5; //rotary encoder CLK pin
int mcp_BT0_pin = 0; //Button 0
int mcp_BT1_pin = 1; //Button 1
int mcp_BT2_pin = 2; //Button 2
int mcp_BT3_pin = 3; //Button 3
int mcp_BT4_pin = 4; //Button 4 --on rotary encoder

void setup() {
  Serial.begin(9600);
  //set interrupts pins:
  pinMode(controlINT_arduino, INPUT);
  pinMode(crossZero_arduino, INPUT);
  //set arduino pins:
  pinMode(motor_pin, OUTPUT);
  pinMode(TG_pin, INPUT);
  pinMode(PS_pin, INPUT);
  pinMode(PTC_pin, OUTPUT);
  //start mcp expander:
  mcp.begin(); // use default address 0
  mcp.setupInterrupts(true, false, LOW); //access to interrupts
  //set mcp pins:
  mcp.pinMode(mcp_BL_pin, OUTPUT);
  mcp.pinMode(mcp_RR1_pin, OUTPUT);
  mcp.pinMode(mcp_RR2_pin, OUTPUT);
  mcp.pinMode(mcp_HR3_pin, OUTPUT);
  mcp.pinMode(mcp_drainPump_pin, OUTPUT);
  mcp.pinMode(mcp_Valve1_pin, OUTPUT);
  mcp.pinMode(mcp_Valve2_pin, OUTPUT);
  //mcp.pinMode(mcp_CM3_pin, OUTPUT);use as relay4 control
  mcp.pinMode(mcp_PLD_pin, INPUT);
  mcp.pinMode(mcp_DP_pin, INPUT);
  //set interrupt pins:
  mcp.pinMode(mcp_DT_pin, INPUT);
  mcp.pullUp(mcp_DT_pin, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(mcp_DT_pin, CHANGE); //setup as interrupt pin //original Falling

  mcp.pinMode(mcp_CLK_pin, INPUT);
  mcp.pullUp(mcp_CLK_pin, HIGH);
  mcp.setupInterruptPin(mcp_CLK_pin, CHANGE);//original Falling

  mcp.pinMode(mcp_BT0_pin, INPUT);
  mcp.pullUp(mcp_BT0_pin, HIGH);
  mcp.setupInterruptPin(mcp_BT0_pin, RISING);

  mcp.pinMode(mcp_BT1_pin, INPUT);
  mcp.pullUp(mcp_BT1_pin, HIGH);
  mcp.setupInterruptPin(mcp_BT1_pin, RISING);

  mcp.pinMode(mcp_BT2_pin, INPUT);
  mcp.pullUp(mcp_BT2_pin, HIGH);
  mcp.setupInterruptPin(mcp_BT2_pin, RISING);

  mcp.pinMode(mcp_BT3_pin, INPUT);
  mcp.pullUp(mcp_BT3_pin, HIGH);
  mcp.setupInterruptPin(mcp_BT3_pin, RISING);

  mcp.pinMode(mcp_BT4_pin, INPUT);
  mcp.pullUp(mcp_BT4_pin, HIGH);
  mcp.setupInterruptPin(mcp_BT4_pin, RISING);
  //attach interrupt on arduino
  attachInterrupt(contorlINT, intCallBack, FALLING);
  attachInterrupt(crossZero , zero_cross_detect, RISING);
  Timer1.initialize(freqStep);// Initialize TimerOne library for the freq
  Timer1.attachInterrupt(dim_check, freqStep);

  //set relays to right position
  mcp.digitalWrite(mcp_HR3_pin, HIGH);//off heating
  //set rotor relays:
  mcp.digitalWrite(mcp_RR2_pin, LOW);
  mcp.digitalWrite(mcp_RR1_pin, HIGH);
  delay (1000);

  //welcome screen
  tone(Buzz_pin, 3500, 500);
  delay (1000);
  noTone(Buzz_pin);
  display.begin();// init done
  display.setContrast(50); //set contrast
  mcp.digitalWrite(mcp_BL_pin, HIGH); //turn on backlight
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.println(" OpenWM 2016");
  display.println("");
  display.println("Firmware");
  display.print("version:");
  display.print(openWM_ver);
  display.display();
  delay(5000);
  display.clearDisplay();   // clears the screen and buffer

  dispb = rtc.now().unixtime(); //save unixtime to variable
}
//-------------------------main loop-----------------
void loop() {
  // NTC thermistor data:
  NTC_temp = Thermistor(analogRead(NTCtherm_pin));
  //preassure data:
  PS_data = (619751.5 / pulseIn(PS_pin, HIGH));
  EIFR = 0x01;
  DateTime now = rtc.now();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Time:");
  display.print(now.hour(), DEC);
  display.print(":");
  display.print(now.minute(), DEC);
  display.print(":");
  display.println(now.second(), DEC);
  display.print("TEMP:");
  display.print(NTC_temp);
  display.print("/");
  display.print(temp_heat);
  display.print("PS:");
  display.println(PS_data);
  display.print("Drum pos:");
  display.println(mcp.digitalRead(mcp_DP_pin));
  display.print("Rot.enc:");
  display.println(test);
  //display.print("Motor:");
  //display.println(dim);
  //display.print("Press 0 for start");

  display.display();

  if (INTsig) handleInterrupt(now); //control interrupt status

  //backlight off
  if (now.unixtime() - dispb > 30)
  {
    mcp.digitalWrite(mcp_BL_pin, LOW);
  }


}
//----------------------------------------------------

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
//callback for buttons and rotary encoder
void intCallBack() {
  INTsig = true;
}

void handleInterrupt(DateTime now) {
  EIFR = 0x01;
  detachInterrupt(contorlINT);
  dispb = now.unixtime(); //timeout
  mcp.digitalWrite(mcp_BL_pin, HIGH);
  // Get more information from the MCP from the INT
  uint8_t pin = mcp.getLastInterruptPin();
  uint8_t val = mcp.getLastInterruptPinValue();
  if (pin == mcp_CLK_pin)
  {
    test++;
    //dim++;
    temp_heat++;
  }

  if (pin == mcp_DT_pin)
  {
    test--;
    //dim--;
    temp_heat--;
  }

  if (pin == mcp_BT0_pin)
  {
    test_program();
  }

  if (pin == mcp_BT1_pin)
  {

    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    drum_unload(49);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    drum_load(45);
    //reverse_motor (0);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    heating(temp_heat, 30, 40 );
    washing_motor (100);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    drum_position();
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    /*drum_load(45);
      reverse_motor (1);
      tone(Buzz_pin, 3500, 600);
      delay (2000);
      noTone(Buzz_pin);
      washing_motor (600);
      tone(Buzz_pin, 3500, 600);
      delay (2000);
      noTone(Buzz_pin);
      drum_unload(49);*/
    drum_unload(49);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
  }
  if (pin == mcp_BT2_pin)
  {
    //temp_heat--;
    drum_unload(49);
  }
  if (pin == mcp_BT3_pin)
  {
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    drum_load(45);
    heating(temp_heat, 30, 10);

    digitalWrite(PTC_pin, LOW);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
    tone(Buzz_pin, 3500, 600);
    delay (2000);
    noTone(Buzz_pin);
  }
  if (pin == mcp_BT4_pin)
  {
    dim = 130;
    test = 130;
  }

  while ( ! (mcp.digitalRead(mcp_CLK_pin) && mcp.digitalRead(mcp_DT_pin) ));
  while (  (mcp.digitalRead(mcp_BT0_pin) || mcp.digitalRead(mcp_BT1_pin) || mcp.digitalRead(mcp_BT2_pin) || mcp.digitalRead(mcp_BT3_pin) || mcp.digitalRead(mcp_BT4_pin) ));
  cleanInterrupts();
}

void cleanInterrupts() {
  EIFR = 0x01;
  INTsig = false;
  attachInterrupt(contorlINT, intCallBack, FALLING);

  //delay(300); // test jamming interrupt on expander
}
//----------------------------------------NTC thermometer-------------------------------------------------
/*
  Schematic:
  [Ground] ---- [4.7k-Resistor] -------|------- [Thermistor] ---- [+5v]
                                                             |
                                                     Analog Pin 0
*/
double Thermistor(int RawADC) {
  // Inputs ADC Value from Thermistor and outputs Temperature in Celsius
  //  requires: include <math.h>
  // Utilizes the Steinhart-Hart Thermistor Equation:
  //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
  //    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
  long Resistance;  double Temp;  // Dual-Purpose variable to save space.
  Resistance = 4700 * ((1024.0 / RawADC) - 1); // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 /ADC -1) * BalanceResistor
  // For a GND-Thermistor-PullUp--Varef circuit it would be Rtherm=Rpullup/(1024.0/ADC-1)
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate it 4 times later. // "Temp" means "Temporary" on this line.
  //Serial.print(Temp);
  Temp = 1 / (0.000789787 + (0.000324949 * Temp) + (-0.000000307362 * Temp * Temp * Temp));   // Now it means both "Temporary" and "Temperature"
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius                                         // Now it only means "Temperature"

  // BEGIN- Remove these lines for the function not to display anything
  //Serial.print("ADC: "); Serial.print(RawADC); Serial.print("/1024");  // Print out RAW ADC Number
  //Serial.print(", Volts: "); printDouble(((RawADC*4.860)/1024.0),3);   // 4.860 volts is what my USB Port outputs.
  // Serial.print(", Resistance: "); Serial.print(Resistance); Serial.print("ohms");
  // END- Remove these lines for the function not to display anything

  // Uncomment this line for the function to return Fahrenheit instead.
  //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert to Fahrenheit
  return Temp;  // Return the Temperature
}

void speedcontrol(int TG_low, int TG_high) { //simple speed control test

  TG_data = pulseIn(TG_pin, HIGH); //read freq from TG (pulses not freq in Hz)

  if (TG_data > 75)
  {
    if (TG_data >= 1 && TG_data <= 1000)
    {
      //Serial.println("out of range!");
    }
    else
    {
      if (TG_data < TG_low && TG_data > TG_high) //in acceptable range
      {
        //ok
        //Serial.println("Control OK");
      }
      else
      {
        if (TG_data > TG_low) //higher number means lower speed
        {
          if (dim < 75)
          {
            //Serial.println("min value!");
          }
          else
          {
            dim -= 1;
            delay(10);
            //Serial.println(dim);
          }
        }
        if (TG_data < TG_high) //lower number means higher speed
        {
          if (dim > 110)
          {
            //Serial.println("max value!");
          }
          else
          {
            dim += 1;
            delay(10);
            //Serial.println(dim);
          }
        }
      }

      delay(5);
    }
  }
  else
  {

    if (TG_data < 2000 && dim > 75)
    {
      dim--;//this-different load needs different value 83
    }

  }
}

void drum_load(long level) { //make better diagnostic
  //lock door
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Lock door!");
  display.display();
  delay (5000); //waiting for PTC door lock (because bimetal needs time)
  if (mcp.digitalRead(mcp_PLD_pin) == 0)
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);

    while (619751.5 / (pulseIn(PS_pin, HIGH)) > level) //level 45-46 full
    {
      while (619751.5 / (pulseIn(PS_pin, HIGH)) > level) //level 45-46 full
      {
        display.clearDisplay();
        display.setCursor(0, 20);
        display.println("Load:");
        display.print(619751.5 / (pulseIn(PS_pin, HIGH)));
        display.print("/");
        display.println(level);
        display.display();
        mcp.digitalWrite(mcp_Valve1_pin, HIGH); //valve1 open
      }
      mcp.digitalWrite(mcp_Valve1_pin, LOW);
      time_motor = rtc.now().unixtime();
      while (rtc.now().unixtime() - time_motor < 20)
      {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Tacho:");
        display.println(TG_data);
        display.print("Motor:");
        display.println(dim);
        display.print("Direction:");
        display.println(change_direction);
        display.print("PS:");
        display.println(619751.5 / (pulseIn(PS_pin, HIGH)));
        display.print("Time:");
        display.print(rtc.now().unixtime() - time_motor);
        display.display();
        speedcontrol(1800, 1500); //low level higher number, high level lower number
      }
      dim = 130;
      delay(2000);

    }
    mcp.digitalWrite(mcp_Valve1_pin, LOW); //valve1 closed
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Load done!");
    display.display();
    delay (2000);

  }
  else {
    digitalWrite(PTC_pin, LOW);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Error-door open!");
    display.display();
    delay (5000);
  }
  //digitalWrite(PTC_pin, LOW); //no needs because next function want locked door
}

void heating (long temp, int time_heating, int time_motor_set) { //heating water (just for test rotate drum better mixing water)
  //lock door
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Check lock door!");
  display.display();
  delay (5000);
  if (mcp.digitalRead(mcp_PLD_pin) == 0 && 619751.5 / (pulseIn(PS_pin, HIGH)) < 47.5  )
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);

    dispb = rtc.now().unixtime();
    while (Thermistor(analogRead(NTCtherm_pin)) < temp )//&& 619751.5 / (pulseIn(PS_pin, HIGH)) < 48
    {

      if (rtc.now().unixtime() - dispb < time_heating)
      {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Heating:");
        display.print(Thermistor(analogRead(NTCtherm_pin)));
        display.print("/");
        display.println(temp);
        display.print("PS:");
        display.println(619751.5 / (pulseIn(PS_pin, HIGH)));
        display.print("Time:");
        display.print(rtc.now().unixtime() - dispb);
        display.display();
        mcp.digitalWrite(mcp_HR3_pin, LOW); //heating on

      }

      else
      {

        reverse_motor(change_direction);

        time_motor = rtc.now().unixtime();
        while (rtc.now().unixtime() - time_motor < time_motor_set)
        {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Tacho:");
          display.println(TG_data);
          display.print("Motor:");
          display.println(dim);
          display.print("Direction:");
          display.println(change_direction);
          display.print("PS:");
          display.println(619751.5 / (pulseIn(PS_pin, HIGH)));
          display.print("Time:");
          display.print(rtc.now().unixtime() - time_motor);
          display.display();
          speedcontrol(1800, 1500); //low level higher number, high level lower number
        }

        dispb = rtc.now().unixtime(); //timeou
        dim = 130;

        if (change_direction == 0) {
          change_direction = 1;
        }
        else {
          change_direction = 0;
        }

      }
      //Serial.print(Thermistor(analogRead(NTCtherm_pin)));
      //Serial.print("-------");
      //Serial.println(619751.5 / (pulseIn(PS_pin, HIGH)));
    }
    dim = 130;
    mcp.digitalWrite(mcp_HR3_pin, HIGH); //heating off
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Heating done!");
    display.display();
    delay (2000);

  }
  else {
    drum_unload(49);
    digitalWrite(PTC_pin, LOW);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Error!");
    display.println("Doorlock open");
    display.println("Low water level");
    display.display();
    delay (5000);
  }
  //digitalWrite(PTC_pin, LOW);
}

void drum_unload (long level) { //unloading drum
  //lock door
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Check door!");
  display.display();
  delay (5000);
  if (mcp.digitalRead(mcp_PLD_pin) == 0 )//&& 619751.5 / (pulseIn(PS_pin, HIGH)) < 49
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);
    while (619751.5 / (pulseIn(10, HIGH)) < level)
    {
      while (619751.5 / (pulseIn(10, HIGH)) < level)
      {
        display.clearDisplay();
        display.setCursor(0, 20);
        display.println("Unloading:");
        display.println(619751.5 / (pulseIn(PS_pin, HIGH)));
        display.display();
        mcp.digitalWrite(mcp_drainPump_pin, HIGH); //drain on
      }
      mcp.digitalWrite(mcp_Valve1_pin, LOW);
      /*time_motor = rtc.now().unixtime();
        while (rtc.now().unixtime() - time_motor < 10)
        {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Tacho:");
        display.println(TG_data);
        display.print("Motor:");
        display.println(dim);
        display.print("Direction:");
        display.println(change_direction);
        display.print("PS:");
        display.println(619751.5 / (pulseIn(PS_pin, HIGH)));
        display.print("Time:");
        display.print(rtc.now().unixtime() - time_motor);
        display.display();
        speedcontrol(1800, 1500); //low level higher number, high level lower number
        }
        dim = 130;*/
      delay(2000);
    }
    delay (10000);
    mcp.digitalWrite(mcp_drainPump_pin, LOW); //drain off
    digitalWrite(PTC_pin, LOW);
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Unload done!");
    display.display();
    delay (2000);
  }
  else
  {
    digitalWrite(PTC_pin, LOW);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Fatal Error!");
    display.println("Doorlock open");
    display.println("Low water level");
    display.display();
    delay (5000);
  }
}

void reverse_motor (int dir) { //reversing motor
  digitalWrite(PTC_pin, LOW);
  if (dir == 0)//forward
  {
    mcp.digitalWrite(mcp_RR2_pin, LOW);
    mcp.digitalWrite(mcp_RR1_pin, HIGH);
  }
  if (dir == 1)//backward
  {
    mcp.digitalWrite(mcp_RR2_pin, HIGH);
    mcp.digitalWrite(mcp_RR1_pin, LOW);
  }
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
}

void drum_position () { //not shure more tests
  dim = 130;
  while (mcp.digitalRead(mcp_DP_pin) == 1)
  {

    while (mcp.digitalRead(mcp_DP_pin) == 1)
    {
      dim--;
      if (dim < 90) dim = 130;

    }
    delay(1000);
    dim = 130;
    delay(2000);
  }
  dim = 130;
  digitalWrite(PTC_pin, LOW);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("Drum OK!");
  display.display();

}

void washing_motor (int time_total) {//dodelat
  //lock door
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Check lock door!");
  display.display();
  delay (5000);
  if (mcp.digitalRead(mcp_PLD_pin) == 0 )//&& 619751.5 / (pulseIn(PS_pin, HIGH)) < 47.5
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);

    dispb = rtc.now().unixtime();
    while (rtc.now().unixtime() - dispb < time_total )
    {

      time_motor = rtc.now().unixtime();
      /*
        if (Thermistor(analogRead(NTCtherm_pin)) < 35)
        {
        dim = 130;
        //heating(temp_heat, 30, 10);
        }
      */
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Tacho:");
      display.println(TG_data);
      display.print("Motor:");
      display.println(dim);
      display.print("Direction:");
      display.println(change_direction);
      display.print("PS:");
      display.println(619751.5 / (pulseIn(PS_pin, HIGH)));
      display.print("Time:");
      display.print(rtc.now().unixtime() - dispb);
      display.display();
      speedcontrol(1600, 1500); //low level higher number, high level lower number
    }


    dim = 130;

    mcp.digitalWrite(mcp_HR3_pin, HIGH); //heating off
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("washing end");
    display.display();
    delay (2000);


  }
  //Serial.print(Thermistor(analogRead(NTCtherm_pin)));
  //Serial.print("-------");
  //Serial.println(619751.5 / (pulseIn(PS_pin, HIGH)));




  else {
    drum_unload(49);
    digitalWrite(PTC_pin, LOW);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    tone(Buzz_pin, 3400, 600);
    delay (2000);
    noTone(Buzz_pin);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Error!");
    display.println("Doorlock open");
    display.println("Low water level");
    display.display();
    delay (5000);
  }
  //digitalWrite(PTC_pin, LOW);
}

void diagnostic_start () {

}

void test_program () { // need better diagnostic later
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  drum_load(46); //loading 46 means level of water

  heating(temp_heat, 30, 10); //heating water 82 menas 82 degree of celsius second number means heating time and thirth menas motor time

  while (Thermistor(analogRead(NTCtherm_pin)) > 60)
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Cooling!");
    display.print("Temp:");
    display.print(Thermistor(analogRead(NTCtherm_pin)));
    display.display();
  }
  drum_position();
  drum_unload(49);//unloading number means level of empty drum
  //drum_load(46);
  //drum_unload(49);

  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
}

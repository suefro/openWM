/*
  openWM--2016
  testing simple washing program
  using arduino nano with ATmega328
  Main board Rev.5
  Triac module Rev.4
*/

//includes:
#include "TimerOne.h"
#include "SPI.h"
#include "Adafruit_GFX.h" //display graphic
#include "Adafruit_PCD8544.h"
#include "Wire.h"
#include "Adafruit_MCP23017.h" //for expander
#include "RTClib.h" //for RTC time module
#include "SensorsWM.h"

//rename library:
Adafruit_MCP23017 mcp;
RTC_DS1307 rtc;
// Software SPI (slower updates, more flexible pin options):
// pin 4 - Serial clock out (SCLK)
// pin 5 - Serial data out (DIN)
// pin 6 - Data/Command select (D/C)
// pin 13 - LCD chip select (CS)
// pin 8 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(4, 5, 6, 7, 8); //set display pins(rev5)

unsigned int dim = 130;
boolean cycle_speed = true;
int cycle_regul = 0;
SensorsWM NTCtemp("4k7", A0, dim, cycle_regul, cycle_speed); //set temperature sensor
SensorsWM PS("L46210", 10, dim, cycle_regul, cycle_speed); //set preassure sensor
SensorsWM TG("L46210", A1, dim, cycle_regul, cycle_speed); //tachometric sensor

//global variables:
unsigned int TG_data; //data from tachometric generator
volatile int crosszero_i = 0; // Variable to use as a counter volatile as it is in an interrupt
volatile boolean zero_cross = 0; // Boolean to store a "switch" to tell us if we have crossed zero
// Dimming level (0-128)  0 = on, 128 = 0ff  50-veryfast
int inc = 1;                    // counting up or down, 1=up, -1=down
int freqStep = 75;    // This is the delay-per-brightness step in microseconds.

long dispb; //timing for display backlight
long time_motor;
volatile boolean INTsig = false; //signal flag from interrupt
String openWM_ver = "1.0";//verze firmware openWM
int change_direction = 0;
long temp_heat = 25;


//define pins:
uint8_t motor_pin = 11; // Output to Opto Triac (motor control)
uint8_t controlINT_arduino = 2; //interrupt for buttons and rotary encoder (set pin mode)
uint8_t crossZero_arduino = 3; //interrupt for corss zero detection (control motor) (set pin mode)
uint8_t controlINT = 0; //interrupt for buttons and rotary encoder
uint8_t crossZero = 1; //interrupt for corss zero detection (control motor)
uint8_t PTC_pin = 12; //ptc doorlock
uint8_t Buzz_pin = 9; //piezo buzzer pin
uint8_t BL_pin = 13; //backlight for display
uint8_t mcp_RR1_pin = 15; //relay1 rotor1
uint8_t mcp_RR2_pin = 14; //relay2 rotor2
uint8_t mcp_HR3_pin = 13; //relay3 heating
uint8_t mcp_R4_pin = 12; //relay4 optional
uint8_t mcp_drainPump_pin = 11; //drain pump
uint8_t mcp_Valve1_pin = 9; //valve1 washing
uint8_t mcp_Valve2_pin = 10; //vlave2 prewash
uint8_t mcp_PLD_pin = 8; //power load detection -------------------here load detection
uint8_t mcp_DP_pin = 7; //WM drum posistion ---------------here drum
uint8_t mcp_DT_pin = 6; //rotary encoder DT pin
uint8_t mcp_CLK_pin = 5; //rotary encoder CLK pin
//uint8_t mcp_BT0_pin = 0; //Button 0
//uint8_t mcp_BT1_pin = 1; //Button 1
//uint8_t mcp_BT2_pin = 2; //Button 2
//uint8_t mcp_BT3_pin = 3; //Button 3
uint8_t mcp_BT4_pin = 4; //Button 4 --on rotary encoder

void setup() {
  Serial.begin(9600);
  pinMode(motor_pin, OUTPUT);

  pinMode(PTC_pin, OUTPUT);//-------PTC
  //start mcp expander:
  mcp.begin(); // use default address 0
  mcp.setupInterrupts(true, false, LOW); //access to interrupts
  //set mcp pins:
  pinMode(BL_pin, OUTPUT);
  mcp.pinMode(mcp_RR1_pin, OUTPUT);
  mcp.pinMode(mcp_RR2_pin, OUTPUT);
  mcp.pinMode(mcp_HR3_pin, OUTPUT);
  mcp.pinMode(mcp_drainPump_pin, OUTPUT);
  mcp.pinMode(mcp_Valve1_pin, OUTPUT);
  mcp.pinMode(mcp_Valve2_pin, OUTPUT);
  //mcp.pinMode(mcp_R4_pin, OUTPUT);relay4 control
  mcp.pinMode(mcp_PLD_pin, INPUT); // ----------PLD
  mcp.pinMode(mcp_DP_pin, INPUT); //------------DP
  //set interrupt pins:
  mcp.pinMode(mcp_DT_pin, INPUT);
  mcp.pullUp(mcp_DT_pin, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(mcp_DT_pin, FALLING); //setup as interrupt pin //original Falling

  mcp.pinMode(mcp_CLK_pin, INPUT);
  mcp.pullUp(mcp_CLK_pin, HIGH);
  mcp.setupInterruptPin(mcp_CLK_pin, FALLING);//original Falling
  /*
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
  */
  mcp.pinMode(mcp_BT4_pin, INPUT);
  mcp.pullUp(mcp_BT4_pin, HIGH);
  mcp.setupInterruptPin(mcp_BT4_pin, RISING);


  //attach interrupt on arduino
  attachInterrupt(controlINT, intCallBack, FALLING);
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
  display.setContrast(43); //set contrast43
  digitalWrite(BL_pin, HIGH); //turn on backlight
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

  //preassure data:
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
  display.print(NTCtemp.temp());
  display.print("/");
  display.println(temp_heat);
  display.print("PS:");
  display.println(PS.preasssw());
  display.print("Drum pos:");
  display.println(mcp.digitalRead(mcp_DP_pin));
  display.display();

  if (INTsig) handleInterrupt(now); //control interrupt status

  //backlight off
  if (now.unixtime() - dispb > 30)
  {
    digitalWrite(BL_pin, LOW);
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
    if (crosszero_i >= TG.getDim()) {
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
  detachInterrupt(controlINT);
  dispb = now.unixtime(); //timeout
  digitalWrite(BL_pin, HIGH);
  // Get more information from the MCP from the INT
  uint8_t pin = mcp.getLastInterruptPin();
  uint8_t val = mcp.getLastInterruptPinValue();
  if (pin == mcp_CLK_pin)
  {
    //dim++;
    temp_heat++;
  }

  if (pin == mcp_DT_pin)
  {
    //dim--;
    temp_heat--;
  }
  /*
    if (pin == mcp_BT0_pin)
    {
      test_program();
    }

    if (pin == mcp_BT1_pin)
    {
      test_program_phases();
    }
    if (pin == mcp_BT2_pin)
    {
      test_program_phases2();
    }
    if (pin == mcp_BT3_pin)
    {

    }
  */
  if (pin == mcp_BT4_pin)
  {
    test_program_phases2();
  }

  while ( ! (mcp.digitalRead(mcp_CLK_pin) && mcp.digitalRead(mcp_DT_pin) ));
  //while (  (mcp.digitalRead(mcp_BT0_pin) || mcp.digitalRead(mcp_BT1_pin) || mcp.digitalRead(mcp_BT2_pin) || mcp.digitalRead(mcp_BT3_pin) || mcp.digitalRead(mcp_BT4_pin) ));
  while (mcp.digitalRead(mcp_BT4_pin));
  cleanInterrupts();

}

void cleanInterrupts() {
  EIFR = 0x01;
  INTsig = false;
  attachInterrupt(controlINT, intCallBack, FALLING);

}


void in_washing() {
  EIFR = 0x01;
  detachInterrupt(controlINT);
  // Get more information from the MCP from the INT
  uint8_t pin = mcp.getLastInterruptPin();
  uint8_t val = mcp.getLastInterruptPinValue();
  if (pin == mcp_CLK_pin)
  {

    dim++;
    //temp_heat++;
  }

  if (pin == mcp_DT_pin)
  {

    dim--;
    //temp_heat--;
  }
  /*
    if (pin == mcp_BT0_pin)
    {

    }

    if (pin == mcp_BT1_pin)
    {

    }
    if (pin == mcp_BT2_pin)
    {

    }
    if (pin == mcp_BT3_pin)
    {
    }
  */
  if (pin == mcp_BT4_pin)
  {
    dim = 130;
  }

  while ( ! (mcp.digitalRead(mcp_CLK_pin) && mcp.digitalRead(mcp_DT_pin) ));
  //  while (  (mcp.digitalRead(mcp_BT0_pin) || mcp.digitalRead(mcp_BT1_pin) || mcp.digitalRead(mcp_BT2_pin) || mcp.digitalRead(mcp_BT3_pin) || mcp.digitalRead(mcp_BT4_pin) ));
  while (mcp.digitalRead(mcp_BT4_pin));
  cleanInterrupts();
}
/*
  void speedcontrol(int TG_low, int TG_high, int dim_min) { //simple speed control test

  TG_data = TG.TachoGen(); //read generating voltage from TG (A/D data)

  if (TG_data < 35 ) //first cycle
  {
    if (TG_data > TG_low && TG_data < TG_high) //in acceptable range
    {
      //ok
      //Serial.println("Control OK");
    }
    else
    {
      if (TG_data < TG_low) //higher number means lower speed
      {
        if (dim < 75)
        {
          cycle_regul = 0;
          //Serial.println("min value!");
        }
        else
        {

          dim -= 1;
          delay(10);
          //Serial.println(dim);

        }
      }
      if (TG_data > TG_high) //lower number means higher speed
      {
        if (dim > 100)
        {
          //Serial.println("max value!");
        }
        else
        {
          if (dim < dim_min - cycle_regul)
          {
            dim += 1;
            delay(10);
            //Serial.println(dim);
          }
        }
      }
    }

    //Serial.println("cycle1");
    delay(100);
  }

  if (TG_data > 40 && TG_data < TG_high - 50 && cycle_speed == true ) //second cycle
  {
    if (TG_data > TG_low && TG_data < TG_high) //in acceptable range
    {
      //ok
      //Serial.println("Control OK");
    }
    else
    {
      if (TG_data < TG_low) //higher number means lower speed
      {
        if (dim < 75)
        {
          cycle_regul = 0;
          //Serial.println("min value!");
        }
        else
        {

          dim -= 1;
          delay(10);
          //Serial.println(dim);

        }
      }
      if (TG_data > TG_high) //lower number means higher speed
      {
        if (dim > 100)
        {
          //Serial.println("max value!");
        }
        else
        {
          if (dim < dim_min - cycle_regul)
          {
            dim += 1;
            delay(10);
            //Serial.println(dim);
          }
        }
      }
    }


    delay(500);
    //Serial.println("cycle2");
  }

  if (TG_data > TG_high) //cycle3
  {
    cycle_speed = false;
    if (dim < dim_min - cycle_regul)
    {
      dim += 1;
    }
    delay(10);
    //Serial.println("cycle3");
  }

  if (TG_data > TG_high + 100) //reduce cycle_regul
  {
    cycle_regul -= 1;
    //Serial.println("cycle3");
  }

  if (TG_data < 50 && cycle_speed == false) //cycle3
  {
    if (90 < dim_min - cycle_regul)
    {
      cycle_regul += 1;
    }

    //Serial.println("cycle_regul");
  }
  if (TG_data > TG_low && TG_data < TG_high && cycle_speed == false) //in acceptable range
  {
    if (TG_data > TG_low + ((TG_high - TG_low) - 10) && TG_data < TG_high - ((TG_high - TG_low) - 10)) //in acceptable range
    {
      //ok
      //Serial.println("Control OK");
    }
    else
    {
      if (TG_data < TG_low + ((TG_high - TG_low) - 10)) //higher number means lower speed
      {
        if (dim < 75)
        {
          cycle_regul = 0;
          //Serial.println("min value!");
        }
        else
        {

          dim -= 1;

          //Serial.println(dim);

        }
      }
      if (TG_data > TG_high - ((TG_high - TG_low) - 10)) //lower number means higher speed
      {
        if (dim > 100)
        {
          //Serial.println("max value!");
        }
        else
        {
          if (dim < dim_min - cycle_regul)
          {
            dim += 1;

            //Serial.println(dim);
          }
        }
      }
    }

    delay(10);
    Serial.println("normal");

  }



  }
*/
void drum_load(long level, long temp) { //make better diagnostic
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

    while ( PS.preasssw() > level) //level 45-46 full
    {
      while ( PS.preasssw() > level) //level 45-46 full
      {
        display.clearDisplay();
        display.setCursor(0, 20);
        display.println("Load:");
        display.print(PS.preasssw());
        display.print("/");
        display.println(level);
        display.display();
        mcp.digitalWrite(mcp_Valve1_pin, HIGH); //valve1 open
      }
      mcp.digitalWrite(mcp_Valve1_pin, LOW);
      time_motor = rtc.now().unixtime();
      if (NTCtemp.temp() < temp)
      {
        mcp.digitalWrite(mcp_HR3_pin, LOW); //heating on
      }
      else
      {
        mcp.digitalWrite(mcp_HR3_pin, HIGH); //heating off
      }
      while (rtc.now().unixtime() - time_motor < 30)
      {


        display.clearDisplay();
        display.setCursor(0, 0);
        //display.print("Tacho:");
        //display.println(TG_data);
        display.print("Motor:");
        display.println(TG.getDim());
        display.print("Direction:");
        display.println(change_direction);
        display.print("PS:");
        display.println(PS.preasssw());
        display.print("Time:");
        display.print(rtc.now().unixtime() - time_motor);
        display.display();
        if (INTsig) in_washing(); //control interrupt status
        //speedcontrol(210, 280, 99); //low level higher number, high level lower number
        TG.speedcontrol(210, 280, 99);
        cycle_regul = TG.getCycle_regul();
        cycle_speed = TG.getCycle_speed();
      }
      TG.setCycle_speed(false);
      TG.setCycle_regul(0);
      TG.setDim(130);
      delay(2000);

    }
    mcp.digitalWrite(mcp_HR3_pin, HIGH); //heating off
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

void heating (long temp, int time_heating, int time_motor_set, long level) { //heating water (just for test rotate drum better mixing water)
  //lock door
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Check lock door!");
  display.display();
  delay (5000);
  if (mcp.digitalRead(mcp_PLD_pin) == 0 && PS.preasssw() < 47.5  )
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);

    dispb = rtc.now().unixtime();
    while (NTCtemp.temp() < temp )//&& PS.preasssw() < 48
    {

      if (rtc.now().unixtime() - dispb < time_heating)
      {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Heating:");
        display.print(NTCtemp.temp());
        display.print("/");
        display.println(temp);
        display.print("PS:");
        display.println(PS.preasssw());
        display.print("Time:");
        display.print(rtc.now().unixtime() - dispb);
        display.display();
        mcp.digitalWrite(mcp_HR3_pin, LOW); //heating on

      }

      else
      {
        if (mcp.digitalRead(mcp_PLD_pin) == 0 && PS.preasssw() < 47.5  )
        {
          while ( PS.preasssw() > level) //level 45-46 full
          {
            mcp.digitalWrite(mcp_Valve1_pin, HIGH); //valve1 open
          }
          mcp.digitalWrite(mcp_Valve1_pin, LOW); //valve1 closed
        }
        reverse_motor(change_direction);

        time_motor = rtc.now().unixtime();
        while (rtc.now().unixtime() - time_motor < time_motor_set)
        {


          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Heating:");
          display.print(NTCtemp.temp());
          display.print("/");
          display.println(temp);
          //display.print("Tacho:");
          //display.println(TG_data);
          display.print("Motor:");
          display.println(TG.getDim());
          display.print("Direction:");
          display.println(change_direction);
          display.print("PS:");
          display.println(PS.preasssw());
          display.print("Time:");
          display.print(rtc.now().unixtime() - time_motor);
          display.display();
          if (INTsig) in_washing(); //control interrupt status
          //speedcontrol(200, 300, 99); //low level higher number, high level lower number
          TG.speedcontrol(200, 300, 99);
          cycle_regul = TG.getCycle_regul();
          cycle_speed = TG.getCycle_speed();

        }

        dispb = rtc.now().unixtime(); //timeou
        TG.setDim(130);

        TG.setCycle_speed(false);
        TG.setCycle_regul(0);

        if (change_direction == 0) {
          change_direction = 1;
        }
        else {
          change_direction = 0;
        }

      }
      //Serial.print(NTCtemp.temp());
      //Serial.print("-------");
      //Serial.println(PS.preasssw());
    }
    TG.setDim(130);
    mcp.digitalWrite(mcp_Valve1_pin, LOW); //valve1 closed
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
  if (mcp.digitalRead(mcp_PLD_pin) == 0 )//&& PS.preasssw() < 49
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
        display.println(PS.preasssw());
        display.display();
        mcp.digitalWrite(mcp_drainPump_pin, HIGH); //drain on
      }
      mcp.digitalWrite(mcp_Valve1_pin, LOW);
      time_motor = rtc.now().unixtime();
      while (rtc.now().unixtime() - time_motor < 30)
      {
        if (INTsig) in_washing(); //control interrupt status

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Unloading..");
        //display.print("Tacho:");
        //display.println(TG_data);
        display.print("Motor:");
        display.println(TG.getDim());
        display.print("Direction:");
        display.println(change_direction);
        display.print("PS:");
        display.println(PS.preasssw());
        display.print("Time:");
        display.print(rtc.now().unixtime() - time_motor);
        display.display();
        //speedcontrol(180, 250, 110);
        TG.speedcontrol(180, 250, 110);
        cycle_regul = TG.getCycle_regul();
        cycle_speed = TG.getCycle_speed();


      }
      TG.setDim(130);
      TG.setCycle_speed(false);
      TG.setCycle_regul(0);
      delay(2000);
    }
    delay (10000);
    mcp.digitalWrite(mcp_drainPump_pin, LOW); //drain off
    //digitalWrite(PTC_pin, LOW);
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
      if (dim < 85) dim = 130; //testing different weight
      delay(1000);
    }

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

void washing_motor (int time_total, int heat, long level) {//dodelat, time in minute
  //lock door
  //Serial.println(mcp.digitalRead(mcp_PLD_pin));

  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Check lock door!");
  display.display();
  delay (5000);
  //Serial.println(mcp.digitalRead(mcp_PLD_pin));

  if (mcp.digitalRead(mcp_PLD_pin) == 0  )//&& PS.preasssw() < 47.5
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);
    if (mcp.digitalRead(mcp_PLD_pin) == 0 && PS.preasssw() < 47.5  )
    {
      while (PS.preasssw() > level) //level 45-46 full
      {
        mcp.digitalWrite(mcp_Valve1_pin, HIGH); //valve1 open
      }
      mcp.digitalWrite(mcp_Valve1_pin, LOW); //valve1 closed
    }
    dispb = rtc.now().unixtime();
    while (rtc.now().unixtime() - dispb < (time_total * 60) )
    {

      //      time_motor = rtc.now().unixtime();

      if (NTCtemp.temp() < (temp_heat - 10) && heat == 1) //if temp fall about 10 celsius, on/off heat(
      {
        TG.setDim(130);
        heating(temp_heat, 30, 60, 45);
      }
      if (INTsig) in_washing(); //control interrupt status


      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Washing...");
      //display.print("Tacho:");
      //display.println(TG_data);
      display.print("RPM:");
      display.println(analogRead(A1));
      //Serial.println(analogRead(A1));
      display.print("Motor:");
      display.println(TG.getDim());
      display.print("Direction:");
      display.println(change_direction);
      display.print("PS:");
      display.println(PS.preasssw());
      display.print("Time:");
      display.print((rtc.now().unixtime() - dispb) / 60);
      display.display();
      //speedcontrol(210, 280, 99); //low level higher number, high level lower number
      TG.speedcontrol(210, 280, 99);
      cycle_regul = TG.getCycle_regul();
      cycle_speed = TG.getCycle_speed();

    }


    TG.setDim(130);
    TG.setCycle_speed(false);
    TG.setCycle_regul(0);
    mcp.digitalWrite(mcp_HR3_pin, HIGH); //heating off
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("washing end");
    display.display();
    delay (2000);


  }
  //Serial.print(NTCtemp.temp());
  //Serial.print("-------");
  //Serial.println(PS.preasssw());




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

void spin_motor (int time_total) { //Spin -odstředění
  //lock door
  digitalWrite(motor_pin, LOW);
  digitalWrite(PTC_pin, HIGH);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Check lock door!");
  display.display();
  delay (5000);
  if (mcp.digitalRead(mcp_PLD_pin) == 0 && PS.preasssw() > 48)
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Sucess!");
    display.display();
    delay (2000);

    dispb = rtc.now().unixtime();
    while (rtc.now().unixtime() - dispb < time_total * 60 )
    {

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Spin...");
      //display.print("Tacho:");
      //display.println(TG_data);
      display.print("Motor:");
      display.println(TG.getDim());
      display.print("Direction:");
      display.println(change_direction);
      display.print("PS:");
      display.println(PS.preasssw());
      display.print("Time:");
      display.print((rtc.now().unixtime() - dispb) / 60);
      display.display();
      //if (PS.preasssw() < 48) drum_unload(49);
      //speedcontrol(600, 700, 110); //low level higher number, high level lower number
      TG.speedcontrol(700, 800, 110);
      cycle_regul = TG.getCycle_regul();
      cycle_speed = TG.getCycle_speed();
    }


    TG.setDim(130);
    TG.setCycle_speed(false);
    TG.setCycle_regul(0);
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print("Spin end");
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
    display.display();
    delay (5000);
  }
  //digitalWrite(PTC_pin, LOW);
}


void diagnostic_start () {

}

void test_program_phases () {

  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_load(45, temp_heat);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  heating(temp_heat, 30, 50, 45 );
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);
  //prani
  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  //machani
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_load(45, temp_heat);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  spin_motor(2);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_load(45, temp_heat);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);
  //spin
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);
  spin_motor(3);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);
  spin_motor(3);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);
  spin_motor(3);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  //drum_position();
  digitalWrite(PTC_pin, LOW);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
}

void test_program_phases2 () {

  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  //napousteni
  drum_load(45, temp_heat);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  //ohrev
  heating(temp_heat, 30, 50, 45 );
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);
  //prani
  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 1, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  //machani
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_load(45, temp_heat);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);
  spin_motor(3);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_load(45, temp_heat);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);

  washing_motor (2, 0, 45);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);
  //spin
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);
  spin_motor(3);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(0);
  spin_motor(3);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  reverse_motor(1);
  spin_motor(3);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);

  drum_unload(49);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  //drum_position();
  digitalWrite(PTC_pin, LOW);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
}

void test_program() { // need better diagnostic later
  tone(Buzz_pin, 3500, 600);
  delay (2000);
  noTone(Buzz_pin);
  drum_unload(49);
  drum_load(46, temp_heat); //loading 46 means level of water

  heating(temp_heat, 30, 10, 45); //heating water 82 menas 82 degree of celsius second number means heating time and thirth menas motor time

  while (NTCtemp.temp() > 60)
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Cooling!");
    display.print("Temp:");
    display.print(NTCtemp.temp());
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

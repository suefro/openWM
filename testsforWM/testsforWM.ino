//complete testing all modules for version WM-mainboard Rev3 2016
//OpenWM --název
/*
-backlight display test
-display test graphic (by Adafruit)
-buzzer test
-relay module test
-triac module test:
 triac switch PTC door lock,valve1,valve2,drain pump
 power load detection
 RTC module
 Preassure switch
 tachometric generator
 motor control
 
 
*/
//by suefro 2016

#include  <TimerOne.h>          // Avaiable from http://www.arduino.cc/playground/Code/Timer1
#include <SPI.h>
#include <Adafruit_GFX.h> //display graphic
#include <Adafruit_PCD8544.h>
#include <Wire.h>
#include "Adafruit_MCP23017.h" //for expander
#include "RTClib.h" //for RTC time module
#include <math.h> //for NTC thermometer

Adafruit_MCP23017 mcp;
RTC_DS1307 rtc;

volatile int i = 0;             // Variable to use as a counter volatile as it is in an interrupt
volatile boolean zero_cross = 0; // Boolean to store a "switch" to tell us if we have crossed zero
volatile int p = 0;
volatile int t = 0;
int AC_pin = 11;                // Output to Opto Triac
int dim = 130;                    // Dimming level (0-128)  0 = on, 128 = 0ff  50-veryfast
int inc = 1;                    // counting up or down, 1=up, -1=down
int freqStep = 75;    // This is the delay-per-brightness step in microseconds.
int TG_pin = 7; //(signal in) from tachometric generator
int TG_high = 1500; //high level
int TG_low = 1520; //low level
//const float TG_tohz = 350000 ; //400000
unsigned long TG_data; //data from tachometric generator
unsigned long PS_data; //data from preassure switch
double NTC_temp; //data from NTC thermistor
boolean unload = false;//test for unload water from WM
int cislo=0;//test for rotary encoder
String stat ="start";
// Software SPI (slower updates, more flexible pin options):
// pin 4 - Serial clock out (SCLK)
// pin 5 - Serial data out (DIN)
// pin 6 - Data/Command select (D/C)
// pin 7 - LCD chip select (CS)
// pin 8 - LCD reset (RST)


//display testing default by adafruit
Adafruit_PCD8544 display = Adafruit_PCD8544(4, 5, 6, 13, 8);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

long dispb;
//interrupt

//byte arduinoIntPin=2;

//byte arduinoInterrupt=0;

volatile boolean INTsig = false;



void setup() {
  Serial.begin(9600);
  //Serial.begin(115200);
  pinMode(2,INPUT);
  pinMode(12, OUTPUT); //ptc doorlock
  pinMode(TG_pin, INPUT); //Tachogen input
  pinMode(10, INPUT); //PS-output from preassure switch
  pinMode(AC_pin, OUTPUT);//motor control

    //start mcp
  mcp.begin();      // use default address 0
  mcp.setupInterrupts(true,false,LOW); //access to interrupts

  mcp.pinMode(7, OUTPUT); //backlight display
  mcp.pinMode(15, OUTPUT);//relay1 rotor1
  mcp.pinMode(14, OUTPUT);//relay2 rotor2
  mcp.pinMode(13, OUTPUT);//relay3 heating
  mcp.pinMode(12, OUTPUT);//drain pump
  mcp.pinMode(10, OUTPUT);//valve1 praní
  mcp.pinMode(11, OUTPUT);//valve2 predepirka
  //mcp.pinMode(6, OUTPUT);//CM3 -IO-relay4 or input
  mcp.pinMode(9, INPUT);//power load detection
  mcp.pinMode(8, INPUT);//location WM drum

  //switches
 //mcp.pinMode(0, INPUT);    
 //mcp.pinMode(1, INPUT);
 //mcp.pinMode(2, INPUT);
 //mcp.pinMode(3, INPUT);
 //mcp.pinMode(4, INPUT);

  //time settings
  //rtc.adjust(DateTime(2016, 4,17,12,55,00));
  //settings interrupt rotary encoder
  
// /*
  mcp.pinMode(6, INPUT);//dt
  mcp.pullUp(6, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(6,CHANGE); 

  mcp.pinMode(5, INPUT);//clk
  mcp.pullUp(5, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(5,CHANGE); 
//*/
// /*
  mcp.pinMode(0, INPUT);//
  mcp.pullUp(0, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(0,RISING); 

  mcp.pinMode(1, INPUT);//
  mcp.pullUp(1, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(1,RISING); 

  mcp.pinMode(2, INPUT);//
  mcp.pullUp(2, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(2,RISING); 

  mcp.pinMode(3, INPUT);//
  mcp.pullUp(3, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(3,RISING); 

  mcp.pinMode(4, INPUT);//
  mcp.pullUp(4, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(4,RISING); 
//   */
  attachInterrupt(0,intCallBack,FALLING);

  pinMode(3, INPUT);
  attachInterrupt(1, zero_cross_detect, RISING);    // Attach an Interupt to Pin 3 (interupt 1) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);
  
  display.begin();
  // init done
  mcp.digitalWrite(7, HIGH);
  // you can change the contrast around to adapt the display
  // for the best viewing!
  display.setContrast(50);

  //start display
  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();   // clears the screen and buffer
/*
  //testing backlight display (blink 5x)
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Testing Backlight!");
  display.display();
  display.clearDisplay();
  
  testbacklight_display();
  
  mcp.digitalWrite(7, HIGH);

  //TESTING DISPLAY:
  
  // draw a single pixel
  display.drawPixel(10, 10, BLACK);
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw many lines
  testdrawline();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw rectangles
  testdrawrect();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw multiple rectangles
  testfillrect();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw mulitple circles
  testdrawcircle();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw a circle, 10 pixel radius
  display.fillCircle(display.width()/2, display.height()/2, 10, BLACK);
  display.display();
  delay(2000);
  display.clearDisplay();

  testdrawroundrect();
  delay(2000);
  display.clearDisplay();

  testfillroundrect();
  delay(2000);
  display.clearDisplay();

  testdrawtriangle();
  delay(2000);
  display.clearDisplay();
   
  testfilltriangle();
  delay(2000);
  display.clearDisplay();

  // draw the first ~12 characters in the font
  testdrawchar();
  display.display();
  delay(2000);
  display.clearDisplay();

  // text display tests
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.setTextColor(WHITE, BLACK); // 'inverted' text
  display.println(3.141592);
  display.setTextSize(2);
  display.setTextColor(BLACK);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.display();
  delay(2000);

  // rotation example
  display.clearDisplay();
  display.setRotation(1);  // rotate 90 degrees counter clockwise, can also use values of 2 and 3 to go further.
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Rotation");
  display.setTextSize(2);
  display.println("Example!");
  display.display();
  delay(2000);

  // revert back to no rotation
  display.setRotation(0);

  // miniature bitmap display
  display.clearDisplay();
  display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.display();

  // invert the display
  display.invertDisplay(true);
  delay(1000); 
  display.invertDisplay(false);
  delay(1000); 
*/
/*
  //Testing BUZZER
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Testing BUZZER!");
  display.display();
  display.clearDisplay();
  delay(2000);
  test_buzzer();
*/
/*
  //Testing relays
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Test relays");
  display.display();
  display.clearDisplay();
  delay(2000);
  test_relays();
 */
  /* 
  //Testing triacswitch

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Test triac switch");
  display.display();
  display.clearDisplay();
  delay(2000);
  test_triacswitch ();
 */
 /*
  //Testing power load detection

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Test power load detction");
  display.display();
  display.clearDisplay();
  delay(2000);
  test_powerloaddet ();
  digitalWrite(12, LOW);
 */
/*
  //Testing wm correct location drum
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Test wmdrum");
  display.display();
  display.clearDisplay();
  delay(2000);
  //test_wmdrum();

*/
//set relays:
   mcp.digitalWrite(13, HIGH);//off heating 
   //set rotor relays:
   mcp.digitalWrite(14, LOW);
   mcp.digitalWrite(15, HIGH);
   delay (1000);
   
   //digitalWrite(12, HIGH);
  //test_triaccomp(); //testing real components
  //test_switch();
  
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("END");
  display.display();
  display.clearDisplay();
  delay(500);

//digitalWrite(12, HIGH);

dispb=rtc.now().unixtime();
  
  

}

void zero_cross_detect() {
  zero_cross = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  i = 0;
  digitalWrite(AC_pin, LOW);       // turn off TRIAC (and AC)
}

// Turn on the TRIAC at the appropriate time
void dim_check() {
  if (zero_cross == true) {
    if (i >= dim) {
      digitalWrite(AC_pin, HIGH); // turn on light
      i = 0; // reset time step counter
      zero_cross = false; //reset zero cross detection
    }
    else {
      i++; // increment time step counter
    }
  }
}

void intCallBack(){
  INTsig=true;
  //Serial.println("Test");
 
  
}

void handleInterrupt(DateTime now){ 
  dispb = now.unixtime(); //timeout
  mcp.digitalWrite(7, HIGH);
  // Get more information from the MCP from the INT
  uint8_t pin=mcp.getLastInterruptPin();
  uint8_t val=mcp.getLastInterruptPinValue();
 
  // We will flash the led 1 or 2 times depending on the PIN that triggered the Interrupt
  // 3 and 4 flases are supposed to be impossible conditions... just for debugging.
  /*
   if  (mcp.digitalRead(5)) // When DAT = HIGH IS FORWARD

  {

   cislo++;

   

  }

else                   // When DAT = LOW IS BackRote

  {

   cislo--;

   

  }
  
  */
/*
  if(pin==0) 
  {
  Serial.println("Unload");  
  //Serial.println(pin);
  //Serial.println(val);
  unload=true;
  }
 */
  if(pin==5) 
  {
  //Serial.println("PinA1");  
  //Serial.println(pin);
  //Serial.println(val);
  cislo--;
  dim--;
    
  }
 /* if(pin==mcpPinB) 
  {
  Serial.println("PinB");
  //Serial.println(pin);
  //Serial.println(val);
  cislo=0;
  //Serial.println(cislo);
  }
  */
  
  
  if(pin==6) 
  {
   

    //Serial.println("PinA2");
    //Serial.println(pin);
    //Serial.println(val);
  cislo++;
  dim++;
  
  }

if(pin==0) 
  {
   

    //Serial.println("PinA2");
    //Serial.println(pin);
    //Serial.println(val);
  //cislo=cislo+10;
  //dim++;
  stat="unload";
  drum_unload();
  stat="start";

  }

  if(pin==1) 
  {
   

    //Serial.println("PinA2");
    //Serial.println(pin);
    //Serial.println(val);
  //cislo=cislo+20;
  //dim++;
  //stat="heating";
  //heating();
  digitalWrite(12, HIGH);
  //stat="start";
  }
   if(pin==2) 
  {
   

    //Serial.println("PinA2");
    //Serial.println(pin);
    //Serial.println(val);
  //cislo=cislo+30;
  //dim++;
  digitalWrite(12, LOW);
  stat="unlock";
  
  }
   if(pin==3) 
  {
   

    //Serial.println("PinA2");
    //Serial.println(pin);
    //Serial.println(val);
  //cislo=cislo+40;
  //dim++;
  stat="loading";
  drum_load();
  //stat="start";
  }
 if(pin==4) 
  {
   

    //Serial.println("PinA2");
    //Serial.println(pin);
    //Serial.println(val);
  cislo=0;
  dim=130;
  
  }
  //Serial.println(cislo);
  //Serial.print(mcp.digitalRead(5));
  //Serial.print("--");

  //Serial.print(mcp.digitalRead(6));
    //Serial.print("--");

  //Serial.println(!(mcp.digitalRead(5) && mcp.digitalRead(6)));


  while( ! (mcp.digitalRead(5) && mcp.digitalRead(6) ));
    while(  (mcp.digitalRead(0) || mcp.digitalRead(1) || mcp.digitalRead(2) || mcp.digitalRead(3) || mcp.digitalRead(4) ));
//Serial.print(mcp.digitalRead(5));
  //Serial.print("--");

  //Serial.println(mcp.digitalRead(6));
  detachInterrupt(0);
  cleanInterrupts();
    }

  void cleanInterrupts(){
  EIFR=0x01;
  INTsig=false;
  
  attachInterrupt(0,intCallBack,FALLING);
} 





//-----------------------------------main loop start-----------------------------------//





void loop() {

  //preassure data:
 // PS_data = pulseIn(10, HIGH);
 //Serial.println(619751.5/PS_data);//adapt to graph in service manual (values from 35-45 Hz)
 /*
 if(p>20)
{
PS_data = pulseIn(20, HIGH);
p=0;
}
p++;
*/
 // NTC thermistor data:
  NTC_temp=Thermistor(analogRead(0)); //analog pin 0
  /*
  digitalWrite(12, HIGH);
  //test cross zero detection and triac switching on light bulb
  dim+=inc;
  if((dim>=110) || (dim<=80))
  inc*=-1;
  delay(1000);
  */
// draw a bitmap icon and 'animate' movement
  //testdrawbitmap(logo16_glcd_bmp, LOGO16_GLCD_WIDTH, LOGO16_GLCD_HEIGHT);

TG_data = pulseIn(TG_pin, HIGH); //read freq from TG (pulses not freq in Hz)
//Serial.println(TG_data);
/*
if(t>10)
{
TG_data=pulseIn(TG_pin, HIGH); //read the current from sensor
t=0;
}
t++;
//TG_data=TG_data/5;
Serial.println(TG_data);
*/
/*
//test simple speed control.
 // Serial.printkn(TG_data);
 // Serial.println("-----");
 if (TG_data > 0)
  {
    if (TG_data >= 1 && TG_data <= 1000)
    {
      Serial.println("out of range!");
    }
    else
    {
      if (TG_data < TG_low && TG_data > TG_high) //in acceptable range
      {
        //ok
        //Serial.println("Control OK");
        noTone(9);
      }
      else
      {
        if (TG_data > TG_low) //higher number means lower speed
        {
          if(dim < 80)
          {
            Serial.println("min value!");
            tone(9,3500,500);
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
          if(dim > 110)
          {
            //Serial.println("max value!");
            tone(9,3000,500);
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
    dim=105;
    /*if(dim <= 115)
    for(int i=0; i<5; i+=1)
    {
    {
    dim -= 1;
    delay(1000);
    }
    }
  }*/


//testing roatry encoder
DateTime now = rtc.now();
//mcp.begin();
display.clearDisplay();
display.setCursor(0,0);
display.println("Hodnota:");
display.print("TEMP:");
display.println(NTC_temp);
display.print("PS:");
display.print(619751.5/PS_data);
display.print(" dr:");
display.print(mcp.digitalRead(8));
display.setCursor(0,25);
display.print("LD:");
display.print(mcp.digitalRead(9));
display.print(" cis:");
display.println(dim);
display.print("TG:");
display.print(TG_data);
display.print(" /");
display.println(stat);

//display.println("Tm:");
/*
display.print(now.hour(),DEC);
display.print(":");
display.print(now.minute(),DEC);
display.print(":");
display.print(now.second(),DEC);
*/
display.display();
Serial.println(TG_data);
//attachInterrupt(0,intCallBack,FALLING);
//detachInterrupt(0);

if(INTsig) handleInterrupt(now);
//detachInterrupt(arduinoInterrupt);

//Serial.println(dispb);

//int time = now.second();

if (now.unixtime()-dispb > 30)
{
  mcp.digitalWrite(7, LOW);
  }





}


//--------------------------------------------------main loop end-----------------------------------------//


//----------------------------------------NTC thermometer-------------------------------------------------//
//Schematic:
// [Ground] ---- [10k-Resistor] -------|------- [Thermistor] ---- [+5v]
//                                                             |
//                                                     Analog Pin 0

double Thermistor(int RawADC) {
 // Inputs ADC Value from Thermistor and outputs Temperature in Celsius
 //  requires: include <math.h>
 // Utilizes the Steinhart-Hart Thermistor Equation:
 //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
 //    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
 long Resistance;  double Temp;  // Dual-Purpose variable to save space.
 Resistance=4700*((1024.0/RawADC) - 1);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 /ADC -1) * BalanceResistor
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

void printDouble(double val, byte precision) {
  // prints val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimal places
  // example: printDouble(3.1415, 2); // prints 3.14 (two decimal places)
  //Serial.print (int(val));  //prints the int part
  if( precision > 0) {
    //Serial.print("."); // print the decimal point
    unsigned long frac, mult = 1;
    byte padding = precision -1;
    while(precision--) mult *=10;
    if(val >= 0) frac = (val - int(val)) * mult; else frac = (int(val) - val) * mult;
    unsigned long frac1 = frac;
    while(frac1 /= 10) padding--;
    while(padding--) ;//Serial.print("0");
    //Serial.print(frac,DEC) ;
  }
}
//-------------------------------testing display-------------------------------------------------//

void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  uint8_t icons[NUMFLAKES][3];
  randomSeed(666);     // whatever seed
 
  // initialize
  for (uint8_t f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS] = random(display.width());
    icons[f][YPOS] = 0;
    icons[f][DELTAY] = random(5) + 1;
    
    //Serial.print("x: ");
    //Serial.print(icons[f][XPOS], DEC);
    //Serial.print(" y: ");
    //Serial.print(icons[f][YPOS], DEC);
    //Serial.print(" dy: ");
    //Serial.println(icons[f][DELTAY], DEC);
  }

  while (1) {
    // draw each icon
    for (uint8_t f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], logo16_glcd_bmp, w, h, BLACK);
    }
    display.display();
    delay(200);
    
    // then erase it + move it
    for (uint8_t f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS],  logo16_glcd_bmp, w, h, WHITE);
      // move it
      icons[f][YPOS] += icons[f][DELTAY];
      // if its gone, reinit
      if (icons[f][YPOS] > display.height()) {
  icons[f][XPOS] = random(display.width());
  icons[f][YPOS] = 0;
  icons[f][DELTAY] = random(5) + 1;
      }
    }
   }
}


void testdrawchar(void) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);

  for (uint8_t i=0; i < 168; i++) {
    if (i == '\n') continue;
    display.write(i);
    //if ((i > 0) && (i % 14 == 0))
      //display.println();
  }    
  display.display();
}

void testdrawcircle(void) {
  for (int16_t i=0; i<display.height(); i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, BLACK);
    display.display();
  }
}

void testfillrect(void) {
  uint8_t color = 1;
  for (int16_t i=0; i<display.height()/2; i+=3) {
    // alternate colors
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, color%2);
    display.display();
    color++;
  }
}

void testdrawtriangle(void) {
  for (int16_t i=0; i<min(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(display.width()/2, display.height()/2-i,
                     display.width()/2-i, display.height()/2+i,
                     display.width()/2+i, display.height()/2+i, BLACK);
    display.display();
  }
}

void testfilltriangle(void) {
  uint8_t color = BLACK;
  for (int16_t i=min(display.width(),display.height())/2; i>0; i-=5) {
    display.fillTriangle(display.width()/2, display.height()/2-i,
                     display.width()/2-i, display.height()/2+i,
                     display.width()/2+i, display.height()/2+i, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
  }
}

void testdrawroundrect(void) {
  for (int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, BLACK);
    display.display();
  }
}

void testfillroundrect(void) {
  uint8_t color = BLACK;
  for (int16_t i=0; i<display.height()/2-2; i+=2) {
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
  }
}
   
void testdrawrect(void) {
  for (int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, BLACK);
    display.display();
  }
}

void testdrawline() {  
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, BLACK);
    display.display();
  }
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, BLACK);
    display.display();
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, BLACK);
    display.display();
  }
  for (int8_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, BLACK);
    display.display();
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, BLACK);
    display.display();
  }
  for (int16_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, BLACK);
    display.display();
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, BLACK);
    display.display();
  }
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, BLACK); 
    display.display();
  }
  delay(250);
  
}

void testbacklight_display ()
{
  for (int i=0 ; i < 5; i++) {
  mcp.digitalWrite(7, HIGH);
  delay (1000);
  mcp.digitalWrite(7, LOW);
  delay (1000);
  }
  
  
}

void test_buzzer ()
{
  
  
  for (int i=0; i < 4000; i=i+10)
  {
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0,0);
    display.println("Testing      BUZZER!");
    display.setCursor(0,20);
    display.print("freq:");
    display.print(i);
    display.display();
    
    display.clearDisplay();
    tone(9,i,500);
    delay (5);
   }  
  noTone(9);
    
  
}

void test_relays ()
{
  for(int i=0; i < 2; i++)
  {
    for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("RELAY 1");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(15, HIGH);
      delay (500);
      mcp.digitalWrite(15, LOW);
      delay (500);
      }
     for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("RELAY 2");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(14, HIGH);
      delay (500);
      mcp.digitalWrite(14, LOW);
      delay (500);
      }
      for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("RELAY 3");
      display.display();
      display.clearDisplay();
      

      mcp.digitalWrite(13, HIGH);
      delay (500);
      mcp.digitalWrite(13, LOW);
      delay (500);
      } 
      for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("RELAY 4");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(6, HIGH);
      delay (500);
      mcp.digitalWrite(6, LOW);
      delay (500);
      }
    
    }


}

void test_triacswitch ()
{
  for(int i=0; i < 2; i++)
  {
    for(int x=0; x < 3; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("drain pump");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(12, HIGH);
      delay (500);
      mcp.digitalWrite(12, LOW);
      delay (500);
      }
     for(int x=0; x < 3; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("valve2");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(11, HIGH);
      delay (500);
      mcp.digitalWrite(11, LOW);
      delay (500);
      }
      for(int x=0; x < 3; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("valve1");
      display.display();
      display.clearDisplay();
      

      mcp.digitalWrite(10, HIGH);
      delay (500);
      mcp.digitalWrite(10, LOW);
      delay (500);
      } 
      for(int x=0; x < 3; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("PTC doorlock");
      display.display();
      display.clearDisplay();
      
      
      digitalWrite(12, HIGH);
      delay (500);
      digitalWrite(12, LOW);
      delay (500);
      }
    
    }

    digitalWrite(12, HIGH);
    
for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("drain pump");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(12, HIGH);
      delay (500);
      mcp.digitalWrite(12, LOW);
      delay (500);
      }
     for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("valve2");
      display.display();
      display.clearDisplay();
      
      
      mcp.digitalWrite(11, HIGH);
      delay (500);
      mcp.digitalWrite(11, LOW);
      delay (500);
      }
      for(int x=0; x < 5; x++)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("valve1");
      display.display();
      display.clearDisplay();
      

      mcp.digitalWrite(10, HIGH);
      delay (500);
      mcp.digitalWrite(10, LOW);
      delay (500);
      } 
      digitalWrite(12, LOW);

}

void test_triaccomp () //testing on real components
{
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("Door lock!");
      display.display();
      display.clearDisplay();
      digitalWrite(12, HIGH);
      delay (2000);
      
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("drain pump");
      display.display();
      display.clearDisplay();
      
      mcp.digitalWrite(12, HIGH);
      delay (15000);
      mcp.digitalWrite(12, LOW);
           
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("valve2");
      display.display();
      display.clearDisplay();
      
      mcp.digitalWrite(11, HIGH);
      delay (5000);
      mcp.digitalWrite(11, LOW);
      
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("valve1");
      display.display();
      display.clearDisplay();
 
      mcp.digitalWrite(10, HIGH);
      delay (5000);
      mcp.digitalWrite(10, LOW);

      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(0,0);
      display.println("Door unlock!");
      display.display();
      display.clearDisplay();
                
      digitalWrite(12, LOW);
      delay (2000);
      

}

void drum_unload () //unload WM drum water out
{

digitalWrite(12, HIGH);
delay (5000); 
if(mcp.digitalRead(9) == 0)
{
  while(619751.5/(pulseIn(10, HIGH)) < 49)
    {
      Serial.println(619751.5/(pulseIn(10, HIGH)));
      mcp.digitalWrite(12, HIGH); //drain on
      }
      delay (10000); 

       mcp.digitalWrite(12, LOW); //drain off

  }
  digitalWrite(12, LOW);
      
}

void heating () //heating water
{
digitalWrite(12, HIGH);
delay (5000); 
if(mcp.digitalRead(9) == 0)//a zároven musí být voda v bubnu
{
  while(Thermistor(analogRead(0)) < 30)
    {
      Serial.println(Thermistor(analogRead(0)));
      mcp.digitalWrite(13, LOW); //heating on
      }
      mcp.digitalWrite(13, HIGH); //heating off

  }
  digitalWrite(12, LOW);
}

void drum_load () //load WM drum water in
{
digitalWrite(12, HIGH);
delay (5000); 
if(mcp.digitalRead(9) == 0)
{
  while(619751.5/(pulseIn(10, HIGH)) > 45.5)
    {
      Serial.println(619751.5/(pulseIn(10, HIGH)));
      mcp.digitalWrite(10, HIGH); //valve1 open
      }
       mcp.digitalWrite(10, LOW); //valve1 open

  }
  digitalWrite(12, LOW);
}

void test_load ()
{
mcp.digitalWrite(10, HIGH);
delay (5000);
mcp.digitalWrite(10, LOW);
}

void test_powerloaddet ()
{
  mcp.digitalWrite(12, HIGH);
  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Test pwld");
  display.setCursor(0,14);
  display.println("PTC door off");
  digitalWrite(12, LOW);
  delay(1000); //need stabilized
  display.setCursor(0,25);
  display.print("PTCDL value:");
  display.print(mcp.digitalRead(9));
  display.display();
  display.clearDisplay();
  delay(2000);

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Test pwld");
  display.setCursor(0,14);
  display.println("PTC door on");
  digitalWrite(12, HIGH);
  delay(1000);//need stabilized
  display.setCursor(0,25);
  display.print("PTCDL value:");
  display.print(mcp.digitalRead(9));
  display.display();
  display.clearDisplay();
  delay(2000);
  }
  mcp.digitalWrite(12, LOW);
  }

  void test_wmdrum ()
{
  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test wmdrum");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(8));
  display.display();
  display.clearDisplay();
  delay(2000);
  }
  
  }

  void test_switch ()
{
  /*for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 0");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(0));
  display.display();
  display.clearDisplay();
  delay(2000);
  }
*/
  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 1");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(1));
  display.display();
  display.clearDisplay();
  delay(2000);
  }

  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 2");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(2));
  display.display();
  display.clearDisplay();
  delay(2000);
  }

  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 3");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(3));
  display.display();
  display.clearDisplay();
  delay(2000);
  }

  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 4");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(4));
  display.display();
  display.clearDisplay();
  delay(2000);
  }
/*
  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 5");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(5));
  display.display();
  display.clearDisplay();
  delay(2000);
  }
  */
  /*
  for(int i=0; i<5; i++)
  {

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.print("Test switch 6");
  display.setCursor(0,14);
  display.print(mcp.digitalRead(6));
  display.display();
  display.clearDisplay();
  delay(2000);
  }
  */
}
  void RTC_set()
  {
    
    
  
  //rtc.adjust(DateTime(2016, 4,17,12,55,00));}

  }
    
  
    
    

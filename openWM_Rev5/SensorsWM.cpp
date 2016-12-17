/*
  Sensors library for openWM
  Sensors commonly used in washing machines
  by Suefro 28 Nov 2016
  ------------------------------------------
  Include to main program as:
  for temp: SensorsWM NTCtemp("4k7", 0);
*/


#include "Arduino.h"
#include "SensorsWM.h"
#include "Adafruit_MCP23017.h" //for expander
#include "math.h" //for NTC thermometer

SensorsWM::SensorsWM(String type, int pin, unsigned int dim)
{
  pinMode(pin, INPUT);
  _dim = dim;
  _pin = pin;
  _type = type;

}


double SensorsWM::temp()
{

  if (_type == "4k7")
    /*
      Schematic:
      [Ground] ---- [4.7k-Resistor] -------|------- [Thermistor] ---- [+5v]
                                                                 |
                                                         Analog Pin 0
    */
  {
    // Inputs ADC Value from Thermistor and outputs Temperature in Celsius
    //  requires: include <math.h>
    // Utilizes the Steinhart-Hart Thermistor Equation:
    //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
    //    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
    long Resistance;  double Temp;  // Dual-Purpose variable to save space.
    Resistance = 4700 * ((1024.0 / _pin) - 1); // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 /ADC -1) * BalanceResistor
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
}
unsigned long SensorsWM::preasssw()
{

  if (_type == "L46210")
    /*
      AEG-Electrolux
      Mod.L46210
      Typ.210C4430
    */
  {
    return (619751.5 / pulseIn(_pin, HIGH));
  }

}

/*
  boolean SensorsWM:: drum ()
  {
  if(_type == "L46210")

  //AEG-Electrolux
  //Mod.L46210
  //Typ.210C4430

  {
  mcp.digitalRead(_pin);
  }

  }
*/
unsigned int SensorsWM::TachoGen()
{
  if (_type == "L46210")
  {
    return analogRead(_pin); //read generating voltage from TG (A/D data)
  }
}

unsigned int SensorsWM::getDim()
{
  return _dim;
}

void SensorsWM::setDim(unsigned int dim)
{
  _dim = dim;
}

void SensorsWM::speedcontrol(int TG_low, int TG_high, int dim_min, int cycle_regul, boolean cycle_speed)
{ //simple speed control test

  //TachoGen(); //read generating voltage from TG (A/D data)

  if (TachoGen() < 35 ) //first cycle
  {
    if (TachoGen() > TG_low && TachoGen() < TG_high) //in acceptable range
    {
      //ok
      //Serial.println("Control OK");
    }
    else
    {
      if (TachoGen() < TG_low) //higher number means lower speed
      {
        if (_dim < 75)
        {
          cycle_regul = 0;
          //Serial.println("min value!");
        }
        else
        {

          _dim -= 1;
          delay(10);
          //Serial.println(dim);

        }
      }
      if (TachoGen() > TG_high) //lower number means higher speed
      {
        if (_dim > 100)
        {
          //Serial.println("max value!");
        }
        else
        {
          if (_dim < dim_min - cycle_regul)
          {
            _dim += 1;
            delay(10);
            //Serial.println(_dim);
          }
        }
      }
    }

    //Serial.println("cycle1");
    delay(100);
  }

  if (TachoGen() > 40 && TachoGen() < TG_high - 50 && cycle_speed == true ) //second cycle
  {
    if (TachoGen() > TG_low && TachoGen() < TG_high) //in acceptable range
    {
      //ok
      //Serial.println("Control OK");
    }
    else
    {
      if (TachoGen() < TG_low) //higher number means lower speed
      {
        if (_dim < 75)
        {
          cycle_regul = 0;
          //Serial.println("min value!");
        }
        else
        {

          _dim -= 1;
          delay(10);
          //Serial.println(_dim);

        }
      }
      if (TachoGen() > TG_high) //lower number means higher speed
      {
        if (_dim > 100)
        {
          //Serial.println("max value!");
        }
        else
        {
          if (_dim < dim_min - cycle_regul)
          {
            _dim += 1;
            delay(10);
            //Serial.println(_dim);
          }
        }
      }
    }


    delay(500);
    //Serial.println("cycle2");
  }

  if (TachoGen() > TG_high) //cycle3
  {
    cycle_speed = false;
    if (_dim < dim_min - cycle_regul)
    {
      _dim += 1;
    }
    delay(10);
    //Serial.println("cycle3");
  }

  if (TachoGen() > TG_high + 100) //reduce cycle_regul
  {
    cycle_regul -= 1;
    //Serial.println("cycle3");
  }

  if (TachoGen() < 50 && cycle_speed == false) //cycle3
  {
    if (90 < dim_min - cycle_regul)
    {
      cycle_regul += 1;
    }

    //Serial.println("cycle_regul");
  }
  if (TachoGen() > TG_low && TachoGen() < TG_high && cycle_speed == false) //in acceptable range
  {
    if (TachoGen() > TG_low + ((TG_high - TG_low) - 10) && TachoGen() < TG_high - ((TG_high - TG_low) - 10)) //in acceptable range
    {
      //ok
      //Serial.println("Control OK");
    }
    else
    {
      if (TachoGen() < TG_low + ((TG_high - TG_low) - 10)) //higher number means lower speed
      {
        if (_dim < 75)
        {
          cycle_regul = 0;
          //Serial.println("min value!");
        }
        else
        {

          _dim -= 1;

          //Serial.println(__dim);

        }
      }
      if (TachoGen() > TG_high - ((TG_high - TG_low) - 10)) //lower number means higher speed
      {
        if (_dim > 100)
        {
          //Serial.println("max value!");
        }
        else
        {
          if (_dim < dim_min - cycle_regul)
          {
            _dim += 1;

            //Serial.println(_dim);
          }
        }
      }
    }

    delay(10);
    Serial.println("normal");

  }



}


/*
  SensorsWM::DrumPosition(String type, int pin)
  {
  pinMode(pin, INPUT);
  _pin = pin;
  }

  SensorsWM::LoadDetection(String type, int pin)
  {
  pinMode(pin, INPUT);
  _pin = pin;
  }

  SensorsWM::TachoGen(String type, int pin)
  {
  pinMode(pin, INPUT);
  _pin = pin;
  }
*/



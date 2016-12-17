/*
Sensors library
Sensors commonly used in washing machines
by Suefro 28 Nov 2016
*/
#include "Arduino.h"


#ifndef SensorsWM_h //this is prevents if someone include library twice 
#define SensorsWM_h


class SensorsWM
  {
    public:
			SensorsWM(String type, int pin, unsigned int dim);
			double temp();
			unsigned long preasssw(); 
			//boolean drum();
            //PTC();
			unsigned int TachoGen();
      unsigned int getDim();
      void setDim(unsigned int dim);
			void speedcontrol(int TG_low, int TG_high, int dim_min, int cycle_regul, boolean cycle_speed);

            
    private:
            String _type;
            int _pin;
            int _dim;
  };
#endif

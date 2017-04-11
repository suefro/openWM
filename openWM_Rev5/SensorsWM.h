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
    SensorsWM(String type, int pin, unsigned int dim, int cycle_regul, boolean cycle_speed);
    double temp();
    unsigned long preasssw();
    //boolean drum();
    //PTC();
    unsigned int TachoGen();
    unsigned int getDim();
    int getCycle_regul();
    boolean getCycle_speed();
    void setCycle_regul(int cycle_regul);
    void setCycle_speed(boolean cycle_speed);
    void setDim(unsigned int dim);
    void speedcontrol(int TG_low, int TG_high, int dim_min);


  private:
    String _type;
    int _pin;
    int _dim;
    int _cycle_regul;
    boolean _cycle_speed;
};
#endif

// StepperDriver
//.h
#ifndef StepperDriver_h
#define StepperDriver_h

#include "Arduino.h"

#define StepperDriver_PULSE_DELAY 100

class StepperDriver
{
public:
  StepperDriver(uint8_t Pin)
  {
    _Pin = Pin;
    pinMode(_Pin, OUTPUT);
    digitalWrite(_Pin, LOW);
  }
  bool Handle();
  void Step(uint16_t Delay);

private:
  uint8_t _Pin;
  bool _Stepping = false;
  uint16_t _Delay;
  uint32_t _Timer = micros();
  uint16_t _StepDelay = 0;
};

//.cpp
//#include "StepperDriver.h"
//#include "Arduino.h"

bool StepperDriver::Handle()
{
  // Handle Step Sequence:
  static uint8_t StepStage = 0;
  if (_Stepping)
  {
    if (micros() - _Timer >= _StepDelay)
    {
      switch (StepStage)
      {
      case 0:
        digitalWrite(_Pin, HIGH);
        StepStage = 1;
        _StepDelay = StepperDriver_PULSE_DELAY;
        _Timer = micros();
        break;
      case 1:
        digitalWrite(_Pin, LOW);
        StepStage = 2;
        _StepDelay = StepperDriver_PULSE_DELAY;
        _Timer = micros();
        break;
      case 2:
        StepStage = 3;
        _StepDelay = _Delay;
        _Timer += StepperDriver_PULSE_DELAY;
        break;
      case 3:
        StepStage = 0;
        _Stepping = false;
        _StepDelay = 0;
        break;
      default:
        StepStage = 0;
        _Stepping = false;
        _StepDelay = 0;
        break;
      }
    }
  }
  return !_Stepping;
}

void StepperDriver::Step(uint16_t Delay)
{
  if(!_Stepping) {
    _Delay = Delay;
    _Stepping = true;
  }
}

#undef StepperDriver_PULSE_DELAY
#endif
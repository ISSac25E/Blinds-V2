// StepperDriver
//.h
#ifndef StepperDriver_h
#define StepperDriver_h

#include "Arduino.h"
#include "ESP8266TimerInterrupt.h"

#define USING_TIM_DIV1 true // Use Fastest Clock

// Keep these accessable only within the file:
static ESP8266Timer _IntTimer;
volatile static uint8_t _Pin;
volatile static bool _Stepping = false;

void IRAM_ATTR IntTimerHandle()
{
  if (_Stepping)
  {
    volatile static bool PinState = false;
    if (PinState)
    {
      digitalWrite(_Pin, LOW);
      PinState = false;
      _Stepping = false;
    }
    else
    {
      digitalWrite(_Pin, HIGH);
      PinState = true;
    }
  }
}

class StepperDriver
{
public:
  StepperDriver(uint8_t Pin, uint16_t Pulse_us = 0)
  {
    _Pin = Pin;
    pinMode(_Pin, OUTPUT);
    digitalWrite(_Pin, LOW);
    this->SetDelay(Pulse_us);
  }
  bool Step();
  bool Ready()
  {
    return !_Stepping;
  }
  void SetDelay(uint16_t Delay)
  {
    if (Delay != _InterruptDelay && !_Stepping)
    {
      _InterruptDelay = Delay;
      if (_Init)
      {
        _IntTimer.detachInterrupt();
      }
      else
        _Init = true;
      _IntTimer.attachInterruptInterval(_InterruptDelay, IntTimerHandle);
    }
  }

private:
  uint16_t _InterruptDelay = 0;
  bool _Init = false;
};

//.cpp
//#include "StepperDriver.h"
//#include "Arduino.h"

bool StepperDriver::Step()
{
  if (!_Stepping)
  {
    _Stepping = true;
    return true;
  }
  return false;
}
#endif
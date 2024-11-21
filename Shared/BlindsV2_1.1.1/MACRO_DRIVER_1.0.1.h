//MACRO_DRIVER
//.h
#ifndef MACRO_DRIVER_h
#define MACRO_DRIVER_h

#include "Arduino.h"

class MACRO_DRIVER
{
public:
  //Runs Macros, Handles Timer:
  bool Run();
  //Return true if ready for the next Macro:
  bool Ready();
  //Stops all Macros:
  void Stop();

  void Fade(uint16_t Target, uint16_t Steps); //Fade Value
  void Set(uint16_t Target, uint16_t Delay); //Set value and Delay
  void Delay(uint16_t Delay);               //Delay without Setting Val

  //Set and read Current Value:
  void Val(uint16_t Val);
  uint16_t Val();

  //Set FPS:
  void FPS(uint16_t FPS);

private:
  bool _MacroRun = false; //Stores if currently running Macro
  //Timer Vars:
  uint32_t _Timer = millis();
  uint16_t _Delay;    //ms
  uint16_t _FPS = 30; //Default 30FPS
  //Values:
  uint16_t _CurVal = 0;
  int32_t _StaVal, _TarVal;
  //Steps:
  uint16_t _CurStep;
  uint16_t _StepsToRun;
};

//.cpp
//#include "MACRO_DRIVER.h"
//#include "Arduino.h"

bool MACRO_DRIVER::Run()
{
  //check if Macro is running:
  if (_MacroRun)
  {
    //If macro, Wait for timer:
    if (millis() - _Timer >= _Delay)
    {
      if (_CurStep < _StepsToRun)
      {
        _CurStep++;
        _CurVal = _StaVal + (((int32_t)((int32_t)(_TarVal - _StaVal) * (int32_t)_CurStep) / (int32_t)_StepsToRun));
        _Timer += _Delay; //Absolute Timing
      }
      else
        _MacroRun = false;
    }
  }
  return !_MacroRun;
}

bool MACRO_DRIVER::Ready()
{
  return !_MacroRun;
}

void MACRO_DRIVER::Stop()
{
  _MacroRun = false;
}

void MACRO_DRIVER::Fade(uint16_t Target, uint16_t Steps)
{
  if (Steps)
  {
    //Set all Vals:
    _StaVal = _CurVal;
    _TarVal = Target;
    //Set all Step Vals:
    _StepsToRun = Steps;
    _CurStep = 0;
    //Set all Timer Vals:
    _Delay = (1000 / _FPS);
    _Timer = millis();
    //Set MacroRun:
    _MacroRun = true;
  }
  else
    _MacroRun = false;
}

void MACRO_DRIVER::Set(uint16_t Target, uint16_t Delay)
{
  _CurVal = Target;
  if (Delay)
  {
    //Set Step Vals:
    _CurStep = 0;
    _StepsToRun = 0;
    //Setup Timing:
    _Delay = Delay;
    _Timer = millis();
    // Set MacroRun
    _MacroRun = true;
  }
  else
    _MacroRun = false;
}

void MACRO_DRIVER::Delay(uint16_t Delay)
{
  if (Delay)
  {
    //Set Step Vals:
    _CurStep = 0;
    _StepsToRun = 0;
    //Setup Timing:
    _Delay = Delay;
    _Timer = millis();
    //Set MacroRun
    _MacroRun = true;
  }
  else
    _MacroRun = false;
}

void MACRO_DRIVER::Val(uint16_t Val)
{
  _CurVal = Val;
}

uint16_t MACRO_DRIVER::Val()
{
  return _CurVal;
}

void MACRO_DRIVER::FPS(uint16_t FPS)
{
  _FPS = FPS;
  if(!_FPS) _FPS = 1;
  else if(_FPS > 1000) _FPS = 1000;
}
#endif
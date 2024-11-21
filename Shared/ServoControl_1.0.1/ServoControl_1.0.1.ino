// Compatable Only with BlindsV2 Versions (1.0.4, 1.0.5)

//////////////////////////////////////////////////////////

// Setup Servo Control:
#define ESP_SIGNAL_PIN 3 // Set Pin which Arduino will Connect to ESP
#define SERVO_PIN 12     // Set Pin to Servo

#define SERVO_OPEN_POS_deg 0    // Servo Open Position in degrees(0-180)
#define SERVO_CLOSE_POS_deg 180 // Servo Close/Normal/Resting Position in degrees(0-180)
#define SERVO_POS_DELAY_ms 2000 // Servo Open Close Delay in ms(milliseconds)
#define SERVO_FADE_FPS 60       // Set Servo Update Rate. Less FPS Might make more Choppy Movment But Less Processing
#define SERVO_POST_DELAY_ms 100 // Set Small Delay Afer Servo Moves to give it a chance to move to final POS

#define MIN_PULSE_LOW_ms 10  // Min required Pulse for Trigger
#define MIN_PULSE_HIGH_ms 10 // Min Required Pulse for Reset

//////////////////////////////////////////////////////////

#include "MACRO_DRIVER_1.0.1.h"
#include <Servo.h>

Servo ArdServo;
MACRO_DRIVER ServoMacro;

bool PinState = true;
uint32_t PinTimer = millis();
bool PinReset = false;
bool RunServo = false;

void setup()
{
  // Set Signal Pin to Input Pullup
  pinMode(ESP_SIGNAL_PIN, INPUT_PULLUP);

  // Setup Macro:
  ServoMacro.FPS(SERVO_FADE_FPS);
  ServoMacro.Val(SERVO_CLOSE_POS_deg);
}

void loop()
{
  PinState = digitalRead(ESP_SIGNAL_PIN);
  // Check Pin Change:
  {
    static bool PrevPinState = true;
    if (PinState != PrevPinState)
    {
      // PinChange
      PrevPinState = PinState;
      PinTimer = millis();
    }
  }
  // Check Pin Timer:
  {
    if (!RunServo)
    {
      if (PinReset)
      {
        if (!PinState && millis() - PinTimer >= MIN_PULSE_LOW_ms)
        {
          // Run Servo
          PinReset = false;
          RunServo = true;
        }
      }
      else
      {
        if (PinState && millis() - PinTimer >= MIN_PULSE_HIGH_ms)
          PinReset = true;
      }
    }
  }
  // Handle Servo:
  {
    if (RunServo)
    {
      static uint8_t RunStage = 0;
      switch (RunStage)
      {
      case 0:
        ServoMacro.Fade(SERVO_OPEN_POS_deg, (SERVO_POS_DELAY_ms / (1000 / SERVO_FADE_FPS)));
        ArdServo.attach(SERVO_PIN);
        RunStage = 1;
        break;
      case 1:
        if (ServoMacro.Ready())
        {
          ServoMacro.Fade(SERVO_CLOSE_POS_deg, (SERVO_POS_DELAY_ms / (1000 / SERVO_FADE_FPS)));
          RunStage = 2;
        }
        break;
      case 2:
        if (ServoMacro.Ready())
        {
          ServoMacro.Delay(SERVO_POST_DELAY_ms);
          RunStage = 3;
        }
        break;
      case 3:
        if (ServoMacro.Ready())
        {
          ArdServo.detach();
          RunStage = 0;
          RunServo = false;
        }
        break;
      default:
        ArdServo.detach();
        RunStage = 0;
        RunServo = false;
        break;
      }
      if (RunServo)
        ArdServo.write(ServoMacro.Val());
      ServoMacro.Run();
    }
  }
}
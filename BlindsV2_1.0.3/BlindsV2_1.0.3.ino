////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// MQTT AND NETWORK SETUP:

// WIFI Network Setup:
#define BLINDS_NETWORK_SSID "*****"
#define BLINDS_NETWORK_PASSWORD "*****"

// MQTT Server Setup:
#define BLINDS_MQTT_SERVER_IP "192.168.0.20"
#define BLINDS_MQTT_PORT 1883
#define BLINDS_MQTT_USERNAME "mqtt"
#define BLINDS_MQTT_PASSWORD "*****"

// MQTT Client Setup:
#define BLINDS_MQTT_CLIENT_NAME "BlindsMCU"

// OTA Setup:
#define BLINDS_OTA_USERNAME "BlindsMCU"

// BLINDS Topic Configure:
#define BLINDS_MQTT_COMMAND_TOPIC_ROOT "/blindsCommand"
#define BLINDS_MQTT_POSITION_TOPIC_ROOT "/positionState"
#define BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT "/State"
#define BLIND_CALIBRATE_TOPIC_ROOT "/blindsCalibrate"

// Servo Topic Configure:
#define SERVO_MQTT_TOPIC "/BUTTON"

// Blinds Stepper Setup:
#define STEPPER_STEP_PIN D7
#define STEPPER_DIR_PIN D8
#define STEPPER_EN_PINS    \
  {                        \
    D0, D1, D2, D3, D4, D5, D6 \
  }

#define STEPPER_STEPS_PER_SEC 300           // Steps per second
#define STEPPER_CALIBRATE_STEPS_PER_SEC 100 // Steps per second When Homeing/Calibrating (Usually slower than normal speed)
#define STEPPER_STEPS_DIVISION 40           // How many times MQTT Position Message will be Multiplied by
#define BLINDS_MAX_CALIBRATE_STEPS 500      // Max Calibration Steps (Max number of Steps Stepper will move while calibrating)

// Servo Setup:
#define SERVO_PIN 3 // <<< !!Warning Serial RX Pin!!

#define SERVO_OPEN_POS_deg 500   // Servo Open Position in degrees(0-180)
#define SERVO_CLOSE_POS_deg 2500 // Servo Close/Normal/Resting Position in degrees(0-180)
#define SERVO_POS_DELAY_ms 500   // Servo Open Close Delay in ms(milliseconds)
#define SERVO_FADE_FPS 60

// DEBUG:
#define DEBUG_EN

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// All Required Libraries:
#include <ESP8266WiFi.h>  //Make Sure ESP8266 Board Manager is Installed
#include <ESP8266mDNS.h>  //Make Sure ESP8266 Board Manager is Installed
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>   //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

// Local Core Files:
#include <CORE.h>
#include "Projects/Serge/BlindsV2/StepperDriver/StepperDriver_1.2.1.h"
#include "Projects/Serge/BlindsV2/ServoDriver/ServoDriver_1.0.0.h"
#include "Projects/Serge/BlindsV2/OUTPUT_MACRO/MACRO_DRIVER/MACRO_DRIVER_1.0.1.h"

// Define us per Stepper Step:
#define STEPPER_SPEED_US 1000000 / STEPPER_STEPS_PER_SEC
#define STEPPER_CALIBRATE_SPEED_US 1000000 / STEPPER_CALIBRATE_STEPS_PER_SEC

// Objects:
WiFiClient ESP_CLIENT;
PubSubClient CLIENT(ESP_CLIENT);
StepperDriver StepperDrive(STEPPER_STEP_PIN);
ServoDriver ESP_Servo(SERVO_PIN);
MACRO_DRIVER ServoMacro;

// Global Vars

// If Connected To Broker:
bool MQTT_Connected = false;

// Steppers Vars:
const uint8_t StepperPins[] = STEPPER_EN_PINS;
const uint8_t StepperCount = sizeof(StepperPins);

bool StepperBoot = false;
uint32_t StepperBootTimer;

bool CalibrateStepper[StepperCount];
uint16_t StepperCurrentVal[StepperCount];
uint16_t StepperTargetVal[StepperCount];

void setup()
{
#ifdef DEBUG_EN
  Serial.begin(115200);
  Serial.println("\nINIT");
#endif

  // Setup:
  WiFi.mode(WIFI_STA);
  WiFi.begin(BLINDS_NETWORK_SSID, BLINDS_NETWORK_PASSWORD);

  // SetUp OTA:
  ArduinoOTA.setHostname(BLINDS_OTA_USERNAME);
  ArduinoOTA.begin();

  CLIENT.setServer(BLINDS_MQTT_SERVER_IP, BLINDS_MQTT_PORT);
  CLIENT.setCallback(ParseMQTT);

  // Set Blind Vals and Pins:
  pinMode(STEPPER_DIR_PIN, OUTPUT);
  for (uint8_t X = 0; X < StepperCount; X++)
  {
    // Set OUTPUTS:
    pinMode(StepperPins[X], OUTPUT);
    digitalWrite(StepperPins[X], HIGH); // Disable Steppers
    StepperCurrentVal[X] = 0;
    StepperTargetVal[X] = 0;
    CalibrateStepper[X] = false;
  }

  // Setup Servo Macro:
  ServoMacro.FPS(SERVO_FADE_FPS);
  ServoMacro.Val(SERVO_CLOSE_POS_deg);
}

void loop()
{
  MQTT_Run();
  CheckStepperBoot();
  RunStepper();
  PublishPosHandle();
  CalibrateStepperHandle();
  RunServo(false);

  ArduinoOTA.handle();
}

void MQTT_Run()
{
  // MQTT:
  {
    // Make Sure MQTT is Connected and Run The Loop:
    CLIENT.loop();
    MQTT_Connected = CLIENT.connected();
    if (!MQTT_Connected)
    {
      // Try to Reconnect:
      if (CLIENT.connect(BLINDS_MQTT_CLIENT_NAME, BLINDS_MQTT_USERNAME, BLINDS_MQTT_PASSWORD))
      {
#ifdef DEBUG_EN
        Serial.println("MQTT Connected");
#endif
        CLIENT.publish(BLINDS_MQTT_CLIENT_NAME "/checkIn", "Rebooted");
        CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME SERVO_MQTT_TOPIC);
#ifdef DEBUG_EN
        Serial.print("MQTT Subscribe: ");
        Serial.println(BLINDS_MQTT_CLIENT_NAME SERVO_MQTT_TOPIC);
#endif
        if (StepperCount)
        {
          CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT);
          CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT);
          CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT);
#ifdef DEBUG_EN
          Serial.print("MQTT Subscribe: ");
          Serial.println(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT);
          Serial.print("MQTT Subscribe: ");
          Serial.println(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT);
          Serial.print("MQTT Subscribe: ");
          Serial.println(BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT);
#endif
          for (uint8_t X = 0; X < StepperCount; X++)
          {
            char NumChar[5];
            itoa(X, NumChar, 10);
            {
              char TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT;
              for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
                TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
              CLIENT.subscribe(TopicSub);
#ifdef DEBUG_EN
              Serial.print("MQTT Subscribe: ");
              Serial.println(TopicSub);
#endif
            }
            {
              char TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT;
              for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
                TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
              CLIENT.subscribe(TopicSub);
#ifdef DEBUG_EN
              Serial.print("MQTT Subscribe: ");
              Serial.println(TopicSub);
#endif
            }
            {
              char TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT;
              for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
                TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
              CLIENT.subscribe(TopicSub);
#ifdef DEBUG_EN
              Serial.print("MQTT Subscribe: ");
              Serial.println(TopicSub);
#endif
            }
          }
        }
        StepperBoot = false;
        StepperBootTimer = millis();
      }
    }
    // MQTT CheckIn Every 15 Seconds, 1 Min Max:
    {
      if (MQTT_Connected)
      {
        static uint32_t CheckInTimer = millis();
        if (millis() - CheckInTimer >= 15000)
        {
#ifdef DEBUG_EN
          Serial.println("MQTT CheckIn");
#endif
          CLIENT.publish(BLINDS_MQTT_CLIENT_NAME "/checkIn", "OK");
          CheckInTimer = millis();
        }
      }
    }
  }
}

void CheckStepperBoot()
{
  if (MQTT_Connected && !StepperBoot && millis() - StepperBootTimer >= 1000)
  {
    StepperBoot = true;
#ifdef DEBUG_EN
    Serial.println("StepperBootTrue");
#endif
  }
}

void RunStepper()
{
  if (StepperDrive.Ready())
  {
    for (uint8_t X = 0; X < StepperCount; X++)
    {
      if (!CalibrateStepper[X])
      {
        if (StepperCurrentVal[X] < StepperTargetVal[X])
        {
          // First Disable all Enable pins:
          for (uint8_t Y = 0; Y < StepperCount; Y++)
            digitalWrite(StepperPins[Y], HIGH);
          StepperCurrentVal[X]++;
          digitalWrite(STEPPER_DIR_PIN, HIGH);
          digitalWrite(StepperPins[X], LOW);
          StepperDrive.SetDelay(STEPPER_SPEED_US);
          StepperDrive.Step();
#ifdef DEBUG_EN
          // Serial Debug:
          {
            Serial.print("PROCESS STEPPER_");
            Serial.print(X);
            Serial.print("++ ");
            Serial.print(StepperCurrentVal[X] - 1);
            Serial.print(">>");
            Serial.print(StepperCurrentVal[X]);
            Serial.print(" ");
            Serial.println(StepperTargetVal[X]);
          }
#endif
          return;
        }
        else if (StepperCurrentVal[X] > StepperTargetVal[X])
        {
          // First Disable all Enable pins:
          for (uint8_t Y = 0; Y < StepperCount; Y++)
            digitalWrite(StepperPins[Y], HIGH);
          StepperCurrentVal[X]--;
          digitalWrite(STEPPER_DIR_PIN, LOW);
          digitalWrite(StepperPins[X], LOW);
          StepperDrive.SetDelay(STEPPER_SPEED_US);
          StepperDrive.Step();
#ifdef DEBUG_EN
          // Serial Debug:
          {
            Serial.print("PROCESS STEPPER_");
            Serial.print(X);
            Serial.print("-- ");
            Serial.print(StepperCurrentVal[X] + 1);
            Serial.print(">>");
            Serial.print(StepperCurrentVal[X]);
            Serial.print(" ");
            Serial.println(StepperTargetVal[X]);
          }
#endif
          return;
        }
        else
        {
          // Disable Stepper Pin as it Searches:
          digitalWrite(StepperPins[X], HIGH);
        }
      }
    }
  }
}

void PublishPosHandle()
{
  static uint32_t PublishTimerLong = millis();
  static uint32_t PublishTimerShort = millis();
  static uint16_t StepperPrevPos[StepperCount];
  bool Change = false;
  for (uint8_t X = 0; X < StepperCount; X++)
  {
    if (StepperPrevPos[X] != StepperCurrentVal[X])
    {
      Change = true;
      break;
    }
  }
  if (Change)
  {
    if (millis() - PublishTimerShort >= 100)
    {
      PublishTimerShort = millis();
      for (uint8_t X = 0; X < StepperCount; X++)
      {
        if (StepperPrevPos[X] != StepperCurrentVal[X])
        {
          StepperPrevPos[X] = StepperCurrentVal[X];
          // Publish MQTT MSG:
          {
            char NumChar[6];
            itoa(X, NumChar, 10);
            char StateTopicPublish[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT;
            for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
            {
              StateTopicPublish[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
            }
            char PosNumChar[6];
            itoa(StepperCurrentVal[X] / STEPPER_STEPS_DIVISION, PosNumChar, 10);
            CLIENT.publish(StateTopicPublish, PosNumChar);
#ifdef DEBUG_EN
            Serial.print("MQTT STEP PUBLISH: ");
            Serial.print(StateTopicPublish);
            Serial.print(" \"");
            Serial.print(StepperCurrentVal[X] / STEPPER_STEPS_DIVISION);
            Serial.println("\"");
#endif
            if (X == 0)
            {
              CLIENT.publish(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT, PosNumChar);
#ifdef DEBUG_EN
              Serial.print("MQTT STEP PUBLISH: ");
              Serial.print(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT);
              Serial.print(" \"");
              Serial.print(StepperCurrentVal[X] / STEPPER_STEPS_DIVISION);
              Serial.println("\"");
#endif
            }
          }
        }
      }
    }
  }
  // Every 5 Seconds Because Each individual Stepper is handled each time:
  if (millis() - PublishTimerLong >= 5000)
  {
    PublishTimerLong = millis();
    static uint8_t StepperNum = 0;
    if (StepperCount)
    {
      uint8_t X = StepperNum;
      // Publish MQTT MSG:
      {
        char NumChar[6];
        itoa(X, NumChar, 10);
        char StateTopicPublish[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT;
        for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
        {
          StateTopicPublish[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
        }
        char PosNumChar[6];
        itoa(StepperCurrentVal[X] / STEPPER_STEPS_DIVISION, PosNumChar, 10);
        CLIENT.publish(StateTopicPublish, PosNumChar);
#ifdef DEBUG_EN
        Serial.print("MQTT STEP PUBLISH: ");
        Serial.print(StateTopicPublish);
        Serial.print(" \"");
        Serial.print(StepperCurrentVal[X] / STEPPER_STEPS_DIVISION);
        Serial.println("\"");
#endif
        if (X == 0)
        {
          CLIENT.publish(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT, PosNumChar);
#ifdef DEBUG_EN
          Serial.print("MQTT STEP PUBLISH: ");
          Serial.print(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POS_UPDATE_TOPIC_ROOT);
          Serial.print(" \"");
          Serial.print(StepperCurrentVal[X] / STEPPER_STEPS_DIVISION);
          Serial.println("\"");
#endif
        }
      }
      // Advence Counter for Next Round
      StepperNum++;
      if (StepperNum >= StepperCount)
        StepperNum = 0;
    }
  }
}

void CalibrateStepperHandle()
{
  static uint16_t StepCaliCount[StepperCount];
  for (uint8_t X = 0; X < StepperCount; X++)
  {
    if (CalibrateStepper[X])
    {
      if (StepCaliCount[X] > BLINDS_MAX_CALIBRATE_STEPS)
      {
        CalibrateStepper[X] = false;
        StepperCurrentVal[X] = 0;
        StepperTargetVal[X] = 0;
      }
      else
      {
        if (StepperDrive.Ready())
        {
          StepCaliCount[X]++;
          for (uint8_t Y = 0; Y < StepperCount; Y++)
            digitalWrite(StepperPins[Y], HIGH);
          digitalWrite(STEPPER_DIR_PIN, LOW);
          digitalWrite(StepperPins[X], LOW);
          StepperDrive.SetDelay(STEPPER_CALIBRATE_SPEED_US);
          StepperDrive.Step();
        }
        return;
      }
    }
    else
    {
      StepCaliCount[X] = 0;
    }
  }
}

void RunServo(bool Run)
{
  static bool Running = false;
  static uint8_t ServoStage = 0;
  if (Running)
  {
    switch (ServoStage)
    {
    case 0:
#ifdef DEBUG_EN
      Serial.println("SERVO: OPENING");
#endif
      ServoMacro.Fade(SERVO_OPEN_POS_deg, (SERVO_POS_DELAY_ms / (1000 / SERVO_FADE_FPS)));
      ESP_Servo.Attach();
      ServoStage = 1;
      break;
    case 1:
      if (ServoMacro.Ready())
      {
#ifdef DEBUG_EN
        Serial.println("SERVO: CLOSEING");
#endif
        ServoMacro.Fade(SERVO_CLOSE_POS_deg, (SERVO_POS_DELAY_ms / (1000 / SERVO_FADE_FPS)));
        ServoStage = 2;
      }
      break;
    case 2:
      if (ServoMacro.Ready())
      {
#ifdef DEBUG_EN
        Serial.println("SERVO: READY");
#endif
        ESP_Servo.Detach();
        ServoStage = 0;
        Running = false;
      }
      break;
    default:
      ServoStage = 0;
      Running = false;
      break;
    }
  }
  else
  {
    if (Run)
    {
#ifdef DEBUG_EN
      Serial.println("SERVO: RUN");
#endif
      Running = true;
      ServoStage = 0;
    }
  }
  ESP_Servo.Handle();
  ESP_Servo.Write(ServoMacro.Val());
  ServoMacro.Run();
}

// Parse Incoming MQTT Messages:
void ParseMQTT(char *Topic, byte *PayLoad, uint16_t Length)
{
  String TopicStr = Topic;
  PayLoad[Length] = '\0';
  String PayLoadStr = String((char *)PayLoad);
  uint8_t PayLoadInt = PayLoadStr.toInt();

  // Compare Topic And PayLoad To Find Correct MSG:

#ifdef DEBUG_EN
  Serial.print("MQTT MSG: ");
  Serial.print(TopicStr);
  Serial.print(" \"");
  Serial.print(PayLoadStr);
  Serial.print("\" :");
  Serial.println(PayLoadInt);
#endif

  if (StepperBoot)
  {
    // Check first Stepper:
    if (StepperCount)
    {
      if (TopicStr == BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT || TopicStr == BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT)
      {
        if (PayLoadStr == "OPEN")
        {
          StepperTargetVal[0] = 0;
        }
        else if (PayLoadStr == "STOP")
        {
          StepperTargetVal[0] = StepperCurrentVal[0];
        }
        else if (PayLoadStr == "CLOSE")
        {
          StepperTargetVal[0] = 0;
        }
        else
        {
          // Assume Int
          StepperTargetVal[0] = PayLoadInt * STEPPER_STEPS_DIVISION;
        }
      }
      if (TopicStr == BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT)
      {
        if (CalibrateStepper[0])
        {
          if (PayLoadStr == "ON" || PayLoadStr == "OFF" || PayLoadStr == "SET")
          {
            CalibrateStepper[0] = false;
            StepperCurrentVal[0] = 0;
            StepperTargetVal[0] = 0;
          }
        }
        else
        {
          if (PayLoadStr == "ON")
          {
            CalibrateStepper[0] = true;
            StepperCurrentVal[0] = 0;
            StepperTargetVal[0] = 0;
          }
          else if (PayLoadStr == "SET")
          {
            StepperCurrentVal[0] = 0;
            StepperTargetVal[0] = 0;
          }
        }
      }
    }
    for (uint8_t X = 0; X < StepperCount; X++)
    {
      // Check the rest of the Steppers:
      {
        char NumChar[5];
        itoa(X, NumChar, 10);
        char CommandTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT;
        char PositionTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT;
        char CalibrateTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT;
        for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
        {
          CommandTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
          PositionTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
          CalibrateTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLIND_CALIBRATE_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
        }

        if (TopicStr == CommandTopicComp || TopicStr == PositionTopicComp)
        {
          if (PayLoadStr == "OPEN")
          {
            StepperTargetVal[X] = 0;
          }
          else if (PayLoadStr == "STOP")
          {
            StepperTargetVal[X] = StepperCurrentVal[X];
          }
          else if (PayLoadStr == "CLOSE")
          {
            StepperTargetVal[X] = 0;
          }
          else
          {
            // Assume Int
            StepperTargetVal[X] = PayLoadInt * STEPPER_STEPS_DIVISION;
          }
        }
        else if (TopicStr == CalibrateTopicComp)
        {
          if (CalibrateStepper[X])
          {
            if (PayLoadStr == "ON" || PayLoadStr == "OFF" || PayLoadStr == "SET")
            {
              CalibrateStepper[X] = false;
              StepperCurrentVal[X] = 0;
              StepperTargetVal[X] = 0;
            }
          }
          else
          {
            if (PayLoadStr == "ON")
            {
              CalibrateStepper[X] = true;
              StepperCurrentVal[X] = 0;
              StepperTargetVal[X] = 0;
            }
            else if (PayLoadStr == "SET")
            {
              StepperCurrentVal[X] = 0;
              StepperTargetVal[X] = 0;
            }
          }
        }
      }
    }
  }
  else
  {
    // Check first Stepper:
    if (StepperCount)
    {
      if (TopicStr == BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT)
      {
        if (PayLoadStr != "OPEN" && PayLoadStr != "CLOSE" && PayLoadStr != "STOP")
        {
          // Assume Int
          StepperCurrentVal[0] = PayLoadInt * STEPPER_STEPS_DIVISION;
          StepperTargetVal[0] = PayLoadInt * STEPPER_STEPS_DIVISION;
        }
      }
    }
    for (uint8_t X = 0; X < StepperCount; X++)
    {
      // Check the rest of the Steppers:
      {
        char NumChar[5];
        itoa(X, NumChar, 10);
        char TopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT;
        for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
          TopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) - 1 + Y] = NumChar[Y];

        if (TopicStr == TopicComp)
        {
          if (PayLoadStr != "OPEN" && PayLoadStr != "CLOSE" && PayLoadStr != "STOP")
          {
            // Assume Int
            StepperCurrentVal[X] = PayLoadInt * STEPPER_STEPS_DIVISION;
            StepperTargetVal[X] = PayLoadInt * STEPPER_STEPS_DIVISION;
          }
        }
      }
    }
  }

  // Check Button MSG
  if (TopicStr == BLINDS_MQTT_CLIENT_NAME SERVO_MQTT_TOPIC)
  {
    if (PayLoadStr == "ON")
    {
      RunServo(true);
    }
  }
}
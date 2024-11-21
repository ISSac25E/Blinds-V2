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

#define BLINDS_MQTT_COMMAND_TOPIC_ROOT "/blindsCommand"
#define BLINDS_MQTT_POSITION_TOPIC_ROOT "/positionState"

// OTA Setup:
#define BLINDS_OTA_USERNAME "BlindsMCU"

// Blinds Stepper Setup:

#define STEPPER_STEP_PIN
#define STEPPER_DIR_PIN
#define STEPPER_EN_PINS        \
  {                            \
    D0, D1, D2, D3, D4, D5, D6 \
  }

#define STEPPER_SPEED_US 1600 // Microseconds Per Step
////////////////////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

// Network Objects:
WiFiClient ESP_CLIENT;
PubSubClient CLIENT(ESP_CLIENT);

// Global Vars

// If Connected To Broker:
bool MQTT_Connected = false;

// Steppers Vars:
const uint8_t StepperPins[] = STEPPER_EN_PINS;
const uint8_t StepperCount = sizeof(StepperPins);

bool StepperBoot = false;
uint32_t StepperBootTimer;

int16_t StepperCurrentVal[StepperCount];
int16_t StepperTargetVal[StepperCount];

void setup()
{
  Serial.begin(115200);
  Serial.println("\nINIT");

  // Setup:
  WiFi.mode(WIFI_STA);
  WiFi.begin(BLINDS_NETWORK_SSID, BLINDS_NETWORK_PASSWORD);

  CLIENT.setServer(BLINDS_MQTT_SERVER_IP, BLINDS_MQTT_PORT);
  CLIENT.setCallback(ParseMQTT);

  // Set Blind Vals:
  for (uint8_t X = 0; X < StepperCount; X++)
  {
    StepperCurrentVal[X] = 0;
    StepperTargetVal[X] = 0;
  }
}

void loop()
{
  MQTT_Run();
  CheckStepperBoot();
  RunStepper();
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
        Serial.println("Connect");
        CLIENT.publish(BLINDS_MQTT_CLIENT_NAME "/checkIn", "Rebooted");
        if (StepperCount)
        {
          CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT);
          CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT);
          CLIENT.subscribe(BLINDS_MQTT_CLIENT_NAME "/BUTTON");
          for (uint8_t X = 0; X < StepperCount; X++)
          {
            char NumChar[5];
            itoa(X, NumChar, 10);
            {
              char TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT;
              for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
                TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
              CLIENT.subscribe(TopicSub);
              Serial.println(TopicSub);
            }
            {
              char TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) + sizeof(NumChar)] = BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT;
              for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
                TopicSub[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
              CLIENT.subscribe(TopicSub);
              Serial.println(TopicSub);
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
    Serial.println("BootTrue");
  }
}

void RunStepper()
{
  static uint32_t StepperRunTimer = millis();
  if (millis() - StepperRunTimer >= 100)
  {
    StepperRunTimer = millis();
    for (uint8_t X = 0; X < StepperCount; X++)
    {
      if (StepperCurrentVal[X] < StepperTargetVal[X])
      {
        StepperCurrentVal[X]++;
        Serial.print("STEPPER_");
        Serial.print(X);
        Serial.print("++ ");
        Serial.print(StepperCurrentVal[X] - 1);
        Serial.print(">>");
        Serial.print(StepperCurrentVal[X]);
        Serial.print(" ");
        Serial.println(StepperTargetVal[X]);
        return;
      }
      else if (StepperCurrentVal[X] > StepperTargetVal[X])
      {
        StepperCurrentVal[X]--;
        Serial.print("STEPPER_");
        Serial.print(X);
        Serial.print("-- ");
        Serial.print(StepperCurrentVal[X] + 1);
        Serial.print(">>");
        Serial.print(StepperCurrentVal[X]);
        Serial.print(" ");
        Serial.println(StepperTargetVal[X]);
        return;
      }
    }
  }
}

// Parse Incoming MQTT Messages:
void ParseMQTT(char *Topic, byte *PayLoad, uint16_t Length)
{
  String TopicStr = Topic;
  PayLoad[Length] = '\0';
  String PayLoadStr = String((char *)PayLoad);
  uint8_t PayLoadInt = PayLoadStr.toInt();

  // Compare Topic And PayLoad To Find Correct MSG:

  if (StepperBoot)
  {
    // Check first Stepper:
    if (StepperCount)
    {
      if (TopicStr == BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT || TopicStr == BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT)
      {
        Serial.print("MATCH BOOT: ");
        Serial.println(PayLoadStr);

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
          StepperTargetVal[0] = PayLoadInt;
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
        for (uint8_t Y = 0; Y < sizeof(NumChar); Y++)
        {
          CommandTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_COMMAND_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
          PositionTopicComp[sizeof(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT) - 1 + Y] = NumChar[Y];
        }

        if (TopicStr == CommandTopicComp || TopicStr == PositionTopicComp)
        {
          Serial.print("MATCH BOOT: ");
          Serial.println(PayLoadStr);

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
            StepperTargetVal[X] = PayLoadInt;
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
        Serial.print("MATCH: ");
        Serial.println(BLINDS_MQTT_CLIENT_NAME BLINDS_MQTT_POSITION_TOPIC_ROOT);
        if (PayLoadStr != "OPEN" && PayLoadStr != "CLOSE" && PayLoadStr != "STOP")
        {
          // Assume Int
          StepperCurrentVal[0] = PayLoadInt;
          StepperTargetVal[0] = PayLoadInt;
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
          Serial.print("MATCH: ");
          Serial.println(TopicComp);
          if (PayLoadStr != "OPEN" && PayLoadStr != "CLOSE" && PayLoadStr != "STOP")
          {
            // Assume Int
            StepperCurrentVal[X] = PayLoadInt;
            StepperTargetVal[X] = PayLoadInt;
          }
        }
      }
    }
  }

  // Check Button MSG
  if (TopicStr == BLINDS_MQTT_CLIENT_NAME "/BUTTON")
  {
    Serial.print("MQTT MSG: ");
    Serial.print(BLINDS_MQTT_CLIENT_NAME "/BUTTON");
    Serial.print(" \"");
    Serial.print(PayLoadStr);
    Serial.println("\"");
  }
}
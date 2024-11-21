//https://youtu.be/1O_1gUFumQM
//https://github.com/thehookup/Motorized_MQTT_Blinds




#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
#include <AH_EasyDriver.h>  //http://www.alhin.de/arduino/downloads/AH_EasyDriver_20120512.zip


#define USER_SSID                 "*****"
#define USER_PASSWORD             "*****"
#define USER_MQTT_SERVER          "192.168.1.100"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "mq"
#define USER_MQTT_PASSWORD        "*****"
#define USER_MQTT_CLIENT_NAME     "BlindsMCU"



#define STEPPER_SPEED             35
#define STEPPER_STEPS_PER_REV     1028
#define STEPPER_MICROSTEPPING     0
#define DRIVER_INVERTED_SLEEP     1


#define STEPPER_DIR_PIN           D7
#define STEPPER_STEP_PIN          D8
#define STEPPER_SLEEP_PIN         D0
#define STEPPER_MICROSTEP_1_PIN   14
#define STEPPER_MICROSTEP_2_PIN   12



#define STEPS_TO_CLOSE              12
#define STEPS_TO_CLOSE_2            10
#define STEPS_TO_CLOSE_3            11
#define STEPS_TO_CLOSE_4            10
#define STEPS_TO_CLOSE_5            10
#define STEPS_TO_CLOSE_6            10
#define STEPS_TO_CLOSE_7            12

const int EN_PIN     =        12;//   D6
const int EN_PIN_2    =       14;//   D5
const int EN_PIN_3    =       2;//   D4
const int EN_PIN_4    =       0;//   D3
const int EN_PIN_5     =      4;//   D2
const int EN_PIN_6    =       5;//   D1
const int EN_PIN_7    =       16;//   D0



WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
AH_EasyDriver shadeStepper(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN , STEPPER_STEP_PIN, STEPPER_MICROSTEP_1_PIN, STEPPER_MICROSTEP_2_PIN, STEPPER_SLEEP_PIN);

bool Serial_Enable = false;

bool boot = true;

bool Boot_Stepper_1 = true;
bool Boot_Stepper_2 = true;
bool Boot_Stepper_3 = true;
bool Boot_Stepper_4 = true;
bool Boot_Stepper_5 = true;
bool Boot_Stepper_6 = true;
bool Boot_Stepper_7 = true;


int currentPosition  = 6;
int currentPosition2 = 6;
int currentPosition3 = 6;
int currentPosition4 = 6;
int currentPosition5 = 6;
int currentPosition6 = 6;
int currentPosition7 = 6;

int newPosition  = 6;
int newPosition2 = 6;
int newPosition3 = 6;
int newPosition4 = 6;
int newPosition5 = 6;
int newPosition6 = 6;
int newPosition7 = 6;

char positionPublish[50];
bool moving = false;
char charPayload[50];

const char* ssid = USER_SSID ;
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME ;

bool CurrentStepper1 = false;
bool CurrentStepper2 = false;
bool CurrentStepper3 = false;
bool CurrentStepper4 = false;
bool CurrentStepper5 = false;
bool CurrentStepper6 = false;
bool CurrentStepper7 = false;



void setup_wifi() {

  if (Serial_Enable) {
    Serial.println("setup_wifi()");
  }
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (Serial_Enable) {
      Serial.print(".");
    }
  }
  if (Serial_Enable) {
    Serial.println();
  }
}

void reconnect() {
  if (Serial_Enable) {
    Serial.println("reconnect()");
  }
  int retries = 0;

  while (!client.connected()) {
    if (retries < 150)
    {
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass))
      {
        if (Serial_Enable) {
          Serial.println("Connected");
        }
        if (boot) {
          if (!Boot_Stepper_1 && !Boot_Stepper_2 && !Boot_Stepper_3 && !Boot_Stepper_4 && !Boot_Stepper_5 && !Boot_Stepper_6 && !Boot_Stepper_7) {
            boot = false;
          }
        }

        if (!boot) {
          if (Serial_Enable) {
            Serial.println("Boot = false");
          }
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn", "Reconnected");
        }
        else {
          if (Serial_Enable) {
            Serial.println("Boot = true");
          }
          Boot_Stepper_1 = true;
          Boot_Stepper_2 = true;
          Boot_Stepper_3 = true;
          Boot_Stepper_4 = true;
          Boot_Stepper_5 = true;
          Boot_Stepper_6 = true;
          Boot_Stepper_7 = true;
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn", "Rebooted");
        }
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand");
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand2");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand2");
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand3");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand3");
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand4");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand4");
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand5");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand5");
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand6");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand6");
        client.subscribe(USER_MQTT_CLIENT_NAME"/blindsCommand7");
        client.subscribe(USER_MQTT_CLIENT_NAME"/positionCommand7");
      }
      else
      {
        retries++;
        delay(5000);
      }
    }
    if (retries > 149)
    {
      ESP.restart();
      if (Serial_Enable) {
        Serial.println("ESP Restart");
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length)
{
  if (Serial_Enable) {
    Serial.println("callback()");
  }
  String newTopic = topic;
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  newPayload.toCharArray(charPayload, newPayload.length() + 1);


  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      if (Serial_Enable) {
        Serial.println("STOP");
      }
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand")
  {
    if (Boot_Stepper_1) {
      if (Serial_Enable) {
        Serial.println("Position Update 1");
      }
      newPosition = intPayload;
      currentPosition = intPayload;
      Boot_Stepper_1 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 1");
      }
      CurrentStepper1 = true;
      newPosition = intPayload;
    }
  }

  //============================================================================================//
  //============================================================================================//
  //============================================================================================//

  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand2")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand2");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand2", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE_2;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand2", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      if (Serial_Enable) {
        Serial.println("STOP");
      }
      String temp_str = String(currentPosition2);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand2", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand2")
  {

    if (Boot_Stepper_2) {
      if (Serial_Enable) {
        Serial.println("Position Update 2");
      }
      newPosition2 = intPayload;
      currentPosition2 = intPayload;
      Boot_Stepper_2 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 2");
      }
      CurrentStepper2 = true;
      newPosition2 = intPayload;
    }
  }

  //============================================================================================//
  //============================================================================================//
  //============================================================================================//

  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand3")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand3");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand3", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE_3;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand3", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      if (Serial_Enable) {
        Serial.println("STOP");
      }
      String temp_str = String(currentPosition3);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand3", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand3")
  {
    if (Boot_Stepper_3) {
      if (Serial_Enable) {
        Serial.println("Position Update 3");
      }
      newPosition3 = intPayload;
      currentPosition3 = intPayload;
      Boot_Stepper_3 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 3");
      }
      CurrentStepper3 = true;
      newPosition3 = intPayload;
    }
  }

  //============================================================================================//
  //============================================================================================//
  //============================================================================================//

  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand4")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand4");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand4", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE_4;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand4", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      if (Serial_Enable) {
        Serial.println("STOP");
      }
      String temp_str = String(currentPosition4);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand4", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand4")
  {
    if (Boot_Stepper_4) {
      if (Serial_Enable) {
        Serial.println("Position Update 4");
      }
      newPosition4 = intPayload;
      currentPosition4 = intPayload;
      Boot_Stepper_4 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 4");
      }
      CurrentStepper4 = true;
      newPosition4 = intPayload;
    }
  }

  //============================================================================================//
  //============================================================================================//
  //============================================================================================//

  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand5")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand5");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand5", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE_5;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand5", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      if (Serial_Enable) {
        Serial.println("STOP");
      }
      String temp_str = String(currentPosition5);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand5", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand5")
  {
    if (Boot_Stepper_5) {
      if (Serial_Enable) {
        Serial.println("Position Update 5");
      }
      newPosition5 = intPayload;
      currentPosition5 = intPayload;
      Boot_Stepper_5 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 5");
      }
      CurrentStepper5 = true;
      newPosition5 = intPayload;
    }
  }

  //============================================================================================//
  //============================================================================================//
  //============================================================================================//

  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand6")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand6");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand6", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE_6;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand6", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      if (Serial_Enable) {
        Serial.println("STOP");
      }
      String temp_str = String(currentPosition6);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand6", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand6")
  {
    if (Boot_Stepper_6) {
      if (Serial_Enable) {
        Serial.println("Position Update 6");
      }
      newPosition6 = intPayload;
      currentPosition6 = intPayload;
      Boot_Stepper_6 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 6");
      }
      CurrentStepper6 = true;
      newPosition6 = intPayload;
    }
  }

  //============================================================================================//
  //============================================================================================//
  //============================================================================================//

  if (newTopic == USER_MQTT_CLIENT_NAME"/blindsCommand7")
  {
    if (Serial_Enable) {
      Serial.println("/blindsCommand7");
    }
    if (newPayload == "OPEN")
    {
      if (Serial_Enable) {
        Serial.println("OPEN");
      }
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand7", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      if (Serial_Enable) {
        Serial.println("CLOSE");
      }
      int stepsToClose = STEPS_TO_CLOSE_7;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand7", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      String temp_str = String(currentPosition7);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionCommand7", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/positionCommand7")
  {
    if (Boot_Stepper_7) {
      if (Serial_Enable) {
        Serial.println("Position Update 7");
      }
      newPosition7 = intPayload;
      currentPosition7 = intPayload;
      Boot_Stepper_7 = false;
    }
    else {
      if (Serial_Enable) {
        Serial.println("Position Command 7");
      }
      CurrentStepper7 = true;
      newPosition7 = intPayload;
    }
  }

}


void processStepper() {
  //  if (Serial_Enable) {
  //    Serial.println("processStepper()");
  //  }
  if (CurrentStepper1) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper1");
    }
    while (newPosition > currentPosition)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition++;
      moving = true;
    }
    while (newPosition < currentPosition)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition--;
      moving = true;
    }
    digitalWrite(EN_PIN, HIGH);
    if (newPosition == currentPosition)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper1 = false;
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState", positionPublish);
      moving = false;
    }
  }
  //======================================//
  //======================================//

  if (CurrentStepper2) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper2");
    }
    while (newPosition2 > currentPosition2)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN_2, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition2++;
      moving = true;
    }
    while (newPosition2 < currentPosition2)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN_2, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition2--;
      moving = true;
    }
    digitalWrite(EN_PIN_2, HIGH);
    if (newPosition2 == currentPosition2)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper2 = false;
      String temp_str = String(currentPosition2);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState2", positionPublish);
      moving = false;
    }
  }
  //======================================//
  //======================================//

  if (CurrentStepper3) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper3");
    }
    while (newPosition3 > currentPosition3)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN_3, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition3++;
      moving = true;
    }
    while (newPosition3 < currentPosition3)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN_3, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition3--;
      moving = true;
    }
    digitalWrite(EN_PIN_3, HIGH);
    if (newPosition3 == currentPosition3)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper3 = false;
      String temp_str = String(currentPosition3);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState3", positionPublish);
      moving = false;
    }
  }
  //======================================//
  //======================================//

  if (CurrentStepper4) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper4");
    }
    while (newPosition4 > currentPosition4)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN_4, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition4++;
      moving = true;
    }
    while (newPosition4 < currentPosition4)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN_4, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition4--;
      moving = true;
    }
    digitalWrite(EN_PIN_4, HIGH);
    if (newPosition4 == currentPosition4)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper4 = false;
      String temp_str = String(currentPosition4);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState4", positionPublish);
      moving = false;
    }

  }
  //======================================//
  //======================================//

  if (CurrentStepper5) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper5");
    }
    while (newPosition5 > currentPosition5)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN_5, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition5++;
      moving = true;
    }
    while (newPosition5 < currentPosition5)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN_5, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition5--;
      moving = true;
    }
    digitalWrite(EN_PIN_5, HIGH);
    if (newPosition5 == currentPosition5)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper5 = false;
      String temp_str = String(currentPosition5);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState5", positionPublish);
      moving = false;
    }
  }
  //======================================//
  //======================================//

  if (CurrentStepper6) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper6");
    }
    while (newPosition6 > currentPosition6)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN_6, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition6++;
      moving = true;
    }
    while (newPosition6 < currentPosition6)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN_6, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition6--;
      moving = true;
    }
    digitalWrite(EN_PIN_6, HIGH);
    if (newPosition6 == currentPosition6)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper6 = false;
      String temp_str = String(currentPosition6);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState6", positionPublish);
      moving = false;
    }
  }
  //======================================//
  //======================================//

  if (CurrentStepper7) {
    if (Serial_Enable) {
      Serial.println("CurrentStepper7");
    }
    while (newPosition7 > currentPosition7)
    {
      if (Serial_Enable) {
        Serial.println("Forward");
      }
      digitalWrite(EN_PIN_7, LOW);
      shadeStepper.move(80, FORWARD);
      currentPosition7++;
      moving = true;
    }
    while (newPosition7 < currentPosition7)
    {
      if (Serial_Enable) {
        Serial.println("Backward");
      }
      digitalWrite(EN_PIN_7, LOW);
      shadeStepper.move(80, BACKWARD);
      currentPosition7--;
      moving = true;
    }
    digitalWrite(EN_PIN_7, HIGH);
    if (newPosition7 == currentPosition7)
    {
      if (Serial_Enable) {
        Serial.println("Done");
      }
      CurrentStepper7 = false;
      String temp_str = String(currentPosition7);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/positionState7", positionPublish);
      moving = false;
    }
  }
}

void checkIn()
{
  if (Serial_Enable) {
    Serial.println("checkIn()");
  }
  client.publish(USER_MQTT_CLIENT_NAME"/checkIn", "OK");
}



void setup() {
  if (Serial_Enable) {
    Serial.begin(115200);
  }
  pinMode(EN_PIN, OUTPUT);
  pinMode(EN_PIN_2, OUTPUT);
  pinMode(EN_PIN_3, OUTPUT);
  pinMode(EN_PIN_4, OUTPUT);
  pinMode(EN_PIN_5, OUTPUT);
  pinMode(EN_PIN_6, OUTPUT);
  pinMode(EN_PIN_7, OUTPUT);
  digitalWrite(EN_PIN, HIGH);
  digitalWrite(EN_PIN_2, HIGH);
  digitalWrite(EN_PIN_3, HIGH);
  digitalWrite(EN_PIN_4, HIGH);
  digitalWrite(EN_PIN_5, HIGH);
  digitalWrite(EN_PIN_6, HIGH);
  digitalWrite(EN_PIN_7, HIGH);

  shadeStepper.setMicrostepping(STEPPER_MICROSTEPPING);
  shadeStepper.setSpeedRPM(STEPPER_SPEED);

  WiFi.mode(WIFI_STA);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin();
  delay(10);
  timer.setInterval(90000, checkIn);
  if (Serial_Enable) {
    Serial.println("setup()");
  }
}

void loop()
{
  digitalWrite(EN_PIN, HIGH);
  digitalWrite(EN_PIN_2, HIGH);
  digitalWrite(EN_PIN_3, HIGH);
  digitalWrite(EN_PIN_4, HIGH);
  digitalWrite(EN_PIN_5, HIGH);
  digitalWrite(EN_PIN_6, HIGH);
  digitalWrite(EN_PIN_7, HIGH);
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  processStepper();
  timer.run();
}

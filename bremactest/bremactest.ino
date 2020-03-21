/*
 * 
 */

#include <EEPROM.h>

const int led1_pin = 9;
const int led2_pin = 8;

const int solenoid1_pin = 5;
const int solenoid2_pin = 4;

typedef struct _bremacSettings_t {
  uint16_t _size;    // settings size in EEPROM to detect if settings have been saved
  uint16_t _version; // version of the settings
  uint16_t s1t_ms;  // s1 on time
  uint16_t s1p_ms;  // pause after s1 time
  uint16_t s2t_ms; // s2 on time
  uint16_t s2p_ms;  // pause after s2 time
} bremacSettings_t;

#define FW_VERSION 1

static bremacSettings_t defaultSettings = {
  ._size = sizeof(bremacSettings_t),
  ._version = FW_VERSION,
  .s1t_ms = 1000,
  .s1p_ms = 500,
  .s2t_ms = 2000,
  .s2p_ms = 500
};

bremacSettings_t settings;

typedef enum _states_t {
  STARTUP,
  MANUAL,
  S1_ON,
  S1_PAUSE,
  S2_ON,
  S2_PAUSE
} states_t;

states_t currentState = STARTUP;
uint32_t nextStateChange = 0;       //! TODO: check timer overflow

String inputString = "";         // a string to hold incoming data

void loadSettings()
{
  EEPROM.get(0, settings);
  if (settings._size != sizeof(settings) || settings._version != FW_VERSION)
  {
    //revert to default settings
    settings = defaultSettings;
  }
}

void saveSettings()
{
  EEPROM.put(0, settings);
}

void setup() {  
  loadSettings();
  
  // initialize serial:
  Serial.begin(9600);
  // reserve 100 bytes for the inputString:
  inputString.reserve(100);

  pinMode(led1_pin, OUTPUT);
  pinMode(led2_pin, OUTPUT);
  pinMode(solenoid1_pin, OUTPUT);
  pinMode(solenoid2_pin, OUTPUT);  
  
  Serial.print("bremactest");
  Serial.print(" v=");
  Serial.print(settings._version);
  Serial.print(" s1t=");
  Serial.print(settings.s1t_ms);
  Serial.print(" s1p=");
  Serial.print(settings.s1p_ms);
  Serial.print(" s2t=");
  Serial.print(settings.s2t_ms);
  Serial.print(" s2p=");
  Serial.print(settings.s2p_ms);
  Serial.println();
}

boolean hasSerialInputString() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if ((inChar == '\n') || (inChar == '\r')) {
      return true;
    } 
    else {
      // add it to the inputString:
      if (inputString.length() > 50) {
        inputString = ""; 
      }
      inputString += inChar;
    }          
  }
  return false;
}

void nextState() {
  if (nextStateChange > millis()) {
    return;
  }
  
  // from current state  
  switch (currentState) {
    case S1_ON:
      digitalWrite(solenoid1_pin, LOW);
      digitalWrite(led1_pin, LOW);
      nextStateChange = millis() + settings.s1p_ms;
      currentState = S1_PAUSE;
      break;
    case S1_PAUSE:
      digitalWrite(solenoid2_pin, HIGH);
      digitalWrite(led2_pin, HIGH);
      nextStateChange = millis() + settings.s2t_ms;
      currentState = S2_ON;
      break;
    case S2_ON:   
      digitalWrite(solenoid2_pin, LOW);
      digitalWrite(led2_pin, LOW);
      nextStateChange = millis() + settings.s2p_ms;
      currentState = S2_PAUSE;
      break;

    case STARTUP:  // these states are handled togethers
    case S2_PAUSE:  
      digitalWrite(solenoid1_pin, HIGH);
      digitalWrite(led1_pin, HIGH);
      nextStateChange = millis() + settings.s1t_ms;
      currentState = S1_ON;
      break;
    
    case MANUAL:
      return;
  }
}

boolean tryGetInputStringParamValue(const char* param, uint16_t *value) {
  if (inputString.startsWith(param)) {    
    // Serial.println(strlen(param));
    String newValue = inputString.substring(strlen(param));
    // Serial.println(newValue);
    *value = newValue.toInt();
    return true;
  } else {
    return false;
  }
}

void checkSerialInput() {
  if (hasSerialInputString()) {    
    //handle the string input
    uint16_t newValue;
    if (inputString.compareTo("s1h") == 0) {
      digitalWrite(solenoid1_pin, HIGH);
      digitalWrite(led1_pin, HIGH);
    } 
    else if (inputString.compareTo("s1l") == 0) {
      digitalWrite(solenoid1_pin, LOW);
      digitalWrite(led1_pin, LOW);
    } 
    else if (inputString.compareTo("s2h") == 0) {
      digitalWrite(solenoid2_pin, HIGH);
      digitalWrite(led2_pin, HIGH);
    } 
    else if (inputString.compareTo("s2l") == 0) {
      digitalWrite(solenoid2_pin, LOW);
      digitalWrite(led2_pin, LOW);
    } else if (tryGetInputStringParamValue("s1t=", &newValue)) {
      settings.s1t_ms = newValue;
      saveSettings();
    } else if (tryGetInputStringParamValue("s1p=", &newValue)) {
      settings.s1p_ms = newValue;
      saveSettings();
    } else if (tryGetInputStringParamValue("s2t=", &newValue)) {
      settings.s2t_ms = newValue;
      saveSettings();
    } else if (tryGetInputStringParamValue("s2p=", &newValue)) {
      settings.s2p_ms = newValue;
      saveSettings();
    }
    else {
      Serial.print("ERROR: ");
      Serial.println(inputString);
      inputString = "";
      return;
    }
    
    Serial.println(inputString);

    // clear the string:
    inputString = "";
  }  
}

void loop()
{
  // check for the input
  checkSerialInput();
  
  // verify next step:
  nextState();
}

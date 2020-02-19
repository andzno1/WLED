/*
 * This file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2750+ are reserved for your custom use case. (if you extend #define EEPSIZE in wled01_eeprom.h)
 * bytes 2400+ are currently ununsed, but might be used for future wled features
 */

//Use userVar0 and userVar1 (API calls &U0=,&U1=, uint16_t)

#define RXD2 16
#define TXD2 17

const char STX = 0x02;
const char ETX = 0x03;

const size_t ANSWER_SIZE = 4096;
const size_t JSON_SIZE = 8192;

StaticJsonDocument<JSON_SIZE> serialDoc;
String readData;

//gets called once at boot. Do all initialization that doesn't depend on network here
void userSetup()
{
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial1.begin(38400, SERIAL_8N1, RXD2, TXD2);
}

//gets called every time WiFi is (re-)connected. Initialize own network interfaces here
void userConnected()
{

}

//loop. You can use "if (WLED_CONNECTED)" to check for successful connection
void userLoop()
{
  
  while (Serial1.available()) {
    const char c = Serial1.read();

    switch (c) {
      case STX:
        readData = "";
        break;
      case ETX:
        handleCommand(readData);
        break;
      default:
        readData += c;
        break;
    }
    
  }

}

void serveJson(String& path, char* answer) {
  JsonObject doc = serialDoc.to<JsonObject>();

  if (path == "/json") {
    JsonObject state = doc.createNestedObject("state");
    serializeState(state);
    JsonObject info  = doc.createNestedObject("info");
    serializeInfo(info);
    doc["effects"]  = serialized((const __FlashStringHelper*)JSON_mode_names);
    doc["palettes"] = serialized((const __FlashStringHelper*)JSON_palette_names);
    serializeJson(serialDoc, answer, ANSWER_SIZE);
  } else if (path == "/json/state") {
    JsonObject state = doc.createNestedObject("state");
    serializeState(state);
    serializeJson(serialDoc, answer, ANSWER_SIZE);
  } else if (path == "/json/info") {
    JsonObject info  = doc.createNestedObject("info");
    serializeInfo(info);
    serializeJson(serialDoc, answer, ANSWER_SIZE);
  } else if (path == "/json/live") {
    serveLiveLeds(answer);
  } else if (path == "/json/effects") {
    doc["effects"]  = serialized((const __FlashStringHelper*)JSON_mode_names);
    serializeJson(serialDoc, answer, ANSWER_SIZE);
  } else if (path == "/json/palettes") {
    doc["palettes"] = serialized((const __FlashStringHelper*)JSON_palette_names);
    serializeJson(serialDoc, answer, ANSWER_SIZE);
  } else {
    strncpy(answer, "{\"error\":\"Not implemented\"}", ANSWER_SIZE);
  }
}

void handleCommand(String command) {
  bool sendAnswer = true;
  char answer[ANSWER_SIZE];

  DeserializationError err = deserializeJson(serialDoc, command);
  switch (err.code()) {
    case DeserializationError::Ok:
      {
        JsonObject state = serialDoc["state"];
        JsonVariant path = serialDoc["path"];
        if (state.isNull() && !path.isNull()) {
          // No state object provided. Serve JSON.
          String pathStr = path.as<String>();
          serveJson(pathStr, answer);
        } else if (!path.isNull()) {
          String pathStr = path.as<String>();
          if (pathStr == "/json" || pathStr == "/json/state") {
            // State object and correct path provided. Setting internal state.  
            if (deserializeState(state)) {
              serveJson(pathStr, answer);
            } else {
              sendAnswer = false;
            }
          }
          
        }
      }
      break;
    case DeserializationError::InvalidInput:
        Serial.println(F("Invalid input"));
        strncpy(answer, "{\"error\":\"Invalid input\"}", ANSWER_SIZE);
        break;
    case DeserializationError::NoMemory:
        Serial.println(F("Not enough memory"));
        strncpy(answer, "{\"error\":\"Not enough memory\"}", ANSWER_SIZE);
        break;
    default:
        Serial.println(F("Deserialization failed"));
        strncpy(answer, "{\"error\":\"Deserialization failed\"}", ANSWER_SIZE);
        break;
  }

  if (sendAnswer) {
    DEBUG_PRINT("Answer: ");
    DEBUG_PRINT(STX);
    DEBUG_PRINT(answer);
    DEBUG_PRINT(ETX);
    DEBUG_PRINT("\n")
    
    Serial1.print(STX);  
    Serial1.print(answer);
    Serial1.print(ETX);
  }
}
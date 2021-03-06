using namespace std;

#define MQTT_MAX_PACKET_SIZE 512

// **************************************** INCLUDES ****************************************
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <time.h>
#include <Time.h>
#include <TimeLib.h>
#include "setup.h"
#include "max7219.h"
#include "fonts.h"

// **************************************** GLOBALS ****************************************
const int del = 3000, value = 0;
const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"},
           *on_cmd = "ON", *off_cmd = "OFF";
char msg[50], txt[30], txt2[20];
String hass_states = "";
vector<string> extras;
String buf = "";
long lastMsg = 0, lastMin = 100;
int cnt = 0;
bool stateOn = true, statesChanged = false;

time_t timeNow;

WiFiClient wifiClient;
WiFiClientSecure wifiClientSecure;

PubSubClient client(wifiClient);

// **************************************** PRINT ****************************************
int dualChar = 0;

unsigned char convertPolish(unsigned char _c) {
  unsigned char c = _c;
  if (c == 196 || c == 197 || c == 195) {
    dualChar = c;
    return 0;
  }
  if (dualChar) {
    switch (_c) {
    case 133:
      c = 1 + '~';
      break; // 'ą'
    case 135:
      c = 2 + '~';
      break; // 'ć'
    case 153:
      c = 3 + '~';
      break; // 'ę'
    case 130:
      c = 4 + '~';
      break; // 'ł'
    case 132:
      c = dualChar == 197 ? 5 + '~' : 10 + '~';
      break; // 'ń' and 'Ą'
    case 179:
      c = 6 + '~';
      break; // 'ó'
    case 155:
      c = 7 + '~';
      break; // 'ś'
    case 186:
      c = 8 + '~';
      break; // 'ź'
    case 188:
      c = 9 + '~';
      break; // 'ż'
    //case 132: c = 10+'~'; break; // 'Ą'
    case 134:
      c = 11 + '~';
      break; // 'Ć'
    case 152:
      c = 12 + '~';
      break; // 'Ę'
    case 129:
      c = 13 + '~';
      break; // 'Ł'
    case 131:
      c = 14 + '~';
      break; // 'Ń'
    case 147:
      c = 15 + '~';
      break; // 'Ó'
    case 154:
      c = 16 + '~';
      break; // 'Ś'
    case 185:
      c = 17 + '~';
      break; // 'Ź'
    case 187:
      c = 18 + '~';
      break; // 'Ż'
    default:
      break;
    }
    dualChar = 0;
    return c;
  }
  switch (_c) {
  case 185:
    c = 1 + '~';
    break;
  case 230:
    c = 2 + '~';
    break;
  case 234:
    c = 3 + '~';
    break;
  case 179:
    c = 4 + '~';
    break;
  case 241:
    c = 5 + '~';
    break;
  case 243:
    c = 6 + '~';
    break;
  case 156:
    c = 7 + '~';
    break;
  case 159:
    c = 8 + '~';
    break;
  case 191:
    c = 9 + '~';
    break;
  case 165:
    c = 10 + '~';
    break;
  case 198:
    c = 11 + '~';
    break;
  case 202:
    c = 12 + '~';
    break;
  case 163:
    c = 13 + '~';
    break;
  case 209:
    c = 14 + '~';
    break;
  case 211:
    c = 15 + '~';
    break;
  case 140:
    c = 16 + '~';
    break;
  case 143:
    c = 17 + '~';
    break;
  case 175:
    c = 18 + '~';
    break;
  default:
    break;
  }
  return c;
}

int charWidth(const char ch, const uint8_t *data, int offs) {
  char c = convertPolish(ch);
  if (c < offs || c > MAX_CHAR)
    return 0;
  c -= offs;
  int len = pgm_read_byte(data);
  return pgm_read_byte(data + 1 + c * len);
}

int stringWidth(const char *s, const uint8_t *data, int offs) {
  int wd = 0;
  while (*s)
    wd += 1 + charWidth(*s++, data, offs);
  return wd - 1;
}

int stringWidth(String str, const uint8_t *data, int offs) {
  return stringWidth(str.c_str(), data, offs);
}

int showChar(char ch, const uint8_t *data) {
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  scr[NUM_MAX * 8] = 0;
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i + 1] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  return w;
}

void printCharWithShift(unsigned char c, int shiftDelay, const uint8_t *data, int offs) {
  c = convertPolish(c);
  if (c < offs || c > MAX_CHAR)
    return;
  c -= offs;
  int w = showChar(c, data);
  for (int i = 0; i < w + 1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

//#define SPEED (strlen(s)+wdR<5 ? shiftDelay*3/2 : shiftDelay)
//#define SPEED2 (strlen(s)<5 ? shiftDelay*3/2 : shiftDelay)
#define SPEED shiftDelay
#define SPEED2 shiftDelay

void printStringWithShift(const char *s, int shiftDelay, const uint8_t *data, int offs) {
  while (*s)
    printCharWithShift(*s++, shiftDelay, data, offs);
}

void printStringWithShift(String str, int shiftDelay, const uint8_t *data, int offs) {
  printStringWithShift(str.c_str(), shiftDelay, data, offs);
}

// void printStringLeft(const char *s, int shiftDelay, const uint8_t *data, int offs)
// {
//   int wd = stringWidth(s, data, offs);

// }

// void printStringLeft(String str, int shiftDelay, const uint8_t *data, int offs)
// {
//   printStringLeft(str.c_str(), shiftDelay, data, offs);
// }

void printStringCenter(const char *s, int shiftDelay, const uint8_t *data, int offs) {
  int wd = stringWidth(s, data, offs);
  int wdL = (NUM_MAX * 8 - wd) / 2;
  int wdR = NUM_MAX * 8 - wdL - wd;
  while (wdL > 0) {
    printCharWithShift('_', shiftDelay, data, ' ');
    wdL--;
  }
  while (*s)
    printCharWithShift(*s++, SPEED, data, offs);
  while (wdR > 0) {
    printCharWithShift('_', SPEED, data, ' ');
    wdR--;
  }
}

void printStringCenter(String str, int shiftDelay, const uint8_t *data, int offs) {
  printStringCenter(str.c_str(), shiftDelay, data, offs);
}

// converts int to string
// centers string on the display
// chooses proper font for string/number length
void printValueWithShift(long val, int shiftDelay, int sign) {
  // for 4*8=32
  //const uint8_t *digits = digits5x7;       // good for max 5 digits = 5*6=30 (99 999)
  //if(val>1999999) digits = digits3x7;      // good for max 8 digits = 8*4=32
  //else if(val>99999) digits = digits4x7;   // good for max 6-7 digits = 6*5+2=32 (1 999 999)

  // for 6*8=48 no gaps
  //const uint8_t *digits = digits5x7;         // good for max 8 digits = 8d*6p=48 (99 999 999)
  //if(val>1999999999) digits = digits3x7;     // good for max 12 digits = 12d*4p=48
  //else if(val>99999999) digits = digits4x7;  // good for max 9-10 digits = 9d*5p+2p=47 (1999 999 999)

  // for 6*8=48 with gaps
  const uint8_t *digits = digits5x7; // good for max 7-8 digits = 18+1+18+1+6+3=47 (19 999 999)
  if (val > 999999999)
    digits = digits3x7; // good for max 11 digits = 12+1+12+1+12+1+8=47 (99 999 999 999>max32b!)
  else if (val > 19999999)
    digits = digits4x7; // good for max 9 digits = 15+1+15+1+15=47 (999 999 999)
  int sg = (val > 0) ? 1 : -1;
  String str = String((sg < 0) ? -val : val);
  int len = str.length();
  int gaps = 1;
  if (len > 4 && gaps) {
    int st = len - (len / 3) * 3;
    String copy = str;
    str = "";
    for (int i = st; i <= len; i += 3) {
      str += copy.substring((i - 3 < 0) ? 0 : i - 3, i);
      if (i < len && i > 0)
        str += "&";
    }
  }
  if (sign) {
    str = ((sg < 0) ? "-" : "+") + str;
  }
  const char *s = str.c_str();
  int wd = 0;
  while (*s)
    wd += 1 + charWidth(*s++, digits, ' ');
  wd--;
  int wdL = (NUM_MAX * 8 - wd) / 2;
  int wdR = NUM_MAX * 8 - wdL - wd;
  s = str.c_str();
  while (wdL > 0) {
    printCharWithShift(' ', shiftDelay, digits, ' ');
    wdL--;
  }
  while (*s)
    printCharWithShift(*s++, SPEED, digits, ' ');
  while (wdR > 0) {
    printCharWithShift(' ', SPEED, digits, ' ');
    wdR--;
  }
}

unsigned int strS = 0, strE = 0, i;
// adds " char to token end
// uses global buf to avoid frequent memory reallocation
String find(String token, const char endCh = ',', int ofs = 0) {
  strS = buf.indexOf(token + "\"");
  if (strS < 0)
    return "";
  if (buf.length() - strS < token.length() + 2 + ofs)
    return "";
  strS += token.length() + 2 + ofs;
  for (i = strS; i < buf.length(); i++)
    if (buf[i] == endCh || buf[i] == '}' || buf[i] == ']')
      break;
  strE = i;
  return buf.substring(strS, strE);
}

// adds " char to token and subtoken end
String findSub(String token, String subToken) {
  strS = buf.indexOf(token + "\"");
  if (strS < 0)
    return "";
  if (buf.length() - strS < token.length() + 2)
    return "";
  strS += token.length() + 2;
  int strS2 = buf.indexOf(subToken + "\"", strS);
  if (strS2 < 0)
    return "";
  if (buf.length() - strS2 < subToken.length() + 2)
    return "";
  strS2 += subToken.length() + 2;
  for (i = strS2; i < buf.length(); i++)
    if (buf[i] == ',' || buf[i] == '}' || buf[i] == ']')
      break;
  strE = i;
  return buf.substring(strS2, strE);
}

void configureTime() {
  Serial.println("");
  configTime(timeOffset * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
}

// **************************************** TIME ****************************************
void updateTime() {
  // Serial.println("updateTime()");
  timeNow = time(nullptr);
  // Serial.print(ctime(&timeNow));
}

// **************************************** EXTRAS ****************************************
bool updateExtras() {
  Serial.println("updateExtras()");

  if (hass_states == "") {
    Serial.println("States blank. Returning");
    return false;
  }

  const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 120;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonArray &states = jsonBuffer.parseArray(hass_states);

  int arrayLength = states.size();

  if (!wifiClientSecure.connect(hass_host, hass_port)) {
    Serial.println("connection failed");
    return false;
  }

  extras.clear();

  for (int i = 0; i <= arrayLength - 1; i++) {
    JsonObject &statesObject = states[i];
    const char *state = statesObject["state"];

    char hostCmd[50], requestCmd[200];
    sprintf(hostCmd, "Host: %s", hass_host);
    sprintf(requestCmd, "GET /api/states/%s?api_password=%s HTTP/1.1", state, hass_api_password);

    wifiClientSecure.println(requestCmd);
    wifiClientSecure.println(hostCmd);
    // wifiClientSecure.println("Accept: application/json");
    wifiClientSecure.println("Connection: close");
    wifiClientSecure.println();

    while (wifiClientSecure.connected()) {
      String line = wifiClientSecure.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        Serial.println(line);
        break;
      }
    }

    while (wifiClientSecure.available()) {
      Serial.println("Reading string");
      String payloadStr = wifiClientSecure.readStringUntil('\n');
      Serial.println(payloadStr);

      const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 240;
      DynamicJsonBuffer jsonBuffer(bufferSize);

      JsonObject &root = jsonBuffer.parseObject(payloadStr);
      if (!root.success()) {
        Serial.println("Parsing root failed");
        return false;
      }
      Serial.println("Parsed root");

      const char *state = root["state"];
      string extra = state;

      // Add measurement to extra
      if (statesObject.containsKey("measurement")) {
        const char *measurement = statesObject["measurement"];
        extra.append(measurement);
      } else {
        JsonObject &jsonAttributes = root["attributes"];
        if (jsonAttributes.success()) {
          const char *measurement = jsonAttributes["unit_of_measurement"];
          extra.append(measurement);
        } else {
          Serial.println("Parsing attributes failed");
        }
      }

      Serial.print("Extra: ");
      Serial.println(extra.c_str());

      extras.push_back(extra);
    }
  }
  // wifiClientSecure.stop();
  return true;
}

void showExtra(int pos) {
  sprintf(txt, " %s", extras[i].c_str());
  Serial.println(txt);
  printStringWithShift(txt, scrollDelay, font, ' ');
}

void showExtras(int pos = -1) {
  Serial.println("showExtras()");

  if (extras.size() < 1) {
    printStringWithShift("  ", scrollDelay, font, ' ');
    return;
  }

  if (pos > -1)
    showExtra(pos);
  else
    for (unsigned int i = 0; i <= extras.size() - 1; i++)
      showExtra(i);
}

void showAll() {
  Serial.println("showAll()");

  showExtras();

  sprintf(txt, "%02d:%02d", hour(timeNow), minute(timeNow));
  sprintf(txt2, "%02d.%02d", day(timeNow), month(timeNow));
  // sprintf(txt2, "%s %02d.%02d", extras.size() > 0 ? extras[0].c_str() : "", day(timeNow), month(timeNow));

  Serial.println(txt);
  Serial.println(txt2);

  int wdL = stringWidth(txt, dig5x8rn, ' ');
  int wdR = stringWidth(txt2, small3x7, ' ');
  int wd = NUM_MAX * 8 - wdL - wdR - 1;

  printStringWithShift(txt, scrollDelay, dig5x8rn, ' '); // time
  while (wd-- > 0)
    printCharWithShift('_', scrollDelay, font, ' ');
  printStringWithShift(txt2, scrollDelay, small3x7, ' '); // date
}

void displayAll() {
  updateTime();
  if (minute(timeNow) != lastMin) {
    sendCmdAll(CMD_INTENSITY, (hour(timeNow) >= 7 && hour(timeNow) <= 16) ? 1 : 0);
    Serial.println("");
    lastMin = minute(timeNow);
    showAll();
  }
  if (second(timeNow) == 10)
    updateExtras();
}

bool processJson(char *message) {
  Serial.print("Message: ");
  Serial.println(message);

  const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 120;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject &root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    } else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  }

  if (root.containsKey("speed")) {
    scrollDelay = root["speed"];
  }

  if (root.containsKey("timeOffset")) {
    timeOffset = root["timeOffset"];
    configureTime();
    updateTime();
  }

  if (root.containsKey("states")) {
    JsonArray &states = root["states"];

    if (!states.success()) {
      Serial.println("parseArray() failed");
    }

    char buffer[states.measureLength() + 1];
    states.printTo(buffer, sizeof(buffer));

    hass_states = buffer;
    statesChanged = true;
  }
  return true;
}

void sendState() {
  const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 120;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject &root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  root["speed"] = scrollDelay;
  root["timeOffset"] = timeOffset;

  if (hass_states != "") {
    Serial.print("hass_states: ");
    Serial.println(hass_states);
    JsonArray &states = jsonBuffer.parseArray(hass_states);
    root["states"] = states;
  }

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.print("Publish: ");
  Serial.println(buffer);
  client.publish(mqtt_state_topic, buffer, true);

  if (statesChanged) {
    updateExtras();
    statesChanged = false;
  }
  showAll();
}

// **************************************** MQTT CALLBACK ****************************************
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  if (!processJson(message))
    return;

  sendState();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_name, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_set_topic);
      Serial.print("Subscribed to: ");
      Serial.println(mqtt_set_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

#ifdef BUTTON_PIN
int buttonState = LOW;

void configureButton() {
  pinMode(BUTTON_PIN, INPUT);
}

void checkButton() {
  int state = digitalRead(BUTTON_PIN);
  if (state != buttonState) {
    buttonState = state;
    Serial.print("Button Changed: ");
    Serial.println(buttonState);
    if (buttonState == (HIGH)) {
      Serial.println("Button Pressed");
      stateOn = !stateOn;
      sendState();
    }
  }
}
#else
void configureButton() {}
void checkButton() {}
#endif

// **************************************** SETUP ****************************************
void setup() {
  buf.reserve(500);
  Serial.begin(9600); // For debugging output
  Serial.println("Start.");
  Serial.println("");

  Serial.print("Connecting to WiFi ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting", 15, font, ' ');
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  ArduinoOTA.setHostname(ota_hostname);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);
  printStringWithShift((String("My IP: ") + WiFi.localIP().toString()).c_str(), scrollDelay, font, ' ');

  configureTime();
  configureButton();
}

// **************************************** MAIN LOOP ****************************************
void loop() {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
    ArduinoOTA.handle();

    checkButton();

    if (stateOn)
      displayAll();
    else
      printStringWithShift("                                                                ", scrollDelay, font, ' ');
  }
  delay(10);
}

using namespace std;

#define NUM_MAX 8 // Amount of 8x8 displays avaliable
#define ROTATE 90

#define CLK_PIN 12 // D6
#define CS_PIN 13  // D7
#define DIN_PIN 15 // D8

#define mqtt_state_topic "display/matrix001"
#define mqtt_set_topic "display/matrix001/set"

const char *ssid = "ssid";              // SSID of local network
const char *password = "pass";          // Password on network
const char *ota_password = "pass";      // OTA Password
const char *ota_hostname = "matrix001"; // OTA Hostname

const char *mqtt_name = "matrix001";
const char *mqtt_server = "domain_or_ip";
const long mqtt_port = 1883;
const char *mqtt_user = "user";
const char *mqtt_password = "pass";

const char *hass_host = "domain_or_ip"; // HASS Hostname
const int hass_port = 443;              // HASS Port. Use Port 80 for http and 443 for https
const char *hass_api_password = "pass"; // HASS Password

const long utcOffset = 1;

int timezone = 1; // TZ: Europe/London
int daylightOffset = 0; // Daylight savings time. Offset in hours

unsigned long delayTime = 10000;
int scrollDelay = 40;

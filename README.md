# ESP8266 LED Matrix MQTT Sign [![Build Status](https://travis-ci.org/timmo001/ESP8266-LED-Matrix-MQTT-Sign.svg?branch=master)](https://travis-ci.org/timmo001/ESP8266-LED-Matrix-MQTT-Sign)

ESP8266 LED Matrix MQTT Sign

## Example Hardware Setup



## Software Setup

- Using Atom or VS Code, install [Platform IO](https://platformio.org/platformio-ide)
- Once setup, install the `esp8266` embedded platform
- Build the project (Ctrl+Alt+B) and check for any errors

  > If the build produces an error referencing dependencies, You will need to manually install these libraries:
    - [ArduinoJson](https://platformio.org/lib/show/64/ArduinoJson)
    - [PubSubClient](https://platformio.org/lib/show/89/PubSubClient)
    - [Time](https://platformio.org/lib/show/44/Time)
> Due to an issue with defining the max MQTT packet size in the main file, you will have to manually increase the size in the following step. (If someone knows a workaround for this, please let me know or make a pull request)
- Inside the created hidden directory, find `.piolibdeps\PubSubClient\PubSubClient_ID89\src\PubSubClient.h` and change the line that states something like `#define MQTT_MAX_PACKET_SIZE 128` to `#define MQTT_MAX_PACKET_SIZE 512`
- Upload to your board of choice (Ctrl+Alt+U). This project was created specifically for the `NodeMCU` but can be configured to work with another WiFi board with some tinkering.

## Example Home Assistant Configuration

```yaml
switch:
  - platform: mqtt
    name: "Matrix Clock"
    command_topic: "display/matrix001/set"
    state_topic: "display/matrix001"
    payload_on: '{ "state": "ON" }'
    payload_off: '{ "state": "OFF" }'
    qos: 1

input_number:
  matrix_clock_speed:
    name: 'Matrix Clock Scroll Speed'
    mode: slider
    initial: 40
    min: 10
    max: 100
    step: 10

automation:
  - action:
      - alias: Set Matrix Clock Speed
        service: mqtt.publish
        data_template:
          topic: display/matrix001/set
          payload: '{"speed":{{ trigger.to_state.state | int }}}'
    alias: Matrix Clock Speed
    trigger:
      - platform: state
        entity_id: input_number.matrix_clock_speed

```

## Sample MQTT Payload

### Switch on with speed of 40, BST and a HASS temperature sensor
```json
{
   "state": "ON",
   "speed": 40,
   "timeOffset": 1,
   "states": [
      {
         "state": "sensor.dht22_01_temperature",
         "measurement": "C"
      }
   ]
}
```

### Switch Off
```json
{
   "state": "OFF"
}
```

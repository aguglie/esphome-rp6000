# ESPHome RP6000 Inverter Component

ESPHome library to monitor and (partially) control a RP6000 inverter (known as 'bestione arancione') via RS485.

This project was tested with a 48v 6kW inverter. It may work with other models, but it has not been tested.

## Disclaimer
Working with high voltage can be dangerous. If you are not confident in your ability to work with high voltage, consult a professional electrician.

**The author is not responsible for any damage or injury caused by the use of this software. Use at your own risk.**

## Requirements
* RS485-to-TTL module
* ESP8266 or ESP32 (tested on ESP32)


## Schematics
Below is the connection schematic for the RP6000 inverter to an ESP32 or ESP8266 via an RS485-to-TTL module. 


```
               RS485                     UART-TTL
┌──────────┐              ┌──────────┐                ┌─────────┐
│          │              │          │<----- RX ----->│         │
│          │<----- A ---->│  RS485   │<----- TX ----->│ ESP32/  │
│  RP6000  │<----- B ---->│  to TTL  │<----- GND ---->│ ESP8266 │
│          │              │  module  │<-- 3.3V VCC -->│         │<--- VCC
│          │              │          │                │         │<--- GND
└──────────┘              └──────────┘                └─────────┘
```


## RJ45 Connection
Your inverter's documentation should specify the RS485 communication pins. Here is the pinout for the tested version of the RP6000 inverter:

```
  1  2  3  4  5  6  7  8

1. +5V (not used in this project)
2. GND (not used in this project)
3. B --> Connect to B of RS485-to-TTL module
4. A --> Connect to A of RS485-to-TTL module
5. RXD (not used in this project)
6. TXD (not used in this project)
7. +5V (not used in this project)
8. GND (not used in this project)
```


## Installation and example usage
Start with `esp32-example.yaml` for your configuration. Adapt the file to your specific setup and WiFi settings.
In the provided example, the RS485<>TTL is connected to the pins 4 and 36.


```bash
# Install esphome
pip3 install esphome

# Validate the configuration, create a binary, upload it, and start logs
# If you use a esp8266 run the esp8266-examle.yaml
esphome run esp32-example.yaml
```


Take a look at the [official documentation of the component](https://esphome.io) for additional details.


## Known issues
Commands issued to the inverter receive an 'ACK' response, but the buffer may not clear properly, causing subsequent messages to be scrambled. Contributions to investigate and fix this issue are welcome!


## Debugging
If this component doesn't work out of the box for your device please update your configuration to enable the debug output of the UART component and increase the log level to the see outgoing and incoming serial traffic:


```yaml
logger:
  level: DEBUG
  # Don't write log messages to UART0 (GPIO1/GPIO3) if the inverter is connected to GPIO1/GPIO3
  baud_rate: 0

uart:
  id: uart_0
  baud_rate: 2400
  tx_pin: GPIO1
  rx_pin: GPIO3
  debug:
    direction: BOTH
    dummy_receiver: false
    after:
      delimiter: "\r"
    sequence:
      - lambda: UARTDebug::log_string(direction, bytes);
```


## Contributing
We welcome contributions! Please submit pull requests, report issues, or suggest improvements on our GitHub repository. Follow our coding standards and PR process outlined in the contributing guide.


## Disclaimer
This project is not affiliated with or endorsed by the manufacturer of the RP6000 inverter. Use at your own risk.


## References
* [esphome-pipsolar](https://github.com/syssi/esphome-pipsolar) their project was used as a blueprint for this one.

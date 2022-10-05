# Water Tank Sensor

Uses an ESP8266 and a JSN SR04T to measure the water level, and post it to [io.adafruit.com](https://io.adafruit.com).


## Configuration

Create a config file `config.h` with the following details:

```c++
#ifndef config_h
#define config_h

#define WLAN_SSID  "..."
#define WLAN_PASSWD "..."
const int SLEEPTIME = 30e7; //300 seconds

//io.adafruit.com
#define IO_USERNAME  "..."
#define IO_KEY       "..."
#define IO_FEEDNAME  "..."

#endif
```

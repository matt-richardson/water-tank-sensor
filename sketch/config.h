#ifndef config_h
#define config_h

#define WLAN_SSID  "${{ secrets.WLAN_SSID }}"
#define WLAN_PASSWD "${{ secrets.WLAN_PASSWD }}"
const int SLEEPTIME = 30e7; //300 seconds

//io.adafruit.com
#define IO_USERNAME  "${{ secrets.IO_USERNAME }}"
#define IO_KEY       "${{ secrets.IO_KEY }}"
#define IO_FEEDNAME  "${{ secrets.IO_FEEDNAME }}"

#endif

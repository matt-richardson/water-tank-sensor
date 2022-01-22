#ifndef config_h
#define config_h

#define WLAN_SSID  "${{ secrets.WLAN_SSID }}"
#define WLAN_PASSWD "${{ secrets.WLAN_PASSWD }}"

const int SLEEPTIME_IN_MINUTES = 15;
const int TANK_SENSOR_HEIGHT_IN_CM = 180;
const int TANK_OVERFLOW_HEIGHT_IN_CM = 150;
const int READINGS_TO_TAKE = 10;
const int DELAY_BETWEEN_READINGS_IN_MS = 50;

//seq logging
#define SEQ_URL      "${{ secrets.SEQ_URL }}"
#define SEQ_API_KEY  "${{ secrets.SEQ_API_KEY }}"

//io.adafruit.com
#define IO_USERNAME  "${{ secrets.IO_USERNAME }}"
#define IO_KEY       "${{ secrets.IO_KEY }}"
#define IO_FEEDNAME  "${{ secrets.IO_FEEDNAME }}"

//auth for the ota endpoint
#define OTA_ENDPOINT "${{ secrets.OTA_ENDPOINT }}"
#define OTA_USERNAME "${{ secrets.OTA_USERNAME }}"
#define OTA_PASSWORD "${{ secrets.OTA_PASSWORD }}"

#define VERSION_NUMBER "${{ github.VERSION_NUMBER }}"

#endif

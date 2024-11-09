This project fixes the tachometer reading on Snoopy, a 32 foot sailboat.

# The Problem

A diesel does not have an ignition system, so the input to the tachometer is a sine-wave from the alternator.
The alternator is belt driven, so pulley sizing and alternator design affect the reading on the tachometer.
To calibrate the RPM reading, the tachometer has a 5-position selector on the back that is adjustable with a screwdriver.

In the case of Snoopy, none of those 5 positions is anywhere near accurate.
The slowest selector position still produces an RPM approximately 2x too fast.

# The Solution

This code runs on a $6 microcontroller development board called ESP32-DOIT-DEV-KIT-V1.
The board is powered by the ignition system of the boat and modifies the ~14v sine-wave from the alternator into a square-wave of a lower frequency.
The frequency ratio can be calibrated by setting a `calibrationFactor`.  Examples:

| calibrationFactor | Change | Example Input | Example Output |
| ----------------- | ------ | ------------- | -------------- |
| 1000              | none   | 500 hz        | 500 hz         |
| 500               | /2     | 500 hz        | 250 hz         |
| 1000              | x2     | 500 hz        | 1000 hz        |

To adjust the `calibrationFactor`:

- With the boat's ignition key in the off position, press and hold the button on the module and turn the key to run
- Within a second or two the esp32 will start a WIFI access point with ssid: `CAT32_TACH`
- Connect with phone or laptop to the using WPA2/WPA3 authentication and the password `m50m50m50`
- Open a browser to `http://192.168.4.1/`
- The current input frequency, output frequency and calibration factor will be displayed
- To update the calibration factor change the URL to `http://192.168.4.1/calibrate/892`

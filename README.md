This project fixes the tachometer reading on a 32 foot sailboat.

# The Problem

A diesel engine does not have an ignition system, so the input to the tachometer is a sine-wave from the alternator.
The alternator is belt driven, so pulley sizing and alternator design (number of poles) affect the reading on the tachometer.
To calibrate the RPM reading, the tachometer has a 5-position selector on the back that is adjustable with a screwdriver.

In the case of this boat, none of those 5 positions are anywhere near accurate.
Actual RPM was measured using a handheld device pointed at reflective tape on the crankshaft.
The slowest selector position still produces a tachometer reading approximately 2x too high.


# The Solution

This code was developed for a ESP32-DOIT-DEV-KIT-V1.
The board is powered by the ignition system of the boat and modifies the ~14v sine-wave from the alternator into a square-wave of a lower frequency.
The frequency ratio can be calibrated by setting a `calibrationFactor`.  Examples:

| calibrationFactor | Change | Example Input | Example Output |
| ----------------- | ------ | ------------- | -------------- |
| 1000              | none   | 500 hz        | 500 hz         |
| 500               | /2     | 500 hz        | 250 hz         |
| 2000              | x2     | 500 hz        | 1000 hz        |

To adjust the `calibrationFactor`:

- With the boat's ignition key in the off position, press and hold the button on the module and turn the key to run
- Within a second or two the esp32 will start a WIFI access point with ssid: `CAT32_TACH`
- Connect with phone or laptop to the using WPA2/WPA3 authentication and the password `m50m50m50`
- Open a browser to `http://192.168.4.1/`
- The current input frequency, output frequency and calibration factor will be displayed
- To update the calibration factor change the URL to `http://192.168.4.1/calibrate/892`

# Schematic:

![schematic](https://github.com/eric-haney/tach-fixer/blob/main/schematic.png?raw=true)

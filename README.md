# tach-fixer

The purpose of this code is to fix the tachometer reading on Snoopy, a Catalina 32 sailboat with a diesel engine.

# The Problem

A diesel does not have an ignition system, so the input to the tachometer is a sine-wave from the alternator.
The alternator is belt driven, so pulley sizing and alternator design affect the reading on the tachometer.
To calibrate the RPM reading, the tachometer has a 5-position selector on the back that is adjustable with a screwdriver.

In the case of Snoopy, none of those 5 positions is anywhere near accurate.
The slowest selector position still produces an RPM approximately 2x too fast.

# The Solution

This code runs on a $6 microcontroller development board called ESP32-DOIT-DEV-KIT-V1.
The board is powered by the ignition system of the boat and modifies the ~14v sine-wave from the alternator into a square-wave of a lower frequency.
The frequency ratio can be calibrated by setting a `calibrationFactor`.  The following chart shows how the calibrationFactor adjusts the output frequency:

| calibrationFactor | Change | Example Input | Example Output |
| ----------------- | ------ | ------------- | -------------- |
| 1000              | none   | 500 hz        | 500 hz         |
| 500               | /2     | 500 hz        | 250 hz         |
| 1000              | x2     | 500 hz        | 1000 hz        |

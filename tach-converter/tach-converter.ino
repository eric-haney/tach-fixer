#include <WiFi.h>

#include <Preferences.h>

Preferences preferences;
const char* PREFERENCES_NAMESPACE       = "tach";
const char* PREFERENCES_CALIBRATION_KEY = "cal";

const int STARTUP_PIN    = 15;  // Controls whether the wifi access point and webserver are started.
const int SIGNAL_IN_PIN  = 36;  // Signal in, from alternator.  Must be 32-39.  The others don't work when wifi is on.
const int SIGNAL_OUT_PIN = 2;   // Signal out, to tachometer.

const int LEDC_CHANNEL = 2;

int inputFrequency    = 0;
int calibrationFactor = 1000;
int outputFrequency   = 0;
bool enableWebServer  = false;

const char* ssid     = "CAT32_TACH";
const char* password = "m50m50m50";

WiFiServer server(80);

String request;

void setup() {
  Serial.begin(115200);

  // set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);
  
  pinMode(STARTUP_PIN, INPUT_PULLUP);
  pinMode(SIGNAL_IN_PIN, INPUT);
  pinMode(SIGNAL_OUT_PIN, OUTPUT);

  ledcAttachPin(SIGNAL_OUT_PIN, LEDC_CHANNEL);

  enableWebServer = digitalRead(STARTUP_PIN) == LOW;

  if (enableWebServer) {
    setupWifi();
  }

  preferences.begin(PREFERENCES_NAMESPACE, true);
  calibrationFactor = preferences.getInt(PREFERENCES_CALIBRATION_KEY, 600);
  preferences.end();
}

void loop(){
  if (enableWebServer) {
    handleWebRequest();
  }

  int newInputFrequency = readInputFrequency();

  if (inputFrequency != newInputFrequency) {
    inputFrequency = newInputFrequency;
    calculateOutputFrequency();    
  }
}

bool setCalibrationFactor(int newCalibrationFactor) {
  Serial.print("Attempting to set calibrationFactor to ");
  Serial.println(newCalibrationFactor);

  if (newCalibrationFactor > 0 && newCalibrationFactor < 100000) {
    calibrationFactor = newCalibrationFactor;
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putInt(PREFERENCES_CALIBRATION_KEY, calibrationFactor);
    preferences.end();
    return true;
  }
  return false;
}

void setupWifi() {
  Serial.print("Calling softAP...");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  
  server.begin();
}

void handleWebRequest() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New HTTP Request.");    // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    unsigned long startTime = millis();     // Timer for calculating time out 
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        request += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            String httpStatus = "200 OK";
            if (request.indexOf("GET /calibrate/") == 0) {
              int indexOfNextSpace = request.substring(15).indexOf(" ");
              if (indexOfNextSpace < 1 && indexOfNextSpace > 10) {
                Serial.println("Error finding space after /calibrate/ in path.");
                httpStatus = "400 Bad Request";
              } else {
                Serial.print("Request calibration next space: ");
                Serial.println(indexOfNextSpace);
                Serial.print("Request calibration value: ");
                Serial.println(request.substring(15, 15 + indexOfNextSpace));
                Serial.print("Request calibration value int: ");
                Serial.println((request.substring(15, 15 + indexOfNextSpace)).toInt());
                bool success = setCalibrationFactor((request.substring(15, 15 + indexOfNextSpace)).toInt());
                if (!success) {
                  httpStatus = "400 Bad Request";
                }
              }
            } else if (!request.startsWith("GET / ")) {
              Serial.println("Don't know what to do with '" + request + "'");
              httpStatus = "404 Not Found";
            }
            client.println("HTTP/1.1 " + String(httpStatus));
            client.println("Content-type:text/plain");
            client.println("Connection: close");
            client.println();
            if (httpStatus.startsWith("200")) {
              calculateOutputFrequency();
              client.print("Input Frequency: ");
              client.println(inputFrequency);
              client.print("Calibration Factor: ");
              client.println(calibrationFactor);
              client.print("Output Frequency: ");
              client.println(outputFrequency);
              client.print("Calculation: ");
              client.print(String(inputFrequency));
              client.print(" ( ");
              client.print(String(calibrationFactor));
              client.print(" / 1000 ) = ");
              client.println(String(outputFrequency));            
            }
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if we got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if we got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the request variable
    request = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

const int HALF_PULSE_MAX_COUNT = 100; // The maximum number of half-pulses to count before returning
const int MAX_POLL_MILLIS = 500;      // The maximum amount of time to count half-pulses in milliseconds
const int RISING_THRESHOLD = 2000;    // millivolts
const int FALLING_THRESHOLD = 1000;   // millivolts
const int AVG_THRESHOLD = (RISING_THRESHOLD + FALLING_THRESHOLD) / 2;

int readInputFrequency() {
  unsigned long startTime = millis();
  uint8_t halfPulseCount = 0;
  unsigned long firstPulseTime = 0;
  uint8_t oldState = (getInputVoltage() < AVG_THRESHOLD) ? LOW : HIGH;
  int inputVoltage = 0;

  while (
    halfPulseCount < HALF_PULSE_MAX_COUNT &&    // we've counted enough half-pulses
    millis() < (startTime + MAX_POLL_MILLIS)    // time's up
  ) {

    inputVoltage = getInputVoltage();  // read millivolts from signal input pin

    if (oldState == LOW && inputVoltage > RISING_THRESHOLD) {
      oldState = HIGH;
      halfPulseCount++;
      if (firstPulseTime == 0) {
      
        firstPulseTime = millis();
      }
    }

    if (oldState == HIGH && inputVoltage < FALLING_THRESHOLD) {
      oldState = LOW;
      halfPulseCount++;
      if (firstPulseTime == 0) {
        firstPulseTime = millis();
      }
    }
  }

//  Serial.print("  Frequency Calc: ((");
//  Serial.print(halfPulseCount);
//  Serial.print(" * 1000) / (");
//  Serial.print(millis());
//  Serial.print(" - ");
//  Serial.print(firstPulseTime);
//  Serial.println(")) / 2");

  return (int) ((halfPulseCount * 1000) / (millis() - firstPulseTime)) / 2;
}

int getInputVoltage() {
  return map(analogRead(SIGNAL_IN_PIN), 0, 4095, 0, 3300);
}

void calculateOutputFrequency() {  
  outputFrequency = (int)(inputFrequency * ((float)calibrationFactor / 1000));

  Serial.print("outputFrequency calculation: ");
  Serial.print(inputFrequency);
  Serial.print(" * (");
  Serial.print(calibrationFactor);
  Serial.print(" / 1000) = ");
  Serial.println(outputFrequency);

  if (outputFrequency == 0) {
    ledcWrite(LEDC_CHANNEL, 0);
  } else {
    ledcSetup(LEDC_CHANNEL, outputFrequency, 8);
    ledcWrite(LEDC_CHANNEL, 127);
  }
}

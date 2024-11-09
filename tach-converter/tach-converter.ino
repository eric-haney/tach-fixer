#include <WiFi.h>

//  The purpose of this code is to fix the tachometer reading on the diesel engine of a Catalina 32 sailboat called Snoopy.
//  A diesel does not have an ignition system, so the input to the tachometer is a sine-wave from the alternator.
//  The alternator is belt driven, so pulley sizing and alternator design affect the reading on the tachometer.
//  To account for this, the tachometer has a 5-position selector on the back to calibrate the RPM reading.
//  In the case of Snoopy, none of those 5 positions is anywhere near accurate.
//  The slowest selector position still produces an RPM approximately 2x too fast.

const int STARTUP_PIN = 15;   // controls whether the wifi access point and webserver are started
const int SIG_IN_PIN = 23;    // signal in, from alternator
const int SIG_OUT_PIN = 2;    // signal out, to tachometer

const int LEDC_CHANNEL = 1;

int inputFrequency = 0;
int calibrationFactor = 1000;
int outputFrequency = 0;
bool enableWebServer = false;

const char* ssid     = "CAT32_TACH";
const char* password = "m50m50m50";

WiFiServer server(80);

String request;

void setup() {
  Serial.begin(115200);

  // TODO: read calibrationFactor from preferences

  pinMode(STARTUP_PIN, INPUT_PULLUP);
  enableWebServer = digitalRead(STARTUP_PIN) == HIGH;

  pinMode(SIG_IN_PIN, INPUT);
  pinMode(SIG_OUT_PIN, OUTPUT);

  if (enableWebServer) {
    setupWifi();
  }

}

void loop(){
  if (enableWebServer) {
    handleWebRequest();
  }

  int newInputFrequency = readInputFrequency();

  Serial.println("New Frequency: " + String(newInputFrequency));

  if (inputFrequency != newInputFrequency) {
    inputFrequency = newInputFrequency;
    calculateOutputFrequency();    
  }
}

bool setCalibrationFactor(int newCalibrationFactor) {
  Serial.println("Attempting to set calibrationFactor to " + newCalibrationFactor);
  if (newCalibrationFactor > 0 && newCalibrationFactor < 100000) {
    calibrationFactor = newCalibrationFactor;
    // TODO: write calibrationFactor to preferences
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
      if (startTime + 2000 > millis()) {
        break;                              // bail after 2 seconds.
      }
      if (currentLine.length() > 1000) {
        break;                              // bail if the client's request is > 1KB
      }
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        request += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            bool error = false;
            if (request.indexOf("GET /calibrate") == 0) {
              error = !setCalibrationFactor(request.substring(21).toInt());
            } else if (request != "GET /") {
              error = true;
            }
            if (error) {
              client.println("HTTP/1.1 400 Bad Request");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();
            } else {
              calculateOutputFrequency();
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();
            }
            client.println("Input Frequency: " + String(inputFrequency));
            client.println("Delay Factor: " + calibrationFactor);
            client.println("Output Frequency: " + outputFrequency);
            client.println("Calculation: " + String(inputFrequency) + " ( " + String(calibrationFactor) + " / 1000 ) = " + String(outputFrequency));            
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

int readInputFrequency() {
  unsigned long startTime = millis();
  uint8_t halfPulseCount = 0;
  unsigned long firstPulseTime = 0;
  uint8_t oldState = getInputVoltage() < ((RISING_THRESHOLD / FALLING_THRESHOLD) / 2) ? LOW : HIGH;
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

  Serial.println("  Frequency Calc: (" + String(halfPulseCount) + " / (" + String(millis()) + " - " + String(firstPulseTime) + " )) / 2");

  return (int) (halfPulseCount / (millis() - firstPulseTime)) / 2;
}

int getInputVoltage() {
  return map(analogRead(SIG_IN_PIN), 0, 4095, 0, 3300);
}

void calculateOutputFrequency() {
  outputFrequency = inputFrequency * (calibrationFactor / 1000);
  if (outputFrequency == 0) {
    ledcWrite(LEDC_CHANNEL, 0);
  } else {
    ledcSetup(LEDC_CHANNEL, outputFrequency, 8);
    ledcWrite(LEDC_CHANNEL, 127);
  }
}

const int SIGNAL_IN_PIN = 36;     // signal in, from alternator

void setup() {
  Serial.begin(115200);

  // set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);
  
  pinMode(SIGNAL_IN_PIN, INPUT);
}

void loop() {
//  Serial.print("digital state: ");
//  Serial.println(digitalRead(SIGNAL_IN_PIN));
//  Serial.print("analog voltage: ");
//  Serial.println(getInputVoltage());
  Serial.print(readInputFrequency());
  Serial.println(" hz");
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

//  Serial.println("  Frequency Calc: ((" + String(halfPulseCount) + " * 1000) / (" + String(millis()) + " - " + String(firstPulseTime) + ")) / 2");

  return (int) ((halfPulseCount * 1000) / (millis() - firstPulseTime)) / 2;
}

int getInputVoltage() {
  return map(analogRead(SIGNAL_IN_PIN), 0, 4095, 0, 3300);
}

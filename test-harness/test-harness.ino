
const int OUT_PIN = 2;
const int LEDC_CHANNEL = 1;
const int POT_PIN = 15;
const int MIN_FREQ = 5;
const int MAX_FREQ = 1000;

int frequency = 0;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  // set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);
  ledcAttachPin(OUT_PIN, LEDC_CHANNEL);
}

void loop() {
  int newFrequency = getFrequency();
  if (newFrequency < (frequency - 1) || newFrequency > (frequency + 1)) {
    Serial.print("Frequency: ");
    Serial.print(newFrequency);
    Serial.print(" ");
    Serial.println(frequency);
    if (newFrequency == MIN_FREQ) {
      ledcWrite(LEDC_CHANNEL, 0);
    } else {
      ledcSetup(LEDC_CHANNEL, newFrequency, 8);
      ledcWrite(LEDC_CHANNEL, 127);
    }
    frequency = newFrequency;
  }
  delay(100);
}

int getFrequency() {
  return map(analogRead(POT_PIN), 0, 4095, MIN_FREQ, MAX_FREQ);
}

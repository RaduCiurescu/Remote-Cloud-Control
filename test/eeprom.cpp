#include <Arduino.h>
#include <EEPROM.h>

const int lm35Pin = 15;            // ADC-capable pin
const float adcResolution = 4095.0;
const float referenceVoltage = 3.3;
const float mvPerDegree = 10.0;

#define EEPROM_SIZE 64
#define NUM_VALUES 10

void addValueToEEPROM(uint8_t newValue) {
  // Shift values from [8]→[9], ..., [0]→[1]
  for (int i = NUM_VALUES - 2; i >= 0; i--) {
    uint8_t v = EEPROM.read(i);
    EEPROM.write(i + 1, v);
  } 
  EEPROM.write(0, newValue);
  EEPROM.commit();  // VERY IMPORTANT on ESP32
}

void readEEPROMValues() {
  Serial.println("EEPROM values:");
  for (int i = 0; i < NUM_VALUES; i++) {
    uint8_t val = EEPROM.read(i);
    Serial.print("Index ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(val);
  }
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(lm35Pin, INPUT);
  delay(1000); // Stabilizare senzor
}

void loop() {
  int adcValue = analogRead(lm35Pin);
  float voltage = (adcValue / adcResolution) * referenceVoltage;
  float temperatureC = voltage * 100.0;

  uint8_t tempRounded = (uint8_t)round(temperatureC);
  Serial.print("Temperatură actuală: ");
  Serial.println(tempRounded);

  addValueToEEPROM(tempRounded);
  readEEPROMValues();

  delay(1000); // Pauză între citiri
}

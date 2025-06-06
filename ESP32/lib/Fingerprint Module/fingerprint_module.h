// fingerprint_module.h
#ifndef FINGERPRINT_MODULE_H
#define FINGERPRINT_MODULE_H

#include <TaskBase.h>
#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

class FingerprintModule : public TaskBase {
  public:
    FingerprintModule(HardwareSerial* serial, uint8_t relayPin, uint8_t buzzerPin);

    void begin();
    void setEnrollMode(bool mode);
    bool isEnrollMode() const;
    void deleteFingerprint(uint8_t id);
    void deleteAll();
  protected:
    void run() override;

  private:
    Adafruit_Fingerprint finger;
    HardwareSerial* hwSerial;
    uint8_t relayPin, buzzerPin;
    bool enrollMode = false;
    unsigned long timeWait;
    int id = 1;
    unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;

    const uint8_t buttonPin = 0; // BOOT button
    bool lastButtonState = HIGH;
    bool currentState = HIGH;
    unsigned long buttonPressTime = 0;
    bool longPressHandled = false;

    
    void controlRelay();
    void beep(int n, int d);
    void getFingerprintIDs();
    uint8_t getFingerprintID();
    uint8_t getFingerprintEnroll();
    void handleButton();
};

#endif
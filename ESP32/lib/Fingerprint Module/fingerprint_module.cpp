// fingerprint_module.cpp
#include "fingerprint_module.h"

FingerprintModule::FingerprintModule(HardwareSerial* serial, uint8_t relayPin, uint8_t buzzerPin)
  : TaskBase("Fingerprint", 4096, 1, 1),
    finger(serial), 
    hwSerial(serial), 
    relayPin(relayPin), 
    buzzerPin(buzzerPin) {}

//Setup the fingerprint module
void FingerprintModule::begin() {
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  hwSerial->begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor is ready.");
    finger.getParameters();
    getFingerprintIDs();
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1)); }
  }
}

//Set ID,ON/OFF relay
void FingerprintModule::run() {
  while(1){
    handleButton();
    if (enrollMode) {
      getFingerprintEnroll();
    } else {
      getFingerprintID();
      if (digitalRead(relayPin) == HIGH) {
        if (millis() - timeWait > 5000) {
          digitalWrite(relayPin, LOW);
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void FingerprintModule::setEnrollMode(bool mode) {
  enrollMode = mode;
}

bool FingerprintModule::isEnrollMode() const {
  return enrollMode;
}

//Xóa vân tay theo ID
void FingerprintModule::deleteFingerprint(uint8_t id) {
  if (id == 0) {
    Serial.println("ID must be greater than 0.");
    return;
  }
  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    Serial.print("Deleted ID #"); Serial.println(id);
    beep(2, 200);
  } else {
    Serial.print("Failed to delete ID #"); Serial.println(id);
    beep(1, 200);
  }
}

void FingerprintModule::deleteAll() {
  finger.emptyDatabase();
  id = 1;
  Serial.println("All fingerprints deleted.");
  beep(3, 150);
}

void FingerprintModule::handleButton() {
  bool temp = digitalRead(buttonPin);
  if (temp != lastButtonState) {// Nếu trạng thái nút thay đổi
    lastDebounceTime = millis();
  }
  // Nếu đã qua đủ thời gian debounce
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Nếu trạng thái thay đổi so với lần xử lý trước đó
    if (temp != currentState) {
      currentState = temp;

      if (currentState == LOW) {
        // Nút vừa được nhấn
        buttonPressTime = millis();
        longPressHandled = false;
      } else {
        // Nút vừa được nhả ra
        if (!longPressHandled) {
          enrollMode = !enrollMode;
          Serial.print("Enroll Mode: ");
          Serial.println(enrollMode ? "ON" : "OFF");
          beep(1, 100);
        }
      }
    }
    // Nếu đang giữ nút, kiểm tra long press
    if (currentState == LOW && !longPressHandled && (millis() - buttonPressTime > 5000)) {
      Serial.println("Deleting all fingerprints");
      deleteAll();
      longPressHandled = true;
    }
  }
  lastButtonState = temp;
}

//Buzzer beep
void FingerprintModule::beep(int n, int d) {
  for (int i = 0; i < n; i++) {
    digitalWrite(buzzerPin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(d));
    digitalWrite(buzzerPin, LOW);
    vTaskDelay(pdMS_TO_TICKS(d));
  }
}

//Relay control
void FingerprintModule::controlRelay() {
  digitalWrite(relayPin, HIGH);
  timeWait = millis();
}

//In các ID đã lưu trong bộ nhớ
//Lưu ý: ID bắt đầu từ 1, không phải 0
void FingerprintModule::getFingerprintIDs() {
  int lastID = 0;
  Serial.println("temp stored fingerprint IDs...");
  for (uint16_t i = 1; i <= finger.capacity; i++) {
    if (finger.loadModel(i) == FINGERPRINT_OK) {
      Serial.print("ID "); Serial.println(i);
      lastID = i;
    }
  }
  id = lastID + 1;
}

//Quét vân tay
//Nhận diện vân tay và điều khiển relay
uint8_t FingerprintModule::getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Found ID #"); Serial.print(finger.fingerID);
    Serial.println(" Open the door!");
    controlRelay();
    beep(1, 200);
    return finger.fingerID;
  } else {
    Serial.println("No match found. Fingerprint not recognized!");
  }

  beep(2, 500);
  return p;
}

//Thiết lập lưu vân tay
//Lưu vân tay vào ID 1,2,3... và tăng ID lên 1 sau mỗi lần lưu
uint8_t FingerprintModule::getFingerprintEnroll() {
  if (id > finger.capacity) {
    Serial.println("Memory full!");
    beep(3, 300);
    return FINGERPRINT_PACKETRECIEVEERR;
  }
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    handleButton();
    if (!enrollMode) return true;
    p = finger.getImage();
  }
  beep(1, 200);

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;

  Serial.println("Remove finger");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  while (finger.getImage() != FINGERPRINT_NOFINGER){
    handleButton();
    if (!enrollMode) return true;
  }

  Serial.println("Place same finger again");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    handleButton();
    if (!enrollMode) return true;
  }
  beep(1, 200);

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Fingerprint already exists with ID #");
    Serial.println(finger.fingerID);
    beep(3, 300);
    return p;
  }
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    id++;
    beep(2, 200);
  }
  return p;
}

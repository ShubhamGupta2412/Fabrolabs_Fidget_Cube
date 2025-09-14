#include <Wire.h>

// IQS5xx Configuration
#define IQS5xx_ADDR     0x74
#define RDY_PIN         2
#define END_WINDOW      0xEEEE

// Register addresses  
#define ProductNumber_adr     0x0000
#define GestureEvents0_adr    0x000D
#define GestureEvents1_adr    0x000E
#define SystemInfo0_adr       0x000F
#define SystemControl0_adr    0x0431

// System flags
#define SHOW_RESET           0x80
#define ACK_RESET            0x80

// Single Finger Gestures (GestureEvents0) - from your reference
#define SINGLE_TAP           0x01
#define TAP_AND_HOLD         0x02
#define SWIPE_X_NEG          0x04
#define SWIPE_X_POS          0x08
#define SWIPE_Y_POS          0x10
#define SWIPE_Y_NEG          0x20

// Multi Finger Gestures (GestureEvents1) - from your reference
#define TWO_FINGER_TAP       0x01
#define SCROLL               0x02
#define ZOOM                 0x04

// Global variables
uint8_t Data_Buff[44];
bool deviceReady = false;
bool useRDY = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("=== ProxSense TPS43 Complete Gestures ===");
  
  // I2C initialize
  Wire.begin();
  Wire.setClock(100000);
  
  // RDY pin setup
  pinMode(RDY_PIN, INPUT_PULLUP);
  
  delay(1000);
  
  // RDY check
  checkRDYPin();
  
  // Device detection
  if (detectDevice()) {
    Serial.println("✓ Device ready - All gestures enabled!");
    deviceReady = true;
  } else {
    Serial.println("✗ Device nahi mila");
  }
}

void loop() {
  if (!deviceReady) {
    delay(1000);
    return;
  }
  
  // Data read
  if (readTouchData()) {
    processTouchData();
  }
  
  delay(useRDY ? 20 : 100);
}

void checkRDYPin() {
  Serial.print("RDY pin check... ");
  
  bool rdyWorking = false;
  for (int i = 0; i < 20; i++) {
    if (digitalRead(RDY_PIN) == HIGH) {
      rdyWorking = true;
      break;
    }
    delay(50);
  }
  
  if (rdyWorking) {
    useRDY = true;
    Serial.println("Working! 👍");
  } else {
    useRDY = false;
    Serial.println("Polling mode 🔄");
  }
}

bool detectDevice() {
  Serial.println("Device detect kar rahe hain...");
  
  uint8_t deviceInfo[6];
  bool detected = false;
  
  for (int attempt = 1; attempt <= 5; attempt++) {
    Serial.print("Attempt ");
    Serial.print(attempt);
    Serial.print("... ");
    
    if (I2C_Read_Direct(ProductNumber_adr, deviceInfo, 6)) {
      detected = true;
      Serial.println("Success! 🎉");
      break;
    } else {
      Serial.println("Fail ❌");
      delay(200);
    }
  }
  
  if (!detected) return false;
  
  // Device info
  uint16_t product = (deviceInfo[0] << 8) + deviceInfo[1];
  Serial.print("Product: ");
  Serial.print(product, HEX);
  Serial.print(" Version: ");
  Serial.print(deviceInfo[4]);
  Serial.print(".");
  Serial.println(deviceInfo[5]);
  
  handleDeviceReset();
  return true;
}

bool I2C_Read_Direct(uint16_t regAddr, uint8_t *data, uint8_t len) {
  Wire.beginTransmission(IQS5xx_ADDR);
  Wire.write(regAddr >> 8);
  Wire.write(regAddr & 0xFF);
  
  if (Wire.endTransmission() != 0) return false;
  
  Wire.requestFrom(IQS5xx_ADDR, len);
  
  uint8_t received = 0;
  unsigned long start = millis();
  
  while (Wire.available() && received < len) {
    if (millis() - start > 100) break;
    data[received++] = Wire.read();
  }
  
  return (received == len);
}

bool readTouchData() {
  if (useRDY) {
    if (!waitForRDY(50)) {
      useRDY = false;
      Serial.println("RDY timeout - polling mode");
    }
  }
  
  bool success = I2C_Read_Direct(GestureEvents0_adr, Data_Buff, 44);
  
  if (useRDY && success) {
    closeComms();
  }
  
  return success;
}

bool waitForRDY(uint16_t timeoutMs) {
  unsigned long start = millis();
  while (digitalRead(RDY_PIN) == LOW) {
    if (millis() - start > timeoutMs) return false;
    delayMicroseconds(100);
  }
  return true;
}

void closeComms() {
  uint8_t dummy = 0;
  Wire.beginTransmission(IQS5xx_ADDR);
  Wire.write(END_WINDOW >> 8);
  Wire.write(END_WINDOW & 0xFF);
  Wire.write(dummy);
  Wire.endTransmission();
}

void handleDeviceReset() {
  uint8_t sysInfo;
  if (I2C_Read_Direct(SystemInfo0_adr, &sysInfo, 1)) {
    if (sysInfo & SHOW_RESET) {
      Serial.println("Reset detected - acknowledging");
      uint8_t ackReset = ACK_RESET;
      
      Wire.beginTransmission(IQS5xx_ADDR);
      Wire.write(SystemControl0_adr >> 8);
      Wire.write(SystemControl0_adr & 0xFF);
      Wire.write(ackReset);
      Wire.endTransmission();
      
      delay(100);
    }
  }
}

void processTouchData() {
  static unsigned long lastPrint = 0;
  
  // Output throttle
  if (millis() - lastPrint < 100) return;
  
  uint8_t gestureEvents0 = Data_Buff[0];  // Single finger gestures
  uint8_t gestureEvents1 = Data_Buff[1];  // Multi finger gestures
  uint8_t systemInfo0 = Data_Buff[2];
  uint8_t systemInfo1 = Data_Buff[3];
  uint8_t fingers = Data_Buff[4];
  
  // Reset check
  if (systemInfo0 & SHOW_RESET) {
    Serial.println("🔄 RESET DETECTED");
    handleDeviceReset();
    return;
  }
  
  // Single Finger Gestures (Data_Buff[0])
  if (gestureEvents0 != 0) {
    Serial.print("👆 Single Finger: ");
    
    if (gestureEvents0 & SINGLE_TAP) {
      Serial.print("TAP 👆 ");
    }
    if (gestureEvents0 & TAP_AND_HOLD) {
      Serial.print("HOLD ✋ ");
    }
    if (gestureEvents0 & SWIPE_X_NEG) {
      Serial.print("SWIPE_LEFT ⬅️ ");
    }
    if (gestureEvents0 & SWIPE_X_POS) {
      Serial.print("SWIPE_RIGHT ➡️ ");
    }
    if (gestureEvents0 & SWIPE_Y_NEG) {
      Serial.print("SWIPE_UP ⬆️ ");
    }
    if (gestureEvents0 & SWIPE_Y_POS) {
      Serial.print("SWIPE_DOWN ⬇️ ");
    }
    
    Serial.println();
    lastPrint = millis();
  }
  
  // Multi Finger Gestures (Data_Buff[1]) 
  if (gestureEvents1 != 0) {
    Serial.print("✌️ Multi Finger: ");
    
    if (gestureEvents1 & TWO_FINGER_TAP) {
      Serial.print("TWO_FINGER_TAP 👆👆 ");
    }
    if (gestureEvents1 & SCROLL) {
      Serial.print("SCROLL 📜 ");
    }
    if (gestureEvents1 & ZOOM) {
      Serial.print("ZOOM 🔍 ");
    }
    
    Serial.println();
    lastPrint = millis();
  }
  
  // Touch coordinates
  if (fingers > 0) {
    // Relative movement (Data_Buff[5-8])
    int16_t relX = (Data_Buff[5] << 8) | Data_Buff[6];
    int16_t relY = (Data_Buff[7] << 8) | Data_Buff[8];
    
    // Absolute coordinates for finger 1 (Data_Buff[9-15])
    uint16_t absX1 = (Data_Buff[9] << 8) | Data_Buff[10];
    uint16_t absY1 = (Data_Buff[11] << 8) | Data_Buff[12];
    uint16_t strength1 = (Data_Buff[13] << 8) | Data_Buff[14];
    uint8_t area1 = Data_Buff[15];
    
    if (absX1 > 0 && absY1 > 0) {
      // Convert to percentage
      float xPercent = (float)absX1 / 32768.0 * 100.0;
      float yPercent = (float)absY1 / 32768.0 * 100.0;
      
      Serial.print("🖐️ Touch: (");
      Serial.print(xPercent, 1);
      Serial.print("%, ");
      Serial.print(yPercent, 1);
      Serial.print("%) | Strength: ");
      Serial.print(strength1);
      Serial.print(" | Area: ");
      Serial.print(area1);
      Serial.print(" | Rev Movement: (");
      Serial.print(relX);
      Serial.print(", ");
      Serial.print(relY);
      Serial.print(") | Fingers: ");
      Serial.println(fingers);
      
      lastPrint = millis();
    }
    
    // Multiple fingers ke liye coordinates (agar chahiye to)
    if (fingers > 1) {
      for (int i = 2; i <= fingers && i <= 5; i++) {
        uint8_t offset = 9 + (i-1) * 7;  // Each finger data is 7 bytes apart
        uint16_t absX = (Data_Buff[offset] << 8) | Data_Buff[offset + 1];
        uint16_t absY = (Data_Buff[offset + 2] << 8) | Data_Buff[offset + 3];
        uint16_t strength = (Data_Buff[offset + 4] << 8) | Data_Buff[offset + 5];
        uint8_t area = Data_Buff[offset + 6];
        
        if (absX > 0 && absY > 0) {
          float xPercent = (float)absX / 32768.0 * 100.0;
          float yPercent = (float)absY / 32768.0 * 100.0;
          
          Serial.print("👆 Finger ");
          Serial.print(i);
          Serial.print(": (");
          Serial.print(xPercent, 1);
          Serial.print("%, ");
          Serial.print(yPercent, 1);
          Serial.print("%) Strength: ");
          Serial.print(strength);
          Serial.print(" Area: ");
          Serial.println(area);
        }
      }
      lastPrint = millis();
    }
  }
}

// Utility functions - your reference implementation style
bool isTouchActive() {
  return Data_Buff[4] > 0;
}

uint8_t getFingerCount() {
  return Data_Buff[4];
}

void getAbsoluteCoordinates(uint8_t finger, uint16_t &x, uint16_t &y) {
  if (finger == 0 || finger > 5) return;
  
  uint8_t offset = 9 + (finger - 1) * 7;
  x = (Data_Buff[offset] << 8) | Data_Buff[offset + 1];
  y = (Data_Buff[offset + 2] << 8) | Data_Buff[offset + 3];
}

void getRelativeMovement(int16_t &x, int16_t &y) {
  x = (Data_Buff[5] << 8) | Data_Buff[6];
  y = (Data_Buff[7] << 8) | Data_Buff[8];
}

// Debug function
void printGestureStatus() {
  Serial.println("\n=== Gesture Status ===");
  Serial.print("Single Finger Gestures: 0x");
  Serial.println(Data_Buff[0], HEX);
  Serial.print("Multi Finger Gestures: 0x");
  Serial.println(Data_Buff[1], HEX);
  Serial.print("Active Fingers: ");
  Serial.println(Data_Buff[4]);
}

#include <Wire.h>  
void setup() {  
Serial.begin(115200);  
Wire.begin(21, 22); // SDA, SCL — change for your board  
}  
void loop() {  
for (byte a = 1; a < 127; a++) {  
Wire.beginTransmission(a);  
if (Wire.endTransmission() == 0) { Serial.print("Found 0x"); Serial.println(a, HEX); }  
}  
Serial.println("--"); delay(2000);  
}

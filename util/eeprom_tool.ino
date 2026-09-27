// Copyright (C) 2026 C. Mong (cheukton.mong@student.unimelb.edu.au)

#include <EEPROM.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.setTimeout(3600000);
}

void loop() {
  // put your main code here, to run repeatedly:
  while (Serial.available()) {
    char x = Serial.read();
  } Serial.print("\n\n");
  int j = 0;
  unsigned int len = EEPROM.length();
  Serial.print("Length = ");
  Serial.println(len);
  Serial.println("job?");
  String job = Serial.readStringUntil('\n');
  if (strcmp(job.c_str(), "wipe") == 0) {
    Serial.println("====>  wipe  <====");
    for (unsigned int i = 0; i < len/sizeof(char); i++) {
      EEPROM.write(i*sizeof(char), (char)0x00);
      j++; if (j == 32) { j = 0; Serial.println(i+1); };
    } Serial.println("====>  end   <====");
  } else if (strcmp(job.c_str(), "wipex") == 0) {
    Serial.println("====>  wipex <====");
    for (unsigned int i = 0; i < len/sizeof(char); i++) {
      EEPROM.put(i*sizeof(char), (char)'x');
      j++; if (j == 32) { j = 0; Serial.println(i+1); };
    } Serial.println("====>  end   <====");
  } else {
    Serial.print("\n====>len="); 
    Serial.print(len); 
    Serial.println("<====");
    for (unsigned int i = 0; i < len/sizeof(char); i++) {
      char x = EEPROM.read(i*sizeof(char));
      Serial.print(x);
      j++; if (j == 64) { j = 0; Serial.print('\n'); }
    } Serial.println("====>  end   <====");
  } delay(3600000);
}

#pragma once

#include <Arduino.h>
#include <string.h>

const uint8_t SERIAL_DEBUG_INNER_WIDTH = 60;

inline void serialDebugBoxLine(const char* prefix, const char* title) {
  char content[SERIAL_DEBUG_INNER_WIDTH + 1];
  snprintf(content, sizeof(content), "%s%s", prefix, title);
  size_t length = strlen(content);

  Serial.print("| ");
  Serial.print(content);
  for (size_t i = length; i < SERIAL_DEBUG_INNER_WIDTH; i++) {
    Serial.print(' ');
  }
  Serial.println(" |");
}

inline void serialDebugBegin(const char* title) {
  Serial.println();
  Serial.println("+==============================================================+");
  serialDebugBoxLine("BEGIN DEBUG: ", title);
  Serial.println("+--------------------------------------------------------------+");
}

inline void serialDebugEnd(const char* title) {
  Serial.println("+--------------------------------------------------------------+");
  serialDebugBoxLine("END DEBUG:   ", title);
  Serial.println("+==============================================================+");
  Serial.println();
}

inline void serialDebugEvent(const char* source, const char* message) {
  Serial.print("[EVENT][");
  Serial.print(source);
  Serial.print("] ");
  Serial.println(message);
}

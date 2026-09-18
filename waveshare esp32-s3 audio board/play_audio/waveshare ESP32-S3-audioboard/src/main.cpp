#include "audio_system.h"

void setup()
{
  Serial.begin(115200);
  delay(500);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  audioSystemInit();
}

void loop()
{
  static unsigned long lastPrint = 0;
  uint32_t touchValue = audioSystemUpdate();

  if (millis() - lastPrint > 500)
  {
    lastPrint = millis();
    Serial.printf("touch=%lu  active=%d\n", (unsigned long)touchValue,
                  audioSystemIsActive());
  }
}
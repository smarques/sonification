#pragma once

#include <Arduino.h>
#include "AudioTools.h"
#include "AudioTools/AudioLibs/I2SCodecStream.h"

#define PIN_I2C_SDA 11
#define PIN_I2C_SCL 10
#define PIN_I2S_MCLK 12
#define PIN_I2S_BCK 13
#define PIN_I2S_WS 14
#define PIN_I2S_DOUT 16
#define PIN_TOUCH 8

#define TCA9555_ADDR 0x20
#define TCA9555_REG_OUTPUT_PORT0 0x02
#define TCA9555_REG_OUTPUT_PORT1 0x03
#define TCA9555_REG_CONFIG_PORT0 0x06
#define TCA9555_REG_CONFIG_PORT1 0x07

#define TOUCH_THRESHOLD 70000

class SawtoothGenerator : public SoundGenerator<int16_t>
{
public:
  SawtoothGenerator(float frequency = 220.0f, float amplitude = 0.3f);

  void begin(AudioInfo info, float frequency);
  int16_t readSample() override;

  bool active = false;

private:
  float frequency;
  float amplitude;
  float phase = 0.0f;
};

void audioSystemInit();
uint32_t audioSystemUpdate();
bool audioSystemIsActive();

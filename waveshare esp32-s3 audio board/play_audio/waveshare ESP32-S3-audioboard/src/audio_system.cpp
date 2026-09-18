#include "audio_system.h"

SawtoothGenerator::SawtoothGenerator(float frequency, float amplitude)
{
  this->frequency = frequency;
  this->amplitude = amplitude;
}

void SawtoothGenerator::begin(AudioInfo info, float frequency)
{
  SoundGenerator<int16_t>::begin(info);
  this->frequency = frequency;
  phase = 0.0f;
}

int16_t SawtoothGenerator::readSample()
{
  if (!active)
  {
    phase += frequency / info.sample_rate;
    if (phase >= 1.0f)
      phase -= 1.0f;
    return 0;
  }

  float value = 2.0f * phase - 1.0f;
  phase += frequency / info.sample_rate;
  if (phase >= 1.0f)
    phase -= 1.0f;
  return (int16_t)(value * amplitude * 32767.0f);
}

static AudioInfo info(44100, 1, 16);
static SawtoothGenerator sawtooth(220.0f, 0.3f);
static GeneratedSoundStream<int16_t> sound(sawtooth);
static I2SCodecStream out(GenericES8311);
static StreamCopy copier(out, sound);

void audioSystemInit()
{
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  Serial.println("Scanning I2C bus...");
  bool foundExpander = false;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("Found I2C device at 0x%02X\n", addr);
      if (addr == TCA9555_ADDR)
        foundExpander = true;
    }
  }

  if (foundExpander)
  {
    Wire.beginTransmission(TCA9555_ADDR);
    Wire.write(TCA9555_REG_CONFIG_PORT1);
    Wire.write((uint8_t)0xFE);
    Wire.endTransmission();

    Wire.beginTransmission(TCA9555_ADDR);
    Wire.write(TCA9555_REG_OUTPUT_PORT1);
    Wire.write((uint8_t)0x01);
    Wire.endTransmission();

    Serial.println("TCA9555 found: enabled speaker amp (EXIO08 high).");
  }
  else
  {
    Serial.println("TCA9555 (0x20) NOT found on I2C bus.");
  }

  auto config = out.defaultConfig(TX_MODE);
  config.copyFrom(info);
  config.pin_mck = PIN_I2S_MCLK;
  config.pin_bck = PIN_I2S_BCK;
  config.pin_ws = PIN_I2S_WS;
  config.pin_data = PIN_I2S_DOUT;

  bool codecOk = out.begin(config);
  Serial.printf("Codec/I2S init %s\n", codecOk ? "OK" : "FAILED - check wiring/pins");
  out.setVolume(0.8f);

  sawtooth.begin(info, 220.0f);

  Serial.println("Ready. Touch GPIO8 to play the sawtooth.");
  Serial.printf("Baseline touch reading (untouched): %lu\n",
                (unsigned long)touchRead(PIN_TOUCH));
}

uint32_t audioSystemUpdate()
{
  uint32_t touchValue = touchRead(PIN_TOUCH);
  sawtooth.active = (touchValue > TOUCH_THRESHOLD);
  copier.copy();
  return touchValue;
}

bool audioSystemIsActive()
{
  return sawtooth.active;
}

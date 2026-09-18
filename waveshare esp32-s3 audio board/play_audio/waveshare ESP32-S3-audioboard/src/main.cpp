#include "AudioTools.h"
#include "AudioTools/AudioLibs/I2SCodecStream.h"

// ---------------------------------------------------------------
// Pins from the Waveshare ESP32-S3-AUDIO-Board wiki pinout table
// ---------------------------------------------------------------
//   ES8311 (codec)      ESP32-S3
//   I2C_SDA          ->  GPIO11
//   I2C_SCL          ->  GPIO10
//   I2S_MCLK         ->  GPIO12
//   I2S_SCLK (BCK)   ->  GPIO13
//   I2S_LRCK (WS)    ->  GPIO14
//   I2S_DSDIN (DOUT) ->  GPIO16   (ESP32 -> codec, audio out)
//
// Note: the mic (ES7210) shares the same I2C and I2S clock/word-select
// lines but uses a different data line (GPIO15, input). We don't touch
// that here since we're only playing audio out.

#define PIN_I2C_SDA 11
#define PIN_I2C_SCL 10
#define PIN_I2S_MCLK 12
#define PIN_I2S_BCK 13
#define PIN_I2S_WS 14
#define PIN_I2S_DOUT 16

// ---------------------------------------------------------------
// Capacitive touch input, available on the populated pin header
// (labelled IO8 on the board silkscreen).
// ---------------------------------------------------------------
#define PIN_TOUCH 8

// ---------------------------------------------------------------
// TCA9555 I/O expander (I2C 0x20). Waveshare's docs put the
// speaker amp enable (PA_CTRL) on one of its EXIO pins, but the
// exact EXIO number isn't confirmed yet - so for this diagnostic
// build we drive ALL expander pins as outputs, HIGH, to cover
// every numbering possibility. Narrow it down once sound appears.
// ---------------------------------------------------------------
#define TCA9555_ADDR 0x20
#define TCA9555_REG_OUTPUT_PORT0 0x02
#define TCA9555_REG_OUTPUT_PORT1 0x03
#define TCA9555_REG_CONFIG_PORT0 0x06
#define TCA9555_REG_CONFIG_PORT1 0x07

// Touch threshold: on ESP32-S3, touchRead() value DROPS when touched.
// Start with this value and watch the Serial Monitor to calibrate:
// note the "untouched" baseline and the "touched" value it drops to,
// then set the threshold roughly halfway between the two.
#define TOUCH_THRESHOLD 70000

// ---------------------------------------------------------------
// Simple band-limited-ish sawtooth generator (naive, no anti-aliasing).
// Good enough for a first test tone; you'll hear some aliasing at
// higher frequencies, which is normal for a naive sawtooth.
// ---------------------------------------------------------------
class SawtoothGenerator : public SoundGenerator<int16_t>
{
public:
  SawtoothGenerator(float frequency = 220.0f, float amplitude = 0.3f)
  {
    this->frequency = frequency;
    this->amplitude = amplitude;
  }

  void begin(AudioInfo info, float frequency)
  {
    SoundGenerator<int16_t>::begin(info);
    this->frequency = frequency;
    phase = 0.0f;
  }

  int16_t readSample() override
  {
    if (!active)
    {
      // keep phase running so the wave doesn't "click"/jump on restart
      phase += frequency / info.sample_rate;
      if (phase >= 1.0f)
        phase -= 1.0f;
      return 0;
    }
    // phase runs 0..1, output ramps from -amplitude to +amplitude
    float value = 2.0f * phase - 1.0f;
    phase += frequency / info.sample_rate;
    if (phase >= 1.0f)
      phase -= 1.0f;
    return (int16_t)(value * amplitude * 32767.0f);
  }

  bool active = false; // gated by the touch pin in loop()

private:
  float frequency;
  float amplitude;
  float phase = 0.0f;
};

// ---------------------------------------------------------------
// Audio pipeline: generator -> stream -> I2S/codec output
// ---------------------------------------------------------------
AudioInfo info(44100, 1, 16);             // 44.1kHz, mono, 16-bit
SawtoothGenerator sawtooth(220.0f, 0.3f); // 220Hz (A3), 30% volume
GeneratedSoundStream<int16_t> sound(sawtooth);
I2SCodecStream out(GenericES8311);
StreamCopy copier(out, sound);

void setup()
{
  Serial.begin(115200);
  delay(500);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // I2C bus used to configure the ES8311 codec registers (and,
  // on this board, a TCA9555 I/O expander)
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
    // Speaker amp enable (PA_CTRL) is EXIO08 on this board, which is
    // bit 0 of the TCA9555's port 1. Set just that pin as an output
    // and drive it high; leave every other expander pin untouched
    // (still floating/input) so we don't affect anything else on it.
    Wire.beginTransmission(TCA9555_ADDR);
    Wire.write(TCA9555_REG_CONFIG_PORT1);
    Wire.write((uint8_t)0xFE); // bit0 = output, bits1-7 = input
    Wire.endTransmission();

    Wire.beginTransmission(TCA9555_ADDR);
    Wire.write(TCA9555_REG_OUTPUT_PORT1);
    Wire.write((uint8_t)0x01); // bit0 = high (PA_CTRL enabled)
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

void loop()
{
  static unsigned long lastPrint = 0;
  uint32_t touchValue = touchRead(PIN_TOUCH);

  sawtooth.active = (touchValue > TOUCH_THRESHOLD);

  // Print the raw value twice a second - useful for calibrating
  // TOUCH_THRESHOLD to your specific object/material.
  if (millis() - lastPrint > 500)
  {
    lastPrint = millis();
    Serial.printf("touch=%lu  active=%d\n", (unsigned long)touchValue,
                  sawtooth.active);
  }

  copier.copy();
}
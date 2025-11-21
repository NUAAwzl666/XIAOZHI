#include "AudioI2C.h"
#include <math.h>

namespace AudioI2C {

static int currentVolume = 80; // 0..100（默认80%音量）
static constexpr float MAX_GAIN = 1.0f; // 最大增益1倍
static constexpr float MIN_DB = -40.0f; // 0音量对应的dB衰减

bool begin() {
	currentVolume = 50;
	return true;
}

void setVolume(int vol) {
	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;
	currentVolume = vol;
}

void increase() {
	if (currentVolume < 100) currentVolume++;
}

void decrease() {
	if (currentVolume > 0) currentVolume--;
}

int getVolume() {
	return currentVolume;
}

float getSoftwareGain() {
	float level = (float)currentVolume / 100.0f; // 0..1
	float db = MIN_DB * (1.0f - level); // level=0 -> MIN_DB, level=1 -> 0dB
	float linear = powf(10.0f, db / 20.0f);
	return linear * MAX_GAIN;
}

} // namespace AudioI2C


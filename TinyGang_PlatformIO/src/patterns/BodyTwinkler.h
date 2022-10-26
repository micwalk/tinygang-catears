#ifndef H_BODYTWINKLE
#define H_BODYTWINKLE

#include "Pattern.h"

class BodyTwinkler : public Pattern {
   public:
	// inline uint32_t shade (float height, uint32_t color, uint32_t currentColor, float remaining, uint32_t secondaryColor) {
	CRGB paintLed(float position, float remaining, CRGB previous, int primaryHue) {
		int normalizedHue = primaryHue + 255.0 * position;
		if (normalizedHue > 255) {
			normalizedHue -= 255;
		}

		float rand01 = ((float)random(100)) / 100;
		float thresh = remaining - .05;
		thresh *= thresh;

		if (rand01 < thresh) {
			return CHSV(normalizedHue, 225, 128);
		} else {
			return previous.nscale8(16);
		}

		return previous;
	}
};

#endif

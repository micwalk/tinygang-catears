#ifndef __CHASER__H__
#define __CHASER__H__


#include "SpatialPattern.h"
#include <math.h>

// const size_t NUM_DEBUG_COLORS = 5;
// const CRGB::HTMLColorCode debugColorList[NUM_DEBUG_COLORS] = {CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::Green};
// //{CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::DarkGreen};

// const size_t NUM_DEBUG_HUE = 3;
// const uint8_t debugHueList[NUM_DEBUG_HUE] = {HSVHue::HUE_RED, HSVHue::HUE_GREEN, HSVHue::HUE_BLUE};

class Chaser final : public SpatialPattern {
    
	const float width = 15; //Width of line that chases down pattern. Units = wire pixel count
	//enum falloff_type -- linear, sin, spherical, etc.
	//inverseSpeed -- delay time to wait between units in delta micros.

	//todo: start pattern / reset pattern to reset line position. for now I suppose just use time remaining?

	float chasePosition = 0;
	const unsigned inverseSpeed = 5000000; //micros between pixels
	unsigned long patternRuntime = 0;

	CRGB paintSpatialLed(unsigned ledIndex, const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        float thisLedPos = (float) position.idWire;
        
		patternRuntime += deltaMicros;
		chasePosition = (float)patternRuntime / (float)inverseSpeed; 

		float chaseCenterPos = fmod(chasePosition, context.Count) - width;
        // float chaseCenterPos = (1-remaining) * (float) context.Count ;//deltaMicros * last position

		float distToCenter = abs(thisLedPos - chaseCenterPos);
		distToCenter = fmod(distToCenter, context.Count);
		float relDist = distToCenter / width;
		
		if(ledIndex == 0) {
			Serial.printf("pattern run %u. chasepos: %f centerpos %f\n", patternRuntime, chasePosition, chaseCenterPos);
		}


		byte brightness = 255;

		//simple square implementaion
		if(relDist < 1) {
			brightness = 255;

			byte sinInput = (int)(relDist*128) + 64;
			byte waveValue = sin8(sinInput);
			Serial.printf("led: %u. wirepos: %u dist %f rd: %f. sin input %u sin out %u\n", ledIndex, position.idWire, distToCenter, relDist, sinInput, waveValue);

			//brightness = 255;
			brightness = waveValue;
		} else {
			brightness = 0;
		}
		
		

		// brightness = waveValue;

		//also look for:
		//quadwave8 -- drop in for sin, faster
		// cubicwave8 -- harsher transitions, more time spent at peak.
		// triwave8 -- triangle wave, linear interpolation but with same domain as above.

        return CHSV(primaryHue, 255, brightness);
	}
};

#endif  //!__CHASER__H__
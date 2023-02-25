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
    
	public:

	byte m_hue = HUE_PURPLE; //Color of the chase line
	float m_width = 15; //Width of line that chases down pattern. Units = wire pixel count
	bool m_reversed = false; //Default low pixel to high pixel, true to reverse high to low.
	bool m_mirror = true; // two chasers ending/starting in the middle. non reversed starts on ears.

	byte m_hueRotation = 0;

	Chaser(byte hue, float width = 15, float speedMult = 1, bool reversed = false, bool mirrored = true, byte hueRotationAmt = 0) {
		m_hue = hue;
		m_width = width;
		m_reversed = reversed;
		m_mirror = mirrored;
		m_hueRotation = hueRotationAmt;

		inverseSpeed = 5000000 / speedMult; //micros between pixels
	}

	//enum falloff_type -- linear, sin, spherical, etc.
	

	//todo: start pattern / reset pattern to reset line position. for now I suppose just use time remaining?

	private:
	float chasePosition = 0;
	//inverseSpeed -- delay time to wait between units in delta micros.
	unsigned inverseSpeed = 5000000; //Time in micros it takes to move chaser center to next pixel
	unsigned long patternRuntime = 0;
	
	// bool m_hueChangeReady = false;
	float m_lastPos = 0;

	public:

	CRGB paintSpatialLed(unsigned ledIndex, const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
		// if(ledIndex == 0) {
		// 	Serial.printf("pattern run %u. chasepos: %f centerpos %f\n", patternRuntime, chasePosition, chaseCenterPos);
		// }


		//Get position of led
		float thisLedPos;
		unsigned patternCount = context.Count;

		if(m_mirror) {
			patternCount /= 2;
			thisLedPos = (float) context.ChasePosMirror(position.idWire);
		} else {
			thisLedPos = (float) context.ChasePosNice(position.idWire);
		}

		patternRuntime += deltaMicros;

		if(!m_reversed){
			chasePosition = (float)patternRuntime / (float)inverseSpeed; 
		} else {
			chasePosition = (float)(-1 * patternRuntime) / (float)inverseSpeed; 
		}

		float chaseCenterPos = fmod(chasePosition, patternCount) - m_width;
		if(chaseCenterPos <= m_lastPos) {
			m_hue += m_hueRotation;
		}
		m_lastPos = chaseCenterPos;

        // float chaseCenterPos = (1-remaining) * (float) patternCount ;//deltaMicros * last position

		float distToCenter = abs(thisLedPos - chaseCenterPos);
		distToCenter = fmod(distToCenter, patternCount);
		
		float relDist = distToCenter / m_width;
		
		// if(ledIndex == 0) {
		// 	Serial.printf("pattern run %u. chasepos: %f centerpos %f\n", patternRuntime, chasePosition, chaseCenterPos);
		// }

		//NOT WORKING
		// byte brightness = FalloffSin(distToCenter, m_width, 0);	
		
		byte brightness = 255;

		//simple square implementaion
		if(relDist < 1) {
			brightness = 255;

			byte sinInput = (int)(relDist*128) + 64;
			byte waveValue = sin8(sinInput);
			// Serial.printf("led: %u. wirepos: %u dist %f rd: %f. sin input %u sin out %u\n", ledIndex, position.idWire, distToCenter, relDist, sinInput, waveValue);

			//brightness = 255;
			brightness = waveValue;
		} else {
			brightness = 0;
		}

        return CHSV(m_hue, 255, brightness);
	}
};

#endif  //!__CHASER__H__
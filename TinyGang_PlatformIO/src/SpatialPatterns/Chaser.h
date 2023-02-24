#ifndef __CHASER__H__
#define __CHASER__H__


#include "SpatialPattern.h"

// const size_t NUM_DEBUG_COLORS = 5;
// const CRGB::HTMLColorCode debugColorList[NUM_DEBUG_COLORS] = {CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::Green};
// //{CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::DarkGreen};

// const size_t NUM_DEBUG_HUE = 3;
// const uint8_t debugHueList[NUM_DEBUG_HUE] = {HSVHue::HUE_RED, HSVHue::HUE_GREEN, HSVHue::HUE_BLUE};


//TODO: add led context to access neighbor leds, see total leds, etc.
class SpatialDebugger final : public SpatialPattern {
    
	virtual CRGB paintLed(unsigned ledIndex, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        int whichIdx = position.idWire;
        
        relPosition = whichIdx / LED_COUNT;
        NUM_LEDS;
        
        // trace
		if (abs(remaining - position) < .20) {
			return CHSV(primaryHue, 255, 255);
		}
		
		// return CHSV(220, 200, 128); // pink
		// fade
		return previous.nscale8(190);
        
        
        return CHSV(primaryHue, 255, std::max(255, 55 + 20 * block10Rem));
        
	}
};

#endif  //!__CHASER__H__
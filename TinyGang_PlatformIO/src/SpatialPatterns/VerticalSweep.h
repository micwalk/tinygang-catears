#ifndef __VERTICALSWEEP__H__
#define __VERTICALSWEEP__H__


#include "SpatialPattern.h"

class VerticalSweep final : public SpatialPattern {
    
	virtual CRGB paintLed(unsigned ledIndex, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        
        float relVertPos = (position.position.y + 100) / 200;
        
        float scanLinePos = 1 - remaining;
        
        // trace
		if (abs(scanLinePos - relVertPos) < .1) {
			return CHSV(primaryHue, 255, 255);
		}
		
		// return CHSV(220, 200, 128); // pink
		// fade
		return previous.nscale8(10);
        
	}
};

#endif  //!__VERTICALSWEEP__H__
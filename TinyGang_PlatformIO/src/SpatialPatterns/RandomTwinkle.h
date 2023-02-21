#ifndef __RANDOMTWINKLE__H__
#define __RANDOMTWINKLE__H__

#include "SpatialPattern.h"

class RandomTwinkle : public SpatialPattern {
    virtual CRGB paintLed(unsigned ledIndex, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        // println("position:" + position +", remaining:" + remaining);
		if (random(100) < 10) {
            return CHSV(random(150,250), 255, random(180,255) * (remaining + .2));
        }
		return previous.nscale8(32);
        
    }
    
};

#endif  //!__RANDOMTWINKLE__H__
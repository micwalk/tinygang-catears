#ifndef __RANDOMTWINKLE__H__
#define __RANDOMTWINKLE__H__

#include "SpatialPattern.h"

class RandomTwinkle : public SpatialPattern {

    byte m_hueBegin = 150;
    byte m_hueEnd = 250;

    byte m_brightLow = 180;
    byte m_brightHigh = 255;

    byte m_satLow = 150;
    byte m_satHigh = 255;

    CRGB paintSpatialLed(unsigned ledIndex, const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        // println("position:" + position +", remaining:" + remaining);
		if (random(100) < 15) {
            return CHSV(random8(m_hueBegin,m_hueEnd), random8(m_satLow, m_satHigh), random8(m_brightLow,m_brightHigh) * (remaining + .2));
        }
		return previous.nscale8(32);
        
    }
    
};

#endif  //!__RANDOMTWINKLE__H__
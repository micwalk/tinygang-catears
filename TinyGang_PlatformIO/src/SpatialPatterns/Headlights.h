#ifndef __HEADLIGHTS__H__
#define __HEADLIGHTS__H__


#include "SpatialPattern.h"

class Headlights : public SpatialPattern {
    CRGB paintSpatialLed(unsigned ledIndex,const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        // println("position:" + position +", remaining:" + remaining);
		// switch (position.section)
        // {
        // case LedSection::BAND:
        //     /* code */
        //     break;
        
        
        // default:
        //     break;
        // }
        
        if(position.section != LedSection::BAND) {
            return CHSV(primaryHue, 20, 255);
        }
		return previous.nscale8(32);
        
    }
    
};

#endif  //!__HEADLIGHTS__H__
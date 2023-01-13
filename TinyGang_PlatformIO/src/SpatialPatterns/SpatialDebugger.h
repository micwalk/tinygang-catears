#ifndef __SPATIALDEBUGGER__H__
#define __SPATIALDEBUGGER__H__

#include "SpatialPattern.h"

const size_t NUM_DEBUG_COLORS = 5;
const CRGB::HTMLColorCode debugColorList[NUM_DEBUG_COLORS] = {CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::DarkGreen};

class SpatialDebugger final : public SpatialPattern {
    
	virtual CRGB paintLed(unsigned ledIndex, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        int whichIdx = position.idWire;
        
        if(position.idSection == 0) {
            return CRGB::White;
        }
        if(position.idSection == 7) {
            return CRGB::DarkMagenta;
        }
        
        // if(whichIdx == 0) {
        //     return CRGB::Pink;
        // }
        
        int idMod = whichIdx % NUM_DEBUG_COLORS;
        return CRGB(debugColorList[idMod]);
	}
};

#endif  //!__SPATIALDEBUGGER__H__
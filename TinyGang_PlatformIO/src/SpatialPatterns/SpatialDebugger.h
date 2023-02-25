#ifndef __SPATIALDEBUGGER__H__
#define __SPATIALDEBUGGER__H__

#include "SpatialPattern.h"

const size_t NUM_DEBUG_COLORS = 5;
const CRGB::HTMLColorCode debugColorList[NUM_DEBUG_COLORS] = {CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::Green};
//{CRGB::Blue, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::DarkGreen};

const size_t NUM_DEBUG_HUE = 3;
const uint8_t debugHueList[NUM_DEBUG_HUE] = {HSVHue::HUE_RED, HSVHue::HUE_GREEN, HSVHue::HUE_BLUE};


class SpatialDebugger final : public SpatialPattern {
    
	CRGB paintSpatialLed(unsigned ledIndex, const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {
        int whichIdx = position.idWire;
        
        if(position.idSection == 0) {
            return CRGB::White;
        }
        // if(position.idSection == 7) {
        //     return CRGB::DarkMagenta;
        // }
        
        // if(whichIdx == 0) {
        //     return CRGB::Pink;
        // }
        
        // int idMod = whichIdx % NUM_DEBUG_COLORS;
        // return CRGB(debugColorList[idMod]);
        
        int block10Num = position.idSection / 10;
        int block10Rem = 10 - position.idSection % 10;
        
        return CHSV(debugHueList[block10Num % NUM_DEBUG_HUE], 255, std::max(255, 55 + 20 * block10Rem));
        
	}
};

#endif  //!__SPATIALDEBUGGER__H__
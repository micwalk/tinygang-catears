#ifndef __COLORWHEEL__H__
#define __COLORWHEEL__H__
#include "SpatialPattern.h"


class ColorWheel final : public SpatialPattern {
	public:
    uint32_t shiftInterval = 1000000; //Approx 4s/revolution?
	
	private:
	uint8_t hueShift = 0;
	uint32_t timeAccum = 0;
	
	public:

	ColorWheel(float speedMult = 1) {
		shiftInterval =  1000000 / speedMult;
	}

	CRGB paintSpatialLed(unsigned ledIndex, const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) {    
		//Logic: map angle (-180,180) to hue (0,255)
		// then shift mapping w/ deltatime and speed
		
		timeAccum += deltaMicros;
		uint32_t shifts = timeAccum / shiftInterval;
		if(shifts > 0) {
			hueShift += shifts; // should rollover
			timeAccum = 0;
		}
		
		float whichAngle;
		switch (position.section)
		{
		case LedSection::BAND :
			whichAngle = position.relCenter.angle;
			break;
		case LedSection::EAR_LEFT :
			whichAngle = position.relLeft.angle;
			break;
		case LedSection::EAR_RIGHT :
			whichAngle = -position.relRight.angle;
			break;
		default:
			whichAngle = 0;
			break;
		}
		
		float angle01 = ((whichAngle + 180) / 360);
		uint8_t targetHue = 255 * angle01 + hueShift;
		
		return CHSV(targetHue, 255, 255);
	}
};

#endif  //!__COLORWHEEL__H__
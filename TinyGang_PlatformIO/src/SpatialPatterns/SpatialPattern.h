#ifndef __SPATIALPATTERN__H__
#define __SPATIALPATTERN__H__

#include <FastLED.h>
//Included for CRGB class

#include "LedPosition.h"
#include "LedContext.h"

// Note: Ideally these virual calls are de-virtualized at compile time
// TODO: Check devirtualization works or convert to simple function pointers.

class SpatialPattern {
   public:
	/// @brief Calculates the new CRGB color value for a LED according to a pattern.
	///
    /// @param ledIndex physical led index
	/// @param context context to understand what other LEDs are present, especially the count.
    /// @param position LedPosition data of the led
	/// @param deltaMicros micros elapsed since last execution of pattern. 0 For first execution. 
    /// @param remaining Percent of time remaining. 0 is None, 1 is full length
	/// @param previous previous (current?) CRGB color value of LED
	/// @param primaryHue Hue to display
    ///
	/// @return New CRGB color of the LED
	virtual CRGB paintSpatialLed(unsigned ledIndex, const LedContext& context, const LedPosition& position, unsigned long deltaMicros, float remaining, CRGB previous, int primaryHue) = 0;

	protected:

	// Easy interpolation

		//also look for:
		//quadwave8 -- drop in for sin, faster
		// cubicwave8 -- harsher transitions, more time spent at peak.
		// triwave8 -- triangle wave, linear interpolation but with same domain as above.


	byte FalloffSin(float inputValue, float maxBright = 1, float minBright = 0) {
		// float chaseCenterPos = fmod(chasePosition, patternCount) - m_width;
		// float distToCenter = abs(thisLedPos - chaseCenterPos);
		// distToCenter = fmod(distToCenter, patternCount);
		// float relDist = distToCenter / m_width;

		//Bail if range is gonna be zero
		if(minBright == maxBright) {
			return inputValue == maxBright ? 255 : 0;
		}

		//check for max < min
		bool inverted = false;
		if(minBright > maxBright) {
			//inverted logic
			inverted = true;
			//Swap min and max
			float tmp = minBright;
			minBright = maxBright;
			maxBright = tmp;
		}

		byte brightness = 255;
		if(inputValue < minBright) brightness = 0;
		else if(inputValue > maxBright) brightness = 255;
		else {
			float range = maxBright - minBright;
			float scaledInput = (inputValue - minBright) / range;

			byte sinInput = (int)(scaledInput*128) + 64;
			brightness = quadwave8(sinInput);
		}
		
		if(inverted) brightness = 255 - brightness;
		return brightness;
	}
			
};

#endif  //!__SPATIALPATTERN__H__
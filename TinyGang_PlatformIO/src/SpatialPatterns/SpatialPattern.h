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
};

#endif  //!__SPATIALPATTERN__H__
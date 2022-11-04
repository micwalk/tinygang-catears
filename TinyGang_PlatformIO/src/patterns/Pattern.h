
#ifndef H_PATTERN
#define H_PATTERN

class Pattern {
   public:
	/// @brief Calculates the new CRGB color value for a LED according to a pattern.
	/// @param position Percentage of position along the strip. 0 is begining, 1 is end.
	/// @param remaining Percent of time remaining. 0 is None, 1 is full length
	/// @param previous previous CRGB color value of LED
	/// @param primaryHue Hue to display
	/// @return New CRGB color of the LED
	virtual CRGB paintLed(float position, float remaining, CRGB previous, int primaryHue);
};

#endif

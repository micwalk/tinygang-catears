#include "PatternSerialization.h"

//Static pattern library forming implicit index -> pattern map.
// Pattern* PATTERN_LIBRARY[PATTERNS_COUNT] = {
// 	new BodyTwinkler(),    // 0 red yellow sparkle short
// 	new BassShader(),      // 1 pattern individually triggered
// 	new RainbowSparkle(),  // 2 more pale pink and blue
// 	new WhiteTrace(),      // 3
// 	new BookendTrace(),    // 4
// 	new Twinkler(),        // 5
// 	new BookendFlip()      // 6
// };

SpatialPattern* PATTERN_LIBRARY[PATTERNS_COUNT] = {
	new SpatialDebugger()
};

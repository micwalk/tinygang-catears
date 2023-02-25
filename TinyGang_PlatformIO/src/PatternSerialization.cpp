#include "PatternSerialization.h"

#include "SpatialPatterns/Chaser.h"
#include "SpatialPatterns/VerticalSweep.h"
#include "SpatialPatterns/ColorWheel.h"
#include "SpatialPatterns/RandomTwinkle.h"

#include "SpatialPatterns/SpatialDebugger.h"
#include "SpatialPatterns/Headlights.h"

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
	new Chaser(),
	new RandomTwinkle(),
	new ColorWheel(),
	new VerticalSweep(),
	new Headlights(),
	new SpatialDebugger()
};

int PATTERN_HUE[] = {HUE_PURPLE, 0, HUE_PINK, 0, HUE_AQUA, 0};

static_assert(
	PATTERNS_COUNT == (sizeof(PATTERN_HUE) / sizeof(PATTERN_HUE[0])),
	"PATTERN_HUE doesn't match size of PATTERNS_COUNT");
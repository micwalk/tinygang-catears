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

//update note: go to the cpp
SpatialPattern* PATTERN_LIBRARY[PATTERNS_COUNT] = {
	new Chaser(HUE_PINK, 30, true, true),
	new Chaser(HUE_AQUA, 15, false, false),
	new Chaser(HUE_GREEN, 30, true),
	new Chaser(HUE_PURPLE, 30, true),
	new RandomTwinkle(), //COLOR?
	new ColorWheel(), //add speed mod
	new VerticalSweep(HUE_RED), //ADD COLOR. red.
	new VerticalSweep(HUE_BLUE), //ADD COLOR. red.
	// new Headlights() //kinda just debug, probably drop.
	// new SpatialDebugger()
};

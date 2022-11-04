#ifndef __PATTERNSERIALIZATION__H__
#define __PATTERNSERIALIZATION__H__

/* *********************
Pattern serialization is getting complicated because of what it means to be a "pattern"
Need to rename some concepts...

The "Pattern" class might be renamed "PatternFunction"
    It defines a single function to compute the current value for a pixel.

These patterns are stored in a fixed library / array, referenced through
an integer index. This index has now been typedef'd to a PatternReference.

A PatternReference is serialized and deserialized for network communication by
conversion into a single char by using the PATTERN_COMMANDS array. 

********************* */


//For String
#include "Arduino.h"

#include "patterns/BassShader.h"
#include "patterns/BodyTwinkler.h"
#include "patterns/BookendFlip.h"
#include "patterns/BookendTrace.h"
#include "patterns/RainbowSparkle.h"
#include "patterns/Twinkler.h"
#include "patterns/WhiteTrace.h"

//This must be fixed pattern library
Pattern *patterns[] = {
	new BodyTwinkler(),    // 0 red yellow sparkle short
	new BassShader(),      // 1 pattern individually triggered
	new RainbowSparkle(),  // 2 more pale pink and blue
	new WhiteTrace(),      // 3
	new BookendTrace(),    // 4
	new Twinkler(),        // 5
	new BookendFlip()      // 6
};

const size_t PATTERNS_COUNT = (sizeof(patterns)) / 4;

constexpr char PATTERN_COMMANDS[] = {
	'q', 'a', 'z',
	'w', 's', 'x',
	'e', 'd', 'c',
	'r', 'f', 'v'};
const size_t SERIALIZED_PATTERN_COUNT = std::min(PATTERNS_COUNT, sizeof(PATTERN_COMMANDS));

//Intentionally using signed int so we can use negative as invalid flag;
using PatternReference = int;
const PatternReference INVALID_PATTERN_REF = -1;
inline bool PatternRefValid(PatternReference ref) {
	return ref >= 0 && ref < SERIALIZED_PATTERN_COUNT;
}

struct SerializedPattern {
	char commandChar;
    
    SerializedPattern(PatternReference patternIndex = 0) {
        if(PatternRefValid(patternIndex)) {
            //Lookup command in array
            commandChar = PATTERN_COMMANDS[patternIndex];
        } else {
            //Normally would throw, but since exceptions are disabled...
            //On error just return first
            commandChar = PATTERN_COMMANDS[0];
        }
        
    }
    
    String toString() {
        return String(commandChar);
    }
};

inline SerializedPattern SerializePattern(PatternReference patternRef) {
	return SerializedPattern(patternRef);
}

inline PatternReference DeserializePattern(SerializedPattern serializedPattern) {
	//Search by index into PATTERN_COMMANDS
	for (int i = 0; i < std::min(PATTERNS_COUNT, SERIALIZED_PATTERN_COUNT); i++) {
		if (serializedPattern.commandChar == PATTERN_COMMANDS[i]) {
			return i;
		}
	}
	return INVALID_PATTERN_REF;
}

#endif  //!__PATTERNSERIALIZATION__H__
#ifndef __PATTERNRUNNER__H__
#define __PATTERNRUNNER__H__

#include <elapsedMillis.h>

#include "PatternSerialization.h"

int PATTERN_HUE[] = {0, 20, 255, 229, 120, 200, 207};
// 120 should be green, not cyan
// 229 pink
// 22 orange
// 200 lilac
// 'a' green
static_assert(
	PATTERNS_COUNT == (sizeof(PATTERN_HUE) / sizeof(PATTERN_HUE[0])),
	"PATTERN_HUE doesn't match size of PATTERNS_COUNT");

// Each user (gang member) on the mesh gets to pick and broadcast
// their own custom pattern. This custom pattern is a pattern algorithm (patternRef)
// plus a pattern color (hue)
struct UserPattern {
	PatternReference patternRef;
	uint8_t hue;
	// IsActive?
	// Duration? Remaining Time?
};

// When keeping track of running patterns, we need to know
// both the UserPattern info as well as timing information
// When to start running it, when to stop running it.
struct ScheduledPattern {
	UserPattern userPattern;

	unsigned long startMs;
	unsigned long endMs;

	// Getter
	bool IsActive() const {
		auto now = millis();
		return now >= startMs && now < endMs;
	}

	unsigned long Duration() const {
		return endMs - startMs;
	}

	unsigned long Elapsed() const {
		auto now = millis();
		if (now < startMs) return 0;
		return now - startMs;
	}
	
	Pattern* GetPattern() const{
		return patterns[userPattern.patternRef];
	}

	// Returns a float in range [0-1] where 1 Represents full time remaining and 0 represents no time remaining
	//Note: if this is called on an inactive pattern, will return values outside of range [0-1]
	float RelativeRemaining() {
		return (1.0 - (float)Elapsed() / (float)Duration());
	}

	// Mutators
	void ScheduleNow(unsigned long duration) {
		startMs = millis();
		// TODO: overflow check
		endMs = startMs + duration;
	}
};

template <unsigned int LED_COUNT>
class PatternRunner {
   private:
	
	ScheduledPattern m_patternSchedules[PATTERNS_COUNT];
	unsigned long m_durationMs;

   public:
	// Just making this buffer public to avoid more complicated getters
	CRGB m_outBuffer[LED_COUNT];

	PatternRunner(unsigned long duration = 1000) : m_durationMs(duration) {
		// Init pattern schedules
		// Original logic: Create one schedule for each pattern. Pre-define hues
		for (size_t i = 0; i < PATTERNS_COUNT; i++) {
			// Init userPattern for this scheduled pattern.
			m_patternSchedules[i].userPattern.patternRef = i;
			m_patternSchedules[i].userPattern.hue = PATTERN_HUE[i];
			m_patternSchedules[i].ScheduleNow(m_durationMs);
		}
	}
	~PatternRunner() = default;

	// Getters
	unsigned long PatternElapsed(int patternId) const {
		return m_patternSchedules[patternId].Elapsed();
	}

	bool PatternActive(int patternId) const {
		return m_patternSchedules[patternId].IsActive();
	}

	size_t PatternCount() const { return PATTERNS_COUNT; }

	// Setter
	void StartPattern(int patternId) {
		Serial.printf("%u: PatternRunner: Starting pattern %i\n", millis(), patternId);
		m_patternSchedules[patternId].ScheduleNow(m_durationMs);
	}

	// Key Callbacks

	// Called on main.setup
	void setup() {
		for (int i = 0; i < LED_COUNT; i++) {
			m_outBuffer[i] = CHSV(255 / LED_COUNT * i, 255 / LED_COUNT * i, 255);
		}
	}

   private:
	uint32_t m_lastPatternMask = 0;

	uint32_t activePatternMask() {
		uint32_t patternMask = 0;
		for (int i = 0; i < (PATTERNS_COUNT > 32 ? 32 : PATTERNS_COUNT); i++) {
			if (!PatternActive(i)) continue;
			patternMask |= (1 << i);
		}
		return patternMask;
	}

   public:
	// TODO: migrate to .cpp?
	void updateLedColors() {
		int activeCt = 0;
		for (int i = 0; i < PATTERNS_COUNT; i++) {
			ScheduledPattern& scheduledPattern = m_patternSchedules[i];
			if (!scheduledPattern.IsActive()) continue;
			activeCt++;
			
			//Execute pattern on all pixels
			for (int j = 0; j < LED_COUNT; j++) {
				float position = (float)j / (float)LED_COUNT;
				float remaining = scheduledPattern.RelativeRemaining();
				Pattern* patternAlgo = scheduledPattern.GetPattern();
				
				// TODO: re-introduce the concept of inboundHue (renamed to legacy_inbound_hue), which isn't sent / changed currently
				//  This also includes the idea of self color => pattern_hue lookup vs inbound hue for self vs other patterns
				m_outBuffer[j] = patternAlgo->paintLed(position, remaining, m_outBuffer[j], scheduledPattern.userPattern.hue);
			}
		}

		//Look at change in pattern mask to see when patterns stop/start and print message about it
		uint32_t activeMask = activePatternMask();
		if (activeMask != m_lastPatternMask) {
			Serial.printf("%u: PatternRunner: %i Active Patterns. PatternMask:%i\n", millis(), activeCt, activeMask);
			if (activeCt == 0) Serial.print("    *All Patterns Dormant*\n");
		}

		if (activeCt == 0) {
			// Slowly fade out
			for (int j = 0; j < LED_COUNT; j++) {
				m_outBuffer[j] = m_outBuffer[j].nscale8(200);
			}
		}

		m_lastPatternMask = activeMask;
	}
};

#endif  //!__PATTERNRUNNER__H__
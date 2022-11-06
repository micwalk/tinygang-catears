#ifndef __PATTERNRUNNER__H__
#define __PATTERNRUNNER__H__

#include <elapsedMillis.h>

#include "PatternSerialization.h"

// When keeping track of running patterns, we need to know
// both the Shared Node pattern info as well as timing information
// When to start running it, when to stop running it.
struct ScheduledPattern {
	SharedNodeData nodeData;

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
		return patterns[nodeData.nodePattern];
	}

	// Returns a float in range [0-1] where 1 Represents full time remaining and 0 represents no time remaining
	//Note: if this is called on an inactive pattern, will return values outside of range [0-1]
	float RelativeRemaining() {
		return (1.0 - (float)Elapsed() / (float)Duration());
	}

	// Mutators
	void StopNow() {
		startMs = 0;
		endMs = 0;
	}
	
	void ScheduleNow(unsigned long duration) {
		startMs = millis();
		// TODO: overflow check
		endMs = startMs + duration;
	}
	void ScheduleFuture(unsigned long start, unsigned long duration) {
		startMs = start;
		// TODO: overflow check
		endMs = startMs + duration;
	}
};

template <unsigned int LED_COUNT>
class PatternRunner {
   private:
   	//Main schedule stuff
	ustd::array<ScheduledPattern> m_patternSchedules;
	
	//Metadata stuff for tracking active patterns
   	uint32_t m_lastActivePatterns = 0;
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
	// Just making this buffer public to avoid more complicated getters
	CRGB m_outBuffer[LED_COUNT];

	PatternRunner(unsigned long duration = 1000) : m_patternSchedules(2, MAX_PEERS, 2, false) {
		// Init pattern schedules
		// Original logic: Create one schedule for each pattern. Pre-define hues
		// for (size_t i = 0; i < PATTERNS_COUNT; i++) {
		// 	// Init userPattern for this scheduled pattern.
		// 	m_patternSchedules[i].userPattern.patternRef = i;
		// 	m_patternSchedules[i].userPattern.hue = PATTERN_HUE[i];
		// 	m_patternSchedules[i].ScheduleNow(m_durationMs);
		// }
	}
	~PatternRunner() = default;

	// Getters
	unsigned long PatternElapsed(int patternId) const {
		return m_patternSchedules[patternId].Elapsed();
	}

	bool PatternActive(int patternId) const {
		return m_patternSchedules[patternId].IsActive();
	}
	
	//todo: use
	uint32_t GetNumActivePatterns() {
		return m_lastActivePatterns;
	}


	// Setter
	void StartPattern(int patternId, unsigned long duration) {
		Serial.printf("%u: PatternRunner: Starting pattern %i\n", millis(), patternId);
		m_patternSchedules[patternId].ScheduleNow(duration);
	}
	
	void StopAllPatterns() {
		for (auto& ps : m_patternSchedules) {
			ps.StopNow();
		}
	}
	
	void SetPatternSlot(size_t slotIdx, SharedNodeData nodeData, uint32_t startMilis, uint32_t duration) {
		if(slotIdx >= MAX_PEERS) {
			Serial.println("exceed max pattern slots, return");
			return;
		}
		
		ScheduledPattern& slotData = m_patternSchedules[slotIdx];
					
		slotData.nodeData = nodeData;
		
		Serial.printf("%u: PatternRunner.SetPatternSlot: Updating slot %u to have pattern %i. Starting at %u for duration %u\n",
					millis(), slotIdx, slotData.nodeData.nodePattern, startMilis, duration);
					
		slotData.ScheduleFuture(startMilis, duration);
	}

	// Key Callbacks

	// Called on main.setup
	void setup() {
		for (int i = 0; i < LED_COUNT; i++) {
			m_outBuffer[i] = CHSV(255 / LED_COUNT * i, 255 / LED_COUNT * i, 255);
		}
	}

   public:
	// TODO: migrate to .cpp?
	void updateLedColors() {
		m_lastActivePatterns = 0;
		for (ScheduledPattern& scheduledPattern : m_patternSchedules) {
			if (!scheduledPattern.IsActive()) continue;
			m_lastActivePatterns++;
			
			//Execute pattern on all pixels
			for (int j = 0; j < LED_COUNT; j++) {
				float position = (float)j / (float)LED_COUNT;
				float remaining = scheduledPattern.RelativeRemaining();
				Pattern* patternAlgo = scheduledPattern.GetPattern();
				
				// TODO: re-introduce the concept of inboundHue (renamed to legacy_inbound_hue), which isn't sent / changed currently
				//  This also includes the idea of self color => pattern_hue lookup vs inbound hue for self vs other patterns
				m_outBuffer[j] = patternAlgo->paintLed(position, remaining, m_outBuffer[j], scheduledPattern.nodeData.hue);
			}
		}

		//Look at change in pattern mask to see when patterns stop/start and print message about it
		uint32_t activeMask = activePatternMask();
		if (activeMask != m_lastPatternMask) {
			Serial.printf("%u: PatternRunner: %i Active Patterns. PatternMask:%i\n", millis(), m_lastActivePatterns, activeMask);
			if (m_lastActivePatterns == 0) Serial.print("    *All Patterns Dormant*\n");
		}

		if (m_lastActivePatterns == 0) {
			// Slowly fade out
			for (int j = 0; j < LED_COUNT; j++) {
				m_outBuffer[j] = m_outBuffer[j].nscale8(200);
			}
		}

		m_lastPatternMask = activeMask;
	}
};

#endif  //!__PATTERNRUNNER__H__
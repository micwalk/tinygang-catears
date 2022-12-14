#ifndef __PATTERNRUNNER__H__
#define __PATTERNRUNNER__H__

#include <elapsedMillis.h>

#include "PatternSerialization.h"

// When keeping track of running patterns, we need to know
// both the Shared Node pattern info as well as timing information
// When to start running it, when to stop running it.
struct ScheduledPattern {
	SharedNodeData nodeData;
	
	elapsedMicros timeSinceSchedule;
	uint32_t startDelay;
	uint32_t duration; //using duration instead of endMicros
	
	// Getter
	bool IsActive() const {
		return (timeSinceSchedule >= startDelay) && (timeSinceSchedule < (startDelay + duration));	
	}

	unsigned long Duration() const {
		return duration;
	}

	unsigned long Elapsed() const {
		auto now = micros();
		if (timeSinceSchedule < startDelay) return 0;
		return timeSinceSchedule - startDelay;
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
		timeSinceSchedule = 0;
		startDelay = 0;
		duration = 0;
	}
	
	void ScheduleNow(unsigned long durationMicros) {
		timeSinceSchedule = 0;
		startDelay = 0;
		duration = durationMicros;
	}
	
	void ScheduleFuture(unsigned long start, unsigned long durationMicros) {
		auto now = micros();
		//Note: this doesn't work when start has rolled over, will think its far in the past.
		bool schedPast = start < now;
		if(schedPast) {
			timeSinceSchedule = now - start;			
			startDelay = 0;
			duration = durationMicros;
		} else {
			timeSinceSchedule = 0;
			startDelay = start - now; 
			duration = durationMicros;	
		}
		
		// Serial.printf("%u [%u]: Sched(a_start:%u) %s timeSinceSchedule:%u, startDelay:%u, duration:%u\n",
		// 	millis(), now, start, schedPast ? "PAST" : "FUTURE" , timeSinceSchedule, startDelay, duration);
		
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

	uint32_t activeScheduleMask() {
		uint32_t mask = 0;
		for (int i = 0; i < (scheduleCount() > 32 ? 32 : scheduleCount()); i++) {
			if (!PatternActive(i)) continue;
			mask |= (1 << i);
		}
		return mask;
	}
	
   public:
	// Just making this buffer public to avoid more complicated getters
	CRGB m_outBuffer[LED_COUNT];

	PatternRunner() : m_patternSchedules(2, MAX_PEERS, 2, false) {
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

	unsigned int scheduleCount() {
		return m_patternSchedules.length();
	}
	
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
	void StartPattern(int patternId, unsigned long durationMicros) {
		Serial.printf("%u: PatternRunner: Starting pattern %i\n", millis(), patternId);
		m_patternSchedules[patternId].ScheduleNow(durationMicros);
	}
	
	void StopAllPatterns() {
		for (auto& ps : m_patternSchedules) {
			ps.StopNow();
		}
	}
	
	void SetPatternSlot(size_t slotIdx, SharedNodeData nodeData, uint32_t startMicros, uint32_t durationMicros) {
		if(slotIdx >= MAX_PEERS) {
			Serial.println("exceed max pattern slots, return");
			return;
		}
		
		//Update slot data
		ScheduledPattern& slotData = m_patternSchedules[slotIdx];
		slotData.nodeData = nodeData;
		
		//Schedule
		Serial.printf("%u [%u]: PatternRunner.SetPatternSlot: Updating slot %u to have pattern %i. Starting at %u for durationMicros %u\n",
					millis(), micros(), slotIdx, slotData.nodeData.nodePattern, startMicros, durationMicros);
					
		slotData.ScheduleFuture(startMicros, durationMicros);
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
		uint32_t activeMask = activeScheduleMask();
		if (activeMask != m_lastPatternMask) {
			Serial.printf("%u: PatternRunner: ActiveCt: %u PatternMask: %u\n", millis(), m_lastActivePatterns, activeMask);
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
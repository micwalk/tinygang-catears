/*
   Tiny Gang is an open source project to include multiple jacket wearers
   the base code is developed by @mpinner and @adellelin for two jacket
   interaction and is to be expanded for multiple players
*/

#include <Arduino.h>
// Note: for these two, must include FastLED and elapsedMillis libraries in your Audrino IDE/platformIO
#include <FastLED.h>
#include <elapsedMillis.h>

#include "GangMesh.h"
#include "PatternRunner.h"

// *** LOOKING TO CHANGE WHAT PIN YOU HAVE ATTACHED? ***
// *** SEE UserConfig.h ***
#include "UserConfig.h"

// Some State
int legacy_inbound_hue = 229;  // incoming char sets color
//Storage for own node data, separate from mesh.
size_t m_ownNodeIndex = 0;
SharedNodeData m_ownNodeData(DEFAULT_PATTERN);


#if defined(PATTERN_SELECT_PUSHBTN)
// Pushbutton state
//  Setup a new OneButton on pin A1.
OneButton cyclePatternBtn(PUSHBUTTON_PIN, true);
int chosenPattern = m_ownNodeData.nodePattern;  // Current pattern state
#endif

// Pattern running info
uint32_t PATTERN_DURATION = 4000; //Length each pattern plays. Note: If this isn't the same on all nodes then they won't sync up!
unsigned PATTERN_OVERLAP_MS = 150;

PatternRunner<NUM_LEDS> patternRunner(PATTERN_DURATION);

// Pattern display
CRGB render_leds[NUM_LEDS];

// Pattern Broadcasting
void broadcastPattern();
uint32_t BROADCAST_REPEAT_TIME = PATTERN_DURATION + PATTERN_OVERLAP_MS * 2;  // Rebroadcast info timer
elapsedMillis lastBroadcastTime;

// More Declarations
void receiveKeyboardMsg(char inChar);
void checkPatternTimer();
int getChosenPattern();
void show();

//Helper class for 
GangMesh gangMesh;

//Function to update own node data.
void updateOwnNodeData() {
	//TODO: refactor getChosenPattern to updateOwnNodeData;
	m_ownNodeData.nodePattern = getChosenPattern();
	m_ownNodeData.resetDefaultHue();
}

void setup() {
	const unsigned long BAUD = 921600;
	// put your setup code here, to run once:
	Serial.begin(BAUD);
	Serial.println("\n\n******TinyGang Reset******");
	Serial.println(__FILE__);
	Serial.println(__DATE__);

#if defined(PATTERN_SELECT_PUSHBTN)
	// Setup pushbutton
	cyclePatternBtn.attachClick(onPatternChangeClick);
	// pinMode(PUSHBUTTON_PIN, INPUT);
#elif defined(PATTERN_SELECT_DIPSWITCH)
	// setup dipswitch
	for (int i = 0; i < 2; i++) {
		pinMode(dipswitch_pins[i], INPUT_PULLUP);
	}
#endif
	// No setup needed for const pattern select

	// TODO: Better user configurable LED setup
	FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(render_leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	FastLED.setBrightness(LED_BRIGHTNESS);
	FastLED.setMaxPowerInVoltsAndMilliamps(MAX_VOLTS, MAX_MILLIAMPS);
	FastLED.setDither(0);

	patternRunner.setup();
	patternRunner.SetPatternSlot(0, m_ownNodeData, 0, PATTERN_DURATION);
	
	gangMesh.setup();
}

void handleKeyboardInput() {
	// available() -- Get the number of bytes (characters) available for reading from the serial port.
	// This is data that's already arrived and stored in the serial receive buffer (which holds 64 bytes).
	// This can be used to force sending a message from the keyboard while debugging.
	while (Serial.available()) {
		char inChar = (char)Serial.read();
		Serial.println("Got msg from serial read.");
		receiveKeyboardMsg(inChar);
	}
}
void loop() {
// Only btn PUSHBTN needs a loop callback.
#if defined(PATTERN_SELECT_PUSHBTN)
	cyclePatternBtn.tick();
#endif

	handleKeyboardInput();
	checkPatternTimer();
	broadcastPattern();
	patternRunner.updateLedColors();  // TODO: send legacy_inbound_hue again?
	show();
	gangMesh.update();
	
	delay(10);
}

#if defined(PATTERN_SELECT_PUSHBTN)
void onPatternChangeClick() {
	auto btnVal = digitalRead(PUSHBUTTON_PIN);
	int oldPattern = chosenPattern;
	chosenPattern = (oldPattern + 1) % PATTERNS_COUNT;
	Serial.printf("%u: Button Clicked Callback!. Changing Pattern from %i to %i\n", millis(), oldPattern, chosenPattern);
}
#endif

int getChosenPattern() {
#if defined(PATTERN_SELECT_PUSHBTN)
	return chosenPattern;
#elif defined(PATTERN_SELECT_DIPSWITCH)
	int swA = digitalRead(dipswitch_pins[0]);
	int swB = digitalRead(dipswitch_pins[1]);
	if (swA == 0 && swB == 0) return 0;
	if (swA == 0 && swB == 1) return 1;
	if (swA == 1 && swB == 0) return 2;
	if (swA == 1 && swB == 1) return 3;
	return 0;  // Default..
#elif defined(PATTERN_SELECT_CONST)
	return USER_PATTERN;
#endif
}

void show() {
	// jacket 1 and 2 have different mappings for their led strips
	// jacket 1 is mirrored, jacket 2 is mirrored
	int half_leds = (NUM_LEDS / 2);

	switch (RENDER_TYPE) {
		case RENDER_MIRRORED:
			for (int i = 0; i < half_leds; i++) {
				render_leds[i] = patternRunner.m_outBuffer[i];
			}
			for (int i = 0; i < half_leds; i++) {
				render_leds[i + half_leds] = patternRunner.m_outBuffer[half_leds - i];
			}
			break;
		case RENDER_NORMAL:
			for (int i = 0; i < NUM_LEDS; i++) {
				render_leds[i] = patternRunner.m_outBuffer[i];
			}
			break;
	}

	FastLED.show();

	return;
}

void checkPatternTimer() {
	// Checks chosen pattern and launches own pattern on repeat

	auto oldPatternId = m_ownNodeData.nodePattern;
	updateOwnNodeData();

	bool patternChanged = oldPatternId != m_ownNodeData.nodePattern;
	
	if (patternChanged) {
		Serial.printf("Own pattern switch detected from %i to %i\n", oldPatternId, m_ownNodeData.nodePattern);
		lastBroadcastTime = BROADCAST_REPEAT_TIME + 1;
		// Might want to do something special??
		// TODO: Create startPattern / pattern renderer class and pattern selector class.
		rescheduleLightsCallbackMain();
	}
	
	if(patternRunner.GetNumActivePatterns() == 0) {
		Serial.printf("%u: No patterns active!\n", millis());
		rescheduleLightsCallbackMain();
	}
	
	return;
}

// Broadcasts current pattern to others on the mesh.
void broadcastPattern() {
	// If already sent or not playing yet, then don't broadcast
	if (lastBroadcastTime < BROADCAST_REPEAT_TIME || !patternRunner.PatternActive(m_ownNodeIndex)) return;

	String msg = SerializeNodeData(m_ownNodeData).toString();

	// send message to other jacket
	Serial.printf("%u: SENDING:: my-pattern-id: %i. Msg: %s\n", millis(), m_ownNodeData.nodePattern, msg);
	// TODO: Also send hue!
	// TODO: enhance message to include time remaining? -- However this usually sends with maximum time remaining
	gangMesh.sendBroadcast(msg);
	lastBroadcastTime=0;
}

void receiveKeyboardMsg(char inChar) {
	// TODO: change input message to type char.
	// 2 callers: one from keybaord, one from GangMesh
	// Keyboard : need to create simplified / legacy pattern
	// GangMesh -- Need to make PatternRunner.h importable

	SerializedNodeData sPattern(inChar);

	// Deserialize pattern index, don't forget to check for error return value
	int patternIdx = DeserializeNodeData(sPattern).nodePattern;

	if (patternIdx >= 0) {
		// We have a valid pattern, start the corresponding pattern
		Serial.printf("%u: Message: %c. Corresponds to pattern %i. Resetting that pattern. PatternCount: %u\n",
		              millis(), inChar, patternIdx, PATTERNS_COUNT);
		m_ownNodeData.nodePattern = patternIdx;
		//todo: update data in pattern runner
		patternRunner.StartPattern(m_ownNodeIndex, PATTERN_DURATION);
	} else {
		// if incoming characters do not correspond to pattern ID
		// then the other device must be running wire protocol v1.
		// Therefore set set inbound pattern color
		Serial.printf("%u: Message: %c. PatternChar out of bounds. Activating legacy handling to set inbound hue\n",
		              millis(), inChar);
		legacy_inbound_hue = inChar;
		rescheduleLightsCallbackMain();
	}
}

void receiveMeshMsg(uint32_t nodeName, SharedNodeData nodeData) {
	int patternIdx = nodeData.nodePattern;
	Serial.printf("%u: Node data for node %u deserialized to pattern %i. Waiting for schedule.\n",
		              millis(), nodeName, patternIdx);
}

void rescheduleLightsCallbackMain() {
	auto& allNodes = gangMesh.getNodeList();
	auto nodeCount = allNodes.size();
	
	Serial.printf("%u: *Pattern Scheduler* Triggered reschedule with %u nodes\n", millis(), nodeCount);
	if(nodeCount == 0) {
		Serial.printf("%u:   bailing bc no nodes\n", millis());
		return;
	}
	
	unsigned long myMicros = micros();
	uint32_t meshMicros = gangMesh.mesh.getNodeTime();
	int meshDiff = meshMicros - myMicros;

	Serial.printf("%u: Pattern Scheduler: MyMicros: %u MeshMicros: %u (diff: %i)\n",
					millis(), myMicros, meshMicros, meshDiff);

	
	uint32_t durationMicros = PATTERN_DURATION * 1000;
	
	uint32_t totalRepeatMicros = durationMicros * nodeCount;
	uint32_t intervalPortion = meshMicros % totalRepeatMicros;
	
	uint32_t lastStartTime = meshMicros - intervalPortion;
	uint32_t nextStartTime = lastStartTime + totalRepeatMicros;

	Serial.printf("%u: Pattern Scheduler: Currently %u into pattern repeat. Current round lasts from [%u - %u] mesh. [%u - %u] local \n",
					millis(), intervalPortion, lastStartTime, nextStartTime, gangMesh.TimeMeshToLocal(lastStartTime)/1000, gangMesh.TimeMeshToLocal(nextStartTime) / 1000);

	patternRunner.StopAllPatterns();
	size_t nodeIdx = 0;
	for (auto& nodeId : allNodes) {
		uint32_t nodeStartTime = lastStartTime + durationMicros * nodeIdx;
		uint32_t nodeEndTime = lastStartTime + durationMicros * (1+nodeIdx);
		
		Serial.printf("%u: [%u] Pattern Scheduler: Slot: [%u-%u]mesh [%u-%u]local\n",
					millis(), nodeIdx, nodeStartTime, nodeEndTime, gangMesh.TimeMeshToLocal(nodeStartTime)/1000, gangMesh.TimeMeshToLocal(nodeEndTime)/1000);
					
		if(nodeEndTime < meshMicros) {
			//push it to the next one instead			
			nodeStartTime = nextStartTime + durationMicros * nodeIdx;
			nodeEndTime   = nextStartTime + durationMicros * (1+nodeIdx);
			
			Serial.printf("%u: [%u] Pattern Scheduler: Shifting since slot expired. New Slot: [%u-%u]mesh [%u-%u]local\n",
					millis(), nodeIdx, nodeStartTime, nodeEndTime, gangMesh.TimeMeshToLocal(nodeStartTime)/1000, gangMesh.TimeMeshToLocal(nodeEndTime)/1000);
		}
		
		uint32_t nodeStartLocal = gangMesh.TimeMeshToLocal(nodeStartTime) / 1000;
		
		SharedNodeData *whichNodeData = 0;
		if(gangMesh.mesh.getNodeId() == nodeId) {
			//Self, re-generate own pattern
			m_ownNodeIndex = nodeIdx;
			updateOwnNodeData();
			whichNodeData = &m_ownNodeData;
		} else {
			//Other, lookup
			whichNodeData = &gangMesh.m_nodeData[nodeId];
		}
		 
		Serial.printf("%u: [%u] Pattern Scheduler: Should start node %u's pattern %i at mesh time %u local time %u\n",
					millis(), nodeIdx, nodeId, whichNodeData->nodePattern, nodeStartTime, nodeStartLocal);
		
		
		uint32_t startWithOverlap = std::min(nodeStartLocal, nodeStartLocal - PATTERN_OVERLAP_MS);
		//Actually update info and trigger pattern
		//TODO: convert to passing ptrs all through code.
		patternRunner.SetPatternSlot(nodeIdx, *whichNodeData, startWithOverlap, PATTERN_DURATION + PATTERN_OVERLAP_MS);
		nodeIdx++;
	}//End loop over nodes	
	
} //end rescheduleLightsCallbackMain()
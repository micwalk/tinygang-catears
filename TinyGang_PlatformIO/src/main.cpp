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

// Pattern Choosing
int myPatternId = DEFAULT_PATTERN;

#if defined(PATTERN_SELECT_PUSHBTN)
// Pushbutton state
//  Setup a new OneButton on pin A1.
OneButton cyclePatternBtn(PUSHBUTTON_PIN, true);
int chosenPattern = myPatternId;  // Current pattern state
#endif

// Pattern running info
const float PATTERN_DURATION = 4000;
float repeatDurationMs = 8000;  // Will display an animation every this often.
PatternRunner<NUM_LEDS> patternRunner(PATTERN_DURATION);

// Pattern display
CRGB render_leds[NUM_LEDS];

// Pattern Broadcasting
void broadcastPattern();
boolean sentAlready = false;  // State to track if we broadcasted our pattern yet

// More Declarations
void receiveKeyboardMsg(char inChar);
void checkPatternTimer();
int getChosenPattern();
void show();

GangMesh gangMesh;

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
	gangMesh.setup();
}

void loop() {
// Only btn PUSHBTN needs a loop callback.
#if defined(PATTERN_SELECT_PUSHBTN)
	cyclePatternBtn.tick();
#endif

	// available() -- Get the number of bytes (characters) available
	// for reading from the serial port.
	// This is data that's already arrived and stored in the
	// serial receive buffer (which holds 64 bytes).

	// This can be used to force sending a message from the keyboard while debugging.
	while (Serial.available()) {
		char inChar = (char)Serial.read();
		Serial.println("Got msg from serial read.");
		receiveKeyboardMsg(inChar);
	}

	checkPatternTimer();
	broadcastPattern();
	patternRunner.updateLedColors();  // TODO: send legacy_inbound_hue again?
	show();
	delay(10);

	// painless mesh
	gangMesh.update();
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

	auto oldPatternId = myPatternId;
	myPatternId = getChosenPattern();

	bool patternChanged = oldPatternId != myPatternId;

		// Automatically start own pattern every repeatDuration
	if (patternRunner.PatternElapsed(myPatternId) > repeatDurationMs) {
		Serial.printf("%u: Legacy Pattern Scheduler: repeatDurationMs has elapsed, restarting self pattern: %i\n",
		              millis(),  myPatternId);
		
		patternRunner.StartPattern(myPatternId);
		sentAlready = false;
	}
	
	if (patternChanged) {
		Serial.printf("Own pattern switch detected from %i to %i\n", oldPatternId, myPatternId);
		// Might want to do something special??
		// TODO: Create startPattern / pattern renderer class and pattern selector class.
		rescheduleLightsCallbackMain();
	}
	
	return;
}

// Broadcasts current pattern to others on the mesh.
void broadcastPattern() {
	// If already sent or not playing yet, then don't broadcast
	if (sentAlready || !patternRunner.PatternActive(myPatternId)) return;

	SerializedNodeData currentPattern(myPatternId);
	String msg = currentPattern.toString();

	// send message to other jacket
	Serial.printf("%u: SENDING:: my-pattern-id: %i. Msg: %s\n", millis(), myPatternId, msg);
	// TODO: Also send hue!
	// TODO: enhance message to include time remaining? -- However this usually sends with maximum time remaining
	gangMesh.sendBroadcast(msg);
	sentAlready = true;
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
		patternRunner.StartPattern(patternIdx);
	} else {
		// if incoming characters do not correspond to pattern ID
		// then the other device must be running wire protocol v1.
		// Therefore set set inbound pattern color
		Serial.printf("%u: Message: %c. PatternChar out of bounds. Activating legacy handling to set inbound hue\n",
		              millis(), inChar);
		legacy_inbound_hue = inChar;
	}
}

void receiveMeshMsg(uint32_t nodeName, SharedNodeData nodeData){
	
	int patternIdx = nodeData.nodePattern;

	if (patternIdx >= 0) {
		// We have a valid pattern, start the corresponding pattern
		Serial.printf("%u: Node data for node %u deserialized to pattern %i. Starting that pattern.\n",
		              millis(), nodeName, patternIdx);
		patternRunner.StartPattern(patternIdx);
	} else {
		//Note: this fn should not get called
		Serial.printf("%u: Node data for node %u deserialized to invalid pattern.\n", millis(), nodeName);
		

		// if incoming characters do not correspond to pattern ID
		// then the other device must be running wire protocol v1.
		// Therefore set set inbound pattern color
					  
		// Serial.printf("%u: Message: %c. PatternChar out of bounds. Activating legacy handling to set inbound hue\n",
		//               millis(), inChar);
		// legacy_inbound_hue = inChar;
	}
}

void rescheduleLightsCallbackMain() {
	auto& allNodes = gangMesh.getNodeList();
	auto nodeCount = allNodes.size();
	
	Serial.printf("%u: *Pattern Scheduler* Triggered reschedule with %u nodes\n", millis(), nodeCount);
	
	unsigned long myMicros = micros();
	uint32_t meshMicros = gangMesh.mesh.getNodeTime();
	int meshDiff = meshMicros - myMicros;

	Serial.printf("%u: Pattern Scheduler: MyMicros: %u MeshMicros: %u (diff: %i)\n",
					millis(), myMicros, meshMicros, meshDiff);

	
	uint32_t durationMicros = PATTERN_DURATION * 1000;
	
	uint32_t totalMicros = durationMicros * nodeCount;
	
	uint32_t intervalPortion = meshMicros % totalMicros;
	uint32_t nextTime = meshMicros + (totalMicros - intervalPortion);

	Serial.printf("%u: Pattern Scheduler: Currently %u into pattern repeat. Next round time: %u\n",
					millis(), intervalPortion, nextTime);
					
	size_t nodeIdx = 0;
	for (auto& nodeId : allNodes) {
		uint32_t nodeStartTime = nextTime + durationMicros * nodeIdx;
		
		SharedNodeData nodeData;
		if(gangMesh.mesh.getNodeId() == nodeId){
			//Self, re-generate own pattern
			nodeData = SharedNodeData(getChosenPattern());
		} else {
			//Other, lookup
			nodeData = gangMesh.m_nodeData[nodeId];
		}
		 
		Serial.printf("%u: Pattern Scheduler: Should start node %u's pattern %i at mesh time %u\n",
					millis(), nodeId, nodeData.nodePattern, nodeStartTime);
		//TODO: actually trigger pattern!
		nodeIdx++;
	}
	
	sentAlready = false;	
}
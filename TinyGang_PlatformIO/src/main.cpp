/*
   Tiny Gang is an open source project to include multiple jacket wearers
   the base code is developed by @mpinner and @adellelin for two jacket
   interaction and is to be expanded for multiple players

   Enhancements made by Michael Walker to support PlatformIO IDE, and
   enable deterministic scheduling for larger meshes.
*/

#include <Arduino.h>
#include "WiFi.h"

// *** LOOKING TO CHANGE WHAT PIN YOU HAVE ATTACHED? ***
// *** SEE UserConfig.h ***
#include "UserConfig.h"

// Note: for these two, must include FastLED and elapsedMillis libraries in your Audrino IDE/platformIO
#define FASTLED_ALLOW_INTERRUPTS 0 //Should disable handing interrupts after sending each pixel, disable interrupts for entire length of transmission
#define FASTLED_INTERRUPT_RETRY_COUNT 1 //Not sure what this does 
#define FASTLED_RMT_MAX_CHANNELS 1 //Reserves less RMT channels
 
// #define FASTLED_ESP32_I2S true //Try to use I2S instead of RMT. Fails to compile.
// #define FASTLED_RMT_BUILTIN_DRIVER 1 //Disables all output...idk
#include <FastLED.h>
#include <elapsedMillis.h>

#include "GangMesh.h"
#include "PatternRunner.h"

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
const uint32_t PATTERN_DURATION_MS = 4000; //Length each pattern plays. Note: If this isn't the same on all nodes then they won't sync up!
const unsigned PATTERN_OVERLAP_MS = 300;

const uint32_t PATTERN_DURATION_MICROS = PATTERN_DURATION_MS * 1000;
const unsigned PATTERN_OVERLAP_MICROS = PATTERN_OVERLAP_MS * 1000;

PatternRunner<NUM_LEDS> patternRunner;

// Pattern display
CRGB render_leds[NUM_LEDS];

// Pattern Broadcasting
void broadcastPattern();
uint32_t BROADCAST_REPEAT_TIME = PATTERN_DURATION_MS + PATTERN_OVERLAP_MS * 2;  // Rebroadcast info timer
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

elapsedMillis rescheduleTimerFailsafe;


// #define SIZE 232879
// char dummy[SIZE] = {'a'};


// This failed workaround comes from:
// Issue: https://github.com/FastLED/FastLED/issues/849
// Code: https://github.com/stnkl/FastLEDHub/commit/54bbaf1d12c89977501e1dea95f9347ae0a954ed
// //Define custom controller timings
// template <uint8_t DATA_PIN, EOrder GRB_ORDER = GRB, int WAIT_TIME = 5>
// class WS2812Controller800Khz_noflicker : public ClocklessController<DATA_PIN, C_NS(250), C_NS(625), C_NS(375), GRB_ORDER, 0, false, WAIT_TIME> {};
// template<uint8_t DATA_PIN, EOrder GRB_ORDER>
// class WS2812B_noflicker : public WS2812Controller800Khz_noflicker<DATA_PIN, RGB_ORDER> {};
//
// Then in setup init with:
// FastLED.addLeds<WS2812B_noflicker, PIN_LED_STRIP_1>(render_leds, NUM_LEDS);


void setup() {
	//What's up with all the delay calls? The ESP32C3 is weird AF. thats why.
	
	const unsigned long BAUD = 921600;
	// put your setup code here, to run once:
	Serial.begin(BAUD);
	delay(10);
	Serial.println("\n\n******TinyGang Reset******");
	delay(10);
	Serial.println(__FILE__);
	delay(10);
	Serial.println(__DATE__);
	delay(10);

	// dummy[SIZE-1] = '\n';
    // if(dummy[0] == 'a') Serial.println("Hello, bloated world");

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

	// delay(150);
	pinMode(PIN_LED_STATUS, OUTPUT);
	
	delay(150);
	//Setup FastLED
	// TODO: Better user configurable LED setup
	FastLED.addLeds<CHIPSET, PIN_LED_STRIP_1, COLOR_ORDER>(render_leds, NUM_LEDS);//.setCorrection(TypicalLEDStrip);
	// FastLED.setBrightness(LED_BRIGHTNESS);
	// FastLED.setMaxPowerInVoltsAndMilliamps(MAX_VOLTS, MAX_MILLIAMPS);
	
	FastLED.clear();

	std::fill(std::begin(render_leds), std::end(render_leds), CRGB::Black);
	FastLED.show();
  
	
	// FastLED.setDither(0);
	delay(150);
	
	//Setup pattern runner
	patternRunner.setup();
	patternRunner.SetPatternSlot(0, m_ownNodeData, 0, PATTERN_DURATION_MS);
	rescheduleTimerFailsafe = PATTERN_DURATION_MS*2;

	delay(150);
	//Setup wifi mesh
	// gangMesh.setup();
	
	WiFi.softAP("junk", "123456");
	
	
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

// unsigned main_loop_ct = 0;
void otherLoop(); //fwd declare

void loop() {
// Only btn PUSHBTN needs a loop callback.
#if defined(PATTERN_SELECT_PUSHBTN)
	cyclePatternBtn.tick();
#endif

	// handleKeyboardInput();
	// checkPatternTimer();
	// broadcastPattern();
	// patternRunner.updateLedColors();  // TODO: send legacy_inbound_hue again?
	// show();
	// gangMesh.update();

	// Serial.printf("%u: Loop? %u\n", millis(), main_loop_ct++);

	// delay(10);
	
	otherLoop();
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
		// Scratch code for only doing the middle
		// case RENDER_MIDDLE:
		// 	float middlePortion = .66f;
		// 	float halfMid = (1-middlePortion) / 2;
		// 	int startLed = (int) (halfMid * NUM_LEDS);
		// 	int endLed = (int) ((1-halfMid) * NUM_LEDS);

		// 	for (int i = startLed; i < endLed; i++) {
		// 		render_leds[i] = patternRunner.m_outBuffer[i];
		// 	}
		// 	break;
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

	if(patternRunner.GetNumActivePatterns() == 0 && rescheduleTimerFailsafe > PATTERN_DURATION_MS/2) {
		Serial.printf("%u: No patterns active!\n", millis());
		rescheduleLightsCallbackMain();
		rescheduleTimerFailsafe = 0;
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
		patternRunner.StartPattern(m_ownNodeIndex, PATTERN_DURATION_MS);
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
	int meshDiff = meshMicros - myMicros; //TODO: Rollover protection...can just change type i think?

	// Serial.printf("%u: Pattern Scheduler: MyMicros: %u MeshMicros: %u (diff: %i)\n",
	// 				millis(), myMicros, meshMicros, meshDiff);

	uint32_t totalRepeatMicros = PATTERN_DURATION_MICROS * nodeCount;
	uint32_t intervalPortion = meshMicros % totalRepeatMicros;

	//TODO: check for rollover safety
	uint32_t lastStartTime = meshMicros - intervalPortion;
	uint32_t nextStartTime = lastStartTime + totalRepeatMicros;

	// Serial.printf("%u [%u]: Pattern Scheduler: Currently %u into pattern repeat. Current round lasts from [%u - %u] mesh. [%u - %u] local \n",
	// 				millis(), micros(), intervalPortion, lastStartTime, nextStartTime, gangMesh.TimeMeshToLocal(lastStartTime), gangMesh.TimeMeshToLocal(nextStartTime));

	patternRunner.StopAllPatterns();
	size_t nodeIdx = 0;
	for (auto& nodeId : allNodes) {

		//Get Node data
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

		//Figure out schedule
		uint32_t nodeStartTime = lastStartTime + PATTERN_DURATION_MICROS * nodeIdx - PATTERN_OVERLAP_MICROS;
		uint32_t nodeEndTime = lastStartTime + PATTERN_DURATION_MICROS * (1+nodeIdx) + PATTERN_OVERLAP_MICROS;

		// Serial.printf("%u: (Node %u Id: %u ) Pattern Scheduler: Slot: [%u-%u]mesh [%u-%u]local\n",
		// 			millis(), nodeIdx, nodeId, nodeStartTime, nodeEndTime, gangMesh.TimeMeshToLocal(nodeStartTime), gangMesh.TimeMeshToLocal(nodeEndTime));

		//TODO: make rollover safe -- needs to compare duration not current time.
		if(nodeEndTime < meshMicros) {
			//push it to the next one instead
			nodeStartTime = nextStartTime + PATTERN_DURATION_MICROS * nodeIdx - PATTERN_OVERLAP_MICROS;
			nodeEndTime   = nextStartTime + PATTERN_DURATION_MICROS * (1+nodeIdx) + PATTERN_OVERLAP_MICROS;

			// Serial.printf("%u: (Node %u Id: %u ) Pattern Scheduler: Shifting since slot expired. New Slot: [%u-%u]mesh [%u-%u]local\n",
			// 		millis(), nodeIdx, nodeId, nodeStartTime, nodeEndTime, gangMesh.TimeMeshToLocal(nodeStartTime), gangMesh.TimeMeshToLocal(nodeEndTime));
		}

		//Convert to local, add overlap
		uint32_t nodeStartLocal = gangMesh.TimeMeshToLocal(nodeStartTime);
		//todo: overlap rollover?

		//Actually update info and trigger pattern
		//TODO: convert to passing ptrs all through code.
		patternRunner.SetPatternSlot(nodeIdx, *whichNodeData, nodeStartLocal, PATTERN_DURATION_MICROS + 2*PATTERN_OVERLAP_MICROS);
		nodeIdx++;
	}//End loop over nodes

} //end rescheduleLightsCallbackMain()


///Debug code -- alternate light code


// Define the array of leds
const size_t NUM_COLORS = 5;
CRGB colorList[NUM_COLORS] = {CRGB::Red, CRGB::Purple, CRGB::Green, CRGB::DeepPink, CRGB::RoyalBlue};
int colorIdx = 0;


const uint32_t innerDelay = 50;
const uint32_t outterDelay = 100;
unsigned int loopCt = 0;

bool onboardOn = true;

void slowColorLeds() {
  //Start next transition
  colorIdx = (colorIdx + 1) % NUM_COLORS;
  CRGB nextColor = colorList[colorIdx];

  int stripMid = NUM_LEDS/2; // 72/2=36
  for (int i = 0; i <= stripMid; i++)
  {
    int hi = std::min(stripMid + i, (int) NUM_LEDS-1);
    int lo = std::max(stripMid - i,  0);

    // Serial.printf("%u | i= %i. hi=%i, lo=%i \n", loopCt, i, hi, lo);

    render_leds[hi] = nextColor;
    render_leds[lo] = nextColor;

  // for (int i = 0; i < NUM_LEDS; i++)
  // {
  //   int hi = i;
  //   leds[hi] = nextColor;
    delay(innerDelay);
    FastLED.show();
  }
}
void otherLoop() {

  loopCt++;

    Serial.printf("%u | loop %b\n", loopCt, onboardOn);


  if(loopCt < 20){
    delay(50);
    onboardOn = !onboardOn;
    digitalWrite(PIN_LED_STATUS, onboardOn ? HIGH : LOW);
    // mpu_loop();

    return;
  }


  //Flip the onboard
  onboardOn = !onboardOn;
  digitalWrite(PIN_LED_STATUS, onboardOn ? HIGH : LOW);

  // mpu_loop();
  // colorLedByPitch();
//   slowColorLeds();

	std::fill(std::begin(render_leds), std::end(render_leds), CRGB(0,0,5)); //CRGB(0,0,10)
	FastLED.show();
	
  // delay(10);
  // FastLED.show();
  //I sleeps
  delay(outterDelay);
}



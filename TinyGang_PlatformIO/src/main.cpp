/*
   Tiny Gang is an open source project to include multiple jacket wearers
   the base code is developed by @mpinner and @adellelin for two jacket
   interaction and is to be expanded for multiple players
*/

#include <Arduino.h>
//Note: for these two, must include FastLED and elapsedMillis libraries in your Audrino IDE/platformIO
#include <FastLED.h>
#include <elapsedMillis.h>

#include "PatternRunner.h"
#include "GangMesh.h"

// *** LOOKING TO CHANGE WHAT PIN YOU HAVE ATTACHED? ***
// *** SEE UserConfig.h ***
#include "UserConfig.h"

//Some State
int legacy_inbound_hue = 229;  // incoming char sets color

//Pattern Choosing
int myPatternId = DEFAULT_PATTERN;

#if defined(PATTERN_SELECT_PUSHBTN)
	//Pushbutton state
    // Setup a new OneButton on pin A1.
    OneButton cyclePatternBtn(PUSHBUTTON_PIN, true);
    int chosenPattern = myPatternId;  // Current pattern state
#endif


//Pattern running info
const float PATTERN_DURATION = 4000;
float repeatDurationMs = 8000;  // Will display an animation every this often.
PatternRunner<NUM_LEDS> patternRunner(PATTERN_DURATION);

//Pattern display
CRGB render_leds[NUM_LEDS];

//Pattern Broadcasting
char patternCommand[] = {
	'q', 'a', 'z',
	'w', 's', 'x',
	'e', 'd', 'c',
	'r', 'f', 'v'};
void broadcastPattern();
boolean sentAlready = false;    // State to track if we broadcasted our pattern yet

//Pattern receiving
void receiveMeshMsg(char inChar);

// More Declarations
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
		receiveMeshMsg(inChar);
	}

	checkPatternTimer();
	broadcastPattern();
	patternRunner.updateLedColors(); //TODO: send legacy_inbound_hue again?
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

	if (patternChanged) {
		Serial.printf("Pattern switch detected from %i to %i\n", oldPatternId, myPatternId);
		// Might want to do something special??
		// TODO: Create startPattern / pattern renderer class and pattern selector class.
	}

	// Automatically start own pattern every repeatDuration
	if (patternRunner.PatternElapsed(myPatternId) > repeatDurationMs) {
		Serial.printf("%u: repeatDurationMs has elapsed, restarting self pattern: %i\n", millis(), myPatternId);
		patternRunner.StartPattern(myPatternId);
		sentAlready = false;
	}
	return;
}


// Broadcasts current pattern to others on the mesh.
void broadcastPattern() {
	// If already sent or not playing yet, then don't broadcast
	if (sentAlready || patternRunner.PatternElapsed(myPatternId) <= 0) return;

	// send message to other jacket
	Serial.printf("%u: SENDING::", millis());
	Serial.print(" my-pattern-id:");
	Serial.print(myPatternId);
	// Serial1.print((char)PATTERN_HUE[myPatternId]); // sending color to IMU jackets
	Serial.print(" cmd:");
	Serial.println(patternCommand[myPatternId]);
	// Serial1.print(patternCommand[myPatternId]);

	String msg = (String)patternCommand[myPatternId];
	// TODO: enhance message to include time remaining?
	gangMesh.sendBroadcast(msg);
	sentAlready = true;
}

void receiveMeshMsg(char inChar) {
	bool patternFound = false;

	// read the incoming character and check it's index,
	// start the corresponding pattern ID by setting its timer to 0
	for (int i = 0; i < std::min(PATTERNS_COUNT, sizeof(patternCommand)); i++) {
		if (inChar == patternCommand[i]) {
			patternFound = true;
			Serial.printf("%u: Message: %c. Corresponds to pattern %i. Resetting that pattern. PatternCount: %u\n",
			              millis(), inChar, i, PATTERNS_COUNT);
			patternRunner.StartPattern(i);
		}
	}

	// if incoming characters do not correspond to pattern ID
	// then the other device must be running wire protocol v1.
	// Therefore set set inbound pattern color
	if (!patternFound) {
		Serial.printf("%u: Message: %c. Activating legacy handling to set inbound hue\n",
			              millis(), inChar);
		legacy_inbound_hue = inChar;
	}
}
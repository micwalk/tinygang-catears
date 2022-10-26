/*
   Tiny Gang is an open source project to include multiple jacket wearers
   the base code is developed by @mpinner and @adellelin for two jacket
   interaction and is to be expanded for multiple players
*/

#include <Arduino.h>
#include <FastLED.h>


const unsigned char LED_PIN = 4; //14 on lhs board, 4 on rhs
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define NUM_LEDS    72
const uint8_t MAX_VOLTS = 5;
const uint32_t MAX_MILLIAMPS = 500;

#include <elapsedMillis.h>

#include "patterns/RainbowSparkle.h"
#include "patterns/WhiteTrace.h"
#include "patterns/BookendTrace.h"
#include "patterns/Twinkler.h"
#include "patterns/BookendFlip.h"
#include "patterns/BassShader.h"
#include "patterns/BodyTwinkler.h"

#define JACKET1 1 // use this when other jacket is HR, otherwise 0
#define JACKET2 0 // we used JACKET2 0 for white jacket

#define WHOSE_HARDWARE JACKET1 // little black jacet with one long strip split to two connectors


int myPatternId = 0;

Pattern* patterns[] = {
  new BodyTwinkler(), // red yellow sparkle short
  new BassShader(), // pattern individually triggered
  new RainbowSparkle(), // more pale pink and blue
  new WhiteTrace(),
  new BookendTrace(),
  new Twinkler()
};
const size_t PATTERNS_COUNT = (sizeof(patterns)) / 4;

int inboundHue = 229; // incoming char sets color
int ledHue[] = { 0, 20, 255, 229, 229, 200};
// 120 was cyan
// 229 pink
// 22 orange
// 200 lilac
// 'a' green

elapsedMillis ellapseTimeMs[PATTERNS_COUNT];

const float PATTERN_DURATION = 4000;
float durationMs = PATTERN_DURATION;

float repeatDurationMs = 8000; //Will display an animation every this often.
boolean sentAlready = false; // send message boolean

elapsedMillis sendEllapseMs;

//Per pattern Leds?
CRGB led[PATTERNS_COUNT][NUM_LEDS]; //

// Define the array of leds
CRGB leds_right[NUM_LEDS];
CRGB leds_left[NUM_LEDS];

char patternCommand[] = {
  'q', 'a', 'z',
  'w', 's', 'x',
  'e', 'd', 'c',
  'r', 'f', 'v'
};

//****************
//User pattern selection
//Three methods supported: push button, dip switch, or const
//Uncomment the define relevant to your hardware
#define PATTERN_SELECT_PUSHBTN
// #define PATTERN_SELECT_DIPSWITCH
//#define PATTERN_SELECT_CONST

#if defined(PATTERN_SELECT_PUSHBTN)
  // PUSH BUTTON Config
  #include "OneButton.h"
  const uint8_t PUSHBUTTON_PIN = 5;
  // Setup a new OneButton on pin A1.  
  void onPatternChangeClick();
  OneButton cyclePatternBtn(PUSHBUTTON_PIN, true);
  int chosenPattern = 3; //Current pattern state
#elif defined(PATTERN_SELECT_DIPSWITCH)
  // DIP SWITCHES Config
  const int dipswitch_pins[] = {2, 15};
#elif defined(PATTERN_SELECT_CONST)
  const int USER_PATTERN = 0;
#endif






//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <painlessMesh.h>

// some gpio pin that is connected to an LED to incicate wifi status. change to the right number of your LED.
#define   STATUS_LED      LED_BUILTIN       // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Prototypes For painlessMesh
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

//More Declarations
void checkPatternTimer();
void broadcastPattern();
void receiveMeshMsg(char inChar);
void setupPainlessMesh();
int getChosenPattern();
void updateLedColors();
void show();

//Painless Mesh Setup
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

//Tasks on painless mesh
void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

void setup() {

  const unsigned long BAUD = 921600;
  // put your setup code here, to run once:
  Serial.begin(BAUD);
  Serial.println("\n\n******TinyGang Reset******");
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  
  #if defined(PATTERN_SELECT_PUSHBTN)
    //Setup pushbutton
    cyclePatternBtn.attachClick(onPatternChangeClick);
    // pinMode(PUSHBUTTON_PIN, INPUT);
  #elif defined(PATTERN_SELECT_DIPSWITCH)
    // setup dipswitch
    for (int i = 0 ; i < 2; i++) {
      pinMode(dipswitch_pins[i], INPUT_PULLUP);
    }
  #endif
  //No setup needed for const

  //TODO: Better user configurable LED setup
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds_right, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(50);
  FastLED.setMaxPowerInVoltsAndMilliamps(MAX_VOLTS, MAX_MILLIAMPS);

  FastLED.setDither( 0 );

  // For adding a switch to have a pattern that is always on vs regular
  for (int i = 0; i < PATTERNS_COUNT; i++) {
    for (int j = 0; j < NUM_LEDS; j ++) {
      //led[i][j] = CHSV(100,100,100);
      led[i][j] = CHSV(255 / NUM_LEDS * j, 255 / PATTERNS_COUNT * i, 255);
    }
  }

  setupPainlessMesh();
}

void loop() {  
  //Only btn PUSHBTN needs a loop callback.
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
  updateLedColors();
  show();
  delay(10);

  // painless mesh
  mesh.update();
  digitalWrite(STATUS_LED, !onFlag);
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
  return 0; //Default..
#elif defined(PATTERN_SELECT_CONST)
  return USER_PATTERN;
#endif
}

//Broadcasts current pattern to others on the mesh.
void broadcastPattern() {
  //If already sent or not playing yet, then don't broadcast
  if(sentAlready || ellapseTimeMs[myPatternId] <= 0) return;

  // send message to other jacket
  Serial.printf("%u: SENDING::", millis());
  Serial.print(" my-pattern-id:");
  Serial.print(myPatternId);
  // Serial1.print((char)ledHue[myPatternId]); // sending color to IMU jackets
  Serial.print(" cmd:");
  Serial.println(patternCommand[myPatternId]);
  // Serial1.print(patternCommand[myPatternId]);
  
  String msg = (String)patternCommand[myPatternId];
  //TODO: enhance message to include time remaining?
  mesh.sendBroadcast(msg);
  sentAlready = true;
}

void updateLedColors() {
  for (int i = 0; i < PATTERNS_COUNT; i++) {
    for (int j = 0; j < NUM_LEDS; j ++) {
    // check timers for all patterns
      if (ellapseTimeMs[i] > durationMs ) {
        //led[i][j] = CHSV(100,100,100);
        led[i][j] = 0; // turn all LED to black once time hits
      } else {
        float position = (float)j / (float)NUM_LEDS;
        float remaining = 1.0 - ellapseTimeMs[i] / durationMs;
        // set color to be the incoming message color
        int color = inboundHue;
        // if own pattern, then lookup color array
        if (i == myPatternId) {
          color = ledHue[i];
        }
        led[0][j] = patterns[i]->paintLed(position, remaining, led[0][j], color);
        //Serial.println(ledHue[i]);
      }
    }
  }
} 

void show() {

  // jacket 1 and 2 have different mappings for their led strips
  // jacket 1 is mirrored, jacket 2 is mirrored
  int half_leds = (NUM_LEDS / 2);

  switch (WHOSE_HARDWARE) {
    case JACKET1 :
      for (int i = 0; i < half_leds; i ++) {
        leds_left[i] = led[0][i];
        leds_right[i] = led[0][i];
      }
      for (int i = 0; i < half_leds; i ++) {
        leds_left[i + half_leds] = led[0][half_leds - i];
        leds_right[i + half_leds] = led[0][half_leds - i];
      }
      break;
    case JACKET2:
      for (int i = 0; i < NUM_LEDS; i ++) {
        leds_left[i] = led[0][i];
        leds_right[i] = led[0][i];
      }
      break;
  }

  FastLED.show();

  return;
}

void checkPatternTimer() {
  // sets the timers for my pattern,  when timer goes to 0
  // set the ellapseTimeMs to 0, which triggers 
  // all actions
  
  auto oldPatternId = myPatternId;
  myPatternId = getChosenPattern();
  
  bool patternChanged = oldPatternId != myPatternId;
  
  if(patternChanged) {
    Serial.printf("Pattern switch detected from %i to %i\n", oldPatternId, myPatternId);
    //Might want to do something special?? 
    //TODO: Create startPattern / pattern renderer class and pattern selector class. 
  }

  // autofire over repeat duration
  if (ellapseTimeMs[myPatternId] > repeatDurationMs) {
    //Serial.printf("%u: Repeating my pattern: %i\n", millis(), myPatternId);
    // pattern fires when ellapseTimeMs of the pattern is 0
    Serial.printf("%u: Starting own pattern: %i\n", millis(), myPatternId);
    ellapseTimeMs[myPatternId] = 0;
    durationMs = PATTERN_DURATION;
    sentAlready = false;
  }
  return;
}

// painless mesh

void receiveMeshMsg(char inChar) {
  bool patternFound = false;
  
  // read the incoming character and check it's index,
  // start the corresponding pattern ID by setting its timer to 0
  for (int i = 0; i < std::min(PATTERNS_COUNT, sizeof(patternCommand)); i++) {
    if (inChar == patternCommand[i]) {
      Serial.printf("%u: Message: %c. Corresponds to pattern %i. Resetting that pattern. PatternCount: %u\n", 
      millis(), inChar, i, PATTERNS_COUNT);
      Serial.printf("%u: Starting other pattern: %i\n", millis(), i);
      ellapseTimeMs[i] = 0;
    }
  }
  
  // if incoming characters do not correspond to pattern ID 
  // then set inbound color (jacket color)
  if( !patternFound) {
    inboundHue = inChar;
  }
}

void setupPainlessMesh() {
  pinMode(STATUS_LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off, else switch on
    onFlag = !onFlag;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));
}

void sendMessage() {
  
  // String msg = "Hello from node ";
  // msg += mesh.getNodeId();
  // msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  //Serial.printf("Sending message: %s\n", msg.c_str());
  // mesh.sendBroadcast(msg);

  if (calc_delay) {
    Serial.println("%u: TaskSendMessage launching delay calculation");
    
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }


  taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("%u: painlessMesh: Received from %u msg=%s\n", millis(), from, msg.c_str());
  receiveMeshMsg(msg[0]);
}

//Resets the blink task.
void resetBlinkTask(){
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("%u: --> painlessMesh: New Connection, nodeId = %u\n", millis(), nodeId);
  Serial.printf("%u: --> painlessMesh: New Connection, %s\n", millis(), mesh.subConnectionJson(true).c_str());
  
  resetBlinkTask();
}

void changedConnectionCallback() {
  Serial.printf("painlessMesh: Changed connections\n");
  resetBlinkTask();
  
  nodes = mesh.getNodeList();
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}

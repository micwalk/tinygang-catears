#ifndef __USERCONFIG__H__
#define __USERCONFIG__H__
#include <Arduino.h>

//***********************************************************
// Board Config

//Must define platform in order for the ustd library to work.
#define __ESP32__

//***********************************************************
// LED CONFIG

#ifdef  PLATFORM_XIAO_C3
// STATUS LED -- some gpio pin that is connected to an LED to incicate wifi status. change to the right number of your LED.
const uint8_t PIN_LED_STATUS = D0;
// LED STRIP --What pin is your LED strip attached to?
const unsigned char PIN_LED_STRIP_1 = D1;
#endif  //PLATFORM_XIAO_C3

#ifdef  PLATFORM_UPESY
// STATUS LED -- some gpio pin that is connected to an LED to incicate wifi status. change to the right number of your LED.
const uint8_t PIN_LED_STATUS = LED_BUILTIN; 
// LED STRIP -- What pin is your LED strip attached to?
const unsigned char PIN_LED_STRIP_1 = 14;  // 14 on lhs board, 4 on rhs
#endif  //PLATFORM_UPESY


// How many LEDs are in it?
const uint32_t NUM_LEDS = 25;//93;//74

// What type of LED strip?
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
// Power limiting settings
const uint8_t LED_BRIGHTNESS = 10; //Out of 255
const uint8_t MAX_VOLTS = 5;
const uint32_t MAX_MILLIAMPS = 500;

//Do you want your LED strip to be mirrored down the middle?
enum RenderType {
	RENDER_NORMAL,  //Legacy Jacket1
	RENDER_MIRRORED //Legacy Jacket2
};

//Change this to normal or mirrored
const RenderType RENDER_TYPE = RENDER_NORMAL;

//***********************************************************
// PATTERN SELECTOR CONFIG
//
// User pattern selection
// Three methods supported: push button, dip switch, or const

//Default pattern
const int DEFAULT_PATTERN = 3;


// Uncomment the define relevant to your hardware
#define PATTERN_SELECT_PUSHBTN
// #define PATTERN_SELECT_DIPSWITCH
//#define PATTERN_SELECT_CONST

//Now define constants / pins relative to you selection above.
#if defined(PATTERN_SELECT_PUSHBTN)
    // PUSH BUTTON Config
    #include "OneButton.h"
#ifdef  PLATFORM_XIAO_C3
const uint8_t PUSHBUTTON_PIN = D2;
#endif
#ifdef  PLATFORM_UPESY
const uint8_t PUSHBUTTON_PIN = 12;
#endif
    
    void onPatternChangeClick();
#elif defined(PATTERN_SELECT_DIPSWITCH)
    // DIP SWITCHES Config
    const int dipswitch_pins[] = {2, 15};
#elif defined(PATTERN_SELECT_CONST)
    // No switch? no problem! Just set your constant pattern here:
    const int USER_PATTERN = 0;
#endif

//***********************************************************
// WIFI MESH CONFIG
//
// Make sure to set the SSID and Password the same as your TinyGang!
// Port doesn't really matter...
#define MESH_SSID "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

//Unfortunately we have to set a limit because we're statically allocating memory for everyone.
const int MAX_PEERS = 100;


#endif  //!__USERCONFIG__H__


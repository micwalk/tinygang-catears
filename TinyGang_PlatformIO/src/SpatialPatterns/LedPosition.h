#ifndef __LEDPOSITION__H__
#define __LEDPOSITION__H__

enum class LedSection {
    BAND,
    EAR_LEFT,
    EAR_RIGHT
};

struct vec2 {
    float x;
    float y;
};

struct polar2 {
    float angle;
    float radius;
};

struct LedPosition {
    int idBlender; // AKA array index of LED_POSITIONS -- Order Blender happend to iterate over. Not Too Helpful.
    int idWire; //Wire position, use raw for a chase.
    LedSection section; //What section, ears or band
    int idSection; //Position within section, use for cooler chaser when properly configured.
    vec2 position;
    polar2 relCenter;
    polar2 relLeft;
    polar2 relRight;
};

#endif  //!__LEDPOSITION__H__
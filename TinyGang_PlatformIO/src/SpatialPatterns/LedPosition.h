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
    int idBlender; // AKA array index of LED_POSITIONS -- Order Blender happend to iterate over
    int idWire;
    LedSection section;
    int idSection;
    vec2 position;
    polar2 relCenter;
    polar2 relLeft;
    polar2 relRight;
};

#endif  //!__LEDPOSITION__H__
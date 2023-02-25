#ifndef __LED_ARRANGEMENT__
#define __LED_ARRANGEMENT__

#include "Arduino.h"
#include "LedPosition.h"

//first set xy, second theta, radius.
// left ear center: 66.17855834960938, 55.993038177490234, 49.76576258488397, 86.70529846612996
// right ear center: -66.18331146240234, 55.990379333496094, -49.76913324198114, 86.74013417014348

class LedContext {
    public:

    const vec2 LeftEarCenter = {66.17855834960938, 55.993038177490234};
    const vec2 RightEarCenter = {-66.18331146240234, 55.990379333496094};

    unsigned int Count;
    LedPosition* Positions;

    LedContext(unsigned int count, LedPosition* positions) {
        Count = count;
        Positions = Positions;
    }

    float RelativePosition(unsigned ledIdx) {
        return (float) ledIdx / (float) Count;
    }

    const LedPosition& PositionInfo(unsigned ledIdx) {
        return Positions[ledIdx];
    }


    //section count?
};

#endif // __LED_ARRANGEMENT__

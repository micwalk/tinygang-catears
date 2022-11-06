# TinyGang PlatformIO User Documentation
## Configuring to your hardware
Open up `UserConfig.h` and read the comments there and configure what pins your leds and switches are connected to as well as the mesh wifi connection information.

## Flashing to your device using PlatformIO
1. First you must ensure you have the right USB to UART chipset driver installed for your development board. This lets your computer recognize the device in the first place. If you bought the same board as us, the chipset drivers can be found here: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
If you aren't sure / its some random unbranded board from Alibaba, read the chip on your dev board with a magnifying glass. Look for something that says "SILABS CP2102" google that, and if the results say the magic words UART, downlaod drivers for that.
2. Install Visual Studio Code - https://code.visualstudio.com/
3. Install the "PlatformIO IDE" extension from within VS Code on the Extensions tab.
4. Download this repository.
5. From the PlatformIO home page (alien head icon) open a project, select the "TinyGang_PlatformIO" folder. Note to NOT pick the base repository folder (just plain TinyGang)
6. Plug your board into your computer USB
7. Make sure to perform the "Configuring to your hardware" step above!
8. From the PlatformIO sidebar tab you should now see several options under "Project Tasks". Select the "Upload and Monitor" Task under "upesy_wroom" -- This should connect to your device, upload the firmware, then display a log of the device's execution. If things start flashing and messages start scrolling, you're done!

## Hardware Ref
### Boards
I purchased this set of 3 boards: https://www.amazon.com/dp/B09GK74F7N
Seems to be right pinout: https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/

![Pinout Image](https://circuits4you.com/wp-content/uploads/2018/12/ESP32-Pinout.jpg)

[Pinout Image Repo Link for posterity](../img/ESP32-Pinout.jpg)


Also purchased Seeed Studio XIAO ESP32C3: https://www.amazon.com/dp/B0B94JZ2YF
 -- This thing is TINY
### Sensors:
MPU 6050 Accelerometer + Gyroscope: https://www.amazon.com/dp/B00LP25V1A


### Michael Pinouts
Since the above devboard is kinda large for a breadboard, I have it attached with only the left side (as in pinouts, with usb port on bottom) accessible.

The following pins are attached
* VIN to breadboard + rail
* GND to breadboard - rail
* 100uf cap between breadboard +/- rails
* D12 - to pushbutton then to GND
* D14 - to 330 ohm resistor then to WS2812b strip

WS2812 is connected to breadboard power rails + d12. Amazingly this also works for powering them off of microusb as well as a breadboard power supply @ 5v

# TinyGang PlatformIO Code Documentation

## PlatformIO
*Why aren't you using the Audrino IDE??*
1. visual studio code is a great editor with code completion. PlatformIO is a near 1 click install using the built in package manager.
2. Better dependency management -- Dependencies are tracked on a per project basis instead of per ide. Dependencies are stored in platformio.ini, and will automatically download and resolve transative dependencies on a per project basis. The same platformio.ino also tracks monitor baud rate, chipset, board, etc.
3. More explicit C++ -- The Aurino `.ino` gets translated into a c++ file with minor modifications, but doens't have great tooling support.

## General Logic

### The Chosen Pattern
Each jacket wearer / gang member / cat chooses their **chosen pattern** using one of three methods defined in `UserConfig.h`:
1. A dip switch (original design, 4 options)
2. A single pushbutton that cycles through patterns
3. A constant called `USER_PATTERN` defined in `UserConfig.h`
The `getChosenPattern` returns the current pattern slection regardless of the above method.

### PatternRunner
The class `PatternRunner` keeps track of which patterns are playing. Multiple patterns can be active at a given time. When multiple patterns are active, the highest numbered pattern takes precidence.

The `PatternRunner` maintains a schedule of patterns managed by an array of `ScheduledPattern[]`. Currently it maintains the legacy logic of having as many schedule slots as there are patterns.

A `ScheduledPattern` contains info on the:
* NodeInfo to display
* start and stop times

This creates 3 implict states
1. Active -- Currently playing an animation. Recognized when current time is between start and end times.
2. Scheduled -- Current time is before star time
3. Done -- current time is after end time

The **chosen pattern** will always be scheduled.

### Pattern Blending: 
When multiple patterns are playing, they are all drawn to the same framebuffer in the order of the pattern id. The `PatternRunner` does not perform any blending itself, BUT it does pass the current value of the pixel into the next pattern. This pattern may choose to do something with this parial render, or overwrite it entirely. On several patterns, such as WhiteTrace, it will replace the pixels corresponding to the trace, but simply fade out other pixels. The scheduler maintains a small overlap between patterns (150ms by default). This looks better on some patterns than others.

### Mesh Pattern Scheduler & Sync: 
When you're on a mesh, everyone broadcasts when they are playing their pattern. When you receive a pattern broadcast, it is stored on your device.

The pattern scheduler works by sorting everyone's NodeId, then using the **mesh time** (separate from local time, provided by painlessMesh), schedules everyone's pattern to play for `PATTERN_DURATION` in order of NodeId. The start time is a simple modulo of `mesh time % (duration * node count)` Since everyone shares node id and mesh time, everyone derives an identical schedule! 

This schedule actually gets recomputed overly often, but it is deterministic and takes into account partial execution, so you don't even notice.

If no patterns are playing, a reschedule is forced. Might be smarter to have a concept of repeat time built in, but this works for now.

## Potential Improvements

1. Send Hue for user set hue
2. Send Duration for user set duration
3. Dynamic pattern precidence? Leader election?
4. Scaling time with lots of nodes?

## The Wire Protocol
I'm calling the format of primary intentionally broadcast messages between mesh nodes the wire protocol. The painlessMesh library itself has detailed documentation on automatically sent messages like time synchronization and latency detection.

### Wire Protocol V1 - Original TinyGang.ino
Two bytes are sent in sequence.
Byte 1: the chosen hue / color
Byte 2: the "pattern command", a single ASCII character that is tranlsated into a patternID
### Wire Protocol V2 - TinyGang_esp
Only one Byte is sent, corresponding to the "pattern command" as above.
Backwards compatability is semi preserved, where if the pattern command is unmatched, then this is instead interpreted as a hue.

### Wire Protocol V3 - TinyGang_PlatformIO aka GlitterCats
For now, attempting to maintain the same as protocol V2 but subject to change! All pattern scheduler changes use the same wire protocol and since the own node broadcast time is the same, old nodes sorta stay in sync.
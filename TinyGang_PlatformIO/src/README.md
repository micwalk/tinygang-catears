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

## Hardware List

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

Primary pattern state is managed through a varaible `m_ellapseTimeMs[pattern_id]`. This is a measure of how long each animation has been playing.

This creates 3 implict states
1. Active -- Currently playing an animation. Recognized when ellapseTimeMs `ellapseTimeMs[i] <= durationMs`. Queryable via `PatternRunner::PatternActive(int patternId)`
2. Timeout -- Done playing an animation, but before elligible for replay.In this state when `ellapseTimeMs[i] >= durationMs && ellapseTimeMs[i] <= repeatDurationMs`
3. Ready -- Inactive but waiting to be triggered. This when NOT the above to, AKA: `ellapseTimeMs[i] > repeatDurationMs`

The **chosen pattern** will always trigger automatically when they reach the Ready state. Essentially starting to play again every `repeatDurationMs` millis.
When you are on a **mesh**, each jacket will broadcast what pattern it is playing. When you receive a broadcast for a pattern, the pattern is activated by resetting its elapsedTime to 0.

### Pattern Blending: 
When multiple patterns are playing, they are all drawn to the same framebuffer in the order of the pattern id. The `PatternRunner` does not perform any blending itself, BUT it does pass the current value of the pixel into the next pattern. This pattern may choose to do something with this parial render, or overwrite it entirely. On several patterns, such as WhiteTrace, it will replace the pixels corresponding to the trace, but simply fade out other pixels.

### Mesh Pattern Synchronization: 
When you're on a mesh, everyone broadcasts their pattern on repeat every `repeatDurationMs` milliseconds. All patterns are played simultaneuously, starting at the moment they are recieved.

Sometimes this create's an effect where all patterns in the mesh will play out in sequence. However sometimes when the natural timing of the devices is very similar, the patterns play in parallel, overlapping each other. This seems to be because there is no timing information synchronised. And because the pattern precidence is always defined by pattern id, this sometimes creates something like a race condition, where in the worst case scenario a higher numbered pattern will be broadcast miliseconds after starting your own pattern.

Consider a mesh with 2 nodes, `N1` and `N2`, with `durationMs = 4000` and `repeatDurationMs = 8000`. Call the difference in boot time in seconds between them `T`.
- When `T % 8 == 4` the patterns are perfectly in sequence. One pattern plays, then the other, and no time in between.
- When `T % 8 == 0` the patterns are perfectly overlapping. Both patterns will play simultaneously, mostly resulting in the higher mode pattern being visible. Worse still, after 4 seconds of play time, there will be 4 seconds of darkness.
- When `T % 8 == 2` N1's pattern pattern will play for 2 seconds, followed by 2 seconds of overlap, 2 seconds of only N2's pattern, then 2 seconds of darkness.

## Potential Improvements

1. Pattern precidence is always just based on predefined pattern selection and it alwyas overwrites.
    Also the way this is implemented is weird, if there is only one true output buffer then it shouldn't be repeated in code. Just have one consolidated output buffer.
    Implement leader selection
2. Send time remaining for better sync
3. Dynamic pattern precidence 

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
For now, attempting to maintain the same as protocol V2 but subject to change!
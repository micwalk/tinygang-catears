# TinyGang PlatformIO Code Documentation

## PlatformIO
*Why aren't you using the Audrino IDE??*
1. visual studio code is a great editor with code completion. PlatformIO is a near 1 click install using the built in package manager.
2. Better dependency management -- Dependencies are tracked on a per project basis instead of per ide. Dependencies are stored in platformio.ini, and will automatically download and resolve transative dependencies on a per project basis. The same platformio.ino also tracks monitor baud rate, chipset, board, etc.
3. More explicit C++ -- The Aurino `.ino` gets translated into a c++ file with minor modifications, but doens't have great tooling support.

## General Logic

Each jacket wearer / gang member / cat chooses their **chosen pattern** using a dip switch (4 options). This is read with the getChosenPattern function which reads the dip switch values. In my branch i've replace this with a const CHOSEN_PATTERN.

Multiple patterns can be active at a given time. When multiple patterns are active, the highest numbered pattern takes precidence.

Primary pattern state is managed through a varaible `ellapseTimeMs[pattern_id]`. This is a measure of how long each animation has been playing.

This creates 3 implict (unnamed) states
1. Active -- Currently playing an animation. Recognized when ellapseTimeMs `ellapseTimeMs[i] <= durationMs`
2. Timeout -- Done playing an animation, but before elligible for replay.In this state when `ellapseTimeMs[i] >= durationMs && ellapseTimeMs[i] <= repeatDurationMs`
3. Ready -- Inactive but waiting to be triggered. This when NOT the above to, AKA: `ellapseTimeMs[i] > repeatDurationMs`

The **chosen pattern** will always re trigger automatically when they reach the Ready state. Other patterns will trigger when around a friend with that pattern.

When you are on a **mesh**, each jacket will broadcast what pattern it is playing. When you receive a broadcast for a pattern, it resets that pattern's elapsed time to zero, activating it.

Sometimes this create's an effect where all patterns in the mesh will play out in sequence. However sometimes when the natural timing of the devices is very similar, the patterns play in parallel, overlapping each other. This seems to be because there is no timing information synchronised. And because the pattern precidence is always defined by pattern id, this sometimes creates something like a race condition, where in the worst case scenario a higher numbered pattern will be broadcast miliseconds after starting your own pattern.

Pattern Blending: it does sometimes appear to blend patterns, but it doesn't seem this way in the code. Perhaps it is the case that sometimes patterns "finish" but don't clear their LEDs back to zero/black, and the next pattern does look at the previous color value.

## Improvements

1. Pattern precidence is always just based on predefined pattern selection and it alwyas overwrites.
    Also the way this is implemented is weird, if there is only one true output buffer then it shouldn't be repeated in code. Just have one consolidated output buffer.
    Implement leader selection
2. Send time remaining for better sync
3. Dynamic pattern precidence 
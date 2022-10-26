# TinyGang PlatformIO Code Documentation

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


## Improvements

1. Pattern precidence is always just based on predefined pattern selection and it alwyas overwrites.
    Also the way this is implemented is weird, if there is only one true output buffer then it shouldn't be repeated in code. Just have one consolidated output buffer.
    Implement leader selection
2. Send time remaining for better sync?
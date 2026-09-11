# Sound tutorial checkpoints

These projects accompany [Making sounds with DISSCO and LASSIE](../../tutorial_sound.md).

| File | Starting point | What it contains |
| --- | --- | --- |
| [first_tone.dissco](first_tone.dissco) | Section 9; a starting copy for sections 11–19 and 23 | One five-second, 440 Hz tone at time zero, centered between two speakers, with a triangular amplitude fade and no reverb or modifiers. |
| [tutorial_sound.dissco](tutorial_sound.dissco) | End of section 10; a starting copy for sections 20–22 | Five tones with random start times and pitches, loudness 45, stereo movement, and simple reverberation. |

1. Copy/download each file you want to use into its own working folder.
2. Open it with LASSIE and accept Project Properties.
3. Choose **Project > Run**, enter seed `apple`, and request `1` run.
4. Open the newest AIFF in the generated **SoundFiles** folder. Its filename begins with the project filename, such as `first_tone_0.aiff`.

Both projects contain Top 0, Bottom s1, Spectrum sp1, triangular Envelope 1, and constant Envelope 2. All envelope data is embedded. Output is stereo, 44100 Hz, 24-bit AIFF. The parent spans 30 seconds, but the first-tone output lasts five seconds; the completed piece ends after its final sound and reverb tail, so its audio need not be exactly 30 seconds.

Use **File > Save As** before changing a supplied project. Keep the first tone as a recovery point and use the tutorial's listening checkpoints to compare one change at a time. In section 22, start a fresh copy for each example and apply its stated timing setup.

Both files have been saved and reopened using LASSIE's project reader/writer and rendered with CMOD. The first-tone checkpoint was also exercised through LASSIE's Run action, including the seed and run-count dialogs. Its audio was checked for a 440 Hz tone, centered channels, a five-second duration, and nonzero samples.

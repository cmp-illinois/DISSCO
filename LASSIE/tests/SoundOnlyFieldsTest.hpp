#ifndef SOUNDONLYFIELDSTEST_HPP
#define SOUNDONLYFIELDSTEST_HPP

// Checks that spatialization, reverb, filter and modifiers are hidden where
// they cannot change the output (Sound Synthesis off, or a note Bottom whose
// name starts with "n"), and that hidden values survive saving. Needs
// DISSCO_TEST_PROJECT. Returns 0 if every check passed.
int runSoundOnlyFieldsTest();

#endif // SOUNDONLYFIELDSTEST_HPP

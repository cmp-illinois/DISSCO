#ifndef FUNCTIONPICKERTEST_HPP
#define FUNCTIONPICKERTEST_HPP

// Checks what LASSIE's function picker offers: renamed functions under their
// new names (and fields saved with the old names still open), CURRENT_SEGMENT
// no longer offered, and a child Start Time picker that leaves out
// CURRENT_TYPE and CURRENT_LAYER, also in pickers opened from inside it.
// Returns 0 if every check passed. -- Tony Chai, 2026
int runFunctionPickerTest();

#endif // FUNCTIONPICKERTEST_HPP

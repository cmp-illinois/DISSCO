#ifndef PROJECTROUNDTRIPTEST_HPP
#define PROJECTROUNDTRIPTEST_HPP

// Opens, saves, reopens and re-saves every .dissco file in the folders listed
// in DISSCO_TEST_PROJECT_DIRS ('|'-separated), checking nothing is lost or
// changed. Returns 0 if every project passed. -- Tony Chai, 2026
int runProjectRoundTripTest();

#endif // PROJECTROUNDTRIPTEST_HPP

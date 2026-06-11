#ifndef TABLES_H
#define TABLES_H

/**
 * @file tables.h
 * @brief Static lookup tables used by the notated-score path.
 *
 * Pitch names (sharps and flats), tuplet name labels, rhythmic-ratio
 * names, rest-sign labels, the canonical list of note-modifier strings,
 * and a few power-of-two lookups. The arrays are declared `extern` here
 * and defined once in tables.cpp.
 */

#include <string>
using namespace std;

#define SIZE 8
#define SIZE2 8



extern string pitchNames[12];

extern string OutNames[12];

extern string ratios[SIZE];

extern string types[SIZE];

extern string tuplet_names[SIZE];

// extern int valid_time[13];
extern int valid_time[19];

extern string rest_signs[SIZE2];

extern string rest_ratios[SIZE2];

extern string pow_of_2[10];

extern string modifiers[40];

#endif

/*
CMOD (composition module)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef TIME_SIG_H
#define TIME_SIG_H

#include "Libraries.h"

#include "Tempo.h"
#include "Note.h"

/**
 * Represents a time signature in a NotationScore.
**/
struct TimeSignature {
  TimeSignature() {
    tempo_ = Tempo();

    start_time_global_ = tempo_.getStartTime(); // global start time in seconds

    time_signature_ = tempo_.getTimeSignature();
    beat_edus_ = Note::str_to_int(tempo_.getEDUPerTimeSignatureBeat().toPrettyString());
    bar_edus_ = Note::str_to_int(tempo_.getEDUPerBar().toPrettyString());
    unit_note_ = tempo_.getTimeSignatureBeat().Den(); // the note that represents one beat

    tuplet_limit_ = static_cast<int>(CalculateTupletLimit());

    ConstructTupletNames();
  }

  TimeSignature(Tempo& tempo) {
    tempo_ = tempo;

    start_time_global_ = tempo.getStartTime(); // global start time in seconds

    time_signature_ = tempo.getTimeSignature();
    beat_edus_ = Note::str_to_int(tempo.getEDUPerTimeSignatureBeat().toPrettyString());
    bar_edus_ = Note::str_to_int(tempo.getEDUPerBar().toPrettyString());
    unit_note_ = tempo.getTimeSignatureBeat().Den(); // the note that represents one beat

    tuplet_limit_ = static_cast<int>(CalculateTupletLimit());

    ConstructTupletNames();
  }

  /**
   * Calculate the greatest tuplet type possible in the score, exclusive,
   * and store the duration of one tuplet note of every possible type in
   * valid_dividers_ (from the whole beat down to one EDU). A tuplet type
   * is possible when it divides the beat into equal whole numbers of
   * EDUs, so every divisor of beat_edus_ is one, and the greatest is
   * beat_edus_ itself (one EDU per note). E.g. with 10 EDUs per beat the
   * types are 1, 2, 5 and 10, and CalculateTupletLimit() will return 11.
   * 
   * @return the greatest tuplet type possible in the score, exclusive.
  **/
  size_t CalculateTupletLimit() {
    size_t tuplet_limit = 2; // the whole beat (type 1) is always possible
    valid_dividers_.clear();

    for (int tuplet_num = 1; tuplet_num <= beat_edus_; ++tuplet_num) {
      if (beat_edus_ % tuplet_num == 0) {
        valid_dividers_.push_back(beat_edus_ / tuplet_num);
        tuplet_limit = static_cast<size_t>(tuplet_num) + 1;
      }
    }

    return tuplet_limit;
  }

  /** 
   * Determine what type of tuplet will fit the input duration. Eg. if 
   * a triplet fits the input duration return 3
   * 
   * @param dur The duration to evaluate
   * @return The fitting tuplet; -1 if input invalid
  **/
  int DetermineTuplet(int dur) {
    // valid_dividers_ runs from the longest tuplet note (the whole beat,
    // type 1) to the shortest, so the first fit is the simplest tuplet
    for (size_t i = 0; i < valid_dividers_.size(); ++i) {
      int tuplet_num = beat_edus_ / valid_dividers_[i];
      if (tuplet_num >= 2 && dur % valid_dividers_[i] == 0) {
        return tuplet_num;
      }
    }

    return -1; // invalid tuplet
  }

  /**
   * Construct tuplet names as strings for Lilypond score generation,
   * one for each possible tuplet type.
  **/
  void ConstructTupletNames() {
    tuplet_types_.clear();

    for (size_t i = 0; i < valid_dividers_.size(); ++i) {
      int tuplet_num = beat_edus_ / valid_dividers_[i];
      int l = CalculateNearestPow2(tuplet_num);
      string t = "\\tuplet ";
      t += Note::int_to_str(tuplet_num) + "/" + Note::int_to_str(l) + "{ ";
      tuplet_types_[tuplet_num] = t;
    }
  }

  /**
   * Determine whether this TimeSignature is equivalent to another.
   * Two TimeSignatures are equivalent if they have equivalent time
   * signatures, bead edus, bar edus, unit notes, and global start
   * times within a tolerance of 1/1000th of a second.
   * 
   * @param other The other TimeSignature with which to compare this
   * @return True if the two TimeSignatures are equivalent; else, false
  **/
  bool operator==(const TimeSignature& other) const {
    return time_signature_ == other.time_signature_ &&
           beat_edus_ == other.beat_edus_ &&
           bar_edus_ == other.bar_edus_ &&
           unit_note_ == other.unit_note_ &&
           fabs(start_time_global_ - other.start_time_global_) < 0.001;
  }

  /**
   * Determine whether this TimeSignature is not equivalent to another.
   * Refer to operator== for critera.
   * 
   * @param other The other TimeSignature with which to compare this
   * @return False if the two TimeSignatures are equivalent; else, true
  **/
  bool operator!=(const TimeSignature& other) const {
    return !operator==(other);
  }

  /**
   * Print this TimeSignature to an ostream.
  **/
  friend ostream& operator<<(ostream& stream, const TimeSignature& ts) {
    stream << ts.time_signature_ << '\n' << ts.beat_edus_ << '\n' << ts.bar_edus_ << 
          '\n' << ts.unit_note_ << '\n' << ts.start_time_global_ << endl;

    return stream;
  }

  /**
   * Calculate log2(num).
   * 
   * @param num The number to evaluate
   * @return The exponent of 2 which gives num (0 for num = 1); -1 if
   * num is not a power of 2
   **/
  static int DiscreteLog2(int num) {
    if (!IsPowerOf2(num)) {
      return -1;
    }

    int pow = 0;
    while (num > 1) {
      num /= 2;
      ++pow;
    }

    return pow;
  }

  /**
   * Determine whether num is a power of 2 (1, 2, 4, 8, ...).
   *
   * @param num The number to evaluate
   * @return True if num is a positive power of 2; else, false
   **/
  static bool IsPowerOf2(int num) {
    return num > 0 && (num & (num - 1)) == 0;
  }

  /**
   * Find the nearest power of 2 lower than the given number.
   * E.g. if num = 7, return 4; if num = 9, return 8.
   * 
   * @param num The number to evaluate
   * @return The nearest power of 2 lower than num
  **/
  static int CalculateNearestPow2(int num) {
    int multiple_of_2 = 1;

    while (multiple_of_2 <= num) {
      multiple_of_2 *= 2;
    }

    return multiple_of_2 / 2;
  }

  /**
   * Calculate base^pow
   * 
   * @param base The integer to exponentiate
   * @param pow The exponential power
   * @return base^pow
  **/
  static int Power(int base, int pow) {
    int output = 1;
    for (int i = 0; i < pow; ++i) {
      output *= base;
    }

    return output;
  }

  Tempo tempo_;
  float start_time_global_; // global start time in seconds

  string time_signature_;
  int beat_edus_;
  int bar_edus_;
  int unit_note_ ; // the note that represents one beat

  map<int, string> tuplet_types_; // tuplet type -> LilyPond tuplet opening
  vector<int> valid_dividers_;
  int tuplet_limit_;
};

#endif

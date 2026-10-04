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
// all codes with the comment "multistaffs" are added by xiaoyi han
#ifndef NOTATION_SCORE_H
#define NOTATION_SCORE_H

#include "Libraries.h"

#include "Note.h"
#include "Section.h"
#include "TimeSignature.h"
#include "Rational.h"
#include "Tempo.h"
#include "TimeSpan.h"

/**
 * @file NotationScore.h
 * @brief Notated-score model rendered out as LilyPond.
 *
 * Original notation module written by Haorong Sun; multistaff extensions
 * tagged "multistaffs" in the source were added by Xiaoyi Han. A
 * NotationScore owns the per-staff stream of @ref Section objects, the
 * tempo / time-signature spine, and the metadata (title, staff count,
 * grand-staff layout) that LilyPond needs to typeset the result.
 */

class Section;

/**
 * A class representing a notated score for output using Lilypond.
 * 
 * Original notation module written by Haorong Sun
**/
class NotationScore {

public:
  /**
   * Construct a notation score.
  **/
  NotationScore();

  /**
   * Construct a notation score with the provided title.
   * 
   * @param score_title The title of this score
   * @param grandStaff Whether to create a two-staff grand staff
   * @param numberOfStaff Staff count when grandStaff is false; nonpositive
   * values fall back to one staff
  **/
  // NotationScore(const string& score_title);
  // multistaffs
  NotationScore(const string& score_title,bool grandStaff, int numberOfStaff);

  /**
   * Insert a Tempo into this score: each tempo (of one root exact
   * ancestor) is a section of its own on the staff, even when it starts
   * together with another one.
   *
   * @param tempo The tempo to insert
   * @param numberOfStaff Zero-based index of the staff receiving the tempo,
   * clamped to the available staff range (not a staff count)
  **/
  // void RegisterTempo(Tempo& tempo);
  // multistaffs
  void RegisterTempo(Tempo& tempo, int numberOfStaff);


  /**
   * Insert a Note into this score, in the section of its tempo on its
   * staff. A note whose staff number is out of range goes on the nearest
   * staff, and Build warns about it.
   *
   * @param n A pointer to the note to insert
   * @param tempo The tempo of the note, registered with RegisterTempo
  **/
  void InsertNote(Note* n, Tempo& tempo);

  /**
   * Build the text representation of this score by adding bars,
   * rests, and adjusting durations. The project is rejected when sections
   * with different tempos overlap in time, on any staves, since the staves
   * share one tempo. Sections that overlap on a staff are
   * merged onto the bars of the earlier one when they share its tempo,
   * time signature and EDU Per Beat, the later one starting at the nearest
   * edu of the earlier one's grid; otherwise the project is
   * rejected. The score starts at 0 seconds: the silence before the first
   * section is rests in its tempo and meter. The time a staff has no
   * section of its own is filled with sections of rests taken from the
   * other staves, and all staves end together, at the end of the bar that
   * holds the last note of the score.
  **/
  void Build();

  /**
   * Output the text representation of a score. The top staff shows a
   * tempo mark at the start and wherever the tempo changes.
   *
   * @param output_stream The stream to which the text will be appended
   * @param notation_score The score whose text representation to output
   * @returns The modified stream
  **/
  friend ostream& operator<<(ostream& output_stream, 
                             NotationScore& notation_score);

private:
  /**
   * Get the staff a Staff Number goes on: an out-of-range number goes
   * on the nearest staff.
   *
   * @param staff The Staff Number
   * @return The zero-based index of an existing staff
  **/
  int StaffIndex(int staff) const;

  string score_title_;

  vector<Section> score_;
  vector< vector <Section> > score_staff;
  bool is_built_;
  bool is_grand;
  // the total number of staffs
  int staffSum;
  // staff number out of range -> the number of notes that asked for it
  map<int, int> out_of_range_staffs_;
};

#endif
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
  **/
  // NotationScore(const std::string& score_title);
  // multistaffs
  NotationScore(const std::string& score_title,bool grandStaff, int numberOfStaff);

  /**
   * Insert a Tempo into this score.
   * 
   * @param tempo The tempo to insert
  **/
  // void RegisterTempo(Tempo& tempo);
  // multistaffs
  void RegisterTempo(Tempo& tempo, int numberOfStaff);


  /**
   * Insert a Note into this score.
   * 
   * @param n A pointer to the note to insert
  **/
  void InsertNote(Note* n);
  
  /**
   * Build the text representation of this score by adding bars,
   * rests, and adjusting durations.
  **/
  void Build();

  /**
   * Output the text representation of a score.
   * 
   * @param out_stream The stream to which the text will be appended
   * @param notation_score The score whose text representation to output
   * @returns The modified stream
  **/
  friend std::ostream& operator<<(std::ostream& output_stream, 
                             NotationScore& notation_score);

private:
  void PrintScoreFlat() const {
    size_t section_idx = 0;
    std::cout << std::endl << std::endl;
    std::cout << "SCORE: " << std::endl;
    for (std::vector<Section>::const_iterator iter = score_.begin();
         iter != score_.end();
         ++iter) {
      const Section& section = *iter;
      std::cout << "SECTION " << section_idx << std::endl;
      section.PrintAllNotesFlat("Score printing");
      ++section_idx;
    }
    std::cout << std::endl << std::endl;
  }

  std::string score_title_;

  std::vector<Section> score_;
  std::vector< std::vector <Section> > score_staff;
  bool is_built_;
  bool is_grand;
  // the total number of staffs
  int staffSum;
};

#endif
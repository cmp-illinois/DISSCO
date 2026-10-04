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
#ifndef SECTION_H
#define SECTION_H

#include "Libraries.h"

#include "TimeSignature.h"
#include "Note.h"

/**
 * A class representing a notated section for output using 
 * Lilypond (or something else in the future).
**/
class Section {
public:
  /**
   * Construct a notation section with the provided time signature.
   * 
   * @param time_signature The time signature of the section
  **/
  Section(TimeSignature time_signature);
  TimeSignature GetTimeSignature() const; //diyun

  /**
   * Copy a Section.
   * 
   * @param other The other Section to copy
  **/
  Section(const Section& other);

  /*
   * Move a Section.
   * 
   * @param source The Section to move from
  **/
  // Section(Section&& source);

  /**
   * Copy a Section using assignment operator.
   * 
   * @param other The other Section to copy
   * @return This Section
  **/
  Section& operator=(const Section& other);

  /*
   * Move a Section using assignment operator.
   * 
   * @param source The Section to move from
   * @return This Section
  **/
  // Section& operator=(Section&& source);

  /**
   * Destruct this notation section.
  **/
  ~Section();

  /**
   * Determine whether this Section is the section of a tempo: the tempo
   * of the same root exact ancestor, with the same timing and start.
   *
   * @param tempo The tempo of a note's Bottom
   * @return True if the tempo's notes belong to this Section; else, false
  **/
  bool IsSectionOf(Tempo& tempo);

  /**
   * Add a note to this section, with its start and end in edus from the
   * section start. Notes are placed into bars when InsertAddedNotes is
   * called, so a section can first take the notes of a section that
   * overlaps it (TakeNotes).
   *
   * @param n A pointer to the note to add
  **/
  void AddNote(Note* n);

  /**
   * Move the notes added to a later Section that overlaps this one onto
   * this Section's bars. Call it before InsertAddedNotes.
   *
   * @param later The Section whose notes to take; it is left without notes
   * @param offset_edus How many edus after this Section's start the later
   * Section starts
  **/
  void TakeNotes(Section& later, int offset_edus);

  /**
   * Insert the added notes into the bars of this section, combining
   * notes that overlap into chords.
  **/
  void InsertAddedNotes();

  /**
   * Set the duration of this Section in edus with respect to this
   * Section's time signature. Providing (-1) sets no limit on the 
   * duration.
   * 
   * @param edus The duration of this Section in edus
  **/
  void SetDurationEDUS(int edus);

  /**
   * Get the end of the last note added to this Section, in edus from its
   * start. Call it before building.
   *
   * @return The end of the last note in edus; 0 if no note was added
  **/
  int GetLastNoteEnd() const;

  /**
   * Get how much building this Section lengthened it to give its cap
   * bar a notatable time signature.
   *
   * @return The extension in seconds; 0 if there was none
  **/
  float GetCapExtensionSeconds() const;

  /**
   * Get the global start time of this section in seconds.
   * 
   * @return The global start time of this section in seconds
  **/
  float GetStartTimeGlobal() const;

  /**
   * Convert a quantity of seconds to edus with respect to this
   * Section's time signature.
   * 
   * @param seconds The quantity in seconds to convert
   * @return The equivalent edus
  **/
  int CalculateEDUsFromSecondsInTempo(float seconds);
  
  /**
   * Convert a quantity of edus to seconds with respect to this
   * Section's time signature.
   *
   * @param edus The quantity in edus to convert
   * @return The equivalent seconds
  **/
  float CalculateSecondsFromEDUsInTempo(int edus);
  
  /**
   * Build the text representation of this section by adding bars,
   * rests, and adjusting durations.
   * 
   * @param notate_time_signature True if the time signature 
   * should be notated; else, false
   * @param staff_loudness The last loudness mark notated on this
   * Section's staff before it ("" if none); updated to the last
   * loudness mark notated in this Section
  **/
  void Build(bool notate_time_signature, string& staff_loudness);

  /**
   * Get this section as a flattened entity ready for output.
  **/
  const list<Note*>& GetSectionFlat();

  /**
   * Get the time signature in force at the end of this Section after
   * building: that of its cap bar if it has one, else its own.
   *
   * @return The time signature in force at the end of this Section
  **/
  TimeSignature GetEndingTimeSignature() const;

  /**
   * Get the time signature of each bar of this Section after building,
   * in order: its own, except for its cap bar if it has one.
   *
   * @return The time signature of each bar, such as "4/4"
  **/
  vector<string> GetBarTimeSignatures() const;

  /**
   * Get whether each bar of this Section notates its time signature at
   * its start, after building, in the order of GetBarTimeSignatures.
   *
   * @return True for each bar whose time signature is notated
  **/
  vector<bool> GetBarTimeSignatureMarks() const;

  /**
   * Notate the time signature of a bar of this Section at its start,
   * after building, where it is not notated yet.
   *
   * @param bar The index of the bar, as in GetBarTimeSignatures
  **/
  void NotateBarTimeSignature(size_t bar);

  bool operator<(const TimeSignature& other) const;

  bool operator>(const TimeSignature& other) const;

  bool operator==(const TimeSignature& other) const;

  bool operator!=(const TimeSignature& other) const;

private:
  /**
   * Insert a note into the bars of this section. Where it overlaps notes
   * already inserted, they are cut into chords.
   *
   * @param n A pointer to the note to insert
  **/
  void InsertNote(Note* n);

  /**
   * If the remainder given by the note's end time modulo the edu's per beat
   * is not divisible by any element of valid_dividers_, find the smallest 
   * difference between a multiple of the valid dividers and the remainder. 
   * Then, shorten the note's duration by this difference to make the note's
   * duration expressible.
   * 
   * @param n The note whose duration to examine and modify
  **/
  void EnsureNoteExpressible(Note* n);

  /**
   * Resize the section to the specified size.
   * 
   * @param new_size The desired size of the section
  **/
  void ResizeSection(int new_size);

  /**
   * Add bars to this section.
  **/
  void AddBars();

  /**
   * Add rests between notes and flatten the section to section_flat_. 
   * Rests are not processed and may have invalid (inexpressible) 
   * durations.
  **/
  void AddRestsAndFlatten();

  /**
   * Choose the tuplet type of every beat in which a sound or silence
   * starts or ends: the simplest tuplet whose notes divide the beat at
   * all of those points, so every sound and silence in the beat can be
   * written in it without being moved or shortened. Stores the result
   * in beat_tuplets_.
  **/
  void DetermineBeatTuplets();

  /**
   * Run the notation loop for the section. 
  **/
  void Notate();

  /**
   * Cap the ending of this Section according to the EDU allotment: when
   * the next section starts partway through the last bar, rewrite that
   * bar as a shorter cap bar with its own time signature.
  **/
  void CapEnding();

  /**
   * Notate the current note's duration given the previous tuplet type and the current
   * tuplet duration. This involves filling the current tuplet duration, filling the
   * subsequent complete beats if the note extends past the tuplet, and then using
   * rests to fill the remaining duration.
   * 
   * @param current_note The current note whose duration to notate
   * @param prev_tuplet The type of the previous tuplet
   * @param tuplet_dur The duration of the current tuplet to fill
   * @return The remaining duration to carry over in the notation loop
  **/
  int NotateCurrentNote(Note* current_note, int* prev_tuplet, int tuplet_dur);

  /**
   * Fill the current tuplet duration using the current note's duration.
   * 
   * @param current_note The current note whose duration to notate
   * @param prev_tuplet The type of the previous tuplet
   * @param tuplet_dur The duration of the current tuplet to fill
   * @return The remaining note duration
  **/
  int FillCurrentTupletDur(Note* current_note, int prev_tuplet, int tuplet_dur);
  
  /**
   * Fill the complete beats of the remaining current note's duration.
   * 
   * @param current_note The current note whose duration to notate
   * @param remaining_dur The remaining duration of the current note
   * @return The final leftover duration to be notated with rests
  **/
  int FillCompleteBeats(Note* current_note, int remaining_dur);

  /**
   * Create a tuplet with rests for the current note's final remaining duration.
   * The remainder starts a beat and uses that beat's tuplet from beat_tuplets_.
   *
   * @param current_note The current note whose duration to notate
   * @param prev_tuplet The type of the previous tuplet to set
   * @param remaining_dur The current note's leftover duration to be notated with rests
   * @return The tuplet duration to carry over in the notation loop
  **/
  int CreateTupletWithRests(Note* current_note, int* prev_tuplet, int remaining_dur);

  /**
   * Notate a note (or rest) in a tuplet.
   * 
   * @param current_note A pointer to the note to notate
   * @param tuplet_type The type of the tuplet in which to notate the note
   * @param duration The duration of the note to notate
  **/
  void NoteInTuplet(Note* current_note, int tuplet_type, int duration);

  /**
   * Add a loudness mark to the current note.
   * 
   * @param current_note The note to which to add a loudness mark
  **/
  void LoudnessMark(Note* current_note);

  /**
   * Make the item that notates a time signature at the start of a bar.
   *
   * @param time_signature The time signature, such as "4/4"
   * @return The new item, owned by the caller
  **/
  static Note* TimeSignatureMark(const string& time_signature);

  /**
   * Report a beat unit that cannot be written as a note value. Time
   * signatures are validated when read, so this means an internal error;
   * it stops the beat-filling loops instead of letting them spin.
   *
   * @param unit_note The note value (1/unit_note) that is not a power of 2
  **/
  void ThrowUnnotatableBeat(int unit_note) const;

  /**
   * Get __only the notes__ of the last bar of this Section 
   * after building and __remove the full bar from this Section__,
   * keeping the barline before it
   * 
   * @return The notes of the last bar of this Section
  **/
  list<Note*> PopLastBarNotes();

  TimeSignature time_signature_;

  vector<Note*> added_notes_; // notes added but not yet inserted into bars
  vector< vector<Note*> > section_;
  list<Note*> section_flat_;
  bool is_built_;

  int remaining_edus_;
  bool is_edu_limit_; // true if the Section has an edu allotment; else false
  Section* cap_; // TODO - this is poor. the solution is to stop using pointers for Notes.
  float cap_extension_seconds_; // how much CapEnding lengthened this Section

  map<int, int> beat_tuplets_; // beat index -> the tuplet type of that beat in the notation loop
  string prev_loudness_; // the previous loudness mark on this staff in the notation loop
  string last_bar_loudness_; // the loudness mark in force where the last bar starts
};

#endif

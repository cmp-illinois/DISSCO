#include "NotationScore.h"
#include "CmodError.h"

// Escapes text for a LilyPond "..." string, so it reads exactly as given.
static string EscapeLilyPondString(const string& text) {
  string escaped;
  for (char c : text) {
    if (c == '\\' || c == '"') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}

// Chooses the clef for the notes of a built section from their average
// pitch: treble if it is not below middle C (pitch 48), else bass. Returns
// "" for a section with only rests.
static string ChooseClef(const list<Note*>& section_flat) {
  int avePitchNum = 0;
  int pitchSum = 0;
  for (list<Note*>::const_iterator iter = section_flat.begin();
       iter != section_flat.end();
       ++iter) {
    if ((*iter)->is_real_note()) {
      avePitchNum = avePitchNum + (*iter)->getPitchNum();
      pitchSum = pitchSum + 1;
    }
  }
  if (pitchSum == 0) {
    return "";
  }
  return (avePitchNum / pitchSum >= 48) ? "treble" : "bass";
}

// Writes a note value, given as a fraction of a whole note, as a LilyPond
// duration: a power-of-two value, plain or with up to two dots ("4", "4.",
// "8.."). Returns "" for a value that cannot be written so, such as a
// triplet quarter (1/6).
static string LilyPondDuration(Ratio value) {
  int dots;
  if (value.Num() == 1) {
    dots = 0;
  } else if (value.Num() == 3) {
    dots = 1;
  } else if (value.Num() == 7) {
    dots = 2;
  } else {
    return "";
  }
  // The undotted value, 1/undotted, is 2^dots / Den: a whole note or shorter
  const int undotted = value.Den() >> dots;
  if (!TimeSignature::IsPowerOf2(value.Den()) || undotted < 1) {
    return "";
  }
  return to_string(undotted) + string(static_cast<size_t>(dots), '.');
}

// Gets the tempo mark of a tempo, written as it was entered (\tempo 4 = 60,
// \tempo 4. = 80). LilyPond writes only plain or dotted beats and whole
// numbers of beats per minute, so a beat it cannot write (such as a triplet
// quarter) is restated in the beat of the time signature, which is the same
// tempo, and beats per minute that are not a whole number are rounded to
// the nearest one.
static string TempoMark(Tempo tempo) {
  string beat = LilyPondDuration(tempo.getTempoBeat());
  Ratio beats_per_minute = tempo.getTempoBeatsPerMinute();
  if (beat.empty()) {
    beat = LilyPondDuration(tempo.getTimeSignatureBeat());
    beats_per_minute = tempo.getTimeSignatureBeatsPerMinute();
  }
  const long long rounded = std::max(1LL, std::llround(beats_per_minute.To<double>()));
  return "\\tempo " + beat + " = " + to_string(rounded);
}

// Gets how many whole notes a tempo plays per minute. Two tempos are the
// same when a whole note lasts as long in both, such as quarter = 60 in 4/4
// and eighth = 120 in 6/8.
static Rational<long long> WholeNotesPerMinute(Tempo tempo) {
  Ratio beats_per_minute = tempo.getTempoBeatsPerMinute();
  Ratio beat = tempo.getTempoBeat();
  return Rational<long long>(static_cast<long long>(beats_per_minute.Num()) * beat.Num(),
                             static_cast<long long>(beats_per_minute.Den()) * beat.Den());
}

// Gets the end of the bar that holds the last note of an unbuilt section,
// in edus from its start; 0 for a section without notes.
static int EndOfLastBar(const Section& section) {
  const int bar_edus = section.GetTimeSignature().bar_edus_;
  return (section.GetLastNoteEnd() + bar_edus - 1) / bar_edus * bar_edus;
}

// Warns that building a section lengthened it to give its cap bar a
// notatable time signature, unless the same warning (start time and
// extension, in seconds) was already printed for another staff.
static void WarnCapExtension(const Section& section,
                             vector< pair<float, float> >& warned) {
  pair<float, float> extension(section.GetStartTimeGlobal(), section.GetCapExtensionSeconds());
  if (extension.second == 0 ||
      std::find(warned.begin(), warned.end(), extension) != warned.end()) {
    return;
  }
  warned.push_back(extension);
  cout << "Warning: Score section starting at " << extension.first
       << " seconds was extended by " << extension.second
       << " seconds to fit a notatable time signature. "
       << "Suggestion: Align tempo changes to the timing grid if this extension is not intended." << endl;
}

// Tells whether the staves of a built score have different bars, such as
// 4/4 on one staff while another is in 3/4 (polymeter).
static bool StavesHaveDifferentBars(const vector< vector<Section> >& staves) {
  vector<string> first_staff_bars;
  for (size_t i = 0; i < staves.size(); i++) {
    vector<string> bars;
    for (vector<Section>::const_iterator iter = staves[i].begin();
         iter != staves[i].end();
         ++iter) {
      vector<string> section_bars = iter->GetBarTimeSignatures();
      bars.insert(bars.end(), section_bars.begin(), section_bars.end());
    }
    if (i == 0) {
      first_staff_bars = bars;
    } else if (bars != first_staff_bars) {
      return true;
    }
  }
  return false;
}

// In a polymetric score (staves in different meters at the same time),
// notates a staff's own time signature wherever it has a barline and
// another staff notates a time signature, even where its meter goes on,
// so a player does not take the other staff's change for its own. The
// staves are built; bars are placed by their written length in whole
// notes from the start of the score, as LilyPond places them.
static void NotateTimeSignaturesTogether(vector< vector<Section> >& staves) {
  struct Bar {
    Ratio start;
    string time_signature;
    Section* section;
    size_t index; // in its section
    bool notated; // its time signature is notated at its start
  };
  vector< vector<Bar> > bars(staves.size());
  for (size_t i = 0; i < staves.size(); i++) {
    Ratio start(0);
    for (vector<Section>::iterator section = staves[i].begin();
         section != staves[i].end();
         ++section) {
      vector<string> time_signatures = section->GetBarTimeSignatures();
      vector<bool> marks = section->GetBarTimeSignatureMarks();
      for (size_t k = 0; k < time_signatures.size(); k++) {
        Bar bar = {start, time_signatures[k], &*section, k, k < marks.size() && marks[k]};
        bars[i].push_back(bar);
        start += Ratio(time_signatures[k]);
      }
    }
  }

  // For each bar of one staff, the bar of another staff in force where it
  // starts (every staff starts with the score) tells whether their meters
  // differ there, and whether the other staff has a barline where this
  // one notates a time signature
  bool polymetric = false;
  vector<Bar*> to_notate;
  for (size_t i = 0; i < bars.size(); i++) {
    for (size_t j = 0; j < bars.size(); j++) {
      if (i == j || bars[j].empty()) {
        continue;
      }
      size_t in_force = 0;
      for (size_t k = 0; k < bars[i].size(); k++) {
        Bar& bar = bars[i][k];
        while (in_force + 1 < bars[j].size() && bars[j][in_force + 1].start <= bar.start) {
          ++in_force;
        }
        Bar& other = bars[j][in_force];
        polymetric = polymetric || other.time_signature != bar.time_signature;
        if (bar.notated && other.start == bar.start) {
          to_notate.push_back(&other);
        }
      }
    }
  }
  if (!polymetric) {
    return;
  }
  for (size_t k = 0; k < to_notate.size(); k++) {
    to_notate[k]->section->NotateBarTimeSignature(to_notate[k]->index);
  }
}

// Describes a section for an error message: its start and its timing.
static string DescribeSection(const Section& section) {
  TimeSignature ts = section.GetTimeSignature();
  return to_string(section.GetStartTimeGlobal()) + " seconds (" + ts.time_signature_ +
         ", tempo " + ts.tempo_.getTempoBeat().toPrettyString() + " = " +
         ts.tempo_.getTempoBeatsPerMinute().toPrettyString() +
         ", EDU Per Beat " + to_string(ts.beat_edus_) + ")";
}

// Gets where a later section starts on the edu grid of an earlier one: the
// time between their starts, rounded to the nearest edu of the earlier
// section, as a time between edus is rounded. The sections overlap when
// the earlier one's notes end after that.
static int StartOffsetEDUs(Section& earlier, const Section& later) {
  return earlier.CalculateEDUsFromSecondsInTempo(later.GetStartTimeGlobal() -
                                                 earlier.GetStartTimeGlobal());
}

// Rejects a score in which tempo sections (notes not yet inserted) with
// different tempos overlap in time on different staves (on one staff,
// MergeOverlappingSections rejects them): the staves of a score share one
// tempo, so the staves that sound at the same time must have the same one
// (WholeNotesPerMinute). The sections of each staff are already merged, so
// they overlap as the score places them (StartOffsetEDUs). A tempo change
// after the notes of the earlier tempo end is fine.
static void CheckSimultaneousTempos(vector< vector<Section> >& staves) {
  struct TempoSection {
    Section* section;
    int staff;
    int last_note_end;
    Rational<long long> whole_notes_per_minute;
  };
  vector<TempoSection> sections;
  for (size_t i = 0; i < staves.size(); i++) {
    for (vector<Section>::iterator iter = staves[i].begin();
         iter != staves[i].end();
         ++iter) {
      TempoSection tempo_section = {&*iter, static_cast<int>(i), iter->GetLastNoteEnd(),
                                    WholeNotesPerMinute(iter->GetTimeSignature().tempo_)};
      sections.push_back(tempo_section);
    }
  }
  std::stable_sort(sections.begin(), sections.end(),
                   [](const TempoSection& a, const TempoSection& b) {
                     return a.section->GetStartTimeGlobal() < b.section->GetStartTimeGlobal();
                   });

  for (size_t later = 1; later < sections.size(); later++) {
    for (size_t earlier = 0; earlier < later; earlier++) {
      TempoSection& first = sections[earlier];
      TempoSection& second = sections[later];
      if (first.whole_notes_per_minute == second.whole_notes_per_minute) {
        continue;
      }
      if (first.last_note_end <= StartOffsetEDUs(*first.section, *second.section)) {
        continue;
      }
      throw CmodError(CmodError::Kind::Project,
                      "Two tempo sections overlap in time but differ in tempo. Staves that sound "
                      "at the same time must share one tempo in the score.",
                      "Score output, sections starting at " + DescribeSection(*first.section) +
                          " on staff " + to_string(first.staff) + " and at " +
                          DescribeSection(*second.section) + " on staff " +
                          to_string(second.staff),
                      "Give events that sound at the same time the same tempo (the same length "
                      "of a whole note, such as quarter = 60 in 4/4 and eighth = 120 in 6/8), "
                      "start the later one after the earlier one's notes end, or disable score "
                      "output.");
    }
  }
}

// Merges the sections of a staff (in time order, notes not yet inserted)
// that overlap in time: where the notes of one still sound when the next
// starts, the next section's notes go onto the bars of the first, which
// continue until the merged music ends. That needs one bar grid for both:
// the same tempo, time signature and EDU Per Beat. The next section starts
// at the nearest edu of the first one's grid (StartOffsetEDUs), and its
// notes keep their offsets from that start. Sections that overlap without
// sharing a grid cannot be notated on one staff.
static void MergeOverlappingSections(vector<Section>& sections, int staff) {
  size_t earlier = 0;
  while (earlier + 1 < sections.size()) {
    Section& first = sections[earlier];
    Section& second = sections[earlier + 1];
    const int offset_edus = StartOffsetEDUs(first, second);
    if (first.GetLastNoteEnd() <= offset_edus) {
      ++earlier;
      continue;
    }

    TimeSignature first_ts = first.GetTimeSignature();
    TimeSignature second_ts = second.GetTimeSignature();
    if (first_ts.time_signature_ != second_ts.time_signature_ ||
        first_ts.beat_edus_ != second_ts.beat_edus_ ||
        first_ts.tempo_.getTimeSignatureBeatsPerMinute() !=
            second_ts.tempo_.getTimeSignatureBeatsPerMinute()) {
      throw CmodError(CmodError::Kind::Project,
                      "Two tempo sections overlap on one staff but differ in tempo, time signature "
                      "or EDU Per Beat, so they cannot share its bars.",
                      "Score output, staff " + to_string(staff) + ", sections starting at " +
                          DescribeSection(first) + " and at " + DescribeSection(second),
                      "Give the overlapping events the same Tempo, Time Signature and EDU Per "
                      "Beat, start the later one after the earlier one's notes end, or put their "
                      "notes on different staves.");
    }

    first.TakeNotes(second, offset_edus);
    sections.erase(sections.begin() + static_cast<vector<Section>::difference_type>(earlier + 1));
  }
}

// Fills the time a staff has no section of its own with sections of rests,
// so that it covers the whole score in step with the other staves. Where a
// section of the score (timeline, in time order) starts, the staff takes it
// as rests unless it has a section of its own starting there, or the notes
// of its section in force are still sounding there. So the rests come
// before its first section and after the notes of a section have ended;
// a section of its own that overlaps other staves' sections is kept.
static void FillWithRests(vector<Section>& sections,
                          const vector<TimeSignature>& timeline) {
  for (vector<TimeSignature>::const_iterator ts = timeline.begin();
       ts != timeline.end();
       ++ts) {
    Section rests(*ts);
    // The first section of the staff that does not start before this one
    vector<Section>::iterator next = sections.begin();
    while (next != sections.end() && *next < *ts) {
      ++next;
    }
    // Less than one edu of rests would be left before it
    if (next != sections.end() &&
        rests.CalculateEDUsFromSecondsInTempo(next->GetStartTimeGlobal() -
                                              ts->start_time_global_) == 0) {
      continue;
    }
    if (next != sections.begin()) {
      Section& previous = *(next - 1);
      int previous_edus = previous.CalculateEDUsFromSecondsInTempo(
          ts->start_time_global_ - previous.GetStartTimeGlobal());
      if (previous_edus == 0 || previous.GetLastNoteEnd() > previous_edus) {
        continue;
      }
    }
    sections.insert(next, rests);
  }
}

NotationScore::NotationScore() : 
    score_title_("Score"),
    is_built_(false) {}

// NotationScore::NotationScore(const string& score_title) :
//     is_built_(false),
//     score_title_(score_title) {
//     }

// multistaffs
NotationScore::NotationScore(const string& score_title,bool grandStaff, int numberOfStaff) :
    score_title_(score_title),
    is_built_(false) {
    // initialize the staff
    if(grandStaff){
      is_grand = true;
      staffSum = 2;
    }else{
      is_grand = false;
      if(numberOfStaff > 0){
        staffSum = numberOfStaff;
      }else{
        cout<<"WARNING: the total number of staffs should be an integer and greater than 0." << "\n";
        staffSum = 1;
      } 
    }
    // define the layer number for the staff layer.
    score_staff.resize(staffSum);
    }

// void NotationScore::RegisterTempo(Tempo& tempo) {
//   // Find insertion point by comparing the global start in __seconds__
//   TimeSignature ts = TimeSignature(tempo);
//   vector<Section>::iterator section_iter = score_.begin();
//   while (section_iter != score_.end() && 
//          *section_iter < ts) {
//     ++section_iter;
//   }

//   if (score_.empty() || *section_iter != ts) {
//     score_.insert(section_iter, Section(ts));
//   }
// }

// multistaffs
void NotationScore::RegisterTempo(Tempo& tempo,int staffNum) {
  // make sure staffNum is a valid number
  // start from 0
  staffNum = StaffIndex(staffNum);
  for (vector<Section>::iterator iter = score_staff[staffNum].begin();
       iter != score_staff[staffNum].end();
       ++iter) {
    if (iter->IsSectionOf(tempo)) {
      return;
    }
  }
  // Find insertion point by comparing the global start in __seconds__;
  // sections that start together stay in the order they were registered
  TimeSignature ts = TimeSignature(tempo);
  vector<Section>::iterator section_iter = score_staff[staffNum].begin();
  while (section_iter != score_staff[staffNum].end() &&
         !(*section_iter > ts)) {
    ++section_iter;
  }
  score_staff[staffNum].insert(section_iter, Section(ts));
}

int NotationScore::StaffIndex(int staff) const {
  if (staff < 0) {
    return 0;
  }
  return staff >= staffSum ? staffSum - 1 : staff;
}

// void NotationScore::InsertNote(Note* n) {
//   if (score_.empty()) {
//     cerr << "Cannot add note to score without any sections!" << endl;
//     exit(1);
//   }

//   vector<Section>::iterator section_iter = score_.begin();
//   while (section_iter != score_.end() && !(*section_iter).InsertNote(n)) ++section_iter;

//   if (section_iter == score_.end()) {
//     cerr << "Note does not belong to any section in the score!" << endl;
//     exit(1);
//   }
// }

// multistaffs
void NotationScore::InsertNote(Note* n, Tempo& tempo) {
  // judge if the staff number is out of range; Build warns about it
  if(n->getStaffNum() != StaffIndex(n->getStaffNum())){
    ++out_of_range_staffs_[n->getStaffNum()];
    n->setStaffNum(StaffIndex(n->getStaffNum()));
  }
  if (score_staff[n->getStaffNum()].empty()) {
    throw CmodError(CmodError::Kind::Internal,
                    "A note was generated before its score tempo was registered.",
                    "Score output, staff " + to_string(n->getStaffNum()),
                    "Report this error to the DISSCO developers with the project file, seed, and full output.");
  }

  vector<Section>::iterator section_iter = score_staff[n->getStaffNum()].begin();
  while (section_iter != score_staff[n->getStaffNum()].end() && !section_iter->IsSectionOf(tempo)) ++section_iter;

  if (section_iter == score_staff[n->getStaffNum()].end()) {
    throw CmodError(CmodError::Kind::Internal,
                    "A note could not be assigned to a registered tempo section.",
                    "Score output, staff " + to_string(n->getStaffNum()),
                    "Report this error to the DISSCO developers with the project file, seed, and full output.");
  }
  // Build places the notes into bars, once sections that overlap are merged
  section_iter->AddNote(n);
}

// void NotationScore::Build() {
//   if (!is_built_) {
//     // Since all tempos are registered, calculate their start times 
//     // in terms of the previous tempo's EDU's
//     vector<Section>::iterator iter = score_.begin();
//     vector<Section>::iterator next = score_.begin() + 1;
//     int last_start_time_edu = 0;

//     while (next != score_.end()) {
//       float dur_seconds = next->GetStartTimeGlobal() - iter->GetStartTimeGlobal();
//       iter->SetDurationEDUS(iter->CalculateEDUsFromSecondsInTempo(dur_seconds));
//       iter->Build(true);
//       ++iter; ++next;
//     }
//     iter->SetDurationEDUS(-1);
//     iter->Build(true);

//     is_built_ = true;
//   }
// }

// multistaffs

void NotationScore::Build() {
  if (!is_built_) {
    for (map<int, int>::const_iterator staff = out_of_range_staffs_.begin();
         staff != out_of_range_staffs_.end();
         ++staff) {
      cout << "Warning: Staff Number " << staff->first << " does not exist in this score (staves 0 to "
           << staffSum - 1 << "), so " << staff->second << (staff->second == 1 ? " note was" : " notes were")
           << " placed on staff " << StaffIndex(staff->first) << ". Suggestion: Use a Staff Number from 0 to "
           << staffSum - 1 << (is_grand ? "." : ", or raise Number of Staff in the project properties.") << endl;
    }

    // Sections that overlap on a staff share the bars of the earlier one,
    // and staves that sound at the same time share one tempo; then each
    // section's notes go into its bars
    for (int i = 0; i < staffSum; i++) {
      MergeOverlappingSections(score_staff[i], i);
    }
    CheckSimultaneousTempos(score_staff);
    for (int i = 0; i < staffSum; i++) {
      for (vector<Section>::iterator iter = score_staff[i].begin();
           iter != score_staff[i].end();
           ++iter) {
        iter->InsertAddedNotes();
      }
    }

    // Every section of the score, in time order; at the same start time
    // the section of the lowest staff comes first
    vector<TimeSignature> timeline;
    for (int i = 0; i < staffSum; i++) {
      for (vector<Section>::iterator iter = score_staff[i].begin();
           iter != score_staff[i].end();
           ++iter) {
        if (iter->GetLastNoteEnd() == 0) {
          throw CmodError(CmodError::Kind::Project,
                          "No notes with a notatable duration remain in this score section.",
                          "Score output, staff " + to_string(i) + ", section starting at " +
                              to_string(iter->GetStartTimeGlobal()) + " seconds",
                          "Check note durations and EDU settings. Give notes a positive duration that can be represented in the score, or disable score output.");
        }
        timeline.push_back(iter->GetTimeSignature());
      }
    }
    if (timeline.empty()) {
      throw CmodError(CmodError::Kind::Project,
                      "No note events were generated for the score.",
                      "Score output",
                      "Add note events to the project, or disable score output.");
    }
    std::stable_sort(timeline.begin(), timeline.end(),
                     [](const TimeSignature& a, const TimeSignature& b) {
                       return a.start_time_global_ < b.start_time_global_;
                     });

    // The score starts at the start of the piece: the silence before the
    // first section is a section of rests in its tempo and meter, which
    // ends in a cap bar where it does not fill whole bars
    Tempo piece_start = timeline.front().tempo_;
    if (piece_start.convertSecondsToEDUs(timeline.front().start_time_global_) > 0) {
      piece_start.setStartTime(0);
      timeline.insert(timeline.begin(), TimeSignature(piece_start));
    }

    // A staff with no notes, or with no notes yet (such as before the first
    // section), is rests in step with the others
    for (int i = 0; i < staffSum; i++) {
      FillWithRests(score_staff[i], timeline);
    }

    // All staves end together, at the end of the bar that holds the last
    // note of the score: a staff that ends sooner gets rests up to there
    float score_end = 0;
    for (int i = 0; i < staffSum; i++) {
      Section& last = score_staff[i].back();
      score_end = std::max(score_end, last.GetStartTimeGlobal() +
                                      last.CalculateSecondsFromEDUsInTempo(EndOfLastBar(last)));
    }
    vector<int> last_section_edus(score_staff.size(), -1);
    for (int i = 0; i < staffSum; i++) {
      Section& last = score_staff[i].back();
      int edus = last.CalculateEDUsFromSecondsInTempo(score_end - last.GetStartTimeGlobal());
      if (edus > EndOfLastBar(last)) {
        last_section_edus[i] = edus;
      }
    }

    // A section that several staves carry is warned about once
    vector< pair<float, float> > warned_extensions;

    for(int i=0 ; i<staffSum; i++){
      try {
      // Since all tempos are registered, calculate their start times 
      // in terms of the previous tempo's EDU's
      vector<Section>::iterator iter = score_staff[i].begin();
      vector<Section>::iterator next = score_staff[i].begin() + 1;
      string previous_time_signature;
      bool first_section = true;    
      // Each staff starts with no dynamic and carries it across its sections
      string staff_loudness;

      while (next != score_staff[i].end()) {
        float dur_seconds = next->GetStartTimeGlobal() - iter->GetStartTimeGlobal();
        iter->SetDurationEDUS(iter->CalculateEDUsFromSecondsInTempo(dur_seconds));

        string current_time_signature = iter->GetTimeSignature().time_signature_;
        bool print_time_signature = first_section || current_time_signature != previous_time_signature;
        iter->Build(print_time_signature, staff_loudness);
        WarnCapExtension(*iter, warned_extensions);

        first_section = false;
        // The next section compares with the meter in force after any cap bar
        previous_time_signature = iter->GetEndingTimeSignature().time_signature_;
        ++iter; ++next;
      }

      iter->SetDurationEDUS(last_section_edus[i]);
      string current_time_signature = iter->GetTimeSignature().time_signature_;
      bool print_time_signature = first_section || current_time_signature != previous_time_signature;
      iter->Build(print_time_signature, staff_loudness);
      WarnCapExtension(*iter, warned_extensions);
      } catch (CmodError& error) {
        error.addContext("Score output, staff " + to_string(i));
        throw;
      }
    }

    // In polymeter every staff with a barline where one staff notates a
    // time signature notates its own
    NotateTimeSignaturesTogether(score_staff);
    is_built_ = true;
  }
}

// ostream& operator<<(ostream& output_stream,
//                     NotationScore& notation_score) {
//   if (!notation_score.is_built_) {
//     notation_score.Build();
//   }

//   output_stream << "\\header {\n  title=\"" << notation_score.score_title_ 
//                 << "\"\ncomposer=\"DISSCO\"\n}" << endl;
//   output_stream << "\\new Voice \\with {" << endl;
//   output_stream << "\\remove \"Note_heads_engraver\"" << endl;
//   output_stream << "\\consists \"Completion_heads_engraver\"" << endl;
//   output_stream << "\\remove \"Rest_engraver\"" << endl;
//   output_stream << "\\consists \"Completion_rest_engraver\"" << endl;
//   output_stream << "}" << endl;

//   output_stream << "{" << endl;
//   // Staffs
//   // TimeSignature
//   for (vector<Section>::iterator iter = notation_score.score_.begin();
//        iter != notation_score.score_.end();
//        ++iter) {
//     iter->PrintAllNotesFlat("Final output");
//     list<Note*> section_flat = iter->GetSectionFlat();
//     // for each notes
//     for (list<Note*>::iterator iter_iter = section_flat.begin();
//          iter_iter != section_flat.end();
//          ++iter_iter) {
//       Note*& cur_note = *iter_iter;
//       output_stream << cur_note->GetText();
//     }
    
//     output_stream << '\n';
//   }
  
//   output_stream << "\\bar \"|.\"" << endl;
//   output_stream << "}" << endl;

//   return output_stream;
// }

// multistaffs
ostream& operator<<(ostream& output_stream,
                    NotationScore& notation_score) {
  if (!notation_score.is_built_) {
    notation_score.Build();
  }

  output_stream << "\\header {\n  title=\"" << EscapeLilyPondString(notation_score.score_title_)
                << "\"\ncomposer=\"DISSCO\"\n}" << endl;
  output_stream << "\\version \"2.18.2\" " << endl;
  // LilyPond keeps the bars of all staves in the Score context, so staves
  // with different bars take their timing into the Staff context
  if (StavesHaveDifferentBars(notation_score.score_staff)) {
    output_stream << "% Each staff keeps its own time signature and bars. LilyPond 2.24 and later\n"
                  << "% warn that Default_bar_line_engraver is unknown; it is needed up to 2.22.\n"
                  << "\\layout {\n"
                  << "  \\context {\n"
                  << "    \\Score\n"
                  << "    \\remove \"Timing_translator\"\n"
                  << "    \\remove \"Default_bar_line_engraver\"\n"
                  << "  }\n"
                  << "  \\context {\n"
                  << "    \\Staff\n"
                  << "    \\consists \"Timing_translator\"\n"
                  << "    \\consists \"Default_bar_line_engraver\"\n"
                  << "  }\n"
                  << "}" << endl;
  }
  // output_stream << "\\new Voice \\with {" << endl;
  // output_stream << "\\remove \"Note_heads_engraver\"" << endl;
  // output_stream << "\\consists \"Completion_heads_engraver\"" << endl;
  // output_stream << "\\remove \"Rest_engraver\"" << endl;
  // output_stream << "\\consists \"Completion_rest_engraver\"" << endl;
  // output_stream << "}" << endl;

  // Staffs
  if(notation_score.is_grand){
    output_stream << "\\new GrandStaff " << endl;
  }
  output_stream << " << " << endl;
  for(int i=0;i<notation_score.staffSum; i++){
    output_stream << "\\new Staff" << endl;
    output_stream << "{" << endl;
    // TimeSignature
    if(!notation_score.score_staff[i].empty()){
      // A section with only rests on this staff keeps the clef in force.
      // Rests before the staff's first notes take the clef of those notes;
      // a staff without notes takes bass for the left hand of a grand
      // staff, else treble.
      string clef = (notation_score.is_grand && i == 1) ? "bass" : "treble";
      for (vector<Section>::iterator iter = notation_score.score_staff[i].begin();
        iter != notation_score.score_staff[i].end();
        ++iter) {
        string first_clef = ChooseClef(iter->GetSectionFlat());
        if (!first_clef.empty()) {
          clef = first_clef;
          break;
        }
      }

      // The top staff shows the tempo at the start of the score and
      // wherever it changes; every staff has a section wherever the tempo
      // can change (Build fills the time a staff has no notes with rests)
      Rational<long long> tempo_in_force; // 0: no tempo yet
      string mark_in_force;

      for (vector<Section>::iterator iter = notation_score.score_staff[i].begin();
        iter != notation_score.score_staff[i].end();
        ++iter) {
        const list<Note*>& section_flat = iter->GetSectionFlat();
        // for each notes
        string notes_stream=" ";
        for (list<Note*>::const_iterator iter_iter = section_flat.begin();
            iter_iter != section_flat.end();
            ++iter_iter) {
          notes_stream = notes_stream + (*iter_iter)->GetText();
        }
        string section_clef = ChooseClef(section_flat);
        if (!section_clef.empty()) {
          clef = section_clef;
        }
        output_stream << "\\clef " << clef << endl;
        if (i == 0) {
          Tempo tempo = iter->GetTimeSignature().tempo_;
          Rational<long long> whole_notes_per_minute = WholeNotesPerMinute(tempo);
          if (whole_notes_per_minute != tempo_in_force) {
            tempo_in_force = whole_notes_per_minute;
            // Rounding can give a different tempo the mark already in force
            string mark = TempoMark(tempo);
            if (mark != mark_in_force) {
              mark_in_force = mark;
              output_stream << mark << endl;
            }
          }
        }
        output_stream << notes_stream;
      }
      output_stream << "\\bar \"|.\"";
    }
    output_stream << "}" << endl;
  }

  // if(notation_score.is_grand){
  //   output_stream << ">>" << endl;
  // }
  output_stream << ">>" << endl;

  return output_stream;
}

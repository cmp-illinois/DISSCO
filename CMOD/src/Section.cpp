#include "Section.h"
#include "CmodError.h"

#include <numeric>

Note* Section::TimeSignatureMark(const string& time_signature) {
  Note* mark = new Note();
  mark->start_t = 0;
  mark->end_t = 0;
  mark->type_out = "\\time " + time_signature + '\n';
  mark->type = NoteType::kTimeSignature;
  return mark;
}

Section::Section(TimeSignature time_signature) : 
    time_signature_(time_signature),
    is_built_(false),
    is_edu_limit_(true),
    cap_(0),
    cap_extension_seconds_(0) {
  section_ = vector< vector<Note*> >(0);
  section_flat_ = list<Note*>();
  remaining_edus_ = 0;
}

Section::Section(const Section& other) {
  time_signature_ = other.time_signature_;
  added_notes_ = other.added_notes_;
  section_ = other.section_;
  section_flat_ = other.section_flat_;
  is_built_ = other.is_built_;
  remaining_edus_ = other.remaining_edus_;
  is_edu_limit_ = other.is_edu_limit_;
  cap_ = other.cap_;
  cap_extension_seconds_ = other.cap_extension_seconds_;
  beat_tuplets_ = other.beat_tuplets_;
  prev_loudness_ = other.prev_loudness_;
  last_bar_loudness_ = other.last_bar_loudness_;
}

/* For upgrading to c++11
Section::Section(Section&& source) {
  time_signature_ = std::move(source.time_signature_);
  section_ = std::move(source.section_);
  section_flat_ = std::move(source.section_flat_);
  is_built_ = source.is_built_; source.is_built_ = false;
  remaining_edus_ = source.remaining_edus_; source.remaining_edus_ = 0;
  is_edu_limit_ = source.is_edu_limit_; source.is_edu_limit_ = false;
  cap_ = source.cap_; source.cap_ = 0;
}
*/

Section& Section::operator=(const Section& other) {
  time_signature_ = other.time_signature_;
  added_notes_ = other.added_notes_;
  section_ = other.section_;
  section_flat_ = other.section_flat_;
  is_built_ = other.is_built_;
  remaining_edus_ = other.remaining_edus_;
  is_edu_limit_ = other.is_edu_limit_;
  cap_ = other.cap_;
  cap_extension_seconds_ = other.cap_extension_seconds_;
  beat_tuplets_ = other.beat_tuplets_;
  prev_loudness_ = other.prev_loudness_;
  last_bar_loudness_ = other.last_bar_loudness_;
  return *this;
}

/* For upgrading to c++11
Section& Section::operator=(Section&& source) {
  time_signature_ = std::move(source.time_signature_);
  section_ = std::move(source.section_);
  section_flat_ = std::move(source.section_flat_);
  is_built_ = source.is_built_; source.is_built_ = false;
  remaining_edus_ = source.remaining_edus_; source.remaining_edus_ = 0;
  is_edu_limit_ = source.is_edu_limit_; source.is_edu_limit_ = false;
  cap_ = source.cap_; source.cap_ = 0;
  return *this;
}
*/

Section::~Section() {
/*
There's a discussion on memory management to be had here. Currently, notes are dynamically
allocated outside and inside of the Section class. The deallocation is handled right here.
This leads to unnecessary complexity in the Section class and it also necessitates allocating
the end cap (which is also a Section) dynamically on the heap to manually manage its lifetime
so notes don't get deleted before their use is over. Unfortunately, there simply was not enough
time to change this.
*/
  for (vector< vector<Note*> >::iterator iter = section_.begin();
       iter != section_.end();
       ++iter) {
    vector<Note*> bar = *iter;
    bar.clear();
  }
  section_.clear();

  for (list<Note*>::iterator iter = section_flat_.begin();
       iter != section_flat_.end();
       ++iter) {
    Note* note = *iter;
    delete note;
  }
  section_flat_.clear();

  if (cap_) {
    delete cap_;
  }
}
bool Section::IsSectionOf(Tempo& tempo) {
  return time_signature_.tempo_.getRootExactAncestor() == tempo.getRootExactAncestor() &&
         time_signature_.tempo_.isTempoSameAs(tempo);
}

void Section::AddNote(Note* n) {
  added_notes_.push_back(n);
  is_built_ = false;
}

void Section::TakeNotes(Section& later, int offset_edus) {
  for (vector<Note*>::iterator iter = later.added_notes_.begin();
       iter != later.added_notes_.end();
       ++iter) {
    (*iter)->shiftEDUs(offset_edus);
    added_notes_.push_back(*iter);
  }
  later.added_notes_.clear();
  is_built_ = false;
}

void Section::InsertAddedNotes() {
  for (vector<Note*>::iterator iter = added_notes_.begin();
       iter != added_notes_.end();
       ++iter) {
    InsertNote(*iter);
  }
  added_notes_.clear();
}

// This function is rewrote by xiaoyi han
void Section::InsertNote(Note* n) {
  //initialize n
  n->type_out = "";
  n->type = NoteType::kNote;
  is_built_ = false;

  n->prepareForInsertion();
  EnsureNoteExpressible(n);

  if (n->end_t <= n->start_t ) {
	  return; // Discard invalid note
  }

  int bar_num = n->start_t / time_signature_.bar_edus_;

  if (bar_num >= (int)section_.size()) {
    ResizeSection(bar_num);
  }

  // if note is out of a bar time, the part after the barline is a new note
  // that its pitches are tied into. A note that ends on the barline keeps
  // its ties: a split before (at a chord change) may have tied it already.
  const int bar_end = (bar_num + 1) * time_signature_.bar_edus_;
  if (n->end_t > bar_end) {
    Note* second = new Note(*n);
    second->start_t = bar_end;
    n->end_t = bar_end;
    n->tieAllPitches();
    InsertNote(second);
  }
  // now note is completely inside the bar, insert it. Where notes overlap
  // they are cut into chords; each part keeps the ties of the pitches that
  // end with it, and every pitch that sounds on past a part is tied.
  vector<Note*>::iterator iter;
  for ( iter = section_[bar_num].begin(); 
        iter != section_[bar_num].end(); 
        iter++) {
    Note* cur = *iter;
    // if there is no pitch in this bar
    // if there is no overlap
    if(cur->start_t >= n->end_t){
      section_[bar_num].insert(iter, n);
      return;
    }
    // if there is overlap
    else if(cur->start_t == n->start_t){
      // for the same start time
      if(cur->end_t == n->end_t){
        // if the end time is still the same
        // Group the pitches while retaining each event's own modifiers.
        cur->mergePitches(*n);
        return;
      }
      else if (cur->end_t > n->end_t){
        // insertnote has the shorter end time
        // merge the overlap part and insert the left of existnode again
        cur->start_t = n->end_t;
        n->mergePitches(*cur, true, true);
        InsertNote(n);
        return;
      }
      else if(cur->end_t < n->end_t){
        if(cur->start_t != cur->end_t){
          // insertnote has the longer end time and ensure they have the overlap part.
          // merge the overlap part, and insert the left of insernote 
          cur->mergePitches(*n, false, true);
          n->start_t = cur->end_t;
        }
      }
    }
    else if ((cur->end_t > n->start_t) && (cur->start_t < n->start_t)){
      // insertnode has later start time
      if(cur->end_t == n->end_t){
        // if they have the same end time.
        // shorten the existnode and merge the overlap part
        n->mergePitches(*cur, true);
        cur->end_t = n->start_t;
        cur->tieAllPitches();
      }
      else if (cur->end_t > n->end_t){
        // if insertnode ends early
        // shorten the existnode, merge the overlap part, and cut the unoverlap part
        // to a new note and insert them. 
        Note* sec_chord = new Note(*cur);
        cur->end_t = n->start_t;
        cur->tieAllPitches();
        sec_chord->start_t = n->end_t;
        n->mergePitches(*cur, true);
        InsertNote(sec_chord);
        InsertNote(n);
        return;
      }
      else if (cur->end_t < n->end_t){
        // if insertnode ends later.
        Note* sec_chord = new Note(*n);
        sec_chord->end_t = cur->end_t;
        sec_chord->tieAllPitches();
        sec_chord->mergePitches(*cur, true);
        cur->end_t = sec_chord->start_t;
        cur->tieAllPitches();
        n->start_t = sec_chord->end_t;
        InsertNote(sec_chord);
        InsertNote(n);
        return;
      }
    }
    else if ((cur->start_t > n->start_t) && (cur->start_t < n->end_t)){
      // insertnode has earlier start time
      if(cur->end_t == n->end_t){
        // if they ends at the same time
        cur->mergePitches(*n);
        n->end_t = cur->start_t;
        n->tieAllPitches();
        InsertNote(n);
        return;
      }
      else if(cur->end_t > n->end_t){
        // insertnode ends early
        Note* sec_chord = new Note(*cur);
        sec_chord->start_t = n->end_t;
        cur->end_t = n->end_t;
        cur->tieAllPitches();
        InsertNote(n);
        InsertNote(sec_chord);
        return;
      }
      else if(cur->end_t < n->end_t){
        // insertnode ends later
        Note* sec_chord = new Note(*n);
        sec_chord->start_t = cur->end_t;
        n->end_t = cur->end_t;
        n->tieAllPitches();
        InsertNote(n);
        InsertNote(sec_chord);
        return;
      }
      
    }
  }
  // insert the node
  section_[bar_num].insert(iter, n);
}

void Section::SetDurationEDUS(int edus) {
  if (edus == -1) {
    is_edu_limit_ = false;
  }
  remaining_edus_ = edus;
}

int Section::GetLastNoteEnd() const {
  int last_end = 0;
  // A note that does not end after it starts is discarded when inserted
  for (vector<Note*>::const_iterator note = added_notes_.begin();
       note != added_notes_.end();
       ++note) {
    if ((*note)->end_t > (*note)->start_t) {
      last_end = std::max(last_end, (*note)->end_t);
    }
  }
  for (vector< vector<Note*> >::const_iterator bar = section_.begin();
       bar != section_.end();
       ++bar) {
    for (vector<Note*>::const_iterator note = bar->begin();
         note != bar->end();
         ++note) {
      if ((*note)->type == NoteType::kNote) {
        last_end = std::max(last_end, (*note)->end_t);
      }
    }
  }
  return last_end;
}

float Section::GetCapExtensionSeconds() const {
  return cap_extension_seconds_;
}

float Section::GetStartTimeGlobal() const {
  return time_signature_.start_time_global_;
}

TimeSignature Section::GetTimeSignature() const{
    return time_signature_;
} //diyun

int Section::CalculateEDUsFromSecondsInTempo(float seconds) {
  return time_signature_.tempo_.convertSecondsToEDUs(seconds);
}

float Section::CalculateSecondsFromEDUsInTempo(int edus) {
  return time_signature_.tempo_.calculateSecondsFromEDUs(edus);
}

void Section::Build(bool notate_time_signature, string& staff_loudness) {
  if (!is_built_) {
    // Dynamics continue from the end of the previous section on this staff
    prev_loudness_ = staff_loudness;

    if (is_edu_limit_ && remaining_edus_ == 0) {
      throw CmodError(CmodError::Kind::Project,
                      "Two tempo sections start too close together to leave a notatable duration.",
                      "Score section starting at " + to_string(GetStartTimeGlobal()) + " seconds",
                      "Separate the tempo changes by at least one EDU, or use a finer EDU Per Beat setting.");
    }

    section_flat_.clear();

    if (notate_time_signature) {
      section_flat_.push_back(TimeSignatureMark(time_signature_.time_signature_));
    }

    Note* first_barline = new Note();
    first_barline->start_t=0;
    first_barline->end_t=0;
    first_barline->type = NoteType::kBarline;
    section_flat_.push_back(first_barline);

    // Silence up to the next section is written as rests, so the section
    // has bars up to there even where it has no notes
    if (is_edu_limit_) {
      ResizeSection((remaining_edus_ - 1) / time_signature_.bar_edus_);
    }

    AddBars();
    AddRestsAndFlatten();
    Notate();
    if (is_edu_limit_) {
      CapEnding();
    }

    is_built_ = true;
  }
  staff_loudness = prev_loudness_;
}

const list<Note*>& Section::GetSectionFlat() {
  if (is_built_) {
    return section_flat_;
  }

  throw CmodError(CmodError::Kind::Internal,
                  "Score output requested a section before it was built.",
                  "Score section starting at " + to_string(GetStartTimeGlobal()) + " seconds",
                  "Report this error to the DISSCO developers with the project file, seed, and full output.");
}

TimeSignature Section::GetEndingTimeSignature() const {
  return cap_ ? cap_->time_signature_ : time_signature_;
}

vector<string> Section::GetBarTimeSignatures() const {
  // section_ has one entry per bar; the cap bar is written in place of
  // the last one
  vector<string> bars(section_.size(), time_signature_.time_signature_);
  if (cap_ && !bars.empty()) {
    bars.back() = cap_->time_signature_.time_signature_;
  }
  return bars;
}

// Each bar starts with the empty item ResizeSection gives it (the only
// kUnknown items in section_flat_), and a time signature notated at the
// start of a bar comes before that item
vector<bool> Section::GetBarTimeSignatureMarks() const {
  vector<bool> marks;
  bool mark = false;
  for (list<Note*>::const_iterator iter = section_flat_.begin();
       iter != section_flat_.end();
       ++iter) {
    if ((*iter)->type == NoteType::kTimeSignature) {
      mark = true;
    } else if ((*iter)->type == NoteType::kUnknown) {
      marks.push_back(mark);
      mark = false;
    }
  }
  return marks;
}

void Section::NotateBarTimeSignature(size_t bar) {
  if (GetBarTimeSignatureMarks().at(bar)) {
    return;
  }
  size_t bar_index = 0;
  for (list<Note*>::iterator iter = section_flat_.begin();
       iter != section_flat_.end();
       ++iter) {
    if ((*iter)->type == NoteType::kUnknown && bar_index++ == bar) {
      section_flat_.insert(iter, TimeSignatureMark(GetBarTimeSignatures().at(bar)));
      return;
    }
  }
}

bool Section::operator<(const TimeSignature& time_signature) const {
  return time_signature_.start_time_global_ < time_signature.start_time_global_;
}

bool Section::operator>(const TimeSignature& time_signature) const {
  return time_signature_.start_time_global_ > time_signature.start_time_global_;
}
  
bool Section::operator==(const TimeSignature& time_signature) const {
  return time_signature_ == time_signature;
}

bool Section::operator!=(const TimeSignature& time_signature) const {
  return !operator==(time_signature);
}

void Section::EnsureNoteExpressible(Note* n) {
  int dur = n->end_t % time_signature_.beat_edus_;
  int min_diff = time_signature_.beat_edus_;
  bool note_needs_chop = true;

  for (unsigned i = 0; i < time_signature_.valid_dividers_.size(); i++) {
    if (dur % time_signature_.valid_dividers_[i] == 0) {
      note_needs_chop = false;
      break;
    } else {
      if (min_diff > dur % time_signature_.valid_dividers_[i]) {
        min_diff = dur % time_signature_.valid_dividers_[i];
      }
    }
  }

  if (note_needs_chop) {
    n->end_t -= min_diff;
  }
}

void Section::ResizeSection(int new_size) {
  for(int bar_idx = static_cast<int>(section_.size()); bar_idx <= new_size; ++bar_idx) {
    vector<Note*> bar = vector<Note*>(0);
    Note* n = new Note();
    n->start_t = time_signature_.bar_edus_ * bar_idx;
    n->end_t = time_signature_.bar_edus_ * bar_idx;
    n->type_out = " ";
    n->type = NoteType::kUnknown;
    bar.push_back(n);
    section_.push_back(bar);
  }
}

void Section::AddBars() {
  int bar_idx = 1;
  for (vector< vector<Note*> >::iterator iter = section_.begin();
       iter != section_.end();
       ++iter) {
    vector<Note*>& bar = *iter;
    Note* n = new Note();
    n->start_t = time_signature_.bar_edus_ * bar_idx;
    n->end_t = time_signature_.bar_edus_ * bar_idx;
    n->type_out = "\\bar\"|\" \n";
    n->type = NoteType::kBarline;
    bar.push_back(n);
    ++bar_idx;
  }
}

void Section::AddRestsAndFlatten() {
  for(size_t i = 0; i < section_.size(); ++i) {
    vector<Note*>::iterator it;
    Note* prev = *(section_[i].begin());
    section_flat_.push_back(prev);
    Note* cur;

    for (it = section_[i].begin() + 1; it != section_[i].end(); it++) {
      cur = *it;
      int gap = cur->start_t - prev->end_t;

      // a rest will be placed only if gap is more than half of the
      // smallest non-zero valid duration
      if(gap > (time_signature_.beat_edus_ / (time_signature_.tuplet_limit_ - 1) / 2)) {
        Note* rest = new Note();
        rest->start_t = prev->end_t;
        rest->end_t = cur->start_t;
        rest->pitch_out = "r";
        rest->type = NoteType::kRest;
        section_flat_.push_back(rest);
      } else {
        if(it+1 == section_[i].end()){
          prev->end_t = cur->start_t; // Can't shorten current note because end of bar
        } else {
          cur->adjustStartTime(prev->end_t);
        }
      }
      section_flat_.push_back(cur);
      prev = cur;
    }
  }
}

void Section::DetermineBeatTuplets() {
  const int beat_edus = time_signature_.beat_edus_;
  // For each beat, the greatest common divisor of the beat and every
  // position inside it where a sound or silence starts or ends
  map<int, int> beat_grids;

  for (list<Note*>::iterator it = section_flat_.begin();
       it != section_flat_.end();
       ++it) {
    Note* cur = *it;
    if (cur->type != NoteType::kNote && cur->type != NoteType::kRest) {
      continue;
    }

    const int boundaries[] = {cur->start_t, cur->end_t};
    for (int boundary : boundaries) {
      int offset = boundary % beat_edus;
      if (offset != 0) {
        int beat = boundary / beat_edus;
        map<int, int>::iterator grid = beat_grids.find(beat);
        int beat_grid = (grid == beat_grids.end()) ? beat_edus : grid->second;
        beat_grids[beat] = std::gcd(beat_grid, offset);
      }
    }
  }

  // The tuplet whose notes are one grid step long fits every position
  beat_tuplets_.clear();
  for (map<int, int>::iterator grid = beat_grids.begin();
       grid != beat_grids.end();
       ++grid) {
    beat_tuplets_[grid->first] = time_signature_.DetermineTuplet(grid->second);
  }
}

void Section::Notate() {
  int prev_tuplet = 0; // the previous tuplet type
  int tuplet_dur = 0; // the current tuplet duration in edus

  DetermineBeatTuplets();

  for (list<Note*>::iterator it = section_flat_.begin();
       it != section_flat_.end();
       ++it) {

    list<Note*>::iterator next_it = it;
    ++next_it;  // valid: increment current iterator once to get next/end

    Note* cur = *it;

    // CapEnding re-notates the last bar (the one after the last barline
    // that is not the final item), so remember the dynamic in force there
    if (cur->type == NoteType::kBarline && next_it != section_flat_.end()) {
      last_bar_loudness_ = prev_loudness_;
    }

    // adjust note duration according to the tuplet type
    if(tuplet_dur > 0) {
      int duration = cur->end_t - cur->start_t;
      int dur_remainder = duration % time_signature_.beat_edus_;
      int dur_beats = duration / time_signature_.beat_edus_;
      int desire_type = prev_tuplet;
      int cur_type = time_signature_.DetermineTuplet(dur_remainder);
      int excess_tuplet_type = time_signature_.DetermineTuplet(duration - tuplet_dur);

      // True if the note exceeds the current tuplet duration
      // but the excess is expressible in another tuplet
      bool note_is_valid = (duration > tuplet_dur) && (excess_tuplet_type != -1);

      // if the current note's duration cannot be fitted in the tuplet
      // find the closest value and change the end time of the note.
      // Also change the start time of the next note
      if ((desire_type != cur_type) && !note_is_valid) {
        int t = time_signature_.beat_edus_ / desire_type;
        double a = (double)dur_remainder / (double)t;
        int best_fit = (int) round(a) * t;

        cur->end_t = cur->start_t + dur_beats * time_signature_.beat_edus_ + best_fit;

        if (next_it != section_flat_.end()) {
          Note* next_note = *next_it;
          if (next_note != 0) {
            next_note->adjustStartTime(
                cur->start_t + dur_beats * time_signature_.beat_edus_ + best_fit);
          }
        }
      }
    }

    // force the closing of the tuplet before the bar line
    if(cur->type_out == "\\bar\"|\" \n" || cur->type_out == " "){
      if(tuplet_dur > 0){
        cur->type_out = "} " + cur->type_out;
        tuplet_dur = 0;
      }
      continue;
    }

    cur->beginNotation();
    tuplet_dur = NotateCurrentNote(cur, &prev_tuplet, tuplet_dur);
  }
}

void Section::CapEnding() {
  // The end of the last sound in this section (section_flat_ is in time
  // order); -1 if the section has only rests on this staff
  int used_edus = -1;
  for (list<Note*>::reverse_iterator iter = section_flat_.rbegin();
       iter != section_flat_.rend();
       ++iter) {
    if ((*iter)->type == NoteType::kNote) {
      used_edus = (*iter)->end_t;
      break;
    }
  }

  if (used_edus > remaining_edus_) {
    throw CmodError(CmodError::Kind::Project,
                    "Notes extend " + to_string(used_edus - remaining_edus_) + " EDU past the next tempo section.",
                    "Score section starting at " + to_string(GetStartTimeGlobal()) + " seconds",
                    "Shorten the preceding notes or move the next tempo change later so score sections do not overlap.");
  }

  // Build gave the section bars up to the next section, so only the last
  // bar, if the next section starts before it is full, needs capping
  const int total_edus_to_use = remaining_edus_ % time_signature_.bar_edus_;
  if (total_edus_to_use == 0) {
    return; // Sections align perfectly!
  }
  const int cap_start = remaining_edus_ - total_edus_to_use; // the last bar's start

  list<Note*> last_bar = PopLastBarNotes();

  // The cap is the shortest dyadic time signature that holds the rest of
  // the bar. The unsplit beat is always the first candidate. Establish its
  // signature before searching smaller dyadic beats for a closer fit.
  int pow_2 = 1;
  int best_pow_2 = 0;
  int ts_num = total_edus_to_use / time_signature_.beat_edus_;
  int ts_den = time_signature_.unit_note_;
  const int remainder = total_edus_to_use % time_signature_.beat_edus_;
  int min_err = 0;
  if (remainder != 0) {
    ++ts_num;
    min_err = time_signature_.beat_edus_ - remainder;
  }
  while (min_err != 0 && time_signature_.beat_edus_ % TimeSignature::Power(2, pow_2) == 0) {
    int tmp_beat_edus = time_signature_.beat_edus_ / TimeSignature::Power(2, pow_2);
    if (total_edus_to_use % tmp_beat_edus == 0) {
      ts_num = total_edus_to_use / tmp_beat_edus;
      ts_den = time_signature_.unit_note_ * TimeSignature::Power(2, pow_2);
      min_err = 0;
      best_pow_2 = pow_2;
      break; // Overhanging time forms a dyadic time signature
    } else {
      // Form a dyadic time signature by adding sound or rest with the least error
      int num_beats = (total_edus_to_use / tmp_beat_edus) + 1; // TODO - do this better
      int err = tmp_beat_edus * num_beats - total_edus_to_use;
      if (err < min_err) {
        ts_num = num_beats; // Add time
        ts_den = time_signature_.unit_note_ * TimeSignature::Power(2, pow_2);
        min_err = err;
        best_pow_2 = pow_2;
      }
    }
    ++pow_2;
  }

  int beat_divisor = TimeSignature::Power(2, best_pow_2);
  Tempo new_tempo(time_signature_.tempo_);

  new_tempo.setEDUPerTimeSignatureBeat(time_signature_.beat_edus_ / beat_divisor);
  new_tempo.setTimeSignature(Note::int_to_str(ts_num) + "/" + Note::int_to_str(ts_den));
  // IMPORTANT - Tempo rate (i.e. 1/4=60bpm) is left the same
  // IMPORTANT - Tempo start time left unchanged because the calculation is not necessary

  cap_ = new Section(TimeSignature(new_tempo));
  cap_->SetDurationEDUS(-1);
  cap_->ResizeSection(0); // the cap bar is written even if it is all silence

  // The leftover time does not fill a time signature, so the cap bar is
  // lengthened: a sound still going at the end of the section is held
  // through the added time (otherwise it is a rest). The note itself is
  // lengthened, rather than tied to an added copy, so it is written as one
  // value where it can be.
  Note* last_note = last_bar.empty() ? 0 : last_bar.back();
  if (min_err != 0 && last_note != 0 && used_edus == remaining_edus_) {
    last_note->end_t += min_err;
  }

  while (!last_bar.empty()) {
    // Make the cap start from 0 at the start of the bar, so silence before
    // the first note stays, while preserving the original attack of every
    // individual pitch in a grouped chord.
    last_bar.front()->shiftEDUs(-cap_start);

    cap_->InsertNote(last_bar.front());
    last_bar.pop_front();
  }

  // NotationScore::Build warns about the extension, once for all staves
  if (min_err != 0) {
    cap_extension_seconds_ = new_tempo.calculateSecondsFromEDUs(min_err);
  }

  // Only notate time signature if different
  const bool notate_cap_time_signature = cap_->time_signature_ != time_signature_;
  if (notate_cap_time_signature && cap_start == 0) {
    // The whole section is the cap bar, whose time signature replaces the section's
    list<Note*>::iterator iter = section_flat_.begin();
    while (iter != section_flat_.end()) {
      if ((*iter)->type == NoteType::kTimeSignature) {
        delete *iter;
        iter = section_flat_.erase(iter);
      } else {
        ++iter;
      }
    }
  }

  // The cap bar continues the dynamics from where its notes' bar starts
  prev_loudness_ = last_bar_loudness_;
  cap_->Build(notate_cap_time_signature, prev_loudness_);

  // The cap bar, with the barline that closes it, ends this section
  section_flat_.splice(section_flat_.end(), cap_->section_flat_);
}

int Section::NotateCurrentNote(Note* current_note, 
                               int* prev_tuplet, 
                               int tuplet_dur) {
  int dur = current_note->end_t - current_note->start_t;
  if (dur == 0) {
    return tuplet_dur;
  }

  int remaining_dur = dur;
  if (tuplet_dur > 0) {
    remaining_dur = FillCurrentTupletDur(current_note, *prev_tuplet, tuplet_dur);
    if (remaining_dur < 0) { // Note fits inside the current tuplet
      return -remaining_dur; // Tuplet partially filled
    }
    tuplet_dur = 0; // Tuplet now filled
  }

  remaining_dur = FillCompleteBeats(current_note, remaining_dur);

  if (remaining_dur > 0) {
    tuplet_dur = CreateTupletWithRests(current_note, prev_tuplet, remaining_dur);
  }

  return tuplet_dur;
}

int Section::FillCurrentTupletDur(Note* current_note, 
                                        int prev_tuplet, 
                                        int tuplet_dur) {
  int dur = current_note->end_t - current_note->start_t;
  // if the current duration is less than the duration of the tuplet,
  // the entire duration will be inserted in the tuplet and the tuplet will
  // be completed by the next sound or silence.
  if (dur < tuplet_dur) {
    NoteInTuplet(current_note, prev_tuplet, dur);
    // The whole note is written; tie its last value where it continues
    current_note->writeEndTie();
    return dur - tuplet_dur;
  }
  // even if the previous tuplet is an eighth note or sixteenth note,
  // it is still necessary to split part of the current note.
  if (prev_tuplet == 2 || prev_tuplet == 4) {
    int unit = tuplet_dur / (time_signature_.beat_edus_ / prev_tuplet);
    if(unit == 3) {
      string s = Note::int_to_str(time_signature_.unit_note_ * 2);
      current_note->writeNextPitch();
      current_note->type_out += s + ".";
      LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
    } else {
      string s = Note::int_to_str(time_signature_.unit_note_ * prev_tuplet / unit);
      current_note->writeNextPitch();
      current_note->type_out += s;
      LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
    }
    if ((dur > tuplet_dur) && (current_note->pitch_out != "r")) {
      current_note->type_out += "~ ";
    } else {
      current_note->writeEndTie();
      current_note->type_out += " ";
    }

  } else {
    // if the current sound completes the tuplet use the LilyPond symbol and
    // close the tuplet
    NoteInTuplet(current_note, prev_tuplet, tuplet_dur);
    if ((dur > tuplet_dur) && (current_note->pitch_out != "r")) {
      current_note->type_out += "~ ";
    } else {
      current_note->writeEndTie();
    }
    // a power-of-2 division of the beat is plain note values, with no tuplet to close
    if (!TimeSignature::IsPowerOf2(prev_tuplet)) {
      current_note->type_out += "} ";
    }
  }

  return dur - tuplet_dur;
}

int Section::FillCompleteBeats(Note* current_note, int remaining_dur) {
  int remainder = remaining_dur % time_signature_.beat_edus_;
  int mainDur = remaining_dur / time_signature_.beat_edus_;
  // CreateTupletWithRests writes the remainder only if it fits a tuplet
  bool remainder_follows = (remainder > 0) &&
                           (time_signature_.DetermineTuplet(remainder) != -1);
  // The longest value is a whole note (unit_note_ beats); each pass writes at
  // least one beat, which needs a power-of-2 unit note (1 = whole-note beat)
  const int max_power_of_2 = TimeSignature::DiscreteLog2(time_signature_.unit_note_);
  if (mainDur > 0 && max_power_of_2 < 0) {
    ThrowUnnotatableBeat(time_signature_.unit_note_);
  }
  // LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
  while (mainDur > 0) {
    int power_of_2 = max_power_of_2;
    while (power_of_2 >= 0) {
      int beats = TimeSignature::Power(2, power_of_2);
      if (mainDur >= beats) {
        current_note->writeNextPitch();
        current_note->type_out += Note::int_to_str(time_signature_.unit_note_ / beats);
        // The dot has to be right after the duration, before the dynamic mark
        mainDur -= beats;
        if (mainDur >= beats / 2 && beats >= 2){
          current_note->type_out += ".";
          mainDur -= beats / 2;
        }
        LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
        break;
      }
      power_of_2--;
    }

    // LoudnessMark(current_note);

    // Every written value is tied to the next one of the same note, and the
    // last value is tied where the note continues past this fragment
    if (mainDur > 0 || remainder_follows) {
      if (current_note->pitch_out != "r") {
        current_note->type_out += "~ ";
      }
    } else {
      current_note->writeEndTie();
      current_note->type_out += " ";
    }
  }

  // int last = current_note->type_out.size() - 1;
  // while (current_note->type_out[last] == ' ') {
  //   current_note->type_out = current_note->type_out.substr(0, last - 1);
  //   last--;
  // }
  // if (current_note->type_out[last] == '~') {
  //   current_note->type_out = current_note->type_out.substr(0, last - 1);
  //   current_note->type_out += " ";
  // }

  return remainder;
}

int Section::CreateTupletWithRests(Note* current_note, 
                                         int* prev_tuplet,
                                         int remaining_dur) {
  int tuplet_dur = 0;

  // The remainder starts a beat, whose tuplet has to fit the sounds and
  // silences that complete the beat as well
  int beat = (current_note->end_t - remaining_dur) / time_signature_.beat_edus_;
  map<int, int>::const_iterator beat_tuplet = beat_tuplets_.find(beat);
  int tuplet_type = (beat_tuplet != beat_tuplets_.end()) ?
      beat_tuplet->second : time_signature_.DetermineTuplet(remaining_dur);
  if (tuplet_type == -1) {
    throw CmodError(CmodError::Kind::Internal,
                    "A duration of " + to_string(remaining_dur) + " EDU cannot be written as a division of a " +
                    to_string(time_signature_.beat_edus_) + "-EDU beat.",
                    "Score section starting at " + to_string(GetStartTimeGlobal()) + " seconds",
                    "Report this error to the DISSCO developers with the project file, seed, and full output.");
  }

  if (TimeSignature::IsPowerOf2(tuplet_type)) {
    // A power-of-2 division of the beat needs no tuplet: write the
    // remainder in the simplest plain note values that express it
    int plain_type = time_signature_.DetermineTuplet(remaining_dur);
    if (plain_type == 2 || plain_type == 4) {
      if (remaining_dur / (time_signature_.beat_edus_ / plain_type) == 3) {
        string s = Note::int_to_str(time_signature_.unit_note_ * 2);
        current_note->writeNextPitch();
        current_note->type_out += s + ". ";
        LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
      } else {
        string s = Note::int_to_str(time_signature_.unit_note_ * plain_type);
        current_note->writeNextPitch();
        current_note->type_out += s + " ";
        LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
      }
    } else {
      NoteInTuplet(current_note, plain_type, remaining_dur);
    }
  } else {
    current_note->type_out += time_signature_.tuplet_types_[tuplet_type];
    NoteInTuplet(current_note, tuplet_type, remaining_dur);
  }
  tuplet_dur = time_signature_.beat_edus_ - remaining_dur;
  current_note->writeEndTie();
  *prev_tuplet = tuplet_type;
  return tuplet_dur;
}

void Section::NoteInTuplet(Note* current_note, int tuplet_type, int duration) {
  bool first = true;

  int beat = duration / (time_signature_.beat_edus_ / tuplet_type); // working in tuplet beats
  int unit_in_tuplet = time_signature_.unit_note_ * TimeSignature::CalculateNearestPow2(tuplet_type);
  // Each pass writes at least one tuplet beat, which needs a power-of-2 unit
  const int max_power_of_2 = TimeSignature::DiscreteLog2(unit_in_tuplet);
  if (beat > 0 && max_power_of_2 < 0) {
    ThrowUnnotatableBeat(unit_in_tuplet);
  }
  while (beat > 0){

    int power_of_2 = max_power_of_2;
    while(power_of_2 >= 0){
      int beats = TimeSignature::Power(2, power_of_2);
      if(beat >= beats){
        current_note->writeNextPitch();
        current_note->type_out += Note::int_to_str(unit_in_tuplet / beats);
        //Rubin Du 2024: The dot has to be right after duration but not after modifier
        beat -= beats;
        if(beat >= beats/2 && beats >= 2){
          current_note->type_out += ".";
          beat -= beats/2;
        }
        LoudnessMark(current_note);  // this places all the dynamic marks at the beginning of a sound
        break;
      }
      power_of_2--;
    }

    // Tie to the next value of the same note; the caller ties the last value
    if ((beat > 0) && (current_note->pitch_out != "r")) {
      current_note->type_out += "~ ";
    } else {
      current_note->type_out += " ";
    }

    if(first == true){
      LoudnessMark(current_note);
      first = false;
    }
  }
}

void Section::LoudnessMark(Note* current_note) {
  if (current_note->loudness_out != prev_loudness_ && 
      current_note->pitch_out != "r") {
  	current_note->type_out += current_note->loudness_out + " ";
  	prev_loudness_ = current_note->loudness_out;
  }
}

void Section::ThrowUnnotatableBeat(int unit_note) const {
  throw CmodError(CmodError::Kind::Internal,
                  "Time signature " + time_signature_.time_signature_ + " needs a 1/" +
                  to_string(unit_note) + " note value, which is not a power of two.",
                  "Score section starting at " + to_string(GetStartTimeGlobal()) + " seconds",
                  "Report this error to the DISSCO developers with the project file, seed, and full output.");
}

list<Note*> Section::PopLastBarNotes() {
  list<Note*> last_bar(0);

  Note* last_barline = 0;
  list<Note*>::iterator note_iter = section_flat_.begin();
  int num_items_in_bar = 0;
  for (; note_iter != section_flat_.end(); ++note_iter) {
    list<Note*>::iterator next = note_iter;
    ++next;
    Note* note = *note_iter;

    // The last bar starts after the last barline that does not end the
    // section; that barline stays, as it closes the bar before
    if (next != section_flat_.end() && 
        note->type == NoteType::kBarline) {
      num_items_in_bar = 0;
      last_barline = note;
      last_bar.clear();
      continue;
    }

    if (note->type == NoteType::kNote) {
      last_bar.push_back(note);
    }

    ++num_items_in_bar;
  }

  if (last_barline == 0) {
    throw CmodError(CmodError::Kind::Project,
                    "No notatable bar was generated before a tempo transition.",
                    "Score section starting at " + to_string(GetStartTimeGlobal()) + " seconds",
                    "Check that the section contains notes with positive durations compatible with its EDU and time signature settings.");
  }

  // Remove the last bar from section_flat_
  while (num_items_in_bar > 0) {
    if (section_flat_.back()->type != NoteType::kNote) {
      delete section_flat_.back(); // not returned so they have to be deleted here
    }
    section_flat_.pop_back();
    --num_items_in_bar;
  }

  return last_bar;
}

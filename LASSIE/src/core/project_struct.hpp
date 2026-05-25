#ifndef PROJECTSTRUCT_HPP
#define PROJECTSTRUCT_HPP

#include <QString>
#include <QList>
#include <QMap>
#include <QHash>
#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

#include "event_struct.hpp"

/**
 * @file project_struct.hpp
 * @brief In-memory model of a DISSCO project and its top-level manager.
 *
 * Two classes live here:
 *  - Project: a value-like aggregate of everything a `.dissco` file holds —
 *    global synthesis settings, the event hierarchy (top/high/mid/low/bottom
 *    plus spectrum, envelope, sieve, spatialization, pattern, reverb,
 *    filter, note events), the envelope library, and any Markov models.
 *  - ProjectManager: the single entry point that creates, opens, saves, and
 *    closes Project instances. UI code never touches Project members
 *    directly; it goes through ProjectManager's typed accessors.
 *
 * Project's data members are private and all UI access flows through
 * ProjectManager. Adding a new field to a DISSCO project therefore means
 * extending: this struct, ProjectManager's accessor list, the XML parser in
 * project_struct.cpp, and the save routine.
 */


class ProjectManager;
class EnvelopeLibraryEntry;
template<typename T>
class MarkovModel;

/**
 * @brief Aggregate state for a single open DISSCO project.
 *
 * All members are private; ProjectManager is the only friend and is the
 * intended access path for the rest of LASSIE. Construction is restricted —
 * client code goes through `ProjectManager::create()` /
 * `ProjectManager::open()` rather than instantiating Project directly.
 */
class Project : public QObject {
    friend class ProjectManager;

    Q_OBJECT
    
    public: 
        /* to be called by ProjectView */
        void showContents();
        void hideContents();
    private:
        /**
         *  This function constructs a Project object that will represent the data contained in a DISSCO project
         *	\param _title The title of the project, default initialization to empty QString
         *  \param _id The UUID corresponding to this project, default initialization to empty QByteArray
         *  \returns 
        **/
        Project(const QString& _title = QString(), const QByteArray& _id = QByteArray());
        void parseEvent(QXmlStreamReader& reader);
        /* the *.dissco file */
        QFileInfo fileinfo;
        QByteArray id;

        /* properties */
        QString title; /* file name */
        QString file_flag = "THMLBsnv";
        QString start_time = "0";
        QString duration = "";
        QString num_channels = "2";
        QString sample_rate = "44100";
        QString sample_size = "16";
        QString num_threads = "1";
        QString num_staffs = "1";
        QString dat_path; /* excl filename */
        QString lib_path; /* incl filename */
        QString seed = "";
        QString measure;

        bool modifiedButNotSaved = true;

        /* flags for CMOD */
        bool grand_staff = false;
        bool score = false;
        bool synthesis = true;
        bool output_particel = false;
        bool empty_project = false;

        /* event details */
        HEvent top_event;
        QList<HEvent> high_events;
        QList<HEvent> mid_events;
        QList<HEvent> low_events;
        QList<BottomEvent> bottom_events;
        QList<SpectrumEvent> spectrum_events;
        QList<NoteEvent> note_events;
        QList<EnvelopeEvent> envelope_events;
        QList<SieveEvent> sieve_events;
        QList<SpaEvent> spa_events;
        QList<PatternEvent> pattern_events;
        QList<ReverbEvent> reverb_events;
        QList<FilterEvent> filter_events;

        EnvelopeLibraryEntry *elentry = nullptr;
        QList<MarkovModel<float>> markov_models;

        /* list of custom note modifiers, per user */
        QList<QString> custom_note_modifiers;
        /* dict of known default modifiers */
        QMap<QString, bool> default_note_modifiers{
            {"+8va", true},          {"-8va", true},
            {"accent", true},        {"coda", true},
            {"downbow", true},       {"downmordent", true},
            {"downprall", true},     {"espressivo", true},
            {"fermata", true},       {"flageolet", true},
            {"halfopen", true},      {"lheel", true},
            {"lineprall", true},     {"longfermata", true},
            {"ltoe", true},          {"marcato", true},
            {"mordent", true},       {"null", true},
            {"open", true},          {"portato", true},
            {"prall", true},         {"pralldown", true},
            {"prallmordent", true},  {"prallprall", true},
            {"prallup", true},       {"reverseturn", true},
            {"rheel", true},         {"rtoe", true},
            {"segno", true},         {"shortfermata", true},
            {"snappizzicato", true}, {"staccatissimo", true},
            {"staccato", true},      {"stopped", true},
            {"tenuto", true},        {"trill", true},
            {"turn", true},          {"upbow", true},
            {"upmordent", true},     {"upprall", true},
            {"varcoda", true},       {"verylongfermata", true}};
};

/**
 * @brief Owner and broker for all open Project instances.
 *
 * ProjectManager presents a thin façade over the currently-open Project.
 * Most accessors return a reference into `curr_project_` so the UI can bind
 * widgets directly to model state; `markModified()` then emits
 * `dataModified()` so dependent windows refresh.
 *
 * Lifecycle methods (create / open / build / save / saveAs / close) are the
 * only ones that allocate or destroy a Project — UI code must never `new`
 * or `delete` Project itself.
 *
 * @warning Every accessor that returns a Project member dereferences
 *          `curr_project_` without a null check. Callers must ensure
 *          `get_curr_project() != nullptr` before using them.
 */
class ProjectManager : public QObject {
    Q_OBJECT

    public:
        ProjectManager(){
            curr_project_ = nullptr;
        }

        /// Create a fresh in-memory Project; not yet bound to a file on disk.
        Project* create(const QString& title = QString(), const QByteArray& id = QByteArray());
        /// Validate and open a `.dissco` file from disk, returning a new Project.
        Project* open(const QString& filepath, const QByteArray& id = QByteArray());
        /// Build a Project from an arbitrary file path (e.g. import path).
        Project* build(const QString& filepath, const QByteArray& id = QByteArray());
        /// Populate @p p from the XML found in the file at @p filepath.
        void parse(Project*, const QString&);
        /// Tear down @p p and free its resources.
        void close(Project*);
        /// Serialize @p p to its existing on-disk path. Returns 0 on success.
        int save(Project*);
        /// Prompt for a new path and serialize @p p there. Returns 0 on success.
        int saveAs(Project*);
        
        void set_curr_project(class Project* p) { curr_project_ = p; }
        Project *get_curr_project() { return curr_project_; }

        QFileInfo& fileinfo()        { return curr_project_->fileinfo; }
        // ALL GETTERS ASSUME THAT THERE IS A CURR_PROJECT!
        QString& title()            { return curr_project_->title; }
        QString& fileflag()         { return curr_project_->file_flag; }
        QString& starttime()        { return curr_project_->start_time; }
        QString& duration()         { return curr_project_->duration; }
        QString& numchannels()      { return curr_project_->num_channels; }
        QString& samplerate()       { return curr_project_->sample_rate; }
        QString& samplesize()       { return curr_project_->sample_size; }
        QString& numthreads()       { return curr_project_->num_threads; }
        QString& numstaffs()        { return curr_project_->num_staffs; }
        QString& seed()             { return curr_project_->seed; }
        QString& measure()          { return curr_project_->measure; }

        bool& modified()            { return curr_project_->modifiedButNotSaved; }
        bool& grandstaff()          { return curr_project_->grand_staff; }
        bool& score()               { return curr_project_->score; }
        bool& synthesis()           { return curr_project_->synthesis; }
        bool& outputparticel()      { return curr_project_->output_particel; }
        bool& emptyproject()        { return curr_project_->empty_project; }

        HEvent& topevent() { return curr_project_->top_event; }
        QList<HEvent>& highevents() { return curr_project_->high_events; }
        QList<HEvent>& midevents() { return curr_project_->mid_events; }
        QList<HEvent>& lowevents() { return curr_project_->low_events; }
        QList<BottomEvent>& bottomevents() { return curr_project_->bottom_events; }
        QList<SpectrumEvent>& spectrumevents() { return curr_project_->spectrum_events; }
        QList<NoteEvent>& noteevents() { return curr_project_->note_events; }
        QList<EnvelopeEvent>& envelopeevents() { return curr_project_->envelope_events; }
        QList<SieveEvent>& sieveevents() { return curr_project_->sieve_events; }
        QList<SpaEvent>& spaevents() { return curr_project_->spa_events; }
        QList<PatternEvent>& patternevents() { return curr_project_->pattern_events; }
        QList<ReverbEvent>& reverbevents() { return curr_project_->reverb_events; }
        QList<FilterEvent>& filterevents() { return curr_project_->filter_events; }

        EnvelopeLibraryEntry*& envlibentries() { return curr_project_->elentry; }
        QList<MarkovModel<float>>& markovmodels() { return curr_project_->markov_models; }
        
        QList<QString>& customnotemodifiers() { return curr_project_->custom_note_modifiers; }

        /**
         * @brief Append a default-constructed event of the given type to the project.
         * @param newEvent  which event family the new entry belongs to
         * @param eventName  user-visible name for the new event
         */
        void addEvent(Eventtype newEvent, QString eventName);
#ifdef TABEDITOR
        QList<Project*> get_projects() { return project_hash_.values(); }
        QList<QByteArray> get_project_IDs() { return project_hash_.keys(); }
#endif

        /// Mark the current project as having unsaved changes and emit dataModified().
        void markModified();

        /// Persist the user-supplied PRNG seed for the active project to disk.
        void writeSeedEntry(const QString& seed) const;

    signals:
        void dataModified();

    private:
#ifdef TABEDITOR
        QHash<QByteArray, Project*> project_hash_;
#endif
        /* the project to be displayed by the bound ProjectView */
        Project *curr_project_;
};

#endif
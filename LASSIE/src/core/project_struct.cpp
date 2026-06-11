/* 
The 'project' object that keeps track of project details to deploy
in the associated window (currently, the project view).
*/
#include "project_struct.hpp"
#include "event_struct.hpp"

#include "../../LASS/src/LASS.h"
#include "EnvelopeLibraryEntry.hpp"

// cmod
#include "MarkovModel.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QProcess>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <sstream>

namespace QtParser {
    /// @brief Capture the inner content of the element currently at StartElement.
    ///   Returns plain text for text-only elements, or serialized inner XML for
    ///   elements containing nested children (e.g. `<Fun>...</Fun>` blocks).
    ///   Pre: r.tokenType() == StartElement.
    ///   Post: r.tokenType() == EndElement of the same element.
    inline QString readInner(QXmlStreamReader& r) {
        QString result;
        QXmlStreamWriter w(&result);
        bool hasChildElement = false;
        QString textBuffer;
        int depth = 0;
        while (!r.atEnd()) {
            r.readNext();
            switch (r.tokenType()) {
                case QXmlStreamReader::StartElement: {
                    hasChildElement = true;
                    w.writeStartElement(r.name().toString());
                    const auto attrs = r.attributes();
                    for (const auto& a : attrs)
                        w.writeAttribute(a.name().toString(), a.value().toString());
                    ++depth;
                    break;
                }
                case QXmlStreamReader::EndElement:
                    if (depth == 0)
                        return hasChildElement ? result : textBuffer;
                    w.writeEndElement();
                    --depth;
                    break;
                case QXmlStreamReader::Characters:
                    if (hasChildElement)
                        w.writeCharacters(r.text().toString());
                    else
                        textBuffer += r.text().toString();
                    break;
                default:
                    break;
            }
        }
        return hasChildElement ? result : textBuffer;
    }

    /// @brief Advance to the next child StartElement and return its inner content.
    ///   Returns an empty QString if no further child element exists in the
    ///   current scope (and leaves r at the parent EndElement).
    inline QString nextChildInner(QXmlStreamReader& r) {
        if (!r.readNextStartElement()) return QString();
        return readInner(r);
    }

    /// @brief Consume any remaining children of the element currently being read,
    ///   advancing to its EndElement.
    inline void consumeRest(QXmlStreamReader& r) {
        while (r.readNextStartElement()) r.skipCurrentElement();
    }

    inline Package parsePackage(QXmlStreamReader& r) {
        // r at <Package> StartElement; consumes through </Package>.
        Package package;
        package.event_name           = nextChildInner(r);
        package.event_type           = nextChildInner(r);
        bool ok = false;
        package.event_type.toInt(&ok);
        if (!ok)
            package.event_type = displayStringToEventtypeString(package.event_type);
        package.weight               = nextChildInner(r);
        package.attack_envelope      = nextChildInner(r);
        package.attackenvelope_scale = nextChildInner(r);
        package.duration_envelope    = nextChildInner(r);
        package.durationenvelope_scale = nextChildInner(r);
        consumeRest(r);
        return package;
    }

    inline Layer parseLayer(QXmlStreamReader& r) {
        // r at <Layer>; first child is <ByLayer>, second is <DiscretePackages>.
        Layer layer;
        if (r.readNextStartElement()) // <ByLayer>
            layer.by_layer = readInner(r);
        if (r.readNextStartElement()) { // <DiscretePackages>
            while (r.readNextStartElement())
                layer.discrete_packages.append(parsePackage(r));
        }
        consumeRest(r);
        return layer;
    }

    inline Modifier parseModifier(QXmlStreamReader& r) {
        Modifier modifier;
        modifier.type                  = nextChildInner(r).toUInt();
        modifier.applyhow_flag         = (nextChildInner(r) == "0");
        modifier.probability           = nextChildInner(r);
        modifier.amplitude             = nextChildInner(r);
        modifier.rate                  = nextChildInner(r);
        modifier.width                 = nextChildInner(r);
        modifier.detune_spread         = nextChildInner(r);
        modifier.detune_direction      = nextChildInner(r);
        modifier.detune_velocity       = nextChildInner(r);
        modifier.group_name            = nextChildInner(r);
        modifier.partialresult_string  = nextChildInner(r);
        consumeRest(r);
        return modifier;
    }

    inline void parseTimeSig(QXmlStreamReader& r, TimeSignature& ts) {
        ts.bar_value  = nextChildInner(r);
        ts.note_value = nextChildInner(r);
        consumeRest(r);
    }

    inline void parseTempo(QXmlStreamReader& r, Tempo& t) {
        t.method_flag = nextChildInner(r).toUInt();
        t.prefix      = nextChildInner(r);
        t.note_value  = nextChildInner(r);
        t.frentry_1   = nextChildInner(r);
        t.frentry_2   = nextChildInner(r);
        t.valentry    = nextChildInner(r);
        consumeRest(r);
    }

    inline void parseNumChildren(QXmlStreamReader& r, NumChildren& n) {
        n.method_flag = nextChildInner(r).toUInt();
        n.entry_1     = nextChildInner(r);
        n.entry_2     = nextChildInner(r);
        n.entry_3     = nextChildInner(r);
        consumeRest(r);
    }

    inline void parseChildEventDef(QXmlStreamReader& r, ChildDef& c) {
        c.entry_1            = nextChildInner(r);
        c.entry_2            = nextChildInner(r);
        c.entry_3            = nextChildInner(r);
        c.attack_sieve       = nextChildInner(r);
        c.duration_sieve     = nextChildInner(r);
        c.definition_flag    = nextChildInner(r).toUInt();
        c.starttype_flag     = nextChildInner(r).toUInt();
        c.durationtype_flag  = nextChildInner(r).toUInt();
        consumeRest(r);
    }

    inline void parseModifiers(QXmlStreamReader& r, QList<Modifier>& out) {
        while (r.readNextStartElement())
            out.append(parseModifier(r));
    }

    /// @brief Parse the shared "HEvent core" children of `<Event>`: from `<EventName>`
    ///   through `<Filter>`. The `<EventType>` child must already have been consumed
    ///   by the caller. Stops at `<Filter>` so callers can read the differing
    ///   trailing siblings (HEvent: `<Modifiers>`; BottomEvent: `<ExtraInfo>`).
    inline void parseHEventCore(QXmlStreamReader& r, HEvent& event) {
        event.name              = nextChildInner(r);   // <Name>
        event.max_child_duration = nextChildInner(r);  // <MaxChildDuration>
        event.edu_perbeat       = nextChildInner(r);   // <EDUPerBeat>

        if (r.readNextStartElement()) parseTimeSig(r, event.timesig);
        if (r.readNextStartElement()) parseTempo(r, event.tempo);
        if (r.readNextStartElement()) parseNumChildren(r, event.numchildren);
        if (r.readNextStartElement()) parseChildEventDef(r, event.child_event_def);

        if (r.readNextStartElement()) { // <Layers>
            while (r.readNextStartElement())
                event.event_layers.append(parseLayer(r));
        }

        event.spa    = nextChildInner(r); // <Spatialization>
        event.reverb = nextChildInner(r); // <Reverb>
        event.filter = nextChildInner(r); // <Filter>
    }

    inline void parseHEventChildren(QXmlStreamReader& r, HEvent& event) {
        parseHEventCore(r, event);
        if (r.readNextStartElement()) // <Modifiers>
            parseModifiers(r, event.modifiers);
        consumeRest(r);
    }

    inline void parseExtraInfo(QXmlStreamReader& r, ExtraInfo& info) {
        if (r.readNextStartElement()) { // <FreqInfo>
            info.freq_info.freq_flag      = nextChildInner(r).toUInt();
            info.freq_info.continuum_flag = nextChildInner(r).toUInt();
            info.freq_info.entry_1        = nextChildInner(r);
            info.freq_info.entry_2        = nextChildInner(r);
            consumeRest(r);
        }
        info.loudness        = nextChildInner(r);
        info.spa             = nextChildInner(r);
        info.reverb          = nextChildInner(r);
        info.filter          = nextChildInner(r);
        info.modifier_group  = nextChildInner(r);
        if (r.readNextStartElement()) // <Modifiers>
            parseModifiers(r, info.modifiers);
        consumeRest(r);
    }

    inline void parseBottomEventChildren(QXmlStreamReader& r, BottomEvent& bev) {
        parseHEventCore(r, bev.event);
        if (r.readNextStartElement()) // <ExtraInfo>
            parseExtraInfo(r, bev.extra_info);
        consumeRest(r);

        QString prefix = bev.event.name.isEmpty() ? QString() : QString(bev.event.name[0]);
        if (prefix == "s")      bev.extra_info.childtype_flag = 0;
        else if (prefix == "n") bev.extra_info.childtype_flag = 1;
        else                    bev.extra_info.childtype_flag = 2;
    }

    inline Spectrum parseSpectrum(QXmlStreamReader& r) {
        Spectrum spectrum;
        while (r.readNextStartElement())
            spectrum.partials.append(readInner(r));
        if (spectrum.partials.size() > 1 && spectrum.partials[0] == "")
            spectrum.partials.removeFirst();
        return spectrum;
    }

    inline void parseSpectrumEventChildren(QXmlStreamReader& r, SpectrumEvent& event) {
        event.name              = nextChildInner(r);
        event.num_partials      = nextChildInner(r);
        event.deviation         = nextChildInner(r);
        event.generate_spectrum = nextChildInner(r);
        if (r.readNextStartElement())
            event.spectrum = parseSpectrum(r);
        consumeRest(r);
    }

    inline NoteInfo parseNoteInfo(QXmlStreamReader& r) {
        NoteInfo ni;
        ni.staffs = nextChildInner(r);
        if (r.readNextStartElement()) { // <Modifiers>
            while (r.readNextStartElement())
                ni.modifiers.append(readInner(r));
        }
        consumeRest(r);
        return ni;
    }

    inline void parseNoteEventChildren(QXmlStreamReader& r, NoteEvent& event) {
        event.name = nextChildInner(r);
        if (r.readNextStartElement())
            event.note_info = parseNoteInfo(r);
        consumeRest(r);
    }

}

void Project::parseEvent(QXmlStreamReader& r) {
    // r at <Event> StartElement (with orderInPalette attribute).
    QString orderinpalette = r.attributes().value("orderInPalette").toString();

    // First child is <EventType>, whose text content is the integer type.
    if (!r.readNextStartElement()) { r.skipCurrentElement(); return; }
    int typeInt = QtParser::readInner(r).toInt();
    Eventtype type = (Eventtype)typeInt;

    switch (type) {
        case top:
        case high:
        case mid:
        case low: {
            HEvent eh;
            eh.orderinpalette = orderinpalette;
            eh.type = type;
            QtParser::parseHEventChildren(r, eh);
            qDebug() << "parsed" << eh.type << "event named" << eh.name;
            switch (type) {
                case top:   top_event = eh; break;
                case high:  high_events.append(eh); break;
                case mid:   mid_events.append(eh); break;
                case low:   low_events.append(eh); break;
                default: break;
            }
            break;
        }
        case bottom: {
            BottomEvent eb;
            eb.event.orderinpalette = orderinpalette;
            eb.event.type = type;
            QtParser::parseBottomEventChildren(r, eb);
            qDebug() << "parsed Bottom event named" << eb.event.name;
            bottom_events.append(eb);
            break;
        }
        case sound: {
            SpectrumEvent espec;
            espec.orderinpalette = orderinpalette;
            QtParser::parseSpectrumEventChildren(r, espec);
            qDebug() << "parsed Spectrum event named" << espec.name;
            spectrum_events.append(espec);
            break;
        }
        case note: {
            NoteEvent en;
            en.orderinpalette = orderinpalette;
            QtParser::parseNoteEventChildren(r, en);
            qDebug() << "parsed Note event named " << en.name;
            note_events.append(en);
            break;
        }
        case env: {
            EnvelopeEvent ee;
            ee.orderinpalette = orderinpalette;
            ee.name             = QtParser::nextChildInner(r);
            ee.envelope_builder = QtParser::nextChildInner(r);
            QtParser::consumeRest(r);
            qDebug() << "parsed Envelope event named" << ee.name;
            envelope_events.append(ee);
            break;
        }
        case sieve: {
            SieveEvent esi;
            esi.orderinpalette = orderinpalette;
            esi.name          = QtParser::nextChildInner(r);
            esi.sieve_builder = QtParser::nextChildInner(r);
            QtParser::consumeRest(r);
            qDebug() << "parsed Sieve event named" << esi.name;
            sieve_events.append(esi);
            break;
        }
        case spa: {
            SpaEvent espa;
            espa.orderinpalette = orderinpalette;
            espa.name           = QtParser::nextChildInner(r);
            espa.spatialization = QtParser::nextChildInner(r);
            QtParser::consumeRest(r);
            qDebug() << "parsed Spa event named" << espa.name;
            spa_events.append(espa);
            break;
        }
        case pattern: {
            PatternEvent ep;
            ep.orderinpalette = orderinpalette;
            ep.name            = QtParser::nextChildInner(r);
            ep.pattern_builder = QtParser::nextChildInner(r);
            QtParser::consumeRest(r);
            qDebug() << "parsed Pattern event named" << ep.name;
            pattern_events.append(ep);
            break;
        }
        case reverb: {
            ReverbEvent er;
            er.orderinpalette = orderinpalette;
            er.name          = QtParser::nextChildInner(r);
            er.reverberation = QtParser::nextChildInner(r);
            QtParser::consumeRest(r);
            qDebug() << "parsed Reverb event named" << er.name;
            reverb_events.append(er);
            break;
        }
        case filter: {
            FilterEvent ef;
            ef.orderinpalette = orderinpalette;
            ef.name           = QtParser::nextChildInner(r);
            ef.filter_builder = QtParser::nextChildInner(r);
            QtParser::consumeRest(r);
            qDebug() << "parsed Filter event named" << ef.name;
            filter_events.append(ef);
            break;
        }
        default:
            qDebug() << "ERROR: parsing event gave event type outside defined types";
            r.skipCurrentElement();
    }
}

void ProjectManager::parse(Project* p, const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ERROR: cannot open" << filepath;
        return;
    }
    QXmlStreamReader r(&file);

    if (!r.readNextStartElement()) return; // <DISSCO> root

    while (r.readNextStartElement()) {
        const QString elName = r.name().toString();

        if (elName == "ProjectConfiguration") {
            // Children: <Title>, <FileFlag>, <TopEvent>, <PieceStartTime>, <Duration>,
            //           <Synthesis>, <Score>, <Grandstaff>, <NumOfStaff>, <NumOfChannels>,
            //           <SampleRate>, <SampleSize>, <NumOfThreads>, <OutputParticel> (optional)
            r.readNextStartElement(); // <Title> — skipped
            QtParser::readInner(r);
            p->file_flag = QtParser::nextChildInner(r);
            QtParser::nextChildInner(r); // <TopEvent> — always "0"
            p->top_event.name = "0";
            p->start_time   = QtParser::nextChildInner(r);
            p->duration     = QtParser::nextChildInner(r);
            p->synthesis    = (QtParser::nextChildInner(r) == "True");
            p->score        = (QtParser::nextChildInner(r) == "True");
            p->grand_staff  = (QtParser::nextChildInner(r) == "True");
            p->num_staffs   = QtParser::nextChildInner(r);
            p->num_channels = QtParser::nextChildInner(r);
            p->sample_rate  = QtParser::nextChildInner(r);
            p->sample_size  = QtParser::nextChildInner(r);
            p->num_threads  = QtParser::nextChildInner(r);
            // <OutputParticel> is optional; defaults to false.
            p->output_particel = false;
            while (r.readNextStartElement()) {
                if (r.name() == QStringView(u"OutputParticel"))
                    p->output_particel = (QtParser::readInner(r) == "True");
                else
                    r.skipCurrentElement();
            }
        }
        else if (elName == "NoteModifiers") {
            // First child: default modifiers bitstring (currently ignored — see
            // pre-Qt code: the loop that consumed it was commented out).
            if (r.readNextStartElement()) {
                QtParser::readInner(r);
                // Second child: custom modifiers list
                if (r.readNextStartElement()) {
                    while (r.readNextStartElement())
                        p->custom_note_modifiers.append(QtParser::readInner(r));
                }
                QtParser::consumeRest(r);
            }
            qDebug() << "Passed modifiers";
        }
        else if (elName == "EnvelopeLibrary") {
            QString envLibText = r.readElementText(QXmlStreamReader::IncludeChildElements);
            EnvelopeLibrary* envelopeLibrary = new EnvelopeLibrary();
            if (!envLibText.isEmpty()) {
                QString tempPath = filepath + ".lib.temp";
                QFile temp(tempPath);
                if (temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&temp);
                    out << envLibText;
                    temp.close();
                    envelopeLibrary->loadLibraryNewFormat(
                        const_cast<char*>(tempPath.toLocal8Bit().constData()));
                    QFile::remove(tempPath);
                }
            }
            EnvelopeLibraryEntry* previousEntry = nullptr;
            for (int i = 1; i <= envelopeLibrary->size(); i++) {
                Envelope* thisEnvelope = envelopeLibrary->getEnvelope(i);
                EnvelopeLibraryEntry* thisEntry = new EnvelopeLibraryEntry(thisEnvelope, i);
                delete thisEnvelope;
                if (previousEntry == nullptr) {
                    p->elentry = thisEntry;
                    thisEntry->prev = nullptr;
                } else {
                    previousEntry->next = thisEntry;
                    thisEntry->prev = previousEntry;
                }
                previousEntry = thisEntry;
            }
            delete envelopeLibrary;
            qDebug() << "Passed envelopes";
        }
        else if (elName == "MarkovModelLibrary") {
            QString data = r.readElementText(QXmlStreamReader::IncludeChildElements);
            if (!data.isEmpty()) {
                std::stringstream ss(data.toStdString());
                long long size = 0;
                ss >> size;
                p->markov_models.resize(size);
                std::string line;
                std::getline(ss, line, '\n');
                for (long long i = 0; i < size; i++) {
                    std::string modelText;
                    std::getline(ss, line, '\n'); modelText  = line + '\n';
                    std::getline(ss, line, '\n'); modelText += line + '\n';
                    std::getline(ss, line, '\n'); modelText += line + '\n';
                    std::getline(ss, line, '\n'); modelText += line;
                    p->markov_models[i].from_str(modelText);
                }
            }
            qDebug() << "Passed markov";
        }
        else if (elName == "Events") {
            while (r.readNextStartElement()) {
                if (r.name() == QStringView(u"Event"))
                    p->parseEvent(r);
                else
                    r.skipCurrentElement();
            }
        }
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError())
        qDebug() << "XML parse error:" << r.errorString();
}


/** the empty constructor for a NEW project, will return a ProjectView  **/
    
    // filepath = "";
    // project_title = "Untitled-" + std::to_string(counter);
    // file_flag = "";
    // duration = "";
    // num_channels = "2";
    // sample_rate = std::to_string(SAMPLING_RATE);
    // sample_size = "16";
    // num_threads = "1";
    // num_staffs = "1";
    // top_event_num = "0";

    // synthesis = true;
    // score = false;
    // grand_staff = false;

    // topwin = NULL;
    // highwin = NULL;
    // midwin = NULL;
    // lowwin = NULL;
    // bottomwin = NULL;
    // spectrumwin = NULL;
    // envwin = NULL;
    // sievewin = NULL;
    // spatialwin = NULL;
    // patternwin = NULL;
    // reverbwin = NULL;
    // notewin = NULL;
    // filterwin = NULL;
    // measurewin = NULL;
    // env_lib_entries = NULL;

#include <QDomDocument>
#include <QTextStream>

Project::Project(const QString& _title, const QByteArray& _id){
    if(_title.isEmpty()){
        title = tr("Untitled");
    }else{
        title = _title;
    }
#ifdef TABEDITOR
    if(_id.isEmpty())
        id = QUuid::createUuid().toString().toLatin1();
    else
        id = _id;
#endif
}

/* create a most barebones Project object: just the title and the UUID. Add it to the hash! */
Project* ProjectManager::create(const QString& title, const QByteArray& id){
    Project *project = new Project(title, id);
#ifdef TABEDITOR
    project_hash_.insert(project->id, project);
#endif

    return project;
}

Project* ProjectManager::open(const QString& filepath, const QByteArray& id){
    QFileInfo info(filepath);
    QString cpath = info.canonicalFilePath();
    info.setFile(cpath);

    Project *project = create(info.baseName(), id);
    QFileInfo fileinfo(filepath);
    project->fileinfo = fileinfo;

    curr_project_ = project;

    qDebug() << "Now parsing " << filepath;
    parse(project, filepath);
    
    return project;
}

Project* ProjectManager::build(const QString& filepath, const QByteArray& id){
    QFileInfo info(filepath);

    Project *project = create(info.baseName(), id);
    QFileInfo fileinfo(filepath);
    project->fileinfo = fileinfo;
    project->dat_path = fileinfo.absolutePath();
    project->lib_path = fileinfo.absoluteFilePath();

    curr_project_ = project;  

    // Create a default top event
    HEvent& topevent = this->topevent();
    HEvent defaultTop;
    defaultTop.type = top;
    defaultTop.name = "0";
    defaultTop.orderinpalette = "-1";
    topevent = defaultTop;

    return project;
}

void ProjectManager::close(Project* project) {
    if (curr_project_ == project)
        curr_project_ = nullptr;
    delete project;
}

void ProjectManager::addEvent(Eventtype newEvent, QString eventName) {
    switch(newEvent) {
        case high: {
            QList<HEvent>& eventList = highevents();
            HEvent newObj = {};
            newObj.type = high;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case mid: {
            QList<HEvent>& eventList = midevents();
            HEvent newObj = {};
            newObj.type = mid;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case low: {
            QList<HEvent>& eventList = lowevents();
            HEvent newObj = {};
            newObj.type = low;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case bottom: {
            QList<BottomEvent>& eventList = bottomevents();
            BottomEvent newObj = {};
            newObj.event.type = bottom;
            newObj.event.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case sound: {
            QList<SpectrumEvent>& eventList = spectrumevents();
            SpectrumEvent newObj;
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case env: {
            QList<EnvelopeEvent>& eventList = envelopeevents();
            EnvelopeEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case sieve: {
            QList<SieveEvent>& eventList = sieveevents();
            SieveEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case spa: {
            QList<SpaEvent>& eventList = spaevents();
            SpaEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case pattern: {
            QList<PatternEvent>& eventList = patternevents();
            PatternEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case reverb: {
            QList<ReverbEvent>& eventList = reverbevents();
            ReverbEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case note: {
            QList<NoteEvent>& eventList = noteevents();
            NoteEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case filter: {
            QList<FilterEvent>& eventList = filterevents();
            FilterEvent newObj = {};
            newObj.orderinpalette = QString::number(eventList.size()+1);;
            newObj.name = eventName;
            eventList.push_back(newObj);
            break;
        }
        case folder: case mea: case spec:
            break;
        default:
            break;
    }

}

void ProjectManager::writeSeedEntry(const QString& seed) const {
    QString filepath = curr_project_->fileinfo.absoluteFilePath();

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement seedEl = doc.documentElement()
        .firstChildElement("ProjectConfiguration")
        .firstChildElement("Seed");

    if (!seedEl.isNull()) {
        QDomNode text = seedEl.firstChild();
        if (!text.isNull())
            seedEl.removeChild(text);
        seedEl.appendChild(doc.createTextNode(seed));
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);
    out << doc.toString();
    file.close();
}

void ProjectManager::markModified() {
    if (curr_project_)
        curr_project_->modifiedButNotSaved = true;
    emit dataModified();
}

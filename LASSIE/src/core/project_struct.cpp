/* 
The 'project' object that keeps track of project details to deploy
in the associated window (currently, the project view).
*/
#include "project_struct.hpp"
#include "event_struct.hpp"
#include "ProjectXmlWriter.hpp"
#include "RandomOrderIntId.hpp"

#include "../../LASS/src/LASS.h"
#include "EnvelopeLibraryEntry.hpp"

// cmod
#include "MarkovModel.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
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
                    // Bug fix: text before the first <Fun> (e.g. "2 * " in
                    // "2 * <Fun>...</Fun>") was left in textBuffer and lost,
                    // so opening and saving a project changed the formula.
                    // It's now written out before the element. -- Tony Chai, 2026
                    if (!hasChildElement && !textBuffer.trimmed().isEmpty())
                        w.writeCharacters(textBuffer.trimmed());
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
                case QXmlStreamReader::Characters: {
                    QString text = r.text().toString();
                    if (hasChildElement) {
                       if (!text.trimmed().isEmpty()) {
                        w.writeCharacters(text.trimmed());
                       }
                    } else
                        textBuffer += text;
                    break;
            }
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

    inline bool readNextRequiredChild(QXmlStreamReader& r,
                                      QStringView expectedName) {
        if (!r.readNextStartElement()) {
            if (!r.hasError()) {
                r.raiseError(QStringLiteral("Expected required <%1> element.")
                                 .arg(expectedName.toString()));
            }
            return false;
        }
        if (r.name() != expectedName) {
            r.raiseError(QStringLiteral("Expected <%1>, but found <%2>.")
                             .arg(expectedName.toString(), r.name().toString()));
            return false;
        }
        return true;
    }

    inline QString nextRequiredChildInner(QXmlStreamReader& r,
                                          QStringView expectedName) {
        if (!readNextRequiredChild(r, expectedName))
            return {};
        return readInner(r);
    }

    /// @brief Consume any remaining children of the element currently being read,
    ///   advancing to its EndElement.
    inline void consumeRest(QXmlStreamReader& r) {
        while (r.readNextStartElement()) r.skipCurrentElement();
    }

    inline bool hasValidEventName(QXmlStreamReader& r, const QString& name) {
        if (r.hasError())
            return false;
        if (!name.trimmed().isEmpty())
            return true;
        r.raiseError(QStringLiteral("<Event> is missing a non-empty <Name>."));
        return false;
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

    inline bool parseModifierState(const QString& source, bool& requiredOn) {
        const QString state = source.trimmed().toLower();
        if (state == QStringLiteral("on")
            || state == QStringLiteral("true")
            || state == QStringLiteral("1")) {
            requiredOn = true;
            return true;
        }
        if (state == QStringLiteral("off")
            || state == QStringLiteral("false")
            || state == QStringLiteral("0")) {
            requiredOn = false;
            return true;
        }
        return false;
    }

    inline ModifierChanceRule parseModifierException(QXmlStreamReader& r) {
        // r at <Exception>; attributes are read before consuming its children.
        ModifierChanceRule rule;
        rule.on_chance =
            r.attributes().value(QStringLiteral("onChance")).toString();

        while (r.readNextStartElement()) {
            if (r.name() != QStringView(u"When")) {
                r.skipCurrentElement();
                continue;
            }

            const auto attributes = r.attributes();
            ModifierCondition condition;
            condition.modifier_id =
                attributes.value(QStringLiteral("modifierId")).toString().trimmed();

            bool validState = false;
            bool requiredOn = true;
            validState = parseModifierState(
                attributes.value(QStringLiteral("state")).toString(),
                requiredOn);
            condition.required_on = requiredOn;

            // Malformed predicates are ignored rather than being silently
            // interpreted as an OFF dependency.
            if (!condition.modifier_id.isEmpty() && validState)
                rule.conditions.append(condition);
            r.skipCurrentElement();
        }
        return rule;
    }

    inline bool parseModifierUsage(QXmlStreamReader& r, Modifier& modifier) {
        // r at <Usage>.
        const auto attributes = r.attributes();
        const QString id =
            attributes.value(QStringLiteral("id")).toString().trimmed();
        const bool hasDefaultOn =
            attributes.hasAttribute(QStringLiteral("defaultOn"));
        const QString defaultOn =
            attributes.value(QStringLiteral("defaultOn")).toString();
        if (!id.isEmpty())
            modifier.instance_id = id;
        if (hasDefaultOn)
            modifier.default_on_chance = defaultOn;

        modifier.rules.clear();
        while (r.readNextStartElement()) {
            if (r.name() != QStringView(u"Exceptions")) {
                r.skipCurrentElement();
                continue;
            }

            while (r.readNextStartElement()) {
                if (r.name() == QStringView(u"Exception"))
                    modifier.rules.append(parseModifierException(r));
                else
                    r.skipCurrentElement();
            }
        }

        return ModifierUsageImportPolicy::hasCompleteMetadata(
            id, hasDefaultOn, defaultOn);
    }

    inline Modifier parseModifier(QXmlStreamReader& r) {
        Modifier modifier;
        bool hasCompleteUsageMetadata = false;
        while (r.readNextStartElement()) {
            const QString tag = r.name().toString();

            if (tag == QStringLiteral("Type")) {
                bool valid = false;
                const unsigned type = readInner(r).trimmed().toUInt(&valid);
                if (valid)
                    modifier.type = type;
            } else if (tag == QStringLiteral("ApplyHow")) {
                const QString value = readInner(r).trimmed();
                bool valid = false;
                const int applyHow = value.toInt(&valid);
                if (valid) {
                    modifier.applyhow_flag = (applyHow != 0);
                } else if (value.compare(QStringLiteral("PARTIAL"),
                                         Qt::CaseInsensitive) == 0) {
                    modifier.applyhow_flag = true;
                } else if (value.compare(QStringLiteral("SOUND"),
                                         Qt::CaseInsensitive) == 0) {
                    modifier.applyhow_flag = false;
                }
            } else if (tag == QStringLiteral("Probability")) {
                // Consumed only for one-way import of pre-Modifier-Usage files.
                r.skipCurrentElement();
            } else if (tag == QStringLiteral("Amplitude")) {
                modifier.amplitude = readInner(r);
            } else if (tag == QStringLiteral("Rate")) {
                modifier.rate = readInner(r);
            } else if (tag == QStringLiteral("Width")) {
                modifier.width = readInner(r);
            } else if (tag == QStringLiteral("DetuneSpread")) {
                modifier.detune_spread = readInner(r);
            } else if (tag == QStringLiteral("DetuneDirection")) {
                modifier.detune_direction = readInner(r);
            } else if (tag == QStringLiteral("DetuneVelocity")) {
                modifier.detune_velocity = readInner(r);
            } else if (tag == QStringLiteral("GroupName")) {
                // Legacy group membership has no equivalent in the new model.
                r.skipCurrentElement();
            } else if (tag == QStringLiteral("PartialResultString")) {
                modifier.partialresult_string = readInner(r);
            } else if (tag == QStringLiteral("Usage")) {
                hasCompleteUsageMetadata = parseModifierUsage(r, modifier);
            } else {
                // A future field must not shift the interpretation of any
                // legacy sibling.
                r.skipCurrentElement();
            }
        }

        if (modifier.instance_id.trimmed().isEmpty()) {
            modifier.instance_id =
                QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        modifier.usage_metadata_needs_review = !hasCompleteUsageMetadata;
        return modifier;
    }

    inline void parseTimeSig(QXmlStreamReader& r, TimeSignature& ts) {
        ts.bar_value = nextRequiredChildInner(r, QStringView(u"Entry1"));
        ts.note_value = nextRequiredChildInner(r, QStringView(u"Entry2"));
        consumeRest(r);
    }

    inline void parseTempo(QXmlStreamReader& r, Tempo& t) {
        t.method_flag = nextRequiredChildInner(
            r, QStringView(u"MethodFlag")).toUInt();
        t.prefix = nextRequiredChildInner(r, QStringView(u"Prefix"));
        t.note_value = nextRequiredChildInner(r, QStringView(u"NoteValue"));
        t.frentry_1 = nextRequiredChildInner(
            r, QStringView(u"FractionEntry1"));
        t.frentry_2 = nextRequiredChildInner(
            r, QStringView(u"FractionEntry2"));
        t.valentry = nextRequiredChildInner(r, QStringView(u"ValueEntry"));
        consumeRest(r);
    }

    inline void parseNumChildren(QXmlStreamReader& r, NumChildren& n) {
        n.method_flag = nextRequiredChildInner(
            r, QStringView(u"MethodFlag")).toUInt();
        n.entry_1 = nextRequiredChildInner(r, QStringView(u"Entry1"));
        n.entry_2 = nextRequiredChildInner(r, QStringView(u"Entry2"));
        n.entry_3 = nextRequiredChildInner(r, QStringView(u"Entry3"));
        consumeRest(r);
    }

    inline void parseChildEventDef(QXmlStreamReader& r, ChildDef& c) {
        c.entry_1 = nextRequiredChildInner(r, QStringView(u"Entry1"));
        c.entry_2 = nextRequiredChildInner(r, QStringView(u"Entry2"));
        c.entry_3 = nextRequiredChildInner(r, QStringView(u"Entry3"));
        c.attack_sieve = nextRequiredChildInner(
            r, QStringView(u"AttackSieve"));
        c.duration_sieve = nextRequiredChildInner(
            r, QStringView(u"DurationSieve"));
        c.definition_flag = nextRequiredChildInner(
            r, QStringView(u"DefinitionFlag")).toUInt();
        c.starttype_flag = nextRequiredChildInner(
            r, QStringView(u"StartTypeFlag")).toUInt();
        c.durationtype_flag = nextRequiredChildInner(
            r, QStringView(u"DurationTypeFlag")).toUInt();
        consumeRest(r);
    }

    inline void parseModifiers(QXmlStreamReader& r, QList<Modifier>& out) {
        while (r.readNextStartElement()) {
            if (r.name() == QStringView(u"Modifier"))
                out.append(parseModifier(r));
            else
                r.skipCurrentElement();
        }
    }

    /// @brief Parse the shared "HEvent core" children of `<Event>`: from `<EventName>`
    ///   through `<Filter>`. The `<EventType>` child must already have been consumed
    ///   by the caller. Stops at `<Filter>` so callers can read the differing
    ///   trailing siblings (HEvent: `<Modifiers>`; BottomEvent: `<ExtraInfo>`).
    inline bool parseHEventCore(QXmlStreamReader& r, HEvent& event) {
        event.name = nextRequiredChildInner(r, QStringView(u"Name"));
        event.max_child_duration = nextRequiredChildInner(
            r, QStringView(u"MaxChildDuration"));
        event.edu_perbeat = nextRequiredChildInner(
            r, QStringView(u"EDUPerBeat"));
        if (r.hasError())
            return false;

        if (!readNextRequiredChild(r, QStringView(u"TimeSignature")))
            return false;
        parseTimeSig(r, event.timesig);
        if (!readNextRequiredChild(r, QStringView(u"Tempo")))
            return false;
        parseTempo(r, event.tempo);
        if (!readNextRequiredChild(r, QStringView(u"NumberOfChildren")))
            return false;
        parseNumChildren(r, event.numchildren);
        if (!readNextRequiredChild(r, QStringView(u"ChildEventDefinition")))
            return false;
        parseChildEventDef(r, event.child_event_def);
        if (r.hasError())
            return false;

        if (readNextRequiredChild(r, QStringView(u"Layers"))) {
            while (r.readNextStartElement())
                event.event_layers.append(parseLayer(r));
        } else {
            return false;
        }

        event.spa = nextRequiredChildInner(r, QStringView(u"Spatialization"));
        event.reverb = nextRequiredChildInner(r, QStringView(u"Reverb"));
        event.filter = nextRequiredChildInner(r, QStringView(u"Filter"));
        return !r.hasError();
    }

    inline void parseHEventChildren(QXmlStreamReader& r, HEvent& event) {
        if (!parseHEventCore(r, event))
            return;
        if (!readNextRequiredChild(r, QStringView(u"Modifiers")))
            return;
        parseModifiers(r, event.modifiers);
        consumeRest(r);
    }

    inline void parseExtraInfo(QXmlStreamReader& r, ExtraInfo& info) {
        // Phase was added after Loudness. Parse by tag rather than position so
        // projects written before <Phase> existed keep all following fields
        // aligned. Unknown future fields are ignored safely as well.
        info.phase = QStringLiteral("0");
        bool modifierUsageMarkerSupported = false;
        info.modifier_sampling_scope = ModifierSamplingScope::PerSound;
        QSet<QString> requiredFields{
            QStringLiteral("FrequencyInfo"), QStringLiteral("Loudness"),
            QStringLiteral("Spatialization"), QStringLiteral("Reverb"),
            QStringLiteral("Modifiers")
        };
        while (r.readNextStartElement()) {
            const QString tag = r.name().toString();
            requiredFields.remove(tag);

            if (tag == QStringLiteral("FrequencyInfo")) {
                info.freq_info.freq_flag = nextRequiredChildInner(
                    r, QStringView(u"FrequencyFlag")).toUInt();
                info.freq_info.continuum_flag = nextRequiredChildInner(
                    r, QStringView(u"FrequencyContinuumFlag")).toUInt();
                info.freq_info.entry_1 = nextRequiredChildInner(
                    r, QStringView(u"FrequencyEntry1"));
                info.freq_info.entry_2 = nextRequiredChildInner(
                    r, QStringView(u"FrequencyEntry2"));
                consumeRest(r);
            } else if (tag == QStringLiteral("Loudness")) {
                info.loudness = readInner(r);
            } else if (tag == QStringLiteral("Phase")) {
                const QString phase = readInner(r);
                info.phase = phase.trimmed().isEmpty() ? QStringLiteral("0") : phase;
            } else if (tag == QStringLiteral("Spatialization")) {
                info.spa = readInner(r);
            } else if (tag == QStringLiteral("Reverb")) {
                info.reverb = readInner(r);
            } else if (tag == QStringLiteral("Filter")) {
                info.filter = readInner(r);
            } else if (tag == QStringLiteral("ModifierGroup")) {
                // Consume the old selector without keeping a second model.
                r.skipCurrentElement();
            } else if (tag == QStringLiteral("ModifierUsage")) {
                const QString version = r.attributes()
                    .value(QStringLiteral("version"))
                    .toString()
                    .trimmed();
                const QString scope = r.attributes()
                    .value(QStringLiteral("samplingScope"))
                    .toString()
                    .trimmed()
                    .toLower();
                const bool scopeSupported =
                    scope == QStringLiteral("per-sound")
                    || scope == QStringLiteral("per-bottom");
                modifierUsageMarkerSupported =
                    version == QStringLiteral("1") && scopeSupported;
                info.modifier_sampling_scope =
                    scope == QStringLiteral("per-bottom")
                    ? ModifierSamplingScope::PerBottom
                    : ModifierSamplingScope::PerSound;
                r.skipCurrentElement();
            } else if (tag == QStringLiteral("Modifiers")) {
                parseModifiers(r, info.modifiers);
            } else {
                r.skipCurrentElement();
            }
        }

        if (!r.hasError() && !requiredFields.isEmpty()) {
            QStringList missingFields = requiredFields.values();
            missingFields.sort();
            r.raiseError(QStringLiteral(
                "<ExtraInfo> is missing required element(s): %1.")
                             .arg(missingFields.join(QStringLiteral(", "))));
        }
        info.modifier_usage_needs_review = !modifierUsageMarkerSupported;
    }

    inline void parseBottomEventChildren(QXmlStreamReader& r, BottomEvent& bev) {
        if (!parseHEventCore(r, bev.event))
            return;
        if (!readNextRequiredChild(r, QStringView(u"ExtraInfo")))
            return;
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
        event.name = nextRequiredChildInner(r, QStringView(u"Name"));
        event.num_partials = nextRequiredChildInner(
            r, QStringView(u"NumberOfPartials"));
        event.deviation = nextRequiredChildInner(r, QStringView(u"Deviation"));
        event.generate_spectrum = nextRequiredChildInner(
            r, QStringView(u"GenerateSpectrum"));
        if (r.hasError())
            return;
        if (!readNextRequiredChild(r, QStringView(u"Spectrum")))
            return;
        event.spectrum = parseSpectrum(r);
        consumeRest(r);
    }

    inline NoteInfo parseNoteInfo(QXmlStreamReader& r) {
        NoteInfo ni;
        ni.staffs = nextRequiredChildInner(r, QStringView(u"Staffs"));
        if (!readNextRequiredChild(r, QStringView(u"Modifiers")))
            return ni;
        while (r.readNextStartElement())
            ni.modifiers.append(readInner(r));
        consumeRest(r);
        return ni;
    }

    inline void parseNoteEventChildren(QXmlStreamReader& r, NoteEvent& event) {
        event.name = nextRequiredChildInner(r, QStringView(u"Name"));
        if (!readNextRequiredChild(r, QStringView(u"NoteInfo")))
            return;
        event.note_info = parseNoteInfo(r);
        consumeRest(r);
    }

}

bool Project::parseEvent(QXmlStreamReader& r, Eventtype* parsedType) {
    // r at <Event> StartElement (with orderInPalette attribute).
    QString orderinpalette = r.attributes().value("orderInPalette").toString();

    // First child is <EventType>, whose text content is the integer type.
    if (!r.readNextStartElement()) {
        r.raiseError(QStringLiteral("<Event> is missing <EventType>."));
        return false;
    }
    if (r.name() != QStringView(u"EventType")) {
        r.raiseError(QStringLiteral("Expected <EventType> as the first child of <Event>."));
        return false;
    }

    bool validType = false;
    int typeInt = QtParser::readInner(r).toInt(&validType);
    if (!validType || typeInt < static_cast<int>(top)
        || typeInt > static_cast<int>(spec)) {
        r.raiseError(QStringLiteral("<EventType> contains an invalid event type."));
        return false;
    }
    Eventtype type = (Eventtype)typeInt;
    if (type == folder || type == mea || type == spec) {
        r.raiseError(QStringLiteral("<EventType> contains an unsupported event type."));
        return false;
    }

    switch (type) {
        case top:
        case high:
        case mid:
        case low: {
            HEvent eh;
            eh.orderinpalette = orderinpalette;
            eh.type = type;
            QtParser::parseHEventChildren(r, eh);
            if (!QtParser::hasValidEventName(r, eh.name))
                return false;
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
            if (!QtParser::hasValidEventName(r, eb.event.name))
                return false;
            qDebug() << "parsed Bottom event named" << eb.event.name;
            bottom_events.append(eb);
            break;
        }
        case sound: {
            SpectrumEvent espec;
            espec.orderinpalette = orderinpalette;
            QtParser::parseSpectrumEventChildren(r, espec);
            if (!QtParser::hasValidEventName(r, espec.name))
                return false;
            qDebug() << "parsed Spectrum event named" << espec.name;
            spectrum_events.append(espec);
            break;
        }
        case note: {
            NoteEvent en;
            en.orderinpalette = orderinpalette;
            QtParser::parseNoteEventChildren(r, en);
            if (!QtParser::hasValidEventName(r, en.name))
                return false;
            qDebug() << "parsed Note event named " << en.name;
            note_events.append(en);
            break;
        }
        case env: {
            EnvelopeEvent ee;
            ee.orderinpalette = orderinpalette;
            ee.name = QtParser::nextRequiredChildInner(
                r, QStringView(u"Name"));
            ee.envelope_builder = QtParser::nextRequiredChildInner(
                r, QStringView(u"EnvelopeBuilder"));
            QtParser::consumeRest(r);
            if (!QtParser::hasValidEventName(r, ee.name))
                return false;
            qDebug() << "parsed Envelope event named" << ee.name;
            envelope_events.append(ee);
            break;
        }
        case sieve: {
            SieveEvent esi;
            esi.orderinpalette = orderinpalette;
            esi.name = QtParser::nextRequiredChildInner(
                r, QStringView(u"Name"));
            esi.sieve_builder = QtParser::nextRequiredChildInner(
                r, QStringView(u"SieveBuilder"));
            QtParser::consumeRest(r);
            if (!QtParser::hasValidEventName(r, esi.name))
                return false;
            qDebug() << "parsed Sieve event named" << esi.name;
            sieve_events.append(esi);
            break;
        }
        case spa: {
            SpaEvent espa;
            espa.orderinpalette = orderinpalette;
            espa.name = QtParser::nextRequiredChildInner(
                r, QStringView(u"Name"));
            espa.spatialization = QtParser::nextRequiredChildInner(
                r, QStringView(u"Spatialization"));
            QtParser::consumeRest(r);
            if (!QtParser::hasValidEventName(r, espa.name))
                return false;
            qDebug() << "parsed Spa event named" << espa.name;
            spa_events.append(espa);
            break;
        }
        case pattern: {
            PatternEvent ep;
            ep.orderinpalette = orderinpalette;
            ep.name = QtParser::nextRequiredChildInner(
                r, QStringView(u"Name"));
            ep.pattern_builder = QtParser::nextRequiredChildInner(
                r, QStringView(u"PatternBuilder"));
            QtParser::consumeRest(r);
            if (!QtParser::hasValidEventName(r, ep.name))
                return false;
            qDebug() << "parsed Pattern event named" << ep.name;
            pattern_events.append(ep);
            break;
        }
        case reverb: {
            ReverbEvent er;
            er.orderinpalette = orderinpalette;
            er.name = QtParser::nextRequiredChildInner(
                r, QStringView(u"Name"));
            er.reverberation = QtParser::nextRequiredChildInner(
                r, QStringView(u"Reverberation"));
            QtParser::consumeRest(r);
            if (!QtParser::hasValidEventName(r, er.name))
                return false;
            qDebug() << "parsed Reverb event named" << er.name;
            reverb_events.append(er);
            break;
        }
        case filter: {
            FilterEvent ef;
            ef.orderinpalette = orderinpalette;
            ef.name = QtParser::nextRequiredChildInner(
                r, QStringView(u"Name"));
            ef.filter_builder = QtParser::nextRequiredChildInner(
                r, QStringView(u"FilterBuilder"));
            QtParser::consumeRest(r);
            if (!QtParser::hasValidEventName(r, ef.name))
                return false;
            qDebug() << "parsed Filter event named" << ef.name;
            filter_events.append(ef);
            break;
        }
        default:
            return false;
    }

    if (parsedType)
        *parsedType = type;
    return true;
}

bool ProjectManager::parse(Project* p, const QString& filepath,
                           QString* errorMessage) {
    const auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return fail(QStringLiteral("Cannot read the project file: %1")
                        .arg(file.errorString()));
    }
    RandomOrderIntId::reserve(QString::fromUtf8(file.readAll()));
    file.seek(0);
    QXmlStreamReader r(&file);

    if (!r.readNextStartElement()) {
        if (r.hasError()) {
            return fail(QStringLiteral("XML error at line %1, column %2: %3")
                            .arg(r.lineNumber())
                            .arg(r.columnNumber())
                            .arg(r.errorString()));
        }
        return fail(QStringLiteral("The file does not contain an XML root element."));
    }

    if (r.name() != QStringView(u"ProjectRoot")) {
        return fail(QStringLiteral(
                        "Expected <ProjectRoot> at line %1, but found <%2>.")
                        .arg(r.lineNumber())
                        .arg(r.name().toString()));
    }

    bool foundProjectConfiguration = false;
    bool foundEvents = false;
    bool foundTopEvent = false;
    QString configuredTopEventName;
    QString structureError;

    while (r.readNextStartElement()) {
        const QString elName = r.name().toString();

        if (elName == "ProjectConfiguration") {
            if (foundProjectConfiguration) {
                structureError = QStringLiteral(
                    "The project contains more than one <ProjectConfiguration> section.");
                r.skipCurrentElement();
                continue;
            }
            foundProjectConfiguration = true;

            QSet<QString> requiredFields{
                QStringLiteral("Title"), QStringLiteral("FileFlag"),
                QStringLiteral("TopEvent"), QStringLiteral("PieceStartTime"),
                QStringLiteral("Duration"), QStringLiteral("NumberOfStaff"),
                QStringLiteral("NumberOfChannels"),
                QStringLiteral("SampleRate"), QStringLiteral("SampleSize"),
                QStringLiteral("NumberOfThreads")
            };

            // Older LASSIE versions omitted false boolean fields entirely.
            p->synthesis = false;
            p->score = false;
            p->grand_staff = false;
            p->output_particel = false;
            while (r.readNextStartElement()) {
                const QString field = r.name().toString();
                requiredFields.remove(field);

                if (field == QStringLiteral("Title")) {
                    // The filename remains the authoritative in-app title.
                    QtParser::readInner(r);
                } else if (field == QStringLiteral("FileFlag")) {
                    p->file_flag = QtParser::readInner(r);
                } else if (field == QStringLiteral("TopEvent")) {
                    configuredTopEventName = QtParser::readInner(r).trimmed();
                } else if (field == QStringLiteral("PieceStartTime")) {
                    p->start_time = QtParser::readInner(r);
                } else if (field == QStringLiteral("Duration")) {
                    p->duration = QtParser::readInner(r);
                } else if (field == QStringLiteral("Synthesis")) {
                    p->synthesis = (QtParser::readInner(r) == QStringLiteral("True"));
                } else if (field == QStringLiteral("Score")) {
                    p->score = (QtParser::readInner(r) == QStringLiteral("True"));
                } else if (field == QStringLiteral("GrandStaff")) {
                    p->grand_staff = (QtParser::readInner(r) == QStringLiteral("True"));
                } else if (field == QStringLiteral("NumberOfStaff")) {
                    p->num_staffs = QtParser::readInner(r);
                } else if (field == QStringLiteral("NumberOfChannels")) {
                    p->num_channels = QtParser::readInner(r);
                } else if (field == QStringLiteral("SampleRate")) {
                    p->sample_rate = QtParser::readInner(r);
                } else if (field == QStringLiteral("SampleSize")) {
                    p->sample_size = QtParser::readInner(r);
                } else if (field == QStringLiteral("NumberOfThreads")) {
                    p->num_threads = QtParser::readInner(r);
                } else if (field == QStringLiteral("OutputParticel")) {
                    p->output_particel =
                        (QtParser::readInner(r) == QStringLiteral("True"));
                } else if (field == QStringLiteral("Seed")) {
                    p->seed = QtParser::readInner(r);
                } else {
                    r.skipCurrentElement();
                }
            }

            if (!requiredFields.isEmpty() && structureError.isEmpty()) {
                QStringList missingFields = requiredFields.values();
                missingFields.sort();
                structureError = QStringLiteral(
                    "<ProjectConfiguration> is missing required field(s): %1.")
                    .arg(missingFields.join(QStringLiteral(", ")));
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
            if (foundEvents) {
                structureError = QStringLiteral(
                    "The project contains more than one <Events> section.");
                r.skipCurrentElement();
                continue;
            }
            foundEvents = true;
            while (r.readNextStartElement()) {
                if (r.name() == QStringView(u"Event")) {
                    Eventtype parsedType = top;
                    if (p->parseEvent(r, &parsedType) && parsedType == top) {
                        if (foundTopEvent) {
                            structureError = QStringLiteral(
                                "The project contains more than one top event.");
                        }
                        foundTopEvent = true;
                    }
                } else {
                    r.skipCurrentElement();
                }
            }
        }
        else {
            r.skipCurrentElement();
        }
    }

    if (r.hasError()) {
        return fail(QStringLiteral("XML error at line %1, column %2: %3")
                        .arg(r.lineNumber())
                        .arg(r.columnNumber())
                        .arg(r.errorString()));
    }
    if (!foundProjectConfiguration)
        return fail(QStringLiteral("The project is missing <ProjectConfiguration>."));
    if (!foundEvents)
        return fail(QStringLiteral("The project is missing <Events>."));
    if (!foundTopEvent)
        return fail(QStringLiteral("The project is missing a top event."));
    if (!structureError.isEmpty())
        return fail(structureError);
    if (configuredTopEventName.isEmpty()) {
        return fail(QStringLiteral(
            "<ProjectConfiguration><TopEvent> must name the top event."));
    }
    if (p->top_event.name != configuredTopEventName) {
        return fail(QStringLiteral(
            "Configured top event '%1' does not match parsed top event '%2'.")
                        .arg(configuredTopEventName, p->top_event.name));
    }

    if (errorMessage)
        errorMessage->clear();
    return true;
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
#ifndef TABEDITOR
    Q_UNUSED(_id);
#endif
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

Project* ProjectManager::open(const QString& filepath, const QByteArray& id,
                              QString* errorMessage, bool makeCurrent){
    QFileInfo info(filepath);
    QString cpath = info.canonicalFilePath();
    info.setFile(cpath);

    Project *project = create(info.baseName(), id);
    QFileInfo fileinfo(filepath);
    project->fileinfo = fileinfo;

    qDebug() << "Now parsing " << filepath;
    if (!parse(project, filepath, errorMessage)) {
#ifdef TABEDITOR
        project_hash_.remove(project->id);
#endif
        delete project;
        return nullptr;
    }

    project->modifiedButNotSaved = false;
    if (makeCurrent)
        curr_project_ = project;
    
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
    defaultTop.event_layers.append(Layer());
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
    ProjectXmlWriter::updateProjectSeed(
        curr_project_->fileinfo.absoluteFilePath(), seed);
}

void ProjectManager::markModified() {
    if (curr_project_)
        curr_project_->modifiedButNotSaved = true;
    emit dataModified();
}

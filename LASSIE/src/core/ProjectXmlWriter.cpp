#include "ProjectXmlWriter.hpp"

#include <QDomDocument>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QXmlStreamWriter>

namespace {

void writeDomNode(QXmlStreamWriter& writer, const QDomNode& node)
{
    if (node.isElement()) {
        const QDomElement element = node.toElement();
        writer.writeStartElement(element.tagName());
        const QDomNamedNodeMap attributes = element.attributes();
        for (int i = 0; i < attributes.count(); ++i) {
            const QDomAttr attribute = attributes.item(i).toAttr();
            writer.writeAttribute(attribute.name(), attribute.value());
        }
        for (QDomNode child = element.firstChild(); !child.isNull();
             child = child.nextSibling()) {
            writeDomNode(writer, child);
        }
        writer.writeEndElement();
    } else if (node.isCDATASection()) {
        writer.writeCDATA(node.toCDATASection().data());
    } else if (node.isText()) {
        const QString text = node.toText().data();
        if (!text.trimmed().isEmpty())
            writer.writeCharacters(text);
    }
}

QString activeModifierField(const Modifier& modifier, int fieldIndex)
{
    // Columns: amplitude, rate, width, spread, direction, velocity.
    static constexpr bool fields[8][6] = {
        /* TREMOLO   */ { true,  true,  false, false, false, false },
        /* VIBRATO   */ { true,  true,  false, false, false, false },
        /* GLISSANDO */ { true,  false, false, false, false, false },
        /* DETUNE    */ { false, false, false, true,  true,  true  },
        /* AMPTRANS  */ { true,  true,  true,  false, false, false },
        /* FREQTRANS */ { true,  true,  true,  false, false, false },
        /* WAVE_TYPE */ { true,  false, false, false, false, false },
        /* PHASE_MOD */ { true,  true,  false, false, false, false },
    };

    if (modifier.type >= 8 || fieldIndex < 0 || fieldIndex >= 6
        || !fields[modifier.type][fieldIndex]) {
        return {};
    }

    switch (fieldIndex) {
    case 0: return modifier.amplitude;
    case 1: return modifier.rate;
    case 2: return modifier.width;
    case 3: return modifier.detune_spread;
    case 4: return modifier.detune_direction;
    case 5: return modifier.detune_velocity;
    default: return {};
    }
}

void writeElement(QXmlStreamWriter& writer, const QString& name,
                  const QString& value)
{
    writer.writeStartElement(name);
    ProjectXmlWriter::writeInlineXml(writer, value);
    writer.writeEndElement();
}

QString samplingScopeName(ModifierSamplingScope scope)
{
    return scope == ModifierSamplingScope::PerBottom
        ? QStringLiteral("per-bottom")
        : QStringLiteral("per-sound");
}

void writeModifierUsage(QXmlStreamWriter& writer, const Modifier& modifier)
{
    writer.writeStartElement(QStringLiteral("Usage"));
    writer.writeAttribute(QStringLiteral("id"), modifier.instance_id);
    // A Default ON envelope replaces the fixed chance. The attribute is left
    // out so an older CMOD reports the project instead of ignoring the curve.
    const QString envelope = modifier.default_on_envelope.trimmed();
    if (envelope.isEmpty()) {
        writer.writeAttribute(QStringLiteral("defaultOn"),
                              modifier.default_on_chance);
    } else {
        writeElement(writer, QStringLiteral("DefaultOn"), envelope);
    }

    writer.writeStartElement(QStringLiteral("Exceptions"));
    for (const ModifierChanceRule& rule : modifier.rules) {
        writer.writeStartElement(QStringLiteral("Exception"));
        writer.writeAttribute(QStringLiteral("onChance"), rule.on_chance);
        for (const ModifierCondition& condition : rule.conditions) {
            writer.writeEmptyElement(QStringLiteral("When"));
            writer.writeAttribute(QStringLiteral("modifierId"),
                                  condition.modifier_id);
            writer.writeAttribute(QStringLiteral("state"),
                                  condition.required_on
                                      ? QStringLiteral("on")
                                      : QStringLiteral("off"));
        }
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndElement();
}

} // namespace

void ProjectXmlWriter::writeInlineXml(QXmlStreamWriter& writer,
                                      const QString& value)
{
    QDomDocument document;
    const QString wrapped = QStringLiteral("<root>%1</root>").arg(value.trimmed());
    if (document.setContent(wrapped)) {
        const QDomNodeList children = document.documentElement().childNodes();
        for (int i = 0; i < children.count(); ++i)
            writeDomNode(writer, children.at(i));
    } else {
        writer.writeCharacters(value);
    }
}

void ProjectXmlWriter::writeModifier(QXmlStreamWriter& writer,
                                     const Modifier& modifier)
{
    writer.writeStartElement(QStringLiteral("Modifier"));
    writeElement(writer, QStringLiteral("Type"), QString::number(modifier.type));
    writeElement(writer, QStringLiteral("ApplyHow"),
                 modifier.applyhow_flag ? QStringLiteral("1") : QStringLiteral("0"));
    writeElement(writer, QStringLiteral("Amplitude"), activeModifierField(modifier, 0));
    writeElement(writer, QStringLiteral("Rate"), activeModifierField(modifier, 1));
    writeElement(writer, QStringLiteral("Width"), activeModifierField(modifier, 2));
    writeElement(writer, QStringLiteral("DetuneSpread"), activeModifierField(modifier, 3));
    writeElement(writer, QStringLiteral("DetuneDirection"), activeModifierField(modifier, 4));
    writeElement(writer, QStringLiteral("DetuneVelocity"), activeModifierField(modifier, 5));
    writeElement(writer, QStringLiteral("PartialResultString"),
                 modifier.applyhow_flag ? modifier.partialresult_string : QString{});
    writeModifierUsage(writer, modifier);
    writer.writeEndElement();
}

void ProjectXmlWriter::writeBottomExtraInfo(QXmlStreamWriter& writer,
                                            const ExtraInfo& extraInfo)
{
    writer.writeStartElement(QStringLiteral("ExtraInfo"));
    writer.writeStartElement(QStringLiteral("FrequencyInfo"));
    writeElement(writer, QStringLiteral("FrequencyFlag"),
                 QString::number(extraInfo.freq_info.freq_flag));
    writeElement(writer, QStringLiteral("FrequencyContinuumFlag"),
                 QString::number(extraInfo.freq_info.continuum_flag));
    writeElement(writer, QStringLiteral("FrequencyEntry1"), extraInfo.freq_info.entry_1);
    writeElement(writer, QStringLiteral("FrequencyEntry2"), extraInfo.freq_info.entry_2);
    writer.writeEndElement();

    writeElement(writer, QStringLiteral("Loudness"), extraInfo.loudness);
    writeElement(writer, QStringLiteral("Phase"),
                 extraInfo.phase.trimmed().isEmpty() ? QStringLiteral("0")
                                                     : extraInfo.phase);
    writeElement(writer, QStringLiteral("Spatialization"), extraInfo.spa);
    writeElement(writer, QStringLiteral("Reverb"), extraInfo.reverb);
    writeElement(writer, QStringLiteral("Filter"), extraInfo.filter);
    writer.writeEmptyElement(QStringLiteral("ModifierUsage"));
    writer.writeAttribute(QStringLiteral("version"), QStringLiteral("1"));
    writer.writeAttribute(QStringLiteral("samplingScope"),
                          samplingScopeName(
                              extraInfo.modifier_sampling_scope));

    writer.writeStartElement(QStringLiteral("Modifiers"));
    for (const Modifier& modifier : extraInfo.modifiers)
        writeModifier(writer, modifier);
    writer.writeEndElement();
    writer.writeEndElement();
}

bool ProjectXmlWriter::updateProjectSeed(const QString& filePath,
                                         const QString& seed)
{
    QFile input(filePath);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument document;
    if (!document.setContent(&input))
        return false;
    input.close();

    QDomElement seedElement = document.documentElement()
        .firstChildElement(QStringLiteral("ProjectConfiguration"))
        .firstChildElement(QStringLiteral("Seed"));
    if (seedElement.isNull())
        return false;

    while (!seedElement.firstChild().isNull())
        seedElement.removeChild(seedElement.firstChild());
    seedElement.appendChild(document.createTextNode(seed));

    QSaveFile output(filePath);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&output);
    stream << document.toString();
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        output.cancelWriting();
        return false;
    }
    return output.commit();
}

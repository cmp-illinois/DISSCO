#include "ProjectXmlWriter.hpp"

#include <QDomDocument>
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
    // Columns: probability, amplitude, rate, width, spread, direction,
    // velocity. This preserves LASSIE's existing per-type save policy.
    static constexpr bool fields[8][7] = {
        /* TREMOLO   */ { true,  true,  true,  false, false, false, false },
        /* VIBRATO   */ { true,  true,  true,  false, false, false, false },
        /* GLISSANDO */ { true,  true,  false, false, false, false, false },
        /* DETUNE    */ { true,  false, false, false, true,  true,  true  },
        /* AMPTRANS  */ { true,  true,  true,  true,  false, false, false },
        /* FREQTRANS */ { true,  true,  true,  true,  false, false, false },
        /* WAVE_TYPE */ { false, true,  false, false, false, false, false },
        /* PHASE_MOD */ { true,  true,  true,  false, false, false, false },
    };

    if (modifier.type >= 8 || fieldIndex < 0 || fieldIndex >= 7
        || !fields[modifier.type][fieldIndex]) {
        return {};
    }

    switch (fieldIndex) {
    case 0: return modifier.probability;
    case 1: return modifier.amplitude;
    case 2: return modifier.rate;
    case 3: return modifier.width;
    case 4: return modifier.detune_spread;
    case 5: return modifier.detune_direction;
    case 6: return modifier.detune_velocity;
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
    writeElement(writer, QStringLiteral("Probability"), activeModifierField(modifier, 0));
    writeElement(writer, QStringLiteral("Amplitude"), activeModifierField(modifier, 1));
    writeElement(writer, QStringLiteral("Rate"), activeModifierField(modifier, 2));
    writeElement(writer, QStringLiteral("Width"), activeModifierField(modifier, 3));
    writeElement(writer, QStringLiteral("DetuneSpread"), activeModifierField(modifier, 4));
    writeElement(writer, QStringLiteral("DetuneDirection"), activeModifierField(modifier, 5));
    writeElement(writer, QStringLiteral("DetuneVelocity"), activeModifierField(modifier, 6));
    writeElement(writer, QStringLiteral("GroupName"), modifier.group_name);
    writeElement(writer, QStringLiteral("PartialResultString"),
                 modifier.applyhow_flag ? modifier.partialresult_string : QString{});
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
    writeElement(writer, QStringLiteral("ModifierGroup"), extraInfo.modifier_group);

    writer.writeStartElement(QStringLiteral("Modifiers"));
    for (const Modifier& modifier : extraInfo.modifiers)
        writeModifier(writer, modifier);
    writer.writeEndElement();
    writer.writeEndElement();
}

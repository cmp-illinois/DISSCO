#include "PartialModifierFormat.hpp"

#include <QCoreApplication>
#include <QDomDocument>
#include <QTextStream>

namespace {

QString elementInnerXml(const QDomElement& element)
{
    QString result;
    QTextStream stream(&result);
    for (QDomNode child = element.firstChild(); !child.isNull();
         child = child.nextSibling()) {
        if (child.isText() || child.isCDATASection())
            stream << child.nodeValue();
        else
            child.save(stream, 0);
    }
    return result;
}

QString envelopeXml(const QString& value)
{
    const QString normalized = value.trimmed().isEmpty() ? QStringLiteral("N/A")
                                                          : value.trimmed();

    QDomDocument fragment;
    const QString wrapped = QStringLiteral("<root>") + normalized
        + QStringLiteral("</root>");
    const QString body = fragment.setContent(wrapped)
        ? normalized
        : normalized.toHtmlEscaped();
    return QStringLiteral("<Envelope>") + body + QStringLiteral("</Envelope>");
}

QString translated(const char* source)
{
    return QCoreApplication::translate("PartialModifierDialog", source);
}

} // namespace

QString PartialModifierFormat::normalizedValue(const QString& value, bool enabled)
{
    if (!enabled)
        return QStringLiteral("N/A");
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("N/A") : trimmed;
}

QVector<PartialModifierFormat::Values>
PartialModifierFormat::parse(const QString& source, QString* warning)
{
    QVector<Values> values;
    if (warning)
        warning->clear();
    if (source.trimmed().isEmpty())
        return values;

    QDomDocument document;
    if (!document.setContent(source)) {
        if (warning) {
            *warning = translated(
                "The existing Partial Result String is not valid XML; default rows are shown.");
        }
        return values;
    }

    const QDomElement root = document.documentElement();
    const QDomElement name = root.firstChildElement(QStringLiteral("Name"));
    const QDomElement envelopes = root.firstChildElement(QStringLiteral("Envelopes"));
    if (root.tagName() != QStringLiteral("Fun")
        || name.text().trimmed() != QStringLiteral("Partials")
        || envelopes.isNull()) {
        if (warning) {
            *warning = translated(
                "The existing value is not a Partials function; default rows are shown.");
        }
        return values;
    }

    QVector<QString> envelopeValues;
    for (QDomElement envelope = envelopes.firstChildElement(QStringLiteral("Envelope"));
         !envelope.isNull();
         envelope = envelope.nextSiblingElement(QStringLiteral("Envelope"))) {
        const QString value = elementInnerXml(envelope).trimmed();
        envelopeValues.append(value.isEmpty() ? QStringLiteral("N/A") : value);
    }

    if (envelopeValues.size() % 4 != 0) {
        if (warning) {
            *warning = translated(
                "The existing Partials function does not contain four envelopes per partial; missing values were filled with N/A.");
        }
        while (envelopeValues.size() % 4 != 0)
            envelopeValues.append(QStringLiteral("N/A"));
    }

    for (int i = 0; i < envelopeValues.size(); i += 4) {
        Values row;
        row.probability = envelopeValues.at(i);
        row.magnitude = envelopeValues.at(i + 1);
        row.width = envelopeValues.at(i + 2);
        row.rate = envelopeValues.at(i + 3);
        values.append(row);
    }
    return values;
}

QString PartialModifierFormat::serialize(const QVector<Values>& partials)
{
    QString result = QStringLiteral("<Fun><Name>Partials</Name><Envelopes>");
    for (const Values& row : partials) {
        result += envelopeXml(normalizedValue(row.probability, true));
        result += envelopeXml(normalizedValue(row.magnitude, true));
        // CMOD's legacy PARTIAL wire format always has four slots. PHASE_MOD
        // does not consume width, so keep the placeholder explicit.
        result += envelopeXml(QStringLiteral("N/A"));
        result += envelopeXml(normalizedValue(row.rate, true));
    }
    result += QStringLiteral("</Envelopes></Fun>");
    return result;
}

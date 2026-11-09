#include "PartialModifierFormat.hpp"

#include <QCoreApplication>
#include <QDomDocument>
#include <QTextStream>

#include <cmath>

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

bool hasConfiguredValue(const QString& value)
{
    const QString normalized = value.trimmed();
    return !normalized.isEmpty()
        && normalized.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) != 0;
}

QString missingEffectError(int zeroBasedRow, const QString& effect)
{
    return QCoreApplication::translate(
               "PartialModifierDialog",
               "Partial %1 is enabled but has no %2 envelope.")
        .arg(zeroBasedRow + 1)
        .arg(effect);
}

// DETUNE by partial takes the same three numbers as DETUNE by sound.
QString detuneRowError(int zeroBasedRow,
                       const PartialModifierFormat::Values& value)
{
    const int partial = zeroBasedRow + 1;
    const auto fieldError = [partial](const QString& field,
                                      const QString& rule) {
        return QCoreApplication::translate(
                   "PartialModifierDialog",
                   "Partial %1: Detune %2 must be %3.")
            .arg(partial)
            .arg(field)
            .arg(rule);
    };

    const QString spreadText = value.magnitude.trimmed();
    if (spreadText.startsWith(QLatin1Char('<'))) {
        return QCoreApplication::translate(
                   "PartialModifierDialog",
                   "Partial %1 still uses the old Detuning envelope. DETUNE by "
                   "partial now takes Spread, Direction and Velocity numbers, "
                   "like DETUNE by sound.")
            .arg(partial);
    }

    bool ok = false;
    const double spread = spreadText.toDouble(&ok);
    if (!hasConfiguredValue(spreadText) || !ok || !std::isfinite(spread)
        || spread < 0.0 || spread > 1.0) {
        return fieldError(translated("Spread"),
                          translated("a number from 0 to 1"));
    }

    const double direction = value.width.trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(direction) || direction == 0.0) {
        return fieldError(translated("Direction"),
                          translated("a negative number (detune) or a positive "
                                     "number (tune)"));
    }

    const double velocity = value.rate.trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(velocity) || velocity < -1.0 || velocity > 1.0) {
        return fieldError(translated("Velocity"),
                          translated("a number from -1 to 1"));
    }
    return {};
}

} // namespace

QString PartialModifierFormat::normalizedValue(const QString& value, bool enabled)
{
    if (!enabled)
        return QStringLiteral("N/A");
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()
        || trimmed.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("N/A");
    }
    return trimmed;
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
        // Keep all four legacy slots. The editor supplies N/A for a field the
        // selected modifier type does not consume; transient modifiers retain
        // their real Width envelope here.
        result += envelopeXml(normalizedValue(row.width, true));
        result += envelopeXml(normalizedValue(row.rate, true));
    }
    result += QStringLiteral("</Envelopes></Fun>");
    return result;
}

QString PartialModifierFormat::validationError(int modifierType,
                                               const QString& source)
{
    QString warning;
    const QVector<Values> values = parse(source, &warning);
    if (!warning.isEmpty())
        return warning;
    if (values.isEmpty())
        return translated("Configure at least one partial row.");

    for (int row = 0; row < values.size(); ++row) {
        const Values& value = values[row];
        if (!hasConfiguredValue(value.probability))
            continue;

        const auto require = [&](const QString& effectValue,
                                 const char* effectName) -> QString {
            return hasConfiguredValue(effectValue)
                ? QString()
                : missingEffectError(row, translated(effectName));
        };

        QString error;
        switch (modifierType) {
        case 0: // Tremolo
        case 1: // Vibrato
        case 7: // Phase Modulation
            error = require(value.magnitude, "Magnitude");
            if (error.isEmpty())
                error = require(value.rate, "Rate");
            break;
        case 2: // Glissando
        case 6: // Wave Type
            error = require(value.magnitude, "Magnitude");
            break;
        case 3: // Detune
            error = detuneRowError(row, value);
            break;
        case 4: // Amplitude Transient
        case 5: // Frequency Transient
            error = require(value.magnitude, "Magnitude");
            if (error.isEmpty())
                error = require(value.width, "Width");
            if (error.isEmpty())
                error = require(value.rate, "Rate");
            break;
        default:
            return translated("The selected modifier type is not supported.");
        }
        if (!error.isEmpty())
            return error;
    }
    return {};
}

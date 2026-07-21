#ifndef PARTIALMODIFIERFORMAT_HPP
#define PARTIALMODIFIERFORMAT_HPP

#include <QString>
#include <QVector>

namespace PartialModifierFormat {

struct Values {
    QString probability = QStringLiteral("N/A");
    QString magnitude = QStringLiteral("N/A");
    QString width = QStringLiteral("N/A");
    QString rate = QStringLiteral("N/A");
};

QVector<Values> parse(const QString& source, QString* warning = nullptr);
QString serialize(const QVector<Values>& partials);

QString normalizedValue(const QString& value, bool enabled);

} // namespace PartialModifierFormat

#endif // PARTIALMODIFIERFORMAT_HPP

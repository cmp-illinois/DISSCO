#include "MultiEntryFunction.hpp"

#include "../../widgets/generic/FunctionEntryRow.hpp"

#include <QVBoxLayout>

MultiEntryFunction::MultiEntryFunction(QList<EntrySpec> specs, QWidget* parent)
    : FunctionWidget(parent),
      m_specs(std::move(specs))
{
    auto* layout = new QVBoxLayout(this);

    int idx = 0;
    for (const EntrySpec& s : m_specs) {
        auto* row = new FunctionEntryRow(s.labelText, idx++, s.fnReturnType,
                                         /*rmVisible=*/false, /*insVisible=*/false,
                                         this, s.fnVisible);
        if (!s.defaultValue.isEmpty()) row->setText(s.defaultValue);
        connect(row, &FunctionEntryRow::textChanged,
                this, [this]() { emit xmlChanged(); });
        layout->addWidget(row);
        m_rows.append(row);
    }
}

QString MultiEntryFunction::buildXMLString() const {
    QString out = QStringLiteral("<Fun><Name>") + xmlName() + QStringLiteral("</Name>");
    for (int i = 0; i < m_rows.size(); ++i) {
        const QString& tag = m_specs[i].xmlTag;
        out += QStringLiteral("<") + tag + QStringLiteral(">")
             + m_rows[i]->getText()
             + QStringLiteral("</") + tag + QStringLiteral(">");
    }
    out += QStringLiteral("</Fun>");
    return out;
}

void MultiEntryFunction::populateFromXML(QXmlStreamReader& reader) {
    for (FunctionEntryRow* row : m_rows)
        row->setText(nextChildInner(reader));
}

void MultiEntryFunction::reset() {
    for (int i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setText(m_specs[i].defaultValue);
}

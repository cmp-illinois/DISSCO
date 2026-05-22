#include "SingleEntryFunction.hpp"

#include "../../widgets/generic/FunctionEntryRow.hpp"

#include <QVBoxLayout>

SingleEntryFunction::SingleEntryFunction(const QString& labelText,
                                         FunctionReturnType rowReturnType,
                                         QWidget* parent)
    : FunctionWidget(parent)
{
    m_row = new FunctionEntryRow(labelText, /*index=*/0, rowReturnType,
                                 /*rmVisible=*/false, /*insVisible=*/false,
                                 this);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_row);

    connect(m_row, &FunctionEntryRow::textChanged,
            this, [this]() { emit xmlChanged(); });
}

QString SingleEntryFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>") + xmlName()
         + QStringLiteral("</Name><Entry>")
         + m_row->getText()
         + QStringLiteral("</Entry></Fun>");
}

void SingleEntryFunction::populateFromXML(QXmlStreamReader& reader) {
    m_row->setText(nextChildInner(reader));
}

void SingleEntryFunction::reset() {
    m_row->setText(QString());
}

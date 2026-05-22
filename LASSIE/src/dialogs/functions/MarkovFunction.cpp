#include "MarkovFunction.hpp"

#include "../../widgets/generic/FunctionEntryRow.hpp"

#include <QVBoxLayout>

MarkovFunction::MarkovFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    m_row = new FunctionEntryRow(tr("Markov Model Id:"), /*index=*/0,
                                 FunctionReturnType::functionReturnInt,
                                 /*rmVisible=*/false, /*insVisible=*/false,
                                 this, /*fnVisible=*/false);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_row);

    connect(m_row, &FunctionEntryRow::textChanged,
            this, [this]() { emit xmlChanged(); });
}

QString MarkovFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>Markov</Name><Entry>")
         + m_row->getText()
         + QStringLiteral("</Entry></Fun>");
}

void MarkovFunction::populateFromXML(QXmlStreamReader& reader) {
    m_row->setText(nextChildInner(reader));
}

void MarkovFunction::reset() {
    m_row->setText(QString());
}

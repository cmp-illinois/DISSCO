#include "RangeFunction.hpp"

#include "../../widgets/generic/FunctionEntryRow.hpp"

#include <QVBoxLayout>

RangeFunction::RangeFunction(FunctionReturnType rowReturnType, QWidget* parent)
    : FunctionWidget(parent)
{
    m_lowRow  = new FunctionEntryRow(tr("Lower Bound:"), /*index=*/0, rowReturnType,
                                     /*rmVisible=*/false, /*insVisible=*/false,
                                     this);
    m_highRow = new FunctionEntryRow(tr("Upper Bound:"), /*index=*/1, rowReturnType,
                                     /*rmVisible=*/false, /*insVisible=*/false,
                                     this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_lowRow);
    layout->addWidget(m_highRow);

    m_lowRow->setText(QStringLiteral("0"));
    m_highRow->setText(QStringLiteral("1"));

    auto forward = [this]() { emit xmlChanged(); };
    connect(m_lowRow,  &FunctionEntryRow::textChanged, this, forward);
    connect(m_highRow, &FunctionEntryRow::textChanged, this, forward);
}

QString RangeFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>") + xmlName()
         + QStringLiteral("</Name><Low>")
         + m_lowRow->getText()
         + QStringLiteral("</Low><High>")
         + m_highRow->getText()
         + QStringLiteral("</High></Fun>");
}

void RangeFunction::populateFromXML(QXmlStreamReader& reader) {
    m_lowRow->setText(nextChildInner(reader));
    m_highRow->setText(nextChildInner(reader));
}

void RangeFunction::reset() {
    m_lowRow->setText(QStringLiteral("0"));
    m_highRow->setText(QStringLiteral("1"));
}

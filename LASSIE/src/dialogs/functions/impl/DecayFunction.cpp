#include "DecayFunction.hpp"

#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

DecayFunction::DecayFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // Type radio group
    auto* typeGroup = new QGroupBox(this);
    auto* typeLayout = new QHBoxLayout(typeGroup);
    typeLayout->addWidget(new QLabel(tr("Decay Type:"), typeGroup));
    m_exponential = new QRadioButton(tr("Exponential"), typeGroup);
    m_linear      = new QRadioButton(tr("Linear"), typeGroup);
    m_exponential->setChecked(true);  // legacy default
    typeLayout->addWidget(m_exponential);
    typeLayout->addWidget(m_linear);
    layout->addWidget(typeGroup);

    m_baseRow  = new FunctionEntryRow(tr("Base:"),  0,
                                      FunctionReturnType::functionReturnFloat,
                                      false, false, this);
    m_rateRow  = new FunctionEntryRow(tr("Rate:"),  1,
                                      FunctionReturnType::functionReturnFloat,
                                      false, false, this);
    m_indexRow = new FunctionEntryRow(tr("Index:"), 2,
                                      FunctionReturnType::functionReturnFloat,
                                      false, false, this);
    layout->addWidget(m_baseRow);
    layout->addWidget(m_rateRow);
    layout->addWidget(m_indexRow);

    auto forward = [this]() { emit xmlChanged(); };
    connect(m_baseRow,    &FunctionEntryRow::textChanged, this, forward);
    connect(m_rateRow,    &FunctionEntryRow::textChanged, this, forward);
    connect(m_indexRow,   &FunctionEntryRow::textChanged, this, forward);
    connect(m_exponential, &QRadioButton::toggled, this, forward);
    connect(m_linear,      &QRadioButton::toggled, this, forward);
}

QString DecayFunction::buildXMLString() const {
    const QString type = m_linear->isChecked()
        ? QStringLiteral("LINEAR")
        : QStringLiteral("EXPONENTIAL");
    return QStringLiteral("<Fun><Name>Decay</Name><Base>")
         + m_baseRow->getText()
         + QStringLiteral("</Base><Type>") + type
         + QStringLiteral("</Type><Rate>")
         + m_rateRow->getText()
         + QStringLiteral("</Rate><Index>")
         + m_indexRow->getText()
         + QStringLiteral("</Index></Fun>");
}

void DecayFunction::populateFromXML(QXmlStreamReader& reader) {
    m_baseRow->setText(nextChildInner(reader));
    const QString type = nextChildInner(reader);
    if (type == QStringLiteral("LINEAR")) m_linear->setChecked(true);
    else                                  m_exponential->setChecked(true);
    m_rateRow->setText(nextChildInner(reader));
    m_indexRow->setText(nextChildInner(reader));
}

void DecayFunction::reset() {
    m_baseRow->setText(QString());
    m_rateRow->setText(QString());
    m_indexRow->setText(QString());
    m_exponential->setChecked(true);
}

#include "ExpandPatternFunction.hpp"

#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

ExpandPatternFunction::ExpandPatternFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* methodGroup = new QGroupBox(this);
    auto* methodLayout = new QHBoxLayout(methodGroup);
    methodLayout->addWidget(new QLabel(tr("Method:"), methodGroup));
    m_equivalence = new QRadioButton(tr("EQUIVALENCE"), methodGroup);
    m_symmetries  = new QRadioButton(tr("SYMMETRIES"),  methodGroup);
    m_distort     = new QRadioButton(tr("DISTORT"),     methodGroup);
    m_equivalence->setChecked(true);
    methodLayout->addWidget(m_equivalence);
    methodLayout->addWidget(m_symmetries);
    methodLayout->addWidget(m_distort);
    layout->addWidget(methodGroup);

    m_moduloRow  = new FunctionEntryRow(tr("Modulo:"), 0,
                                        FunctionReturnType::functionReturnInt,
                                        false, false, this);
    m_lowRow     = new FunctionEntryRow(tr("Low:"),    1,
                                        FunctionReturnType::functionReturnInt,
                                        false, false, this);
    m_highRow    = new FunctionEntryRow(tr("High:"),   2,
                                        FunctionReturnType::functionReturnInt,
                                        false, false, this);
    m_patternRow = new FunctionEntryRow(tr("Pattern:"), 3,
                                        FunctionReturnType::functionReturnPAT,
                                        false, false, this);
    layout->addWidget(m_moduloRow);
    layout->addWidget(m_lowRow);
    layout->addWidget(m_highRow);
    layout->addWidget(m_patternRow);

    auto forward = [this]() { emit xmlChanged(); };
    for (auto* b : { m_equivalence, m_symmetries, m_distort })
        connect(b, &QRadioButton::toggled, this, forward);
    for (auto* r : { m_moduloRow, m_lowRow, m_highRow, m_patternRow })
        connect(r, &FunctionEntryRow::textChanged, this, forward);
}

QString ExpandPatternFunction::currentMethod() const {
    if (m_symmetries->isChecked()) return QStringLiteral("SYMMETRIES");
    if (m_distort->isChecked())    return QStringLiteral("DISTORT");
    return QStringLiteral("EQUIVALENCE");
}

void ExpandPatternFunction::selectMethod(const QString& m) {
    if      (m == QStringLiteral("SYMMETRIES")) m_symmetries->setChecked(true);
    else if (m == QStringLiteral("DISTORT"))    m_distort->setChecked(true);
    else                                         m_equivalence->setChecked(true);
}

QString ExpandPatternFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>ExpandPattern</Name><Method>")
         + currentMethod()
         + QStringLiteral("</Method><Modulo>") + m_moduloRow->getText()
         + QStringLiteral("</Modulo><Low>")    + m_lowRow->getText()
         + QStringLiteral("</Low><High>")      + m_highRow->getText()
         + QStringLiteral("</High><Pattern>")  + m_patternRow->getText()
         + QStringLiteral("</Pattern></Fun>");
}

void ExpandPatternFunction::populateFromXML(QXmlStreamReader& reader) {
    selectMethod(nextChildInner(reader));
    m_moduloRow->setText(nextChildInner(reader));
    m_lowRow->setText(nextChildInner(reader));
    m_highRow->setText(nextChildInner(reader));
    m_patternRow->setText(nextChildInner(reader));
}

void ExpandPatternFunction::reset() {
    m_equivalence->setChecked(true);
    m_moduloRow->setText(QString());
    m_lowRow->setText(QString());
    m_highRow->setText(QString());
    m_patternRow->setText(QString());
}

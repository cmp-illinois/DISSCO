#include "GetPatternFunction.hpp"

#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

GetPatternFunction::GetPatternFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* methodGroup = new QGroupBox(this);
    auto* methodLayout = new QHBoxLayout(methodGroup);
    methodLayout->addWidget(new QLabel(tr("Method:"), methodGroup));
    m_inOrder      = new QRadioButton(tr("IN_ORDER"),      methodGroup);
    m_other        = new QRadioButton(tr("OTHER"),         methodGroup);
    m_typeClusters = new QRadioButton(tr("TYPE_CLUSTERS"), methodGroup);
    m_timeDepend   = new QRadioButton(tr("TIME_DEPEND"),   methodGroup);
    m_probability  = new QRadioButton(tr("PROBABILITY"),   methodGroup);
    m_inOrder->setChecked(true);
    methodLayout->addWidget(m_inOrder);
    methodLayout->addWidget(m_other);
    methodLayout->addWidget(m_typeClusters);
    methodLayout->addWidget(m_timeDepend);
    methodLayout->addWidget(m_probability);
    layout->addWidget(methodGroup);

    m_offsetRow  = new FunctionEntryRow(tr("Origin:"), 0,
                                        FunctionReturnType::functionReturnInt,
                                        false, false, this);
    m_patternRow = new FunctionEntryRow(tr("Pattern to choose from:"), 1,
                                        FunctionReturnType::functionReturnPAT,
                                        false, false, this);
    layout->addWidget(m_offsetRow);
    layout->addWidget(m_patternRow);

    auto forward = [this]() { emit xmlChanged(); };
    for (auto* b : { m_inOrder, m_other, m_typeClusters, m_timeDepend, m_probability })
        connect(b, &QRadioButton::toggled, this, forward);
    connect(m_offsetRow,  &FunctionEntryRow::textChanged, this, forward);
    connect(m_patternRow, &FunctionEntryRow::textChanged, this, forward);
}

QString GetPatternFunction::currentMethod() const {
    if (m_other->isChecked())        return QStringLiteral("OTHER");
    if (m_typeClusters->isChecked()) return QStringLiteral("TYPE_CLUSTERS");
    if (m_timeDepend->isChecked())   return QStringLiteral("TIME_DEPEND");
    if (m_probability->isChecked())  return QStringLiteral("PROBABILITY");
    return QStringLiteral("IN_ORDER");
}

void GetPatternFunction::selectMethod(const QString& method) {
    if      (method == QStringLiteral("OTHER"))         m_other->setChecked(true);
    else if (method == QStringLiteral("TYPE_CLUSTERS")) m_typeClusters->setChecked(true);
    else if (method == QStringLiteral("TIME_DEPEND"))   m_timeDepend->setChecked(true);
    else if (method == QStringLiteral("PROBABILITY"))   m_probability->setChecked(true);
    else                                                 m_inOrder->setChecked(true);
}

QString GetPatternFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>GetPattern</Name><Method>")
         + currentMethod()
         + QStringLiteral("</Method><Offset>")
         + m_offsetRow->getText()
         + QStringLiteral("</Offset><Pattern>")
         + m_patternRow->getText()
         + QStringLiteral("</Pattern></Fun>");
}

void GetPatternFunction::populateFromXML(QXmlStreamReader& reader) {
    selectMethod(nextChildInner(reader));
    m_offsetRow->setText(nextChildInner(reader));
    m_patternRow->setText(nextChildInner(reader));
}

void GetPatternFunction::reset() {
    m_inOrder->setChecked(true);
    m_offsetRow->setText(QString());
    m_patternRow->setText(QString());
}

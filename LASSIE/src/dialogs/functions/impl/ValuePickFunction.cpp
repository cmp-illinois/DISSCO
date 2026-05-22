#include "ValuePickFunction.hpp"

#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

ValuePickFunction::ValuePickFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_rangeRow = new FunctionEntryRow(tr("Absolute Range:"), 0,
                                      FunctionReturnType::functionReturnFloat,
                                      false, false, this);
    m_lowRow   = new FunctionEntryRow(tr("Low:"),  1,
                                      FunctionReturnType::functionReturnENV,
                                      false, false, this);
    m_highRow  = new FunctionEntryRow(tr("High:"), 2,
                                      FunctionReturnType::functionReturnENV,
                                      false, false, this);
    m_distRow  = new FunctionEntryRow(tr("Distribution:"), 3,
                                      FunctionReturnType::functionReturnENV,
                                      false, false, this);
    layout->addWidget(m_rangeRow);
    layout->addWidget(m_lowRow);
    layout->addWidget(m_highRow);
    layout->addWidget(m_distRow);

    // Elements method radio + values entry
    auto* elementsGroup = new QGroupBox(this);
    auto* elementsLayout = new QHBoxLayout(elementsGroup);
    elementsLayout->addWidget(new QLabel(tr("Elements Method:"), elementsGroup));
    m_elementsMeaningful = new QRadioButton(tr("MEANINGFUL"), elementsGroup);
    m_elementsMods       = new QRadioButton(tr("MODS"),       elementsGroup);
    m_elementsFake       = new QRadioButton(tr("FAKE"),       elementsGroup);
    m_elementsFib        = new QRadioButton(tr("FIBONACCI"),  elementsGroup);
    m_elementsMeaningful->setChecked(true);
    elementsLayout->addWidget(m_elementsMeaningful);
    elementsLayout->addWidget(m_elementsMods);
    elementsLayout->addWidget(m_elementsFake);
    elementsLayout->addWidget(m_elementsFib);
    layout->addWidget(elementsGroup);

    m_elementsValuesRow = new FunctionEntryRow(tr("Values:"), 4,
                                               FunctionReturnType::functionReturnInt,
                                               false, false, this,
                                               /*fnVisible=*/false);
    layout->addWidget(m_elementsValuesRow);

    // Weights method radio + values entry
    auto* weightsGroup = new QGroupBox(this);
    auto* weightsLayout = new QHBoxLayout(weightsGroup);
    weightsLayout->addWidget(new QLabel(tr("Weights Method:"), weightsGroup));
    m_weightsPeriodic   = new QRadioButton(tr("PERIODIC"),   weightsGroup);
    m_weightsHierarchic = new QRadioButton(tr("HIERARCHIC"), weightsGroup);
    m_weightsInclude    = new QRadioButton(tr("INCLUDE"),    weightsGroup);
    m_weightsPeriodic->setChecked(true);
    weightsLayout->addWidget(m_weightsPeriodic);
    weightsLayout->addWidget(m_weightsHierarchic);
    weightsLayout->addWidget(m_weightsInclude);
    layout->addWidget(weightsGroup);

    m_weightsValuesRow = new FunctionEntryRow(tr("Values:"), 5,
                                              FunctionReturnType::functionReturnFloat,
                                              false, false, this,
                                              /*fnVisible=*/false);
    layout->addWidget(m_weightsValuesRow);

    // Type radio
    auto* typeGroup = new QGroupBox(this);
    auto* typeLayout = new QHBoxLayout(typeGroup);
    typeLayout->addWidget(new QLabel(tr("Type:"), typeGroup));
    m_typeVariable = new QRadioButton(tr("VARIABLE"), typeGroup);
    m_typeConstant = new QRadioButton(tr("CONSTANT"), typeGroup);
    m_typeVariable->setChecked(true);
    typeLayout->addWidget(m_typeVariable);
    typeLayout->addWidget(m_typeConstant);
    layout->addWidget(typeGroup);

    m_offsetRow = new FunctionEntryRow(tr("Offset:"), 6,
                                       FunctionReturnType::functionReturnInt,
                                       false, false, this,
                                       /*fnVisible=*/false);
    layout->addWidget(m_offsetRow);

    auto forward = [this]() { emit xmlChanged(); };
    for (auto* b : { m_elementsMeaningful, m_elementsMods, m_elementsFake, m_elementsFib,
                     m_weightsPeriodic, m_weightsHierarchic, m_weightsInclude,
                     m_typeVariable, m_typeConstant })
        connect(b, &QRadioButton::toggled, this, forward);
    for (auto* r : { m_rangeRow, m_lowRow, m_highRow, m_distRow,
                     m_elementsValuesRow, m_weightsValuesRow, m_offsetRow })
        connect(r, &FunctionEntryRow::textChanged, this, forward);
}

QString ValuePickFunction::currentElementsMethod() const {
    if (m_elementsMods->isChecked()) return QStringLiteral("MODS");
    if (m_elementsFake->isChecked()) return QStringLiteral("FAKE");
    if (m_elementsFib->isChecked())  return QStringLiteral("FIBONACCI");
    return QStringLiteral("MEANINGFUL");
}

QString ValuePickFunction::currentWeightsMethod() const {
    if (m_weightsHierarchic->isChecked()) return QStringLiteral("HIERARCHIC");
    if (m_weightsInclude->isChecked())    return QStringLiteral("INCLUDE");
    return QStringLiteral("PERIODIC");
}

QString ValuePickFunction::currentType() const {
    if (m_typeConstant->isChecked()) return QStringLiteral("CONSTANT");
    return QStringLiteral("VARIABLE");
}

void ValuePickFunction::selectElementsMethod(const QString& m) {
    if      (m == QStringLiteral("MODS"))      m_elementsMods->setChecked(true);
    else if (m == QStringLiteral("FAKE"))      m_elementsFake->setChecked(true);
    else if (m == QStringLiteral("FIBONACCI")) m_elementsFib->setChecked(true);
    else                                        m_elementsMeaningful->setChecked(true);
}

void ValuePickFunction::selectWeightsMethod(const QString& m) {
    if      (m == QStringLiteral("HIERARCHIC")) m_weightsHierarchic->setChecked(true);
    else if (m == QStringLiteral("INCLUDE"))    m_weightsInclude->setChecked(true);
    else                                         m_weightsPeriodic->setChecked(true);
}

void ValuePickFunction::selectType(const QString& t) {
    if (t == QStringLiteral("CONSTANT")) m_typeConstant->setChecked(true);
    else                                  m_typeVariable->setChecked(true);
}

QString ValuePickFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>ValuePick</Name><Range>")
         + m_rangeRow->getText()
         + QStringLiteral("</Range><Low>")    + m_lowRow->getText()
         + QStringLiteral("</Low><High>")     + m_highRow->getText()
         + QStringLiteral("</High><Dist>")    + m_distRow->getText()
         + QStringLiteral("</Dist><Method>")  + currentElementsMethod()
         + QStringLiteral("</Method><Elements>") + m_elementsValuesRow->getText()
         + QStringLiteral("</Elements><WeightMethod>") + currentWeightsMethod()
         + QStringLiteral("</WeightMethod><Weight>")   + m_weightsValuesRow->getText()
         + QStringLiteral("</Weight><Type>")  + currentType()
         + QStringLiteral("</Type><Offsets>") + m_offsetRow->getText()
         + QStringLiteral("</Offsets></Fun>");
}

void ValuePickFunction::populateFromXML(QXmlStreamReader& reader) {
    m_rangeRow->setText(nextChildInner(reader));
    m_lowRow->setText(nextChildInner(reader));
    m_highRow->setText(nextChildInner(reader));
    m_distRow->setText(nextChildInner(reader));
    selectElementsMethod(nextChildInner(reader));
    m_elementsValuesRow->setText(nextChildInner(reader));
    selectWeightsMethod(nextChildInner(reader));
    m_weightsValuesRow->setText(nextChildInner(reader));
    selectType(nextChildInner(reader));
    m_offsetRow->setText(nextChildInner(reader));
}

void ValuePickFunction::reset() {
    m_rangeRow->setText(QString());
    m_lowRow->setText(QString());
    m_highRow->setText(QString());
    m_distRow->setText(QString());
    m_elementsMeaningful->setChecked(true);
    m_elementsValuesRow->setText(QString());
    m_weightsPeriodic->setChecked(true);
    m_weightsValuesRow->setText(QString());
    m_typeVariable->setChecked(true);
    m_offsetRow->setText(QString());
}

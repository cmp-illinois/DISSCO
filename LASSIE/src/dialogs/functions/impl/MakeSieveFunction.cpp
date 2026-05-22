#include "MakeSieveFunction.hpp"

#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

namespace {
    QGroupBox* makeRadioGroup(QWidget* parent,
                              const QString& title,
                              std::initializer_list<QRadioButton**> buttons,
                              std::initializer_list<const char*> labels) {
        auto* group = new QGroupBox(parent);
        auto* layout = new QHBoxLayout(group);
        layout->addWidget(new QLabel(title, group));
        auto bit = buttons.begin();
        auto lit = labels.begin();
        for (; bit != buttons.end() && lit != labels.end(); ++bit, ++lit) {
            *(*bit) = new QRadioButton(QObject::tr(*lit), group);
            layout->addWidget(*(*bit));
        }
        return group;
    }
}

MakeSieveFunction::MakeSieveFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // Low/High bounds. Legacy uses ENV-returning dialogs here (matching).
    m_lowRow  = new FunctionEntryRow(tr("Low Bound:"),  0,
                                     FunctionReturnType::functionReturnENV,
                                     false, false, this);
    m_highRow = new FunctionEntryRow(tr("High Bound:"), 1,
                                     FunctionReturnType::functionReturnENV,
                                     false, false, this);
    layout->addWidget(m_lowRow);
    layout->addWidget(m_highRow);

    auto* elementsGroup = makeRadioGroup(this, tr("Elements Method:"),
        { &m_elementsMeaningful, &m_elementsMods, &m_elementsFake, &m_elementsFib },
        { "MEANINGFUL", "MODS", "FAKE", "FIBONACCI" });
    m_elementsMeaningful->setChecked(true);
    layout->addWidget(elementsGroup);

    m_elementsValuesRow = new FunctionEntryRow(tr("Values:"), 2,
                                               FunctionReturnType::functionReturnInt,
                                               false, false, this,
                                               /*fnVisible=*/false);
    layout->addWidget(m_elementsValuesRow);

    auto* weightsGroup = makeRadioGroup(this, tr("Weights Method:"),
        { &m_weightsPeriodic, &m_weightsHierarchic, &m_weightsInclude },
        { "PERIODIC", "HIERARCHIC", "INCLUDE" });
    m_weightsPeriodic->setChecked(true);
    layout->addWidget(weightsGroup);

    m_weightsValuesRow = new FunctionEntryRow(tr("Values:"), 3,
                                              FunctionReturnType::functionReturnFloat,
                                              false, false, this,
                                              /*fnVisible=*/false);
    layout->addWidget(m_weightsValuesRow);

    m_offsetRow = new FunctionEntryRow(tr("Offset:"), 4,
                                       FunctionReturnType::functionReturnInt,
                                       false, false, this,
                                       /*fnVisible=*/false);
    layout->addWidget(m_offsetRow);

    auto forward = [this]() { emit xmlChanged(); };
    for (auto* b : { m_elementsMeaningful, m_elementsMods, m_elementsFake, m_elementsFib,
                     m_weightsPeriodic, m_weightsHierarchic, m_weightsInclude })
        connect(b, &QRadioButton::toggled, this, forward);
    for (auto* r : { m_lowRow, m_highRow, m_elementsValuesRow, m_weightsValuesRow, m_offsetRow })
        connect(r, &FunctionEntryRow::textChanged, this, forward);
}

QString MakeSieveFunction::currentElementsMethod() const {
    if (m_elementsMods->isChecked()) return QStringLiteral("MODS");
    if (m_elementsFake->isChecked()) return QStringLiteral("FAKE");
    if (m_elementsFib->isChecked())  return QStringLiteral("FIBONACCI");
    return QStringLiteral("MEANINGFUL");
}

QString MakeSieveFunction::currentWeightsMethod() const {
    if (m_weightsHierarchic->isChecked()) return QStringLiteral("HIERARCHIC");
    if (m_weightsInclude->isChecked())    return QStringLiteral("INCLUDE");
    return QStringLiteral("PERIODIC");
}

void MakeSieveFunction::selectElementsMethod(const QString& m) {
    if      (m == QStringLiteral("MODS"))      m_elementsMods->setChecked(true);
    else if (m == QStringLiteral("FAKE"))      m_elementsFake->setChecked(true);
    else if (m == QStringLiteral("FIBONACCI")) m_elementsFib->setChecked(true);
    else                                        m_elementsMeaningful->setChecked(true);
}

void MakeSieveFunction::selectWeightsMethod(const QString& m) {
    if      (m == QStringLiteral("HIERARCHIC")) m_weightsHierarchic->setChecked(true);
    else if (m == QStringLiteral("INCLUDE"))    m_weightsInclude->setChecked(true);
    else                                         m_weightsPeriodic->setChecked(true);
}

QString MakeSieveFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>MakeSieve</Name><Low>")
         + m_lowRow->getText()
         + QStringLiteral("</Low><High>")          + m_highRow->getText()
         + QStringLiteral("</High><Method>")       + currentElementsMethod()
         + QStringLiteral("</Method><Elements>")   + m_elementsValuesRow->getText()
         + QStringLiteral("</Elements><WeightMethod>") + currentWeightsMethod()
         + QStringLiteral("</WeightMethod><Weight>")   + m_weightsValuesRow->getText()
         + QStringLiteral("</Weight><Offset>")     + m_offsetRow->getText()
         + QStringLiteral("</Offset></Fun>");
}

void MakeSieveFunction::populateFromXML(QXmlStreamReader& reader) {
    m_lowRow->setText(nextChildInner(reader));
    m_highRow->setText(nextChildInner(reader));
    selectElementsMethod(nextChildInner(reader));
    m_elementsValuesRow->setText(nextChildInner(reader));
    selectWeightsMethod(nextChildInner(reader));
    m_weightsValuesRow->setText(nextChildInner(reader));
    m_offsetRow->setText(nextChildInner(reader));
}

void MakeSieveFunction::reset() {
    m_lowRow->setText(QString());
    m_highRow->setText(QString());
    m_elementsMeaningful->setChecked(true);
    m_elementsValuesRow->setText(QString());
    m_weightsPeriodic->setChecked(true);
    m_weightsValuesRow->setText(QString());
    m_offsetRow->setText(QString());
}

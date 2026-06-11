#include "MakeFilterFunction.hpp"

#include "../../../widgets/generic/FunctionEntryRow.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

MakeFilterFunction::MakeFilterFunction(QWidget* parent)
    : FunctionWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* typeGroup = new QGroupBox(this);
    auto* typeLayout = new QHBoxLayout(typeGroup);
    typeLayout->addWidget(new QLabel(tr("Filter Type:"), typeGroup));
    m_lpf   = new QRadioButton(tr("LPF"),  typeGroup);
    m_hpf   = new QRadioButton(tr("HPF"),  typeGroup);
    m_bpf   = new QRadioButton(tr("BPF"),  typeGroup);
    m_hsf   = new QRadioButton(tr("HSF"),  typeGroup);
    m_lsf   = new QRadioButton(tr("LSF"),  typeGroup);
    m_nf    = new QRadioButton(tr("NF"),   typeGroup);
    m_pbeqf = new QRadioButton(tr("PBEQ"), typeGroup);
    m_lpf->setChecked(true);
    for (auto* b : { m_lpf, m_hpf, m_bpf, m_hsf, m_lsf, m_nf, m_pbeqf })
        typeLayout->addWidget(b);
    layout->addWidget(typeGroup);

    m_frequencyRow = new FunctionEntryRow(tr("Frequency:"), 0,
                                          FunctionReturnType::functionReturnFloat,
                                          false, false, this);
    m_bandWidthRow = new FunctionEntryRow(tr("Band Width:"), 1,
                                          FunctionReturnType::functionReturnFloat,
                                          false, false, this);
    m_dbGainRow    = new FunctionEntryRow(tr("dB Gain:"), 2,
                                          FunctionReturnType::functionReturnFloat,
                                          false, false, this);
    layout->addWidget(m_frequencyRow);
    layout->addWidget(m_bandWidthRow);
    layout->addWidget(m_dbGainRow);
    updateDbGainEnabled();

    auto forward = [this]() {
        updateDbGainEnabled();
        emit xmlChanged();
    };
    for (auto* b : { m_lpf, m_hpf, m_bpf, m_hsf, m_lsf, m_nf, m_pbeqf })
        connect(b, &QRadioButton::toggled, this, forward);
    for (auto* r : { m_frequencyRow, m_bandWidthRow, m_dbGainRow })
        connect(r, &FunctionEntryRow::textChanged, this,
                [this]() { emit xmlChanged(); });
}

QString MakeFilterFunction::currentType() const {
    if (m_hpf->isChecked())   return QStringLiteral("HPF");
    if (m_bpf->isChecked())   return QStringLiteral("BPF");
    if (m_hsf->isChecked())   return QStringLiteral("HSF");
    if (m_lsf->isChecked())   return QStringLiteral("LSF");
    if (m_nf->isChecked())    return QStringLiteral("NF");
    if (m_pbeqf->isChecked()) return QStringLiteral("PBEQF");
    return QStringLiteral("LPF");
}

void MakeFilterFunction::selectType(const QString& type) {
    if      (type == QStringLiteral("HPF"))   m_hpf->setChecked(true);
    else if (type == QStringLiteral("BPF"))   m_bpf->setChecked(true);
    else if (type == QStringLiteral("HSF"))   m_hsf->setChecked(true);
    else if (type == QStringLiteral("LSF"))   m_lsf->setChecked(true);
    else if (type == QStringLiteral("NF"))    m_nf->setChecked(true);
    else if (type == QStringLiteral("PBEQF")) m_pbeqf->setChecked(true);
    else                                       m_lpf->setChecked(true);
}

void MakeFilterFunction::updateDbGainEnabled() {
    const bool useGain = m_hsf->isChecked() || m_lsf->isChecked() || m_pbeqf->isChecked();
    m_dbGainRow->setEnabled(useGain);
}

QString MakeFilterFunction::buildXMLString() const {
    return QStringLiteral("<Fun><Name>MakeFilter</Name><Type>")
         + currentType()
         + QStringLiteral("</Type><Frequency>") + m_frequencyRow->getText()
         + QStringLiteral("</Frequency><BandWidth>") + m_bandWidthRow->getText()
         + QStringLiteral("</BandWidth><dBGain>") + m_dbGainRow->getText()
         + QStringLiteral("</dBGain></Fun>");
}

void MakeFilterFunction::populateFromXML(QXmlStreamReader& reader) {
    selectType(nextChildInner(reader));
    m_frequencyRow->setText(nextChildInner(reader));
    m_bandWidthRow->setText(nextChildInner(reader));
    m_dbGainRow->setText(nextChildInner(reader));
    updateDbGainEnabled();
}

void MakeFilterFunction::reset() {
    m_lpf->setChecked(true);
    m_frequencyRow->setText(QString());
    m_bandWidthRow->setText(QString());
    m_dbGainRow->setText(QString());
    updateDbGainEnabled();
}

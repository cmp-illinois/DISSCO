#ifndef MAKEFILTERFUNCTION_HPP
#define MAKEFILTERFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QRadioButton;

/** MakeFilter: filter-type radio (7 options) plus Frequency / BandWidth
 *  / dBGain entries. dBGain is only meaningful for HSF / LSF / PBEQF
 *  filters; for the others the row is disabled.
 *
 *  <Fun><Name>MakeFilter</Name>
 *    <Type>LPF|HPF|BPF|HSF|LSF|NF|PBEQF</Type>
 *    <Frequency>V</Frequency>
 *    <BandWidth>V</BandWidth>
 *    <dBGain>V</dBGain>
 *  </Fun>
 */
class MakeFilterFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit MakeFilterFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionMakeFilter; }
    QString xmlName() const override { return QStringLiteral("MakeFilter"); }
    QString displayName() const override { return QStringLiteral("MakeFilter"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnFIL };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    QRadioButton*     m_lpf;
    QRadioButton*     m_hpf;
    QRadioButton*     m_bpf;
    QRadioButton*     m_hsf;
    QRadioButton*     m_lsf;
    QRadioButton*     m_nf;
    QRadioButton*     m_pbeqf;
    FunctionEntryRow* m_frequencyRow;
    FunctionEntryRow* m_bandWidthRow;
    FunctionEntryRow* m_dbGainRow;

    QString currentType() const;
    void selectType(const QString& type);
    void updateDbGainEnabled();
};

#endif // MAKEFILTERFUNCTION_HPP

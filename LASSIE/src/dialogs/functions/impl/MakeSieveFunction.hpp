#ifndef MAKESIEVEFUNCTION_HPP
#define MAKESIEVEFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QRadioButton;

/** MakeSieve. Two bounds, an elements list with method radio, a weights
 *  list with method radio, and an offset.
 *
 *  <Fun><Name>MakeSieve</Name>
 *    <Low>V</Low>
 *    <High>V</High>
 *    <Method>MEANINGFUL|MODS|FAKE|FIBONACCI</Method>
 *    <Elements>V</Elements>
 *    <WeightMethod>PERIODIC|HIERARCHIC|INCLUDE</WeightMethod>
 *    <Weight>V</Weight>
 *    <Offset>V</Offset>
 *  </Fun>
 */
class MakeSieveFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit MakeSieveFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionMakeSieve; }
    QString xmlName() const override { return QStringLiteral("MakeSieve"); }
    QString displayName() const override { return QStringLiteral("MakeSieve"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnSIV };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    FunctionEntryRow* m_lowRow;
    FunctionEntryRow* m_highRow;
    QRadioButton*     m_elementsMeaningful;
    QRadioButton*     m_elementsMods;
    QRadioButton*     m_elementsFake;
    QRadioButton*     m_elementsFib;
    FunctionEntryRow* m_elementsValuesRow;
    QRadioButton*     m_weightsPeriodic;
    QRadioButton*     m_weightsHierarchic;
    QRadioButton*     m_weightsInclude;
    FunctionEntryRow* m_weightsValuesRow;
    FunctionEntryRow* m_offsetRow;

    QString currentElementsMethod() const;
    QString currentWeightsMethod() const;
    void selectElementsMethod(const QString& m);
    void selectWeightsMethod(const QString& m);
};

#endif // MAKESIEVEFUNCTION_HPP

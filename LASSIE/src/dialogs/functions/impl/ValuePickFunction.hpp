#ifndef VALUEPICKFUNCTION_HPP
#define VALUEPICKFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QRadioButton;

/** ValuePick. Range/Low/High/Dist plus two radio-method element lists
 *  (elements + weights), a type radio, and an offset.
 *
 *  <Fun><Name>ValuePick</Name>
 *    <Range>V</Range><Low>V</Low><High>V</High><Dist>V</Dist>
 *    <Method>MEANINGFUL|MODS|FAKE|FIBONACCI</Method>
 *    <Elements>V</Elements>
 *    <WeightMethod>PERIODIC|HIERARCHIC|INCLUDE</WeightMethod>
 *    <Weight>V</Weight>
 *    <Type>VARIABLE|CONSTANT</Type>
 *    <Offsets>V</Offsets>
 *  </Fun>
 */
class ValuePickFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit ValuePickFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionValuePick; }
    QString xmlName() const override { return QStringLiteral("ValuePick"); }
    QString displayName() const override { return QStringLiteral("ValuePick"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    FunctionEntryRow* m_rangeRow;
    FunctionEntryRow* m_lowRow;
    FunctionEntryRow* m_highRow;
    FunctionEntryRow* m_distRow;
    QRadioButton*     m_elementsMeaningful;
    QRadioButton*     m_elementsMods;
    QRadioButton*     m_elementsFake;
    QRadioButton*     m_elementsFib;
    FunctionEntryRow* m_elementsValuesRow;
    QRadioButton*     m_weightsPeriodic;
    QRadioButton*     m_weightsHierarchic;
    QRadioButton*     m_weightsInclude;
    FunctionEntryRow* m_weightsValuesRow;
    QRadioButton*     m_typeVariable;
    QRadioButton*     m_typeConstant;
    FunctionEntryRow* m_offsetRow;

    QString currentElementsMethod() const;
    QString currentWeightsMethod() const;
    QString currentType() const;
    void selectElementsMethod(const QString& m);
    void selectWeightsMethod(const QString& m);
    void selectType(const QString& t);
};

#endif // VALUEPICKFUNCTION_HPP

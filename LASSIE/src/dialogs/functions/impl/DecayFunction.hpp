#ifndef DECAYFUNCTION_HPP
#define DECAYFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QRadioButton;

/** Decay. Three numeric entries (Base/Rate/Index) plus a Type radio
 *  (Exponential/Linear) that sits between Base and Rate in the XML.
 *
 *  @code{.xml}
 *  <Fun><Name>Decay</Name>
 *    <Base>V</Base>
 *    <Type>EXPONENTIAL|LINEAR</Type>
 *    <Rate>V</Rate>
 *    <Index>V</Index>
 *  </Fun>
 *  @endcode
 */
class DecayFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit DecayFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionDecay; }
    QString xmlName() const override { return QStringLiteral("Decay"); }
    QString displayName() const override { return QStringLiteral("Decay"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    FunctionEntryRow* m_baseRow;
    FunctionEntryRow* m_rateRow;
    FunctionEntryRow* m_indexRow;
    QRadioButton*     m_exponential;
    QRadioButton*     m_linear;
};

#endif // DECAYFUNCTION_HPP

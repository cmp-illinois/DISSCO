#ifndef EXPANDPATTERNFUNCTION_HPP
#define EXPANDPATTERNFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QRadioButton;

/** ExpandPattern: method radio + four entries (Modulo/Low/High/Pattern).
 *
 *  @code{.xml}
 *  <Fun><Name>ExpandPattern</Name>
 *    <Method>EQUIVALENCE|SYMMETRIES|DISTORT</Method>
 *    <Modulo>V</Modulo>
 *    <Low>V</Low>
 *    <High>V</High>
 *    <Pattern>V</Pattern>
 *  </Fun>
 *  @endcode
 */
class ExpandPatternFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit ExpandPatternFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionExpandPattern; }
    QString xmlName() const override { return QStringLiteral("ExpandPattern"); }
    QString displayName() const override { return QStringLiteral("ExpandPattern"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnPAT };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    QRadioButton*     m_equivalence;
    QRadioButton*     m_symmetries;
    QRadioButton*     m_distort;
    FunctionEntryRow* m_moduloRow;
    FunctionEntryRow* m_lowRow;
    FunctionEntryRow* m_highRow;
    FunctionEntryRow* m_patternRow;

    QString currentMethod() const;
    void selectMethod(const QString& m);
};

#endif // EXPANDPATTERNFUNCTION_HPP

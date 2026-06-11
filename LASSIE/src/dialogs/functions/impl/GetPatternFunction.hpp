#ifndef GETPATTERNFUNCTION_HPP
#define GETPATTERNFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QRadioButton;

/** GetPattern: a method choice plus offset + pattern entries.
 *
 *  @code{.xml}
 *  <Fun><Name>GetPattern</Name>
 *    <Method>IN_ORDER|OTHER|TYPE_CLUSTERS|TIME_DEPEND|PROBABILITY</Method>
 *    <Offset>V</Offset>
 *    <Pattern>V</Pattern>
 *  </Fun>
 *  @endcode
 */
class GetPatternFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit GetPatternFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionGetPattern; }
    QString xmlName() const override { return QStringLiteral("GetPattern"); }
    QString displayName() const override { return QStringLiteral("GetPattern"); }
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
    QRadioButton*     m_inOrder;
    QRadioButton*     m_other;
    QRadioButton*     m_typeClusters;
    QRadioButton*     m_timeDepend;
    QRadioButton*     m_probability;
    FunctionEntryRow* m_offsetRow;
    FunctionEntryRow* m_patternRow;

    QString currentMethod() const;
    void selectMethod(const QString& method);
};

#endif // GETPATTERNFUNCTION_HPP

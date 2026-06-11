#ifndef STOCHOSFUNCTION_HPP
#define STOCHOSFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QScrollArea;
class QVBoxLayout;
class Stochos;

/** Stochos: a method radio (RANGE_DISTRIB / FUNCTIONS) plus a list of
 *  envelope rows. In RANGE_DISTRIB mode each row has min/max/dist
 *  entries (three `<Envelope>` tags); in FUNCTIONS mode each row has one
 *  entry (one `<Envelope>`). Switching methods clears existing rows.
 *
 *  @code{.xml}
 *  <Fun><Name>Stochos</Name>
 *    <Method>RANGE_DISTRIB|FUNCTIONS</Method>
 *    <Envelopes>...rows...</Envelopes>
 *    <Offset>V</Offset>
 *  </Fun>
 *  @endcode
 */
class StochosFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit StochosFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionStochos; }
    QString xmlName() const override { return QStringLiteral("Stochos"); }
    QString displayName() const override { return QStringLiteral("Stochos"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnENV,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    void addNode();
    void removeNode(Stochos* node);
    void clearNodes();
    void reindexNodes();
    QString joinedNodesText() const;
    bool isRangeMode() const;

    QRadioButton*     m_methodRange;
    QRadioButton*     m_methodFunctions;
    QLineEdit*        m_offsetEdit;
    QScrollArea*      m_scrollArea;
    QVBoxLayout*      m_nodesLayout;
    QLineEdit*        m_lastFocusedEdit = nullptr;
    QPushButton*      m_insertFnButton;
};

#endif // STOCHOSFUNCTION_HPP

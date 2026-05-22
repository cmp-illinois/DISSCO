#ifndef SELECTFUNCTION_HPP
#define SELECTFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class Select;

/** Select: a list of values (each a "Select" row) plus an Index field
 *  that picks one of them.
 *
 *  <Fun><Name>Select</Name>
 *    <List>v1, v2, &lt;Fun&gt;...&lt;/Fun&gt;, ...</List>
 *    <Index>V</Index>
 *  </Fun>
 *
 *  Items in <List> are joined with ", "; nested <Fun>...</Fun> values
 *  are preserved verbatim and the parser splits only on top-level commas.
 */
class SelectFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit SelectFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionSelect; }
    QString xmlName() const override { return QStringLiteral("Select"); }
    QString displayName() const override { return QStringLiteral("Select"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnENV,
            FunctionReturnType::functionReturnSPA,
            FunctionReturnType::functionReturnREV,
            FunctionReturnType::functionReturnPAT,
            FunctionReturnType::functionReturnFIL,
            FunctionReturnType::functionReturnMGP,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    void addNode(const QString& initialText = QString());
    void removeNode(Select* node);
    void reindexNodes();
    QString joinedNodesText() const;
    static QStringList splitTopLevelCommas(const QString& s);

    FunctionEntryRow* m_indexRow;
    QVBoxLayout*      m_nodesLayout;
    QScrollArea*      m_scrollArea;
    QLineEdit*        m_lastFocusedEdit = nullptr;
    QPushButton*      m_insertFnButton;
};

#endif // SELECTFUNCTION_HPP

#ifndef MAKEENVELOPEFUNCTION_HPP
#define MAKEENVELOPEFUNCTION_HPP

#include "../FunctionWidget.hpp"

class FunctionEntryRow;
class MakeEnvelopeRow;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

/** MakeEnvelope: a dynamic list of (X, Y, interpolation, proportion)
 *  points plus a Scale factor.
 *
 *  <Fun><Name>MakeEnvelope</Name>
 *    <Xs><X>v</X>...</Xs>                              // N values
 *    <Ys><Y>v</Y>...</Ys>                              // N values
 *    <Types><T>LINEAR|SPLINE|EXPONENTIAL</T>...</Types> // N-1 (segments)
 *    <Pros><P>FLEXIBLE|FIXED</P>...</Pros>              // N-1 (segments)
 *    <Scale>v</Scale>
 *  </Fun>
 */
class MakeEnvelopeFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit MakeEnvelopeFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionMakeEnvelope; }
    QString xmlName() const override { return QStringLiteral("MakeEnvelope"); }
    QString displayName() const override { return QStringLiteral("MakeEnvelope"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnENV,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    MakeEnvelopeRow* insertRow(MakeEnvelopeRow* prevRow);
    void removeRow(MakeEnvelopeRow* row);
    void clearRows();
    void updateRowLabels();

    QScrollArea*    m_scrollArea;
    QVBoxLayout*    m_rowsLayout;
    FunctionEntryRow* m_scaleRow;
    QList<MakeEnvelopeRow*> m_rows;
};

#endif // MAKEENVELOPEFUNCTION_HPP

#ifndef LNFUNCTION_HPP
#define LNFUNCTION_HPP

#include "FunctionWidget.hpp"

class QLineEdit;

/** Natural log. Serializes as <Fun><Name>LN</Name><Entry>X</Entry></Fun>. */
class LNFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit LNFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionLN; }
    QString xmlName() const override { return QStringLiteral("LN"); }
    QString displayName() const override { return QStringLiteral("LN"); }
    QList<FunctionReturnType> supportedReturnTypes() const override;

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    QLineEdit* m_entryEdit;
};

#endif // LNFUNCTION_HPP

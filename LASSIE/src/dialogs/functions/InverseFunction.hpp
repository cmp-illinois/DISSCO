#ifndef INVERSEFUNCTION_HPP
#define INVERSEFUNCTION_HPP

#include "FunctionWidget.hpp"

class QLineEdit;

/**
 * FunctionWidget for the "Inverse" function: one numeric entry, one
 * "Insert Function" button. Serializes as
 *     <Fun><Name>Inverse</Name><Entry>X</Entry></Fun>.
 *
 * First port from FunctionGenerator's legacy monolithic switch — kept
 * deliberately simple as a template for subsequent ports.
 */
class InverseFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit InverseFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionInverse; }
    QString xmlName() const override { return QStringLiteral("Inverse"); }
    QString displayName() const override { return QStringLiteral("Inverse"); }
    QList<FunctionReturnType> supportedReturnTypes() const override;

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    QLineEdit* m_entryEdit;
};

#endif // INVERSEFUNCTION_HPP

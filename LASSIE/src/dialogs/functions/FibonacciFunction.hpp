#ifndef FIBONACCIFUNCTION_HPP
#define FIBONACCIFUNCTION_HPP

#include "FunctionWidget.hpp"

class QLineEdit;

/** Fibonacci. Serializes as <Fun><Name>Fibonacci</Name><Entry>X</Entry></Fun>. */
class FibonacciFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit FibonacciFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionFibonacci; }
    QString xmlName() const override { return QStringLiteral("Fibonacci"); }
    QString displayName() const override { return QStringLiteral("Fibonacci"); }
    QList<FunctionReturnType> supportedReturnTypes() const override;

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    QLineEdit* m_entryEdit;
};

#endif // FIBONACCIFUNCTION_HPP

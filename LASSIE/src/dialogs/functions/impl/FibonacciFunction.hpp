#ifndef FIBONACCIFUNCTION_HPP
#define FIBONACCIFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class FibonacciFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit FibonacciFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Fibonacci:"), "Entry",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionFibonacci; }
    QString xmlName() const override { return QStringLiteral("Fibonacci"); }
    QString displayName() const override { return QStringLiteral("Fibonacci"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // FIBONACCIFUNCTION_HPP

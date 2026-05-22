#ifndef INVERSEFUNCTION_HPP
#define INVERSEFUNCTION_HPP

#include "MultiEntryFunction.hpp"

class InverseFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit InverseFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Number To Inverse:"), "Entry",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionInverse; }
    QString xmlName() const override { return QStringLiteral("Inverse"); }
    QString displayName() const override { return QStringLiteral("Inverse"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // INVERSEFUNCTION_HPP

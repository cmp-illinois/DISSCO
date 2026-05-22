#ifndef INVERSEFUNCTION_HPP
#define INVERSEFUNCTION_HPP

#include "SingleEntryFunction.hpp"

/** Inverse. <Fun><Name>Inverse</Name><Entry>X</Entry></Fun>. */
class InverseFunction : public SingleEntryFunction {
    Q_OBJECT

public:
    explicit InverseFunction(QWidget* parent = nullptr)
        : SingleEntryFunction(tr("Number To Inverse:"),
                              FunctionReturnType::functionReturnFloat,
                              parent) {}

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

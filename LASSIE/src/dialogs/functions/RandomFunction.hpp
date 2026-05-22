#ifndef RANDOMFUNCTION_HPP
#define RANDOMFUNCTION_HPP

#include "RangeFunction.hpp"

/** Uniform float random. <Fun><Name>Random</Name><Low>X</Low><High>Y</High></Fun>. */
class RandomFunction : public RangeFunction {
    Q_OBJECT

public:
    explicit RandomFunction(QWidget* parent = nullptr)
        : RangeFunction(FunctionReturnType::functionReturnFloat, parent) {}

    CMODFunction id() const override { return CMODFunction::functionRandom; }
    QString xmlName() const override { return QStringLiteral("Random"); }
    QString displayName() const override { return QStringLiteral("Random"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // RANDOMFUNCTION_HPP

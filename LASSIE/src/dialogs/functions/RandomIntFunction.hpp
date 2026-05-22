#ifndef RANDOMINTFUNCTION_HPP
#define RANDOMINTFUNCTION_HPP

#include "RangeFunction.hpp"

/** Uniform int random. <Fun><Name>RandomInt</Name><Low>X</Low><High>Y</High></Fun>. */
class RandomIntFunction : public RangeFunction {
    Q_OBJECT

public:
    explicit RandomIntFunction(QWidget* parent = nullptr)
        : RangeFunction(FunctionReturnType::functionReturnInt, parent) {}

    CMODFunction id() const override { return CMODFunction::functionRandomInt; }
    QString xmlName() const override { return QStringLiteral("RandomInt"); }
    QString displayName() const override { return QStringLiteral("RandomInt"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // RANDOMINTFUNCTION_HPP

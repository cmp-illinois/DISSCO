#ifndef RANDOMINTFUNCTION_HPP
#define RANDOMINTFUNCTION_HPP

#include "MultiEntryFunction.hpp"

class RandomIntFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit RandomIntFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Lower Bound:"), "Low",
                FunctionReturnType::functionReturnInt, QStringLiteral("0") },
              { tr("Upper Bound:"), "High",
                FunctionReturnType::functionReturnInt, QStringLiteral("1") },
          }, parent) {}

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

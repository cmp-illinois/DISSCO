#ifndef RANDOMFUNCTION_HPP
#define RANDOMFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class RandomFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit RandomFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Lower Bound:"), "Low",
                FunctionReturnType::functionReturnFloat, QStringLiteral("0") },
              { tr("Upper Bound:"), "High",
                FunctionReturnType::functionReturnFloat, QStringLiteral("1") },
          }, parent) {}

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

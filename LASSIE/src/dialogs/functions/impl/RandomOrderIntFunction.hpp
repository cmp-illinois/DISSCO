#ifndef RANDOMORDERINTFUNCTION_HPP
#define RANDOMORDERINTFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class RandomOrderIntFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit RandomOrderIntFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Lower Bound:"), "Low",
                FunctionReturnType::functionReturnInt, QStringLiteral("0") },
              { tr("Upper Bound:"), "High",
                FunctionReturnType::functionReturnInt, QStringLiteral("1") },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionRandomOrderInt; }
    QString xmlName() const override { return QStringLiteral("RandomOrderInt"); }
    QString displayName() const override { return QStringLiteral("RandomOrderInt"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // RANDOMORDERINTFUNCTION_HPP

#ifndef RANDOMORDERINTFUNCTION_HPP
#define RANDOMORDERINTFUNCTION_HPP

#include "RangeFunction.hpp"

/** Random ordering of ints in [Low, High].
 *  <Fun><Name>RandomOrderInt</Name><Low>X</Low><High>Y</High></Fun>. */
class RandomOrderIntFunction : public RangeFunction {
    Q_OBJECT

public:
    explicit RandomOrderIntFunction(QWidget* parent = nullptr)
        : RangeFunction(FunctionReturnType::functionReturnInt, parent) {}

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

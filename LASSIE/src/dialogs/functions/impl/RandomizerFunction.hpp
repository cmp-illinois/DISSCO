#ifndef RANDOMIZERFUNCTION_HPP
#define RANDOMIZERFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class RandomizerFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit RandomizerFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Base:"),      "Base",
                FunctionReturnType::functionReturnFloat, {} },
              { tr("Deviation:"), "Deviation",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionRandomizer; }
    QString xmlName() const override { return QStringLiteral("Randomizer"); }
    QString displayName() const override { return QStringLiteral("Randomizer"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // RANDOMIZERFUNCTION_HPP

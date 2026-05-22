#ifndef RANDOMDENSITYFUNCTION_HPP
#define RANDOMDENSITYFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class RandomDensityFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit RandomDensityFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Envelope Id:"), "Envelope",
                FunctionReturnType::functionReturnENV, {} },
              { tr("Start Time:"),  "Low",
                FunctionReturnType::functionReturnFloat, {} },
              { tr("End Time:"),    "High",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionRandomDensity; }
    QString xmlName() const override { return QStringLiteral("RandomDensity"); }
    QString displayName() const override { return QStringLiteral("RandomDensity"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // RANDOMDENSITYFUNCTION_HPP

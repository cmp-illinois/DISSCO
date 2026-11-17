#ifndef LNFUNCTION_HPP
#define LNFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

// EXP_DECAY gives e^(-x). It used to be called LN, though it was never a
// natural log; Prof. Tipei kept the curve and renamed it. -- Tony Chai, 2026
class LNFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit LNFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("x:"), "Entry",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionLN; }
    QString xmlName() const override { return QStringLiteral("EXP_DECAY"); }
    QString displayName() const override { return QStringLiteral("EXP_DECAY: e^(-x)"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // LNFUNCTION_HPP

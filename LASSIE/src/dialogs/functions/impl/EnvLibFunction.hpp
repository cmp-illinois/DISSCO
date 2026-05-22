#ifndef ENVLIBFUNCTION_HPP
#define ENVLIBFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class EnvLibFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit EnvLibFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Envelope Number:"), "Env",
                FunctionReturnType::functionReturnInt, {} },
              { tr("Scaling Factor:"),  "Scale",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionEnvLib; }
    QString xmlName() const override { return QStringLiteral("EnvLib"); }
    QString displayName() const override { return QStringLiteral("EnvLib"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnENV,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // ENVLIBFUNCTION_HPP

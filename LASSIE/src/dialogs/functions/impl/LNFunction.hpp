#ifndef LNFUNCTION_HPP
#define LNFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class LNFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit LNFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("LN:"), "Entry",
                FunctionReturnType::functionReturnFloat, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionLN; }
    QString xmlName() const override { return QStringLiteral("LN"); }
    QString displayName() const override { return QStringLiteral("LN"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // LNFUNCTION_HPP

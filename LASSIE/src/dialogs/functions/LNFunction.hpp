#ifndef LNFUNCTION_HPP
#define LNFUNCTION_HPP

#include "SingleEntryFunction.hpp"

/** Natural log. <Fun><Name>LN</Name><Entry>X</Entry></Fun>. */
class LNFunction : public SingleEntryFunction {
    Q_OBJECT

public:
    explicit LNFunction(QWidget* parent = nullptr)
        : SingleEntryFunction(tr("LN:"),
                              FunctionReturnType::functionReturnFloat,
                              parent) {}

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

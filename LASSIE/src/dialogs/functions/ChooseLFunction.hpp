#ifndef CHOOSELFUNCTION_HPP
#define CHOOSELFUNCTION_HPP

#include "SingleEntryFunction.hpp"

/** ChooseL. <Fun><Name>ChooseL</Name><Entry>X</Entry></Fun>.
 *  Entry is a sieve, so the fn button opens a SIV-returning dialog. */
class ChooseLFunction : public SingleEntryFunction {
    Q_OBJECT

public:
    explicit ChooseLFunction(QWidget* parent = nullptr)
        : SingleEntryFunction(tr("ChooseL:"),
                              FunctionReturnType::functionReturnSIV,
                              parent) {}

    CMODFunction id() const override { return CMODFunction::functionChooseL; }
    QString xmlName() const override { return QStringLiteral("ChooseL"); }
    QString displayName() const override { return QStringLiteral("ChooseL"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
            FunctionReturnType::functionReturnMakeListFun,
        };
    }
};

#endif // CHOOSELFUNCTION_HPP

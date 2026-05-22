#ifndef CHOOSELFUNCTION_HPP
#define CHOOSELFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class ChooseLFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit ChooseLFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("ChooseL:"), "Entry",
                FunctionReturnType::functionReturnSIV, {} },
          }, parent) {}

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

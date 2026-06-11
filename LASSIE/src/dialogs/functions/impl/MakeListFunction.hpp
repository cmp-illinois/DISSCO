#ifndef MAKELISTFUNCTION_HPP
#define MAKELISTFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class MakeListFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit MakeListFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Function:"), "Func",
                FunctionReturnType::functionReturnMakeListFun, {} },
              { tr("Size:"),     "Size",
                FunctionReturnType::functionReturnMakeListFun, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionMakeList; }
    QString xmlName() const override { return QStringLiteral("MakeList"); }
    QString displayName() const override { return QStringLiteral("MakeList"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnList };
    }
};

#endif // MAKELISTFUNCTION_HPP

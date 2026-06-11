#ifndef MAKEPATTERNFUNCTION_HPP
#define MAKEPATTERNFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

class MakePatternFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit MakePatternFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Intervals:"), "List",
                FunctionReturnType::functionReturnList, {} },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionMakePattern; }
    QString xmlName() const override { return QStringLiteral("MakePattern"); }
    QString displayName() const override { return QStringLiteral("MakePattern"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnPAT };
    }
};

#endif // MAKEPATTERNFUNCTION_HPP

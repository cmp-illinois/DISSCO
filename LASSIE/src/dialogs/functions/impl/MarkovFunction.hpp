#ifndef MARKOVFUNCTION_HPP
#define MARKOVFUNCTION_HPP

#include "../MultiEntryFunction.hpp"

/** Combo label is "GetFromMarkovChain"; XML tag is "Markov". Legacy UI
 *  has no fn button -- the field is a bare Markov-model id string. */
class MarkovFunction : public MultiEntryFunction {
    Q_OBJECT

public:
    explicit MarkovFunction(QWidget* parent = nullptr)
        : MultiEntryFunction({
              { tr("Markov Model Id:"), "Entry",
                FunctionReturnType::functionReturnInt, {},
                /*fnVisible=*/false },
          }, parent) {}

    CMODFunction id() const override { return CMODFunction::functionMarkov; }
    QString xmlName() const override { return QStringLiteral("Markov"); }
    QString displayName() const override { return QStringLiteral("GetFromMarkovChain"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
        };
    }
};

#endif // MARKOVFUNCTION_HPP

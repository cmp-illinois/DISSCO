#ifndef MARKOVFUNCTION_HPP
#define MARKOVFUNCTION_HPP

#include "FunctionWidget.hpp"

class FunctionEntryRow;

/** Markov-chain lookup. Combo label is "GetFromMarkovChain"; the XML
 *  tag is just "Markov". Legacy UI has no fn button -- the field is a
 *  bare Markov-model id string. */
class MarkovFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit MarkovFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionMarkov; }
    QString xmlName() const override { return QStringLiteral("Markov"); }
    QString displayName() const override { return QStringLiteral("GetFromMarkovChain"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return {
            FunctionReturnType::functionReturnInt,
            FunctionReturnType::functionReturnFloat,
        };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    FunctionEntryRow* m_row;
};

#endif // MARKOVFUNCTION_HPP

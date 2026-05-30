#ifndef MULTIENTRYFUNCTION_HPP
#define MULTIENTRYFUNCTION_HPP

#include "FunctionWidget.hpp"

class FunctionEntryRow;

/**
 * Base class for functions whose body is a flat sequence of labeled,
 * single-line entries -- each wrapped in its own XML tag under `<Fun>`.
 *
 * Subsumes the simple single- and multi-entry cases:
 *   `<Fun><Name>X</Name><Entry>V</Entry></Fun>`                 // Inverse / LN
 *   `<Fun><Name>X</Name><Low>V</Low><High>V</High></Fun>`       // Random / RandomInt
 *   `<Fun><Name>X</Name><Envelope>V</Envelope><Low>V</Low>...`  // RandomDensity
 *
 * Functions with extra controls (radio buttons, textareas, repeated
 * rows) need their own widget; this base is only for flat row stacks.
 */
class MultiEntryFunction : public FunctionWidget {
    Q_OBJECT

public:
    struct EntrySpec {
        QString             labelText;     // shown beside the row
        QString             xmlTag;        // wraps the value, e.g. "Low", "Base"
        FunctionReturnType  fnReturnType;  // type for the fn-button dialog
        QString             defaultValue;  // initial line-edit text (may be empty)
        bool                fnVisible = true;
    };

    MultiEntryFunction(QList<EntrySpec> specs, QWidget* parent = nullptr);

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

protected:
    QList<EntrySpec>           m_specs;
    QList<FunctionEntryRow*>   m_rows;
};

#endif // MULTIENTRYFUNCTION_HPP

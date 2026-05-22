#ifndef SINGLEENTRYFUNCTION_HPP
#define SINGLEENTRYFUNCTION_HPP

#include "FunctionWidget.hpp"

class FunctionEntryRow;

/**
 * Base class for functions with one labeled input that serializes as
 *     <Fun><Name>X</Name><Entry>value</Entry></Fun>.
 *
 * Subclasses only declare id/xmlName/displayName/supportedReturnTypes
 * and the label text + nested-dialog return type for the row.
 */
class SingleEntryFunction : public FunctionWidget {
    Q_OBJECT

public:
    SingleEntryFunction(const QString& labelText,
                        FunctionReturnType rowReturnType,
                        QWidget* parent = nullptr);

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

protected:
    FunctionEntryRow* m_row;
};

#endif // SINGLEENTRYFUNCTION_HPP

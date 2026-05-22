#ifndef RANGEFUNCTION_HPP
#define RANGEFUNCTION_HPP

#include "FunctionWidget.hpp"

class FunctionEntryRow;

/**
 * Base class for functions that take a lower/upper-bound pair and
 * serialize as
 *     <Fun><Name>X</Name><Low>...</Low><High>...</High></Fun>.
 *
 * Initial 0/1 defaults are set in the constructor (the legacy code
 * forced them on every combo-box selection).
 */
class RangeFunction : public FunctionWidget {
    Q_OBJECT

public:
    RangeFunction(FunctionReturnType rowReturnType, QWidget* parent = nullptr);

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

protected:
    FunctionEntryRow* m_lowRow;
    FunctionEntryRow* m_highRow;
};

#endif // RANGEFUNCTION_HPP

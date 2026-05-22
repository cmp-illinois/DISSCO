#ifndef FUNCTIONWIDGET_HPP
#define FUNCTIONWIDGET_HPP

#include "../../lassie.hpp"

#include <QList>
#include <QString>
#include <QWidget>
#include <QXmlStreamReader>

#include <functional>

/**
 * Base class for the per-function input widgets shown inside
 * FunctionGenerator's stacked widget.
 *
 * The legacy FunctionGenerator handles every CMODFunction inside one
 * monolithic class: ~33 stacked-widget pages, ~100 slots, and matching
 * branches in the XML read and write paths. This base class is the
 * abstraction we're migrating to — each function becomes one subclass
 * that owns its own UI, its own slots, and both directions of XML I/O.
 *
 * Subclasses are registered with FunctionRegistry at startup and
 * instantiated on demand by FunctionGenerator.
 */
class FunctionWidget : public QWidget {
    Q_OBJECT

public:
    /** Callback type used by nestedFunctionRequested. Aliased because Qt's
     *  MOC mis-parses raw `std::function<void(const QString&)>` in signal
     *  declarations (the inner comma trips its template tokenizer). */
    using NestedAcceptCallback = std::function<void(const QString&)>;

    explicit FunctionWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    ~FunctionWidget() override = default;

    /** Stable identity used as the registry key. */
    virtual CMODFunction id() const = 0;

    /** Tag written under <Fun><Name>...</Name></Fun>, also the lookup key
     *  when parsing incoming function strings (e.g. "Random"). */
    virtual QString xmlName() const = 0;

    /** Label shown in FunctionGenerator's combo box. */
    virtual QString displayName() const = 0;

    /** Return-type contexts in which this function should appear in the
     *  picker. Most functions support a single type; some appear in
     *  multiple (e.g. Random for both Int and Float pickers). */
    virtual QList<FunctionReturnType> supportedReturnTypes() const = 0;

    /** Serialize the widget's current field values to an XML string.
     *  Convention: returns "<Fun><Name>xmlName()</Name>...</Fun>". */
    virtual QString buildXMLString() const = 0;

    /** Restore field values from a parsed XML element. The reader is
     *  positioned immediately after the <Name>...</Name> child; subclasses
     *  consume the remaining siblings up to the closing </Fun>. */
    virtual void populateFromXML(QXmlStreamReader& reader) = 0;

    /** Clear all fields back to their default state. */
    virtual void reset() = 0;

    // -----------------------------------------------------------------
    // Shared XML helpers used by populateFromXML implementations.
    //
    // The on-disk format mixes two child shapes under <Fun>: leaf text
    // (e.g. "<Low>0</Low>") and nested elements (e.g. "<Low><Fun>...</Fun></Low>"
    // for a nested function). These helpers handle both: readInner
    // captures whatever lives between the current StartElement and its
    // matching EndElement, returning either the text or a serialized
    // inner-XML string.
    // -----------------------------------------------------------------

    /** Read the body of the current element. The reader must be at a
     *  StartElement; on return it is at the matching EndElement. */
    static QString readInner(QXmlStreamReader& reader);

    /** Advance to the next child StartElement and return its body via
     *  readInner. Returns an empty string if no more children remain. */
    static QString nextChildInner(QXmlStreamReader& reader);

signals:
    /** Emitted whenever a user edit may have changed the output XML.
     *  FunctionGenerator listens to refresh its preview text edit. */
    void xmlChanged();

    /** Emitted when an "Insert Function" button is clicked. The host
     *  FunctionGenerator handles this by launching a nested dialog of
     *  the given return type; on accept, the dialog's result string is
     *  passed to onAccept. The callback indirection keeps the widget
     *  agnostic of where the resulting text lands (a QLineEdit, a row
     *  of a custom widget, etc.). */
    void nestedFunctionRequested(FunctionReturnType returnType,
                                 NestedAcceptCallback onAccept);
};

#endif // FUNCTIONWIDGET_HPP

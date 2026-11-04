#ifndef FUNCTIONREGISTRY_HPP
#define FUNCTIONREGISTRY_HPP

#include "FunctionWidget.hpp"
#include "../../lassie.hpp"

#include <QList>
#include <QMap>
#include <QString>

#include <functional>

/**
 * Process-wide registry of FunctionWidget subclasses.
 *
 * Each subclass is registered once at startup via registerType<T>(). The
 * registry instantiates a short-lived prototype to read the type's
 * metadata (id, xmlName, displayName, supportedReturnTypes) and stores a
 * factory closure for producing fresh instances at runtime.
 *
 * FunctionGenerator queries the registry in two directions:
 *  - functionsFor(rt)        : populate the combo box for a return-type context.
 *  - create(id) / idFromXmlName(name)
 *                            : produce the widget that handles a given function.
 *
 * Lookup is by CMODFunction (registry key) or by the XML `<Name>` tag
 * (the on-disk serialization key). Keeping these decoupled lets us
 * eventually treat CMODFunction as a pure in-memory id without churning
 * the file format.
 */
class FunctionRegistry {
public:
    static FunctionRegistry& instance();

    /** Register a FunctionWidget subclass. Must be called before any
     *  call to create() / functionsFor() / idFromXmlName(). Typically
     *  invoked once at startup from a central registration site. */
    template <typename T>
    void registerType(bool inMenu = true) {
        T proto;
        const CMODFunction id = proto.id();
        const QString xml = proto.xmlName();
        Entry e;
        e.factory = []() -> FunctionWidget* { return new T(); };
        e.xmlName = xml;
        e.displayName = proto.displayName();
        e.returnTypes = proto.supportedReturnTypes();
        e.inMenu = inMenu;
        m_entries.insert(id, e);
        m_order.append(id);
        m_byXmlName.insert(xml, id);
    }

    /** Also recognize `oldXmlName` as `id`, so fields saved under a
     *  function's old name still open in the picker. The picker writes
     *  the current name. -- Tony Chai, 2026 */
    void registerOldXmlName(const QString& oldXmlName, CMODFunction id) {
        m_byXmlName.insert(oldXmlName, id);
    }

    /** Instantiate the widget for the given id. Returns nullptr if no
     *  type is registered for that id. Caller owns the returned widget. */
    FunctionWidget* create(CMODFunction id) const;

    /** Look up a function id by its XML `<Name>` tag. Returns
     *  CMODFunction::NOT_A_FUNCTION if no registered type uses that tag. */
    CMODFunction idFromXmlName(const QString& xmlName) const;

    /** All registered functions that should appear in the picker for the
     *  given return-type context, in registration order. */
    QList<CMODFunction> functionsFor(FunctionReturnType returnType) const;

    /** Metadata lookups (cached at registration time). */
    QString xmlName(CMODFunction id) const;
    QString displayName(CMODFunction id) const;
    bool isRegistered(CMODFunction id) const;

private:
    FunctionRegistry() = default;
    FunctionRegistry(const FunctionRegistry&) = delete;
    FunctionRegistry& operator=(const FunctionRegistry&) = delete;

    struct Entry {
        std::function<FunctionWidget*()> factory;
        QString xmlName;
        QString displayName;
        QList<FunctionReturnType> returnTypes;
        bool inMenu = true;   // false: still read, but not offered in the picker
    };

    QMap<CMODFunction, Entry> m_entries;
    QList<CMODFunction> m_order;          // registration order, stable combo-box order
    QMap<QString, CMODFunction> m_byXmlName;
};

#endif // FUNCTIONREGISTRY_HPP

#include "FunctionRegistry.hpp"

FunctionRegistry& FunctionRegistry::instance() {
    static FunctionRegistry s;
    return s;
}

FunctionWidget* FunctionRegistry::create(CMODFunction id) const {
    const auto it = m_entries.find(id);
    if (it == m_entries.end()) return nullptr;
    return it->factory();
}

CMODFunction FunctionRegistry::idFromXmlName(const QString& xmlName) const {
    const auto it = m_byXmlName.find(xmlName);
    if (it == m_byXmlName.end()) return CMODFunction::NOT_A_FUNCTION;
    return it.value();
}

QList<CMODFunction> FunctionRegistry::functionsFor(FunctionReturnType returnType) const {
    QList<CMODFunction> result;
    for (CMODFunction id : m_order) {
        const Entry& e = m_entries.value(id);
        if (e.returnTypes.contains(returnType)) result.append(id);
    }
    return result;
}

QString FunctionRegistry::xmlName(CMODFunction id) const {
    const auto it = m_entries.find(id);
    return it == m_entries.end() ? QString() : it->xmlName;
}

QString FunctionRegistry::displayName(CMODFunction id) const {
    const auto it = m_entries.find(id);
    return it == m_entries.end() ? QString() : it->displayName;
}

bool FunctionRegistry::isRegistered(CMODFunction id) const {
    return m_entries.contains(id);
}

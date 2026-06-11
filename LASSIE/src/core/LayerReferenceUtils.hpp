#ifndef LAYERREFERENCEUTILS_HPP
#define LAYERREFERENCEUTILS_HPP

/**
 * @file LayerReferenceUtils.hpp
 * @brief Helpers for reconciling cross-event Package references after a
 *        rename or delete.
 *
 * Every layer of every HEvent can hold a list of Packages — the
 * (type, name)-keyed pointers an event uses to refer to a child event. When
 * a referenced event is renamed or deleted those references go stale; the
 * functions in @ref LayerRefs walk the whole project once, collect every
 * matching reference into an @ref LayerRefs::Assoc, and then offer two
 * resolutions: rewrite the name (@ref LayerRefs::applyRename) or drop the
 * referencing packages entirely (@ref LayerRefs::applyDelete).
 *
 * All routines are inline and header-only — the file is small and is
 * included where event rename/delete operations are performed.
 */

/**
 * @namespace LayerRefs
 * @brief Project-wide scan/mutate utilities for layer-package references.
 */

#include <QList>
#include <QPair>
#include <QString>
#include <algorithm>
#include <functional>

#include "event_struct.hpp"
#include "project_struct.hpp"
#include "../inst.hpp"

namespace LayerRefs {

/**
 * @brief Identifies one specific layer inside one specific HEvent.
 *
 * Acts as the key half of an @ref Assoc entry. `ownerIndex` is ignored for
 * the top event (there is only one) and indexes into the corresponding
 * QList for high / mid / low / bottom owners.
 */
struct OwnerKey {
    Eventtype ownerType;
    int       ownerIndex;  ///< Ignored when @ref ownerType == top.
    int       layerIndex;  ///< Index into the owner's `event_layers` list.
};
/**
 * @brief Result of a project-wide reference scan.
 *
 * Each entry pairs an @ref OwnerKey with the indices, within that layer's
 * `discrete_packages` list, of the Packages that matched the (type, name)
 * the scan was looking for.
 */
using Assoc = QList<QPair<OwnerKey, QList<int>>>;

/**
 * @brief Find every Package across the current project that points at an
 *        event identified by (type, name).
 * @param typeNumStr  numeric Eventtype the package's `event_type` must equal
 * @param name        package `event_name` to match
 * @return list of matching references, grouped by owning layer
 */
inline Assoc collect(const QString& typeNumStr, const QString& name) {
    Assoc refs;
    ProjectManager* pm = Inst::get_project_manager();

    auto scan = [&](HEvent& h, Eventtype ownerType, int ownerIndex) {
        for (int li = 0; li < h.event_layers.size(); ++li) {
            const QList<Package>& pkgs = h.event_layers[li].discrete_packages;
            QList<int> hits;
            for (int pi = 0; pi < pkgs.size(); ++pi) {
                if (pkgs[pi].event_type == typeNumStr &&
                    pkgs[pi].event_name == name) {
                    hits.append(pi);
                }
            }
            if (!hits.isEmpty())
                refs.append(qMakePair(OwnerKey{ownerType, ownerIndex, li}, hits));
        }
    };

    scan(pm->topevent(), top, -1);
    for (int i = 0; i < pm->highevents().size(); ++i)
        scan(pm->highevents()[i], high, i);
    for (int i = 0; i < pm->midevents().size(); ++i)
        scan(pm->midevents()[i], mid, i);
    for (int i = 0; i < pm->lowevents().size(); ++i)
        scan(pm->lowevents()[i], low, i);
    for (int i = 0; i < pm->bottomevents().size(); ++i)
        scan(pm->bottomevents()[i].event, bottom, i);

    return refs;
}

/**
 * @brief Dereference an @ref OwnerKey to the actual HEvent in the project.
 * @return pointer to the owning HEvent, or nullptr if the index is out of range
 */
inline HEvent* resolveOwner(const OwnerKey& k) {
    ProjectManager* pm = Inst::get_project_manager();
    switch (k.ownerType) {
        case top:    return &pm->topevent();
        case high:   return (k.ownerIndex < pm->highevents().size())   ? &pm->highevents()[k.ownerIndex]         : nullptr;
        case mid:    return (k.ownerIndex < pm->midevents().size())    ? &pm->midevents()[k.ownerIndex]          : nullptr;
        case low:    return (k.ownerIndex < pm->lowevents().size())    ? &pm->lowevents()[k.ownerIndex]          : nullptr;
        case bottom: return (k.ownerIndex < pm->bottomevents().size()) ? &pm->bottomevents()[k.ownerIndex].event : nullptr;
        default:     return nullptr;
    }
}

/**
 * @brief Rewrite the `event_name` field of every package collected in @p refs.
 * @param refs     references previously gathered by @ref collect
 * @param newName  replacement name
 */
inline void applyRename(const Assoc& refs, const QString& newName) {
    for (const auto& entry : refs) {
        HEvent* h = resolveOwner(entry.first);
        if (!h || entry.first.layerIndex >= h->event_layers.size()) continue;
        QList<Package>& pkgs = h->event_layers[entry.first.layerIndex].discrete_packages;
        for (int pi : entry.second)
            if (pi >= 0 && pi < pkgs.size()) pkgs[pi].event_name = newName;
    }
}

/**
 * @brief Remove every package collected in @p refs from its owning layer.
 *
 * Indices are processed in descending order so that earlier removals do not
 * invalidate later ones.
 */
inline void applyDelete(const Assoc& refs) {
    for (const auto& entry : refs) {
        HEvent* h = resolveOwner(entry.first);
        if (!h || entry.first.layerIndex >= h->event_layers.size()) continue;
        QList<Package>& pkgs = h->event_layers[entry.first.layerIndex].discrete_packages;
        QList<int> desc = entry.second;
        std::sort(desc.begin(), desc.end(), std::greater<int>());
        for (int pi : desc)
            if (pi >= 0 && pi < pkgs.size()) pkgs.removeAt(pi);
    }
}

/// @brief Total number of individual package references across @p refs.
inline int totalCount(const Assoc& refs) {
    int n = 0;
    for (const auto& entry : refs) n += entry.second.size();
    return n;
}

} // namespace LayerRefs

#endif

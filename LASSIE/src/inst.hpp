#ifndef INST_HPP
#define INST_HPP

#include <QObject>
#include <QAction>

#include "core/project_struct.hpp"

/**
 * @brief Process-wide singleton that owns LASSIE's top-level services.
 *
 * Inst is the single root from which every window, dialog, and widget reaches
 * shared state. Today it exposes exactly one service — the ProjectManager
 * that holds the currently-open Project — but the class exists so that
 * additional global services (preferences, recent-file lists, the Markov
 * model registry, etc.) can be hung off `Inst::instance()` without rewiring
 * the rest of the GUI.
 *
 * Construction is lazy: the first call to instance() builds the singleton
 * and its ProjectManager. The instance is never freed; QApplication's
 * teardown is relied on for process exit.
 *
 * @note Convenience accessor get_project_manager() is what most callers use;
 *       direct `Inst::instance()` access is reserved for code that needs to
 *       hold the QObject for signal/slot connections.
 */
class Inst : public QObject {
    Q_OBJECT

    public:
        static Inst* instance(void){
            static Inst *that = new Inst;
            return that;
        }

        static ProjectManager* get_project_manager() { return instance()->project_manager_; }
        
    private:
        Inst(void){
            project_manager_ = new ProjectManager();
        }

        ProjectManager *project_manager_;

};

#endif
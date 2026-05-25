#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <memory>
#include <QStatusBar>

class QListWidgetItem;
class QTreeWidgetItem;

#include "../inst.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EnvelopeLibraryWindow;
class MarkovModelLibraryWindow; //nhi: more descriptive than MarkovWindow
class ProjectView;

/**
 * @brief Top-level Qt main window for LASSIE.
 *
 * MainWindow is the application's root widget. It owns the menus, toolbars,
 * status bar, recent-projects landing page, and the embedded ProjectView
 * shown once a project is loaded. The two satellite library windows for
 * envelope and Markov-model editing are owned here as unique_ptrs so they
 * close cleanly when the app exits.
 *
 * Construction takes the global @ref Inst singleton; MainWindow registers
 * itself as a static instance() so dialogs that need to post status messages
 * can find it without a parent pointer.
 *
 * @note Project lifecycle (new / open / save / close) is mediated through
 *       maybeSaveBeforeClose() and closeCurrentProject() to keep the
 *       project model and the visible UI in sync.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    public:
        //MainWindow(QWidget *parent = nullptr);
        /// Build the main window and bind it to the application singleton.
        explicit MainWindow(Inst*);
        /// Access the most recently constructed MainWindow.
        static MainWindow* instance() { return instance_; }
        ~MainWindow() override;
        /// Update the window title to reflect @p file (with a [modified] tag if requested).
        void setCurrentFile(const QString &file, bool modified = false);

        std::unique_ptr<Ui::MainWindow> ui;
        std::unique_ptr<EnvelopeLibraryWindow> envelopeLibraryWindow;
        std::unique_ptr<MarkovModelLibraryWindow> markovWindow;

    public slots:
        void showStatusMessage(const QString& message) const { statusBar()->showMessage(message, 5000); }

    private slots:
        // File operations
        void newFile();
        void openFile();
        void saveFile();
        void saveFileAs();

        // Edit operations
         /* TODO: implement undo /edo */
        void undo() { statusBar()->showMessage(tr("Undo"), 2000); }
        void redo() { statusBar()->showMessage(tr("Redo"), 2000); }

        // Window operations
        void showEnvelopeLibraryWindow() const;
        void showMarkovWindow() const;
        void showPropertiesDialog() const;
        void showFileNewObjectDialog() const;

        // Welcome / recent projects
        void openRecentProject(QListWidgetItem *item);
        void openRecentProjectTree(QTreeWidgetItem *item, int column);
        void toggleRecentViewMode();
        void showRecentListContextMenu(const QPoint &pos);
        void showRecentTreeContextMenu(const QPoint &pos);

        

    protected:
        void closeEvent(QCloseEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        void readSettings();
        void writeSettings() const;

    private:
        void createActions();
        void enableProjectActions(bool enabled) const;
        void createMenus();
        void createToolBars();
        void createStatusBar() const { statusBar()->showMessage(tr("Ready")); }
        void runProject();
        void showFile();
        void openProjectPath(const QString &path);

        // Returns false if the user cancelled, true otherwise.
        // Prompts to save unsaved changes when a project is already open.
        bool maybeSaveBeforeClose();
        // Tears down the current project view and project data, resetting UI state.
        void closeCurrentProject();

        // Welcome / recent projects helpers
        void showWelcomePage();
        void showProjectPage() const;
        void refreshRecentProjects() const;
        void addToRecentProjects(const QString &path) const;
        void applyRecentViewMode() const;
        QStringList selectedRecentPaths() const;
        void removeRecents(const QStringList &paths) const;
        void promptAndRemoveSelectedRecents();

        QString currentFile;
        bool recentIconMode = true;
        
        // Actions
        QAction *newAct = nullptr;
        QAction *openAct = nullptr;
        QAction *saveAct = nullptr;
        QAction *saveAsAct = nullptr;
        QAction *exitAct = nullptr;

        QAction *undoAct = nullptr;
        QAction *redoAct = nullptr;
        QAction *cutAct = nullptr;
        QAction *copyAct = nullptr;
        QAction *pasteAct = nullptr;

        QAction *newObjAct = nullptr;
        QAction *setPropAct = nullptr;
        QAction *runAct = nullptr;
        QAction *configNoteModAct = nullptr;

        QAction *showEnvelopeLibraryAct = nullptr;
        QAction *showMarkovAct = nullptr;
        QAction *aboutAct = nullptr;
        QAction *aboutQtAct = nullptr;

        // Menus
        QMenu *fileMenu = nullptr;
        QMenu *editMenu = nullptr;
        QMenu *projectMenu = nullptr;
        QMenu *viewMenu = nullptr;
        QMenu *helpMenu = nullptr;

        // Toolbars
        QToolBar *fileToolBar = nullptr;
        QToolBar *projectToolBar = nullptr;
        QToolBar *editToolBar = nullptr;

        QStatusBar *statusbar_ = nullptr;
        
        // projectView pointer for testing
        ProjectView* projectView = nullptr;
        
        Inst *main_;
        static MainWindow *instance_;
};

#endif // MAINWINDOW_HPP

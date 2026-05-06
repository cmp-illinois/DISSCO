#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <memory>
#include <QStatusBar>

#include "../inst.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EnvelopeLibraryWindow;
class MarkovModelLibraryWindow; //nhi: more descriptive than MarkovWindow
class ProjectView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    public:
        //MainWindow(QWidget *parent = nullptr);
        explicit MainWindow(Inst*);
        static MainWindow* instance() { return instance_; }
        ~MainWindow() override;
        void setCurrentFile(const QString &file, bool modified = false);
        void openFile(const QString &path);

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

        

    protected:
        void closeEvent(QCloseEvent *event) override;
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

        // Returns false if the user cancelled, true otherwise.
        // Prompts to save unsaved changes when a project is already open.
        bool maybeSaveBeforeClose();
        // Tears down the current project view and project data, resetting UI state.
        void closeCurrentProject();

        QString currentFile;
        
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

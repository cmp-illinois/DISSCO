#include "MainWindow.hpp"
#include "../ui/ui_mainwindow.h"

#include "EnvelopeLibraryWindow.hpp"
#include "MarkovModelLibraryWindow.hpp"
#include "../widgets/ProjectViewController.hpp"
#include "PostWindow.hpp"

#include "../core/project_struct.hpp"
#include "../core/Updater.hpp"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QTabWidget>
#include <QCloseEvent>
#include <QSettings>
#include <QStyle>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <QProcess>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStackedWidget>
#include <QToolButton>
#include <QFileIconProvider>
#include <QLocale>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QTimer>

MainWindow *MainWindow::instance_ = nullptr;

namespace {
    constexpr int kMaxRecentProjects = 12;
    constexpr const char *kRecentProjectsKey = "recentProjects";
    constexpr const char *kRecentViewModeIconKey = "recentViewModeIcon";
}

static QString resolveCmodBinary()
{
    // Prefer a CMOD sitting next to the LASSIE binary (packaged builds:
    // LASSIE.app/Contents/MacOS/CMOD on macOS, the AppDir's usr/bin/CMOD on
    // Linux). Fall back to the compile-time CMOD_BINARY path for dev builds.
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &candidate : { appDir + "/CMOD", appDir + "/../Resources/CMOD" }) {
        if (QFileInfo(candidate).isExecutable()) {
            return QDir::cleanPath(candidate);
        }
    }
    return QStringLiteral(CMOD_BINARY);
}

MainWindow::MainWindow(Inst* m)
    : QMainWindow()
    , ui(std::make_unique<Ui::MainWindow>())
    , envelopeLibraryWindow(std::make_unique<EnvelopeLibraryWindow>(this))
    , markovWindow(std::make_unique<MarkovModelLibraryWindow>(this))
{
    Q_ASSERT(instance_ == nullptr);
    instance_ = this;
    main_ = m;

    ui->setupUi(this);

    createActions();
    enableProjectActions(false);
    createMenus();
    createToolBars();
    createStatusBar();

    readSettings();
    applyRecentViewMode();
    refreshRecentProjects();
    showWelcomePage();

    setWindowTitle(tr("LASSIE"));
    setUnifiedTitleAndToolBarOnMac(true);

    connect(ui->envButton, &QPushButton::clicked, this, &MainWindow::showEnvelopeLibraryWindow);
    connect(ui->markovButton, &QPushButton::clicked, this, &MainWindow::showMarkovWindow);
    connect(ui->viewModeButton, &QToolButton::clicked, this, &MainWindow::toggleRecentViewMode);
    connect(ui->recentProjectsList, &QListWidget::itemActivated, this, &MainWindow::openRecentProject);
    connect(ui->recentProjectsTree, &QTreeWidget::itemActivated, this, &MainWindow::openRecentProjectTree);

    ui->recentProjectsList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->recentProjectsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->recentProjectsList, &QWidget::customContextMenuRequested,
            this, &MainWindow::showRecentListContextMenu);
    connect(ui->recentProjectsTree, &QWidget::customContextMenuRequested,
            this, &MainWindow::showRecentTreeContextMenu);
    ui->recentProjectsList->installEventFilter(this);
    ui->recentProjectsTree->installEventFilter(this);

    QHeaderView *header = ui->recentProjectsTree->header();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(Inst::get_project_manager(), &ProjectManager::dataModified,
            this, [this]{ setWindowModified(true); });

    updater_ = new Updater(this);
    if (Updater::shouldAutoCheckNow()) {
        // Defer until the event loop is running so the main window has
        // painted before any dialog can appear.
        QTimer::singleShot(0, this, [this]() {
            updater_->checkForUpdates(Updater::Trigger::Auto);
        });
    }
}

//MainWindow::~MainWindow() = default;
MainWindow::~MainWindow() {
    if(projectView != nullptr)
        delete projectView;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

bool MainWindow::maybeSaveBeforeClose()
{
    if (!projectView)
        return true;

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Save Changes"),
        tr("Would you like to save your changes to the current project?"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
    );

    if (reply == QMessageBox::Yes)
        saveFile();

    return reply != QMessageBox::Cancel;
}

void MainWindow::closeCurrentProject()
{
    if (!projectView)
        return;

    // Detach and hide the auxiliary windows that reference the current project
    envelopeLibraryWindow->hide();
    markovWindow->hide();

    // Delete the view — its destructor removes palette/attributes widgets from the UI
    delete projectView;
    projectView = nullptr;
    
    // Grab the project pointer before tearing down the view
    ProjectManager *pm = Inst::get_project_manager();
    // Delete the project data
    pm->close(pm->get_curr_project());

    // Reset UI to the no-project state
    enableProjectActions(false);
    openAct->setEnabled(true);
    newAct->setEnabled(true);

    setCurrentFile(QString(), false);

    refreshRecentProjects();
    showWelcomePage();
}

void MainWindow::newFile()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("New Project"),
                                                  QString(),
                                                  tr("DISSCO Files (*.dissco);;All Files (*)"));
    if (fileName.isEmpty() || !maybeSaveBeforeClose())
        return;
    
    closeCurrentProject();

    const QFileInfo fileInfo(fileName);
    const QString projectName = fileInfo.completeBaseName();
    const QString projectFolder = fileInfo.absolutePath() + "/" + projectName;
    if (const QDir dir; !dir.exists(projectFolder))
        dir.mkdir(projectFolder);

    const QString fullFilePath = projectFolder + "/" + projectName + ".dissco";
    currentFile = fullFilePath;

    setCurrentFile(currentFile, true);
    Inst::get_project_manager()->build(currentFile, nullptr);
    addToRecentProjects(currentFile);
    showFile();
}

void MainWindow::openFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                  QString(),
                                                  tr("DISSCO Files (*.dissco);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    openProjectPath(fileName);
}

void MainWindow::openProjectPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("LASSIE"),
                        tr("Cannot read file %1:\n%2.")
                        .arg(QDir::toNativeSeparators(path),
                            file.errorString()));
        // Remove a missing/unreadable entry from the recents list so it doesn't keep haunting the welcome page.
        QSettings settings;
        QStringList recents = settings.value(kRecentProjectsKey).toStringList();
        if (recents.removeAll(path) > 0) {
            settings.setValue(kRecentProjectsKey, recents);
            refreshRecentProjects();
        }
        return;
    }
    file.close();

    if (!maybeSaveBeforeClose())
        return;

    closeCurrentProject();

    currentFile = path;
    Inst::get_project_manager()->open(currentFile, nullptr);
    addToRecentProjects(currentFile);
    showFile();
}

void MainWindow::saveFile()
{    
    //nhi: ensure directory exists before saving
    const QFileInfo fileInfo(currentFile);
    if (const QDir dir = fileInfo.absoluteDir(); !dir.exists()) {
        if (!dir.mkpath(".")) {
            QMessageBox::critical(this, tr("Error"),
                                tr("Failed to create directory:\n%1")
                                .arg(dir.absolutePath()));
            return;
        }
    }
    
    projectView->save();

    //nhi: update window title and status after successful save
    Inst::get_project_manager()->modified() = false;
    setWindowModified(false);
    statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveFileAs()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                  currentFile,
                                                  tr("DISSCO Files (*.dissco);;All Files (*)"));
    if (!fileName.isEmpty()){
        currentFile = fileName;
        ProjectManager *pm = Inst::get_project_manager();
        pm->fileinfo() = QFileInfo(currentFile);
        saveFile();
        addToRecentProjects(currentFile);
    }
}

void MainWindow::showEnvelopeLibraryWindow() const {
    if (envelopeLibraryWindow->isVisible()) {
        envelopeLibraryWindow->raise();
        envelopeLibraryWindow->activateWindow();
        return;
    }

    envelopeLibraryWindow->show();
    if (projectView) {
        envelopeLibraryWindow->setActiveProject(projectView);
    }
}

void MainWindow::showMarkovWindow() const {
    // If the window is already open, just bring it to the front / refocus it
    // instead of re-running setActiveProject (which would discard the current
    // editor state, undo history, and selection).
    if (markovWindow->isVisible()) {
        markovWindow->raise();
        markovWindow->activateWindow();
        return;
    }

    markovWindow->show();
    if (projectView) {
        markovWindow->setActiveProject(projectView);
    }
}

void MainWindow::showPropertiesDialog() const {
    projectView->setProperties();
}

void MainWindow::showFileNewObjectDialog() const {
    projectView->insertObject();
    
}

void MainWindow::runProject()
{
    ProjectManager *pm = Inst::get_project_manager();
    if(pm->modified()){
        QMessageBox msgbox;
        msgbox.setText("This project has been modified.");
        msgbox.setText("Do you want to save your changes before running?");
        msgbox.setStandardButtons(QMessageBox::Save | QMessageBox::Ignore | QMessageBox::Cancel);

        switch(msgbox.exec()) {
            case QMessageBox::Save:
                saveFile();
                break;
            case QMessageBox::Ignore:
                break;
            case QMessageBox::Cancel:
                return;
        }
    }

    bool ok{};
    const QString seed = QInputDialog::getText(this, tr("QInputDialog::getText()"), 
                                        tr("Enter a seed:"), QLineEdit::Normal,
                                        "abcd", &ok);
    if(!ok) return;
    pm->seed() = seed;
    pm->writeSeedEntry(seed);

    using namespace Qt::StringLiterals;

    const auto cmod = new QProcess(this);
    connect(cmod, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            [=](const int exit_code)
            { 
                statusBar()->showMessage(tr("CMOD exited with code %1").arg(exit_code)); 
            }
        );
    const QString cmodBinary = resolveCmodBinary();
    qDebug() << "Project run with string:" << cmodBinary + " " + pm->fileinfo().canonicalFilePath();

    const auto pw = new PostWindow(cmod, this);
    pw->resize(600,400);
    pw->show();

    cmod->start(cmodBinary, QStringList() << pm->fileinfo().canonicalFilePath());
}

void MainWindow::readSettings()
{
    const QSettings settings;
    if (const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray(); geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() * 2/3, availableGeometry.height() * 2/3);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
    recentIconMode = settings.value(kRecentViewModeIconKey, true).toBool();
}

void MainWindow::writeSettings() const {
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::createActions()
{
    // File actions
    newAct = new QAction(QIcon::fromTheme("document-new"), tr("&New Project"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new project"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    
    openAct = new QAction(QIcon::fromTheme("document-open"), tr("&Open Project"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing project"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
    
    saveAct = new QAction(QIcon::fromTheme("document-save"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the project to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);
    
    saveAsAct = new QAction(QIcon::fromTheme("document-save"), tr("Save &As"), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the project under a new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveFileAs);
    
    exitAct = new QAction(QIcon::fromTheme("application-exit"), tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setShortcutContext(Qt::ApplicationShortcut);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    /* These seem to take precedence over widget-specific copy/paste/undo/redo associations, which is generally bad. 
     * Initializing these (ESPECIALLY initializing these to QKeySequences shared with other widget-specific shortcuts)
     * will cause those widget-specific shortcuts to not work.
     * \todo Rectify the copy/paste in the menubar to refer to the scope of whatever is being selected/viewed.
     * - Jacob, 2/26/26
     */
    // Edit actions
    // undoAct = new QAction(QIcon::fromTheme("edit-undo"), tr("&Undo"), this);
    // undoAct->setShortcuts(QKeySequence::Undo);
    // undoAct->setStatusTip(tr("Undo the last operation"));
    // connect(undoAct, &QAction::triggered, this, &MainWindow::undo);

    // redoAct = new QAction(QIcon::fromTheme("edit-redo"), tr("&Redo"), this);
    // redoAct->setShortcuts(QKeySequence::Redo);
    // redoAct->setStatusTip(tr("Redo the last operation"));
    // connect(redoAct, &QAction::triggered, this, &MainWindow::redo);
    
    // cutAct = new QAction(QIcon::fromTheme("edit-cut"), tr("Cu&t"), this);
    // cutAct->setShortcuts(QKeySequence::Cut);
    // cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    
    // copyAct = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    // copyAct->setShortcuts(QKeySequence::Copy);
    // copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    // connect(copyAct, &QAction::triggered, this, [](){
    //     qDebug() << "[DEBUG] MainWindow window-scope copyAct triggered (Ctrl/Cmd+C eaten here)";
    // });

    // pasteAct = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    // pasteAct->setShortcuts(QKeySequence::Paste);
    // pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    // connect(pasteAct, &QAction::triggered, this, [](){
    //     qDebug() << "[DEBUG] MainWindow window-scope pasteAct triggered (Ctrl/Cmd+V eaten here)";
    // });

    // Project actions
    newObjAct = new QAction(QIcon::fromTheme("list-add"), tr("New Object"), this);
    newObjAct->setStatusTip(tr("Create a new object"));
    connect(newObjAct, &QAction::triggered, this, &MainWindow::showFileNewObjectDialog);

    setPropAct = new QAction(tr("Set Properties"), this);
    setPropAct->setStatusTip(tr("Set project properties"));
    connect(setPropAct, &QAction::triggered, this, &MainWindow::showPropertiesDialog);

    runAct = new QAction(tr("Run"), this);
    runAct->setStatusTip(tr("Run"));
    connect(runAct, &QAction::triggered, this, &MainWindow::runProject);

    configNoteModAct = new QAction(tr("Configure Note Modifiers"), this);
    configNoteModAct->setStatusTip(tr("Configure note modifiers"));

    // View actions
    showEnvelopeLibraryAct = new QAction(tr("&Envelope Library"), this);
    showEnvelopeLibraryAct->setStatusTip(tr("Show the envelope library window"));
    connect(showEnvelopeLibraryAct, &QAction::triggered, this, &MainWindow::showEnvelopeLibraryWindow);

    showMarkovAct = new QAction(tr("&Markov Library"), this);
    showMarkovAct->setStatusTip(tr("Show the Markov chain library window"));
    connect(showMarkovAct, &QAction::triggered, this, &MainWindow::showMarkovWindow);

    // Help actions
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About LASSIE"),
            tr("LASSIE (Library for Algorithmic Sound Synthesis and Interactive Exploration) "
               "is a tool for creating and manipulating sound synthesis algorithms.\n\n"
               "Version: %1\nBranch: %2\nCommit: %3")
                .arg(Updater::currentVersion().toString(),
                     QString::fromLatin1(GIT_BRANCH),
                     Updater::currentCommit().left(7)));
    });

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    checkForUpdatesAct = new QAction(tr("Check for &Updates..."), this);
    checkForUpdatesAct->setStatusTip(tr("Check GitHub for a newer LASSIE release"));
    connect(checkForUpdatesAct, &QAction::triggered, this, [this]() {
        if (updater_) {
            updater_->checkForUpdates(Updater::Trigger::Manual);
        }
    });

    autoCheckUpdatesAct = new QAction(tr("Check for Updates on &Startup"), this);
    autoCheckUpdatesAct->setStatusTip(tr("Automatically check for updates once per day on launch"));
    autoCheckUpdatesAct->setCheckable(true);
    autoCheckUpdatesAct->setChecked(QSettings().value(Updater::kAutoCheckKey, false).toBool());
    connect(autoCheckUpdatesAct, &QAction::toggled, this, [](const bool on) {
        QSettings().setValue(Updater::kAutoCheckKey, on);
    });
}

void MainWindow::enableProjectActions(const bool enabled) const {
    saveAct->setEnabled(enabled);
    saveAsAct->setEnabled(enabled);
    showEnvelopeLibraryAct->setEnabled(enabled);
    showMarkovAct->setEnabled(enabled);
    newObjAct->setEnabled(enabled);
    setPropAct->setEnabled(enabled);
    runAct->setEnabled(enabled);
    configNoteModAct->setEnabled(enabled);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    // editMenu->addAction(undoAct);
    // editMenu->addAction(redoAct);
    //editMenu->addSeparator();
    // editMenu->addAction(cutAct);
    // editMenu->addAction(copyAct);
    // editMenu->addAction(pasteAct);

    projectMenu = menuBar()->addMenu(tr("&Project"));
    projectMenu->addAction(newObjAct);
    projectMenu->addAction(setPropAct);
    projectMenu->addAction(runAct);
    projectMenu->addAction(configNoteModAct);
    
    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(showEnvelopeLibraryAct);
    viewMenu->addAction(showMarkovAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(checkForUpdatesAct);
    helpMenu->addAction(autoCheckUpdatesAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addAction(saveAsAct);
    fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    projectToolBar = addToolBar(tr("Project"));
    projectToolBar->addAction(newObjAct);
    //nhi: envelope library action moved to view menu instead of project toolbar
    projectToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    /* TODO: implement edit, undo/redo functionality */
    // editToolBar = addToolBar(tr("Edit"));
    // editToolBar->addAction(undoAct);
    // editToolBar->addAction(redoAct);
    // //editToolBar->addSeparator();
    // editToolBar->addAction(cutAct);
    // editToolBar->addAction(copyAct);
    // editToolBar->addAction(pasteAct);
    // fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::showFile()
{
    if(currentFile != nullptr){
        if(projectView == nullptr){
            projectView = new ProjectView(this, currentFile);

            setCurrentFile(currentFile, false);
            statusBar()->showMessage(tr("Project loaded"), 2000);
            projectView->setProperties();

            //nhi: connect envelope library to loaded project
            envelopeLibraryWindow->setActiveProject(projectView);

            showProjectPage();
            enableProjectActions(true);
        }else{
            qDebug() << "WARNING: file attempted to display while ProjectView is already allocated for an existing project.";
        }
    }else{
        qDebug() << "WARNING: file attempted to display when there is no file to display!";
    }
}

void MainWindow::setCurrentFile(const QString &file, bool modified){
    currentFile = file;
    if (currentFile.isEmpty())
        setWindowTitle(tr("LASSIE"));
    else
        setWindowTitle(tr("%1[*] - %2").arg(currentFile, tr("LASSIE")));
    setWindowModified(modified);
}

void MainWindow::showWelcomePage()
{
    ui->mainStack->setCurrentWidget(ui->welcomePage);
}

void MainWindow::showProjectPage() const
{
    ui->mainStack->setCurrentWidget(ui->projectPage);
}

void MainWindow::refreshRecentProjects() const
{
    const QSettings settings;
    const QStringList recents = settings.value(kRecentProjectsKey).toStringList();

    ui->recentProjectsList->clear();
    ui->recentProjectsTree->clear();

    const QFileIconProvider iconProvider;
    const QIcon fallbackIcon = iconProvider.icon(QFileIconProvider::File);
    const QLocale locale;

    for (const QString &path : recents) {
        const QFileInfo info(path);

        QIcon icon = iconProvider.icon(info);
        if (icon.isNull())
            icon = fallbackIcon;

        // Icon-mode list
        auto *listItem = new QListWidgetItem(ui->recentProjectsList);
        listItem->setText(info.completeBaseName());
        listItem->setToolTip(QDir::toNativeSeparators(path));
        listItem->setData(Qt::UserRole, path);
        listItem->setIcon(icon);
        listItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);

        // Details tree (list mode)
        auto *treeItem = new QTreeWidgetItem(ui->recentProjectsTree);
        treeItem->setText(0, info.completeBaseName());
        treeItem->setIcon(0, icon);
        treeItem->setToolTip(0, QDir::toNativeSeparators(path));
        treeItem->setData(0, Qt::UserRole, path);

        if (info.exists()) {
            treeItem->setText(1, locale.toString(info.lastModified(), QLocale::ShortFormat));
            treeItem->setText(2, locale.formattedDataSize(info.size()));
        } else {
            treeItem->setText(1, tr("(missing)"));
            treeItem->setText(2, QString());
        }
    }
}

void MainWindow::addToRecentProjects(const QString &path) const
{
    if (path.isEmpty())
        return;

    const QString canonical = QFileInfo(path).absoluteFilePath();

    QSettings settings;
    QStringList recents = settings.value(kRecentProjectsKey).toStringList();

    // Remove any existing entry that refers to the same file, then prepend.
    recents.removeAll(canonical);
    recents.removeAll(path);
    recents.prepend(canonical);
    while (recents.size() > kMaxRecentProjects)
        recents.removeLast();

    settings.setValue(kRecentProjectsKey, recents);
    refreshRecentProjects();
}

void MainWindow::applyRecentViewMode() const
{
    if (recentIconMode) {
        ui->recentProjectsStack->setCurrentWidget(ui->recentProjectsList);
        ui->viewModeButton->setText(tr("List View"));
    } else {
        ui->recentProjectsStack->setCurrentWidget(ui->recentProjectsTree);
        ui->viewModeButton->setText(tr("Icon View"));
    }
}

void MainWindow::toggleRecentViewMode()
{
    recentIconMode = !recentIconMode;
    QSettings settings;
    settings.setValue(kRecentViewModeIconKey, recentIconMode);
    applyRecentViewMode();
}

void MainWindow::openRecentProject(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;
    openProjectPath(path);
}

void MainWindow::openRecentProjectTree(QTreeWidgetItem *item, int /*column*/)
{
    if (!item)
        return;
    const QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty())
        return;
    openProjectPath(path);
}

QStringList MainWindow::selectedRecentPaths() const
{
    QStringList paths;
    if (recentIconMode) {
        const QList<QListWidgetItem *> items = ui->recentProjectsList->selectedItems();
        paths.reserve(items.size());
        for (const QListWidgetItem *item : items) {
            const QString path = item->data(Qt::UserRole).toString();
            if (!path.isEmpty())
                paths.append(path);
        }
    } else {
        const QList<QTreeWidgetItem *> items = ui->recentProjectsTree->selectedItems();
        paths.reserve(items.size());
        for (const QTreeWidgetItem *item : items) {
            const QString path = item->data(0, Qt::UserRole).toString();
            if (!path.isEmpty())
                paths.append(path);
        }
    }
    return paths;
}

void MainWindow::removeRecents(const QStringList &paths) const
{
    if (paths.isEmpty())
        return;

    QSettings settings;
    QStringList recents = settings.value(kRecentProjectsKey).toStringList();
    for (const QString &path : paths)
        recents.removeAll(path);

    settings.setValue(kRecentProjectsKey, recents);
    refreshRecentProjects();
}

void MainWindow::promptAndRemoveSelectedRecents()
{
    const QStringList paths = selectedRecentPaths();
    if (paths.isEmpty())
        return;

    const QString message = paths.size() == 1
        ? tr("Remove \"%1\" from recent projects?\n\nThe project files on disk will not be affected.")
              .arg(QFileInfo(paths.first()).completeBaseName())
        : tr("Remove %1 entries from recent projects?\n\nThe project files on disk will not be affected.")
              .arg(paths.size());

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Remove from Recents"),
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes)
        removeRecents(paths);
}

void MainWindow::showRecentListContextMenu(const QPoint &pos)
{
    const QListWidgetItem *item = ui->recentProjectsList->itemAt(pos);
    if (!item)
        return;
    if (!item->isSelected())
        ui->recentProjectsList->setCurrentItem(const_cast<QListWidgetItem *>(item));

    const int count = ui->recentProjectsList->selectedItems().size();
    QMenu menu(this);
    QAction *removeAct = menu.addAction(count > 1
        ? tr("Remove %1 entries from Recents").arg(count)
        : tr("Remove from Recents"));
    if (menu.exec(ui->recentProjectsList->viewport()->mapToGlobal(pos)) == removeAct)
        removeRecents(selectedRecentPaths());
}

void MainWindow::showRecentTreeContextMenu(const QPoint &pos)
{
    const QTreeWidgetItem *item = ui->recentProjectsTree->itemAt(pos);
    if (!item)
        return;
    if (!item->isSelected())
        ui->recentProjectsTree->setCurrentItem(const_cast<QTreeWidgetItem *>(item));

    const int count = ui->recentProjectsTree->selectedItems().size();
    QMenu menu(this);
    QAction *removeAct = menu.addAction(count > 1
        ? tr("Remove %1 entries from Recents").arg(count)
        : tr("Remove from Recents"));
    if (menu.exec(ui->recentProjectsTree->viewport()->mapToGlobal(pos)) == removeAct)
        removeRecents(selectedRecentPaths());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == ui->recentProjectsList || watched == ui->recentProjectsTree)
            && event->type() == QEvent::KeyPress) {
        const auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
            promptAndRemoveSelectedRecents();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

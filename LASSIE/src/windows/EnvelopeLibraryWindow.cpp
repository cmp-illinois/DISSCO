#include "EnvelopeLibraryWindow.hpp"
#include "../widgets/EnvLibDrawingArea.hpp"
#include "../core/EnvelopeLibraryEntry.hpp"
#include "../core/ProjectClipboard.hpp"

#include <QTreeView>
#include <QItemSelectionModel>
#include <QModelIndexList>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QShortcut>

#include "../utilities.hpp"

namespace {

struct EnvelopeClipboardNode {
    double x;
    double y;
    envSegmentType type = envSegmentTypeLinear;
    envSegmentProperty property = envSegmentPropertyFlexible;
};

struct EnvelopeClipboard {
    QList<EnvelopeClipboardNode> nodes;
};

} // namespace

/**
 * @brief Constructor: builds the UI
 * @param parent  parent widget
 */
EnvelopeLibraryWindow::EnvelopeLibraryWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // Window setup
    setWindowTitle("Envelope Library");
    resize(760, 700);

    // Central widget and layout
    QWidget* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    // Add button row for create/duplicate/delete
    auto* buttonRow = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonRow);
    QPushButton* btnCreate = new QPushButton("Create New Envelope", this);
    QPushButton* btnDuplicate = new QPushButton("Duplicate Envelope", this);
    QPushButton* btnDelete = new QPushButton("Delete Envelope", this);
    buttonLayout->addWidget(btnCreate);
    buttonLayout->addWidget(btnDuplicate);
    buttonLayout->addWidget(btnDelete);
    buttonRow->setLayout(buttonLayout);
    layout->addWidget(buttonRow);
    connect(btnCreate, &QPushButton::clicked, this, &EnvelopeLibraryWindow::createNewEnvelope);
    connect(btnDuplicate, &QPushButton::clicked, this, &EnvelopeLibraryWindow::duplicateEnvelope);
    connect(btnDelete, &QPushButton::clicked, this, &EnvelopeLibraryWindow::deleteEnvelope);

    // Envelope list view (moved above drawing area)
    envelopeLibrary = new QTreeView(this);
    refModel = new QStandardItemModel(this);
    refModel->setHorizontalHeaderLabels({ "Envelope Library Number" });
    envelopeLibrary->setModel(refModel);
    envelopeLibrary->header()->setStretchLastSection(true);
    envelopeLibrary->setMinimumWidth(120);
    envelopeLibrary->setMinimumHeight(180);
    layout->addWidget(envelopeLibrary);

    // Drawing area (moved below envelope list)
    drawingArea = new EnvLibDrawingArea(this);
    layout->addWidget(drawingArea);

    // X/Y entry fields
    xEntry = new QLineEdit(this);
    yEntry = new QLineEdit(this);
    xEntry->setPlaceholderText("X value");
    yEntry->setPlaceholderText("Y value");

    // Connect X/Y entry fields to update node positions when text changes
    connect(xEntry, &QLineEdit::textChanged, this, &EnvelopeLibraryWindow::valueEntriesChanged);
    connect(yEntry, &QLineEdit::textChanged, this, &EnvelopeLibraryWindow::valueEntriesChanged);

    // Instead, create a horizontal layout for X/Y fields and legend:
    QHBoxLayout* xyLegendLayout = new QHBoxLayout();
    QWidget* xyWidget = new QWidget(this);
    QFormLayout* xyForm = new QFormLayout();
    xyForm->addRow("X value:", xEntry);
    xyForm->addRow("Y value:", yEntry);
    xyWidget->setLayout(xyForm);
    xyLegendLayout->addWidget(xyWidget, 0, Qt::AlignLeft);
    QLabel* legend = new QLabel("Right click the graph to see available actions or click-and-drag a node to adjust the envelope.\nThick segment = Flexible; Thin segment = Fixed.\nBlue = Linear; Green = Spline; Red = Exponential (curved).\nDragging near Y = 1 snaps to exactly 1. Type X and Y values for precise positions.", this);
    legend->setObjectName("envelopeLegend");
    legend->setWordWrap(true);
    // Keep QLabel's height-for-width policy so wrapped text gets enough room.
    xyLegendLayout->addWidget(legend, 1);
    layout->addLayout(xyLegendLayout);

    // Actions for context menu & save
    actionAdd = new QAction("Create New Envelope", this);
    connect(actionAdd, &QAction::triggered,
            this, &EnvelopeLibraryWindow::createNewEnvelope);

    actionDuplicate = new QAction("Duplicate Envelope", this);
    connect(actionDuplicate, &QAction::triggered,
            this, &EnvelopeLibraryWindow::duplicateEnvelope);

    actionSave = new QAction("Save", this);
    actionSave->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(actionSave, &QAction::triggered,
            this, &EnvelopeLibraryWindow::fileSave);
    addAction(actionSave);

    // Build popup menu
    popupMenu = new QMenu(this);
    popupMenu->addAction(actionAdd);
    popupMenu->addAction(actionDuplicate);
    popupMenu->addSeparator();
    auto* copyAction = popupMenu->addAction("Copy Envelope",
        this, &EnvelopeLibraryWindow::copyEnvelope);
    auto* pasteAction = popupMenu->addAction("Paste Envelope",
        this, &EnvelopeLibraryWindow::pasteEnvelope);
    connect(popupMenu, &QMenu::aboutToShow, this, [this, copyAction, pasteAction] {
        copyAction->setEnabled(activeProject
            && envelopeLibrary->selectionModel()->hasSelection());
        pasteAction->setEnabled(activeProject
            && ProjectClipboard::get<EnvelopeClipboard>(
                Inst::get_project_manager()->get_curr_project()));
    });

    auto* copyShortcut = new QShortcut(envelopeLibrary);
    copyShortcut->setKeys(QKeySequence::keyBindings(QKeySequence::Copy));
    copyShortcut->setContext(Qt::WidgetShortcut);
    connect(copyShortcut, &QShortcut::activated,
            this, &EnvelopeLibraryWindow::copyEnvelope);

    auto* pasteShortcut = new QShortcut(envelopeLibrary);
    pasteShortcut->setKeys(QKeySequence::keyBindings(QKeySequence::Paste));
    pasteShortcut->setContext(Qt::WidgetShortcut);
    connect(pasteShortcut, &QShortcut::activated,
            this, &EnvelopeLibraryWindow::pasteEnvelope);

    // Connect tree signals
    connect(envelopeLibrary, &QTreeView::activated,
            this, &EnvelopeLibraryWindow::objectActivated);
    // selectionChanged (not currentChanged) — fires *after* Qt applies the
     // selection change, and only when the selection actually changes, so
     // currentChanged also fires when a focused tree view auto-assigns a
     // current index on refocus (which would auto-load the first envelope).
    connect(envelopeLibrary->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this]{ onCursorChanged(); });

    envelopeLibrary->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(envelopeLibrary, &QWidget::customContextMenuRequested,
            this, &EnvelopeLibraryWindow::onRightClick);

    setCentralWidget(central);
    drawingArea->setMinimumSize(200, 200); // Ensure it has a visible size
    drawingArea->show(); // Explicitly show the widget
    drawingArea->raise(); // Bring it to the front
    drawingArea->repaint(); // Force repaint after window is shown
}

/**
 * @brief Destructor
 */
EnvelopeLibraryWindow::~EnvelopeLibraryWindow() = default;

/**
 * @brief Set the active project and populate the list
 * @param project  project to set as active
 */
void EnvelopeLibraryWindow::setActiveProject(ProjectView* project)
{
    drawingArea->clearGraph();
    activeProject = project;
    refreshEnvelopeList();

    ProjectManager *pm = Inst::get_project_manager();
    EnvelopeLibraryEntry* entry = pm->envlibentries();
    while (entry) {
        // append each entry to the tree
        QStandardItem* item = new QStandardItem(entry->getNumberString());
        // store the pointer in UserRole
        item->setData(QVariant::fromValue<void*>(entry), Qt::UserRole);
        refModel->appendRow(item);
        entry = entry->next;
    }
    activeEnvelope = nullptr;
}

/**
 * @brief Create a new envelope via the project controller
 */
void EnvelopeLibraryWindow::createNewEnvelope()
{
    if (!activeProject) return;

    ProjectManager *pm = Inst::get_project_manager();
    EnvelopeLibraryEntry *newEnv = EnvelopeUtilities::createNewEnvelopeHelper(pm->envlibentries());
    if (pm->envlibentries() == nullptr) {
        pm->envlibentries() = newEnv;
    }

    MUtilities::modified();

    QStandardItem* item = new QStandardItem(newEnv->getNumberString());
    item->setData(QVariant::fromValue<void*>(newEnv), Qt::UserRole);
    refModel->appendRow(item);

    // Automatically select the new envelope
    QModelIndex newIndex = refModel->index(refModel->rowCount() - 1, 0);
    envelopeLibrary->setCurrentIndex(newIndex);
    // This will trigger onCursorChanged and update the graph
}

/**
 * @brief Duplicate the selected envelope
 */
void EnvelopeLibraryWindow::duplicateEnvelope()
{
    QModelIndex idx = envelopeLibrary->currentIndex();
    if (!idx.isValid()) return;

    QStandardItem* origItem = refModel->itemFromIndex(idx);
    auto ptr = static_cast<EnvelopeLibraryEntry*>(origItem->data(Qt::UserRole).value<void*>());
    if (!ptr || !activeProject) return;

    EnvelopeLibraryEntry* dupEnv = EnvelopeUtilities::duplicateEnvelopeHelper(Inst::get_project_manager()->envlibentries(), ptr);
    MUtilities::modified();

    QStandardItem* newItem = new QStandardItem(dupEnv->getNumberString());
    newItem->setData(QVariant::fromValue<void*>(dupEnv), Qt::UserRole);
    refModel->appendRow(newItem);
}

void EnvelopeLibraryWindow::copyEnvelope()
{
    if (!activeProject) return;
    const auto rows = envelopeLibrary->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    auto* item = refModel->itemFromIndex(rows.first());
    auto* entry = static_cast<EnvelopeLibraryEntry*>(
        item->data(Qt::UserRole).value<void*>());
    if (!entry) return;

    EnvelopeClipboard copied;
    for (auto* node = entry->head; node;) {
        EnvelopeClipboardNode value{node->x, node->y};
        if (node->rightSeg) {
            value.type = node->rightSeg->segmentType;
            value.property = node->rightSeg->segmentProperty;
        }
        copied.nodes.append(value);
        node = node->rightSeg ? node->rightSeg->rightNode : nullptr;
    }
    ProjectClipboard::copy(Inst::get_project_manager()->get_curr_project(),
        std::move(copied), tr("Envelope %1").arg(entry->number));
}

void EnvelopeLibraryWindow::pasteEnvelope()
{
    if (!activeProject) return;
    auto* pm = Inst::get_project_manager();
    const auto* copied =
        ProjectClipboard::get<EnvelopeClipboard>(pm->get_curr_project());
    if (!copied || copied->nodes.size() < 2) return;

    auto* last = pm->envlibentries();
    while (last && last->next) last = last->next;
    auto* entry = new EnvelopeLibraryEntry(last ? last->number + 1 : 1);
    auto* node = entry->head;
    for (qsizetype index = 0; index < copied->nodes.size(); ++index) {
        const auto& value = copied->nodes[index];
        node->x = value.x;
        node->y = value.y;
        if (index + 1 == copied->nodes.size()) break;

        // Reuse the default envelope's two endpoints and add any others.
        if (!node->rightSeg) {
            node->rightSeg = new EnvLibEntrySeg;
            node->rightSeg->leftNode = node;
            node->rightSeg->rightNode = new EnvLibEntryNode(0.0, 0.0);
            node->rightSeg->rightNode->leftSeg = node->rightSeg;
        }
        node->rightSeg->segmentType = value.type;
        node->rightSeg->segmentProperty = value.property;
        node = node->rightSeg->rightNode;
    }

    entry->prev = last;
    if (last) last->next = entry;
    else pm->envlibentries() = entry;

    auto* item = new QStandardItem(entry->getNumberString());
    item->setData(QVariant::fromValue<void*>(entry), Qt::UserRole);
    refModel->appendRow(item);
    envelopeLibrary->setCurrentIndex(refModel->index(refModel->rowCount() - 1, 0));
    MUtilities::modified();
}

/**
 * @brief Delete the selected envelope
 */
void EnvelopeLibraryWindow::deleteEnvelope()
{
    QModelIndex idx = envelopeLibrary->currentIndex();
    if (!idx.isValid()) return;

    QStandardItem* item = refModel->itemFromIndex(idx);
    auto ptr = static_cast<EnvelopeLibraryEntry*>(item->data(Qt::UserRole).value<void*>());
    if (!ptr || !activeProject) return;

    bool wasCurrent = (ptr == activeEnvelope);

    EnvelopeUtilities::deleteEnvelope(Inst::get_project_manager()->envlibentries(), ptr);
    MUtilities::modified();

    refModel->removeRow(idx.row());

    if (wasCurrent) {
        activeEnvelope = nullptr;
        drawingArea->resetFields();
        drawingArea->clearGraph();
    }
}

/**
 * @brief Return the active envelope pointer
 * @return pointer to the currently active envelope
 */
EnvelopeLibraryEntry* EnvelopeLibraryWindow::getActiveEnvelope() const
{
    return activeEnvelope;
}

/**
 * @brief Handle double‐click/Enter on a tree item
 * @param index  index of the activated item
 */
void EnvelopeLibraryWindow::objectActivated(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QStandardItem* item = refModel->itemFromIndex(index);
    activeEnvelope = static_cast<EnvelopeLibraryEntry*>(item->data(Qt::UserRole).value<void*>());

    drawingArea->resetFields();
    drawingArea->adjustBoundary(activeEnvelope);
    drawingArea->showGraph(activeEnvelope);
    drawingArea->update();
}

/**
 * @brief Handle selection changes
 * @param current   current selection index
 * @param previous  previous selection index
 */
void EnvelopeLibraryWindow::onCursorChanged()
{
    const QModelIndexList rows =
        envelopeLibrary->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    const QModelIndex current = rows.first();
    if (!current.isValid()) return;
    QStandardItem* item = refModel->itemFromIndex(current);
    activeEnvelope = static_cast<EnvelopeLibraryEntry*>(item->data(Qt::UserRole).value<void*>());

    drawingArea->resetFields();
    drawingArea->adjustBoundary(activeEnvelope);
    drawingArea->showGraph(activeEnvelope);
    drawingArea->update();
}

/**
 * @brief Show right‐click context menu
 * @param pos  position where right-click occurred
 */
void EnvelopeLibraryWindow::onRightClick(const QPoint& pos)
{
    QModelIndex idx = envelopeLibrary->indexAt(pos);
    if (idx.isValid()) envelopeLibrary->setCurrentIndex(idx);
    popupMenu->exec(envelopeLibrary->viewport()->mapToGlobal(pos));
}

/**
 * @brief Clear and rebuild the model
 */
void EnvelopeLibraryWindow::refreshEnvelopeList()
{
    refModel->removeRows(0, refModel->rowCount());
}

/**
 * @brief Save via project controller
 */
void EnvelopeLibraryWindow::fileSave()
{
    if (activeProject) activeProject->save();
}

/**
 * @brief Capture Ctrl+S
 * @param event  key event to handle
 */
void EnvelopeLibraryWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_S) {
        fileSave();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

/**
 * @brief Called whenever X or Y entry changes
 */
void EnvelopeLibraryWindow::valueEntriesChanged()
{
    if (!drawingArea) return;
    
    // Only update if we have valid numeric input
    bool xOk, yOk;
    xEntry->text().toDouble(&xOk);
    yEntry->text().toDouble(&yOk);
    
    if (xOk && yOk) {
        drawingArea->setActiveNodeCoordinate(
            xEntry->text(),
            yEntry->text());
    }
}

/**
 * @brief Update the line edits from code
 * @param x  x-coordinate value to set
 * @param y  y-coordinate value to set
 */
void EnvelopeLibraryWindow::setEntries(const QString& x, const QString& y)
{
    // Temporarily disconnect signals to prevent infinite loops
    disconnect(xEntry, &QLineEdit::textChanged, this, &EnvelopeLibraryWindow::valueEntriesChanged);
    disconnect(yEntry, &QLineEdit::textChanged, this, &EnvelopeLibraryWindow::valueEntriesChanged);
    
    xEntry->setText(x);
    yEntry->setText(y);
    
    // Reconnect signals
    connect(xEntry, &QLineEdit::textChanged, this, &EnvelopeLibraryWindow::valueEntriesChanged);
    connect(yEntry, &QLineEdit::textChanged, this, &EnvelopeLibraryWindow::valueEntriesChanged);
}

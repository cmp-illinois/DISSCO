#include "MarkovModelLibraryWindow.hpp"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDoubleValidator>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QModelIndexList>
#include <QShortcut>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QUndoCommand>
#include <QUndoStack>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <limits>

#include "../utilities.hpp"
#include "../widgets/ProjectViewController.hpp"

namespace {

// Item delegate restricting input to nonnegative real numbers (ints/doubles).
class NonNegativeRealDelegate : public QStyledItemDelegate {
public:
    using OnEditStart = std::function<void()>;

    explicit NonNegativeRealDelegate(OnEditStart onEditStart,
                                     QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_onEditStart(std::move(onEditStart)) {}

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& /*option*/,
                          const QModelIndex& /*index*/) const override {
        if (m_onEditStart) m_onEditStart();
        auto* editor = new QLineEdit(parent);
        auto* validator = new QDoubleValidator(
            0.0, std::numeric_limits<double>::max(), 12, editor);
        validator->setNotation(QDoubleValidator::StandardNotation);
        // Use C locale so '.' is always the decimal separator.
        QLocale c(QLocale::C);
        c.setNumberOptions(QLocale::RejectGroupSeparator);
        validator->setLocale(c);
        editor->setValidator(validator);

        // Commit on every keystroke (not just focus-out / Enter) so that a
        // partially-typed value like ".1" is pushed into the model — and
        // therefore into the underlying MarkovModel — even if the user
        // triggers a project save without leaving the cell.
        connect(editor, &QLineEdit::textEdited, this,
                [this, editor](const QString&) {
                    const_cast<NonNegativeRealDelegate*>(this)
                        ->emitCommitData(editor);
                });
        return editor;
    }

    void emitCommitData(QWidget* editor) { emit commitData(editor); }

    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        auto* le = qobject_cast<QLineEdit*>(editor);
        if (!le) return;
        const QString text = le->text().trimmed();
        if (text.isEmpty()) {
            model->setData(index, QStringLiteral("0"), Qt::EditRole);
            return;
        }
        bool ok = false;
        const double v = QLocale::c().toDouble(text, &ok);
        if (!ok || v < 0.0) {
            // Reject: leave existing model value unchanged.
            return;
        }
        model->setData(index, text, Qt::EditRole);
    }

private:
    OnEditStart m_onEditStart;
};

// Undo command that captures two editor snapshots and swaps them.
// applySnapshot is idempotent, so the initial redo() after push is harmless.
class SnapshotCommand : public QUndoCommand {
public:
    SnapshotCommand(MarkovModelLibraryWindow* win,
                    int modelIdx,
                    MarkovModelLibraryWindow::EditorSnapshot before,
                    MarkovModelLibraryWindow::EditorSnapshot after,
                    const QString& text)
        : QUndoCommand(text),
          m_win(win),
          m_modelIdx(modelIdx),
          m_before(std::move(before)),
          m_after(std::move(after)) {}

    void undo() override {
        m_win->applySnapshotToWithDiff(m_modelIdx, m_after, m_before);
    }
    void redo() override {
        m_win->applySnapshotToWithDiff(m_modelIdx, m_before, m_after);
    }

private:
    MarkovModelLibraryWindow* m_win;
    int m_modelIdx;
    MarkovModelLibraryWindow::EditorSnapshot m_before;
    MarkovModelLibraryWindow::EditorSnapshot m_after;
};

// Create: first redo() appends a default model; subsequent redoes restore
// whatever serialized content the model had when the user last undid it.
class CreateModelCommand : public QUndoCommand {
public:
    // If initialSerialized is empty, the first redo() inserts a default model
    // (plain "Create New"); otherwise the first redo() inserts that content
    // (used for "Duplicate"). Subsequent redoes reuse whatever serialized
    // state the model had when the user last undid the creation.
    CreateModelCommand(MarkovModelLibraryWindow* win, int idx,
                       QString label,
                       QString initialSerialized = {})
        : QUndoCommand(std::move(label)),
          m_win(win),
          m_idx(idx),
          m_serialized(std::move(initialSerialized)),
          m_hasSerialized(!m_serialized.isEmpty()) {}

    void redo() override {
        if (m_hasSerialized) m_win->insertModelAt(m_idx, m_serialized);
        else                 m_win->insertDefaultModelAt(m_idx);
    }
    void undo() override {
        m_serialized = m_win->serializeModelAt(m_idx);
        m_hasSerialized = true;
        m_win->removeModelAt(m_idx);
    }

private:
    MarkovModelLibraryWindow* m_win;
    int m_idx;
    QString m_serialized;
    bool m_hasSerialized = false;
};

// Delete: push() runs redo() which performs the removal, so the caller must
// have captured the serialized content *before* pushing.
class DeleteModelCommand : public QUndoCommand {
public:
    DeleteModelCommand(MarkovModelLibraryWindow* win, int idx, QString serialized)
        : QUndoCommand(
              QCoreApplication::translate("MarkovModelLibraryWindow",
                                          "Delete Markov model %1").arg(idx)),
          m_win(win),
          m_idx(idx),
          m_serialized(std::move(serialized)) {}

    void redo() override { m_win->removeModelAt(m_idx); }
    void undo() override { m_win->insertModelAt(m_idx, m_serialized); }

private:
    MarkovModelLibraryWindow* m_win;
    int m_idx;
    QString m_serialized;
};

QStringList numberedHeaders(int n) {
    QStringList out;
    out.reserve(n);
    for (int i = 0; i < n; ++i) out << QString::number(i + 1);
    return out;
}

int rowHeightFor(const QTableView* v) {
    return v->verticalHeader()->defaultSectionSize()
         + v->horizontalHeader()->height()
         + 2 * v->frameWidth();
}

}  // namespace

MarkovModelLibraryWindow::MarkovModelLibraryWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Markov Model Library"));
    resize(900, 650);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    // --- Left panel: model list ---------------------------------------------
    m_listModel = new QStandardItemModel(this);
    m_listModel->setHorizontalHeaderLabels({tr("id")});

    m_treeView = new QTreeView;
    m_treeView->setModel(m_listModel);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->header()->setStretchLastSection(true);

    // Left panel: tree view + add/remove bar.
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    leftLayout->addWidget(m_treeView, /*stretch=*/1);

    auto* buttonBar = new QWidget;
    auto* buttonBarLayout = new QHBoxLayout(buttonBar);
    buttonBarLayout->setContentsMargins(2, 2, 2, 2);
    buttonBarLayout->setSpacing(2);

    auto* addButton = new QToolButton;
    addButton->setText(QStringLiteral("+"));
    addButton->setToolTip(tr("Create new Markov model"));
    addButton->setAutoRaise(true);

    auto* removeButton = new QToolButton;
    removeButton->setText(QStringLiteral("\u2212"));  // U+2212 MINUS SIGN
    removeButton->setToolTip(tr("Delete selected Markov model"));
    removeButton->setAutoRaise(true);

    buttonBarLayout->addWidget(addButton);
    buttonBarLayout->addWidget(removeButton);
    buttonBarLayout->addStretch(1);
    leftLayout->addWidget(buttonBar);

    connect(addButton, &QToolButton::clicked,
            this, &MarkovModelLibraryWindow::createNewModel);
    connect(removeButton, &QToolButton::clicked,
            this, &MarkovModelLibraryWindow::removeModel);

    // Keep the remove button disabled when nothing is selected. Piggyback on
    // the existing enablement path used by the context menu's Delete action.
    m_removeButton = removeButton;

    splitter->addWidget(leftPanel);

    // --- Right panel: stacked (empty placeholder + editor) -----------------
    m_rightStack = new QStackedWidget;

    // Empty page: a single centered message shown when no model is selected.
    auto* emptyPage = new QWidget;
    auto* emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    auto* emptyLabel = new QLabel(
        tr("Select (or create) a Markov library from the list on the left"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    emptyLabel->setEnabled(false);  // muted look
    emptyLayout->addWidget(emptyLabel);
    m_emptyPageIndex = m_rightStack->addWidget(emptyPage);

    // Editor page
    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(8);

    // Size row
    auto* sizeRow = new QHBoxLayout;
    sizeRow->setSpacing(6);
    sizeRow->addWidget(new QLabel(tr("Markov chain size:")));
    m_sizeEntry = new QLineEdit;
    m_sizeEntry->setValidator(new QIntValidator(1, 10000, m_sizeEntry));
    m_sizeEntry->setMaximumWidth(80);
    sizeRow->addWidget(m_sizeEntry);
    m_sizeButton = new QPushButton(tr("Set"));
    sizeRow->addWidget(m_sizeButton);
    sizeRow->addStretch(1);
    rightLayout->addLayout(sizeRow);

    m_undoStack = new QUndoStack(this);

    auto* delegate = new NonNegativeRealDelegate(
        [this]{ beginEditCapture(); }, this);
    connect(delegate, &QAbstractItemDelegate::closeEditor, this,
            &MarkovModelLibraryWindow::onEditorClosed);

    // Initial distribution
    m_distModel = new QStandardItemModel(this);
    m_distView = new QTableView;
    m_distView->setModel(m_distModel);
    m_distView->setItemDelegate(delegate);
    m_distView->verticalHeader()->setVisible(false);
    m_distView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_distView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_distView->setFixedHeight(rowHeightFor(m_distView) + m_distView->verticalHeader()->defaultSectionSize());

    auto* distGroup = new QGroupBox(tr("Initial Distribution"));
    auto* distGroupLayout = new QVBoxLayout(distGroup);
    distGroupLayout->setContentsMargins(6, 6, 6, 6);
    distGroupLayout->addWidget(m_distView);
    rightLayout->addWidget(distGroup);

    // Values for states
    m_valueModel = new QStandardItemModel(this);
    m_valueView = new QTableView;
    m_valueView->setModel(m_valueModel);
    m_valueView->setItemDelegate(delegate);
    m_valueView->verticalHeader()->setVisible(false);
    m_valueView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_valueView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_valueView->setFixedHeight(rowHeightFor(m_valueView) + m_valueView->verticalHeader()->defaultSectionSize());

    auto* valueGroup = new QGroupBox(tr("Values for states"));
    auto* valueGroupLayout = new QVBoxLayout(valueGroup);
    valueGroupLayout->setContentsMargins(6, 6, 6, 6);
    valueGroupLayout->addWidget(m_valueView);
    rightLayout->addWidget(valueGroup);

    // Transition matrix
    m_matrixModel = new QStandardItemModel(this);
    m_matrixView = new QTableView;
    m_matrixView->setModel(m_matrixModel);
    m_matrixView->setItemDelegate(delegate);
    m_matrixView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_matrixView->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_matrixView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    auto* matrixGroup = new QGroupBox(tr("Transition Matrix"));
    auto* matrixGroupLayout = new QVBoxLayout(matrixGroup);
    matrixGroupLayout->setContentsMargins(6, 6, 6, 6);
    matrixGroupLayout->addWidget(m_matrixView);
    rightLayout->addWidget(matrixGroup, /*stretch=*/1);

    m_editorPageIndex = m_rightStack->addWidget(rightPanel);
    m_rightStack->setCurrentIndex(m_emptyPageIndex);

    splitter->addWidget(m_rightStack);
    splitter->setCollapsible(1, false); // prevent collapsing the markov window view
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({160, 740});

    // --- Context menu -------------------------------------------------------
    m_contextMenu = new QMenu(this);
    m_createAction    = m_contextMenu->addAction(tr("Create New Model"),
                                                 this, &MarkovModelLibraryWindow::createNewModel);
    m_duplicateAction = m_contextMenu->addAction(tr("Duplicate Model"),
                                                 this, &MarkovModelLibraryWindow::duplicateModel);
    m_deleteAction    = m_contextMenu->addAction(tr("Delete Model"),
                                                 this, &MarkovModelLibraryWindow::removeModel);
    updateContextMenuEnablement();

    // --- Signal wiring ------------------------------------------------------
    // selectionChanged (not currentChanged) — fires *after* Qt applies the
    // selection, so hasSelection()/selectedRows() reflect the real state.
    // currentChanged also fires when a focused tree view auto-assigns a
    // currentIndex with no actual selection, which we don't want to react to.
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]{ onSelectionChanged(); });
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MarkovModelLibraryWindow::onRightClick);
    connect(m_sizeButton, &QPushButton::clicked,
            this, &MarkovModelLibraryWindow::onSetSize);
    connect(m_sizeEntry, &QLineEdit::returnPressed,
            this, &MarkovModelLibraryWindow::onSetSize);
    connect(m_distModel, &QStandardItemModel::itemChanged,
            this, &MarkovModelLibraryWindow::onItemChanged);
    connect(m_valueModel, &QStandardItemModel::itemChanged,
            this, &MarkovModelLibraryWindow::onItemChanged);
    connect(m_matrixModel, &QStandardItemModel::itemChanged,
            this, &MarkovModelLibraryWindow::onItemChanged);

    installCopyPasteShortcuts(m_distView);
    installCopyPasteShortcuts(m_valueView);
    installCopyPasteShortcuts(m_matrixView);

    // Window-scoped undo/redo: works from anywhere in the window (tree view,
    // size entry, plus/minus buttons, etc.) — not just when a table view has
    // focus. While a cell editor (QLineEdit) is focused, attemptUndo/Redo
    // defers to the QLineEdit's native character-level undo so in-progress
    // typing can be undone character-by-character.
    auto* winUndo = new QShortcut(QKeySequence::Undo, this);
    winUndo->setContext(Qt::WindowShortcut);
    connect(winUndo, &QShortcut::activated,
            this, &MarkovModelLibraryWindow::attemptUndo);

    auto* winRedo = new QShortcut(QKeySequence::Redo, this);
    winRedo->setContext(Qt::WindowShortcut);
    connect(winRedo, &QShortcut::activated,
            this, &MarkovModelLibraryWindow::attemptRedo);

    auto* winClose = new QShortcut(QKeySequence::Close, this);
    winClose->setContext(Qt::WindowShortcut);
    connect(winClose, &QShortcut::activated, this, &MarkovModelLibraryWindow::close);
}

MarkovModelLibraryWindow::~MarkovModelLibraryWindow() = default;

void MarkovModelLibraryWindow::setActiveProject(ProjectView* project) {
    activeProject = project;
    currentSelection = -1;
    currentSize = 0;
    m_sizeEntry->clear();
    resizeTables(0);
    m_undoStack->clear();
    m_rightStack->setCurrentIndex(m_emptyPageIndex);
    rebuildModelList();
    updateContextMenuEnablement();
}

void MarkovModelLibraryWindow::hideEvent(QHideEvent* event) {
    if (currentSelection >= 0) {
        saveEditorIntoModel(currentSelection);
    }
    QMainWindow::hideEvent(event);
}

void MarkovModelLibraryWindow::closeEvent(QCloseEvent* event) {
    if (currentSelection >= 0) {
        saveEditorIntoModel(currentSelection);
    }
    QMainWindow::closeEvent(event);
}

void MarkovModelLibraryWindow::rebuildModelList() {
    m_listModel->removeRows(0, m_listModel->rowCount());
    if (!activeProject) return;
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;
    const auto& models = pm->markovmodels();
    for (int i = 0; i < models.size(); ++i) {
        auto* item = new QStandardItem(QString::number(i));
        item->setEditable(false);
        m_listModel->appendRow(item);
    }
}

void MarkovModelLibraryWindow::resizeTables(int size) {
    suppressItemChanged = true;

    const QStringList headers = numberedHeaders(size);

    m_distModel->clear();
    m_distModel->setRowCount(size > 0 ? 1 : 0);
    m_distModel->setColumnCount(size);
    m_distModel->setHorizontalHeaderLabels(headers);

    m_valueModel->clear();
    m_valueModel->setRowCount(size > 0 ? 1 : 0);
    m_valueModel->setColumnCount(size);
    m_valueModel->setHorizontalHeaderLabels(headers);

    m_matrixModel->clear();
    m_matrixModel->setRowCount(size);
    m_matrixModel->setColumnCount(size);
    m_matrixModel->setHorizontalHeaderLabels(headers);
    m_matrixModel->setVerticalHeaderLabels(headers);

    currentSize = size;
    suppressItemChanged = false;
}

void MarkovModelLibraryWindow::loadModelIntoEditor(int modelIdx) {
    if (!activeProject) return;
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;
    auto& models = pm->markovmodels();
    if (modelIdx < 0 || modelIdx >= models.size()) return;

    auto& model = models[modelIdx];
    const int size = model.getStateSize();

    resizeTables(size);
    m_sizeEntry->setText(QString::number(size));

    suppressItemChanged = true;
    for (int j = 0; j < size; ++j) {
        m_distModel->setItem(0, j,
            new QStandardItem(QString::number(model.getInitialProbability(j))));
        m_valueModel->setItem(0, j,
            new QStandardItem(QString::number(model.getStateValue(j))));
    }
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            m_matrixModel->setItem(i, j,
                new QStandardItem(QString::number(model.getTransitionProbability(i, j))));
        }
    }
    suppressItemChanged = false;
}

QString MarkovModelLibraryWindow::serializeEditor() const {

    auto cell = [](QStandardItem* it) -> QString {
        if (!it || it->text().isEmpty()) return QStringLiteral("0");
        return it->text();
    };

    QString s = QString::number(currentSize) + "\n";
    for (int j = 0; j < currentSize; ++j) {
        s += cell(m_valueModel->item(0, j)) + " ";
    }
    s += "\n";
    for (int j = 0; j < currentSize; ++j) {
        s += cell(m_distModel->item(0, j)) + " ";
    }
    s += "\n";
    for (int i = 0; i < currentSize; ++i) {
        for (int j = 0; j < currentSize; ++j) {
            s += cell(m_matrixModel->item(i, j)) + " ";
        }
    }
    return s;
}

void MarkovModelLibraryWindow::saveEditorIntoModel(int modelIdx) {
    if (!activeProject) return;
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;
    auto& models = pm->markovmodels();
    if (modelIdx < 0 || modelIdx >= models.size()) return;
    if (currentSize <= 0) return;

    models[modelIdx].from_str(serializeEditor().toStdString());
    MUtilities::modified();
}

void MarkovModelLibraryWindow::onSelectionChanged() {
    // Save whatever model was previously loaded into the editor.
    const int previousRow = currentSelection;
    if (previousRow >= 0) {
        saveEditorIntoModel(previousRow);
    }

    // NOTE: we intentionally do NOT clear the undo stack here. Editor-level
    // SnapshotCommands remember which model they apply to (and switch the
    // tree-view selection on undo/redo), and list-level Create/Delete
    // commands need to persist across selection changes.

    const QModelIndexList selected =
        m_treeView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        currentSelection = selected.first().row();
        loadModelIntoEditor(currentSelection);
        m_rightStack->setCurrentIndex(m_editorPageIndex);
    } else {
        currentSelection = -1;
        resizeTables(0);
        m_sizeEntry->clear();
        m_rightStack->setCurrentIndex(m_emptyPageIndex);
    }
    updateContextMenuEnablement();
}

void MarkovModelLibraryWindow::onRightClick(const QPoint& pos) {
    updateContextMenuEnablement();
    m_contextMenu->exec(m_treeView->viewport()->mapToGlobal(pos));
}

void MarkovModelLibraryWindow::onSetSize() {
    bool ok = false;
    const int newSize = m_sizeEntry->text().toInt(&ok);
    if (!ok || newSize <= 0) return;
    if (newSize == currentSize) return;

    const EditorSnapshot before = snapshotEditor();

    const int oldSize = currentSize;
    QVector<QString> oldVals(oldSize);
    QVector<QString> oldDists(oldSize);
    QVector<QVector<QString>> oldMat(oldSize, QVector<QString>(oldSize));
    for (int j = 0; j < oldSize; ++j) {
        if (auto* it = m_valueModel->item(0, j)) oldVals[j] = it->text();
        if (auto* it = m_distModel->item(0, j)) oldDists[j] = it->text();
    }
    for (int i = 0; i < oldSize; ++i) {
        for (int j = 0; j < oldSize; ++j) {
            if (auto* it = m_matrixModel->item(i, j)) oldMat[i][j] = it->text();
        }
    }

    resizeTables(newSize);

    suppressItemChanged = true;
    const int overlap = std::min(oldSize, newSize);
    for (int j = 0; j < overlap; ++j) {
        m_valueModel->setItem(0, j, new QStandardItem(oldVals[j]));
        m_distModel->setItem(0, j, new QStandardItem(oldDists[j]));
    }
    for (int i = 0; i < overlap; ++i) {
        for (int j = 0; j < overlap; ++j) {
            m_matrixModel->setItem(i, j, new QStandardItem(oldMat[i][j]));
        }
    }
    suppressItemChanged = false;

    if (currentSelection >= 0) {
        saveEditorIntoModel(currentSelection);
    }

    pushSnapshotCommand(before, snapshotEditor(), tr("Change size"));
}

void MarkovModelLibraryWindow::onItemChanged(QStandardItem* /*item*/) {
    if (suppressItemChanged) return;
    if (currentSelection >= 0) {
        saveEditorIntoModel(currentSelection);
    }
}

void MarkovModelLibraryWindow::createNewModel() {
    if (!activeProject) return;
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;

    if (currentSelection >= 0) {
        saveEditorIntoModel(currentSelection);
    }

    // Push as an undoable command; redo() performs the actual append.
    const int newIdx = pm->markovmodels().size();
    m_undoStack->push(new CreateModelCommand(
        this, newIdx, tr("Create Markov model %1").arg(newIdx)));
}

void MarkovModelLibraryWindow::duplicateModel() {
    if (!activeProject || currentSelection < 0) return;
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;

    saveEditorIntoModel(currentSelection);

    // Duplicate = create a new model at the end, seeded with the current
    // model's serialized contents. CreateModelCommand handles this directly.
    const int newIdx = pm->markovmodels().size();
    const QString serialized = serializeModelAt(currentSelection);
    m_undoStack->push(new CreateModelCommand(
        this, newIdx,
        tr("Duplicate Markov model %1").arg(currentSelection),
        serialized));
}

void MarkovModelLibraryWindow::removeModel() {
    if (!activeProject || currentSelection < 0) return;
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;

    // Flush editor state into the MarkovModel so the snapshot captures the
    // most recent edits.
    saveEditorIntoModel(currentSelection);

    const int idx = currentSelection;
    if (idx >= pm->markovmodels().size()) return;

    const QString serialized = serializeModelAt(idx);
    m_undoStack->push(new DeleteModelCommand(this, idx, serialized));
}

void MarkovModelLibraryWindow::updateContextMenuEnablement() {
    const bool hasSelection = currentSelection >= 0;
    if (m_duplicateAction) m_duplicateAction->setEnabled(hasSelection);
    if (m_deleteAction)    m_deleteAction->setEnabled(hasSelection);
    if (m_removeButton)    m_removeButton->setEnabled(hasSelection);
}

// ---------------------------------------------------------------------------
// Copy / paste
// ---------------------------------------------------------------------------

void MarkovModelLibraryWindow::installCopyPasteShortcuts(QTableView* view) {
    // Scoped to the view so that typing Ctrl+C/V/Z inside an open cell editor
    // goes to the QLineEdit's normal handling, not here. (I.e. while a cell
    // is being edited, Ctrl+Z does character-level undo within that QLineEdit;
    // once the editor is dismissed, Ctrl+Z undoes the whole edit.)
    auto* copy = new QShortcut(QKeySequence::Copy, view);
    copy->setContext(Qt::WidgetShortcut);
    connect(copy, &QShortcut::activated, this,
            [this, view]{ copySelection(view); });

    auto* paste = new QShortcut(QKeySequence::Paste, view);
    paste->setContext(Qt::WidgetShortcut);
    connect(paste, &QShortcut::activated, this,
            [this, view]{ pasteSelection(view); });

    auto* undo = new QShortcut(QKeySequence::Undo, view);
    undo->setContext(Qt::WidgetShortcut);
    connect(undo, &QShortcut::activated,
            this, &MarkovModelLibraryWindow::attemptUndo);

    auto* redo = new QShortcut(QKeySequence::Redo, view);
    redo->setContext(Qt::WidgetShortcut);
    connect(redo, &QShortcut::activated,
            this, &MarkovModelLibraryWindow::attemptRedo);
}

void MarkovModelLibraryWindow::copySelection(QTableView* view) const {
    auto* model = qobject_cast<QStandardItemModel*>(view->model());
    if (!model) return;
    const QModelIndexList sel = view->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    // Bounding rect of the selection.
    int minRow = sel.first().row(), maxRow = minRow;
    int minCol = sel.first().column(), maxCol = minCol;
    for (const QModelIndex& idx : sel) {
        minRow = std::min(minRow, idx.row());
        maxRow = std::max(maxRow, idx.row());
        minCol = std::min(minCol, idx.column());
        maxCol = std::max(maxCol, idx.column());
    }

    // Fill a dense grid; unselected cells in the bounding rect become empty
    // so downstream paste sees a regular shape.
    const int rows = maxRow - minRow + 1;
    const int cols = maxCol - minCol + 1;
    QVector<QVector<QString>> grid(rows, QVector<QString>(cols));
    for (const QModelIndex& idx : sel) {
        if (auto* it = model->item(idx.row(), idx.column())) {
            grid[idx.row() - minRow][idx.column() - minCol] = it->text();
        }
    }

    QString out;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (j > 0) out += QLatin1Char('\t');
            out += grid[i][j];
        }
        if (i + 1 < rows) out += QLatin1Char('\n');
    }
    QApplication::clipboard()->setText(out);
}

void MarkovModelLibraryWindow::pasteSelection(QTableView* view) {
    auto* model = qobject_cast<QStandardItemModel*>(view->model());
    if (!model) return;
    const QModelIndex anchor = view->selectionModel()->currentIndex();
    if (!anchor.isValid()) return;

    const QString text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    // Strip a single trailing newline (common from Excel / our own copy), but
    // leave internal blank rows alone.
    QString body = text;
    if (body.endsWith(QLatin1Char('\n'))) body.chop(1);
    if (body.endsWith(QLatin1Char('\r'))) body.chop(1);

    // Row split accepts \r\n, \n, \r (tolerant of platform differences).
    const QStringList rows = body.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")));

    const int r0 = anchor.row();
    const int c0 = anchor.column();
    const int rowCount = model->rowCount();
    const int colCount = model->columnCount();

    const EditorSnapshot before = snapshotEditor();

    suppressItemChanged = true;
    bool anyChange = false;
    for (int i = 0; i < rows.size(); ++i) {
        const int targetRow = r0 + i;
        if (targetRow >= rowCount) break;  // overflow: drop remaining rows
        const QStringList cells = rows[i].split(QLatin1Char('\t'));
        for (int j = 0; j < cells.size(); ++j) {
            const int targetCol = c0 + j;
            if (targetCol >= colCount) break;  // overflow: drop remaining cols
            const QString raw = cells[j].trimmed();
            if (raw.isEmpty()) continue;  // leave empty cells alone
            bool ok = false;
            const double v = QLocale::c().toDouble(raw, &ok);
            if (!ok || v < 0.0) continue;  // same validation as the delegate
            auto* it = model->item(targetRow, targetCol);
            if (!it) {
                it = new QStandardItem(raw);
                model->setItem(targetRow, targetCol, it);
            } else {
                it->setText(raw);
            }
            anyChange = true;
        }
    }
    suppressItemChanged = false;

    // Single save for the whole paste, not one per cell.
    if (anyChange) {
        if (currentSelection >= 0) saveEditorIntoModel(currentSelection);
        pushSnapshotCommand(before, snapshotEditor(), tr("Paste"));
    }
}

// ---------------------------------------------------------------------------
// Undo / redo
// ---------------------------------------------------------------------------

MarkovModelLibraryWindow::EditorSnapshot
MarkovModelLibraryWindow::snapshotEditor() const {
    EditorSnapshot s;
    s.size = currentSize;
    s.dist.resize(currentSize);
    s.values.resize(currentSize);
    s.matrix.resize(currentSize * currentSize);
    for (int j = 0; j < currentSize; ++j) {
        if (auto* it = m_distModel->item(0, j))  s.dist[j] = it->text();
        if (auto* it = m_valueModel->item(0, j)) s.values[j] = it->text();
    }
    for (int i = 0; i < currentSize; ++i) {
        for (int j = 0; j < currentSize; ++j) {
            if (auto* it = m_matrixModel->item(i, j))
                s.matrix[i * currentSize + j] = it->text();
        }
    }
    return s;
}

void MarkovModelLibraryWindow::applySnapshot(const EditorSnapshot& s) {
    applyingSnapshot = true;
    suppressItemChanged = true;

    if (s.size != currentSize) {
        resizeTables(s.size);
        m_sizeEntry->setText(QString::number(s.size));
    }

    for (int j = 0; j < s.size; ++j) {
        m_distModel->setItem(0, j, new QStandardItem(s.dist.value(j)));
        m_valueModel->setItem(0, j, new QStandardItem(s.values.value(j)));
    }
    for (int i = 0; i < s.size; ++i) {
        for (int j = 0; j < s.size; ++j) {
            m_matrixModel->setItem(i, j,
                new QStandardItem(s.matrix.value(i * s.size + j)));
        }
    }

    suppressItemChanged = false;
    applyingSnapshot = false;

    // Propagate the restored state into the backing MarkovModel so a save
    // right after an undo persists the undone state.
    if (currentSelection >= 0 && currentSize > 0) {
        saveEditorIntoModel(currentSelection);
    }
}

void MarkovModelLibraryWindow::beginEditCapture() {
    if (m_editInProgress) return;  // nested / already captured
    m_editStartSnapshot = snapshotEditor();
    m_editInProgress = true;
}

void MarkovModelLibraryWindow::onEditorClosed() {
    if (!m_editInProgress) return;
    m_editInProgress = false;
    const EditorSnapshot after = snapshotEditor();
    pushSnapshotCommand(m_editStartSnapshot, after, tr("Edit cell"));
}

void MarkovModelLibraryWindow::pushSnapshotCommand(const EditorSnapshot& before,
                                                   const EditorSnapshot& after,
                                                   const QString& label) {
    // No-op if nothing actually changed.
    if (before.size == after.size &&
        before.dist == after.dist &&
        before.values == after.values &&
        before.matrix == after.matrix) {
        return;
    }
    if (currentSelection < 0) return;  // nothing to attach the command to
    // push() calls redo() once; state is already `after`, and applySnapshot
    // is idempotent, so the extra call is cheap and correct.
    m_undoStack->push(new SnapshotCommand(
        this, currentSelection, before, after, label));
}

void MarkovModelLibraryWindow::applySnapshotTo(int modelIdx,
                                               const EditorSnapshot& s) {
    // Ensure the target model is the one currently loaded in the editor,
    // so applySnapshot's save path writes into the right MarkovModel.
    if (modelIdx != currentSelection) {
        if (modelIdx < 0 || modelIdx >= m_listModel->rowCount()) return;
        selectTreeRow(modelIdx);
        // selectionChanged fired synchronously → currentSelection == modelIdx,
        // and the model was loaded into the editor.
    } else {
        // Already the active model, but still scroll it into view so the user
        // sees which row the undo/redo is affecting.
        if (modelIdx >= 0 && modelIdx < m_listModel->rowCount())
            m_treeView->scrollTo(m_listModel->index(modelIdx, 0));
    }
    applySnapshot(s);
}

void MarkovModelLibraryWindow::applySnapshotToWithDiff(
        int modelIdx,
        const EditorSnapshot& from,
        const EditorSnapshot& to) {
    applySnapshotTo(modelIdx, to);

    // Clear any prior selection before highlighting the changed cells.
    if (m_distView->selectionModel())
        m_distView->selectionModel()->clearSelection();
    if (m_valueView->selectionModel())
        m_valueView->selectionModel()->clearSelection();
    if (m_matrixView->selectionModel())
        m_matrixView->selectionModel()->clearSelection();

    // If the size itself changed, the notion of "which cells differ" is
    // ill-defined — skip highlighting rather than selecting everything.
    if (from.size != to.size) return;

    const int sz = to.size;
    if (sz <= 0) return;

    auto selectCell = [](QTableView* view, QStandardItemModel* model,
                         int row, int col) {
        if (!view || !model) return;
        auto* sm = view->selectionModel();
        if (!sm) return;
        sm->select(model->index(row, col),
                   QItemSelectionModel::Select);
    };

    QTableView* firstChangedView = nullptr;
    QModelIndex firstChangedIndex;

    // 1xN initial-distribution row.
    if (from.dist.size() == sz && to.dist.size() == sz) {
        for (int i = 0; i < sz; ++i) {
            if (from.dist[i] != to.dist[i]) {
                selectCell(m_distView, m_distModel, 0, i);
                if (!firstChangedView) {
                    firstChangedView = m_distView;
                    firstChangedIndex = m_distModel->index(0, i);
                }
            }
        }
    }

    // 1xN state-values row.
    if (from.values.size() == sz && to.values.size() == sz) {
        for (int i = 0; i < sz; ++i) {
            if (from.values[i] != to.values[i]) {
                selectCell(m_valueView, m_valueModel, 0, i);
                if (!firstChangedView) {
                    firstChangedView = m_valueView;
                    firstChangedIndex = m_valueModel->index(0, i);
                }
            }
        }
    }

    // NxN transition matrix.
    if (from.matrix.size() == sz * sz && to.matrix.size() == sz * sz) {
        for (int r = 0; r < sz; ++r) {
            for (int c = 0; c < sz; ++c) {
                const int k = r * sz + c;
                if (from.matrix[k] != to.matrix[k]) {
                    selectCell(m_matrixView, m_matrixModel, r, c);
                    if (!firstChangedView) {
                        firstChangedView = m_matrixView;
                        firstChangedIndex = m_matrixModel->index(r, c);
                    }
                }
            }
        }
    }

    // Make one of the changed cells the current index and scroll it into
    // view so the user sees the highlight.
    if (firstChangedView && firstChangedIndex.isValid()) {
        firstChangedView->selectionModel()->setCurrentIndex(
            firstChangedIndex, QItemSelectionModel::NoUpdate);
        firstChangedView->scrollTo(firstChangedIndex);
    }
}

void MarkovModelLibraryWindow::selectTreeRow(int row) {
    if (!m_listModel || !m_treeView) return;
    if (row < 0 || row >= m_listModel->rowCount()) return;
    const QModelIndex idx = m_listModel->index(row, 0);
    m_treeView->selectionModel()->setCurrentIndex(
        idx,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_treeView->scrollTo(idx);
}

// ---------------------------------------------------------------------------
// List-level helpers used by CreateModelCommand / DeleteModelCommand.
// ---------------------------------------------------------------------------

void MarkovModelLibraryWindow::insertDefaultModelAt(int idx) {
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;
    auto& v = pm->markovmodels();
    idx = std::clamp(idx, 0, static_cast<int>(v.size()));
    v.insert(v.begin() + idx, MarkovModel<float>());
    MUtilities::modified();

    rebuildModelList();
    // Select the newly-inserted row.
    selectTreeRow(idx);
}

void MarkovModelLibraryWindow::insertModelAt(int idx, const QString& serialized) {
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;
    auto& v = pm->markovmodels();
    idx = std::clamp(idx, 0, static_cast<int>(v.size()));
    MarkovModel<float> m;
    if (!serialized.isEmpty()) m.from_str(serialized.toStdString());
    v.insert(v.begin() + idx, std::move(m));
    MUtilities::modified();

    rebuildModelList();
    selectTreeRow(idx);
}

void MarkovModelLibraryWindow::removeModelAt(int idx) {
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return;
    auto& v = pm->markovmodels();
    if (idx < 0 || idx >= static_cast<int>(v.size())) return;

    // If we're deleting the currently-loaded model, clear the selection state
    // first so the cascading onSelectionChanged doesn't try to save editor
    // contents into a model that's about to disappear.
    if (currentSelection == idx) currentSelection = -1;
    else if (currentSelection > idx) currentSelection -= 1;

    v.erase(v.begin() + idx);
    MUtilities::modified();

    rebuildModelList();

    const int remaining = m_listModel->rowCount();
    if (remaining > 0) {
        const int nextRow = std::min(idx, remaining - 1);
        m_treeView->selectionModel()->setCurrentIndex(
            m_listModel->index(nextRow, 0),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    } else {
        // selectionChanged may not fire (selection already empty) — force
        // the empty-page transition explicitly.
        m_treeView->selectionModel()->clearSelection();
        currentSelection = -1;
        resizeTables(0);
        m_sizeEntry->clear();
        m_rightStack->setCurrentIndex(m_emptyPageIndex);
        updateContextMenuEnablement();
    }
}

QString MarkovModelLibraryWindow::serializeModelAt(int idx) const {
    ProjectManager* pm = Inst::get_project_manager();
    if (!pm || !pm->get_curr_project()) return {};
    auto& v = pm->markovmodels();
    if (idx < 0 || idx >= static_cast<int>(v.size())) return {};

    // MarkovModel's getters aren't const-qualified, so take a non-const ref.
    auto& m = v[idx];
    const int sz = m.getStateSize();
    QString s = QString::number(sz) + "\n";
    for (int j = 0; j < sz; ++j)
        s += QString::number(m.getStateValue(j)) + " ";
    s += "\n";
    for (int j = 0; j < sz; ++j)
        s += QString::number(m.getInitialProbability(j)) + " ";
    s += "\n";
    for (int i = 0; i < sz; ++i)
        for (int j = 0; j < sz; ++j)
            s += QString::number(m.getTransitionProbability(i, j)) + " ";
    return s;
}

// ---------------------------------------------------------------------------
// Prompt-wrapped undo/redo.
// ---------------------------------------------------------------------------


void MarkovModelLibraryWindow::attemptUndo() {
    // If a QLineEdit (e.g. an open cell editor, or the size-entry field) has
    // focus and can undo its own typing, let it handle Ctrl+Z character-by-
    // character. Only when it's got nothing left to undo do we fall through
    // to the editor-level undo stack.
    if (auto* le = qobject_cast<QLineEdit*>(QApplication::focusWidget())) {
        if (le->isUndoAvailable()) { le->undo(); return; }
    }
    if (!m_undoStack->canUndo()) return;
    const QString label = m_undoStack->undoText();
    // Destructive: undoing a creation removes the model.
    if (label.startsWith(QLatin1String("Create")) ||
        label.startsWith(QLatin1String("Duplicate"))) {
        const auto answer = QMessageBox::question(
            this, tr("Undo"),
            tr("Undo '%1'?").arg(label),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }
    m_undoStack->undo();
}

void MarkovModelLibraryWindow::attemptRedo() {
    // Symmetric guard: let a focused QLineEdit redo its own character-level
    // typing first.
    if (auto* le = qobject_cast<QLineEdit*>(QApplication::focusWidget())) {
        if (le->isRedoAvailable()) { le->redo(); return; }
    }
    if (!m_undoStack->canRedo()) return;
    const QString label = m_undoStack->redoText();
    // Destructive: redoing a deletion removes the model again.
    if (label.startsWith(QLatin1String("Delete"))) {
        const auto answer = QMessageBox::question(
            this, tr("Redo"),
            tr("Redo '%1'?").arg(label),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }
    m_undoStack->redo();
}

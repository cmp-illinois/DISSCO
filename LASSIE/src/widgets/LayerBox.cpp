#include "LayerBox.hpp"
#include "../inst.hpp"
#include <algorithm>

LayerBox::LayerBox(Eventtype eventType, int eventIndex, int layerIndex, QWidget* parent)
    : QFrame(parent),
      m_eventType(eventType),
      m_eventIndex(eventIndex),
      m_layerIndex(layerIndex)
{
    // Main Layout
    m_mainLayout = new QVBoxLayout();
    this->setLayout(m_mainLayout);
    this->setFrameShape(QFrame::StyledPanel);
    this->setLineWidth(1);

    // Header
    auto* weightHBox = new QHBoxLayout;
    auto* weightLabel = new QLabel("Number of children in this layer:");
    m_weightEntry = new QLineEdit;
    m_weightFuncButton = new QPushButton("Insert Function");
    m_deleteLayerButton = new QPushButton("Delete This Layer");

    connect(m_weightFuncButton, &QPushButton::clicked,
            this, &LayerBox::onWeightFunctionClicked);
    connect(m_deleteLayerButton, &QPushButton::clicked,
            this, &LayerBox::onDeleteLayerClicked);
    connect(m_weightEntry, &QLineEdit::textChanged,
            this, &LayerBox::onWeightChanged);

    weightHBox->addWidget(weightLabel);
    weightHBox->addWidget(m_weightEntry);
    weightHBox->addWidget(m_weightFuncButton);
    weightHBox->addWidget(m_deleteLayerButton);

    m_mainLayout->addLayout(weightHBox);

    // Tree View
    m_treeView = new QTreeView;
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_model = new QStandardItemModel;
    m_model->setHorizontalHeaderLabels({"Child Type", "Class", "Name",
        "Weight", "Attack Env.", "A. Env. Scaler", "Duration Env.", "D. Env. Scaler"});

    m_treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_treeView->header()->setStretchLastSection(false);

    m_treeView->setModel(m_model);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    m_treeView->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);

    m_treeView->setDragDropMode(QAbstractItemView::DropOnly);
    m_treeView->setDefaultDropAction(Qt::MoveAction);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setIndentation(0);

    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);

    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &LayerBox::onContextMenu);

    // Widget-scope shortcuts override the window-level copyAct/pasteAct in MainWindow
    auto* copyShortcut = new QShortcut(QKeySequence::Copy, m_treeView);
    copyShortcut->setContext(Qt::WidgetShortcut);
    connect(copyShortcut, &QShortcut::activated, this, [this](){
        qDebug() << "[DEBUG] LayerBox widget-scope copy shortcut activated";
        onCopySelected();
    });

    auto* pasteShortcut = new QShortcut(QKeySequence::Paste, m_treeView);
    pasteShortcut->setContext(Qt::WidgetShortcut);
    connect(pasteShortcut, &QShortcut::activated, this, [this](){
        qDebug() << "[DEBUG] LayerBox widget-scope paste shortcut activated";
        onPasteClipboard();
    });

    m_mainLayout->addWidget(m_treeView);

    this->setMinimumHeight(200);

    // Populate UI from backend (handles reload when switching events)
    Layer& layer = getBackendLayer();

    // Block signals so onWeightChanged doesn't write back during initialisation
    m_weightEntry->blockSignals(true);
    m_weightEntry->setText(layer.by_layer);
    m_weightEntry->blockSignals(false);

    for (const Package& pkg : layer.discrete_packages) {
        int row = m_model->rowCount();
        auto* rowItem  = new QStandardItem(QString::number(row));
        auto* typeItem = new QStandardItem(eventtypeToDisplayString(pkg.event_type.toInt()));
        auto* nameItem = new QStandardItem(pkg.event_name);
        rowItem->setFlags(rowItem->flags()   & ~Qt::ItemIsEditable);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_model->appendRow({rowItem, typeItem, nameItem,
            new QStandardItem(pkg.weight),
            new QStandardItem(pkg.attack_envelope),
            new QStandardItem(pkg.attackenvelope_scale),
            new QStandardItem(pkg.duration_envelope),
            new QStandardItem(pkg.durationenvelope_scale)
        });
    }

    // Sync editable package-field columns back to the backend on edit
    connect(m_model, &QStandardItemModel::dataChanged,
            this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
        for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
            for (int col = topLeft.column(); col <= bottomRight.column(); ++col) {
                if (col < 3) continue;
                Layer& layer = getBackendLayer();
                if (row < 0 || row >= layer.discrete_packages.size()) continue;
                Package& pkg = layer.discrete_packages[row];
                QString val = m_model->item(row, col)->text();
                switch (col) {
                    case 3: pkg.weight                  = val; break;
                    case 4: pkg.attack_envelope         = val; break;
                    case 5: pkg.attackenvelope_scale    = val; break;
                    case 6: pkg.duration_envelope       = val; break;
                    case 7: pkg.durationenvelope_scale  = val; break;
                }
            }
        }
    });

    // Package-field columns are hidden until Discrete mode is active
    for (int col = 3; col <= 7; ++col)
        m_treeView->setColumnHidden(col, true);
}


void LayerBox::reloadFromBackend() {
    Layer& layer = getBackendLayer();

    m_weightEntry->blockSignals(true);
    m_weightEntry->setText(layer.by_layer);
    m_weightEntry->blockSignals(false);

    // Preserve package-field column visibility across the rebuild.
    QList<bool> hiddenCols;
    for (int col = 0; col < m_model->columnCount(); ++col)
        hiddenCols.append(m_treeView->isColumnHidden(col));

    m_model->removeRows(0, m_model->rowCount());
    for (const Package& pkg : layer.discrete_packages) {
        int row = m_model->rowCount();
        auto* rowItem  = new QStandardItem(QString::number(row));
        auto* typeItem = new QStandardItem(eventtypeToDisplayString(pkg.event_type.toInt()));
        auto* nameItem = new QStandardItem(pkg.event_name);
        rowItem->setFlags(rowItem->flags()   & ~Qt::ItemIsEditable);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_model->appendRow({rowItem, typeItem, nameItem,
            new QStandardItem(pkg.weight),
            new QStandardItem(pkg.attack_envelope),
            new QStandardItem(pkg.attackenvelope_scale),
            new QStandardItem(pkg.duration_envelope),
            new QStandardItem(pkg.durationenvelope_scale)
        });
    }

    for (int col = 0; col < hiddenCols.size() && col < m_model->columnCount(); ++col)
        m_treeView->setColumnHidden(col, hiddenCols[col]);
}


Layer& LayerBox::getBackendLayer() {
    ProjectManager* pm = Inst::get_project_manager();
    HEvent* hevent = nullptr;
    if (m_eventType == top) {
        hevent = &pm->topevent();
    } else if (m_eventType == high) {
        hevent = &pm->highevents()[m_eventIndex];
    } else if (m_eventType == mid) {
        hevent = &pm->midevents()[m_eventIndex];
    } else if (m_eventType == low) {
        hevent = &pm->lowevents()[m_eventIndex];
    } else { // bottom
        hevent = &pm->bottomevents()[m_eventIndex].event;
    }
    return hevent->event_layers[m_layerIndex];
}


QStandardItem* LayerBox::extractItemFromDrop(QDropEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (!mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
        qDebug() << "extractItemFromDrop: MIME format not matched";
        return nullptr;
    }

    QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    QString foundType;
    QString foundName;
    bool gotType = false;
    bool gotName = false;

    while (!stream.atEnd()) {
        int row, col;
        QMap<int, QVariant> roleDataMap;
        stream >> row >> col >> roleDataMap;

        QString roleType = roleDataMap.value(Qt::UserRole + 1).toString();
        QString roleName = roleDataMap.value(Qt::UserRole + 2).toString();
        if (!roleType.isEmpty()) { foundType = roleType; gotType = true; }
        if (!roleName.isEmpty()) { foundName = roleName; gotName = true; }
        if (gotType && gotName) break;
    }

    if (!gotType && !gotName) {
        qDebug() << "extractItemFromDrop: no usable data found";
        return nullptr;
    }

    QStandardItem* item = new QStandardItem;
    item->setText(gotName ? foundName : foundType);
    item->setData(foundType, Qt::UserRole + 1);
    item->setData(foundName, Qt::UserRole + 2);
    return item;
}

void LayerBox::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void LayerBox::dropEvent(QDropEvent* event) {
    event->acceptProposedAction();

    QStandardItem* droppedItem = extractItemFromDrop(event);
    if (!droppedItem) {
        qDebug() << "extractItemFromDrop returned null";
        return;
    }

    int newIndex = m_model->rowCount();
    m_model->appendRow({
        new QStandardItem(QString::number(newIndex)),
        new QStandardItem(droppedItem->text()),
        new QStandardItem("Dropped item info here")
    });
}

bool LayerBox::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_treeView && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        const Qt::Key key = static_cast<Qt::Key>(keyEvent->key());

        if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
            onDeleteSelected();
            return true;
        }
    }

    if (obj == m_treeView->viewport()) {
        if (event->type() == QEvent::Drop) {

            QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
            QStandardItem* droppedItem = extractItemFromDrop(dropEvent);

            if (!droppedItem) {
                qDebug() << "eventFilter: extractItemFromDrop returned null";
                return false;
            }

            QString droppedType = droppedItem->data(Qt::UserRole + 1).toString();
            QString droppedName = droppedItem->data(Qt::UserRole + 2).toString();

            qDebug() << "eventFilter: Item is" << droppedName;

            int index = m_model->rowCount();
            auto* rowItem  = new QStandardItem(QString::number(index));
            auto* typeItem = new QStandardItem(droppedType);  // display string for UI
            auto* nameItem = new QStandardItem(droppedName);
            rowItem->setFlags(rowItem->flags()   & ~Qt::ItemIsEditable);
            typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            m_model->appendRow({rowItem, typeItem, nameItem,
                new QStandardItem(""),
                new QStandardItem(""),
                new QStandardItem(""),
                new QStandardItem(""),
                new QStandardItem("")
            });

            // Write the new package directly into the backend layer
            Package pkg;
            pkg.event_name = droppedName;
            pkg.event_type = displayStringToEventtypeString(droppedType);
            getBackendLayer().discrete_packages.append(pkg);

            qDebug() << "Added package" << droppedName
                     << "- total packages:" << getBackendLayer().discrete_packages.size();

            return true;
        }
    }
    return QFrame::eventFilter(obj, event);
}

void LayerBox::onWeightChanged(const QString& text) {
    getBackendLayer().by_layer = text;
}

void LayerBox::onDeleteLayerClicked() {
    emit deleteRequested(this);
}

QList<int> LayerBox::selectedRows() const {
    QList<int> rows;
    for (const QModelIndex& idx : m_treeView->selectionModel()->selectedRows()) {
        rows.append(idx.row());
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

void LayerBox::onCopySelected() {
    qDebug() << "[DEBUG] LayerBox::onCopySelected called, selected rows:" << selectedRows();
    QList<int> rows = selectedRows();
    if (rows.isEmpty()) return;
    Layer& layer = getBackendLayer();
    m_clipboard.clear();
    for (int row : rows) {
        m_clipboard.append(layer.discrete_packages[row]);
    }
}

void LayerBox::onPasteClipboard() {
    qDebug() << "[DEBUG] LayerBox::onPasteClipboard called, clipboard size:" << m_clipboard.size();
    if (m_clipboard.isEmpty()) return;
    Layer& layer = getBackendLayer();
    for (const Package& pkg : m_clipboard) {
        int index = m_model->rowCount();
        auto* rowItem  = new QStandardItem(QString::number(index));
        auto* typeItem = new QStandardItem(eventtypeToDisplayString(pkg.event_type.toInt()));
        auto* nameItem = new QStandardItem(pkg.event_name);
        rowItem->setFlags(rowItem->flags()   & ~Qt::ItemIsEditable);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_model->appendRow({rowItem, typeItem, nameItem,
            new QStandardItem(pkg.weight),
            new QStandardItem(pkg.attack_envelope),
            new QStandardItem(pkg.attackenvelope_scale),
            new QStandardItem(pkg.duration_envelope),
            new QStandardItem(pkg.durationenvelope_scale)
        });
        layer.discrete_packages.append(pkg);
    }
}

void LayerBox::onDeleteSelected() {
    QList<int> rows = selectedRows();
    if (rows.isEmpty()) return;
    Layer& layer = getBackendLayer();
    // Remove in reverse order so indices stay valid
    for (int i = rows.size() - 1; i >= 0; --i) {
        int row = rows[i];
        m_model->removeRow(row);
        layer.discrete_packages.removeAt(row);
    }
    // Refresh row-number column
    for (int i = 0; i < m_model->rowCount(); ++i) {
        m_model->item(i, 0)->setText(QString::number(i));
    }
}

void LayerBox::onContextMenu(const QPoint& pos) {
    bool hasSelection = !selectedRows().isEmpty();
    QMenu menu(this);
    //QAction* dupAction  = menu.addAction("Duplicate");
    QAction* delAction  = menu.addAction("Delete");
    //dupAction->setEnabled(hasSelection);
    delAction->setEnabled(hasSelection);

    QAction* chosen = menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    /*if (chosen == dupAction) {
        onCopySelected();
        onPasteClipboard();
    } else */ 
    if (chosen == delAction) {
        onDeleteSelected();
    }
}

LayerBox::~LayerBox() {}

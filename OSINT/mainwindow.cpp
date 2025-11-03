#include "mainwindow.h"
#include <QTreeWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateEdit>
#include <QMessageBox>
#include <QFile>
#include <QHeaderView>
#include <QScrollArea>
#include <QLabel>
#include <QGroupBox>
#include <QFileInfo>

QJsonObject Entry::toJson() const {
    QJsonObject o;
    QJsonObject f;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
        f[it.key()] = it.value();
    o["fields"] = f;
    if (date.isValid()) o["date"] = date.toString(Qt::ISODate);
    return o;
}

Entry Entry::fromJson(const QJsonObject &o) {
    Entry e;
    if (o.contains("fields") && o["fields"].isObject()) {
        QJsonObject fo = o["fields"].toObject();
        for (auto it = fo.constBegin(); it != fo.constEnd(); ++it)
            e.fields[it.key()] = it.value().toString();
    }
    if (o.contains("date")) e.date = QDate::fromString(o["date"].toString(), Qt::ISODate);
    return e;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    tree(nullptr),
    formScroll(nullptr),
    formContainer(nullptr),
    formLayout(nullptr),
    addFieldBtn(nullptr),
    addCatBtn(nullptr),
    addPlatBtn(nullptr)
{
    setupUi();
    newProject();
}

MainWindow::~MainWindow() = default;
void MainWindow::setupUi() {
    // Menu
    auto *fileMenu = menuBar()->addMenu(tr("File"));
    QAction *newProjectAct = fileMenu->addAction(tr("New Project"));
    QMenu *newFromTemplateMenu = new QMenu(tr("New From Template"), this);
    fileMenu->addMenu(newFromTemplateMenu);

    QAction *openAct = fileMenu->addAction(tr("Open..."));
    QAction *saveAct = fileMenu->addAction(tr("Save"));
    fileMenu->addSeparator();
    QAction *exportAct = fileMenu->addAction(tr("Export..."));
    QAction *remove = fileMenu->addAction(tr("Remove Selected"));

    connect(newProjectAct, &QAction::triggered, this, &MainWindow::newProject);
    connect(openAct, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveProject);
    connect(exportAct, &QAction::triggered, this, &MainWindow::exportData);
    connect(remove, &QAction::triggered, this, &MainWindow::removeSelected);

    QStringList templateFiles = {"C:/Git/OSINT/Templates/basic.json"};
    for (const QString &file : templateFiles) {
        QAction *templateAction = new QAction(QFileInfo(file).baseName(), this);
        newFromTemplateMenu->addAction(templateAction);
        connect(templateAction, &QAction::triggered, [this, file](){ this->loadProjectTemplate(file); });
    }
    QAction *customTemplateAct = newFromTemplateMenu->addAction(tr("Browse..."));
    connect(customTemplateAct, &QAction::triggered, this, [this](){
        QString file = QFileDialog::getOpenFileName(this, tr("Select Template JSON"), QString(), tr("Template JSON (*.json);;All Files (*)"));
        if (!file.isEmpty()) { loadProjectTemplate(file); }
    });

    // Layout
    auto *splitter = new QSplitter(this);

    // Left panel
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);

    tree = new QTreeWidget;
    tree->setHeaderLabel(tr("Categories / Platforms"));
    tree->header()->setSectionResizeMode(QHeaderView::Stretch);
    leftLayout->addWidget(tree, 1);  // stretch=1 zodat deze groeit

    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showTreeContextMenu);

    // Spacer pushes buttons down
    leftLayout->addStretch();

    addCatBtn = new QPushButton(tr("Add Category"));
    addPlatBtn = new QPushButton(tr("Add Platform"));

    // Make buttons small
    QSize btnSize(80, 25);
    addCatBtn->setMaximumSize(btnSize);
    addPlatBtn->setMaximumSize(btnSize);

    QHBoxLayout *buttonLayoutLeft = new QHBoxLayout;
    buttonLayoutLeft->addWidget(addCatBtn);
    buttonLayoutLeft->addWidget(addPlatBtn);
    buttonLayoutLeft->setAlignment(Qt::AlignLeft);

    leftLayout->addLayout(buttonLayoutLeft);

    leftWidget->setLayout(leftLayout);
    splitter->addWidget(leftWidget);

    // Right panel
    QWidget *right = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);

    // Label bovenaan het rechterpaneel
    QLabel *rightLabel = new QLabel(tr("Details / Form"), right);
    rightLayout->addWidget(rightLabel);

    // Scroll area en form container
    formScroll = new QScrollArea;
    formScroll->setWidgetResizable(true);
    formContainer = new QWidget;
    formLayout = new QVBoxLayout(formContainer);
    formContainer->setLayout(formLayout);
    formScroll->setWidget(formContainer);
    rightLayout->addWidget(formScroll, 1); // stretch = 1 zodat groeit

    // Spacer om knoppen onderaan te forceren
    rightLayout->addStretch();

    addFieldBtn = new QPushButton(tr("Add Field"));
    addFieldBtn->setEnabled(false);
    addFieldBtn->setMaximumSize(QSize(100, 30));
    rightLayout->addWidget(addFieldBtn);
    rightLayout->setAlignment(addFieldBtn, Qt::AlignBottom);

    right->setLayout(rightLayout);
    splitter->addWidget(right);


    setCentralWidget(splitter);

    connect(tree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onTreeSelectionChanged);
    connect(addFieldBtn, &QPushButton::clicked, this, &MainWindow::addFieldForCurrentPlatform);
    connect(addCatBtn, &QPushButton::clicked, this, &MainWindow::addCategory);
    connect(addPlatBtn, &QPushButton::clicked, this, &MainWindow::addPlatform);

    rebuildMenus();
}

void MainWindow::newProject() {
    platformFields.clear();
    platformDrafts.clear();
    currentProjectPath.clear();
    rebuildMenus();
    clearFormArea();
}

void MainWindow::loadProjectTemplate(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open template file."));
        return;
    }
    QByteArray dataB = f.readAll();
    f.close();
    QJsonDocument doc = QJsonDocument::fromJson(dataB);
    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("Invalid"), tr("Template file is not a valid JSON object."));
        return;
    }
    loadFromJson(doc.object());
    currentProjectPath.clear();
    rebuildMenus();
    buildFormForSelection();
}

void MainWindow::addCategory() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Category"), tr("Category name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;
    if (platformFields.contains(name)) {
        QMessageBox::information(this, tr("Exists"), tr("Category already exists"));
        return;
    }
    platformFields[name] = QMap<QString, QStringList>();
    platformDrafts[name] = QMap<QString, Entry>();
    rebuildMenus();
}

void MainWindow::addPlatform() {
    bool ok;
    QStringList cats = platformFields.keys();
    if (cats.isEmpty()) {
        QMessageBox::information(this, tr("No categories"), tr("Add a category first"));
        return;
    }
    QString cat = QInputDialog::getItem(this, tr("Select Category"), tr("Category:"), cats, 0, false, &ok);
    if (!ok) return;
    QString plat = QInputDialog::getText(this, tr("Add Platform"), tr("Platform name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || plat.isEmpty()) return;
    if (platformFields[cat].contains(plat)) {
        QMessageBox::information(this, tr("Exists"), tr("Platform already exists in this category"));
        return;
    }
    platformFields[cat][plat] = QStringList();
    platformDrafts[cat][plat] = Entry();
    rebuildMenus();
}

void MainWindow::addFieldForCurrentPlatform() {
    if (currentCategory.isEmpty() || currentPlatform.isEmpty()) return;
    bool ok;
    QString title = QInputDialog::getText(this, tr("Add Field"), tr("Field title:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || title.isEmpty()) return;
    QStringList &titles = platformFields[currentCategory][currentPlatform];
    if (titles.contains(title)) {
        QMessageBox::information(this, tr("Exists"), tr("A field with that title already exists for this platform."));
        return;
    }
    titles.append(title);
    platformDrafts[currentCategory][currentPlatform]; // ensure draft exists
    buildFormForSelection();
}

void MainWindow::onTreeSelectionChanged() {
    auto items = tree->selectedItems();
    if (items.isEmpty()) {
        currentCategory.clear();
        currentPlatform.clear();
        addFieldBtn->setEnabled(false);
        clearFormArea();
        return;
    }
    QTreeWidgetItem *it = items.first();
    QString type = it->data(0, Qt::UserRole).toString();
    if (type == "platform") {
        currentPlatform = it->text(0);
        currentCategory = it->parent()->text(0);
        addFieldBtn->setEnabled(true);
    } else {
        currentPlatform.clear();
        currentCategory = it->text(0);
        addFieldBtn->setEnabled(false);
    }
    buildFormForSelection();
}

void MainWindow::buildFormForSelection() {
    clearFormArea();
    currentFieldEdits.clear();
    if (!currentCategory.isEmpty() && currentPlatform.isEmpty()) {
        QMap<QString, QStringList> &plats = platformFields[currentCategory];
        for (const QString &platform : plats.keys())
            buildFormForPlatform(currentCategory, platform, formLayout);
    } else if (!currentCategory.isEmpty() && !currentPlatform.isEmpty()) {
        buildFormForPlatform(currentCategory, currentPlatform, formLayout);
    }
}

void MainWindow::buildFormForPlatform(const QString &category, const QString &platform, QVBoxLayout *parentLayout) {
    QGroupBox *group = new QGroupBox(category + " / " + platform);
    QFormLayout *fLayout = new QFormLayout(group);
    Entry &entry = platformDrafts[category][platform];

    QStringList fields = platformFields.value(category).value(platform);
    for (const QString &title : fields) {
        QLineEdit *edit = new QLineEdit;
        edit->setText(entry.fields.value(title));
        fLayout->addRow(title, edit);

        // Context menu for each field (QLineEdit)
        edit->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(edit, &QLineEdit::customContextMenuRequested, this, [=](const QPoint &pos){
            QMenu menu(edit);
            QAction *removeFieldAct = menu.addAction(tr("Remove Field"));
            QAction *selected = menu.exec(edit->mapToGlobal(pos));
            if (selected == removeFieldAct) {
                // Remove field from data structures
                auto &fieldList = platformFields[category][platform];
                fieldList.removeAll(title);
                platformDrafts[category][platform].fields.remove(title);
                // Rebuild the form to reflect changes
                buildFormForSelection();
            }
        });

        connect(edit, &QLineEdit::textChanged, [=](const QString &txt){
            onFieldValueChanged(category, platform, title, txt);
        });
        currentFieldEdits.append({category, platform, edit});
    }
    parentLayout->addWidget(group);
}


void MainWindow::clearFormArea() {
    while (QLayoutItem *item = formLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    currentFieldEdits.clear();
}

void MainWindow::onFieldValueChanged(const QString &category, const QString &platform, const QString &fieldTitle, const QString &value) {
    platformDrafts[category][platform].fields[fieldTitle] = value;
    platformDrafts[category][platform].date = QDate::currentDate();
}

void MainWindow::rebuildMenus() {
    tree->clear();
    for (const QString &cat : platformFields.keys()) {
        QTreeWidgetItem *catItem = new QTreeWidgetItem(tree);
        catItem->setText(0, cat);
        catItem->setData(0, Qt::UserRole, "category");
        for (const QString &plat : platformFields[cat].keys()) {
            QTreeWidgetItem *platItem = new QTreeWidgetItem(catItem);
            platItem->setText(0, plat);
            platItem->setData(0, Qt::UserRole, "platform");
        }
    }
    tree->expandAll();
}

// For brevity, you may leave out save/open export, or adapt them only to platformDrafts if needed.
void MainWindow::saveProject() {
    QString fn = currentProjectPath;
    if (fn.isEmpty()) fn = QFileDialog::getSaveFileName(this, tr("Save Project"), QString(), tr("OSINT Project (*.json)"));
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly)) { QMessageBox::warning(this, tr("Error"), tr("Could not open file for writing.")); return; }
    QJsonDocument doc(toJson());
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    currentProjectPath = fn;
    QMessageBox::information(this, tr("Saved"), tr("Project saved."));
}

void MainWindow::openProject() {
    QString fn = QFileDialog::getOpenFileName(this, tr("Open Project"), QString(), tr("OSINT Project (*.json);;All Files (*)"));
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, tr("Error"), tr("Could not open file.")); return; }
    QByteArray dataB = f.readAll();
    f.close();
    QJsonDocument doc = QJsonDocument::fromJson(dataB);
    if (!doc.isObject()) { QMessageBox::warning(this, tr("Invalid"), tr("File does not contain a valid project.")); return; }
    loadFromJson(doc.object());
    currentProjectPath = fn;
    rebuildMenus();
    buildFormForSelection();
}

QJsonObject MainWindow::toJson() const {
    QJsonObject root;
    for (auto cit = platformFields.constBegin(); cit != platformFields.constEnd(); ++cit) {
        QString cat = cit.key();
        QJsonObject catObj;
        for (auto pit = cit.value().constBegin(); pit != cit.value().constEnd(); ++pit) {
            QString plat = pit.key();
            QJsonObject platObj;
            Entry entry = platformDrafts.value(cat).value(plat);
            QJsonArray entriesArr;
            entriesArr.append(entry.toJson());
            platObj["entries"] = entriesArr;
            QJsonArray fieldsArr;
            for (const QString &f : pit.value()) fieldsArr.append(f);
            platObj["fields"] = fieldsArr;
            catObj[plat] = platObj;
        }
        root[cat] = catObj;
    }
    return root;
}

void MainWindow::loadFromJson(const QJsonObject &root) {
    platformFields.clear();
    platformDrafts.clear();
    for (auto cit = root.constBegin(); cit != root.constEnd(); ++cit) {
        QString cat = cit.key();
        if (!cit.value().isObject()) continue;
        QJsonObject catObj = cit.value().toObject();
        QMap<QString, QStringList> fieldsMap;
        QMap<QString, Entry> drafts;
        for (auto pit = catObj.constBegin(); pit != catObj.constEnd(); ++pit) {
            QString plat = pit.key();
            if (!pit.value().isObject()) continue;
            QJsonObject platObj = pit.value().toObject();
            QStringList flds;
            Entry draft;
            if (platObj.contains("fields") && platObj["fields"].isArray()) {
                QJsonArray arr = platObj["fields"].toArray();
                for (auto f : arr) flds.append(f.toString());
            }
            if (platObj.contains("entries") && platObj["entries"].isArray()) {
                QJsonArray arr = platObj["entries"].toArray();
                if (!arr.isEmpty() && arr.first().isObject())
                    draft = Entry::fromJson(arr.first().toObject());
            }
            fieldsMap[plat] = flds;
            drafts[plat] = draft;
        }
        platformFields[cat] = fieldsMap;
        platformDrafts[cat] = drafts;
    }
}

void MainWindow::removeSelected() {
    auto items = tree->selectedItems();
    if (items.isEmpty()) return;
    QTreeWidgetItem *it = items.first();
    QString type = it->data(0, Qt::UserRole).toString();
    if (type == "platform") {
        QString plat = it->text(0);
        QTreeWidgetItem *parent = it->parent();
        if (!parent) return;
        QString cat = parent->text(0);
        platformFields[cat].remove(plat);
        platformDrafts[cat].remove(plat);
    } else if (type == "category") {
        QString cat = it->text(0);
        platformFields.remove(cat);
        platformDrafts.remove(cat);
    }
    rebuildMenus();
    clearFormArea();
}

void MainWindow::showTreeContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = tree->itemAt(pos);
    if (!item) return;

    QString type = item->data(0, Qt::UserRole).toString();
    QMenu contextMenu(this);

    QAction *removeAct = contextMenu.addAction(tr("Remove"));
    connect(removeAct, &QAction::triggered, this, [this, item, type]() {
        tree->setCurrentItem(item);
        removeSelected();
    });

    contextMenu.exec(tree->viewport()->mapToGlobal(pos));
}

void MainWindow::exportData() {
    QString fn = QFileDialog::getSaveFileName(this, tr("Export Data"), QString(), tr("JSON (*.json);;CSV (*.csv)"));
    if (fn.isEmpty()) return;

    if (fn.endsWith(".json", Qt::CaseInsensitive)) {
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Error"), tr("Could not open file for writing."));
            return;
        }
        QJsonDocument doc(toJson());
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
        QMessageBox::information(this, tr("Exported"), tr("Exported as JSON."));
        return;
    }
    if (fn.endsWith(".csv", Qt::CaseInsensitive)) {
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Error"), tr("Could not open file for writing."));
            return;
        }
        QTextStream out(&f);
        out << "Category,Platform,FieldsAsJSON,Date\n";
        for (auto cit = platformFields.constBegin(); cit != platformFields.constEnd(); ++cit) {
            QString cat = cit.key();
            for (auto pit = cit.value().constBegin(); pit != cit.value().constEnd(); ++pit) {
                QString plat = pit.key();
                Entry entry = platformDrafts.value(cat).value(plat);
                QJsonObject fo;
                for (const QString &field : pit.value()) {
                    fo[field] = entry.fields.value(field);
                }
                QJsonDocument d(fo);
                QString fieldsJson = QString::fromUtf8(d.toJson(QJsonDocument::Compact));
                auto esc = [](const QString &s) {
                    QString t = s;
                    t.replace("\"", "\"\"");
                    return "\"" + t + "\"";
                };
                out << esc(cat) << "," << esc(plat) << "," << esc(fieldsJson) << "," << esc(entry.date.toString(Qt::ISODate)) << "\n";
            }
        }
        f.close();
        QMessageBox::information(this, tr("Exported"), tr("Exported as CSV."));
        return;
    }
    QMessageBox::information(this, tr("Unknown"), tr("Unsupported export format. Choose .json or .csv"));
}

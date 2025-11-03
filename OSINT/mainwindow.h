#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QDate>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QFormLayout;
class QTreeWidgetItem;
class QWidget;
class QGroupBox;
QT_END_NAMESPACE

struct Entry {
    QMap<QString, QString> fields;
    QDate date; // optional general date field
    QJsonObject toJson() const;
    static Entry fromJson(const QJsonObject &o);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void addCategory();
    void addPlatform();
    void removeSelected();
    void onTreeSelectionChanged();
    void newProject();
    void openProject();
    void saveProject();
    void exportData();
    void addFieldForCurrentPlatform();
    void loadProjectTemplate(const QString &filePath);
//
    void showTreeContextMenu(const QPoint &pos);

private:
    void setupUi();
    void rebuildMenus();
    QTreeWidgetItem* findOrCreateCategoryItem(const QString &category);
    QTreeWidgetItem* findPlatformItem(const QString &category, const QString &platform);
    void loadFromJson(const QJsonObject &root);
    QJsonObject toJson() const;
    void buildFormForSelection();
    void buildFormForPlatform(const QString &category, const QString &platform, QVBoxLayout *parentLayout);
    void clearFormArea();
    void onFieldValueChanged(const QString &category, const QString &platform, const QString &fieldTitle, const QString &value);

    // Layout and widgets
    QTreeWidget *tree;
    QScrollArea *formScroll;
    QWidget *formContainer;
    QVBoxLayout *formLayout;
    QPushButton *addFieldBtn;

    // Left panel quick-add buttons
    QPushButton *addCatBtn;
    QPushButton *addPlatBtn;

    // Data structures
    QMap<QString, QMap<QString, QStringList>> platformFields;
    QMap<QString, QMap<QString, Entry>> platformDrafts;

    struct FieldWidget { QString category, platform; QLineEdit *edit; };
    QVector<FieldWidget> currentFieldEdits;

    QString currentCategory;
    QString currentPlatform;
    QString currentProjectPath;
//
    void createContextMenuForTree();
};

#endif // MAINWINDOW_H

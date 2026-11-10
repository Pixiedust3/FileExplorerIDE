#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include "FileExplorer.h"

class QDockWidget;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QTreeView;
class TerminalPanel;
class  EditorTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void buildLayout();
    void buildMenus();

    EditorTab *currentTab() const;
    void openFile(const QString &path);
    void refreshTabTitle(EditorTab *tab);
    void updateWindowTitle();
    FileExplorer explorer;

    QAction *openFolderAction = nullptr;
    QDockWidget    *filesDock    = nullptr;
    QTreeView      *treeView     = nullptr;
    QStackedWidget *treeStack    = nullptr;
    QTabWidget     *tabWidget    = nullptr;
    TerminalPanel  *terminalPane = nullptr;
    QSplitter      *splitter     = nullptr;

private slots:
    void onOpenFolderTriggered();
    void onNewFileTriggered();
    void onOpenFileTriggered();
    void onSaveTriggered();
    void onSaveAsTriggered();
    void onCloseTabRequested(int index);
    void onCurrentTabChanged(int index);
    void onFileDoubleClicked(const QModelIndex &index);


};
#endif // MAINWINDOW_H

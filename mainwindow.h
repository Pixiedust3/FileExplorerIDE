#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "FileExplorer.h"

class QDockWidget;
class QPlainTextEdit;
class QSplitter;
class QTabWidget;
class QTreeView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void buildLayout();
    void buildMenus();

    FileExplorer explorer;

    QDockWidget    *filesDock    = nullptr;
    QTreeView      *treeView     = nullptr;
    QTabWidget     *tabWidget    = nullptr;
    QPlainTextEdit *terminalPane = nullptr;
    QSplitter      *splitter     = nullptr;

private slots:
    void onOpenFolderTriggered();


};
#endif // MAINWINDOW_H

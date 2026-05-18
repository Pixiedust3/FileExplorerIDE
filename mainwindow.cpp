#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,explorer(this)
{
    ui->setupUi(this);
    connect(ui->actionOpen_Folder_2, &QAction::triggered, this, &MainWindow::onOpenFolderTriggered);
    explorer.setupDirectoryView(ui->treeView , QDir::home().path());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onOpenFolderTriggered()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Open Folder"), QDir::homePath());
    if (directory.isEmpty()) {
        return;
    }

    explorer.setupDirectoryView(ui->treeView, directory);
}


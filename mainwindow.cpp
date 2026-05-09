#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,explorer(this)
{
    ui->setupUi(this);
    explorer.setupDirectoryView(ui->treeView , QDir::home().path());
}

MainWindow::~MainWindow()
{
    delete ui;
}

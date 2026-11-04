#include "mainwindow.h"

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTreeView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , explorer(this)
{
    buildLayout();
    buildMenus();

    explorer.setupDirectoryView(treeView, QDir::homePath());

    setWindowTitle("FileExplorer IDE");
    resize(1249, 757);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildLayout()
{
    // --- Left: file tree in a dock ---
    filesDock = new QDockWidget(tr("Files"), this);
    filesDock->setObjectName("filesDock");
    treeView = new QTreeView(filesDock);
    filesDock->setWidget(treeView);
    addDockWidget(Qt::LeftDockWidgetArea, filesDock);
    filesDock->setMinimumWidth(180);
    filesDock->setMaximumWidth(420);

    // --- Central: tabs stacked over the terminal ---
    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("tabWidget");
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);

    terminalPane = new QPlainTextEdit(this);
    terminalPane->setObjectName("terminalPane");
    terminalPane->setReadOnly(true);
    terminalPane->setPlaceholderText("Terminal place holder");

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->setObjectName("splitter");
    splitter->addWidget(tabWidget);
    splitter->addWidget(terminalPane);
    splitter->setStretchFactor(0, 3);   // tabs get 3/4 of resize
    splitter->setStretchFactor(1, 1);   // terminal gets 1/4
    splitter->setSizes({540, 190});

    setCentralWidget(splitter);
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::buildMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openFolder = fileMenu->addAction(tr("Open &Folder"));
    openFolder->setShortcut(QKeySequence(tr("Ctrl+Shift+O")));
    connect(openFolder, &QAction::triggered,
            this, &MainWindow::onOpenFolderTriggered);

    QAction *openFile = fileMenu->addAction(tr("&Open File"));
    openFile->setShortcut(QKeySequence(tr("Ctrl+O")));
    // TODO Phase 2: wire up

    fileMenu->addSeparator();

    QAction *save = fileMenu->addAction(tr("&Save"));
    save->setShortcut(QKeySequence::Save);          // Ctrl+S
    // TODO Phase 2: wire up

    QAction *saveAs = fileMenu->addAction(tr("Save &As"));
    saveAs->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
    // TODO Phase 2: wire up

    fileMenu->addSeparator();

    QAction *quit = fileMenu->addAction(tr("&Quit"));
    quit->setShortcut(QKeySequence::Quit);          // Ctrl+Q
    connect(quit, &QAction::triggered, this, &QWidget::close);

    // --- View menu: toggles the file tree dock ---
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(filesDock->toggleViewAction());
}

void MainWindow::onOpenFolderTriggered()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, tr("Open Folder"), QDir::homePath());
    if (directory.isEmpty())
        return;

    explorer.setupDirectoryView(treeView, directory);
    statusBar()->showMessage(tr("Opened %1").arg(directory), 3000);
}

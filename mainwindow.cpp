#include "mainwindow.h"
#include "EditorTab.h"
#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
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
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onCloseTabRequested);
    connect(tabWidget, &QTabWidget::currentChanged,    this, &MainWindow::onCurrentTabChanged);

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

    QAction *newFile = fileMenu->addAction(tr("&New File"));
    newFile->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(newFile, &QAction::triggered, this, &MainWindow::onNewFileTriggered);

    QAction *openFile = fileMenu->addAction(tr("&Open File"));
    openFile->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(openFile, &QAction::triggered, this, &MainWindow::onOpenFileTriggered);


    QAction *openFolder = fileMenu->addAction(tr("Open &Folder"));
    openFolder->setShortcut(QKeySequence(tr("Ctrl+Shift+O")));
    connect(openFolder, &QAction::triggered,
            this, &MainWindow::onOpenFolderTriggered);


    fileMenu->addSeparator();

    QAction *save = fileMenu->addAction(tr("&Save"));
    save->setShortcut(QKeySequence::Save);          // Ctrl+S
    connect(save, &QAction::triggered, this, &MainWindow::onSaveTriggered);

    QAction *saveAs = fileMenu->addAction(tr("Save &As"));
    saveAs->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
    connect(saveAs, &QAction::triggered, this, &MainWindow::onSaveAsTriggered);

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

EditorTab *MainWindow::currentTab() const
{
    return qobject_cast<EditorTab *>(tabWidget->currentWidget());
}

void MainWindow::updateWindowTitle()
{
    if (auto *tab = currentTab())
        setWindowTitle(tr("%1 — FileExplorer IDE").arg(tab->displayName()));
    else
        setWindowTitle(tr("FileExplorer IDE"));
}

void MainWindow::openFile(const QString &path)
{
    // Already open? Focus it instead of duplicating.
    for (int i = 0; i < tabWidget->count(); ++i) {
        auto *tab = qobject_cast<EditorTab *>(tabWidget->widget(i));
        if (tab && tab->filePath() == path) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }

    auto *tab = new EditorTab(this);
    if (!tab->load(path)) {
        delete tab;
        return;
    }

    connect(tab, &EditorTab::titleChanged, this, &MainWindow::refreshTabTitle);
    connect(tab, &EditorTab::statusMessage, this,
            [this](const QString &msg, int ms) { statusBar()->showMessage(msg, ms); });

    const int index = tabWidget->addTab(tab, tab->displayName());
    tabWidget->setTabToolTip(index, path);
    tabWidget->setCurrentIndex(index);
    statusBar()->showMessage(tr("Opened %1").arg(path), 3000);
}

void MainWindow::refreshTabTitle(EditorTab *tab)
{
    const int index = tabWidget->indexOf(tab);
    if (index < 0)
        return;

    tabWidget->setTabText(index, tab->displayName());
    tabWidget->setTabToolTip(index, tab->filePath());

    if (tab == currentTab())
        updateWindowTitle();
}

void MainWindow::onCurrentTabChanged(int index)
{
    Q_UNUSED(index)
    updateWindowTitle();
}

void MainWindow::onNewFileTriggered()
{
    auto *tab = new EditorTab(this);
    connect(tab, &EditorTab::titleChanged, this, &MainWindow::refreshTabTitle);
    connect(tab, &EditorTab::statusMessage, this,
            [this](const QString &msg, int ms) { statusBar()->showMessage(msg, ms); });

    const int index = tabWidget->addTab(tab, tab->displayName());
    tabWidget->setCurrentIndex(index);
    tab->setFocus();
}

void MainWindow::onOpenFileTriggered()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open File"), QDir::homePath());
    if (path.isEmpty())
        return;

    openFile(path);
}

void MainWindow::onSaveTriggered()
{
    if (auto *tab = currentTab())
        tab->save();
    else
        statusBar()->showMessage(tr("No file open"), 3000);
}

void MainWindow::onSaveAsTriggered()
{
    if (auto *tab = currentTab())
        tab->saveAs();
    else
        statusBar()->showMessage(tr("No file open"), 3000);
}

void MainWindow::onCloseTabRequested(int index)
{
    auto *tab = qobject_cast<EditorTab *>(tabWidget->widget(index));
    if (!tab)
        return;

    if (tab->isDirty()) {
        const auto choice = QMessageBox::warning(
            this, tr("Unsaved changes"),
            tr("Save changes to %1?").arg(tab->displayName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (choice == QMessageBox::Cancel)
            return;
        if (choice == QMessageBox::Save && !tab->save())
            return;
    }

    tabWidget->removeTab(index);
    tab->deleteLater();
}

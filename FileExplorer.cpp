#include "FileExplorer.h"
#include <QDir>

FileExplorer::FileExplorer(QObject *parent) : QObject(parent) {
    model_ = new QFileSystemModel(this);
}

void FileExplorer::setupDirectoryView(QTreeView *view, const QString &path) {
    model_->setRootPath(path);

    view->setModel(model_);
    view->setRootIndex(model_->index(path));

    view->setSortingEnabled(true);
    view->setColumnWidth(0, 250);
}
void applyTheme(QWidget* target)
{
    QString style = QString(
        "QTreeView {"
        "  background-color: #292828;"
        "  color: #D4BE98;"
        "  selection-background-color: #4B4A4A;"
        "}"
        "QHeaderView::section {"
        "  background-color: #171819;"
        "  color: #D4BE98;"
        "}"
        );
    target->setStyleSheet(style);
}

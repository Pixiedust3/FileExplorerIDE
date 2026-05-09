#include <Theme.h>
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

    view->styleSheet(QString
                     (
        "QTreeView{"
        "background-color: %1;"
        "color: %2;"
        "selection-background-color: %3;"
        "}"
        "QHeaderView::section{"
        "background-color: %4;"
        "color:%2;"
        "}"
        ).arg(
        Theme::background,
        Theme::text,
        Theme::selected,
        Theme::surface1
)));


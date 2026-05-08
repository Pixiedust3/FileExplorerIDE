#ifndef FILEEXPLORER_H
#define FILEEXPLORER_H

#include<QObject>
#include<QTreeView>
#include<QFileSystemModel>

class FileExplorer : public QObject{
    Q_OBJECT
public:
    explicit FileExplorer(QObject *root = nullptr);
    void setupDirectoryView(QTreeView *view , const QString& path);
};


#endif // FILEEXPLORER_

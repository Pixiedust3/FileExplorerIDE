#ifndef EDITORTAB_H
#define EDITORTAB_H

#include<QPlainTextEdit>

class EditorTab : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit EditorTab(QWidget *parent = nullptr);

    bool load(const QString &path);
    bool save();
    bool saveAs();

    QString filePath() const      { return path_; }
    bool isUntitled() const       { return path_.isEmpty(); }
    bool isDirty() const          { return document()->isModified(); }
    QString displayName() const;

signals:
    void titleChanged(EditorTab *tab);
    void statusMessage(const QString &msg, int timeoutMs);

private:
    bool writeToDisk(const QString &path);
    QString path_;
};


#endif // EDITORTAB_H

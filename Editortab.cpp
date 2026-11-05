#include "EditorTab.h"
#include "Theme.h"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>

EditroTab::EditorTab(QWidget *parent)
    :QPlainTextEdit(parent)
{
    QFont font =QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    setFont(font);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    setLineWrapMode(QPlainTextEdit::NoWrap);

    setStyleSheet(QStringLiteral(
                      "QPlainTextEdit { background-color: %1; color: %2; selection-background-color: %3; }")
                      .arg(Theme::background, Theme::text, Theme::selected));

    connect(document(), &QTextDocument::modificationChanged,
            this, [this](bool) { emit titleChanged(this); });
}

QString EditorTab::displayName() const
{
    const QString name = path_.isEmpty() ? tr("untitled") : QFileInfo(path_).fileName();
    return isDirty() ? QStringLiteral("● ") + name : name;
}

bool EditorTab::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit statusMessage(tr("Cannot open %1: %2").arg(path, file.errorString()), 4000);
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    // Guard: refuse obvious binaries (NUL byte in the first 4 KB)
    if (data.left(4096).contains('\0')) {
        emit statusMessage(tr("%1 is a binary file, not opening").arg(path), 4000);
        return false;
    }

    setPlainText(QString::fromUtf8(data));
    path_ = path;
    document()->setModified(false);
    return true;
}

bool EditorTab::writeToDisk(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit statusMessage(tr("Save failed: %1").arg(file.errorString()), 4000);
        return false;
    }

    const QByteArray data = toPlainText().toUtf8();
    if (file.write(data) != data.size()) {
        emit statusMessage(tr("Save failed: %1").arg(file.errorString()), 4000);
        return false;
    }

    file.close();
    document()->setModified(false);
    emit statusMessage(tr("Saved %1").arg(path), 3000);
    return true;
}

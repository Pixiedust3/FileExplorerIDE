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



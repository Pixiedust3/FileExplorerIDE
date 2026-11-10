#include "TerminalPanel.h"
#include "AnsiFilter.h"
#include "Theme.h"

#include <QDir>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QApplication>
#include <QClipboard>

#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

namespace {
const QChar kPromptStart('\x01');
const QChar kPromptEnd('\x02');
const int kMaxHistory = 200;

QString shellQuote(const QString &value)
{
    QString quoted = value;
    quoted.replace(QLatin1Char('\''), QLatin1String("'\\''"));
    return QLatin1Char('\'') + quoted + QLatin1Char('\'');
}

int signalGroupExceptBash(pid_t pgid, pid_t bashPid)
{
    DIR *proc = opendir("/proc");
    if (!proc)
        return -1;

    int signalled = 0;
    while (dirent *entry = readdir(proc)) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9')
            continue;

        const pid_t pid = static_cast<pid_t>(atoi(entry->d_name));
        if (pid == bashPid)
            continue;

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);

        FILE *file = fopen(path, "r");
        if (!file)
            continue;

        char line[512];
        if (fgets(line, sizeof(line), file)) {
            char *closing = strrchr(line, ')');
            if (closing) {
                char state;
                int ppid, pgrp;
                if (sscanf(closing + 1, " %c %d %d", &state, &ppid, &pgrp) == 3
                    && pgrp == pgid) {
                    if (kill(pid, SIGINT) == 0)
                        ++signalled;
                }
            }
        }
        fclose(file);
    }
    closedir(proc);
    return signalled;
}
}

TerminalPanel::TerminalPanel(QWidget *parent)
    : QPlainTextEdit(parent)
    , shell_(new QProcess(this))
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    setFont(font);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setUndoRedoEnabled(false);
    setMaximumBlockCount(20000);

    setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background-color: %1; color: %2; selection-background-color: %3; }")
        .arg(Theme::surface2, Theme::textfile, Theme::selected));

    shell_->setProcessChannelMode(QProcess::MergedChannels);
    shell_->setChildProcessModifier([]() { setpgid(0, 0); });

    connect(shell_, &QProcess::started, this, &TerminalPanel::onStarted);
    connect(shell_, &QProcess::readyReadStandardOutput, this, &TerminalPanel::onReadyRead);
    connect(shell_, &QProcess::finished, this, &TerminalPanel::onFinished);
    connect(shell_, &QProcess::errorOccurred, this, &TerminalPanel::onErrorOccurred);

    connect(document(), &QTextDocument::contentsChange, this,
            [this](int from, int charsRemoved, int charsAdded) {
                Q_UNUSED(charsAdded)
                if (charsRemoved > 0 && from < inputStart_)
                    inputStart_ = qMax(from, inputStart_ - charsRemoved);
            });

    showStartupHint();
}

TerminalPanel::~TerminalPanel()
{
    if (shell_->state() != QProcess::NotRunning) {
        shell_->kill();
        shell_->waitForFinished(1000);
    }
}

bool TerminalPanel::isRunning() const
{
    return shell_->state() == QProcess::Running;
}

void TerminalPanel::showStartupHint()
{
    clear();
    inputStart_ = 0;
    appendText(QStringLiteral("Terminal ready.\n"));
    appendText(QStringLiteral("Type a command and press Enter. Ctrl+C interrupts the running program.\n\n"));
    inputStart_ = document()->characterCount() - 1;
}

void TerminalPanel::startShell(const QString &workingDirectory)
{
    if (isRunning())
        return;

    workingDirectory_ = workingDirectory.isEmpty() ? QDir::homePath() : workingDirectory;
    pendingOutput_.clear();
    awaitingOutput_ = false;
    historyIndex_ = -1;

    clear();
    inputStart_ = 0;

    shell_->setWorkingDirectory(workingDirectory_);
    shell_->start(QStringLiteral("bash"),
                  {QStringLiteral("--norc"), QStringLiteral("--noprofile")});
}

void TerminalPanel::setWorkingDirectory(const QString &path)
{
    workingDirectory_ = path;

    if (!isRunning()) {
        startShell(path);
        return;
    }

    runCommand(QStringLiteral("cd -- ") + shellQuote(path));
}

void TerminalPanel::onStarted()
{
    appendText(QStringLiteral("Shell started in %1\n\n").arg(workingDirectory_));
    awaitingOutput_ = false;
    printPrompt(workingDirectory_);
}

void TerminalPanel::onErrorOccurred(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        appendText(QStringLiteral("Failed to start bash. Check that bash is installed.\n"));
        inputStart_ = document()->characterCount() - 1;
    }
}

void TerminalPanel::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!pendingOutput_.isEmpty()) {
        appendText(pendingOutput_);
        pendingOutput_.clear();
    }

    awaitingOutput_ = false;

    if (status == QProcess::CrashExit)
        appendText(QStringLiteral("\n[shell terminated]\n"));
    else
        appendText(QStringLiteral("\n[shell exited with code %1]\n").arg(exitCode));

    appendText(QStringLiteral("Press Enter to start a new shell.\n"));
    inputStart_ = document()->characterCount() - 1;
}

void TerminalPanel::onReadyRead()
{
    pendingOutput_ += QString::fromUtf8(shell_->readAllStandardOutput());
    processPending();
}

void TerminalPanel::processPending()
{
    const int start = pendingOutput_.indexOf(kPromptStart);
    if (start < 0) {
        if (!awaitingOutput_) {
            appendText(pendingOutput_);
            pendingOutput_.clear();
            return;
        }
        if (pendingOutput_.endsWith(kPromptStart)) {
            appendText(pendingOutput_.chopped(1));
            pendingOutput_ = kPromptStart;
        } else {
            appendText(pendingOutput_);
            pendingOutput_.clear();
        }
        return;
    }

    appendText(pendingOutput_.left(start));
    pendingOutput_ = pendingOutput_.mid(start);

    const int end = pendingOutput_.indexOf(kPromptEnd);
    if (end < 0)
        return;

    const QString cwd = pendingOutput_.mid(1, end - 1);
    pendingOutput_ = pendingOutput_.mid(end + 1);

    if (!pendingOutput_.isEmpty()) {
        appendText(pendingOutput_);
        pendingOutput_.clear();
    }

    if (!textCursor().block().text().isEmpty())
        appendText(QStringLiteral("\n"));

    awaitingOutput_ = false;
    printPrompt(cwd.isEmpty() ? workingDirectory_ : cwd);
}

void TerminalPanel::printPrompt(const QString &cwd)
{
    workingDirectory_ = cwd;

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(QColor(Theme::color1));
    cursor.insertText(QDir(cwd).dirName() + QStringLiteral(" $ "), format);

    inputStart_ = cursor.position();
    setTextCursor(cursor);
    ensureCursorVisible();
}

void TerminalPanel::runCommand(const QString &line)
{
    if (!isRunning())
        return;

    awaitingOutput_ = true;
    historyIndex_ = -1;

    QByteArray payload = line.toUtf8();
    payload += '\n';
    payload += "printf '\\001%s\\002' \"$PWD\"\n";
    shell_->write(payload);
}

void TerminalPanel::submitLine()
{
    const QString line = currentInput();

    appendText(QStringLiteral("\n"));
    inputStart_ = document()->characterCount() - 1;

    if (line.trimmed().isEmpty()) {
        awaitingOutput_ = false;
        printPrompt(workingDirectory_);
        return;
    }

    history_.append(line);
    if (history_.size() > kMaxHistory)
        history_.removeFirst();

    runCommand(line);
}

void TerminalPanel::sendInterrupt()
{
    if (!isRunning())
        return;

    const pid_t bashPid = static_cast<pid_t>(shell_->processId());
    signalGroupExceptBash(bashPid, bashPid);
}

void TerminalPanel::appendText(const QString &text)
{
    if (text.isEmpty())
        return;

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(AnsiFilter::strip(text));
    setTextCursor(cursor);
    ensureCursorVisible();
}

QString TerminalPanel::currentInput() const
{
    return document()->toPlainText().mid(inputStart_);
}

void TerminalPanel::setInput(const QString &text)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(inputStart_);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.insertText(text);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void TerminalPanel::insertInput(const QString &text)
{
    QTextCursor cursor = textCursor();
    if (cursor.position() < inputStart_) {
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
    }
    cursor.insertText(text);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void TerminalPanel::backspace()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        if (cursor.selectionStart() < inputStart_) {
            cursor.clearSelection();
            setTextCursor(cursor);
            return;
        }
        cursor.removeSelectedText();
        setTextCursor(cursor);
        return;
    }
    if (cursor.position() <= inputStart_)
        return;
    cursor.deletePreviousChar();
    setTextCursor(cursor);
}

void TerminalPanel::deleteForward()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        if (cursor.selectionStart() < inputStart_) {
            cursor.clearSelection();
            setTextCursor(cursor);
            return;
        }
        cursor.removeSelectedText();
        setTextCursor(cursor);
        return;
    }
    if (cursor.position() < inputStart_)
        return;
    cursor.deleteChar();
    setTextCursor(cursor);
}

void TerminalPanel::clampCursor()
{
    QTextCursor cursor = textCursor();
    if (cursor.position() < inputStart_) {
        cursor.setPosition(inputStart_);
        setTextCursor(cursor);
    }
}

void TerminalPanel::historyPrev()
{
    if (history_.isEmpty())
        return;

    if (historyIndex_ < 0)
        historyIndex_ = history_.size();

    if (historyIndex_ > 0)
        --historyIndex_;

    setInput(history_.at(historyIndex_));
}

void TerminalPanel::historyNext()
{
    if (historyIndex_ < 0)
        return;

    if (historyIndex_ < history_.size() - 1) {
        ++historyIndex_;
        setInput(history_.at(historyIndex_));
        return;
    }

    historyIndex_ = -1;
    setInput(QString());
}

void TerminalPanel::keyPressEvent(QKeyEvent *event)
{
    if (!isRunning()) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            startShell(workingDirectory_);
        return;
    }

    const int key = event->key();
    const Qt::KeyboardModifiers modifiers = event->modifiers();

    if (awaitingOutput_) {
        if (key == Qt::Key_C && (modifiers & Qt::ControlModifier)
            && !(modifiers & Qt::ShiftModifier))
            sendInterrupt();
        return;
    }

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        submitLine();
        return;
    }

    if (modifiers & Qt::ControlModifier) {
        switch (key) {
        case Qt::Key_C:
            if (textCursor().hasSelection())
                copy();
            else
                sendInterrupt();
            return;
        case Qt::Key_V:
            insertInput(QApplication::clipboard()->text());
            return;
        case Qt::Key_L: {
            clear();
            inputStart_ = 0;
            printPrompt(workingDirectory_);
            return;
        }
        case Qt::Key_A: {
            QTextCursor cursor = textCursor();
            cursor.setPosition(inputStart_);
            setTextCursor(cursor);
            return;
        }
        case Qt::Key_E: {
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End);
            setTextCursor(cursor);
            return;
        }
        case Qt::Key_Up:
            historyPrev();
            return;
        case Qt::Key_Down:
            historyNext();
            return;
        default:
            return;
        }
    }

    if (modifiers & (Qt::AltModifier | Qt::MetaModifier)) {
        QPlainTextEdit::keyPressEvent(event);
        clampCursor();
        return;
    }

    switch (key) {
    case Qt::Key_Backspace:
        backspace();
        return;
    case Qt::Key_Delete:
        deleteForward();
        return;
    case Qt::Key_Up:
        historyPrev();
        return;
    case Qt::Key_Down:
        historyNext();
        return;
    default:
        break;
    }

    if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
        insertInput(event->text());
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
    clampCursor();
}

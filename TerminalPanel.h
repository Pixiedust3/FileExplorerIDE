#ifndef TERMINALPANEL_H
#define TERMINALPANEL_H

#include <QPlainTextEdit>
#include <QProcess>
#include <QStringList>

class TerminalPanel : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget *parent = nullptr);
    ~TerminalPanel() override;

    void startShell(const QString &workingDirectory);
    void setWorkingDirectory(const QString &path);
    bool isRunning() const;

signals:
    void statusMessage(const QString &msg, int timeoutMs);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onStarted();
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    void appendText(const QString &text);
    void printPrompt(const QString &cwd);
    void runCommand(const QString &line);
    void submitLine();
    void sendInterrupt();
    void insertInput(const QString &text);
    void backspace();
    void deleteForward();
    void clampCursor();
    void historyPrev();
    void historyNext();
    QString currentInput() const;
    void setInput(const QString &text);
    void processPending();
    void showStartupHint();

    QProcess *shell_;
    QString workingDirectory_;
    QString pendingOutput_;
    QStringList history_;
    int historyIndex_ = -1;
    int inputStart_ = 0;
    bool awaitingOutput_ = false;
};

#endif // TERMINALPANEL_H

#include "AnsiFilter.h"

#include <QRegularExpression>

QString AnsiFilter::strip(const QString &text)
{
    static const QRegularExpression escapeRe(
        QStringLiteral("\\x1b\\][^\\x07\\x1b]*(?:\\x07|\\x1b\\\\)")
        + QStringLiteral("|\\x1b\\[[0-9;?]*[ -/]*[@-~]")
        + QStringLiteral("|\\x1b[@-Z\\\\-_]"));

    QString result = text;
    result.remove(escapeRe);
    result.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    result.replace(QChar('\r'), QChar('\n'));
    return result;
}

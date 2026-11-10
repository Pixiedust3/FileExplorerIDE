#ifndef ANSIFILTER_H
#define ANSIFILTER_H

#include <QString>

class AnsiFilter
{
public:
    static QString strip(const QString &text);
};

#endif // ANSIFILTER_H

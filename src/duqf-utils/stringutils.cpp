#include "stringutils.h"

QString StringUtils::toTitleCase(const QString &str) {
    QLocale locale;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList parts = str.split(QRegularExpression("[_ ]+"), Qt::SkipEmptyParts);
#else
    QStringList parts = str.split(QRegularExpression("[_ ]+"), QString::SkipEmptyParts);
#endif
    for (int i = 0; i < parts.size(); ++i)
        parts[i].replace(0, 1, locale.toUpper(parts[i].at(0)) );

    return parts.join(" ");
}

QString StringUtils::toSnakeCase(const QString &str)
{
    QString snake = str;
    snake.replace(" ", "_");
    QRegularExpression reCamel("([A-Z]+)");
    snake.replace(reCamel, "_\\1");
    return snake.toLower();
}

QString StringUtils::capitalize(QString str)
{
    QLocale locale;
    if (str.isEmpty()) return str;

    return str.replace(0, 1, locale.toUpper(str.at(0)) );
}

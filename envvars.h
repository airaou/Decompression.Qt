#ifndef ENVVARS_H
#define ENVVARS_H

#include <QMap>
#include <QString>

class EnvVars : public QMap<QString, QString> {
public:
    using QMap<QString, QString>::QMap;
    QString parse(QString const& str) const;
    QString parse(QString const& str, EnvVars const& defaultenv) const;

protected:
    static constexpr const char* var_syntax = "\\$\\(([^)]+)\\)";
};

#endif // ENVVARS_H

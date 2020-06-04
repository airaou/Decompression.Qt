#ifndef EXTRACTER_H
#define EXTRACTER_H

#include <QJsonObject>
#include <QStringList>
#include <QRegExp>
#include <optional>
#include "envvars.h"

class Extracter {
public:
    Extracter(QJsonObject const& jobj, EnvVars const& variables);
    bool canExtract(QString const& ext_name);
    bool extract(QString const& infilepath, QString const& outdirpath, QString &errormsg);

private:
    EnvVars const& variables;
    QStringList ext_names;
    QString program;
    QStringList args;
    std::optional<QStringList> args_psw;
    std::optional<int> succeed_errorlevel;
    std::optional<QRegExp> need_psw;
    std::optional<QRegExp> archive_error;
};

#endif // EXTRACTER_H

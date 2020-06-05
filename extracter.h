#ifndef EXTRACTER_H
#define EXTRACTER_H

#include <QJsonObject>
#include <QStringList>
#include <QRegExp>
#include <optional>
#include "envvars.h"

#include <QObject>

class Extracter {
public:
    enum ErrorCode {
        SUCCEED,
        PGM_ERROR,
        PSW_ERROR,
        ARCH_ERROR,
        EXITCODE_INCORRECT,
    };

public:
    Extracter(QJsonObject const& jobj, EnvVars const& variables);
    QJsonObject export_extracts() const;

    bool canExtract(QString ext_name) const;
    ErrorCode extract(QString infilepath,
                      QString outdirpath,
                      QSet<QString> const& psws,
                      bool &quick_exit,
                      QString &errmsg) const;

private:
    EnvVars const& variables;
    QStringList ext_names;
    QString program;
    QStringList args_template;
    std::optional<int> succeed_errorlevel;
    std::optional<QRegExp> psw_error;
    std::optional<QRegExp> archive_error;
};

#endif // EXTRACTER_H

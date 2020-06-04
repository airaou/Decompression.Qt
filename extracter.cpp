#include "extracter.h"
#include <QJsonValue>
#include <QJsonArray>
#include <QProcess>
#include <QFile>

Extracter::Extracter(QJsonObject const& jobj, EnvVars const& variables)
    : variables(variables)
    , args_psw(std::nullopt)
    , succeed_errorlevel(std::nullopt)
    , need_psw(std::nullopt)
    , archive_error(std::nullopt)
{
    QJsonValue jval;

    jval = jobj["ext_names"];
    if(jval.isArray())
        for(auto ext_name : jval.toArray())
            if(ext_name.isString())
                ext_names.append(ext_name.toString());

    jval = jobj["program"];
    if(jval.isString())
        program = jval.toString();

    jval = jobj["args"];
    if(jval.isArray())
        for(auto arg : jval.toArray())
            if(arg.isString())
                args.append(arg.toString());

    jval = jobj["args_psw"];
    if(jval.isArray()) {
        args_psw = QStringList();
        for(auto arg : jval.toArray())
            if(arg.isString())
                args_psw->append(arg.toString());
    }

    jval = jobj["succeed_errorlevel"];
    if(jval.isDouble())
        succeed_errorlevel = jval.toInt();

    jval = jobj["error_detect"];
    if(jval.isObject()) {
        QJsonObject jerrs = jval.toObject();

        jval = jerrs["need_psw"];
        if(jval.isString())
            need_psw = QRegExp(jval.toString());

        jval = jerrs["archive_error"];
        if(jval.isString())
            archive_error = QRegExp(jval.toString());
    }
}

bool Extracter::canExtract(QString const& ext_name) {
    return ext_names.contains(ext_name);
}

bool Extracter::extract(const QString &infilepath,
                        const QString &outdirpath,
                        QString &errormsg) {
    QProcess process;
    EnvVars tmpenv;
    tmpenv["infile"] = infilepath;
    tmpenv["outdir"] = outdirpath;
    QString pgm = variables.parse(program, tmpenv);
    QStringList realargs;
    for(auto arg : args) {
        realargs << variables.parse(arg, tmpenv);
    }
    process.start(pgm, realargs, QFile::ReadOnly);
    process.waitForFinished();
    QByteArray cmd_output1 = process.readAll();
    return false;
}

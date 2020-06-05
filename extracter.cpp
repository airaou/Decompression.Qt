#include "extracter.h"
#include <QJsonValue>
#include <QJsonArray>
#include <QProcess>
#include <QFile>
#include <QDebug>
#include <QFileInfo>

Extracter::Extracter(QJsonObject const& jobj, EnvVars const& variables)
    : variables(variables)
    , succeed_errorlevel(std::nullopt)
    , psw_error(std::nullopt)
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
                args_template.append(arg.toString());

    jval = jobj["succeed_errorlevel"];
    if(jval.isDouble())
        succeed_errorlevel = jval.toInt();

    jval = jobj["error_detect"];
    if(jval.isObject()) {
        QJsonObject jerrs = jval.toObject();

        jval = jerrs["psw_error"];
        if(jval.isString())
            psw_error = QRegExp(jval.toString());

        jval = jerrs["archive_error"];
        if(jval.isString())
            archive_error = QRegExp(jval.toString());
    }
}

QJsonObject Extracter::export_extracts() const {
    QJsonObject jobj;
    {
        QJsonArray jext_names;
        for(auto n : ext_names) {
            jext_names.append(n);
        }
        jobj["ext_names"] = jext_names;
    }
    jobj["program"] = program;
    {
        QJsonArray jargs;
        for(auto n : args_template) {
            jargs.append(n);
        }
        jobj["args"] = jargs;
    }
    if(succeed_errorlevel.has_value()) {
        jobj["succeed_errorlevel"] = *succeed_errorlevel;
    }
    {
        QJsonObject jdet;
        if(psw_error.has_value()) {
            jdet["psw_error"] = psw_error->pattern();
        }
        if(archive_error.has_value()) {
            jdet["archive_error"] = archive_error->pattern();
        }
        if(jdet.size() > 0) {
            jobj["error_detect"] = jdet;
        }
    }
    return jobj;
}

bool Extracter::canExtract(QString ext_name) const {
    return ext_names.contains(ext_name);
}

Extracter::ErrorCode Extracter::extract(QString infilepath,
                                        QString outdirpath,
                                        const QSet<QString> &psws,
                                        QString &errmsg) const {

    EnvVars tmpenv;
    tmpenv["infile"] = infilepath;
    tmpenv["outdir"] = outdirpath;

    for(auto psw : psws) {
        tmpenv["psw"] = psw;
        QString pgm = variables.parse(program, tmpenv);
        {
            QFileInfo pgminfo(pgm);
            if(!pgminfo.exists()) {
                errmsg = QObject::tr("%1 is not an excutable program.").arg(pgm);
                return Extracter::PGM_ERROR;
            }
        }
        QStringList args;
        for(auto arg : args_template) {
            args << variables.parse(arg, tmpenv);
        }

        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(pgm, args);
        QString out;
        while(!process.waitForFinished(1)) {
            QString tmpout = QString::fromLocal8Bit(process.readAll());
            if(tmpout.size()) {
                qDebug().noquote() << tmpout;
                out += tmpout;
            }
        }

        if(psw_error.has_value()) {
            if(out.indexOf(*psw_error) != -1) {
                continue;
            }
        }

        if(archive_error.has_value()) {
            if(out.indexOf(*archive_error) != -1) {
                errmsg = QObject::tr("Archive error");
                return Extracter::ARCH_ERROR;
            }
        }

        if(succeed_errorlevel.has_value()) {
            if(process.exitCode() != *succeed_errorlevel) {
                errmsg = QObject::tr("Exit code incorrect");
                return Extracter::EXITCODE_INCORRECT;
            }
        }

        return Extracter::SUCCEED;
    }

    errmsg = QObject::tr("Password error");
    return Extracter::PSW_ERROR;
}

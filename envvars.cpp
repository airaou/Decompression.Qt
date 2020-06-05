#include "envvars.h"
#include <QRegExp>
#include <QStringBuilder>

QString EnvVars::parse(const QString &str, EnvVars const& defaultenv) const {
    QRegExp varreg(EnvVars::var_syntax);
    int pstidx = 0;
    int curidx = 0;
    QString outstr;
    while((curidx = str.indexOf(varreg, curidx)) != -1) {
        outstr += str.mid(pstidx, curidx);
        QString name = varreg.cap(1);
        QString value = this->value(name, "");
        if(value.size() == 0) {
            value = defaultenv.value(name, "");
        }
        outstr += value;
        pstidx = varreg.pos() + varreg.cap().size();
        curidx++;
    }
    outstr += str.mid(pstidx);
    return outstr;
}

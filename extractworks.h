#ifndef EXTRACTWORKS_H
#define EXTRACTWORKS_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QFileInfo>
#include <QDir>
#include "extracter.h"
#include "inputfiles.h"
#include "filedetector.h"

class ExtractWorks : public QThread {
    Q_OBJECT
public:
    using QThread::QThread;

signals:
    void message(QString msg);

protected:
    void run() override;

    FileDetector *head_pattern;
    InputFiles *infiles;
    bool mksubdir;
    QDir defoutdir;
    QSet<QString> passwords;
    QRegExp filename_psw_pattern;

    QList<Extracter> extrs;
};

#endif // EXTRACTWORKS_H

#ifndef FILEDETECTOR_H
#define FILEDETECTOR_H

#include <QByteArray>
#include <QMap>

class FileDetector : public QMap<QByteArray, QString> {
public:
    using QMap<QByteArray, QString>::QMap;

    bool detect(QString filepath, QString &ext_name);

private:
    int head_pattern_maxlen();
};

#endif // FILEDETECTOR_H

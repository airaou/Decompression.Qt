#ifndef FILEDETECTOR_H
#define FILEDETECTOR_H

#include <QByteArray>
#include <QMap>

class FileDetector {

public:
    FileDetector();

    void append(QByteArray head_bytes, QString ext_name);
    bool detect(QString filepath, QString &ext_name);

private:
    int head_pattern_maxlen();
    QMap<QByteArray, QString> head_pattern;
};

#endif // FILEDETECTOR_H

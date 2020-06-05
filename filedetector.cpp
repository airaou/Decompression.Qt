#include "filedetector.h"
#include <QFile>

bool FileDetector::detect(QString filepath, QString &ext_name) {
    QFile file(filepath);
    if(!file.exists()) {
        return false;
    }
    if(!file.open(QFile::ReadOnly)) {
        return false;
    }
    QByteArray head = file.read(head_pattern_maxlen());
    file.close();
    for(auto i : keys()) {
        if(i == head.mid(0, i.size())) {
            ext_name = this->value(i);
            return true;
        }
    }
    return false;
}

int FileDetector::head_pattern_maxlen() {
    int maxlen = 0;
    for(auto i : keys()) {
        if(i.size() > maxlen) {
            maxlen = i.size();
        }
    }
    return maxlen;
}

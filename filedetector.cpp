#include "filedetector.h"
#include <QFile>

FileDetector::FileDetector()
    : head_pattern()
{
}

void FileDetector::append(QByteArray head_bytes, QString ext_name) {
    head_pattern[head_bytes] = ext_name;
}

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
    for(auto i : head_pattern.keys()) {
        if(i == head.mid(0, i.size())) {
            ext_name = head_pattern[i];
            return true;
        }
    }
    return false;
}

int FileDetector::head_pattern_maxlen() {
    int maxlen = 0;
    for(auto i : head_pattern.keys()) {
        if(i.size() > maxlen) {
            maxlen = i.size();
        }
    }
    return maxlen;
}

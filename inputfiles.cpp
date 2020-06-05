#include "inputfiles.h"

InputFiles::InputFiles()
    : QList<InputFile>()
    , listWidget(nullptr)
{

}

void InputFiles::load_icons(const QJsonObject &iconpathobj) {
    QString ok_path     = iconpathobj["ok"]    .toString("./ok.png"    );
    QString pswerr_path = iconpathobj["pswerr"].toString("./pswerr.png");
    QString deferr_path = iconpathobj["deferr"].toString("./deferr.png");
    QString ready_path  = iconpathobj["ready"] .toString("./ready.png" );

    QFileInfo finfo;
    if((finfo = ok_path    ).isFile()) ok_icon      = QIcon(ok_path     );
    if((finfo = pswerr_path).isFile()) pswerr_icon  = QIcon(pswerr_path );
    if((finfo = deferr_path).isFile()) deferr_icon  = QIcon(deferr_path);
    if((finfo = ready_path ).isFile()) waiting_icon = QIcon(ready_path  );
}

bool InputFiles::contains(QFileInfo fileinfo) const {
    for(auto const& i : *this) {
        if(i.info == fileinfo) {
            return true;
        }
    }
    return false;
}

void InputFiles::append(QFileInfo const& fileinfo) {
    InputFile infile {
        fileinfo,
        InputFile::WAITING
    };
    QList::append(infile);
    if(listWidget) {
        auto item = new QListWidgetItem(waiting_icon, fileinfo.filePath(), listWidget);
        listWidget->addItem(item);
    }
}

void InputFiles::append(const QList<QFileInfo> &fileinfos) {
    for(auto const& f : fileinfos) {
        append(f);
    }
}

void InputFiles::clear() {
    QList::clear();
    if(listWidget) {
        listWidget->clear();
    }
}

bool InputFiles::removeOne(QString const& t) {
    QFileInfo info(t);
    int i = 0;
    for(auto const& d : *this) {
        if(d.info == info) {
            break;
        }
        i += 1;
    }
    if(i < size()) {
        removeAt(i);
        return true;
    }
    return false;
}

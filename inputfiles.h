#ifndef INPUTFILES_H
#define INPUTFILES_H

#include <QList>
#include <QFileInfo>
#include <QListWidget>
#include <QListWidgetItem>
#include <QJsonObject>

struct InputFile {
    QFileInfo info;
    enum State {
        WAITING,
        PSWERR,
        UNKNOWNERR,
        FINISHED,
    } state;
};

class InputFiles : protected QList<InputFile> {
public:
    InputFiles();
    void load_icons(QJsonObject const& iconpathobj);

    bool contains(QFileInfo fileinfo) const;
    void append(QFileInfo const& fileinfo);
    void append(QList<QFileInfo> const& fileinfos);
    void push_back(QFileInfo const& fileinfo) { append(fileinfo); }
    bool removeOne(QString const& t);
    void clear();

    using QList<InputFile>::operator[];
    using QList<InputFile>::begin;
    using QList<InputFile>::end;
    using QList<InputFile>::size;
    using QList<InputFile>::back;
    using QList<InputFile>::front;

    class iterator : public QList<InputFile>::iterator {
    public:
        QListWidgetItem *item;
        using QList<InputFile>::iterator::iterator;
    };

public:
    QListWidget *listWidget;
    QIcon ok_icon;
    QIcon pswerr_icon;
    QIcon deferr_icon;
    QIcon ready_icon;
    QIcon waiting_icon;
};

#endif // INPUTFILES_H

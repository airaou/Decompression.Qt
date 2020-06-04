#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QTranslator>
#include <QRegExp>
#include <QList>
#include <QDir>
#include "extracter.h"
#include "filedetector.h"
#include "envvars.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private slots:
    void on_startButton_clicked();
    void on_clearButton_clicked();

    void on_actionAbout_triggered();
    void on_actionHelp_triggered();
    void on_actionLoadQm_triggered();

    void on_infilesList_itemDoubleClicked(QListWidgetItem *item);

protected:
    void set_qmfile(QString filepath);
    void load_config(QString filepath);

    void append_passwords(QJsonArray const& psws);
    void append_variables(QJsonObject const& vars);
    void append_extract_pgmname(QJsonArray const& names);

    bool load_jsonfile(QString filepath, QJsonObject &jobj);

private:
    Ui::MainWindow *ui;
    QTranslator *translator;

    QDir outdir;
    QList<QFileInfo> infiles;
    QStringList passwords;
    EnvVars variables;
    QStringList extract_pgmname;
    QRegExp filename_psw_pattern;

    QList<Extracter> extrs;
    FileDetector detector;
};
#endif // MAINWINDOW_H

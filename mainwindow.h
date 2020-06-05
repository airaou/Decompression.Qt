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
#include "inputfiles.h"

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
    void on_actionLoadCfg_triggered();
    void on_actionSaveCfg_triggered();
    void on_actionExit_triggered();
    void on_actionDbgout_triggered();

    void on_pswEdit_returnPressed();
    void on_pswDelButton_clicked();
    void on_archDelButton_clicked();

public slots:
    void save_config();

protected:
    void set_qmfile(QString filepath);
    void import_config(QString filepath);
    void export_config(QString filepath);

    void append_passwords(QJsonArray const& psws);
    QJsonArray export_passwords();
    void append_variables(QJsonObject const& vars);
    QJsonObject export_variables();
    void append_extract_pgmnames(QJsonArray const& names);
    QJsonArray export_extract_pgmnames();

    bool load_jsonfile(QString filepath, QJsonObject &jobj);

private:
    Ui::MainWindow *ui;
    QJsonObject jcfgobj;

    QTranslator *translator;

    QDir defoutdir;
    InputFiles infiles;
    QSet<QString> passwords;
    EnvVars variables;
    QStringList extract_pgmname;
    QRegExp filename_psw_pattern;

    QList<Extracter> extrs;
    FileDetector head_pattern;
};
#endif // MAINWINDOW_H

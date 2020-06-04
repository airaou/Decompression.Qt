#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QTranslator>
#include <QProcess>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>
#include <QJsonArray>
#include <QRegExp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , translator(nullptr)
    , outdir(".")
    , infiles()
    , filename_psw_pattern("<(.*)>")
{
    ui->setupUi(this);
    qDebug() << "outdir: " << outdir.absolutePath();
    ui->outdirEdit->setText(outdir.absolutePath());

    set_qmfile("./lang_zh-CN.qm");
    load_config("./extractcfg.json");
    ui->statusbar->showMessage(tr("Ready."));
}

MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::set_qmfile(QString filepath) {
    ui->statusbar->showMessage(tr("Loading language file."));
    QTranslator *newtranslator = new QTranslator();
    if(!newtranslator->load(filepath)) {
        delete newtranslator;
        QMessageBox::information(nullptr,
                                 tr("Error"),
                                 tr("Can not find %1.").arg(filepath));
        return;
    }
    if(translator) {
        qApp->removeTranslator(translator);
        delete translator;
        translator = nullptr;
    }
    translator = newtranslator;
    qApp->installTranslator(translator);
    ui->retranslateUi(this);
}

void MainWindow::load_config(QString filepath) {
    ui->statusbar->showMessage(tr("Loading config file."));
    QJsonObject jobj;
    if(!load_jsonfile(filepath, jobj)) {
        return;
    }
    QJsonValue jvalue;

    jvalue = jobj["passwords"];
    if(jvalue.isArray())
        append_passwords(jvalue.toArray());

    jvalue = jobj["variables"];
    if(jvalue.isObject())
        append_variables(jvalue.toObject());

    jvalue = jobj["extract_pgmname"];
    if(jvalue.isArray())
        append_extract_pgmname(jvalue.toArray());

    jvalue = jobj["filename_psw_pattern"];
    if(jvalue.isString())
        filename_psw_pattern = QRegExp(jvalue.toString());

    jvalue = jobj["extracts"];
    if(jvalue.isArray())
        for(auto extr : jvalue.toArray())
            if(extr.isObject())
                extrs.append(Extracter(extr.toObject(), variables));

    jvalue = jobj["head_pattern"];
    if(jvalue.isArray())
        for(auto ptn : jvalue.toArray())
            if(ptn.isObject()) {
                QJsonObject ptnobj = ptn.toObject();
                QString ext_name = ptnobj["ext_name"].toString();
                QByteArray head = ptnobj["head"].toString().toLatin1();
                detector.append(head, ext_name);
            }
}

void MainWindow::append_passwords(QJsonArray const& psws) {
    ui->statusbar->showMessage(tr("Loading passwords."));
    for(auto psw : psws) {
        if(psw.isString()) {
            passwords.append(psw.toString());
            ui->pswList->addItem(passwords.back());
        }
    }
}

void MainWindow::append_variables(QJsonObject const& vars) {
    ui->statusbar->showMessage(tr("Loading variables."));
    for(auto key : vars.keys()) {
        auto value = vars[key];
        if(value.isString()) {
            variables[key] = value.toString();
        }
    }
}

void MainWindow::append_extract_pgmname(const QJsonArray &names) {
    ui->statusbar->showMessage(tr("Loading name of programs."));
    for(auto name : names) {
        if(name.isString()) {
            extract_pgmname.append(name.toString());
        }
    }
}

bool MainWindow::load_jsonfile(QString filepath, QJsonObject &jobj) {
    ui->statusbar->showMessage(tr("Loading file %1.").arg(filepath));
    QFile file(filepath);
    if(!file.exists()) {
        QMessageBox::information(nullptr,
                                 tr("Error"),
                                 tr("Can not find %1.").arg(filepath));
        return false;
    }
    if(!file.open(QFile::ReadOnly)) {
        QMessageBox::information(nullptr,
                                 tr("Error"),
                                 tr("Can not open %1.").arg(filepath));
        return false;
    }
    QByteArray bindata = file.readAll();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(bindata, &err);
    if(err.error != QJsonParseError::NoError) {
        QString ctx;
        file.seek(qMax(err.offset - 30, 0));
        QByteArray binline1 = file.read(30);
        QByteArray binline2 = file.read(30);
        QMessageBox::information(nullptr,
                                 tr("Error"),
                                 tr("Json syntax error.\n"
                                    "File: %1.\n"
                                    "Offset: %2.\n"
                                    "The context is shown below:\n\n")
                                 .arg(filepath).arg(err.offset) +
                                 binline1 + tr("(<- about here)") + binline2);
        return false;
    }
    if(!doc.isObject()) {
        QMessageBox::information(nullptr,
                                 tr("Error"),
                                 tr("Root should be an object."));
        return false;
    }
    jobj = doc.object();
    return true;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    qDebug() << "DragEnter";
    auto mimeData = e->mimeData();
    if(mimeData->hasUrls()) {
        for(auto url : mimeData->urls()) {
            if(url.isLocalFile()) {
                e->acceptProposedAction();
                QString msg;
                int nums = mimeData->urls().size();
                if(nums > 1) {
                    msg += tr("Found %1 files.").arg(nums) + "\n";
                }
                msg += tr("Release to append the file or set the output folder.");
                ui->statusbar->showMessage(msg);
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent *e) {
    qDebug() << "dropEvent";
    bool hadSucceed = false;
    if(e->mimeData()->hasUrls()) {
        auto urls = e->mimeData()->urls();
        for(auto url : urls) {
            if(url.isLocalFile()) {
                QFileInfo path(url.toLocalFile());
                if(path.isDir()) {
                    qDebug() << "dir: " << path.filePath();
                    outdir = QDir(path.absoluteFilePath());
                } else {
                    if(extract_pgmname.contains(path.fileName())) {
                        qDebug() << "pgm: " << path.filePath();
                        variables[path.fileName()] = path.absoluteFilePath();
                        ui->statusbar->showMessage(tr("set '%1' = '%2'.").arg(path.fileName(), path.absoluteFilePath()));
                    } else {
                        qDebug() << "file: " << path.filePath();
                        if(!infiles.contains(path.filePath())) {
                            QString ext_name;
                            if(detector.detect(path.filePath(), ext_name)) {
                                qDebug() << "ext: " << ext_name;
                            }
                            infiles.push_back(path.filePath());
                            ui->infilesList->addItem(path.filePath());
                        }
                    }
                }
                hadSucceed = true;
            }
        }
        ui->outdirEdit->setText(outdir.absolutePath());
    }
    if(!hadSucceed) {
        QMessageBox::information(this, tr("Error"), tr("Please provide a local file or a local folder."));
    }
}


void MainWindow::on_startButton_clicked() {
    qDebug() << "start";
    if(infiles.size() == 0) {
        ui->statusbar->showMessage(tr("No archive to extract."));
        return;
    }
    for(auto infile : infiles) {

        outdir.mkdir(infile.fileName());
        qDebug() << "infile: " << infile.absoluteFilePath();
        qDebug() << "outdir: " << outdir.absoluteFilePath(infile.fileName());

        if(QString ext_name; detector.detect(infile.absoluteFilePath(), ext_name)) {

            bool succeed = false;
            for(auto extr : extrs) {
                if(extr.canExtract(ext_name)) {
                    QString errmsg;
                    if(!extr.extract(infile.absoluteFilePath(), outdir.absoluteFilePath(infile.fileName()), errmsg)) {
                        qDebug() << "failed: " << infile.absoluteFilePath();
                    } else {
                        succeed = true;
                    }
                    break;
                }
            }

        }

        outdir.cd("..");
    }
}

void MainWindow::on_clearButton_clicked() {
    infiles.clear();
    ui->infilesList->clear();
}

void MainWindow::on_actionAbout_triggered() {
    qDebug() << "About";
    QMessageBox::information(this, tr("About"), tr("ABOUT_CONTENT"));
}

void MainWindow::on_actionHelp_triggered() {
    qDebug() << "Help";
    QMessageBox::information(this, tr("Help"), tr("HELP_CONTENT"));
}

void MainWindow::on_infilesList_itemDoubleClicked(QListWidgetItem *item) {
    qDebug() << "itemDoubleClicked";
}

void MainWindow::on_actionLoadQm_triggered() {
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Please choose a qm language file."));
    dialog.setDirectory(".");
    dialog.setNameFilter("File(*.qm)");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::ViewMode::Detail);
    if(!dialog.exec()) {
        return;
    }
    QString file = dialog.selectedFiles().first();
    set_qmfile(file);
}

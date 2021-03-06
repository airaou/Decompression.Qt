﻿#include "mainwindow.h"
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
#include <QMutex>

static MainWindow* single = nullptr;
static QMutex single_mutex;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , translator(nullptr)
    , infiles()
    , thread_quick_exit(false)
    , thread_finished(true)
{
    ui->setupUi(this);

    ui->dbgGroupBox->setVisible(false);
    ui->line->setVisible(false);

    infiles.listWidget = ui->infilesList;
    set_qmfile("./lang_zh-CN.qm");
    import_config("./extractcfg.json");
    ui->statusbar->showMessage(tr("Ready."));
    single = this;
    qInstallMessageHandler([](QtMsgType type,
                           const QMessageLogContext &ctx,
                           const QString &msg) {
        QMutexLocker locker(&single_mutex);
        if(single) {
            // single->ui->dbgoutEdit->appendPlainText(msg);
            emit single->debugString(msg);
        }
    });
    connect(this, &MainWindow::message, ui->statusbar, &QStatusBar::showMessage);
    connect(this, &MainWindow::debugString, ui->dbgoutEdit, &QPlainTextEdit::appendPlainText);
    connect(this, &MainWindow::workFinished, ui->startButton, &QPushButton::setEnabled);
    connect(this, &MainWindow::workProgress, ui->progressBar, &QProgressBar::setValue);
    connect(this, &MainWindow::changeInfileState, this, &MainWindow::setInfileState);
    connect(this, &MainWindow::workFinished, this, &MainWindow::setWorkFinished);
    qDebug() << "Setup";
}

MainWindow::~MainWindow() {
    thread_quick_exit = true;
    if(t.joinable()) {
        t.join();
    }
    single = nullptr;
    delete ui;
}


void MainWindow::set_qmfile(QString filepath) {
    ui->statusbar->showMessage(tr("Loading language file."));
    QTranslator *newtranslator = new QTranslator();
    if(!newtranslator->load(filepath)) {
        delete newtranslator;
        QMessageBox::warning(this,
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

void MainWindow::import_config(QString filepath) {
    ui->statusbar->showMessage(tr("Loading config file."));
    while(!load_jsonfile(filepath, jcfgobj)) {
        QMessageBox::information(this,
                                 tr("Info"),
                                 tr("Please choose an extractcfg.json config file."));
        QFileDialog dialog(this,
                           tr("Please choose an extractcfg.json config file."),
                           ".",
                           "File(*.json)");
        dialog.setFileMode(QFileDialog::ExistingFile);
        if(dialog.exec()) {
            filepath = dialog.selectedFiles().first();
            continue;
        }
        emit close();
        return;
    }

    QJsonValue jvalue;

    jvalue = jcfgobj["passwords"];
    append_passwords(jvalue.toArray(QJsonArray()));

    jvalue = jcfgobj["variables"];
    append_variables(jvalue.toObject(QJsonObject()));

    jvalue = jcfgobj["extract_pgmnames"];
    append_extract_pgmnames(jvalue.toArray(QJsonArray()));

    filename_psw_pattern = QRegExp(jcfgobj["filename_psw_pattern"].toString("\\{([^}]+)\\}"));

    jvalue = jcfgobj["extracts"];
    for(auto extr : jvalue.toArray(QJsonArray()))
        if(extr.isObject()) {
            extrs.append(Extracter(extr.toObject(), variables));
        }

    {
        QFileInfo outdirinfo(jcfgobj["outdir"].toString("."));
        if(outdirinfo.isDir()) {
            defoutdir = QDir(outdirinfo.filePath());
        } else {
            QMessageBox::warning(this, tr("Warning"), tr("%1 is not a folder.").arg(outdirinfo.filePath()));
        }
        ui->outdirEdit->setText(defoutdir.absolutePath());
    }

    jvalue = jcfgobj["head_pattern"];
    for(auto ptn : jvalue.toArray(QJsonArray()))
        if(ptn.isObject()) {
            QJsonObject ptnobj = ptn.toObject();
            QString ext_name = ptnobj["ext_name"].toString();
            QByteArray head;
            for(auto i : ptnobj["head"].toArray()) {
                head.append(i.toInt());
            }
            head_pattern[head] = ext_name;
        }

    jvalue = jcfgobj["icons"];
    infiles.load_icons(jvalue.toObject(QJsonObject()));
    ui->statusbar->showMessage(tr("Config file loaded."));
}

void MainWindow::export_config(QString filepath) {
    QFile file(filepath);
    if(!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::warning(this,
                             tr("Error"),
                             tr("Save %1 failed.").arg(filepath));
        return;
    }

    ui->statusbar->showMessage(tr("Saving config file."));
    jcfgobj["passwords"] = export_passwords();
    jcfgobj["variables"] = export_variables();
    jcfgobj["extract_pgmnames"] = export_extract_pgmnames();
    jcfgobj["filename_psw_pattern"] = filename_psw_pattern.pattern();
    jcfgobj["outdir"] = defoutdir.path();
    {
        QJsonArray jarr;
        for(auto k : head_pattern.keys()) {
            QJsonObject jptn;
            jptn["ext_name"] = head_pattern[k];
            {
                QJsonArray headarr;
                for(auto i : k) {
                    headarr.append((unsigned char)i);
                }
                jptn["head"] = headarr;
            }

            jarr.append(jptn);
        }
        jcfgobj["head_pattern"] = jarr;
    }
    {
        QJsonArray jarr;
        for(auto const& i : extrs) {
            jarr.append(i.export_extracts());
        }
        jcfgobj["extracts"] = jarr;
    }

    QJsonDocument jdoc(jcfgobj);
    file.write(jdoc.toJson(QJsonDocument::JsonFormat::Indented));
    file.close();
    ui->statusbar->showMessage(tr("Config file saved."));
}

void MainWindow::append_passwords(QJsonArray const& psws) {
    ui->statusbar->showMessage(tr("Loading passwords."));
    for(auto psw : psws) {
        if(psw.isString()) {
            passwords << psw.toString();
            ui->pswList->addItem(psw.toString());
        }
    }
}

QJsonArray MainWindow::export_passwords() {
    QJsonArray jpsws;
    for(auto psw : passwords) {
        jpsws.append(psw);
    }
    return jpsws;
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

QJsonObject MainWindow::export_variables() {
    QJsonObject vars;
    for(auto key : variables.keys()) {
        vars[key] = variables[key];
    }
    return vars;
}

void MainWindow::append_extract_pgmnames(const QJsonArray &names) {
    ui->statusbar->showMessage(tr("Loading name of programs."));
    for(auto name : names) {
        if(name.isString()) {
            extract_pgmname.append(name.toString());
        }
    }
}

QJsonArray MainWindow::export_extract_pgmnames() {
    QJsonArray names;
    for(auto name : extract_pgmname) {
        names.append(name);
    }
    return names;
}

bool MainWindow::load_jsonfile(QString filepath, QJsonObject &jobj) {
    qDebug() << "Loading json file: " << filepath;
    ui->statusbar->showMessage(tr("Loading file %1.").arg(filepath));
    QFile file(filepath);
    if(!file.exists()) {
        QMessageBox::warning(this,
                             tr("Error"),
                             tr("Can not find %1.").arg(filepath));
        return false;
    }
    if(!file.open(QFile::ReadOnly)) {
        QMessageBox::information(this,
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
        QMessageBox::warning(this,
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
        QMessageBox::warning(this,
                             tr("Error"),
                             tr("Root should be an object."));
        return false;
    }
    jobj = doc.object();
    return true;
}


void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    qDebug() << "Drag enter";
    auto mimeData = e->mimeData();
    if(mimeData->hasUrls()) {
        for(auto url : mimeData->urls()) {
            qDebug() << "url: " << url;
        }
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
    qDebug() << "Drop event";
    bool hadSucceed = false;
    if(e->mimeData()->hasUrls()) {
        auto urls = e->mimeData()->urls();
        for(auto url : urls) {
            if(url.isLocalFile()) {
                QFileInfo path(url.toLocalFile());
                if(path.isDir()) {
                    qDebug() << "Load outdir: " << path.filePath();
                    defoutdir = QDir(path.absoluteFilePath());
                } else if(extract_pgmname.contains(path.fileName())) {
                    qDebug() << "Load program: " << path.filePath();
                    variables[path.fileName()] = path.absoluteFilePath();
                    ui->statusbar->showMessage(tr("set '%1' = '%2'.").arg(path.fileName(), path.absoluteFilePath()));
                } else {
                    qDebug() << "Load archive: " << path.filePath();
                    if(!infiles.contains(path)) {
                        QString ext_name;
                        infiles.append(path.filePath());
                        if(head_pattern.detect(path.filePath(), ext_name)) {
                            qDebug() << "Known archvie type: " << ext_name;
                        } else {
                            qDebug() << "Unknow archive type.";
                            infiles.back().state = InputFile::UNKNOWNERR;
                            emit setInfileState(infiles.size() - 1, infiles.deferr_icon, tr("Unknown archive type."));
                        }
                    }
                }

                hadSucceed = true;
            }
        }
        ui->outdirEdit->setText(defoutdir.absolutePath());
    }
    if(!hadSucceed) {
        QMessageBox::warning(this, tr("Error"), tr("Please provide a local file or a local folder."));
    }
}


void MainWindow::on_startButton_clicked() {
    qDebug() << "Start button clicked";
    if(!thread_finished) {
        ui->statusbar->showMessage(tr("Trying stopping."));
        thread_quick_exit = true;
        if(t.joinable()) {
            t.join();
        }
        thread_quick_exit = false;
        ui->startButton->setText(tr("Start extract"));
        ui->statusbar->showMessage(tr("Stopped."));
        thread_finished = true;
        return;
    }
    ui->startButton->setText(tr("Stop"));
    ui->progressBar->setValue(0);
    if(infiles.size() == 0) {
        ui->statusbar->showMessage(tr("No archive to extract."));
        return;
    }
    ui->progressBar->setRange(0, infiles.size());

    if(t.joinable()) {
        t.join();
    }
    thread_finished = false;
    t = std::thread([this]() {

        QList<QFileInfo> newinfiles;
        int succeed = 0;
        int failed = 0;
        int pswerr = 0;
        int skiped = 0;
        int row = 0;
        for(auto &infile : infiles) {
            if(infile.state == infile.WAITING) {
                QFileInfo infileinfo = infile.info;
                QString infileabsfilepath = infileinfo.absoluteFilePath();
                QString infilename = infileinfo.fileName();

                ui->statusbar->showMessage(tr("Handling archive %1.").arg(infileinfo.fileName()));
                emit changeInfileState(row, infiles.waiting_icon, "");

                QDir outdir(defoutdir);
                if(ui->mkdirCheckBox->isChecked()) {
                    outdir.mkdir(infilename);
                    outdir.cd(infilename);
                }

                qDebug() << "archive: " << infileabsfilepath;
                qDebug() << "outdir: " << outdir.path();

                if(QString ext_name; head_pattern.detect(infileabsfilepath, ext_name)) {
                    bool extr_found = false;
                    for(auto &extr : extrs) {
                        if(!extr.canExtract(ext_name)) {
                            continue;
                        }
                        extr_found = true;
                        QSet<QString> namepsws(passwords);
                        for(int idx = 0;; idx++) {
                            if((idx = infileabsfilepath.indexOf(filename_psw_pattern, idx)) == -1) {
                                break;
                            }
                            namepsws << filename_psw_pattern.cap(1);
                        }
                        qDebug() << "psws: " << namepsws;
                        QString errmsg;
                        Extracter::ErrorCode code = extr.extract(infileabsfilepath,
                                                                 outdir.path(),
                                                                 namepsws,
                                                                 thread_quick_exit,
                                                                 errmsg);
                        if(errmsg.size() > 0 || code != Extracter::SUCCEED) {
                            qDebug() << "Found error: " << errmsg;
                            qDebug() << "Error code: " << code;
                            ui->statusbar->showMessage(tr("Found error: %1.").arg(errmsg));
                        }
                        if(thread_quick_exit) {
                            // 响应外部终止请求
                            failed += 1;
                            infile.state = infile.UNKNOWNERR;
                            emit changeInfileState(row, infiles.deferr_icon, errmsg);
                            qDebug() << "Quick exit";
                            return;
                        }
                        switch(code) {
                        case Extracter::PSW_ERROR:
                            pswerr += 1;
                            infile.state = infile.PSWERR;
                            emit changeInfileState(row, infiles.pswerr_icon, errmsg);
                            break;
                        case Extracter::ARCH_ERROR:
                        case Extracter::PGM_ERROR:
                        case Extracter::EXITCODE_INCORRECT:
                            failed += 1;
                            infile.state = infile.UNKNOWNERR;
                            emit changeInfileState(row, infiles.deferr_icon, errmsg);
                            break;
                        case Extracter::SUCCEED:
                        default:
                            // 未知错误均为通过
                            succeed += 1;
                            infile.state = infile.FINISHED;
                            emit changeInfileState(row, infiles.ok_icon, "");
                            break;
                        }
                        if(code == Extracter::SUCCEED) {
                            // TODO: 深入检查内部压缩包
                            break;
                        }
                        // 切换另一解压器
                    }
                    if(!extr_found) {
                        // 找不到解压器
                        qDebug() << "Can not find an extracter.";
                        failed += 1;
                        infile.state = infile.UNKNOWNERR;
                        emit changeInfileState(row, infiles.deferr_icon,
                                               tr("Can not find an extracter. "
                                                  "Please check the config file extractcfg.json."));
                    }
                } else {
                    // 未知压缩包类型
                    qDebug() << "Unknown archive type";
                    failed += 1;
                    infile.state = infile.UNKNOWNERR;
                    emit changeInfileState(row, infiles.deferr_icon, tr("Unknown archive type."));
                }
            } else {
                // 已经处理过的压缩包
                qDebug() << "Skiped";
                skiped += 1;
            }
            row += 1;
            emit workProgress(row);
        }
        infiles.append(newinfiles);
        emit message(tr("Succeed: %1, failed: %2, password incorrect: %3, skiped: %4. Found new archive: %5.")
                     .arg(succeed).arg(failed).arg(pswerr).arg(skiped).arg(newinfiles.size()), 0);
        emit workFinished(true);
        thread_finished = true;
    });
}

void MainWindow::on_clearButton_clicked() {
    qDebug() << "Clear button";
    infiles.clear();
}

void MainWindow::on_actionAbout_triggered() {
    qDebug() << "About action";
    QMessageBox::information(this, tr("About"), tr("ABOUT_CONTENT"));
}

void MainWindow::on_actionHelp_triggered() {
    qDebug() << "Help action";
    QMessageBox::information(this, tr("Help"), tr("HELP_CONTENT"));
}

void MainWindow::on_actionLoadQm_triggered() {
    qDebug() << "Load .qm action";
    QFileDialog dialog(this, tr("Please choose a qm language file."), ".", "File(*.qm)");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::ViewMode::Detail);
    if(!dialog.exec()) {
        return;
    }
    QString file = dialog.selectedFiles().first();
    set_qmfile(file);
}

void MainWindow::on_actionLoadCfg_triggered() {
    qDebug() << "Load config action";
    ui->statusbar->showMessage(tr("Please choose an extractcfg.json config file."));
    QFileDialog dialog(this, tr("Please choose an extractcfg.json config file."), ".", "File(*.json)");
    dialog.setFileMode(QFileDialog::ExistingFile);
    if(!dialog.exec()) {
        return;
    }
    QString filepath = dialog.selectedFiles().first();
    passwords.clear();
    ui->pswList->clear();
    variables.clear();
    extract_pgmname.clear();
    filename_psw_pattern = QRegExp("\\{([^}]+)\\}");
    extrs.clear();
    head_pattern.clear();
    import_config(filepath);
    ui->statusbar->showMessage(tr("Ready."));
}

void MainWindow::on_actionSaveCfg_triggered() {
    qDebug() << "Save config action";
    saveConfig();
}

void MainWindow::on_actionExit_triggered() {
    qDebug() << "Exit action";
    saveConfig();
}

void MainWindow::on_pswEdit_returnPressed() {
    qDebug() << "Input password action";
    QString psw = ui->pswEdit->text();
    if(!passwords.contains(psw)) {
        passwords << psw;
        ui->pswList->addItem(psw);
        ui->pswEdit->clear();
    }
}

void MainWindow::on_pswDelButton_clicked() {
    qDebug() << "Delete password aciton";
    for(auto item : ui->pswList->selectedItems()) {
        passwords.remove(item->text());
        ui->pswList->removeItemWidget(item);
        delete item;
    }
}

void MainWindow::on_archDelButton_clicked() {
    qDebug() << "Delete archive action(not the real disk file)";
    for(auto item : ui->infilesList->selectedItems()) {
        infiles.removeOne(item->text());
        ui->infilesList->removeItemWidget(item);
        delete item;
    }
}

void MainWindow::on_resetErrButton_clicked() {
    int row = 0;
    for(auto &i : infiles) {
        if(i.state != InputFile::FINISHED) {
            i.state = InputFile::WAITING;
            ui->infilesList->item(row)->setIcon(infiles.ready_icon);
        }
        row += 1;
    }
}

void MainWindow::on_clrDbgoutButton_clicked() {
    ui->dbgoutEdit->clear();
}

void MainWindow::on_actionDbgout_triggered() {
    qDebug() << "Triggle debug output textedit";
    ui->line->setVisible(!ui->line->isVisible());
    ui->dbgGroupBox->setVisible(!ui->dbgGroupBox->isVisible());
}

void MainWindow::saveConfig() {
    qDebug() << "Save config";
    export_config("./extractcfg.json");
}

void MainWindow::setInfileState(int row, QIcon icon, QString tooltip) {
    ui->infilesList->item(row)->setIcon(icon);
    ui->infilesList->item(row)->setToolTip(tooltip);
}

void MainWindow::setWorkFinished() {
    ui->startButton->setText(tr("Start extract"));
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QTranslator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , outdir(".") {
    ui->setupUi(this);
    qDebug() << "outdir: " << outdir.absoluteFilePath();
    ui->outdirEdit->setText(outdir.absoluteFilePath());
    ui->statusbar->showMessage(tr("MSG_READY"));

    auto t = new QTranslator();
    if(!t->load("./lang_zh-CN.qm")) {
        QMessageBox::information(nullptr, "警告", "找不到语言文件");
    }
    qApp->installTranslator(t);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    qDebug() << "DragEnter";
    auto mimeData = e->mimeData();
    if(mimeData->hasUrls()) {
        for(auto url : mimeData->urls()) {
            if(url.isLocalFile()) {
                e->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent *e) {
    qDebug() << "dropEvent";
    if(e->mimeData()->hasUrls()) {
        auto urls = e->mimeData()->urls();
        for(auto url : urls) {
            if(url.isLocalFile()) {
                QFileInfo path(url.toLocalFile());
                if(path.isDir()) {
                    qDebug() << "dir: " << path.filePath();
                    outdir = path;
                } else {
                    qDebug() << "file: " << path.filePath();
                    if(!infiles.contains(path.filePath())) {
                        infiles.push_back(path.filePath());
                        ui->infilesList->addItem(path.filePath());
                    }
                }
            }
        }
        ui->outdirEdit->setText(outdir.absoluteFilePath());
    }
}


void MainWindow::on_startButton_clicked() {
    if(infiles.size() == 0) {
        ui->statusbar->showMessage(tr("MSG_NOINFILES"));
        return;
    }
    qDebug() << "start";
    QFileInfo infile(infiles.first());

    qDebug() << "infile: " << infile.absoluteFilePath();
    qDebug() << "outdir: " << outdir.absoluteFilePath();

    QDir fileoutdir(outdir.filePath());
    fileoutdir.mkdir(infile.fileName());
    fileoutdir.cd(infile.fileName());
    // JlCompress::extractDir(infile.absoluteFilePath(), fileoutdir.path());
}

void MainWindow::on_clearButton_clicked() {
    infiles.clear();
    ui->infilesList->clear();
}

void MainWindow::on_actionABOUT_triggered() {
    qDebug() << "About";
    QMessageBox::information(nullptr, tr("ABOUT_TITLE"), tr("ABOUT_CONTENT"));
}

void MainWindow::on_actionHelp_triggered() {
    qDebug() << "Help";
    QMessageBox::information(nullptr, tr("HELP_TITLE"), tr("HELP_CONTENT"));
}

void MainWindow::on_infilesList_itemDoubleClicked(QListWidgetItem *item) {
    qDebug() << "itemDoubleClicked";
}
